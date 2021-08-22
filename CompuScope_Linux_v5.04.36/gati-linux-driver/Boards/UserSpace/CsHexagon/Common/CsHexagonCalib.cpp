//----------------------------------------------------------------------------------
//	This file contains all member functions that access to addon hardware including
//	Frontend set up and calibration
//
//
//-----------------------------------------------------------------------------------
#include "StdAfx.h"
#include "CsTypes.h"
#include "GageWrapper.h"
#include "CsHexagon.h"
#include "CsHexagonCal.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#else
#include "GageCfgParser.h"
#endif

		
#define	 CSRB_CALIB_TIMEOUT_SHORT		KTIME_MILISEC(300)		// 300 milisecond timeout
#define	 HEXAGON_CALIB_TIMEOUT_LONG		KTIME_SECOND(1)			// 1 second timeout
#define	 HEXAGON_CALIB_TIMEOUT_SHORT	KTIME_MILISEC(300)		// 300 milisecond timeout

#define	 HEXAGON_CALIB_TIMEOUT			KTIME_MILISEC(300)		// 300 milisecond timeout

#define	 NB_CALIB_GAIN_LOOP				100						// should not wait for too long, normally it should take 6-10 loop

//#define DBG_CALIB_OFFSET				// Offset Calib info
//#define DBG_CALIB_GAIN				// Gain Calib Info
//#define DBG_CALIB_POSITION			// Position calib info

//#define DBG_CALIB_DAC					// DAC update info
//#define	DBG_CALIB_INFO

#define HEXAGON_DB_CALIB_DEPTH 100
#define HEXAGON_DB_CALIB_BUF_MARGIN 10


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsHexagon::CalibrateGain( uInt16 u16Channel )
{
	int32	i32Ret = CS_SUCCESS;
	uInt16	u16ChanZeroBased = u16Channel-1;
	uInt32	u32InpuRange = m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange;

	int32	i32Avg_x10 = 0;	
	uInt16	u16GainCodePos = 0;
	uInt16	u16BestGainCode = HEXAGON_GAIN_CODE_INVALID;
	int16	i16GainCode = 0;
	int16	i16GainCodeEnd = 0;
	double	fVolt  =  0;
	double	fError =  0;

	HEXAGON_CAL_LUT CalInfo;
	double	fErrorTarget  =  0;
	double	fVoltPos_Ref = 0;
	double	fTargetMax  =  0;
	double	fErrorMin  =  0;
	double	fScale  =  CS_GAIN_2_V/2;          // unipolaire voltage ref

	// Find the setup for the current input ranges
	i32Ret = FindCalLutItem(u32InpuRange, 50, CalInfo);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	fVoltPos_Ref = CalInfo.Vref_mV;

#if 0
	// For testing only.
	i32Ret = UpdateCalibDac(u16Channel, ecdGain, CalInfo.u16GainCtrl, true );
	return i32Ret;
#endif

	if ( m_bNeverCalibrateGain )
		return i32Ret;

	// Calibrate only Pos Voltage Side
	i32Ret = WriteRegisterThruNios( HEXAGON_CAL_SRC_POS, HEXAGON_ADDON_CPLD_CALSRC_REF | HEXAGON_SPI_WRITE );
	if( CS_FAILED(i32Ret) )
		goto EndCalibrateGain;
	BBtiming( 50 );


	/* As design the highrange gain is around 8 (calib range 12-6)
	   the lowrange is around 1 (calib range 6-0)
	*/
	if (m_bLowRange_mV)
	{
		i16GainCode		=  HEXAGON_GAIN_CODE_LR_START;	
		i16GainCodeEnd	=  0;
		fErrorTarget  =  fVoltPos_Ref * m_u32GainTolerance_lr/100;	// Default = 10%Vref 
		fTargetMax	  =  fVoltPos_Ref*125/100;				// 125%Vref
		fScale		  =  HEXAGON_GAIN_LR_MV/2;				// 240mv
	}
	else
	{
		i16GainCode   =  HEXAGON_GAIN_CODE_HR_START;		// Start from middle value.
		i16GainCodeEnd	=  HEXAGON_GAIN_CODE_MIN_ALLOWED;	// protect ADC by noy overvoltage setting
		fErrorTarget  =  fVoltPos_Ref*6/100;				// 6%Vref = 1/2Db gain
		fTargetMax	  =  fVoltPos_Ref*125/100;				// 125%Vref
		fScale		  =  CS_GAIN_2_V/2;						// 1000mv
	}

	fErrorMin	  =  fErrorTarget;

	while ( i16GainCode >= i16GainCodeEnd) 
	{
		i32Ret = UpdateCalibDac(u16Channel, ecdGain, i16GainCode );
		if( CS_FAILED(i32Ret) )
			goto EndCalibrateGain;

		BBtiming( 25 );

		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )	
			goto EndCalibrateGain;
		BBtiming( 10 );

		fVolt = double( i32Avg_x10/10 - m_pCardState->AcqConfig.i32SampleOffset );
		fVolt =  (-1.0)*(fVolt/m_pCardState->AcqConfig.i32SampleRes)*fScale;

		TraceCalib( u16Channel, TRACE_CAL_ITER, (uInt16)i16GainCode, TRACE_CAL_GAIN, i32Avg_x10, (int32)fVoltPos_Ref);

		fError = Abs(Abs(fVolt) - Abs(fVoltPos_Ref));


		// Search for gain near the target voltage.
		if (fError < fErrorMin) 
		{
			u16GainCodePos = i16GainCode;
			fErrorMin = fError;
		}

		// To protect ADC, calib until max voltage reached. 
		if ( fVolt > fTargetMax)
			break;

		i16GainCode--;
	}

	u16BestGainCode = u16GainCodePos;
	// Gain code not found ?. Use default ?
	if (fErrorMin >= fErrorTarget) 
	{
		u16BestGainCode = HEXAGON_GAIN_CODE_INVALID;
	}

EndCalibrateGain:   
	// Restore 0V Vref
	i32Ret = WriteRegisterThruNios( HEXAGON_CAL_SRC_0V, HEXAGON_ADDON_CPLD_CALSRC_REF | HEXAGON_SPI_WRITE );
	BBtiming( 20 );

	if( CS_SUCCEEDED(i32Ret) && (u16BestGainCode != HEXAGON_GAIN_CODE_INVALID) )
	{
		i32Ret = UpdateCalibDac(u16Channel, ecdGain, u16BestGainCode, true );
#ifdef DBG_CALIB_GAIN
		sprintf_s( szDbgText, sizeof(szDbgText), TEXT("CalibrateGain(%d):  u16GainCode %ld fVolt %lf \n"), u16Channel, u16BestGainCode, fVolt);
		OutputDebugString(szDbgText);
#endif
	}
	else
	{
		UpdateCalibDac(u16Channel, ecdGain, CalInfo.u16GainCtrl, true );

		// Set error return code and log the error.
		sprintf_s( szErrorStr, sizeof(szErrorStr), "%d, Calib Gain Channel %d [ 0x%02x ]", 
					CS_GAIN_CAL_FAILED, ConvertToUserChannel(u16Channel), u16GainCodePos );
		m_EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );

		if ( 0 != ( m_u32IgnoreCalibError  & (1 << (ConvertToUserChannel(u16Channel)-1)) ) )
			i32Ret = CS_SUCCESS;
		else
			i32Ret = CS_GAIN_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
	}

	BBtiming( 50 );

	return i32Ret;
}

int32 CsHexagon::ValidateAllChannelGain()
{
	int32	i32Ret = CS_SUCCESS;
	uInt32 u32RangeIndex[4], u32FeModIndex[4];
	uInt16 u16UserChan = 0;
	uInt16 u16GainMax = 0;
	uInt16 u16GainMin = 0x20;

	for( u16UserChan = 1; u16UserChan <= m_PlxData->CsBoard.NbAnalogChannel; u16UserChan += GetUserChannelSkip() )
	{
		i32Ret = FindFeCalIndices( u16UserChan, u32RangeIndex[u16UserChan-1], u32FeModIndex[u16UserChan-1] );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		
		uInt16 u16GainTmp = m_pCardState->DacInfo[u16UserChan-1][u32FeModIndex[u16UserChan-1]][u32RangeIndex[u16UserChan-1]].u16Gain;
		if ( u16GainMax < u16GainTmp )
			u16GainMax = u16GainTmp;
		if ( u16GainMin > u16GainTmp )
			u16GainMin = u16GainTmp;
	}

#ifdef DBG_CALIB_GAIN
	sprintf_s( szDbgText, sizeof(szDbgText), "ValidateAllChannelGain Max %d, Min %d \n", u16GainMax, u16GainMin);
	OutputDebugString(szDbgText);
#endif

	// All channels gain code should not excess 1db.
	if ( (u16GainMax - u16GainMin) > 1)
	{
		if( m_PlxData->CsBoard.NbAnalogChannel == 4)
			sprintf_s( szErrorStr, sizeof(szErrorStr), "%d, Gain code boundary discrepancy [ %02d, %02d, %02d, %02d ]'", 
						CS_GAIN_CROSS_CHECK_FAILED, 
						m_pCardState->DacInfo[0][u32FeModIndex[0]][u32RangeIndex[0]].u16Gain,
						m_pCardState->DacInfo[1][u32FeModIndex[1]][u32RangeIndex[1]].u16Gain,
						m_pCardState->DacInfo[2][u32FeModIndex[2]][u32RangeIndex[2]].u16Gain,
						m_pCardState->DacInfo[3][u32FeModIndex[3]][u32RangeIndex[3]].u16Gain);
		else
			sprintf_s( szErrorStr, sizeof(szErrorStr), "%d, 'Gain code boundary discrepancy [ %02d, %02d ]'", 
						CS_GAIN_CROSS_CHECK_FAILED, 
						m_pCardState->DacInfo[0][u32FeModIndex[0]][u32RangeIndex[0]].u16Gain,
						m_pCardState->DacInfo[1][u32FeModIndex[1]][u32RangeIndex[1]].u16Gain);

#ifdef DBG_CALIB_GAIN
		OutputDebugString(szErrorStr);
#endif
                  
		//error will be logged but not return.
		m_EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );

		//i32Ret = CS_GAIN_CROSS_CHECK_FAILED;
	}

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

