#include "stdafx.h"
#include "CsSpDevice.h"

#define ONE_GHz 1000000000I64

int32 CsSpDevice::ConfigureAcq( PCSACQUISITIONCONFIG pAcqCfg, bool bCoerce, bool bValidateOnly)
{
	///Acqusition configuration
	if( NULL == pAcqCfg )
		return CS_NULL_POINTER;
	if( ::IsBadReadPtr( pAcqCfg, sizeof(uInt32) ) || pAcqCfg->u32Size < sizeof(CSACQUISITIONCONFIG) )
		return CS_INVALID_STRUCT_SIZE;
	if( ::IsBadReadPtr( pAcqCfg, sizeof(CSACQUISITIONCONFIG) ) )
		return CS_INVALID_PARAMETER;

	int r(0);
	bool bChanged = false;
	//Mode
	bool bRef = 0 != (pAcqCfg->u32Mode & CS_MODE_REFERENCE_CLK);
	uInt32 u32Mode = pAcqCfg->u32Mode & CS_MASKED_MODE;
	uInt32 u32AllowedMode = 1 == m_Info.usChanNumber ? CS_MODE_SINGLE : CS_MODE_DUAL;

	m_Acq.u32TimeStampConfig = pAcqCfg->u32TimeStampConfig;	
	//Only TIMESTAMP_GCLK | TIMESTAMP_FREERUN is possible
	if( (TIMESTAMP_GCLK | TIMESTAMP_FREERUN )!= pAcqCfg->u32TimeStampConfig  )
		if( bCoerce ){bChanged = true; pAcqCfg->u32TimeStampConfig = TIMESTAMP_GCLK | TIMESTAMP_FREERUN;}

	if( u32Mode != u32AllowedMode ) 
		if(bCoerce)
		{
			pAcqCfg->u32Mode = u32AllowedMode;
			if(bRef) pAcqCfg->u32Mode |= CS_MODE_REFERENCE_CLK;
			bChanged = true;
		}
		else
			return CS_INVALID_ACQ_MODE;

	if( 0 == pAcqCfg->u32ExtClk )
	{
		//verify possible SR
		size_t sz;
		for( sz = 0; sz < m_szSrTableSize; sz++ )
		{
			if( m_pSrTable[sz].i64Sr == pAcqCfg->i64SampleRate )
				break;
			else if( m_pSrTable[sz].i64Sr < pAcqCfg->i64SampleRate )
			{
				if( bCoerce )
				{
					bChanged = true;
					if( sz > 0 && pAcqCfg->i64SampleRate - m_pSrTable[sz].i64Sr > m_pSrTable[sz-1].i64Sr - pAcqCfg->i64SampleRate )
						sz--;
					pAcqCfg->i64SampleRate = m_pSrTable[sz].i64Sr;
					break;
				}
				else
					return CS_INVALID_SAMPLE_RATE;
			}
		}
		if( sz == m_szSrTableSize  && pAcqCfg->i64SampleRate != m_pSrTable[sz].i64Sr )
			if( bCoerce ){	bChanged = true;pAcqCfg->i64SampleRate = m_pSrTable[sz].i64Sr;}
			else return CS_INVALID_SAMPLE_RATE;
		if( !bValidateOnly )
		{
//			unsigned int uDataFormat = pAcqCfg->i64SampleRate > ONE_GHz ? 0 : 1; // packed : unpacked
			///gggg  Firmware issue
			unsigned int uDataFormat = m_nType == 112  ? 0 : 1; // packed for 112 : unpacked for others
			r = m_pAdq->SetDataFormat(uDataFormat); 
			
			r = m_pAdq->SetClockSource( 0 );
			r = m_pAdq->SetPllFreqDivider( m_pSrTable[sz].nDiv );
			
			r = m_pAdq->SetSampleSkip( m_pSrTable[sz].uSkip );
			r = m_pAdq->SetClockSource( bRef? 1: 0 );
			r = m_pAdq->SetPllFreqDivider( m_pSrTable[sz].nDiv );
			if(m_bDisplayTrace)
			{
				char str[128];
				::_stprintf_s( str, 128,  "m_pAdq->SetDataFormat(%d);\n", uDataFormat );
				::OutputDebugString(str);
				::OutputDebugString("m_pAdq->SetClockSource(0);\n");
				::_stprintf_s( str, 128,  "m_pAdq->SetPllFreqDivider(%d)\n", m_pSrTable[sz].nDiv );
				::OutputDebugString(str);
				::_stprintf_s( str, 128,  "m_pAdq->SetSampleSkip(%d);\n", m_pSrTable[sz].uSkip );
				::OutputDebugString(str);
				::_stprintf_s( str, 128,  "m_pAdq->SetClockSource(%d);\n",  bRef? 1: 0 );
				::OutputDebugString(str);
				::_stprintf_s( str, 128,  "m_pAdq->SetPllFreqDivider(%d) - returned %d\n", m_pSrTable[sz].nDiv, r );
				::OutputDebugString(str);
					
			}
			if( 0 == r )
				return CS_CLK_NOT_LOCKED;
		}
	}
	else //extclk
	{
		int64 i64MaxRate = GetMaxSr();
		if( 1 == m_Info.usChanNumber)i64MaxRate /= 2;
		if( pAcqCfg->i64SampleRate > i64MaxRate )
			if( bCoerce ){	bChanged = true;pAcqCfg->i64SampleRate = i64MaxRate;}
			else return CS_INVALID_SAMPLE_RATE;
		if( pAcqCfg->i64SampleRate < SPD_MIN_EXT_CLK )
			if( bCoerce ){	bChanged = true;pAcqCfg->i64SampleRate = SPD_MIN_EXT_CLK;}
			else return CS_INVALID_SAMPLE_RATE;

		if( !bValidateOnly )
		{
//			unsigned int uDataFormat = pAcqCfg->i64SampleRate > ONE_GHz ? 0 : 1; // packed : unpacked
			///gggg  Firmware issue
			unsigned int uDataFormat = m_nType == 112  ? 0 : 1; // packed for 112 : unpacked for others
			r = m_pAdq->SetDataFormat(uDataFormat); 
			r = m_pAdq->SetClockSource(2);
			r = m_pAdq->SetSampleSkip( 1 );
			r = m_pAdq->SetClockFrequencyMode(pAcqCfg->i64SampleRate > SPD_LOW_EXT_CLK ? 1: 0);
			if(m_bDisplayTrace) 
			{
				char str[128];
				::_stprintf_s( str, 128,  "m_pAdq->SetDataFormat(%d);\n", uDataFormat );
				::OutputDebugString(str);
				::OutputDebugString("m_pAdq->SetClockSource(2);\n");
				::OutputDebugString("m_pAdq->SetSampleSkip(1);\n");
				::_stprintf_s( str, 128,  "m_pAdq->SetClockFrequencyMode(%d);\n", pAcqCfg->i64SampleRate > SPD_LOW_EXT_CLK ? 1: 0 );
				::OutputDebugString(str);
			}
		}
	}
	if( 0 > pAcqCfg->i64TriggerHoldoff )
		if( bCoerce ){bChanged = true; pAcqCfg->i64TriggerHoldoff = 0;}
		else return CS_INVALID_TRIGHOLDOFF;
	if( 0 > pAcqCfg->i64TriggerDelay)
		if( bCoerce ){bChanged = true; pAcqCfg->i64TriggerDelay = 0;}
		else return CS_INVALID_TRIGDELAY;

	if( SPD_MIN_DEPTH > pAcqCfg->i64Depth)
		if( bCoerce ){bChanged = true; pAcqCfg->i64Depth = SPD_MIN_DEPTH;}
		else return CS_INVALID_TRIG_DEPTH;
	if( pAcqCfg->i64Depth > m_u32MemSize )
		if( bCoerce ){bChanged = true; pAcqCfg->i64Depth = m_u32MemSize;}
		else return CS_INVALID_TRIG_DEPTH;
		
	if( pAcqCfg->i64SegmentSize < pAcqCfg->i64Depth )
		if( bCoerce ){bChanged = true; pAcqCfg->i64SegmentSize = pAcqCfg->i64Depth;	}
		else return CS_INVALID_SEGMENT_SIZE;
	if( pAcqCfg->i64SegmentSize > m_u32MemSize )
		if( bCoerce ){bChanged = true; pAcqCfg->i64SegmentSize = m_u32MemSize;}
		else return CS_INVALID_SEGMENT_SIZE;

	if( 0 != pAcqCfg->i64TriggerDelay )
		if( 0 != pAcqCfg->i64TriggerHoldoff)
			if( bCoerce ){bChanged = true; pAcqCfg->i64TriggerDelay = 0;}
			else return CS_INVALID_TRIGDELAY;
		else if( pAcqCfg->i64TriggerDelay > SPD_MAX_TRIG_DELAY )
			if( bCoerce ){bChanged = true; pAcqCfg->i64TriggerDelay = SPD_MAX_TRIG_DELAY;}
			else return CS_INVALID_TRIGDELAY;

	uInt32 u32MaxSeg = m_u32MemSize /(SPD_ITRA_RECORD+uInt32(pAcqCfg->i64SegmentSize));
	if( pAcqCfg->u32SegmentCount > u32MaxSeg )
		if(bCoerce){bChanged = true; pAcqCfg->u32SegmentCount = u32MaxSeg;}
		else return CS_INVALID_SEGMENT_COUNT;

	pAcqCfg->i64TriggerHoldoff = min( pAcqCfg->i64SegmentSize - pAcqCfg->i64Depth, pAcqCfg->i64TriggerHoldoff );
	pAcqCfg->i64SegmentSize = pAcqCfg->i64TriggerHoldoff + pAcqCfg->i64Depth;

	if(!bValidateOnly)
	{
		int nMemSize = m_pAdq->GetMaxBufferSize();
		if(m_bDisplayTrace) 
		{
			char str[128];
			::_stprintf_s( str, 128,  "m_pAdq->GetMaxBufferSize() returned %d (0x%x)\n", nMemSize, nMemSize );
			::OutputDebugString(str);
		}
		if( (int)pAcqCfg->i64SegmentSize > nMemSize )
			return CS_INVALID_SEGMENT_SIZE;

		// Trigger delay and holdoff must be set before MultiRecordSetup
		if( 0 < pAcqCfg->i64TriggerDelay )
		{
			r = m_pAdq->SetTriggerHoldOffSamples(uInt32(pAcqCfg->i64TriggerDelay));
			if(m_bDisplayTrace) 
			{
				char str[128];
				::_stprintf_s( str, 128,  "m_pAdq->SetTriggerHoldOffSamples(%d);\n", uInt32(pAcqCfg->i64TriggerDelay) );
				OutputDebugString(str);
			}
		}
		else
		{
			r = m_pAdq->SetPreTrigSamples( uInt32(pAcqCfg->i64TriggerHoldoff) );
			if(m_bDisplayTrace) 
			{
				char str[128];
				::_stprintf_s( str, 128,  "m_pAdq->SetPreTrigSamples(%d);\n", uInt32(pAcqCfg->i64TriggerHoldoff) );
				OutputDebugString(str);
			}
		}

		//MultiRecordSetup will be done in configure trigger after determination of the m_i64TrigAddrAdj
		m_Acq = *pAcqCfg;
		m_Acq.i32SampleOffset= CS_SAMPLE_OFFSET_16;		
		m_Acq.i64TriggerTimeout = -1;	
		m_Acq.u32TrigEnginesEn = 0;
		m_Acq.u32SampleBits = m_Info.usBits;
		m_Acq.i32SampleRes = m_Info.lResolution;
		m_Acq.u32SampleSize = CS_SAMPLE_SIZE_2;
		m_Acq.u32ExtClkSampleSkip = 1 == m_Info.usChanNumber ? CS_SAMPLESKIP_FACTOR/2 : CS_SAMPLESKIP_FACTOR;
	}
	return bChanged? CS_CONFIG_CHANGED: CS_SUCCESS;
}

