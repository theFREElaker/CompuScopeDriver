//  CsRabitDataXfer.cpp
///	Implementation of CsRabbitDev class
//
//  This file contains all functions related to BusMaster/Slave Data transfer
//


#include "StdAfx.h"
#include "CsTypes.h"
#include "CsIoctl.h"
#include "CsDrvDefines.h"
#include "CsRabbit.h"
#include "CsPlxBase.h"

#ifdef _WINDOWS_
#include <process.h>			// for _beginthreadex
#endif

int32	CsRabbitDev::ProcessDataTransfer(in_DATA_TRANSFER *InDataXfer)
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
	pInParams->i64Length  /=  m_pCardState->AcqConfig.u32SampleSize;
	pOutParams->i64ActualLength = pInParams->i64Length;

	// Validate start Address
	i64LimitStart = m_pCardState->AcqConfig.i64TriggerDelay + m_pCardState->AcqConfig.i64Depth - m_pCardState->AcqConfig.i64SegmentSize;

	if ( pInParams->i64StartAddress < i64LimitStart )
		i64AdjustStartAddr = i64LimitStart;
	else
		i64AdjustStartAddr = pInParams->i64StartAddress;

#ifdef DEBUG_MASTERSLAVE

	if ( 1 == m_u16DebugMsPulse )
	{
		GageCopyMemoryX(pInParams->pDataBuffer, m_PulseData, Min(PAGE_SIZE, (uInt32) pInParams->i64Length) );
		InterlockedCompareExchange(&m_DataTransfer.BusyTransferCount, 0, 1 );
		return CS_SUCCESS;;
	}

#endif

	// Increase busy transfer counter
	InterlockedIncrement(&m_pSystem->BusyTransferCount);
	int32 i32OneSampleDepthAdjust = 0;		// Adjustment for one sample depth resolution
	if ( m_bOneSampleResolution )
	{
		if ( m_i64HwTriggerDelay > 0  )
			i32OneSampleDepthAdjust = (int32) (i64AdjustStartAddr % m_u32SegmentResolution);
		else
			i32OneSampleDepthAdjust = (int32) (i64AdjustStartAddr % (2*m_u32SegmentResolution ));

		i64AdjustStartAddr -= i32OneSampleDepthAdjust;
	}

	// Prepare for data transfer
	in_PREPARE_DATATRANSFER	 InXferEx = {0};
	out_PREPARE_DATATRANSFER OutXferEx;

	InXferEx.u32Segment		= pInParams->u32Segment;
	InXferEx.u16Channel		= u16Channel;
	InXferEx.i64StartAddr	= i64AdjustStartAddr;
	InXferEx.i32OffsetAdj	= GetTriggerAddressOffsetAdjust( (uInt32) m_pCardState->AcqConfig.i64SampleRate );

	InXferEx.u32DataMask	= GetDataMaskModeTransfer ( pInParams->u32Mode );
	InXferEx.u32FifoMode	= GetFifoModeTransfer(u16Channel);
	InXferEx.bBusMasterDma	= ! m_DataTransfer.bUseSlaveTransfer;
	InXferEx.bXpertHwAVG	= m_bHardwareAveraging;
	InXferEx.bIntrEnable	= TRUE;

	i32Status = IoctlPreDataTransfer( &InXferEx, &OutXferEx );
	
	if ( CS_SUCCEEDED(i32Status) ) 
	{
		pOutParams->i32HighPart		= OutXferEx.u32LocalAlignOffset;
		pOutParams->i64ActualStart	= OutXferEx.i64ActualStartAddr + OutXferEx.i32ActualStartOffset;
		pOutParams->i64ActualStart += i32OneSampleDepthAdjust;

		// Adjust actual length to recover the lost of alignment
		if ( pOutParams->i64ActualStart < pInParams->i64StartAddress )
			pOutParams->i64ActualLength += (pInParams->i64StartAddress - pOutParams->i64ActualStart);

		// After all adjustments, limit the maximum transfer size to the size reuested .
		if( pOutParams->i64ActualLength > pInParams->i64Length )
			pOutParams->i64ActualLength = pInParams->i64Length;

		if ( pOutParams->i64ActualStart + pOutParams->i64ActualLength > i64LimitEnd )
			pOutParams->i64ActualLength = i64LimitEnd - pOutParams->i64ActualStart;

		pOutParams->i32LowPart = 0;

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
				m_DataTransfer.hUserAsynEvent = NULL;
				i32Status = TransferDataFromOnBoardMemory(pInParams->pDataBuffer, pOutParams->i64ActualLength * m_pCardState->AcqConfig.u32SampleSize );
			}
		}
	}

	if ( CS_FAILED(i32Status) || 0 == pInParams->pDataBuffer || 0 == pInParams->hNotifyEvent )
	{
		// Decrease busy counter for data transfer 
		// If it is syncrhonous data transfer, the transfer should have been completed at this time.
		// Otherwise the transfer complted only when we receive the event AsynDataTransferDone.
		InterlockedCompareExchange(&m_DataTransfer.BusyTransferCount, 0, 1 );
		InterlockedDecrement(&m_pSystem->BusyTransferCount);
	}

	return i32Status;
}


////////////////////////////////////////////////////////////////////////
//
int32	CsRabbitDev::ProcessDataTransferModeAveraging(in_DATA_TRANSFER *InDataXfer)
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

