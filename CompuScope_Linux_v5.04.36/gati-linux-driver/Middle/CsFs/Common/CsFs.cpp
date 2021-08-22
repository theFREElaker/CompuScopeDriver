// CsFs.cpp
//

#include "StdAfx.h"
#include "CsDefines.h"
#include "CsErrors.h"
#include "CsStruct.h"
#include "CsFs.h"
#include "CsPrivatePrototypes.h"
#include "CsExpert.h"
#include "SigUtils.h"

#ifdef _WIN32
	#include "Debug.h"
#endif

bool			g_RMInitialized = false;


uInt16 GetTriggerDate(void)
{
	uInt16 u16Date;
#ifdef _WIN32

	SYSTEMTIME stSysTime;
	GetLocalTime(&stSysTime);

	u16Date = ((stSysTime.wYear - 1980) << 9) | ((stSysTime.wMonth & 0xf) << 5) | (stSysTime.wDay & 0x1f);

#else

	time_t ltime = time(NULL);
	struct tm *tm = localtime(&ltime);

	// we're subtracting 1980 from the year because that's how the sig file defines it.
	// we're adding 1 to the month because the Windows structure SYSTEMTIME has the month go from 1-12 
	// and localtime goes from 0-11.  GageScope expects it to be like the SYSTEMTIME structure.
	u16Date = (((tm->tm_year + 1900) - 1980) << 9) | (((tm->tm_mon + 1) & 0xf) << 5) | (tm->tm_mday & 0x1f);

#endif

	return u16Date;
}

void GetTriggerDateTime(uInt16 *u16Date, uInt16 *u16Time)
{
#ifdef _WIN32

	SYSTEMTIME stSysTime;
	GetLocalTime(&stSysTime);

	*u16Date = ((stSysTime.wYear - 1980) << 9) | ((stSysTime.wMonth & 0xf) << 5) | (stSysTime.wDay & 0x1f);
	*u16Time = (stSysTime.wHour << 11) | ((stSysTime.wMinute & 0x3f) << 5) | ((stSysTime.wSecond & 0x3f) >> 1);

#else

    struct timeval tv;
    struct timezone tz;
    struct tm *tm;

    gettimeofday(&tv, &tz);
    tm=localtime(&tv.tv_sec);

	// we're subtracting 1980 from the year because that's how the sig file defines it.
	// we're adding 1 to the month because the Windows structure SYSTEMTIME has the month go from 1-12 
	// and localtime goes from 0-11.  GageScope expects it to be like the SYSTEMTIME structure.
	*u16Date = (((tm->tm_year + 1900) - 1980) << 9) | (((tm->tm_mon + 1) & 0xf) << 5) | (tm->tm_mday & 0x1f);
	*u16Time = (tm->tm_hour << 11) | ((tm->tm_min & 0x3f) << 5) | ((tm->tm_sec & 0x3f) >> 1);

#endif
}


void GetExtendedTriggerDateTime(uInt16 *u16Date, uInt32 *u32Time)
{
#ifdef _WIN32

	SYSTEMTIME stSysTime;
	GetLocalTime(&stSysTime);

	*u16Date = ((stSysTime.wYear - 1980) << 9) | ((stSysTime.wMonth & 0xf) << 5) | (stSysTime.wDay & 0x1f);

	*u32Time = (stSysTime.wHour & 0x1f) << 19;
	*u32Time |= (stSysTime.wMinute & 0x3f) << 13;
	*u32Time |= (stSysTime.wSecond & 0x3f) << 7;
	*u32Time |= (stSysTime.wMilliseconds / 10) & 0x7f; // 100ths of a second

#else

    struct timeval tv;
    struct timezone tz;
    struct tm *tm;

    gettimeofday(&tv, &tz);
    tm=localtime(&tv.tv_sec);

	// we're subtracting 1980 from the year because that's how the sig file defines it.
	// we're adding 1 to the month because the Windows structure SYSTEMTIME has the month go from 1-12 
	// and localtime goes from 0-11.  GageScope expects it to be like the SYSTEMTIME structure.
	*u16Date = (uInt16)((((tm->tm_year + 1900) - 1980) << 9) | (((tm->tm_mon + 1) & 0xf) << 5) | (tm->tm_mday & 0x1f));

	*u32Time = (tm->tm_hour & 0x1f) << 19;
	*u32Time |= (tm->tm_min & 0x3f) << 13;
	*u32Time |= (tm->tm_sec & 0x3f) << 7;
	*u32Time |= (tv.tv_usec / 10000) & 0x7f; // 100ths of a second

#endif
}

#ifdef _WIN32

CTraceManager*	g_pTraceMngr= NULL;

BOOL APIENTRY DllMain( HANDLE , 
                       DWORD  ul_reason_for_call, 
                       LPVOID 
					 )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
#if defined (_DEBUG) || defined  (RELEASE_TRACE)
			g_pTraceMngr = new CTraceManager();
#endif
			break;

		case DLL_THREAD_ATTACH: break;
		case DLL_THREAD_DETACH: break;
		case DLL_PROCESS_DETACH:
			delete g_pTraceMngr;
			break;
	}
    return TRUE;
}