int32 CsSpDevice::ConfigureChan( PCSCHANNELCONFIG aChanCfg, bool bCoerce, bool bValidateOnly)
{
	bool bChanged = false;
	///Channel configuration
	if( NULL == aChanCfg )
		return CS_NULL_POINTER;
	if( ::IsBadReadPtr( aChanCfg, m_Info.usChanNumber*sizeof(CSCHANNELCONFIG) ) )
		return CS_INVALID_STRUCT_SIZE;
	
	if( 0 == aChanCfg->u32ChannelIndex )
		if( bCoerce ){bChanged = true; aChanCfg->u32ChannelIndex = CS_CHAN_1;}
		else return CS_INVALID_CHANNEL;
	if( m_Info.usChanNumber < aChanCfg->u32ChannelIndex )
		if( bCoerce ){bChanged = true; aChanCfg->u32ChannelIndex = m_Info.usChanNumber;}
		else return CS_INVALID_CHANNEL;

	uInt32 u32 = aChanCfg->u32ChannelIndex - 1;

	if( aChanCfg->u32Size < sizeof( CSCHANNELCONFIG ) )
		return CS_INVALID_STRUCT_SIZE;
	if( ::IsBadReadPtr( aChanCfg, sizeof(CSCHANNELCONFIG) ) )
		return CS_INVALID_PARAMETER;

	uInt32 u32Ir = SPD_INPUT_RANGE;
	uInt32 u32Term = CS_COUPLING_AC;
	if( m_bDcInput )
	{
		u32Ir = SPD_DC_INPUT_RANGE;
		u32Term = CS_COUPLING_DC;
	}

	if( !m_bDcAcInput )
	{
		if( u32Term != aChanCfg->u32Term )
			if( bCoerce ){bChanged = true; aChanCfg->u32Term = u32Term;}
			else return CS_INVALID_COUPLING;
	}
	else
	{
		if( CS_COUPLING_AC != aChanCfg->u32Term && CS_COUPLING_DC != aChanCfg->u32Term )
			if( bCoerce ){bChanged = true; aChanCfg->u32Term = u32Term; }
			else return CS_INVALID_COUPLING;
	}

	if( u32Ir != aChanCfg->u32InputRange )
		if( bCoerce ){bChanged = true; aChanCfg->u32InputRange = u32Ir;}
		else return CS_INVALID_GAIN;

	if( CS_REAL_IMP_50_OHM != aChanCfg->u32Impedance )
		if( bCoerce ){bChanged = true; aChanCfg->u32Impedance = CS_REAL_IMP_50_OHM;}
		else return CS_INVALID_IMPEDANCE;

	if( 0 != aChanCfg->i32DcOffset )
		if( bCoerce ){bChanged = true; aChanCfg->i32DcOffset = 0;}
		else return CS_INVALID_POSITION;

	if( 0 != aChanCfg->u32Filter )
		if( bCoerce ){bChanged = true; aChanCfg->u32Filter = 0;}
		else return CS_INVALID_FILTER;
	
	aChanCfg->i32Calib = 0;
	if( !bValidateOnly )
	{
		if( m_bDcAcInput )
		{
			uInt32 u32ChanMask = SPD_CHAN_AFE_MASK;
			if( CS_CHAN_1 == aChanCfg->u32ChannelIndex ) //Channel indexes are reversed on gage version of SPDev
				u32ChanMask <<= 1; 
			m_u32AFE &= ~u32ChanMask;
			if( aChanCfg->u32Term == CS_COUPLING_DC)
				m_u32AFE |= u32ChanMask;
			m_pAdq->SetAfeSwitch( m_u32AFE );
			if(m_bDisplayTrace) 
			{
				char str[128];
				::_stprintf_s( str, 128,  "m_pAdq->SetAfeSwitch(0x%x);\n", m_u32AFE );
				OutputDebugString(str);
			}
		}
		m_aChan[u32] = *aChanCfg;
	}

	return bChanged? CS_CONFIG_CHANGED: CS_SUCCESS;
}

