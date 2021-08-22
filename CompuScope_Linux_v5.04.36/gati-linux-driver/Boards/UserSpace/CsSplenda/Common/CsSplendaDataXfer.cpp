//  CsSplendaDataXfer.cpp
///	Implementation of CsSplendaDev class
//
//  This file contains all functions related to BusMaster/Slave Data transfer
//


#include "StdAfx.h"
#include "CsSplenda.h"
#include "CsPlxBase.h"

#ifdef _WINDOWS_
#include <process.h>			// for _beginthreadex
#endif

int32	CsSplendaDev::ProcessDataTransfer(in_DATA_TRANSFER *InDataXfer)
{
	int32						i32Status = CS_SUCCESS;
	PIN_PARAMS_TRANSFERDATA		pInParams;
	POUT_PARAMS_TRANSFERDATA	pOutParams;


	pInParams	= &InDataXfer->InParams;
	pOutParams	= InDataXfer->OutParams;

	if ( pInParams->u32Segment > m_MasterIndependent->m_pCardState->u32ActualSegmentCount )
	{
		InterlockedCompareExchange(&m_DataTransfer.BusyTransferCount, 0, 1 );
		return CS_CHANNEL_PROTECT_FAULT;
	}

	uInt16	u16Channel = NormalizedChannelIndex( pInParams->u16Channel );
	m_pSystem->u16ActiveChannelTx	= pInParams->u16Channel;

	// Position of the last valid sample
	int64	i64LimitEnd;
	int64	i64LimitStart;
	int64	i64AdjustStartAddr;

	i64LimitEnd = m_pCardState->AcqConfig.i64Depth + m_pCardState->AcqConfig.i64TriggerDelay;

	// pInParams->i64Length.LowPart is the number of bytes
	// Convert from bytes to number of samples
	pInParams->i64Length  /= m_pCardState->AcqConfig.u32SampleSize;
	pOutParams->i64ActualLength = pInParams->i64Length;

	// Validate start Address
	i64LimitStart = m_pCardState->AcqConfig.i64TriggerDelay + m_pCardState->AcqConfig.i64Depth - m_pCardState->AcqConfig.i64SegmentSize;

	if ( pInParams->i64StartAddress < i64LimitStart )
		i64AdjustStartAddr = i64LimitStart;
	else
		i64AdjustStartAddr = pInParams->i64StartAddress;

	// Increase busy transfer counter
	InterlockedIncrement(&m_pSystem->BusyTransferCount);

	//----- Prepare for transfer
	in_PREPARE_DATATRANSFER		InXferEx = {0};
	out_PREPARE_DATATRANSFER	OutXferEx = {0};

	InXferEx.u32Segment		= pInParams->u32Segment;
	InXferEx.u16Channel		= u16Channel;
	InXferEx.i64StartAddr	= i64AdjustStartAddr;
	InXferEx.i32OffsetAdj	= GetTriggerAddressOffsetAdjust(  m_pCardState->AcqConfig.u32Mode, (uInt32) m_pCardState->AcqConfig.i64SampleRate );
	InXferEx.u32SampleSize  = m_pCardState->AcqConfig.u32SampleSize;

	InXferEx.u32DataMask	= GetDataMaskModeTransfer( pInParams->u32Mode );
	InXferEx.u32FifoMode	= GetFifoModeTransfer(u16Channel);
	InXferEx.bBusMasterDma	= ! m_DataTransfer.bUseSlaveTransfer;
	InXferEx.bXpertFIR		= (BOOLEAN) m_FirConfig.bEnable;
	InXferEx.bIntrEnable	= TRUE;

	i32Status = IoctlPreDataTransfer( &InXferEx, &OutXferEx );

	if ( CS_SUCCEEDED(i32Status) ) 
	{
		if ( CS_FAILED( OutXferEx.i32Status ) )
		{
			InterlockedCompareExchange(&m_DataTransfer.BusyTransferCount, 0, 1 );
			InterlockedDecrement(&m_pSystem->BusyTransferCount);
			return OutXferEx.i32Status;
		}

		pOutParams->i32HighPart		= OutXferEx.u32LocalAlignOffset;
		m_MemEtb					= OutXferEx.m_MemEtb;

		pOutParams->i64ActualStart	= OutXferEx.i64ActualStartAddr + OutXferEx.i32ActualStartOffset;

		// Adjust actual length to recover the lost of alignment
		if ( pOutParams->i64ActualStart < pInParams->i64StartAddress )
			pOutParams->i64ActualLength += (pInParams->i64StartAddress - pOutParams->i64ActualStart);

		// After all adjustments, limit the maximum transfer size to the size reuested .
		if( pOutParams->i64ActualLength > pInParams->i64Length )
			pOutParams->i64ActualLength = pInParams->i64Length;

		if ( pOutParams->i64ActualStart + pOutParams->i64ActualLength > i64LimitEnd )
			pOutParams->i64ActualLength = i64LimitEnd - pOutParams->i64ActualStart;

		pOutParams->i32LowPart = 0;

		if ( m_bNucleon )
		{
			// Nucleon PCI-E can only transfer multiple of 4 bytes
			if ( (2 == m_pCardState->AcqConfig.u32SampleSize) && (0 != pOutParams->i64ActualLength % 2) )
				pOutParams->i64ActualLength -= 1;
		}

		if ( 0 !=  pInParams->pDataBuffer )
		{
			if (0 != pInParams->hNotifyEvent)
			{
				// Asynchronous transfer
				m_DataTransfer.hUserAsynEvent = *((EVENT_HANDLE *)pInParams->hNotifyEvent);
				i32Status = TransferDataFromOnBoardMemory(pInParams->pDataBuffer, pOutParams->i64ActualLength * m_pCardState->AcqConfig.u32SampleSize, m_DataTransfer.hAsynEvent );
			}
			else
			{
				// Synchronous transfer
				uInt32	u32TransferCount = 0;
				m_DataTransfer.hUserAsynEvent = NULL;
				i32Status = TransferDataFromOnBoardMemory(pInParams->pDataBuffer, pOutParams->i64ActualLength * m_pCardState->AcqConfig.u32SampleSize, 0, &u32TransferCount );

				pOutParams->i32HighPart	= (int32)u32TransferCount;
			}
		}
	}

	if ( CS_FAILED(i32Status) || 0 == pInParams->pDataBuffer || 0 == pInParams->hNotifyEvent )
	{
		// Decrease busy counter for data transfer 
		// If it is syncrhonous data transfer, the transfer should have been completed at this time.
		// Otherwise the transfer completed only when we receive the event AsynDataTransferDone.
		InterlockedCompareExchange(&m_DataTransfer.BusyTransferCount, 0, 1 );
		InterlockedDecrement(&m_pSystem->BusyTransferCount);
	}

	return i32Status;
}

