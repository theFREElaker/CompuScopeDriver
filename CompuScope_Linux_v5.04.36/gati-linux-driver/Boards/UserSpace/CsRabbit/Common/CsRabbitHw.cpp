//	CsxyG8Hw.cpp
///	Implementation of CsRabbitDev class
//
//	This file contains all member functions that access to addon hardware including
//	Frontend set up and calibration
//
//

#include "StdAfx.h"
#include "CsTypes.h"
#include "GageWrapper.h"
#include "CsRabbit.h"
#include "CsxyG8CapsInfo.h"
#include "CsRabbitFlash.h"
#include "CsEventLog.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#endif
#include "CsIoctl.h"


#define	 CSRB_CALIB_TIMEOUT_LONG	KTIME_SECOND(1)			// 1 second timeout
#define	 CSRB_CALIB_TIMEOUT_SHORT	KTIME_MILISEC(300)		// 300 milisecond timeout


const FRONTEND_RABBIT_INFO	CsRabbitDev::c_FrontEndRabitGainDirectADC[] = 
{        		//        Gain  Flt AttC   AttB AttA ACnDC nCal
				//       +14 dB    -6 dB  -8 dB-14 dB
	  400, 0x000000,	//  0    0    0      0    0    0    0
};

#define c_RB_FE_INFO_SIZE_DIRECTADC   (sizeof(CsRabbitDev::c_FrontEndRabitGainDirectADC)/sizeof(FRONTEND_RABBIT_INFO))


const FRONTEND_RABBIT_INFO	CsRabbitDev::c_FrontEndRabitGain50[] = 
{        		//        Gain  Flt AttC   AttB AttA ACnDC nCal
				//       +14 dB    -6 dB  -8 dB-14 dB
	10000, 0x00001c,	//  0    0    1      1    1    0    0
	 4000, 0x000014,	//  0    0    1      0    1    0    0
	 2000, 0x000004,	//  0    0    0      0    1    0    0
	 1000, 0x000008,	//  0    0    0      1    0    0    0
	  400, 0x000000,	//  0    0    0      0    0    0    0
	  200, 0x000050,	//  1    0    1      0    0    0    0
	  100, 0x000040,	//  1    0    0      0    0    0    0
};

#define c_RB_FE_INFO_SIZE   (sizeof(CsRabbitDev::c_FrontEndRabitGain50)/sizeof(FRONTEND_RABBIT_INFO))


// SampleRate	ClkFreq	Decim	ClkDiv	Pinpong
const RABBIT_SAMPLE_RATE_INFO	CsRabbitDev::Rabit_2_2[] = 
{
	2000000000,	1000,	1,		1,		1,
    1000000000, 1000,	1,		1,		0,
	500000000,  1000,	2,		1,		0,
	250000000,	1000,	4,		1,		0,
	125000000,	1000,	8,		1,		0,
	100000000,	1000,	10,		1,		0,
	50000000,	1000,	20,		1,		0,
	25000000,	1000,	40,		1,		0,
	10000000,	1000,	100,	1,		0,
	5000000,	1000,	200,	1,		0,
	2000000,	1000,	500,	1,		0,
	1000000,	1000,	1000,	1,		0,
	500000,		1000,	2000,	1,		0,
	200000,		1000,	5000,	1,		0,
	100000,		1000,	10000,	1,		0,
	50000,		1000,	20000,	1,		0,
	20000,		1000,	50000,	1,		0,
	10000,		1000,	100000,	1,		0,
	5000,		1000,	200000,	1,		0,
	2000,		1000,	500000,	1,		0
//	1000,		500,	1000000,	2,		0
};

#define c_RB_SR_INFO_SIZE_2_2   (sizeof(CsRabbitDev::Rabit_2_2)/sizeof(RABIT_SAMPLE_RATE_INFO))


// SampleRate	ClkFreq	Decim	ClkDiv	Pinpong
const RABBIT_SAMPLE_RATE_INFO	CsRabbitDev::Rabit_2_1[] = 
{
    1000000000, 500,	1,		2,		1,
	500000000,  500,	1,		2,		0,
	250000000,	500,	2,		2,		0,
	125000000,	500,	4,		2,		0,
	50000000,	500,	10,		2,		0,
	25000000,	500,	20,		2,		0,
	10000000,	500,	50,		2,		0,
	5000000,	500,	100,	2,		0,
	2000000,	500,	250,	2,		0,
	1000000,	500,	500,	2,		0,
	500000,		500,	1000,	2,		0,
	200000,		500,	2500,	2,		0,
	100000,		500,	5000,	2,		0,
	50000,		500,	10000,	2,		0,
	20000,		500,	25000,	2,		0,
	10000,		500,	50000,	2,		0,
	5000,		500,	100000,	2,		0,
	2000,		500,	250000,	2,		0,
	1000,		500,	500000,	2,		0
};

#define c_RB_SR_INFO_SIZE_2_1   (sizeof(CsRabbitDev::Rabit_2_1)/sizeof(RABIT_SAMPLE_RATE_INFO))


// SampleRate	ClkFreq	Decim	ClkDiv	Pinpong
const RABBIT_SAMPLE_RATE_INFO	CsRabbitDev::Rabit_1_1[] = 
{
    1000000000, 500,	1,		2,		1,
	500000000,  500,	1,		2,		0,
	250000000,	500,	2,		2,		0,
	125000000,	500,	4,		2,		0,
	50000000,	500,	10,		2,		0,
	25000000,	500,	20,		2,		0,
	10000000,	500,	50,		2,		0,
	5000000,	500,	100,	2,		0,
	2000000,	500,	250,	2,		0,
	1000000,	500,	500,	2,		0,
	500000,		500,	1000,	2,		0,
	200000,		500,	2500,	2,		0,
	100000,		500,	5000,	2,		0,
	50000,		500,	10000,	2,		0,
	20000,		500,	25000,	2,		0,
	10000,		500,	50000,	2,		0,
	5000,		500,	100000,	2,		0,
	2000,		500,	250000,	2,		0,
	1000,		500,	500000,	2,		0
};


#define c_RB_SR_INFO_SIZE_1_1   (sizeof(CsRabbitDev::Rabit_1_1)/sizeof(RABIT_SAMPLE_RATE_INFO))


const RABBIT_SAMPLE_RATE_INFO	CsRabbitDev::CobraMax_2_4[] = 
{
// SampleRate	ClkFreq	Decim	ClkDiv	Pinpong		
	4000000000, 2000,	1,		1,		1,
	2000000000, 2000,	1,		1,		0,
	1000000000, 2000,	2,		1,		0,
	500000000,  2000,	4,		1,		0,
	250000000,  2000,	8,		1,		0,
	125000000,	2000,	16,	    1,		0,
	50000000,	2000,	40,	    1,		0,
	25000000,	2000,	80,	    1,		0,
	10000000,	2000,	200,    1,		0,
	5000000,	2000,	400,    1,		0,
	2000000,	2000,	1000,	1,		0,
	1000000,	2000,	2000,	1,		0,
	500000,		2000,	4000,	1,		0,
	200000,		2000,	10000,	1,		0,
	100000,		2000,	20000,	1,		0,
	50000,		2000,	40000,  1,		0,
	20000,		2000,	100000, 1,		0,
	10000,		2000,	200000, 1,		0,
	5000,		2000,	400000, 1,		0,
//	2000,		 500,	50000, 15,		0,			1,			0,		
//	1000,		 500,	100000,15,		0,			1,			0,		*/
};

const RABBIT_SAMPLE_RATE_INFO	CsRabbitDev::CobraMax_1_4[] = 
{
// SampleRate	ClkFreq	Decim	ClkDiv	Pinpong		
	4000000000, 2000,	1,		1,		1,
	2000000000, 2000,	2,		1,		1,
	1000000000, 2000,	4,		1,		1,
	500000000,  2000,	8,		1,		1,
	250000000,  2000,	16,		1,		1,
	100000000,	2000,	40,	    1,		1,
	50000000,	2000,	80,	    1,		1,
	25000000,	2000,	160,	1,		1,
	10000000,	2000,	400,    1,		1,
	5000000,	2000,	800,    1,		1,
	2000000,	2000,	2000,	1,		1,
	1000000,	2000,	4000,	1,		1,
	500000,		2000,	8000,	1,		1,
	200000,		2000,	20000,	1,		1,
	100000,		2000,	40000,	1,		1,
	50000,		2000,	80000,  1,		1,
	20000,		2000,	200000, 1,		1,
	10000,		2000,	400000, 1,		1,
	5000,		2000,	800000, 1,		1,
//	2000,		 500,	50000, 15,		0,			1,			0,		
//	1000,		 500,	100000,15,		0,			1,			0,		*/
};

#define c_RB_SR_INFO_SIZE_2_4   (sizeof(CsRabbitDev::CobraMax_2_4)/sizeof(RABIT_SAMPLE_RATE_INFO))
#define c_RB_SR_INFO_SIZE_1_4   (sizeof(CsRabbitDev::CobraMax_1_4)/sizeof(RABIT_SAMPLE_RATE_INFO))


const NEWRABBIT_TRIGADDR_OFFSET	CsRabbitDev::m_NewTrigAddrOffsetTable[] = 
{ 
	//Decimation	Offset Dual		Offset Single
	0,				0,				12,
    1,				4,				12,
	2,				4,				12,
	4,				5,				13,
	8,				5,				13,
	10,				6,				14,
	20,				5,				13,	
	40,				5,				13,
	50,				5,				13,
	100,			5,				13,
	250,			5,				13,
	500,			5,				13,
	1000,			5,				13,	
	2500,			5,				13,
	5000,			5,				13,
	10000,			5,				13,
	25000,			5,				13,
	50000,			5,				13,
	100000,			5,				13,
	250000,			5,				13,
	500000,			5,				13
};


const NEWRABBIT_TRIGADDR_OFFSET	CsRabbitDev::m_MsTrigAddrOffsetAdjust[] = 
{ 
	//Decimation	Offset Dual		Offset Single
	0,				0,				-16,
    1,				-8,				-8,
	2,				-4,				-4,
	4,				-3,				-3,
	8,				-1,				-1,
	10,				0,				0,
	20,				0,				0,	
	40,				0,				0,
	50,				0,				0,
	100,			0,				0,
	250,			0,				0,
	500,			0,				0,
	1000,			0,				0,	
	2500,			0,				0,
	5000,			0,				0,
	10000,			0,				0,
	25000,			0,				0,
	50000,			0,				0,
	100000,			0,				0,
	250000,			0,				0,
	500000,			0,				0
};


const uInt8 CsRabbitDev::m_ExtClkTrigAddrOffsetTable[2][2] = 
{ 
//	OffsetDual		OffsetSingle
    0,					12,				// Pingpong
	4,					28,				// No Pingpong
};




