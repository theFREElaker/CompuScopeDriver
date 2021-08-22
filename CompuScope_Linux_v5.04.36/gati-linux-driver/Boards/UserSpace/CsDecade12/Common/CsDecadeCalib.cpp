//
//	This file contains all member functions that access to addon hardware including
//	Frontend set up and calibration
//
//

#include "StdAfx.h"
#include "CsTypes.h"
#include "GageWrapper.h"
#include "CsDecade.h"
#include "CsDecadeCal.h"
#include "CsEventLog.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#else
#include "GageCfgParser.h"
#endif
		
#define	 CSRB_CALIB_TIMEOUT_SHORT							KTIME_MILISEC(300)		// 300 milisecond timeout
#define	 DECADE_CALIB_TIMEOUT_LONG	KTIME_SECOND(1)			// 1 second timeout
#define	 DECADE_CALIB_TIMEOUT_SHORT	KTIME_MILISEC(300)		// 300 milisecond timeout

#define	 DECADE_CALIB_TIMEOUT	KTIME_MILISEC(300)			// 300 milisecond timeout

#define	 NB_CALIB_GAIN_LOOP		100							// should not wait for too long, normally it should take 6-10 loop

//#define DBG_CALIB_OFFSET
//#define DBG_CALIB_GAIN

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsDecade::CalibrateGain( uInt16 u16Channel )
{
	int32 i32Gain = 0;
	double fVolt =  0;
	double	fTolerance = 0.004;		// .4% of the full scale
	uInt16 u16ChanZeroBased = u16Channel-1;
	uInt32 u32InpuRange = m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange;
	int32 i32Ret = CS_SUCCESS;

	DECADE_CAL_LUT CalInfo;
	uInt32 u32CalRange = u32InpuRange;
	double	fRangeFactor = 1.0*u32InpuRange/2000;	// factor compare to +/-1V Range 
	uInt16 u16Chan2Protect = u16Channel;
	uInt32 u32FrontEndRegVal = 0;
	uInt32 u32FrontEndCmd = 0;

	if( m_PlxData->CsBoard.NbAnalogChannel > 1)
		u16Chan2Protect = (u16Channel == 1) ? 2 : 1;

	// Find the setup for the current input ranges
	i32Ret = FindCalLutItem(u32CalRange, 50, CalInfo);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	//DumpAverageTable(u16Channel, ecdGain, CalInfo.u16PosRegMask, 0, 32);

	// fVTarget is the theorical gain we are trying to match (in V)
	double	fVTarget = 1.0*(CalInfo.i32PosLevel_uV-CalInfo.i32NegLevel_uV)/1000000;
	// Normalize for +/- 1V range
	fVTarget /= fRangeFactor;


	fTolerance = (fVTarget * .4)/100;      // .1% of error

	double	fError = 0.0;
	uInt32	u32Iteration = 0;
	bool	bBigger = false;
	uInt16  u16Data(0);
	uInt16	u16GainCode = 0x4000;		// Middle of 0 and 0x8000
	uInt16	u16StepCode = 0x2000;
	int32	i32SetLevel(0);

	TraceCalib( u16Channel, TRACE_CAL_SIGNAL, 50, u32InpuRange, CalInfo.i32PosLevel_uV, CalInfo.i32PosLevel_uV);

	// The new add-on has over-voltage protection. It may occur if one channel is in lower range (100mv, 200mv, 500mv)
	// and other channel is in higher range (1V, 2V, 5V). To overcome the issue in calibrate mode, we should check 
	// and manipulate the bit Att 20 db.
	if (  ( u16Chan2Protect != u16Channel ) &&( m_PlxData->CsBoard.u32AddonBoardVersion >= ADDON_HW_VERSION_1_07) )
	{
		uInt32 u32FECh1Reg = ReadRegisterFromNios( 0, DECADE_ADDON_CPLD_FRONTEND_CH1 );
		uInt32 u32FECh2Reg = ReadRegisterFromNios( 0, DECADE_ADDON_CPLD_FRONTEND_CH2 );
		uInt32 u32RegTmp   = 0;

#ifdef _DEBUG
		sprintf_s( szDbgText, sizeof(szDbgText),  "\tCalibrateGain u32FECh1Reg 0x%x u32FECh2Reg 0x%x m_bFirmwareChannelRemap %d \n", 
					u32FECh1Reg, u32FECh2Reg, m_bFirmwareChannelRemap); 
		OutputDebugString(szDbgText);
#endif
		// nothing to do if same range (higher or lower)
		if ( ( u32FECh1Reg & DECADE_FE_ATT20DB_RLY_BIT) != (u32FECh2Reg & DECADE_FE_ATT20DB_RLY_BIT) )
		{
			// try to have same bit (Att 20Db) setting for both channnels. 
			// use calibrate channel as reference. After calibrate, restore this bit...
			if (u16Chan2Protect == 2) 
			{
				u32FrontEndCmd = m_bFirmwareChannelRemap ? DECADE_ADDON_CPLD_FRONTEND_CH1 : DECADE_ADDON_CPLD_FRONTEND_CH2;
				u32RegTmp = m_bFirmwareChannelRemap ? u32FECh2Reg : u32FECh1Reg; 
				u32FrontEndRegVal = m_bFirmwareChannelRemap ? u32FECh1Reg: u32FECh2Reg;
			}
			else
			{
				u32FrontEndCmd = m_bFirmwareChannelRemap? DECADE_ADDON_CPLD_FRONTEND_CH2 : DECADE_ADDON_CPLD_FRONTEND_CH1;
				u32RegTmp = m_bFirmwareChannelRemap ? u32FECh1Reg :  u32FECh2Reg;
				u32FrontEndRegVal = m_bFirmwareChannelRemap ? u32FECh2Reg : u32FECh1Reg;
			}

			u32RegTmp &= DECADE_FE_ATT20DB_RLY_BIT;			

			// FrontEnd Att20Db bit set ?
			if ( u32RegTmp )
				WriteRegisterThruNios( u32FrontEndRegVal | DECADE_FE_ATT20DB_RLY_BIT , u32FrontEndCmd | DECADE_SPI_WRITE );
			else
				WriteRegisterThruNios( u32FrontEndRegVal & DECADE_FE_ATT20DB_RLY_BIT , u32FrontEndCmd | DECADE_SPI_WRITE );
			BBtiming( 75 );
		}
	}

	// Once we calibrate gain, set a flag so that we remember to calibrate later for better ENOB
	m_pCardState->bEnobCalibRequired = TRUE;

	do 
	{
		// Update new gain adjustment for the ADC
		i32Ret = AdjustAdcGain(u16Channel, u16GainCode);
		BBtiming( 75 );

		// Setup and capture the +Vref
		u16Data = CalInfo.u16PosRegMask;
		i32SetLevel = CalInfo.i32PosLevel_uV;
		
		u16Data |= 0x40;
		i32Ret = UpdateDcCalibReg(u16Data);
		m_pCardState->AddOnReg.u16VSC = u16Data;
		BBtiming( 200 );

			// Measure and average the voltage +Vref
		int32	i32Avg_x10_A = 0;
		int32	i32Avg_x10_B = 0;

		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10_A);
		if( CS_FAILED(i32Ret) )
            goto EndCalibrateGain;

		// Setup and capture the -Vref
		u16Data = CalInfo.u16NegRegMask;
		i32SetLevel = CalInfo.i32NegLevel_uV;
		
		u16Data |= 0x40;
		i32Ret = UpdateDcCalibReg(u16Data);
		m_pCardState->AddOnReg.u16VSC = u16Data;
		BBtiming( 200 );

			// Measure and average the voltage -Vref
		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10_B);
		if( CS_FAILED(i32Ret) )
            goto EndCalibrateGain;

		// Calculate the actual gain
		i32Gain = i32Avg_x10_A - i32Avg_x10_B;
		fVolt =  -1.0*i32Gain/m_pCardState->AcqConfig.i32SampleRes/10;

//		TraceDebugCalib( u16Channel, TRACE_GAIN_ADJ, u32GainCode, 0, i3Gain );

#ifdef  DBG_CALIB_GAIN
		{
		char	szText[256]={0};
		sprintf_s( szText, sizeof(szText),  "\t : : GainCode 0x%04x Vref+[ %ld ] Vref-[ %ld ]\n",
				u16GainCode, i32Avg_x10_A, i32Avg_x10_B); 
		OutputDebugString(szText);
		}
#endif

		// Check for error tolerance
		fError =  Abs(fVolt-fVTarget);
		if (( fError <= fTolerance ) && ( Abs(fVolt) < fVTarget))
			break;						// succeeded quit

		if ( Abs(fVolt) > fVTarget )
		{
			if ( u32Iteration > 0 && !bBigger )
				u16StepCode /= 2;

			u16GainCode +=  u16StepCode;
			bBigger = true;
		}
		else
		{
			if ( u32Iteration > 0 && bBigger )
				u16StepCode /= 2;

			u16GainCode -=  u16StepCode;
			bBigger = false;
		}

		TraceCalib( u16Channel, TRACE_CAL_ITER, u16GainCode, TRACE_CAL_GAIN, i32Gain, CalInfo.i32PosLevel_uV-CalInfo.i32NegLevel_uV);
		u32Iteration++;

		if (u32Iteration > NB_CALIB_GAIN_LOOP)
			break;

	} while (u16GainCode < 0x8000 );

	TraceCalib(u16Channel, TRACE_CAL_GAIN, u16GainCode, i32Gain);
	i32Ret = UpdateCalibDac(u16Channel, ecdGain, u16GainCode, true );
	if( CS_FAILED(i32Ret) )
		goto EndCalibrateGain;


