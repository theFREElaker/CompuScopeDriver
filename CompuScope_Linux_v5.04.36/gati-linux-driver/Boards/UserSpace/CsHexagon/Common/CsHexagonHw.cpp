//	CsHexagonHw.cpp
///	Implementation of CsHexagon class
//
//	This file contains all member functions that access to addon hardware including
//	Frontend set up and calibration
//
//

#include "StdAfx.h"
#include "CsTypes.h"
#include "GageWrapper.h"
#include "CsHexagon.h"
#include "CsxyG8CapsInfo.h"
#include "CsIoctl.h"

#ifdef _WINDOWS_
#include "CsMsgLog.h"
#endif

#define	 CSRB_CALIB_TIMEOUT_SHORT	KTIME_MILISEC(300)		// 300 milisecond timeout


const FRONTEND_HEXAGON_INFO	CsHexagon::c_FrontEndGain50[] = 
{  	//Gain  Val				PreBypass	Att_20Db	Att_6Db
	 CS_GAIN_2_V,	0x00003,	//  0			1			1
};

const FRONTEND_HEXAGON_INFO	CsHexagon::c_FrontEndGainLowRange50[] =
{  	//Gain  Val				PreBypass	Att_20Db	Att_6Db
	HEXAGON_GAIN_LR_MV,	0x00007,	//  0			1			1
};

#define c_HEXAGON_FE_INFO_SIZE   (sizeof(CsHexagon::c_FrontEndGain50)/sizeof(FRONTEND_HEXAGON_INFO))

// As Desgin
//  HighRange:  2000mv. Voltage DC Reference 512mv 
//  LowRange:   480mv.  Voltage DC Reference 120mv.
HEXAGON_CAL_LUT	CsHexagon::s_CalCfg50[] = 
{ //u32FeSwing_mV Vref_factor	Gain Control
	    0,       20.00,		0x1A,	
	HEXAGON_GAIN_LR_MV,		120.00,		HEXAGON_DC_GAIN_CODE_LR,
	CS_GAIN_2_V,			512.00,		HEXAGON_DC_GAIN_CODE_1V,
};
#define c_HEXAGON_50_CAL_LUT_SIZE   (sizeof(CsHexagon::s_CalCfg50)/sizeof(CsHexagon::s_CalCfg50[0]))

// Direct sampling rate are supported from 300-1000Hz. See PLL frequency set. Section 12.2
// SampleRate	ClkFreq	Decim	ClkDiv	Pinpong
const HEXAGON_SAMPLE_RATE_INFO	CsHexagon::Hexagon_SR[] = 
{
	1000000000,	1000,	1,		
	 875000000,	 875,	1,		
	 800000000,	 800,	1,		
	 750000000,	 750,	1,		
	 650000000,	 650,	1,		
	 600000000,	 600,	1,		
	 525000000,	 525,	1,		
	 500000000,	 500,	1,		
	 425000000,	 425,	1,		
	 400000000,	 400,	1,		
	 375000000,	 375,	1,		
	 325000000,	 325,	1,		
	 300000000,	 300,	1,		
	 250000000,	 500,	2,		
	 200000000,	 400,	2,		
	 100000000,	 400,	4,		
	  50000000,	 500,	10,		
	  20000000,	 400,	20,		
	  10000000,	 500,	50, 	
	   5000000,	 500,	100,	
	   2000000,	 500,	250,	
	   1000000,	 500,	500,	
	    500000,	 500,	1000,	
	    200000,	 500,	2500,	
	    100000,	 500,	5000,	
	     50000,	 500,	10000,	
	     20000,	 500,	25000,	
	     10000,	 500,	50000,	
	      5000,	 500,	100000,	
	      2000,	 500,	250000,	
	      1000,	 500,	500000,	
};

#define c_HEXAGON_SR_INFO_SIZE   (sizeof(CsHexagon::Hexagon_SR)/sizeof(HEXAGON_SAMPLE_RATE_INFO))


//-----------------------------------------------------------------------------
//
// u32ExtClkSampleSkip = 0	-> internal clock
// u32ExtClkSampleSkip != 0	-> external clock
//-----------------------------------------------------------------------------
int32	CsHexagon::SendClockSetting(LONGLONG llSampleRate, BOOLEAN bExtClk, BOOLEAN bRefClk)
{
	int32	i32Ret = CS_SUCCESS;
	bool	bForceDefault = (-1 == llSampleRate);

	if (bForceDefault)
	{
		if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & HEXAGON_ADDON_HW_OPTION_500MHZ) )
			llSampleRate = CS_SR_500MHZ;
		else
			llSampleRate = CS_SR_1GHZ;
	}

	DEC_FACTOR_REG				DecimationReg = {0};
	HEXAGON_CLOCK_REG			ClkReg = m_pCardState->ClockReg;
	HEXAGON_CLK_CTRL_REG		ClkCtrlReg = m_pCardState->ClkCtrlReg;
	HEXAGON_SAMPLE_RATE_INFO	SrInfo = {0};
	PHEXAGON_SAMPLE_RATE_INFO	pSrInfo = &SrInfo;
	uInt32						u32PllFrequencyValMHz = 0;
	CLOCK_MODE					ClockMode = SampleClk;

	ClkReg.u32RegVal		&= ~0x35;
	ClkCtrlReg.bits.ClkInDisbaled = (!bExtClk && !bRefClk);

	if ( bExtClk )
	{
		// External clock mode
		ClockMode						= ExternalClk;
		SrInfo.llSampleRate				= llSampleRate;
		SrInfo.u32Decimation			= 1;
		SrInfo.u32ClockFrequencyMhz		= (uInt32) llSampleRate/1000000;
	}
	else
	{
		// Internal clock or referencde clock mode
		if ( bRefClk )
			ClockMode = ReferenceClk;

		if ( !FindSrInfo( llSampleRate, &pSrInfo ) )
			return CS_INVALID_SAMPLE_RATE;

		DecimationReg.bits.DeciFactor	= pSrInfo->u32Decimation;
		u32PllFrequencyValMHz			= pSrInfo->u32ClockFrequencyMhz;
	}

	DecimationReg.bits.DeciFactor	= pSrInfo->u32Decimation;

	if ( ( ClockMode != m_ClockMode ) ||
		 ( !bExtClk && (m_pCardState->AddOnReg.u32ClkFreqMHz !=  u32PllFrequencyValMHz ) ) ||  
		 ( bExtClk && (m_pCardState->AcqConfig.i64SampleRate !=llSampleRate ) ) )
	{
		i32Ret = WriteRegisterThruNios(u32PllFrequencyValMHz, CSPLXBASE_PLL_FREQUENCY_SET_CMD);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	
		// From the function call above, the FW  may overwrite the setting of the clock out
		// Restore clockout
		i32Ret = SetClockControlRegister( ClkCtrlReg.u32RegVal, true );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
 
		// We need to perform some calibration when changing the clock
		m_pCardState->bExtTriggerAligned		= FALSE;

		m_pCardState->AddOnReg.u32ClkFreqMHz	= pSrInfo->u32ClockFrequencyMhz;
		m_pCardState->bAdcDcLevelFreezed		= FALSE;
		m_ClockMode								= ClockMode;
	}

	if ( m_pCardState->AddOnReg.u32Decimation != pSrInfo->u32Decimation )
	{
		i32Ret = WriteRegisterThruNios(DecimationReg.u32RegVal, HEXAGON_DECIMATION_SETUP);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		m_pCardState->AddOnReg.u32Decimation = pSrInfo->u32Decimation;
	}

	m_pCardState->AcqConfig.u32ExtClk		 = bExtClk ? 1 : 0; 
	m_pCardState->AcqConfig.i64SampleRate	 = pSrInfo->llSampleRate;

	i32Ret = SendADC_DC_Level_Freeze( 1 );			// which option to use ?

	return i32Ret;
}

//-----------------------------------------------------------------------------
// See Hexagon section 12.2
// Channel control:
//	00: 4 channels, 01/10: 2 channels (1 & 3): 11: 1 channel (channel 1)
//-----------------------------------------------------------------------------

