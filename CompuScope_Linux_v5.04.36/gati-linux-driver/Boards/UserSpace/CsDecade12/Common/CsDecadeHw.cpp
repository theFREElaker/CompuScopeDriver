//	CsDecadeHw.cpp
///	Implementation of CsDecade class
//
//	This file contains all member functions that access to addon hardware including
//	Frontend set up and calibration
//
//

#include "StdAfx.h"
#include "CsTypes.h"
#include "GageWrapper.h"
#include "CsDecade.h"
#include "CsxyG8CapsInfo.h"
#include "CsEventLog.h"
#include "CsIoctl.h"

#ifdef _WINDOWS_
#include "CsMsgLog.h"
#endif

#define	 DECADE_TIMEOUT_4SEC		40000			//	(in .1ms or 100 microsecond unit)
#define	 DECADE_TIMEOUT_10SEC		100000			//	(in .1ms or 100 microsecond unit)
#define	 DECADE_TIMEOUT_30SEC		300000			//	(in .1ms or 100 microsecond unit)

const FRONTEND_DECADE_INFO	CsDecade::c_FrontEndGain50[] = 
{  	//Gain  Val				PreBypass	Att_20Db	Att_6Db
	10000,	0x00006,	//  1			1			0
	 4000,	0x00002,	//  0			1			0
	 2000,	0x00003,	//  0			1			1
	 1000,	0x00004,	//  1			0			0
	  400,	0x00000,	//  0			0			0
	  200,	0x00001,	//  0			0			1
};

#define c_DECADE_FE_INFO_SIZE   (sizeof(CsDecade::c_FrontEndGain50)/sizeof(FRONTEND_DECADE_INFO))


DECADE_CAL_LUT	CsDecade::s_CalCfg50[] = 
{ //u32FeSwing_mV i32PosLevel_uV  i32NegLevel_uV u16PosRegMask u16NegRegMask Vref_mV
	    0,              0,                 0,		  0x53,       0x53,		(float)0.002,
	  200,            97240,          -97240,         0x50,       0x60,		(float)97.24,
	  400,           194500,         -194500,         0x51,       0x61,		(float)194.5,
	 1000,           474800,         -474800,         0x52,       0x62,		(float)474.8,
	 2000,           926700,         -926700,         0x44,       0x48,		(float)926.7,
     4000,          1853500,        -1853500,         0x45,       0x49,		(float)1853.5,
    10000,          1853500,        -1853500,         0x45,       0x49,		(float)1853.5
};
#define c_DECADE_50_CAL_LUT_SIZE   (sizeof(CsDecade::s_CalCfg50)/sizeof(CsDecade::s_CalCfg50[0]))


// SampleRate	ClkFreq	Decim	ClkDiv	Pinpong
const DECADE_SAMPLE_RATE_INFO	CsDecade::Decade12_3_2[] = 
{
	6000000000,	3000,	1,			1,		1,
    3000000000, 3000,	1,			1,		0,
	2000000000, 3000,	3,			1,		1,
	1500000000, 3000,	2,			1,		0,
	1000000000,	3000,	3,			1,		0,
	 750000000,	3000,	4,			1,		0,
	 500000000,	3000,	6,			1,		0,
	 375000000,	3000,	8,			1,		0,
	 250000000,	3000,	12,			1,		0,
	 187500000, 3000,   16,			1,		0,
	 125000000,	3000,	24,			1,		0,
	  75000000,	3000,	40,			1,		0,
	  50000000,	3000,	60,			1,		0,
	  30000000,	3000,	100,		1,		0,
	  20000000,	3000,	150,		1,		0,
	  10000000,	3000,	300,		1,		0,
	   5000000,	3000,	600,		1,		0,
	   2000000,	3000,	1500,		1,		0,
	   1000000,	3000,	3000,		1,		0,
	    500000,	3000,	6000,		1,		0,
	    200000,	3000,	15000,		1,		0,
	    100000,	3000,	30000,		1,		0,
	     50000,	3000,	60000,		1,		0,
	     20000,	3000,	150000,		1,		0,
	     10000,	3000,	300000,		1,		0,
	      5000,	3000,	600000,		1,		0,
	      2000,	3000,	1500000,	1,		0,
	      1000,	3000,	3000000,	1,		0
};

#define c_DECADE_SR_INFO_SIZE_3_2   (sizeof(CsDecade::Decade12_3_2)/sizeof(DECADE_SAMPLE_RATE_INFO))


//-----------------------------------------------------------------------------
//
// u32ExtClkSampleSkip = 0	-> internal clock
// u32ExtClkSampleSkip != 0	-> external clock
//-----------------------------------------------------------------------------
int32	CsDecade::SendClockSetting(LONGLONG llSampleRate, uInt32 u32ExtClkSampleSkip, BOOLEAN bRefClk )
{
	int32	i32Ret = CS_SUCCESS;
	bool	bForceDefault = (-1 == llSampleRate);

/*
	Add-on v1.07 base on the truth table follow:
	--------------------------------------------------------------------------------
							CLK_SOURCE_SEL_N	REF_SOURCE_SEL0		CLKIN_SOURCE_SEL

	Internal VCO			0					0					0
	Internal VCO + Ref		0					1					1
	External Clk In			1					x					1
	--------------------------------------------------------------------------------
*/

	if (bForceDefault)
	{
		if ( m_PlxData->CsBoard.u32BoardType == CSDECADE123G_BOARDTYPE )
			llSampleRate = CS_SR_3GHZ;
		else
			llSampleRate = CS_SR_6GHZ;
	}

	DEC_FACTOR_REG DecimationReg = {0};
	DECADE_CLOCK_REG			ClkReg;
	DECADE_SAMPLE_RATE_INFO		SrInfo = { 0 };
	PDECADE_SAMPLE_RATE_INFO	pSrInfo = &SrInfo;
	uInt16						u16AdcReg0_val = 0x0A0;
	DECADE_TRIG_IO_REG			TrigIoReg = {0};
	bool						bClockChanged = false;

	ClkReg = m_pCardState->ClockReg;
	ClkReg.bits.ClkIn_SrcSel =
	ClkReg.bits.Clk_SrcSel =
	ClkReg.bits.ExtClk_Div2 =
	ClkReg.bits.Ref_SrcSel0 = 0;

	if ( 0 != u32ExtClkSampleSkip )
	{
		// Ext clock mode
		SrInfo.llSampleRate = llSampleRate;
		SrInfo.u32ClockFrequencyMhz = uInt32(llSampleRate / 1000000);
		// Add-on board version 1.7 required to set clock source.
		if (m_PlxData->CsBoard.u32AddonBoardVersion >= ADDON_HW_VERSION_1_07)
		{
			ClkReg.bits.Clk_SrcSel = 1;
			if (CSDECADE126G_BOARDTYPE == m_PlxData->CsBoard.u32BoardType)
			{
				// Update TrigOut & PingPong bit
				TrigIoReg.u32RegVal = m_pCardState->TrigIoReg.u32RegVal;
				SrInfo.u16PingPong = ((CS_SAMPLESKIP_FACTOR >> 1) == u32ExtClkSampleSkip);
				SetTriggerIoRegister(TrigIoReg.u32RegVal);
			}
		}

		if ( llSampleRate >= 1000000000 )
		{
			// Setup for Ext clock 1 GHz and above
			SrInfo.u16ClockDivision = 1;
			SrInfo.u32Decimation	= 1;
			ClkReg.bits.ExtClk_Div2 = 1; // (CS_SAMPLESKIP_FACTOR == u32ExtClkSampleSkip);
			u16AdcReg0_val			= 0x0A0;
		}
		else
		{
			// Setup for Ext clock below 1GHz
			SrInfo.u16ClockDivision = 0;
			SrInfo.u32Decimation	= 2;
			ClkReg.bits.ExtClk_Div2	= 0;
			u16AdcReg0_val			= 0x120;
		}

		DecimationReg.bits.DeciFactor = pSrInfo->u32Decimation;
	}	
	else
	{
		u16AdcReg0_val = 0x0A0;

		// Internal clock or referencde clock mode
		if ( !FindSrInfo( llSampleRate, &pSrInfo ) )
			return CS_INVALID_SAMPLE_RATE;

		if ( m_PlxData->CsBoard.u32AddonBoardVersion >= ADDON_HW_VERSION_1_07)
		{
		// Update TrigOut & PingPong bit
			TrigIoReg.u32RegVal = 0;
			TrigIoReg.bits.TriggerOut = m_pCardState->bDisableTrigOut ? 1 : 0;
//			TrigIoReg.bits.PingPong = pSrInfo->u16PingPong ? 1 : 0 ;	
			SetTriggerIoRegister(TrigIoReg.u32RegVal, true);
		}

		DecimationReg.bits.DeciFactor = pSrInfo->u32Decimation;
		if ( (DecimationReg.bits.DeciFactor < 32) && ( 0 == (DecimationReg.bits.DeciFactor % 3)) )
		{
			DecimationReg.bits.DeciFactor /= 3;
			DecimationReg.bits.Factor_3 = 1;
		}

		ClkReg.bits.Ref_SrcSel0		= ( bRefClk )? 1 : 0;
		if( ( ( m_PlxData->CsBoard.u32AddonBoardVersion < ADDON_HW_VERSION_1_07) || 
			( (m_PlxData->CsBoard.u32AddonBoardVersion >= ADDON_HW_VERSION_1_07) & bRefClk) ) )
		ClkReg.bits.ClkIn_SrcSel	= 1;
	}

	// Check if we need to perform some calibration when changing the clock
	if (m_pCardState->AddOnReg.u16PingPong != pSrInfo->u16PingPong )
	{
		// Swiching from non-pingpong to pingpong
		if (0 == m_pCardState->AddOnReg.u16PingPong && !m_pCardState->bClockPingpPong_Calibrated)
		{
			m_pCardState->bAdcLvdsAligned = FALSE;
			m_u32CurrentPingPongShift = 0;

			m_Adc_GainIQ_CalNeeded[0] =
			m_Adc_GainIQ_CalNeeded[1] =
			m_Adc_OffsetIQ_CalNeeded[0] =
			m_Adc_OffsetIQ_CalNeeded[1] = TRUE;
		}
	}

	// Check if we need to perform some calibration when changing the clock
	if ( ( ClkReg.bits.ClkIn_SrcSel !=  m_pCardState->ClockReg.bits.ClkIn_SrcSel )		||
		 ( ClkReg.bits.Clk_SrcSel !=  m_pCardState->ClockReg.bits.Clk_SrcSel )		||
		 ( ClkReg.bits.Ref_SrcSel0  != m_pCardState->ClockReg.bits.Ref_SrcSel0 )									 // clock source changed
		 || ( (0 != u32ExtClkSampleSkip) &&( m_pCardState->AddOnReg.u32ClkFreq != pSrInfo->u32ClockFrequencyMhz ))   // Ext clock frequency changed
		 )
	{
		bClockChanged						= true;
		m_pCardState->bClockPingpPong_Calibrated =
		m_pCardState->bExtTriggerAligned	=
		m_pCardState->bAdcLvdsAligned		= FALSE;
		m_u32CurrentPingPongShift			= 0;

		m_Adc_GainIQ_CalNeeded[0] =
		m_Adc_GainIQ_CalNeeded[1] =
		m_Adc_OffsetIQ_CalNeeded[0] =
		m_Adc_OffsetIQ_CalNeeded[1] = TRUE;
	}

	i32Ret = SendAdcReggister0(u16AdcReg0_val);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret = SetClockControlRegister(ClkReg.u32RegVal, bForceDefault);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret = WriteRegisterThruNios(DecimationReg.u32RegVal, DECADE_DECIMATION_SETUP);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	m_pCardState->AddOnReg.u16PingPong   = pSrInfo->u16PingPong;
	m_pCardState->AddOnReg.u32ClkFreq    = pSrInfo->u32ClockFrequencyMhz;
	m_pCardState->AddOnReg.u32Decimation = pSrInfo->u32Decimation;
	m_pCardState->AddOnReg.u16ClockDiv   = pSrInfo->u16ClockDivision;
	m_pCardState->AcqConfig.u32ExtClk	 = (u32ExtClkSampleSkip > 0) ? 1 : 0; 

	m_pCardState->AcqConfig.i64SampleRate	= pSrInfo->llSampleRate;
	if (0 == u32ExtClkSampleSkip)
		m_pCardState->AcqConfig.u32ExtClkSampleSkip = m_u32DefaultExtClkSkip;
	else
		m_pCardState->AcqConfig.u32ExtClkSampleSkip = u32ExtClkSampleSkip;


	// check lock PLL only if we are in interncal clock or external reference mode mode
	if ( 0 == u32ExtClkSampleSkip )
		LockPLL();    

	i32Ret = SendADC_LVDS_Align( true );
	if (CS_FAILED(i32Ret))
		return i32Ret;
/*
	if (bClockChanged)
	{
		i32Ret = CalibrateADC_IQ();
		if (CS_FAILED(i32Ret))
			return i32Ret;
	}
*/
	// If we are here, all the clock settings have been complete successfully.
	// Set the flag to indcated that we have set and calibrated successully for in pingpong mode
	if (0 != m_pCardState->AddOnReg.u16PingPong)
		m_pCardState->bClockPingpPong_Calibrated = TRUE;

	return i32Ret;
}