int32	CsSplendaDev::ProcessDataTransferDebug(in_DATA_TRANSFER *InDataXfer)
{
	int32						i32Status = CS_SUCCESS;
	PIN_PARAMS_TRANSFERDATA		pInParams;
	POUT_PARAMS_TRANSFERDATA	pOutParams;


	pInParams	= &InDataXfer->InParams;
	pOutParams	= InDataXfer->OutParams;

	if ( pInParams->u32Segment > m_MasterIndependent->m_pCardState->u32ActualSegmentCount )
	{
		InterlockedCompareExchange(&m_DataTransfer.BusyTransferCount, 0, 1 );
		return CS_CHANNEL_PROTECT_FAULT;
	}

	m_pSystem->u16ActiveChannelTx	= pInParams->u16Channel;

	// Position of the last valid sample
	int64	i64LimitEnd;
	int64	i64LimitStart;
	int64	i64AdjustStartAddr;

	i64LimitEnd = m_pCardState->AcqConfig.i64Depth + m_pCardState->AcqConfig.i64TriggerDelay;

	// pInParams->i64Length.LowPart is the number of bytes
	// Convert from bytes to number of samples
	pInParams->i64Length  /= m_pCardState->AcqConfig.u32SampleSize;
	pOutParams->i64ActualLength = pInParams->i64Length;

	// Validate start Address
	i64LimitStart = m_pCardState->AcqConfig.i64TriggerDelay + m_pCardState->AcqConfig.i64Depth - m_pCardState->AcqConfig.i64SegmentSize;

	if ( pInParams->i64StartAddress < i64LimitStart )
		i64AdjustStartAddr = i64LimitStart;
	else
		i64AdjustStartAddr = pInParams->i64StartAddress;

	// Increase busy transfer counter
	InterlockedIncrement(&m_pSystem->BusyTransferCount);

	// Synchronous transfer
	uInt32	u32TransferCount = 0;
	m_DataTransfer.hUserAsynEvent = NULL;
	pOutParams->i64ActualLength = pInParams->i64Length;

	i32Status = TransferDataFromOnBoardMemory(pInParams->pDataBuffer, pOutParams->i64ActualLength * m_pCardState->AcqConfig.u32SampleSize, 0, &u32TransferCount );

	pOutParams->i32HighPart	= (int32)u32TransferCount;


	if ( CS_FAILED(i32Status) || 0 == pInParams->pDataBuffer || 0 == pInParams->hNotifyEvent || pInParams->u32Mode != TxMODE_DEBUG_DISP)
	{
		// Decrease busy counter for data transfer 
		// If it is syncrhonous data transfer, the transfer should have been completed at this time.
		// Otherwise the transfer completed only when we receive the event AsynDataTransferDone.
		InterlockedCompareExchange(&m_DataTransfer.BusyTransferCount, 0, 1 );
		InterlockedDecrement(&m_pSystem->BusyTransferCount);
	}

	return i32Status;
}

