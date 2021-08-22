#include "stdafx.h"
#include "CsSpDevice.h"

int32 CsSpDevice::TransferData(IN_PARAMS_TRANSFERDATA inParams, POUT_PARAMS_TRANSFERDATA pOutParams)
{
	int r(0);
	if( inParams.u32Segment > m_Acq.u32SegmentCount || 0 == inParams.u32Segment )
	{
		if (NULL != inParams.hNotifyEvent)	::SetEvent( inParams.hNotifyEvent );
		return CS_INVALID_SEGMENT;
	}

	if ( inParams.u16Channel >  m_Info.usChanNumber )
	{
		if (NULL != inParams.hNotifyEvent)	::SetEvent( inParams.hNotifyEvent );
		return CS_INVALID_CHANNEL;
	}

	if( TxMODE_TIMESTAMP == inParams.u32Mode )
	{
		if ( NULL != inParams.pDataBuffer  )
		{
			int64* pTimeStamp = (int64*)inParams.pDataBuffer;
			for ( uInt32 i = inParams.u32Segment, j = 0; i <= uInt32(inParams.i64Length); i++, j++ )
			{
				r = m_pAdq->MultiRecordGetRecord( i-1 ); // records start from 0 with the usb device
				pTimeStamp[j] = int64(m_pAdq->GetTrigTime());
				
				if(m_bDisplayTrace) 
				{
					char str[128];
					::_stprintf_s( str, 128,  "m_pAdq->MultiRecordGetRecord(%d)\n", i-1 );
					::OutputDebugString(str);
					::OutputDebugString("m_pAdq->GetTrigTime()\n");
				}
			}
		}
		m_i64TickFrequency =   m_Acq.i64SampleRate * m_Info.usChanNumber;
		pOutParams->i32LowPart = int32(m_i64TickFrequency & 0x00000000FFFFFFFF); 
		pOutParams->i32HighPart = int32((m_i64TickFrequency & 0xFFFFFFFF00000000) >> 32);
	}
	else 
	{
		const int64 ci64StartSample = -m_Acq.i64TriggerHoldoff + m_Acq.i64TriggerDelay;
		const int64 ci64EndSample = ci64StartSample + m_Acq.i64SegmentSize;

		if( inParams.i64StartAddress < ci64StartSample )
			inParams.i64StartAddress = ci64StartSample;

		if( inParams.i64StartAddress > ci64EndSample - 1 )
			inParams.i64StartAddress = ci64EndSample - 1;

		if( inParams.i64Length < 1 )
			inParams.i64Length = 1;

		if( inParams.i64StartAddress + inParams.i64Length > ci64EndSample )
			inParams.i64Length = ci64EndSample - inParams.i64StartAddress;

		if( 0 >= inParams.i64Length )
			return CS_INVALID_LENGTH;

		pOutParams->i64ActualStart = inParams.i64StartAddress;
		pOutParams->i64ActualLength = inParams.i64Length;

		if( NULL != inParams.pDataBuffer )
		{
			if( ::IsBadWritePtr( inParams.pDataBuffer, m_Acq.u32SampleSize * uInt32(inParams.i64Length) ) )
				return CS_BUFFER_TOO_SMALL;
		
			void *p[2];
			p[0] = inParams.pDataBuffer;
			p[1] = inParams.pDataBuffer;
			LARGE_INTEGER llStart, llFinish, llFreq;
			if(m_bDisplayXferTrace) 
				::QueryPerformanceCounter( &llStart );
			m_pAdq->GetData( p, (unsigned int)inParams.i64Length, (unsigned char)m_Acq.u32SampleSize,
				             inParams.u32Segment-1, 1, 
							 (unsigned char)ConvertToHw(inParams.u16Channel),
							 (unsigned int)(inParams.i64StartAddress + m_Acq.i64TriggerHoldoff + m_i64TrigAddrAdj), (unsigned int)inParams.i64Length, 
							 ADQ_TRANSFER_MODE_NORMAL);
			
			if(m_bDisplayXferTrace) 
			{
				::QueryPerformanceCounter( &llFinish );
				::QueryPerformanceFrequency( &llFreq );
				llFinish.QuadPart -= llStart.QuadPart;
				double dRate = double(llFreq.QuadPart);
				dRate *= inParams.i64Length*m_Acq.u32SampleSize * 1e-6;
				dRate /= llFinish.QuadPart; 
				char str[128];
				::_stprintf_s( str, 128, "Xfer Size 0x%08x bytes Rate %8.3f Mb/sec\n", uInt32(inParams.i64Length*m_Acq.u32SampleSize), dRate);
				OutputDebugString( str );
			}
			
			if(m_bDisplayTrace) 
			{
				char str[128];
				::_stprintf_s( str, 128,  "m_pAdq->GetData(%p (%p, %p), %d, %d, %d, 1, %d, %d, %d, ADQ_TRANSFER_MODE_NORMAL)\n", 
				p, p[0], p[1], (unsigned int)inParams.i64Length, (unsigned char)m_Acq.u32SampleSize, inParams.u32Segment-1,
				(unsigned char)ConvertToHw(inParams.u16Channel),(unsigned int)(inParams.i64StartAddress + m_Acq.i64TriggerHoldoff), (unsigned int)inParams.i64Length);
				::OutputDebugString(str);
			}
			
		} //End reading data
	}
	
	if (NULL != inParams.hNotifyEvent)	::SetEvent( inParams.hNotifyEvent );
	return CS_SUCCESS;
}


