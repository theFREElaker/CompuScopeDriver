//  CsDeereDataXfer.cpp
///	Implementation of CsDeereDevice class
//
//  This file contains all functions related to BusMaster/Slave Data transfer
//


#include "StdAfx.h"
#include "CsDeere.h"
#ifdef _WINDOWS_
#include <process.h>			// for _beginthreadex
#endif


int32 CsDeereDevice::ProcessDataTransfer(in_DATA_TRANSFER *InDataXfer)
{
	int32						i32Status = CS_SUCCESS;
	PIN_PARAMS_TRANSFERDATA		pInParams;
	POUT_PARAMS_TRANSFERDATA	pOutParams;

	pInParams	= &InDataXfer->InParams;
	pOutParams	= InDataXfer->OutParams;

	if ( ! m_bMulrecAvgTD )
	{
		if( pInParams->u32Segment > m_MasterIndependent->m_pCardState->u32ActualSegmentCount )
		{
			InterlockedCompareExchange(&m_DataTransfer.BusyTransferCount, 0, 1 );
			return CS_CHANNEL_PROTECT_FAULT;
		}
	}

	m_pSystem->u16ActiveChannelTx	= pInParams->u16Channel;
	uInt16 u16Channel = NormalizedChannelIndex( pInParams->u16Channel );

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

#ifdef DEBUG_MASTERSLAVE

	if ( 1 == m_u16DebugMsPulse )
	{
		RtlCopyMemory(pInParams->pDataBuffer, m_PulseData, Min(PAGE_SIZE, (int32)  pInParams->i64Length) );
		InterlockedCompareExchange(&m_DataTransfer.BusyTransferCount, 0, 1 );
		return CS_SUCCESS;
	}

#endif

#ifdef DEBUG_EXTTRIGCALIB

	if ( 1 == m_u16DebugExtTrigCalib )
	{
		if ( 1 == u16Channel )
			RtlCopyMemory(pInParams->pDataBuffer, m_pu8DebugBuffer1, Min(PAGE_SIZE, (int32) pInParams->i64Length) );
		else
			RtlCopyMemory(pInParams->pDataBuffer, m_pu8DebugBufferEx, Min(PAGE_SIZE, (int32) pInParams->i64Length) );

		InterlockedCompareExchange(&m_DataTransfer.BusyTransferCount, 0, 1 );
		return CS_SUCCESS;
	}

#endif

	// Increase busy transfer counter
	InterlockedIncrement(&m_pSystem->BusyTransferCount);

	//----- Calculate the reading offset
	in_PREPARE_DATATRANSFER	 InXferEx = {0};
	out_PREPARE_DATATRANSFER OutXferEx;

	InXferEx.u32Segment		= pInParams->u32Segment;
	InXferEx.u16Channel		= u16Channel;
	InXferEx.u32TxMode		= pInParams->u32Mode;
	InXferEx.i64StartAddr	= i64AdjustStartAddr;
	InXferEx.i32OffsetAdj	= 0;
	InXferEx.u32SampleSize  = m_pCardState->AcqConfig.u32SampleSize;

	InXferEx.u32DataMask	= GetDataMaskModeTransfer ( pInParams->u32Mode );
	InXferEx.bBusMasterDma	= ! m_DataTransfer.bUseSlaveTransfer;
	InXferEx.u32FifoMode	= GetFifoModeTransfer(u16Channel);
	InXferEx.bXpertFIR		= (BOOLEAN) m_FirConfig.bEnable;
	InXferEx.bXpertHwAVG	= m_bHardwareAveraging;
	InXferEx.bIntrEnable	= TRUE;

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

		// Convert Stb to 1/2GHz unit and return to application
		// Return the information related to sub sample trigger bits to application
		if ( 1 == m_pCardState->AddOnReg.u32Decimation && m_Stb != 0 )
		{
			// Cannot happen
			// Error from firmware !!!!!
			pOutParams->i32LowPart = 0;
		}
		else
		{
			LARGE_INTEGER	StbNormalized;

			StbNormalized.QuadPart = m_SkipEtb;
			StbNormalized.QuadPart = StbNormalized.QuadPart	* CS_SR_2GHZ / m_pCardState->u32AdcClock;

			if ( StbNormalized.QuadPart > 0x7FFFFFFF )
			{
				// Normalized Stb is over 32 bits and cannot be returned properly to application
				ASSERT(FALSE);
				pOutParams->i32LowPart = 0x7FFFFFFF;	// Maximum value can be returned;
			}
			else
				pOutParams->i32LowPart = StbNormalized.LowPart;
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


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::ProcessDataTransferEx(io_DATA_TRANSFER_EX *DataXfer)
{
	int32							i32Status = CS_SUCCESS;
	PIN_PARAMS_TRANSFERDATA_EX		pInParams;
	POUT_PARAMS_TRANSFERDATA_EX		pOutParams;

	pInParams	= &DataXfer->InParams;
	pOutParams	= DataXfer->OutParams;

	bool	bRawTimeStamp = (0!= (pInParams->u32Mode & TxMODE_SEGMENT_TAIL));
	bool	bInterleaved  = (0!= (pInParams->u32Mode & TxMODE_DATA_INTERLEAVED));

	// overwite interleaved mode 
	if ( 0 != ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE ))
		bInterleaved = false;
	else if ( bRawTimeStamp )
		bInterleaved = true;

	uInt16	u16Channel = NormalizedChannelIndex( pInParams->u16Channel );
	m_pSystem->u16ActiveChannelTx = pInParams->u16Channel;

	// pInParams->i64Length.LowPart is the number of bytes
	// Convert from bytes to number of samples
	pInParams->i64Length  /= m_pCardState->AcqConfig.u32SampleSize;

	uInt32	u32ReadingOffset = 0;
	int64	i64DataSize;
	int64	i64ReadOffset;

	i64DataSize =  m_i64HwSegmentSize;

	uInt32	u32SkipReadCount = 0;			// u32SkipReadCount in 2 samples resolution
	if ( bRawTimeStamp )
	{
		// Start reading from the first tail and skip all channel data
		i64ReadOffset = i64DataSize;	
		u32SkipReadCount = (uInt32) (i64DataSize >> 1);
	}
	else
	{
		// Calculate ReadOffset and SkipReadCount depending on values from user input
		i64ReadOffset = pInParams->i64StartAddress + m_i64HwSegmentSize - m_i64HwPostDepth - m_pCardState->AcqConfig.i64TriggerDelay;
		u32SkipReadCount = (uInt32) ((i64DataSize - pInParams->i64Length) >> 1);

		if ( 0 != ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL ))
			u32SkipReadCount += DEERE_BYTESRESERVED_HWMULREC_SQR_TAIL >> 2; 
		else
			u32SkipReadCount += DEERE_BYTESRESERVED_HWMULREC_SQR_TAIL >> 1; 
	}
	
	// Converting bytes offset into memory location
	if ( 0 != ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL ))
	{
		i64ReadOffset += (i64DataSize + DEERE_BYTESRESERVED_HWMULREC_SQR_TAIL/2) * (pInParams->u32StartSegment - 1);
		u32ReadingOffset = (uInt32) (i64ReadOffset >> SHIFT_DUAL);		// 8 samples resolution (memory location)
	}
	else
	{
		i64ReadOffset += (i64DataSize + DEERE_BYTESRESERVED_HWMULREC_SQR_TAIL) * (pInParams->u32StartSegment - 1);
		u32ReadingOffset = (uInt32) (i64ReadOffset >> SHIFT_SINGLE);	// 16 samples resolution (memory location)
	}

	uInt32	u32ReadCount = (uInt32) (pInParams->i64Length >> 1);	// u32ReadCount in 2 samples resolution

	if ( bInterleaved )
	{
		i64ReadOffset <<= 1;
		u32SkipReadCount <<= 1;
		if ( ! bRawTimeStamp )
			u32ReadCount <<= 1;
	}
	
	if ( bRawTimeStamp )
	{
		if ( pInParams->i64Length > DEERE_BYTESRESERVED_HWMULREC_SQR_TAIL )
			pInParams->i64Length = DEERE_BYTESRESERVED_HWMULREC_SQR_TAIL;
		u32SkipReadCount += (DEERE_BYTESRESERVED_HWMULREC_SQR_TAIL - (uInt32)pInParams->i64Length) >> 1; 
	}

	// Increase busy transfer counter
	InterlockedIncrement(&m_pSystem->BusyTransferCount);

	in_PREPARE_DATATRANSFER	 InXferEx = {0};
	out_PREPARE_DATATRANSFER OutXferEx;

	InXferEx.u32Segment		= pInParams->u32StartSegment;
	InXferEx.u16Channel		= u16Channel;
	InXferEx.u32TxMode		= pInParams->u32Mode;
	InXferEx.i64StartAddr	= u32ReadingOffset;
	InXferEx.u32ReadCount	= u32ReadCount;
	InXferEx.u32SkipCount   = u32SkipReadCount;
	InXferEx.u32SampleSize  = m_pCardState->AcqConfig.u32SampleSize;

	InXferEx.u32DataMask	= GetDataMaskModeTransfer ( pInParams->u32Mode );
	InXferEx.bBusMasterDma	= ! m_DataTransfer.bUseSlaveTransfer;
	InXferEx.u32FifoMode	= GetFifoModeTransfer( u16Channel, pInParams->u32Mode );
	InXferEx.bIntrEnable	= TRUE;

	i32Status = IoctlPreDataTransfer( &InXferEx, &OutXferEx );

	if ( CS_SUCCEEDED(i32Status) )
	{
		if (0 != pInParams->hNotifyEvent)
		{
			// Asynchronous transfer
			m_DataTransfer.hUserAsynEvent = *((EVENT_HANDLE *)pInParams->hNotifyEvent);
			i32Status = CardReadMemory(pInParams->pDataBuffer, (uInt32) pInParams->i64BufferLength, m_DataTransfer.hAsynEvent );
		}
		else
		{
			// Synchronous transfer
			m_DataTransfer.hUserAsynEvent = NULL;
			i32Status = CardReadMemory(pInParams->pDataBuffer, (uInt32) pInParams->i64BufferLength);
		}

		if ( CS_SUCCEEDED(i32Status) )
		{
			pOutParams->u32DataFormat1 = 0;
			if ( (0 != (m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL)) && (0 != (pInParams->u32Mode & TxMODE_DATA_INTERLEAVED)) )
				pOutParams->u32DataFormat0 = 0x11221122;
			else 
				pOutParams->u32DataFormat0 = 0x11111111;
		}
	}

	if (CS_FAILED(i32Status) || 0 == pInParams->hNotifyEvent)
	{
		// Decrease busy counter for data transfer
		// If it is syncrhonous data transfer, the transfer should have been completed at this time.
		// Otherwise the transfer complted only when we receive the event AsynDataTransferDone.
		InterlockedCompareExchange(&m_DataTransfer.BusyTransferCount, 0, 1 );
		InterlockedDecrement(&m_pSystem->BusyTransferCount);
	}

	return i32Status;
}

