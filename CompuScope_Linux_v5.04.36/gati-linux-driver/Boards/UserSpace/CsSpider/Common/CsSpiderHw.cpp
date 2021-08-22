//	CsxyG8Hw.cpp
///	Implementation of CsSpiderDev class
//
//	This file contains all member functions that access to addon hardware including
//	Frontend set up and calibration
//
//

#include "StdAfx.h"
#include "CsSpider.h"
#include "Cs8xxxHwCaps.h"
#include "CsEventLog.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#endif
#include "CsIoctl.h"


#define	 CSPDR_CALIB_TIMEOUT_LONG	KTIME_SECOND(1)			// 1 second timeout
#define	 CSPDR_CALIB_TIMEOUT_SHORT	KTIME_MILISEC(300)		// 300 milisecond timeout

//-----------------------------------------------------------------------------
//----- SendClockSetting
//-----------------------------------------------------------------------------

int32	CsSpiderDev::SendClockSetting(LONGLONG llSampleRate, uInt32 u32ExtClk, uInt32 u32ExtClkSampleSkip, BOOLEAN bRefClk )
{
	SPIDER_SAMPLE_RATE_INFO SrInfo = {0};

	uInt16 u16ClkSelect = m_pCardState->AddOnReg.u16ClkSelect & CSPDR_CLK_OUT_MUX_MASK;

	u16ClkSelect &= ~CSPRD_CLK_OUT_ENABLE;

  	if ( 0 != u32ExtClk )
	{
		if( 0 != (m_pCardState->PlxData.CsBoard.u32AddonHardwareOptions&CSPDR_AOHW_OPT_EMONA_FF) || m_pCardState->bZap )
			return CS_NO_EXT_CLK;

		if( (CSPDR_MIN_EXT_CLK > llSampleRate) || (m_pCardState->SrInfo[0].llSampleRate < llSampleRate) )
			return CS_EXT_CLK_OUT_OF_RANGE;

		if ( (u32ExtClkSampleSkip / 1000 ) == 0 )
			SrInfo.u32Decimation = 1;
		else
			SrInfo.u32Decimation = u32ExtClkSampleSkip / 1000;

		SrInfo.llSampleRate = llSampleRate;
		SrInfo.u32ClockFrequencyMhz = uInt32(llSampleRate / 1000000);
		SrInfo.u32ClockFrequencyMhz |= CSPDR_EXT_CLK_BYPASS;		// Bypass mode

		u16ClkSelect |= CSPRD_SET_EXT_CLK | CSPRD_SET_PLL_CLK | CSPRD_SET_STRAIGHT_CLK;
	}
	else
	{
		u16ClkSelect |= CSPRD_SET_PLL_CLK | CSPRD_SET_STRAIGHT_CLK;
		if( bRefClk )
		{
			u16ClkSelect |= CSPRD_SET_EXT_CLK;
		}

		if ( !FindSrInfo( llSampleRate, &SrInfo ) )
		{
			return CS_INVALID_SAMPLE_RATE;
		}
	}

	uInt16 u16MsSetup = 0;
	if ( ermStandAlone == m_pCardState->ermRoleMode )
		u16MsSetup = 0x7F;
	else
		u16MsSetup = 0;

	uInt16 u16DecimationLow = uInt16(SrInfo.u32Decimation & 0x7fff);
	uInt16 u16DecimationHigh = uInt16((SrInfo.u32Decimation>>15) & 0x7fff);

	if(  m_pCardState->AddOnReg.u32ClkFreq != SrInfo.u32ClockFrequencyMhz  ||
		 m_pCardState->AddOnReg.u32Decimation != SrInfo.u32Decimation ||
		 m_pCardState->AddOnReg.u16ClkSelect != u16ClkSelect ||
		 m_pCardState->AddOnReg.u16MsSetup != u16MsSetup ) 
	{
		int32 i32Ret = SetClockThruNios( SrInfo.u32ClockFrequencyMhz );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = WriteRegisterThruNios(u16DecimationLow, CSPDR_DEC_FACTOR_LOW_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = WriteRegisterThruNios(u16DecimationHigh, CSPDR_DEC_FACTOR_HIGH_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = WriteRegisterThruNios(u16ClkSelect & ~CSPRD_CLK_OUT_ENABLE, CSPDR_CLK_SET_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		// Set clock and reset clock out
		i32Ret = WriteRegisterThruNios(u16ClkSelect, CSPDR_CLK_SET_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		BBtiming(100);

		u16ClkSelect |= m_pCardState->AddOnReg.u16ClkOut;
		i32Ret = WriteRegisterThruNios(u16ClkSelect, CSPDR_CLK_SET_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		if ( m_bNucleon )
		{
			i32Ret = WriteRegisterThruNios(u16MsSetup, CSPDR_MS_SETUP_WD);
			if( CS_FAILED(i32Ret) )	return i32Ret;

			m_pCardState->AddOnReg.u16MsSetup = u16MsSetup;

			// Clock source changed. Need a M/S reset. Otherwise there will be a skew beween Master and slave cards
			AddonReset();
			if ( ermMaster == m_pCardState->ermRoleMode )
				m_bMsResetNeeded = true;
		}

		m_pCardState->AddOnReg.u32ClkFreq = SrInfo.u32ClockFrequencyMhz;
		m_pCardState->AddOnReg.u32Decimation = SrInfo.u32Decimation;
		m_pCardState->AddOnReg.u16ClkSelect = u16ClkSelect;

		m_pCardState->AcqConfig.i64SampleRate = SrInfo.llSampleRate;
		m_pCardState->AcqConfig.u32ExtClk = u32ExtClk;
		m_pCardState->AcqConfig.u32ExtClkSampleSkip = u32ExtClkSampleSkip;
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

int32	CsSpiderDev::SendCaptureModeSetting(uInt32 u32Mode)
{
	uInt16	u16ChanSelect = CSPDR_OCT_CHAN;
	bool	bResetDefault =	((uInt32) CSPDR_DEFAULT_MODE == u32Mode );

	if ( bResetDefault )
	{
		// Default acquisition mode depending on number of channels
		switch( m_pCardState->PlxData.CsBoard.NbAnalogChannel )
		{
			case 8:
				u32Mode = CS_MODE_OCT;
				break;
			case 4:
				u32Mode = CS_MODE_QUAD;
				break;
			case 2:
				u32Mode = CS_MODE_DUAL;
				break;
			case 1:
				u32Mode = CS_MODE_SINGLE;
				break;
		}
	}
	else
	{
		// Validattion of acquisition mode
		switch( m_pCardState->PlxData.CsBoard.NbAnalogChannel )
		{
			case 4:
				if ( u32Mode & CS_MODE_OCT )
					return CS_INVALID_ACQ_MODE;
				break;
			case 2:
				if ( u32Mode & (CS_MODE_OCT | CS_MODE_QUAD) )
					return CS_INVALID_ACQ_MODE;
				break;
			case 1:
				if ( u32Mode & (CS_MODE_OCT | CS_MODE_QUAD | CS_MODE_DUAL) )
					return CS_INVALID_ACQ_MODE;
				break;
		}
	}

	if ( u32Mode & CS_MODE_OCT )
		u16ChanSelect = CSPDR_OCT_CHAN;
	else if ( u32Mode & CS_MODE_QUAD )
		u16ChanSelect = CSPDR_QUAD_CHAN;
	else if ( u32Mode & CS_MODE_DUAL )
		u16ChanSelect = CSPDR_DUAL_CHAN;
	else if ( u32Mode & CS_MODE_SINGLE )
		u16ChanSelect = CSPDR_SINGLE_CHAN;

	m_pCardState->AcqConfig.u32Mode = u32Mode;

	int32 i32Ret = CS_SUCCESS;
	if ( bResetDefault || m_pCardState->AddOnReg.u16ChanSelect != u16ChanSelect )
	{
		i32Ret = WriteRegisterThruNios(u16ChanSelect, CSPDR_DEC_CTL_WR_CMD);
		m_pCardState->AddOnReg.u16ChanSelect = u16ChanSelect;
		
		AddonReset();
	}

	return i32Ret;
}

//-----------------------------------------------------------------------------
//----- WriteToSelfTestRegister
//----- ADD-ON REGISTER
//-----------------------------------------------------------------------------
int32 CsSpiderDev::WriteToSelfTestRegister( uInt16 u16SelfTestMode )
{
	uInt16	u16Data = 0;

	switch( u16SelfTestMode )
	{
	case CSST_DISABLE:
		u16Data = 0;
		break;
	case CSST_CAPTURE_DATA:
		u16Data = CSPDR_SELF_CAPTURE;
		break;
	case CSST_BIST:
		u16Data = CSPDR_SELF_BIST;
		break;
	case CSST_COUNTER_DATA:
		u16Data = CSPDR_SELF_COUNTER;
		break;
	case CSST_MISC:
		u16Data = CSPDR_SELF_MISC;
		break;
	default:
		return CS_INVALID_SELF_TEST;
	}

	int32 i32Ret = WriteRegisterThruNios(u16Data, CSPDR_SELFTEST_MODE_WR_CMD);
	if( CS_FAILED(i32Ret) )
	{
		return i32Ret;
	}

	m_pCardState->AddOnReg.u16SelfTest = u16Data ;
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsSpiderDev::SendChannelSetting(uInt16 u16HwChannel, uInt32 u32InputRange, uInt32 u32Term, uInt32 u32Impendance, int32 i32Position, uInt32 u32Filter, int32 i32ForceCalib)
{
	int32 i32Ret = CS_SUCCESS;

	if( (0 == u16HwChannel) || (u16HwChannel > CSPDR_CHANNELS)  )
	{
		return CS_INVALID_CHANNEL;
	}
	uInt16 u16ChanZeroBased = u16HwChannel-1;
	uInt32 u32FrontIndex = 0;

	i32Ret = FindFeIndex( u32InputRange, u32Impendance, u32FrontIndex);

	if( CS_FAILED(i32Ret) )
	{
		return i32Ret;
	}

	uInt32 u32FrontEndSetup = 0;

	if ( CS_COUPLING_AC == (u32Term & CS_COUPLING_MASK) )
	{
		if ( 0 != (CSPDR_AC_COUPL_INST & m_pCardState->u32InstalledTerminations ) )
			u32FrontEndSetup |= CSPDR_SET_AC_COUPLING;
		else
			return CS_INVALID_COUPLING;
	}
	else if ( CS_COUPLING_DC == (u32Term & CS_COUPLING_MASK) )
	{
		if ( 0 != (CSPDR_DC_COUPL_INST & m_pCardState->u32InstalledTerminations ) )
			u32FrontEndSetup |= CSPDR_SET_DC_COUPLING;
		else
			return CS_INVALID_COUPLING;
	}
	else
	{
		return CS_INVALID_COUPLING;
	}

	if ( 0 == u32Filter && !m_pCardState->bZap )
	{
		u32FrontEndSetup |= CSPDR_CLR_CHAN_FILTER;
	}
	else
	{
		if( m_pCardState->u32LowCutoffFreq[1] != CS_NO_FILTER )
			u32FrontEndSetup |= CSPDR_SET_CHAN_FILTER;
		else
			return CS_INVALID_FILTER;
	}

	if( m_pCardState->bInCal[u16ChanZeroBased] )
	{
		u32FrontEndSetup &= ~CSPDR_CLR_CHAN_CAL;
		u32FrontEndSetup |=  CSPDR_SET_CHAN_CAL;

		u32FrontEndSetup &= ~CSPDR_SET_AC_COUPLING;
		u32FrontEndSetup |=  CSPDR_SET_DC_COUPLING;
	}
	else
	{
		u32FrontEndSetup &= ~CSPDR_SET_CHAN_CAL;
		u32FrontEndSetup |=  CSPDR_CLR_CHAN_CAL;
	}

	if( i32ForceCalib < 0 )
	{
		m_u32IgnoreCalibErrorMaskLocal |= 1 << u16ChanZeroBased;
	}
	else if( i32ForceCalib > 0 )
	{
		m_pCardState->bCalNeeded[u16ChanZeroBased] = true;
		m_bForceCal[u16ChanZeroBased] = true;
		i32Ret = SendAdcOffsetAjust(u16HwChannel, m_pCardState->u16AdcOffsetAdjust );
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}

	if ( ( m_pCardState->AddOnReg.u32FeIndex[u16ChanZeroBased] != u32FrontIndex ||
		  m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] != u32FrontEndSetup ||
		  m_pCardState->AddOnReg.u32Impendance[u16ChanZeroBased] != u32Impendance ) )
	{

		i32Ret = ConfigureFe( u16HwChannel, u32FrontIndex, u32Impendance, u32FrontEndSetup);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		m_pCardState->bCalNeeded[u16ChanZeroBased] = true;

		m_pCardState->AddOnReg.u32FeIndex[u16ChanZeroBased] = u32FrontIndex;
		m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] = u32FrontEndSetup;
		m_pCardState->AddOnReg.u32Impendance[u16ChanZeroBased] = u32Impendance;

		m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange = u32InputRange;
		m_pCardState->ChannelParams[u16ChanZeroBased].u32Term = u32Term & CS_COUPLING_MASK;
		m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance = u32Impendance;
		m_pCardState->ChannelParams[u16ChanZeroBased].u32Filter = u32Filter;
	}

	i32Ret = SetPosition(u16HwChannel, i32Position);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSpiderDev::SendSegmentSetting(uInt32 u32SegmentCount, int64 i64PostDepth, int64 i64SegmentSize , int64 i64HoldOff, int64 i64TrigDelay )
{
	uInt8	u8Shift = 0;
	uInt32	u32Mode = m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE;

	AbortAcquisition(); //----- Abort the previous acquisition, i.e. reset the counters, it's weird but should be like that

	switch(u32Mode)
	{
	case CS_MODE_OCT:
		u8Shift = 0;
		break;
	case CS_MODE_QUAD:
		u8Shift = 1;
		break;
	case CS_MODE_DUAL:
		u8Shift = 2;
		break;
	case CS_MODE_SINGLE:
		u8Shift = 3;
		break;
	}

	m_Stream.bInfiniteDepth = i64SegmentSize == -1;		// Infinite 

	// From samples, we have to convert to memory location then send this value to firmware
	// Because the firmware only support 30 bit register depth, we have to validate if the depth value exeeds 30 bit wide
	if ( ! m_Stream.bInfiniteDepth )
	{
		if ( (i64SegmentSize >> u8Shift) >= MAX_HW_DEPTH_COUNTER  )
			return CS_SEGMENTSIZE_TOO_BIG;
		if ( (i64PostDepth >> u8Shift) >= MAX_HW_DEPTH_COUNTER  ) 
			return CS_INVALID_TRIG_DEPTH;
	}

	if ( (i64HoldOff >> u8Shift) >= MAX_HW_DEPTH_COUNTER ) 
		return CS_INVALID_TRIGHOLDOFF;
	if ( (i64TrigDelay >> u8Shift) >= MAX_HW_DEPTH_COUNTER )
		return CS_INVALID_TRIGDELAY;


	m_u32HwSegmentCount		= u32SegmentCount;
	m_i64HwSegmentSize		= i64SegmentSize;
	m_i64HwPostDepth		= i64PostDepth;
	m_i64HwTriggerHoldoff	= i64HoldOff;

	// Adjust Segemnt Parameters for Hardware
	AdjustedHwSegmentParameters( m_pCardState->AcqConfig.u32Mode, &m_u32HwSegmentCount, &m_i64HwSegmentSize, &m_i64HwPostDepth );

	uInt32	u32SegmentSize	= (uInt32) (m_i64HwSegmentSize >> u8Shift);
	uInt32	u32PostDepth	= (uInt32) (m_i64HwPostDepth >> u8Shift);
	uInt32	u32HoldOffDepth = (uInt32) (i64HoldOff >> u8Shift);
	uInt32	u32TrigDelay    = (uInt32) (i64TrigDelay >> u8Shift);

	int32 i32Ret = CS_SUCCESS;
	if ( m_bMulrecAvgTD )
	{
		// Interchange Segment count and Mulrec Acg count
		// From firmware point of view, the SegmentCount is number of averaging
		// and the number of averaging is segment count
		i32Ret = WriteRegisterThruNios( u32SegmentCount, CSPLXBASE_SET_MULREC_AVG_COUNT );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
		u32SegmentCount = m_MasterIndependent->m_u32MulrecAvgCount;
	}

	i32Ret = WriteRegisterThruNios( u32SegmentSize, CSPLXBASE_SET_SEG_LENGTH_CMD );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret = WriteRegisterThruNios( m_Stream.bInfiniteDepth ? -1 : u32PostDepth,	CSPLXBASE_SET_SEG_POST_TRIG_CMD	);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret = WriteRegisterThruNios( ( m_Stream.bEnabled && ! m_bCalibActive ) ? -1 : u32SegmentCount, CSPLXBASE_SET_SEG_NUMBER_CMD );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	if ( m_bNucleon )
	{
		// Square record
		uInt32 u32SqrPreTrig		= u32SegmentSize - u32PostDepth;
		uInt32 u32SqrTrigDelay		= u32TrigDelay;
		uInt32 u32SqrHoldOffDepth	= 0;

		if ( u32HoldOffDepth >  u32SqrPreTrig )
			u32SqrHoldOffDepth	= u32HoldOffDepth - u32SqrPreTrig;

		i32Ret = WriteRegisterThruNios( u32SqrPreTrig, CSPLXBASE_SET_SEG_PRE_TRIG_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteRegisterThruNios( u32SqrHoldOffDepth, CSPLXBASE_SET_SQUARE_THOLDOFF_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteRegisterThruNios( u32SqrTrigDelay, CSPLXBASE_SET_SQUARE_TDELAY_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}
	else
	{
		i32Ret = WriteRegisterThruNios( u32HoldOffDepth, CSPLXBASE_SET_SEG_PRE_TRIG_CMD );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	if ( m_bMulrecAvgTD )
	{
		i32Ret = WriteRegisterThruNios( u32TrigDelay, CSPLXBASE_SET_MULREC_AVG_DELAY );
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}

	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
BOOLEAN	CsSpiderDev::FindSrInfo( const LONGLONG llSr, SPIDER_SAMPLE_RATE_INFO *pSrInfo )
{
	if( -1 == llSr && 0 != m_pCardState->u32SrInfoSize )
	{
		*pSrInfo = m_pCardState->SrInfo[0];
		return true;
	}

	for( unsigned i = 0; i < m_pCardState->u32SrInfoSize; i++ )
	{
		if ( llSr == m_pCardState->SrInfo[i].llSampleRate )
		{
			*pSrInfo  = m_pCardState->SrInfo[i];
			return TRUE;
		}
	}

	return FALSE;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSpiderDev::SendExtTriggerSetting( BOOLEAN  bExtTrigEnabled,
										   eTrigEnableMode eteSet,
										   int32	i32ExtLevel,
										   uInt32	u32ExtCondition,
										   uInt32	u32ExtTriggerRange,
										   uInt32	u32ExtCoupling )
{
	uInt32	u32Data = 0;
	int32	i32Ret;


	if  ( ( CS_GAIN_2_V != u32ExtTriggerRange ) &&
			( CS_GAIN_10_V != u32ExtTriggerRange ) )
		return CS_INVALID_EXT_TRIG;

	if  ( (CS_COUPLING_DC != u32ExtCoupling) &&
			(CS_COUPLING_AC != u32ExtCoupling) )
		return CS_INVALID_COUPLING;

	m_pCardState->TriggerParams[CSPDR_TRIGENGINE_EXT].i32Source				= bExtTrigEnabled ? -1 : 0;
	m_pCardState->TriggerParams[CSPDR_TRIGENGINE_EXT].u32ExtTriggerRange	= u32ExtTriggerRange;
	m_pCardState->TriggerParams[CSPDR_TRIGENGINE_EXT].u32ExtCoupling		= u32ExtCoupling;
	m_pCardState->TriggerParams[CSPDR_TRIGENGINE_EXT].u32ExtImpedance		= CSPDR_DEFAULT_EXT_IMPEDANCE;
	m_pCardState->TriggerParams[CSPDR_TRIGENGINE_EXT].u32Condition			= u32ExtCondition;
	m_pCardState->TriggerParams[CSPDR_TRIGENGINE_EXT].i32Level				= i32ExtLevel;

	// Enabled all internal Trigger engines
	u32Data |= CSPDR_SET_GBLTRIG_ENABLED;

	if ( m_pCardState->bSpiderLP )
	{
		if ( m_TrigOutCfg.bClockPolarity && (1 == m_pCardState->AddOnReg.u32Decimation) )
			u32Data |= CSPDR_SET_TRIGOUT_CLKPHASE;

		i32Ret = SendTriggerDelay();
		if( CS_FAILED(i32Ret) )
		{
			return i32Ret;
		}
	}

	if ( m_pCardState->bSpiderV12 && ! m_pCardState->bDisableTrigOut )
		u32Data |= CSPDR_SET_DIG_TRIG_TRIGOUTENABLED;

	switch(eteSet)
	{
		default:
		case eteAlways:      u32Data |= CSPDR_SET_DIG_TRIG_EXT_EN_OFF;
						     break;
		case eteExtLive:     u32Data |= CSPDR_SET_DIG_TRIG_EXT_EN_LIVE;
						     break;
		case eteExtPosLatch: u32Data |= CSPDR_SET_DIG_TRIG_EXT_EN_POS;
						     break;
		case eteExtNegLatch: u32Data |= CSPDR_SET_DIG_TRIG_EXT_EN_NEG;
						     break;
	}

	i32Ret = WriteRegisterThruNios(u32Data, CSPDR_TRIG_SET_WR_CMD);
	if( CS_FAILED(i32Ret) )
	{
		return i32Ret;
	}

	m_pCardState->AddOnReg.u16TrigEnable = (uInt16) u32Data;


	u32Data = 0;

	if ( bExtTrigEnabled )
		u32Data |= CSPDR_SET_EXT_TRIG_ENABLED;

	if (  CS_TRIG_COND_POS_SLOPE == u32ExtCondition )
		u32Data |= CSPDR_SET_EXT_TRIG_POS_SLOPE;
	if ( CS_GAIN_2_V == u32ExtTriggerRange )
		u32Data |= CSPDR_SET_EXT_TRIG_EXT1V;
	if ( CS_COUPLING_DC == u32ExtCoupling )
		u32Data |=  CSPDR_SET_EXT_TRIG_DC;

	i32Ret = WriteRegisterThruNios(u32Data, CSPDR_EXT_TRIG_SET_WR_CMD);
	if( CS_FAILED(i32Ret) )
	{
		return i32Ret;
	}

	// QQQ
	i32Ret = SendExtTriggerLevel( i32ExtLevel, u32ExtCondition, u32ExtTriggerRange);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//----- SendTriggerEngineSetting
//----- ADD-ON REGISTER
//----- Address CS12400_TRIG_ENG_1_ADD, CS12400_TRIG_ENG_2_ADD, CS12400_EXT_TRIG_ADD
//-----------------------------------------------------------------------------
int32 CsSpiderDev::SendTriggerEngineSetting(uInt16	  u16ChannelTrigEngine,
											  int32	  i32SourceA,
											  uInt32  u32ConditionA,
											  int32   i32LevelA,
											  int32	  i32SourceB,
											  uInt32  u32ConditionB,
											  int32   i32LevelB )
{
	uInt16	u16Data = 0;
	uInt32	u32Command = CSPDR_DTS1_SET_WR_CMD;
	uInt16	u16TrigEngine = 1 + 2*(u16ChannelTrigEngine-1);

	if ( u16TrigEngine == 0 )
		return CS_INVALID_TRIGGER;

	// These inputs should have been already validated from user DLL level
	// Double check ..
	if ( ( i32LevelA > 100  || i32LevelA < -100 ) ||
		 ( i32LevelB > 100  || i32LevelB < -100 ) )
		return CS_INVALID_TRIG_LEVEL;

	if ( i32SourceA > m_pCardState->PlxData.CsBoard.NbAnalogChannel || i32SourceB > m_pCardState->PlxData.CsBoard.NbAnalogChannel )
		return CS_INVALID_TRIG_SOURCE;

	m_pCardState->TriggerParams[u16TrigEngine].i32Source    = i32SourceA;
	m_pCardState->TriggerParams[u16TrigEngine].u32Condition = u32ConditionA;
	m_pCardState->TriggerParams[u16TrigEngine].i32Level     = i32LevelA;

	if ( i32SourceA != 0 )
		u16Data |= CSPDR_SET_DIG_TRIG_A_ENABLE;

	if ( u32ConditionA == CS_TRIG_COND_NEG_SLOPE )
		u16Data |= CSPDR_SET_DIG_TRIG_A_NEG_SLOPE;

	m_pCardState->TriggerParams[u16TrigEngine+1].i32Source    = i32SourceB;
	m_pCardState->TriggerParams[u16TrigEngine+1].u32Condition = u32ConditionB;
	m_pCardState->TriggerParams[u16TrigEngine+1].i32Level     = i32LevelB;

	if ( i32SourceB != 0 )
		u16Data |= CSPDR_SET_DIG_TRIG_B_ENABLE;

	if ( u32ConditionB == CS_TRIG_COND_NEG_SLOPE )
		u16Data |= CSPDR_SET_DIG_TRIG_B_NEG_SLOPE;


	u32Command = CSPDR_DTS1_SET_WR_CMD + (u16ChannelTrigEngine - 1) * 0x100;

	int32 i32Ret = CS_SUCCESS;

	i32Ret = WriteRegisterThruNios(u16Data, u32Command);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret =  SendDigitalTriggerLevel( u16TrigEngine, TRUE, i32LevelA );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret =  SendDigitalTriggerLevel( u16TrigEngine, FALSE, i32LevelB );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret =  SendDigitalTriggerSensitivity( u16TrigEngine, TRUE, m_u16TrigSensitivity );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret =  SendDigitalTriggerSensitivity( u16TrigEngine, FALSE, m_u16TrigSensitivity );
	if( CS_FAILED(i32Ret) )
		return i32Ret;


	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSpiderDev::SendDigitalTriggerLevel( uInt16 u16TrigEngine, BOOLEAN bA, int32 i32Level )
{
	uInt16	u16Data = 0;
	int32	i32LevelConverted = i32Level;
	uInt32	u32Command;


	//----- Convert from (-100, 99) range to ADC range
	int32  i32ADCRange = 2 * CSPDR_DIG_TRIGLEVEL_SPAN;
	i32LevelConverted *= i32ADCRange;
	i32LevelConverted /= 200L;

	if( 2*CSPDR_DIG_TRIGLEVEL_SPAN <= i32LevelConverted )
		i32LevelConverted = 2* CSPDR_DIG_TRIGLEVEL_SPAN - 1;
	else if( -2*CSPDR_DIG_TRIGLEVEL_SPAN > i32LevelConverted )
		i32LevelConverted = -2*CSPDR_DIG_TRIGLEVEL_SPAN;

	int16 i16LevelDAC = (int16)i32LevelConverted;

	if( m_pCardState->bZap )
		i16LevelDAC *= 8;

	u16Data |= i16LevelDAC;

	if ( bA )
		u32Command = CSPDR_DL_1A_SET_WR_CMD + (u16TrigEngine - 1) * 0x200;
	else
		u32Command = CSPDR_DL_1B_SET_WR_CMD + (u16TrigEngine - 1) * 0x200;

	return WriteRegisterThruNios(u16Data, u32Command);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSpiderDev::SendDigitalTriggerSensitivity( uInt16 u16TrigEngine, BOOLEAN bA, uInt16 u16Sensitivity )
{
	uInt32	u32Command;

	if( m_pCardState->bZap )
		u16Sensitivity *= 4;

	if ( bA )
		u32Command = CSPDR_DS_1A_SET_WR_CMD + (u16TrigEngine - 1) * 0x200;
	else
		u32Command = CSPDR_DS_1B_SET_WR_CMD + (u16TrigEngine - 1) * 0x200;

	return WriteRegisterThruNios(u16Sensitivity, u32Command);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSpiderDev::SendExtTriggerLevel( int32 i32Level,  uInt32 u32ExtCondition,  uInt32 u32ExtTriggerRange)
{
	int32	i32Ret = CS_SUCCESS;
	int32	i32LevelConverted = i32Level;
	BOOL	bInvert = TRUE;

	if ( bInvert )
	{
		// The trigger level is inverted
		//----- Convert from (-100, 99) to (0, 199) range
		i32LevelConverted = 100L - i32LevelConverted;
	}
	else
	{
		//----- Convert from (-100, 99) to (0, 199) range
		i32LevelConverted = i32LevelConverted + 100L;
	}

	//----- Convert from (0, 199) range to (0, 4096) range
	//----- This is the range that 12 bit DAC supports
	i32LevelConverted *= 2*CSPDR_DAC_SPAN;
	i32LevelConverted /= 200L;

	// Adustment for Gain
	i32LevelConverted -= CSPDR_DAC_SPAN;
	i32LevelConverted *= CSPDR_CAL_TRIGGER_GAIN_FACTOR; //GetTriggerGainAdjust( u16TrigEngine );
	i32LevelConverted /= CSPDR_CAL_TRIGGER_GAIN_FACTOR;
	i32LevelConverted += CSPDR_DAC_SPAN;

	if( 2*CSPDR_DAC_SPAN <= i32LevelConverted )
		i32LevelConverted = 2*CSPDR_DAC_SPAN - 1;
	else if( 0 > i32LevelConverted )
		i32LevelConverted = 0;

	int16 i16LevelDAC = (int16)i32LevelConverted;

	i32Ret = WriteToCalibDac( CSPDR_EXTRIG_LEVEL, i16LevelDAC );

	// Default value, while there is no calibration for ext trigger

	if (CS_TRIG_COND_POS_SLOPE == u32ExtCondition)
	{
		if (CS_GAIN_2_V == u32ExtTriggerRange)
		{
			i16LevelDAC= 2900; //2120;
		}
		else
		{
			// CS_GAIN_10_V
			i16LevelDAC= 2800; //2120;
		}
	}
	else
	{
		 // Negative slope
		if (CS_GAIN_2_V == u32ExtTriggerRange)
		{
			i16LevelDAC= 1500; //2120;
		}
		else
		{
			// CS_GAIN_10_V
			i16LevelDAC= 1300; //2120;
		}
	}


	i32Ret = WriteToCalibDac( CSPDR_EXTRIG_POSITION, i16LevelDAC );


	return i32Ret;

}

//-----------------------------------------------------------------------------
//-----	SendCalibModeSetting
//----- ADD-ON REGISTER
//-----------------------------------------------------------------------------
int32 CsSpiderDev::SendCalibModeSetting(const uInt16 u16Channel, const BOOL bSet)
{
	int32 i32Ret = CS_SUCCESS;

	if( (0 == u16Channel) || (u16Channel > CSPDR_CHANNELS)  )
	{
		return CS_INVALID_CHANNEL;
	}
	uInt16 u16ChanZeroBased = u16Channel-1;

	if( bSet != m_pCardState->bInCal[u16ChanZeroBased] )
	{
		uInt32 u32FrontEndSetup = m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased];
		if( bSet )
		{
			u32FrontEndSetup &= ~CSPDR_CLR_CHAN_CAL;
			u32FrontEndSetup |=  CSPDR_SET_CHAN_CAL;

			u32FrontEndSetup &= ~CSPDR_SET_AC_COUPLING;
			u32FrontEndSetup |=  CSPDR_SET_DC_COUPLING;
		}
		else
		{
			u32FrontEndSetup &= ~CSPDR_SET_CHAN_CAL;
			u32FrontEndSetup |=  CSPDR_CLR_CHAN_CAL;
		}
		m_pCardState->bInCal[u16ChanZeroBased] = (BOOLEAN)bSet;

		i32Ret = ConfigureFe( u16Channel, m_pCardState->AddOnReg.u32FeIndex[u16ChanZeroBased], m_pCardState->AddOnReg.u32Impendance[u16ChanZeroBased], u32FrontEndSetup);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSpiderDev::WriteCalibTableToEeprom(uInt8	u8CalibTableId, uInt32 u32Valid )
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32Addr;
	uInt32	u32EepromSize;


	i32Ret = UpdateCalibInfoStrucure();
	if ( CS_FAILED(i32Ret) )
		return i32Ret;

	switch ( u8CalibTableId )
	{
		case 3:
			u32Addr = FIELD_OFFSET( AddOnFlash32Mbit, Data );
			u32Addr += FIELD_OFFSET( AddOnFlashData, Calib3 );
			u32EepromSize = sizeof((((AddOnFlashData *)0)->Calib3));
			break;
		case 2:
			u32Addr = FIELD_OFFSET( AddOnFlash32Mbit, Data );
			u32Addr += FIELD_OFFSET( AddOnFlashData, Calib2 );
			u32EepromSize = sizeof((((AddOnFlashData *)0)->Calib2));
			break;
		default:
			u32Addr = FIELD_OFFSET( AddOnFlash32Mbit, Data );
			u32Addr += FIELD_OFFSET( AddOnFlashData, Calib1 );
			u32EepromSize = sizeof((((AddOnFlashData *)0)->Calib1));
			break;
	}

	// Because the calibration table is eventually very big
	// and the stack from kernel is very limited.
	// Do not create any local variable like:  CS14160_CAL_INFO  CalInfo;
	// because this way may result stack overlow
	// Use member variables instead ....

	uInt32 u32CalibTableSize = sizeof(m_CalibInfoTable);

	if ( u32EepromSize < u32CalibTableSize )
	{
		// Should not happen but playing on safe side ...
		return CS_FAIL;
	}

	//Clear valid bits if invalid were set
	uInt32 u32InvalidMask = m_CalibInfoTable.u32Valid & (CSPDR_VALID_MASK >> 1 );
	m_CalibInfoTable.u32Valid &= ~(u32InvalidMask << 1);

	//clear valid bits
	m_CalibInfoTable.u32Valid &= (u32Valid << 1) & CSPDR_VALID_MASK;

	//set valid bits
	m_CalibInfoTable.u32Valid |= u32Valid & CSPDR_VALID_MASK;

	return WriteSectorsOfEeprom( &m_CalibInfoTable, u32Addr, sizeof(m_CalibInfoTable) );
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSpiderDev::ReadCalibTableFromEeprom(uInt8	u8CalibTableId)
{

	UNREFERENCED_PARAMETER(u8CalibTableId);
#if 0
	uInt32	u32Addr;
	uInt32	u32EepromSize;


	switch ( u8CalibTableId )
	{
		case 3:
			u32Addr = FIELD_OFFSET( AddOnFlash32Mbit, Data );
			u32Addr += FIELD_OFFSET( AddOnFlashData, Calib3 );
			u32EepromSize = sizeof((((AddOnFlashData *)0)->Calib3));
			break;
		case 2:
			u32Addr = FIELD_OFFSET( AddOnFlash32Mbit, Data );
			u32Addr += FIELD_OFFSET( AddOnFlashData, Calib2 );
			u32EepromSize = sizeof((((AddOnFlashData *)0)->Calib2));
			break;
		default:
			u32Addr = FIELD_OFFSET( AddOnFlash32Mbit, Data );
			u32Addr += FIELD_OFFSET( AddOnFlashData, Calib1 );
			u32EepromSize = sizeof((((AddOnFlashData *)0)->Calib1));
			break;
	}


	// Because the calibration table is eventually very big
	// and the stack from kernel is very limited.
	// Do not create any local variable like:  CSPDR_CAL_INFO  CalInfo;
	// because this way may result stack overlow
	// Use member variables instead ....


	uInt32 u32CalibTableSize = sizeof(m_CalibInfoTable);

	if ( u32EepromSize < u32CalibTableSize )
	{
		// Should not happen but playing on safe side ...
		return CS_FAIL;
	}


	InitFlash(0);
	AddonResetFlash();

	uInt8	*pPtr = (uInt8 *)(&m_CalibInfoTable);
	for( uInt32 u32 = 0; u32 < u32CalibTableSize ; u32++)
	{
		*pPtr++ = (uInt8) AddonReadFlashByte( u32Addr++ );
	}
#endif

	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsSpiderDev::GetFEIndex(uInt16 u16Channel)
{
	int32 i32(0);

	uInt16 u16ChanZeroBased = u16Channel-1;
	if( (0 == u16Channel) || (u16Channel > CSPDR_CHANNELS)  )
	{
		return i32;
	}

	if( 0 != (m_pCardState->ChannelParams[u16ChanZeroBased].u32Term & CS_COUPLING_AC) )
	{
		i32 |= CSPDR_AC_COUPLING_INDEX;
	}

	if( 0 != m_pCardState->ChannelParams[u16ChanZeroBased].u32Filter  )
	{
		i32 |= CSPDR_FILTER_INDEX;
	}

	if( CS_REAL_IMP_50_OHM == m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance  )
	{
		i32 |= CSPDR_IMPEDANCE_50_INDEX;
	}

	return i32;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsSpiderDev::InitHwTables()
{
	uInt32	u32Value;		// Data read from Nios
	uInt32	u32Value1;		// Data read from Nios

	RtlZeroMemory( m_pCardState->u32SwingTableSize, sizeof(m_pCardState->u32SwingTableSize) );

	if ( ! m_pCardState->PlxData.InitStatus.Info.u32Nios )
	{
		// Nios is not initialized.
		// Assume that it is a spider 14bits 8 channels, 125MHz
		// This is just for CsHardwareConfig ....
		m_pCardState->PlxData.CsBoard.NbAnalogChannel = 8;
		m_pCardState->PlxData.CsBoard.SampleRateLimit = 9;

		m_pCardState->AcqConfig.u32SampleBits = CS_SAMPLE_BITS_14;
		m_pCardState->AcqConfig.i32SampleOffset = CS_SAMPLE_OFFSET_14_LJ;

		return CS_SUCCESS;
	}

	// Check if the card has HW limitation table
	// Attemp to read Low Imped and High Imped ranges
	// Boards with old firmware or
	// Boards new firmware but no limitation table will return garbage
	// Consider ranges > +-100V is invalid
	u32Value = ReadRegisterFromNios( 0, CSPLXBASE_READ_RANGES_LOW_IMPED );
	u32Value1 = ReadRegisterFromNios( 0, CSPLXBASE_READ_RANGES_HIGH_IMPED );

	if ( ((u32Value == 0) && (u32Value1 == 0)) ||
		(u32Value > 200000 || u32Value1 > 200000 ) )
	{
		// Limitation table is not present
		m_bInvalidHwConfiguration = TRUE;
		return	CS_MISC_ERROR;
	}


	if ( u32Value != 0 )
	{
		// Low Imdedance supported
		for( int i = 0; i < CSPDR_MAX_RANGES; i++ )
		{
			u32Value = ReadRegisterFromNios( i, CSPLXBASE_READ_RANGES_LOW_IMPED );

			if ( u32Value != 0 )
			{
				m_pCardState->u32SwingTableSize[0]++;
				m_pCardState->u32SwingTable[0][i] = u32Value;
			}
			else
			{
				break;
			}
		}
	}

	if ( u32Value1 != 0 )
	{
		// High Imdedance supported
		for( int i = 0; i < CSPDR_MAX_RANGES; i++ )
		{
			u32Value = ReadRegisterFromNios( i, CSPLXBASE_READ_RANGES_HIGH_IMPED );

			if ( u32Value != 0 )
			{
				m_pCardState->u32SwingTableSize[1]++;
				m_pCardState->u32SwingTable[1][i] = u32Value;
			}
			else
			{
				break;
			}
		}
	}

	m_pCardState->u32InstalledTerminations = ReadRegisterFromNios( SPDRCAPS_TERMINATION, CSPLXBASE_READ_HW_ALLOWS_CAPS );
	m_pCardState->u32InstalledMisc = CSPDR_EXT_5V_INST | CSPDR_EXT_1V_INST;

	m_pCardState->u32LowCutoffFreq[0] = ReadRegisterFromNios( SPDRCAPS_FLTR1_LOWCUTOFF, CSPLXBASE_READ_HW_ALLOWS_CAPS );
	m_pCardState->u32LowCutoffFreq[1] = ReadRegisterFromNios( SPDRCAPS_FLTR2_LOWCUTOFF, CSPLXBASE_READ_HW_ALLOWS_CAPS );

	m_pCardState->u32HighCutoffFreq[0] = ReadRegisterFromNios( SPDRCAPS_FLTR1_HIGHCUTOFF, CSPLXBASE_READ_HW_ALLOWS_CAPS );
	m_pCardState->u32HighCutoffFreq[1] = ReadRegisterFromNios( SPDRCAPS_FLTR2_HIGHCUTOFF, CSPLXBASE_READ_HW_ALLOWS_CAPS );

	m_pCardState->PlxData.CsBoard.NbAnalogChannel = (uInt8) ReadRegisterFromNios( SPDRCAPS_CHANNELNUM, CSPLXBASE_READ_HW_ALLOWS_CAPS );

	// Determine sample bit base on Data mask
	m_pCardState->u32DataMask = ReadRegisterFromNios( SPDRCAPS_ANALOG_DATAMASK, CSPLXBASE_READ_HW_ALLOWS_CAPS );

	CsSetDataFormat();

	// Limit of physical memory in bytes
	UpdateMaxMemorySize( m_pCardState->PlxData.CsBoard.u32BaseBoardHardwareOptions, CSPDR_DEFAULT_SAMPLE_SIZE );

	u32Value = ReadRegisterFromNios( SPDRCAPS_MAX_CLOCK, CSPLXBASE_READ_HW_ALLOWS_CAPS );

	if( m_pCardState->bZap )
		u32Value /= CSPDR_ZAP_AVG_SIZE;

	SPIDER_SAMPLE_RATE_INFO  *SrInfo = m_pCardState->SrInfo;
	switch( u32Value )
	{
		case 125:
				m_pCardState->PlxData.CsBoard.SampleRateLimit = 9;
				m_pCardState->u32SrInfoSize = c_SPDR_SR_INFO_SIZE_9;
				::GageCopyMemoryX( SrInfo, c_SPDR_SR_INFO_9, m_pCardState->u32SrInfoSize * sizeof(SPIDER_SAMPLE_RATE_INFO));
				break;
		case 105:
				m_pCardState->PlxData.CsBoard.SampleRateLimit = 8;
				m_pCardState->u32SrInfoSize = c_SPDR_SR_INFO_SIZE_8;
				::GageCopyMemoryX( SrInfo, c_SPDR_SR_INFO_8, m_pCardState->u32SrInfoSize * sizeof(SPIDER_SAMPLE_RATE_INFO));
				break;
		case 100:
				m_pCardState->PlxData.CsBoard.SampleRateLimit = 7;
				m_pCardState->u32SrInfoSize = c_SPDR_SR_INFO_SIZE_7;
				::GageCopyMemoryX( SrInfo, c_SPDR_SR_INFO_7, m_pCardState->u32SrInfoSize * sizeof(SPIDER_SAMPLE_RATE_INFO));
				break;
		case 80:
				m_pCardState->PlxData.CsBoard.SampleRateLimit = 6;
				m_pCardState->u32SrInfoSize = c_SPDR_SR_INFO_SIZE_6;
				::GageCopyMemoryX( SrInfo, c_SPDR_SR_INFO_6, m_pCardState->u32SrInfoSize * sizeof(SPIDER_SAMPLE_RATE_INFO));
				break;
		case 65:
				m_pCardState->PlxData.CsBoard.SampleRateLimit = 5;
				m_pCardState->u32SrInfoSize = c_SPDR_SR_INFO_SIZE_5;
				::GageCopyMemoryX( SrInfo, c_SPDR_SR_INFO_5, m_pCardState->u32SrInfoSize * sizeof(SPIDER_SAMPLE_RATE_INFO));
				break;
		case 50:
				m_pCardState->PlxData.CsBoard.SampleRateLimit = 4;
				m_pCardState->u32SrInfoSize = c_SPDR_SR_INFO_SIZE_4;
				::GageCopyMemoryX( SrInfo, c_SPDR_SR_INFO_4, m_pCardState->u32SrInfoSize * sizeof(SPIDER_SAMPLE_RATE_INFO));
				break;
		case 40:
				m_pCardState->PlxData.CsBoard.SampleRateLimit = 3;
				m_pCardState->u32SrInfoSize = c_SPDR_SR_INFO_SIZE_3;
				::GageCopyMemoryX( SrInfo, c_SPDR_SR_INFO_3, m_pCardState->u32SrInfoSize * sizeof(SPIDER_SAMPLE_RATE_INFO));
				break;
		case 25:
				m_pCardState->PlxData.CsBoard.SampleRateLimit = 2;
				m_pCardState->u32SrInfoSize = c_SPDR_SR_INFO_SIZE_2;
				::GageCopyMemoryX( SrInfo, c_SPDR_SR_INFO_2, m_pCardState->u32SrInfoSize * sizeof(SPIDER_SAMPLE_RATE_INFO));
				break;
		case 20:
				m_pCardState->PlxData.CsBoard.SampleRateLimit = 1;
				m_pCardState->u32SrInfoSize = c_SPDR_SR_INFO_SIZE_1;
				::GageCopyMemoryX( SrInfo, c_SPDR_SR_INFO_1, m_pCardState->u32SrInfoSize * sizeof(SPIDER_SAMPLE_RATE_INFO));
				break;
		case 10:
		default:
				m_pCardState->PlxData.CsBoard.SampleRateLimit = 0;
				m_pCardState->u32SrInfoSize = c_SPDR_SR_INFO_SIZE_0;
				::GageCopyMemoryX( SrInfo, c_SPDR_SR_INFO_0, m_pCardState->u32SrInfoSize * sizeof(SPIDER_SAMPLE_RATE_INFO));
				break;
	}

	if( m_pCardState->bZap )
	{
		for( uInt32 u = 0; u < m_pCardState->u32SrInfoSize; u++ )
			m_pCardState->SrInfo[u].u32ClockFrequencyMhz *= CSPDR_ZAP_AVG_SIZE;
	}

	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsSpiderDev::SetClockThruNios( uInt32 u32ClockMhz )
{

	int32 i32Ret = WriteRegisterThruNios(u32ClockMhz, CSPLXBASE_PLL_FREQUENCY_SET_CMD);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Read the return value from Nios
	uInt32	DataRead = ReadGageRegister(DATA_READ_FROM_NIOS);


	if( 0 != (u32ClockMhz & CSPDR_EXT_CLK_BYPASS) )
	{
		if ( u32ClockMhz != ( DataRead | CSPDR_EXT_CLK_BYPASS ) )
		{
			return CS_EXT_CLK_NOT_PRESENT;
		}
	}
	else if ( u32ClockMhz != DataRead )
	{
        return CS_INVALID_SAMPLE_RATE;
	}

	return CS_SUCCESS;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsSpiderDev::ConfigureFe( const uInt16 u16Channel, const uInt32 u32Index, const uInt32 u32Impedance, const uInt32 u32Unrestricted)
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32Reg;
	uInt8   u8Imped = (u32Impedance == 50) ? 1 : 0;

	u32Reg = (u32Unrestricted << 16) | (u8Imped << 14) | ((u32Index & 0xFF) << 4) | (u16Channel & 0xF);

	i32Ret = WriteRegisterThruNios(u32Reg, CSPLXBASE_SETFRONTEND_CMD);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSpiderDev::SendPosition(uInt16 u16HwChannel, int32 i32Position_mV)
{
	if( (0 == u16HwChannel) || (u16HwChannel > CSPDR_CHANNELS)  )
		return CS_INVALID_CHANNEL;

	if ( m_pCardState->bSpiderV12 && !m_bDisableUserOffet )
	{
		uInt16 u16ChanZeroBased = u16HwChannel-1;

		int32 i32FrontUnipolar_mV = m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange / 2;

		if( i32Position_mV > i32FrontUnipolar_mV ||
			i32Position_mV <-i32FrontUnipolar_mV )
		{
			return CS_INVALID_POSITION;
		}

		int32 i32Index = GetFEIndex(u16HwChannel);
		uInt32 u32RangeIndex = m_pCardState->AddOnReg.u32FeIndex[u16ChanZeroBased];

		uInt16 u16CodeDeltaForHalfIr = m_pCardState->DacInfo[u16ChanZeroBased][i32Index][u32RangeIndex].u16CodeDeltaForHalfIR;

		int32 i32PosCode = -(2* i32Position_mV * u16CodeDeltaForHalfIr)/i32FrontUnipolar_mV;
		i32PosCode += m_pCardState->DacInfo[u16ChanZeroBased][i32Index][u32RangeIndex].u16CoarseOffset;

		uInt16 u16Code = i32PosCode > 0 ? ( i32PosCode < int32(c_u16CalDacCount-1) ? uInt16(i32PosCode) : c_u16CalDacCount-1 ) : 0;
		uInt16 u16PositionDac  = GetControlDacId( ecdPosition, u16HwChannel );

		m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = i32Position_mV;

		return WriteToCalibDac(u16PositionDac, u16Code, m_u8CalDacSettleDelay);
	}
	else
		return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsSpiderDev::SetPosition(uInt16 u16Channel, int32 i32Position_mV)
{
	if( (0 == u16Channel) || (u16Channel > CSPDR_CHANNELS)  )
		return CS_INVALID_CHANNEL;

	if ( m_pCardState->bSpiderV12 && !m_bDisableUserOffet )
	{
		uInt16 u16ChanZeroBased = u16Channel - 1;

		if( m_pCardState->bCalNeeded[u16ChanZeroBased] )
		{
			//store position it will be set after calibration
			m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = i32Position_mV;
			return CS_SUCCESS;
		}
		else
			return SendPosition(u16Channel, i32Position_mV);
	}
	else
		return CS_SUCCESS;

}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	 CsSpiderDev::SendAdcOffsetAjust(uInt16 u16Channel, uInt16 u16Offet )
{
	if ( !m_pCardState->bSpiderV12 )
		return CS_FALSE;

	// For unknown reason, with STLP81 this function will cause signal clipped
	// before reach full scale on positive side
	// To be investigate ...
	if ( m_pCardState->bSpiderLP )
		return CS_FALSE;

	return WriteRegisterThruNios( u16Offet, CSPDR_ADC_DOC1 + ( (u16Channel- 1) << 8 ) );
}


//-----------------------------------------------------------------------------
//----- WriteToCalibDac
//----- ADD-ON REGISTER
//----- Write to the AD5516
//----- Address AD5516_XX
//----- u16Data: 12bits
//----- u8Delay: DefaultDelay is 32 us
//-----  Delay = DefaultDelay << u8Delay;
//-----------------------------------------------------------------------------
#define CSPDR_ADC_OFFSET_ADJUST 31

int32 CsSpiderDev::WriteToCalibDac(uInt16 u16Addr, uInt16 u16Data, uInt8 u8Delay)
{
	uInt32 u32DacAddr = u16Addr;
	uInt32 u32Data = u8Delay;
	u32Data <<= 16;
	u32Data |= u16Data;

	switch ( u16Addr )
	{
	case CSPDR_POSITION_1:
		u32DacAddr = (FENDA_OFFSET1_CS | DAC7552_OUTA);
		break;
	case CSPDR_POSITION_2:
		u32DacAddr = (FENDA_OFFSET2_CS | DAC7552_OUTA);
		break;
	case CSPDR_POSITION_3:
		u32DacAddr = (FENDA_OFFSET3_CS | DAC7552_OUTA);
		break;
	case CSPDR_POSITION_4:
		u32DacAddr = (FENDA_OFFSET4_CS | DAC7552_OUTA);
		break;
	case CSPDR_POSITION_5:
		u32DacAddr = (FENDB_OFFSET1_CS | DAC7552_OUTA);
		break;
	case CSPDR_POSITION_6:
		u32DacAddr = (FENDB_OFFSET2_CS | DAC7552_OUTA);
		break;
	case CSPDR_POSITION_7:
		u32DacAddr = (FENDB_OFFSET3_CS | DAC7552_OUTA);
		break;
	case CSPDR_POSITION_8:
		u32DacAddr = (FENDB_OFFSET4_CS | DAC7552_OUTA);
		break;
	case CSPDR_VCALFINE_1:
		u32DacAddr = (FENDA_OFFSET1_CS | DAC7552_OUTB);
		break;
	case CSPDR_VCALFINE_2:
		u32DacAddr = (FENDA_OFFSET2_CS | DAC7552_OUTB);
		break;
	case CSPDR_VCALFINE_3:
		u32DacAddr = (FENDA_OFFSET3_CS | DAC7552_OUTB);
		break;
	case CSPDR_VCALFINE_4:
		u32DacAddr = (FENDA_OFFSET4_CS | DAC7552_OUTB);
		break;
	case CSPDR_VCALFINE_5:
		u32DacAddr = (FENDB_OFFSET1_CS | DAC7552_OUTB);
		break;
	case CSPDR_VCALFINE_6:
		u32DacAddr = (FENDB_OFFSET2_CS | DAC7552_OUTB);
		break;
	case CSPDR_VCALFINE_7:
		u32DacAddr = (FENDB_OFFSET3_CS | DAC7552_OUTB);
		break;
	case CSPDR_VCALFINE_8:
		u32DacAddr = (FENDB_OFFSET4_CS | DAC7552_OUTB);
		break;
	case CSPDR_CAL_GAIN_1:
		u32DacAddr = (FENDA_GAIN12_CS | DAC7552_OUTA);
		break;
	case CSPDR_CAL_GAIN_2:
		u32DacAddr = (FENDA_GAIN12_CS | DAC7552_OUTB);
		break;
	case CSPDR_CAL_GAIN_3:
		u32DacAddr = (FENDA_GAIN34_CS | DAC7552_OUTA);
		break;
	case CSPDR_CAL_GAIN_4:
		u32DacAddr = (FENDA_GAIN34_CS | DAC7552_OUTB);
		break;
	case CSPDR_CAL_GAIN_5:
		u32DacAddr = (FENDB_GAIN56_CS | DAC7552_OUTA);
		break;
	case CSPDR_CAL_GAIN_6:
		u32DacAddr = (FENDB_GAIN56_CS | DAC7552_OUTB);
		break;
	case CSPDR_CAL_GAIN_7:
		u32DacAddr = (FENDB_GAIN78_CS | DAC7552_OUTA);
		break;
	case CSPDR_CAL_GAIN_8:
		u32DacAddr = (FENDB_GAIN78_CS | DAC7552_OUTB);
		break;
	case CSPDR_CAL_VA:
		u32DacAddr = (CAL_DACVARREF_CS | DAC7552_OUTA);
		break;
	case CSPDR_CAL_VB:
		u32DacAddr = (CAL_DACVARREF_CS | DAC7552_OUTB);
		break;
	case CSPDR_CAL_CODE:
		u32DacAddr = (CAL_DACDC_CS | DAC7552_OUTA);
		break;
	case CSPDR_CAL_REF:
		u32DacAddr = (CAL_DACDC_CS | DAC7552_OUTB);
		break;
	case CSPDR_EXTRIG_POSITION:
		u32DacAddr = (CAL_DACEXTRIG_CS | DAC7552_OUTA);
		break;
	case CSPDR_EXTRIG_LEVEL:
		u32DacAddr = (CAL_DACEXTRIG_CS | DAC7552_OUTB);
		break;

	case CSPDR_ADC_OFFSET_ADJUST:
		if( m_pCardState->u16AdcOffsetAdjust != u16Data )
		{
			m_pCardState->u16AdcOffsetAdjust = u16Data;
			for( uInt16 i = 1; i <= m_pCardState->PlxData.CsBoard.NbAnalogChannel; i += GetUserChannelSkip() )
			{
				uInt16 u16HwChan = ConvertToHwChannel(i);
				SendChannelSetting( u16HwChan,
									 m_pCardState->ChannelParams[u16HwChan-1].u32InputRange,
									 m_pCardState->ChannelParams[u16HwChan-1].u32Term,
									 m_pCardState->ChannelParams[u16HwChan-1].u32Impedance,
									 m_pCardState->ChannelParams[u16HwChan-1].i32Position,
									 m_pCardState->ChannelParams[u16HwChan-1].u32Filter,
									 1);
			}
			CalibrateAllChannels();
		}
		return CS_SUCCESS;

	default:
		return CS_INVALID_DAC_ADDR;
		break;
	}

	int32 i32Ret = WriteRegisterThruNios(u32Data, CSPSR_WRITE_TO_DAC7552_CMD | u32DacAddr);
	if( CS_FAILED(i32Ret) )	return i32Ret;


	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

uInt16 CsSpiderDev::GetControlDacId( eCalDacId ecdId, uInt16 u16Channel )
{
	switch ( ecdId )
	{
		case ecdPosition: return CSPDR_POSITION_1 + u16Channel - 1;
		case ecdOffset: return CSPDR_VCALFINE_1 + u16Channel - 1;
		case ecdGain: return CSPDR_CAL_GAIN_1 + u16Channel - 1;
	}

	return 0xffff;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsSpiderDev::SendCalibSource(const eCalMode ecMode, const uInt32 u32CalFequency)
{
	int32 i32Ret = CS_SUCCESS;

	uInt16 u16CalibSrc = 0;

	if(ecMode == ecmCalModeDc )
	{
		u16CalibSrc |= CSPDR_CALIB_SRC_DC;

		i32Ret = WriteRegisterThruNios(u16CalibSrc, CSPDR_CALIBMODE_SET_WR_CMD);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		m_u32CalFreq = 0;
		m_CalMode = ecmCalModeDc;
	}
	else if( ecMode == ecmCalModeAc)
	{
		if ( u32CalFequency >= CS_SR_1MHZ )
			u16CalibSrc |= CSPDR_CALIB_SRC_HI_FREQ;
		else if ( u32CalFequency < CS_SR_100KHZ )
			u16CalibSrc |= CSPDR_CALIB_SRC_LO_FREQ;
		else
			return CS_INVALID_CAL_MODE;

		//Configure DDS
		LONGLONG llFTW = u32CalFequency;
		llFTW <<= 32;
		llFTW /= CSPDR_DDS_CLK_FREQ;
		if( llFTW > (((LONGLONG)1)<<31) )
			return CS_INVALID_CAL_MODE;

		uInt32 u32FTW = (uInt32)llFTW;

		i32Ret = WriteRegisterThruNios(CSPDR_DDS_DEFAULT_CFR1, CSPDR_DDS_CFR1_WR_CMD);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteRegisterThruNios(CSPDR_DDS_DEFAULT_CFR2, CSPDR_DDS_CFR2_WR_CMD);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteRegisterThruNios(CSPDR_DDS_DEFAULT_ASF, CSPDR_DDS_ASF_WR_CMD);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteRegisterThruNios(CSPDR_DDS_DEFAULT_ARR, CSPDR_DDS_ARR_WR_CMD);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteRegisterThruNios(u32FTW, CSPDR_DDS_FTW0_WR_CMD);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteRegisterThruNios(CSPDR_DDS_DEFAULT_POW0, CSPDR_DDS_POW0_WR_CMD);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteRegisterThruNios(u16CalibSrc, CSPDR_CALIBMODE_SET_WR_CMD);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteRegisterThruNios(u16CalibSrc|CSPDR_CALIB_SRC_UPDATE, CSPDR_CALIBMODE_SET_WR_CMD);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteRegisterThruNios(u16CalibSrc, CSPDR_CALIBMODE_SET_WR_CMD);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		m_u32CalFreq = u32CalFequency;
		m_CalMode = ecmCalModeAc;
	}
	else
	{
		return CS_FALSE;
	}

	m_pCardState->AddOnReg.u16Calib = u16CalibSrc;

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSpiderDev::RelockReference()
{
	// Following code is comented out due to malfunction od the CSPDR_CLK_SET_RD_CMD
	//if( ( 0 != ( m_pCardState->AddOnReg.u16ClkSelect & CSPRD_SET_EXT_CLK ) ) &&
	//    ( 0 == ( m_pCardState->AddOnReg.u32ClkFreq & CSPDR_EXT_CLK_BYPASS) ) )
	//{
	//	uInt32 u32ClkStatus = ReadRegisterFromNios(m_pCardState->AddOnReg.u16ClkSelect, CSPDR_CLK_SET_RD_CMD);
	//	if( (u32ClkStatus & CSPDR_PLL_NOT_LOCKED) != 0 )
	//	{
	//		return SetClockThruNios( m_pCardState->AddOnReg.u32ClkFreq );
	//	}
	//}
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void  CsSpiderDev::ControlSpiClock(bool bStart)
{
	if ( bStart )
	{
		// turn on all clocks
		m_u32Ad7450CtlReg &= ~CSPDR_START_SPI_CLK;
	}
	else
	{
		// turn off all clocks
		m_u32Ad7450CtlReg |= CSPDR_START_SPI_CLK;
	}
	WriteGIO( CSPDR_AD7450_WRITE_REG, m_u32Ad7450CtlReg );
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

int32 CsSpiderDev::SendTriggerRelation( uInt32 u32Relation	)
{
	uInt16	u16Data = 0;
	uInt32	u32Command = CSPDR_DTS_GBL_SET_WR_CMD;
	int32	i32Ret ;

	u32Relation;

	i32Ret = WriteRegisterThruNios(u16Data, u32Command);
	return i32Ret;
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

int32	CsSpiderDev::SendTriggerDelay()
{
	int32 i32Ret = CS_SUCCESS;

	i32Ret = WriteRegisterThruNios(m_TrigOutCfg.u16Delay, CSPDR_TRIGDELAY_SET_WR_CMD);
	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsSpiderDev::FillOutBoardCaps(PCS8XXXBOARDCAPS pCapsInfo)
{
	int32		i32Ret = CS_SUCCESS;
	uInt32		u32Temp0;
	uInt32		u32Temp1;
	uInt32		i;
	uInt32		u32Size;

	::GageZeroMemoryX( pCapsInfo, sizeof(CS8XXXBOARDCAPS) );

	//Fill SampleRate
	for( i = 0; i < m_pCardState->u32SrInfoSize; i++ )
	{
		pCapsInfo->SrTable[i].PublicPart.i64SampleRate = m_pCardState->SrInfo[i].llSampleRate;
		u32Size =  sizeof(pCapsInfo->SrTable[i].PublicPart.strText);

		if( m_pCardState->SrInfo[i].llSampleRate >= 1000000 )
		{
			u32Temp0 = (uInt32) m_pCardState->SrInfo[i].llSampleRate/1000000;
			u32Temp1 = (uInt32) (m_pCardState->SrInfo[i].llSampleRate);
			u32Temp1 = (u32Temp1 % 1000000) /100000;
			if( u32Temp1 )
				sprintf_s( pCapsInfo->SrTable[i].PublicPart.strText, u32Size, "%d.%d MS/s", u32Temp0, u32Temp1 );
			else
				sprintf_s( pCapsInfo->SrTable[i].PublicPart.strText, u32Size, "%d MS/s", u32Temp0 );
		}
		else if( m_pCardState->SrInfo[i].llSampleRate >= 1000 )
		{
			u32Temp0 = (uInt32) m_pCardState->SrInfo[i].llSampleRate/1000;
			u32Temp1 = (uInt32) (m_pCardState->SrInfo[i].llSampleRate);
			u32Temp1 = (u32Temp1 % 1000) /100;
			if( u32Temp1 )
				sprintf_s( pCapsInfo->SrTable[i].PublicPart.strText, u32Size, "%d.%d kS/s", u32Temp0, u32Temp1 );
			else
				sprintf_s( pCapsInfo->SrTable[i].PublicPart.strText, u32Size, "%d kS/s", u32Temp0 );
		}
		else
		{
			sprintf_s( pCapsInfo->SrTable[i].PublicPart.strText, u32Size, "%lld S/s", m_pCardState->SrInfo[i].llSampleRate );
		}

		pCapsInfo->SrTable[i].u32Mode = CS_MODE_OCT | CS_MODE_QUAD | CS_MODE_DUAL | CS_MODE_SINGLE;
	}

	// Fill External Clock info
	if( 0 == (m_pCardState->PlxData.CsBoard.u32AddonHardwareOptions&CSPDR_AOHW_OPT_EMONA_FF) && !m_pCardState->bZap )
	{
		pCapsInfo->ExtClkTable[0].i64Max = m_pCardState->SrInfo[0].llSampleRate;
		pCapsInfo->ExtClkTable[0].i64Min = CSPDR_MIN_EXT_CLK;
		pCapsInfo->ExtClkTable[0].u32SkipCount = CS_SAMPLESKIP_FACTOR;
		pCapsInfo->ExtClkTable[0].u32Mode = CS_MODE_OCT | CS_MODE_QUAD | CS_MODE_DUAL | CS_MODE_SINGLE;
	}

	//Fill Range table
	// sorting is done on user level

	for( int k = 0; k < CSPDR_IMPED; k++ )
	{
		for( i = 0; i < m_pCardState->u32SwingTableSize[k]; i++ )
		{
			for( uInt32 j = 0; j < CSPDR_MAX_RANGES; j++ )
			{
				u32Size =  sizeof(pCapsInfo->RangeTable[j].strText);

				if( pCapsInfo->RangeTable[j].u32InputRange == m_pCardState->u32SwingTable[k][i] )
				{
					pCapsInfo->RangeTable[j].u32Reserved |= k == 0 ? CS_IMP_50_OHM : CS_IMP_1M_OHM;
					break;
				}
				else if( pCapsInfo->RangeTable[j].u32InputRange == 0 )
				{
					pCapsInfo->RangeTable[j].u32InputRange = m_pCardState->u32SwingTable[k][i];

					if( pCapsInfo->RangeTable[j].u32InputRange >= 2000 &&  0 == (pCapsInfo->RangeTable[j].u32InputRange % 2000))
						sprintf_s( pCapsInfo->RangeTable[j].strText, u32Size, "±%d V", pCapsInfo->RangeTable[j].u32InputRange / 2000 );
					else
						sprintf_s( pCapsInfo->RangeTable[j].strText, u32Size, "±%d mV", pCapsInfo->RangeTable[j].u32InputRange / 2 );

					pCapsInfo->RangeTable[j].u32Reserved |= k == 0 ? CS_IMP_50_OHM : CS_IMP_1M_OHM;
					break;
				}
			}
		}
	}

	//Fill ext trig table
	i = 0;
	u32Size =  sizeof(pCapsInfo->ExtTrigRangeTable[0].strText);
	if( 0 != (m_pCardState->u32InstalledMisc & CSPDR_EXT_1V_INST ) )
	{
		pCapsInfo->ExtTrigRangeTable[i].u32InputRange = CS_GAIN_2_V;
		strcpy_s( pCapsInfo->ExtTrigRangeTable[i].strText, u32Size, "Â±1 V" );
		pCapsInfo->ExtTrigRangeTable[i].u32Reserved = CS_IMP_1M_OHM|CS_IMP_EXT_TRIG;
		i++;
	}
	if( 0 != (m_pCardState->u32InstalledMisc & CSPDR_EXT_5V_INST ) )
	{
		pCapsInfo->ExtTrigRangeTable[i].u32InputRange = CS_GAIN_10_V;
		strcpy_s( pCapsInfo->ExtTrigRangeTable[i].strText, u32Size, "Â±5 V" );
		pCapsInfo->ExtTrigRangeTable[i].u32Reserved = CS_IMP_1M_OHM|CS_IMP_EXT_TRIG;
		i++;
	}

	//Fill coupling table
	i = 0;
	u32Size =  sizeof(pCapsInfo->CouplingTable[0].strText);
	if( 0 != (m_pCardState->u32InstalledTerminations & CSPDR_AC_COUPL_INST ) )
	{
		pCapsInfo->CouplingTable[i].u32Coupling = CS_COUPLING_AC;
		strcpy_s( pCapsInfo->CouplingTable[i].strText, u32Size, "AC" );
		pCapsInfo->CouplingTable[i].u32Reserved = 0;
		i++;
	}
	if( 0 != (m_pCardState->u32InstalledTerminations & CSPDR_DC_COUPL_INST ) )
	{
		pCapsInfo->CouplingTable[i].u32Coupling = CS_COUPLING_DC;
		strcpy_s( pCapsInfo->CouplingTable[i].strText, u32Size, "DC" );
		pCapsInfo->CouplingTable[i].u32Reserved = 0;
		i++;
	}

	//Fill impedance table
	i = 0;
	u32Size =  sizeof(pCapsInfo->ImpedanceTable[0].strText);
	if( 0 != m_pCardState->u32SwingTableSize[0]  )
	{
		pCapsInfo->ImpedanceTable[i].u32Imdepdance = CS_REAL_IMP_50_OHM;
		strcpy_s( pCapsInfo->ImpedanceTable[i].strText, u32Size, "50 Ohm" );
		pCapsInfo->ImpedanceTable[i].u32Reserved = 0;
		i++;
	}
	if( 0 != m_pCardState->u32SwingTableSize[1] )
	{
		pCapsInfo->ImpedanceTable[i].u32Imdepdance = CS_REAL_IMP_1M_OHM;
		strcpy_s( pCapsInfo->ImpedanceTable[i].strText, u32Size, "1 MOhm" );
		pCapsInfo->ImpedanceTable[i].u32Reserved = 0;
		i++;
	}

	// External trigge impedance
	pCapsInfo->ImpedanceTable[i].u32Imdepdance = CS_REAL_IMP_1M_OHM;
	strcpy_s( pCapsInfo->ImpedanceTable[i].strText, u32Size, "1 MOhm" );
	pCapsInfo->ImpedanceTable[i].u32Reserved = CS_IMP_EXT_TRIG;
	i++;


	//Fill filter table
	i = 0;
	if( !m_pCardState->bZap )
	{
		pCapsInfo->FilterTable[i].u32LowCutoff  = m_pCardState->u32LowCutoffFreq[0];
		pCapsInfo->FilterTable[i].u32HighCutoff = m_pCardState->u32HighCutoffFreq[0];
		i++;
	}

	pCapsInfo->FilterTable[i].u32LowCutoff  = m_pCardState->u32LowCutoffFreq[1];
	pCapsInfo->FilterTable[i].u32HighCutoff = m_pCardState->u32HighCutoffFreq[1];
	i++;

	pCapsInfo->bDcOffset = ( m_pCardState->bSpiderV12 && ! m_pCardState->bSpiderLP );

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL	CsSpiderDev::IsNiosInit(void)
{
	uInt32		u32Status = CS_SUCCESS;;

	if ( m_bNucleon )
	{
		Sleep(400);		// 400 ms

		// Wait for initialization of Nios
		for ( int i = 0; i < 8; i ++ )
		{
			u32Status = ReadGageRegister(STATUS_OF_NIOS);
			if(ERROR_FLAG != (u32Status & ERROR_FLAG) )
				break;
		}

		if(ERROR_FLAG == (u32Status & ERROR_FLAG) )
		{
			m_PlxData->InitStatus.Info.u32Fpga = 0;
			m_PlxData->InitStatus.Info.u32Nios = 0;

			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "NiosInit error: STATUS_REG_PLX error" );
		}
		else
		{
			m_PlxData->InitStatus.Info.u32Fpga = 1;
			m_PlxData->InitStatus.Info.u32Nios = 1;
		}
		return (bool)(m_PlxData->InitStatus.Info.u32Nios);
	}
	else
		return CsPlxBase::IsNiosInit();
}


int32	CsSpiderDev::BaseBoardConfigurationReset(uInt8	u8UserImageId)
{
	if (m_bNucleon)
		return ConfigurationReset(u8UserImageId);
	else
	{
		uInt32	u32Addr = 0;
		switch( u8UserImageId )
		{
		case 1:
			u32Addr = FIELD_OFFSET( BaseBoardFlashData, OptionnalImage1 );
			break;
		case 2:
			u32Addr = FIELD_OFFSET( BaseBoardFlashData, OptionnalImage2 );
			break;
		default:
			u32Addr = FIELD_OFFSET( BaseBoardFlashData, DefaultImage );
		}

		return CsPlxBase::BaseBoardConfigReset(u32Addr);
	}
}


#define NUCLEON_WAITCONFIGRESET			2000		// 200 ms
#define NUCLEON_CONFIG_RESET			0x11
#define NUCLEON_RECONFIG_BIT			0x8000
#define NUCLEON_REACONFIG_ADDR0			0x7
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

BOOLEAN CsSpiderDev::ConfigurationReset( uInt8	u8UserImageId )
{
	if ( m_bNoConfigReset || u8UserImageId > 2 )
		return TRUE;

	uInt32	u32Addr = FIELD_OFFSET(NUCLEON_FLASH_LAYOUT, FlashImage1);

	u32Addr += u8UserImageId*FIELD_OFFSET(NUCLEON_FLASH_LAYOUT, FlashImage1);

	u32Addr /= 2;		// Word address
	WriteGioCpld(NUCLEON_REACONFIG_ADDR0, (u32Addr >> 1));
	WriteGioCpld(NUCLEON_REACONFIG_ADDR0+1, ((u32Addr>>1) >> 8));
	WriteGioCpld(NUCLEON_REACONFIG_ADDR0+2, ((u32Addr>>1) >> 16));

	WriteFlashRegister( 0, NUCLEON_RECONFIG_BIT );
	WriteFlashRegister( 0, 0 );

	Sleep ( NUCLEON_WAITCONFIGRESET );
	
	CSTIMER	CsTimer;
	CsTimer.Set( KTIME_SECOND(2) );
	PCI_CONFIG_HEADER_0	TempHeader = {0};	
	while( TempHeader.DeviceID != m_PlxData->PCIConfigHeader.DeviceID )
	{
		ReadPciConfigSpace( &TempHeader, sizeof(TempHeader),  0 );	//---- Read the PCI configuration Registers
		if ( CsTimer.State() ) 
			break;
	}

	// Restore PCI config registers
	WritePciConfigSpace(&m_PlxData->PCIConfigHeader, sizeof(PCI_CONFIG_HEADER_0), 0);

	// Check the last Firmware page load
	uInt32 u32RegVal = ReadGioCpld(6);

	// Waitng for Nios ready
	CsTimer.Set(  KTIME_SECOND(2) );
	while( ! IsNiosInit() )
	{
		if ( CsTimer.State() ) 
			break;
	}

	if ( (u32RegVal & 0x40) == 0 )
	{
		m_pCardState->u8ImageId = u8UserImageId;
		return TRUE;
	}
	else
		return FALSE;
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int32	CsSpiderDev::ReadBaseBoardHardwareInfoFromFlash( BOOLEAN bChecksum )
{
	if ( ! m_bNucleon )
		return CsPlxBase::ReadBaseBoardHardwareInfoFromFlash( bChecksum );

	// Read BaseBoard board Hardware info
	uInt32		u32Offset;
	int32		i32Status = CS_FLASH_NOT_INIT;

	// Verify the default image checksum
	uInt32	u32ChecksumSignature;

	u32Offset = FIELD_OFFSET(NUCLEON_FLASH_IMAGE_STRUCT, FwInfo)+ FIELD_OFFSET(CS_FIRMWARE_INFOS, u32ChecksumSignature);
	if ( m_pCardState->bSafeBootImageActive )
		u32Offset += FIELD_OFFSET(NUCLEON_FLASH_LAYOUT, BootImage);
	else
		u32Offset += FIELD_OFFSET(NUCLEON_FLASH_LAYOUT, FlashImage1);

	i32Status = IoctlReadFlash( u32Offset, &u32ChecksumSignature, sizeof(u32ChecksumSignature) );

	if (! m_pCardState->bVirginBoard && bChecksum && (u32ChecksumSignature != CSI_FWCHECKSUM_VALID ) )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_FWCHECKSUM_ERROR, "Baseboard" );
		m_PlxData->CsBoard.u8BadBbFirmware |= BAD_DEFAULT_FW;
		return CS_INVALID_FW_VERSION;
	}

	NUCLEON_FLASH_MISC_DATA		BoardCfg;

	u32Offset = FIELD_OFFSET(NUCLEON_FLASH_LAYOUT, BoardData);

	i32Status = IoctlReadFlash( u32Offset, &BoardCfg, sizeof(BoardCfg) );
	if ( CS_SUCCEEDED(i32Status) )
	{
		if ( -1 != BoardCfg.Footer.i64ExpertPermissions )
			m_PlxData->CsBoard.i64ExpertPermissions	= BoardCfg.Footer.i64ExpertPermissions;
		else
			m_PlxData->CsBoard.i64ExpertPermissions = 0;
		m_PlxData->CsBoard.u32BaseBoardVersion	= BoardCfg.Footer.u32HwVersion;
		m_PlxData->CsBoard.u32SerNum			= BoardCfg.Footer.u32SerialNumber;
		m_PlxData->CsBoard.u32BaseBoardHardwareOptions = BoardCfg.Footer.u32HwOptions;
	}
	else
	{
		m_PlxData->CsBoard.i64ExpertPermissions	= 0;
		m_PlxData->CsBoard.u32BaseBoardVersion	= 0;
		m_PlxData->CsBoard.u32SerNum			= 0;
		m_PlxData->CsBoard.u32BaseBoardHardwareOptions = 0;
	}

	// Read Expert firmware info
	CS_FIRMWARE_INFOS		FwInfo;

	u32Offset = FIELD_OFFSET(NUCLEON_FLASH_LAYOUT, FlashImage2) + FIELD_OFFSET(NUCLEON_FLASH_IMAGE_STRUCT, FwInfo)
		 + FIELD_OFFSET(NUCLEON_FLASH_FWINFO_SECTOR, FwInfoStruct);
	i32Status = IoctlReadFlash( u32Offset, &FwInfo, sizeof(FwInfo) );
	if ( CS_SUCCEEDED(i32Status) && FwInfo.u32ChecksumSignature == CSI_FWCHECKSUM_VALID )
	{
		// Prevent using all Expert FWs that require ConfigReset
		if ( m_bNoConfigReset && FwInfo.u32FirmwareOptions != CS_BBOPTIONS_STREAM )
		{
			FwInfo.u32FirmwareVersion = 
			FwInfo.u32FirmwareOptions = 0;
		}

		m_PlxData->CsBoard.u32UserFwVersionB[1].u32Reg  = FwInfo.u32FirmwareVersion;
		m_PlxData->CsBoard.u32BaseBoardOptions[1]		= FwInfo.u32FirmwareOptions;
	}

	u32Offset = FIELD_OFFSET(NUCLEON_FLASH_LAYOUT, FlashImage3) + FIELD_OFFSET(NUCLEON_FLASH_IMAGE_STRUCT, FwInfo)
		 + FIELD_OFFSET(NUCLEON_FLASH_FWINFO_SECTOR, FwInfoStruct);
	i32Status = IoctlReadFlash( u32Offset, &FwInfo, sizeof(FwInfo) );
	if ( CS_SUCCEEDED(i32Status) && FwInfo.u32ChecksumSignature == CSI_FWCHECKSUM_VALID )
	{
		// Prevent using all Expert FWs that require ConfigReset
		if ( m_bNoConfigReset && FwInfo.u32FirmwareOptions != CS_BBOPTIONS_STREAM )
		{
			FwInfo.u32FirmwareVersion = 
			FwInfo.u32FirmwareOptions = 0;
		}

		m_PlxData->CsBoard.u32UserFwVersionB[2].u32Reg  = FwInfo.u32FirmwareVersion;
		m_PlxData->CsBoard.u32BaseBoardOptions[2]		= FwInfo.u32FirmwareOptions;
	}

	return i32Status;
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int32	CsSpiderDev::CsNucleonCardUpdateFirmware()
{
	int32					i32RetStatus = CS_SUCCESS;
	int32					i32Status = CS_SUCCESS;
	uInt8					*pBuffer;
	char					szText[128];
	char					OldFwFlVer[40];
	char					NewFwFlVer[40];
	FILE_HANDLE				hCsiFile = (FILE_HANDLE)0;
	FILE_HANDLE				hFwlFile = (FILE_HANDLE)0;
	FILE_HANDLE				hFile = (FILE_HANDLE)0;
	char					szCsiFileName[MAX_FILENAME_LENGTH];
	char					szFwlFileName[MAX_FILENAME_LENGTH];

	if ( !m_bNucleon )
		return CS_SUCCESS;

	sprintf_s(szCsiFileName, sizeof(szCsiFileName), "%s.csi", m_pCardState->szFpgaFileName );
	sprintf_s(szFwlFileName, sizeof(szCsiFileName), "%s.fwl", m_pCardState->szFpgaFileName );

	if ( (hCsiFile = GageOpenFileSystem32Drivers( szCsiFileName )) == 0 )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_OPEN_FILE, szCsiFileName );
		return CS_FAIL;
	}

	// check if any expert firmware needs updating. If not don't bother reading the fwl file.
	// if they do need updating and we can't open the file it's an error
	BOOL bUpdateFwl = ( m_PlxData->CsBoard.u8BadBbFirmware & (BAD_EXPERT1_FW | BAD_EXPERT2_FW) );

	if ( bUpdateFwl )
	{
		if ( (hFwlFile = GageOpenFileSystem32Drivers( szFwlFileName )) == 0 )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_OPEN_FILE, szFwlFileName );
			GageCloseFile( hCsiFile );
			return CS_FAIL;
		}
	}

	pBuffer = (uInt8 *) GageAllocateMemoryX( Max(NUCLEON_FLASH_FWIMAGESIZE, SPIDER_FLASHIMAGE_SIZE) );
	if ( NULL == pBuffer )
	{
		GageCloseFile( hCsiFile );
		if ( (FILE_HANDLE)0 != hFwlFile )
			GageCloseFile( hFwlFile );
		return CS_INSUFFICIENT_RESOURCES;
	}

	//-------------      UPDATE BASE FIRMWARE  --------------------------
	CS_FIRMWARE_INFOS FwInfo;

	uInt32 u32FpgaImageOffset = 0;
	uInt32 u32FpgaImageSize = 0;
	uInt32 u32FwInfoOffset = 0;

	uInt32	u32ImageOffset = 0;
	uInt32	u32FwVersion = 0;
	uInt32	u32FwOption = 0;
	uInt32	u32RealImageLocation = 0;

	if ( m_PlxData->CsBoard.u8BadBbFirmware )
	{
		// Loop for the default image and 2 expert images
		for (int i = 0; i <4; i++ )
		{
			if ( 0 == ( m_PlxData->CsBoard.u8BadBbFirmware & (BAD_DEFAULT_FW << i) ) )
				continue;

			if ( 0 == i || 3 == i )
			{
				// Load new image from file if it is not already in buffer
				::GageZeroMemoryX( pBuffer, NUCLEON_FLASH_FWIMAGESIZE);

				hFile				= hCsiFile;
				u32ImageOffset		= m_BaseBoardCsiEntry.u32ImageOffset;
				u32FpgaImageSize	= m_BaseBoardCsiEntry.u32ImageSize;
				u32FwVersion		= m_BaseBoardCsiEntry.u32FwVersion;
				u32FwOption			= m_BaseBoardCsiEntry.u32FwOptions;
				FriendlyFwVersion( u32FwVersion, NewFwFlVer, sizeof(NewFwFlVer), m_bCsiOld );
			}
			else
			{
				hFile				= hFwlFile;
				u32ImageOffset		= m_BaseBoardExpertEntry[i-1].u32ImageOffset;
				u32FpgaImageSize	= m_BaseBoardExpertEntry[i-1].u32ImageSize;
				u32FwVersion		= m_BaseBoardExpertEntry[i-1].u32Version;
				u32FwOption			= m_BaseBoardExpertEntry[i-1].u32Option;
				FriendlyFwVersion( u32FwVersion, NewFwFlVer, sizeof(NewFwFlVer), m_bFwlOld );
			}
			FriendlyFwVersion( m_PlxData->CsBoard.u32UserFwVersionB[i].u32Reg, OldFwFlVer, sizeof(OldFwFlVer), false);
			
			GageReadFile(hFile, pBuffer, u32FpgaImageSize, &u32ImageOffset);
			switch(i)
			{
				case 0:
					if ( m_pCardState->bSafeBootImageActive )
					{
						u32FpgaImageOffset = FIELD_OFFSET(NUCLEON_FLASH_LAYOUT, BootImage);
						u32RealImageLocation = 0;
					}
					else
					{
						u32FpgaImageOffset = FIELD_OFFSET(NUCLEON_FLASH_LAYOUT, FlashImage1);
						u32RealImageLocation = 1;
					}
					break;
				case 1:
					u32FpgaImageOffset = FIELD_OFFSET(NUCLEON_FLASH_LAYOUT, FlashImage2);
					u32RealImageLocation = 2;
					break;
				case 2:
					u32FpgaImageOffset = FIELD_OFFSET(NUCLEON_FLASH_LAYOUT, FlashImage3);
					u32RealImageLocation = 3;
					break;
				case 3:		// Config Boot FW with (CPLD 1.12 and up)
					u32FpgaImageOffset = FIELD_OFFSET(NUCLEON_FLASH_LAYOUT, FlashImage1);
					u32RealImageLocation = 1;
					break;
			}

			u32FwInfoOffset = u32FpgaImageOffset + FIELD_OFFSET(NUCLEON_FLASH_IMAGE_STRUCT, FwInfo) +
													FIELD_OFFSET(NUCLEON_FLASH_FWINFO_SECTOR, FwInfoStruct);

			i32Status = IoctlReadFlash( u32FwInfoOffset, &FwInfo, sizeof(FwInfo) );
			if ( CS_FAILED(i32Status) )
			{
				sprintf_s( szText, sizeof(szText), TEXT("Update Image(%d) error on reading firmware info."), u32RealImageLocation );
				CsEventLog EventLog;
				EventLog.Report( CS_ERROR_TEXT, szText );
				continue;
			}

			// Clear valid Checksum
			FwInfo.u32ChecksumSignature		= 0;
			i32Status = NucleonWriteFlash( u32FwInfoOffset, &FwInfo, sizeof(FwInfo) );
			if ( CS_FAILED(i32Status) )
			{
				sprintf_s( szText, sizeof(szText), TEXT("Update Image(%d) error on clearing checkum."), u32RealImageLocation );
				CsEventLog EventLog;
				EventLog.Report( CS_ERROR_TEXT, szText );
				continue;
			}

			// Write Base board FPGA image
			i32Status = NucleonWriteFlash( u32FpgaImageOffset, pBuffer, u32FpgaImageSize );
			if ( CS_SUCCEEDED(i32Status) )
			{
				FwInfo.u32FirmwareOptions = u32FwOption;
				FwInfo.u32FirmwareVersion = u32FwVersion;
				FwInfo.u32ChecksumSignature	= CSI_FWCHECKSUM_VALID;

				i32Status = NucleonWriteFlash( u32FwInfoOffset, &FwInfo, sizeof(FwInfo) );
				if ( CS_SUCCEEDED(i32Status) )
				{
					m_PlxData->CsBoard.u8BadBbFirmware &= ~(BAD_DEFAULT_FW << i);
					sprintf_s( szText, sizeof(szText), TEXT("Image(%d )from v%s to v%s"), u32RealImageLocation, OldFwFlVer, NewFwFlVer );
					CsEventLog EventLog;
					EventLog.Report( CS_BBFIRMWARE_UPDATED, szText );
				}
			}
			else
			{
				sprintf_s( szText, sizeof(szText), TEXT("Update Image(%d) error on writting Fpga image."), u32RealImageLocation );
				CsEventLog EventLog;
				EventLog.Report( CS_ERROR_TEXT, szText );
			}

			// The firmware has been changed.
			// Invalidate this firmware version in order to force reload the firmware version later
			GetBoardNode()->u32RawFwVersionB[i] = 0;
		}

		if ( m_bNoConfigReset )
			i32RetStatus = CS_FWUPDATED_SHUTDOWN_REQUIRED;
		else
		{
			i32RetStatus = i32Status;
			ConfigurationReset(0);
			ResetCachedState();
		}
	}

	// Update Addon firmware
	i32Status = UpdateAddonFirmware( hCsiFile, pBuffer );

	if ( hFwlFile )
		GageCloseFile( hFwlFile );

	GageCloseFile( hCsiFile );
	GageFreeMemoryX( pBuffer );

	if ( CS_FAILED(i32RetStatus) )
		return i32RetStatus;
	else
		return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSpiderDev::CsReadPlxNvRAM( PPLX_NVRAM_IMAGE nvRAM_image )
{
	if ( !m_bNucleon )
		return CsPlxBase::CsReadPlxNvRAM( nvRAM_image );

	// Simulate Nvram for backward compatible
	RtlZeroMemory( &nvRAM_image->Data, sizeof(nvRAM_image->Data) );

	nvRAM_image->Data.VendorId					= m_PlxData->PCIConfigHeader.VendorID;
	nvRAM_image->Data.DeviceId					= m_PlxData->PCIConfigHeader.DeviceID;
	nvRAM_image->Data.BaseBoardSerialNumber		= m_PlxData->CsBoard.u32SerNum;
	nvRAM_image->Data.BaseBoardVersion			= (uInt16)m_PlxData->CsBoard.u32BaseBoardVersion;

	nvRAM_image->Data.BaseBoardHardwareOptions	= m_PlxData->CsBoard.u32BaseBoardHardwareOptions;
	nvRAM_image->Data.MemorySize				= (uInt32 )(m_PlxData->CsBoard.i64MaxMemory / 1024); // Kb

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSpiderDev::CsWritePlxNvRam( PPLX_NVRAM_IMAGE nvRAM_image )
{
	if ( !m_bNucleon )
		return CsPlxBase::CsWritePlxNvRam( nvRAM_image );

	// Simulate Nvram for backward compatible
	NUCLEON_FLASH_MISC_DATA		BoardCfg;

	uInt32 u32Offset = FIELD_OFFSET(NUCLEON_FLASH_LAYOUT, BoardData);
	int32 i32Status = IoctlReadFlash( u32Offset, &BoardCfg, sizeof(BoardCfg) );
	if ( CS_SUCCEEDED(i32Status) )
	{
		BoardCfg.Footer.u32SerialNumber = nvRAM_image->Data.BaseBoardSerialNumber;
		BoardCfg.Footer.u32HwVersion	= nvRAM_image->Data.BaseBoardVersion;
		BoardCfg.Footer.u32HwOptions	= nvRAM_image->Data.BaseBoardHardwareOptions;
		BoardCfg.u32MemSizeKb			= nvRAM_image->Data.MemorySize;

		i32Status = NucleonWriteFlash( u32Offset, &BoardCfg, sizeof(BoardCfg) );
	}
	return i32Status;
}

int32 CsSpiderDev::IoctlGetPciExLnkInfo(PPCIeLINK_INFO pPciExInfo)
{
	uInt32	u32RetCode;
	uInt32	u32ByteReturned;

#ifdef _WINDOWS_
	
	u32RetCode = DeviceIoControl( GetIoctlHandle(), CS_IOCTL_READ_PCIeLINK_INFO,
									NULL, 0, pPciExInfo, sizeof(PCIeLINK_INFO), &u32ByteReturned, NULL );

	_ASSERT( u32RetCode != DEV_IOCTL_ERROR );
	if ( u32RetCode == DEV_IOCTL_ERROR )
	{
		u32RetCode = GetLastError();

		TRACE( DRV_TRACE_LEVEL, "IOCTL_READ_PCIeLINK_INFO fails. (WinError = %x )\n", u32RetCode );
		return CS_DEVIOCTL_ERROR;
	}
	else
		return CS_SUCCESS;

#endif

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSpiderDev::NucleonWriteFlash(uInt32 u32StartAddr, void *pBuffer, uInt32 u32Size, bool bBackup )
{	
	int32	i32Status = CS_SUCCESS;
	uInt8	*pu8Data = (uInt8 *) pBuffer;
	uInt8	*pBackupMemory = NULL;			// Hold the contents of eeprom sectors
	uInt32	u32EndAddr = u32StartAddr + u32Size;
	uInt32	u32BackupSizeFisrt = 0;
	uInt32	u32BackupSizeLast = 0;
	uInt8	*pFirstSectorBackup = NULL;		// Point to the backup memory of the first sector
	uInt8	*pLastSectorBackup = NULL;		// Point to the backup memory of the lastt sector

	uInt32	u32SectorSize = NUCLEON_FLASH_SECTOR_SIZE1;
	uInt32	u32StartSector = 0;
	uInt32	u32EndSector = 0;
	
	if ( u32StartAddr + u32Size < NUCLEON_FLASH_IMAGETRUCT_SIZE )
	{
		// Writtng boot image. Do not need to backup anything
		bBackup = false;
	}

	// Sectors 0-3 have the sector size NUCLEON_FLASH_SECTOR_SIZE0
	// Sectors 4 and up have the sector size NUCLEON_FLASH_SECTOR_SIZE1
	if ( u32StartAddr < NUCLEON_SECTORSIZE_CHANGED_FRONTIER )
	{
		u32StartSector		= u32StartAddr/NUCLEON_FLASH_SECTOR_SIZE0;
	}
	else if ( u32StartAddr >= NUCLEON_FLASH_SIZE - NUCLEON_SECTORSIZE_CHANGED_FRONTIER )
	{
		u32StartSector = u32StartAddr/NUCLEON_FLASH_SECTOR_SIZE1;
		// Added sector offset because of the small sectors from 0-3
		u32StartSector += (NUCLEON_FLASH_SECTOR_SIZE1/NUCLEON_FLASH_SECTOR_SIZE0)-1;
		// Added sector offset because of the small sectors above NUCLEON_FLASH_SIZE - NUCLEON_SECTORSIZE_CHANGED_FRONTIER
		u32StartSector +=  (u32StartAddr-(NUCLEON_FLASH_SIZE - NUCLEON_SECTORSIZE_CHANGED_FRONTIER))/NUCLEON_FLASH_SECTOR_SIZE0;
	}
	else
	{
		u32StartSector = u32StartAddr/NUCLEON_FLASH_SECTOR_SIZE1;
		// Added sector offset because of the small sectors from 0-3
		u32StartSector += (NUCLEON_FLASH_SECTOR_SIZE1/NUCLEON_FLASH_SECTOR_SIZE0)-1;

		if ( bBackup && (0 != (u32StartAddr % NUCLEON_FLASH_SECTOR_SIZE1)) )
			u32BackupSizeFisrt = u32StartAddr % NUCLEON_FLASH_SECTOR_SIZE1;
	}

	if ( u32EndAddr < NUCLEON_SECTORSIZE_CHANGED_FRONTIER )
		u32EndSector = u32EndAddr/NUCLEON_FLASH_SECTOR_SIZE0;
	else if ( u32StartAddr >= NUCLEON_FLASH_SIZE - NUCLEON_SECTORSIZE_CHANGED_FRONTIER )
	{
		u32EndSector = (u32EndAddr-1)/NUCLEON_FLASH_SECTOR_SIZE1;
		// Added sector offset because of the smal sectors from 0-3
		u32EndSector += (NUCLEON_FLASH_SECTOR_SIZE1/NUCLEON_FLASH_SECTOR_SIZE0)-1;
		// Added sector offset because of the small sectors above NUCLEON_FLASH_SIZE - NUCLEON_SECTORSIZE_CHANGED_FRONTIER
		u32EndSector +=  ((u32EndAddr-1)-(NUCLEON_FLASH_SIZE - NUCLEON_SECTORSIZE_CHANGED_FRONTIER))/NUCLEON_FLASH_SECTOR_SIZE0;
	}
	else
	{
		u32EndSector = u32EndAddr/NUCLEON_FLASH_SECTOR_SIZE1;
		// Added sector offset because of the smal sectors from 0-3
		u32EndSector += (NUCLEON_FLASH_SECTOR_SIZE1/NUCLEON_FLASH_SECTOR_SIZE0)-1;

		if ( bBackup && (0 != (u32EndAddr % NUCLEON_FLASH_SECTOR_SIZE1)) )
			u32BackupSizeLast = NUCLEON_FLASH_SECTOR_SIZE1 - (u32EndAddr % NUCLEON_FLASH_SECTOR_SIZE1);
	}

	bool	bMultipleSectors = (u32StartSector != u32EndSector);

	// Allocate memory for backup the contents of first and last sectors
	if ( u32BackupSizeFisrt > 0  || u32BackupSizeLast > 0 )
	{
		pBackupMemory = (uInt8 * ) ::GageAllocateMemoryX( 2*NUCLEON_FLASH_SECTOR_SIZE1 );
		if ( NULL == pBackupMemory )
			return CS_INSUFFICIENT_RESOURCES;

		pFirstSectorBackup = pBackupMemory;
		if (bMultipleSectors)
			pLastSectorBackup = pFirstSectorBackup + NUCLEON_FLASH_SECTOR_SIZE1;
		else
			pLastSectorBackup = pFirstSectorBackup;
	}

	if ( bMultipleSectors )
	{
		if ( u32BackupSizeFisrt > 0 )
		{
			// Backup the contents of the sector before StartAddr
			IoctlReadFlash( u32StartAddr - (u32StartAddr % NUCLEON_FLASH_SECTOR_SIZE1), pFirstSectorBackup, u32BackupSizeFisrt );
			// Copy new data to buffer
			GageCopyMemoryX( pFirstSectorBackup + u32BackupSizeFisrt, pu8Data, (NUCLEON_FLASH_SECTOR_SIZE1 - u32BackupSizeFisrt));
		}

		if ( u32BackupSizeLast > 0 )
		{
			// Backup the contents of the sector after the EndAddr ( StartAddr + Size )
			IoctlReadFlash( u32EndAddr , pLastSectorBackup + (u32EndAddr % NUCLEON_FLASH_SECTOR_SIZE1),  u32BackupSizeLast );
			// Copy new data to buffer
			GageCopyMemoryX( pLastSectorBackup, pu8Data + u32Size - (NUCLEON_FLASH_SECTOR_SIZE1 - u32BackupSizeLast), (NUCLEON_FLASH_SECTOR_SIZE1 - u32BackupSizeLast));
		}
	}
	else
	{
		if ( u32BackupSizeFisrt > 0  || u32BackupSizeLast > 0 )
		{
			// Backup the contents of the sector
			IoctlReadFlash( u32StartAddr - (u32StartAddr % NUCLEON_FLASH_SECTOR_SIZE), pFirstSectorBackup, NUCLEON_FLASH_SECTOR_SIZE1 );
			// Copy new data to buffer
			GageCopyMemoryX( pFirstSectorBackup + u32BackupSizeFisrt, pu8Data, u32Size);
		}
	}


	uInt32	u32Remain = u32Size;
	uInt32	u32WriteOffset = 0;
	uInt32	u32WriteSize = 0;
	uInt32	u32CurrentAddr = u32StartAddr;
	uInt32	u32CurrentSector = u32StartSector;

	if ( u32BackupSizeFisrt > 0 || u32BackupSizeLast > 0 )
	{
		// WRITE WITH BACKUP 
		if ( bMultipleSectors )
		{
			u32SectorSize = NUCLEON_FLASH_SECTOR_SIZE1;
			if ( u32BackupSizeFisrt > 0 )
			{
				i32Status = IoctlWriteFlash( u32StartSector, 0, pFirstSectorBackup, NUCLEON_FLASH_SECTOR_SIZE1 );
				u32Remain -= (NUCLEON_FLASH_SECTOR_SIZE1 - u32BackupSizeFisrt);
				pu8Data	  += (NUCLEON_FLASH_SECTOR_SIZE1 - u32BackupSizeFisrt);
				u32CurrentAddr += (NUCLEON_FLASH_SECTOR_SIZE1 - u32BackupSizeFisrt);
				u32CurrentSector++;
			}

			ASSERT( 0 == (u32CurrentAddr % u32SectorSize) );
			while ( u32Remain >= NUCLEON_FLASH_SECTOR_SIZE1 )
			{
				u32WriteOffset	= (u32CurrentAddr % u32SectorSize);
				u32WriteSize	= NUCLEON_FLASH_SECTOR_SIZE1;

				i32Status = IoctlWriteFlash( u32CurrentSector++, 0, pu8Data, u32WriteSize );
			
				pu8Data			+= u32WriteSize;
				u32Remain		-= u32WriteSize;
				u32CurrentAddr	+= u32WriteSize;			
			}

			if ( u32BackupSizeLast > 0 )
			{
				ASSERT( u32Remain > 0 );
				i32Status = IoctlWriteFlash( u32EndSector, 0, pLastSectorBackup, NUCLEON_FLASH_SECTOR_SIZE1 );
			}
		}
		else
			i32Status = IoctlWriteFlash( u32StartSector, 0, pFirstSectorBackup, NUCLEON_FLASH_SECTOR_SIZE1 );
	}
	else if ( bMultipleSectors )
	{
		while ( u32Remain > 0 )
		{
			if ( u32CurrentAddr < NUCLEON_SECTORSIZE_CHANGED_FRONTIER )
			{
				u32SectorSize = NUCLEON_FLASH_SECTOR_SIZE0;
			}
			else
			{
				u32SectorSize = NUCLEON_FLASH_SECTOR_SIZE1;
			}

			// Write only new data size
			u32WriteOffset	= (u32CurrentAddr % u32SectorSize);
			u32WriteSize	= (u32SectorSize - u32WriteOffset);
			if ( u32Remain < u32WriteSize )
				u32WriteSize = u32Remain;

			i32Status = IoctlWriteFlash( u32CurrentSector++, u32WriteOffset, pu8Data, u32WriteSize );
		
			pu8Data			+= u32WriteSize;
			u32Remain		-= u32WriteSize;
			u32CurrentAddr	+= u32WriteSize;			
		}
	}
	else
	{
		if ( u32StartAddr < NUCLEON_SECTORSIZE_CHANGED_FRONTIER || ( u32StartAddr >= NUCLEON_FLASH_SIZE - NUCLEON_SECTORSIZE_CHANGED_FRONTIER ))
			u32SectorSize = NUCLEON_FLASH_SECTOR_SIZE0;
		else
			u32SectorSize = NUCLEON_FLASH_SECTOR_SIZE1;
		i32Status = IoctlWriteFlash( u32StartSector, (u32StartAddr % u32SectorSize), pu8Data, u32Size );
	}

	// Write completed, Reset Flash
	IoctlWriteFlash( (uInt32)-1, 0, 0, 0 );
	GageFreeMemoryX(pBackupMemory);
	return i32Status;
}



//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int32	CsSpiderDev::UpdateAddonFirmware( FILE_HANDLE hCsiFile, uInt8 *pBuffer )
{
	uInt32					u32ChecksumOffset = 0;
	uInt32					u32Checksum = 0;
	int32					i32Status = CS_SUCCESS;
	char					szText[128];
	char					OldFwFlVer[40];
	char					NewFwFlVer[40];


	// ADDON firmware
	uInt32 u32OptionsOffset = FIELD_OFFSET(AddOnFlashData, ImagesHeader) + FIELD_OFFSET(AddOnFlashHeader, HdrElement) +
							  FIELD_OFFSET(AddonHdrElement, u32AddOnOptions);

	u32ChecksumOffset = FIELD_OFFSET(AddOnFlash32Mbit, Data) + FIELD_OFFSET(AddOnFlashData, u32ChecksumSignature);

	if ( m_PlxData->CsBoard.u8BadAddonFirmware && m_AddonCsiEntry.u32FwVersion != 0 )
	{
		// Load new image from file if it is not already in buffer
		::GageZeroMemoryX( pBuffer, BB_FLASHIMAGE_SIZE );
		GageReadFile(hCsiFile, pBuffer, m_AddonCsiEntry.u32ImageSize, &m_AddonCsiEntry.u32ImageOffset);

		// Clear valid Checksum
		u32Checksum	= 0;
		i32Status = WriteEepromEx( &u32Checksum, u32ChecksumOffset, sizeof(u32Checksum), AccessFirst );

		// Write Addon FPGA image
		i32Status = WriteEepromEx(pBuffer, 0, m_AddonCsiEntry.u32ImageSize, AccessMiddle);
		if ( CS_SUCCEEDED(i32Status) )
		{
			i32Status = WriteEepromEx( &m_AddonCsiEntry.u32FwOptions, u32OptionsOffset, sizeof(ULONG), AccessMiddle);
		}

		// Set valid Checksum
		if ( CS_SUCCEEDED(i32Status) )
		{
			u32Checksum	= CSI_FWCHECKSUM_VALID;
			i32Status = WriteEepromEx( &u32Checksum, u32ChecksumOffset, sizeof(u32Checksum), AccessLast );
		}

		if ( CS_SUCCEEDED(i32Status) )
		{
			FriendlyFwVersion(m_AddonCsiEntry.u32FwVersion, NewFwFlVer, sizeof(NewFwFlVer), m_bCsiOld);
			FriendlyFwVersion( m_PlxData->CsBoard.u32UserFwVersionA[0].u32Reg, OldFwFlVer, sizeof(OldFwFlVer), false);

			sprintf_s( szText, sizeof(szText), TEXT("Image%d from v%s to v%s"), 0, OldFwFlVer, NewFwFlVer );
			CsEventLog EventLog;
			EventLog.Report( CS_ADDONFIRMWARE_UPDATED, szText );

			m_pCardState->bAddOnRegistersInitialized = false;
			InitAddOnRegisters();
			ReadAddOnHardwareInfoFromEeprom();
			m_PlxData->CsBoard.u8BadAddonFirmware = 0;
		}
		else
			AddonConfigReset();

		// The firmware has been changed.
		// Invalidate this firmware version in order to force reload the firmware version later
		GetBoardNode()->u32RawFwVersionA[0] = 0;
	}

	return i32Status;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsSpiderDev::Trim1M_TCAP( uInt16 u16Channel, BOOL bTCap2Sel, uInt8 u8TrimCount )
{
	int32 i32Ret = CS_SUCCESS;
	uInt32	u32Data = 0;

	if ( ! m_pCardState->bSpiderV12 )
		return CS_FUNCTION_NOT_SUPPORTED;

	u32Data = ConvertToHwChannel( u16Channel );
	if ( bTCap2Sel )
		u32Data |= ( 2 << 3);	// Select Tcap2
	else
		u32Data |= ( 1 << 3);	// Select Tcap1

	u32Data |= ( (u8TrimCount % 32) << 5);

	i32Ret = WriteRegisterThruNios( u32Data, CSPLXBASE_TRIM_FE_CAPS );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsSpiderDev::UpdateBaseBoardMsStatus()
{
	// PCIe only
	// Read M/S setting from addon then send this information to base board
	return WriteRegisterThruNios( m_pCardState->u8MsCardId, CSPDR_MSCTRL_REG_WR );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsSpiderDev::GetTriggerInfo( uInt32	u32Segment, uInt32 u32NumOfSegment, int16 *pi16Channel )
{
	if ( ! m_bNucleon )
		return CS_INVALID_REQUEST;

	int32	i32Status = CS_SUCCESS;
	uInt8	u8MsCardId;
	uInt32	u32TrigAddress;

	if ( 0 == u32Segment || u32Segment > m_pCardState->AcqConfig.u32SegmentCount )
		return CS_INVALID_SEGMENT;

	if ( u32Segment + u32NumOfSegment - 1 > m_pCardState->AcqConfig.u32SegmentCount )
		return CS_INVALID_SEGMENT;

	// Set Mode select 0
	m_MasterIndependent->WriteGageRegister(MODESEL, MSEL_CLR | 0);
	m_MasterIndependent->WriteGageRegister(MODESEL, (~MSEL_CLR) & 0);
	m_MasterIndependent->WriteGageRegister( MASK_REG, 0 );

	for ( uInt32 i = 0; i < u32NumOfSegment; i++ )
	{
		CsSpiderDev *pCard = m_MasterIndependent;

		if ( 0 !=  m_MasterIndependent->m_Next )
		{
			// Find the card that has TRIGGERED event
			u32TrigAddress = m_MasterIndependent->ReadRegisterFromNios( u32Segment-1, CSPLXBASE_READ_TRIG_ADD_CMD ); 
			u8MsCardId = (uInt8) (u32TrigAddress & 0x7 );

			pCard = GetCardPointerFromMsCardId( u8MsCardId );
		}

		if ( pCard )
		{
			uInt32	u32TsHigh;
			uInt16	u16ChanIndex;

			// Find the channels that has TRIGGERED
			u32TsHigh = pCard->ReadRegisterFromNios( i+u32Segment-1, CSPLXBASE_READ_TIMESTAMP0_CMD );
			u32TsHigh = (u32TsHigh >> 16) & 0xF;

			if ( 1 <= u32TsHigh  && u32TsHigh <= 8 )
			{
				// Triggered from one of channels
				u16ChanIndex = ConvertToUserChannel( (uInt16) u32TsHigh );
				pi16Channel[i] = (int16) ((pCard->m_pCardState->u16CardNumber-1) * m_PlxData->CsBoard.NbAnalogChannel ) + u16ChanIndex;
			}
			else if ( 9 == u32TsHigh )
				pi16Channel[i] = -1;		// External trigger
			else
				pi16Channel[i] = 0;			// Cannot determine the channel that has trigger event
		}
		else
			pi16Channel[i] = 0;				// Cannot determine the channel that has trigger event
	}

	return i32Status;
}

uInt16 CsSpiderDev::ReadBusyStatus()
{
	if ( m_bMulrecAvgTD )
	{
		uInt32 RegValue = ReadGageRegister( INT_STATUS );
		if ( RegValue & MULREC_AVG_DONE )
			return 0;
		else
			return STILL_BUSY;
	}
	else
		return CsNiosApi::ReadBusyStatus();
}