int32	CsRabbitDev::SendClockSetting(uInt32 u32Mode, LONGLONG llSampleRate, uInt32 u32ExtClk, BOOLEAN bRefClk, uInt32 u32ExtClkSkip )
{
	if ( m_bCobraMaxPCIe )
		return CobraMaxSendClockSetting(u32Mode, llSampleRate, bRefClk );

	int32	i32Ret = CS_SUCCESS;
	bool	bClockSourceChanged = false;

	BOOLEAN	bInit = ( CSRB_DEFAULT_SAMPLE_RATE == llSampleRate );

	PRABBIT_SAMPLE_RATE_INFO	 pSrInfo;
	uInt16	u16ClkSelect1 = CSRB_CLKR1_RESET_BT;

	if ( 0 != u32ExtClk )
	{
		RABBIT_SAMPLE_RATE_INFO		SrInfo = {0};

		SrInfo.u16ClockDivision = 1;
		SrInfo.u32Decimation = u32ExtClkSkip/CS_SAMPLESKIP_FACTOR;
		m_pCardState->AcqConfig.u32ExtClkSampleSkip = SrInfo.u32Decimation * CS_SAMPLESKIP_FACTOR;
		
		SrInfo.llSampleRate = llSampleRate;
		SrInfo.u32ClockFrequencyMhz = uInt32(llSampleRate / 1000000);
		
		LONGLONG	llMaxExtClk = GetMaxExtClockRate( u32Mode );
		// Validation of minimum ext clock frequency
		if ( llSampleRate < CSRB_MIN_EXT_CLK || llSampleRate > llMaxExtClk )
			return CS_INVALID_SAMPLE_RATE;

		if ( ( 0 != ( u32Mode & CS_MODE_SINGLE ) ) && ( llSampleRate > GetMaxSampleRate( false ) ) )
		{
			SrInfo.u16PingPong = 1;

			if ( m_pCardState->ermRoleMode == ermStandAlone )
				SrInfo.u16ClockDivision = 2;
			else if ( ! m_bAllowExtClkMs_pp )
				return CS_INVALID_SAMPLE_RATE;
		}

		if ( m_MasterIndependent == this )
			u16ClkSelect1 |= CSRB_CLKR1_EXTCLKINPUT_BT;

		pSrInfo = &SrInfo;
	}
	else
	{
		if ( bInit )
		{
			uInt32 u32BoardType = m_PlxData->CsBoard.u32BoardType & ~CSNUCLEONBASE_BOARDTYPE;
			if ( ( CS22G8_BOARDTYPE == u32BoardType ) || ( CS11G8_BOARDTYPE == u32BoardType ) )
				llSampleRate	= CS_SR_1GHZ;
			else
				llSampleRate	= CS_SR_500MHZ;

			m_pCardState->AddOnReg.u16ClkOut = CSRB_CLK_OUT_NONE;
		}
 
		if ( !FindSrInfo( llSampleRate, &pSrInfo ) )
		{
			return CS_INVALID_SAMPLE_RATE;
		}

		m_pCardState->u32SampleClock = pSrInfo->u32ClockFrequencyMhz * 1000;
		m_pCardState->AcqConfig.u32ExtClkSampleSkip = CS_SAMPLESKIP_FACTOR;

		u16ClkSelect1 |= CSRB_CLKR1_PLLCLKINPUT_BT;

		if ( bRefClk && m_MasterIndependent == this)
			u16ClkSelect1 |= CSRB_CLKR1_EXTCLKINPUT_BT;
	}

	bClockSourceChanged = (m_pCardState->AcqConfig.u32ExtClk != u32ExtClk);

	if ( m_bNucleon )
	{	
		if ( m_pCardState->ermRoleMode != ermStandAlone  )
			u16ClkSelect1 |= CSRB_CLKR1_MASTERCLOCK_EN; 
	}
	else
	{
		if ( ! m_pCardState->bEco10 && m_pCardState->ermRoleMode == ermMaster  )
			u16ClkSelect1 |= CSRB_CLKR1_MASTERCLOCK_EN; 
	}

	if ( m_pCardState->ermRoleMode == ermStandAlone )
		u16ClkSelect1 |= (0x400 | CSRB_CLKR1_STDCLKINPUT_BT);
	else
		u16ClkSelect1 |= 0x800;

	//Out0 is disabled, but stil set deviders to 1 for it OUT2 is the same devider as out 1
	uInt16 u16ClkSelect2 = ClkDivToAD9514(pSrInfo->u16ClockDivision) <<8 | ClkDivToAD9514(pSrInfo->u16ClockDivision) << 4;
	uInt16 u16DecimationLow = uInt16(pSrInfo->u32Decimation & 0x7fff);
	uInt16 u16DecimationHigh = uInt16((pSrInfo->u32Decimation>>15) & 0x7fff);
	
	if ( m_bNucleon || m_pCardState->bEco10 )
		u16ClkSelect2 |= ClkDivToAD9514( 8 * pSrInfo->u16ClockDivision );
	else
		u16ClkSelect2 |= ClkDivToAD9514(1);
	
	uInt32	u32NewDecimation;
	// Because calibrationa (M/S and Front end) are done with the highest sample clock (decimation 1)
	// To avoid the problem swiching back and fortth with different decimations for calibrations,
	// Here, we force dectmation 1 before calibration and 
	// only set to the real decimation at the end of calibrations.
	// 
	if ( m_bPreCalib )
		u32NewDecimation = 1;
	else
		u32NewDecimation = pSrInfo->u32Decimation;
	
	bool	bUseI = (CS_CHAN_1 == ConvertToHwChannel( CS_CHAN_1 )) || m_bSingleChannel2;

	if( (m_pCardState->AddOnReg.u32ClkFreq != pSrInfo->u32ClockFrequencyMhz  || 
		 m_pCardState->AddOnReg.u32Decimation != pSrInfo->u32Decimation ) ||
		 m_pCardState->AddOnReg.u16PingPong != pSrInfo->u16PingPong ||
		 m_pCardState->AddOnReg.u16ClkSelect1 != u16ClkSelect1   ||
		 m_pCardState->AddOnReg.u16ClkSelect2 != u16ClkSelect2 ||
		 m_bUseI != bUseI )
	{
		m_bUseI = bUseI;
		i32Ret = SetAdcMode(bUseI, pSrInfo->u16PingPong != 0, m_bAutoClockAdjust);
		if( CS_FAILED ( i32Ret ) )		return i32Ret;

		if ( m_pCardState->AddOnReg.u16PingPong != pSrInfo->u16PingPong || 
			 m_pCardState->AddOnReg.u32ClkFreq != pSrInfo->u32ClockFrequencyMhz ||
			 m_pCardState->AddOnReg.u16ClkSelect1 != u16ClkSelect1 ) 
		{
			m_pCardState->bMasterSlaveCalib = m_pCardState->bExtTrigCalib = true;
			i32Ret = SendAdcSelfCal();
			if( CS_FAILED ( i32Ret ) )
				return i32Ret;
		}

		if ( bClockSourceChanged || m_pCardState->AddOnReg.u32Decimation != pSrInfo->u32Decimation ) 
			m_MasterIndependent->m_pCardState->bMsDecimationReset = TRUE;

		i32Ret = WriteRegisterThruNios(u16ClkSelect1|m_pCardState->AddOnReg.u16ClkOut, CSRB_CLOCK_REG1);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		
		i32Ret = WriteRegisterThruNios(u16ClkSelect2, CSRB_CLOCK_REG2);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		
		if ( u16DecimationLow <= 1 )
			u16DecimationLow = 0;
		i32Ret = WriteRegisterThruNios(u16DecimationLow, CSRB_DEC_FACTOR_LOW_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		
		i32Ret = WriteRegisterThruNios(u16DecimationHigh, CSRB_DEC_FACTOR_HIGH_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		if(m_pCardState->AddOnReg.u16PingPong != pSrInfo->u16PingPong )
		{
			if ( u32Mode & CS_MODE_SINGLE )
			{
				if (m_bSingleChannel2)
					m_pCardState->bCalNeeded[ConvertToHwChannel( CS_CHAN_2 )-1] = true;
				else
					m_pCardState->bCalNeeded[ConvertToHwChannel( CS_CHAN_1 )-1] = true;
			}
			else
				m_pCardState->bCalNeeded[0] = m_pCardState->bCalNeeded[1] = true;
		}

		m_pCardState->AddOnReg.u16PingPong		= pSrInfo->u16PingPong;
		m_pCardState->AddOnReg.u32ClkFreq		= pSrInfo->u32ClockFrequencyMhz;
		m_pCardState->AddOnReg.u32Decimation	= pSrInfo->u32Decimation;
		m_pCardState->AddOnReg.u16ClkSelect1	= u16ClkSelect1;
		m_pCardState->AddOnReg.u16ClkSelect2	= u16ClkSelect2;
		m_pCardState->AddOnReg.u16ClockDiv		= pSrInfo->u16ClockDivision;

		m_pCardState->u32AdcClock = (uInt32) pSrInfo->u32ClockFrequencyMhz * 1000000;
		m_pCardState->AcqConfig.i64SampleRate = pSrInfo->llSampleRate;
		m_pCardState->AcqConfig.u32ExtClk = u32ExtClk;
	}
	
	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

int32	CsRabbitDev::SendCaptureModeSetting(uInt32 u32Mode, uInt16 u16HwChannelSingle )
{
	uInt16	u16ChanCtrl = 0;

	if ( u32Mode & CS_MODE_DUAL )
	{
		if( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_1CHANNEL) )
			return CS_INVALID_ACQ_MODE;
		u16ChanCtrl = 0;
	}
	else if ( u32Mode & CS_MODE_SINGLE )
	{
		if ( 0 != m_pCardState->AddOnReg.u16PingPong )
			u16ChanCtrl = m_bNucleon ? 3 : 0;
		else
			u16ChanCtrl = u16HwChannelSingle;
	}
	else
		return CS_INVALID_ACQ_MODE;

	if ( m_pCardState->AcqConfig.u32Mode != u32Mode ) 
		m_MasterIndependent->m_pCardState->bMsDecimationReset = TRUE;

	m_pCardState->AcqConfig.u32Mode = u32Mode;

	int32 i32Ret = WriteRegisterThruNios(u16ChanCtrl, CSRB_CHANNEL_CTRL);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

int32	CsRabbitDev::SetDataPath( uInt32 u16DataSelect )
{
	int32 i32Ret = WriteRegisterThruNios(u16DataSelect << 1, CSRB_DATA_SEL_ADDR);

	return i32Ret;
}


//-----------------------------------------------------------------------------
//-----	SendCalibModeSetting
//----- ADD-ON REGISTER
//-----------------------------------------------------------------------------
int32 CsRabbitDev::SendCalibModeSetting(uInt16 u16HwChannel, eCalMode eMode, uInt32 u32WaitTime )
{
	int32 i32Ret = CS_SUCCESS;
	const uInt16 u16ChanZeroBased = u16HwChannel - 1;
	uInt32 u32FrontEndSetup = m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] & ~(CSRB_SET_CHAN_CAL|CSRB_SET_AC_COUPLING);
	uInt16 u16CalibSetup	= CSRB_CAL_RESET | CSRB_CAL_POWER | CSRB_CAL_AC_GAIN | CSRB_MSPULSE_DISABLE | CSRB_CAL_EXT_TRIG_SRC;
	
	if( (0 == u16HwChannel) || (u16HwChannel > CSRB_CHANNELS)  )
	{
		return CS_INVALID_CHANNEL;
	}

	SetCalibBit( (ecmCalModeNormal != eMode) , &u32FrontEndSetup );

	switch ( eMode )
	{
	case ecmCalModeNormal:
		if( CS_COUPLING_AC == m_pCardState->ChannelParams[u16ChanZeroBased].u32Term )
			u32FrontEndSetup |= CSRB_SET_AC_COUPLING;
		break;
	case ecmCalModeMs:
		u16CalibSetup &= ~CSRB_MSPULSE_DISABLE;
		u16CalibSetup |= CSRB_PULSE_SIGNAL;
		u32FrontEndSetup |= CSRB_SET_AC_COUPLING;
		break;
	case ecmCalModeExtTrig:
		u16CalibSetup |= CSRB_LOCAL_PULSE_ENBLE;
		if(!m_bOnlyOnePulseLoad) u16CalibSetup |= CSRB_PULSE_SIGNAL;
		u16CalibSetup &= ~CSRB_CAL_EXT_TRIG_SRC;
		u32FrontEndSetup |= CSRB_SET_AC_COUPLING;
		break;

	case ecmCalModeTAO:
	case ecmCalModeExtTrigChan:
		u16CalibSetup |= CSRB_LOCAL_PULSE_ENBLE;
		u16CalibSetup |= CSRB_PULSE_SIGNAL;
		if(!m_bOnlyOnePulseLoad) u16CalibSetup &= ~CSRB_CAL_EXT_TRIG_SRC;
		u32FrontEndSetup |= CSRB_SET_AC_COUPLING;
		break;
		
	default:
	case ecmCalModeDc:
		//only one channel can be in calibration
/*	if( m_PlxData->CsBoard.NbAnalogChannel > 1 &&
			0 == (m_pCardState->AddOnReg.u32FeSetup[0] ^ m_pCardState->AddOnReg.u32FeSetup[1])  )
		{
			return CS_INVALID_CHANNEL;
		}
*/
		break;
	}

	if ( u16CalibSetup != m_pCardState->AddOnReg.u16Calib )
	{
		i32Ret = WriteRegisterThruNios(u16CalibSetup, CSRB_SET_CALIB);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		m_pCardState->AddOnReg.u16Calib = u16CalibSetup;
	}

	u32FrontEndSetup &= ~(3 << 9);
	u32FrontEndSetup |= u16HwChannel << 9;
	if( u32FrontEndSetup != m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] )
	{
		TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, eMode == ecmCalModeNormal?0:1,1,0);
		i32Ret = WriteRegisterThruNios(u32FrontEndSetup, CSPLXBASE_SETFRONTEND_CMD);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] = u32FrontEndSetup;
	}

	// Waiting for the channel switched between Calib and Input source
	// In the case this function has to be call many times, optimization can be done
	// by setting the waiting time only on the last call.
	if ( u32WaitTime )
		BBtiming( u32WaitTime );

	m_CalibMode = eMode;

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsRabbitDev::SendChannelSetting(uInt16 u16HwChannel, uInt32 u32InputRange, uInt32 u32Term, uInt32 u32Impendance, int32 i32Position, uInt32 u32Filter, int32 i32ForceCalib)
{
	int32 i32Ret = CS_SUCCESS;
	
	if( (CS_CHAN_1 != u16HwChannel) && ((CS_CHAN_2 != u16HwChannel))  )
	{
		return CS_INVALID_CHANNEL;
	}
	if (CS_REAL_IMP_50_OHM != u32Impendance )
	{
		return CS_INVALID_IMPEDANCE;
	}

	if ( CSRB_DEFAULT_INPUTRANGE == u32InputRange )
	{
		if ( m_pCardState->bHighBandwidth )
			u32InputRange = CSRB_DEFAULT_GAIN_HIGHBW;
		else
			u32InputRange = CSRB_DEFAULT_GAIN;
	}

	uInt16 u16ChanZeroBased = u16HwChannel-1;
	uInt32 u32FrontIndex = 0;

	i32Ret = FindFeIndex( u32InputRange, u32FrontIndex);
	if( CS_FAILED(i32Ret) ) return i32Ret;
	
	uInt32 u32FrontEndSetup = m_pCardState->FeInfoTable[u32FrontIndex].u32RegSetup;

	if ( CS_COUPLING_AC == (u32Term & CS_COUPLING_MASK) )
		u32FrontEndSetup |= CSRB_SET_AC_COUPLING;
	else if ( CS_COUPLING_DC == (u32Term & CS_COUPLING_MASK) )
		u32FrontEndSetup &= ~CSRB_SET_AC_COUPLING;
	else
		return CS_INVALID_COUPLING;

	if( i32Position >  int32(c_u32MaxPositionOffset_mV) || 
		i32Position < -int32(c_u32MaxPositionOffset_mV) ||
		i32Position >  int32(u32InputRange) || 
		i32Position < -int32(u32InputRange) )
	{
		return CS_INVALID_POSITION;
	}

	if( !m_bDebugCalibSrc )
	{
		if ( 0 == u32Filter )
		{
			u32FrontEndSetup &= ~CSRB_SET_CHAN_FILTER;
		}
		else 
		{
			u32FrontEndSetup |= CSRB_SET_CHAN_FILTER;
		}
	}
	else
	{
		if( m_pCardState->ChannelParams[u16ChanZeroBased].u32Filter != u32Filter )
		{
			m_pCardState->ChannelParams[u16ChanZeroBased].u32Filter = u32Filter;
		}
		u32FrontEndSetup &= ~CSRB_SET_CHAN_FILTER;
	}

	u32FrontEndSetup |= u16HwChannel << 9;
	if ( 0 == m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] )
	{
		// First time setup. Front end setting Normal input by default
		SetCalibBit( false, &u32FrontEndSetup );
	}
	else
	{
		//preserve calibration bit
		u32FrontEndSetup &= ~CSRB_SET_CHAN_CAL;
		u32FrontEndSetup |= (m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] & CSRB_SET_CHAN_CAL);
	}
		
	if( i32ForceCalib > 0 )
	{
		m_pCardState->bCalNeeded[u16ChanZeroBased] = true;
		m_bForceCal[u16ChanZeroBased] = true;
	}

	if ( m_pCardState->AddOnReg.u32FeIndex[u16ChanZeroBased] != u32FrontIndex || 
		 m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] != u32FrontEndSetup )
	{
		i32Ret = WriteRegisterThruNios(u32FrontEndSetup, CSPLXBASE_SETFRONTEND_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		m_pCardState->bCalNeeded[u16ChanZeroBased] = true;

		m_pCardState->AddOnReg.u32FeIndex[u16ChanZeroBased] = u32FrontIndex;
		m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] = u32FrontEndSetup;

		m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange = u32InputRange;
		m_pCardState->ChannelParams[u16ChanZeroBased].u32Term = u32Term & CS_COUPLING_MASK;
		m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance = CS_REAL_IMP_50_OHM;
		m_pCardState->ChannelParams[u16ChanZeroBased].u32Filter = u32Filter;
	}

	m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = i32Position;

	return i32Ret;
}




//-----------------------------------------------------------------------------
//----- SendTriggerEngineSetting
//----- ADD-ON REGISTER
//-----------------------------------------------------------------------------
int32 CsRabbitDev::SendTriggerEngineSetting(int32	i32SourceA1,
											uInt32  u32ConditionA1,
											int32   i32LevelA1,
                                            int32	i32SourceB1,
											uInt32  u32ConditionB1,
											int32   i32LevelB1, 
											int32	i32SourceA2,
											uInt32  u32ConditionA2,
											int32   i32LevelA2,
                                            int32	i32SourceB2,
											uInt32  u32ConditionB2,
											int32   i32LevelB2 )
											