#ifdef _WINDOWS_
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDeereDevice::CreateThreadAsynTransfer()
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

void CsDeereDevice::AsynDataTransferDone()
{
	CsDeereDevice *pDevice = GetCardPointerFromChannelIndex( m_pSystem->u16ActiveChannelTx );

	// Decrease busy counter for for data transfer
	InterlockedCompareExchange(&pDevice->m_DataTransfer.BusyTransferCount, 0, 1 );
	InterlockedDecrement(&m_pSystem->BusyTransferCount);
}


////////////////////////////////////////////////////////////////////////
//
int32	CsDeereDevice::ProcessDataTransferModeAveraging(in_DATA_TRANSFER *InDataXfer)
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

	int32	*pi32Buffer = (int32 *)	pInParams->pDataBuffer;

	// Increase busy transfer counter
	InterlockedIncrement(&m_pSystem->BusyTransferCount);

	i32Status = ProcessSoftwareAveraging( i64StartAddress, u16Channel, pi32Buffer, (uInt32) pOutParams->i64ActualLength );

	// Decrease busy counter for data transfer
	InterlockedCompareExchange(&m_DataTransfer.BusyTransferCount, 0, 1 );
	InterlockedDecrement(&m_pSystem->BusyTransferCount);

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::ProcessSoftwareAveraging( int64 i64StartAddress, uInt16 u16Channel, int32 *pi32ResultBuffer, uInt32 u32SamplesInBuffer )
{
	int32	i32Status = CS_SUCCESS;

	if ( u32SamplesInBuffer > MAX_SW_AVERAGING_SEMGENTSIZE )
		u32SamplesInBuffer = MAX_SW_AVERAGING_SEMGENTSIZE;

	//----- Calculate the reading offset
	uInt32				u32BufferLength = (u32SamplesInBuffer + SW_AVERAGING_BUFFER_PADDING) * sizeof(uInt16);
	int32				i32OffsetActualStart = 0;
	in_PREPARE_DATATRANSFER		InXferEx = {0};
	out_PREPARE_DATATRANSFER	OutXferEx;

	for ( uInt32 u32Segment = 1; u32Segment <= m_pCardState->AcqConfig.u32SegmentCount; u32Segment ++ )
	{
		InXferEx.u32Segment		= u32Segment;
		InXferEx.u16Channel		= u16Channel;
		InXferEx.i64StartAddr	= i64StartAddress;
		InXferEx.u32SampleSize  = m_pCardState->AcqConfig.u32SampleSize;

		InXferEx.u32DataMask = GetDataMaskModeTransfer ( TxMODE_DATA_32 );
		InXferEx.u32FifoMode = GetFifoModeTransfer(u16Channel);
		InXferEx.bBusMasterDma	= ! m_DataTransfer.bUseSlaveTransfer;

		i32Status = IoctlPreDataTransfer( &InXferEx, &OutXferEx );
		if ( CS_FAILED(i32Status) )
			break;

		i32OffsetActualStart =  -1*OutXferEx.i32ActualStartOffset;

		i32Status = CardReadMemory( m_pAverageBuffer, u32BufferLength, NULL );
		if ( CS_FAILED(i32Status) )
			break;

		int16	*pi16AvBuffer  =	(int16	*) m_pAverageBuffer;
		// Converted to 2's complement and put the result of sum in the output buffer
		if ( 1 == u32Segment )
		{
			for ( uInt32 i = 0; i < u32SamplesInBuffer; i ++ )
			{
				pi32ResultBuffer[i] = pi16AvBuffer[i+i32OffsetActualStart];
			}
		}
		else
		{
			for ( uInt32 i = 0; i < u32SamplesInBuffer; i ++ )
			{
				pi32ResultBuffer[i] += pi16AvBuffer[i+i32OffsetActualStart];
			}
		}

	}

	return i32Status;
}