////////////////////////////////////////////////////////////////////////
//
int32	CsSplendaDev::ProcessDataTransferModeAveraging(in_DATA_TRANSFER *InDataXfer)
{
	PIN_PARAMS_TRANSFERDATA		pInParams;
	POUT_PARAMS_TRANSFERDATA	pOutParams;
	int64						i64PreDepth;
	int32						i32Status = CS_SUCCESS;

	pInParams	= &InDataXfer->InParams;
	pOutParams	= InDataXfer->OutParams;

	uInt16	u16Channel = NormalizedChannelIndex( pInParams->u16Channel );

	// Only 32 bit Data transfer accepted
	if ( 0 == (pInParams->u32Mode & TxMODE_DATA_32) )
		return CS_INVALID_TRANSFER_MODE;

	// Convert from bytes to number of 32 bit samples
	pInParams->i64Length /= sizeof(uInt32);

	// Position of the last valid sample
	int64	i64LimitEnd;
	uInt32	u32TailCutOffSamples = 0;
	uInt32	u32HeadCutOffSamples = 0;

	if ( !m_bSoftwareAveraging )
	{
		u32HeadCutOffSamples = 0;
		u32TailCutOffSamples = 0;
	}

	i64LimitEnd = m_pCardState->AcqConfig.i64Depth  + m_pCardState->AcqConfig.i64TriggerDelay;

	if ( i64LimitEnd <= pInParams->i64StartAddress )
		return CS_INVALID_START;

	// Make sure that we do averaging only valid portion of data of pretrigger
	if ( (m_pCardState->AcqConfig.i64SegmentSize - m_pCardState->AcqConfig.i64Depth) >= m_pCardState->AcqConfig.i64TriggerHoldoff )
		i64PreDepth= m_pCardState->AcqConfig.i64TriggerHoldoff;
	else
		i64PreDepth = m_pCardState->AcqConfig.i64SegmentSize - m_pCardState->AcqConfig.i64Depth;

	// Validate start Address
	if ( pInParams->i64StartAddress < m_pCardState->AcqConfig.i64TriggerDelay + m_pCardState->AcqConfig.i64Depth - m_pCardState->AcqConfig.i64SegmentSize )
		pOutParams->i64ActualStart = m_pCardState->AcqConfig.i64TriggerDelay + m_pCardState->AcqConfig.i64Depth - m_pCardState->AcqConfig.i64SegmentSize;
	else
		pOutParams->i64ActualStart = pInParams->i64StartAddress;

	//----- iBuffer->i64Length.LowPart is the number of DWORD
	pOutParams->i64ActualLength = pInParams->i64Length;
	pOutParams->i32LowPart = 0;

	if ( pOutParams->i64ActualStart  < 0 )
	{
		//----- Application requested for pretrigger data
		//----- Adjust ActualStart address acordingly to pre trigger data
		if ( i64PreDepth <  ( Abs(pOutParams->i64ActualStart) ) )
		{
			//----- There is less pre trigger data than requested. Re-adjust the start position
			pOutParams->i64ActualStart =  -1*i64PreDepth;
		}
	}

	//----- Calculate the reading offset
	int64	i64StartAddress;
	uInt32	u32AlignOffset = 0;

	i64StartAddress  = pOutParams->i64ActualStart;

	if( m_bSoftwareAveraging )
		i64StartAddress += GetTriggerAddressOffsetAdjust( m_pCardState->AcqConfig.u32Mode, (uInt32) m_pCardState->AcqConfig.i64SampleRate );

	pOutParams->i32HighPart = u32AlignOffset;
	pOutParams->i64ActualStart -= u32AlignOffset;

	// After all adjustments, limit the maximum transfer size to the size reuested .
	if( pOutParams->i64ActualLength > pInParams->i64Length )
		pOutParams->i64ActualLength = pInParams->i64Length;

	if ( pOutParams->i64ActualStart + pOutParams->i64ActualLength > i64LimitEnd )
		pOutParams->i64ActualLength = i64LimitEnd - pOutParams->i64ActualStart;

	// Increase busy transfer counter
	InterlockedIncrement(&m_pSystem->BusyTransferCount);

	if ( m_bSoftwareAveraging )
	{
		int32	*pi32Buffer = (int32 *)	pInParams->pDataBuffer;

		i32Status = ProcessSoftwareAveraging( i64StartAddress, u16Channel, pi32Buffer, (uInt32) pOutParams->i64ActualLength );

		// Decrease busy counter for data transfer
		InterlockedCompareExchange(&m_DataTransfer.BusyTransferCount, 0, 1 );
		InterlockedDecrement(&m_pSystem->BusyTransferCount);

		return i32Status;
	}

	return i32Status;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSplendaDev::ProcessSoftwareAveraging( int64 i64StartAddress, uInt16 u16Channel, int32 *pi32ResultBuffer, uInt32 u32SamplesInBuffer )
{
	int32	i32Status = CS_SUCCESS;


	if ( u32SamplesInBuffer > MAX_SW_AVERAGING_SEMGENTSIZE )
		u32SamplesInBuffer = MAX_SW_AVERAGING_SEMGENTSIZE;

	//----- Calculate the reading offset
	uInt32				u32BufferLength = (u32SamplesInBuffer + SW_AVERAGING_BUFFER_PADDING) * sizeof(int16);
	int32				i32OffsetActualStart = 0;
	in_PREPARE_DATATRANSFER		InXferEx = {0};
	out_PREPARE_DATATRANSFER	OutXferEx = {0};


	for ( uInt32 u32Segment = 1; u32Segment <= m_pCardState->AcqConfig.u32SegmentCount; u32Segment ++ )
	{
		InXferEx.u32Segment		= u32Segment;
		InXferEx.u16Channel		= u16Channel;
		InXferEx.i64StartAddr	= i64StartAddress;
		InXferEx.i32OffsetAdj	= GetTriggerAddressOffsetAdjust( m_pCardState->AcqConfig.u32Mode, (uInt32) m_pCardState->AcqConfig.i64SampleRate );

		InXferEx.u32DataMask	= 0;
		InXferEx.u32FifoMode	= GetFifoModeTransfer(u16Channel);
		InXferEx.bBusMasterDma	= ! m_DataTransfer.bUseSlaveTransfer;
		InXferEx.u32SampleSize  = sizeof(uInt16);

		i32Status = IoctlPreDataTransfer( &InXferEx, &OutXferEx );
		if ( CS_FAILED(i32Status) )
			break;

		i32OffsetActualStart =  -1*OutXferEx.i32ActualStartOffset;

		i32Status = CardReadMemory( m_pAverageBuffer, u32BufferLength, NULL );
		if ( CS_FAILED(i32Status) )
			break;

		// Put the result of summation in the output buffer
		if ( 1 == u32Segment )
		{
			for ( uInt32 i = 0; i < u32SamplesInBuffer; i ++ )
			{
				pi32ResultBuffer[i] = m_pAverageBuffer[i+i32OffsetActualStart];
			}
		}
		else
		{
			for ( uInt32 i = 0; i < u32SamplesInBuffer; i ++ )
			{
				pi32ResultBuffer[i] += m_pAverageBuffer[i+i32OffsetActualStart];
			}
		}
	}

	return i32Status;
}


#ifdef _WINDOWS_

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsSplendaDev::CreateThreadAsynTransfer()
{
	m_DataTransfer.hAsynThread = (HANDLE)::_beginthreadex(NULL, 0, ThreadFuncTxfer, this, 0, (unsigned int *)&m_DataTransfer.u32AsynThreadId);
	if ( ! m_DataTransfer.hAsynThread )
	{
		CloseHandle ( m_DataTransfer.hAsynEvent );
		m_DataTransfer.hAsynEvent = 0;
		return CS_INSUFFICIENT_RESOURCES;
	}

	::SetThreadPriority(m_DataTransfer.hAsynThread, THREAD_PRIORITY_HIGHEST);

	return CS_SUCCESS;
}

#endif


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsSplendaDev::AsynDataTransferDone()
{
	CsSplendaDev *pDevice = GetCardPointerFromChannelIndex( m_pSystem->u16ActiveChannelTx );
	
	// Decrease busy counter for data transfer
	InterlockedCompareExchange(&pDevice->m_DataTransfer.BusyTransferCount, 0, 1 );
	InterlockedDecrement(&m_pSystem->BusyTransferCount);
}