int32	CsHexagon::SendCaptureModeSetting(uInt32 u32Mode)
{
	uInt16	u16ChanCtrl = 0;

	if ( u32Mode != m_pCardState->AcqConfig.u32Mode )
	{
		if ( -1 == u32Mode )
		{
			// Force to default mode
			u32Mode = ( 2 == GetBoardNode()->NbAnalogChannel ) ? CS_MODE_DUAL : CS_MODE_QUAD;
		}

		switch( u32Mode & CS_MASKED_MODE )
		{
			case CS_MODE_QUAD:
				u16ChanCtrl = 0;
				break;
			case CS_MODE_DUAL:
				u16ChanCtrl = 1;
				break;
			case CS_MODE_SINGLE:
				u16ChanCtrl = 3;
				break;
			default:
				return CS_INVALID_ACQ_MODE;
		}

		int32 i32Ret = WriteRegisterThruNios(u16ChanCtrl, HEXAGON_CHANNEL_CTRL);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		m_pCardState->AcqConfig.u32Mode = u32Mode;
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsHexagon::SendChannelSetting(uInt16 u16HwChannel, uInt32 u32InputRange, uInt32 u32Term, uInt32 u32Impendance, int32 i32Position, uInt32 u32Filter, int32 i32ForceCalib)
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32ValidCoupling = CS_COUPLING_DC;
	
	if( (u16HwChannel < 0) || u16HwChannel > GetBoardNode()->NbAnalogChannel  )
	{
		return CS_INVALID_CHANNEL;
	}
	if (CS_REAL_IMP_50_OHM != u32Impendance )
	{
		return CS_INVALID_IMPEDANCE;
	}

	if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & HEXAGON_ADDON_HW_OPTION_AC_COUPLING) )
		u32ValidCoupling = CS_COUPLING_AC;

	if ( u32ValidCoupling != (u32Term & u32ValidCoupling) )
		return CS_INVALID_COUPLING;