int32 CsSpDevice::ConfigureTrig( PCSTRIGGERCONFIG pTrigCfg, bool bCoerce, bool bValidateOnly)
{
	bool bChanged = false;
	///Trigger configuration
	if( NULL == pTrigCfg )
		return CS_NULL_POINTER;
	if( ::IsBadReadPtr( pTrigCfg, sizeof(uInt32) ) || pTrigCfg->u32Size < sizeof(CSTRIGGERCONFIG) )
		return CS_INVALID_STRUCT_SIZE;
	if( ::IsBadWritePtr( pTrigCfg, sizeof(CSTRIGGERCONFIG) ) )
		return CS_INVALID_PARAMETER;

	if(  1 != pTrigCfg->u32TriggerIndex )
		if( bCoerce ){bChanged = true; pTrigCfg->u32TriggerIndex = 1;}
		else return CS_INVALID_TRIGGER;

	CSTRIGGERCONFIG TempCfg = {0};
	TempCfg.u32TriggerIndex = 1;
	TempCfg.u32Size = sizeof(TempCfg);
	TempCfg.u32ExtCoupling = CS_COUPLING_DC;
	TempCfg.u32ExtTriggerRange = CS_GAIN_2_V;
	TempCfg.u32ExtImpedance = CS_REAL_IMP_50_OHM;
	
	int r(0);
	if( pTrigCfg->i32Source <= CS_TRIG_SOURCE_EXT )
	{
		TempCfg.i32Source = CS_TRIG_SOURCE_EXT;
		TempCfg.i32Level = SPD_EXT_TRIG_LEVEL; //this lines provides better report of actual trigger level in ext.trig source
		//TempCfg.i32Level = m_Trig.i32Level;  //this lines provides better switchin betwen sources. Level of Chan trigger will preserved
		TempCfg.u32Condition = CS_TRIG_COND_POS_SLOPE;
		if( 0 != memcmp( &TempCfg, pTrigCfg, sizeof( CSTRIGGERCONFIG)) )
			bChanged = true;
		*pTrigCfg = TempCfg;

		if( !bValidateOnly )
		{
			m_i64TrigAddrAdj = m_Info.lExtTrigAddrAdj;
			unsigned int uSkip = m_pAdq->GetSampleSkip();
			if( 0 == uSkip ) uSkip = 1;
			m_i64TrigAddrAdj /= uSkip;

			r = m_pAdq->SetTriggerMode( 2 );
			if(m_bDisplayTrace) 
				OutputDebugString("m_pAdq->SetTriggerMode( 2 )\n");
			m_Trig = TempCfg;
		}
	}
	else if( pTrigCfg->i32Source == CS_TRIG_SOURCE_DISABLE )
	{
		TempCfg.i32Source = CS_TRIG_SOURCE_DISABLE;
		TempCfg.i32Level = 0;
		TempCfg.u32Condition = CS_TRIG_COND_POS_SLOPE;
		if( 0 != memcmp( &TempCfg, pTrigCfg, sizeof( CSTRIGGERCONFIG)) )
			bChanged = true;
		*pTrigCfg = TempCfg;

		if( !bValidateOnly )
		{
			m_i64TrigAddrAdj = 0;
			r = m_pAdq->SetTriggerMode( 1 );
			if(m_bDisplayTrace) 
				OutputDebugString("m_pAdq->SetTriggerMode( 1 )\n");
						
			m_Trig = TempCfg;
		}
	}
	else
	{
		if( pTrigCfg->i32Source > m_Info.usChanNumber )
			if( bCoerce ){bChanged = true; pTrigCfg->i32Source = CS_CHAN_1;}
			else return CS_INVALID_TRIG_SOURCE;
		if( pTrigCfg->i32Level > 100 )
			if( bCoerce ){bChanged = true; pTrigCfg->i32Level = 100;}
			else return CS_INVALID_TRIG_LEVEL;
		if( pTrigCfg->i32Level < -100 )
			if( bCoerce ){bChanged = true; pTrigCfg->i32Level = -100;}
			else return CS_INVALID_TRIG_LEVEL;
		if( pTrigCfg->u32Condition != CS_TRIG_COND_POS_SLOPE && pTrigCfg->u32Condition != CS_TRIG_COND_NEG_SLOPE )
			if( bCoerce ){bChanged = true; pTrigCfg->u32Condition = CS_TRIG_COND_POS_SLOPE;}
			else return CS_INVALID_TRIG_COND;

		TempCfg.i32Source = pTrigCfg->i32Source;
		TempCfg.i32Level = pTrigCfg->i32Level;
		TempCfg.u32Condition = pTrigCfg->u32Condition;
		
		if( 0 != memcmp( &TempCfg, pTrigCfg, sizeof( CSTRIGGERCONFIG)) )
			bChanged = true;
		*pTrigCfg = TempCfg;

		if( !bValidateOnly )
		{
			m_i64TrigAddrAdj = 0;
			r = m_pAdq->SetTriggerMode( 3 );
			if(m_bDisplayTrace) 
				OutputDebugString("m_pAdq->SetTriggerMode( 3 )\n");

			int nLevel = (-m_Info.lResolution * pTrigCfg->i32Level )/100;
			r = m_pAdq->SetTrigLevelResetValue( m_nTrigSense );
			r = m_pAdq->SetLvlTrigLevel( nLevel );
			r = m_pAdq->SetLvlTrigEdge( CS_TRIG_COND_POS_SLOPE == pTrigCfg->u32Condition? 1 : 0 );
			r = m_pAdq->SetLvlTrigChannel( CS_CHAN_1 == ConvertToHw(uInt16(pTrigCfg->i32Source)) ? 1 : 2 ); 
			
			if(m_bDisplayTrace) 
			{
				char str[128];
				::_stprintf_s( str, 128,  "m_pAdq->SetTrigLevelResetValue(%d);\n", m_nTrigSense );
				OutputDebugString(str);
				::_stprintf_s( str, 128,  "m_pAdq->SetLvlTrigLevel( %d )\n", nLevel );
				::OutputDebugString(str);
				::_stprintf_s( str, 128,  "m_pAdq->SetLvlTrigEdge( %d )\n", CS_TRIG_COND_POS_SLOPE == pTrigCfg->u32Condition? 1 : 0 );
				::OutputDebugString(str);
				::_stprintf_s( str, 128,  "m_pAdq->SetLvlTrigChannel( %d )\n", CS_CHAN_1 == ConvertToHw(uInt16(pTrigCfg->i32Source)) ? 1 : 2 );
				::OutputDebugString(str);
			}
			m_Trig = TempCfg;
		}
	}

	//Configure record after m_i64TrigAddrAdj is determined
	if( !bValidateOnly )
	{
		r = m_pAdq->MultiRecordSetup( m_Acq.u32SegmentCount, uInt32(m_Acq.i64SegmentSize + m_i64TrigAddrAdj) );
		if(m_bDisplayTrace) 
		{
			char str[128];
			::_stprintf_s( str, 128,  "Trigger addres adjust = %d\n", m_i64TrigAddrAdj );
			OutputDebugString(str);
			::_stprintf_s( str, 128,  "m_pAdq->MultiRecordSetup(%d, %d);\n", m_Acq.u32SegmentCount, uInt32(m_Acq.i64SegmentSize + m_i64TrigAddrAdj) );
			OutputDebugString(str);
		}
	}

	return bChanged? CS_CONFIG_CHANGED: CS_SUCCESS;
}