#endif // _WIN32
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 FS_API RetrieveChannelFromRawBuffer ( PVOID	pMulrecRawDataBuffer,
											int64	i64RawDataBufferSize,
											uInt32	u32SegmentId,
											uInt16	ChannelIndex,
											int64	i64Start,
											int64	i64Length,
											PVOID	pNormalizedDataBuffer,
											int64*	pi64TrigTimeStamp,
											int64*	pi64ActualStart,
											int64*	pi64ActualLength )
{
	PMULRECRAWDATA_HEADER	pHeader = (PMULRECRAWDATA_HEADER) pMulrecRawDataBuffer;
	PUCHAR					pUserBuffer = (PUCHAR) pMulrecRawDataBuffer;

	int32		i32Status = CS_SUCCESS;
	PUCHAR		pFileStartSegment;
	PUCHAR		pFileFooter;
	PUCHAR		pTrigMasterFooter;
	PUCHAR		pDataStartSegment;
	PUCHAR		pCurrentLocation;
	int64		i64SegmentDataSize;
	int64		u32TriggerPosition = 0;
	uInt32		u32AdjustPosition = 0;
	int64		i64OffsetSegment;
	uInt32		i;
	uInt32		u32SkipCount = 0;


	if ( ! CsIsCombineBoard( pHeader->u32BoardType ) )
		return CS_FUNCTION_NOT_SUPPORTED;

	// Validation of channel index
	for (i = 1; i <= pHeader->u32ChannelsPerCard * pHeader->u32CardCount; )
	{
		if ( i == ChannelIndex )
			break;
		i += pHeader->u16ChannelSkip;
		u32SkipCount++;
	}

	if ( i > pHeader->u32ChannelsPerCard * pHeader->u32CardCount )
		return CS_INVALID_CHANNEL;

	if ( u32SegmentId > pHeader->u32LastSegment ||
		 u32SegmentId < pHeader->u32FirstSegment )
		 return CS_INVALID_SEGMENT;

	if ( i64Start >  (pHeader->i64Depth + pHeader->i64TriggerDelay) )
		return	CS_INVALID_START;

	// Calculation for boudary of data
	int64	i64LimitStart; 
	int64	i64LimitEnd; 

	i64LimitEnd	  = pHeader->i64TriggerDelay + pHeader->i64Depth;
	i64LimitStart = pHeader->i64TriggerDelay - (pHeader->i64SegmentSize - pHeader->i64Depth);
	
	if ( -1*pHeader->i64TriggerHoldOff > i64LimitStart )
		i64LimitStart = -1*pHeader->i64TriggerHoldOff;

	if ( i64Start >= i64LimitEnd )
		return CS_INVALID_START;

	if ( i64Start < i64LimitStart )
		i64Start = i64LimitStart;

	if ( i64Start + i64Length > i64LimitEnd )
		i64Length = i64LimitEnd - i64Start;

	*pi64ActualStart = i64Start;
	*pi64ActualLength = i64Length;
    
	pUserBuffer = pUserBuffer + sizeof(MULRECRAWDATA_HEADER) + pHeader->u32DmaOffsetAdjust * sizeof(uInt32);

	i64SegmentDataSize = pHeader->i64MemorySegmentSize - pHeader->u16FooterSize;

	// Total Data (inluded footers) for one card (in samples)
	int64 i64OneCardDataSize  = (pHeader->u32LastSegment - pHeader->u32FirstSegment + 1) * pHeader->i64MemorySegmentSize;


	if ( i64RawDataBufferSize != int64(i64OneCardDataSize * pHeader->u32SampleSize * pHeader->u32CardCount + sizeof(MULRECRAWDATA_HEADER)))
		return CS_INVALID_POINTER_BUFFER;
	//
	// Calculate position of the trigger master card's footer 
	//

	// Card Data offset in buffer (in samples )
	i64OffsetSegment = (pHeader->u16TrigMasterCardIndex - 1) * i64OneCardDataSize;

	// Segment offset data in file (in samples )
	i64OffsetSegment = i64OffsetSegment + (u32SegmentId-pHeader->u32FirstSegment) * pHeader->i64MemorySegmentSize;

	//  Segment offset in bytes
	i64OffsetSegment = i64OffsetSegment * pHeader->u32SampleSize;

	pTrigMasterFooter = pUserBuffer + i64OffsetSegment + i64SegmentDataSize * pHeader->u32SampleSize;


	//
	// Calculate position of the current card's footer 
	//

	uInt16 u16CardIndex = (uInt16) (1 + ( ChannelIndex - 1) / pHeader->u32ChannelsPerCard );

	// Card Data offset in buffer (in samples )
	i64OffsetSegment = (u16CardIndex - 1) * i64OneCardDataSize;

	// Segment offset data in file (in samples )
	i64OffsetSegment = i64OffsetSegment + (u32SegmentId-pHeader->u32FirstSegment) * pHeader->i64MemorySegmentSize;

	// Segment Size in bytes
	i64SegmentDataSize = i64SegmentDataSize * pHeader->u32SampleSize;

	//  Segment offset in bytes
	i64OffsetSegment = i64OffsetSegment * pHeader->u32SampleSize;

	pFileStartSegment = pUserBuffer + i64OffsetSegment;
	pFileFooter = pFileStartSegment + i64SegmentDataSize;

	// Calulcate trigger position from the footer
	uInt32	u32TriggerAddress;
	uInt32	u32TaddrAndRecs;
	uInt32* pu32Ptr = (uInt32*) pFileFooter;
	uInt32* pu32TrigMasterPtr = (uInt32*) pTrigMasterFooter;


	// Always return trigger timestamp from the Trigger Master card
	int64 i64TimeStamp = pu32TrigMasterPtr[0];
	i64TimeStamp = (i64TimeStamp << 32 ) |  pu32TrigMasterPtr[1];
	*pi64TrigTimeStamp = (i64TimeStamp >> 4);

	if ( CsIsSpiderBoard( pHeader->u32BoardType ) )
	{
		// Take trigger address and ebts from the current card
		u32TriggerAddress	= pu32Ptr[2];
		u32TaddrAndRecs		= pu32Ptr[1];
	}
	else
	{
		if ( CS12400_BOARDTYPE == pHeader->u32BoardType )
		{
			// Take trigger address and ebts from the trigger master card
			u32TriggerAddress	= pu32TrigMasterPtr[3];
			u32TaddrAndRecs		= pu32TrigMasterPtr[2];
		}
		else
		{
			// Take trigger address and ebts from the current card
			u32TriggerAddress	= pu32Ptr[3];
			u32TaddrAndRecs		= pu32Ptr[2];
		}
	}


	// Calculate the trigger Address
	if ( pHeader->bBigMemoryFirmware )
		u32TaddrAndRecs = (u32TaddrAndRecs >> 3) & 0x1f000000;
	else
		u32TaddrAndRecs = (u32TaddrAndRecs >> 4) & 0x0f000000;

	// The current driver support max 4G Samples
	_ASSERT ( i64Start < 0x100000000LL );
	_ASSERT ( pHeader->i64RealDepth < 0x100000000LL );
	_ASSERT ( pHeader->i64RealSegmentSize < 0x100000000LL );
	_ASSERT ( pHeader->i64TriggerDelay < 0x100000000LL );

	if ( ( pHeader->i64RealDepth < 0x100000000LL )&&
         (pHeader->i64RealSegmentSize < 0x100000000LL ) &&
         ( pHeader->i64TriggerDelay < 0x100000000LL ) )
	{

		if ( CsIsSpiderBoard( pHeader->u32BoardType ) )


			u32TriggerPosition = CalculateReadingOffsetSpider ( pHeader->u32Mode,
															(uInt32) pHeader->i64RealDepth,
															(uInt32) pHeader->i64RealSegmentSize,
															(uInt32) (i64Start + pHeader->u32TrigAddrAdjust),
															u32TriggerAddress,
															u32TaddrAndRecs,
															&u32AdjustPosition);

		else
			u32TriggerPosition = CalculateReadingOffset ( pHeader->u32Mode,
															pHeader->u32DecimationFactor,
															(uInt32) pHeader->i64RealDepth,
															(uInt32) pHeader->i64RealSegmentSize,
															(uInt32) (i64Start + pHeader->u32TrigAddrAdjust),
															u32TriggerAddress,
															u32TaddrAndRecs,
															&u32AdjustPosition);
	}


	// Search for start of the Segment from the trigger address
	// u32TriggerPosition is at 8 samples resolution
	pDataStartSegment = pFileStartSegment + 8 * pHeader->u32SampleSize * u32TriggerPosition;

	pUserBuffer = (PUCHAR) pNormalizedDataBuffer;

	if ( 1 == pHeader->u32ChannelsInRawData )
	{
		// Adjust for Etbs and Alignment
		pCurrentLocation = pDataStartSegment + (u32AdjustPosition * pHeader->u32SampleSize );
		if ( pCurrentLocation >= pFileFooter )
			pCurrentLocation = pCurrentLocation - i64SegmentDataSize;

		int64 i64WorkingSize = ( pFileFooter - pCurrentLocation );
		int64 i64BytesToCopy = i64Length * pHeader->u32SampleSize;

		if ( i64BytesToCopy  <=  i64WorkingSize )
			memcpy( pUserBuffer, pCurrentLocation, (size_t) i64BytesToCopy );
		else //	( i64BytesToCopy  >  i64WorkingSize )
		{
			memcpy( pUserBuffer, pCurrentLocation, (size_t) i64WorkingSize );
			i64BytesToCopy -= i64WorkingSize;

			pUserBuffer += i64WorkingSize;
			pCurrentLocation = pFileStartSegment;
			memcpy( pUserBuffer, pCurrentLocation, (size_t) i64BytesToCopy );
		}
	}
	else
	{
		if ( CsIsSpiderBoard( pHeader->u32BoardType ) )
		{
			if ( pHeader->u32Mode & CS_MODE_DUAL )
			{
				// Data will be in interleave in a chunk of 8 samples
				// A0A1 A2A3 B0B1 B2B3

				uInt8	u8ChunkSize = 8;
				uInt8	u8ChannelDataInChunk = 4;
				uInt8	u8OffsetInChunk = (uInt8)(u32AdjustPosition % u8ChannelDataInChunk);
				uInt32	u32CopyDataSize;

				pCurrentLocation = pDataStartSegment;

				i = 0;
				if ( ChannelIndex > pHeader->u16ChannelSkip )
				{
					// Skip the first 4 samples of channel 1
					pCurrentLocation += u32SkipCount*u8ChannelDataInChunk*pHeader->u32SampleSize;
				}

				// Adjust for Etbs and Alignment
				pCurrentLocation = pCurrentLocation + (u32AdjustPosition / u8ChannelDataInChunk) * u8ChunkSize * pHeader->u32SampleSize;

				if ( u8OffsetInChunk )
					pCurrentLocation +=  u8OffsetInChunk * pHeader->u32SampleSize;

				if ( pCurrentLocation >= pFileFooter )
					pCurrentLocation = pCurrentLocation - i64SegmentDataSize;

				if ( u8OffsetInChunk )
				{
					u32CopyDataSize = (u8ChannelDataInChunk-u8OffsetInChunk)*pHeader->u32SampleSize;
					memcpy( &pUserBuffer[i], pCurrentLocation, u32CopyDataSize );
					pCurrentLocation += (u8ChunkSize-u8OffsetInChunk)*pHeader->u32SampleSize;

					if ( pCurrentLocation >= pFileFooter )
						pCurrentLocation = pCurrentLocation - i64SegmentDataSize;

					i += u32CopyDataSize;
					i64Length --;
				}

				while ( i64Length >= u8ChannelDataInChunk )
				{
					u32CopyDataSize = u8ChannelDataInChunk*pHeader->u32SampleSize;
					memcpy( &pUserBuffer[i], pCurrentLocation, u32CopyDataSize );
					i += u32CopyDataSize;
					i64Length -= u8ChannelDataInChunk;
					pCurrentLocation += u8ChunkSize*pHeader->u32SampleSize;

					if ( pCurrentLocation >= pFileFooter )
						pCurrentLocation = pCurrentLocation - i64SegmentDataSize;
				}

				if ( i64Length > 0 )
				{
					u32CopyDataSize = (uInt32) (i64Length*pHeader->u32SampleSize);
					memcpy( &pUserBuffer[i], pCurrentLocation, u32CopyDataSize );
					i += (uInt32) i64Length;
					i64Length = 0;
				}
			}
		}
		else
		{
				// Data will be in interleave in a chunk of 4 samples
			// A0A1 B0B1 A2A3 B2B3 A4A5  B4B5 ....

			pCurrentLocation = pDataStartSegment;

			i = 0;
			if ( (ChannelIndex % 2) == 0 )
			{
				// Skip the first 2 samples of channel 1
				pCurrentLocation += 2*pHeader->u32SampleSize;
			}

			// Adjust for Etbs and Alignment
			pCurrentLocation = pCurrentLocation + (u32AdjustPosition / 2) * 4 * pHeader->u32SampleSize;
			
			if ( u32AdjustPosition % 2 )
				pCurrentLocation += pHeader->u32SampleSize;

			if ( pCurrentLocation >= pFileFooter )
				pCurrentLocation = pCurrentLocation - i64SegmentDataSize;

			if ( u32AdjustPosition % 2 )
			{
				memcpy( &pUserBuffer[i], pCurrentLocation, pHeader->u32SampleSize );
				pCurrentLocation += pHeader->u32SampleSize;
				pCurrentLocation += 2*pHeader->u32SampleSize;

				if ( pCurrentLocation >= pFileFooter )
					pCurrentLocation = pCurrentLocation - i64SegmentDataSize;

				i += pHeader->u32SampleSize;
				i64Length --;
			}
				
			while ( i64Length > 1 )
			{
				memcpy( &pUserBuffer[i], pCurrentLocation, 2*pHeader->u32SampleSize );
				i += 2*pHeader->u32SampleSize;
				i64Length -= 2;
				pCurrentLocation += 4*pHeader->u32SampleSize;

				if ( pCurrentLocation >= pFileFooter )
					pCurrentLocation = pCurrentLocation - i64SegmentDataSize;
			}

			if ( i64Length > 0 )
			{
				memcpy( &pUserBuffer[i], pCurrentLocation, pHeader->u32SampleSize );
				i++;
				i64Length --;
			}
		}
	}


	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32	CalculateReadingOffset ( uInt32 u32Mode,
								 uInt32 u32DeimationFactor,
								 uInt32	u32RealDepth,
								 uInt32	u32RealSegmentSize,
								 uInt32	u32StartAddressOffset,
								 uInt32 u32TaLow,
								 uInt32 u32TaHigh,
								 uInt32	*u32OffsetAdjust )
{
	uInt32	m_Etb;			// Enhanced trigger bit
	uInt32	m_Stb;			// Sub sample trigger bits

	uInt32	m_MemEtb;		// Memory address trigger bit
	uInt32	m_SkipEtb;		// Skiping enhanced trigger bit
	uInt32	m_AddonEtb;		// Add-on enhanced trigger bit
	uInt32	m_ChannelSkipEtb;		// Channel skipping enhanced trigger bit
	uInt32  m_DecEtb;				// Decimation etb
	uInt32	u32HalfDecFactor;
	uInt16	m_u16DecimationFactor = (uInt16) u32DeimationFactor;
	uInt32	u32AlignmentOffset;

	uInt32	StartOffset;
	uInt8	u8Shift = ((u32Mode & CS_MODE_DUAL) != 0) ? 1: 2;

	uInt32	u32PostTrig = u32RealDepth  >> u8Shift;
	uInt32	u32RecLen	= u32RealSegmentSize >> u8Shift;
	uInt32	u32PreTrig;
			
	if (  u32RecLen > u32PostTrig  )
		u32PreTrig = u32RecLen - u32PostTrig;
	else
		u32PreTrig = 0;

	// Relative offset from beginning of the segment
	uInt32	u32Offset = (u32PreTrig << u8Shift) + u32StartAddressOffset;
	u32Offset = u32Offset % u32RealSegmentSize;


	if ((u32Mode & CS_MODE_SINGLE) != 0) 
	{
		u32AlignmentOffset = u32Offset & 0x7;
		u32Offset = u32Offset >> 1;
	}
	else
		u32AlignmentOffset = u32Offset & 0x3;


	// TriggerAddress is the address of 8 samples (128 bits) 4 samples for CH1, 4 samples for Ch2
	// StartAddressOffset is the Offset for 1 Channel, so to add it to trigger it must be shifted by 2 

	//Take add on etb from the trigger master and the rest localy

	m_Etb = ((u32TaLow & 0x0F) << 4) | ((u32TaLow & 0xF0) >> 4) | (u32TaLow & 0x100);

	m_MemEtb = (m_Etb & 0x100) >> 8;
	m_SkipEtb = (m_Etb & 0xFE) >> 1;
	m_AddonEtb = m_Etb & 0x1;

	if ( m_MemEtb != 0)
		m_MemEtb = m_MemEtb;

	if ( m_u16DecimationFactor > 1 )
		m_ChannelSkipEtb = m_Etb & 0x1;
	else
		m_ChannelSkipEtb = m_Etb & 0x2;


	m_DecEtb = 0;

	// Skipping Etb
	u32HalfDecFactor = m_u16DecimationFactor >> 1;

	if ( m_u16DecimationFactor > 1)
	{
		if( m_u16DecimationFactor > 0x1FFF )
			m_SkipEtb <<= 7;
		else if( m_u16DecimationFactor > 0xFFF )
			m_SkipEtb <<= 6;
		else if( m_u16DecimationFactor > 0x7FF )
			m_SkipEtb <<= 5;
		else if( m_u16DecimationFactor > 0x3FF )
			m_SkipEtb <<= 4;
		else if( m_u16DecimationFactor > 0x1FF )
			m_SkipEtb <<= 3;
		else if( m_u16DecimationFactor > 0xFF )
			m_SkipEtb <<= 2;
		else if( m_u16DecimationFactor > 0x7F )
			m_SkipEtb <<= 1;

		if ( m_SkipEtb  >= u32HalfDecFactor )
			m_DecEtb = 1;
	}

	m_Stb = (u32TaLow >> 5) & 0x07;
	m_Stb = m_Stb | ((u32TaLow & 0xF) << 3);

	u32TaLow >>= 9;//----- as required by the HW

	u32TaLow |= (u32TaHigh >> 1);

	StartOffset = 0;
	StartOffset = (u32TaLow >=(u32PreTrig>>1))?(u32TaLow - (u32PreTrig>>1)):((u32RecLen>>1) + u32TaLow - (u32PreTrig>>1));
	
	StartOffset += (u32Offset>>2);//offset is 16bits res

	StartOffset %= (u32RecLen >> 1);

	*u32OffsetAdjust = u32AlignmentOffset;

	if ( m_DecEtb )
		*u32OffsetAdjust += 1;

	if ( m_u16DecimationFactor <=1 && !m_AddonEtb )
		*u32OffsetAdjust += 1;

	if (( u32Mode & CS_MODE_SINGLE ) && ! m_ChannelSkipEtb )
		*u32OffsetAdjust += 2;

	if ( m_MemEtb )
	{

		if ( u32Mode & CS_MODE_SINGLE )
			*u32OffsetAdjust += 4;
		else
			*u32OffsetAdjust += 2;
	}

	return StartOffset;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32	CalculateReadingOffsetSpider ( uInt32 u32Mode,
								 uInt32	u32RealDepth,
								 uInt32	u32RealSegmentSize,
								 uInt32	u32StartAddressOffset,
								 uInt32 u32TaLow,
								 uInt32 u32TaHigh,
								 uInt32	*u32OffsetAdjust )
{
	uInt32	m_Etb;					// Enhanced trigger bit
	uInt32	m_Stb;					// Sub sample trigger bits

	uInt32	m_MemEtb;				// Memory address trigger bit
	uInt32	m_SkipEtb;				// Skiping enhanced trigger bit
	uInt32	m_AddonEtb;				// Add-on enhanced trigger bit
	uInt32	m_ChannelSkipEtb;		// Channel skipping enhanced trigger bit
	uInt32  m_DecEtb;				// Decimation etb
	uInt32	u32AlignmentOffset;

	uInt32	StartOffset;
	uInt8	u8Shift = 0;

	uInt32	u32PostTrig;
	uInt32	u32RecLen;
	uInt32	u32PreTrig;

	if ((u32Mode & CS_MODE_QUAD) != 0)
		u8Shift = 1;
	else if ((u32Mode & CS_MODE_DUAL) != 0)
		u8Shift = 2;
	else if ((u32Mode & CS_MODE_SINGLE) != 0)
		u8Shift = 3;

	u32PostTrig = u32RealDepth  >> u8Shift;
	u32RecLen	= u32RealSegmentSize >> u8Shift;

//	if ( ( m_CsBoard.u32BaseBoardOptions[m_ImageId] & CS_BBOPTIONS_FIR ) != 0 )  
//		u32RecLen += (CSPDR_MAX_FIR_PIPELINE >> u8Shift);

	if ( (int32)( u32RecLen - u32PostTrig ) > 0 )
		u32PreTrig = u32RecLen - u32PostTrig;
	else
		u32PreTrig = 0;

//	uInt32 u32Offset = u32PreTrig  + (int32)(StartAddressOffset.QuadPart >> u8Shift);
//	u32Offset = u32Offset % u32RecLen;
	uInt32 u32Offset = (u32PreTrig << u8Shift) + u32StartAddressOffset;


	if ((u32Mode & CS_MODE_QUAD) != 0)
	{
		u32AlignmentOffset = u32Offset & 0x1;
	}
	else if ((u32Mode & CS_MODE_DUAL) != 0)
	{
		u32AlignmentOffset = u32Offset & 0x3;
	}
	else if ((u32Mode & CS_MODE_SINGLE) != 0)
	{
		u32AlignmentOffset = u32Offset & 0x7;
	}
	else
		u32AlignmentOffset = 0;

	u32Offset = u32Offset >> u8Shift;
	u32Offset = u32Offset % u32RealSegmentSize;

	m_MemEtb = (u32TaLow >> 4) & 0xf;

	u32TaLow	= u32TaLow >> 8;		// 128 bits resolution
	u32TaHigh	= (u32TaHigh >> 3) & 0x1f000000;

	u32TaLow |= u32TaHigh;

	StartOffset = 0;
	StartOffset = (u32TaLow >= u32PreTrig)?(u32TaLow - u32PreTrig):(u32RecLen + u32TaLow - u32PreTrig);
	
	StartOffset += u32Offset;
	StartOffset %= u32RecLen;

	m_Etb = 
	m_Stb = 
	m_SkipEtb = 
	m_AddonEtb = 
	m_ChannelSkipEtb = 
	m_DecEtb = 0;

	*u32OffsetAdjust = u32AlignmentOffset;

	if ( m_MemEtb )
		*u32OffsetAdjust += (m_MemEtb >> 1);

	return StartOffset;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 FS_API CreateSigHeader(PCSDISKFILEHEADER pSigHeader, 
	 						 PCSACQUISITIONCONFIG pAcqConfig, 
	 						 PCSCHANNELCONFIG pChanConfig, 
							 int32 i32XferStart, 
							 int32 i32XferLength, 
							 uInt32 u32RecordCount)
{
	// Fillout the header with values from the current system.

	float fTbs;
	short shGainAdj;

	if ( IsBadWritePtr(pSigHeader, sizeof(CSDISKFILEHEADER)) )
	{
		return CS_INVALID_POINTER_BUFFER;
	}
	if ( IsBadReadPtr(pAcqConfig, sizeof(CSACQUISITIONCONFIG)) )
	{
		return CS_INVALID_POINTER_BUFFER;
	}
	if ( IsBadReadPtr(pChanConfig, sizeof(CSCHANNELCONFIG)) )
	{
		return CS_INVALID_POINTER_BUFFER;

	}

	// cast the CSDISKFILEHEADER structure to a disk_file_header structure		
	disk_file_header *pHeader = reinterpret_cast<disk_file_header *>(pSigHeader);

	_tcsncpy_s(pHeader->file_version, DISK_FILE_FILEVERSIZE, FILE_VERSION, DISK_FILE_FILEVERSIZE);
	pHeader->crlf1 = 0x0A0D;

	_stprintf_s(pHeader->name, DISK_FILE_CHANNAMESIZE, _T("Ch %02d"), pChanConfig->u32ChannelIndex);
	pHeader->crlf2 = 0x0A0D;
	_tcsncpy_s(pHeader->comment, DISK_FILE_COMMENT_SIZE, _T("GageScope SIG file"), DISK_FILE_COMMENT_SIZE);
	pHeader->crlf3 = 0x0A0D;
	pHeader->control_z = 0x1A;


	// Check to see if the sample rate is in the sample_rate_table in 
	// structs.c. If it is, then use the sample rate index. If it is not,
	// make the sample rate index external clock.
	// Regardless, set external_clock_rate and external_tbs to the actual 
	// sample rate and tbs.
	pHeader->external_clock_rate = float(pAcqConfig->i64SampleRate);
	fTbs = 1000000000.0F / pHeader->external_clock_rate;
	pHeader->sample_rate_index = GetSRIndex(fTbs);

	pHeader->external_tbs = 1000000000.0F / pHeader->external_clock_rate;


	// header mode should always be dual channel
	pHeader->operation_mode = GAGE_DUAL_CHAN;
	pHeader->trigger_slope = GAGE_POSITIVE; 
	pHeader->trigger_source = GAGE_CHAN_A;
	pHeader->trigger_level = (8 == pAcqConfig->u32SampleBits) ? 128 : 0;

	shGainAdj = ConstructGainConstant(pChanConfig->u32InputRange, &pHeader->captured_gain);
	
	pHeader->trigger_probe = GetProbe(shGainAdj);

	pHeader->captured_coupling = GAGE_DC;

	pHeader->starting_address = 0;
	pHeader->trigger_address = int32(-i32XferStart);

	pHeader->trigger_depth = i32XferLength;
	pHeader->sample_depth = i32XferLength * u32RecordCount;

	pHeader->ending_address = pHeader->trigger_address + pHeader->trigger_depth - 1;

	pHeader->current_mem_ptr = pHeader->trigger_address;

	uInt16	trigger_time = 0;
	uInt16	trigger_date = 0;
	::GetTriggerDateTime(&trigger_date, &trigger_time);

	pHeader->trigger_date = trigger_date;
	pHeader->trigger_time = trigger_time;
	pHeader->extended_trigger_time = 0;

	pHeader->trigger_coupling = GAGE_DC;
	pHeader->trigger_gain = GAGE_PM_1_V;
	pHeader->probe = 1;
	pHeader->inverted_data = 0;
	pHeader->resolution_12_bits = int16(pAcqConfig->u32SampleSize) - 1;
	pHeader->multiple_record = (u32RecordCount > 1) ? 2 : 0;
	pHeader->sample_bits = int16(pAcqConfig->u32SampleBits);
	pHeader->sample_offset_32 =  pAcqConfig->i32SampleOffset;
	pHeader->sample_resolution_32 = pAcqConfig->i32SampleRes;
	pHeader->sample_offset = (int16)pHeader->sample_offset_32; 
	
	if ( (pHeader->sample_resolution_32 > SHRT_MAX || pHeader->sample_resolution_32 < SHRT_MIN) )
	{
		pHeader->sample_resolution = 1;
	}
	else // if sample_res_32 exists set sample_res to 1 so GageScope will use sample_offset_32 as well
	{
		pHeader->sample_resolution = pHeader->sample_resolution_32 ? 1 : (int16)pHeader->sample_resolution_32;
	}

	pHeader->board_type = MakeBoardType(pAcqConfig->u32SampleBits);

	pHeader->imped_a = (50 == pChanConfig->u32Impedance) ? GAGE_50_OHM_INPUT : GAGE_1_MOHM_INPUT;
	pHeader->imped_b = GAGE_50_OHM_INPUT;
	pHeader->file_options = 0;
	pHeader->version = 0;
	pHeader->eeprom_options = 0;
	pHeader->trigger_hardware = 0;
	pHeader->record_depth = (uInt32)i32XferLength;
	pHeader->multiple_record_count = u32RecordCount; // or acqInfo.u32SegmentCount;

	pHeader->dc_offset = ConstructDcOffset(pChanConfig->i32DcOffset, pChanConfig->u32InputRange);
	pHeader->UnitFactor = 1;
	_tcsncpy_s(pHeader->UnitString, _countof(pHeader->UnitString), _T("V"), _countof(pHeader->UnitString));
	
	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 FS_API ConvertToSigHeader(PCSDISKFILEHEADER pSigHeader, 
								PCSSIGSTRUCT pSigStruct,
								TCHAR* szComment,
								TCHAR* szName)
{
	// Fillout the header with values from the current system.

	float fTbs;
	short shGainAdj;


	// check that header and sigStruct are readable
	if ( IsBadReadPtr (pSigStruct, sizeof(CSSIGSTRUCT)) )
		return CS_INVALID_POINTER_BUFFER;

	if ( IsBadWritePtr(pSigHeader, sizeof(CSDISKFILEHEADER)) )
		return CS_INVALID_POINTER_BUFFER;

	if ( pSigStruct->u32Size != sizeof(CSSIGSTRUCT) )
		return CS_INVALID_STRUCT_SIZE;					

	// cast the CSDISKFILEHEADER structure to a disk_file_header structure		
	disk_file_header *pHeader = reinterpret_cast<disk_file_header *>(pSigHeader);

	_tcsncpy_s(pHeader->file_version, DISK_FILE_FILEVERSIZE, FILE_VERSION, DISK_FILE_FILEVERSIZE);
	pHeader->crlf1 = 0x0A0D;

	if ( NULL == szName) // if NULL use the default name of the channel number
	{
		_stprintf_s(pHeader->name, DISK_FILE_CHANNAMESIZE, _T("Ch %02d"), pSigStruct->u32Channel);
	}
	else
	{
		_tcsncpy_s(pHeader->name, DISK_FILE_CHANNAMESIZE, szName, DISK_FILE_CHANNAMESIZE);
	}
	pHeader->crlf2 = 0x0A0D;

	if ( NULL == szComment ) // if NULL use the default comment
	{
		_tcsncpy_s(pHeader->comment, DISK_FILE_COMMENT_SIZE, _T("GageScope SIG file"), DISK_FILE_COMMENT_SIZE);
	}
	else
	{
		_tcsncpy_s(pHeader->comment, DISK_FILE_COMMENT_SIZE, szComment, DISK_FILE_COMMENT_SIZE);
	}
	pHeader->crlf3 = 0x0A0D;
	pHeader->control_z = 0x1A;


	// Check to see if the sample rate is in the sample_rate_table in 
	// structs.c. If it is, then use the sample rate index. If it is not,
	// make the sample rate index external clock.
	// Regardless, set external_clock_rate and external_tbs to the actual 
	// sample rate and tbs.
	pHeader->external_clock_rate = float(pSigStruct->i64SampleRate);
	fTbs = 1000000000.0F / pHeader->external_clock_rate;
	pHeader->sample_rate_index = GetSRIndex(fTbs);

	pHeader->external_tbs = 1000000000.0F / pHeader->external_clock_rate;


	// header mode should always be dual channel
	pHeader->operation_mode = GAGE_DUAL_CHAN;
	pHeader->trigger_slope = GAGE_POSITIVE; 
	pHeader->trigger_source = GAGE_CHAN_A;
	pHeader->trigger_level = (8 == pSigStruct->u32SampleBits) ? 128 : 0;

	shGainAdj = ConstructGainConstant(pSigStruct->u32InputRange, &pHeader->captured_gain);
	
	pHeader->probe = GetProbe(shGainAdj);

	pHeader->captured_coupling = GAGE_DC;

	pHeader->starting_address = 0;
	pHeader->trigger_address = int32(-pSigStruct->i64RecordStart);
	pHeader->trigger_depth = int32(pSigStruct->i64RecordLength + pSigStruct->i64RecordStart);
	pHeader->sample_depth = int32(pSigStruct->i64RecordLength * pSigStruct->u32RecordCount);

	pHeader->ending_address = int32(pSigStruct->i64RecordLength - 1);

	pHeader->current_mem_ptr = pHeader->trigger_address;


	pHeader->trigger_date = ::GetTriggerDate();
	pHeader->trigger_time = 0;

	uInt32 u32TrigTime = 0;
	u32TrigTime = (pSigStruct->TimeStamp.u16Hour & 0x1f) << 19;
	u32TrigTime |= (pSigStruct->TimeStamp.u16Minute & 0x3f) << 13;
	u32TrigTime |= (pSigStruct->TimeStamp.u16Second & 0x3f) << 7;
	// convert milliseconds into 100ths of a second to store in extended_trigger_time
	u32TrigTime |= ((pSigStruct->TimeStamp.u16Point1Second) & 0x7f);  
	pHeader->extended_trigger_time = u32TrigTime;

	// if it's 0, then fill in extended trigger time field using the values in system time
	if (0 == pHeader->extended_trigger_time)
	{
		uInt16 u16TriggerDate = 0;
		::GetExtendedTriggerDateTime(&u16TriggerDate, &u32TrigTime);
		pHeader->trigger_date = u16TriggerDate;
		pHeader->extended_trigger_time = u32TrigTime;
	}

	pHeader->trigger_coupling = GAGE_DC;
	pHeader->trigger_gain = GAGE_PM_1_V;
	pHeader->trigger_probe = 0;
	pHeader->inverted_data = 0;
	pHeader->resolution_12_bits = int16(pSigStruct->u32SampleSize) - 1;
	pHeader->multiple_record = (pSigStruct->u32RecordCount > 1) ? 2 : 0;
	pHeader->sample_bits = int16(pSigStruct->u32SampleBits);
	pHeader->sample_offset_32 =  pSigStruct->i32SampleOffset;
	pHeader->sample_resolution_32 = pSigStruct->i32SampleRes;
	pHeader->sample_offset = (int16)pHeader->sample_offset_32; 
	if ( (pHeader->sample_resolution_32 > SHRT_MAX || pHeader->sample_resolution_32 < SHRT_MIN) )
	{
		pHeader->sample_resolution = 1;
	}
	else // if sample_res_32 exists set sample_res to 1 so GageScope will use sample_offset_32 as well
	{
		pHeader->sample_resolution = pHeader->sample_resolution_32 ? 1 : (int16)pHeader->sample_resolution_32;
	}

	pHeader->board_type = MakeBoardType(pSigStruct->u32SampleBits);

	pHeader->imped_a = GAGE_50_OHM_INPUT;
	pHeader->imped_b = GAGE_50_OHM_INPUT;
	pHeader->file_options = 0;
	pHeader->version = 0;
	pHeader->eeprom_options = 0;
	pHeader->trigger_hardware = 0;
	pHeader->record_depth = (uInt32)pSigStruct->i64RecordLength;
	pHeader->multiple_record_count = pSigStruct->u32RecordCount; // or acqInfo.u32SegmentCount;

	pHeader->dc_offset = ConstructDcOffset(pSigStruct->i32DcOffset, pSigStruct->u32InputRange);
	pHeader->UnitFactor = 1;
	_tcsncpy_s(pHeader->UnitString, _countof(pHeader->UnitString), _T("V"), _countof(pHeader->UnitString));

	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 FS_API ConvertFromSigHeader(PCSDISKFILEHEADER pSigHeader, 
								  PCSSIGSTRUCT pSigStruct, 
								  TCHAR* szComment,
								  TCHAR* szName)
{
	// check that header and sigStruct are readable
	if ( ::IsBadWritePtr (pSigStruct, sizeof(CSSIGSTRUCT)) )
		return CS_INVALID_POINTER_BUFFER;

	if ( ::IsBadReadPtr(pSigHeader, sizeof(CSDISKFILEHEADER)) )
		return CS_INVALID_POINTER_BUFFER;
	
	if ( pSigStruct->u32Size != sizeof(CSSIGSTRUCT) )
		return CS_INVALID_STRUCT_SIZE;

	// cast the CSDISKFILEHEADER structure to a disk_file_header structure	
	disk_file_header *pHeader = reinterpret_cast<disk_file_header *>(pSigHeader);

	// Validate for SIG Format file !
	if (pHeader->control_z !=	0x1A)
	{
		return CS_INVALID_SIG_HEADER;
	}

	UINT Version = GetSIGVersion(pHeader->file_version);
	if ( !Version )
	{
		return CS_INVALID_SIG_HEADER;
	}
	if ( NULL != szComment )
	{
		_tcsncpy_s(szComment, DISK_FILE_COMMENT_SIZE, pHeader->comment, DISK_FILE_COMMENT_SIZE);
	}

	if ( NULL != szName )
	{
		_tcsncpy_s(szName, DISK_FILE_CHANNAMESIZE, pHeader->name, DISK_FILE_CHANNAMESIZE);
	}


	AdjustHeaderToVersion(pHeader);

	pSigStruct->i64SampleRate = GetSampleRateFromHeader(pHeader);
	pSigStruct->i64RecordStart = -pHeader->trigger_address;
	pSigStruct->i64RecordLength = pHeader->ending_address - pHeader->starting_address + 1;
	pSigStruct->u32RecordCount = GetMulRecCountFromHeader(pHeader);
	pSigStruct->u32SampleBits = pHeader->sample_bits;
	pSigStruct->u32SampleSize = (pHeader->sample_bits + 7) / 8;
	pSigStruct->i32SampleOffset = GetSampleOffsetFromHeader(pHeader);
	pSigStruct->i32SampleRes = GetSampleResFromHeader(pHeader);
	pSigStruct->u32Channel = 1; 
	pSigStruct->u32InputRange = ConstructDataRange(pHeader->captured_gain);

	pSigStruct->i32DcOffset = ConstructDcOffsetPercent(pHeader->dc_offset, pSigStruct->u32InputRange);

	if ( 0 != pHeader->extended_trigger_time )
	{
		pSigStruct->TimeStamp.u16Hour = uInt16((pHeader->extended_trigger_time >> 19) & 0x1f);		// hours
		pSigStruct->TimeStamp.u16Minute = uInt16((pHeader->extended_trigger_time >> 13) & 0x3f);	// minutes
		pSigStruct->TimeStamp.u16Second = uInt16((pHeader->extended_trigger_time >> 7) & 0x3f);		// seconds
		pSigStruct->TimeStamp.u16Point1Second = uInt16(pHeader->extended_trigger_time & 0x7f);		// 100ths of a second
	}
	else // if extended_trigger_time is 0, used trigger_time
	{
		pSigStruct->TimeStamp.u16Hour = uInt16((pHeader->trigger_time & 0xf800) >> 11);		// hours
		pSigStruct->TimeStamp.u16Minute = uInt16((pHeader->trigger_time & 0x7e0) >> 5);		// minutes
		pSigStruct->TimeStamp.u16Second = uInt16((pHeader->trigger_time & 0x1f) << 1);		// seconds
		pSigStruct->TimeStamp.u16Point1Second = 0;											// 100ths of a second
	}
	
	return CS_SUCCESS;
}