EndCalibrateGain:   
	i32Ret = FindCalLutItem(0, 50, CalInfo);
	if( CS_SUCCEEDED(i32Ret) )
	{
		// Restore 0V Vref
		u16Data = 0x40 | CalInfo.u16PosRegMask;
		i32Ret = UpdateDcCalibReg(u16Data);
		m_pCardState->AddOnReg.u16VSC = u16Data;
		BBtiming( 200 );
	}

	if ( ( u16GainCode >= 0x8000 ) || (u32Iteration >= NB_CALIB_GAIN_LOOP) )
	{
		char szErrorStr[128];
		CsEventLog EventLog;
		i32Ret = UpdateCalibDac(u16Channel, ecdGain, (uInt16) (c_u32CalDacCount/4), true );
		i32Ret = CS_GAIN_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
		sprintf_s( szErrorStr, sizeof(szErrorStr), "%d, Channel %d DAC code %d", 
					CS_GAIN_CAL_FAILED, ConvertToUserChannel(u16Channel), u16GainCode );
		EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );
	}

	// Att20Db bit changed ?
	if ( ( u16Chan2Protect != u16Channel ) && u32FrontEndRegVal && u32FrontEndCmd )
	{
		// Restore Att20Db bits for other channel.
		WriteRegisterThruNios( u32FrontEndRegVal , u32FrontEndCmd | DECADE_SPI_WRITE );
	}

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