{
	uInt16	u16Data = 0;

	// These inputs should have been already validated from DLL level
	// Double check ..
	if ( ( i32SourceA1 != CS_TRIG_SOURCE_CHAN_1 ) &&
		 ( i32SourceA1 != CS_TRIG_SOURCE_CHAN_2 ) &&
		 ( i32SourceA1 != CS_TRIG_SOURCE_DISABLE ) )
		return CS_INVALID_TRIG_SOURCE;

	if ( ( i32SourceB1 != CS_TRIG_SOURCE_CHAN_1 ) &&
		 ( i32SourceB1 != CS_TRIG_SOURCE_CHAN_2 ) &&
		 ( i32SourceB1 != CS_TRIG_SOURCE_DISABLE ) )
		return CS_INVALID_TRIG_SOURCE;

	if ( ( i32SourceA2 != CS_TRIG_SOURCE_CHAN_1 ) &&
		 ( i32SourceA2 != CS_TRIG_SOURCE_CHAN_2 ) &&
		 ( i32SourceA2 != CS_TRIG_SOURCE_DISABLE ) )
		return CS_INVALID_TRIG_SOURCE;

	if ( ( i32SourceB2 != CS_TRIG_SOURCE_CHAN_1 ) &&
		 ( i32SourceB2 != CS_TRIG_SOURCE_CHAN_2 ) &&
		 ( i32SourceB2 != CS_TRIG_SOURCE_DISABLE ) )
		return CS_INVALID_TRIG_SOURCE;

	if ( ( i32LevelA1 > 100  || i32LevelA1 < -100 ) ||
		 ( i32LevelB1 > 100  || i32LevelB1 < -100 ) ||
		 ( i32LevelA2 > 100  || i32LevelA2 < -100 ) ||
		 ( i32LevelB2 > 100  || i32LevelB2 < -100 ))
		return CS_INVALID_TRIG_LEVEL;


	if ( 0 < i32SourceA1  )
	{
		u16Data |= CSRB_SET_DIG_TRIG_A1_ENABLE;
		
		if ( CS_TRIG_SOURCE_CHAN_2 == ConvertToHwChannel( (uInt16) i32SourceA1 ) )
			u16Data |= CSRB_SET_DIG_TRIG_A1_CHAN_SEL;

		if ( u32ConditionA1 == CS_TRIG_COND_POS_SLOPE )
			u16Data |= CSRB_SET_DIG_TRIG_A1_POS_SLOPE;
	}
	if ( 0 < i32SourceB1  )
	{
		u16Data |= CSRB_SET_DIG_TRIG_B1_ENABLE;
		
		if ( CS_TRIG_SOURCE_CHAN_1 == ConvertToHwChannel( (uInt16) i32SourceB1 ) )
			u16Data |= CSRB_SET_DIG_TRIG_B1_CHAN_SEL;

		if ( u32ConditionB1 == CS_TRIG_COND_POS_SLOPE )
			u16Data |= CSRB_SET_DIG_TRIG_B1_POS_SLOPE;
	}
	if ( 0 < i32SourceA2  )
	{
		u16Data |= CSRB_SET_DIG_TRIG_A2_ENABLE;
		
		if ( CS_TRIG_SOURCE_CHAN_2 == ConvertToHwChannel( (uInt16) i32SourceA2 ) )
			u16Data |= CSRB_SET_DIG_TRIG_A2_CHAN_SEL;

		if ( u32ConditionA2 == CS_TRIG_COND_POS_SLOPE )
			u16Data |= CSRB_SET_DIG_TRIG_A2_POS_SLOPE;
	}
	if ( 0 < i32SourceB2  )
	{
		u16Data |= CSRB_SET_DIG_TRIG_B2_ENABLE;
		
		if ( CS_TRIG_SOURCE_CHAN_1 == ConvertToHwChannel( (uInt16) i32SourceB2 ) )
			u16Data |= CSRB_SET_DIG_TRIG_B2_CHAN_SEL;

		if ( u32ConditionB2 == CS_TRIG_COND_POS_SLOPE )
			u16Data |= CSRB_SET_DIG_TRIG_B2_POS_SLOPE;
	}

	int32 i32Ret = CS_SUCCESS;

	i32Ret = WriteRegisterThruNios(u16Data, CSRB_DTE_SETTINGS_ADDR);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret =  SendDigitalTriggerLevel( CSRB_TRIGENGINE_A1, i32LevelA1 );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret =  SendDigitalTriggerLevel( CSRB_TRIGENGINE_B1, i32LevelB1 );
	if( CS_FAILED(i32Ret) )
		return i32Ret;
	
	i32Ret =  SendDigitalTriggerLevel( CSRB_TRIGENGINE_A2, i32LevelA2 );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret =  SendDigitalTriggerLevel( CSRB_TRIGENGINE_B2, i32LevelB2 );
	if( CS_FAILED(i32Ret) )
		return i32Ret;


	for ( uInt16 i = CSRB_TRIGENGINE_A1 ; i < m_PlxData->CsBoard.NbTriggerMachine; i ++ )
	{
		i32Ret =  SendDigitalTriggerSensitivity( i, m_u8TrigSensitivity );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	m_pCardState->TriggerParams[CSRB_TRIGENGINE_A1].i32Source	= i32SourceA1;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_A1].i32Level	= i32LevelA1;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_A1].u32Condition = u32ConditionA1;

	m_pCardState->TriggerParams[CSRB_TRIGENGINE_B1].i32Source	= i32SourceB1;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_B1].i32Level	= i32LevelB1;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_B1].u32Condition = u32ConditionB1;

	m_pCardState->TriggerParams[CSRB_TRIGENGINE_A2].i32Source	= i32SourceA2;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_A2].i32Level	= i32LevelA2;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_A2].u32Condition = u32ConditionA2;

	m_pCardState->TriggerParams[CSRB_TRIGENGINE_B2].i32Source	= i32SourceB2;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_B2].i32Level	= i32LevelB2;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_B2].u32Condition = u32ConditionB2;

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsRabbitDev::SendExtTriggerSetting( BOOLEAN  bExtTrigEnabled,
										   int32	i32ExtLevel,
										   uInt32	u32ExtCondition,
										   uInt32	u32ExtTriggerRange, 
										   uInt32	u32ExtTriggerImped, 
										   uInt32	u32ExtCoupling,
										   int32	i32Sensitivity )
{
	uInt16	u16DataTrig = CSRB_SET_EXT_TRIG_BASE;
	uInt16	u16DataTrigger = 0;
	int32	i32Ret = CS_SUCCESS;


	if ( m_pCardState->bLAB11G && bExtTrigEnabled )
	{
		i32Ret = SendChannelSetting(ConvertToHwChannel(CS_CHAN_2), u32ExtTriggerRange, u32ExtCoupling );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	// Do not support Exttrigger from slave cards
	if ( bExtTrigEnabled && ! m_pCardState->bLAB11G )
	{
		if(	this != m_MasterIndependent )
			return CS_INVALID_EXT_TRIG;
		
		if( CS_GAIN_10_V == u32ExtTriggerRange ) 
			u16DataTrig |= CSRB_SET_EXT_TRIG_EXT5V;
		else if( CS_GAIN_2_V != u32ExtTriggerRange ) 
			return CS_INVALID_GAIN;

		if( CS_COUPLING_AC == u32ExtCoupling ) 
			u16DataTrig |= CSRB_SET_EXT_TRIG_AC;
		else if( CS_COUPLING_DC != u32ExtCoupling)
			  return CS_INVALID_COUPLING;

		if( CS_REAL_IMP_1M_OHM == u32ExtTriggerImped )
			  u16DataTrig |= CSRB_SET_EXT_TRIG_HiZ;
		else if( CS_REAL_IMP_50_OHM != u32ExtTriggerImped )
			return CS_INVALID_IMPEDANCE;

		if( CS_TRIG_COND_POS_SLOPE == u32ExtCondition )
			u16DataTrigger |=  CSRB_SET_EXT_TRIGGER_POS_SLOPE;
		else if( CS_TRIG_COND_NEG_SLOPE != u32ExtCondition )
			return CS_INVALID_TRIG_COND;

		u16DataTrigger |= CSRB_SET_EXT_TRIGGER_ENABLED;
		
		if ( m_bCalibActive )
			i32Ret = SendTriggerEnableSetting( eteAlways );
		else if ( m_MasterIndependent == this )
		{
			// Ext trigger and triggerout are only avaiable from master card so far....
			i32Ret = SendTriggerEnableSetting( m_pCardState->eteTrigEnable );
		}
	}
	else
		i32Ret = SendTriggerEnableSetting( eteAlways );

	if( CS_FAILED(i32Ret) )
		return i32Ret;

	if ( ! m_pCardState->bDisableTrigOut )
		u16DataTrig |= CSRB_SET_EXT_TRIG_TRIGOUTEN;

	if ( m_pCardState->AddOnReg.u16ExtTrigEnable != u16DataTrigger )
	{
		i32Ret = WriteRegisterThruNios(u16DataTrigger, CSRB_EXT_TRIGGER_SETTING);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		m_pCardState->AddOnReg.u16ExtTrigEnable = u16DataTrigger;
	}
	
	if ( m_pCardState->AddOnReg.u16ExtTrig != u16DataTrig )
	{
		i32Ret = WriteRegisterThruNios( u16DataTrig, CSRB_SET_EXT_TRIG);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		m_pCardState->AddOnReg.u16ExtTrig = u16DataTrig;
	}

	// Enabled All trigger engines
	i32Ret = WriteRegisterThruNios(  CSRB_GLB_TRIG_ENG_ENABLE, CSRB_TRIG_SETTING_ADDR);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	if ( bExtTrigEnabled && ! m_pCardState->bLAB11G )
	{
		if( 0 == i32Sensitivity )
			i32Sensitivity = m_i32ExtTrigSensitivity;
		i32Ret = SendExtTriggerLevel( i32ExtLevel, CS_TRIG_COND_POS_SLOPE == u32ExtCondition, i32Sensitivity );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].i32Source = bExtTrigEnabled ? -1 : 0;

	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].i32Level	= i32ExtLevel;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32Condition = u32ExtCondition;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange = u32ExtTriggerRange;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtCoupling = u32ExtCoupling;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance = u32ExtTriggerImped;

	return CS_SUCCESS;
}



//-----------------------------------------------------------------------------
// Input signal is scaled to �1000 mV (c_u32ExtTrigRange)
// Then it is attuneated by devider R322 and R320
// Trigger levels swing �c_u32MaxTrigDacLevel
// So c_u32ExtTrigRange input result in  c_u32ExtTrigRange * R320 / (R320+R322) input of comparator
// wich corresponds to c_u32MaxTrigDacLevel * MaxLelelCode / (c_u16CalDacCount/2)
// thus 
// MaxLevelCode = c_u32ExtTrigRange * R320 * (c_u16CalDacCount/2) / ((R320+R322) * c_u32MaxTrigDacLevel)
// this code leanly scales to smaller levels
//-----------------------------------------------------------------------------

const uInt32  CsRabbitDev::c_u32R320               = 19600; //mOhm
const uInt32  CsRabbitDev::c_u32R322               = 24900; //mOhm
const uInt32  CsRabbitDev::c_u32MaxExtTrigDacLevel = 500;   //mV
const uInt32  CsRabbitDev::c_u32ExtTrigRange       = 1000;  //mV