int32 CsSpDevice::TransferDataEx(PIN_PARAMS_TRANSFERDATA_EX pInParamsEx, POUT_PARAMS_TRANSFERDATA_EX pOutParamsEx)
{
	int r(0);
	bool	bRawTimeStamp = (0!= (pInParamsEx->u32Mode & TxMODE_SEGMENT_TAIL));
	bool	bInterleaved  = (0!= (pInParamsEx->u32Mode & TxMODE_DATA_INTERLEAVED));

	if( NULL == pInParamsEx->pDataBuffer )
	{
		return CS_NULL_POINTER;
	}

	if( ::IsBadWritePtr(pInParamsEx->pDataBuffer, UINT_PTR(pInParamsEx->i64BufferLength)) )
	{
		return CS_INVALID_POINTER_BUFFER;
	}

	if( pInParamsEx->u32StartSegment > m_Acq.u32SegmentCount || 0 == pInParamsEx->u32StartSegment )
	{
		if (NULL != pInParamsEx->hNotifyEvent)	::SetEvent( pInParamsEx->hNotifyEvent );
		return CS_INVALID_SEGMENT;
	}

	if ( pInParamsEx->u16Channel >  m_Info.usChanNumber )
	{
		if (NULL != pInParamsEx->hNotifyEvent)	::SetEvent( pInParamsEx->hNotifyEvent );
		return CS_INVALID_CHANNEL;
	}

	// overwite interleaved mode 
	if ( 0 != ( m_Acq.u32Mode & CS_MODE_SINGLE ))
		bInterleaved = false;
	else if ( bRawTimeStamp )
		bInterleaved = true;

	if( bRawTimeStamp )
	{
		if ( NULL != pInParamsEx->pDataBuffer  )
		{
			PCSSEGMENTTAIL pTimeStamp = (PCSSEGMENTTAIL)pInParamsEx->pDataBuffer;

			for ( uInt32 i = pInParamsEx->u32StartSegment, j = 0; j < pInParamsEx->u32SegmentCount; i++, j++ )
			{
				r = m_pAdq->MultiRecordGetRecord( i-1 ); // records start from 0 with the usb device
				pTimeStamp[j].i64TimeStamp = int64(m_pAdq->GetTrigTime());
				pTimeStamp[j].i64Reserved0 = pTimeStamp[j].i64Reserved1 = pTimeStamp[j].i64Reserved2 = 0;
				
				if(m_bDisplayTrace) 
				{
					char str[128];
					::_stprintf_s( str, 128,  "m_pAdq->MultiRecordGetRecord(%d)\n", i-1 );
					::OutputDebugString(str);
					::OutputDebugString("m_pAdq->GetTrigTime()\n");
				}
			}
		}
		m_i64TickFrequency =   m_Acq.i64SampleRate * m_Info.usChanNumber;
	}
	else 
	{
		const int64 ci64StartSample = -m_Acq.i64TriggerHoldoff + m_Acq.i64TriggerDelay;
		const int64 ci64EndSample = ci64StartSample + m_Acq.i64SegmentSize;

		if( pInParamsEx->i64StartAddress < ci64StartSample )
			pInParamsEx->i64StartAddress = ci64StartSample;

		if( pInParamsEx->i64StartAddress > ci64EndSample - 1 )
			pInParamsEx->i64StartAddress = ci64EndSample - 1;

		if( pInParamsEx->i64Length < 1 )
			pInParamsEx->i64Length = 1;

		if( pInParamsEx->i64StartAddress + pInParamsEx->i64Length > ci64EndSample )
			pInParamsEx->i64Length = ci64EndSample - pInParamsEx->i64StartAddress;

		if( 0 >= pInParamsEx->i64Length )
			return CS_INVALID_LENGTH;

		if( NULL != pInParamsEx->pDataBuffer )
		{
			if( ::IsBadWritePtr( pInParamsEx->pDataBuffer, m_Acq.u32SampleSize * uInt32(pInParamsEx->i64Length) ) )
				return CS_BUFFER_TOO_SMALL;
		
			void *p[2]; //RG - right now USB devices will never have to handle more than 2 channels
			p[1] = pInParamsEx->pDataBuffer; // because all the 2 channel devices are reversed so chan 2 is chan 1 and vice versa

			if ( !bInterleaved )
			{
				p[0] = pInParamsEx->pDataBuffer;
				pOutParamsEx->u32DataFormat0 = TxFORMAT_SINGLE;
			}
			else
			{
				// i64BufferLength is in bytes
				p[0] = (unsigned char*)pInParamsEx->pDataBuffer + (pInParamsEx->i64BufferLength / (m_Acq.u32Mode & CS_MASKED_MODE));
				pInParamsEx->u16Channel = m_Acq.u32Mode & CS_MASKED_MODE; 
				pOutParamsEx->u32DataFormat0 = TxFORMAT_STACKED;
			}

			LARGE_INTEGER llStart, llFinish, llFreq;
			if(m_bDisplayXferTrace) 
				::QueryPerformanceCounter( &llStart );
			
			unsigned char ucChannelMask = ( CS_MODE_DUAL == m_Acq.u32Mode ) ? 3 : 1;
			unsigned char ucHwChannel = ( bInterleaved ) ? ucChannelMask : (unsigned char)ConvertToHw(pInParamsEx->u16Channel);
			m_pAdq->GetData( p, (unsigned int)pInParamsEx->i64Length, (unsigned char)m_Acq.u32SampleSize,
							 pInParamsEx->u32StartSegment-1, pInParamsEx->u32SegmentCount, 
							 ucHwChannel,  
							 (unsigned int)(pInParamsEx->i64StartAddress + m_Acq.i64TriggerHoldoff), (unsigned int)pInParamsEx->i64Length, 
							 ADQ_TRANSFER_MODE_NORMAL);
			

			if(m_bDisplayXferTrace) 
			{
				::QueryPerformanceCounter( &llFinish );
				::QueryPerformanceFrequency( &llFreq );
				llFinish.QuadPart -= llStart.QuadPart;
				double dRate = double(llFreq.QuadPart);
				dRate *= pInParamsEx->i64Length*m_Acq.u32SampleSize * 1e-6;
				dRate /= llFinish.QuadPart; 
				char str[128];
				::_stprintf_s( str, 128, "Xfer Size 0x%08x bytes Rate %8.3f Mb/sec\n", uInt32(pInParamsEx->i64Length*m_Acq.u32SampleSize), dRate);
				OutputDebugString( str );
			}
			
			if(m_bDisplayTrace) 
			{
				char str[128];
				::_stprintf_s( str, 128,  "m_pAdq->GetData(%p (%p, %p), %d, %d, %d, 1, %d, %d, %d, ADQ_TRANSFER_MODE_NORMAL)\n", 
				p, p[0], p[1], (unsigned int)pInParamsEx->i64Length, (unsigned char)m_Acq.u32SampleSize, /*pInParamsEx->u32Segment-1*/pInParamsEx->u32StartSegment-1,
				(unsigned char)ConvertToHw(pInParamsEx->u16Channel),(unsigned int)(pInParamsEx->i64StartAddress + m_Acq.i64TriggerHoldoff), (unsigned int)pInParamsEx->i64Length);
				::OutputDebugString(str);
			}
			
		} //End reading data
	}
	
	if (NULL != pInParamsEx->hNotifyEvent)	::SetEvent( pInParamsEx->hNotifyEvent );
	return CS_SUCCESS;
}


uInt16 CsSpDevice::ConvertToHw( uInt16 u16SwChannel )
{
	if( 1 == m_Info.usChanNumber )
		return u16SwChannel;
	else if( 2 == m_Info.usChanNumber )
		return CS_CHAN_1 == u16SwChannel? CS_CHAN_2 : CS_CHAN_1; //channel are inverted on two channel boards
	else
		return CS_CHAN_1;
}