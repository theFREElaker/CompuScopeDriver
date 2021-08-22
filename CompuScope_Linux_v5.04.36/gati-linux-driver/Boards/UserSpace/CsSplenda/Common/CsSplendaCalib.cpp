//	CsxyG8Hw.cpp
///	Implementation of CsSplendaDev class
//
//	This file contains all member functions that access to addon hardware including
//	Frontend set up and calibration
//
//

#include "StdAfx.h"
#include "CsSplenda.h"
#include "CsSplendaCal.h"
#include "CsEventLog.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#else
#include "GageCfgParser.h"
#endif

#define	 CSPLNDA_CALIB_TIMEOUT_LONG	KTIME_SECOND(1)			// 1 second timeout
#define	 CSPLNDA_CALIB_TIMEOUT_SHORT	KTIME_MILISEC(300)		// 300 milisecond timeout

#define	 CSPLNDA_CALIB_TIMEOUT	KTIME_MILISEC(300)		// 300 milisecond timeout

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSplendaDev::CsGetAverage(uInt16 u16Channel, int16* pi16Buffer, uInt32 u32BufferSizeSamples, int32* pi32Avrage_x10)
{
	int64	i64StartPos = {0};			// Relative to trigger position
		
	in_PREPARE_DATATRANSFER		InXferEx;
	out_PREPARE_DATATRANSFER	OutXferEx;

	::GageZeroMemoryX( &InXferEx, sizeof(InXferEx));
	InXferEx.u32Segment		= 1;
	InXferEx.u16Channel		= u16Channel;
	InXferEx.i64StartAddr	= i64StartPos;
	InXferEx.i32OffsetAdj	= GetTriggerAddressOffsetAdjust(  m_pCardState->AcqConfig.u32Mode, (uInt32) m_pCardState->AcqConfig.i64SampleRate );
	InXferEx.u32DataMask	= 0;
	InXferEx.u32FifoMode	= GetFifoModeTransfer(u16Channel);
	InXferEx.bBusMasterDma	= m_BusMasterSupport;
	InXferEx.u32SampleSize  = sizeof(uInt16);

	IoctlPreDataTransfer( &InXferEx, &OutXferEx );
	CardReadMemory( pi16Buffer, u32BufferSizeSamples * sizeof(uInt16) );

	int32 i32Sum = 0;
	int32 i32Count = 0;
	const int32 c32SampleOffset = m_pCardState->AcqConfig.i32SampleOffset;
	
	for (uInt32 u32Addr = CSPLNDA_CALIB_BUF_MARGIN;  u32Addr < u32BufferSizeSamples; i32Count++)
	{
		i32Sum = i32Sum + pi16Buffer[u32Addr++] - c32SampleOffset;
	}

	*pi32Avrage_x10  = ( i32Sum * 10 / i32Count );  
		
	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsSplendaDev::AcquireAndAverage(uInt16	u16Channel, int32* pi32Avrage_x10)
{
	CSTIMER		CsTimeout;

	AcquireData(MAOINT);
	ForceTrigger();

	CsTimeout.Set( CSPLNDA_CALIB_TIMEOUT_LONG );
	while( STILL_BUSY == ReadBusyStatus() )
	{
		ForceTrigger();
		if ( CsTimeout.State() )
		{
			if( STILL_BUSY == ReadBusyStatus() )
			{
				Abort();
				return CS_CAL_BUSY_TIMEOUT;
			}
		}
		BBtiming(1);
	}

	if ( m_MasterIndependent->m_pCardState->bAlarm )
	{
		if ( GetChannelProtectionStatus() )
		{
			ReportProtectionFault();
			return  CS_CHANNEL_PROTECT_FAULT;
		}
	}

	//Calculate logical channel
	uInt16 u16UserChannel = ConvertToUserChannel(u16Channel);
	int32 i32Ret = CsGetAverage( u16UserChannel, m_pi16CalibBuffer, CSPLNDA_CALIB_DEPTH, pi32Avrage_x10);
	return  i32Ret;
}


int32 CsSplendaDev::SendCalibDC(uInt32 u32Imped, uInt32 u32Range, int32 i32Level, int32* pi32SetLevel_uV, bool bCheckRef )
{
	SPLENDA_CAL_LUT CalInfo;
	uInt32 u32CalRange = u32Range;
	if( 0 == u32Range  )
		return CS_INVALID_GAIN;
	if( 0 == i32Level )
		u32CalRange = 0;

	int32 i32Ret = FindCalLutItem(u32CalRange, u32Imped, CalInfo);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	uInt16 u16Data(0);
	int32 i32SetLevel(0);
	if( i32Level > 0 )
	{
		u16Data = CalInfo.u16PosRegMask;
		i32SetLevel = CalInfo.i32PosLevel_uV;
	}
	else 
	{
		u16Data = CalInfo.u16NegRegMask;
		i32SetLevel = CalInfo.i32NegLevel_uV;
	}
	
	i32Ret = WriteRegisterThruNios(u16Data, CSPLNDA_VSC_SET_WR_CMD);
	m_pCardState->AddOnReg.u16VSC = u16Data;
	BBtiming( CS_REAL_IMP_50_OHM == u32Imped ? m_u32CalSrcSettleDelay : m_u32CalSrcSettleDelay_1MOhm);

	if( NULL != pi32SetLevel_uV )
		*pi32SetLevel_uV = i32SetLevel;

	int32 i32V_uV;
	i32Ret = ReadCalibA2D(i32SetLevel/1000, eciCalSrc, &i32V_uV);
	if( CS_FAILED(i32Ret) || (CS_FALSE == i32Ret) ) return i32Ret;

	TraceCalib( u16Data, TRACE_CAL_SIGNAL, u32Imped, u32Range, i32SetLevel, i32V_uV);

	int32 i32MaxError = int32(u32Range * CSPLNDA_MAX_CALREF_ERR_P)/100; //5% of the range
	i32MaxError = Max(i32MaxError, CSPLNDA_MAX_CALREF_ERR_C); // Constant error
	i32MaxError *= 1000; //in uV

	if( labs(i32SetLevel - i32V_uV ) > i32MaxError )
	{
		char szErrorStr[128];
		sprintf_s( szErrorStr, sizeof(szErrorStr), "%d (expected %d, measured %d)", CS_DAC_CALIB_FAILURE, i32SetLevel/1000, i32V_uV/1000 );
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );
		return CS_DAC_CALIB_FAILURE;
	}

	if( NULL != pi32SetLevel_uV )
		*pi32SetLevel_uV = i32SetLevel;
	if( bCheckRef )
	{
		int32 i32RefUsed(0);
		switch(u16Data & 0x60 )
		{
			default:
			case 0x00: i32RefUsed = CSPLNDA_CALREF0; break;
			case 0x20: i32RefUsed = CSPLNDA_CALREF1; break;
			case 0x40: i32RefUsed = CSPLNDA_CALREF2; break;
			case 0x60: i32RefUsed = CSPLNDA_CALREF3; break;
		}

		i32Ret = ReadCalibA2D(i32RefUsed, eciDirectRef, &i32V_uV);
		if( CS_FAILED(i32Ret) ) return i32Ret;

		//Return CalAdc input to default position
		i32Ret = ReadCalibA2D(0, eciCalSrc);
		if( CS_FAILED(i32Ret) ) return i32Ret;

		i32MaxError = int32(i32RefUsed * CSPLNDA_MAX_CALREF_ERR_P)/100; //5% of the reference
		i32MaxError = Max(i32MaxError, CSPLNDA_MAX_CALREF_ERR_C); // Constant error
		if( i32MaxError < i32RefUsed - i32V_uV/1000 )
		{
			char szErrorStr[128];
			sprintf_s( szErrorStr, sizeof(szErrorStr),  "%d (expected %d, measured %d)", CS_CALIB_REF_FAILURE, i32RefUsed, i32V_uV/1000 );
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );
			return CS_CALIB_REF_FAILURE;
		}
	}
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//----- ReadCalibA2D
//		returns in uV
//-----------------------------------------------------------------------------