int32 CsRabbitDev::SendExtTriggerLevel( int32 i32Level, bool bRising, int32 i32Sensitivity )
{
	int32	i32Ret = CS_SUCCESS;
	const LONGLONG cllFactor1 = 100;
	const LONGLONG cllFactor2 = 256;
	const LONGLONG cllNom = LONGLONG(c_u32ExtTrigRange)*c_u16CalDacCount/2 * c_u32R320;
	const LONGLONG cllDenom = LONGLONG(c_u32MaxExtTrigDacLevel) * (c_u32R320 + c_u32R322) * 100;
	const int32 ci32LevelSensitivity = int32((cllFactor1* cllNom * m_u8TrigSensitivity)/(cllFactor2 * cllDenom));

	//----- Convert from (-100, 99) range to ADC range
	int32 i32LevelConverted = - int32(LONGLONG(i32Level)*cllNom/cllDenom);
	i32LevelConverted += c_u16CalAdcCount/2;

	if ( 0 == i32Sensitivity )
		i32Sensitivity = ci32LevelSensitivity;

	int32 i32LevelHi = i32LevelConverted;
	if( !bRising )
		i32LevelHi -= i32Sensitivity;

	if( c_u16CalDacCount <= i32LevelHi )
		i32LevelHi = c_u16CalDacCount - 1;
	else if( 0 > i32LevelHi )
		i32LevelHi = 0;

	i32Ret = WriteToDac(BUNNY_EXTRIG_LEVEL_HIGH, (uInt16)i32LevelHi);
	if( CS_FAILED(i32Ret) )
		return i32Ret;
	
	int32 i32LevelLo = i32LevelConverted;

	if( bRising )
		i32LevelLo += i32Sensitivity;
		
	if( c_u16CalDacCount <= i32LevelLo )
		i32LevelLo = c_u16CalDacCount - 1;
	else if( 0 > i32LevelLo )
		i32LevelLo = 0;

	// On Ext Trig calib mode, only Level Low DAC has effect on ext trigger sensitivity
	// This hardcoded value here has been checked by Hw engineer. 
	// With this value, it should have Trigger on Calib pulse
	if ( ecmCalModeExtTrig == m_CalibMode )
		i32LevelLo = EXTTRIGCALIB_LEVELLOW_DAC_VAL;

	i32Ret = WriteToDac(BUNNY_EXTRIG_LEVEL_LOW, (uInt16)i32LevelLo);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsRabbitDev::SendDigitalTriggerLevel( uInt16  u16TrigEngine, int32 i32Level )
{
	uInt16	u16Data = CSRB_SPI;
	int32	i32Ret = CS_SUCCESS;		
	int32	i32LevelConverted = i32Level;

	//----- Convert from (-100, 99) range to ADC range
	int32  i32ADCRange = 1 << m_pCardState->AcqConfig.u32SampleBits;

	i32LevelConverted = i32Level + 100;
	i32LevelConverted *= i32ADCRange;
	i32LevelConverted /= 200L;

	if( CSRB_DIG_TRIGLEVEL_SPAN <= i32LevelConverted )
		i32LevelConverted = CSRB_DIG_TRIGLEVEL_SPAN - 1;
	else if( 0 > i32LevelConverted )
		i32LevelConverted = 0;

	int16 i16LevelDAC = (int16)i32LevelConverted;

	u16Data |= i16LevelDAC;

	switch ( u16TrigEngine )
	{
	case CSRB_TRIGENGINE_A1:
		i32Ret = WriteRegisterThruNios(u16Data, CSRB_DLA1_SETTING_ADDR);
		break;
	case CSRB_TRIGENGINE_B1:
		i32Ret = WriteRegisterThruNios(u16Data, CSRB_DLB1_SETTING_ADDR);
		break;
	case CSRB_TRIGENGINE_A2:
		i32Ret = WriteRegisterThruNios(u16Data, CSRB_DLA2_SETTING_ADDR);
		break;
	case CSRB_TRIGENGINE_B2:
		i32Ret = WriteRegisterThruNios(u16Data, CSRB_DLB2_SETTING_ADDR);
		break;

	}

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsRabbitDev::SendDigitalTriggerSensitivity( uInt16  u16TrigEngine, uInt8 u8Sensitive )
{
	uInt16	u16Data = CSRB_SPI | u8Sensitive;
	int32	i32Ret = CS_SUCCESS;		

	switch ( u16TrigEngine )
	{
	case CSRB_TRIGENGINE_A1:
		i32Ret = WriteRegisterThruNios(u16Data, CSRB_DSA1_SETTING_ADDR);
		break;
	case CSRB_TRIGENGINE_B1:
		i32Ret = WriteRegisterThruNios(u16Data, CSRB_DSB1_SETTING_ADDR);
		break;
	case CSRB_TRIGENGINE_A2:
		i32Ret = WriteRegisterThruNios(u16Data, CSRB_DSA2_SETTING_ADDR);
		break;
	case CSRB_TRIGENGINE_B2:
		i32Ret = WriteRegisterThruNios(u16Data, CSRB_DSB2_SETTING_ADDR);
		break;

	}

	return i32Ret;
}

//-----------------------------------------------------------------------------
//----- WriteToSelfTestRegister
//----- ADD-ON REGISTER
//-----------------------------------------------------------------------------
int32 CsRabbitDev::WriteToSelfTestRegister( uInt16 u16SelfTestMode )
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

	int32 i32Ret = WriteRegisterThruNios(u16Data, CSRB_SELFTEST_MODE_WR_CMD);
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

int32 CsRabbitDev::SetClockOut( eRoleMode eMode, eClkOutMode ecomSet )
{
	if ( m_bCobraMaxPCIe )
		return CobraMaxSetClockOut( eMode, ecomSet );

	uInt16 u16Data = 0;
	eRoleMode LocaleMode = eMode;

	if( !m_pCardState->bEco10 ) //prior to eco10 all board were configured as master regarding clock out
		LocaleMode = ermMaster;

	if  ( ermSlave == LocaleMode )
	{
			ecomSet = eclkoNone;
			u16Data = CSRB_CLK_OUT_NONE;
	}
	else 
	{
		switch( ecomSet )
		{
			default: ecomSet = eclkoNone;
			case eclkoNone:      u16Data = CSRB_CLK_OUT_NONE; break;
			case eclkoRefClk:    u16Data = CSRB_CLK_OUT_REF;  break;
			case eclkoGBusClk:   u16Data = CSRB_CLK_OUT_FPGA; break;
			case eclkoSampleClk:
			{
				u16Data = CSRB_CLK_OUT_ADC;
				if ( eMode == ermStandAlone  )
					u16Data |= 0xA00;
			}
		}
	}

	if ( m_PlxData->InitStatus.Info.u32Nios != 0 && 
		m_PlxData->InitStatus.Info.u32AddOnPresent != 0 &&
	    (( m_pCardState->AddOnReg.u16ClkOut != u16Data ) || (eMode != m_pCardState->ermRoleMode) ))
	{
		uInt16 u16ClkSelect1 = m_pCardState->AddOnReg.u16ClkSelect1 & ~(CSRB_CLKR1_STDCLKINPUT_BT | CSRB_CLKR1_MASTERCLOCK_EN | 0x1E00);
		if ( m_bNucleon )
		{	
			if ( eMode != ermStandAlone  )
				u16ClkSelect1 |= CSRB_CLKR1_MASTERCLOCK_EN; 
		}
		else
		{
			if ( ! m_pCardState->bEco10 && eMode == ermMaster  )
				u16ClkSelect1 |= CSRB_CLKR1_MASTERCLOCK_EN; 
		}

		if ( eMode == ermStandAlone )
			u16ClkSelect1 |= (0x400 | CSRB_CLKR1_STDCLKINPUT_BT);
		else
			u16ClkSelect1 |= 0x800;

		int32 i32Ret = WriteRegisterThruNios(u16ClkSelect1|u16Data, CSRB_CLOCK_REG1);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		m_pCardState->AddOnReg.u16ClkSelect1 = u16ClkSelect1;
		m_pCardState->AddOnReg.u16ClkOut = u16Data;
		m_pCardState->eclkoClockOut = ecomSet;
	}

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsRabbitDev::WriteToAdcReg(uInt16 u16Addr, uInt16 u16Data)
{
	if ( m_pCardState->b14G8 )
	{
		switch( u16Addr )
		{
			case CSRB_ADC_REG_CFG: u16Data |= RB_ADC_CFG_MASK; m_pCardState->AddOnReg.AdcReg.Cfg = u16Data; break;
			case CSRB_ADC_REG_OFF1: u16Data |= CBMAX_ADC_OFF_MASK; m_pCardState->AddOnReg.AdcReg.Off1 = u16Data; break;
			case CSRB_ADC_REG_GAIN1: u16Data |= CBMAX_ADC_GAIN_MASK; m_pCardState->AddOnReg.AdcReg.Gain1 = u16Data; break;
			case CBMAX_ADC_REG_CLKPHASE_FINE: u16Data |= CBMAX_ADC_OFF_MASK; m_pCardState->AddOnReg.AdcReg.DES = u16Data; break;
			case CBMAX_ADC_REG_CLKPHASE_COARSE: u16Data |= CBMAX_ADC_COARSE_MASK; m_pCardState->AddOnReg.AdcReg.CoarseDly = u16Data; break;
				break;
			default: return CS_INVALID_DAC_ADDR;
		}
	}
	else
	{
		switch( u16Addr )
		{
			case CSRB_ADC_REG_CFG: u16Data |= RB_ADC_CFG_MASK; m_pCardState->AddOnReg.AdcReg.Cfg = u16Data; break;
			case CSRB_ADC_REG_OFF1: u16Data |= RB_ADC_OFF_MASK; m_pCardState->AddOnReg.AdcReg.Off1 = u16Data; break;
			case CSRB_ADC_REG_GAIN1: u16Data |= RB_ADC_REG_MASK; m_pCardState->AddOnReg.AdcReg.Gain1 = u16Data; break;
			case CSRB_ADC_REG_OFF2: u16Data |= RB_ADC_OFF_MASK; m_pCardState->AddOnReg.AdcReg.Off2 = u16Data; break;
			case CSRB_ADC_REG_GAIN2: u16Data |= RB_ADC_REG_MASK; m_pCardState->AddOnReg.AdcReg.Gain2 = u16Data; break;
			default:
			{
				if ( m_bCobraMaxPCIe )
				{
					switch( u16Addr )
					{
						case CBMAX_ADC_REG_CLKP_FINE:
							u16Data |= RB_ADC_REG_MASK; m_pCardState->AddOnReg.AdcReg.FineDly = u16Data; break;
						case CBMAX_ADC_REG_CLKP_COARSE:
							u16Data |= 0x7F; m_pCardState->AddOnReg.AdcReg.CoarseDly = u16Data; break;
						case CBMAX_ADC_REG_ECM:
							break;
						default: 
							return CS_INVALID_DAC_ADDR;
					}
				}
				else
				{
					switch( u16Addr )
					{
						case CSRB_ADC_REG_DES: u16Data |= RB_ADC_DES_MASK; m_pCardState->AddOnReg.AdcReg.DES = u16Data; break;
						case CSRB_ADC_REG_FINE: u16Data |= RB_ADC_REG_MASK; m_pCardState->AddOnReg.AdcReg.FineDly = u16Data; break;
						case CSRB_ADC_REG_COARSE: u16Data |= RB_ADC_COARSE_MASK; m_pCardState->AddOnReg.AdcReg.CoarseDly = u16Data; break;
						default: 
							return CS_INVALID_DAC_ADDR;
					}
				}
			}
		}
	}

	return WriteRegisterThruNios( u16Data,  CSRB_ADC_WR_CMD | u16Addr );
}

//-----------------------------------------------------------------------------
// 0, 1 and -1 are treated like +zero
//-----------------------------------------------------------------------------

int32 CsRabbitDev::SetAdcOffset(uInt16 u16Chan, int16 i16Offset)
{
	if ( CS_CHAN_1 > u16Chan || CS_CHAN_2 < u16Chan )
		return CS_INVALID_CHANNEL;

	uInt16 u16Addr =  CSRB_ADC_REG_OFF1;
	if( CS_CHAN_2 == u16Chan )
		u16Addr =  CSRB_ADC_REG_OFF2;

	int16 i16Off = i16Offset  > 0? i16Offset-1 : (i16Offset < 0 ? -i16Offset+1 : 0);

	if( i16Off > BUNNY_ADC_REG_SPAN )
		i16Off = BUNNY_ADC_REG_SPAN;

	uInt16 u16Data = i16Off << 8;
	u16Data |= i16Offset  < 0 ? RB_ADC_OFFSET_SIGN : 0;

	return WriteToAdcReg(u16Addr, u16Data);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsRabbitDev::SetAdcGain(uInt16 u16Chan, uInt16 u16Gain)
{
	if ( CS_CHAN_1 > u16Chan || CS_CHAN_2 < u16Chan )
		return CS_INVALID_CHANNEL;

	uInt16 u16Addr =  CSRB_ADC_REG_GAIN1;
	if( CS_CHAN_2 == u16Chan )
		u16Addr =  CSRB_ADC_REG_GAIN2;

	if( u16Gain > BUNNY_ADC_REG_SPAN )
		u16Gain = BUNNY_ADC_REG_SPAN;

	return WriteToAdcReg(u16Addr, u16Gain << 8);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsRabbitDev::SetAdcDelay(int16 i16Coarse, int16 i16Fine)
{
	if ( m_bCobraMaxPCIe )
	{
		if ( m_pCardState->b14G8 )
		{
			int32 i32Ret = WriteToAdcReg(CBMAX_ADC_REG_CLKPHASE_COARSE, (i16Coarse<<10));
			if( CS_FAILED(i32Ret) )
				return i32Ret;
			return WriteToAdcReg(CBMAX_ADC_REG_CLKPHASE_FINE, (i16Fine<<7));
		}
		else
		{
			int32 i32Ret = WriteToAdcReg(CBMAX_ADC_REG_CLKP_COARSE, (i16Coarse<<10));
			if( CS_FAILED(i32Ret) )
				return i32Ret;
			return WriteToAdcReg(CBMAX_ADC_REG_CLKP_FINE, (i16Fine<<8));
		}
	}
	else
	{
		if( 0 != (m_pCardState->AddOnReg.AdcReg.DES & RB_ADC_DESENABLE_AUTO) )
			return CS_FALSE;

		if ( (i16Coarse * i16Fine) < 0 )
			return CS_FALSE;

		uInt16 u16Data= m_pCardState->AddOnReg.AdcReg.CoarseDly & ~RB_ADC_COARSE_BITS_MASK;
		if( i16Coarse <= 0 && i16Fine < 0 )
		{
			u16Data |= RB_ADC_DESCOARSE_SIGN;
		}

		if( i16Coarse < 0 )
			i16Coarse = -i16Coarse;
		if( i16Fine < 0 )
			i16Fine = -i16Coarse;
		if( i16Coarse > BUNNY_ADC_COARSE_SPAN )
			i16Coarse = BUNNY_ADC_COARSE_SPAN;
		if( i16Fine > BUNNY_ADC_REG_SPAN )
			i16Fine = BUNNY_ADC_REG_SPAN;
		
		u16Data |= (i16Coarse<<10);

		int32 i32Ret = WriteToAdcReg(CSRB_ADC_REG_COARSE, u16Data);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
		return WriteToAdcReg(CSRB_ADC_REG_FINE, (i16Fine<<8));
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsRabbitDev::SetAdcMode( bool bUseI, bool bPingPong, bool bAuto )
{
	int32	i32Ret = CS_SUCCESS;

	uInt32	u32ClkCtrl = bPingPong ? 0 : CSRB_NOT_INTERLEAVED_BT;
	i32Ret = WriteRegisterThruNios(u32ClkCtrl,  CSRB_CLK_CTRL);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	if ( ! m_bCobraMaxPCIe )
	{
		uInt16	u16Data = RB_ADC_DES_MASK;

		if( bPingPong )
			u16Data |= RB_ADC_DESENABLE_PINGPONG;
		if( bAuto )
			u16Data |= RB_ADC_DESENABLE_AUTO;

		int32 i32Ret = WriteToAdcReg(CSRB_ADC_REG_DES, u16Data);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		u16Data = m_pCardState->AddOnReg.AdcReg.CoarseDly & RB_ADC_COARSE_MASK;
		if( !bUseI )
			u16Data |= RB_ADC_DESCOARSE_INPUTQ;

		i32Ret = WriteToAdcReg(CSRB_ADC_REG_COARSE, u16Data);
	}

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::ResetMasterSlave()
{
	int32 i32Ret = CS_SUCCESS;

	// This function is only for master card ...
	if ( ermMaster != m_pCardState->ermRoleMode )
		return i32Ret;

	uInt16	u16Data = m_pCardState->AddOnReg.u16MasterSlave;

	u16Data |= (RB_MS_RESET |RB_STATE_MACHINE_RESET);
	i32Ret = WriteRegisterThruNios(u16Data, CSRB_MS_REG_WR);
	if( CS_FAILED(i32Ret) )
		return i32Ret;
		
	u16Data &= ~(RB_MS_RESET |RB_STATE_MACHINE_RESET);
	i32Ret = WriteRegisterThruNios(u16Data, CSRB_MS_REG_WR);

	BBtiming(1000);

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::ResetMSDecimationSynch()
{
	int32 i32Ret = CS_SUCCESS;

	// This function is only for master card ...
	if ( ermMaster != m_pCardState->ermRoleMode )
		return i32Ret;

	uInt16	u16Data = m_pCardState->AddOnReg.u16MasterSlave;

//	u16Data |= (RB_MS_DECSYNCH_RESET | RB_STATE_MACHINE_RESET);
	u16Data |= RB_MS_DECSYNCH_RESET;
	i32Ret = WriteRegisterThruNios(u16Data, CSRB_MS_REG_WR);
	if( CS_FAILED(i32Ret) )
		return i32Ret;
		
//	u16Data &= ~(RB_MS_DECSYNCH_RESET | RB_STATE_MACHINE_RESET);
	u16Data &= ~RB_MS_DECSYNCH_RESET;
	i32Ret = WriteRegisterThruNios(u16Data, CSRB_MS_REG_WR);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	BBtiming(1000);

	m_MasterIndependent->m_pCardState->bMsDecimationReset = FALSE;
	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsRabbitDev::GeneratePulseCalib()
{
	// Toggle CSRB_LOCAL_SINGLEPULSE_ENABLE bit
	WriteRegisterThruNios(m_pCardState->AddOnReg.u16Calib, CSRB_SET_CALIB);
	WriteRegisterThruNios(m_pCardState->AddOnReg.u16Calib, CSRB_SET_CALIB);		// just for delay
	WriteRegisterThruNios(m_pCardState->AddOnReg.u16Calib, CSRB_SET_CALIB);		// just for delay
	return  WriteRegisterThruNios(m_pCardState->AddOnReg.u16Calib | CSRB_LOCAL_SINGLEPULSE_ENABLE, CSRB_SET_CALIB);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void	CsRabbitDev::InitBoardCaps()
{
	uInt32 u32BoardType = m_PlxData->CsBoard.u32BoardType & ~CSNUCLEONBASE_BOARDTYPE;

	if ( CSX24G8_BOARDTYPE == u32BoardType )
	{
		m_pCardState->u32SrInfoSize = sizeof(CobraMax_2_4) / sizeof(RABBIT_SAMPLE_RATE_INFO);
		RtlCopyMemory( m_pCardState->SrInfoTable, CobraMax_2_4, sizeof(CobraMax_2_4) );
	}
	else if ( CSX14G8_BOARDTYPE == u32BoardType )
	{
		m_pCardState->u32SrInfoSize = sizeof(CobraMax_1_4) / sizeof(RABBIT_SAMPLE_RATE_INFO);
		RtlCopyMemory( m_pCardState->SrInfoTable, CobraMax_1_4, sizeof(CobraMax_1_4) );
	}
	else if ( CS22G8_BOARDTYPE == u32BoardType || LAB11G_BOARDTYPE == u32BoardType )
	{
		m_pCardState->u32SrInfoSize = sizeof(Rabit_2_2) / sizeof(RABBIT_SAMPLE_RATE_INFO);
		RtlCopyMemory( m_pCardState->SrInfoTable, Rabit_2_2, sizeof(Rabit_2_2) );
	}
	else if ( CS21G8_BOARDTYPE == u32BoardType )
	{
		m_pCardState->u32SrInfoSize = sizeof(Rabit_2_1) / sizeof(RABBIT_SAMPLE_RATE_INFO);
		RtlCopyMemory( m_pCardState->SrInfoTable, Rabit_2_1, sizeof(Rabit_2_1) );
	}
	else
	{
		m_pCardState->u32SrInfoSize = sizeof(Rabit_1_1) / sizeof(RABBIT_SAMPLE_RATE_INFO);
		RtlCopyMemory( m_pCardState->SrInfoTable, Rabit_1_1, sizeof(Rabit_1_1) );
	}

	if ( m_pCardState->bHighBandwidth )
	{
		// 1GHz banwith cards
		m_pCardState->u32FeInfoSize = c_RB_FE_INFO_SIZE_DIRECTADC;
		RtlCopyMemory( m_pCardState->FeInfoTable, c_FrontEndRabitGainDirectADC, sizeof( c_FrontEndRabitGainDirectADC ) );
	}
	else
	{
		if ( m_bNucleon && ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_SONOSCAN) ) && m_bSonoscan_LimitedRanges )
		{
			// Sonoscan cards, limit the number of input ranges
			m_pCardState->u32FeInfoSize = 0;
			for ( int i = 0, j = 0; i < c_RB_FE_INFO_SIZE; i++ )
			{
				// Only 2000mV and 4000mV range are available
				if (( 2000 == c_FrontEndRabitGain50[i].u32Swing_mV) || (4000 == c_FrontEndRabitGain50[i].u32Swing_mV) )
				{
					RtlCopyMemory( &m_pCardState->FeInfoTable[j++], &c_FrontEndRabitGain50[i], sizeof(FRONTEND_RABBIT_INFO) );
					m_pCardState->u32FeInfoSize ++;
				}
			}
		}
		else
		{
			// Regular Rabbit cards
			m_pCardState->u32FeInfoSize = c_RB_FE_INFO_SIZE;
			RtlCopyMemory( m_pCardState->FeInfoTable, c_FrontEndRabitGain50, sizeof( c_FrontEndRabitGain50 ) );
		}
	}
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void CsRabbitDev::UpdateCalibInfoStructure(uInt16 u16DacAddr, uInt16 u16Data)
{	
	uInt16 u16ChanZeroBased = 0;
	uInt32 u32Index;

	if ( u16DacAddr < BUNNY_2_CHAN_DAC )
		u16ChanZeroBased = 0;
	else
		u16ChanZeroBased = 1;

	FindFeIndex( m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange, u32Index );

	switch ( u16DacAddr )
	{
		case BUNNY_COARSE_OFF_1:
			m_pCardState->DacInfo[0][u32Index]._NullOffset = u16Data;
			break;
		case BUNNY_FINE_OFF_1:
			break;
		case BUNNY_POSITION_1:
			m_pCardState->DacInfo[0][u32Index]._FeOffset = u16Data;
			break;
		case BUNNY_COARSE_OFF_2:
			m_pCardState->DacInfo[1][u32Index]._NullOffset = u16Data;
			break;
		case BUNNY_FINE_OFF_2:
			break;
		case BUNNY_POSITION_2:
			m_pCardState->DacInfo[1][u32Index]._FeOffset = u16Data;
			break;
		case BUNNY_ADC_OFFSET_1:
			m_pCardState->DacInfo[0][u32Index]._AdcOff = u16Data;;
			break;
		case BUNNY_ADC_OFFSET_2:
			m_pCardState->DacInfo[1][u32Index]._AdcOff = u16Data;;
			break;
		case BUNNY_ADC_GAIN_1:
			m_pCardState->DacInfo[0][u32Index]._Gain = u16Data;;
			break;
		case BUNNY_ADC_GAIN_2:
			m_pCardState->DacInfo[1][u32Index]._Gain = u16Data;
			break;
	}

}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsRabbitDev::WriteToDac(uInt16 u16Addr, uInt16 u16Data, uInt8 u8Delay, BOOLEAN bUpdateDacInfo )
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32DacAddr = u16Addr;
	uInt32	u32Data = u8Delay;
	BOOLEAN	bCtrlADC = FALSE;


	u32Data <<= 16;
	u32Data |= u16Data;
	
	switch ( u16Addr )
	{
		case BUNNY_COARSE_OFF_1:
			u32DacAddr = (RB_CAL_OFFSET_1 | RB_DAC7552_OUTA);
				break;
		case BUNNY_FINE_OFF_1:
			u32DacAddr = (RB_CAL_OFFSET_1 | RB_DAC7552_OUTB);
			break;
		case BUNNY_POSITION_1:
			u32DacAddr = (RB_USER_OFFSET_CS | RB_DAC7552_OUTA);
			break;
		case BUNNY_POSITION_2:
			u32DacAddr = (RB_USER_OFFSET_CS | RB_DAC7552_OUTB);
			break;
		case BUNNY_COARSE_OFF_2:
			u32DacAddr = (RB_CAL_OFFSET_2 | RB_DAC7552_OUTA);
			break;
		case BUNNY_FINE_OFF_2:
			u32DacAddr = (RB_CAL_OFFSET_2 | RB_DAC7552_OUTB);
			break;
		case BUNNY_EXTRIG_LEVEL_HIGH:
			u32DacAddr = (BR_CAL_DACEXTTRIG_NCS | RB_DAC7552_OUTA);
			break;
		case BUNNY_EXTRIG_LEVEL_LOW:
			u32DacAddr = (BR_CAL_DACEXTTRIG_NCS | RB_DAC7552_OUTB);
			break;
		case BUNNY_CAL_DC:
			u32DacAddr = (RB_CAL_SRC | RB_DAC7552_OUTA);
			break;
		case BUNNY_VAR_REF:
			u32DacAddr = (RB_CAL_SRC | RB_DAC7552_OUTB);
			break;
		case BUNNY_DELAY_MS:
			u32DacAddr = (RB_MS_DELAY | RB_DAC7552_OUTB);
			break;
		case BUNNY_ADC_OFFSET_1:
			bCtrlADC = TRUE;
			i32Ret =  SetAdcOffset( CS_CHAN_1, int16(u16Data) );
			break;
		case BUNNY_ADC_OFFSET_2:
			bCtrlADC = TRUE;
			i32Ret =  SetAdcOffset( CS_CHAN_2, int16(u16Data) );
			break;
		case BUNNY_ADC_GAIN_1:
			bCtrlADC = TRUE;
			i32Ret =  SetAdcGain( CS_CHAN_1, u16Data );
			break;
		case BUNNY_ADC_GAIN_2:
			bCtrlADC = TRUE;
			i32Ret =  SetAdcGain( CS_CHAN_2, u16Data );
			break;
		default:
			return CS_INVALID_DAC_ADDR;
	}

	if( CS_FAILED(i32Ret) )
		return i32Ret;

	if ( ! bCtrlADC )
	{
		i32Ret = WriteRegisterThruNios(u32Data, CSRB_WRITE_TO_DAC7552_CMD | u32DacAddr);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}
	
	if ( bUpdateDacInfo ) 
		UpdateCalibInfoStructure(u16Addr, u16Data);

	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsRabbitDev::WriteToCalibCtrl(eBunnyCalCtrl eccId, uInt16 u16HwChannel, int16 i16Data, uInt8 u8Delay)
{
	if ( CS_CHAN_1 > u16HwChannel || CS_CHAN_2 < u16HwChannel )
		return CS_INVALID_CHANNEL;

	uInt16 u16ChanZeroBased = u16HwChannel - 1;
	uInt32 u32Index;
	int32 i32Ret = FindFeIndex( m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange, u32Index );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	PBUNNYCHANCALIBINFO pDacInfo = &(m_pCardState->DacInfo[u16ChanZeroBased][u32Index]);
	uInt16 u16DacId(0);
	
	switch ( eccId )
	{
		case eccNullOffset: 
			i16Data = i16Data < 0 ? 0 : (i16Data >= c_u16CalDacCount ? c_u16CalDacCount-1 : i16Data );
			u16DacId = BUNNY_COARSE_OFF_1; pDacInfo->_NullOffset=i16Data; break;
		case eccFEOffset:   
			i16Data = i16Data < 0 ? 0 : (i16Data >= c_u16CalDacCount ? c_u16CalDacCount-1 : i16Data );
			u16DacId = BUNNY_POSITION_1;   pDacInfo->_FeOffset=i16Data;   break;
		case eccAdcGain:    
			i16Data = i16Data < 0 ? 0 : (i16Data >= BUNNY_ADC_REG_SPAN ? BUNNY_ADC_REG_SPAN-1 : i16Data );
			u16DacId = BUNNY_ADC_GAIN_1;   pDacInfo->_Gain=i16Data;       break;
		case eccAdcOffset: 
			i16Data = i16Data < -BUNNY_ADC_REG_SPAN ? -BUNNY_ADC_REG_SPAN : (i16Data > BUNNY_ADC_REG_SPAN ? BUNNY_ADC_REG_SPAN : i16Data );
			u16DacId = BUNNY_ADC_OFFSET_1; pDacInfo->_AdcOff=i16Data;     break;
		case eccPosition: 
			u16DacId = BUNNY_POSITION_1; 
			i16Data = i16Data + pDacInfo->_FeOffset; 
			i16Data = i16Data < 0 ? 0 : (i16Data >= c_u16CalDacCount ? c_u16CalDacCount-1 : i16Data );
			break;
	
		default: return CS_INVALID_DAC_ADDR;
	}
	if( CS_CHAN_2 == u16HwChannel  )
		u16DacId += BUNNY_2_CHAN_DAC;
	
	TraceCalib( TRACE_DAC, uInt16(i16Data), u16DacId, 0, 0);

	i32Ret = WriteToDac(u16DacId, uInt16(i16Data), u8Delay);
	return i32Ret;
}


///////////////////////////////////////////////
// Measurment loop
// Reference voltage of U3 depends on range of calibration voltages , we assume that swing of the calibration signal 
// is equal to front end swing.
// For U55:
// V+ = Vin * R26/(R28+R26) + Vvr * R28/(R28+R26)
// V- = Vvr * R22/(R20+R22)
// Designate Alpha = R22 / (R20+R22) and Beta = R26 / (R28+R26)
// DeltaV = V+ - V- = Vin*Beta + Vvr* (1 - Beta - Alpha), asuuming that Alpha + Beta = 1
// DeltaV = Vin * Beta
// CodeU3 = 2* DeltaV / (Vvr*Alpha)* M = Vin/Vvr * Beta/Alpha * M/2, or
// Vin = Vvr * CodeU3/M * 2*Alpha/Beta
//
// Note. Vvr is stored in  m_u32VarRefLevel_uV
// Therefore 
// Vin = 2 * Alpha * m_u32VarRefLevel_uV * CodeU3/(M * Beta)
// assuming A = 2 * Alpha and B = M*Beta
// A = 2 * R22 / (R20+R22)
// B = M * R26 / (R28+R26)
// Vin = A * m_u32VarRefLevel_uV * CodeU3 / B

const uInt16 CsRabbitDev::c_u16R20 = 1000;
const uInt16 CsRabbitDev::c_u16R22 = 1000;

const LONGLONG	cllFactor = 2;
const uInt32 CsRabbitDev::c_u32A_scaled = uInt32((cllFactor*CsRabbitDev::c_u32ScaleFactor*CsRabbitDev::c_u16R22)/(CsRabbitDev::c_u16R20+CsRabbitDev::c_u16R22));
const uInt32 CsRabbitDev::c_u32B_scaled = uInt32((LONGLONG(CsRabbitDev::c_u32ScaleFactor)*CsRabbitDev::c_u16CalAdcCount*CsRabbitDev::c_u16R26)/(CsRabbitDev::c_u16R28+CsRabbitDev::c_u16R26));

int32 CsRabbitDev::ReadCalibA2D(int32* pi32Mean, int32* pi32MeanSquare )
{
	ASSERT( uInt32(c_u16R26)*(c_u16R20+c_u16R22) + uInt32(c_u16R22)*(c_u16R28+c_u16R26) == uInt32(c_u16R28+c_u16R26)*uInt32(c_u16R20+c_u16R22) );

	WriteGIO(CSRB_AD7450_WRITE_REG,CSRB_AD7450_RESTART);
   Sleep(10);     // Add delay (read CSRB_AD7450_READ_REG may fail without delay)
	WriteGIO(CSRB_AD7450_WRITE_REG,0 );
   Sleep(10);     // Add delay
	
	uInt32 u32Code(0);
	CSTIMER	CsTimeout;
	CsTimeout.Set( CSRB_CALIB_TIMEOUT_SHORT );
	for(;;)
	{
		u32Code = ReadGIO(CSRB_AD7450_READ_REG);
		if( 0 != (u32Code & CSRB_AD7450_RD_FIFO_FULL) )
			break;
		else
		{
			if ( CsTimeout.State() )
			{
				u32Code = ReadGIO(CSRB_AD7450_READ_REG);
				if( 0 != (u32Code & CSRB_AD7450_RD_FIFO_FULL) )
					break;
				else
				{
					return CS_CALIB_ADC_CAPTURE_FAILURE;
				}
			}
			else
				BBtiming(1);
		}
	}
	
	int	nValidSamples = 0;
	int i = 0;
	for( i = 0; i < CSRB_CALIB_BLOCK_SIZE; i++ )
	{
		WriteGIO(CSRB_AD7450_WRITE_REG,CSRB_AD7450_READ);
		WriteGIO(CSRB_AD7450_WRITE_REG,0 );
		u32Code = ReadGIO(CSRB_AD7450_READ_REG);
		if( 0 == (u32Code & CSRB_AD7450_DATA_READ) )
			continue;
				
		int16 i16Code = int16(u32Code & 0xfff);
		//sign extention
		i16Code <<= 4;
		i16Code >>= 4;

		m_pi32CalibA2DBuffer[nValidSamples++] = i16Code;
	}

	if ( nValidSamples < 2 )
		return CS_CALIB_ADC_READ_FAILURE;

	int32 i32Avg = 0;
	for( i = 0; i < nValidSamples; i++ )
	{
		i32Avg += m_pi32CalibA2DBuffer[i];
	}

	// Vin = A * m_u32VarRefLevel_uV * CodeU3 / B
	LONGLONG llCode = i32Avg;
	llCode = llCode * LONGLONG(m_u32VarRefLevel_uV) * c_u32A_scaled;
	llCode += llCode > 0 ? LONGLONG(c_u32B_scaled)*LONGLONG(nValidSamples)/2 : - LONGLONG(c_u32B_scaled)*LONGLONG(nValidSamples)/2;
	llCode = llCode / (LONGLONG(c_u32B_scaled)*LONGLONG(nValidSamples));
	
	*pi32Mean = int32(llCode)+m_i32CalAdcOffset_uV;

	if( NULL != pi32MeanSquare )
	{
		LONGLONG llMeanSquare = 0;
		for( i = 0; i < nValidSamples; i++ )
		{
			LONGLONG llT = m_pi32CalibA2DBuffer[i] - i32Avg;
			llMeanSquare += llT*llT;
		}

		llMeanSquare *= 10;
		llMeanSquare += llMeanSquare > 0 ? LONGLONG(nValidSamples/2) : -LONGLONG(nValidSamples/2);
		llMeanSquare /= LONGLONG(nValidSamples);
		
		llCode = llMeanSquare/10;
		llCode *= LONGLONG(m_u32VarRefLevel_uV) * LONGLONG(m_u32VarRefLevel_uV) * c_u32A_scaled * c_u32A_scaled;
		llCode /=  LONGLONG(c_u32B_scaled) * LONGLONG(c_u32B_scaled);
		
		*pi32MeanSquare = int32(llCode);
	}
	return CS_SUCCESS;
}

int32 CsRabbitDev::SendCalibDC(int32 i32Target_mV, bool bTerm, int32* pVoltage_uV, uInt16* pu16InitialCode )
{
	int32 i32Ret = CS_SUCCESS;
	if( 0 == m_u32CalibRange_mV ) return i32Ret;
	//Calculate target correction
	int32 i32Target_uV = i32Target_mV*1000;
	
	//Calculate load
	int32 i32Range_uV = (int32)m_u32CalibSwing_uV;
	if( !bTerm )
	{
		i32Range_uV *= int32(c_u16R19 + CS_REAL_IMP_50_OHM);
		i32Range_uV /= CS_REAL_IMP_50_OHM;
	}

	if( i32Target_uV > i32Range_uV )
		i32Target_uV = i32Range_uV;

	if( i32Target_uV < -i32Range_uV )
		i32Target_uV = -i32Range_uV;
	
	uInt16 u16Code(0);
	if( NULL == pu16InitialCode || *pu16InitialCode >= c_u16CalDacCount )
	{
		LONGLONG llCode = -i32Target_uV;
		llCode *= c_u16CalDacCount/2;
		llCode /= i32Range_uV;
		llCode += c_u16CalDacCount/2;
		u16Code = llCode < 0 ? 0 : (llCode > c_u16CalDacCount-1 ? c_u16CalDacCount-1 : uInt16(llCode));
	}
	else
	{
		u16Code = *pu16InitialCode;
	}

	int32 i32Signal_uV(0);
	i32Ret = WriteToDac(BUNNY_CAL_DC, u16Code, m_u8CalDacSettleDelay);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = ReadCalibA2D(&i32Signal_uV);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	TraceCalib( TRACE_CAL_SIGNAL_ITER, u16Code, 0, i32Target_uV, i32Signal_uV );

	int32 i32NewDiff, i32Diff = i32Signal_uV - i32Target_uV;	

	int16 i16Correction(0), i16PrevCorrection(0);	
	uInt8 u8Iter(0);
	int32 i32Scale = 2*i32Range_uV; 
	for( u8Iter = 1; u8Iter <= c_u16MaxIterations; u8Iter++ )	
	{
		LONGLONG llCor = i32Diff * LONGLONG(c_u16CalDacCount);
		llCor /= i32Scale;
		i16Correction = int16(llCor);
		if( 0 == i16Correction )
			i16Correction = i32Diff > 0 ? 1 : -1;
		
		uInt16 u16PrevCode = u16Code;
		int16 i16NewCode = int16(u16Code) +i16Correction;
		u16Code = i16NewCode < 0 ? 0 : ( i16NewCode > c_u16CalDacCount-1 ? c_u16CalDacCount - 1 : uInt16(i16NewCode) );

		if( u16PrevCode == u16Code ) //Only if we are at the limits 
			return CS_CALIB_DAC_OUT_OF_RANGE;
	
		i32Ret = WriteToDac(BUNNY_CAL_DC, u16Code, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = ReadCalibA2D(&i32Signal_uV);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		TraceCalib( TRACE_CAL_SIGNAL_ITER, u16Code, u8Iter, i32Target_uV, i32Signal_uV );

		i32NewDiff = i32Signal_uV - i32Target_uV;

		if( labs(i32NewDiff) > labs(i32Diff) && (i32NewDiff*i32Diff) < 0 )
		{
			i32Scale *= 2;
		}

		if( i16PrevCorrection*i16Correction == -1 )
		{
			if( labs(i32NewDiff) > labs(i32Diff) )
			{
				i32Ret = WriteToDac(BUNNY_CAL_DC, u16PrevCode, m_u8CalDacSettleDelay);
				if( CS_FAILED(i32Ret) )	return i32Ret;
				i32Ret = ReadCalibA2D(&i32Signal_uV);
				if( CS_FAILED(i32Ret) )	return i32Ret;
				TraceCalib( TRACE_CAL_SIGNAL_ITER, u16PrevCode, ++u8Iter, i32Target_uV, i32Signal_uV );

				i32NewDiff = i32Signal_uV - i32Target_uV;
			}
			break;
		}
	
		i16PrevCorrection = i16Correction;
		i32Diff = i32NewDiff;
	}

	TraceCalib( TRACE_CAL_SIGNAL, u16Code, u8Iter, i32Target_uV, i32Signal_uV );

	const int32 ci32MaxError = int32(( CS_REAL_IMP_50_OHM * m_u32VarRefLevel_uV) /(int32(c_u16R19 + CS_REAL_IMP_50_OHM) * 100));
	if (u8Iter >= c_u16MaxIterations || labs(i32Diff) > ci32MaxError)
	{
		return CS_DAC_CALIB_FAILURE;
	}

	if( NULL != pu16InitialCode ) 
		*pu16InitialCode = u16Code;
	
	if( NULL != pVoltage_uV )
	{
		*pVoltage_uV = i32Signal_uV;
	}
	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

const uInt32 CsRabbitDev::c_u32MaxPositionOffset_mV = 2000;

int32 CsRabbitDev::SendPosition(uInt16 u16HwChannel, int32 i32Position_mV)
{
	if( (0 == u16HwChannel) || (u16HwChannel > CSRB_CHANNELS)  )
	{
		return CS_INVALID_CHANNEL;
	}

	uInt16 u16ChanZeroBased = u16HwChannel - 1;
	if( m_bNeverCalibrateDc )
	{
		m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = 0;
		return CS_SUCCESS;
	}
	if( i32Position_mV >  int32(c_u32MaxPositionOffset_mV) || 
		i32Position_mV < -int32(c_u32MaxPositionOffset_mV) ||
		i32Position_mV >  int32(m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange) || 
		i32Position_mV < -int32(m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange) )
	{
		m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = 0;
		return CS_INVALID_POSITION;
	}
	// m_pCardState->DacInfo[u16ChanZeroBased][u32Index]._Pos corresponds to 50% of FS
	// we calculate Dac control delta assuming linear relationship
	uInt32 u32Index;
	int32 i32Ret = FindFeIndex( m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange, u32Index );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	if( 0 == m_pCardState->DacInfo[u16ChanZeroBased][u32Index]._Pos )
	{ //Something did not work or we are using defaults
		m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = 0;
		return CS_FALSE;
	}

	int32 i32DacDelta = m_pCardState->DacInfo[u16ChanZeroBased][u32Index]._Pos; 
	i32DacDelta *= i32Position_mV;
	i32DacDelta /= int32(m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange/4);

	int32 i32DacData = i32DacDelta + m_pCardState->DacInfo[u16ChanZeroBased][u32Index]._FeOffset; 

	i32DacData = i32DacData < -c_u16CalDacCount ? -c_u16CalDacCount : (i32DacData >= c_u16CalDacCount ? c_u16CalDacCount-1 : int16(i32DacData) );
	int16 i16DacDelta = int16(i32DacData - m_pCardState->DacInfo[u16ChanZeroBased][u32Index]._FeOffset);

	if( i32DacDelta != i16DacDelta )
	{
		int32 i32NewPosition_mv = int32(m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange/4);
		i32NewPosition_mv *= i16DacDelta;
		i32NewPosition_mv += i32NewPosition_mv * m_pCardState->DacInfo[u16ChanZeroBased][u32Index]._Pos > 0 ? m_pCardState->DacInfo[u16ChanZeroBased][u32Index]._Pos/2:-m_pCardState->DacInfo[u16ChanZeroBased][u32Index]._Pos/2;
		i32NewPosition_mv /= m_pCardState->DacInfo[u16ChanZeroBased][u32Index]._Pos;

		m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = i32NewPosition_mv;

		if ( m_bErrorOnOffsetSaturation[u16ChanZeroBased] )
		{
			m_bErrorOnOffsetSaturation[u16ChanZeroBased] = false;

			char szErrorStr[128];
			sprintf_s( szErrorStr, sizeof(szErrorStr), "to %d mV on channel %d", i32NewPosition_mv, ConvertToUserChannel(u16HwChannel) );
			CsEventLog EventLog;
			EventLog.Report( CS_WARN_DC_OFFSET, szErrorStr );
		}

	}
	else
	{
		m_bErrorOnOffsetSaturation[u16ChanZeroBased] = true;
	}

	i32Ret = WriteToCalibCtrl(eccPosition, u16HwChannel, int16(i32DacDelta), m_u8CalDacSettleDelay);
	if( CS_FAILED(i32Ret) )	return false;

	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsRabbitDev::SendAdcSelfCal()
{
	int32 i32Ret = WriteRegisterThruNios(0, CSRB_SET_ADC);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	BBtiming(1);

	i32Ret = WriteRegisterThruNios(CSRB_ADC_CAL_ACTIVATE, CSRB_SET_ADC);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	BBtiming(10);
	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsRabbitDev::DetectMasterSlave(int16 *i16NumMasterSlaveSystems, int16 *i16NumStandAloneSystems)
{
	CsRabbitDev* pHead	= (CsRabbitDev *) m_ProcessGlblVars->pFirstUnCfg;
	CsRabbitDev* pCurrent = pHead;
	bool	         bFoundMaster = false;

	uInt32	u32Data;

	*i16NumStandAloneSystems = 0;
	*i16NumMasterSlaveSystems = 0;

	// First pass. Search for master cards
	while( NULL != pCurrent )
	{
		// Skip all boards that have errors on Nios init or AddOn connection
		if( pCurrent->m_PlxData->InitStatus.Info.u32Nios == 0 ||
			pCurrent->m_PlxData->InitStatus.Info.u32AddOnPresent == 0)
		{
			TRACE(DISP_TRACE_LEVEL, "DetectMasterSlave continue\n");
			pCurrent = pCurrent->m_Next;
			continue;
		}
		
		// Search for Master card
		u32Data = pCurrent->ReadRegisterFromNios( 0, CSRB_MS_REG_RD );

		TRACE(DISP_TRACE_LEVEL, "CardID register = %i\n", u32Data);

		// !! To be retested....
		// pCurrent->m_pCardState->u8MsCardId = (((uInt8) (u32Data >>10)) & 0x7 ) >> 2 ;
		pCurrent->m_pCardState->u8MsCardId = (((uInt8) (u32Data >>10)) & 0x7 );

		TRACE(DISP_TRACE_LEVEL, "(2) DetectMasterSlave CardID = %i, PulseCalib = %i\n", pCurrent->m_pCardState->u8MsCardId, pCurrent-> m_bPulseCalib);	
		if ( 0 == pCurrent->m_pCardState->u8MsCardId && pCurrent-> m_bPulseCalib )
		{
			TRACE(DISP_TRACE_LEVEL, "Master board found\n");
			bFoundMaster = true;
			pCurrent->SetRoleMode(ermMaster);
		}
		pCurrent = pCurrent->m_Next;
	}
	// If Master card not found, it is not worth to go further
	// All cards will behave as stand alone cards
	if ( !bFoundMaster )
	{
		pCurrent = pHead;
		while( NULL != pCurrent )
		{
			// Skip all boards that have errors on Nios init or AddOn connection
			if( pCurrent->m_PlxData->InitStatus.Info.u32Nios != 0 && 
				pCurrent->m_PlxData->InitStatus.Info.u32AddOnPresent != 0 )
			{
				(*i16NumStandAloneSystems)++;
			}
			pCurrent = pCurrent->m_Next;
		}
		return;
	}

	// Second pass. Enable MS BIST
	pCurrent = pHead;
	while( NULL != pCurrent )
	{
		// Skip all boards that have errors on Nios init or AddOn connection
		if( pCurrent->m_PlxData->InitStatus.Info.u32Nios == 0 ||
			pCurrent->m_PlxData->InitStatus.Info.u32AddOnPresent == 0 )
		{
			pCurrent = pCurrent->m_Next;
			continue;
		}
		pCurrent->EnableMsBist(true);
		pCurrent = pCurrent->m_Next;
	}
	
	// Third pass. Find slaves that belong to the master
	pCurrent = pHead;
	while( NULL != pCurrent )
	{
		// Skip all boards that have errors on Nios init or AddOn connection
		if( pCurrent->m_PlxData->InitStatus.Info.u32Nios == 0 ||
			pCurrent->m_PlxData->InitStatus.Info.u32AddOnPresent == 0 )
		{
			pCurrent = pCurrent->m_Next;
			continue;
		}

		if( ermMaster == pCurrent->m_pCardState->ermRoleMode  )
		{
			if( CS_SUCCEEDED(pCurrent->DetectSlaves()) )
			{
				TRACE(DISP_TRACE_LEVEL, "Slaves detected\n");
				(*i16NumMasterSlaveSystems)++;
			}
			else
			{
				TRACE(DISP_TRACE_LEVEL, "No slave - Standalone\n");
				pCurrent->SetRoleMode(ermStandAlone);
			}
		}
		pCurrent = pCurrent->m_Next;
	}
	// Third pass. Count remaining stand alone cards
	// and disable the bist
	pCurrent = pHead;
	while( NULL != pCurrent )
	{
		// Skip all boards that have errors on Nios init or AddOn connection
		if( pCurrent->m_PlxData->InitStatus.Info.u32Nios == 0 ||
			pCurrent->m_PlxData->InitStatus.Info.u32AddOnPresent == 0 )
		{
			pCurrent = pCurrent->m_Next;
			continue;
		}
		if( pCurrent->m_pCardState->ermRoleMode == ermStandAlone )
			(*i16NumStandAloneSystems)++;
		pCurrent->EnableMsBist(false);
		pCurrent = pCurrent->m_Next;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::DetectSlaves()
{
	CsRabbitDev* pHead	= (CsRabbitDev *) m_ProcessGlblVars->pFirstUnCfg;
	CsRabbitDev* pCurrent = pHead;
	CsRabbitDev* pMS = this;
	CsRabbitDev* pMSTable[8] = {NULL};
	char			 szText[128] = {0};
	int				 char_i = 0;
	uInt32           u32Data;

	if ( 0 != m_pCardState->u8MsCardId )
	{
		TRACE(DISP_TRACE_LEVEL, "Slave detection error CardID (%i)\n", m_pCardState->u8MsCardId);
		return CS_FALSE;
	}

	pMSTable[0] = this;			// Master candiate
	szText[char_i++] = '0';
	//Configure master tranceivers
	m_pCardState->AddOnReg.u16MsBist1 = CSRB_BIST_EN|CSRB_BIST_DIR1;
	int32 i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist1, CSRB_MS_BIST_CTL1_WR);
	if( CS_FAILED(i32Ret) )	
	{
		TRACE(DISP_TRACE_LEVEL, "Slave detection error 1 (%i)\n", i32Ret);
		return i32Ret;
	}
	
	m_pCardState->AddOnReg.u16MsBist2 = 0;
	i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSRB_MS_BIST_CTL2_WR);
	if( CS_FAILED(i32Ret) )	
	{
		TRACE(DISP_TRACE_LEVEL, "Slave detection error 2 (%i)\n", i32Ret);
		return i32Ret;
	}

	// Detect all slave card conntected to the master
	pCurrent = pHead;
	while( NULL != pCurrent )
	{
		if( pCurrent->m_pCardState->u8MsCardId  != 0 && 
			pCurrent->m_PlxData->CsBoard.u32BoardType == this->m_PlxData->CsBoard.u32BoardType)
		{
			i32Ret = pCurrent->SetRoleMode(ermSlave);
			if( CS_FAILED(i32Ret) )	
			{
				TRACE(DISP_TRACE_LEVEL, "Slave detection error 3 (%i)\n", i32Ret);
				return i32Ret;
			}

			u32Data = pCurrent->ReadRegisterFromNios(pCurrent->m_pCardState->AddOnReg.u16MsBist1, CSRB_MS_BIST_ST1_RD);
			if(  0 == ((u32Data & CSRB_BIST_MS_SLAVE_ACK_MASK) & (1 << (pCurrent->m_pCardState->u8MsCardId-1))) )
			{
				m_pCardState->AddOnReg.u16MsBist2 = 0x7FFF;
				i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSRB_MS_BIST_CTL2_WR);
				if( CS_FAILED(i32Ret) )	
				{
					TRACE(DISP_TRACE_LEVEL, "Slave detection error 4 (%i)\n", i32Ret);
					return i32Ret;
				}
	
				u32Data = pCurrent->ReadRegisterFromNios(pCurrent->m_pCardState->AddOnReg.u16MsBist1, CSRB_MS_BIST_ST1_RD);
				if(  0 != ((u32Data & CSRB_BIST_MS_SLAVE_ACK_MASK) & (1 << (pCurrent->m_pCardState->u8MsCardId-1))) )
				{
					m_pCardState->AddOnReg.u16MsBist2 = 0;
					i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSRB_MS_BIST_CTL2_WR);
					if( CS_FAILED(i32Ret) )
					{
						TRACE(DISP_TRACE_LEVEL, "Slave detection error 5 (%i)\n", i32Ret);
						return i32Ret;
					}
					
					u32Data = pCurrent->ReadRegisterFromNios(pCurrent->m_pCardState->AddOnReg.u16MsBist1, CSRB_MS_BIST_ST1_RD);
					if(  0 == ((u32Data & CSRB_BIST_MS_SLAVE_ACK_MASK) & (1 << (pCurrent->m_pCardState->u8MsCardId-1))) )
					{
						if ( pCurrent-> m_bPulseCalib )
							pMSTable[pCurrent->m_pCardState->u8MsCardId] = pCurrent;
						else
						{
							i32Ret = pCurrent->SetRoleMode(ermStandAlone);
							if( CS_FAILED(i32Ret) )
							{
								TRACE(DISP_TRACE_LEVEL, "Slave detection error 6 (%i)\n", i32Ret);
								return i32Ret;
							}
						}
						pCurrent = pCurrent->m_Next;
						continue;
					}
				}
				else
				{
					i32Ret = pCurrent->SetRoleMode(ermStandAlone);
					if( CS_FAILED(i32Ret) )
					{
						TRACE(DISP_TRACE_LEVEL, "Slave detection error 7 (%i)\n", i32Ret);
						return i32Ret;
					}

					m_pCardState->AddOnReg.u16MsBist2 = 0;
					i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSRB_MS_BIST_CTL2_WR);
					if( CS_FAILED(i32Ret) )
					{
						TRACE(DISP_TRACE_LEVEL, "Slave detection error 8 (%i)\n", i32Ret);
						return i32Ret;
					}
				}
			}
			else
			{
				// A stand alone card (m_u8MScardId = 7) could be a slave card with a bad contection with M/S bridge
				i32Ret = pCurrent->SetRoleMode(ermStandAlone);
				if( CS_FAILED(i32Ret) )
				{
					TRACE(DISP_TRACE_LEVEL, "Slave detection error 9 (%i)\n", i32Ret);
					return i32Ret;
				}
			}
		}
		pCurrent = pCurrent->m_Next;
	}

	// Re-organize slave cards
	pMS =this;
	for (int i = 1; i <= 7; i++)
	{
		if ( NULL != pMSTable[i] )
		{
			pMS->m_pNextInTheChain = pMSTable[i];
			pMS = pMSTable[i];
			m_pCardState->u8SlaveConnected |= (1 << (pMS->m_pCardState->u8MsCardId-1));
			
			szText[char_i++] = ',';
			szText[char_i++] = (char)('0' + i );
		}
	}

	int32 i32Status = TestMsBridge();
	if (CS_FAILED(i32Status))
	{
		CsRabbitDev* pTemp;
		// Undo master/slave
		m_pCardState->u8SlaveConnected = 0;
		pCurrent = this;
		while( pCurrent )
		{
			pCurrent->SetRoleMode(ermStandAlone);
			pTemp		= pCurrent;
			pCurrent	= pCurrent->m_pNextInTheChain;
			pTemp->m_pNextInTheChain = pTemp->m_Next;
		}
		return i32Status;
	}

	// A Master slave system should have at least one slave connected
	if ( m_pCardState->u8SlaveConnected  )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_INFO_MSDETECT, szText );
		return CS_SUCCESS;
	}
	else
	{
		TRACE(DISP_TRACE_LEVEL, "Slave detection error\n");
		return CS_MISC_ERROR;
	}					
}


//-----------------------------------------------------------------------------
// Step 1. All 0 one bit 1 CSRB_BIST_MS_MULTIDROP_MASK
// Step 2. All 1 one bit 0
// Step 3. Clock counters
//-----------------------------------------------------------------------------


int32 CsRabbitDev::TestMsBridge()
{
	int32 i32Ret = CS_SUCCESS;
	if ( ermMaster != m_pCardState->ermRoleMode )
		return CS_FALSE;
	
	uInt16 u16(0);
	uInt32 u32Data(0);
	CsRabbitDev* pSlave(NULL);
	char szText[128] = {0};
	
	//Configure master tranceivers
	m_pCardState->AddOnReg.u16MsBist1 = CSRB_BIST_EN|CSRB_BIST_DIR2;
	WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist1, CSRB_MS_BIST_CTL1_WR);
		
	//Step 1.
	m_pCardState->AddOnReg.u16MsBist2 = 0;
	WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSRB_MS_BIST_CTL2_WR);
	for( u16 = CSRB_BIST_MS_MULTIDROP_FIRST; u16 <= CSRB_BIST_MS_MULTIDROP_LAST; u16 <<= 1 )
	{
		m_pCardState->AddOnReg.u16MsBist2 = u16;
		WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSRB_MS_BIST_CTL2_WR);
		pSlave = m_pNextInTheChain;
		while( NULL != pSlave )
		{
			u32Data = pSlave->ReadRegisterFromNios(pSlave->m_pCardState->AddOnReg.u16MsBist1, CSRB_MS_BIST_ST1_RD);
			if( (u32Data & CSRB_BIST_MS_MULTIDROP_MASK) != u16 )
			{
				sprintf_s( szText, sizeof(szText), TEXT("Bit 0x%x card %d low"), u16, pSlave->m_pCardState->u8MsCardId );
				CsEventLog EventLog;
				EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
				i32Ret =CS_MS_BRIDGE_FAILED;
			}
			pSlave = pSlave->m_pNextInTheChain;
		}			
	}
	//Step 2.
	m_pCardState->AddOnReg.u16MsBist2 = CSRB_BIST_MS_MULTIDROP_MASK;
	WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSRB_MS_BIST_CTL2_WR);
	for( u16 = CSRB_BIST_MS_MULTIDROP_FIRST; u16 <= CSRB_BIST_MS_MULTIDROP_LAST; u16 <<= 1 )
	{
		m_pCardState->AddOnReg.u16MsBist2 = (CSRB_BIST_MS_MULTIDROP_MASK & ~u16);
		WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSRB_MS_BIST_CTL2_WR);
		pSlave = m_pNextInTheChain;
		while( NULL != pSlave )
		{
			u32Data = pSlave->ReadRegisterFromNios(pSlave->m_pCardState->AddOnReg.u16MsBist1, CSRB_MS_BIST_ST1_RD);
			if( (u32Data & CSRB_BIST_MS_MULTIDROP_MASK) != (uInt32)(CSRB_BIST_MS_MULTIDROP_MASK & ~u16) )
			{
				sprintf_s( szText, sizeof(szText), TEXT("Bit 0x%x card %d high"), u16, pSlave->m_pCardState->u8MsCardId );
				CsEventLog EventLog;
				EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
				i32Ret =CS_MS_BRIDGE_FAILED;
			}
			pSlave = pSlave->m_pNextInTheChain;
		}			
	}

	//Step 3
	uInt32 u32FpgaClkCnt(0), u32AdcClkCnt(0);
	u32Data = ReadRegisterFromNios(m_pCardState->AddOnReg.u16MsBist1, CSRB_MS_BIST_ST2_RD);
	u32FpgaClkCnt = u32Data & CSRB_BIST_MS_FPGA_COUNT_MASK;
	u32AdcClkCnt  = u32Data & CSRB_BIST_MS_ADC_COUNT_MASK;

	u32Data = ReadRegisterFromNios(m_pCardState->AddOnReg.u16MsBist1, CSRB_MS_BIST_ST2_RD);
	if( u32FpgaClkCnt == (u32Data & CSRB_BIST_MS_FPGA_COUNT_MASK) )
	{
		sprintf_s( szText, sizeof(szText), TEXT("Fpga count failed. Card %d"), m_pCardState->u8MsCardId );
		CsEventLog EventLog;
		EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
		i32Ret = CS_MS_BRIDGE_FAILED;
	}
	if( u32AdcClkCnt == (u32Data & CSRB_BIST_MS_ADC_COUNT_MASK) )
	{
		sprintf_s( szText, sizeof(szText), TEXT("ADC count failed. Card %d"), m_pCardState->u8MsCardId );
		CsEventLog EventLog;
		EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
		i32Ret = CS_MS_BRIDGE_FAILED;
	}

	pSlave = m_pNextInTheChain;
	while( NULL != pSlave )
	{
		u32Data = pSlave->ReadRegisterFromNios(pSlave->m_pCardState->AddOnReg.u16MsBist1, CSRB_MS_BIST_ST2_RD);
		u32FpgaClkCnt = u32Data & CSRB_BIST_MS_FPGA_COUNT_MASK;
		u32AdcClkCnt  = u32Data & CSRB_BIST_MS_ADC_COUNT_MASK;

		u32Data = pSlave->ReadRegisterFromNios(pSlave->m_pCardState->AddOnReg.u16MsBist1, CSRB_MS_BIST_ST2_RD);
		if( u32FpgaClkCnt == (u32Data & CSRB_BIST_MS_FPGA_COUNT_MASK) )
		{
			sprintf_s( szText, sizeof(szText), TEXT("Fpga count failed. Card %d"), pSlave->m_pCardState->u8MsCardId );
			CsEventLog EventLog;
			EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
			i32Ret =CS_MS_BRIDGE_FAILED;
		}
		if( u32AdcClkCnt == (u32Data & CSRB_BIST_MS_ADC_COUNT_MASK) )
		{
			sprintf_s( szText, sizeof(szText), TEXT("ADC count failed. Card %d"), pSlave->m_pCardState->u8MsCardId );
			CsEventLog EventLog;
			EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
			i32Ret = CS_MS_BRIDGE_FAILED;
		}
		pSlave = pSlave->m_pNextInTheChain;
	}

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::SendMasterSlaveSetting()
{
	uInt16 u16Data = 0;

	uInt8	u8MsMode = 0; 
	switch(m_pCardState->ermRoleMode)
	{
		default: return CS_INVALID_PARAMETER;
		case ermStandAlone:
			break;
		case ermMaster:
		case ermSlave:
		case ermLast:
			u8MsMode = m_pCardState->u8MsTriggMode;
			break;
	}
	u16Data = (u8MsMode) << 11 | (m_pCardState->u8SlaveConnected << 2);

	if ( 0 == u16Data || u16Data != m_pCardState->AddOnReg.u16MasterSlave )
	{
		u16Data |= RB_STATE_MACHINE_RESET;

		int32 i32Ret = WriteRegisterThruNios(u16Data, CSRB_MS_REG_WR);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		u16Data &= ~RB_STATE_MACHINE_RESET;
		i32Ret = WriteRegisterThruNios(u16Data, CSRB_MS_REG_WR);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
		
		m_pCardState->AddOnReg.u16MasterSlave = u16Data;
	}
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsRabbitDev::EnableMsBist(bool bEnable)
{
	if(bEnable)
	{
		m_pCardState->AddOnReg.u16MsBist1 |= CSRB_BIST_EN;
		m_pCardState->AddOnReg.u16MasterSlave = (CSRB_DEFAULT_MSTRIGMODE) << 11 ;
	}
	else
	{
		if( ermStandAlone == m_pCardState->ermRoleMode )
			m_pCardState->AddOnReg.u16MasterSlave = 0;

		m_pCardState->AddOnReg.u16MsBist1 &=~CSRB_BIST_EN;
	}
	int32 i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MasterSlave, CSRB_MS_REG_WR);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	return WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist1, CSRB_MS_BIST_CTL1_WR);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::SendTriggerEnableSetting( eTrigEnableMode ExtTrigEn )
{
	int32		i32Ret = CS_SUCCESS;
	uInt16		u16Data = 0;

	switch ( ExtTrigEn )
	{
		case eteExtLive:
			u16Data = 0x1F;
			break;
		case eteExtNegLatch:
			u16Data = 0x13;
			break;
		case eteExtNegLatchOnce:
			u16Data = 0x3;
			break;
		case eteExtPosLatch:
			u16Data = 0x1B;
			break;
		case eteExtPosLatchOnce:
			u16Data = 0xB;
			break;
		case eteAlways:
		default:
			u16Data = 0;
			break;
	}

	if ( u16Data )
		m_pCardState->bDisableTrigOut = true;			// Trigout connnector becomes input

	if ( m_bCobraMaxPCIe )
		i32Ret = WriteRegisterThruNios(u16Data, CSFB_TRIGGATECTRL_ADDR);
	else
		i32Ret = WriteRegisterThruNios(u16Data, CSRB_TGC_ADDR);

	return i32Ret;

}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::ValidateExtClockFrequency( uInt32 u32Mode, uInt32 u32ClkFreq_Hz, uInt32 u32ExtClkSkip )
{
	if (m_bNeverValidateExtClk )
		return CS_SUCCESS;

	int32	i32RetChkClk = CS_SUCCESS;
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32OldMode = m_pCardState->AcqConfig.u32Mode;
	uInt32	u32OldExtClk = m_pCardState->AcqConfig.u32ExtClk;
	uInt32	u32OldExtClkSampleSkip = m_pCardState->AcqConfig.u32ExtClkSampleSkip;
	LONGLONG llOldSampleRate = m_pCardState->AcqConfig.i64SampleRate;
	BOOLEAN	bError = FALSE;

	// Switch to Ext clock mode
	i32Ret = SendClockSetting( u32Mode, u32ClkFreq_Hz, 1, 0, u32ExtClkSkip );
	if( CS_FAILED(i32Ret) )
		bError = TRUE;

	//Since Firmware is using the ADC clock in ping-pong mode the verified frequency
	// should be devided by two.
	u32ClkFreq_Hz /= m_pCardState->AddOnReg.u16ClockDiv;

	if ( !bError )
	{
		// Validation of Ext clock input
		int32	i32NiosStatus = 0;
	
		if ( m_bAllowExtClkMs_pp && ( u32ClkFreq_Hz > (uInt32) GetMaxSampleRate( false ) ) )
		{
			// In this mode u32ClkFreq_Hz is the sampling frequency. the input extclk should be half of sampling frequency because
			// the card is in pingpong mode
			u32ClkFreq_Hz /= 2;
		}

		i32NiosStatus = ReadRegisterFromNios( u32ClkFreq_Hz/1000, CSPLXBASE_CHECK_CLK_FREQUENCY );
		if ( i32NiosStatus != 0x7 )
		{
			bError = TRUE;
			i32RetChkClk = CS_EXT_CLK_NOT_PRESENT;
		}
	}

	if ( bError )
	{
		// Restore the previos settings then recalibrate
		m_pCardState->AddOnReg.u16ClkSelect1 = 0;
		m_pCardState->AddOnReg.u16ClkSelect2 = 0;

		i32Ret = SendClockSetting( u32OldMode, llOldSampleRate, u32OldExtClk, ((u32OldMode & CS_MODE_REFERENCE_CLK) != 0), u32OldExtClkSampleSkip);		// Previous clk setting
	}
	
	if( CS_FAILED(i32RetChkClk) )
		return i32RetChkClk;
	else
		return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL	CsRabbitDev::IsNiosInit(void)
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
			Sleep(3000);
			u32Status = ReadFlashRegister(STATUS_REG_PLX);
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
		return (0 != m_PlxData->InitStatus.Info.u32Nios);
	}
	else
		return CsPlxBase::IsNiosInit();
}

int32	CsRabbitDev::BaseBoardConfigurationReset(uInt8	u8UserImageId)
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

BOOLEAN CsRabbitDev::ConfigurationReset( uInt8	u8UserImageId )
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
int32	CsRabbitDev::CsNucleonCardUpdateFirmware()
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

	if ( 0 == m_PlxData->CsBoard.u8BadBbFirmware && 0 == m_PlxData->CsBoard.u8BadAddonFirmware )
		return CS_SUCCESS;

	if ( (hCsiFile = GageOpenFileSystem32Drivers( m_szCsiFileName )) == 0 )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_OPEN_FILE, m_szCsiFileName );
		return CS_FAIL;
	}

	// check if any expert firmware needs updating. If not don't bother reading the fwl file.
	// if they do need updating and we can't open the file it's an error
	BOOL bUpdateFwl = ( m_PlxData->CsBoard.u8BadBbFirmware & (BAD_EXPERT1_FW | BAD_EXPERT2_FW) );

	if ( bUpdateFwl )
	{
		if ( (hFwlFile = GageOpenFileSystem32Drivers( m_szFwlFileName )) == 0 )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_OPEN_FILE, m_szFwlFileName );
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
					sprintf_s( szText, sizeof(szText), TEXT("Image(%d) from v%s to v%s"), u32RealImageLocation, OldFwFlVer, NewFwFlVer );
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
	if ( m_PlxData->CsBoard.u8BadAddonFirmware )
		i32Status = UpdateAddonFirmware( hCsiFile, pBuffer );

	// After firmware update we need to set clock properly. Otherwise Master/Slave BIST test will fail 
	InitAddOnRegisters(true);

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
int32	CsRabbitDev::NucleonWriteFlash(uInt32 u32StartAddr, void *pBuffer, uInt32 u32Size, bool bBackup )
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
		u32StartSector = u32StartAddr/NUCLEON_FLASH_SECTOR_SIZE0;
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
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::CsReadPlxNvRAM( PPLX_NVRAM_IMAGE nvRAM_image )
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
//	nvRAM_image->Data.MemorySize				= (uInt32 )(m_PlxData->CsBoard.i64MaxMemory / 1024); // Kb
	nvRAM_image->Data.MemorySize				= (uInt32 )(m_PlxData->u32NvRamSize); // already in Kb // CR #4786 and 4733

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::CsWritePlxNvRam( PPLX_NVRAM_IMAGE nvRAM_image )
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

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsRabbitDev::IoctlGetPciExLnkInfo(PPCIeLINK_INFO pPciExInfo)
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

//-----------------------------------------------------------------------------
int32	CsRabbitDev::UpdateAddonFirmware( FILE_HANDLE hCsiFile, uInt8 *pBuffer )
{
	uInt32					u32ChecksumOffset = 0;
	uInt32					u32Checksum = 0;
	int32					i32Status = CS_SUCCESS;
	char					szText[128];
	char					OldFwFlVer[40];
	char					NewFwFlVer[40];
	RBT_ADDONFLASH_FIRMWARE_INFOS AddonFwInfo = {0};

	uInt32	u32FwInfoOffset = FIELD_OFFSET(RBT_ADDONFLASH_LAYOUT, FlashImage1) + 	FIELD_OFFSET(RBT_ADDONFLASH_IMAGE_STRUCT, FwInfo) +
							  FIELD_OFFSET(RBT_ADDONFLASH_FWINFO_SECTOR, FwInfoStruct);

	u32ChecksumOffset = u32FwInfoOffset + FIELD_OFFSET(CS_FIRMWARE_INFOS, u32ChecksumSignature);

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
			// Set valid Checksum
			AddonFwInfo.u32ChecksumSignature	= CSI_FWCHECKSUM_VALID;
			AddonFwInfo.u32AddOnFwOptions		= m_AddonCsiEntry.u32FwOptions;
			i32Status = WriteEepromEx( &AddonFwInfo, u32FwInfoOffset, sizeof(AddonFwInfo), AccessLast );
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

		// Invalidate the current addon firmware version
		ResetCachedState();
		m_pCardState->VerInfo.AddonFpgaFwInfo.u32Reg = 0;
	}

	return i32Status;
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int32	CsRabbitDev::ReadBaseBoardHardwareInfoFromFlash( BOOLEAN bChecksum )
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

	// Assume the baseboard firmware is valid
	m_PlxData->CsBoard.u8BadBbFirmware = 0;

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

		m_PlxData->u32NvRamSize = BoardCfg.u32MemSizeKb;	//in kBytes
	}
	else
	{
		m_PlxData->CsBoard.i64ExpertPermissions	= 0;
		m_PlxData->CsBoard.u32BaseBoardVersion	= 0;
		m_PlxData->CsBoard.u32SerNum			= 0;
		m_PlxData->CsBoard.u32BaseBoardHardwareOptions = 0;

		m_PlxData->u32NvRamSize = 0x2000000;	//in kBytes
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

		m_PlxData->CsBoard.u32UserFwVersionB[1].u32Reg		= FwInfo.u32FirmwareVersion;
		m_PlxData->CsBoard.u32BaseBoardOptions[1]			= FwInfo.u32FirmwareOptions;
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

		m_PlxData->CsBoard.u32UserFwVersionB[2].u32Reg			= FwInfo.u32FirmwareVersion;
		m_PlxData->CsBoard.u32BaseBoardOptions[2]				= FwInfo.u32FirmwareOptions;
	}
	return i32Status;
}