BOOLEAN	CsDecade::IsCalibrationRequired()
{
	BOOLEAN bCalNeeded = FALSE;

	for( uInt16 i = m_ChannelStartIndex; i <= m_PlxData->CsBoard.NbAnalogChannel; i += GetUserChannelSkip() )
	{
		uInt16 u16ChanZeroBased = ConvertToHwChannel(i) - 1;
		if( m_pCardState->bCalNeeded[u16ChanZeroBased] )
		{
			bCalNeeded = TRUE;
			break;
		}
	}

	return ( bCalNeeded && !m_bNeverCalibrateDc );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDecade::CsGetAverage(uInt16 u16Channel, int16* pi16Buffer, uInt32 u32BufferSizeSamples, int32 i32StartPosistion, int32* pi32OddAvg_x10, int32* pi32EvenAvg_x10)
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
	InXferEx.bBusMasterDma	= m_BusMasterSupport;
	InXferEx.u32SampleSize  = sizeof(uInt16);

	IoctlPreDataTransfer( &InXferEx, &OutXferEx );
	CardReadMemory( pi16Buffer, u32BufferSizeSamples * sizeof(uInt16) );

	if ( 0 != pi32OddAvg_x10 || 0 != pi32EvenAvg_x10 )
	{
		int32 i32SumOdd = 0;
		int32 i32SumEven = 0;
		int32 i32Count = 0;
		const int32 c32SampleOffset = m_pCardState->AcqConfig.i32SampleOffset;
		
		for (uInt32 u32Addr = DECADE_CALIB_BUF_MARGIN;  u32Addr < u32BufferSizeSamples; i32Count++)
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
int32	CsDecade::CaptureCalibSignal( bool bWaitTrigger /* = false */)
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

	CsTimeout.Set( DECADE_CALIB_TIMEOUT_LONG );
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
int32	CsDecade::AcquireAndAverage(uInt16	u16Channel, int32* pi32AvrageOdd_x10, int32* pi32AvrageEven_x10)
{
	int32 i32Ret = CS_SUCCESS;
	
	i32Ret = CaptureCalibSignal();
	if( CS_FAILED(i32Ret) )	return i32Ret;

	//Calculate logical channel
	uInt16 u16UserChannel = ConvertToUserChannel(u16Channel);
	i32Ret = CsGetAverage( u16UserChannel, m_pi16CalibBuffer, DECADE_CALIB_DEPTH, 0, pi32AvrageOdd_x10, pi32AvrageEven_x10);
	return  i32Ret;
}

//-----------------------------------------------------------------------------
//----- ReadCalibA2D
//		returns in uV
//-----------------------------------------------------------------------------

int32 CsDecade::ReadnCheckCalibA2D(int32 i32Expected_mV, int32* pi32V_uv)
{
	int32 i32Ret = CS_SUCCESS;

	UNREFERENCED_PARAMETER(i32Expected_mV);

	if( NULL == m_pi32CalibA2DBuffer)
	{
		m_pi32CalibA2DBuffer = (int32 *)::GageAllocateMemoryX( DECADE_CALIB_BLOCK_SIZE*sizeof(int32) );
		if( NULL == m_pi32CalibA2DBuffer) return CS_INSUFFICIENT_RESOURCES;
	}

	
	i32Ret = UpdateDcCalibReg(m_pCardState->AddOnReg.u16VSC);
	BBtiming(m_u32CalSrcSettleDelay);
	if( CS_FAILED(i32Ret) )
	{
		::GageFreeMemoryX( m_pi32CalibA2DBuffer );
		return i32Ret;
	}
	
	// Readback from the Calib ADC
	LONGLONG	llSum = 0;
	int			nValidSamples = 0;
	uInt32		u32Code(0);

	for( int i = 0; i < DECADE_CALIB_BLOCK_SIZE; i++ )
	{
		i32Ret = WriteRegisterThruNios(0, DECADE_CALIB_ADC112S_CTRL, -1, &u32Code);
		if( CS_FAILED(i32Ret) )
			continue;		// retry

		int16 i16Code = int16(u32Code & 0x1fff);
//		i16Code <<= 3; i16Code >>= 3; //sign extention
//		TraceCalib( 0, TRACE_CAL_SIGNAL_ITER, i, nValidSamples, i16Code);
		m_pi32CalibA2DBuffer[nValidSamples++] = i16Code;
		llSum += LONGLONG(i16Code);
	}

	if ( nValidSamples < 2 )
	{
		i32Ret = CS_CALIB_ADC_READ_FAILURE;
		goto Caliba2DExit;
	}

	*pi32V_uv = (int32) llSum/nValidSamples;

Caliba2DExit:
	if( NULL != m_pi32CalibA2DBuffer)
	{
		::GageFreeMemoryX( m_pi32CalibA2DBuffer );
		m_pi32CalibA2DBuffer = NULL;
	}

	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsDecade::CalibrateFrontEnd(uInt16 u16Channel, uInt32* pu32SuccessFlag)
{
	if( !m_pCardState->bCalNeeded[u16Channel-1] )
		return CS_SUCCESS;
	
	if ( pu32SuccessFlag )
		*pu32SuccessFlag = DECADE_CAL_RESTORE_ALL;

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
		if ( 0 != ( m_u32IgnoreCalibErrorMaskLocal  & (1 << (ConvertToUserChannel(u16Channel)-1)) ) )
		{
			i32Ret = CS_SUCCESS;
		}
	}
	else
	{
		m_pCardState->bCalNeeded[u16Channel-1] = false;
		i32Ret = SendPosition( u16Channel, m_pCardState->ChannelParams[u16Channel-1].i32Position );
	}

	if ( m_bSkipCalib )
		SetFastCalibration(u16Channel);

	TraceCalib( u16Channel, TRACE_CAL_RESULT);
	TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 21);

	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsDecade::SetupForCalibration( bool bSetup)
{
	int32 i32Ret = CS_SUCCESS;

	// in eXpert images there is no calibration capability
	if( 0 != m_pCardState->u8ImageId )
		return CS_SUCCESS;

	m_bCalibActive		= bSetup;

	CSACQUISITIONCONFIG		AcqCfg;
	if( bSetup )
	{
		m_OldAcqConfig.u32Size	= sizeof(CSACQUISITIONCONFIG);
		AcqCfg.u32Size			= sizeof(CSACQUISITIONCONFIG);
		CsGetAcquisitionConfig( &m_OldAcqConfig );
		CsGetAcquisitionConfig( &AcqCfg );

		SetDataPackMode(DataUnpack);

		// Always calibrate in non-pingpong mode
		if ( m_OldAcqConfig.u32ExtClk == 0 )
		{
			// Switch to high sample rate non-poingpong only if internal clock is used
			if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & DECADE_ADDON_HW_OPTION_1CHANNEL6G) )
			{
				// Highest sample rate non pingpong
				AcqCfg.i64SampleRate	 = m_pCardState->SrInfoTable[1].llSampleRate;
			}
			else
				AcqCfg.i64SampleRate	 = m_pCardState->SrInfoTable[0].llSampleRate;
		}
		else
		{
			// Non-pingpong external clock
			AcqCfg.u32ExtClkSampleSkip = CS_SAMPLESKIP_FACTOR;
		}

		AcqCfg.i64TriggerTimeout = 0;
		AcqCfg.i64TriggerDelay   = 0;
		AcqCfg.i64TriggerHoldoff = 0;
		AcqCfg.i64SegmentSize	 = DECADE_CALIB_BUFFER_SIZE;
		AcqCfg.i64Depth			 = DECADE_CALIB_DEPTH;
		AcqCfg.i64TriggerHoldoff = AcqCfg.i64SegmentSize - AcqCfg.i64Depth;
		AcqCfg.u32SegmentCount   = 1;
		AcqCfg.u32Mode			 &= ~(CS_MODE_USER1|CS_MODE_USER2);

		m_pi16CalibBuffer = (int16 *)::GageAllocateMemoryX( DECADE_CALIB_BUFFER_SIZE*sizeof(int16) );
		if( NULL == m_pi16CalibBuffer)
			return CS_INSUFFICIENT_RESOURCES;

		RtlCopyMemory( m_OldTrigParams, m_pCardState->TriggerParams, sizeof(m_pCardState->TriggerParams));
		RtlCopyMemory( m_OldChanParams, m_pCardState->ChannelParams, sizeof(m_pCardState->ChannelParams));

		// Disable all triggers
		SendTriggerEngineSetting( 0 );
		SendExtTriggerSetting();

		if ( m_DDC_enabled )
		{
			m_iOld_DDC_ModeSelect = m_iDDC_ModeSelect;
			DDCModeSelect(CS_MODE_NO_DDC);
		}
		CsResetDataFormat();
	}
	else
	{
		::GageFreeMemoryX(m_pi16CalibBuffer);
		m_pi16CalibBuffer = NULL;

		RtlCopyMemory( m_pCardState->TriggerParams, m_OldTrigParams, sizeof(m_pCardState->TriggerParams));

		AcqCfg = m_OldAcqConfig;

		if ( m_DDC_enabled )
			DDCModeSelect(m_iOld_DDC_ModeSelect);

		uInt8 u8IndexTrig = CSRB_TRIGENGINE_A1;
		i32Ret = SendTriggerEngineSetting( m_pCardState->TriggerParams[u8IndexTrig].i32Source,
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

		SetDataPackMode(m_ConfigDataPackMode);
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

int32 CsDecade::CalibrateAllChannels()
{
	if( ! IsCalibrationRequired() )
		return CS_SUCCESS;

	int32	i32Ret = CS_SUCCESS;
	int32	i32Res = CS_SUCCESS;
	bool	bForceCalib = false;
	uInt16	u16UserChan = 1;

	if ( m_bSkipCalib )
	{
		TraceCalib( 0, TRACE_CAL_PROGRESS, 30);

		for( u16UserChan = m_ChannelStartIndex; u16UserChan <= m_PlxData->CsBoard.NbAnalogChannel; u16UserChan += GetUserChannelSkip() )
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

	// Before calibrate the front end, wait a bit to make sure that all relays are in stable position
	// Sleep(m_u32SettleDelayBeforeCalib);

	TraceCalib( 0, TRACE_CAL_PROGRESS, 30);
	//--------------------------------------------------------------------
	i32Ret = SetupForCalibration( true );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

#ifdef _WINDOWS_     
	__try
#endif   
	{
		// Switch to the Calibmode DC
		i32Ret = SendCalibModeSetting( ecmCalModeDc );
		if( CS_FAILED(i32Ret) )
#ifdef _WINDOWS_            
			__leave;
#else            
            goto EndCalibrateAll;
#endif          

		for( u16UserChan = m_ChannelStartIndex; u16UserChan <= m_PlxData->CsBoard.NbAnalogChannel; u16UserChan += GetUserChannelSkip() )
		{
			uInt32 u32Flag = 0;
			i32Res = CalibrateFrontEnd( ConvertToHwChannel(u16UserChan), &u32Flag );
			if( CS_FAILED(i32Res) )
#ifdef _WINDOWS_            
				__leave;
#else            
            goto EndCalibrateAll;
#endif          
		}

		// 6G boards gain calib for pingpong acquisitions
		// Calibrate so that the second ADC has the same  gain as the first one
		if (m_bPingpongCapture && m_bCalibPingpong)
		{
			// From driver and HW point of view, the input from 6G card will be CS_CHAN_2
			// and the second ADC will be cantrolled through CS_CHAN_1
			uInt16	u16InputChannel = CS_CHAN_2;
	
			// For unknown reason (has to be confirmed with HW group) we have user offset on the input (Position !=0), the gain calibration on the
			// second ADC will fail. That is why we have to zero the user offset before calibrate the gain of second ADC
			int32	i32SavedPosition = m_pCardState->ChannelParams[u16InputChannel-1].i32Position;
			if ( 0 != i32SavedPosition )
				SendPosition(CS_CHAN_2, 0);

			// Setup capture data from the second ADC
			SendCaptureModeSetting(CS_MODE_SINGLE, CS_CHAN_1 );
			// Calibrate the Gain of the Second ADC to the same gain of the first ADC
			i32Res = CalibrateGain( CS_CHAN_1 );

			// Restore the user Offset
			if ( 0 != i32SavedPosition )
				SendPosition(CS_CHAN_2, i32SavedPosition);
			BBtiming( 150 );

			if( CS_FAILED(i32Res) )
#ifdef _WINDOWS_            
				__leave;
#else            
            goto EndCalibrateAll;
#endif          
			else
				m_bCalibPingpong = false;
		}
	}


#ifdef _WINDOWS_      
	__finally
#else
EndCalibrateAll:   
#endif   
	{
		i32Ret = SendCalibModeSetting();
	}

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
void CsDecade::TraceCalib(uInt16 u16Channel, uInt32 u32TraceFlag, uInt32 u32V1, uInt32 u32V2, int32 i32V3, int32 i32V4)
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
			float fVolt =  (-1.0*u32V2/m_pCardState->AcqConfig.i32SampleRes/10)*1000;
			
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
			if (!m_bLogFaluireOnly)
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
			if( 0 != (m_u32DebugCalibTrace & u32V2) )
			{
				if (u32V2==TRACE_COARSE_OFFSET)
				{
					float	fVolt = (float)( 1000*(i32V3-m_pCardState->AcqConfig.i32SampleOffset) );
					fVolt = (float)(fVolt/m_pCardState->AcqConfig.i32SampleRes);
					sprintf_s( szText, sizeof(szText),  "\t :o: Code 0x%04x   Meas 0x%04x ( %.3fmV)\n", u32V1, i32V3, fVolt );
				}
				else
				if (u32V2 ==TRACE_CAL_GAIN)
				{
					/* refer to CalibrateGain. compute in mv */
					float fVolt =  (float)(-1.0*i32V3/m_pCardState->AcqConfig.i32SampleRes/10)*1000;
					float fRangeFactor = (float)(1.0*m_pCardState->ChannelParams[u16Channel-1].u32InputRange/2000);	// factor compare to +/-1V Range 
					float fVTarget = (float)(1.0*(i32V4)/1000);
					// Normalize for +/- 1V range
					fVTarget /= fRangeFactor;

					sprintf_s( szText, sizeof(szText),  "\t :g: Code %4d [0x%4x] Meas %c%3.6fmv (%c%3.6fmv)\n",
					u32V1, u32V1, i32V3>=0?' ':'-', fVolt,
					i32V4>=0?' ':'-', fVTarget); 
				}
				else
				{	
					sprintf_s( szText, sizeof(szText),  "\t :p: Code %4d [0x%4x] Meas %c%5ld.%ld (%c%5ld.%ld)\n",
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



const uInt16 CsDecade::c_u16CalDacCount = 4096;        //4096;
const uInt16 CsDecade::c_u16CalAdcCount = 4096;		//4096;

const uInt32 CsDecade::c_u32CalDacCount = 65536;
const uInt32 CsDecade::c_u32CalAdcCount = 65536;


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsDecade::FastCalibrationSettings(uInt16 u16Channel, bool bForceSet )
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


void CsDecade::SetFastCalibration(uInt16 u16Channel)
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
bool CsDecade::ForceFastCalibrationSettingsAllChannels()
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

void CsDecade::ReadCalibrationSettings(void *Params)
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

	ul = m_bForceCalibExtTrig ? 1 : 0;
	::RegQueryValueEx( hKey, _T("ForceCalibExtTrig"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bForceCalibExtTrig = (ul != 0);

	ul = m_bNeverCalibrateGain ? 1: 0;
	::RegQueryValueEx( hKey, _T("NeverCalibrateGain"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bNeverCalibrateGain = 0 != ul;

	ul = m_u8CalDacSettleDelay;
	::RegQueryValueEx( hKey, _T("CalDacSettleDelay"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u8CalDacSettleDelay = (uInt8)ul;

	ul = m_u32IgnoreCalibErrorMask;
	::RegQueryValueEx( hKey, _T("IgnoreCalibError"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u32IgnoreCalibErrorMask = ul;

	ul = m_u32DebugCalibTrace;
	::RegQueryValueEx( hKey, _T("TraceCalib"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u32DebugCalibTrace = ul;

	ul = m_u32DDCTraceDebug;
	::RegQueryValueEx( hKey, _T("TraceDDC"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u32DDCTraceDebug = ul;

	ul = m_u32OCTTraceDebug;
	::RegQueryValueEx( hKey, _T("TraceOCT"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u32OCTTraceDebug = ul;

	ul = m_bLogFaluireOnly?1:0;
	::RegQueryValueEx( hKey, _T("LogFaluireOnly"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bLogFaluireOnly = ul!=0;

	ul = m_bBetterEnob?1:0;
	::RegQueryValueEx( hKey, _T("BetterEnob"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bBetterEnob = ul!=0;

	ul = m_bBetterCalib_IQ?1:0;
	::RegQueryValueEx( hKey, _T("BetterCalib_IQ"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bBetterCalib_IQ = ul!=0;

	ul = m_i32DcOfffsetFudgeFactor_mV;
	::RegQueryValueEx( hKey, _T("DcOffsetCalibAdj"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_i32DcOfffsetFudgeFactor_mV = ul;

	ul = m_bSkipLVDS_Cal;
	::RegQueryValueEx(hKey, _T("SkipCalibLVDS"), NULL, NULL, (LPBYTE)&ul, &DataSize);
	m_bSkipLVDS_Cal = (0 != ul);

	ul = m_u32PingPongShift;
	::RegQueryValueEx(hKey, _T("PingPongShift"), NULL, NULL, (LPBYTE)&ul, &DataSize);
	if ( ul != 0 )
		m_u32PingPongShift = ul;

}

#else

void CsDecade::ReadCalibrationSettings(char *szIniFile, void *Params)
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

	ul = m_u8CalDacSettleDelay;
   ul = GetCfgKeyInt(szSectionName, _T("CalDacSettleDelay"),  ul,  szIniFile);	
	m_u8CalDacSettleDelay = (uInt8)ul;

	ul = m_u32IgnoreCalibErrorMask;
   ul = GetCfgKeyInt(szSectionName, _T("IgnoreCalibError"),  ul,  szIniFile);	
	m_u32IgnoreCalibErrorMask = ul;

	ul = m_u32DebugCalibTrace;
   ul = GetCfgKeyInt(szSectionName, _T("TraceCalib"),  ul,  szIniFile);	
	m_u32DebugCalibTrace = ul;

	ul = m_u32DDCTraceDebug;
   ul = GetCfgKeyInt(szSectionName, _T("TraceDDC"),  ul,  szIniFile);	
	m_u32DDCTraceDebug = ul;

	ul = m_u32OCTTraceDebug;
   ul = GetCfgKeyInt(szSectionName, _T("TraceOCT"),  ul,  szIniFile);	
	m_u32OCTTraceDebug = ul;

	ul = m_bLogFaluireOnly?1:0;
   ul = GetCfgKeyInt(szSectionName, _T("LogFaluireOnly"),  ul,  szIniFile);	
	m_bLogFaluireOnly = ul!=0;

	ul = m_bBetterEnob?1:0;
   ul = GetCfgKeyInt(szSectionName, _T("BetterEnob"),  ul,  szIniFile);	
	m_bBetterEnob = ul!=0;

	ul = m_bForceCalibExtTrig?1:0;
   ul = GetCfgKeyInt(szSectionName, _T("ForceCalibExtTrig"),  ul,  szIniFile);	
	m_bForceCalibExtTrig = ul!=0;
   
	ul = m_u32PingPongShift;
   ul = GetCfgKeyInt(szSectionName, _T("PingPongShift"),  ul,  szIniFile);	
	m_u32PingPongShift = ul!=0;

}   
   
#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDecade::WriteToAdcRegister(uInt16 u16Channel, uInt16 u16Reg, uInt16 u16Data)
{
	uInt32 u32Cmd = SPI_WRTIE_CMD;

	if ( 1 == u16Channel )
		u32Cmd |= DECADE_ADC2;	
	else
		u32Cmd |= DECADE_ADC1;	

	u32Cmd |= u16Reg << 8;
		
	int32 i32Ret = WriteRegisterThruNios( u16Data, u32Cmd );
	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsDecade::FindFeCalIndices( uInt16 u16Channel, uInt32& u32RangeIndex,  uInt32& u32FeModIndex )
{
	uInt16 u16ChanZeroBased = u16Channel-1;
	
	int32 i32Ret = FindFeIndex( m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange, u32RangeIndex);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	u32FeModIndex = 0;
	if(CS_REAL_IMP_1M_OHM == m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance)
		u32FeModIndex += DECADE_MAX_FE_SET_IMPED_INC;
	else if (CS_REAL_IMP_50_OHM != m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance)
		return CS_INVALID_IMPEDANCE;

	if( CS_COUPLING_AC == (m_pCardState->ChannelParams[u16ChanZeroBased].u32Term & CS_COUPLING_MASK) )
		u32FeModIndex += DECADE_MAX_FE_SET_COUPL_INC;
	else if( CS_COUPLING_DC != (m_pCardState->ChannelParams[u16ChanZeroBased].u32Term & CS_COUPLING_MASK) )
		return CS_INVALID_COUPLING;

	if(0 != m_pCardState->ChannelParams[u16ChanZeroBased].u32Filter)
		u32FeModIndex += DECADE_MAX_FE_SET_FLTER_INC;
	
	return CS_SUCCESS;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsDecade::FindFeIndex( const uInt32 u32Swing, uInt32& u32Index )
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

uInt16 CsDecade::GetControlDacId( eCalDacId ecdId, uInt16 u16Channel )
{
	switch ( ecdId )
	{
		case ecdPosition: return DECADE_POSITION_1 + u16Channel - 1;
		case ecdOffset: return DECADE_VCALFINE_1 + u16Channel - 1;
		case ecdGain: return DECADE_CAL_GAIN_1 + u16Channel - 1;
	}

	return 0xffff;
}


//-----------------------------------------------------------------------------
//	User channel skip depends on mode amd number of channels per card
//-----------------------------------------------------------------------------
uInt16	CsDecade::GetUserChannelSkip()
{
	uInt16	u16ChannelSkip = 0;

	switch (m_PlxData->CsBoard.NbAnalogChannel)
	{
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
//
//------------------------------------------------------------------------------
int32	CsDecade::CalibrateOffset(uInt16	u16HwChannel)
{
	const	int32			i32Tolerance = 32768/200;		// .5% of full scale
	int32	i32Avg_x10 = 0;	
	int32	i32AvgEven_x10 = 0;
	int32	i32Ret			= CS_SUCCESS;

	uInt16	u16Pos			= 0;
	uInt16	u16CurrentPos	= 0x8000;
	
	bool	bGoUp			= false;
	uInt32	u32Command = (CS_CHAN_1 == u16HwChannel) ? DECADE_COARSE_OFFSET_0 : DECADE_COARSE_OFFSET_1;
	uInt16  LastBOffset		= 0;
	int32   LastBTarget		= i32Tolerance;

	int32	i32CurrentRange_mV	= 0;
	int32	i32TargetLevel_mV	= m_i32DcOfffsetFudgeFactor_mV;
	int32	i32TargetBinary		= 0;
	int32	LastDeltaErr		= 0;


	if ( m_bFirmwareChannelRemap )
		u32Command = (CS_CHAN_2 == u16HwChannel) ? DECADE_COARSE_OFFSET_0 : DECADE_COARSE_OFFSET_1;

	DECADE_CAL_LUT CalInfo;
	i32Ret = FindCalLutItem(0, 50, CalInfo);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	uInt32 u32RangeIndex, u32FeModIndex;
	i32Ret = FindFeCalIndices( u16HwChannel, u32RangeIndex, u32FeModIndex );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Problem of HW. The driver completes successfully calibrate DC offset (~0mV DC)
	// Once switch back to external signal input, there is n DC offset ~5mV appear on the signal
	// if the isn put ranges is below +/- 1V.
	// The DC offset appears to be constant (TBD)
	// In order to resolve this problem of DC offset, here instead of trying calibrate to bring the DC ofset to 0mV
	// We wiil calibrate DC offset to bring the DC offset to ~-5mV to compensate the DC offset inhected by HW after calibration.
	// i32TargetLevel_mV will be the target DC offset we want to archive.
	i32CurrentRange_mV = m_pCardState->FeInfoTable[u32RangeIndex].u32Swing_mV;
	if ( i32CurrentRange_mV < 2000 )
		i32TargetBinary = 0x10000 * i32TargetLevel_mV / i32CurrentRange_mV;

	// Initial middle values for DACs
	i32Ret = UpdateCalibDac(u16HwChannel, ecdPosition, c_u32CalDacCount/2, true );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret = UpdateCalibDac(u16HwChannel, ecdOffset, c_u32CalDacCount/2, true );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret = UpdateCalibDac(u16HwChannel, ecdGain, c_u32CalDacCount/4, true );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Set Calib DC to 0V
	i32Ret = UpdateDcCalibReg(0x40 | CalInfo.u16PosRegMask);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	TraceCalib( u16HwChannel, TRACE_CAL_SIGNAL, 50, 0, CalInfo.i32PosLevel_uV, CalInfo.i32PosLevel_uV);
	TraceCalib( u16HwChannel, TRACE_COARSE_OFFSET );


	// up/down algorithm not work for all cases. so simply start with the expected one 
	// (normaly work with all input ranges). 
	// if fail (mostly with bad hardware) then try other direction. 
	bGoUp = true;

	u16CurrentPos = 0x8000;
	u16Pos	= 0;

	// Adjust the DC level and capture 
	i32Ret = WriteRegisterThruNios( u16CurrentPos, u32Command );
	if( CS_FAILED(i32Ret) )
        goto EndCalibrateOffset;

	i32Ret = AcquireAndAverage(u16HwChannel, &i32Avg_x10, &i32AvgEven_x10);
	if( CS_FAILED(i32Ret) )
        goto EndCalibrateOffset;

	// try one direction, if not found then change direction
	i32Ret = TryCalibrateOff(u16HwChannel, bGoUp, &LastBOffset, &LastDeltaErr);
	if ( i32Ret!=CS_SUCCESS || !LastBOffset )
	{
		bGoUp = !bGoUp;
		i32Ret = TryCalibrateOff(u16HwChannel, bGoUp, &LastBOffset, &LastDeltaErr);
	}

	if ( LastBOffset &&  i32Ret==CS_SUCCESS)
	{
		TraceCalib( u16HwChannel, TRACE_COARSE_OFFSET, LastBOffset, 0, LastBTarget );
		if ( LastDeltaErr > i32Tolerance )
		{
			char szErrorStr[256];
			CsEventLog EventLog;
			sprintf_s( szErrorStr, sizeof(szErrorStr), "ch %d, coarse calibration: margin value ( %d ) not in tolerance range ( %d )!", 
				ConvertToUserChannel(u16HwChannel), LastDeltaErr, i32Tolerance );
			EventLog.Report( CS_WARNING_TEXT, szErrorStr );
		}
		i32Ret = UpdateCalibDac(u16HwChannel, ecdPosition, LastBOffset, true );
		if( CS_FAILED(i32Ret) )
	        goto EndCalibrateOffset;
	}
	else
	{
		char szErrorStr[128];
		CsEventLog EventLog;
		sprintf_s( szErrorStr, sizeof(szErrorStr), "Calibration coarse channel %d failed !. Use default value", 
			ConvertToUserChannel(u16HwChannel) );
		EventLog.Report( CS_WARNING_TEXT, szErrorStr );

		// can not find the best value, use default
		TraceCalib( u16HwChannel, TRACE_COARSE_OFFSET, c_u32CalDacCount/2, 0, i32TargetBinary );
		i32Ret = UpdateCalibDac(u16HwChannel, ecdPosition, c_u32CalDacCount/2, true );
		if( CS_FAILED(i32Ret) )
	        goto EndCalibrateOffset;
	}

#ifdef nCALIB_FINE_OFFSET 
	// Then adjust fine offset to match 
	uInt16 u16Offset = 0;
	uInt16 u16OffsetDelta = c_u32CalDacCount/2;
	uInt16  LastBestOffset = 0;
	int32	i32Target = 10*i32Tolerance;
	int32   LastBestError = i32Target;
	
	// Loop to all possible values and find the value that produce smallest error
	while( u16OffsetDelta > 0 )
	{
		i32Ret = UpdateCalibDac(u16HwChannel, ecdOffset, u16Offset | u16OffsetDelta );
		if( CS_FAILED(i32Ret) )
	        goto EndCalibrateOffset;

		i32Ret = AcquireAndAverage(u16HwChannel, &i32Avg_x10, &i32AvgEven_x10);
		if( CS_FAILED(i32Ret) )
	        goto EndCalibrateOffset;

		i32Avg_x10 /= 10;
		i32CurrentError = i32Avg_x10 - i32TargetBinary;
		TraceCalib( u16HwChannel, TRACE_CAL_ITER, u16Offset|u16OffsetDelta, TRACE_FINE_OFFSET, i32Avg_x10, i32TargetBinary);
		if( Abs(i32CurrentError) <= i32Tolerance )
		{
			u16Offset |= u16OffsetDelta;
			if ( Abs(i32CurrentError) < Abs(LastBestError) )
			{
				LastBestError = i32CurrentError;
				LastBestOffset = u16Offset;
			}
		}
		
		//if ( 0 != u16Offset  )
			u16OffsetDelta >>= 1;

		if (!(u16OffsetDelta & 0xFFF0) ) break;				// don't care last 4 bit 
	}
	
	//TraceCalib( u16HwChannel, TRACE_FINE_OFFSET, u16Offset, 0, i32Target, i32Avg_x10 );
	
	if ( LastBestError < i32Target)
	{
		TraceCalib( u16HwChannel, TRACE_FINE_OFFSET, LastBestOffset, 0, i32Target, LastBestError );
		
		i32Ret = UpdateCalibDac(u16HwChannel, ecdOffset, LastBestOffset, true );
		if( CS_FAILED(i32Ret) )
	        goto EndCalibrateOffset;
	}

	if( 0 == u16Offset || (c_u32CalDacCount - 1) == u16Offset )
	{
		i32Ret = CS_FINE_OFFSET_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16HwChannel);
		UpdateCalibDac(u16HwChannel, ecdOffset, c_u32CalDacCount/2, true );
		sprintf_s( szText, sizeof(szText),  "CalibratePosition Fine Offset failed! channel %d\n", u16HwChannel);
		OutputDebugString(szText);
	}		
#endif
EndCalibrateOffset:   

	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsDecade::CalibratePosition(uInt16 u16Channel)
{
	int32 i32Ret = CS_SUCCESS;
	if( (0 == u16Channel) || (u16Channel > DECADE_CHANNELS)  )
		return CS_INVALID_CHANNEL;

	uInt16 u16ChanZeroBased = u16Channel-1;
	int32 i32PosCode = 0;
	uInt32 u32Code = 0;
	char szText[256]={0};
	
	uInt32	u32Command = (CS_CHAN_1 == u16Channel) ? DECADE_COARSE_OFFSET_0 : DECADE_COARSE_OFFSET_1;
	if ( m_bFirmwareChannelRemap )
		u32Command = (CS_CHAN_2 == u16Channel) ? DECADE_COARSE_OFFSET_0 : DECADE_COARSE_OFFSET_1;

	uInt32 u32InpuRange = m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange;
	DECADE_CAL_LUT CalInfo;
	uInt32 u32CalRange = u32InpuRange;

	
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
	int32 i32Target = labs(m_pCardState->AcqConfig.i32SampleRes)*5/2;		//1/4 gain = SampleRes * 10 /4
	int32 i32Avg_x10 = 0;
	uInt32 u32PosPos = 0, u32PosDelta = c_u32CalDacCount/4;

	while( u32PosDelta > 0 )
	{
		i32PosCode = m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset;
		i32PosCode += u32PosPos|u32PosDelta;
		u32Code = i32PosCode > 0 ? ( i32PosCode < int32(c_u32CalDacCount-1) ? uInt16(i32PosCode) : c_u32CalDacCount-1 ) : 0;
		
		i32Ret = WriteRegisterThruNios( u32Code, u32Command );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		BBtiming( 50 );

		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		TraceCalib( u16Channel, TRACE_CAL_ITER, u32Code, TRACE_CAL_POS, i32Avg_x10, i32Target);
		if( Abs(i32Avg_x10) < Abs(i32Target) )
			u32PosPos |= u32PosDelta;
		u32PosDelta >>= 1;
		
		if (!(u32PosDelta & 0xFFF0) ) break;				// don't care last 4 bits 
	}
	
	if( 0 == u32PosPos || (c_u32CalDacCount - 1) == u32PosPos )
	{
		i32Ret = WriteRegisterThruNios( m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset, u32Command );
		sprintf_s( szText, sizeof(szText),  "CalibratePosition failed! Positive side. channel %d\n", u16Channel);
		OutputDebugString(szText);
		return CS_POSITION_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
	}

	// Negative side
	// Find the value of DAC so that we have DC offset at -25%
	i32Target = m_pCardState->AcqConfig.i32SampleRes*5/2;	//1/4 gain = SampleRes * 10 /4
	uInt32 u32NegPos = 0;
	u32PosDelta = c_u32CalDacCount/4;

	while( u32PosDelta > 0 )
	{
		i32PosCode = m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset;
		i32PosCode -= u32NegPos|u32PosDelta;
		u32Code = i32PosCode > 0 ? ( i32PosCode < int32(c_u32CalDacCount-1) ? uInt32(i32PosCode) : c_u32CalDacCount-1 ) : 0;

		i32Ret = WriteRegisterThruNios( u32Code, u32Command );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		BBtiming( 50 );

		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		TraceCalib( u16Channel, TRACE_CAL_ITER, u32Code, TRACE_CAL_POS, i32Avg_x10, i32Target);
//		if( i32Avg_x10 >= i32Target )
		if( Abs(i32Avg_x10) < Abs(i32Target) )
			u32NegPos |= u32PosDelta;
		u32PosDelta >>= 1;
		
		if (!(u32PosDelta & 0xFFF0) ) break;				// don't care last 4 bits 
	}
	
	if( 0 == u32NegPos || (c_u32CalDacCount - 1) == u32NegPos )
	{
		// Write default value.
		i32Ret = WriteRegisterThruNios( m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset, u32Command );
		sprintf_s( szText, sizeof(szText),  "CalibratePosition failed! Negative side. channel %d\n", u16Channel);
		OutputDebugString(szText);
		return CS_POSITION_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
	}
	
	i32Ret = WriteRegisterThruNios( m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset, u32Command );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CodeDeltaForHalfIR = (uInt16) (u32PosPos + u32NegPos)/2;
	
	TraceCalib( u16Channel, TRACE_CAL_POS, u32PosPos, u32NegPos);

	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
//ex:			TraceDebugCalib( u16HwChannel, TRACE_CAL_ITER, u16Pos|u16PosDelta, TRACE_COARSE_OFFSET, i32Avg_x10);
//				TraceDebugCalib( u16HwChannel, TRACE_COARSE_OFFSET );

void CsDecade::TraceDebugCalib(uInt16 u16Channel, uInt32 u32TraceFlag, uInt32 u32V1, uInt32 u32V2, int32 i32V3, int32 i32V4)
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
				sprintf_s( szText, sizeof(szText),  "\t : : Code 0x%04x (%4d) Meas %c%5ld.%ld (%c%5ld.%ld)\n",
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
int32	CsDecade::UpdateCalibDac(uInt16 u16Channel, eCalDacId ecdId, uInt16 u16Data, bool bVal)
{
	int32 i32Ret = CS_SUCCESS;
	uInt32	u32Command = 0;

	if( (0 == u16Channel) || (u16Channel > DECADE_CHANNELS)  )
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
			{
			u32Command = (CS_CHAN_1 == u16Channel) ? DECADE_COARSE_OFFSET_0 : DECADE_COARSE_OFFSET_1;

			if ( m_bFirmwareChannelRemap )
				u32Command = (CS_CHAN_2 == u16Channel) ? DECADE_COARSE_OFFSET_0 : DECADE_COARSE_OFFSET_1;

			// Adjust the DC level 
			i32Ret = WriteRegisterThruNios( u16Data, u32Command );
			if( CS_FAILED(i32Ret) )
				return i32Ret;
			
			if (bVal)
				m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset = u16Data;
			}
			break;

		case ecdOffset:
			{
			u32Command = (CS_CHAN_1 == u16Channel) ? DECADE_FINE__OFFSET_0 : DECADE_FINE__OFFSET_1;
			if ( m_bFirmwareChannelRemap )
				u32Command = (CS_CHAN_2 == u16Channel) ? DECADE_FINE__OFFSET_0 : DECADE_FINE__OFFSET_1;
			i32Ret = WriteRegisterThruNios( u16Data, u32Command );
			if( CS_FAILED(i32Ret) )
				return i32Ret;
			
			if (bVal)
				m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16FineOffset = u16Data; 
			}
			break;
			
		case ecdGain:
			{
				i32Ret = AdjustAdcGain(u16Channel, u16Data);
				if( CS_FAILED(i32Ret) )
					return i32Ret;

				if (bVal)
					m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16Gain = u16Data; 
			break;
			}
	}
	if (bVal)
	{
		TraceCalib( u16Channel, TRACE_CAL_DAC, u16Data, ecdId);
	}
				
	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDecade::RestoreCalibModeSetting()
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32RegVal = 0;

	i32Ret = WriteRegisterThruNios( u32RegVal, DECADE_ADDON_CPLD_MISC_CONTROL, -1, &u32RegVal );
	if ( CS_SUCCEEDED( i32Ret ) )
	{
		i32Ret = WriteRegisterThruNios( u32RegVal, DECADE_ADDON_CPLD_MISC_CONTROL | DECADE_SPI_WRITE );
		BBtiming( 200 );
	}

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDecade::SendCalibModeSetting( eCalMode eMode )
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32RegVal = 0;

	if ( ecmCalModeDc == eMode )
	{
		// Disable all interrupts
		WriteGageRegister( INT_CFG, 0 );
		// Switch to the Calibmode
		u32RegVal = 0;
	}
	else
	{
		// Enable interrupts
		WriteGageRegister(INT_CFG, OVER_VOLTAGE_INTR);
		u32RegVal = 2;					// Set CH01_CAL_RDY_D
	}

	i32Ret = WriteRegisterThruNios( u32RegVal, DECADE_ADDON_CPLD_MISC_CONTROL | DECADE_SPI_WRITE );
	BBtiming( 150 );

	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsDecade::DefaultCalibrationSettings(uInt16 u16Channel, uInt32 u32RestoreFlag)
{
	TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 4, u32RestoreFlag);
	uInt32 u32RangeIndex, u32FeModIndex;
	int32 i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	uInt16 u16ChanZeroBased = u16Channel - 1;
	m_pCardState->bFastCalibSettingsDone[u16ChanZeroBased] = false;

	m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].bCalibrated = false;
	
	if( 0 != (u32RestoreFlag  & DECADE_CAL_RESTORE_FINE) )
		m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16FineOffset = c_u32CalDacCount/2;
		
	if( 0 != (u32RestoreFlag  & DECADE_CAL_RESTORE_COARSE) )
		m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset = c_u32CalDacCount/2;
	if( 0 != (u32RestoreFlag  & DECADE_CAL_RESTORE_GAIN) )
		m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16Gain = c_u32CalDacCount/2;
	if( 0 != (u32RestoreFlag  & DECADE_CAL_RESTORE_POS) )
		m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CodeDeltaForHalfIR = c_u32CalDacCount/8;		// 1/4 unipolaire gain
	
	return FastCalibrationSettings(u16Channel, true );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsDecade::AdjustAdcGain(uInt16 u16Channel, uInt32 u32Val, bool bI, bool bQ)
{
	uInt16	u16ChanZeroBased = u16Channel - 1;
	int32	i32Ret = CS_SUCCESS;

	if (bI)
	{
		i32Ret = WriteToAdcRegister(u16Channel, 0x03, (uInt16)u32Val);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	if (bQ)
	{
		i32Ret = WriteToAdcRegister(u16Channel, 0x0B, (uInt16)u32Val);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	// By changing ADC Gain register, we will have to perform I/Q calib later
	// Set the flag Calib I/Q is required
	m_Adc_GainIQ_CalNeeded[u16ChanZeroBased] = true;

	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsDecade::AdjustAdcOffset(uInt16 u16Channel, uInt32 u32Val, bool bI, bool bQ)
{
	uInt16	u16ChanZeroBased = u16Channel - 1;
	int32	i32Ret = CS_SUCCESS;

	if (bI)
	{
		i32Ret = WriteToAdcRegister(u16Channel, 0x02, (uInt16)u32Val);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	if (bQ)
	{
		i32Ret = WriteToAdcRegister(u16Channel, 0x0A, (uInt16)u32Val);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	// By changing ADC Gain register, we will have to perform I/Q calib later
	// Set the flag Calib I/Q is required
	m_Adc_OffsetIQ_CalNeeded[u16ChanZeroBased] = true;

	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsDecade::UpdateDcCalibReg(uInt16 u16Data)
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32Command = DECADE_BB_DC_CALIB_REG_OLD;
	
//	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("u32AddonBoardVersion=0x%x\n"), m_PlxData->CsBoard.u32AddonBoardVersion );
//	OutputDebugString(szDbgText);
	if( m_PlxData->CsBoard.u32AddonBoardVersion >= ADDON_HW_VERSION_1_07)
 		u32Command = DECADE_BB_DC_CALIB_REG;	

	i32Ret = WriteRegisterThruNios(u16Data, u32Command);	

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDecade::AdcOffsetIQCalib()
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32RetCode = 0;
	uInt32	u32Command = DEACDE_ADC_IQ_OFFSET_CALIB;
	
	// Retry 5 times
	for (int i = 0; i < 5; i++)
	{
		i32Ret = WriteRegisterThruNios( 0, u32Command, -1, &u32RetCode );
		if ( CS_SUCCEEDED(i32Ret) )
		{
			if ( 0 == u32RetCode )
			{
				i32Ret = CS_SUCCESS;
				break;
			}
			else
			{
#ifdef _DEBUG
				char	szText[128];

				sprintf_s( szText, sizeof(szText), TEXT("ADC I/Q OffsetCabib (Cmd 0x%x) failed. RetCode=0x%x\n"), u32Command, u32RetCode );
				OutputDebugString(szText);
#endif
				i32Ret = CS_FAIL;
			}
		}
	}


	// Log the error
	if ( CS_FAIL == i32Ret )
	{
		char	szText[128];
		CsEventLog EventLog;

		sprintf_s( szText, sizeof(szText), TEXT("ADC I/Q OffsetCabib (Cmd 0x%x) failed. RetCode=0x%x\n"), u32Command, u32RetCode );
#ifdef _DEBUG
		OutputDebugString(szText);
#endif
		EventLog.Report( CS_ERROR_TEXT, szText );
	}

	return i32Ret;
}



//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsDecade::DumpAverageTable(uInt16 u16Channel, eCalDacId ecdId, uInt16 u16Vref, bool bInitDefault, uInt16 NbSteps)
{
	int32	i32Ret = CS_SUCCESS;
	int32	i32Avg_x10;	
	uInt16	u16maxStep = 16;	
	uInt16 u16Data = u16Vref | 0x40;	//Vref for setting gain

	uInt16 	u16CurrentPos = 0;
	uInt16 	u16Step = 0;
	int32	i32Avg_x10Table[64]={0};	
	uInt32	u32Command  = 0;
	uInt32 u32AdcRegister1=0, u32AdcRegister2=0;
	DECADE_CAL_LUT CalInfo;

	if ((NbSteps>0) & (NbSteps<=64))
		u16maxStep = NbSteps;

	if (ecdId!=ecdGain)
		u16Step = (uInt16)(c_u32CalDacCount /u16maxStep);
	else
		u16Step = (uInt16)(0x8000 /u16maxStep);		// I/Q Full Scale 0-7FFF. See ADC12Dx100
		

	i32Ret = FindCalLutItem(0, 50, CalInfo);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Make sure to switch to mode DC
	// SendCalibModeSetting( ecmCalModeDc );

	if (bInitDefault)
	{
		// Initial Default values for DACs
		i32Ret = UpdateCalibDac(u16Channel, ecdPosition, c_u32CalDacCount/2, true );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = UpdateCalibDac(u16Channel, ecdOffset, c_u32CalDacCount/2, true );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = UpdateCalibDac(u16Channel, ecdGain, c_u32CalDacCount/4, true );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		// Set Calib DC to 0V
		i32Ret = UpdateDcCalibReg(0x40 | CalInfo.u16PosRegMask);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	switch(ecdId)
	{
		case ecdPosition:
		
			u32Command = (CS_CHAN_1 == u16Channel) ? DECADE_COARSE_OFFSET_0 : DECADE_COARSE_OFFSET_1;

			if ( m_bFirmwareChannelRemap )
				u32Command = (CS_CHAN_2 == u16Channel) ? DECADE_COARSE_OFFSET_0 : DECADE_COARSE_OFFSET_1;
			break;
		
		case ecdOffset:
			u32Command = (CS_CHAN_1 == u16Channel) ? DECADE_FINE__OFFSET_0 : DECADE_FINE__OFFSET_1;
			if ( m_bFirmwareChannelRemap )
				u32Command = (CS_CHAN_2 == u16Channel) ? DECADE_FINE__OFFSET_0 : DECADE_FINE__OFFSET_1;
			break;
			
		case ecdGain:
				u32Command = (CS_CHAN_1 == u16Channel)? DECADE_ADC2 : DECADE_ADC1;
				u32AdcRegister1 = u32Command | 0x0300;	// I-Channel

				u32AdcRegister2 = u32Command | 0x0B00;	// Q-Channel
				break;
		default:
			break;
		
	}

	for (int i=0;i<u16maxStep;i++)
	{
		switch(ecdId)
		{
			case ecdPosition:
			case ecdOffset:
				i32Ret = WriteRegisterThruNios( u16CurrentPos, u32Command );
					break;
			case ecdGain:
				i32Ret = WriteRegisterThruNios( u16CurrentPos, u32AdcRegister1 );
			
				i32Ret = WriteRegisterThruNios( u16CurrentPos, u32AdcRegister2 );

				i32Ret = UpdateDcCalibReg(u16Data);
				BBtiming( 200 );
					break;
		}
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )
			break;
		
		i32Avg_x10Table[i]=i32Avg_x10;
		u16CurrentPos = u16CurrentPos + u16Step;
	}


#ifdef _WINDOWS_
	char szText[256]={0}, szText2[256]={0};
	sprintf_s( szText, sizeof(szText),  "Decade %d Ch. (ecdId=%d) i32Avg_x10Table[0-%d] \n",
				u16Channel, ecdId, u16maxStep);
	DumpOutputMsg(szText);	
	for(int i=0; i<u16maxStep; i++)
	{
		double  fScale = (CS_GAIN_2_V/2);
		double	fVolt =  (-1.0*i32Avg_x10Table[i]/m_pCardState->AcqConfig.i32SampleRes/10)*fScale;

		sprintf_s( szText2, sizeof(szText2),  "[0x%04x]	%07d	\t%lf mv",	
					u16Step*i, i32Avg_x10Table[i], fVolt); 
		DumpOutputMsg(szText2);	
	}
#else
	fprintf( stdout,  "Decade %d Ch. (ecdId=%d) i32Avg_x10Table[0-7] %d %d %d %d %d %d %d %d\n",
				u16Channel, ecdId, 
				i32Avg_x10Table[0], i32Avg_x10Table[1], i32Avg_x10Table[2], i32Avg_x10Table[3], i32Avg_x10Table[4], i32Avg_x10Table[5], i32Avg_x10Table[6], i32Avg_x10Table[7]); 
	fprintf( stdout,  "Decade %d Ch. (ecdId=%d) i32Avg_x10Table[8-15] %d %d %d %d %d %d %d %d\n",
				u16Channel, ecdId, 
				i32Avg_x10Table[8], i32Avg_x10Table[9], i32Avg_x10Table[10], i32Avg_x10Table[11], i32Avg_x10Table[12], i32Avg_x10Table[13], i32Avg_x10Table[14], i32Avg_x10Table[15]); 
#endif	
	
	return i32Ret;

}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDecade::SendADC_IQ_Calib( uInt16 u16HwChannel, bool bGain_Offset /* true = Gain, false = Offset */ )
{
	int32	i32Ret = CS_SUCCESS;
	uInt16 u16ChanZeroBased = u16HwChannel-1;
	uInt32	u32RetCode = 0;
	uInt32	u32Command = bGain_Offset ? DEACDE_ADC_IQ_GAIN_CALIB : DEACDE_ADC_IQ_OFFSET_CALIB;
	DECADE_ADC_IQ_CALIB_REG	I_Q_CalibReg = {0};
	int i = 0;
	char	szText[128];
	bool	b6G_card = (CSDECADE126G_BOARDTYPE == m_PlxData->CsBoard.u32BoardType);

	// For 6G board,  6 GS/s and 2GS/s sampling rate use both ADCs. ChannelSelect specify ADC used.
	if ( 0 != m_pCardState->AddOnReg.u16PingPong )
		I_Q_CalibReg.bits.ChannelSelect = 0x3;
	else
		I_Q_CalibReg.bits.ChannelSelect = u16HwChannel;

	switch (m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange)
	{
	case 200:
		I_Q_CalibReg.bits.GainEncoder = 1;
		break;
	case 400:
		I_Q_CalibReg.bits.GainEncoder = 2;
		break;
	case 1000:
		I_Q_CalibReg.bits.GainEncoder = 3;
		break;
	case 2000:
		I_Q_CalibReg.bits.GainEncoder = 4;
		break;
	case 4000:
		I_Q_CalibReg.bits.GainEncoder = 5;
		break;
	default:
		I_Q_CalibReg.bits.GainEncoder = 6;
		break;
	}

	I_Q_CalibReg.bits.cal_relay_disable = 1;

#ifdef DEBUG_CALIB_IQ
	sprintf_s( szText, sizeof(szText), "SendADC_IQ_Calib(%d, %d) START	ChanZeroBased %d I_Q_CalibReg.u32RegVal 0x%lx \n", 
				u16HwChannel, bGain_Offset, u16ChanZeroBased, I_Q_CalibReg.u32RegVal);
	OutputDebugString(szText);
#endif

	// Patch requested by Nestor
	if ( (b6G_card) && ( m_PlxData->CsBoard.u32AddonBoardVersion >= ADDON_HW_VERSION_1_07))
		I_Q_CalibReg.u32RegVal = 0;

	// FW problems. Have to retry many times until succees
	for (i = 0; i < 20; i++)
	{
		i32Ret = WriteRegisterThruNios( I_Q_CalibReg.u32RegVal, u32Command, -1, &u32RetCode );
		if ( CS_SUCCEEDED(i32Ret) )
		{
			if ( 0 == u32RetCode  )
			{
				if ( bGain_Offset )
					m_Adc_GainIQ_CalNeeded[u16ChanZeroBased] = false;
				else
					m_Adc_OffsetIQ_CalNeeded[u16ChanZeroBased] = false;

				i32Ret = CS_SUCCESS;
				break;
			}
			else
			{
#ifdef _DEBUG
				sprintf_s( szText, sizeof(szText), TEXT("ADC I/Q Calib (Cmd 0x%x) failed (params = 0x%x). RetCode=0x%x\n"), u32Command, I_Q_CalibReg.u32RegVal, u32RetCode );
				OutputDebugString(szText);
#endif
				i32Ret = bGain_Offset ? CS_ADC_IQ_GAIN_CALIB_FAILED : CS_ADC_IQ_OFFSET_CALIB_FAILED ;
			}
		}
	}

	CsEventLog EventLog;
	// Log the error
	if ( CS_ADC_IQ_GAIN_CALIB_FAILED == i32Ret || CS_ADC_IQ_OFFSET_CALIB_FAILED == i32Ret )
		EventLog.Report( CS_ERROR_TEXT, szText );

#ifdef DEBUG_CALIB_IQ
	sprintf_s( szText, sizeof(szText), "WriteRegisterThruNios(val = 0x%x, u32Command = 0x%x) END nbtry %d RetCode=0x%x \n", 
				I_Q_CalibReg.u32RegVal, u32Command, i, u32RetCode);
	OutputDebugString(szText);
#endif

	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsDecade::CalibrateADC_IQ()
{
	int32	i32Ret = CS_SUCCESS;
	uInt16	u16UserChan = 1;
	bool	bIQ_Calib_Required = false;
	int32	i32CurrentPos[DECADE_CHANNELS] = {0};
	uInt16	u16StartIndex = m_ChannelStartIndex;
	uInt16	u16EndIndex = m_PlxData->CsBoard.NbAnalogChannel;
	uInt16	u16Increment = GetUserChannelSkip();

	if ( ! m_bBetterCalib_IQ )
		return CS_SUCCESS;

	if (m_bPingpongCapture)
	{
		// With acquisitions in pingpong mode, we always have to calibrate I/Q for both ADCs
		u16StartIndex	= CS_CHAN_1;
		u16EndIndex		= CS_CHAN_2;
		u16Increment =	 1;
	}

	// Check if we need to perform IQ calib
	for( u16UserChan = u16StartIndex; u16UserChan <= u16EndIndex; u16UserChan += u16Increment )
	{
		uInt16 u16HwChan = ConvertToHwChannel(u16UserChan);
		uInt16 u16ChanZeroBased = u16HwChan-1;
		if ( m_Adc_GainIQ_CalNeeded[u16ChanZeroBased] || m_Adc_OffsetIQ_CalNeeded[u16ChanZeroBased] )
		{
			bIQ_Calib_Required = true;
			break;
		}
	}

	if ( ! bIQ_Calib_Required )
		return CS_SUCCESS;

	// Calib I/Q unable to handle DC Offset. Software should reset offset before Calib_IQ and resend position after
	for( u16UserChan = m_ChannelStartIndex; u16UserChan <= m_PlxData->CsBoard.NbAnalogChannel; u16UserChan += GetUserChannelSkip() )
	{
		uInt16 u16HwChan = ConvertToHwChannel(u16UserChan);
		uInt16 u16ChanZeroBased = u16HwChan - 1;

		if ( m_pCardState->ChannelParams[u16ChanZeroBased].i32Position )
		{
			i32CurrentPos[u16ChanZeroBased] = m_pCardState->ChannelParams[u16ChanZeroBased].i32Position;
			i32Ret = SendPosition(u16HwChan, 0 );
		}
	}

	SendCalibModeSetting( ecmCalModeDc );

	// Perform Calib I/Q on the active channels
	// It is recommended to run always I/Q for Gain and Offset together.
	//for( u16UserChan = m_ChannelStartIndex; u16UserChan <= m_PlxData->CsBoard.NbAnalogChannel; u16UserChan += GetUserChannelSkip() )
	for (u16UserChan = u16StartIndex; u16UserChan <= u16EndIndex; u16UserChan += u16Increment)
	{
		uInt16 u16HwChan = ConvertToHwChannel(u16UserChan);
		// Need to calib for all channels, otherwise Enob decrease.
		//if (m_Adc_GainIQ_CalNeeded[u16HwChan - 1] || m_Adc_OffsetIQ_CalNeeded[u16HwChan - 1])
		{
			// I/Q Offset
			i32Ret = SendADC_IQ_Calib(u16HwChan, false);
			if (CS_FAILED(i32Ret))
				break;
			BBtiming(25);

			// I/Q Gain
			i32Ret = SendADC_IQ_Calib(u16HwChan, true);
			if (CS_FAILED(i32Ret))
				break;
			BBtiming(25);
		}
	}

	if (!m_bPingpongCapture && m_pCardState->VerInfo.BaseBoardFwVersion.Version.uIncRev >= 137)
	{
		i32Ret = WriteRegisterThruNios(0, DEACDE_ADC_INTERNAL_CALIB);
		if (CS_FAILED(i32Ret))
		{
			char szText[128];
			sprintf_s(szText, sizeof(szText), TEXT("Internal ADC Calib (Cmd=0x3800000) failed (RetCode =%d)\n"), i32Ret);
#ifdef _DEBUG
			OutputDebugString(szText);
#endif
			CsEventLog EventLog;
			EventLog.Report(CS_ERROR_TEXT, szText);
		}
	}

	// Waiting for the relay stable, otherwise the first acquisition will have some garbage data
	BBtiming(150);
	SendCalibModeSetting();

	// Restore current position
	for( u16UserChan = m_ChannelStartIndex; u16UserChan <= m_PlxData->CsBoard.NbAnalogChannel; u16UserChan += GetUserChannelSkip() )
	{
		uInt16 u16HwChan = ConvertToHwChannel(u16UserChan);
		uInt16 u16ChanZeroBased = u16HwChan - 1;

		if ( i32CurrentPos[u16ChanZeroBased] )
		{
			i32Ret = SendPosition(u16HwChan, i32CurrentPos[u16ChanZeroBased] );
		}
	}

	return i32Ret;
}


int32	CsDecade::TryCalibrateOff(uInt16 u16HwChannel, bool bIsGoUp, uInt16 *pBestOffset, int32	*i32DeltaErr)
{
	int32	i32Tolerance = 32768/100;		// 1% of full scale
	int32	i32Avg_x10 = 0;
	int32	i32CurrentError	= 0x7FFF;
	int32	i32Ret			= CS_SUCCESS;

	uInt16	u16Pos			= 0;
	uInt16	u16CurrentPos	= 0x8000;
	
	bool	bLastErrorSignPos	= false;
	uInt16  LastBOffset		= 0;
	int32   LastBTarget		= 0;
	bool	bGoUp = bIsGoUp;

	int32	i32TargetBinary		= 0;

	if (pBestOffset)
		*pBestOffset = 0;

	if ( ! bGoUp )
		u16CurrentPos >>= 1;

	LastBTarget		= i32Tolerance;

	// Loop until we get the DC offset reach the target level and within the error tolerance
	while( u16CurrentPos > 0 )
	{
		// Adjust the DC level and capture 
		//i32Ret = WriteRegisterThruNios( u16CurrentPos | u16Pos, u32Command );
		i32Ret = UpdateCalibDac(u16HwChannel, ecdPosition, u16CurrentPos | u16Pos );
		if( CS_FAILED(i32Ret) )
	        goto EndTryCalibrateOff;

		BBtiming( 30 );

		i32Ret = AcquireAndAverage(u16HwChannel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )
	        goto EndTryCalibrateOff;

		i32Avg_x10 /= 10;
		TraceCalib( u16HwChannel, TRACE_CAL_ITER, u16Pos|u16CurrentPos, TRACE_COARSE_OFFSET, i32Avg_x10);

		i32CurrentError = i32Avg_x10 - i32TargetBinary;

		// Check if the DC offset meets the requirement
		if ( Abs(i32CurrentError) < Abs(LastBTarget) )
		{
			LastBTarget = i32CurrentError;
			LastBOffset = u16Pos | u16CurrentPos;
		}				
		if ( bLastErrorSignPos != (i32CurrentError > 0) )
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
		
		if (!(u16CurrentPos & 0xFFF0) ) break;				// don't care last 4 bit 
	}

	// Set the final value
	if ( Abs(LastBTarget) < Abs(i32Tolerance) )
	{
		if (pBestOffset)
			*pBestOffset = LastBOffset;
		if (i32DeltaErr)
			*i32DeltaErr = LastBTarget;
	}
EndTryCalibrateOff:
#ifdef  DBG_CALIB_OFFSET
	{
	char    szText[128]={0};
	sprintf_s( szText, sizeof(szText),  "\t : : TryCalibrateOff(%d) ret %d  bGoUp %d LastBOffset 0x%x LastBTarget %ld i32Tolerance %ld \n",
				u16HwChannel, i32Ret, bIsGoUp, *pBestOffset, LastBTarget, i32Tolerance); 
	OutputDebugString(szText);
	}
#endif
	 
    return(i32Ret);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsDecade::DumpGainTable(uInt16 u16Channel, uInt16 u16Vref, bool bInitDefault, uInt16 Pos_0V, uInt16 NbSteps)
{
	int32	i32Ret = CS_SUCCESS;
	int32	i32Avg_x10;	
	uInt16	u16maxStep = 16;	
	uInt16 u16Data = u16Vref | 0x40;	//Vref for setting gain

	uInt16 	u16CurrentPos = 0;
	uInt16 	u16Step = 0;
	int32	i32Avg_x10Table[64]={0};	
	float	fVolt[64]={0};
	uInt32	u32Command  = 0;
	uInt32 u32AdcRegister1=0, u32AdcRegister2=0;
	DECADE_CAL_LUT CalInfo;

	if ((NbSteps>0) & (NbSteps<=64))
		u16maxStep = NbSteps;

	u16Step = (uInt16)(0x8000 /u16maxStep);		// I/Q Full Scale 0-7FFF. See ADC12Dx100
		
	i32Ret = FindCalLutItem(0, 50, CalInfo);
	if( CS_FAILED(i32Ret) )
		return i32Ret;
	
	u32Command = (CS_CHAN_1 == u16Channel)? DECADE_ADC2 : DECADE_ADC1;
	u32AdcRegister1 = u32Command | 0x0300;	// I-Channel
	u32AdcRegister2 = u32Command | 0x0B00;	// Q-Channel	

	// Make sure to switch to mode DC
	// SendCalibModeSetting( ecmCalModeDc );

	if (bInitDefault)
	{
		// Initial Default values for DACs
		i32Ret = UpdateCalibDac(u16Channel, ecdPosition, c_u32CalDacCount/2, true );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = UpdateCalibDac(u16Channel, ecdOffset, c_u32CalDacCount/2, true );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

//		i32Ret = UpdateCalibDac(u16Channel, ecdGain, c_u32CalDacCount/4, true );
//		if( CS_FAILED(i32Ret) )
//			return i32Ret;

		// Set Calib DC to 0V
		i32Ret = UpdateDcCalibReg(0x40 | CalInfo.u16PosRegMask);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}
	else
	{
		if (Pos_0V != 0x8000) 
		{
			i32Ret = UpdateCalibDac(u16Channel, ecdPosition, c_u32CalDacCount/2, true );
			if( CS_FAILED(i32Ret) )
				return i32Ret;
		}
	}

	for (int i=0;i<u16maxStep;i++)
	{
		i32Ret = WriteRegisterThruNios( u16CurrentPos, u32AdcRegister1 );
			
		i32Ret = WriteRegisterThruNios( u16CurrentPos, u32AdcRegister2 );

		i32Ret = UpdateDcCalibReg(u16Data);
		BBtiming( 200 );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )
			break;
		
		i32Avg_x10Table[i]=i32Avg_x10;
		u16CurrentPos = u16CurrentPos + u16Step;
		fVolt[i] =  (float)(-1.0*i32Avg_x10/m_pCardState->AcqConfig.i32SampleRes/10)*1000;

	}


#ifdef _WINDOWS_
	char szText[256]={0}, szText2[256]={0};
	sprintf_s( szText, sizeof(szText),  "Decade Ch.%d  Vref 0x%x Gain value i32Avg_x10Table[0-%d] \n",
				u16Channel, u16Vref, u16maxStep);
	OutputDebugString(szText);	
	for(int i=0; i<u16maxStep; i++)
	{
		sprintf_s( szText2, sizeof(szText2),  "[0x%x]	%d  %f mv ",
					u16Step*i, i32Avg_x10Table[i], fVolt[i]); 
		OutputDebugString(szText2);	
	}
#else
	fprintf( stdout,  "Decade Ch.%d  Vref 0x%x Gain value i32Avg_x10Table[0-%d] \n",
				u16Channel, u16Vref, u16maxStep);
	for(int i=0; i<u16maxStep; i++)
	{
		fprintf( stdout,  "[0x%x]	%d  %f mv ",
					u16Step*i, i32Avg_x10Table[i], fVolt[i]); 
	}
#endif	
	
	return i32Ret;

}