int32 CsSplendaDev::ReadCalibA2D(int32 i32Expected_mV, eCalInput eInput, int32* pi32V_uv, int32* pi32Noise_01LSB)
{
	if(NULL == m_pi32CalibA2DBuffer )
		return CS_FALSE;

	if( eciDirectRef == eInput )
	{
		switch(m_pCardState->AddOnReg.u16VSC & 0x60 )
		{
			default:
			case 0x00: i32Expected_mV = CSPLNDA_CALREF0; break;
			case 0x20: i32Expected_mV = CSPLNDA_CALREF1; break;
			case 0x40: i32Expected_mV = CSPLNDA_CALREF2; break;
			case 0x60: i32Expected_mV = CSPLNDA_CALREF3; break;
		}
	}

//Set Range	 and input
	uInt32 u32DataCtl = ADC7321_CTRL;
	uInt32 u32DataRange = 0, u32Range_mV = 0;;
	int32 i32Ret;
	if( labs(i32Expected_mV) < CSPLNDA_CALA2D_2p5_V )
	{
		u32Range_mV = CSPLNDA_CALA2D_2p5_V;
		u32DataRange = ADC7321_CH0_2p5_V;
	}
	else if( labs(i32Expected_mV) < CSPLNDA_CALA2D_5_V )
	{
		u32Range_mV = CSPLNDA_CALA2D_5_V;
		u32DataRange = ADC7321_CH0_5_V;
	}
	else 
	{
		u32Range_mV = CSPLNDA_CALA2D_10_V;
		u32DataRange = ADC7321_CH0_10_V;
	}
	
	switch( eInput )
	{
		case eciDirectRef: break;
		case eciCalChan:   m_pCardState->AddOnReg.u16VSC |= VSC_CHAN_SRC;
						   u32DataRange >>= 4; //Channel 1 range is the same decoding but 4 bits lower
		                   u32DataCtl |= ADC7321_CHAN;
						   break;
						   
		case eciCalSrc:	   m_pCardState->AddOnReg.u16VSC &= ~VSC_CHAN_SRC;	
						   u32DataRange >>= 4; //Channel 1 range is the same decoding but 4 bits lower
		                   u32DataCtl |= ADC7321_CHAN;
						   break;
	}

	i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16VSC, CSPLNDA_VSC_SET_WR_CMD);
	BBtiming(m_u32CalSrcRelayDelay);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	u32DataRange |= ADC7321_RANGE;
	u32DataRange |= (u32DataCtl<<16);
//Configure ADC
	WriteGIO(CSPLNDA_AD7321_CFG_REG,u32DataRange);
//Read ADC
	WriteGIO(CSPLNDA_AD7321_WRITE_REG,CSPLNDA_AD7321_RESTART);
	WriteGIO(CSPLNDA_AD7321_WRITE_REG,0 );