int32 CsSpDevice::GetAcquisitionSystemCaps(uInt32 u32CapsId, PCSSYSTEMCONFIG , void* pBuffer, uInt32* pu32BufferSize)
{
	if( NULL != pBuffer )
	{
		if (NULL == pu32BufferSize)
			return (CS_INVALID_PARAMETER);
		if ( ::IsBadWritePtr( pu32BufferSize, sizeof( uInt32) ) )
			return CS_INVALID_POINTER_BUFFER;
	}

	bool bExtTrig = 0 == (u32CapsId & SUBCAPS_ID_MASK);

	switch(u32CapsId & CAPS_ID_MASK)
	{
		case CAPS_SAMPLE_RATES:
			*pu32BufferSize = (uInt32)(sizeof(CSSAMPLERATETABLE)*m_szSrTableSize);
			if(NULL != pBuffer)
			{
				if( *pu32BufferSize < (uInt32)(sizeof(CSSAMPLERATETABLE)*m_szSrTableSize) || ::IsBadWritePtr( pBuffer, sizeof(CSRANGETABLE)* m_szSrTableSize ) )
					return CS_BUFFER_TOO_SMALL;
				CSSAMPLERATETABLE* pTbl = (CSSAMPLERATETABLE*)pBuffer;
				for( size_t sz = 0; sz < m_szSrTableSize; sz++ )
				{
					pTbl->i64SampleRate = m_pSrTable[sz].i64Sr;
					if ( pTbl->i64SampleRate >= 1000000000 )
						::_stprintf_s( pTbl->strText, sizeof(pTbl->strText), "%d.%d GS/s", int32(pTbl->i64SampleRate/1000000000), int32(pTbl->i64SampleRate/100000000)%10 );
					else if ( pTbl->i64SampleRate >= 1000000 )
						::_stprintf_s( pTbl->strText, sizeof(pTbl->strText), "%d.%d MS/s", int32(pTbl->i64SampleRate/1000000), int32(pTbl->i64SampleRate/100000)%10 );
					else if ( pTbl->i64SampleRate >= 1000 )
						::_stprintf_s( pTbl->strText, sizeof(pTbl->strText), "%d.%d kS/s", int32(pTbl->i64SampleRate/1000), int32(pTbl->i64SampleRate/100)%10 );
					else
						::_stprintf_s( pTbl->strText, sizeof(pTbl->strText), "%d S/s", int32(pTbl->i64SampleRate) );
					pTbl++;
				}
			}
			return CS_SUCCESS;
		
		case CAPS_INPUT_RANGES:
			{
				uInt32 u32TableSize = sizeof(CSRANGETABLE);
				*pu32BufferSize = u32TableSize;
				if(NULL != pBuffer)
				{
					if( *pu32BufferSize < u32TableSize || ::IsBadWritePtr( pBuffer, u32TableSize ) )
						return CS_BUFFER_TOO_SMALL;
					CSRANGETABLE tbl;
					tbl.u32InputRange = bExtTrig? SPD_EXTTRIG_RANGE : (m_bDcInput? SPD_DC_INPUT_RANGE : SPD_INPUT_RANGE);
					tbl.u32Reserved = CS_IMP_50_OHM;
					::_stprintf_s( tbl.strText, sizeof(tbl.strText),  m_bDcInput ? "±125 mV":"±1.1 V"  );
					::CopyMemory( pBuffer, &tbl, sizeof(CSRANGETABLE) );
				}
			}
			return CS_SUCCESS;
					
		case CAPS_IMPEDANCES:
			*pu32BufferSize = sizeof(CSIMPEDANCETABLE);
			if(NULL != pBuffer)
			{
				if( *pu32BufferSize < sizeof(CSIMPEDANCETABLE) || ::IsBadWritePtr( pBuffer, sizeof(CSIMPEDANCETABLE)) )
					return CS_BUFFER_TOO_SMALL;
				CSIMPEDANCETABLE tbl;
				tbl.u32Imdepdance = CS_REAL_IMP_50_OHM;
				tbl.u32Reserved = 0;
				::_stprintf_s( tbl.strText, sizeof(tbl.strText),  "50 Ohm" );
				::CopyMemory( pBuffer, &tbl, sizeof(CSIMPEDANCETABLE) );
			}
			return CS_SUCCESS;
					
		case CAPS_COUPLINGS:
			{
				uInt32 u32TableSize = sizeof(CSCOUPLINGTABLE);
				if( m_bDcAcInput ) u32TableSize *= 2;
				*pu32BufferSize = u32TableSize;
				if(NULL != pBuffer)
				{
					if( *pu32BufferSize < u32TableSize || ::IsBadWritePtr( pBuffer, u32TableSize ) )
						return CS_BUFFER_TOO_SMALL;
					CSCOUPLINGTABLE tbl;
					tbl.u32Coupling = m_bDcInput?CS_COUPLING_DC:CS_COUPLING_AC;
					tbl.u32Reserved = 0;
					::_stprintf_s( tbl.strText, sizeof(tbl.strText),  m_bDcInput ? "DC" : "AC" );
					::CopyMemory( pBuffer, &tbl, sizeof(CSCOUPLINGTABLE) );
					if( m_bDcAcInput  )
					{
						tbl.u32Coupling = CS_COUPLING_DC ;
						tbl.u32Reserved = 0;
						::_stprintf_s( tbl.strText, sizeof(tbl.strText), "DC" );
						::CopyMemory( ((PCSCOUPLINGTABLE)pBuffer)+1, &tbl, sizeof(CSCOUPLINGTABLE) );
					}
				}
			}
			return CS_SUCCESS;							
			
		case CAPS_ACQ_MODES:
			*pu32BufferSize = sizeof(uInt32);
			if(NULL != pBuffer)
			{
				if( *pu32BufferSize < sizeof(uInt32) || ::IsBadWritePtr( pBuffer, sizeof(uInt32)) )
					return CS_BUFFER_TOO_SMALL;
				uInt32 u32AllowedModes = 1 == m_Info.usChanNumber ? CS_MODE_SINGLE : CS_MODE_DUAL;
				u32AllowedModes |= CS_MODE_REFERENCE_CLK;
				*((uInt32*)pBuffer) = u32AllowedModes;
			}
			return CS_SUCCESS;
					
		case CAPS_TERMINATIONS:
			*pu32BufferSize = sizeof(uInt32);
			if(NULL != pBuffer)
			{
				if( *pu32BufferSize < sizeof(uInt32) || ::IsBadWritePtr( pBuffer, sizeof(uInt32)) )
					return CS_BUFFER_TOO_SMALL;
				*((uInt32*)pBuffer) = CS_COUPLING_AC;
			}
			return CS_SUCCESS;

		case CAPS_FLEXIBLE_TRIGGER:
			*pu32BufferSize = sizeof(uInt32);
			if(NULL != pBuffer)
			{
				if( *pu32BufferSize < sizeof(uInt32) || ::IsBadWritePtr( pBuffer, sizeof(uInt32)) )
					return CS_BUFFER_TOO_SMALL;
				*((uInt32*)pBuffer) = 0;
			}
			return CS_SUCCESS;

		case CAPS_BOARD_TRIGGER_ENGINES:
			*pu32BufferSize = sizeof(uInt32);
			if(NULL != pBuffer)
			{
				if( *pu32BufferSize < sizeof(uInt32) || ::IsBadWritePtr( pBuffer, sizeof(uInt32)) )
					return CS_BUFFER_TOO_SMALL;
				*((uInt32*)pBuffer) = 1;
			}
			return CS_SUCCESS;
	
		case CAPS_TRIGGER_SOURCES:
			*pu32BufferSize = sizeof(int32)*(1==m_Info.usChanNumber ? 3 : 4);
			if(NULL != pBuffer)
			{
				if( *pu32BufferSize < sizeof(int32)*(1==m_Info.usChanNumber ? 3 : 4) || ::IsBadWritePtr( pBuffer, sizeof(int32)*(1==m_Info.usChanNumber ? 3 : 4) ) )
					return CS_BUFFER_TOO_SMALL;
				int32* p = (int32*)pBuffer;
				*p++ = CS_TRIG_SOURCE_DISABLE;
				*p++ = CS_TRIG_SOURCE_EXT;
				*p++ = CS_TRIG_SOURCE_CHAN_1;
				if( m_Info.usChanNumber > 1 )
					*p++ = CS_TRIG_SOURCE_CHAN_2;
			}
			return CS_SUCCESS;

		case CAPS_MAX_SEGMENT_PADDING:
			*pu32BufferSize = sizeof(uInt32);
			if(NULL != pBuffer)
			{
				if( *pu32BufferSize < sizeof(uInt32) || ::IsBadWritePtr( pBuffer, sizeof(uInt32)) )
					return CS_BUFFER_TOO_SMALL;
				*((uInt32*)pBuffer) = (SPD_ALINGMENT * sizeof(char) + SPD_ITRA_RECORD * sizeof(uInt32))/sizeof(int16);
			}
			return CS_SUCCESS;

		case CAPS_MULREC:
		case CAPS_TRANSFER_EX:
		case CAPS_CLK_IN:
			return CS_SUCCESS;
		
		case CAPS_TRIGGER_RES:
			*pu32BufferSize = sizeof(uInt32);
			if(NULL != pBuffer)
			{
				if( *pu32BufferSize < sizeof(uInt32) || ::IsBadWritePtr( pBuffer, sizeof(uInt32)) )
					return CS_BUFFER_TOO_SMALL;
				*((uInt32*)pBuffer) = SPD_ITRA_RECORD * sizeof(uInt32)/sizeof(int16);
			}
			return CS_SUCCESS;
		case CAPS_MIN_EXT_RATE:
			*pu32BufferSize = sizeof(int64);
			if(NULL != pBuffer)
			{
				if( *pu32BufferSize < sizeof(int64) || ::IsBadWritePtr( pBuffer, sizeof(int64)) )
					return CS_BUFFER_TOO_SMALL;
				*((int64*)pBuffer) = SPD_MIN_EXT_CLK;
			}
			return CS_SUCCESS;
		case CAPS_MAX_EXT_RATE:
			*pu32BufferSize = sizeof(int64);
			if(NULL != pBuffer)
			{
				if( *pu32BufferSize < sizeof(int64) || ::IsBadWritePtr( pBuffer, sizeof(int64)) )
					return CS_BUFFER_TOO_SMALL;
				int64 i64MaxRate = GetMaxSr();
				if( 1 == m_Info.usChanNumber )i64MaxRate /= 2;
				*((int64*)pBuffer) = i64MaxRate;
			}
			return CS_SUCCESS;
		
		case CAPS_SKIP_COUNT:
			*pu32BufferSize = sizeof(uInt32);
			if(NULL != pBuffer)
			{
				if( *pu32BufferSize < sizeof(uInt32) || ::IsBadWritePtr( pBuffer, sizeof(uInt32)) )
					return CS_BUFFER_TOO_SMALL;
				*((uInt32*)pBuffer) = 1 == m_Info.usChanNumber ? CS_SAMPLESKIP_FACTOR/2 : CS_SAMPLESKIP_FACTOR;
			}
			return CS_SUCCESS;

		default:
			return CS_INVALID_REQUEST;
	}
}