int64	CsRabbitDev::GetTimeStampFrequency()
{
	int64	i64TimeStampFreq = 0;
	if ( m_bNucleon )
	{
		if ( m_pCardState->AcqConfig.u32TimeStampConfig & TIMESTAMP_MCLK )
		{
			i64TimeStampFreq = CSRB_PCIEX_MEMORY_CLOCK;
			i64TimeStampFreq <<= 4;
		}
		else
		{
			uInt8 u8Shift = 0;
			switch(m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE)
			{
				default:
				case CS_MODE_DUAL:	u8Shift = 1; break;
				case CS_MODE_SINGLE:u8Shift = 0; break;
			}
			i64TimeStampFreq = m_pCardState->AcqConfig.i64SampleRate;
			i64TimeStampFreq <<= u8Shift;
		}
	}
	else if ( m_pCardState->AcqConfig.u32TimeStampConfig & TIMESTAMP_MCLK )
	{
		uInt8 u8Shift = 0;
		switch(m_pCardState->AcqConfig.u32Mode & (CS_MODE_DUAL | CS_MODE_SINGLE))
		{
			default:
			case CS_MODE_DUAL:	u8Shift = 2; break;
			case CS_MODE_SINGLE:u8Shift = 3; break;
		}
		i64TimeStampFreq = CSRB_MEMORY_CLOCK;
		i64TimeStampFreq <<= u8Shift;
	}
	else
		i64TimeStampFreq = m_pCardState->AcqConfig.i64SampleRate;

	return i64TimeStampFreq;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::SetDigitalDelay( int16 i16Offset )
{
	int32			i32Ret = CS_SUCCESS;
	uInt16			u16Mask = 0x3F;		// 6 bits
	int16			RangeMax = MAX_EXTTRIG_ADJUST;
	int16			RangeMin = MIN_EXTTRIG_ADJUST;

	if ( m_bNucleon )
	{
		// The firmware is now support range (-256, 255)
		RangeMax = MAX_DIGITALDELAY_ADJUST;
		RangeMin = MIN_DIGITALDELAY_ADJUST;
	}

	ASSERT( i16Offset >= RangeMin && i16Offset <= RangeMax );
	if ( i16Offset < RangeMin || i16Offset > RangeMax )
	{
		char szErrorStr[128];
		sprintf_s(szErrorStr, sizeof(szErrorStr), "%d", i16Offset );
		CsEventLog EventLog;
		EventLog.Report( CS_WARN_EXTTRIG_CALIB_FAILED, szErrorStr );
		OutputDebugString("Digital delay is out of range supported.\n");

		if ( i16Offset < RangeMin )
			i16Offset = RangeMin;

		if ( i16Offset > RangeMax )
			i16Offset = RangeMax;
	}

	if ( ! m_bNucleon )
	{
		// Convert from (-32, 31) range to (0, 63) range
		i16Offset += 32;
		i16Offset = (i16Offset & u16Mask);
	}

	i32Ret = WriteRegisterThruNios( (uInt32) i16Offset, CSRB_EXTTRIG_ALIGNMENT);
	return i32Ret;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::CobraMaxSendClockSetting(uInt32 u32Mode, LONGLONG llSampleRate, BOOLEAN bRefClk )
{
	int32	i32Ret = CS_SUCCESS;
	
	BOOLEAN	bInit = ( CSRB_DEFAULT_SAMPLE_RATE == llSampleRate );

	PRABBIT_SAMPLE_RATE_INFO	 pSrInfo;
	uInt16	u16ClkSelect1 = CSFB_CLKR1_RESET_BT|CSFB_CLKR1_PLLCLKINPUT_BT|CSFB_CLKR1_STDCLKINPUT_BT;

	if ( bInit )
	{
 		m_pCardState->AddOnReg.u16ClkOut = CSFB_CLK_OUT_NONE;
		llSampleRate = GetMaxSampleRate(FALSE);
	}

	if ( !FindSrInfo( llSampleRate, &pSrInfo ) )
		return CS_INVALID_SAMPLE_RATE;

	m_pCardState->AcqConfig.u32ExtClkSampleSkip = CS_SAMPLESKIP_FACTOR;

	if ( bRefClk && this == m_MasterIndependent )
		u16ClkSelect1 |= CSFB_CLKR1_EXTCLKINPUT_BT;

	if( ermSlave == m_pCardState->ermRoleMode )
		u16ClkSelect1 |= CSFB_CLKR1_MSCLKINPUT_BT;

	if( ermMaster == m_pCardState->ermRoleMode )
		u16ClkSelect1 |= 0x1800;
	else
		u16ClkSelect1 |= 0xE00;

	//Out0 is disabled, but stil set deviders to 1 for it OUT2 is the same devider as out 1
	uInt16 u16ClkSelect2 = 0x4 | ClkDivToAD9514(pSrInfo->u16ClockDivision) <<8 | ClkDivToAD9514(pSrInfo->u16ClockDivision) << 4;
	uInt16 u16DecimationLow = uInt16(pSrInfo->u32Decimation & 0x7fff);
	uInt16 u16DecimationHigh = uInt16((pSrInfo->u32Decimation>>15) & 0x7fff);

	if ( (0 == (pSrInfo->u32Decimation % 3)) && IsPowerOf2(pSrInfo->u32Decimation / 3) )
	{
		u16DecimationLow = uInt16((pSrInfo->u32Decimation/3) & 0x7fff);
		u16DecimationHigh = uInt16(((pSrInfo->u32Decimation/3)>>15) & 0x3fff);
		// Turn on Decimation by 3 factor
		u16DecimationHigh |= 0x4000;		
	}

	if( (m_pCardState->AddOnReg.u32ClkFreq != pSrInfo->u32ClockFrequencyMhz  || 
		 m_pCardState->AddOnReg.u32Decimation != pSrInfo->u32Decimation ) ||
		 m_pCardState->AddOnReg.u16PingPong != pSrInfo->u16PingPong ||
		 m_pCardState->AddOnReg.u16ClkSelect1 != u16ClkSelect1   ||
		 m_pCardState->AddOnReg.u16ClkSelect2 != u16ClkSelect2  )
	{
		if ( m_pCardState->AddOnReg.u16PingPong != pSrInfo->u16PingPong )
		{
			i32Ret = SetAdcMode(true, pSrInfo->u16PingPong != 0, m_bAutoClockAdjust);
				if( CS_FAILED ( i32Ret ) ) return i32Ret;
		}

		if ( m_pCardState->AddOnReg.u32ClkFreq != pSrInfo->u32ClockFrequencyMhz ) 
		{
			m_MasterIndependent->m_pCardState->bMasterSlaveCalib	= 
			m_MasterIndependent->m_pCardState->bExtTrigCalib		= true;
			m_MasterIndependent->m_pCardState->bFwExtTrigCalibDone  = false;
		}

		if ( m_pCardState->AddOnReg.u32Decimation != pSrInfo->u32Decimation )
			m_MasterIndependent->m_pCardState->bMsDecimationReset = true;

		i32Ret = WriteRegisterThruNios(u16ClkSelect1|(m_pCardState->AddOnReg.u16ClkOut & ~0x1F00), CSRB_CLOCK_REG1);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		
		i32Ret = WriteRegisterThruNios(u16ClkSelect2, CSRB_CLOCK_REG2);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		
		if ( u16DecimationLow <= 1 )
			u16DecimationLow = 0;
		i32Ret = WriteRegisterThruNios(u16DecimationLow, CSRB_DEC_FACTOR_LOW_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		
		i32Ret = WriteRegisterThruNios(u16DecimationHigh, CSRB_DEC_FACTOR_HIGH_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		if ( m_pCardState->AddOnReg.u16PingPong != pSrInfo->u16PingPong || 
			 m_pCardState->AddOnReg.u32ClkFreq != pSrInfo->u32ClockFrequencyMhz ||
			 m_pCardState->AddOnReg.u16ClkSelect1 != u16ClkSelect1 )
		{
			m_MasterIndependent->m_pCardState->bExtTrigCalib		= true;
			m_MasterIndependent->m_pCardState->bMasterSlaveCalib	= true;
			m_MasterIndependent->m_pCardState->bFwExtTrigCalibDone	= false;

			i32Ret = SendAdcSelfCal();
			if( CS_FAILED ( i32Ret ) )	return i32Ret;

			i32Ret = CobraMaxSendDpaAlignment(0 != pSrInfo->u16PingPong);
			if( CS_FAILED(i32Ret) )	return i32Ret;
		}

		if ( !m_pCardState->b14G8 && m_pCardState->AddOnReg.u16PingPong != pSrInfo->u16PingPong )
		{
			uInt16 u16RegVal = pSrInfo->u16PingPong ? 0x23ff : 0x3ff;
			if ( pSrInfo->u16PingPong && ! m_bSingleChannel2 )
				u16RegVal |= 0x1000;

 			i32Ret = WriteToAdcReg( CBMAX_ADC_REG_ECM, u16RegVal );
			if( CS_FAILED ( i32Ret ) )	return i32Ret;

			if ( u32Mode & CS_MODE_SINGLE )
			{
				if (m_bSingleChannel2)
					m_pCardState->bCalNeeded[ConvertToHwChannel( CS_CHAN_2 )-1] = true;
				else
					m_pCardState->bCalNeeded[ConvertToHwChannel( CS_CHAN_1 )-1] = true;
			}
			else
				m_pCardState->bCalNeeded[0] = m_pCardState->bCalNeeded[1] = true;
		}

		m_pCardState->AddOnReg.u16PingPong   = pSrInfo->u16PingPong;
		m_pCardState->AddOnReg.u32ClkFreq    = pSrInfo->u32ClockFrequencyMhz;
		m_pCardState->AddOnReg.u32Decimation = pSrInfo->u32Decimation;
		m_pCardState->AddOnReg.u16ClkSelect1 = u16ClkSelect1;
		m_pCardState->AddOnReg.u16ClkSelect2 = u16ClkSelect2;
		m_pCardState->AddOnReg.u16ClockDiv   = pSrInfo->u16ClockDivision;

		m_pCardState->u32AdcClock				= (uInt32) pSrInfo->u32ClockFrequencyMhz * 1000000;
		m_pCardState->AcqConfig.i64SampleRate		= pSrInfo->llSampleRate;
	}

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::CobraMaxSendDpaAlignment( bool bPingpong )
{
	uInt32	u32NiosReturned = 0;
#ifdef _COBRAMAXPCIE_DRV_
	uInt32	u32RegVal = 0x3;
#else
	uInt32	u32RegVal = 0x1;
#endif

	u32NiosReturned = ReadRegisterFromNios(bPingpong? u32RegVal:0, CSFB_HWALIGN_DPA);
	if( FAILED_NIOS_DATA == u32NiosReturned )
		return CS_INVALID_FRM_CMD;

	if ( 0 != u32NiosReturned )
	{
		::OutputDebugString("DPA calibration failure !!");
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_TEXT, "DPA alignment failure!!" );
	}

	return CS_SUCCESS;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::CobraMaxSonoscanCalibOnStartup()
{
	uInt32	u32NiosReturned = 0;
	uInt8	u8Passed = 0;
	
	for (int i = 0; i < 8; i++)
	{
		u32NiosReturned = ReadRegisterFromNios(0, CSFB_HWALIGN_DPA);
		if( FAILED_NIOS_DATA == u32NiosReturned )
			break;

		if ( 0 != u32NiosReturned )
			continue;			// retry;
		else
		{
			u8Passed++;
			u32NiosReturned = ReadRegisterFromNios_Slow(0, CSFB_HWALIGN_EXTTRIG, 15000 );
			if ( -1 == u32NiosReturned || FAILED_NIOS_DATA == u32NiosReturned )
				continue;		// retry;
			else
			{
				m_pCardState->bSonoscanCalibOnStartUpDone = true;
				u8Passed++;
				break;
			}
		}
	}

	CsEventLog EventLog;
	if ( u8Passed < 1 )
	{
		::OutputDebugString("DPA calibration failure !!");
		EventLog.Report( CS_ERROR_TEXT, "DPA alignment failure!!" );
	}
	else if ( u8Passed < 2 )
	{
		::OutputDebugString("Error from firmware in calibration for Ext Trigger !!!!!");
		EventLog.Report( CS_ALIGNEXTTRIG_FAILED, "" );
	}
	else
		::OutputDebugString("--- Sonoscan Calib on starttup completed SUCCESS ---");


	// Reset the shadow flag so the configuation of Ext trigger will be send down to the card on the next
	// next call of CsSetTrigCfg
	ResetCachedState();		// Force calibration next time
	m_pCardState->AddOnReg.u16ExtTrigEnable =
	m_pCardState->AddOnReg.u16ExtTrig		= (uInt16) -1;
	m_pCardState->bExtTrigCalib = true;

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsRabbitDev::CobraMaxSetClockOut( eRoleMode eMode, eClkOutMode ecomSet )
{
	uInt16 u16Data;
	eRoleMode LocaleMode = eMode;

	switch( LocaleMode )
	{
		case ermSlave:
		{
			ecomSet = eclkoNone;
			u16Data = CSFB_CLK_OUT_NONE;
			break;
		}
		default:
		{
			switch( ecomSet )
			{
				default: ecomSet = eclkoNone;
				case eclkoNone:      u16Data = CSFB_CLK_OUT_NONE; break;
				case eclkoRefClk:    u16Data = CSFB_CLK_OUT_REF;  break;
				case eclkoGBusClk:   u16Data = CSFB_CLK_OUT_FPGA; break;
			}
			break;
		}
	}

	if ( m_PlxData->InitStatus.Info.u32Nios != 0 && 
		 m_PlxData->InitStatus.Info.u32AddOnPresent != 0 &&
	     (( m_pCardState->AddOnReg.u16ClkOut != u16Data ) || (eMode != m_pCardState->ermRoleMode) )  )
	{
		uInt16 u16ClkSelect1 = m_pCardState->AddOnReg.u16ClkSelect1 & ~(CSFB_CLKR1_MSCLKINPUT_BT| CSFB_CLKR1_MASTERCLOCK_EN | 0x1F00);

		if( ermSlave == m_pCardState->ermRoleMode )
			u16ClkSelect1 |= CSFB_CLKR1_MSCLKINPUT_BT;

		if( ermMaster == LocaleMode )
			u16ClkSelect1 |= 0x1800;
		else
			u16ClkSelect1 |= 0xE00;

		int32 i32Ret = WriteRegisterThruNios(u16ClkSelect1|u16Data, CSRB_CLOCK_REG1);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		m_pCardState->AddOnReg.u16ClkSelect1 = u16ClkSelect1;
		m_pCardState->AddOnReg.u16ClkOut = u16Data;
	}
	m_pCardState->eclkoClockOut = ecomSet;

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsRabbitDev::ReadBusyStatus()
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

//-----------------------------------------------------------------------------
//	DPA for External Trigger
//-----------------------------------------------------------------------------

int32 CsRabbitDev::SendInternalHwExtTrigAlign()
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32NiosReturned = 0;

	if ( m_bNucleon && ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_SONOSCAN) ) )
	{
		if ( m_pCardState->bSonoscanCalibOnStartUpDone )
			return CS_SUCCESS;
		else
			return CS_EXT_TRIG_DPA_FAILURE;
	}

	if ( !m_bCobraMaxPCIe || m_pCardState->bFwExtTrigCalibDone )
		return CS_SUCCESS;

	// This call will modify front end and trigger settings and leave the card in Calib mode
	u32NiosReturned = ReadRegisterFromNios(0, CSFB_HWALIGN_EXTTRIG);
	if ( -1 == u32NiosReturned || FAILED_NIOS_DATA == u32NiosReturned )
	{
		::OutputDebugString("Error from firmware in calibration for Ext Trigger !!!!!");
		CsEventLog EventLog;
		EventLog.Report( CS_ALIGNEXTTRIG_FAILED, "" );
		i32Ret = CS_EXT_TRIG_DPA_FAILURE;
	}
	else
		m_pCardState->bFwExtTrigCalibDone = true;

	// After de DPA for External trigger, the trigger and front end settings has been modified by Firmware.(they emain in Calib mode)
	// Restore configurations 
	m_pCardState->AddOnReg.u32FeSetup[0] &= ~CSRB_SET_CHAN_CAL;
	m_pCardState->AddOnReg.u32FeSetup[1] &= ~CSRB_SET_CHAN_CAL;

	i32Ret = SendCalibModeSetting( CS_CHAN_1 );
	i32Ret = SendCalibModeSetting( CS_CHAN_2, ecmCalModeNormal, 500 );


	// Reset the shadow flag so the configuation of Ext trigger will be send down to the card on the next
	// next call of CsSetTrigCfg
	m_pCardState->AddOnReg.u16ExtTrigEnable = (uInt16) -1;

	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsRabbitDev::SetCalibBit( bool bEnableCalMode, uInt32 *pu32Val )
{
	// Sonoscan HW requirea the bit nCal to be inverted
	if ( m_bNucleon && ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_SONOSCAN) ) )
	{
		if ( bEnableCalMode )
			*pu32Val |= CSRB_SET_CHAN_CAL;
		else
			*pu32Val &= ~CSRB_SET_CHAN_CAL;
	}
	else
	{
		if ( bEnableCalMode )
			*pu32Val &= ~CSRB_SET_CHAN_CAL;
		else
			*pu32Val |= CSRB_SET_CHAN_CAL;
	}
}