bool	CsDecade::IsClockChange(LONGLONG llSampleRate, uInt32 u32ExtClkSampleSkip, BOOLEAN bRefClk )
{
	DECADE_CLOCK_REG			ClkReg;
	DECADE_SAMPLE_RATE_INFO		SrInfo = { 0 };
	PDECADE_SAMPLE_RATE_INFO	pSrInfo = &SrInfo;
	bool						bClockChanged = false;

	ClkReg = m_pCardState->ClockReg;
	ClkReg.bits.ClkIn_SrcSel =
	ClkReg.bits.Clk_SrcSel =
	ClkReg.bits.ExtClk_Div2 =
	ClkReg.bits.Ref_SrcSel0 = 0;

	if ( 0 != u32ExtClkSampleSkip )
	{
		// Ext clock mode
		SrInfo.llSampleRate = llSampleRate;
		SrInfo.u32ClockFrequencyMhz = uInt32(llSampleRate / 1000000);
		// Add-on board version 1.7 required to set clock source.
		if( m_PlxData->CsBoard.u32AddonBoardVersion >= ADDON_HW_VERSION_1_07)
			ClkReg.bits.Clk_SrcSel = 1;

		if ( llSampleRate >= 1000000000 )
		{
			// Setup for Ext clock 1 GHz and above
			SrInfo.u16ClockDivision = 1;
			SrInfo.u32Decimation	= 1;
			ClkReg.bits.ExtClk_Div2	= ( CS_SAMPLESKIP_FACTOR == u32ExtClkSampleSkip );	
		}
		else
		{
			// Setup for Ext clock below 1GHz
			SrInfo.u16ClockDivision = 0;
			SrInfo.u32Decimation	= 2;
			ClkReg.bits.ExtClk_Div2	= 0;
		}

	}	
	else
	{
		// Internal clock or reference clock mode
		if ( !FindSrInfo( llSampleRate, &pSrInfo ) )
			return bClockChanged;

		ClkReg.bits.Ref_SrcSel0		= ( bRefClk )? 1 : 0;
		if( ( ( m_PlxData->CsBoard.u32AddonBoardVersion < ADDON_HW_VERSION_1_07) || 
			( (m_PlxData->CsBoard.u32AddonBoardVersion >= ADDON_HW_VERSION_1_07) & bRefClk) ) )
		ClkReg.bits.ClkIn_SrcSel	= 1;
	}

	// Check if we need to perform some calibration when changing the clock
	if (m_pCardState->AddOnReg.u16PingPong != pSrInfo->u16PingPong )
	{
		// Switching from non-pingpong to pingpong
		if (0 == m_pCardState->AddOnReg.u16PingPong && !m_pCardState->bClockPingpPong_Calibrated)
		{
			bClockChanged						= true;
		}
	}

	// Check if we need to perform some calibration when changing the clock
	if ( ( ClkReg.bits.ClkIn_SrcSel !=  m_pCardState->ClockReg.bits.ClkIn_SrcSel )		||
		 ( ClkReg.bits.Clk_SrcSel !=  m_pCardState->ClockReg.bits.Clk_SrcSel )		||
		 ( ClkReg.bits.Ref_SrcSel0  != m_pCardState->ClockReg.bits.Ref_SrcSel0 )									 // clock source changed
		 || ( (0 != u32ExtClkSampleSkip) &&( m_pCardState->AddOnReg.u32ClkFreq != pSrInfo->u32ClockFrequencyMhz ))   // Ext clock frequency changed
		 )
	{
		bClockChanged						= true;
	}

	return bClockChanged;

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDecade::SendAdcReggister0( uInt16 u16Val, bool bForce /* = false */ )
{
	int32	i32Ret = CS_SUCCESS;

	if ( bForce || m_pCardState->AddOnReg.u16AdcRegister0 != u16Val )
	{
		i32Ret = WriteRegisterThruNios(u16Val, DECADE_BOTH_ADCs);
		if( CS_SUCCEEDED(i32Ret) )
			m_pCardState->AddOnReg.u16AdcRegister0 = u16Val;
	}

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDecade::SendCaptureModeSetting(uInt32 u32Mode, uInt16 u16HwChannelSingle )
{
	uInt16	u16ChanCtrl = 0;

	if ( u32Mode & CS_MODE_DUAL )
	{
		u16ChanCtrl = 0;
	}
	else if ( u32Mode & CS_MODE_SINGLE )
	{
		if ( 0 != m_pCardState->AddOnReg.u16PingPong )
			u16ChanCtrl = 3;
		else
			u16ChanCtrl = u16HwChannelSingle;
	}
	else
		return CS_INVALID_ACQ_MODE;

	m_pCardState->AcqConfig.u32Mode = u32Mode;

	int32 i32Ret = WriteRegisterThruNios(u16ChanCtrl, DECADE_CHANNEL_CTRL);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDecade::SendChannelSetting(uInt16 u16HwChannel, uInt32 u32InputRange, uInt32 u32Term, uInt32 u32Impendance, int32 i32Position, uInt32 u32Filter, int32 i32ForceCalib)
{
	int32 i32Ret = CS_SUCCESS;
	uInt32	u32ValidCoupling = CS_COUPLING_DC;
	uInt32 IntFifo = 0;

	// Check for Overvoltage status
	if (m_OverVoltageDetection && m_pCardState->bOverVoltage)
	{
		m_pCardState->bOverVoltage = FALSE;
		RestoreCalibModeSetting();
	}
	
	if( (CS_CHAN_1 != u16HwChannel) && ((CS_CHAN_2 != u16HwChannel))  )
	{
		return CS_INVALID_CHANNEL;
	}
	if (CS_REAL_IMP_50_OHM != u32Impendance )
	{
		return CS_INVALID_IMPEDANCE;
	}

	if (0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & DECADE_ADDON_HW_OPTION_AC_COUPLING))
		u32ValidCoupling = CS_COUPLING_AC;

	if (u32ValidCoupling != (u32Term & u32ValidCoupling))
		return CS_INVALID_COUPLING;

	if ( DECADE_DEFAULT_INPUTRANGE == u32InputRange )
	{
		u32InputRange = DECADE_DEFAULT_GAIN;
	}

	uInt16 u16ChanZeroBased = u16HwChannel-1;
	uInt32 u32FrontIndex = 0;

	i32Ret = FindFeIndex( u32InputRange, u32FrontIndex);
	if( CS_FAILED(i32Ret) ) return i32Ret;
	

	DECADE_FRONTEND_SETUP		FrontEndSetup ={0};	
	FrontEndSetup.u32Val		= m_pCardState->FeInfoTable[u32FrontIndex].u32RegSetup;

	if ( m_bFirmwareChannelRemap )
		FrontEndSetup.bits.Channel	= RemapChannel(u16HwChannel);
	else
		FrontEndSetup.bits.Channel	= u16HwChannel;

	if( i32Position >  int32(u32InputRange) || 
		i32Position < -int32(u32InputRange) )
	{
		return CS_INVALID_POSITION;
	}

	if( i32ForceCalib > 0 )
	{
		m_pCardState->bCalNeeded[u16ChanZeroBased] = true;
		m_bForceCal[u16ChanZeroBased] = true;
	}

	if ( m_pCardState->AddOnReg.u32FeIndex[u16ChanZeroBased] != u32FrontIndex || 
		 m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased].u32Val != FrontEndSetup.u32Val )
	{
		WriteGageRegister(INT_CFG, 0);

		i32Ret = WriteRegisterThruNios(FrontEndSetup.u32Val, DECADE_SETFRONTEND_CMD);

		//OVER VOLTAGE protection may be triggered by command above so clear it by reading FIFO. 
		IntFifo = ReadGageRegister(0x48);
		WriteGageRegister(INT_CFG, OVER_VOLTAGE_INTR);

		if( CS_FAILED(i32Ret) )	return i32Ret;

		m_pCardState->bCalNeeded[u16ChanZeroBased]					= true;

		// When changing input range, the input signal could be over voltage
		// Check for Over voltage status
		if (m_OverVoltageDetection)
		{
			Sleep(200);
			if (m_pCardState->bOverVoltage)
			{
				i32Ret = CS_CHANNEL_PROTECT_FAULT;
				return i32Ret;
			}
		}
	
		m_pCardState->AddOnReg.u32FeIndex[u16ChanZeroBased]			= u32FrontIndex;
		m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased].u32Val	= FrontEndSetup.u32Val;

		m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange = u32InputRange;
		m_pCardState->ChannelParams[u16ChanZeroBased].u32Term		= u32Term & CS_COUPLING_MASK;
		m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance	= CS_REAL_IMP_50_OHM;
		m_pCardState->ChannelParams[u16ChanZeroBased].u32Filter		= u32Filter;
	}

	//m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = i32Position;
	i32Ret = SetPosition(u16HwChannel, i32Position);
	
	return i32Ret;
}