BOOLEAN	CsHexagon::IsCalibrationRequired()
{
	BOOLEAN bCalNeeded = FALSE;
		
	for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i += GetUserChannelSkip() )
	{
		uInt16 u16ChanZeroBased = ConvertToHwChannel(i) - 1;
		if( m_pCardState->bCalNeeded[u16ChanZeroBased] )
		{
			bCalNeeded = TRUE;
			break;
		}
	}
#ifdef DBG_CALIB_INFO
		sprintf_s( szDbgText, sizeof(szDbgText), TEXT("IsCalibrationRequired:  bCalNeeded %d \n"), 
			m_bFWCalibSupported, bCalNeeded);
		OutputDebugString(szDbgText);
#endif

	return ( bCalNeeded && !m_bNeverCalibrateDc );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsHexagon::CsGetAverage(uInt16 u16Channel, int16* pi16Buffer, uInt32 u32BufferSizeSamples, int32 i32StartPosistion, int32* pi32OddAvg_x10, int32* pi32EvenAvg_x10)
{
	in_PREPARE_DATATRANSFER		InXferEx;
	out_PREPARE_DATATRANSFER	OutXferEx;

	::GageZeroMemoryX( &InXferEx, sizeof(InXferEx));
	InXferEx.u32Segment		= 1;
	InXferEx.u16Channel		= u16Channel;
	InXferEx.i64StartAddr	= i32StartPosistion;
	InXferEx.i32OffsetAdj	= 0;
	InXferEx.u32DataMask	= 0;
	InXferEx.u32FifoMode	= GetModeSelect(u16Channel);
	InXferEx.bBusMasterDma	= TRUE;
	InXferEx.u32SampleSize  = sizeof(uInt16);

	IoctlPreDataTransfer( &InXferEx, &OutXferEx );
	CardReadMemory( pi16Buffer, u32BufferSizeSamples * sizeof(uInt16) );

	if ( 0 != pi32OddAvg_x10 || 0 != pi32EvenAvg_x10 )
	{
		int32 i32SumOdd = 0;
		int32 i32SumEven = 0;
		int32 i32Count = 0;
		const int32 c32SampleOffset = m_pCardState->AcqConfig.i32SampleOffset;
		
		for (uInt32 u32Addr = HEXAGON_CALIB_BUF_MARGIN;  u32Addr < u32BufferSizeSamples; i32Count++)
		{
			i32SumEven = i32SumEven + pi16Buffer[u32Addr++] - c32SampleOffset;
			i32SumOdd  = i32SumOdd  + pi16Buffer[u32Addr++] - c32SampleOffset;
		}

		if ( pi32OddAvg_x10 )
			*pi32OddAvg_x10  = ( i32SumOdd*10  / i32Count );

		if ( pi32EvenAvg_x10 )
			*pi32EvenAvg_x10 = ( i32SumEven*10 / i32Count );
	}
		
	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsHexagon::CaptureCalibSignal( bool bWaitTrigger /* = false */)
{
	CSTIMER		CsTimeout;

	AcquireData();

	if ( bWaitTrigger )
	{
		int i = 0;
		// Wating for trigger
		for (i = 0; i <= 5; i++)
		{
			Sleep(10);
			if  ( TRIGGERED == ReadTriggerStatus() )
				break;
		}

		if  ( i > 5  )
			ForceTrigger();
	}

	CsTimeout.Set( HEXAGON_CALIB_TIMEOUT_LONG );
	while( STILL_BUSY == ReadBusyStatus() )
	{
		ForceTrigger();
		if ( CsTimeout.State() )
		{
			if( STILL_BUSY == ReadBusyStatus() )
			{
				MsAbort();
				return CS_CAL_BUSY_TIMEOUT;
			}
		}
		BBtiming(1);
	}

	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsHexagon::AcquireAndAverage(uInt16	u16Channel, int32* pi32AvrageOdd_x10, int32* pi32AvrageEven_x10)
{
	int32 i32Ret = CS_SUCCESS;
	
	i32Ret = CaptureCalibSignal();
	if( CS_FAILED(i32Ret) )	return i32Ret;

	//Calculate logical channel
	uInt16 u16UserChannel = ConvertToUserChannel(u16Channel);
	i32Ret = CsGetAverage( u16UserChannel, m_pi16CalibBuffer, HEXAGON_CALIB_DEPTH, 0, pi32AvrageOdd_x10, pi32AvrageEven_x10);
	return  i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsHexagon::CalibrateFrontEnd(uInt16 u16Channel)
{
	if( !m_pCardState->bCalNeeded[u16Channel-1] )
		return CS_SUCCESS;

	int32 i32Ret(CS_SUCCESS);

	do
	{
		i32Ret = CalibrateOffset( u16Channel );
		if( CS_FAILED(i32Ret) )
			break;
		else
			TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 10);

		if ( ! m_bNeverCalibrateGain )
		{

			i32Ret = CalibrateGain( u16Channel );
			if( CS_FAILED(i32Ret) )
				break;
			else
				TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 11);
		}

		i32Ret = CalibratePosition( u16Channel );
		if( CS_FAILED(i32Ret) )
			break;
		else
			TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 12);
	}
	while ( CS_FAILED(i32Ret) );

	if( CS_FAILED(i32Ret) )
	{
		if ( 0 != ( m_u32IgnoreCalibError  & (1 << (ConvertToUserChannel(u16Channel)-1)) ) )
		{
			i32Ret = CS_SUCCESS;
		}
	}
	else
	{
		m_pCardState->bCalNeeded[u16Channel-1] = false;
		i32Ret = SendPosition( u16Channel, m_pCardState->ChannelParams[u16Channel-1].i32Position );
	}

	TraceCalib( u16Channel, TRACE_CAL_RESULT);
	TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 21);
	
	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsHexagon::SetupForCalibration( bool bSetup)
{
	int32 i32Ret = CS_SUCCESS;
	CSACQUISITIONCONFIG		AcqCfg;

	m_bCalibActive		= bSetup;
	if( bSetup )
	{
		m_OldAcqConfig.u32Size	= sizeof(CSACQUISITIONCONFIG);
		AcqCfg.u32Size			= sizeof(CSACQUISITIONCONFIG);
		CsGetAcquisitionConfig( &m_OldAcqConfig );
		CsGetAcquisitionConfig( &AcqCfg );

		// Switch to high sample rate non-pingpong only if internal clock is used
		if ( m_OldAcqConfig.u32ExtClk == 0 )
			AcqCfg.i64SampleRate	 = m_pCardState->SrInfoTable[0].llSampleRate;

		AcqCfg.i64TriggerTimeout = 0;
		AcqCfg.i64TriggerDelay   = 0;
		AcqCfg.i64TriggerHoldoff = 0;
		AcqCfg.i64SegmentSize	 = HEXAGON_CALIB_BUFFER_SIZE;
		AcqCfg.i64Depth			 = HEXAGON_CALIB_DEPTH;
		AcqCfg.i64TriggerHoldoff = AcqCfg.i64SegmentSize - AcqCfg.i64Depth;
		AcqCfg.u32SegmentCount   = 1;
		AcqCfg.u32Mode			 &= ~(CS_MODE_USER1|CS_MODE_USER2|CS_MODE_REFERENCE_CLK);

		m_pi16CalibBuffer = (int16 *)::GageAllocateMemoryX( HEXAGON_CALIB_BUFFER_SIZE*sizeof(int16) );
		if( NULL == m_pi16CalibBuffer)
			return CS_INSUFFICIENT_RESOURCES;

		RtlCopyMemory( m_OldTrigParams, m_pCardState->TriggerParams, sizeof(m_pCardState->TriggerParams));
		RtlCopyMemory( m_OldChanParams, m_pCardState->ChannelParams, sizeof(m_pCardState->ChannelParams));

		// Disable all triggers
		SendTriggerEngineSetting( 0 );		// first trigger engines table
		SendTriggerEngineSetting( 1 );		// second trigger engines table
		SendExtTriggerSetting();

		PresetDCLevelFreeze();			// Fix ADC DC Level Freeze.

	}
	else
	{
		::GageFreeMemoryX(m_pi16CalibBuffer);
		m_pi16CalibBuffer = NULL;

		RtlCopyMemory( m_pCardState->TriggerParams, m_OldTrigParams, sizeof(m_pCardState->TriggerParams));

		AcqCfg = m_OldAcqConfig;

		uInt8 u8IndexTrig = CSRB_TRIGENGINE_A1;
		i32Ret = SendTriggerEngineSetting(	0,
											  m_pCardState->TriggerParams[u8IndexTrig].i32Source,
											  m_pCardState->TriggerParams[u8IndexTrig].u32Condition,
											  m_pCardState->TriggerParams[u8IndexTrig].i32Level,
											  m_pCardState->TriggerParams[u8IndexTrig+1].i32Source,
											  m_pCardState->TriggerParams[u8IndexTrig+1].u32Condition,
											  m_pCardState->TriggerParams[u8IndexTrig+1].i32Level,
											  m_pCardState->TriggerParams[u8IndexTrig+2].i32Source,
											  m_pCardState->TriggerParams[u8IndexTrig+2].u32Condition,
											  m_pCardState->TriggerParams[u8IndexTrig+2].i32Level,
											  m_pCardState->TriggerParams[u8IndexTrig+3].i32Source,
											  m_pCardState->TriggerParams[u8IndexTrig+3].u32Condition,
											  m_pCardState->TriggerParams[u8IndexTrig+3].i32Level );
		u8IndexTrig = CSRB_TRIGENGINE_A3;
		i32Ret = SendTriggerEngineSetting(	1,
											  m_pCardState->TriggerParams[u8IndexTrig].i32Source,
											  m_pCardState->TriggerParams[u8IndexTrig].u32Condition,
											  m_pCardState->TriggerParams[u8IndexTrig].i32Level,
											  m_pCardState->TriggerParams[u8IndexTrig+1].i32Source,
											  m_pCardState->TriggerParams[u8IndexTrig+1].u32Condition,
											  m_pCardState->TriggerParams[u8IndexTrig+1].i32Level,
											  m_pCardState->TriggerParams[u8IndexTrig+2].i32Source,
											  m_pCardState->TriggerParams[u8IndexTrig+2].u32Condition,
											  m_pCardState->TriggerParams[u8IndexTrig+2].i32Level,
											  m_pCardState->TriggerParams[u8IndexTrig+3].i32Source,
											  m_pCardState->TriggerParams[u8IndexTrig+3].u32Condition,
											  m_pCardState->TriggerParams[u8IndexTrig+3].i32Level );
		if ( CS_FAILED(i32Ret) )
			return i32Ret;

		SendExtTriggerSetting( (BOOLEAN) m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].i32Source,
									m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].i32Level,
									m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32Condition,
									m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange, 
									m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance,
									m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtCoupling );

		if ( CS_FAILED(i32Ret) )
			return i32Ret;
	}


	i32Ret = CsSetAcquisitionConfig( &AcqCfg );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Update kernel segment settings
	CSACQUISITIONCONFIG_EX AcqCfgEx = {0};

	AcqCfgEx.AcqCfg.u32Size		= sizeof(CSACQUISITIONCONFIG);
	CsGetAcquisitionConfig( &AcqCfgEx.AcqCfg );

	AcqCfgEx.i64HwDepth			= m_i64HwPostDepth;
	AcqCfgEx.i64HwSegmentSize	= m_i64HwSegmentSize;
	AcqCfgEx.u32Decimation		= bSetup ? 1 : m_pCardState->AddOnReg.u32Decimation;
	AcqCfgEx.u32ExpertOption	= m_PlxData->CsBoard.u32BaseBoardOptions[m_pCardState->i32ConfigBootLocation];

	IoctlSetAcquisitionConfigEx( &AcqCfgEx ) ;

	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsHexagon::CalibrateAllChannels(bool bForce)
{
	if( !IsCalibrationRequired() && !bForce)
	{
		return CS_SUCCESS;
	}

	int32	i32Ret = CS_SUCCESS;
	int32	i32Res = CS_SUCCESS;
	bool	bForceCalib = false;
	uInt16	u16UserChan = 1;

	if ( m_bSkipCalib || m_bUseEepromCal )
	{
		TraceCalib( 0, TRACE_CAL_PROGRESS, 30);

		for( u16UserChan = 1; u16UserChan <= m_PlxData->CsBoard.NbAnalogChannel; u16UserChan += GetUserChannelSkip() )
		{
			uInt16 u16HwChan = ConvertToHwChannel(u16UserChan);

			if ( !m_bForceCal[u16HwChan-1] )
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

	// Before calibrate the front end, wait a bit to make sure that all relays are in sable position
	// Sleep(m_u32SettleDelayBeforeCalib);

	TraceCalib( 0, TRACE_CAL_PROGRESS, 30);
	//--------------------------------------------------------------------
	i32Ret = SetupForCalibration( true );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Switch to the Calibmode DC
	i32Ret = SendCalibModeSetting( ecmCalModeDc );
	if( CS_FAILED(i32Ret) )
           goto EndCalibrateAll;

	for( u16UserChan = 1; u16UserChan <= m_PlxData->CsBoard.NbAnalogChannel; u16UserChan += GetUserChannelSkip() )
	{
		i32Res = CalibrateFrontEnd( ConvertToHwChannel(u16UserChan) );
		if( CS_FAILED(i32Res) )
			goto EndCalibrateAll;
	}

	// Compare all channel gains, the diff may not exceed 1db. not validate for low range board. 
	if (!m_bLowRange_mV)
		i32Res = ValidateAllChannelGain();

EndCalibrateAll:   
	i32Ret = SendCalibModeSetting();
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	//--------------------------------------------------------------------
	TraceCalib( 0, TRACE_CAL_PROGRESS, 31);
	i32Ret = SetupForCalibration( false );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	return i32Res;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void CsHexagon::TraceCalib(uInt16 u16Channel, uInt32 u32TraceFlag, uInt32 u32V1, uInt32 u32V2, int32 i32V3, int32 i32V4)
{
	char	szText[256];

	szText[0] = 0;
	uInt32 u32TraceType = (u32TraceFlag &  m_u32DebugCalibTrace);
	
	switch (u32TraceType)
	{
		case TRACE_COARSE_OFFSET:
			if ( 0 == u32V1 && 0 == i32V3 )
				sprintf_s( szText, sizeof(szText),  "\nChan %d  Coarse DC calibration\n", u16Channel );
			else
				sprintf_s( szText, sizeof(szText),  "Ch%d. Coarse calibration DAC 0x%04x (%4d) residual offset %d.%ld\n",
						u16Channel, u32V1, u32V1, i32V3/10, labs(i32V3)%10 );
			break;

		case TRACE_FINE_OFFSET:
			sprintf_s( szText, sizeof(szText),  "Ch%d. Fine calibration DAC 0x%04x (%4d) residual offset %c%ld.%ld (Tar %c%ld.%ld)\n",
				u16Channel, u32V1, u32V1, i32V4>0?' ':'-', labs(i32V4)/10, labs(i32V4)%10, i32V3>0?' ':'-', labs(i32V3)/10, labs(i32V3)%10 );
			break;
			
		case TRACE_CAL_GAIN:
		{
//			float fVolt =  (-1.0*u32V2/m_pCardState->AcqConfig.i32SampleRes/10)*1000;
			
			sprintf_s( szText, sizeof(szText),  "Ch%d. Gain calibration DAC 0x%04x (%4d) gain (%5d)\n",
				u16Channel, u32V1, u32V1, u32V2/10);
		}
		break;
			
		case TRACE_CAL_POS:
			sprintf_s( szText, sizeof(szText),  "Ch%d. Position calibration Pos DAC 0x%04x (%4d) Neg DAC 0x%04x (%4d)\n",
				u16Channel, u32V1, u32V1, u32V2, u32V2);
			break;
			
		case TRACE_CAL_SIGNAL:
		{
			bool bVersose = (0 != (TRACE_CAL_SIGNAL_VERB & m_u32DebugCalibTrace));
			if( bVersose )
			{
				sprintf_s( szText, sizeof(szText),  "=== Cal Signal in %d mV %s range. \n",
					u32V2, u32V1 == 50?"50 Ohm":"1 MOhm");
				::OutputDebugString( szText );
			}
			sprintf_s( szText, sizeof(szText),  "Ch%d. Cal Signal:   input range %d Target %c%ld.%03ld mV \n", u16Channel, u32V2, 
				i32V3>0?' ':'-', labs(i32V3)/1000, labs(i32V3)%1000);

		}
		break;
		
		case TRACE_CAL_SIGNAL_ITER:
		{
			sprintf_s( szText, sizeof(szText),  "       Reading sample %2d (%2d valid) Code %+5d\n",
				u32V1, u32V2, i32V3);
		}
		break;
		
		case TRACE_CAL_DAC:
			sprintf_s( szText, sizeof(szText),  "+ + +  Writing  %5d [0x%04x] to the DAC %2d (Ch. %d)\n", u32V1, u32V1, u32V2, u16Channel);
			break;
		
		case TRACE_ALL_DAC:
			sprintf_s( szText, sizeof(szText),  "+|+|+  Writing  0x%04x to the DAC 0x%04x (delay %d)\n", u32V1&0xFFFF, u32V2, u32V1 >>16 ); 
			break;
			
		case TRACE_CAL_RESULT:
		{
			uInt32 u32RangeIndex, u32FeModIndex;
			int32 i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );
			if ( CS_SUCCEEDED(i32Ret) )
			{
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
			else
			{
				sprintf_s( szText, sizeof(szText),  "       TRACE_CAL_RESULT invalid parameters : u32RangeIndex %d u32FeModIndex %d \n",
					u32RangeIndex,
					u32FeModIndex);
				::OutputDebugString( szText );
			
			}
		}
		break;
		
		case TRACE_CAL_RANGE:
		{
			sprintf_s( szText, sizeof(szText),  "- - Input %s Range Â±%d.%d V Signal %c%ld.%03ld mV Noise %d.%d LSB Settled by %d Sample.\n%s",
				u32V2==0?"Ref. ":(u32V2==1?"Src. ":"Chan."), u32V1/1000, (u32V1/100)%10, 
				i32V3>0?' ':'-', labs(i32V3/1000), labs(i32V3)%1000, i32V4/10, i32V4%10, u16Channel, u32V2==0?"\n":"");
		}
		break;
		
		case TRACE_CAL_PROGRESS:
		{
			if (!m_bLogFailureOnly)
			{
				switch (u32V1)
				{
					case 1:		//SendCalibModeSetting
						switch(eCalMode(u32V2))
						{
							case ecmCalModeDc:     sprintf_s( szText, sizeof(szText), "* * * Chan %d switched to DC calibration mode\n", u16Channel ); break;
							case ecmCalModeNormal: sprintf_s( szText, sizeof(szText), "* * * Chan %d switched to channel input  mode\n", u16Channel ); break;
							case ecmCalModeMs:     sprintf_s( szText, sizeof(szText), "* * * Chan %d switched to MS calibration mode\n", u16Channel ); break;
						}
					break;
					case 2:		//m_bSkipCalib
						sprintf_s( szText, sizeof(szText),  "___ Skipped calibration of the channel %d.\n", u16Channel ); 
						break;
					case 3:		//m_bUseEEpromCal
						sprintf_s( szText, sizeof(szText),  "### Using storred calibration values for the channel %d.\n", u16Channel ); 
						break;
					case 4:					
						sprintf_s( szText, sizeof(szText),  "~~~ Using default calibration values for the channel %d ( flag 0x%02X ).\n", u16Channel, u32V2 ); 
						break;
					case 10:	 //Offset
						sprintf_s( szText, sizeof(szText),  "     Offset calibration of the channel %d has succeeded.\n\n", u16Channel); 
						break;
					case 11: 	//Gain
						sprintf_s( szText, sizeof(szText),  "     Gain calibration of the channel %d has succeeded.\n\n", u16Channel); 
						break;
					case 12: 	//Position
						sprintf_s( szText, sizeof(szText),  "     Position calibration of the channel %d has succeeded.\n\n", u16Channel); 
						break;
					case 20: 	//overall one channel
						sprintf_s( szText, sizeof(szText),  "     Starting calibration of the channel %d %5d mV %s\n", 
									u16Channel, m_pCardState->ChannelParams[u16Channel-1].u32InputRange, 
									50==m_pCardState->ChannelParams[u16Channel-1].u32Impedance?"50 Ohm":"1 MOhm"); 
						break;
					case 21: 	//overall one channel
						sprintf_s( szText, sizeof(szText),  "     Calibration of the channel %d has succeeded.\n\n", u16Channel); 
						break;
					case 30: 	//overall all channels
						sprintf_s( szText, sizeof(szText),  "\\/    Calibration sequence has started.\n"); 
						break;
					case 31:  	//overall all channels
						sprintf_s( szText, sizeof(szText),  "/\\    Calibration sequence has finished.\n"); 
						break;
					case 1001: 	//Offset failed
						sprintf_s( szText, sizeof(szText),  "! ! ! Offset calibration of the channel %d has failed. Error %d \n", u16Channel, (i32V3%0x10000) ); 
						break;
					case 1002: 	//gain failed
						sprintf_s( szText, sizeof(szText),  "! ! ! Gain calibration of the channel %d has failed. Error %d \n", u16Channel, (i32V3%0x10000) ); 
						break;
					case 1003: 	//position failed
						sprintf_s( szText, sizeof(szText),  "! ! ! Position calibration of the channel %d has failed. Error %d \n", u16Channel, (i32V3%0x10000) ); 
						break;
				}
			}
		}
		break;
		
		case TRACE_CAL_ITER:
		{
			double fScale = m_bLowRange_mV ? (HEXAGON_GAIN_LR_MV/2) : (CS_GAIN_2_V/2);
			if( 0 != (m_u32DebugCalibTrace & u32V2) )
			{
				if (u32V2==TRACE_COARSE_OFFSET)
				{
					double	fVolt = fScale*(i32V3-m_pCardState->AcqConfig.i32SampleOffset);
					fVolt /= m_pCardState->AcqConfig.i32SampleRes;
					sprintf_s( szText, sizeof(szText),  "\t :o: Code 0x%04x   Meas 0x%04x ( %.3fmV)\n", u32V1, i32V3, fVolt );
				}
				else
				if (u32V2 ==TRACE_CAL_GAIN)
				{
					/* Refer to CalibrateGain. compute in mv */
					double	fVolt =  (-1.0*i32V3/m_pCardState->AcqConfig.i32SampleRes/10)*fScale;
					double	fVTarget = 1.0*(i32V4);

					sprintf_s( szText, sizeof(szText),  "\t :g: Code %2d [0x%02x] Meas %3.6fmv (%3.6fmv)\n",
					u32V1, u32V1, fVolt, fVTarget); 
				}
				else
				{	
					sprintf_s( szText, sizeof(szText),  "\t : : Code %4d [0x%4x] Meas %c%5ld.%ld (%c%5ld.%ld)\n",
					u32V1, u32V1, i32V3>=0?' ':'-', labs(i32V3)/10, labs(i32V3)%10,
					i32V4>=0?' ':'-', labs(i32V4)/10, labs(i32V4)%10); 
				}
			}
		}
		break;


	}

	if ( szText[0] != 0 )
		::OutputDebugString( szText );
}




//	VCAL_DC voltage is
//  Vvr = Vref * B /M *(R10+R8)/R10
//	Vcal = ((R171+R220)/R171) * (R165/(R212+R165)) * Vvr - (R220/R171) * Va
//  Va = Vvr * A/M
//  let assign k as ((R171+R220)/R171) * (R165/(R212+R165)) and D as (R220/R171), then
//	Vcal = (k - D*A/M)* Vvr

//  Max Vcal = Vvr * k   Min Vcal = Vvr * ( k - D )
//  Absolute values of Min amd max Vcal should exceed front end span thus
//  Vvr * Min(k, D-k) = Fe
//  Vvr = Fe/Min(k, D-k) 
//  Vref * (B/M)*(R10+R8)/R10 = Fe/Min(k, D-k) 
//  B = (M*Fe*R10)/(Vref* Min(K, D-K)*(R8+R10))

// To measure Vvr:
// U1code = (Vvr-Vr)/Vr * MU1/2 + MU1/2
// 2*U1code = (Vvr/Vr) * MU1
// Vvr = (2*U1code)*Vr/ MU1


// Due to influence of the Vvr there is desrepancy between Calibration voltage 
// and input one, namely
// Vc = Vo((Rref||Rin)/((Rref||Rin)+Rcal))+Vvr((Rcal||Rin)/((Rcal||Rin)+Rref))
// Vin = Vo(Rin/(Rin+Rcal)), where Vo - voltage @ output of U37, Vin target input voltage, Rref = R26+R28, Rcal = R19, Rin impedance of the input
// making substitution and solving for  Vc, we have

// Vc = Vin((R19+Rin)*(R26+R28)/((R26+R28)*Rin + R19*(R26+R28+Rin)) + Vvr ((R19*Rin)/(R19*Rin+ (R26+R28)*(R19+Rin)))
// marking (R19+Rin)*(R26+R28)/((R26+R28)*Rin + R19*(R26+R28+Rin) as P and
// (R19*Rin)/(R19*Rin+ (R26+R28)*(R19+Rin)) as Q
// we have Vc = Vin * P + Vvr * Q and Vin = (Vc - Vr *Q)/P


const uInt32 CsHexagon::c_u32CalDacCount = 4096;		// DAC128S085
const uInt32 CsHexagon::c_u32CalAdcCount = 4096;


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsHexagon::FastCalibrationSettings(uInt16 u16Channel, bool bForceSet )
{
	uInt32 u32RangeIndex, u32FeModIndex;
	int32 i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	uInt16 u16ChanZeroBased = u16Channel - 1;

	UNREFERENCED_PARAMETER(bForceSet);
	

	//if ( bForceSet ||
	//	( m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].bCalibrated  && ! m_pCardState->bFastCalibSettingsDone[u16ChanZeroBased] ) )
	{
		i32Ret = UpdateCalibDac(u16Channel, ecdPosition, m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = UpdateCalibDac(u16Channel, ecdOffset, m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16FineOffset);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = SendPosition( u16Channel, m_pCardState->ChannelParams[u16ChanZeroBased].i32Position );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		// Update new gain adjustment for the ADC
		i32Ret = UpdateCalibDac(u16Channel, ecdGain, m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16Gain);
		//i32Ret = AdjustAdcGain(u16Channel, m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16Gain);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		m_pCardState->bFastCalibSettingsDone[u16ChanZeroBased] = true;
		m_pCardState->bCalNeeded[u16ChanZeroBased] = false;
		TraceCalib( u16Channel, TRACE_CAL_RESULT);
		return CS_SUCCESS;
	}

//	return CS_FALSE;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void CsHexagon::SetFastCalibration(uInt16 u16Channel)
{
	uInt32	u32TimeElapsed = 0;		// in minutes
	int32	i32Ret = CS_SUCCESS;
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
		i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex,  u32FeModIndex );
		if( CS_SUCCEEDED(i32Ret) )
		{
			m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].bCalibrated = true;
		}
	}

}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool CsHexagon::ForceFastCalibrationSettingsAllChannels()
{
	bool bRetVal = true;

	if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL )
	{
		for( uInt16 i = 1; i <= GetBoardNode()->NbAnalogChannel; i ++ )
		{
			if( ! FastCalibrationSettings( i, true ) )
				bRetVal = false;
		}
	}
	else
	{
		if( ! FastCalibrationSettings( ConvertToHwChannel( CS_CHAN_1 ), true ) )
			bRetVal = false;
	}

	return bRetVal;
}

#ifdef _WINDOWS_

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void CsHexagon::ReadCalibrationSettings(void *Params)
{
	ULONG	ul = 0;
	DWORD	DataSize = sizeof(ul);
	HKEY	hKey = (HKEY) Params;


	ul = m_bSkipCalib ? 1 : 0;
	::RegQueryValueEx( hKey, _T("FastDcCal"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bSkipCalib = 0 != ul;

	ul = m_u16SkipCalibDelay;
	::RegQueryValueEx( hKey, _T("NoCalibrationDelay"), NULL, NULL, (LPBYTE)&ul, &DataSize );

	if(ul > 24 * 60 ) //limitted to 1 day
		ul = 24*60;
	m_u16SkipCalibDelay = (uInt16)ul;

	ul = m_bNeverCalibrateDc ? 1 : 0;
	::RegQueryValueEx( hKey, _T("NeverCalibrateDc"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bNeverCalibrateDc = 0 != ul;

	ul = m_bNeverCalibExtTrig ? 1 : 0;
	::RegQueryValueEx( hKey, _T("NeverCalibExtTrig"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bNeverCalibExtTrig = (ul != 0);

	ul = m_bNeverCalibrateGain ? 1: 0;
	::RegQueryValueEx( hKey, _T("NeverCalibrateGain"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bNeverCalibrateGain = 0 != ul;

	ul = m_u32IgnoreCalibError;
	::RegQueryValueEx( hKey, _T("IgnoreCalibError"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u32IgnoreCalibError = ul;

	ul = m_u32DebugCalibTrace;
	::RegQueryValueEx( hKey, _T("TraceCalib"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u32DebugCalibTrace = ul;

	ul = m_bLogFailureOnly?1:0;
	::RegQueryValueEx( hKey, _T("LogFailureOnly"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bLogFailureOnly = ul!=0;

	ul = m_u16GainCode_2vpp ? 1 : 0;				// Gain code 2V peak
	::RegQueryValueEx( hKey, _T("ForceGainCode2V"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u16GainCode_2vpp = (uInt16)ul;

	ul = m_bNoDCLevelFreeze ? 1 : 0;				// DC Level Freeze
	::RegQueryValueEx( hKey, _T("NoDCLevelFreeze"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bNoDCLevelFreeze = ul != 0;

	// Remove this key together with PresetDCLevelFreeze
	ul = m_bNoDCLevelFreezePreset ? 1 : 0;				// DC Level Freeze Preset (Fix for DC Level Freeze)
	::RegQueryValueEx( hKey, _T("NoDCLevelFreezePreset"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bNoDCLevelFreezePreset = ul != 0;

	m_u32CommitTimeDelay = COMMIT_TIME_DELAY_MS;
	ul = m_u32CommitTimeDelay;
	::RegQueryValueEx( hKey, _T("CommitTimeDelay"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u32CommitTimeDelay = ul;

	m_u32GainTolerance_lr = DEFAULT_GAIN_TOLERANCE_LR;
	ul = m_u32GainTolerance_lr;
	::RegQueryValueEx( hKey, _T("GainTolerance_lr"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u32GainTolerance_lr = ul;

}
#else

void CsHexagon::ReadCalibrationSettings(char *szIniFile, void *Params)
{
	ULONG	ul = 0;
	DWORD	DataSize = sizeof(ul);
   
   char *szSectionName = (char *)Params;
 
	ul = m_bSkipCalib ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("FastDcCal"),  ul,  szIniFile);	
	m_bSkipCalib = 0 != ul;

	ul = m_u16SkipCalibDelay;
   ul = GetCfgKeyInt(szSectionName, _T("NoCalibrationDelay"),  ul,  szIniFile);	

	if(ul > 24 * 60 ) //limitted to 1 day
		ul = 24*60;
	m_u16SkipCalibDelay = (uInt16)ul;

	ul = m_bNeverCalibrateDc ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("NeverCalibrateDc"),  ul,  szIniFile);	
	m_bNeverCalibrateDc = 0 != ul;

	ul = m_bNeverCalibExtTrig ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("NeverCalibExtTrig"),  ul,  szIniFile);	
	m_bNeverCalibExtTrig = (ul != 0);

	ul = m_bNeverCalibrateGain ? 1: 0;
   ul = GetCfgKeyInt(szSectionName, _T("NeverCalibrateGain"),  ul,  szIniFile);	
	m_bNeverCalibrateGain = 0 != ul;

	ul = m_u32IgnoreCalibError;
   ul = GetCfgKeyInt(szSectionName, _T("IgnoreCalibError"),  ul,  szIniFile);	
	m_u32IgnoreCalibError = ul;

	ul = m_u32DebugCalibTrace;
   ul = GetCfgKeyInt(szSectionName, _T("TraceCalib"),  ul,  szIniFile);	
	m_u32DebugCalibTrace = ul;
	
	ul = m_bLogFailureOnly ?1:0;
   ul = GetCfgKeyInt(szSectionName, _T("LogFaluireOnly"),  ul,  szIniFile);	
	m_bLogFailureOnly = ul!=0;

	ul = m_u16GainCode_2vpp ? 1 : 0;				// Gain code 2V peak
   ul = GetCfgKeyInt(szSectionName, _T("ForceGainCode2V"),  ul,  szIniFile);	
	m_u16GainCode_2vpp = (uInt16)ul;

	ul = m_bNoDCLevelFreeze ? 1 : 0;				// DC Level Freeze
   ul = GetCfgKeyInt(szSectionName, _T("NoDCLevelFreeze"),  ul,  szIniFile);	
	m_bNoDCLevelFreeze = ul != 0;

	// Remove this key together with PresetDCLevelFreeze
	ul = m_bNoDCLevelFreezePreset ? 1 : 0;				// DC Level Freeze Preset (Fix for DC Level Freeze)
   ul = GetCfgKeyInt(szSectionName, _T("NoDCLevelFreezePreset"),  ul,  szIniFile);	
	m_bNoDCLevelFreezePreset = ul != 0;

	m_u32CommitTimeDelay = COMMIT_TIME_DELAY_MS;
	ul = m_u32CommitTimeDelay;				
	ul = GetCfgKeyInt(szSectionName, _T("CommitTimeDelay"),  ul,  szIniFile);	
	if (ul)
		m_u32CommitTimeDelay = ul;

	m_u32GainTolerance_lr = DEFAULT_GAIN_TOLERANCE_LR;
	ul = m_u32GainTolerance_lr ;				
	ul = GetCfgKeyInt(szSectionName, _T("GainTolerance_lr"),  ul,  szIniFile);	
	if (ul)
		m_u32GainTolerance_lr = ul ;
}

#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsHexagon::WriteToAdcRegister(uInt16 u16Channel, uInt16 u16Reg, uInt16 u16Data)
{
	uInt32 u32Cmd = SPI_WRTIE_CMD;

	if ( 1 == u16Channel )
		u32Cmd |= HEXAGON_ADC2;	
	else
		u32Cmd |= HEXAGON_ADC1;	

	u32Cmd |= u16Reg << 8;
		
	int32 i32Ret = WriteRegisterThruNios( u16Data, u32Cmd );
	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsHexagon::FindFeCalIndices( uInt16 u16Channel, uInt32& u32RangeIndex,  uInt32& u32FeModIndex )
{
	uInt16 u16ChanZeroBased = u16Channel-1;
	
	int32 i32Ret = FindFeIndex( m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange, u32RangeIndex);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	u32FeModIndex = 0;
	if(CS_REAL_IMP_1M_OHM == m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance)
		u32FeModIndex += HEXAGON_MAX_FE_SET_IMPED_INC;
	else if (CS_REAL_IMP_50_OHM != m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance)
		return CS_INVALID_IMPEDANCE;

	if( CS_COUPLING_AC == (m_pCardState->ChannelParams[u16ChanZeroBased].u32Term & CS_COUPLING_MASK) )
		u32FeModIndex += HEXAGON_MAX_FE_SET_COUPL_INC;
	else if( CS_COUPLING_DC != (m_pCardState->ChannelParams[u16ChanZeroBased].u32Term & CS_COUPLING_MASK) )
		return CS_INVALID_COUPLING;

	if(0 != m_pCardState->ChannelParams[u16ChanZeroBased].u32Filter)
		u32FeModIndex += HEXAGON_MAX_FE_SET_FLTER_INC;
	
	return CS_SUCCESS;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsHexagon::FindFeIndex( const uInt32 u32Swing, uInt32& u32Index )
{
	for( uInt32 i = 0; i < m_pCardState->u32FeInfoSize; i++ )
	{
		if ( u32Swing == m_pCardState->FeInfoTable[i].u32Swing_mV )
		{
			u32Index = i;
			return CS_SUCCESS;
		}
	}

	return CS_INVALID_GAIN;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
uInt16 CsHexagon::GetControlDacId( eCalDacId ecdId, uInt16 u16Channel )
{
	switch ( ecdId )
	{
		case ecdPosition: return HEXAGON_POSITION_1 + u16Channel - 1;
		case ecdOffset: return HEXAGON_VCALFINE_1 + u16Channel - 1;
		case ecdGain: return HEXAGON_CAL_GAIN_1 + u16Channel - 1;
	}

	return 0xffff;
}


//-----------------------------------------------------------------------------
//	User channel skip depends on mode amd number of channels per card
//-----------------------------------------------------------------------------
uInt16 CsHexagon::GetUserChannelSkip()
{
	uInt16	u16ChannelSkip = 0;

	switch (m_PlxData->CsBoard.NbAnalogChannel)
	{
		case 4:
			{
			if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_QUAD )
				u16ChannelSkip = 1;
			else if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL )
				u16ChannelSkip = 2;
			else if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE )
				u16ChannelSkip = 4;
			}
			break;
		case 2:
			{
			if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL )
				u16ChannelSkip = 1;
			else if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE )
				u16ChannelSkip = 2;
			}
			break;
		case 1:
			{
			if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE )
				u16ChannelSkip = 1;
			}
			break;
	}

	ASSERT( u16ChannelSkip != 0 );
	return u16ChannelSkip;
}

//------------------------------------------------------------------------------
// Calibrate offset use the biggest gain value, then value found will be used 
// for all input range.
//------------------------------------------------------------------------------
int32 CsHexagon::CalibrateOffset(uInt16	u16HwChannel)
{
	int32	i32Ret = CS_SUCCESS;

	bool	bGoUp			= false;
	uInt16  LastBOffset		= 0;
	int32   LastBTarget		= 0;
	uInt16	u16ChanZeroBased = u16HwChannel-1;
	uInt32	u32InpuRange	= m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange;
	double	fVolt			= 0;
	int32	i32Avg_Read		= 0;

	HEXAGON_CAL_LUT CalInfo;
	i32Ret = FindCalLutItem(u32InpuRange, 50, CalInfo);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

#ifdef DBG_CALIB_OFFSET
	sprintf_s( szDbgText, sizeof(szDbgText),  "\n----- CalibrateOffset(%d) ENTER  ------------------------- \n", u16HwChannel); 
	OutputDebugString(szDbgText);
#endif

	uInt32 u32RangeIndex, u32FeModIndex;
	i32Ret = FindFeCalIndices( u16HwChannel, u32RangeIndex, u32FeModIndex );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Initial middle values for DACs
	i32Ret = UpdateCalibDac(u16HwChannel, ecdPosition, HEXAGON_DC_OFFSET_DEFAULT_VALUE, true );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret = UpdateCalibDac(u16HwChannel, ecdOffset, HEXAGON_DC_OFFSET_DEFAULT_VALUE, true );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Update calib gain. Amplify the DC offset by using lower theory gain code.
	i32Ret = UpdateCalibDac(u16HwChannel, ecdGain,  0 /* CalInfo.u16GainCtrl */, true );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Set Calib DC to 0V
	i32Ret = WriteRegisterThruNios( HEXAGON_CAL_SRC_0V, HEXAGON_ADDON_CPLD_CALSRC_REF | HEXAGON_SPI_WRITE );
	if( CS_FAILED(i32Ret) )
		return i32Ret;
	BBtiming( 350 );

#ifdef DBG_CALIB_INFO
	int32	i32Avg_x10_ref	= 0;
	i32Ret = AcquireAndAverage(u16HwChannel, &i32Avg_x10);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	// Read Calib ADC reference
	ReadCalibADCRef(&i32Avg_x10_ref);

	sprintf_s( szDbgText, sizeof(szDbgText),  "ReadCalibADCRef(%d) i32Avg_x10_ref 0x%x i32Avg_ref 0x%x \n", 
					u16HwChannel, i32Avg_x10_ref, i32Avg_x10_ref/10);
	OutputDebugString(szDbgText);
#endif

	// try one direction, if not found then change direction
	i32Ret = FindCalibOffset(u16HwChannel, ecdPosition, bGoUp, &LastBOffset, &i32Avg_Read);
	if ( i32Ret!=CS_SUCCESS || !LastBOffset )
	{
		bGoUp = !bGoUp;
		i32Ret = FindCalibOffset(u16HwChannel, ecdPosition, bGoUp, &LastBOffset, &i32Avg_Read);
	}

	// if calibration failed, use default
	if ( (!LastBOffset) || ( Abs( Abs(LastBOffset) - Abs(HEXAGON_DC_OFFSET_DEFAULT_VALUE) ) > 0x100) )
	{
		sprintf_s( szErrorStr, sizeof(szErrorStr), "%d, Channel %d Coarse Offset calib failed (0x%x )! use default offset 0x%x ", 
				CS_COARSE_OFFSET_CAL_FAILED, ConvertToUserChannel(u16HwChannel), LastBOffset, HEXAGON_DC_OFFSET_DEFAULT_VALUE );
		m_EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );
		LastBOffset = HEXAGON_DC_OFFSET_DEFAULT_VALUE;

		if ( 0 != ( m_u32IgnoreCalibError  & (1 << (ConvertToUserChannel(u16HwChannel)-1)) ) )
			i32Ret = CS_SUCCESS;
		else
			i32Ret = CS_COARSE_OFFSET_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16HwChannel);
	}

	TraceCalib( u16HwChannel, TRACE_COARSE_OFFSET, LastBOffset, 0, LastBTarget );
	UpdateCalibDac(u16HwChannel, ecdPosition, LastBOffset, true );

//EndCalibrateOffset:   
	{
		int32	i32Res = CS_SUCCESS;

		// Restore the current Gain
		i32Res = UpdateCalibDac(u16HwChannel, ecdGain, CalInfo.u16GainCtrl, true );

		// i32Ret = AcquireAndAverage(u16HwChannel, &i32Avg_x10);
		fVolt = 1000*(i32Avg_Read - m_pCardState->AcqConfig.i32SampleOffset);
		fVolt /= m_pCardState->AcqConfig.i32SampleRes;

		m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].fVoltOff = fVolt;

		return i32Ret;
	}
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsHexagon::CalibratePosition(uInt16 u16Channel)
{
	int32 i32Ret = CS_SUCCESS;
	int32 i32Res = CS_SUCCESS;
	if( (0 == u16Channel) || (u16Channel > HEXAGON_CHANNELS)  )
		return CS_INVALID_CHANNEL;

	uInt16 u16ChanZeroBased = u16Channel-1;
	int32 i32PosCode = 0;
	uInt32 u32Code = 0;
	uInt32 u32NegPos = 0;
	bool bSuccses = true;

	uInt32 u32InpuRange = m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange;
	HEXAGON_CAL_LUT CalInfo;
	uInt32 u32CalRange = u32InpuRange;

	//DumpAverageTable(u16Channel, ecdPosition, 0, 0, 32);

	uInt32 u32RangeIndex, u32FeModIndex;
	i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );
	if( CS_FAILED(i32Ret) )
		return i32Ret;
	m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CodeDeltaForHalfIR = 0;

	// Find the setup for the current input ranges
	i32Ret = FindCalLutItem(u32CalRange, 50, CalInfo);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Positive side
	// Find the value of DAC so that we have DC offset at +25%
	int32 i32Target = labs(m_pCardState->AcqConfig.i32SampleRes)*5;		//1/2 gain = SampleRes * 10 /2
	int32 i32Avg_x10 = 0;
	uInt32 u32PosPos = 0, u32PosDelta = c_u32CalDacCount/4;

	while( u32PosDelta > 0 )
	{
		i32PosCode = m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset;
		i32PosCode += u32PosPos|u32PosDelta;
		u32Code = i32PosCode > 0 ? ( i32PosCode < int32(c_u32CalDacCount-1) ? uInt16(i32PosCode) : c_u32CalDacCount-1 ) : 0;
		
		i32Ret = UpdateCalibDac(u16Channel, ecdPosition, (uInt16)u32Code, false );

		if( CS_FAILED(i32Ret) )	
		{
			i32Res = CS_POSITION_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
			goto EndCalibratePosition;
		}

		BBtiming( 100 );

		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )	
		{
			i32Res = CS_POSITION_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
			goto EndCalibratePosition;
		}

		TraceCalib( u16Channel, TRACE_CAL_ITER, u32Code, TRACE_CAL_POS, i32Avg_x10/10, i32Target);
		if( Abs(i32Avg_x10) < Abs(i32Target) )
			u32PosPos |= u32PosDelta;
		u32PosDelta >>= 1;
	}
	
	if( 0 == u32PosPos || (c_u32CalDacCount - 1) == u32PosPos )
	{
		i32Ret = UpdateCalibDac(u16Channel, ecdPosition, m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset, false );
		sprintf_s( szDbgText, sizeof(szDbgText),  "CalibratePosition failed! Positive side. channel %d\n", u16Channel);
		OutputDebugString(szDbgText);

		sprintf_s( szErrorStr, sizeof(szErrorStr), "%d, Channel %d Pos side", 
				CS_POSITION_CAL_FAILED, ConvertToUserChannel(u16Channel) );
		m_EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );

		bSuccses = false;
	}

	// Negative side
	// Find the value of DAC so that we have DC offset at -25%
	i32Target = m_pCardState->AcqConfig.i32SampleRes*5;		//1/2 gain = SampleRes * 10 /4
	u32PosDelta = c_u32CalDacCount/4;

	while( u32PosDelta > 0 )
	{
		i32PosCode = m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset;
		i32PosCode -= u32NegPos|u32PosDelta;
		u32Code = i32PosCode > 0 ? ( i32PosCode < int32(c_u32CalDacCount-1) ? uInt32(i32PosCode) : c_u32CalDacCount-1 ) : 0;

		i32Ret = UpdateCalibDac(u16Channel, ecdPosition, (uInt16)u32Code, false );
		if( CS_FAILED(i32Ret) )	
		{
			i32Res = CS_POSITION_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
			goto EndCalibratePosition;
		}
		BBtiming( 100 );

		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )	
		{
			i32Res = CS_POSITION_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
			goto EndCalibratePosition;
		}

		TraceCalib( u16Channel, TRACE_CAL_ITER, u32Code, TRACE_CAL_POS, i32Avg_x10/10, i32Target);
		if( Abs(i32Avg_x10) < Abs(i32Target) )
			u32NegPos |= u32PosDelta;
		u32PosDelta >>= 1;
	}
	
	if( 0 == u32NegPos || (c_u32CalDacCount - 1) == u32NegPos )
	{
		// Write default value.
		i32Ret = UpdateCalibDac(u16Channel, ecdPosition, m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset, false );
		sprintf_s( szDbgText, sizeof(szDbgText),  "CalibratePosition failed! Negative side. channel %d\n", u16Channel);
		OutputDebugString(szDbgText);

		sprintf_s( szErrorStr, sizeof(szErrorStr), "%d, Channel %d Neg side", 
				CS_POSITION_CAL_FAILED, ConvertToUserChannel(u16Channel) );
		m_EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );

		bSuccses = false;
	}
	
EndCalibratePosition:
	i32Ret = UpdateCalibDac(u16Channel, ecdPosition, m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset, true );

	m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CodeDeltaForHalfIR = (uInt16) (u32PosPos + u32NegPos)/2;
	
	TraceCalib( u16Channel, TRACE_CAL_POS, u32PosPos, u32NegPos);

#ifdef DBG_CALIB_POSITION
	sprintf_s( szDbgText, sizeof(szDbgText), "CalibratePosition(%d) result. u16CodeDeltaForHalfIR [%d] u32PosPos 0x%x u32NegPos 0x%x \n", 
				u16Channel,
				m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CodeDeltaForHalfIR,
				u32PosPos,
				u32NegPos);
	OutputDebugString(szDbgText);
#endif

	if( !bSuccses ) 
	{
		if ( 0 != ( m_u32IgnoreCalibError  & (1 << (ConvertToUserChannel(u16Channel)-1)) ) )
			i32Res = CS_SUCCESS;
		else
			i32Res = CS_POSITION_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
	}

	return i32Res;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
//ex:			TraceDebugCalib( u16HwChannel, TRACE_CAL_ITER, u16Pos|u16PosDelta, TRACE_COARSE_OFFSET, i32Avg_x10);
//				TraceDebugCalib( u16HwChannel, TRACE_COARSE_OFFSET );

void CsHexagon::TraceDebugCalib(uInt16 u16Channel, uInt32 u32TraceFlag, uInt32 u32V1, uInt32 u32V2, int32 i32V3, int32 i32V4)
{
	char	szText[256];

	szText[0] = 0;

	if( 0 == (u32TraceFlag &  m_u32DebugCalibTrace) )
		return;

	switch(u32TraceFlag)
	{
		case TRACE_COARSE_OFFSET:
			sprintf_s( szText, sizeof(szText),  "\nHwChan %d  Coarse DC calibration\n", u16Channel );		
			break;

		case TRACE_CAL_ITER:
			if (u32V2==TRACE_COARSE_OFFSET)
			{
				float	fVolt = (float) 1000*(i32V3-m_pCardState->AcqConfig.i32SampleOffset);
				fVolt /= m_pCardState->AcqConfig.i32SampleRes;
				sprintf_s( szText, sizeof(szText),  "\t : : Code 0x%04x   Meas 0x%04x ( %.3fmV)\n", u32V1, i32V3, fVolt );
			}
			else
			{
				sprintf_s( szText, sizeof(szText),  "\t : : Code 0x%03x (%4d) Meas %c%5ld.%ld (%c%5ld.%ld)\n",
				u32V1, u32V1, i32V3>=0?' ':'-', labs(i32V3)/10, labs(i32V3)%10,
				i32V4>=0?' ':'-', labs(i32V4)/10, labs(i32V4)%10); 
			}
			break;
		case TRACE_FINE_OFFSET:
				if ( !u32V1 & !i32V3 & !i32V4 )
					sprintf_s( szText, sizeof(szText),  "\n Chan %d  Fine DC calibration\n", u16Channel );		
				else
					sprintf_s( szText, sizeof(szText),  "%d Ch.     Fine calibration DAC 0x%04x (%4d) residual offset %c%ld.%ld (Tar %c%ld.%ld)\n",
					u16Channel, u32V1, u32V1, i32V4>0?' ':'-', labs(i32V4)/10, labs(i32V4)%10, i32V3>0?' ':'-', labs(i32V3)/10, labs(i32V3)%10 );
			break;
		case TRACE_CAL_GAIN:
			sprintf_s( szText, sizeof(szText),  "\n%d Ch.     Gain calibration Pos DAC 0x%04x (%4d) Neg DAC 0x%04x (%4d)\n",
			u16Channel, u32V1, u32V1, u32V2, u32V2);
			break;
		case TRACE_CAL_POS:
			if ( (u32V1==0) || (u32V2==0) )
				sprintf_s( szText, sizeof(szText),  "\n%d Ch. Position DC calibration \n",	u16Channel);
			if ( u32V1 && !u32V2 )
				sprintf_s( szText, sizeof(szText),  "\n%d Ch. Position DC calibration  Pos DAC 0x%04x (%4d) Delta %d \n", u16Channel, u32V1, u32V1, i32V3);
			if ( !u32V1 && u32V2 )
				sprintf_s( szText, sizeof(szText),  "\n%d Ch. Position DC calibration  Neg DAC 0x%04x (%4d) Delta %d \n", u16Channel, u32V2, u32V2, i32V3);
			if ( u32V1 && u32V2 )
				sprintf_s( szText, sizeof(szText),  "\n%d Ch. Position calibration Pos DAC 0x%04x (%4d) Neg DAC 0x%04x (%4d)\n",
							u16Channel, u32V1, u32V1, u32V2, u32V2);
							
			break;
		default:
			break;
		
	}
	OutputDebugString(szText);

}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsHexagon::UpdateCalibDac(uInt16 u16Channel, eCalDacId ecdId, uInt16 u16Data, bool bUpdated)
{
	int32 i32Ret = CS_SUCCESS;
	uInt16 u16PositionDac = 0;
	uInt16 *pu16Val = NULL;

	if( (0 == u16Channel) || (u16Channel > HEXAGON_CHANNELS)  )
		return CS_INVALID_CHANNEL;

	if( ecdId > ecdGain  )
		return CS_INVALID_PARAMETER;

	uInt16 u16ChanZeroBased = u16Channel-1;
	
	uInt32 u32RangeIndex, u32FeModIndex;
	i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	u16PositionDac  = GetControlDacId( ecdId, u16Channel );
	i32Ret = WriteToCalibDac( u16PositionDac, u16Data);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

#ifdef DBG_CALIB_DAC
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("UpdateCalibDac(%d):  ecdId %ld u16Data %d u16PositionDac %d \n"), 
			u16Channel, ecdId, u16Data, u16PositionDac);
	OutputDebugString(szDbgText);
#endif

	// save current setting ?
	if (bUpdated)
	{
		switch( ecdId )
		{
			case ecdPosition:
				pu16Val = (uInt16*)&m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset;
				break;

			case ecdOffset:
				pu16Val = (uInt16*)&m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16FineOffset;
				break;
				
			case ecdGain:
				pu16Val = (uInt16*)&m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16Gain;
				break;
		}

		*pu16Val = u16Data;
		TraceCalib( u16Channel, TRACE_CAL_DAC, u16Data, ecdId);
	}
				
	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsHexagon::RestoreCalibModeSetting()
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32RegVal = 0;
	uInt32	u32CmdVal = HEXAGON_ADDON_CPLD_CALSEL_CHAN;

	i32Ret = WriteRegisterThruNios( u32RegVal, u32CmdVal, -1, &u32RegVal );
	if ( CS_SUCCEEDED( i32Ret ) )
	{
		i32Ret = WriteRegisterThruNios( u32RegVal, u32CmdVal | HEXAGON_SPI_WRITE );
		BBtiming( 200 );
	}

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsHexagon::SendCalibModeSetting( eCalMode eMode )
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32RegVal = 0;

	if ( ecmCalModeDc == eMode )
	{
		// Switch to the Calibmode
		u32RegVal = 0;
	}
	else
		u32RegVal = 0xF;

	i32Ret = WriteRegisterThruNios( u32RegVal, HEXAGON_ADDON_CPLD_CALSEL_CHAN | HEXAGON_SPI_WRITE );
	BBtiming( 300 );

	return i32Ret;
}

//------------------------------------------------------------------------------
//
// Gain control table use 32 value 0-20h 
// if u16Gain = 0 no gain setting.
//    u16Vref = 0 no source ref setting
//------------------------------------------------------------------------------
int32 CsHexagon::DumpAverageTable(uInt16 u16Channel, eCalDacId ecdId, uInt16 u16Vref, uInt16 u16Gain, bool bForceMode, uInt16 NbSteps, bool bZoomMode, uInt16 u16ZommAddr, uInt16 u16ZoomFactor)
{
	int32	i32Ret = CS_SUCCESS;
	int32	i32Avg_x10, i32AvgRef_x10;	
	uInt16	u16maxStep = 16;	

	uInt16 	u16CurrentPos = 0;
	uInt16 	u16Step = 0;
	int32	i32Avg_x10Table[HEXAGON_DB_MAX_TABLE_SIZE_DMP]={0};	
	int32	i32AvgRef_x10Table[HEXAGON_DB_MAX_TABLE_SIZE_DMP]={0};	
	HEXAGON_CAL_LUT CalInfo;
	bool	bCalibBufAlloc = false;
	bool    bCalibRefBufAlloc = false;
	uInt16  u16Data = u16Gain;
	uInt16	u16MidAddr = 0;
	uInt16  u16StartCode = 0;
	uInt32	u32FeModIndex = 0;
	uInt32	u32RangeIndex = 0;
	uInt16	u16OldCode =0;

	sprintf_s( szDbgText, sizeof(szDbgText),  "DumpAverageTable Ch %d. (ecdId=%d) VRef %d Gain code %d bForceMode %d NbSteps %d bZoomMode %d ZommAddr 0x%x \n",
				u16Channel, ecdId, u16Vref, u16Gain, bForceMode, NbSteps, bZoomMode, u16ZommAddr);
	OutputDebugString(szDbgText);	

	if ((NbSteps>0) & (NbSteps<=HEXAGON_DB_MAX_TABLE_SIZE_DMP))
		u16maxStep = NbSteps;

	u16Step = (c_u32CalDacCount) /u16maxStep;					// 12bits DAC

	i32Ret = FindCalLutItem(0, 50, CalInfo);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	if ( (ecdId!=ecdPosition) && (ecdId!=ecdOffset) && (ecdId!=ecdGain) )
		return i32Ret;

	FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );

	// Make sure to switch to mode DC
	if (bForceMode)
		SendCalibModeSetting( ecmCalModeDc );

	// prepare resource required
	if( NULL == m_pi16CalibBuffer)
	{
		m_pi16CalibBuffer = (int16 *)::GageAllocateMemoryX( HEXAGON_CALIB_BUFFER_SIZE*sizeof(int16) );
		if( NULL == m_pi16CalibBuffer)
			return CS_INSUFFICIENT_RESOURCES;

		bCalibBufAlloc = true;
	}

	if( NULL == m_pi32CalibA2DBuffer)
	{
		m_pi32CalibA2DBuffer = (int32 *)::GageAllocateMemoryX( HEXAGON_CALIB_BLOCK_SIZE*sizeof(int16) );
		if( NULL == m_pi32CalibA2DBuffer) 
			return CS_INSUFFICIENT_RESOURCES;

		bCalibRefBufAlloc = true;
	}

	if ( u16Vref==HEXAGON_CAL_SRC_NEG || u16Vref==HEXAGON_CAL_SRC_0V || u16Vref==HEXAGON_CAL_SRC_POS) 
		i32Ret = WriteRegisterThruNios( u16Vref, HEXAGON_ADDON_CPLD_CALSRC_REF | HEXAGON_SPI_WRITE );

	if (u16Gain > 32)		// gain control max value
	{
		if (IsBoardLowRange())
		u16Data = HEXAGON_DC_GAIN_CODE_1V;
		else
			u16Data = HEXAGON_DC_GAIN_CODE_LR;
	}

	if (u16Gain != HEXAGON_NO_GAIN_SETTING)
		i32Ret = UpdateCalibDac(u16Channel, ecdId, u16Data, false );

	switch(ecdId)
	{
		case ecdPosition:
		case ecdOffset:
			break;
		case ecdGain:
			u16Step = 1;
			if (m_bLowRange_mV == false)
				u16StartCode = u16CurrentPos = HEXAGON_GAIN_CODE_MIN_ALLOWED;	// be careful, max gain allowed. 
			else
				u16StartCode = u16CurrentPos = 0;	// be careful, max gain allowed. 
			u16maxStep = 0x20 - u16CurrentPos;
			break;
		default:
			break;
	}

	// show x value up and x down from center address (increment by 8)
	if ( (bZoomMode) && (ecdId!=ecdGain) )
	{
		uInt16 u16DivFactor = ( 2 <<  u16ZoomFactor);
		u16Step = 4096 / (u16maxStep * u16DivFactor);
		if (u16Step <1)
			u16Step =1;
		
		// offset enter is the center address
		if (u16ZommAddr)
			u16MidAddr = u16ZommAddr;
		else
			u16MidAddr = 0x800;

		if ( 0x800 >u16Step*(u16maxStep/2))
			u16StartCode = u16CurrentPos = u16MidAddr - u16Step*(u16maxStep/2);
		else
			u16StartCode = u16CurrentPos = 0;
	}

	if ( (ecdId==ecdPosition) || (ecdId==ecdOffset) || (ecdId==ecdGain))
	{
		for (int i=0; i<u16maxStep; i++)
		{
			i32Ret = UpdateCalibDac(u16Channel, ecdId, u16CurrentPos, false );
			if( CS_FAILED(i32Ret) )
				return i32Ret;

			i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
			if( CS_FAILED(i32Ret) )
				break;
			
			ReadCalibADCRef(&i32AvgRef_x10);

			i32Avg_x10Table[i]=i32Avg_x10;
			i32AvgRef_x10Table[i]=i32AvgRef_x10;
			u16CurrentPos = u16CurrentPos + u16Step;
		}
	}

	if (bCalibBufAlloc && m_pi16CalibBuffer)
	{
			::GageFreeMemoryX(m_pi16CalibBuffer);
			m_pi16CalibBuffer = NULL;
	}

	if (bCalibRefBufAlloc && m_pi32CalibA2DBuffer)
	{
		::GageFreeMemoryX(m_pi32CalibA2DBuffer);
		m_pi32CalibA2DBuffer = NULL;
	}

	// Restore the old setting
	i32Ret = WriteRegisterThruNios( HEXAGON_CAL_SRC_0V, HEXAGON_ADDON_CPLD_CALSRC_REF | HEXAGON_SPI_WRITE );

	switch(ecdId)
	{
		case ecdPosition:
			if (m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16CoarseOffset)
				u16OldCode = m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16CoarseOffset;
			else
				u16OldCode = HEXAGON_DC_OFFSET_DEFAULT_VALUE;

			i32Ret = UpdateCalibDac(u16Channel, ecdPosition, u16OldCode, false );
			break;
		case ecdOffset:
			if (m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16FineOffset  )
				u16OldCode = m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16FineOffset;
			else
				u16OldCode = HEXAGON_DC_OFFSET_DEFAULT_VALUE;
			i32Ret = UpdateCalibDac(u16Channel, ecdOffset, u16OldCode, false );
			break;
		case ecdGain:
			if (m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16Gain  )
				u16OldCode = m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16Gain;
			else
				u16OldCode = HEXAGON_DC_GAIN_CODE_1V;
			i32Ret = UpdateCalibDac(u16Channel, ecdGain, u16OldCode, false );
			break;
		default:
			break;
	}

	if (bForceMode)
		SendCalibModeSetting();



	char szText[256]={0}, szText2[256]={0};
	double	fVoltTmp = 0.0;

      sprintf_s( szText, sizeof(szText),  "==================================================== \n");
     DumpOutputMsg(szText);	

   if (ecdId == ecdGain)
   {
      sprintf_s( szText, sizeof(szText),  "[ch=%d] Gain Table. VRef=%d GainParams=%d PrevGain=0x%x \n",
               u16Channel, u16Vref, u16Gain, u16OldCode);
      DumpOutputMsg(szText);	
      sprintf_s( szText, sizeof(szText),  "==================================================== \n");
      DumpOutputMsg(szText);	
      sprintf_s( szText, sizeof(szText),  "code	 Avg(dec,hex)          Volt(mv)     AvgRef(dec,hex)         VRef(mv) \n");
      DumpOutputMsg(szText);	
      sprintf_s( szText, sizeof(szText),  "--------------------------------------------------------------------------\n");
      DumpOutputMsg(szText);	
   }
   else 
   {
	  if (ecdId == ecdPosition)
	  {
		  sprintf_s( szText, sizeof(szText),  "[ch=%d] Coarse Table. VRef=%d GainCode=%d Coarse0V=0x%x \n",
				   u16Channel, u16Vref, u16Gain, u16OldCode);
		  DumpOutputMsg(szText);	
	  }
	  else
	  {
		  sprintf_s( szText, sizeof(szText),  "[ch=%d] Fine Table. VRef=%d Gaincode= %d Fine0V=0x%x \n",
				   u16Channel, u16Vref, u16Gain, u16OldCode);
		  DumpOutputMsg(szText);	
	  }
      sprintf_s( szText, sizeof(szText),  "==================================================== \n");
      DumpOutputMsg(szText);	
      sprintf_s( szText, sizeof(szText),  "code	 Avg(dec,hex)         Volt(mv)          Delta(mv) \n");
      DumpOutputMsg(szText);	
      sprintf_s( szText, sizeof(szText),  "------------------------------------------------------- \n");
      DumpOutputMsg(szText);	
   }
      
	for(uInt16 i=0; i<u16maxStep; i++)
	{
		double	fVoltRef = 0.0;
		double  fScale = m_bLowRange_mV ? (HEXAGON_GAIN_LR_MV/2) : (CS_GAIN_2_V/2);
		double	fVolt =  (-1.0*i32Avg_x10Table[i]/m_pCardState->AcqConfig.i32SampleRes/10)*fScale;
		int16 i16Avg = (int16)(i32Avg_x10Table[i]/10 & 0xFFFF);
		u16CurrentPos = u16StartCode + u16Step*i;

      if (ecdId == ecdGain)
      {
         if (i32AvgRef_x10Table[i] <0)
            fVoltRef =  (-1.0)*double(Abs(i32AvgRef_x10Table[i]/10)-0x8000);
         else
            fVoltRef =  double((i32AvgRef_x10Table[i]/10)-0x8000);
         fVoltRef = fVoltRef*12*fScale/m_pCardState->AcqConfig.i32SampleRes;
         int16 i16AvgRef = (int16)(i32AvgRef_x10Table[i]/10 & 0xFFFF);
         sprintf_s( szText2, sizeof(szText2),  "[%03x]	[%05d, 0x%08x]  %lf mv [%05d, 0x%08x] %lf mv \n",
                  u16CurrentPos, 
                  i16Avg,
                  i16Avg, 
                  fVolt, 
                  i16AvgRef, 
                  i16AvgRef, 
                  fVoltRef); 
         DumpOutputMsg(szText2);	
      }
      else
      {
         sprintf_s( szText2, sizeof(szText2),  "[%03x]	[%05d, 0x%08x]  %lf mv  %lf mv \n",
                  u16CurrentPos, 
                  i16Avg,
                  i16Avg, 
                  fVolt, 
                  fVolt - fVoltTmp); 
         DumpOutputMsg(szText2);	         
      }
      
      fVoltTmp = fVolt;
	}



	return i32Ret;

}

int32 CsHexagon::FindCalibOffset(uInt16 u16HwChannel, eCalDacId ecdId, bool bIsGoUp, uInt16 *pBestOffset, int32 *pi32Avg_Meas)
{
	const	int32 i32Tolerance	= 32768/200;		// .5% of full scale
	int32	i32Avg_x10			= 0;
	int32	i32CurrentError		= 0x7FFF;
	int32	i32Ret				= CS_SUCCESS;

	uInt16	u16Pos				= 0;
	uInt16	u16CurrentPos		= HEXAGON_DC_OFFSET_DEFAULT_VALUE;
	
	bool	bLastErrorSignPos	= false;
	uInt16  LastBOffset			= 0;
	int32   LastBTarget			= i32Tolerance;
	bool	bGoUp = bIsGoUp;
	uInt16  u16TraceTag			= TRACE_COARSE_OFFSET;

	int32	i32TargetBinary		= 0;

#ifdef DBG_CALIB_OFFSET
		sprintf_s( szDbgText, sizeof(szDbgText),  "\t : : FindCalibOffset(%d) ecdId %d  bIsGoUp %d \n",
				u16HwChannel, ecdId, bIsGoUp); 
		OutputDebugString(szDbgText);
#endif

	// Find Offset (Coarse/Fine) only.
	if ( (ecdId!=ecdPosition) && (ecdId!=ecdOffset) )
		return(CS_INVALID_PARAMETER);

	if (ecdId==ecdOffset)
		u16TraceTag = TRACE_FINE_OFFSET;


	if (pBestOffset)
		*pBestOffset = 0;

	if (pi32Avg_Meas)
		*pi32Avg_Meas = 0;

	if ( ! bGoUp )
		u16CurrentPos >>= 1;

	// Loop until we get the DC offset reach the target level and within the error tolerance
	while( u16CurrentPos > 0 )
	{
		// Adjust the DC level and capture 
		i32Ret = UpdateCalibDac(u16HwChannel, ecdId, u16CurrentPos | u16Pos );
		if( CS_FAILED(i32Ret) )
	        goto EndFindCalibOffset;

		BBtiming( 25 );

		i32Ret = AcquireAndAverage(u16HwChannel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )
	        goto EndFindCalibOffset;

		i32Avg_x10 /= 10;
		TraceCalib( u16HwChannel, TRACE_CAL_ITER, u16Pos|u16CurrentPos, u16TraceTag, i32Avg_x10);

		i32CurrentError = i32Avg_x10 - i32TargetBinary;

		// Check if the DC offset meets the requirement
		if ( Abs(i32CurrentError) < i32Tolerance )
		{
			//u16Pos |= u16CurrentPos;
			//break;						// succeeded. quit
			if ( Abs(i32CurrentError) < Abs(LastBTarget) )
			{
				LastBTarget = i32CurrentError;
				LastBOffset = u16Pos | u16CurrentPos;
			}				
		}
		else if ( bLastErrorSignPos != (i32CurrentError > 0) )
		{
			bLastErrorSignPos = (i32CurrentError > 0);
			bGoUp = ! bGoUp;
		}
		if ( bGoUp )
		{
			u16Pos |= u16CurrentPos;
		}
		else
		{
			// continue to go down
		}
		u16CurrentPos >>= 1;
	}

	// Set the final value
	if ( Abs(LastBTarget) < Abs(i32Tolerance) )
	{
		if (pBestOffset)
			*pBestOffset = LastBOffset;
		if (pi32Avg_Meas)
			*pi32Avg_Meas = i32Avg_x10;
	}
EndFindCalibOffset:
#ifdef  DBG_CALIB_OFFSET
	sprintf_s( szDbgText, sizeof(szDbgText),  "\t : : FindCalibOffset(%d) ret %d  bGoUp %d LastBOffset 0x%x LastBTarget %ld \n",
				u16HwChannel, i32Ret, bIsGoUp, *pBestOffset, LastBTarget); 
	OutputDebugString(szDbgText);
#endif
	 
    return(i32Ret);
}

// Read reference value from calibration AFC 
int32 CsHexagon::ReadCalibADCRef(int32 *pAvg_x10)
{
	int32 i32Ret = CS_SUCCESS;
	int16 i16tmp = 0;
	int32 i32Count = HEXAGON_CALIB_BLOCK_SIZE - HEXAGON_DB_CALIB_BUF_MARGIN;
	int32 i32Sum = 0;
	int16 i = 0;
	int16 *pi16Buffer = (int16*)m_pi32CalibA2DBuffer;
	bool  bBufAlloc = false;
	const int32 c32SampleOffset = 0x8000;				// ADC reference. 0x8000 refence as Zero. 

	if( !pi16Buffer)
	{
		pi16Buffer = (int16 *)::GageAllocateMemoryX( HEXAGON_CALIB_BLOCK_SIZE*sizeof(int16) );
		if( NULL == pi16Buffer) return CS_INSUFFICIENT_RESOURCES;
		bBufAlloc = true;
	}

	for (i = 0;  i < HEXAGON_CALIB_BLOCK_SIZE; i++)
		i16tmp = pi16Buffer[i] = (int16)(ReadRegisterFromNios( 0, HEXAGON_CAL_ADC_SPI ) & 0xFFFF);

	for (i = HEXAGON_DB_CALIB_BUF_MARGIN; i <i32Count; i++)
		i32Sum = i32Sum + pi16Buffer[i] - c32SampleOffset;

	if ( pAvg_x10 )
		*pAvg_x10 = ( i32Sum*10 / i32Count );

	if( pi16Buffer && bBufAlloc )
	{
		::GageFreeMemoryX( pi16Buffer );
		pi16Buffer = NULL;
	}

//#ifdef DBG_CALIB_INFO
//	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("ReadCalibADCRef: i32sum %d i32Count %d *pAvg_x10 %ld i16tmp 0x%x \n"), 
//		i32Sum, i32Count, *pAvg_x10, i16tmp);
//	OutputDebugString(szDbgText);
//#endif

	return i32Ret;
}