int32	CsRabbitDev::ProcessSoftwareAveraging( int64 i64StartAddress, uInt16 u16Channel, int32 *pi32ResultBuffer, uInt32 u32SamplesInBuffer )
{
	int32				i32DrvStatus = CS_SUCCESS;

	if ( u32SamplesInBuffer > MAX_SW_AVERAGING_SEMGENTSIZE )
		u32SamplesInBuffer = MAX_SW_AVERAGING_SEMGENTSIZE;

	//----- Calculate the reading offset
	uInt32				u32BufferLength = (u32SamplesInBuffer + SW_AVERAGING_BUFFER_PADDING) * sizeof(uInt8);
	int32				i32OffsetActualStart = 0;
	in_PREPARE_DATATRANSFER		InXferEx = {0};
	out_PREPARE_DATATRANSFER	OutXferEx;

	for ( uInt32 u32Segment = 1; u32Segment <= m_pCardState->AcqConfig.u32SegmentCount; u32Segment ++ )
	{
		InXferEx.u32Segment		= u32Segment;
		InXferEx.u16Channel		= u16Channel;
		InXferEx.i64StartAddr	= i64StartAddress;
		InXferEx.i32OffsetAdj	= GetTriggerAddressOffsetAdjust( (uInt32) m_pCardState->AcqConfig.i64SampleRate );

		InXferEx.u32DataMask = GetDataMaskModeTransfer ( TxMODE_DATA_32 );
		InXferEx.u32FifoMode = GetFifoModeTransfer(u16Channel);
		InXferEx.bBusMasterDma	= ! m_DataTransfer.bUseSlaveTransfer;

		i32DrvStatus = IoctlPreDataTransfer( &InXferEx, &OutXferEx );
		if ( CS_SUCCESS != i32DrvStatus )
			break;

		i32OffsetActualStart =  -1*OutXferEx.i32ActualStartOffset;

		CardReadMemory( m_pAverageBuffer, u32BufferLength, NULL );

		uInt8	*pu8AvBuffer  =	(uInt8	*) m_pAverageBuffer;
		// Converted to 2's complement and put the result of sum in the output buffer
		if ( 1 == u32Segment )
		{
			for ( uInt32 i = 0; i < u32SamplesInBuffer; i ++ )
			{
				pi32ResultBuffer[i] = (int8)(0xFF & (pu8AvBuffer[i+i32OffsetActualStart] + 0x80));
			}
		}
		else
		{
			for ( uInt32 i = 0; i < u32SamplesInBuffer; i ++ )
			{
				pi32ResultBuffer[i] += (int8)(0xFF & (pu8AvBuffer[i+i32OffsetActualStart] + 0x80));
			}
		}

	}

	return i32DrvStatus;
}


#ifdef _WINDOWS_

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsRabbitDev::CreateThreadAsynTransfer()
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

void CsRabbitDev::AsynDataTransferDone()
{
	CsRabbitDev *pDevice = GetCardPointerFromChannelIndex( m_pSystem->u16ActiveChannelTx );

	// Decrease busy counter for for data transfer
	InterlockedCompareExchange(&pDevice->m_DataTransfer.BusyTransferCount, 0, 1 );
	InterlockedDecrement(&m_pSystem->BusyTransferCount);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::DataTransferModeHistogram(in_DATA_TRANSFER *InDataXfer)
{
	PIN_PARAMS_TRANSFERDATA		pInParams;
	POUT_PARAMS_TRANSFERDATA	pOutParams;
	int32						i32Status = CS_SUCCESS;

	pInParams	= &InDataXfer->InParams;
	pOutParams	= InDataXfer->OutParams;

	uInt32	*pHistogram = (uInt32 *)pInParams->pDataBuffer;

	uInt16	u16Channel = 0;
	
	if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL )
	{
		u16Channel = NormalizedChannelIndex( pInParams->u16Channel );
		u16Channel = ConvertToHwChannel(u16Channel);
	}

	// Check for Histogram full
	BOOL		bHistogramFull = (0 != (0x1 & ReadGIO( HISTO_HIST_STAT_REG)) );
	uInt16		u16Bin = 0;
	uInt32		u32Temp = HIST_READ_ENABLE_BIT;

	while( u16Bin <= 0xFF )
	{
		u32Temp = u16Bin | HIST_READ_ENABLE_BIT | (u16Channel <<8);
		if ( bHistogramFull )
			u32Temp |= ( 1 << 10 );			// Enable devider by 2

		WriteGIO( HISTO_READ_CTRL_REG, u32Temp );

		pHistogram[u16Bin] = ReadGIO( HISTO_HIST_READ_REG );

		u16Bin ++;
	}

	InterlockedCompareExchange(&m_DataTransfer.BusyTransferCount, 0, 1 );

	WriteGIO( HISTO_READ_CTRL_REG, 0 );

	if ( bHistogramFull )
		i32Status = CS_HISTOGRAM_FULL;

	return i32Status;

}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsRabbitDev::HistogramReset()
{
	int32		nCount = 100;
	bool		bResetDone = false;

	WriteGIO( HISTO_HIST_CTRL_REG, (m_u32HistogramMemDepth << 4) | HIST_ACLR_BIT );
	WriteGIO( HISTO_HIST_CTRL_REG, (m_u32HistogramMemDepth << 4) );

	while ( nCount-- > 0 && !bResetDone )
	{
		bResetDone = ( 0 != (0x2 & ReadGIO( HISTO_HIST_STAT_REG)) );
		Sleep(10);
	}
}