//-----------------------------------------------------------------------------
//----- SendTriggerEngineSetting
//----- ADD-ON REGISTER
//-----------------------------------------------------------------------------
int32 CsDecade::SendTriggerEngineSetting(int32	i32SourceA1,
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
	DIGI_TRIG_CTRL_REG		TrigCtrl_Reg = {0};

	
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
		TrigCtrl_Reg.bits.TrigEng1_En_A = 1;
		
		if ( CS_TRIG_SOURCE_CHAN_2 == ConvertToHwChannel( (uInt16) i32SourceA1 ) )
		{
			TrigCtrl_Reg.bits.DataSelect1_A = 1;
		}

		if ( u32ConditionA1 == CS_TRIG_COND_POS_SLOPE )
		{
			TrigCtrl_Reg.bits.TrigEng1_Slope_A = 1;
		}
	}
	if ( 0 < i32SourceB1  )
	{
		TrigCtrl_Reg.bits.TrigEng1_En_B = 1;
		
		if ( CS_TRIG_SOURCE_CHAN_1 == ConvertToHwChannel( (uInt16) i32SourceB1 ) )
		{
			TrigCtrl_Reg.bits.DataSelect1_B = 1;
		}

		if ( u32ConditionB1 == CS_TRIG_COND_POS_SLOPE )
		{
			TrigCtrl_Reg.bits.TrigEng1_Slope_B = 1;
		}
	}
	if ( 0 < i32SourceA2  )
	{
		TrigCtrl_Reg.bits.TrigEng2_En_A = 1;
		
		if ( CS_TRIG_SOURCE_CHAN_2 == ConvertToHwChannel( (uInt16) i32SourceA2 ) )
		{
			TrigCtrl_Reg.bits.DataSelect2_A = 1;
		}

		if ( u32ConditionA2 == CS_TRIG_COND_POS_SLOPE )
		{
			TrigCtrl_Reg.bits.TrigEng2_Slope_A = 1;
		}
	}
	if ( 0 < i32SourceB2  )
	{
		TrigCtrl_Reg.bits.TrigEng2_En_B = 1;
		
		if ( CS_TRIG_SOURCE_CHAN_1 == ConvertToHwChannel( (uInt16) i32SourceB2 ) )
		{
			TrigCtrl_Reg.bits.DataSelect2_B = 1;
		}

		if ( u32ConditionB2 == CS_TRIG_COND_POS_SLOPE )
		{
			TrigCtrl_Reg.bits.TrigEng2_Slope_B = 1;
		}
	}

	int32 i32Ret = CS_SUCCESS;

	i32Ret = WriteRegisterThruNios(TrigCtrl_Reg.u16RegVal, DECADE_DT_CONTROL);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	m_TrigCtrl_Reg.u16RegVal = TrigCtrl_Reg.u16RegVal;

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
int32 CsDecade::SendExtTriggerSetting( BOOLEAN  bExtTrigEnabled,
										   int32	i32ExtLevel,
										   uInt32	u32ExtCondition,
										   uInt32	u32ExtTriggerRange, 
										   uInt32	u32ExtTriggerImped, 
										   uInt32	u32ExtCoupling,
										   int32	i32Sensitivity )
{
	EXTRTRIG_REG	ExttTrigReg = {0};
	int32			i32Ret = CS_SUCCESS;

	UNREFERENCED_PARAMETER(i32Sensitivity);

	if ( bExtTrigEnabled )
		ExttTrigReg.bits.ExtTrigEnable = 1;

	if( CS_TRIG_COND_POS_SLOPE == u32ExtCondition )
		ExttTrigReg.bits.ExtTrigSlope = 1;
	
	if ( m_pCardState->AddOnReg.u16ExtTrigEnable != ExttTrigReg.u16RegVal )
	{
		i32Ret = WriteRegisterThruNios(ExttTrigReg.u16RegVal, DECADE_EXT_TRIGGER_SETTING);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		m_pCardState->AddOnReg.u16ExtTrigEnable = ExttTrigReg.u16RegVal;
	}
	
	i32Ret = SendExtTriggerLevel( i32ExtLevel );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].i32Source = bExtTrigEnabled ? -1 : 0;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32Condition = u32ExtCondition;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].i32Level = i32ExtLevel;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange = u32ExtTriggerRange;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtCoupling = u32ExtCoupling;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance = u32ExtTriggerImped;

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDecade::SendExtTriggerLevel( int32 i32Level )
{
	int32	i32Ret = CS_SUCCESS;

	//----- Convert from (-100, 99) range to ADC range
	int32	i32LevelConverted = 0;
	uInt32	u32DacCode = 0;
	const	uInt32	u32LevelZeroCode = 0x9B00;

	// Assume we are at +- 5V range
	// converted from % to mV
	i32LevelConverted = 5000/100*i32Level;

	// Advised from the HW group. Because of some problem of HW
	// Hard code for the trigger Level at +1V.
	// the user cannot change the trigger level
	// Internally we set the trigger level at +1V = 1000mv
	// From the user point of view, the trigger level is about ~ +2V
	if ( m_bExtTrigLevelFix )
		i32LevelConverted = 1000;
	else
	{
		// The valid trigger range is from -2.0V to + 3.0 V
		if ( i32LevelConverted > 3000 || i32LevelConverted < -2000 )
			return CS_INVALID_TRIG_LEVEL;
	}

	if ( i32LevelConverted >= 0 )
	{
		// converted to DAC code
		// Level		0			3000 mv
		// DAC code		0x9B00		0

		u32DacCode = u32LevelZeroCode;
		u32DacCode = u32DacCode - u32DacCode*i32LevelConverted/3000;
	}
	else
	{
		// converted to DAC code
		// Level		0			-2000 mv
		// DAC code		0x9B00		0xFFFF
		int16	i16CodeRange = 0xFFFF - u32LevelZeroCode;
		u32DacCode = u32LevelZeroCode;
		u32DacCode = u32DacCode - i16CodeRange*i32LevelConverted/2000;
	}

	i32Ret = WriteRegisterThruNios( u32DacCode, DECADE_EXTTRIG_DAC );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDecade::SendDigitalTriggerLevel( uInt16 u16TrigEngine, int32 i32Level )
{
	int32	i32LevelConverted = i32Level;
	int32	i32Ret = CS_SUCCESS;

	if( 100 < i32Level )
		i32Level = 100;
	else if( -100 > i32Level )
		i32Level = -100;

	//----- Convert from (-100, 100) range to ADC range
	int32  i32ADCRange =  DECADE_DIG_TRIGLEVEL_SPAN;
	i32LevelConverted = i32Level + 100;
	i32LevelConverted = i32LevelConverted * i32ADCRange;
	i32LevelConverted /= 200L;

	int16 i16LevelDAC = (int16)i32LevelConverted;

	switch ( u16TrigEngine )
	{
	case CSRB_TRIGENGINE_A1:
		i32Ret = WriteRegisterThruNios(i16LevelDAC, DECADE_DLA1);
		break;
	case CSRB_TRIGENGINE_B1:
		i32Ret = WriteRegisterThruNios(i16LevelDAC, DECADE_DLB1);
		break;
	case CSRB_TRIGENGINE_A2:
		i32Ret = WriteRegisterThruNios(i16LevelDAC, DECADE_DLA2);
		break;
	case CSRB_TRIGENGINE_B2:
		i32Ret = WriteRegisterThruNios(i16LevelDAC, DECADE_DLB2);
		break;
	}

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDecade::SetTriggerSensitivity()
{
	int32 i32Ret = CS_SUCCESS;

	for (uInt16 i = CSRB_TRIGENGINE_A1; i < m_PlxData->CsBoard.NbTriggerMachine; i++)
	{
		i32Ret = SendDigitalTriggerSensitivity(i, m_u8TrigSensitivity);
		if (CS_FAILED(i32Ret))
			return i32Ret;
	}

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDecade::SendDigitalTriggerSensitivity( uInt16  u16TrigEngine, uInt8 u8Sensitive )
{
	uInt16 u16Data = u8Sensitive;

	int32	i32Ret = CS_SUCCESS;		

	switch ( u16TrigEngine )
	{
	case CSRB_TRIGENGINE_A1:
		i32Ret = WriteRegisterThruNios(u16Data, DECADE_DSA1);
		break;
	case CSRB_TRIGENGINE_B1:
		i32Ret = WriteRegisterThruNios(u16Data, DECADE_DSB1);
		break;
	case CSRB_TRIGENGINE_A2:
		i32Ret = WriteRegisterThruNios(u16Data, DECADE_DSA2);
		break;
	case CSRB_TRIGENGINE_B2:
		i32Ret = WriteRegisterThruNios(u16Data, DECADE_DSB2);
		break;

	}

	return i32Ret;
}

//-----------------------------------------------------------------------------
//----- WriteToSelfTestRegister
//----- ADD-ON REGISTER
//-----------------------------------------------------------------------------
int32 CsDecade::WriteToSelfTestRegister( uInt16 u16SelfTestMode )
{

	uInt16	u16Data = 0;

	switch( u16SelfTestMode )
	{
	case CSST_CAPTURE_DATA:
		u16Data = DECADE_CAPTURE_DATA;
		break;
	case CSST_COUNTER_DATA:
		u16Data = DECADE_SELF_COUNTER;
		break;
	default:
		return CS_INVALID_SELF_TEST;
	}
	
	int32 i32Ret = WriteRegisterThruNios(u16Data, DECADE_DATA_SELECT);
	if( CS_FAILED(i32Ret) )
	{
		return i32Ret;
	}
	else
	{
		m_pCardState->AddOnReg.u16SelfTest = u16Data ;
		return CS_SUCCESS;
	}
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void	CsDecade::InitBoardCaps()
{
	GAGE_CT_ASSERT( DECADE_MAX_SR_COUNT >= c_DECADE_SR_INFO_SIZE_3_2 )

	if ( 0 == m_pCardState->u32SrInfoSize )
	{
		if ( 1 == m_PlxData->CsBoard.NbAnalogChannel )
		{
			// Copy the whole sample rate table
			RtlCopyMemory( m_pCardState->SrInfoTable, Decade12_3_2, sizeof(Decade12_3_2) );
			m_pCardState->u32SrInfoSize = sizeof(Decade12_3_2) / sizeof(DECADE_SAMPLE_RATE_INFO);
		}
		else
		{
			uInt32	j = 0;
			// Removed all sample rate that requires pingpong
			for (uInt32 i = 0; i < c_DECADE_SR_INFO_SIZE_3_2; i++ )
			{
				if ( 1 != Decade12_3_2[i].u16PingPong )
					RtlCopyMemory( &m_pCardState->SrInfoTable[j++], &Decade12_3_2[i], sizeof(DECADE_SAMPLE_RATE_INFO) );
			}

			m_pCardState->u32SrInfoSize = j;
		}
	}

	// Regular Rabbit cards
	if ( 0 == m_pCardState->u32FeInfoSize )
	{
		m_pCardState->u32FeInfoSize = c_DECADE_FE_INFO_SIZE;
		RtlCopyMemory( m_pCardState->FeInfoTable, c_FrontEndGain50, sizeof( c_FrontEndGain50 ) );
	}
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

const LONGLONG	cllFactor = 2;


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL	CsDecade::IsNiosInit(void)
{
	uInt32		u32Status = CS_SUCCESS;;

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

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDecade::CsReadPlxNvRAM( PPLX_NVRAM_IMAGE nvRAM_image )
{
	// Simulate Nvram for backward compatible
	// Fill the Nvram structure
	RtlZeroMemory( &nvRAM_image->Data, sizeof(nvRAM_image->Data) );

	nvRAM_image->Data.VendorId					= m_PlxData->PCIConfigHeader.VendorID;
	nvRAM_image->Data.DeviceId					= m_PlxData->PCIConfigHeader.DeviceID;
	nvRAM_image->Data.BaseBoardSerialNumber		= m_PlxData->CsBoard.u32SerNum;
	nvRAM_image->Data.BaseBoardVersion			= (uInt16)m_PlxData->CsBoard.u32BaseBoardVersion;

	nvRAM_image->Data.BaseBoardHardwareOptions	= m_PlxData->CsBoard.u32BaseBoardHardwareOptions;
	nvRAM_image->Data.MemorySize				= m_PlxData->u32NvRamSize;

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDecade::CsWritePlxNvRam( PPLX_NVRAM_IMAGE nvRAM_image )
{
	// Simulate Nvram for backward compatible
	DECADE_MISC_DATA		BoardCfg;

	uInt32 u32Offset	= FIELD_OFFSET(DECADE_FLASH_LAYOUT, BoardData);
	int32 i32Status		= IoctlReadFlash( u32Offset, &BoardCfg, sizeof(BoardCfg) );
	if ( CS_SUCCEEDED(i32Status) )
	{
		BoardCfg.BaseboardInfo.u32SerialNumber	= nvRAM_image->Data.BaseBoardSerialNumber;
		BoardCfg.BaseboardInfo.u32HwVersion		= nvRAM_image->Data.BaseBoardVersion;
		BoardCfg.BaseboardInfo.u32HwOptions		= nvRAM_image->Data.BaseBoardHardwareOptions;
		BoardCfg.u32MemSizeKb					= nvRAM_image->Data.MemorySize;

		i32Status = WriteBaseBoardFlash( u32Offset, &BoardCfg, sizeof(BoardCfg) );
	}
	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDecade::IoctlGetPciExLnkInfo(PPCIeLINK_INFO pPciExInfo)
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
int32	CsDecade::ReadHardwareInfoFromFlash()
{
	// Read BaseBoard board Hardware info
	uInt32		u32Offset;
	int32		i32Status = CS_FLASH_NOT_INIT;

	DECADE_MISC_DATA		BoardCfg;
	u32Offset = FIELD_OFFSET(DECADE_FLASH_LAYOUT, BoardData);

	i32Status = IoctlReadFlash( u32Offset, &BoardCfg, sizeof(BoardCfg) );
	if ( CS_SUCCEEDED(i32Status) )
	{
		// Force these setting to 0 if the EEPROM is not initialized
		if ( -1 != BoardCfg.BaseboardInfo.i64ExpertPermissions )
			m_PlxData->CsBoard.i64ExpertPermissions		= BoardCfg.BaseboardInfo.i64ExpertPermissions;
		else
			m_PlxData->CsBoard.i64ExpertPermissions		= 0;

		// Base board info
		m_PlxData->CsBoard.u32SerNum					= BoardCfg.BaseboardInfo.u32SerialNumber;
		m_PlxData->CsBoard.u32BaseBoardVersion			= CONVERT_EEPROM_OPTIONS(BoardCfg.BaseboardInfo.u32HwVersion);
		m_PlxData->CsBoard.u32BaseBoardHardwareOptions	= CONVERT_EEPROM_OPTIONS(BoardCfg.BaseboardInfo.u32HwOptions);
		m_PlxData->u32NvRamSize							= BoardCfg.u32MemSizeKb;	//in kBytes

		// addon board infos
		m_PlxData->CsBoard.u32SerNumEx					= BoardCfg.AddonInfo.u32SerialNumber;
		m_PlxData->CsBoard.u32AddonBoardVersion			= CONVERT_EEPROM_OPTIONS(BoardCfg.AddonInfo.u32HwVersion);
		m_PlxData->CsBoard.u32AddonHardwareOptions		= CONVERT_EEPROM_OPTIONS(BoardCfg.AddonInfo.u32HwOptions);

		// TrigOut and ClockOut Config
		m_pCardState->bDisableTrigOut = (1 == BoardCfg.u16TrigOutEnable) ? 0 : 1;
		// Get only valid values
		if ((BoardCfg.ClockOutCfg == CS_OUT_NONE) || (BoardCfg.ClockOutCfg == CS_CLKOUT_SAMPLE) || (BoardCfg.ClockOutCfg == CS_CLKOUT_REF))
			m_pCardState->eclkoClockOut = (eClkOutMode)BoardCfg.ClockOutCfg;
		else
			m_pCardState->eclkoClockOut = DECADE_DEFAULT_CLKOUT;
	}
	else
	{
		m_PlxData->CsBoard.i64ExpertPermissions			= 0;
		m_PlxData->CsBoard.u32BaseBoardVersion			= 0;
		m_PlxData->CsBoard.u32SerNum					= 0;
		m_PlxData->CsBoard.u32BaseBoardHardwareOptions	= 0;
		m_PlxData->u32NvRamSize							= 0x400000;		//in kBytes

		m_PlxData->CsBoard.u32SerNumEx					= 0;
		m_PlxData->CsBoard.u32AddonBoardVersion			= 0;
		m_PlxData->CsBoard.u32AddonHardwareOptions		= 0;

		// Default TrigOut and Clock out
		m_pCardState->bDisableTrigOut = TRUE;
		m_pCardState->eclkoClockOut = DECADE_DEFAULT_CLKOUT;
	}

	CS_FIRMWARE_INFOS		FwInfo;

	// Read the default firmware info
	u32Offset = FIELD_OFFSET(DECADE_FLASH_LAYOUT, FlashImage0) + FIELD_OFFSET(DECADE_FLS_IMAGE_STRUCT, FwInfo)
		 + FIELD_OFFSET(DECADE_FWINFO_SECTOR, FwInfoStruct);
	i32Status = IoctlReadFlash( u32Offset, &FwInfo, sizeof(FwInfo) );
	if ( CS_SUCCEEDED(i32Status) && FwInfo.u32ChecksumSignature == CSI_FWCHECKSUM_VALID )
	{
		m_PlxData->CsBoard.u32UserFwVersionB[REGULAR_INDEX_OFFSET].u32Reg	= CONVERT_EEPROM_OPTIONS(FwInfo.u32FirmwareVersion);
		m_PlxData->CsBoard.u32BaseBoardOptions[REGULAR_INDEX_OFFSET]		= CONVERT_EEPROM_OPTIONS(FwInfo.u32FirmwareOptions);
	}

	// No need to read Expert firmware if Expert Permission not present. 
	if ( m_PlxData->CsBoard.i64ExpertPermissions == 0 )
	{
		m_PlxData->CsBoard.u32UserFwVersionB[EXPERT_INDEX_OFFSET].u32Reg 	= 0;
		m_PlxData->CsBoard.u32BaseBoardOptions[EXPERT_INDEX_OFFSET]			= 0;
		return i32Status;
	}
	
	// Read Expert firmware info
	u32Offset = FIELD_OFFSET(DECADE_FLASH_LAYOUT, FlashImage1) + FIELD_OFFSET(DECADE_FLS_IMAGE_STRUCT, FwInfo)
		 + FIELD_OFFSET(DECADE_FWINFO_SECTOR, FwInfoStruct);
	i32Status = IoctlReadFlash( u32Offset, &FwInfo, sizeof(FwInfo) );
	if ( CS_SUCCEEDED(i32Status) && FwInfo.u32ChecksumSignature == CSI_FWCHECKSUM_VALID )
	{
		m_PlxData->CsBoard.u32UserFwVersionB[EXPERT_INDEX_OFFSET].u32Reg	= CONVERT_EEPROM_OPTIONS(FwInfo.u32FirmwareVersion);
		m_PlxData->CsBoard.u32BaseBoardOptions[EXPERT_INDEX_OFFSET]			= CONVERT_EEPROM_OPTIONS(FwInfo.u32FirmwareOptions);
	}

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int64	CsDecade::GetTimeStampFrequency()
{
	int64	i64TimeStampFreq = 0;
	if ( m_pCardState->AcqConfig.u32TimeStampConfig & TIMESTAMP_MCLK )
	{
		i64TimeStampFreq = DECADE_TS_MEMORY_CLOCK;
	}
	else
	{
		i64TimeStampFreq = m_pCardState->AcqConfig.i64SampleRate;
	}

//	if ( m_bPingpongCapture )
//		i64TimeStampFreq <<= 1;

	return i64TimeStampFreq;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsDecade::ReadBusyStatus()
{
	if ( m_bMulrecAvgTD && ! m_bCalibActive )
	{
		uInt32 RegValue = ReadGageRegister( INT_STATUS );
		if ( RegValue & MULREC_AVG_DONE_INTR )
			return 0;
		else
			return STILL_BUSY;
	}
	else
		return CsNiosApi::ReadBusyStatus();
}


//-----------------------------------------------------------------------------
// Converting the trigger timeout from 100ns to the Hw count unit
//-----------------------------------------------------------------------------
uInt32	CsDecade::ConvertTriggerTimeout ( int64 i64Timeout_in_100ns )
{
	uInt32	u32HwTimeout = (uInt32) -1;
	int64	i64ClockRate = DECADE_TS_SAMPLE_CLOCK;

	if ( -1 != i64Timeout_in_100ns )
	{
		if ( m_pCardState->AcqConfig.u32ExtClk )
			i64ClockRate = m_pCardState->AcqConfig.i64SampleRate / 16;

		double dFactor = 1.0 * m_pCardState->AddOnReg.u32Decimation * 1000000000 / i64ClockRate; 

		if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE )
			dFactor *= 2;

		double dHwTimeout = 1.0 * i64Timeout_in_100ns * 100 / dFactor; 

		// for 6G boards, 6G and 2G sampling rate are special cases.
		if ( (m_pCardState->AcqConfig.i64SampleRate == 6000000000) || (m_pCardState->AcqConfig.i64SampleRate == 2000000000) )
		{
			dFactor = 1.0 * 1000000000 / i64ClockRate;
			if (m_pCardState->AcqConfig.i64SampleRate == 2000000000)
				dFactor *= 3;

			dHwTimeout = 1.0 * i64Timeout_in_100ns * 100 /dFactor; 
		}

		u32HwTimeout = (uInt32) (dHwTimeout + 0.5);

		u32HwTimeout = (uInt32) (dHwTimeout + (double)(0.5) );

		// The current firmware support only 29 bits for trigger counter.
		// When all bits is 1. trigger wait infinite. else wait for number counter set.
		// make sure it's in the range of 29bits. 
		if (u32HwTimeout > 0x1FFFFFFE)
			u32HwTimeout = 0x1FFFFFFE;
	}

	return u32HwTimeout;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsDecade::SetTriggerTimeout (uInt32 u32TimeOut)
{
	if ( m_pCardState->BaseBoardRegs.u32TrigTimeOut != u32TimeOut )
	{
		WriteRegisterThruNios( u32TimeOut, DECADE_BB_TRIGTIMEOUT_REG );
		m_pCardState->BaseBoardRegs.u32TrigTimeOut = u32TimeOut;
	}
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsDecade::FindCalLutItem( const uInt32 u32Swing, const uInt32 u32Imped, DECADE_CAL_LUT& Item )
{
	PDECADE_CAL_LUT pLut;
	int nCount(0);

	if (CS_REAL_IMP_50_OHM == u32Imped)
	{
		pLut = s_CalCfg50;
		nCount = c_DECADE_50_CAL_LUT_SIZE;
	}
	else
		return CS_INVALID_IMPEDANCE;
	
	for( int i = 0; i < nCount; i++ )
	{
		if ( u32Swing == pLut[i].u32FeSwing_mV )
		{
			Item = pLut[i];
			return CS_SUCCESS;
		}
	}
	return CS_INVALID_GAIN;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsDecade::SetPosition(uInt16 u16Channel, int32 i32Position_mV)
{
	int32 i32Ret = CS_SUCCESS;

	if( (0 == u16Channel) || (u16Channel > DECADE_CHANNELS)  )
	{
		return CS_INVALID_CHANNEL;
	}

	uInt16 u16ChanZeroBased = u16Channel - 1;

	if ( m_pCardState->ChannelParams[u16ChanZeroBased].i32Position != i32Position_mV )
	{
		if( m_pCardState->bCalNeeded[u16ChanZeroBased] )
		{
			//store position it will be set after calibration
			m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = i32Position_mV;
		}
		else
		{
			i32Ret = SendPosition(u16Channel, i32Position_mV);
		}
	}
	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsDecade::SendPosition(uInt16 u16Channel, int32 i32Position_mV)
{
	if( (0 == u16Channel) || (u16Channel > DECADE_CHANNELS)  )
		return CS_INVALID_CHANNEL;
	uInt16 u16ChanZeroBased = u16Channel-1;
	uInt32	u32Command = (CS_CHAN_1 == u16Channel) ? DECADE_COARSE_OFFSET_0 : DECADE_COARSE_OFFSET_1;

	if ( m_bFirmwareChannelRemap )
		u32Command = (CS_CHAN_2 == u16Channel) ? DECADE_COARSE_OFFSET_0 : DECADE_COARSE_OFFSET_1;

	int32 i32FrontUnipolar_mV = m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange / 2;
	
	if( i32Position_mV > i32FrontUnipolar_mV ) i32Position_mV = i32FrontUnipolar_mV;
	if( i32Position_mV <-i32FrontUnipolar_mV ) i32Position_mV =-i32FrontUnipolar_mV;
	
	uInt32 u32RangeIndex, u32FeModIndex;
	int32 i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );
	if( CS_FAILED(i32Ret) ) return i32Ret;

	uInt16 u16CodeDeltaForHalfIr = m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CodeDeltaForHalfIR;
	
	// Hardware not working yet. If this range can not be found. use the default the value.
	if (!u16CodeDeltaForHalfIr)
		u16CodeDeltaForHalfIr = (uInt16) c_u32CalDacCount/4;
	else
		u16CodeDeltaForHalfIr *= 2;
		
	if( 0 != u16CodeDeltaForHalfIr  )
	{
		int32 i32PosCode = -(2 * i32Position_mV * u16CodeDeltaForHalfIr)/i32FrontUnipolar_mV;
		i32PosCode += m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset;
		uInt32 u32Code = i32PosCode > 0 ? ( i32PosCode < int32(c_u32CalDacCount-1) ? uInt32(i32PosCode) : c_u32CalDacCount-1 ) : 0;
		
		m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = i32Position_mV;

		//char szText[256]={0};
		//sprintf_s( szText, sizeof(szText),  "=== SendPosition() Channel %d Pos_mV/Unipolar_mV (%d / %d) PosCode A %d PosCode B %d Code %d DeltaForHalfIr %d CoarseOffset %d \n",
		//			u16Channel, i32Position_mV, i32FrontUnipolar_mV, i32Tmp, i32PosCode, u32Code, u16CodeDeltaForHalfIr,
		//			m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset);
		//::OutputDebugString( szText );
		
		i32Ret = WriteRegisterThruNios( u32Code, u32Command );
		return CS_SUCCESS;
	}
	else
	{
		m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = 0;
		return CS_FALSE;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDecade::GetConfigBootLocation()
{
	int32	i32BootLocation = 0;
	uInt32	u32Val = ReadConfigBootLocation();

	switch (u32Val<<2)
	{
		case 0x2180000:
			i32BootLocation = 1;
			break;
		case 0x4300000:
			i32BootLocation = 2;
			break;
	}

	return i32BootLocation;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDecade::SetConfigBootLocation( int32 i32BootLocation )
{
	uInt32	u32Val = 0;

	switch (i32BootLocation)
	{
		case 2:		// eXpert Image
			u32Val = 0x4300000;
			break;
		case 1:		// Default Image
			u32Val = 0x2180000;
			break;
		case 0:		// Safe boot image
			u32Val = 0xFFFFFFFF;
			break;
		default:
			return CS_INVALID_PARAMETER;
	}

	return WriteConfigBootLocation( u32Val>>2 );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDecade::WriteConfigBootLocation( uInt32	u32Location )
{
	char	szText[256];

	// The address where the firmware use to detect the boot location
	uInt32 u32Addr = (0x1FFFFF7<<2);

	int32 i32Status = WriteBaseBoardFlash( u32Addr, &u32Location, sizeof(u32Location) );
	if ( CS_SUCCEEDED(i32Status) )
	{
		uInt32	u32Val = 0xFFFFFFFF;
		i32Status = IoctlReadFlash( u32Addr, &u32Val, sizeof(u32Val) );
		if ( CS_SUCCEEDED(i32Status) )
		{
			if ( u32Location == u32Val )
			{
				sprintf_s( szText, sizeof(szText), TEXT("Boot location updated successfully 0x%x."), u32Location );
			}
			else
				sprintf_s( szText, sizeof(szText), TEXT("Boot location Read/Write Discrepancy."));
		}
	}
	else
		sprintf_s( szText, sizeof(szText), TEXT("Boot location updated failed (WriteFlash error)."));

	CsEventLog EventLog;
	EventLog.Report( CS_INFO_TEXT, szText );

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32	CsDecade::ReadConfigBootLocation()
{
	uInt32	u32Val = 0;

	int32	i32Status = IoctlReadFlash( (0x1FFFFF7<<2), &u32Val, sizeof(u32Val) );
	if ( CS_FAILED(i32Status) )
		return 0;
	
	return u32Val;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDecade::FlashIdentifyLED(bool bOn)
{
	uInt32 u32RegVal = 0;
	
	if ( bOn )
		u32RegVal = 0x300;

	return WriteRegisterThruNios( u32RegVal, DECADE_USER_LED_CRTL );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDecade::SetClockControlRegister( uInt32 u32Val, bool bForce )
{
	int32 i32Ret = CS_SUCCESS;
	
//	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("SetClockControlRegister Value [ 0x%x Command 0x%x ] \n "), 
//			u32Val, DEACDE_CLOCK_CONTROL | DECADE_SPI_WRITE);
//	::OutputDebugString( szDbgText );
	
	if ( bForce || u32Val != m_pCardState->ClockReg.u32RegVal )
	{
		i32Ret = WriteRegisterThruNios(u32Val, DEACDE_CLOCK_CONTROL | DECADE_SPI_WRITE );
		if( CS_SUCCEEDED(i32Ret) )
			 m_pCardState->ClockReg.u32RegVal = u32Val;
	}

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDecade::SetTriggerIoRegister( uInt32 u32Val, bool bForce )
{
	int32 i32Ret = CS_SUCCESS;
	uInt32 u32Cmd = DECADE_TRIGGER_IO_CONTROL | DECADE_SPI_WRITE;

	if (m_PlxData->CsBoard.u32AddonBoardVersion >= ADDON_HW_VERSION_1_07)
		u32Cmd = DECADE_TRIGOUT_N_PINGPONG_SETUP | DECADE_SPI_WRITE;

//	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("SetTriggerIoRegister Value [ 0x%x Command 0x%x ] CurVal 0x%x \n "), u32Val, u32Cmd, 
//				m_pCardState->TrigOutnPingPongReg.u32RegVal);
//	::OutputDebugString( szDbgText );

	if ( bForce || u32Val != m_pCardState->TrigIoReg.u32RegVal )
	{
//		if ( m_PlxData->CsBoard.u32AddonBoardVersion >= ADDON_HW_VERSION_1_07)
//			u32Cmd = DECADE_TRIGOUT_N_PINGPONG_SETUP | DECADE_SPI_WRITE;

		i32Ret = WriteRegisterThruNios(u32Val, u32Cmd );
		if( CS_SUCCEEDED(i32Ret) )
			 m_pCardState->TrigIoReg.u32RegVal = u32Val;
	}

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDecade::SetTrigOut( uInt16 u16Value )
{
	int32 i32Ret = CS_SUCCESS;
	DECADE_TRIG_IO_REG	TrigIoReg ={0};
	//sprintf_s( szDbgText, sizeof(szDbgText), TEXT("SetTrigOut. u16Value = 0x%x "), u16Value);
	//::OutputDebugString( szDbgText );

	if ( m_PlxData->CsBoard.u32AddonBoardVersion >= ADDON_HW_VERSION_1_07)
	{

		// New firmware use A1h register for handling both trigger out and pingpong.
		// TriggerOutEn bit in CPLD Misc control move to bit 0 in A1h
		if ( CS_TRIGOUT == u16Value )
		{
			TrigIoReg.bits.TriggerOut = 0;
			m_pCardState->bDisableTrigOut = FALSE;
		}
		else
		{
			TrigIoReg.bits.TriggerOut = 1;
			m_pCardState->bDisableTrigOut = TRUE;
		}

		//TrigIoReg.bits.PingPong = m_pCardState->AddOnReg.u16PingPong ? 1 : 0;
	}
	else
	{
		TrigIoReg = m_pCardState->TrigIoReg;
		
		if ( CS_TRIGOUT == u16Value )
		{
			TrigIoReg.bits_addon_1v6.TrigOutEn	= 1;
			TrigIoReg.bits_addon_1v6.TrigOutEn_N	= 0;
			m_pCardState->bDisableTrigOut = FALSE;
		}
		else
		{
			TrigIoReg.bits_addon_1v6.TrigOutEn	= 0;
			TrigIoReg.bits_addon_1v6.TrigOutEn_N	= 1;
			m_pCardState->bDisableTrigOut = TRUE;
		}
	}
	i32Ret = SetTriggerIoRegister( TrigIoReg.u32RegVal );

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDecade::SetClockOut( eClkOutMode ClkOutValue )
{
	DECADE_CLOCK_REG	ClkReg = m_pCardState->ClockReg;

	ClkReg.bits.Pll_RfOut_En = 0;
	ClkReg.bits.ClkOut_SrcSel = (CS_CLKOUT_REF == ClkOutValue);

	int32	i32Ret = SetClockControlRegister( ClkReg.u32RegVal );
	if ( CS_SUCCEEDED(i32Ret) )
		m_pCardState->eclkoClockOut = ClkOutValue;

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDecade::SendADC_LVDS_Align( bool bKeep_ADC_Settings, bool bForce /* = false */ )
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32RetCode = 0;
	uInt32	u32Command = bKeep_ADC_Settings ? DEACDE_ADC_LVDS_ALIGN_SHORT : DEACDE_ADC_LVDS_ALIGN;
	uInt32	u32SuccessVal = 0x3FFFFFF;
	int		i = 0;
	char	szText[128] = {0};

	// software should not call DECADE_ADC_LVDS_ALIGN starting from firmware version 100
	if (m_bSkipLVDS_Cal)
		return CS_SUCCESS;

	if (  bForce || (m_pCardState->bAddOnRegistersInitialized && ! m_pCardState->bAdcLvdsAligned) )
	{
		bool b6G_card = (CSDECADE126G_BOARDTYPE == m_PlxData->CsBoard.u32BoardType);
		uInt32	u32Data = 0;
		
		if (b6G_card && (m_PlxData->CsBoard.u32AddonBoardVersion >= ADDON_HW_VERSION_1_07)
			&& (m_pCardState->AddOnReg.u16PingPong != 0))
		{
			u32SuccessVal = 0x7FFFFFF;
			if (m_pCardState->AcqConfig.u32ExtClk != 0 && m_pCardState->AddOnReg.u32ClkFreq != (CS_SR_3GHZ /1000000) )
			{
				u32Command = DEACDE_ADC_INTERNAL_CALIB;
				u32Data = m_u32PingPongShift;
			}
			else
				u32Data = 1;
		}

		// LVDS command may cause OVER VOLTAGE
		WriteGageRegister(INT_CFG, 0);

		// Retry 10 times
		for (i = 0; i < 10; i++)
		{
			//i32Ret = WriteRegisterThruNios(0 != m_pCardState->AddOnReg.u16PingPong, u32Command, DECADE_TIMEOUT_4SEC, &u32RetCode );
			i32Ret = WriteRegisterThruNios(u32Data, u32Command, DECADE_TIMEOUT_10SEC, &u32RetCode );
#ifdef _DEBUG			
			sprintf_s( szText, sizeof(szText), TEXT("WriteRegisterThruNios(bData %d, u32Command 0x%x)  u32RetCode=0x%x u32ClkFreq %ld \n"), 
				u32Data, u32Command, u32RetCode, m_pCardState->AddOnReg.u32ClkFreq );
			OutputDebugString(szText);
#endif			
			if ( CS_SUCCEEDED(i32Ret) )
			{
				if ( u32SuccessVal == (u32RetCode & u32SuccessVal) )
				{
					m_pCardState->bAdcLvdsAligned = true;
					i32Ret = CS_SUCCESS;
					break;
				}
				else
				{
					if (b6G_card && 0 == u32Data)
					{
						// Partial aligment success ?
						if ( 0x9FFFFFF == (u32RetCode & 0x9FFFFFF) )
						{
							m_pCardState->bAdcLvdsAligned = true;
							i32Ret = CS_SUCCESS;
							break;
						}
					}

					i32Ret = CS_LVDS_ALIGNMENT_FAILED;
				}
			}
		}

		// Restore OVER VOLTAGE protection.
		WriteGageRegister(INT_CFG, OVER_VOLTAGE_INTR);

//		if ( bKeep_ADC_Settings )
//			SendAdcReggister0(m_pCardState->AddOnReg.u16AdcRegister0, true);

		// While the firmare calibrates LVDS, it resets the ADC setting to the default
		// This affects the calibration (Gain, offset) of the front end. We have to re-calibrate both channels
		if ( ! bKeep_ADC_Settings )
		{
			m_pCardState->bCalNeeded[0] = 
			m_pCardState->bCalNeeded[1] = true;
		}

		// Log the error
		if ( CS_LVDS_ALIGNMENT_FAILED == i32Ret )
		{
			CsEventLog EventLog;

			sprintf_s( szText, sizeof(szText), TEXT("LVDS Calib (Cmd 0x%x) failed %d times. RetCode=0x%x expected 0x%x \n"), 
				(u32Command >> 20), i, u32RetCode, u32SuccessVal );
#ifdef _DEBUG
			OutputDebugString(szText);
#endif
			EventLog.Report( CS_ERROR_TEXT, szText );
		}
		else
		{
#ifdef _DEBUG
			sprintf_s( szText, sizeof(szText), TEXT("LVDS Calib (Cmd 0x%x) SUCCESS after %d times. RetCode=0x%x\n"), 
				(u32Command >> 20), (i+1), u32RetCode );
			OutputDebugString(szText);
#endif
		}
	}

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
// m_bNeverCalibExtTrig use registry key "NeverCalibExtTrig"
// m_bForceCalibExtTrig use registry key "ForceCalibExtTrig"
//-----------------------------------------------------------------------------
int32 CsDecade::SendExtTriggerAlign()
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	i = 0;
	PARRAY_CHANNELCONFIG	pAChannel = 0;
	PARRAY_TRIGGERCONFIG	pATrigger = 0;
	uInt32	u32NiosReturn = 0;
	BOOLEAN bExtTriggerAlign = FALSE;
	char	szText[128];
	
	if ( ( m_bNeverCalibExtTrig || m_pCardState->bExtTriggerAligned ) && (!m_bForceCalibExtTrig) )
		return i32Ret;

#ifdef EXTTRIGALIN_BACKUP_RESTORE_REQUIRED
	// Backup the old settings
	m_OldAcqConfig.u32Size = sizeof(CSACQUISITIONCONFIG);
	CsGetAcquisitionConfig( &m_OldAcqConfig );

	pAChannel = (PARRAY_CHANNELCONFIG) GageAllocateMemoryX( sizeof(ARRAY_CHANNELCONFIG) + (GetBoardNode()->NbAnalogChannel-1)*sizeof(CSCHANNELCONFIG) );
	pATrigger = (PARRAY_TRIGGERCONFIG) GageAllocateMemoryX( sizeof(ARRAY_TRIGGERCONFIG) + 2*GetBoardNode()->NbAnalogChannel*sizeof(CSTRIGGERCONFIG) );

	if ( NULL == pAChannel || NULL == pATrigger )
	{
		i32Ret = CS_MEMORY_ERROR;
		goto EndSendExtTriggerAlign;
	}
		
	pAChannel->u32ChannelCount = GetBoardNode()->NbAnalogChannel;
	for (i = 0; i< pAChannel->u32ChannelCount; i++)
	{
		pAChannel->aChannel[i].u32Size = sizeof(CSCHANNELCONFIG);
		pAChannel->aChannel[i].u32ChannelIndex = i+1;
	}

	i32Ret = CsGetChannelConfig( pAChannel );
	if ( CS_FAILED( i32Ret) )
		goto EndSendExtTriggerAlign;

	pATrigger->u32TriggerCount = (1+2*GetBoardNode()->NbAnalogChannel);
	for (i = 0; i< pATrigger->u32TriggerCount; i++)
	{
		pATrigger->aTrigger[i].u32Size = sizeof(CSTRIGGERCONFIG);
		pATrigger->aTrigger[i].u32TriggerIndex = i+1;
	}

	i32Ret = CsGetTriggerConfig( pATrigger );
	if ( CS_FAILED( i32Ret) )
         goto EndSendExtTriggerAlign;
#endif


	// Align Ext trigger
	// the Command DEACDE_EXT_TRIG_ALIGN will put he card in Calib mode and will generate an interrupt OVER_VOLTAGE
	// protection. In order to prevent the interrupt, we have to disable the interrupt.
	// Disable all interrupts
	WriteGageRegister( INT_CFG, 0 );

	for(i= 0; i<2; i++)
	{
		i32Ret = WriteRegisterThruNios(0, DECADE_EXT_TRIG_ALIGN, DECADE_TIMEOUT_30SEC, &u32NiosReturn, false );
		if ( CS_SUCCEEDED(i32Ret) )
		{
			if ( 0x3 == u32NiosReturn )
			{
				bExtTriggerAlign = TRUE;
				break;
			}
		}
	}

	// Log the error
	if ( bExtTriggerAlign==FALSE )
	{
		CsEventLog EventLog;
		sprintf_s( szText, sizeof(szText), TEXT("External Trigger Calib (0x%lx) failed (0x%x) %d times \n"), 
				DECADE_EXT_TRIG_ALIGN, u32NiosReturn, i);
#ifdef _DEBUG
		OutputDebugString(szText);
#endif
		EventLog.Report( CS_ERROR_TEXT, szText );
	}
	else
	{
#ifdef _DEBUG
		sprintf_s( szText, sizeof(szText), TEXT("External Trigger Calib (0x%lx) success (0x%x) after %d times \n"), 
				DECADE_EXT_TRIG_ALIGN, u32NiosReturn, i+1);
		OutputDebugString(szText);
#endif
		
	}		
	

	// While executing the command DECADE_EXT_TRIG_ALIGN, the firmware has changed some channel and trigger settings.
	// We need to reset the cache here so that we can restore the setting we use later
	memset( m_pCardState->AddOnReg.u32FeIndex, -1, sizeof(m_pCardState->AddOnReg.u32FeIndex));
	memset( m_pCardState->AddOnReg.u32FeSetup, 0, sizeof(m_pCardState->AddOnReg.u32FeSetup));
	m_pCardState->AddOnReg.u16ExtTrigEnable = (uInt16)-1;

	m_pCardState->AddOnReg.u32FeSetup[0].bits.Channel = 1;
	m_pCardState->AddOnReg.u32FeSetup[1].bits.Channel = 2;

	// Restore the settings
	Sleep(100);
	m_TrigCtrl_Reg.u16RegVal = (uInt16) -1;
	
	// The command above clear trigger out setting. Restore the trigger out.
	if ( m_PlxData->CsBoard.u32AddonBoardVersion < ADDON_HW_VERSION_1_07)
	{
		m_pCardState->TrigIoReg.u32RegVal = 0;		// Clear the record value in oder to update register. 
		if ( m_pCardState->bDisableTrigOut == false)
			SetTrigOut( CS_TRIGOUT );	
		else
			SetTrigOut( CS_OUT_NONE );	
	}
	
//	m_pCardState->AddOnReg.u16ExtTrigEnable = (uInt16) -1;
//	CsSetAcquisitionConfig( &m_OldAcqConfig );
//	CsSetChannelConfig( pAChannel );
//	CsSetTriggerConfig( pATrigger );
//	m_pCardState->bCalNeeded[0] = m_pCardState->bCalNeeded[1] = 0;

#ifdef EXTTRIGALIN_BACKUP_RESTORE_REQUIRED
EndSendExtTriggerAlign:
#endif

	{
		m_pCardState->bExtTriggerAligned = bExtTriggerAlign;
		// Restore the old setting
		SendCalibModeSetting();

		if ( pAChannel )
			GageFreeMemoryX( pAChannel );
		if ( pATrigger )
			GageFreeMemoryX( pATrigger );
	}

	if (bExtTriggerAlign==FALSE) 
		i32Ret = CS_EXTTRIG_CALIB_FAILURE;
    else
		i32Ret = CS_SUCCESS;


	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CsDecade::isPLLLocked()
{
	uInt32	u32Command = DECADE_ADDON_CPLD_MISC_STATUS;
	uInt32	u32Val = ReadRegisterFromNios( 0, u32Command );
	
	if ( 0 == (u32Val & 0x2) )	
		return (FALSE);
	else
		return (TRUE);
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDecade::LockPLL()
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32Command = DECADE_ADDON_CPLD_MISC_STATUS;
	uInt32	u32Val = ReadRegisterFromNios( 0, u32Command );

	// Bug from the firmware. the status of PLL is not stable at the first reading after
	// some nios commands. Read the second time give a better status ???
	u32Val = ReadRegisterFromNios( 0, u32Command );
	if ( 0 == (u32Val & 0x2) )
	{
		// no need to reset PLL
		BBtiming( 25 );
		// PLL should be locked by now
		u32Val = ReadRegisterFromNios( 0, u32Command );
	}

	return i32Ret;
}

#define DECADE_RECONFIG_BIT			0x800000
#define DECADE_WAITCONFIGRESET			6000		// ms
#define NUCLEON_CONFIG_RESET			0x11
#define NUCLEON_REACONFIG_ADDR0			0x7
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

BOOLEAN CsDecade::ConfigurationReset( uInt32 u32BootLocation )
{
	if ( u32BootLocation > 2 )
		return TRUE;

	uInt32	u32Addr = FIELD_OFFSET(DECADE_FLASH_LAYOUT, FlashImage0);

	u32Addr += u32BootLocation*FIELD_OFFSET(DECADE_FLASH_LAYOUT, FlashImage0);

	u32Addr >>= 4;		// Word address
	WriteCpldRegister(11, (u32Addr & 0xFFFF));
	u32Addr >>= 16;
	WriteCpldRegister(12, (u32Addr & 0xFFFF));

	WriteGageRegister( 49, DECADE_RECONFIG_BIT );
	WriteGageRegister( 49, 0 );

	Sleep ( DECADE_WAITCONFIGRESET );
	
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


	return TRUE;
/*
	// Check the last Firmware page load
	uInt32 u32RegVal = ReadGioCpld(6);

	
	if ( (u32RegVal & 0x40) == 0 )
	{
		m_pCardState->u8ImageId = u8UserImageId;
		return TRUE;
	}
	else
		return FALSE;
*/
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsDecade::CsCopyFFTWindowParams ( uInt32 u32WinSize, int16 *pi16Coef )
{
	if( 0 == RtlEqualMemory(m_i16CoefficientFFT, pi16Coef, u32WinSize * sizeof(int16)) )
	{
		RtlCopyMemory( m_i16CoefficientFFT, pi16Coef, u32WinSize * sizeof(int16) );

		CsConfigFFTWindow();
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsDecade::CsConfigFFTWindow ()
{
	if ( ! m_FFTConfig.bWindowing )
		return;

	uInt32 u32Coefficient = 0;

	// Write coeffecients 
	// Two coeffecients get written at a time, Coef 1 in low word, coef 2 in high word, etc.
	// There are N/2 coeffecients
	for (uInt16 i = 0, j = 0; i < m_FFTConfig.u16FFTSize/2; i += 2, j++ )
	{
		u32Coefficient = ((m_i16CoefficientFFT[i+1] & 0xfff) << 16) | (m_i16CoefficientFFT[i] & 0xfff);
		WriteGageRegister( DECADE_FFT_USER_INDEX, j);
		WriteGageRegister( DECADE_FFT_USER_DATA, u32Coefficient );
	}		
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsDecade::CsStartFFT ()
{
	m_u32FftReg.bits.Enabled = 1;
	WriteGageRegister( DECADE_FFT_CONTROL, m_u32FftReg.u32RegVal );		
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsDecade::CsStopFFT ()
{
	m_u32FftReg.bits.Enabled = 0;
	WriteGageRegister( DECADE_FFT_CONTROL, m_u32FftReg.u32RegVal );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsDecade::CsWaitForFFT_DataReady ()
{
	uInt32	u32HostReadFifoEmpty = 0;
	uInt32	u32Count = 200;

	while ( u32Count > 0 )
	{
//		u32DataFifo = ReadRegisterFromNios( 0,  DECADE_RD_BB_REGISTER | 0x32 );
		u32HostReadFifoEmpty = ReadGageRegister( MISC_REG );
		if ( 0 == (u32HostReadFifoEmpty & 0x100) )
			break;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsDecade::CsCopyFFTparams ( CS_FFT_CONFIG_PARAMS *pFFT )
{
	m_FFTConfig.u16FFTSize		= 8192;
	m_FFTConfig.bWindowing		= pFFT->bWindowing;
	m_FFTConfig.bIFFT			= pFFT->bIFFT;

	m_u32FftReg.bits.Window		= m_FFTConfig.bWindowing;
	m_u32FftReg.bits.Inverted	= m_FFTConfig.bIFFT;
}



//-----------------------------------------------------------------------------
// This function is for debugging SendNiosCommand at user mode level instead of Kernel
//-----------------------------------------------------------------------------

#define NIOS_TIMOUT_MSEC			(1000000/8)	//one msec time out


int32	CsDecade::WriteRegisterThruNiosEx(uInt32 Data, uInt32 Command, int32 i32Timeout, uInt32 *pu32NiosReturn )
{
	uInt32 status1, timeout = 10*NIOS_TIMOUT_MSEC;
	int32  i32Ret = CS_SUCCESS;


	if ( i32Timeout > 0)
		timeout = (uInt32) (i32Timeout*NIOS_TIMOUT_MSEC);

	status1 = ReadGageRegister(MISC_REG);
	if ( (status1 & EXE_DONE_FROM_NIOS)!=0 )
	{
		WriteGageRegister(MISC_REG, ACK_TO_NIOS);
		WriteGageRegister(MISC_REG, 0);
	}

	// Set watch dog timer
	WriteGageRegister(WD_TIMER, timeout);
	// start Watch dog timer
	WriteGageRegister(GEN_COMMAND, WATCH_DOG_STOP_MASK);
	WriteGageRegister(GEN_COMMAND, WATCH_DOG_RUN_MASK);
	WriteGageRegister(GEN_COMMAND, 0);


	//----		2- send the write command in "command_to_nios" to the nios (write to spi)
	WriteGageRegister(COMMAND_TO_NIOS, Command);
	//----		3- send the data to the nios ("data_write_to_nios")
	WriteGageRegister(DATA_WRITE_TO_NIOS, Data);
	//----		4- write the "exe_to_nios"
	WriteGageRegister(MISC_REG, 0);
	WriteGageRegister(MISC_REG, EXE_TO_NIOS);

	//----		1- read the "exe_done from_nios" bit
	status1 = ReadGageRegister(MISC_REG);
	while ((status1 & EXE_DONE_FROM_NIOS)==0)
	{
		status1 = ReadGageRegister(MISC_REG);
		if ( 0 != (ReadGageRegister( GEN_COMMAND ) & WD_TIMER_EXPIRED) )
		{
			i32Ret = CS_FRM_NO_RESPONSE;
			break;
		}
	}

	if ( (status1 & PASS_FAIL_NIOS_BIS) == PASS_FAIL_NIOS_BIS )
	{
		i32Ret = CS_INVALID_FRM_CMD;
	}


	if ( CS_SUCCESS == i32Ret  &&  0 != pu32NiosReturn )
		*pu32NiosReturn = ReadGageRegister(DATA_READ_FROM_NIOS);

	//----		5- Write ack_to_nios = 1 to say I'm done with the previous one, ready for the next
	WriteGageRegister(MISC_REG, ACK_TO_NIOS);
	WriteGageRegister(MISC_REG, 0);

	return i32Ret;
}

//-----------------------------------------------------------------------------
// Set the DataPack register 
//-----------------------------------------------------------------------------

int32	CsDecade::SetDataPackMode(CsDataPackMode DataPackMode, bool bForce )
{
	int32	i32Status = CS_SUCCESS;
	uInt32	u32Data = 0;
	uInt32	u32TailSize = DECADE_HWMULREC_TAIL_BYTES;

	if ( bForce || DataPackMode != m_DataPackMode )
	{
		switch (DataPackMode)
		{
			case DataPacked_8:
				u32Data = 5;
				u32TailSize = 64;
				break;
			case DataPacked_12:
				u32Data = 2;
				u32TailSize = 120;
				break;

			case DataUnpack:
			default:
				break;
		}

		i32Status = WriteRegisterThruNios(u32Data, CSNIOS_SET_PACK_MODE_CMD);

		if( CS_SUCCEEDED( i32Status) )
		{
			if ( DataPacked_8 == DataPackMode )
				m_u8DepthIncrement = DECADE_DEPTH_INCREMENT_PACK8;
			else
				m_u8DepthIncrement = DECADE_DEPTH_INCREMENT;

			m_DataPackMode			= DataPackMode;
			m_32SegmentTailBytes	= u32TailSize;
			CsSetDataFormat();
		}
	}

	return i32Status;
}

//-----------------------------------------------------------------------------
// The firmware always prevents reading the memory beyond the segment size.
// In multi segments transfer mode, we allow the user to read multiple segments in one data transfer.
// In order to read the full memory, the SW driver have to change the segment size to the size of memory
//-----------------------------------------------------------------------------
int32	CsDecade::SetupForReadFullMemory(bool bSetup)
{
	int32 i32Status = CS_SUCCESS;

	if (bSetup)
	{
		if (!m_pCardState->bMultiSegmentTransfer)
		{
			// Remember the current segment settinngs
			m_OldAcqConfig = m_pCardState->AcqConfig;

			// Calculate PeudoMax Depth (max memory minus tail size)
			int64 i64PseudoMaxDepth = (m_pSystem->SysInfo.i64MaxMemory - m_32SegmentTailBytes) / (m_OldAcqConfig.u32Mode & CS_MASKED_MODE);

			// Round down to trigger resolution
			i64PseudoMaxDepth /= DECADE_DEPTH_INCREMENT;
			i64PseudoMaxDepth *= DECADE_DEPTH_INCREMENT;

			// Change the segment settings so that we can read accorss multiple segments
			// Set as one single segment with max memory minus tail size
			i32Status = SendSegmentSetting(1, i64PseudoMaxDepth, i64PseudoMaxDepth, 0, 0);
			if (CS_FAILED(i32Status))
				return i32Status;

			CSACQUISITIONCONFIG_EX AcqCfgEx = { 0 };
			AcqCfgEx.AcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);
			CsGetAcquisitionConfig(&AcqCfgEx.AcqCfg);

			AcqCfgEx.AcqCfg.i64TriggerDelay = 0;
			AcqCfgEx.i64HwDepth			= i64PseudoMaxDepth;
			AcqCfgEx.i64HwSegmentSize	= i64PseudoMaxDepth;
			AcqCfgEx.u32Decimation		= m_pCardState->AddOnReg.u32Decimation;
			AcqCfgEx.u32ExpertOption	= m_PlxData->CsBoard.u32BaseBoardOptions[m_pCardState->u8ImageId];

			i32Status = IoctlSetAcquisitionConfigEx(&AcqCfgEx);
			if (CS_FAILED(i32Status))
			{
				// Restore to the current settings.
				SendSegmentSetting(m_OldAcqConfig.u32SegmentCount, m_OldAcqConfig.i64Depth, m_OldAcqConfig.i64SegmentSize, m_OldAcqConfig.i64TriggerHoldoff, m_OldAcqConfig.i64TriggerDelay);
			}

			m_pCardState->bMultiSegmentTransfer = TRUE;
		}
	}
	else 
	{
		if (m_pCardState->bMultiSegmentTransfer)
		{
			// Restore the segment settings.
			i32Status = SendSegmentSetting(m_OldAcqConfig.u32SegmentCount,  m_OldAcqConfig.i64Depth,m_OldAcqConfig.i64SegmentSize, m_OldAcqConfig.i64TriggerHoldoff, m_OldAcqConfig.i64TriggerDelay);
			if (CS_FAILED(i32Status))
				return i32Status;

			CSACQUISITIONCONFIG_EX AcqCfgEx = { 0 };
			AcqCfgEx.AcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);
			CsGetAcquisitionConfig(&AcqCfgEx.AcqCfg);

			AcqCfgEx.i64HwDepth			= m_OldAcqConfig.i64Depth;
			AcqCfgEx.i64HwSegmentSize	= m_OldAcqConfig.i64SegmentSize;
			AcqCfgEx.u32Decimation		= m_pCardState->AddOnReg.u32Decimation;
			AcqCfgEx.u32ExpertOption	= m_PlxData->CsBoard.u32BaseBoardOptions[m_pCardState->u8ImageId];

			i32Status = IoctlSetAcquisitionConfigEx(&AcqCfgEx);
			m_pCardState->bMultiSegmentTransfer = FALSE;
		}
	}

	return i32Status;
}

//-----------------------------------------------------------------------------
// Adjust timing for a specific external clock rate
//-----------------------------------------------------------------------------
void	CsDecade::ClockPingpongAdjust(uInt32	u32Adjust)
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32RetCode = 0;

	if (u32Adjust == m_u32CurrentPingPongShift)
		return;

	if (m_pCardState->AcqConfig.u32ExtClk != 0 && m_pCardState->VerInfo.BaseBoardFwVersion.Version.uIncRev >= 137)
	{
		char szText[128];
		BBtiming(500);
		i32Ret = WriteRegisterThruNios(u32Adjust, DEACDE_ADC_INTERNAL_CALIB, -1, &u32RetCode);
		{
			sprintf_s(szText, sizeof(szText), TEXT("Pingpong Adjust (Cmd=0x3800000, Val=0x%x) RetCode =%d)\n"), u32Adjust, i32Ret);
		}
		if (CS_FAILED(i32Ret))
		{
//			char szText[128];
//			sprintf_s(szText, sizeof(szText), TEXT("Pingpong Adjust (Cmd=0x3800000) failed (RetCode =%d)\n"), i32Ret);
#ifdef _DEBUG
			OutputDebugString(szText);
#endif
			CsEventLog EventLog;
			EventLog.Report(CS_ERROR_TEXT, szText);
		}
		else
			m_u32CurrentPingPongShift = u32Adjust;
	}
}