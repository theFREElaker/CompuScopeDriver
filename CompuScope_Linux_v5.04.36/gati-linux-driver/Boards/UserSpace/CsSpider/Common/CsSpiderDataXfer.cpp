//  CsSpiderDataXfer.cpp
///	Implementation of CsSpiderDev class
//
//  This file contains all functions related to BusMaster/Slave Data transfer
//

#include "StdAfx.h"
#include "CsSpider.h"
#include "CsPlxBase.h"

#ifdef _WINDOWS_
	#include <process.h>			// for _beginthreadex
#endif

#define nSHOW_FW_PERFORMANCE

int32	CsSpiderDev::ProcessDataTransfer(in_DATA_TRANSFER *InDataXfer)
{
	static uInt32	u32DebugCount = 0;
	int32						i32Status = CS_SUCCESS;
	PIN_PARAMS_TRANSFERDATA		pInParams;
	POUT_PARAMS_TRANSFERDATA	pOutParams;

	pInParams	= &InDataXfer->InParams;
	pOutParams	= InDataXfer->OutParams;

	uInt16	u16Channel = NormalizedChannelIndex( pInParams->u16Channel );
	m_pSystem->u16ActiveChannelTx	= pInParams->u16Channel;

	// Position of the last valid sample
	int64	i64LimitEnd;
	i64LimitEnd = m_pCardState->AcqConfig.i64Depth + m_pCardState->AcqConfig.i64TriggerDelay;
	if (0 != (pInParams->u32Mode & TxMODE_DATA_INTERLEAVED))
	{
		i64LimitEnd *= 8;
	//	i64LimitEnd *= (m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE);
	}


	// pInParams->i64Length.LowPart is the number of bytes
	// Convert from bytes to number of samples
	pInParams->i64Length  /= m_pCardState->AcqConfig.u32SampleSize;
	pOutParams->i64ActualLength = pInParams->i64Length;

	// Validate start Address
	int64	i64LimitStart;
	int64	i64AdjustStartAddr;
	i64LimitStart = m_pCardState->AcqConfig.i64TriggerDelay + m_pCardState->AcqConfig.i64Depth - m_pCardState->AcqConfig.i64SegmentSize;

	if ( pInParams->i64StartAddress < i64LimitStart )
		i64AdjustStartAddr = i64LimitStart;
	else
		i64AdjustStartAddr = pInParams->i64StartAddress;

	// Increase busy transfer counter
	InterlockedIncrement(&m_pSystem->BusyTransferCount);

	//----- Calculate the reading offset
	in_PREPARE_DATATRANSFER		InXferEx = {0};
	out_PREPARE_DATATRANSFER	OutXferEx = {0};

	InXferEx.u32Segment		= pInParams->u32Segment;
	InXferEx.u16Channel		= u16Channel;
	InXferEx.i64StartAddr	= i64AdjustStartAddr;
	InXferEx.i32OffsetAdj	= GetTriggerAddressOffsetAdjust(  m_pCardState->AcqConfig.u32Mode, (uInt32) m_pCardState->AcqConfig.i64SampleRate );
	InXferEx.u32SampleSize  = m_pCardState->AcqConfig.u32SampleSize;

	InXferEx.u32DataMask	= 	GetDataMaskModeTransfer ( pInParams->u32Mode );
	InXferEx.u32FifoMode	= GetFifoModeTransfer(u16Channel, 0 != (pInParams->u32Mode & TxMODE_DATA_INTERLEAVED));
	InXferEx.bBusMasterDma	= ! m_DataTransfer.bUseSlaveTransfer;
	InXferEx.bXpertHwAVG	= m_bHardwareAveraging;
	InXferEx.bIntrEnable = TRUE;

#ifdef SHOW_FW_PERFORMANCE
	//WritePciRegister(0xC, 0x1);		// reset performance counter
	//WritePciRegister(0xC, 0x2);		// start performance counter
	WritePciRegister(0x10, m_pCardState->AcqConfig.u32SampleSize*(uInt32) pInParams->i64Length);		// stop performance counter
#endif
	
	i32Status = IoctlPreDataTransfer( &InXferEx, &OutXferEx );

	if ( CS_SUCCEEDED(i32Status) ) 
	{
		pOutParams->i32HighPart		= OutXferEx.u32LocalAlignOffset;
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
			u32DebugCount ++;
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
		else 
			u32DebugCount = 0;
	}

	if ( CS_FAILED(i32Status) || 0 == pInParams->pDataBuffer || 0 == pInParams->hNotifyEvent )
	{
		// Decrease busy counter for data transfer 
		// If it is syncrhonous data transfer, the transfer should have been completed at this time.
		// Otherwise the transfer complted only when we receive the event AsynDataTransferDone.
		InterlockedCompareExchange(&m_DataTransfer.BusyTransferCount, 0, 1 );
		InterlockedDecrement(&m_pSystem->BusyTransferCount);
	}

#ifdef SHOW_FW_PERFORMANCE
	//WritePciRegister(0xC, 0);		// stop performance counter

	uInt32 Time = ReadPciRegister( 0x04 );

	double	dPerformace = (uInt32) pInParams->i64Length;
	dPerformace = dPerformace*m_pCardState->AcqConfig.u32SampleSize*250/Time;

	char szText[256];

	sprintf_s(szText, sizeof(szText), "Performance PCIE transfer (Fw)= %.3f MB/sec\n", dPerformace);
	::OutputDebugString(szText);
#endif


	return i32Status;
}


////////////////////////////////////////////////////////////////////////
//
int32	CsSpiderDev::ProcessDataTransferModeAveraging(in_DATA_TRANSFER *InDataXfer)
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

int32	CsSpiderDev::ProcessSoftwareAveraging( int64 i64StartAddress, uInt16 u16Channel, int32 *pi32ResultBuffer, uInt32 u32SamplesInBuffer )
{
	int32				i32DrvStatus = CS_SUCCESS;


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

		InXferEx.u32DataMask	= GetDataMaskModeTransfer ( TxMODE_DATA_32 );
		InXferEx.u32FifoMode	= GetFifoModeTransfer(u16Channel);
		InXferEx.bBusMasterDma	= ! m_DataTransfer.bUseSlaveTransfer;

		i32DrvStatus = IoctlPreDataTransfer( &InXferEx, &OutXferEx );
		if ( CS_SUCCESS != i32DrvStatus )
			break;

		i32OffsetActualStart =  -1*OutXferEx.i32ActualStartOffset;

		CardReadMemory( m_pAverageBuffer, u32BufferLength, NULL );

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

	return i32DrvStatus;
}


#ifdef _WINDOWS_
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSpiderDev::CreateThreadAsynTransfer()
{
	m_DataTransfer.hAsynThread = (HANDLE)::_beginthreadex(NULL, 0, ThreadFuncTxfer, this, 0, (unsigned int *)&m_DataTransfer.u32AsynThreadId);
	if ( ! m_DataTransfer.hAsynThread )
	{
		CloseHandle ( m_DataTransfer.hAsynEvent );
		return CS_INSUFFICIENT_RESOURCES;
	}

	::SetThreadPriority(m_DataTransfer.hAsynThread, THREAD_PRIORITY_HIGHEST);

	return CS_SUCCESS;
}

#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsSpiderDev::AsynDataTransferDone()
{
	CsSpiderDev *pDevice = GetCardPointerFromChannelIndex( m_pSystem->u16ActiveChannelTx );

	// Decrease busy counter for data transfer
	InterlockedCompareExchange(&pDevice->m_DataTransfer.BusyTransferCount, 0, 1 );
	InterlockedDecrement(&m_pSystem->BusyTransferCount);
}