//	if ( HEXAGON_DEFAULT_INPUTRANGE == u32InputRange )
	{
		if ( m_bLowRange_mV || ( IsBoardLowRange()) )
			u32InputRange = HEXAGON_GAIN_LR_MV;
		else 
			u32InputRange = CS_GAIN_2_V;
	}

	uInt16 u16ChanZeroBased = u16HwChannel-1;

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

	if(	m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange != u32InputRange)
	{
		HEXAGON_CAL_LUT CalInfo = {0};
		i32Ret = FindCalLutItem(u32InputRange, 50, CalInfo);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		// still need to calibrate position
		//if ( u32InputRange==CS_GAIN_2_V)
			m_pCardState->bCalNeeded[u16ChanZeroBased]					= true;

		i32Ret = UpdateCalibDac(u16HwChannel, ecdGain, CalInfo.u16GainCtrl, true );
		BBtiming( 50 );

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
// Channel 1	associate with Trigger Engine 1 & 5
// Channel 2	associate with Trigger Engine 2 & 6
// Channel 3	associate with Trigger Engine 3 & 7
// Channel 4	associate with Trigger Engine 4 & 8
//-----------------------------------------------------------------------------
int32 CsHexagon::SendTriggerEngineSetting(  uInt16	u16EngineTblIdx,
											int32	i32SourceA1,
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
	DIGI_TRIG_CTRL_REG		*pTrigCtrlStore = &m_pCardState->BaseBoardRegs.TrigCtrl_Reg;
	int32	i32Ret = CS_SUCCESS;
	uInt16	nbChannels = m_PlxData->CsBoard.NbAnalogChannel;
	uInt16  u16TriggerEngine = CSRB_TRIGENGINE_A1;

#ifdef DBG_TRIGGER_SETTING
	sprintf_s( szDbgText, sizeof(szDbgText),  "=== SendTriggerEngineSetting EngineTblIdx %d SourceA1 %d SourceB1 %d SourceA2 %d SourceB2 %d ConditionA1 %d u32ConditionB1 %d \n",
				u16EngineTblIdx, i32SourceA1, i32SourceB1, i32SourceA2, i32SourceB2, u32ConditionA1, u32ConditionB1);
	::OutputDebugString( szDbgText );
#endif

	// These inputs should have been already validated from DLL level
	// Double check ..
	if ( u16EngineTblIdx > 1 )
		return CS_INVALID_PARAMETER;

	if ( ( i32SourceA1 < 0 ) || ( i32SourceA1 > 4 ) )
		return CS_INVALID_TRIG_SOURCE;

	if ( ( i32SourceB1 < 0 ) || ( i32SourceB1 > 4 ) )
		return CS_INVALID_TRIG_SOURCE;

	if ( ( i32SourceA2 < 0 ) || ( i32SourceA2 > 4 ) )
		return CS_INVALID_TRIG_SOURCE;

	if ( ( i32SourceB2 < 0 ) || ( i32SourceB2 > 4 ) )
		return CS_INVALID_TRIG_SOURCE;

	if ( ( i32LevelA1 > 100  || i32LevelA1 < -100 ) ||
		 ( i32LevelB1 > 100  || i32LevelB1 < -100 ) ||
		 ( i32LevelA2 > 100  || i32LevelA2 < -100 ) ||
		 ( i32LevelB2 > 100  || i32LevelB2 < -100 ))
		return CS_INVALID_TRIG_LEVEL;

	if (u16EngineTblIdx)
	{
		u16TriggerEngine = CSRB_TRIGENGINE_A3;
		pTrigCtrlStore = &m_pCardState->BaseBoardRegs.TrigCtrl_Reg_2;
	}

	if ( 0 < i32SourceA1  )
	{
		TrigCtrl_Reg.bits.TrigEng1_En_A = 1;
		
		if ( CS_TRIG_SOURCE_CHAN_1 == ConvertToHwChannel( (uInt16) i32SourceA1 ) )
		{
			TrigCtrl_Reg.bits.DataSelect1_A = 0;
		}

		if ( u32ConditionA1 == CS_TRIG_COND_POS_SLOPE )
			TrigCtrl_Reg.bits.TrigEng1_Slope_A = 1;
	}
	if ( 0 < i32SourceB1  )
	{
		TrigCtrl_Reg.bits.TrigEng1_En_B = 1;
		
		if ( CS_TRIG_SOURCE_CHAN_2 == ConvertToHwChannel( (uInt16) i32SourceB1 ) )
		{
			TrigCtrl_Reg.bits.DataSelect1_B = 0;
		}

		if ( u32ConditionB1 == CS_TRIG_COND_POS_SLOPE )
			TrigCtrl_Reg.bits.TrigEng1_Slope_B = 1;
	}
	if ( 0 < i32SourceA2  )
	{
		TrigCtrl_Reg.bits.TrigEng2_En_A = 1;
		
		if ( (CS_TRIG_SOURCE_CHAN_2 + 1)  == ConvertToHwChannel( (uInt16) i32SourceA2 ) )
		{
			TrigCtrl_Reg.bits.DataSelect2_A = 0;
		}

		if ( u32ConditionA2 == CS_TRIG_COND_POS_SLOPE )
			TrigCtrl_Reg.bits.TrigEng2_Slope_A = 1;
	}
	if ( 0 < i32SourceB2  )
	{
		TrigCtrl_Reg.bits.TrigEng2_En_B = 1;
		
		if ( (CS_TRIG_SOURCE_CHAN_2 + 2) == ConvertToHwChannel( (uInt16) i32SourceB2 ) )
		{
			TrigCtrl_Reg.bits.DataSelect2_B = 0;
		}

		if ( u32ConditionB2 == CS_TRIG_COND_POS_SLOPE )
			TrigCtrl_Reg.bits.TrigEng2_Slope_B = 1;
	}

	if ( pTrigCtrlStore->u16RegVal != TrigCtrl_Reg.u16RegVal )
	{
#ifdef DBG_TRIGGER_SETTING
		sprintf_s( szDbgText, sizeof(szDbgText),  "=== Setting [Addr=0x%lx Data=0x%x] \n",
				HEXAGON_DT_CONTROL + u16EngineTblIdx, TrigCtrl_Reg.u16RegVal);
		::OutputDebugString( szDbgText );
#endif
		i32Ret = WriteRegisterThruNios(TrigCtrl_Reg.u16RegVal, HEXAGON_DT_CONTROL + u16EngineTblIdx);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		pTrigCtrlStore->u16RegVal = TrigCtrl_Reg.u16RegVal;
	}

	i32Ret =  SendDigitalTriggerLevel( u16TriggerEngine, i32LevelA1 );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret =  SendDigitalTriggerLevel( u16TriggerEngine+1, i32LevelB1 );
	if( CS_FAILED(i32Ret) )
		return i32Ret;
	
	i32Ret =  SendDigitalTriggerLevel( u16TriggerEngine+2, i32LevelA2 );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret =  SendDigitalTriggerLevel( u16TriggerEngine+3, i32LevelB2 );
	if( CS_FAILED(i32Ret) )
		return i32Ret;


	for ( uInt16 i = u16TriggerEngine ; i < u16TriggerEngine + nbChannels; i ++ )
	{
		i32Ret =  SendDigitalTriggerSensitivity( i, m_u16TrigSensitivity );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	m_pCardState->TriggerParams[u16TriggerEngine].i32Source	= i32SourceA1;
	m_pCardState->TriggerParams[u16TriggerEngine].i32Level	= i32LevelA1;
	m_pCardState->TriggerParams[u16TriggerEngine].u32Condition = u32ConditionA1;

	m_pCardState->TriggerParams[u16TriggerEngine +1].i32Source	= i32SourceB1;
	m_pCardState->TriggerParams[u16TriggerEngine +1].i32Level	= i32LevelB1;
	m_pCardState->TriggerParams[u16TriggerEngine +1].u32Condition= u32ConditionB1;

	m_pCardState->TriggerParams[u16TriggerEngine +2].i32Source	= i32SourceA2;
	m_pCardState->TriggerParams[u16TriggerEngine +2].i32Level	= i32LevelA2;
	m_pCardState->TriggerParams[u16TriggerEngine +2].u32Condition = u32ConditionA2;

	m_pCardState->TriggerParams[u16TriggerEngine +3].i32Source	= i32SourceB2;
	m_pCardState->TriggerParams[u16TriggerEngine +3].i32Level	= i32LevelB2;
	m_pCardState->TriggerParams[u16TriggerEngine +3].u32Condition = u32ConditionB2;

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsHexagon::SendExtTriggerSetting( BOOLEAN  bExtTrigEnabled,
										   int32	i32ExtLevel,
										   uInt32	u32ExtCondition,
										   uInt32	u32ExtTriggerRange, 
										   uInt32	u32ExtTriggerImped, 
										   uInt32	u32ExtCoupling,
										   int32	i32Sensitivity )
{
	EXTRTRIG_REG	ExttTrigReg = {0};
	int32			i32Ret = CS_SUCCESS;

#ifdef DBG_TRIGGER_SETTING
	sprintf_s( szDbgText, sizeof(szDbgText),  "SendExtTriggerSetting: TrigEnabled %d Level %d  Condition %d Range %d Imped %d Coupling %d Sensitivity %d\n",
				bExtTrigEnabled,  i32ExtLevel, u32ExtCondition, u32ExtTriggerRange, u32ExtTriggerImped, u32ExtCoupling, i32Sensitivity );
		::OutputDebugString( szDbgText );
#endif

	UNREFERENCED_PARAMETER(u32ExtTriggerRange);
	UNREFERENCED_PARAMETER(u32ExtTriggerImped);
	UNREFERENCED_PARAMETER(u32ExtCoupling);
	UNREFERENCED_PARAMETER(i32Sensitivity);

	if ( bExtTrigEnabled )
		ExttTrigReg.bits.ExtTrigEnable = 1;

	if( CS_TRIG_COND_POS_SLOPE == u32ExtCondition )
		ExttTrigReg.bits.ExtTrigSlope = 0;					// Hexagon has Ext trigger Slope setting in reverse
	else
		ExttTrigReg.bits.ExtTrigSlope = 1;					// 0=raising, 1=falling
	
/*
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
*/
		

	i32Ret = WriteRegisterThruNios(ExttTrigReg.u16RegVal, HEXAGON_EXT_TRIGGER_SETTING);
	if( CS_FAILED(i32Ret) )	
		return i32Ret;

	if ( m_pCardState->AddOnReg.u16ExtTrigEnable != ExttTrigReg.u16RegVal )
		m_pCardState->AddOnReg.u16ExtTrigEnable = ExttTrigReg.u16RegVal;
	
	// Enabled All trigger engines
	i32Ret = WriteRegisterThruNios(CSRB_GLB_TRIG_ENG_ENABLE, HEXAGON_TRIGENGINES_EN);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret = SendExtTriggerLevel( i32ExtLevel );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].i32Source = bExtTrigEnabled ? -1 : 0;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32Condition = u32ExtCondition;
	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].i32Level = i32ExtLevel;
//	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange = u32ExtTriggerRange;
//	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtCoupling = u32ExtCoupling;
//	m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance = u32ExtTriggerImped;

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsHexagon::SendExtTriggerLevel( int32 i32Level )
{
	int32	i32Ret = CS_SUCCESS;

	//----- Convert from (-100, 99) range to ADC range
	int32	i32LevelConverted = 0;
	uInt32	u32DacCode = 4096;
	//const	uInt32	u32LevelZeroCode = 0;     //0x9B00;

	// Assume we are at +/- 3.3V range
	// converted from % to mV
	i32LevelConverted = 3300/100*i32Level;

	// The valid trigger range is from 0 to + 3.3 V
	if ( ( i32LevelConverted < 0 ) || ( i32LevelConverted > 3300 ) )
		return CS_INVALID_TRIG_LEVEL;

	// DAC Code
	// 0x0000   ---> 0.002 V = 0 V (simplified)
	// 0xCE00   ---> 3.296 V = 3.3 V (simplified)
	u32DacCode = 0xCE00*i32Level/100;

#ifdef DBG_TRIGGER_SETTING
	sprintf_s( szDbgText, sizeof(szDbgText),  "SendExtTriggerLevel: [0x%x] Level %d LevelConverted %d DacCode 0x%x \n",
				HEXAGON_EXTTRIG_DAC | HEXAGON_SPI_WRITE, i32Level, i32LevelConverted, u32DacCode );
		::OutputDebugString( szDbgText );
#endif
	m_pCardState->AddOnReg.u16ExtTrigLevel = (uInt16) u32DacCode;
	i32Ret = WriteRegisterThruNios( u32DacCode, HEXAGON_EXTTRIG_DAC | HEXAGON_SPI_WRITE);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsHexagon::SendDigitalTriggerLevel( uInt16 u16TrigEngine, int32 i32Level )
{
	int32	i32LevelConverted = i32Level;
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32Data = 0;
	uInt32	u32Cmd = 0;

	if( 100 < i32Level )
		i32Level = 100;
	else if( -100 > i32Level )
		i32Level = -100;

	//----- Convert from (-100, 100) range to ADC range
	int32  i32ADCRange =  HEXAGON_DIG_TRIGLEVEL_SPAN;
	i32LevelConverted = i32Level + 100;
	i32LevelConverted = i32LevelConverted * i32ADCRange;
	i32LevelConverted /= 200L;

	int16 i16LevelDAC = (int16)i32LevelConverted;

	switch ( u16TrigEngine )
	{
		case CSRB_TRIGENGINE_A1:
			u32Data = (i16LevelDAC & 0xFFFF) | ( (m_pCardState->BaseBoardRegs.i16TrigLevelA3 & 0xFFFF) << 16);
			u32Cmd = HEXAGON_DLA1;
			m_pCardState->BaseBoardRegs.i16TrigLevelA1 = i16LevelDAC;
			break;
		case CSRB_TRIGENGINE_B1:
			u32Data = (i16LevelDAC & 0xFFFF) | ( (m_pCardState->BaseBoardRegs.i16TrigLevelB3 & 0xFFFF) << 16);
			u32Cmd = HEXAGON_DLB1;
			m_pCardState->BaseBoardRegs.i16TrigLevelB1 = i16LevelDAC;
			break;
		case CSRB_TRIGENGINE_A2:
			u32Data = (i16LevelDAC & 0xFFFF) | ( (m_pCardState->BaseBoardRegs.i16TrigLevelA4 & 0xFFFF) << 16);
			u32Cmd = HEXAGON_DLA2;
			m_pCardState->BaseBoardRegs.i16TrigLevelA2 = i16LevelDAC;
			break;
		case CSRB_TRIGENGINE_B2:
			u32Data = (i16LevelDAC & 0xFFFF) | ( (m_pCardState->BaseBoardRegs.i16TrigLevelB4 & 0xFFFF) << 16);
			u32Cmd = HEXAGON_DLB2;
			m_pCardState->BaseBoardRegs.i16TrigLevelB2 = i16LevelDAC;
			break;
		case CSRB_TRIGENGINE_A3:
			u32Data = ( (i16LevelDAC & 0xFFFF) << 16)| (m_pCardState->BaseBoardRegs.i16TrigLevelA1 & 0xFFFF);
			u32Cmd = HEXAGON_DLA1;
			m_pCardState->BaseBoardRegs.i16TrigLevelA3 = i16LevelDAC;
			break;
		case CSRB_TRIGENGINE_B3:
			u32Data = ( (i16LevelDAC & 0xFFFF) << 16)| (m_pCardState->BaseBoardRegs.i16TrigLevelB1 & 0xFFFF);
			u32Cmd = HEXAGON_DLB1;
			m_pCardState->BaseBoardRegs.i16TrigLevelB3 = i16LevelDAC;
			break;
		case CSRB_TRIGENGINE_A4:
			u32Data = ( (i16LevelDAC & 0xFFFF) << 16)| (m_pCardState->BaseBoardRegs.i16TrigLevelA2 & 0xFFFF);
			u32Cmd = HEXAGON_DLA2;
			m_pCardState->BaseBoardRegs.i16TrigLevelA4 = i16LevelDAC;
			break;
		case CSRB_TRIGENGINE_B4:
			u32Data = ( (i16LevelDAC & 0xFFFF) << 16)| (m_pCardState->BaseBoardRegs.i16TrigLevelB2 & 0xFFFF);
			u32Cmd = HEXAGON_DLB2;
			m_pCardState->BaseBoardRegs.i16TrigLevelB4 = i16LevelDAC;
			break;
	}

	if (u32Cmd)
	{
#ifdef DBG_TRIGGER_SETTING
		i32Ret = WriteRegisterThruNios(u32Data, u32Cmd);
		sprintf_s( szDbgText, sizeof(szDbgText),  "=== SendDigitalTriggerLevel: TrigEngine %d Data 0x%lx Cmd 0x%lx \n",
					u16TrigEngine,
					u32Data,
					u32Cmd);
		::OutputDebugString( szDbgText );
#endif
		i32Ret = WriteRegisterThruNios(u32Data, u32Cmd);
	}
	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsHexagon::SendDigitalTriggerSensitivity( uInt16  u16TrigEngine, uInt16 u16Sensitive )
{
	int32	i32Ret = CS_SUCCESS;		
	uInt16 u16Data = u16Sensitive;
	uInt32 u32Data = 0;
	uInt32 u32Cmd = 0;

	switch ( u16TrigEngine )
	{
	case CSRB_TRIGENGINE_A1:
		u32Data = u16Sensitive | m_pCardState->BaseBoardRegs.u16TrigSensitiveA3 << 16;
		u32Cmd = HEXAGON_DSA1;
		m_pCardState->BaseBoardRegs.u16TrigSensitiveA1 = u16Data;
		break;
	case CSRB_TRIGENGINE_B1:
		u32Data = u16Sensitive | m_pCardState->BaseBoardRegs.u16TrigSensitiveB3 << 16;
		u32Cmd = HEXAGON_DSB1;
		m_pCardState->BaseBoardRegs.u16TrigSensitiveB1 = u16Data;
		break;
	case CSRB_TRIGENGINE_A2:
		u32Data = u16Sensitive | m_pCardState->BaseBoardRegs.u16TrigSensitiveA4 << 16;
		u32Cmd = HEXAGON_DSA2;
		m_pCardState->BaseBoardRegs.u16TrigSensitiveA2 = u16Data;
		break;
	case CSRB_TRIGENGINE_B2:
		u32Data = u16Sensitive | (m_pCardState->BaseBoardRegs.u16TrigSensitiveB4 << 16);
		u32Cmd = HEXAGON_DSB2;
		m_pCardState->BaseBoardRegs.u16TrigSensitiveB2 = u16Data;
		break;

	case CSRB_TRIGENGINE_A3:
		u32Data = (u16Sensitive << 16) | m_pCardState->BaseBoardRegs.u16TrigSensitiveA1 ;
		u32Cmd = HEXAGON_DSA1;
		m_pCardState->BaseBoardRegs.u16TrigSensitiveA3 = u16Data;
		break;
	case CSRB_TRIGENGINE_B3:
		u32Data = (u16Sensitive << 16) | m_pCardState->BaseBoardRegs.u16TrigSensitiveB1 ;
		u32Cmd = HEXAGON_DSB1;
		m_pCardState->BaseBoardRegs.u16TrigSensitiveB3 = u16Data;
		break;
	case CSRB_TRIGENGINE_A4:
		u32Data = (u16Sensitive << 16) | m_pCardState->BaseBoardRegs.u16TrigSensitiveA2 ;
		u32Cmd = HEXAGON_DSA2;
		m_pCardState->BaseBoardRegs.u16TrigSensitiveA4 = u16Data;
		break;
	case CSRB_TRIGENGINE_B4:
		u32Data = (u16Sensitive << 16) | m_pCardState->BaseBoardRegs.u16TrigSensitiveB2 ;
		u32Cmd = HEXAGON_DSB2;
		m_pCardState->BaseBoardRegs.u16TrigSensitiveB4 = u16Data;
		break;
	default:
		break;
	}

	if (u32Cmd)
	{
#ifdef DBG_TRIGGER_SETTING
		sprintf_s( szDbgText, sizeof(szDbgText),  "=== SendDigitalTriggerSensitivity: TrigEngine %d u16Sensitive 0x%x Data 0x%lx Cmd 0x%lx \n",
					u16TrigEngine,
					u16Sensitive,
					u32Data,
					u32Cmd);
		::OutputDebugString( szDbgText );
#endif
		i32Ret = WriteRegisterThruNios(u32Data, u32Cmd);
	}

	return i32Ret;
}

//-----------------------------------------------------------------------------
//----- WriteToSelfTestRegisterSendExt
//----- ADD-ON REGISTER
//-----------------------------------------------------------------------------
int32 CsHexagon::WriteToSelfTestRegister( uInt16 u16SelfTestMode )
{

	uInt16	u16Data = 0;

	switch( u16SelfTestMode )
	{
	case CSST_CAPTURE_DATA:
		u16Data = HEXAGON_CAPTURE_DATA;
		break;
	case CSST_COUNTER_DATA:
		u16Data = HEXAGON_SELF_COUNTER;
		break;
	default:
		return CS_INVALID_SELF_TEST;
	}
	
	int32 i32Ret = WriteRegisterThruNios(u16Data, HEXAGON_DATA_SELECT);
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
void	CsHexagon::InitBoardCaps()
{
	GAGE_CT_ASSERT( HEXAGON_MAX_SR_COUNT >= c_HEXAGON_SR_INFO_SIZE )

	if ( 0 == m_pCardState->u32SrInfoSize )
	{
		if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & HEXAGON_ADDON_HW_OPTION_500MHZ) )
		{
			const HEXAGON_SAMPLE_RATE_INFO *pSr = &Hexagon_SR[0];
			for (int i = 0, j = 0 ; i < sizeof(Hexagon_SR) / sizeof(HEXAGON_SAMPLE_RATE_INFO); i++)
			{
				// Remove samples rates > 500 MHz
				if ( pSr[i].llSampleRate > 500000000 )
					continue;
				else
				{
					RtlCopyMemory( &m_pCardState->SrInfoTable[j++], &pSr[i], sizeof(HEXAGON_SAMPLE_RATE_INFO) );
					m_pCardState->u32SrInfoSize ++;
				}
			}
		}
		else
		{
			// Copy the whole sample rate table
			RtlCopyMemory( m_pCardState->SrInfoTable, Hexagon_SR, sizeof(Hexagon_SR) );
			m_pCardState->u32SrInfoSize = sizeof(Hexagon_SR) / sizeof(HEXAGON_SAMPLE_RATE_INFO);
		}
	}

	// Gain table
	if ( 0 == m_pCardState->u32FeInfoSize )
	{
		// m_bLowRange_mV
		if (IsBoardLowRange())
		{
			RtlCopyMemory( m_pCardState->FeInfoTable, c_FrontEndGainLowRange50, sizeof(c_FrontEndGainLowRange50) );
			m_pCardState->u32FeInfoSize = (sizeof(c_FrontEndGainLowRange50)/sizeof(FRONTEND_HEXAGON_INFO));
			//m_EventLog.Report( CS_INFO_TEXT, "InitBoardCaps : m_bLowRange_mV" );

		}
		else
		{
			RtlCopyMemory(m_pCardState->FeInfoTable, c_FrontEndGain50, sizeof(c_FrontEndGain50));
			m_pCardState->u32FeInfoSize = (sizeof(c_FrontEndGain50)/sizeof(FRONTEND_HEXAGON_INFO));
			//m_EventLog.Report( CS_INFO_TEXT, "InitBoardCaps : HighRange" );
		}

	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL	CsHexagon::IsNiosInit(void)
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

		m_EventLog.Report( CS_ERROR_TEXT, "NiosInit error: STATUS_REG_PLX error" );
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

int32	CsHexagon::CsReadPlxNvRAM( PPLX_NVRAM_IMAGE nvRAM_image )
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

int32	CsHexagon::CsWritePlxNvRam( PPLX_NVRAM_IMAGE nvRAM_image )
{
	// Simulate Nvram for backward compatible
	HEXAGON_MISC_DATA		BoardCfg;

	uInt32 u32Offset	= FIELD_OFFSET(HEXAGON_FLASH_LAYOUT, BoardData);
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
int32 CsHexagon::IoctlGetPciExLnkInfo(PPCIeLINK_INFO pPciExInfo)
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
int32	CsHexagon::ReadHardwareInfoFromFlash()
{
	// Read BaseBoard board Hardware info
	uInt32		u32Offset;
	int32		i32Status = CS_FLASH_NOT_INIT;

	HEXAGON_MISC_DATA		BoardCfg;
	u32Offset = FIELD_OFFSET(HEXAGON_FLASH_LAYOUT, BoardData);

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
	}

	CS_FIRMWARE_INFOS		FwInfo;

	// Read the SafeBoot firmware info
	u32Offset = FIELD_OFFSET(HEXAGON_FLASH_LAYOUT, SafeBoot) + FIELD_OFFSET(HEXAGON_FLS_IMAGE_STRUCT, FwInfo)
		 + FIELD_OFFSET(HEXAGON_FWINFO_SECTOR, FwInfoStruct);
	i32Status = IoctlReadFlash( u32Offset, &FwInfo, sizeof(FwInfo) );
	if ( CS_SUCCEEDED(i32Status) && FwInfo.u32ChecksumSignature == CSI_FWCHECKSUM_VALID )
	{
		m_PlxData->CsBoard.u32UserFwVersionB[0].u32Reg	= CONVERT_EEPROM_OPTIONS(FwInfo.u32FirmwareVersion);
		m_PlxData->CsBoard.u32BaseBoardOptions[0]		= CONVERT_EEPROM_OPTIONS(FwInfo.u32FirmwareOptions);
	}

	// Read the default firmware info
	u32Offset = FIELD_OFFSET(HEXAGON_FLASH_LAYOUT, FlashImage0) + FIELD_OFFSET(HEXAGON_FLS_IMAGE_STRUCT, FwInfo)
		 + FIELD_OFFSET(HEXAGON_FWINFO_SECTOR, FwInfoStruct);
	i32Status = IoctlReadFlash( u32Offset, &FwInfo, sizeof(FwInfo) );
	if ( CS_SUCCEEDED(i32Status) && FwInfo.u32ChecksumSignature == CSI_FWCHECKSUM_VALID )
	{
		m_PlxData->CsBoard.u32UserFwVersionB[REGULAR_INDEX_OFFSET].u32Reg	= CONVERT_EEPROM_OPTIONS(FwInfo.u32FirmwareVersion);
		m_PlxData->CsBoard.u32BaseBoardOptions[REGULAR_INDEX_OFFSET]		= CONVERT_EEPROM_OPTIONS(FwInfo.u32FirmwareOptions);
	}

	if (m_PlxData->CsBoard.i64ExpertPermissions)
	{
	// Read Expert firmware info
	u32Offset = FIELD_OFFSET(HEXAGON_FLASH_LAYOUT, FlashImage1) + FIELD_OFFSET(HEXAGON_FLS_IMAGE_STRUCT, FwInfo)
		 + FIELD_OFFSET(HEXAGON_FWINFO_SECTOR, FwInfoStruct);
	i32Status = IoctlReadFlash( u32Offset, &FwInfo, sizeof(FwInfo) );
	if ( CS_SUCCEEDED(i32Status) && FwInfo.u32ChecksumSignature == CSI_FWCHECKSUM_VALID )
	{
		m_PlxData->CsBoard.u32UserFwVersionB[EXPERT_INDEX_OFFSET].u32Reg	= CONVERT_EEPROM_OPTIONS(FwInfo.u32FirmwareVersion);
		m_PlxData->CsBoard.u32BaseBoardOptions[EXPERT_INDEX_OFFSET]			= CONVERT_EEPROM_OPTIONS(FwInfo.u32FirmwareOptions);
	}
	}
	else
	{
		m_PlxData->CsBoard.u32UserFwVersionB[EXPERT_INDEX_OFFSET].u32Reg	= 0;
		m_PlxData->CsBoard.u32BaseBoardOptions[EXPERT_INDEX_OFFSET]			= 0;
	}

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsHexagon::ReadIoConfigFromFlash()
{
	// Read BaseBoard board Hardware info
	uInt32		u32Offset;
	int32		i32Status = CS_FLASH_NOT_INIT;

	HEXAGON_MISC_DATA		BoardCfg;
	u32Offset = FIELD_OFFSET(HEXAGON_FLASH_LAYOUT, BoardData);

	i32Status = IoctlReadFlash(u32Offset, &BoardCfg, sizeof(BoardCfg));
	if (CS_SUCCEEDED(i32Status))
	{
		// Trig out and Clock out Config
		m_pCardState->bDisableTrigOut = (1 == BoardCfg.u16TrigOutEnable) ? 0 : 1;
		// Get only valid values
		if ((BoardCfg.ClockOutCfg == CS_OUT_NONE) || (BoardCfg.ClockOutCfg == CS_CLKOUT_SAMPLE) || (BoardCfg.ClockOutCfg == CS_CLKOUT_REF))
			m_pCardState->eclkoClockOut = (eClkOutMode)BoardCfg.ClockOutCfg;
		else
			m_pCardState->eclkoClockOut = HEXAGON_DEFAULT_CLKOUT;
	}
	else
	{
		m_pCardState->bDisableTrigOut = TRUE;
		m_pCardState->eclkoClockOut = HEXAGON_DEFAULT_CLKOUT;
	}

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsHexagon:: ReadTriggerTimeStampEtb( uInt32 StartSegment, int64 *pBuffer, uInt32 u32NumOfSeg )
{
	// Ts frequency in Single channel mode
	//uInt32	TsFrequency = ( m_pCardState->AcqConfig.u32TimeStampConfig & TIMESTAMP_MCLK ) ? 50000000: 31250000;

	// Set Mode select 0
	WriteGageRegister(MODESEL, 0);

	for (uInt32 i = 0; i < u32NumOfSeg; i ++)
	{
		// Hight part of TimeStamp
		pBuffer[i] = ReadRegisterFromNios( StartSegment, CSPLXBASE_READ_TIMESTAMP0_CMD );
		pBuffer[i] &= 0xFFFF;
		pBuffer[i] <<= 32;

		// Lowt part of TimeStamp
		uInt32	u32LowTs = ReadRegisterFromNios( StartSegment++, CSPLXBASE_READ_TIMESTAMP1_CMD );
		pBuffer[i] |= u32LowTs;

		uInt32 u32Etb = 0;
		if ( CS_MODE_QUAD == (m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE))
		{
			u32Etb = u32LowTs & 0x07;	// 3 bit Etb
		}
		else if ( CS_MODE_DUAL == (m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE))
		{
			u32Etb = u32LowTs & 0x0F;	// 4 bit Etb
		}
		else
		{
			u32Etb = u32LowTs & 0x1F;	// 5 bit Etb
		}
/*
		// Removed Etbs from TimeStamp 
		pBuffer[i] >>= 5;

		// Converting Ts unit from TsFrequency to the current Sample Rate
		double Factor = 1.0 * m_pCardState->AcqConfig.i64SampleRate / TsFrequency;
		Factor /= m_pCardState->AddOnReg.u32Decimation ;
		Factor /= (m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE);
		pBuffer[i] = (int64) (Factor * pBuffer[i]);
		pBuffer[i] += u32Etb;
*/
	}
		
	
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int64	CsHexagon::GetTimeStampFrequency()
{
	int64	i64TimeStampFreq = 0;

	if ( m_pCardState->AcqConfig.u32TimeStampConfig & TIMESTAMP_MCLK )
	{
		// Get rid of 5 bit Etbs: Ts = Ts *32
		i64TimeStampFreq = HEXAGON_TS_MEMORY_CLOCK;
		i64TimeStampFreq *= 32;
	}
	else
	{
		// Calculation:
		// Ts_Frequency_Quad =  1000000*m_pCardState->AddOnReg.u32ClkFreqMHz/8;
		// Get rid of 5 bit Etbs: Ts = Ts *32
		// Ts_Frequency_Quad = Ts_Frequency_Quad*32;
		// Ts_Frequency_Quad = 4 * 1000000*m_pCardState->AddOnReg.u32ClkFreqMHz;
		// Ts_Frequency_Dual  = Ts_Frequency_Quad / 2;
		// Ts_Frequency_Single =  Ts_Frequency_Quad / 4;
		i64TimeStampFreq = 1000000*m_pCardState->AddOnReg.u32ClkFreqMHz/8;

		// Get rid of Etbs (x 32) and in Single chan mode (/4)
		i64TimeStampFreq *= 8;

		i64TimeStampFreq *= (m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE);
		i64TimeStampFreq /= m_pCardState->AddOnReg.u32Decimation;
	}
		
	return i64TimeStampFreq;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsHexagon::ReadBusyStatus()
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
uInt32	CsHexagon::ConvertTriggerTimeout ( int64 i64Timeout_in_100ns )
{
	uInt32	u32HwTimeout = INFINITE_U32;

	// Timeout frequency is the same as Timesamp frequency
	// Divide by 32 here because we modified (x32) the Timestamp frequency to get rid ob 5 bits Etb
	int64	i64ClockRate = GetTimeStampFrequency()/32;
	
	if ( -1 != i64Timeout_in_100ns )
	{
		double dFactor = 1.0 * 1000000000 / i64ClockRate; 
		double dHwTimeout = 1.0 * i64Timeout_in_100ns * 100 / dFactor; 

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

void	CsHexagon::SetTriggerTimeout (uInt32 u32TimeOut)
{
	if ( m_pCardState->BaseBoardRegs.u32TrigTimeOut != u32TimeOut )
	{
		WriteRegisterThruNios( u32TimeOut, HEXAGON_BB_TRIGTIMEOUT_REG );
		m_pCardState->BaseBoardRegs.u32TrigTimeOut = u32TimeOut;
	}
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsHexagon::FindCalLutItem( const uInt32 u32Swing, const uInt32 u32Imped, HEXAGON_CAL_LUT& Item )
{
	PHEXAGON_CAL_LUT pLut;
	int nCount(0);

	if (CS_REAL_IMP_50_OHM == u32Imped)
	{
		pLut = s_CalCfg50;
		nCount = c_HEXAGON_50_CAL_LUT_SIZE;
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
int32 CsHexagon::SetPosition(uInt16 u16Channel, int32 i32Position_mV)
{
	int32 i32Ret = CS_SUCCESS;
	uInt16 u16ChanZeroBased = u16Channel - 1;

	if( m_pCardState->bCalNeeded[u16ChanZeroBased] )
	{
		//store position it will be set after calibration
		m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = i32Position_mV;
	}
	else
	{
		i32Ret = SendPosition(u16Channel, i32Position_mV);
	}
	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsHexagon::SendPosition(uInt16 u16Channel, int32 i32Position_mV)
{
	uInt16 u16ChanZeroBased = u16Channel-1;


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
		
	if( 0 != u16CodeDeltaForHalfIr  )
	{
		int32 i32PosCode = -(2 * i32Position_mV * u16CodeDeltaForHalfIr)/i32FrontUnipolar_mV;
		i32PosCode += m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset;
		uInt32 u32Code = i32PosCode > 0 ? ( i32PosCode < int32(c_u32CalDacCount-1) ? uInt32(i32PosCode) : c_u32CalDacCount-1 ) : 0;
		
		m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = i32Position_mV;

		//sprintf_s( szDbgText, sizeof(szDbgText),  "=== SendPosition() Channel %d Pos_mV/Unipolar_mV (%d / %d) PosCode B %d Code (0x%03x) DeltaForHalfIr %d CoarseOffset (0x%03x)  \n",
		//			u16Channel, i32Position_mV, i32FrontUnipolar_mV, i32PosCode, (uInt16)u32Code, u16CodeDeltaForHalfIr,
		//			m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset);
		//::OutputDebugString( szDbgText );

		//sprintf_s( szDbgText, sizeof(szDbgText),  "=== update position via Nios u32code 0x%x u32Command 0x%x\n",
		//			u32Code,
		//			u32Command);
		//::OutputDebugString( szDbgText );

		i32Ret = UpdateCalibDac(u16Channel, ecdPosition, (uInt16)u32Code, false );
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

int32	CsHexagon::GetConfigBootLocation()
{
	int32	i32BootLocation = 0;
	uInt32	u32Val = ReadConfigBootLocation();

	switch (u32Val<<2)
	{
		case 0x2180000:			// Default Image
			i32BootLocation = 1;
			break;
		case 0x4300000:			// eXpert Image
			i32BootLocation = 2;
			break;
	}

	return i32BootLocation;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsHexagon::SetConfigBootLocation( int32 i32BootLocation )
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
int32	CsHexagon::WriteConfigBootLocation( uInt32	u32Location )
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

	m_EventLog.Report( CS_INFO_TEXT, szText );

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32	CsHexagon::ReadConfigBootLocation()
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
int32	CsHexagon::FlashIdentifyLED(bool bOn)
{
	uInt32 u32RegVal = 0;
	
	if ( bOn )
		u32RegVal = 0x300;

	return WriteRegisterThruNios( u32RegVal, HEXAGON_USER_LED_CRTL );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsHexagon::SetTriggerIoRegister( uInt32 u32Val, bool bForce )
{
	int32 i32Ret = CS_SUCCESS;
	
	if ( bForce || u32Val != m_pCardState->TrigIoReg.u32RegVal )
	{
		i32Ret = WriteRegisterThruNios(u32Val, HEXAGON_TRIGGER_IO_CONTROL | HEXAGON_SPI_WRITE );
		if( CS_SUCCEEDED(i32Ret) )
			 m_pCardState->TrigIoReg.u32RegVal = u32Val;
	}

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsHexagon::SetTrigOut( uInt16 u16Value )
{
	m_pCardState->bDisableTrigOut = ( CS_TRIGOUT != u16Value );
	int32 i32Ret = WriteRegisterThruNios(m_pCardState->bDisableTrigOut, HEXAGON_TRIGOUT_DIS );
	return i32Ret;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsHexagon::SetClockControlRegister( uInt32 u32Val, bool bForce )
{
	int32 i32Ret = CS_SUCCESS;
	
	// The version of firmware earlier than 45 does not support clock out control
	uInt32 u32Version = ReadGageRegister( HEXAGON_HWR_RD_CMD );	
	if ( u32Version < 45 )
		return CS_SUCCESS;

	if ( bForce || u32Val != m_pCardState->ClkCtrlReg.u32RegVal )
	{
		i32Ret = WriteRegisterThruNios(u32Val, HEXAGON_CLOCKOUT_CTRL );
		if( CS_SUCCEEDED(i32Ret) )
			 m_pCardState->ClkCtrlReg.u32RegVal = u32Val;
	}

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsHexagon::SetClockOut( eClkOutMode ClkOutValue, bool bForce )
{
	HEXAGON_CLK_CTRL_REG	ClkCtrlReg = m_pCardState->ClkCtrlReg;

	if ( CS_CLKOUT_REF == ClkOutValue )
	{
		ClkCtrlReg.bits.ClkOutCtrl = 2;
	}
	else if ( CS_CLKOUT_SAMPLE == ClkOutValue )
	{
		ClkCtrlReg.bits.ClkOutCtrl = 1;
	}
	else
		ClkCtrlReg.bits.ClkOutCtrl = 0;

	int32	i32Ret = SetClockControlRegister( ClkCtrlReg.u32RegVal, bForce );
	if ( CS_SUCCEEDED(i32Ret) )
		m_pCardState->eclkoClockOut = ClkOutValue;

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsHexagon::SendExtTriggerAlign()
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	i = 0;
	PARRAY_CHANNELCONFIG	pAChannel = 0;
	PARRAY_TRIGGERCONFIG	pATrigger = 0;
	uInt32	u32NiosReturn = 0;
	char	szText[128];
	
	if ( m_bNeverCalibExtTrig || m_pCardState->bExtTriggerAligned )
		return i32Ret;

	if ( CS_SUCCEEDED(i32Ret) )
	{
		// Backup the old Front End and triggers settings
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

		// Align Ext trigger
		// the Command HEXAGON_EXT_TRIG_ALIGN will put he card in Calib mode and will generate an interrupt OVER_VOLTAGE
		// protection. In order to prevent the interrupt, we have to disable the interrrupt.
		// Disable all interrupts
		WriteGageRegister( INT_CFG, 0 );
		// The Command HEXAGON_EXT_TRIG_ALIGN also captures data from the channel 1 for calibration.
		// Remove any user DC offset
		for ( int chan=1; chan<=GetBoardNode()->NbAnalogChannel;chan++ )
		{
			if (m_pCardState->ChannelParams[chan-1].i32Position)
				SendPosition(chan, 0);
		}

		//Should we use recommendation value setup (level, hysteresis...) for channel 1 before doing the setting.
//		for(int i= 0; i< 2; i++)
		{
			i32Ret = WriteRegisterThruNios(0, HEXAGON_EXT_TRIG_ALIGN, -1, &u32NiosReturn );
			if ( CS_SUCCEEDED(i32Ret) )
			{
				if ( 0x3 == u32NiosReturn )
				{
					m_pCardState->bExtTriggerAligned = TRUE;
//					break;
				}
				else
				{
					sprintf_s( szText, sizeof(szText), TEXT("External Trigger Calib failed (0x%x)\n"), u32NiosReturn );
					OutputDebugString(szText);
				}
			}
		}

		if ( CS_SUCCEEDED(i32Ret) )
			m_pCardState->bExtTriggerAligned = TRUE;
		else
		{
			sprintf_s( szErrorStr, sizeof(szErrorStr), "%d, External Trigger Align Failed ", 
					CS_EXT_TRIG_DPA_FAILURE );
			m_EventLog.Report( CS_ERROR_EXTTRIG_CALIB_FAILED, szErrorStr );
#ifdef _DEBUG			
			sprintf_s( szDbgText, sizeof(szDbgText), "WriteRegisterThruNios(HEXAGON_EXT_TRIG_ALIGN) Failed ret = [ %d, 0x%x] ", 
					i32Ret, i32Ret );
			OutputDebugString(szDbgText);
#endif			

		}

		// While executing the command HEXAGON_EXT_TRIG_ALIGN, the firmware has changed some channel and trigger settings.
		// We need to reset the cache here so that we can restore the setting we use later
		m_pCardState->AddOnReg.u16ExtTrigEnable				= 
		m_pCardState->BaseBoardRegs.TrigCtrl_Reg.u16RegVal	= 
		m_pCardState->AddOnReg.u16ExtTrigEnable				= INFINITE_U16;

		// Restore the settings
		Sleep(100);
		CsSetChannelConfig( pAChannel );
		CsSetTriggerConfig( pATrigger );
//		m_pCardState->bCalNeeded[0] = m_pCardState->bCalNeeded[1] = 0;
	}

EndSendExtTriggerAlign:
	{
		uInt16 u16Value = m_pCardState->bDisableTrigOut ? CS_OUT_NONE : CS_TRIGOUT;

		// Restore Ext Trigger Calibration Mode
		i32Ret = SetTriggerIoRegister( HEXAGON_DEFAULT_TRIGIO_REG, true );
		if (CS_FAILED(i32Ret))
			return i32Ret;

		i32Ret = SetTrigOut(u16Value);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		// Restore the old setting
		SendCalibModeSetting();

		if ( pAChannel )
			GageFreeMemoryX( pAChannel );
		if ( pATrigger )
			GageFreeMemoryX( pATrigger );
	}

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsHexagon::SendADC_DC_Level_Freeze(uInt32 options, bool bForce /* = false */)
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32Command = HEXAGON_ADC_DC_LEVEL_FREEZE;
	uInt32	u32RetCode = 0;

//#ifdef _DEBUG
//	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("+++ SendADC_DC_Level_Freeze. bAdcDcLevelFreezed=%d m_bNoDCLevelFreeze %d\n"), 
//				m_pCardState->bAdcDcLevelFreezed, m_bNoDCLevelFreeze );
//	OutputDebugString(szDbgText);
//#endif

	if(m_bNoDCLevelFreeze)
		return CS_SUCCESS;

	if (  bForce || (m_pCardState->bAddOnRegistersInitialized && ! m_pCardState->bAdcDcLevelFreezed) )
	{

        // work around. Reset DC offset before apply ADCDCLevelFreeze
		for (uInt16 u16Channel = 1; u16Channel<= m_PlxData->CsBoard.NbAnalogChannel; u16Channel++)
		{
			uInt32 u32RangeIndex, u32FeModIndex;
			i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );

			// Initial Zero DC offset
			if (m_pCardState->ChannelParams[u16Channel-1].i32Position)
			{
#ifdef _DEBUG
				//sprintf_s( szDbgText, sizeof(szDbgText), TEXT("+++ Position %d  UpdateCalibDac ZeroDcOffset 0x%x\n"), 
				//			m_pCardState->ChannelParams[u16Channel-1].i32Position,
				//			m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16CoarseOffset );
				//OutputDebugString(szDbgText);
#endif
				i32Ret = UpdateCalibDac(u16Channel, ecdPosition, m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16CoarseOffset, false );
				if( CS_FAILED(i32Ret) )
					return i32Ret;
			}
		}

		// Retry 5 times
		for (int i = 0; i < 5; i++)
		{
			i32Ret = WriteRegisterThruNios( options, u32Command, -1, &u32RetCode );
			BBtiming( 50 );
			if ( CS_SUCCEEDED(i32Ret) )
			{
				if ( u32RetCode==0)
				{
					m_pCardState->bAdcDcLevelFreezed = true;
					break;
				}
			}
		}

		// Restore the normal mode
		SendCalibModeSetting();

		if 	(!m_pCardState->bAdcDcLevelFreezed)
			i32Ret = CS_DC_LEVEL_FREEZE_FAILED;
	}
	else
		return CS_SUCCESS;
	
	// Log the error
	if ( CS_DC_LEVEL_FREEZE_FAILED == i32Ret )
	{
		char	szText[128];
		sprintf_s( szText, sizeof(szText), TEXT("DC Level Freeze (Cmd 0x%x) failed. RetCode=0x%x\n"), (u32Command >> 20),u32RetCode );
		m_EventLog.Report( CS_ERROR_TEXT, szText );
#ifdef _DEBUG
		OutputDebugString(szText);
#endif
	}
	return i32Ret;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CsHexagon::isPLLLocked()
{
	uInt32	u32Command = HEXAGON_ADDON_CPLD_MISC_STATUS;
	uInt32	u32Val = ReadRegisterFromNios( 0, u32Command );
	
	if ( 0 == (u32Val & 0x2) )	
		return (FALSE);
	else
		return (TRUE);
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsHexagon::LockPLL()
{
	int32	i32Ret = CS_SUCCESS;
	uInt32	u32Command = HEXAGON_ADDON_CPLD_MISC_STATUS;
	uInt32	u32Val = ReadRegisterFromNios( 0, u32Command );

	// Bug from the firmware. the status of PLL is not stable at the first reading after
	// somd nios commands. Read the second time give a bttter status ???
	u32Val = ReadRegisterFromNios( 0, u32Command );
	if ( 0 == (u32Val & 0x2) )
	{
		i32Ret = ResetPLL();
		if ( CS_FAILED(i32Ret) )
			return i32Ret;
		
		// PLL should be locked by now
		u32Val = ReadRegisterFromNios( 0, u32Command );
	}

	return i32Ret;
}


//------------------------------------------------------------------------------
// Reset LMX2541 PLL to default value.
//------------------------------------------------------------------------------
int32 CsHexagon::ResetPLL()
{
	int32	i32Ret = CS_SUCCESS;

	// Reset the PLL
	i32Ret = WriteRegisterThruNios(0, HEXAGON_RESET_PLL);
	return i32Ret;
}

#define HEXAGON_RECONFIG_BIT			0x800000
#define HEXAGON_WAITCONFIGRESET			4000		// ms
#define NUCLEON_CONFIG_RESET			0x11
#define NUCLEON_REACONFIG_ADDR0			0x7
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

BOOLEAN CsHexagon::ConfigurationReset( uInt32 u32BootLocation )
{
	if ( u32BootLocation > 2 )
		return TRUE;

	uInt32	u32Addr = FIELD_OFFSET(HEXAGON_FLASH_LAYOUT, SafeBoot);

	u32Addr += u32BootLocation*FIELD_OFFSET(HEXAGON_FLASH_LAYOUT, FlashImage0);

	u32Addr >>= 2;		// DWord address
	WriteCpldRegister(11, (u32Addr & 0xFFFF));
	u32Addr >>= 16;
	WriteCpldRegister(12, (u32Addr & 0xFFFF));

	WriteGageRegister( CPLD_REG_OFFSET, HEXAGON_RECONFIG_BIT );
	WriteGageRegister( CPLD_REG_OFFSET, 0 );

	Sleep ( HEXAGON_WAITCONFIGRESET );
	
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
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsHexagon::CsCopyFFTWindowParams ( uInt32 u32WinSize, int16 *pi16Coef )
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

void	CsHexagon::CsConfigFFTWindow ()
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
		WriteGageRegister( HEXAGON_FFT_USER_INDEX, j);
		WriteGageRegister( HEXAGON_FFT_USER_DATA, u32Coefficient );
	}		
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsHexagon::CsStartFFT ()
{
	m_u32FftReg.bits.Enabled = 1;
	WriteGageRegister( HEXAGON_FFT_CONTROL, m_u32FftReg.u32RegVal );		
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsHexagon::CsStopFFT ()
{
	m_u32FftReg.bits.Enabled = 0;
	WriteGageRegister( HEXAGON_FFT_CONTROL, m_u32FftReg.u32RegVal );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsHexagon::CsWaitForFFT_DataReady ()
{
	uInt32	u32HostReadFifoEmpty = 0;
	uInt32	u32Count = 200;

	while ( u32Count > 0 )
	{
//		u32DataFifo = ReadRegisterFromNios( 0,  HEXAGON_RD_BB_REGISTER | 0x32 );
		u32HostReadFifoEmpty = ReadGageRegister( MISC_REG );
		if ( 0 == (u32HostReadFifoEmpty & 0x100) )
			break;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	CsHexagon::CsCopyFFTparams ( CS_FFT_CONFIG_PARAMS *pFFT )
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


int32	CsHexagon::WriteRegisterThruNiosEx(uInt32 Data, uInt32 Command, int32 i32Timeout, uInt32 *pu32NiosReturn )
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
//----- WriteToCalibDac (Coarse, Fine, Gain)
//----- ADD-ON REGISTER
//----- u16Data: 12bits data for Coarse & Fine offset. 6 bits for Gain (LMH6401)
//----- u8Delay: DefaultDelay is 32 us
//----- Delay = DefaultDelay << u8Delay;
//-----------------------------------------------------------------------------

int32 CsHexagon::WriteToCalibDac(uInt16 u16Addr, uInt16 u16Data, uInt8 u8Delay)
{
	uInt32 u32DacAddr = u16Addr;
	uInt32 u32Data = u16Data & 0xFFF;		// 12 bits data
	
	UNREFERENCED_PARAMETER(u8Delay);

	switch ( u16Addr )
	{
		case HEXAGON_POSITION_1: 
			u32DacAddr = HEXAGON_COARSE_OFFSET1_CS | HEXAGON_WRITE_TO_DAC128S085_CMD; 
			break;
		case HEXAGON_POSITION_2: 
			u32DacAddr = HEXAGON_COARSE_OFFSET2_CS | HEXAGON_WRITE_TO_DAC128S085_CMD; 
			break;
		case HEXAGON_POSITION_3: 
			u32DacAddr = HEXAGON_COARSE_OFFSET3_CS | HEXAGON_WRITE_TO_DAC128S085_CMD;  
			break;
		case HEXAGON_POSITION_4: 
			u32DacAddr = HEXAGON_COARSE_OFFSET4_CS | HEXAGON_WRITE_TO_DAC128S085_CMD;
			break;

		case HEXAGON_VCALFINE_1: 
			u32DacAddr = HEXAGON_FINE_OFFSET1_CS | HEXAGON_WRITE_TO_DAC128S085_CMD;
			break;
		case HEXAGON_VCALFINE_2: 
			u32DacAddr = HEXAGON_FINE_OFFSET2_CS | HEXAGON_WRITE_TO_DAC128S085_CMD;
			break;
		case HEXAGON_VCALFINE_3: 
			u32DacAddr = HEXAGON_FINE_OFFSET3_CS | HEXAGON_WRITE_TO_DAC128S085_CMD; 
			break;
		case HEXAGON_VCALFINE_4: 
			u32DacAddr = HEXAGON_FINE_OFFSET4_CS | HEXAGON_WRITE_TO_DAC128S085_CMD; 
			break;

		case HEXAGON_CAL_GAIN_1: 
			u32DacAddr = (HEXAGON_GAIN1_CS | HEXAGON_WRITE_TO_LMH6401_CMD);
			break;
		case HEXAGON_CAL_GAIN_2: 
			u32DacAddr = (HEXAGON_GAIN2_CS | HEXAGON_WRITE_TO_LMH6401_CMD);
			break;
		case HEXAGON_CAL_GAIN_3: 
			u32DacAddr = (HEXAGON_GAIN3_CS | HEXAGON_WRITE_TO_LMH6401_CMD);  
			break;
		case HEXAGON_CAL_GAIN_4: 
			u32DacAddr = (HEXAGON_GAIN4_CS | HEXAGON_WRITE_TO_LMH6401_CMD);  
			break;
		
		default: return CS_INVALID_DAC_ADDR;
	}

//	TraceCalib( 0, TRACE_ALL_DAC, u32Data, u32DacAddr);
#ifdef DBG_CALIB_DAC
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("WriteToCalibDac. u32Data=0x%x u32DacAddr=0x%x\n"), u32Data, u32DacAddr );
	OutputDebugString(szDbgText);
#endif

	int32 i32Ret = WriteRegisterThruNios(u32Data, u32DacAddr);
	BBtiming(20);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
// overwrite gain code with value in the registry.
//------------------------------------------------------------------------------
int32 CsHexagon::ForceGainControl(const uInt32 u32Swing, uInt16 u16GainCode)
{
	PHEXAGON_CAL_LUT pLut = s_CalCfg50;;
	int nCount = c_HEXAGON_50_CAL_LUT_SIZE;

	if ( u16GainCode > HEXAGON_GAIN_CODE_MAX)
		return CS_INVALID_PARAMETER;
	
	for( int i = 0; i < nCount; i++ )
	{
		if ( u32Swing == pLut[i].u32FeSwing_mV )
		{
#ifdef DBG_CALIB_INFO
			sprintf_s( szDbgText, sizeof(szDbgText), TEXT("ForceGainControl. i=%d u32Swing=%d  u16GainCtrl=%d GainCode %d \n"), 
							i, u32Swing, pLut[i].u16GainCtrl, u16GainCode );
			OutputDebugString(szDbgText);
#endif
			if (pLut[i].u16GainCtrl !=u16GainCode)
				pLut[i].u16GainCtrl = u16GainCode;
			return CS_SUCCESS;
		}
	}

	return CS_INVALID_GAIN;
}

//-----------------------------------------------------------------------------
// The firmware always prevents reading the memory beyond the segment size.
// In multi segments transfer mode, we allow the user to read multiple segments in one data transfer.
// In order to read the full memory, the SW driver have to change the segment size to the size of memory
//-----------------------------------------------------------------------------
int32	CsHexagon::SetupForReadFullMemory(bool bSetup)
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
			i64PseudoMaxDepth /= HEXAGON_DEPTH_INCREMENT;
			i64PseudoMaxDepth *= HEXAGON_DEPTH_INCREMENT;

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
			AcqCfgEx.u32ExpertOption	= m_PlxData->CsBoard.u32BaseBoardOptions[m_pCardState->i32ConfigBootLocation];

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
			i32Status = SendSegmentSetting(m_OldAcqConfig.u32SegmentCount, m_OldAcqConfig.i64Depth, m_OldAcqConfig.i64SegmentSize, m_OldAcqConfig.i64TriggerHoldoff, m_OldAcqConfig.i64TriggerDelay);
			if (CS_FAILED(i32Status))
				return i32Status;

			CSACQUISITIONCONFIG_EX AcqCfgEx = { 0 };
			AcqCfgEx.AcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);
			CsGetAcquisitionConfig(&AcqCfgEx.AcqCfg);

			AcqCfgEx.i64HwDepth			= m_OldAcqConfig.i64Depth;
			AcqCfgEx.i64HwSegmentSize	= m_OldAcqConfig.i64SegmentSize;
			AcqCfgEx.u32Decimation		= m_pCardState->AddOnReg.u32Decimation;
			AcqCfgEx.u32ExpertOption	= m_PlxData->CsBoard.u32BaseBoardOptions[m_pCardState->i32ConfigBootLocation];

			i32Status = IoctlSetAcquisitionConfigEx(&AcqCfgEx);
			m_pCardState->bMultiSegmentTransfer = FALSE;
		}
	}
	UpdateCardState();
	return i32Status;
}