//Proceed if the reading was requested not only the configuration change
	uInt32 u32Code(0);
	CSTIMER	CsTimeout;
	CsTimeout.Set( CSPLNDA_CALIB_TIMEOUT );
	for(;;)
	{
		u32Code = ReadGIO(CSPLNDA_AD7321_READ_REG);
		if( 0 != (u32Code & CSPLNDA_AD7321_RD_FIFO_FULL) )
			break;
		else if ( CsTimeout.State() )
		{
			u32Code = ReadGIO(CSPLNDA_AD7321_READ_REG);
			if( 0 != (u32Code & CSPLNDA_AD7321_RD_FIFO_FULL) )	
				break;
			else
				return CS_CALIB_ADC_CAPTURE_FAILURE;
		}
		else
			BBtiming(1);
	}
	
		//Switch back  source relay
	if( eciCalChan == eInput )
	{
		m_pCardState->AddOnReg.u16VSC |= VSC_CHAN_SRC;
		i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16VSC, CSPLNDA_VSC_SET_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		BBtiming(m_u32CalSrcRelayDelay);
	}
	
	LONGLONG	llSum = 0;
	int			i = 0;
	int			nValidSamples = 0;
	for( i = 0, nValidSamples = 0; i < CSPLNDA_CALIB_BLOCK_SIZE; i++ )
	{
		WriteGIO(CSPLNDA_AD7321_WRITE_REG,CSPLNDA_AD7321_READ);
		WriteGIO(CSPLNDA_AD7321_WRITE_REG,0 );
		u32Code = ReadGIO(CSPLNDA_AD7321_READ_REG);
		if( 0 == (u32Code & CSPLNDA_AD7321_DATA_READ) )
			continue;
		int16 i16Code = int16(u32Code & 0x1fff);
		i16Code <<= 3; i16Code >>= 3; //sign extention
		TraceCalib( 0, TRACE_CAL_SIGNAL_ITER, i, nValidSamples, i16Code);
		m_pi32CalibA2DBuffer[nValidSamples++] = i16Code;
		llSum += LONGLONG(i16Code);
	}

	if ( nValidSamples < 2 ) return CS_CALIB_ADC_READ_FAILURE;

	LONGLONG llAvg_x10 = (llSum*10)/nValidSamples;
	llSum *= LONGLONG(1000*u32Range_mV);
	llSum += llSum > 0 ? LONGLONG(nValidSamples*CSPLNDA_AD7321_CODE_SPAN)/2 : -LONGLONG(nValidSamples*CSPLNDA_AD7321_CODE_SPAN)/2;
	llSum /= LONGLONG(nValidSamples*CSPLNDA_AD7321_CODE_SPAN);
	if( NULL != pi32V_uv)
	{
		*pi32V_uv = int32(llSum);

#if DBG
#else
		if( NULL != pi32Noise_01LSB )
#endif
		{
			LONGLONG llSum2 = 0;
			int32 i32Final = m_pi32CalibA2DBuffer[nValidSamples-1];
			uInt16 u16SettleCount = 0xffff;
			for( i = 0; i < nValidSamples; i++ )
			{
				int32 i32Err = m_pi32CalibA2DBuffer[i] - i32Final;

				if( 0xffff == u16SettleCount && (labs(i32Err) < 2) )
					u16SettleCount = uInt16(i);
				llSum2 += Abs(llAvg_x10 - m_pi32CalibA2DBuffer[i]*10);
			}

			llSum2 /= nValidSamples;
			if( NULL != pi32Noise_01LSB )
				*pi32Noise_01LSB = int32(llSum2);
			TraceCalib( u16SettleCount, TRACE_CAL_RANGE, u32Range_mV, uInt32(eInput), int32(llSum), int32(llSum2));
		}
	}

	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsSplendaDev::CalibrateFrontEnd(uInt16 u16Channel, uInt32* pu32SuccessFlag)
{
	*pu32SuccessFlag = CSPLNDA_CAL_RESTORE_ALL;
	if( !m_pCardState->bCalNeeded[u16Channel-1] )
		return CS_SUCCESS;

	if ( 0 != m_pCardState->u8ImageId )
	{
		// Never calibrate with expert images.
		m_pCardState->bCalNeeded[u16Channel-1] = false;
		m_bForceCal[u16Channel-1] = false;
		return CS_SUCCESS;
	}
	int32 i32Ret(CS_SUCCESS);

	if( m_bUseEepromCal )
	{
		i32Ret = EepromCalibrationSettings(u16Channel);
		if( CS_FAILED(i32Ret) )	return CS_FAST_CALIB_FAILURE;
		else if( CS_SUCCESS == i32Ret )	return CS_SUCCESS; //if i32Ret == CS_FALSE need to calibrate
	}

	if ( m_bSkipCalib && !m_bForceCal[u16Channel-1] )
	{
		TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 2, 0, 0);
		m_pCardState->bFastCalibSettingsDone[u16Channel-1] = false;
		i32Ret = FastCalibrationSettings(u16Channel);
		if( CS_FAILED(i32Ret) )	return CS_FAST_CALIB_FAILURE;
		else if( CS_SUCCESS == i32Ret )	return CS_SUCCESS; //if i32Ret == CS_FALSE need to calibrate
	}
	TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 20);

	i32Ret = CalibrateOffset(u16Channel, pu32SuccessFlag);
	if( CS_FAILED(i32Ret) )
	{
		SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_200_MV, CSPLNDA_GND_CAL_LEVEL);
		TraceCalib( u16Channel, TRACE_CAL_RESULT);
		TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 1001, 0, i32Ret);
		SendCalibModeSetting(u16Channel, ecmCalModeNormal);
		m_bForceCal[u16Channel-1] = false;
		return i32Ret;
	}
	else
		TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 10);
	
		
	i32Ret = CalibrateGain(u16Channel);
	if( CS_FAILED(i32Ret) )
	{
		SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_200_MV, CSPLNDA_GND_CAL_LEVEL);
		TraceCalib( u16Channel, TRACE_CAL_RESULT);
		TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 1002, 0, i32Ret);
		SendCalibModeSetting(u16Channel, ecmCalModeNormal);
		m_bForceCal[u16Channel-1] = false;
		return i32Ret;
	}
	else
	{
		*pu32SuccessFlag &= ~CSPLNDA_CAL_RESTORE_GAIN;
		TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 11);
	}

	i32Ret = CalibratePosition(u16Channel);
	if( CS_FAILED(i32Ret) )
	{
		SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_200_MV, CSPLNDA_GND_CAL_LEVEL);
		TraceCalib( u16Channel, TRACE_CAL_RESULT);
		TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 1003, 0, i32Ret);
		SendCalibModeSetting(u16Channel, ecmCalModeNormal);
		m_bForceCal[u16Channel-1] = false;
		return i32Ret;
	}
	else
	{
		*pu32SuccessFlag &= ~CSPLNDA_CAL_RESTORE_POS;
		TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 12);
	}

	SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_200_MV, CSPLNDA_GND_CAL_LEVEL);
	SendCalibModeSetting(u16Channel, ecmCalModeNormal);
	m_pCardState->bCalNeeded[u16Channel-1] = false;
	m_bForceCal[u16Channel-1] = false;
	
	i32Ret = SendPosition( u16Channel, m_pCardState->ChannelParams[u16Channel-1].i32Position );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	SetFastCalibration(u16Channel);

	TraceCalib( u16Channel, TRACE_CAL_RESULT);
	TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 21);
	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsSplendaDev::MsSetupAcquireAverage( bool bSetup)
{
	CsSplendaDev	*pCard = m_MasterIndependent;
	int32			i32Status = CS_SUCCESS;

	while( pCard )
	{
		i32Status = pCard->SetupAcquireAverage( bSetup );
		if ( CS_FAILED(i32Status) )
			return i32Status;

		pCard = pCard->m_Next;
	}

	return i32Status;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsSplendaDev::SetupAcquireAverage( bool bSetup)
{
	int32 i32Ret = CS_SUCCESS;

	// in eXpert images there is no calibration capability
	if( 0 != m_pCardState->u8ImageId )
		return CS_SUCCESS;

	m_bCalibActive		= bSetup;

	CSACQUISITIONCONFIG		AcqCfg;
	if( bSetup )
	{
		m_OldAcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);
		AcqCfg.u32Size		= sizeof(CSACQUISITIONCONFIG);
		CsGetAcquisitionConfig( &m_OldAcqCfg );
		CsGetAcquisitionConfig( &AcqCfg );

		AcqCfg.i64TriggerTimeout = 0;
		AcqCfg.i64TriggerDelay   = 0;
		AcqCfg.i64TriggerHoldoff = 0;
		AcqCfg.i64SampleRate	 = m_pCardState->SrInfo[0].llSampleRate;
		AcqCfg.i64SegmentSize	 = CSPLNDA_CALIB_BUFFER_SIZE;
		AcqCfg.i64Depth			 = CSPLNDA_CALIB_DEPTH;
		AcqCfg.u32SegmentCount   = 1;
		AcqCfg.u32ExtClk		 = 0;
		AcqCfg.u32Mode			 &= ~(CS_MODE_USER1|CS_MODE_USER2|CS_MODE_REFERENCE_CLK);

		m_pi16CalibBuffer = (int16 *)::GageAllocateMemoryX( CSPLNDA_CALIB_BUFFER_SIZE*sizeof(int16) );
		if( NULL == m_pi16CalibBuffer)
			return CS_INSUFFICIENT_RESOURCES;
	}
	else
	{
		::GageFreeMemoryX(m_pi16CalibBuffer);
		m_pi16CalibBuffer = NULL;

		AcqCfg = m_OldAcqCfg;
	}

	//Set timeout
	SetTriggerTimeout( bSetup ? 0 : m_u32HwTrigTimeout );

	i32Ret = SendClockSetting( AcqCfg.i64SampleRate, AcqCfg.u32ExtClk, AcqCfg.u32ExtClkSampleSkip, (AcqCfg.u32Mode&CS_MODE_REFERENCE_CLK) != 0 );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Adjust Segemnt Parameters for Hardware
	AdjustedHwSegmentParameters( m_pCardState->AcqConfig.u32Mode, &AcqCfg.u32SegmentCount, &AcqCfg.i64SegmentSize, &AcqCfg.i64Depth );
	i32Ret = SendSegmentSetting( AcqCfg.u32SegmentCount, AcqCfg.i64Depth, AcqCfg.i64SegmentSize, AcqCfg.i64TriggerHoldoff, AcqCfg.i64TriggerDelay );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Update kernel segment settings
	CSACQUISITIONCONFIG_EX AcqCfgEx = {0};

	AcqCfgEx.AcqCfg.u32Size		= sizeof(CSACQUISITIONCONFIG);
	CsGetAcquisitionConfig( &AcqCfgEx.AcqCfg );

	AcqCfgEx.i64HwDepth			= m_i64HwPostDepth;
	AcqCfgEx.i64HwSegmentSize	= m_i64HwSegmentSize;
	AcqCfgEx.u32Decimation		= bSetup ? 1 : m_pCardState->AddOnReg.u32Decimation;
	AcqCfgEx.u32ExpertOption	= m_PlxData->CsBoard.u32BaseBoardOptions[m_pCardState->u8ImageId];

	IoctlSetAcquisitionConfigEx( &AcqCfgEx ) ;

	return i32Ret;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSplendaDev::SetClockForCalibration(const bool bSet)
{
	int32 i32Ret = CS_SUCCESS;
	if( bSet )
	{
		//set clock to highest internal
		i32Ret = WriteRegisterThruNios(CSPLNDA_CLK_SRC_SEL | CSPLNDA_SET_TCXO_REF_CLK, CSPLNDA_CLK_SET_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		// 1 as decimation factor
		i32Ret = WriteRegisterThruNios(1, CSPLNDA_DEC_FACTOR_LOW_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = WriteRegisterThruNios(0, CSPLNDA_DEC_FACTOR_HIGH_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = SetClockThruNios( m_pCardState->SrInfo[0].u32ClockFrequencyMhz );
		if( CS_FAILED(i32Ret) ) return i32Ret;
		AddonReset();
	}
	else
	{
		int32 i32Ret = WriteRegisterThruNios((m_pCardState->AddOnReg.u16ClkSelect&~CSPLNDA_CLK_OUT_ENABLE), CSPLNDA_CLK_SET_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		
		i32Ret = WriteRegisterThruNios( (m_pCardState->AddOnReg.u32Decimation&0x7fff), CSPLNDA_DEC_FACTOR_LOW_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = WriteRegisterThruNios( ((m_pCardState->AddOnReg.u32Decimation>>15)&0x7fff), CSPLNDA_DEC_FACTOR_HIGH_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = SetClockThruNios( m_pCardState->AddOnReg.u32ClkFreq );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		if( 0 != (m_pCardState->AddOnReg.u16ClkSelect&CSPLNDA_CLK_OUT_ENABLE) )
		{
			i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16ClkSelect, CSPLNDA_CLK_SET_WR_CMD);
			if( CS_FAILED(i32Ret) )	return i32Ret;
		}
		AddonReset();
	}
	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSplendaDev::SetSegmentForCalibration(const bool bSet)
{
	int32 i32Ret = CS_SUCCESS;
	uInt32 u32SegmentCount;
	int64  i64SegmentSize;
	int64  i64PostDepth;
	int64  i64Holdoff;
	uInt32 u32HwTimeout = 0;

	if( bSet )
	{
		
		u32HwTimeout = 0;
		u32SegmentCount = 1;
		i64Holdoff = 0;
		i64SegmentSize = CSPLNDA_CALIB_BUFFER_SIZE;
		i64PostDepth = CSPLNDA_CALIB_DEPTH;
	}
	else
	{
		//Restore timeout
		u32HwTimeout = (uInt32) m_pCardState->AcqConfig.i64TriggerTimeout;
		if( CS_TIMEOUT_DISABLE != u32HwTimeout && 0 != u32HwTimeout)
		{
			LONGLONG llTimeout = (LONGLONG)u32HwTimeout * m_pCardState->AcqConfig.i64SampleRate;
			llTimeout /= (8 / (m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE));
			//round up 100 ns
			llTimeout += 9999999;
			llTimeout /= 10000000; 
			if ( llTimeout > (uInt32) CS_TIMEOUT_DISABLE )
				u32HwTimeout = (uInt32) CS_TIMEOUT_DISABLE;
			else
				u32HwTimeout = (uInt32)llTimeout;
		}
		
		u32SegmentCount = m_pCardState->AcqConfig.u32SegmentCount;
		i64SegmentSize = m_pCardState->AcqConfig.i64SegmentSize;
		i64PostDepth = m_pCardState->AcqConfig.i64Depth;
		i64Holdoff =  m_pCardState->AcqConfig.i64TriggerHoldoff;
	}
	//Set timeout
	SetTriggerTimeout (u32HwTimeout);
	// Adjust Segemnt Parameters for Hardware
	AdjustedHwSegmentParameters( m_pCardState->AcqConfig.u32Mode, &u32SegmentCount, &i64SegmentSize, &i64PostDepth );
	i32Ret = SendSegmentSetting (u32SegmentCount, i64PostDepth, i64SegmentSize, i64Holdoff);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSplendaDev::MsCalibrateAllChannels()
{
	CsSplendaDev	*pDevice = m_MasterIndependent;
	int32			i32Status = CS_SUCCESS;
	int32			i32ErrorStatus = CS_SUCCESS;
	bool			bRequiredCalibrate = false;

	while (pDevice != NULL)
	{
		if ( pDevice->IsCalibrationRequired() )
		{
			bRequiredCalibrate = true;
			break;
		}
		pDevice = pDevice->m_Next;
	}

	if ( bRequiredCalibrate )
	{
		i32Status = MsSetupAcquireAverage( true );
		if ( CS_FAILED(i32Status) )
			return i32Status;
		
		pDevice = m_MasterIndependent;
		while (pDevice != NULL)
		{
			i32Status = pDevice->CalibrateAllChannels();
			if ( CS_FAILED(i32Status) )
				i32ErrorStatus = i32Status;

			pDevice = pDevice->m_Next;
		}

		i32Status = MsSetupAcquireAverage( false );
	}

	if ( m_MasterIndependent->m_pCardState->bAlarm )
		return CS_CHANNEL_PROTECT_FAULT;
	else
	{
		// return the last error found
		return i32ErrorStatus;
	}
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSplendaDev::CalibrateAllChannels()
{
	if( ! IsCalibrationRequired(true) )
		return CS_SUCCESS;

	int32 i32Ret = CS_SUCCESS;
	int32 i32Res = CS_SUCCESS;
	bool bForceCalib = false;
	uInt16 u16UserChan = 1;

	if ( m_bSkipCalib || m_bUseEepromCal )
	{
		TraceCalib( 0, TRACE_CAL_PROGRESS, 30);

		for( u16UserChan = 1; u16UserChan <= m_PlxData->CsBoard.NbAnalogChannel; u16UserChan += GetUserChannelSkip() )
		{
			uInt16 u16HwChan = ConvertToHwChannel(u16UserChan);

			if( m_bUseEepromCal )
			{
				i32Ret = EepromCalibrationSettings(u16HwChan);
				if( CS_FAILED(i32Ret) )
					return CS_FAST_CALIB_FAILURE;
				else if( CS_SUCCESS != i32Ret )
					bForceCalib = true;   //if i32Ret == CS_FALSE need to calibrate
			}
			else if ( !m_bForceCal[u16HwChan-1] )
			{
				TraceCalib( u16HwChan, TRACE_CAL_PROGRESS, 2, 0, 0);
				m_pCardState->bFastCalibSettingsDone[u16HwChan-1] = false;
				i32Ret = FastCalibrationSettings(u16HwChan);
				if( CS_FAILED(i32Ret) )
					return CS_FAST_CALIB_FAILURE;
				else if( CS_SUCCESS != i32Ret )
					bForceCalib = true; //if i32Ret == CS_FALSE need to calibrate
			}
			else
				bForceCalib = true;
		}

		TraceCalib( 0, TRACE_CAL_PROGRESS, 31);
	
		if ( !bForceCalib )
			return CS_SUCCESS;
	}

	TraceCalib( 0, TRACE_CAL_PROGRESS, 30);
	//--------------------------------------------------------------------

		char szErrorStr[128];
		bool bError = false;
		for( u16UserChan = 1; u16UserChan <= m_PlxData->CsBoard.NbAnalogChannel; u16UserChan += GetUserChannelSkip() )
		{
			uInt32 u32Flag;
			i32Ret = CalibrateFrontEnd( ConvertToHwChannel(u16UserChan), &u32Flag );
			if( CS_FAILED(i32Ret) )
			{
				uInt16 u16HwChan = uInt16(-i32Ret/CS_ERROR_FORMAT_THRESHOLD);
				int32 i32Reason = i32Ret%CS_ERROR_FORMAT_THRESHOLD;
				if( 0 != u16HwChan )
				{
					CsEventLog EventLog;
					sprintf_s( szErrorStr, sizeof(szErrorStr), "%d (channel %d, reason %d)", i32Ret, u16HwChan, i32Reason );
					EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );
				}
				// try to recover using eeprom cal values
				if( m_bUseEepromCalBackup  && (CS_SUCCESS == EepromCalibrationSettings(ConvertToHwChannel(u16UserChan))) )
				{
					m_pCardState->bCalNeeded[ConvertToHwChannel(u16UserChan)-1] = false;
					continue;
				}
				DefaultCalibrationSettings(ConvertToHwChannel(u16UserChan), u32Flag);
				if ( 0 == ( m_u32IgnoreCalibErrorMaskLocal  & (1 << (ConvertToHwChannel(u16UserChan)-1)) ) )
				{
					if( !bError )
						i32Res = i32Ret;
					bError = true;
				}
				else
					m_pCardState->bCalNeeded[ConvertToHwChannel(u16UserChan)-1] = false;
			}
		}

	//--------------------------------------------------------------------
	TraceCalib( 0, TRACE_CAL_PROGRESS, 31);


	WriteGIO(CSPLNDA_AD7321_WRITE_REG,CSPLNDA_AD7321_CLK_DIS ); // turn off CalAdc clock
	
	uInt16 u16CalibSrcChan = 0;
	for( u16UserChan = 1; u16UserChan <= m_PlxData->CsBoard.NbAnalogChannel; u16UserChan += GetUserChannelSkip() )
		if( 0 != (m_u32DebugCalibSrc & 1 << (ConvertToHwChannel(u16UserChan) - 1)) &&
		    0 != m_pCardState->ChannelParams[ConvertToHwChannel(u16UserChan) - 1].u32Filter )
				u16CalibSrcChan = ConvertToHwChannel(u16UserChan);
	
	if( 0 != u16CalibSrcChan )
		SendCalibModeSetting( u16CalibSrcChan, ecmCalModeDc );
	return i32Res;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsSplendaDev::UpdateCalibDac(uInt16 u16Channel, eCalDacId ecdId, uInt16 u16Data)
{
	int32 i32Ret = CS_SUCCESS;
	if( (0 == u16Channel) || (u16Channel > CSPLNDA_CHANNELS)  )
	{
		return CS_INVALID_CHANNEL;
	}

	uInt16 u16ChanZeroBased = u16Channel-1;
	
	uInt32 u32RangeIndex, u32FeModIndex;
	i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	uInt16 u16Dac = GetControlDacId( ecdId, u16Channel );
	TraceCalib( u16Channel, TRACE_CAL_DAC, u16Data, u16Dac);
    
	i32Ret = WriteToCalibDac(u16Dac, u16Data, m_u8CalDacSettleDelay);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	switch( ecdId )
	{ 
		case ecdPosition: m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset = u16Data; break;
		case ecdOffset:   m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16FineOffset = u16Data; break;
		case ecdGain:     m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16Gain = u16Data; break;
	}
	
	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsSplendaDev::GetEepromCalibInfoValues(uInt16 u16Channel, eCalDacId ecdId, uInt16 &u16Data)
{
	int32 i32Ret = CS_SUCCESS;
	if( (0 == u16Channel) || (u16Channel > CSPLNDA_CHANNELS)  )
	{
		return CS_INVALID_CHANNEL;
	}

	uInt16 u16ChanZeroBased = u16Channel-1;
	uInt32 u32RangeIndex, u32FeModIndex;
	i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	switch( ecdId )
	{ 
		case ecdPosition: 
			u16Data = m_pCardState->CalibInfoTable.DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset;
			break;
		case ecdOffset:
			u16Data = m_pCardState->CalibInfoTable.DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16FineOffset;
			break;
		case ecdGain: 
			u16Data = m_pCardState->CalibInfoTable.DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16Gain;
			break;
	}
	
	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsSplendaDev::UpdateCalibInfoStrucure()
{
	uInt16	u16HwChannel;
	uInt16	u16ChanZeroBased;
	uInt32	u32MaxRangeIndex;

	uInt32	u32MaxFEIndex = CSPLNDA_MAX_FE_SET_INDEX;
	
	// Verify that all channels has been calibrated
	for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i++ )
	{
		u16HwChannel = ConvertToHwChannel(i);
		u16ChanZeroBased = u16HwChannel-1;

		for ( uInt16 j = 0; j < u32MaxFEIndex; j ++ )
		{
			if ( j & CSPLNDA_MAX_FE_SET_IMPED_INC )
				u32MaxRangeIndex = m_pCardState->u32SwingTableSize[1];
			else
				u32MaxRangeIndex = m_pCardState->u32SwingTableSize[0];

			for ( uInt16 k = 0; k < u32MaxRangeIndex; k++ )
			{
				if ( ! m_pCardState->DacInfo[u16ChanZeroBased][j][k].bCalibrated )
				{
					// If filtered calibration was not done use unfiltered one
					if ( m_pCardState->DacInfo[u16ChanZeroBased][j^CSPLNDA_MAX_FE_SET_FLTER_INC][k].bCalibrated)
						m_pCardState->DacInfo[u16ChanZeroBased][j][k] = m_pCardState->DacInfo[u16ChanZeroBased][j^CSPLNDA_MAX_FE_SET_FLTER_INC][k];
					else
						return CS_CHANNELS_NOTCALIBRATED;
				}	
			}
		}
	}

	GageCopyMemoryX( m_pCardState->CalibInfoTable.DacInfo, m_pCardState->DacInfo, sizeof( m_pCardState->CalibInfoTable.DacInfo ) );
	
	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsSplendaDev::EepromCalibrationSettings(uInt16 u16Channel, bool bForceSet )
{
	TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 3);

	uInt32 u32RangeIndex, u32FeModIndex;
	int32 i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	uInt16 u16ChanZeroBased = u16Channel - 1;
	m_pCardState->bFastCalibSettingsDone[u16ChanZeroBased] = false;
	m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex] = m_pCardState->CalibInfoTable.DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex];

	return FastCalibrationSettings(u16Channel, bForceSet );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsSplendaDev::DefaultCalibrationSettings(uInt16 u16Channel, uInt32 u32RestoreFlag)
{
	TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 4, u32RestoreFlag);
	uInt32 u32RangeIndex, u32FeModIndex;
	int32 i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	uInt16 u16ChanZeroBased = u16Channel - 1;
	m_pCardState->bFastCalibSettingsDone[u16ChanZeroBased] = false;

	m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].bCalibrated = false;
	
	if( 0 != (u32RestoreFlag  & CSPLNDA_CAL_RESTORE_FINE) )
		m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16FineOffset = c_u16CalDacCount/2;
		
	if( 0 != (u32RestoreFlag  & CSPLNDA_CAL_RESTORE_COARSE) )
		m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset = c_u16CalDacCount/2;
	if( 0 != (u32RestoreFlag  & CSPLNDA_CAL_RESTORE_GAIN) )
		m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16Gain = c_u16CalDacCount/2;
	if( 0 != (u32RestoreFlag  & CSPLNDA_CAL_RESTORE_POS) )
		m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CodeDeltaForHalfIR = c_u16CalDacCount/5;
	
	return FastCalibrationSettings(u16Channel, true );
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSplendaDev::FastCalibrationSettings(uInt16 u16Channel, bool bForceSet )
{
	uInt32 u32RangeIndex, u32FeModIndex;
	int32 i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	uInt16 u16ChanZeroBased = u16Channel - 1;
	
	if ( bForceSet ||
		( m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].bCalibrated  && ! m_pCardState->bFastCalibSettingsDone[u16ChanZeroBased] ) )
	{
		i32Ret = WriteToCalibDac( GetControlDacId( ecdPosition, u16Channel ), 
			                      m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset);

		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = WriteToCalibDac( GetControlDacId( ecdOffset, u16Channel ), 
			                      m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16FineOffset);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = SendPosition( u16Channel, m_pCardState->ChannelParams[u16ChanZeroBased].i32Position );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = WriteToCalibDac( GetControlDacId( ecdGain, u16Channel ),
			                      m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16Gain, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		m_pCardState->bFastCalibSettingsDone[u16ChanZeroBased] = true;
		m_pCardState->bCalNeeded[u16ChanZeroBased] = false;
		TraceCalib( u16Channel, TRACE_CAL_RESULT);
		return CS_SUCCESS;
	}
	return CS_FALSE;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------


void CsSplendaDev::SetFastCalibration(uInt16 u16Channel)
{
	uInt32	u32TimeElapsed = 0;		// in minutes
#ifdef _WINDOWS_
	u32TimeElapsed = GetTickCount();	// in ms
	u32TimeElapsed /= 1000;	//in sec
	u32TimeElapsed /= 60;	//in min
#endif
	if ( u32TimeElapsed > 24*60 )
		u32TimeElapsed = 24*60;

	if( u32TimeElapsed > m_u16SkipCalibDelay)
	{
		uInt32 u32RangeIndex, u32FeModIndex;
		FindFeCalIndices( u16Channel, u32RangeIndex,  u32FeModIndex );
	    m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].bCalibrated = true;
	}

}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool	CsSplendaDev::ForceFastCalibrationSettingsAllChannels()
{
	bool bRetVal = true;

	for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i += GetUserChannelSkip() )
	{
		if( ! FastCalibrationSettings( ConvertToHwChannel(i), true ) )
			bRetVal = FALSE;
	}

	return bRetVal;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	 CsSplendaDev::SendAdcOffsetAjust(uInt16 u16Channel, uInt16 u16Offet )
{
	return WriteRegisterThruNios( u16Offet, CSPLNDA_ADC_DOC1 + ( (u16Channel- 1) << 8 ) );
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
uInt32 CsSplendaDev::GetDefaultRange()
{
	uInt32*	pSwingTable;
	uInt32	u32MaxIndex = 0;
	uInt32  u32DefaultImped = CSPLNDA_DEFAULT_IMPEDANCE;


	if(CS_REAL_IMP_1M_OHM == u32DefaultImped)
	{
		pSwingTable = m_pCardState->u32SwingTable[1];
		u32MaxIndex = m_pCardState->u32SwingTableSize[1];
	}
	else
	{
		pSwingTable = m_pCardState->u32SwingTable[0];
		u32MaxIndex = m_pCardState->u32SwingTableSize[0];
	}

	for( uInt32 i = 0; i < u32MaxIndex; i++ )
	{
		if ( CSPLNDA_DEFAULT_GAIN == pSwingTable[i] )
			return CSPLNDA_DEFAULT_GAIN;
	}

	return pSwingTable[0];
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

#define CSPLNDA_COARSE_CAL_LIMIT    16384

int32	CsSplendaDev::CalibrateOffset(uInt16 u16Channel, uInt32* pu32SuccessFlag)
{
	int32 i32Ret = CS_SUCCESS;
	if( (0 == u16Channel) || (u16Channel > CSPLNDA_CHANNELS)  )
	{
		return CS_INVALID_CHANNEL;
	}

	uInt16 u16ChanZeroBased = u16Channel-1;
	//set offset controls to default positions
	i32Ret = UpdateCalibDac(u16Channel, ecdOffset, c_u16CalDacCount/2);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = UpdateCalibDac(u16Channel, ecdPosition, c_u16CalDacCount/2 );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	// first connect calibration circuitry and set cal voltage to Zero
	i32Ret = SendCalibModeSetting(u16Channel, ecmCalModeDc);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	int32  i32Voltage_uV;
	i32Ret = SendCalibDC( m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance, CS_GAIN_200_MV, CSPLNDA_GND_CAL_LEVEL, &i32Voltage_uV);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	//Calculate expected code: (SampleRes * i32Voltage_uV)/InputRange
	LONGLONG llCode_x10 = m_pCardState->AcqConfig.i32SampleRes;
	llCode_x10 *= i32Voltage_uV;
	// account for voltage devidor
	llCode_x10 *= m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance;
	llCode_x10 /= (m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance+c_u32R77_R67);
	// m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange * 1000 (mV => u) /2 (unipolar) /10 (times 10)
	llCode_x10 /= m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange * 50;
	
	//Target 
	int32 i32Target = int32(llCode_x10);
	
	int32 i32Avg_x10 = 0;
	uInt16 u16Pos = 0;
	uInt16 u16PosDelta = c_u16CalDacCount/2;
	
	while( u16PosDelta > 0 )
	{
		i32Ret = UpdateCalibDac(u16Channel, ecdPosition, u16Pos | u16PosDelta );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		TraceCalib( u16Channel, TRACE_CAL_ITER, u16Pos|u16PosDelta, TRACE_COARSE_OFFSET, i32Avg_x10, i32Target);
		if( i32Avg_x10 < i32Target )
			u16Pos |= u16PosDelta;

		u16PosDelta >>= 1;
	}
	TraceCalib( u16Channel, TRACE_COARSE_OFFSET, u16Pos, 0, i32Avg_x10 );
	
	if ( i32Avg_x10 > CSPLNDA_COARSE_CAL_LIMIT || i32Avg_x10 < -CSPLNDA_COARSE_CAL_LIMIT )
	{
		return CS_COARSE_OFFSET_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
	}
	
	*pu32SuccessFlag &= ~CSPLNDA_CAL_RESTORE_COARSE;
	
	// Then adjust fine offset to match 
	uInt16 u16Offset = 0;
	uInt16 u16OffsetDelta = c_u16CalDacCount/2;
	
	while( u16OffsetDelta > 0 )
	{
		i32Ret = UpdateCalibDac(u16Channel, ecdOffset, u16Offset | u16OffsetDelta );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		TraceCalib( u16Channel, TRACE_CAL_ITER, u16Offset|u16OffsetDelta, TRACE_FINE_OFFSET, i32Avg_x10, i32Target);
		if( i32Avg_x10 <= i32Target )
			u16Offset |= u16OffsetDelta;

		u16OffsetDelta >>= 1;
	}
	
	TraceCalib( u16Channel, TRACE_FINE_OFFSET, u16Offset, 0, i32Target, i32Avg_x10 );

	if( 0 == u16Offset || (c_u16CalDacCount - 1) == u16Offset )
	{
		return CS_FINE_OFFSET_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
	}
	*pu32SuccessFlag &= ~CSPLNDA_CAL_RESTORE_FINE;
	return	CS_SUCCESS;
}



//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsSplendaDev::CalibrateGain(uInt16 u16Channel)
{
	int32 i32Ret = CS_SUCCESS;
	if( (0 == u16Channel) || (u16Channel > CSPLNDA_CHANNELS)  )
		return CS_INVALID_CHANNEL;
	uInt16 u16ChanZeroBased = u16Channel-1;

	uInt16 u16GainFactor = CSPLNDA_DEFAULT_GAINTARGET_FACTOR;

	if (m_bGainTargetAdjust)
	{
		uInt32	u32RangeIndex, u32ImpedIndex, u32FilterIndex;
		i32Ret = FindGainFactorIndex( u16Channel, m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange,
									  m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance, m_pCardState->ChannelParams[u16ChanZeroBased].u32Filter,
									  &u32RangeIndex, &u32ImpedIndex, &u32FilterIndex );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		u16GainFactor = m_u16GainTarFactor[u16ChanZeroBased][u32RangeIndex][u32ImpedIndex][u32FilterIndex];
	}

//Positive side
	int32  i32Voltage_uV;
	i32Ret = SendCalibDC( m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance, 
		                  m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange, 
						  CSPLNDA_POS_CAL_LEVEL, 
						  &i32Voltage_uV);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	LONGLONG llCode_x10 = -m_pCardState->AcqConfig.i32SampleRes;
	llCode_x10 *= i32Voltage_uV;
	// account for voltage devidor
	llCode_x10 *= m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance;
	llCode_x10 /= (m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance+c_u32R77_R67);
	llCode_x10 /=  m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange * 50;
	//Adjust tartget code to the voltage diver due to fron port resistance
	llCode_x10 *= ( m_u32FrontPortResistance_mOhm[u16ChanZeroBased] + m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance * 1000);
	llCode_x10 /= m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance*1000;
	int32 i32Target = int32((llCode_x10*u16GainFactor)/CSPLNDA_DEFAULT_GAINTARGET_FACTOR);
	
	TraceCalib( u16Channel, TRACE_GAIN_ADJ, u16GainFactor, 0, int32(llCode_x10), i32Target);

	int32 i32Avg_x10 = 0;
	uInt16 u16PosGain = 0;
	uInt16 u16PosDelta = c_u16CalDacCount/2;
	
	while( u16PosDelta > 0 )
	{
		i32Ret = UpdateCalibDac(u16Channel, ecdGain, u16PosGain | u16PosDelta );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		TraceCalib( u16Channel, TRACE_CAL_ITER, u16PosGain|u16PosDelta, TRACE_CAL_GAIN, i32Avg_x10, i32Target);
		if( i32Avg_x10 < i32Target )
			u16PosGain |= u16PosDelta;
		u16PosDelta >>= 1;

	}
	
	if( 0 == u16PosGain || (c_u16CalDacCount - 1) == u16PosGain )
	{
		return CS_GAIN_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
	}

//Negative side
	i32Ret = SendCalibDC( m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance, 
		                  m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange, 
						  CSPLNDA_NEG_CAL_LEVEL, 
						  &i32Voltage_uV);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	llCode_x10 = -m_pCardState->AcqConfig.i32SampleRes;
	llCode_x10 *= i32Voltage_uV;
	// account for voltage devidor
	llCode_x10 *= m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance;
	llCode_x10 /= (m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance+c_u32R77_R67);
	// m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange * 1000 (mV => u) /2 (unipolar) /10 (times 10)
	llCode_x10 /= m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange * 50;
	//Adjust tartget code to the voltage diver due to fron port resistance
	llCode_x10 *= ( m_u32FrontPortResistance_mOhm[u16ChanZeroBased] + m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance * 1000);
	llCode_x10 /= m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance*1000;
	i32Target = int32((llCode_x10*u16GainFactor)/CSPLNDA_DEFAULT_GAINTARGET_FACTOR);
	
	TraceCalib( u16Channel, TRACE_GAIN_ADJ, u16GainFactor, 1, int32(llCode_x10), i32Target);

	i32Avg_x10 = 0;
	uInt16 u16NegGain = 0;
	uInt16 u16NegDelta = c_u16CalDacCount/2;
	
	while( u16NegDelta > 0 )
	{
		i32Ret = UpdateCalibDac(u16Channel, ecdGain, u16NegGain | u16NegDelta );
		if( CS_FAILED(i32Ret) )return i32Ret;

		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		TraceCalib( u16Channel, TRACE_CAL_ITER, u16NegGain|u16NegDelta, TRACE_CAL_GAIN, i32Avg_x10, i32Target);
		if( i32Avg_x10 >= i32Target )
			u16NegGain |= u16NegDelta;
		u16NegDelta >>= 1;
	}
	if( 0 == u16NegGain || (c_u16CalDacCount - 1) == u16NegGain )
	{
		return CS_GAIN_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
	}

	i32Ret = UpdateCalibDac(u16Channel, ecdGain, (u16PosGain+u16NegGain)/2 );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	TraceCalib( u16Channel, TRACE_CAL_GAIN, u16PosGain, u16NegGain);

	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsSplendaDev::CalibratePosition(uInt16 u16Channel)
{
	int32 i32Ret = CS_SUCCESS;
	if( (0 == u16Channel) || (u16Channel > CSPLNDA_CHANNELS)  )
		return CS_INVALID_CHANNEL;
	uInt16 u16ChanZeroBased = u16Channel-1;
	
	uInt32 u32RangeIndex, u32FeModIndex;
	i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CodeDeltaForHalfIR = 0;

	int32  i32Voltage_uV;
	i32Ret = SendCalibDC( m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance, 
		                  m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange, 
						  CSPLNDA_GND_CAL_LEVEL, 
						  &i32Voltage_uV);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	LONGLONG llCode_x10 = -m_pCardState->AcqConfig.i32SampleRes;
	llCode_x10 *= i32Voltage_uV;
	// account for voltage devidor
	llCode_x10 *= m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance;
	llCode_x10 /= (m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance+c_u32R77_R67);
	// m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange * 1000 (mV => u) /2 (unipolar) /10 (times 10)
	llCode_x10 /= m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange * 50;
	
	uInt16 u16Dac = GetControlDacId( ecdPosition, u16Channel );

//Positive side
	int32 i32Target = int32(llCode_x10) - m_pCardState->AcqConfig.i32SampleRes*5;
	int32 i32Avg_x10 = 0;
	uInt16 u16PosPos = 0, u16PosDelta = c_u16CalDacCount/2;
	
	while( u16PosDelta > 0 )
	{
		int32 i32PosCode = m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset;
		i32PosCode += u16PosPos|u16PosDelta;
		uInt16 u16Code = i32PosCode > 0 ? ( i32PosCode < int32(c_u16CalDacCount-1) ? uInt16(i32PosCode) : c_u16CalDacCount-1 ) : 0;

		i32Ret = WriteToCalibDac(u16Dac, u16Code, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		TraceCalib( u16Channel, TRACE_CAL_ITER, u16Code, TRACE_CAL_POS, i32Avg_x10, i32Target);
		if( i32Avg_x10 < i32Target )
			u16PosPos |= u16PosDelta;
		u16PosDelta >>= 1;
	}
	if( 0 == u16PosPos || (c_u16CalDacCount - 1) == u16PosPos )
	{
		return CS_POSITION_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
	}

//Negative side
	i32Target = int32(llCode_x10) + m_pCardState->AcqConfig.i32SampleRes*5;
	uInt16 u16NegPos = 0;
	u16PosDelta = c_u16CalDacCount/2;
	
	while( u16PosDelta > 0 )
	{
		int32 i32PosCode = m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset;
		i32PosCode -= u16NegPos|u16PosDelta;
		uInt16 u16Code = i32PosCode > 0 ? ( i32PosCode < int32(c_u16CalDacCount-1) ? uInt16(i32PosCode) : c_u16CalDacCount-1 ) : 0;

		i32Ret = WriteToCalibDac(u16Dac, u16Code, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		TraceCalib( u16Channel, TRACE_CAL_ITER, u16Code, TRACE_CAL_POS, i32Avg_x10, i32Target);
		if( i32Avg_x10 >= i32Target )
			u16NegPos |= u16PosDelta;
		u16PosDelta >>= 1;
	}
	if( 0 == u16NegPos || (c_u16CalDacCount - 1) == u16PosPos )
	{
		return CS_POSITION_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
	}
	i32Ret = WriteToCalibDac(u16Dac, m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset, m_u8CalDacSettleDelay);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CodeDeltaForHalfIR = (u16PosPos + u16NegPos)/2;
	TraceCalib( u16Channel, TRACE_CAL_POS, u16PosPos, u16NegPos);

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
// bForHw flag is set to false by default it is true only from CalibrateAllChannels
// to determine weather CalibrateAllChannels should be called
// Return true if real change has occured or SendCalibModeSetting may need updating 
// to determine weather hw calibration should be performed
// Return true if real change has occured and Calibration is not disabled 
//-----------------------------------------------------------------------------

bool	CsSplendaDev::IsCalibrationRequired(bool bForHw)
{
	bool bRealNeed = false;
	for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i += GetUserChannelSkip() )
	{
		uInt16 u16ChanZeroBased = ConvertToHwChannel(i) - 1;
		if( m_pCardState->bCalNeeded[u16ChanZeroBased] )
		{
			bRealNeed = true;
			break;
		}
	}
	
	if( bForHw )
		return (bRealNeed && !m_bNeverCalibrateDc);
	else
		return (bRealNeed || (0 != m_u32DebugCalibSrc)); //So we call CalibrateAllChannels
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void CsSplendaDev::TraceCalib(uInt16 u16Channel, uInt32 u32TraceFlag, uInt32 u32V1, uInt32 u32V2, int32 i32V3, int32 i32V4)
{
	char	szText[256];

	szText[0] = 0;
	if( m_u32DebugCalibTrace != 0 )
	{
		if( TRACE_COARSE_OFFSET  == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			sprintf_s( szText, sizeof(szText),  "\n%d Ch. Coarse calibration DAC 0x%04x (%4d) residual offset %d.%ld\n",
				u16Channel, u32V1, u32V1, i32V3/10, labs(i32V3)%10 );
		}
		else if( TRACE_FINE_OFFSET  == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			sprintf_s( szText, sizeof(szText),  "%d Ch.     Fine calibration DAC 0x%04x (%4d) residual offset %c%ld.%ld (Tar %c%ld.%ld)\n",
				u16Channel, u32V1, u32V1, i32V4>0?' ':'-', labs(i32V4)/10, labs(i32V4)%10, i32V3>0?' ':'-', labs(i32V3)/10, labs(i32V3)%10 );
		}
		else if( TRACE_CAL_GAIN  == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			sprintf_s( szText, sizeof(szText),  "\n%d Ch.     Gain calibration Pos DAC 0x%04x (%4d) Neg DAC 0x%04x (%4d)\n",
				u16Channel, u32V1, u32V1, u32V2, u32V2);
		}
		else if( TRACE_CAL_POS  == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			sprintf_s( szText, sizeof(szText),  "\n%d Ch. Position calibration Pos DAC 0x%04x (%4d) Neg DAC 0x%04x (%4d)\n",
				u16Channel, u32V1, u32V1, u32V2, u32V2);
		}
		else if( TRACE_CAL_SIGNAL == (u32TraceFlag &  m_u32DebugCalibTrace))
		{
			bool bVersose = (0 != (TRACE_CAL_SIGNAL_VERB & m_u32DebugCalibTrace));
			LONGLONG llChanLevel = i32V3;
			llChanLevel *= u32V1;
			llChanLevel /= u32V1 + c_u32R77_R67;
			int32 i32Chan = int32(llChanLevel);
	
			uInt32 u32RefVolt(0);
			switch(u16Channel & 0x60 )
			{
				default:
				case 0x00: u32RefVolt = CSPLNDA_CALREF0;break;
				case 0x20: u32RefVolt = CSPLNDA_CALREF1;break;
				case 0x40: u32RefVolt = CSPLNDA_CALREF2;break;
				case 0x60: u32RefVolt = CSPLNDA_CALREF3;break;
			}
			if( bVersose )
			{
				sprintf_s( szText, sizeof(szText),  "=== Cal Signal in %d mV %s range. Using ref %d mV\n",
					u32V2, u32V1 == 50?"50 Ohm":"1 MOhm",  u32RefVolt);
				::OutputDebugString( szText );
			}
			sprintf_s( szText, sizeof(szText),  "    Cal Signal:   %c%ld.%03ld mV (%c%ld.%03ld mV)\n", 
				i32V3>0?' ':'-', labs(i32V3)/1000, labs(i32V3)%1000, i32Chan>0?' ':'-', labs(i32Chan/1000), labs(i32Chan)%1000);
			if(bVersose)
			{
				::OutputDebugString( szText );
				sprintf_s( szText, sizeof(szText),  "         measured %c%ld.%03ld mV @ %s\n", 
					i32V4>0?' ':'-', labs(i32V4)/1000, labs(i32V4)%1000, 0==(m_pCardState->AddOnReg.u16VSC&VSC_CHAN_SRC)?"src":"ch.");
			}
		}
		else if( TRACE_CAL_SIGNAL_ITER  == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			sprintf_s( szText, sizeof(szText),  "       Reading sample %2d (%2d valid) Code %+5d\n",
				u32V1, u32V2, i32V3);
		}
		else if( TRACE_CAL_DAC == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			sprintf_s( szText, sizeof(szText),  "+ + +  Writing  0x%04x to the DAC %2d (Ch. %d)\n", u32V1, u32V2, u16Channel);
		}
		else if( TRACE_ALL_DAC == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			sprintf_s( szText, sizeof(szText),  "+|+|+  Writing  0x%04x to the DAC 0x%04x (delay %d)\n", u32V1&0xFFFF, u32V2, u32V1 >>16 ); 
		}
		else if( TRACE_CAL_RESULT == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			uInt32 u32RangeIndex, u32FeModIndex;
			FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );
			sprintf_s( szText, sizeof(szText),  "       Coarse %5d (0x%03x)      Fine  %5d (0x%03x).\n",
				m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16CoarseOffset,
				m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16CoarseOffset,
				m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16FineOffset,
				m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16FineOffset);
			::OutputDebugString( szText );
			sprintf_s( szText, sizeof(szText),  "       Gain   %5d (0x%03x)   Position %5d (0x%03x).\n",
				m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16Gain,
				m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16Gain,
				m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16CodeDeltaForHalfIR,
				m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16CodeDeltaForHalfIR);
		}
		else if ( TRACE_CAL_RANGE == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			sprintf_s( szText, sizeof(szText),  "- - Input %s Range ±%d.%d V Signal %c%ld.%03ld mV Noise %d.%d LSB Settled by %d Sample.\n%s",
				u32V2==0?"Ref. ":(u32V2==1?"Src. ":"Chan."), u32V1/1000, (u32V1/100)%10, 
				i32V3>0?' ':'-', labs(i32V3/1000), labs(i32V3)%1000, i32V4/10, i32V4%10, u16Channel, u32V2==0?"\n":"");
		}
		else if( TRACE_CAL_PROGRESS == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			if( 1 == u32V1 && !m_bLogFaluireOnly ) //SendCalibModeSetting
			{
				switch(eCalMode(u32V2))
				{
					case ecmCalModeDc:     sprintf_s( szText, sizeof(szText), "* * * Chan %d switched to DC calibration mode\n", u16Channel ); break;
					case ecmCalModeNormal: sprintf_s( szText, sizeof(szText), "* * * Chan %d switched to channel input  mode\n", u16Channel ); break;
					case ecmCalModeMs:     sprintf_s( szText, sizeof(szText), "* * * Chan %d switched to MS calibration mode\n", u16Channel ); break;
				}
			}
			else if ( 2 == u32V1 && !m_bLogFaluireOnly ) //m_bSkipCalib
				sprintf_s( szText, sizeof(szText),  "___ Skipped calibration of the channel %d.\n", u16Channel ); 
			else if ( 3 == u32V1 && !m_bLogFaluireOnly ) //m_bUseEEpromCal
				sprintf_s( szText, sizeof(szText),  "### Using storred calibration values for the channel %d.\n", u16Channel ); 
			else if ( 4 == u32V1 && !m_bLogFaluireOnly ) 
				sprintf_s( szText, sizeof(szText),  "~~~ Using default calibration values for the channel %d ( flag 0x%02X ).\n", u16Channel, u32V2 ); 
			else if ( 10 == u32V1 && !m_bLogFaluireOnly ) //Offset
				sprintf_s( szText, sizeof(szText),  "     Offset calibration of the channel %d has succeeded.\n", u16Channel); 
			else if ( 11 == u32V1 && !m_bLogFaluireOnly ) //Gain
				sprintf_s( szText, sizeof(szText),  "     Gain calibration of the channel %d has succeeded.\n", u16Channel); 
			else if ( 12 == u32V1 && !m_bLogFaluireOnly ) //Position
				sprintf_s( szText, sizeof(szText),  "     Position calibration of the channel %d has succeeded.\n", u16Channel); 
			else if ( 20 == u32V1 && !m_bLogFaluireOnly ) //overall one cahnnel
				sprintf_s( szText, sizeof(szText),  "     Starting calibration of the channel %d %5d mV %s\n", u16Channel, m_pCardState->ChannelParams[u16Channel-1].u32InputRange, 50==m_pCardState->ChannelParams[u16Channel-1].u32Impedance?"50 Ohm":"1 MOhm"); 
			else if ( 21 == u32V1 && !m_bLogFaluireOnly ) //overall one cahnnel
				sprintf_s( szText, sizeof(szText),  "     Calibration of the channel %d has succeeded.\n\n", u16Channel); 
			else if ( 30 == u32V1 && !m_bLogFaluireOnly ) //overall all chnnels
				sprintf_s( szText, sizeof(szText),  "V    Calibration sequence has started.\n"); 
			else if ( 31 == u32V1 && !m_bLogFaluireOnly ) //overall all channels
				sprintf_s( szText, sizeof(szText),  "/\\    Calibration sequence has finished.\n"); 
			else if ( 1001 == u32V1 ) //Offset failed
				sprintf_s( szText, sizeof(szText),  "! ! ! Offset calibration of the channel %d has failed. Error %d \n", u16Channel, (i32V3%0x10000) ); 
			else if ( 1002 == u32V1 ) //gain failed
				sprintf_s( szText, sizeof(szText),  "! ! ! Gain calibration of the channel %d has failed. Error %d \n", u16Channel, (i32V3%0x10000) ); 
			else if ( 1003 == u32V1 ) //position failed
				sprintf_s( szText, sizeof(szText),  "! ! ! Position calibration of the channel %d has failed. Error %d \n", u16Channel, (i32V3%0x10000) ); 
			else if(!m_bLogFaluireOnly )
			{
				sprintf_s( szText, sizeof(szText),  "----Called TraceCalib(%d, TRACE_CAL_PROGRESS, %d, %d, %d,%d )\n", 
									u16Channel,u32V1,u32V2,i32V3,i32V4 ); 
			}
		}
		else if( TRACE_CAL_ITER == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			if( 0 != (m_u32DebugCalibTrace & u32V2) )
			{
				sprintf_s( szText, sizeof(szText),  "\t : : Code %4d Meas %c%5ld.%ld (%c%5ld.%ld)\n",
				u32V1, i32V3>=0?' ':'-', labs(i32V3)/10, labs(i32V3)%10,
				i32V4>=0?' ':'-', labs(i32V4)/10, labs(i32V4)%10); 
			}
		}	
	}

	if ( szText[0] != 0 )
		::OutputDebugString( szText );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSplendaDev::FindGainFactorIndex( uInt16 u16Channel, uInt32 u32InputRange, uInt32 u32Impedance, uInt32 u32Filter,
											 uInt32 *u32RangeIndex, uInt32 *u32ImpedIndex, uInt32 *u32FilterIndex )
{
	int32	i32Ret = CS_SUCCESS;

	if ( 0 == u16Channel || u16Channel > CSPLNDA_CHANNELS )
		return CS_INVALID_CHANNEL;

	i32Ret = FindFeIndex( u32InputRange, u32Impedance, *u32RangeIndex);
	if( CS_FAILED(i32Ret) ) return i32Ret;

	*u32ImpedIndex	= (u32Impedance ==  CS_REAL_IMP_50_OHM) ? 0 : 1;
	*u32FilterIndex	= (u32Filter == 0) ?  0 : 1;

	return CS_SUCCESS;
}

#ifdef _WINDOWS_

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void CsSplendaDev::ReadCalibrationSettings(void *Params)
{
	HKEY hKey;
	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Cs16xyyWDF\\Parameters", 0, KEY_QUERY_VALUE, &hKey) )
	{
		ULONG	ul(0);
		DWORD	DataSize = sizeof(ul);

		ul = m_bSkipCalib ? 1 : 0;
		::RegQueryValueEx( hKey, _T("FastDcCal"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bSkipCalib = (ul != 0);

		ul = m_bUseEepromCal ? 1 : 0;
		::RegQueryValueEx( hKey, _T("UseEepromCal"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bUseEepromCal = (ul != 0);

		ul = m_bUseEepromCalBackup ? 1 : 0;
		::RegQueryValueEx( hKey, _T("UseEepromCalBackup"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bUseEepromCalBackup = (ul != 0);

		ul = m_u16SkipCalibDelay;
		::RegQueryValueEx( hKey, _T("NoCalibrationDelay"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		if( ul > 24 * 60 )	//limitted to 1 day
			ul = 24*60;
		m_u16SkipCalibDelay = (uInt16)ul;
	
		ul = m_bNeverCalibrateDc ? 1 : 0;
		::RegQueryValueEx( hKey, _T("NeverCalibrateDc"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bNeverCalibrateDc = (ul != 0);

		ul = m_u32DebugCalibSrc;
		::RegQueryValueEx( hKey, _T("DebugCalibSource"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_u32DebugCalibSrc = (ul != 0);

		ul = m_u32DebugCalibTrace;
		::RegQueryValueEx( hKey, _T("TraceCalib"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_u32DebugCalibTrace = ul;

		ul = m_bLogFaluireOnly?1:0;
		::RegQueryValueEx( hKey, _T("LogFaluireOnly"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bLogFaluireOnly = ul!=0;

		ul = m_u8CalDacSettleDelay;
		::RegQueryValueEx( hKey, _T("CalDacSettleDelay"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_u8CalDacSettleDelay = (uInt8)ul;

		ul = m_u32CalSrcRelayDelay;
		::RegQueryValueEx( hKey, _T("CalSrcDelay"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_u32CalSrcRelayDelay = ul;

		ul = m_u32CalSrcSettleDelay;
		::RegQueryValueEx( hKey, _T("CalSrcSettleDelay"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_u32CalSrcSettleDelay = ul;

		ul = m_u32CalSrcSettleDelay_1MOhm;
		::RegQueryValueEx( hKey, _T("CalSrcSettleDelay1MOhm"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_u32CalSrcSettleDelay_1MOhm = ul;

		ul = m_u32IgnoreCalibErrorMask;
		::RegQueryValueEx( hKey, _T("IgnoreCalibError"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_u32IgnoreCalibErrorMask = ul;

		ul = m_bInvalidateCalib ?1:0;
		::RegQueryValueEx( hKey, _T("InvalidateCalib"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_bInvalidateCalib = (ul != 0);
	}
}

#else
void CsSplendaDev::ReadCalibrationSettings(char *szIniFile, void *Params)
{
	ULONG	ul = 0;
	DWORD	DataSize = sizeof(ul);
   char *szSectionName = (char *)Params;
	ul = m_bSkipCalib ? 1 : 0;

   if (!szIniFile)
      return;
 
	ul = m_bSkipCalib ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("FastDcCal"),  ul,  szIniFile);	
	m_bSkipCalib = (ul != 0);

	ul = m_bUseEepromCal ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("UseEepromCal"),  ul,  szIniFile);	
	m_bUseEepromCal = (ul != 0);

	ul = m_bUseEepromCalBackup ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("UseEepromCalBackup"),  ul,  szIniFile);	
	m_bUseEepromCalBackup = (ul != 0);

		ul = m_u16SkipCalibDelay;
   ul = GetCfgKeyInt(szSectionName, _T("NoCalibrationDelay"),  ul,  szIniFile);	
	if( ul > 24 * 60 )	//limitted to 1 day
		ul = 24*60;
	m_u16SkipCalibDelay = (uInt16)ul;
	
	ul = m_bNeverCalibrateDc ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("NeverCalibrateDc"),  ul,  szIniFile);	
	m_bNeverCalibrateDc = (ul != 0);

	ul = m_u32DebugCalibSrc;
   ul = GetCfgKeyInt(szSectionName, _T("DebugCalibSource"),  ul,  szIniFile);	
	m_u32DebugCalibSrc = (ul != 0);

	ul = m_u32DebugCalibTrace;
   ul = GetCfgKeyInt(szSectionName, _T("TraceCalib"),  ul,  szIniFile);	
	m_u32DebugCalibTrace = ul;

	ul = m_bLogFaluireOnly?1:0;
   ul = GetCfgKeyInt(szSectionName, _T("LogFaluireOnly"),  ul,  szIniFile);	
	m_bLogFaluireOnly = ul!=0;

	ul = m_u8CalDacSettleDelay;
   ul = GetCfgKeyInt(szSectionName, _T("CalDacSettleDelay"),  ul,  szIniFile);	
	m_u8CalDacSettleDelay = (uInt8)ul;

	ul = m_u32CalSrcRelayDelay;
   ul = GetCfgKeyInt(szSectionName, _T("CalSrcDelay"),  ul,  szIniFile);	
	m_u32CalSrcRelayDelay = ul;

	ul = m_u32CalSrcSettleDelay;
   ul = GetCfgKeyInt(szSectionName, _T("CalSrcSettleDelay"),  ul,  szIniFile);	
	m_u32CalSrcSettleDelay = ul;

	ul = m_u32CalSrcSettleDelay_1MOhm;
   ul = GetCfgKeyInt(szSectionName, _T("CalSrcSettleDelay1MOhm"),  ul,  szIniFile);	
	m_u32CalSrcSettleDelay_1MOhm = ul;

	ul = m_u32IgnoreCalibErrorMask;
   ul = GetCfgKeyInt(szSectionName, _T("IgnoreCalibError"),  ul,  szIniFile);	
	m_u32IgnoreCalibErrorMask = ul;

	ul = m_bInvalidateCalib ?1:0;
   ul = GetCfgKeyInt(szSectionName, _T("InvalidateCalib"),  ul,  szIniFile);	
	m_bInvalidateCalib = (ul != 0);

}   
   
#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CsSplendaDev::UpdateAuxCalibInfoStrucure()
{
	RtlCopyMemory( m_pCardState->CalibInfoTable.u16TrimCap, m_u16TrimCap, sizeof(m_u16TrimCap) );
	RtlCopyMemory( m_pCardState->CalibInfoTable.u32FrontPortResistance_mOhm, m_u32FrontPortResistance_mOhm, sizeof(m_u32FrontPortResistance_mOhm) );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsSplendaDev::MsInvalidateCalib()
{
	// For NT Power integration only.
	// Clear Fastcalib flag and force calibration on all channels 
	CsSplendaDev *pDevice = m_MasterIndependent;
	uInt16			u16ChanZeroBased = 0;
	while ( NULL != pDevice )
	{						
		// Force calibration for all activated channels depending on the current mode (single, dual , quad ...)
		for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i++ )
		{
			u16ChanZeroBased = ConvertToHwChannel(i) - 1;
			pDevice->m_pCardState->bCalNeeded[u16ChanZeroBased] = true;
			pDevice->m_bForceCal[u16ChanZeroBased] = true;
			pDevice->m_pCardState->bFastCalibSettingsDone[u16ChanZeroBased] = false;

			for( uInt16 j = 0; j <= CSPLNDA_MAX_FE_SET_INDEX; j++ )
			{
				for( uInt16 k = 0; k <= CSPLNDA_MAX_RANGES; k++ )
				{				
					pDevice->m_pCardState->DacInfo[u16ChanZeroBased][j][k].bCalibrated = false;
				}
			}
		}

		pDevice = pDevice->m_Next;
	}
}
