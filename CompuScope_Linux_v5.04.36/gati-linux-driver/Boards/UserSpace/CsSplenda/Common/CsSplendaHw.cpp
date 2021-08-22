//	CsxyG8Hw.cpp
///	Implementation of CsSplendaDev class
//
//	This file contains all member functions that access to addon hardware including
//	Frontend set up and calibration
//
//

#include "StdAfx.h"
#include "CsSplenda.h"
#include "CsSplendaHwCaps.h"
#include "CsSplendaOptions.h"
#include "CsSplendaFlash.h"
#include "CsEventLog.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#endif
#include "CsNucleonBaseFlash.h"
#include "CsIoctl.h"

#define	 CSPLNDA_CALIB_TIMEOUT	KTIME_MILISEC(300)		// 300 milisecond timeout
const uInt32 CsSplendaDev::c_u32R77_R67 = 50;



const FRONTEND_SPLENDA_INFO	CsSplendaDev::c_FrontEndSplendaGain50[] = 
{						 //	1M |Attn0  50_DC 1M_AC 1M|Mux Cal fltr nUsed|Attn1  Gain nUsed
	10000, 0x0000080C,   //  0 |  1     0     0     0|0    0   0     0  |  1     10   0
	 4000, 0x00000804,   //  0 |  1     0     0     0|0    0   0     0  |  0     10   0
	 2000, 0x0000000C,   //  0 |  0     0     0     0|0    0   0     0  |  1     10   0
	 1000, 0x0000000A,   //  0 |  0     0     0     0|0    0   0     0  |  1     01   0
	  400, 0x00000002,   //  0 |  0     0     0     0|0    0   0     0  |  0     01   0
	  200, 0x00000000,   //  0 |  0     0     0     0|0    0   0     0  |  0     00   0
};

#define c_SPLENDA_50_FE_INFO_SIZE   (sizeof(CsSplendaDev::c_FrontEndSplendaGain50)/sizeof(FRONTEND_SPLENDA_INFO))

const FRONTEND_SPLENDA_INFO	CsSplendaDev::s_FrontEndSplendaGain1M[] = 
{    					 //	1M |Attn0 50_DC 1M_AC 1M|Mux Cal fltr nUsed|Attn1  Gain nUsed 
   100000, 0x0000180C,   //  1 |   1     0    0    0|0    0   0     0  |  1     10   0
	40000, 0x00001804,   //  1 |   1     0    0    0|0    0   0     0  |  0     10   0
	20000, 0x00001802,   //  1 |   1     0    0    0|0    0   0     0  |  0     01   0
	10000, 0x00001800,   //  1 |   1     0    0    0|0    0   0     0  |  0     00   0
	 4000, 0x00001884,   //  1 |   1     0    0    0|1    0   0     0  |  0     10   0
     2000, 0x00001882,   //  1 |   1     0    0    0|1    0   0     0  |  0     01   0
     1000, 0x00001880,   //  1 |   1     0    0    0|1    0   0     0  |  0     00   0
	  400, 0x00001902,   //  1 |   1     0    0    1|0    0   0     0  |  0     01   0
	  200, 0x00001900,   //  1 |   1     0    0    1|0    0   0     0  |  0     00   0
};				 
#define c_SPLENDA_1M_FE_INFO_SIZE   (sizeof(CsSplendaDev::c_FrontEndSplendaGain1M)/sizeof(FRONTEND_SPLENDA_INFO))

#define CSPLENDA_10V_1MOHM_DIV50	0x00001800

const FRONTEND_SPLENDA_INFO	CsSplendaDev::s_FrontEndSplendaGain1M_div10[] = 
{    					 //	1M |Attn0 50_DC 1M_AC 1M|Mux Cal fltr nUsed|Attn1  Gain nUsed 
   100000, 0x0000180C,   //  1 |   1     0    0    0|0    0   0     0  |  1     10   0
	40000, 0x00001804,   //  1 |   1     0    0    0|0    0   0     0  |  0     10   0
	20000, 0x0000188C,   //  1 |   1     0    0    0|1    0   0     0  |  1     10   0
	10000, 0x0000188A,   //  1 |   1     0    0    0|1    0   0     0  |  1     01   0   
	 4000, 0x00001882,   //  1 |   1     0    0    0|1    0   0     0  |  0     01   0
     2000, 0x0000190C,   //  1 |   1     0    0    1|0    0   0     0  |  1     10   0
     1000, 0x0000190A,   //  1 |   1     0    0    1|0    0   0     0  |  1     01   0
	  400, 0x00001902,   //  1 |   1     0    0    1|0    0   0     0  |  0     01   0
	  200, 0x00001900,   //  1 |   1     0    0    1|0    0   0     0  |  0     00   0
};				 


SPLENDA_CAL_LUT	CsSplendaDev::s_CalCfg50[] = 
{ //u32FeSwing_mV i32PosLevel_uV  i32NegLevel_uV u16PosRegMask u16NegRegMask
	    0,              -18,             -18,         0xee,       0xee,
	  200,           194475,         -194468,         0x9e,       0x9b,      
	  400,           388950,         -388937,         0xbe,       0xbb,
	 1000,           949584,         -949552,         0xde,       0xdb,
	 2000,          1853403,        -1853225,         0x9d,       0x97,
     4000,          3706797,        -3706450,         0xbd,       0xb7,
    10000,          9049578,        -9048951,         0xdd,       0xd7,
};
#define c_SPLENDA_50_CAL_LUT_SIZE   (sizeof(CsSplendaDev::s_CalCfg50)/sizeof(CsSplendaDev::s_CalCfg50[0]))

SPLENDA_CAL_LUT	CsSplendaDev::s_CalCfg1M[] = 
{//u32FeSwing_mV i32PoshLevel_uV i32NegLevel_uV u16PosRegMask u16NegRegMask
	    0,              -18,             -18,         0xee,       0xee,
	  200,            77981,          -74298,         0x8e,       0x8b,      
	  400,           194475,         -194468,         0x9e,       0x9b,
	 1000,           380766,         -362785,         0xce,       0xcb,
	 2000,           942359,         -589363,         0x8d,       0x87,
     4000,          1853403,        -1853225,         0x9d,       0x97,
    10000,          4601372,        -3706360,         0xcd,       0xb7,
	20000,          9049818,        -9048951,         0xdd,       0xd7,
	40000,          9049818,        -9048951,         0xdd,       0xd7,
   100000,          9049818,        -9048951,         0xdd,       0xd7,
};
#define c_SPLENDA_1M_CAL_LUT_SIZE   (sizeof(CsSplendaDev::s_CalCfg1M)/sizeof(CsSplendaDev::s_CalCfg1M[0]))


const SPLENDA_SAMPLE_RATE_INFO CsSplendaDev::c_SPLNDA_SR_INFO_200[] =
{// SampleRate ClkFreq	Decim  
	200000000,  200,       1,  
	100000000,  200,       2,  
	 50000000,  200,       4,  
	 25000000,  200,       8,  
	 10000000,  200,      20,  
	  5000000,  200,      40,  
	  2000000,  200,     100,  
	  1000000,  200,     200,  
	   500000,  200,     400,  
	   200000,  200,    1000,  
	   100000,  200,    2000,  
	    50000,  200,    4000,  
	    20000,  200,   10000,  
		10000,  200,   20000,  
		 5000,  200,   40000,  
	     2000,  200,  100000,  
	     1000,  200,  200000,  
};
#define c_SPLNDA_SR_INFO_SIZE_200   (sizeof(c_SPLNDA_SR_INFO_200)/sizeof(SPLENDA_SAMPLE_RATE_INFO))


const SPLENDA_SAMPLE_RATE_INFO CsSplendaDev::c_SPLNDA_SR_INFO_100[] =
{// SampleRate ClkFreq	Decim  
	100000000,  100,       1,  
	 50000000,  100,       2,  
	 25000000,  100,       4,  
	 10000000,  100,      10,  
	  5000000,  100,      20,  
	  2000000,  100,      50,  
	  1000000,  100,     100,  
	   500000,  100,     200,  
	   200000,  100,     500,  
	   100000,  100,    1000,  
	    50000,  100,    2000,  
	    20000,  100,    5000,  
		10000,  100,   10000,  
		 5000,  100,   20000,  
	     2000,  100,   50000,  
	     1000,  100,  100000,  
};
#define c_SPLNDA_SR_INFO_SIZE_100   (sizeof(c_SPLNDA_SR_INFO_100)/sizeof(SPLENDA_SAMPLE_RATE_INFO))



//-----------------------------------------------------------------------------
//----- SendClockSetting
//-----------------------------------------------------------------------------

int32	CsSplendaDev::SendClockSetting(LONGLONG llSampleRate, uInt32 u32ExtClk, uInt32 u32ExtClkSampleSkip, BOOLEAN bRefClk )
{
	SPLENDA_SAMPLE_RATE_INFO SrInfo = {0};
	int32 i32Ret;
	uInt16 u16ClkSelect = 0;
	
	if ( 0 != u32ExtClk )
	{
		if( (CSPLNDA_MIN_EXT_CLK > llSampleRate) || (m_pCardState->SrInfo[0].llSampleRate < llSampleRate) )
			return CS_EXT_CLK_OUT_OF_RANGE;

		if ( (u32ExtClkSampleSkip / 1000 ) == 0 )
			SrInfo.u32Decimation = 1;
		else
			SrInfo.u32Decimation = u32ExtClkSampleSkip / 1000;

		SrInfo.llSampleRate = llSampleRate;
		SrInfo.u32ClockFrequencyMhz = m_pCardState->SrInfo[0].u32ClockFrequencyMhz;
		SrInfo.u32ClockFrequencyMhz |= CSPLNDA_EXT_CLK_BYPASS;
		SrInfo.u32ClockFrequencyMhz |= CSPLNDA_EXT_CLK_BYPASS;

		if ( ermMaster == m_pCardState->ermRoleMode || ermStandAlone == m_pCardState->ermRoleMode )
			u16ClkSelect |= CSPLNDA_SET_EXT_CLK;
	}
	else
	{
		if( m_bClockFilter )
			u16ClkSelect |= CSPLNDA_SET_CLK_FILTER;
		
		if( !bRefClk )
			u16ClkSelect |= CSPLNDA_SET_TCXO_REF_CLK;

		if ( !FindSrInfo( llSampleRate, &SrInfo ) )
			return CS_INVALID_SAMPLE_RATE;

		if( m_bCalibActive ||
			((SrInfo.u32ClockFrequencyMhz == m_pCardState->SrInfo[0].u32ClockFrequencyMhz) && (ermSlave != m_pCardState->ermRoleMode)) )
			u16ClkSelect |= CSPLNDA_CLK_SRC_SEL;
	}
	bool bResetNeeded = false;
	i32Ret = WriteRegisterThruNios(u16ClkSelect, CSPLNDA_CLK_SET_WR_CMD);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	m_pCardState->AddOnReg.u16ClkSelect = u16ClkSelect;
	if ( (m_pCardState->AddOnReg.u16ClkSelect & ~CSPLNDA_CLK_OUT_ENABLE) != u16ClkSelect )
		bResetNeeded = true;

	if ( ermMaster == m_pCardState->ermRoleMode )
		SrInfo.u32ClockFrequencyMhz |= CSPLNDA_MASTER_CLK;
	else if ( ermSlave == m_pCardState->ermRoleMode )
		SrInfo.u32ClockFrequencyMhz |= CSPLNDA_EXT_CLK_BYPASS;

	uInt16 u16DecimationLow = uInt16(SrInfo.u32Decimation & 0x7fff);
	uInt16 u16DecimationHigh = uInt16((SrInfo.u32Decimation>>15) & 0x7fff);
		
	if ( m_pCardState->AddOnReg.u32Decimation != SrInfo.u32Decimation )
	{
		i32Ret = WriteRegisterThruNios(u16DecimationLow, CSPLNDA_DEC_FACTOR_LOW_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = WriteRegisterThruNios(u16DecimationHigh, CSPLNDA_DEC_FACTOR_HIGH_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		
		m_pCardState->AddOnReg.u32Decimation = SrInfo.u32Decimation;
		m_pCardState->AcqConfig.u32ExtClkSampleSkip = u32ExtClkSampleSkip;
		bResetNeeded = true;
	}

	if( (m_pCardState->AddOnReg.u32ClkFreq != SrInfo.u32ClockFrequencyMhz ) )
	{
		i32Ret = SetClockThruNios( SrInfo.u32ClockFrequencyMhz );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		m_pCardState->AddOnReg.u32ClkFreq = SrInfo.u32ClockFrequencyMhz;
		bResetNeeded = true;
	}

	// Always disable clock out during calibration
	if( ! m_bCalibActive && eclkoNone != m_pCardState->eclkoClockOut )
	{
		if( eclkoRefClk == m_pCardState->eclkoClockOut)
			u16ClkSelect |= CSPLNDA_SET_REFCLK_OUT;
		// Set clock out
		u16ClkSelect |= CSPLNDA_CLK_OUT_ENABLE;
		i32Ret = WriteRegisterThruNios(u16ClkSelect, CSPLNDA_CLK_SET_WR_CMD);
		m_pCardState->AddOnReg.u16ClkSelect = u16ClkSelect;
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}

	if( bResetNeeded )
		AddonReset();

	m_pCardState->AcqConfig.i64SampleRate = SrInfo.llSampleRate;
	m_pCardState->AcqConfig.u32ExtClk = u32ExtClk;

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

int32 CsSplendaDev::RelockReference()
{
	if( 0 == (m_pCardState->AddOnReg.u16ClkSelect & (CSPLNDA_SET_EXT_CLK|CSPLNDA_SET_TCXO_REF_CLK) ) )
	{
		uInt32 u32ClkStatus = ReadRegisterFromNios(m_pCardState->AddOnReg.u16ClkSelect, CSPLNDA_CLK_SET_RD_CMD);
		if( (u32ClkStatus & CSPLNDA_PLL_NOT_LOCKED) != 0 )
		{
			return SetClockThruNios( m_pCardState->AddOnReg.u32ClkFreq );
		}
	}
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

int32	CsSplendaDev::SendCaptureModeSetting(uInt32 u32Mode)
{
	uInt16 u16ChanSelect = CSPLNDA_QUAD_CHAN;

	if ( -1 == u32Mode )
	{
		// Default acquisition mode depending on number of channels
		switch( m_PlxData->CsBoard.NbAnalogChannel )
		{
			case 4:	u32Mode = CS_MODE_QUAD;	 break;
			case 2: u32Mode = CS_MODE_DUAL;  break;
			case 1:	u32Mode = CS_MODE_SINGLE;break;
		}
	}
	else
	{
		// Validattion of acquisition mode
		switch( m_PlxData->CsBoard.NbAnalogChannel )
		{
			case 2:	if ( 0 != (u32Mode & CS_MODE_QUAD) ) return CS_INVALID_ACQ_MODE;	break;
			case 1: if ( 0 != (u32Mode & (CS_MODE_QUAD|CS_MODE_DUAL)) )return CS_INVALID_ACQ_MODE; break;
		}
	}

	if ( u32Mode & CS_MODE_QUAD )
		u16ChanSelect = CSPLNDA_QUAD_CHAN;
	else if ( u32Mode & CS_MODE_DUAL )
		u16ChanSelect = CSPLNDA_DUAL_CHAN;
	else if ( u32Mode & CS_MODE_SINGLE )
		u16ChanSelect = CSPLNDA_SINGLE_CHAN;
	
	m_pCardState->AcqConfig.u32Mode = u32Mode;

	int32 i32Ret = WriteRegisterThruNios(u16ChanSelect, CSPLNDA_DEC_CTL_WR_CMD);
	m_pCardState->AddOnReg.u16ChanSelect = u16ChanSelect;
	
	return i32Ret;
}

//-----------------------------------------------------------------------------
//-----	SendCalibModeSetting
//----- ADD-ON REGISTER
//-----------------------------------------------------------------------------
int32 CsSplendaDev::SendCalibModeSetting(const uInt16 u16Channel, const eCalMode ecMode)
{
	int32 i32Ret = CS_SUCCESS;
	
	if( (0 == u16Channel) || (u16Channel > CSPLNDA_CHANNELS)  )
	{
		return CS_INVALID_CHANNEL;
	}

	m_CalMode = ecMode;
	uInt16 u16ChanZeroBased = u16Channel-1;

	uInt32 u32FrontEndSetup = m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased];
	//No pulse
	uInt16 u16CalibSetup = CSPLNDA_PUSLE_DIS_LAST | CSPLNDA_PUSLE_DIS_FIRST;

	if( ecMode == ecmCalModeDc )
	{
		//Switch channle to DC mode
		u32FrontEndSetup &= ~CSPLNDA_SET_50_AC_COUPLING;
		u32FrontEndSetup |=  CSPLNDA_SET_50_DC_COUPLING;
		u32FrontEndSetup &= ~CSPLNDA_SET_1M_AC_COUPLING;
		u32FrontEndSetup |=  CSPLNDA_SET_1M_DC_COUPLING;
		//Switch to cal input
		u32FrontEndSetup &= ~CSPLNDA_CHAN_INPUT;
		u32FrontEndSetup |=  CSPLNDA_CHAN_CAL;
	}
	else if( ecMode == ecmCalModeNormal )
	{
		//Switch to channel input
		u32FrontEndSetup |=  CSPLNDA_CHAN_INPUT;
		u32FrontEndSetup &= ~CSPLNDA_CHAN_CAL;
	}
	else if( ecMode == ecmCalModeMs )
	{
		//Switch channle to AC mode
		u32FrontEndSetup |=  CSPLNDA_SET_50_AC_COUPLING;
		u32FrontEndSetup &= ~CSPLNDA_SET_50_DC_COUPLING;
		u32FrontEndSetup |=  CSPLNDA_SET_1M_AC_COUPLING;
		u32FrontEndSetup &= ~CSPLNDA_SET_1M_DC_COUPLING;
		//Switch to cal input
		u32FrontEndSetup &= ~CSPLNDA_CHAN_INPUT;
		u32FrontEndSetup |=  CSPLNDA_CHAN_CAL;

		//Ebable pusle with long pulsewidth
		u16CalibSetup = CSPLNDA_PUSLE_TO_CAL | CSPLNDA_PUSLE_EN | CSPLNDA_PUSLE_WIDTH;
	}

	i32Ret = WriteRegisterThruNios(u32FrontEndSetup | (u16Channel & 0xf) << 13, CSPLXBASE_SETFRONTEND_CMD);
	if( CS_FAILED(i32Ret) ) return i32Ret;

	i32Ret = WriteRegisterThruNios(u16CalibSetup, CSPLNDA_PULSE_SET_WR_CMD);
	if( CS_FAILED(i32Ret) ) return i32Ret;

	TraceCalib( u16Channel, TRACE_CAL_PROGRESS, 1, ecMode );

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsSplendaDev::SendChannelSetting(uInt16 u16HwChannel, uInt32 u32InputRange, uInt32 u32Term, uInt32 u32Impedance, int32 i32Position, uInt32 u32Filter, int32 i32ForceCalib)
{
	int32 i32Ret = CS_SUCCESS;

	if( (0 == u16HwChannel) || (u16HwChannel > CSPLNDA_CHANNELS)  )
	{
		return CS_INVALID_CHANNEL;
	}
	uInt16 u16ChanZeroBased = u16HwChannel-1;
	uInt32 u32FrontIndex = 0;

	i32Ret = FindFeIndex( u32InputRange, u32Impedance, u32FrontIndex);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	uInt32 u32FrontEndSetup	= 0;
	u32FrontEndSetup = m_pCardState->u32FeSetupTable[CS_REAL_IMP_50_OHM == u32Impedance? 0 : 1][u32FrontIndex];

	if ( (CS_COUPLING_AC == (u32Term & CS_COUPLING_MASK)) )
	{
		if ( CS_REAL_IMP_50_OHM == u32Impedance )
			u32FrontEndSetup |= CSPLNDA_SET_50_AC_COUPLING;
	}
	else if ( CS_COUPLING_DC == (u32Term & CS_COUPLING_MASK) )
	{
		if ( CS_REAL_IMP_1M_OHM == u32Impedance )
			u32FrontEndSetup |= CSPLNDA_SET_1M_DC_COUPLING;
	}
	else
	{
		return CS_INVALID_COUPLING;
	}
	
	if( m_bNoFilter )
	{
		m_pCardState->ChannelParams[u16ChanZeroBased].u32Filter = u32Filter;
		u32FrontEndSetup |= CSPLNDA_SET_CHAN_FILTER;
	}
	else if ( 0 == u32Filter  )
	{
		u32FrontEndSetup |= CSPLNDA_SET_CHAN_FILTER;
	}
	else
	{
		u32FrontEndSetup &= ~CSPLNDA_SET_CHAN_FILTER;
	}

	if( m_pCardState->bInCal[u16ChanZeroBased] )
	{
		u32FrontEndSetup &= ~CSPLNDA_CHAN_INPUT;
		u32FrontEndSetup |=  CSPLNDA_CHAN_CAL;
        
		u32FrontEndSetup &= ~CSPLNDA_SET_50_AC_COUPLING;
		u32FrontEndSetup |=  CSPLNDA_SET_50_DC_COUPLING;
		u32FrontEndSetup &= ~CSPLNDA_SET_1M_AC_COUPLING;
		u32FrontEndSetup |=  CSPLNDA_SET_1M_DC_COUPLING;
	}
	else
	{
		u32FrontEndSetup &= ~CSPLNDA_CHAN_CAL;
		u32FrontEndSetup |=  CSPLNDA_CHAN_INPUT;
	}
	
	if( i32ForceCalib < 0 )
	{
		m_u32IgnoreCalibErrorMaskLocal |= 1 << u16ChanZeroBased;
	}
	else if( i32ForceCalib > 0 )
	{
		m_pCardState->bCalNeeded[u16ChanZeroBased] = true;
		m_bForceCal[u16ChanZeroBased] = true;
		i32Ret = SendAdcOffsetAjust(u16HwChannel, m_u16AdcOffsetAdjust );
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}
	

	if ( m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] != u32FrontEndSetup )
	{
		// The papacitor is chared on 1 MOhm DC coupling
		if ( 0 != ( u32Term & CS_COUPLING_DC )&&( CS_REAL_IMP_1M_OHM == u32Impedance ) )
			m_pCardState->bCapacitorCharged[u16ChanZeroBased] = TRUE;

		if (  m_pCardState->bCapacitorCharged[u16ChanZeroBased]  && 0 != (u32Term & CS_COUPLING_AC) &&( CS_REAL_IMP_1M_OHM == u32Impedance ) )
		{
			// Switching from DC to AC requires extras delay for capacitor to discharge
			m_bExtraDelayNeeded = true;
			m_pCardState->bCapacitorCharged[u16ChanZeroBased] = FALSE;
		}

		uInt32 u32Reg = u32FrontEndSetup | (u16HwChannel & 0xf) << 13; 
		i32Ret = WriteRegisterThruNios(u32Reg, CSPLXBASE_SETFRONTEND_CMD);
		if( CS_FAILED(i32Ret) ) return i32Ret;

		m_pCardState->bCalNeeded[u16ChanZeroBased] = true;
		
		m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] = u32FrontEndSetup;

		m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange = u32InputRange;
		m_pCardState->ChannelParams[u16ChanZeroBased].u32Term = u32Term & CS_COUPLING_MASK;
		m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance = u32Impedance;
		m_pCardState->ChannelParams[u16ChanZeroBased].u32Filter = u32Filter;
	}

	i32Ret = SetPosition(u16HwChannel, i32Position);
	
	return i32Ret;
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

int32 CsSplendaDev::SendTriggerRelation( uInt32 u32Relation	)
{
	uInt16	u16Data = 0;
	uInt32	u32Command = CSPLNDA_DTS_GBL_SET_WR_CMD;
	int32	i32Ret ;

	u32Relation;

	i32Ret = WriteRegisterThruNios(u16Data, u32Command);
	return i32Ret;
}

//-----------------------------------------------------------------------------
//----- SendTriggerEngineSetting
//----- ADD-ON REGISTER
//----- Address CS12400_TRIG_ENG_1_ADD, CS12400_TRIG_ENG_2_ADD, CS12400_EXT_TRIG_ADD
//-----------------------------------------------------------------------------
int32 CsSplendaDev::SendTriggerEngineSetting(uInt16	  u16ChannelTrigEngine,
											  int32	  i32SourceA,
											  uInt32  u32ConditionA,
											  int32   i32LevelA,
											  int32	  i32SourceB,
											  uInt32  u32ConditionB,
											  int32   i32LevelB )
{
	uInt16	u16Data = 0;
	uInt32	u32Command = CSPLNDA_DTS1_SET_WR_CMD;
	uInt16	u16TrigEngine = 1 + 2*(u16ChannelTrigEngine-1);

	if ( u16TrigEngine == 0 )
		return CS_INVALID_TRIGGER;

	// These inputs should have been already validated from DLL level
	// Double check ..
	if ( ( i32LevelA > 100  || i32LevelA < -100 ) ||
		 ( i32LevelB > 100  || i32LevelB < -100 ) )
		return CS_INVALID_TRIG_LEVEL;

	if ( i32SourceA > m_PlxData->CsBoard.NbAnalogChannel || i32SourceB > m_PlxData->CsBoard.NbAnalogChannel )
		return CS_INVALID_TRIG_SOURCE;

	m_pCardState->TriggerParams[u16TrigEngine].i32Source    = i32SourceA;
	m_pCardState->TriggerParams[u16TrigEngine].u32Condition = u32ConditionA;
	m_pCardState->TriggerParams[u16TrigEngine].i32Level     = i32LevelA;

	if ( i32SourceA != 0 )
	{
		u16Data |= CSPLNDA_SET_DIG_TRIG_A_ENABLE;

		if ( (int32) u16ChannelTrigEngine != ConvertToHwChannel((uInt16)i32SourceA) )
			u16Data |= CSPLNDA_SET_DIG_TRIG_A_ADJ_INPUT;
	}
	
	if ( u32ConditionA == CS_TRIG_COND_NEG_SLOPE )
		u16Data |= CSPLNDA_SET_DIG_TRIG_A_NEG_SLOPE;

	m_pCardState->TriggerParams[u16TrigEngine+1].i32Source    = i32SourceB;
	m_pCardState->TriggerParams[u16TrigEngine+1].u32Condition = u32ConditionB;
	m_pCardState->TriggerParams[u16TrigEngine+1].i32Level     = i32LevelB;

	if ( i32SourceB != 0 )
	{
		u16Data |= CSPLNDA_SET_DIG_TRIG_B_ENABLE;
		if ( (int32) u16ChannelTrigEngine != ConvertToHwChannel((uInt16)i32SourceB) )
			u16Data |= CSPLNDA_SET_DIG_TRIG_B_ADJ_INPUT;
	}

	if ( u32ConditionB == CS_TRIG_COND_NEG_SLOPE )
		u16Data |= CSPLNDA_SET_DIG_TRIG_B_NEG_SLOPE;


	u32Command = CSPLNDA_DTS1_SET_WR_CMD + (u16ChannelTrigEngine - 1) * 0x100;

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

	i32Ret =  SendDigitalTriggerSensitivity( u16TrigEngine, TRUE, m_u8TrigSensitivity );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret =  SendDigitalTriggerSensitivity( u16TrigEngine, FALSE, m_u8TrigSensitivity );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSplendaDev::SendExtTriggerSetting( BOOLEAN  bExtTrigEnabled,
										   eTrigEnableMode eteSet,
										   int32	i32ExtLevel,
										   uInt32	u32ExtCondition,
										   uInt32	u32ExtTriggerRange, 
										   uInt32	u32ExtCoupling )
{
	uInt32	u32Data = 0;
	int32	i32Ret;

	// Enabled all internal Trigger engines
	u32Data |= CSPLNDA_SET_GBLTRIG_ENABLED;

	if ( !m_pCardState->bDisableTrigOut )
		u32Data |= CSPLNDA_SET_DIG_TRIG_TRIGOUTENABLED;

	// Enabled all internal Trigger engines
	u32Data |= CSPLNDA_SET_GBLTRIG_ENABLED;

	switch(eteSet)
	{
		default:
		case eteAlways:      u32Data |= CSPLNDA_SET_DIG_TRIG_EXT_EN_OFF;
						     break;
		case eteExtLive:     u32Data |= CSPLNDA_SET_DIG_TRIG_EXT_EN_LIVE;
						     break;
		case eteExtPosLatch: u32Data |= CSPLNDA_SET_DIG_TRIG_EXT_EN_POS;
						     break;
		case eteExtNegLatch: u32Data |= CSPLNDA_SET_DIG_TRIG_EXT_EN_NEG;
						     break;
	}
	
	if( !m_TrigOutCfg.bClockPolarity )
		u32Data |= CSPLNDA_SET_TRIGOUT_CLKPHASE;

	i32Ret = WriteRegisterThruNios(u32Data, CSPLNDA_TRIG_SET_WR_CMD);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	m_pCardState->AddOnReg.u16TrigEnable = (uInt16) u32Data;

	i32Ret = WriteRegisterThruNios(m_TrigOutCfg.u16Delay, CSPLNDA_TRIGDELAY_SET_WR_CMD);
	if( CS_FAILED(i32Ret) ) return i32Ret;

	u32Data = 0;
	if ( bExtTrigEnabled )
	{
		if(	this != m_MasterIndependent )
			return CS_INVALID_EXT_TRIG;
		
		if( CS_GAIN_2_V == u32ExtTriggerRange ) 
			u32Data |= CSPLNDA_SET_EXT_TRIG_EXT1V;
		else if( CS_GAIN_10_V != u32ExtTriggerRange ) 
			return CS_INVALID_GAIN;

		if( CS_COUPLING_DC == u32ExtCoupling ) 
			u32Data |=  CSPLNDA_SET_EXT_TRIG_DC;
		else if( CS_COUPLING_AC != u32ExtCoupling)
			  return CS_INVALID_COUPLING;

		if( CS_TRIG_COND_POS_SLOPE == u32ExtCondition )
			u32Data |= CSPLNDA_SET_EXT_TRIG_POS_SLOPE;
		else if( CS_TRIG_COND_NEG_SLOPE != u32ExtCondition )
			return CS_INVALID_TRIG_COND;

		u32Data |= CSPLNDA_SET_EXT_TRIG_ENABLED;
	}

	i32Ret = WriteRegisterThruNios(u32Data, CSPLNDA_EXT_TRIG_SET_WR_CMD);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	m_pCardState->TriggerParams[CSPLNDA_TRIGENGINE_EXT].u32ExtTriggerRange	= u32ExtTriggerRange;
	m_pCardState->TriggerParams[CSPLNDA_TRIGENGINE_EXT].u32ExtCoupling		= u32ExtCoupling;
	m_pCardState->TriggerParams[CSPLNDA_TRIGENGINE_EXT].u32ExtImpedance	= CSPLNDA_DEFAULT_EXT_IMPEDANCE;
	m_pCardState->TriggerParams[CSPLNDA_TRIGENGINE_EXT].u32Condition		= u32ExtCondition;
	m_pCardState->TriggerParams[CSPLNDA_TRIGENGINE_EXT].i32Source			= bExtTrigEnabled ? CS_TRIG_SOURCE_EXT:CS_TRIG_SOURCE_DISABLE;

	if ( bExtTrigEnabled )
	{
		i32Ret = SendExtTriggerLevel( i32ExtLevel, u32ExtCondition, u32ExtTriggerRange);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}
	m_pCardState->TriggerParams[CSPLNDA_TRIGENGINE_EXT].i32Level = i32ExtLevel;

	return CS_SUCCESS;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSplendaDev::SendDigitalTriggerLevel( uInt16 u16TrigEngine, BOOLEAN bA, int32 i32Level )
{
	uInt16	u16Data = 0;
	int32	i32LevelConverted = i32Level;
	uInt32	u32Command;

	//----- Convert from (-100, 99) range to ADC range
	int32  i32ADCRange = 2 * CSPLNDA_DIG_TRIGLEVEL_SPAN;
	i32LevelConverted *= i32ADCRange;
	i32LevelConverted /= 200L;

	if( 2*CSPLNDA_DIG_TRIGLEVEL_SPAN <= i32LevelConverted )
		i32LevelConverted = 2* CSPLNDA_DIG_TRIGLEVEL_SPAN - 1;
	else if( -2*CSPLNDA_DIG_TRIGLEVEL_SPAN > i32LevelConverted )
		i32LevelConverted = -2*CSPLNDA_DIG_TRIGLEVEL_SPAN;

	int16 i16LevelDAC = (int16)i32LevelConverted;

	u16Data |= i16LevelDAC;

	if ( bA )
		u32Command = CSPLNDA_DL_1A_SET_WR_CMD + (u16TrigEngine - 1) * 0x200;
	else
		u32Command = CSPLNDA_DL_1B_SET_WR_CMD + (u16TrigEngine - 1) * 0x200;

	return WriteRegisterThruNios(u16Data, u32Command);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSplendaDev::SendDigitalTriggerSensitivity( uInt16 u16TrigEngine, BOOLEAN bA, uInt16 u16Sensitivity )
{
	uInt32	u32Command;

	if ( bA )
		u32Command = CSPLNDA_DS_1A_SET_WR_CMD + (u16TrigEngine - 1) * 0x200;
	else
		u32Command = CSPLNDA_DS_1B_SET_WR_CMD + (u16TrigEngine - 1) * 0x200;

	return WriteRegisterThruNios(u16Sensitivity, u32Command);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsSplendaDev::SetTrigOut( uInt16 u16Value )
{
	uInt32 u32Data = m_pCardState->AddOnReg.u16TrigEnable & ~(CSPLNDA_SET_DIG_TRIG_TRIGOUTENABLED|CSPLNDA_SET_TRIGOUT_CLKPHASE);
	if (  CS_TRIGOUT == u16Value  ) 
	{
		u32Data |= CSPLNDA_SET_DIG_TRIG_TRIGOUTENABLED;
		m_pCardState->bDisableTrigOut = FALSE;
	}
	else
		m_pCardState->bDisableTrigOut = TRUE;

	if( !m_TrigOutCfg.bClockPolarity )
		u32Data |= CSPLNDA_SET_TRIGOUT_CLKPHASE;
	
	m_pCardState->AddOnReg.u16TrigEnable = (uInt16) u32Data;
	int32 i32Ret = WriteRegisterThruNios(u32Data, CSPLNDA_TRIG_SET_WR_CMD);
	if( CS_FAILED(i32Ret) ) return i32Ret;

	i32Ret = WriteRegisterThruNios(m_TrigOutCfg.u16Delay, CSPLNDA_TRIGDELAY_SET_WR_CMD);
	if( CS_FAILED(i32Ret) ) return i32Ret;

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSplendaDev::SetClockOut( eClkOutMode ecomSet )
{
	uInt16	u16Data(0);
	int32	i32Ret;
	
	switch (ecomSet)
	{
		case eclkoNone:		 break;
		case eclkoRefClk:    u16Data |= CSPLNDA_SET_REFCLK_OUT|CSPLNDA_CLK_OUT_ENABLE;	break;
		case eclkoSampleClk: u16Data |= CSPLNDA_CLK_OUT_ENABLE;   break;
		default:             return CS_INVALID_PARAMETER;
	}
	uInt16		u16CurrentSetting = m_pCardState->AddOnReg.u16ClkSelect;

	m_pCardState->eclkoClockOut = ecomSet;

	u16CurrentSetting &= ~CSPLNDA_CLK_OUT_MUX_MASK;
	u16CurrentSetting |= (u16Data & ~CSPLNDA_CLK_OUT_ENABLE);
	
	i32Ret = WriteRegisterThruNios(u16CurrentSetting, CSPLNDA_CLK_SET_WR_CMD);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	BBtiming(100);
	u16CurrentSetting |= u16Data;
	i32Ret = WriteRegisterThruNios(u16CurrentSetting, CSPLNDA_CLK_SET_WR_CMD);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	m_pCardState->AddOnReg.u16ClkSelect = u16CurrentSetting;
	
	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsSplendaDev::CheckCalibrationReferences()
{
	bool bFree = false;
	if( NULL == m_pi32CalibA2DBuffer)
	{
		bFree = true;
		m_pi32CalibA2DBuffer = (int32 *)::GageAllocateMemoryX( CSPLNDA_CALIB_BLOCK_SIZE*sizeof(int32) );
		if( NULL == m_pi32CalibA2DBuffer) return CS_INSUFFICIENT_RESOURCES;
	}	
	
	int32  i32VSet;

	//Using 2.048 V
	int32 i32Ret = SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_200_MV, CSPLNDA_POS_CAL_LEVEL, &i32VSet, true);
	if( CS_FAILED(i32Ret) ) return i32Ret;
	
	//Using 4.096 V
	i32Ret = SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_400_MV, CSPLNDA_POS_CAL_LEVEL, &i32VSet, true);
	if( CS_FAILED(i32Ret) ) return i32Ret;
	
	//Using 10 V
	i32Ret = SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_1_V, CSPLNDA_POS_CAL_LEVEL, &i32VSet, true);
	if( CS_FAILED(i32Ret) ) return i32Ret;
	
	//Using ground
	i32Ret = SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_200_MV, CSPLNDA_GND_CAL_LEVEL, &i32VSet, true);
	if( CS_FAILED(i32Ret) ) return i32Ret;

	if( bFree )
	{
		::GageFreeMemoryX(m_pi32CalibA2DBuffer);
		m_pi32CalibA2DBuffer = NULL;
	}
		
	return CS_SUCCESS;	
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
#define CSPLNDA_ADC_OFFSET_ADJUST 31
#define CSPLNDA_CAL_SRC           30
#define CSPLNDA_CAL_METHODS       29

int32 CsSplendaDev::WriteToCalibDac(uInt16 u16Addr, uInt16 u16Data, uInt8 u8Delay)
{
	uInt32 u32DacAddr = u16Addr;
	uInt32 u32Data = u8Delay;
	u32Data <<= 16;
	u32Data |= u16Data;
	
	switch ( u16Addr )
	{
		case CSPLNDA_POSITION_1: u32DacAddr = (FENDA_OFFSET1_CS | DAC7552_OUTA); break;
		case CSPLNDA_POSITION_2: u32DacAddr = (FENDA_OFFSET2_CS | DAC7552_OUTA); break;
		case CSPLNDA_POSITION_3: u32DacAddr = (FENDA_OFFSET3_CS | DAC7552_OUTA); break;
		case CSPLNDA_POSITION_4: u32DacAddr = (FENDA_OFFSET4_CS | DAC7552_OUTA); break;

		case CSPLNDA_VCALFINE_1: u32DacAddr = (FENDA_OFFSET1_CS | DAC7552_OUTB); break;
		case CSPLNDA_VCALFINE_2: u32DacAddr = (FENDA_OFFSET2_CS | DAC7552_OUTB); break;
		case CSPLNDA_VCALFINE_3: u32DacAddr = (FENDA_OFFSET3_CS | DAC7552_OUTB); break;
		case CSPLNDA_VCALFINE_4: u32DacAddr = (FENDA_OFFSET4_CS | DAC7552_OUTB); break;

		case CSPLNDA_CAL_GAIN_1: u32DacAddr = (FENDA_GAIN12_CS | DAC7552_OUTA);  break;
		case CSPLNDA_CAL_GAIN_2: u32DacAddr = (FENDA_GAIN12_CS | DAC7552_OUTB);  break;
		case CSPLNDA_CAL_GAIN_3: u32DacAddr = (FENDA_GAIN34_CS | DAC7552_OUTA);  break;
		case CSPLNDA_CAL_GAIN_4: u32DacAddr = (FENDA_GAIN34_CS | DAC7552_OUTB);  break;
		
		case CSPLNDA_EXTRIG_POSITION: u32DacAddr = (CAL_DACEXTRIG_CS | DAC7552_OUTA); break;
		case CSPLNDA_EXTRIG_LEVEL:    u32DacAddr = (CAL_DACEXTRIG_CS | DAC7552_OUTB); break;

		case CSPLNDA_CAL_METHODS:
			{
				uInt32 u32;
				SetupAcquireAverage( true );
				switch(u16Data)
				{
					case 10: CalibrateOffset(1, &u32); SendCalibModeSetting(1, ecmCalModeNormal); break;
					case 11: SendCalibModeSetting(1, ecmCalModeDc); CalibrateGain(1); SendCalibModeSetting(1, ecmCalModeNormal); break;
					case 12: SendCalibModeSetting(1, ecmCalModeDc); CalibratePosition(1); SendCalibModeSetting(1, ecmCalModeNormal); break;
					
					case 20: CalibrateOffset(2, &u32); SendCalibModeSetting(2, ecmCalModeNormal); break;
					case 21: SendCalibModeSetting(2, ecmCalModeDc); CalibrateGain(2); SendCalibModeSetting(2, ecmCalModeNormal); break;
					case 22: SendCalibModeSetting(2, ecmCalModeDc); CalibratePosition(2);  SendCalibModeSetting(2, ecmCalModeNormal); break;
					
					case 30: CalibrateOffset(3, &u32);SendCalibModeSetting(3, ecmCalModeNormal); break;
					case 31: SendCalibModeSetting(3, ecmCalModeDc); CalibrateGain(3); SendCalibModeSetting(3, ecmCalModeNormal); break;
					case 32: SendCalibModeSetting(3, ecmCalModeDc); CalibratePosition(3); SendCalibModeSetting(3, ecmCalModeNormal); break;
					
					case 40: CalibrateOffset(4, &u32); SendCalibModeSetting(4, ecmCalModeNormal); break;
					case 41: SendCalibModeSetting(4, ecmCalModeDc); CalibrateGain(4); SendCalibModeSetting(4, ecmCalModeNormal); break;
					case 42: SendCalibModeSetting(4, ecmCalModeDc); CalibratePosition(4); SendCalibModeSetting(4, ecmCalModeNormal); break;
					
					default: break;
				}
				SetupAcquireAverage( false );
			}

		case CSPLNDA_CAL_SRC:
			{
				switch(u16Data)
				{
					case   0: return SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_200_MV,  CSPLNDA_GND_CAL_LEVEL);
					
					case    1: return SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_200_MV, CSPLNDA_POS_CAL_LEVEL);
					case    2: return SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_400_MV, CSPLNDA_POS_CAL_LEVEL);
					case    5: return SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_1_V,    CSPLNDA_POS_CAL_LEVEL);
					case   10: return SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_2_V,    CSPLNDA_POS_CAL_LEVEL);
					case   20: return SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_4_V,    CSPLNDA_POS_CAL_LEVEL);
					case  100: return SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_10_V,   CSPLNDA_POS_CAL_LEVEL);

					case  501: return SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_200_MV, CSPLNDA_NEG_CAL_LEVEL);
					case  502: return SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_400_MV, CSPLNDA_NEG_CAL_LEVEL);
					case  505: return SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_1_V,    CSPLNDA_NEG_CAL_LEVEL);
					case  510: return SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_2_V,    CSPLNDA_NEG_CAL_LEVEL);
					case  520: return SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_4_V,    CSPLNDA_NEG_CAL_LEVEL);
					case  600: return SendCalibDC( CS_REAL_IMP_50_OHM, CS_GAIN_10_V,   CSPLNDA_NEG_CAL_LEVEL);

					case 1001: return SendCalibDC( CS_REAL_IMP_1M_OHM, CS_GAIN_200_MV, CSPLNDA_POS_CAL_LEVEL);
					case 1002: return SendCalibDC( CS_REAL_IMP_1M_OHM, CS_GAIN_400_MV, CSPLNDA_POS_CAL_LEVEL);
					case 1005: return SendCalibDC( CS_REAL_IMP_1M_OHM, CS_GAIN_1_V,    CSPLNDA_POS_CAL_LEVEL);
					case 1010: return SendCalibDC( CS_REAL_IMP_1M_OHM, CS_GAIN_2_V,    CSPLNDA_POS_CAL_LEVEL);
					case 1020: return SendCalibDC( CS_REAL_IMP_1M_OHM, CS_GAIN_4_V,    CSPLNDA_POS_CAL_LEVEL);
					case 1100: return SendCalibDC( CS_REAL_IMP_1M_OHM, CS_GAIN_10_V,   CSPLNDA_POS_CAL_LEVEL);
					case 1200: return SendCalibDC( CS_REAL_IMP_1M_OHM, CS_GAIN_20_V,   CSPLNDA_POS_CAL_LEVEL);

					case 1501: return SendCalibDC( CS_REAL_IMP_1M_OHM, CS_GAIN_200_MV,  CSPLNDA_NEG_CAL_LEVEL);
					case 1502: return SendCalibDC( CS_REAL_IMP_1M_OHM, CS_GAIN_400_MV,  CSPLNDA_NEG_CAL_LEVEL);
					case 1505: return SendCalibDC( CS_REAL_IMP_1M_OHM, CS_GAIN_1_V,    CSPLNDA_NEG_CAL_LEVEL);
					case 1510: return SendCalibDC( CS_REAL_IMP_1M_OHM, CS_GAIN_2_V,    CSPLNDA_NEG_CAL_LEVEL);
					case 1520: return SendCalibDC( CS_REAL_IMP_1M_OHM, CS_GAIN_4_V,    CSPLNDA_NEG_CAL_LEVEL);
					case 1600: return SendCalibDC( CS_REAL_IMP_1M_OHM, CS_GAIN_10_V,   CSPLNDA_NEG_CAL_LEVEL);
					case 1700: return SendCalibDC( CS_REAL_IMP_1M_OHM, CS_GAIN_20_V,   CSPLNDA_NEG_CAL_LEVEL);
					
					default : return CS_INVALID_GAIN;
				}
			}

		case CSPLNDA_ADC_OFFSET_ADJUST:
			if( m_u16AdcOffsetAdjust != u16Data )
			{
				m_u16AdcOffsetAdjust = u16Data;
				for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i += GetUserChannelSkip() )
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
				MsCalibrateAllChannels();
				m_MasterIndependent->ResyncClock();

			}
			return CS_SUCCESS;

		case CSPLNDA_TRIM_CAP_10_CHAN4:
		case CSPLNDA_TRIM_CAP_10_CHAN3:
		case CSPLNDA_TRIM_CAP_10_CHAN2:
		case CSPLNDA_TRIM_CAP_10_CHAN1:
			return TrimCapacitor( u16Addr - CSPLNDA_TRIM_CAP_10_CHAN1, u16Data, true );
		case CSPLNDA_TRIM_CAP_50_CHAN4:
		case CSPLNDA_TRIM_CAP_50_CHAN3:
		case CSPLNDA_TRIM_CAP_50_CHAN2:
		case CSPLNDA_TRIM_CAP_50_CHAN1:
			return TrimCapacitor( u16Addr - CSPLNDA_TRIM_CAP_50_CHAN1, u16Data, false );


		default: return CS_INVALID_DAC_ADDR;
	}

	TraceCalib( 0, TRACE_ALL_DAC, u32Data, u32DacAddr);

	int32 i32Ret = WriteRegisterThruNios(u32Data, CSPLNDA_WRITE_TO_DAC7552_CMD | u32DacAddr);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void CsSplendaDev::ReportProtectionFault()
{
	if( 0 == m_u16ChanProtectStatus )
		return;
	
	char	cStartChannel = '1' + (char) ((m_pCardState->u16CardNumber-1) * m_PlxData->CsBoard.NbAnalogChannel);
	int j = 0;
	char ChannelText[50] = {0};
	for( uInt16 u = 0; u < m_PlxData->CsBoard.NbAnalogChannel; u++ )
	{
		if( 0 != (m_u16ChanProtectStatus & ( 1 << u )) )
		{
			if ( j > 0)
			{
				ChannelText[j++] = ',';
				ChannelText[j++] = ' ';
			}
			ChannelText[j++] = '#';
			ChannelText[j++] = cStartChannel + (char) (ConvertToUserChannel( u + 1 ) - 1);
		}
	}
	CsEventLog EventLog;
	EventLog.Report( CS_INFO_OVERVOLTAGE, ChannelText );
	char	szText[128];
	sprintf_s( szText, sizeof(szText), "Protection fault on channel(s) %s\n", ChannelText );
	::OutputDebugString(szText);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSplendaDev::ResyncClock()
{
	int32 i32Ret(CS_SUCCESS);
	i32Ret = m_MasterIndependent->WriteRegisterThruNios( m_MasterIndependent->m_pCardState->AddOnReg.u16ClkSelect & ~CSPLNDA_CLK_OUT_ENABLE, CSPLNDA_CLK_SET_WR_CMD);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	i32Ret = m_MasterIndependent->WriteRegisterThruNios( m_MasterIndependent->m_pCardState->AddOnReg.u32Decimation & 0x7FFF, CSPLNDA_DEC_FACTOR_LOW_WR_CMD);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = m_MasterIndependent->WriteRegisterThruNios( (m_MasterIndependent->m_pCardState->AddOnReg.u32Decimation>>15) & 0x7FFF, CSPLNDA_DEC_FACTOR_HIGH_WR_CMD);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = m_MasterIndependent->SetClockThruNios( m_MasterIndependent->m_pCardState->AddOnReg.u32ClkFreq );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	m_MasterIndependent->AddonReset();

	for ( CsSplendaDev* pDevice = m_MasterIndependent->m_Next; NULL != pDevice; pDevice = pDevice->m_Next)
	{
		i32Ret = pDevice->WriteRegisterThruNios( pDevice->m_pCardState->AddOnReg.u16ClkSelect & ~CSPLNDA_CLK_OUT_ENABLE, CSPLNDA_CLK_SET_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		
		i32Ret = pDevice->WriteRegisterThruNios( pDevice->m_pCardState->AddOnReg.u32Decimation & 0x7FFF, CSPLNDA_DEC_FACTOR_LOW_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = pDevice->WriteRegisterThruNios( (pDevice->m_pCardState->AddOnReg.u32Decimation>>15) & 0x7FFF, CSPLNDA_DEC_FACTOR_HIGH_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = pDevice->SetClockThruNios( pDevice->m_pCardState->AddOnReg.u32ClkFreq );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		pDevice->AddonReset();
	}

	if( 0 != (m_MasterIndependent->m_pCardState->AddOnReg.u16ClkSelect & CSPLNDA_CLK_OUT_ENABLE) )
	{
		i32Ret = m_MasterIndependent->WriteRegisterThruNios(m_MasterIndependent->m_pCardState->AddOnReg.u16ClkSelect, CSPLNDA_CLK_SET_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}
	return i32Ret;
}


//-----------------------------------------------------------------------------
//----- InitCs12400AddOnRegisters
//-----------------------------------------------------------------------------
int32	CsSplendaDev::SendAddonInit()
{
	int32 i32Ret = WriteRegisterThruNios(0, CSPLXBASE_ADDON_INIT);
	if (CS_FAILED(i32Ret))	return i32Ret;

	// all this hard-coded in Nestor's new nios command CMD_ADDON_INIT
	uInt32 u32FrontEndSetup = 0x1802;
	uInt32 u32Impedance = 1000000;

	uInt32 u32InputRange = 20000;
	uInt32 u32Filter = 1; 

	uInt32 u32Term = CS_COUPLING_AC;

	for (int i = 0; i < CSPLNDA_CHANNELS; i++) 
	{
		m_pCardState->AddOnReg.u32FeSetup[i] = u32FrontEndSetup;

		m_pCardState->ChannelParams[i].u32InputRange = u32InputRange;
		m_pCardState->ChannelParams[i].u32Term = u32Term & CS_COUPLING_MASK;
		m_pCardState->ChannelParams[i].u32Impedance = u32Impedance;
		m_pCardState->ChannelParams[i].u32Filter = u32Filter;
	}

	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsSplendaDev::SetPosition(uInt16 u16Channel, int32 i32Position_mV)
{
	int32 i32Ret = CS_SUCCESS;

	if( (0 == u16Channel) || (u16Channel > CSPLNDA_CHANNELS)  )
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

int32 CsSplendaDev::SendPosition(uInt16 u16Channel, int32 i32Position_mV, uInt8 u8Delay)
{
	if( (0 == u16Channel) || (u16Channel > CSPLNDA_CHANNELS)  )
		return CS_INVALID_CHANNEL;
	uInt16 u16ChanZeroBased = u16Channel-1;

	int32 i32FrontUnipolar_mV = m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange / 2;
	
	if( i32Position_mV > i32FrontUnipolar_mV ) i32Position_mV = i32FrontUnipolar_mV;
	if( i32Position_mV <-i32FrontUnipolar_mV ) i32Position_mV =-i32FrontUnipolar_mV;
	
	uInt32 u32RangeIndex, u32FeModIndex;
	int32 i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );
	if( CS_FAILED(i32Ret) ) return i32Ret;

	uInt16 u16CodeDeltaForHalfIr = m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CodeDeltaForHalfIR;
	
	if( 0 != u16CodeDeltaForHalfIr  )
	{
		int32 i32PosCode = -(2* i32Position_mV * u16CodeDeltaForHalfIr)/i32FrontUnipolar_mV;
		i32PosCode += m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset;

		uInt16 u16Code = i32PosCode > 0 ? ( i32PosCode < int32(c_u16CalDacCount-1) ? uInt16(i32PosCode) : c_u16CalDacCount-1 ) : 0;
		uInt16 u16PositionDac  = GetControlDacId( ecdPosition, u16Channel );

		int32 i32RealPosCode = int32(u16Code) - int32(m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset);
		int32 i32RealPosition_mV = -(i32RealPosCode*i32FrontUnipolar_mV);
		i32RealPosition_mV += i32RealPosition_mV >= 0 ? int32(u16CodeDeltaForHalfIr) : -int32(u16CodeDeltaForHalfIr);
 		i32RealPosition_mV /= int32(2*u16CodeDeltaForHalfIr);

		m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = i32RealPosition_mV;
		return WriteToCalibDac(u16PositionDac, u16Code, u8Delay);
	}
	else
	{
		m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = 0;
		return CS_FALSE;
	}
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsSplendaDev::SetClockThruNios( uInt32 u32ClockMhz )
{
	uInt32	u32DataRead =  ReadRegisterFromNios(u32ClockMhz, CSPLXBASE_PLL_FREQUENCY_SET_CMD);
	if( FAILED_NIOS_DATA == u32DataRead )	return CS_INVALID_SAMPLE_RATE;
	// Read the return value from Nios

	u32DataRead &= CSPLNDA_CLK_FREQ_MASK;
	uInt32 u32SentFreq = u32ClockMhz & CSPLNDA_CLK_FREQ_MASK;
	if ( u32SentFreq !=  u32DataRead )
		if( 0 != (u32ClockMhz & CSPLNDA_EXT_CLK_BYPASS ) )
			return CS_EXT_CLK_NOT_PRESENT;
		else 
			return CS_INVALID_SAMPLE_RATE;
	else  
		return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool	CsSplendaDev::FindSrInfo( const LONGLONG llSr, SPLENDA_SAMPLE_RATE_INFO *pSrInfo )
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
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsSplendaDev::FindFeIndex( const uInt32 u32Swing, const uInt32 u32Imed, uInt32& u32Index )
{
	uInt32* pSwingTable;
	uInt32 u32MaxIndex = 0;

	if(CS_REAL_IMP_1M_OHM == u32Imed)
	{
		pSwingTable = m_pCardState->u32SwingTable[1];
		u32MaxIndex = m_pCardState->u32SwingTableSize[1];
	}
	else if (CS_REAL_IMP_50_OHM == u32Imed)
	{
		pSwingTable = m_pCardState->u32SwingTable[0];
		u32MaxIndex = m_pCardState->u32SwingTableSize[0];
	}
	else
	{
		return CS_INVALID_IMPEDANCE;
	}
	
	for( uInt32 i = 0; i < u32MaxIndex; i++ )
	{
		if ( u32Swing == pSwingTable[i] )
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
int32 CsSplendaDev::FindFeCalIndices( uInt16 u16Channel, uInt32& u32RangeIndex,  uInt32& u32FeModIndex )
{
	uInt16 u16ChanZeroBased = u16Channel-1;
	
	int32 i32Ret = FindFeIndex( m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange, m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance, u32RangeIndex);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	u32FeModIndex = 0;
	if(CS_REAL_IMP_1M_OHM == m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance)
		u32FeModIndex += CSPLNDA_MAX_FE_SET_IMPED_INC;
	else if (CS_REAL_IMP_50_OHM != m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance)
		return CS_INVALID_IMPEDANCE;

	if( CS_COUPLING_AC == (m_pCardState->ChannelParams[u16ChanZeroBased].u32Term & CS_COUPLING_MASK) )
		u32FeModIndex += CSPLNDA_MAX_FE_SET_COUPL_INC;
	else if( CS_COUPLING_DC != (m_pCardState->ChannelParams[u16ChanZeroBased].u32Term & CS_COUPLING_MASK) )
		return CS_INVALID_COUPLING;

	if(0 != m_pCardState->ChannelParams[u16ChanZeroBased].u32Filter)
		u32FeModIndex += CSPLNDA_MAX_FE_SET_FLTER_INC;
	
	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsSplendaDev::FindCalLutItem( const uInt32 u32Swing, const uInt32 u32Imped, SPLENDA_CAL_LUT& Item )
{
	PSPLENDA_CAL_LUT pLut;
	int nCount(0);
	if (CS_REAL_IMP_1M_OHM == u32Imped)
	{
		pLut = s_CalCfg1M;	
		nCount = c_SPLENDA_1M_CAL_LUT_SIZE;
	}
	else if (CS_REAL_IMP_50_OHM == u32Imped)
	{
		pLut = s_CalCfg50;
		nCount = c_SPLENDA_50_CAL_LUT_SIZE;
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
void	CsSplendaDev::InitBoardCaps()
{
	if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSPLNDA_ADDON_HW_OPTION_100MS) )
	{
		m_pCardState->u32SrInfoSize = c_SPLNDA_SR_INFO_SIZE_100;
		switch( m_pCardState->AcqConfig.i64SampleRate )
		{
		case CS_SR_50MHZ:
			m_pCardState->u32SrInfoSize -= 1;		// removed 100 MHz sampling rate
			GageCopyMemoryX( m_pCardState->SrInfo, &c_SPLNDA_SR_INFO_100[1], m_pCardState->u32SrInfoSize * sizeof(SPLENDA_SAMPLE_RATE_INFO));
			break;
		case CS_SR_25MHZ:
			m_pCardState->u32SrInfoSize -= 2;		// removed 100 and 50 MHz sampling rate
			GageCopyMemoryX( m_pCardState->SrInfo, &c_SPLNDA_SR_INFO_100[2], m_pCardState->u32SrInfoSize * sizeof(SPLENDA_SAMPLE_RATE_INFO));
			break;
		case CS_SR_10MHZ:
			m_pCardState->u32SrInfoSize -= 3;		// removed 100 ,50 and 25 MHz sampling rate
			GageCopyMemoryX( m_pCardState->SrInfo, &c_SPLNDA_SR_INFO_100[3], m_pCardState->u32SrInfoSize * sizeof(SPLENDA_SAMPLE_RATE_INFO));
			break;
		default:
			GageCopyMemoryX( m_pCardState->SrInfo, c_SPLNDA_SR_INFO_100, m_pCardState->u32SrInfoSize * sizeof(SPLENDA_SAMPLE_RATE_INFO));
			break;
		}
	}
	else
	{
		if( m_bForce100MSps )
		{
			m_pCardState->u32SrInfoSize = c_SPLNDA_SR_INFO_SIZE_200-1;
			GageCopyMemoryX( m_pCardState->SrInfo, c_SPLNDA_SR_INFO_200+1, m_pCardState->u32SrInfoSize * sizeof(SPLENDA_SAMPLE_RATE_INFO));
			for( uInt32 i = 0; i< m_pCardState->u32SrInfoSize; i++ )
			{
				m_pCardState->SrInfo[0].u32ClockFrequencyMhz = 100;
				m_pCardState->SrInfo[0].u32Decimation /= 2;
			}
		}
		else
		{
			m_pCardState->u32SrInfoSize = c_SPLNDA_SR_INFO_SIZE_200;
			GageCopyMemoryX( m_pCardState->SrInfo, c_SPLNDA_SR_INFO_200, m_pCardState->u32SrInfoSize * sizeof(SPLENDA_SAMPLE_RATE_INFO));
		}
	}

	uInt32  u32FeCount = sizeof(c_FrontEndSplendaGain50) / sizeof(c_FrontEndSplendaGain50[0]);
	uInt32  i;

	for (i = 0; i < u32FeCount; i++)
	{
		m_pCardState->u32SwingTable[0][i] = c_FrontEndSplendaGain50[i].u32Swing_mV;
		m_pCardState->u32FeSetupTable[0][i] = c_FrontEndSplendaGain50[i].u32RegSetup;
	}
	m_pCardState->u32SwingTableSize[0] = u32FeCount;

	if(0 == (m_PlxData->CsBoard.u32AddonHardwareOptions & CSPLNDA_ADDON_HW_OPTION_1MOHM_DIV10))
	{
		u32FeCount = sizeof(s_FrontEndSplendaGain1M) / sizeof(s_FrontEndSplendaGain1M[0]);
		for (i = 0; i < u32FeCount; i++)
		{
			m_pCardState->u32SwingTable[1][i] = s_FrontEndSplendaGain1M[i].u32Swing_mV;
			m_pCardState->u32FeSetupTable[1][i] = s_FrontEndSplendaGain1M[i].u32RegSetup;
			if( m_b5V1MDiv50 && (CS_GAIN_10_V == m_pCardState->u32SwingTable[1][i])  )
				m_pCardState->u32FeSetupTable[1][i] = CSPLENDA_10V_1MOHM_DIV50;
		}
		m_pCardState->u32SwingTableSize[1] = u32FeCount;
	}
	else
	{
		u32FeCount = sizeof(s_FrontEndSplendaGain1M_div10) / sizeof(s_FrontEndSplendaGain1M_div10[0]);
		for (i = 0; i < u32FeCount; i++)
		{
			m_pCardState->u32SwingTable[1][i] = s_FrontEndSplendaGain1M_div10[i].u32Swing_mV;
			m_pCardState->u32FeSetupTable[1][i] = s_FrontEndSplendaGain1M_div10[i].u32RegSetup;
		}
		m_pCardState->u32SwingTableSize[1] = u32FeCount;
	}

	m_pCardState->bVarCap = 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSPLNDA_ADDON_HW_OPTION_VAR_CAP);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsSplendaDev::FillOutBoardCaps(PCSSPLENDABOARDCAPS pCapsInfo)
{
	int32		i32Ret = CS_SUCCESS;
	uInt32		u32Temp0;
	uInt32		u32Temp1;
	uInt32		i;
	uInt32		u32Size;

	::GageZeroMemoryX( pCapsInfo, sizeof(CSSPLENDABOARDCAPS) );

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
	pCapsInfo->ExtClkTable[0].i64Max = m_pCardState->SrInfo[0].llSampleRate;
	pCapsInfo->ExtClkTable[0].i64Min = CSPLNDA_MIN_EXT_CLK;
	pCapsInfo->ExtClkTable[0].u32SkipCount = CS_SAMPLESKIP_FACTOR;
	pCapsInfo->ExtClkTable[0].u32Mode = CS_MODE_OCT | CS_MODE_QUAD | CS_MODE_DUAL | CS_MODE_SINGLE;

	//Fill Range table
	// sorting is done on user level
	//Fill longer table first 
	// we assume that longer table is a superset of the shorter one
	int nLongerTable = m_pCardState->u32SwingTableSize[0] > m_pCardState->u32SwingTableSize[1]? 0:1;
	unsigned uRanges = Min( m_pCardState->u32SwingTableSize[nLongerTable], CSPLNDA_MAX_RANGES );

	u32Size =  sizeof(pCapsInfo->RangeTable[0].strText);
	for( i = 0; i < uRanges; i++ )
	{
		pCapsInfo->RangeTable[i].u32InputRange = m_pCardState->u32SwingTable[nLongerTable][i];
		pCapsInfo->RangeTable[i].u32Reserved = nLongerTable == 0 ? CS_IMP_50_OHM : CS_IMP_1M_OHM;
		if( pCapsInfo->RangeTable[i].u32InputRange >= 2000 &&  0 == (pCapsInfo->RangeTable[i].u32InputRange % 2000))
			sprintf_s( pCapsInfo->RangeTable[i].strText, u32Size, "±%d V", pCapsInfo->RangeTable[i].u32InputRange / 2000 );
		else
			sprintf_s( pCapsInfo->RangeTable[i].strText, u32Size, "±%d mV", pCapsInfo->RangeTable[i].u32InputRange / 2 );
	}
	
	int nShorterTable = m_pCardState->u32SwingTableSize[0] > m_pCardState->u32SwingTableSize[1]? 1:0;
	unsigned uSizeShort = Min( m_pCardState->u32SwingTableSize[nShorterTable], CSPLNDA_MAX_RANGES );
	//go through the shorter table and update impedace field
	for( unsigned j = 0; j < uSizeShort; j++ )
	{
		for( i = 0; i < uRanges; i++ )
		{
			if( pCapsInfo->RangeTable[i].u32InputRange == m_pCardState->u32SwingTable[nShorterTable][j] )
			{
				pCapsInfo->RangeTable[i].u32Reserved |= nShorterTable == 0 ? CS_IMP_50_OHM : CS_IMP_1M_OHM;
				break;
			}
		}
	}

	//Fill ext trig table
	i = 0;
	u32Size =  sizeof(pCapsInfo->ExtTrigRangeTable[0].strText);
	pCapsInfo->ExtTrigRangeTable[i].u32InputRange = CS_GAIN_2_V;
	strcpy_s( pCapsInfo->ExtTrigRangeTable[i].strText, u32Size, "±1 V" );
	pCapsInfo->ExtTrigRangeTable[i].u32Reserved = CS_IMP_1M_OHM|CS_IMP_EXT_TRIG;
	i++;

	pCapsInfo->ExtTrigRangeTable[i].u32InputRange = CS_GAIN_10_V;
	strcpy_s( pCapsInfo->ExtTrigRangeTable[i].strText, u32Size, "±5 V" );
	pCapsInfo->ExtTrigRangeTable[i].u32Reserved = CS_IMP_1M_OHM|CS_IMP_EXT_TRIG;
	i++;

	//Fill coupling table
	i = 0;
	u32Size =  sizeof(pCapsInfo->CouplingTable[0].strText);

	pCapsInfo->CouplingTable[i].u32Coupling = CS_COUPLING_AC;
	strcpy_s( pCapsInfo->CouplingTable[i].strText, u32Size, "AC" );
	pCapsInfo->CouplingTable[i].u32Reserved = 0;
	i++;

	pCapsInfo->CouplingTable[i].u32Coupling = CS_COUPLING_DC;
	strcpy_s( pCapsInfo->CouplingTable[i].strText, u32Size, "DC" );
	pCapsInfo->CouplingTable[i].u32Reserved = 0;
	i++;


	//Fill impedance table
	i = 0;
	u32Size =  sizeof(pCapsInfo->ImpedanceTable[0].strText);
	pCapsInfo->ImpedanceTable[i].u32Imdepdance = CS_REAL_IMP_50_OHM;
	strcpy_s( pCapsInfo->ImpedanceTable[i].strText, u32Size, "50 Ohm" );
	pCapsInfo->ImpedanceTable[i].u32Reserved = 0;
	i++;

	pCapsInfo->ImpedanceTable[i].u32Imdepdance = CS_REAL_IMP_1M_OHM;
	strcpy_s( pCapsInfo->ImpedanceTable[i].strText, u32Size, "1 MOhm" );
	pCapsInfo->ImpedanceTable[i].u32Reserved = 0;
	i++;

	// External trigge impedance 
	pCapsInfo->ImpedanceTable[i].u32Imdepdance = CS_REAL_IMP_1M_OHM;
	strcpy_s( pCapsInfo->ImpedanceTable[i].strText, u32Size, "1 MOhm" );
	pCapsInfo->ImpedanceTable[i].u32Reserved = CS_IMP_EXT_TRIG;
	i++;


	//Fill filter table
	i = 0;
	pCapsInfo->FilterTable[i].u32LowCutoff  = m_pCardState->u32LowCutoffFreq[0];
	pCapsInfo->FilterTable[i].u32HighCutoff = m_pCardState->u32HighCutoffFreq[0];
	i++;

	pCapsInfo->FilterTable[i].u32LowCutoff  = m_pCardState->u32LowCutoffFreq[1];
	pCapsInfo->FilterTable[i].u32HighCutoff = m_pCardState->u32HighCutoffFreq[1];

	pCapsInfo->bDcOffset = true;

	return i32Ret;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSplendaDev::SendExtTriggerLevel( int32 i32Level,  uInt32 u32ExtCondition,  uInt32 u32ExtTriggerRange)
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
	i32LevelConverted *= 2*CSPLNDA_DAC_SPAN;
	i32LevelConverted /= 200L;

	// Adustment for Gain
	i32LevelConverted -= CSPLNDA_DAC_SPAN;
	i32LevelConverted *= CSPLNDA_CAL_TRIGGER_GAIN_FACTOR; //GetTriggerGainAdjust( u16TrigEngine );
	i32LevelConverted /= CSPLNDA_CAL_TRIGGER_GAIN_FACTOR;
	i32LevelConverted += CSPLNDA_DAC_SPAN;
	
	if( 2*CSPLNDA_DAC_SPAN <= i32LevelConverted )
		i32LevelConverted = 2*CSPLNDA_DAC_SPAN - 1;
	else if( 0 > i32LevelConverted )
		i32LevelConverted = 0;

	int16 i16LevelDAC = (int16)i32LevelConverted;

	i32Ret = WriteToCalibDac( CSPLNDA_EXTRIG_LEVEL, i16LevelDAC );

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


	i32Ret = WriteToCalibDac( CSPLNDA_EXTRIG_POSITION, i16LevelDAC );


	return i32Ret;

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSplendaDev::CsDisableAllTriggers()
{
	int32 i32Ret = WriteRegisterThruNios(0, CSPLNDA_TRIG_SET_WR_CMD);
	if( CS_FAILED(i32Ret) )
	{
		return i32Ret;
	}

	for ( uInt16 u16ChannelTrigEngine = 0; u16ChannelTrigEngine < CSPLNDA_CHANNELS; u16ChannelTrigEngine++)
	{
		uInt32 u32Command = CSPLNDA_DTS1_SET_WR_CMD + u16ChannelTrigEngine * 0x100;
		i32Ret = WriteRegisterThruNios(0, u32Command);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//----- WriteToSelfTestRegister
//----- ADD-ON REGISTER
//-----------------------------------------------------------------------------
int32 CsSplendaDev::WriteToSelfTestRegister( uInt16 u16SelfTestMode )
{
	uInt16	u16Data = 0;

	switch( u16SelfTestMode )
	{
	case CSST_DISABLE:
		u16Data = 0;
		break;
	case CSST_CAPTURE_DATA:
		u16Data = CSPLNDA_SELF_CAPTURE;
		break;
	case CSST_BIST:
		u16Data = CSPLNDA_SELF_BIST;
		break;
	case CSST_COUNTER_DATA:
		u16Data = CSPLNDA_SELF_COUNTER;
		break;
	case CSST_MISC:
		u16Data = CSPLNDA_SELF_MISC;
		break;
	default:
		return CS_INVALID_SELF_TEST;
	}

	int32 i32Ret = WriteRegisterThruNios(u16Data, CSPLNDA_SELFTEST_MODE_WR_CMD);
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
bool	CsSplendaDev::GetChannelProtectionStatus()
{
	if ( CSPLNDA_CHANPROTECT_INVALID == m_u16ChanProtectStatus )
	{
		m_u16ChanProtectStatus = (uInt16) (0xF & ReadRegisterFromNios( 0, CSPLNDA_PROTECTION_STATUS ));
		return ( 0 != m_u16ChanProtectStatus );
	}

	return  false;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsSplendaDev::ClearChannelProtectionStatus()
{
//	KIRQL kIrql;
//	KeAcquireSpinLock( &m_pSystem->kSpinLockForEvents, &kIrql );
	int32 i32Ret =  WriteRegisterThruNios( 0, CSPLNDA_PROTECTION_STATUS | SPI_WRITE_CMD_MASK);
//	KeReleaseSpinLock( &m_pSystem->kSpinLockForEvents, kIrql );
	
	if( CS_SUCCEEDED(i32Ret) )// Reset channels protection status This will force updatting this status on the next Data transfer
		m_u16ChanProtectStatus = CSPLNDA_CHANPROTECT_INVALID;

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsSplendaDev::MsClearChannelProtectionStatus( bool bForce )
{
	CsSplendaDev *pDevice = m_MasterIndependent;

	if ( ! bForce && ! m_MasterIndependent->m_pCardState->bAlarm )
		return CS_SUCCESS;

	m_MasterIndependent->m_pCardState->bAlarm = FALSE;
	while( pDevice )
	{
		pDevice->ClearChannelProtectionStatus();
		pDevice = pDevice->m_Next;
	}

	// Waiting for all relay stable
	Sleep(200);
	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSplendaDev::SendSegmentSetting(uInt32 u32SegmentCount, int64 i64PostDepth, int64 i64SegmentSize , int64 i64HoldOff, int64 i64TrigDelay)
{
	uInt8	u8Shift = 0;
	uInt32	u32Mode = m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE;


	AbortAcquisition(); //----- Abort the previous acquisition, i.e. reset the counters, it's weird but should be like that

	switch(u32Mode)
	{
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
	
	m_Stream.bInfiniteDepth = (i64SegmentSize == -1);		// Infinite 
	
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


	uInt32	u32SegmentSize	= (uInt32) (i64SegmentSize >> u8Shift);
	uInt32	u32PostDepth	= (uInt32) (i64PostDepth >> u8Shift);
	uInt32	u32HoldOffDepth = (uInt32) (i64HoldOff >> u8Shift);
	uInt32	u32TrigDelay    = (uInt32) (i64TrigDelay >> u8Shift);


	int32 i32Ret = CS_SUCCESS;
	if ( m_bMulrecAvgTD )
	{
		// Interchange Segment count and Mulrec Acg count
		// From firmware point of view, the SegmentCount is number of averaging
		// and the number of averaging is segment count
		i32Ret = WriteRegisterThruNios( u32SegmentCount, CSPLXBASE_SET_MULREC_AVG_COUNT );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		u32SegmentCount = m_MasterIndependent->m_u32MulrecAvgCount;
	}

	i32Ret = WriteRegisterThruNios( u32SegmentSize, CSPLXBASE_SET_SEG_LENGTH_CMD );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret = WriteRegisterThruNios( m_Stream.bInfiniteDepth ? -1 : u32PostDepth,	CSPLXBASE_SET_SEG_POST_TRIG_CMD	);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret = WriteRegisterThruNios( u32SegmentCount, CSPLXBASE_SET_SEG_NUMBER_CMD );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	if ( m_bNucleon )
	{
		// Square record
		uInt32 u32SqrPreTrig		= u32SegmentSize - u32PostDepth;
		uInt32 u32SqrTrigDelay		= (uInt32) (m_pCardState->AcqConfig.i64TriggerDelay >> u8Shift);
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

	m_u32HwSegmentCount   = u32SegmentCount;
	m_i64HwSegmentSize    = i64SegmentSize;
	m_i64HwPostDepth      = i64PostDepth;
	m_i64HwTriggerHoldoff = i64HoldOff;

	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSplendaDev::WriteCalibTableToEeprom(uInt32 u32Valid)
{
	if ( 0 != (u32Valid & (CSPLNDA_CAL_TARGETFACTOR_VALID|CSPLNDA_CAL_INFO_VALID)))
	{
		int32 i32Ret = UpdateCalibInfoStrucure();
		if ( CS_FAILED(i32Ret) ) return i32Ret;
	}
	UpdateAuxCalibInfoStrucure();
	
	uInt32 u32Addr = FIELD_OFFSET( SPLNDA_FLASH_LAYOUT, FlashImage );
	u32Addr += FIELD_OFFSET( SPLNDA_FLASH_IMAGE_STRUCT, Calib );
	uInt32 u32EepromSize = sizeof((((SPLNDA_FLASH_IMAGE_STRUCT *)0)->Calib));

	uInt32 u32CalibTableSize = sizeof(m_pCardState->CalibInfoTable);
	if ( u32EepromSize < u32CalibTableSize )
	{
		// Should not happen but playing on safe side ...
		return CS_FAIL;
	}

	//Clear valid bits if invalid were set
	uInt32 u32InvalidMask = u32Valid & (CSPLNDA_VALID_MASK >> 1 );
	m_pCardState->CalibInfoTable.u32Valid &= ~(u32InvalidMask << 1);

	//clear valid bits
	m_pCardState->CalibInfoTable.u32Valid &= (u32Valid << 1) & CSPLNDA_VALID_MASK;

	//set valid bits
	m_pCardState->CalibInfoTable.u32Valid |= u32Valid & CSPLNDA_VALID_MASK;
	if( 0 != (u32Valid & CSPLNDA_CAL_INFO_VALID) )
	{
		m_bUseEepromCalBackup = true;
		if( 0 != (u32Valid & CSPLNDA_CAL_USAGE_VALID) )
		{
			m_bUseEepromCal = true;
		}
	}

	return WriteEepromEx( &m_pCardState->CalibInfoTable, u32Addr, sizeof(m_pCardState->CalibInfoTable) );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSplendaDev::ReadCalibTableFromEeprom()
{
	uInt32	u32Addr = FIELD_OFFSET( SPLNDA_FLASH_LAYOUT, FlashImage );
	u32Addr += FIELD_OFFSET( SPLNDA_FLASH_IMAGE_STRUCT, Calib );

	uInt32 u32EepromSize = sizeof((((SPLNDA_FLASH_IMAGE_STRUCT *)0)->Calib));
	uInt32 u32CalibTableSize = sizeof(m_pCardState->CalibInfoTable);

	if ( u32EepromSize < u32CalibTableSize )
	{
		// Should not happen but playing on safe side ...
		return CS_FAIL;
	}


	uInt8	*pPtr = (uInt8 *)(&m_pCardState->CalibInfoTable);
	for( uInt32 u32 = 0; u32 < u32CalibTableSize ; u32++)
	{
		*pPtr++ = (uInt8) AddonReadFlashByte( u32Addr++ );
	}

	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void CsSplendaDev::UpdateCalibVariables()
{
	if( ( 0 != (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_INFO_VALID)   ) &&
		( 0 == (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_INFO_INVALID) ) )
	{
		m_bUseEepromCalBackup = true;
	}

	if( ( 0 != (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_USAGE_VALID)   ) &&
		( 0 == (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_USAGE_INVALID) ) &&
		m_bUseEepromCalBackup )
	{
		m_bUseEepromCal = true;
	}

	if( ( 0 != (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_PORT_RESISTANCE_VALID)   ) &&
		( 0 == (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_PORT_RESISTANCE_INVALID) ) )
	{
		for ( int i = 0; i < CSPLNDA_CHANNELS; i++ )
			m_u32FrontPortResistance_mOhm[i] = m_pCardState->CalibInfoTable.u32FrontPortResistance_mOhm[i];
	}
	else
	{
		m_u32FrontPortResistance_mOhm[0] = CSPLNDA_DEFAULT_PORT_RESISTANCE_1;
		for ( int i = 1; i < CSPLNDA_CHANNELS; i++ )
			m_u32FrontPortResistance_mOhm[i] = CSPLNDA_DEFAULT_PORT_RESISTANCE_REST;
	}

	if( ( 0 != (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_VARCAP_VALID)   ) &&
		( 0 == (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_VARCAP_INVALID) ) )
	{
		RtlCopyMemory( m_u16TrimCap, m_pCardState->CalibInfoTable.u16TrimCap, sizeof(m_u16TrimCap) );
	}
	for ( uInt16 i = 0; i < CSPLNDA_CHANNELS; i++ )
	{
		TrimCapacitor(i, m_u16TrimCap[i][0], false );
		TrimCapacitor(i, m_u16TrimCap[i][1], true );
	}
	

	if( ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSPLNDA_ADDON_HW_OPTION_AC_GAIN_ADJ) ) && 
		( 0 != (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_TARGETFACTOR_VALID)   ) &&
		( 0 == (m_pCardState->CalibInfoTable.u32Valid & CSPLNDA_CAL_TARGETFACTOR_INVALID) ) )
	{
#ifndef DBG
		CsEventLog EventLog;
		EventLog.Report( CS_INFO_TEXT, "AC gain adjust activated" );
#endif
		m_bGainTargetAdjust = true;
		RtlCopyMemory( m_u16GainTarFactor, m_pCardState->CalibInfoTable.u16GainTgFactor, sizeof(m_u16GainTarFactor) );
	}
	else
	{
		for ( int i = 0; i < CSPLNDA_CHANNELS; i++ )
		{
			for ( int j = 0; j < CSPLNDA_MAX_RANGES; j++ )
			{
				for ( int k = 0; k < CSPLNDA_IMPED; k++ )
				{
					for ( int n = 0; n < CSPLNDA_FILTERS; n++ )
						m_u16GainTarFactor[i][j][k][n] = CSPLNDA_DEFAULT_GAINTARGET_FACTOR;
				}
			}
		}
	}
}



uInt16 CsSplendaDev::ReadTriggerStatus()	
{
	if ( m_bMulrecAvgTD )
	{
		uInt32 RegValue = ReadGageRegister( INT_STATUS );
		if ( RegValue & MULREC_AVG_TRIGGERED )
			return TRIGGERED;
		else
			return WAIT_TRIGGER;
	}
	else
		return CsNiosApi::ReadTriggerStatus();
}


uInt16 CsSplendaDev::ReadBusyStatus()
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

int32 CsSplendaDev::TrimCapacitor( const uInt16 cu16ZeroBased, uInt16 u16Value, bool bDiv10 )
{
	if( !m_pCardState->bVarCap )
		return CS_SUCCESS;
	if( cu16ZeroBased >= CSPLNDA_CHANNELS )
		return CS_INVALID_CHANNEL;
	if( u16Value > CSPLNDA_VARCAP_STEPS )
		u16Value = CSPLNDA_VARCAP_STEPS;

	uInt16 u16Enable = CSPLNDA_VARCAP_BASE + cu16ZeroBased * CSPLNDA_VARCAP_CHAH_STEP;
	if( bDiv10 )
		u16Enable += CSPLNDA_VARCAP_DIV10;
	
	u16Enable = 1 << u16Enable;
	
	int32 i32Ret = WriteRegisterThruNios(u16Enable, CSPLNDA_VARCAP_SET_WR_CMD);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	for (uInt16 u = 0; u < u16Value; u++ )
	{
		uInt16 u16Data = u16Enable| CSPLNDA_VARCAP_DATA;
		i32Ret = WriteRegisterThruNios(u16Data, CSPLNDA_VARCAP_SET_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = WriteRegisterThruNios(u16Enable, CSPLNDA_VARCAP_SET_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}
	i32Ret = WriteRegisterThruNios(0, CSPLNDA_VARCAP_SET_WR_CMD);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	m_u16TrimCap[cu16ZeroBased][bDiv10?1:0] = u16Value;

	return i32Ret;
}





//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL	CsSplendaDev::IsNiosInit(void)
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


int32	CsSplendaDev::BaseBoardConfigurationReset(uInt8	u8UserImageId)
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

BOOLEAN CsSplendaDev::ConfigurationReset( uInt8	u8UserImageId )
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
int32	CsSplendaDev::ReadBaseBoardHardwareInfoFromFlash( BOOLEAN bChecksum )
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

	// Force default option for Oscar cards
	if ( m_bOscar )
		m_PlxData->CsBoard.u32BaseBoardHardwareOptions |= CSPLNDA_BB_HW_OPTION_OSCAR;
	else
		m_PlxData->CsBoard.u32BaseBoardHardwareOptions &= ~CSPLNDA_BB_HW_OPTION_OSCAR;

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

		m_PlxData->CsBoard.u32UserFwVersionB[2].u32Reg		= FwInfo.u32FirmwareVersion;
		m_PlxData->CsBoard.u32BaseBoardOptions[2]			= FwInfo.u32FirmwareOptions;
	}

	return i32Status;
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int32	CsSplendaDev::CsNucleonCardUpdateFirmware()
{
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

	if ( (hCsiFile = GageOpenFileSystem32Drivers( m_szCsiFileName )) == 0 )
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
		if ( (hFwlFile = GageOpenFileSystem32Drivers( m_szFwlFileName )) == 0 )
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
				FriendlyFwVersion( u32FwVersion, NewFwFlVer, sizeof(NewFwFlVer), m_bCsiOld);
			}
			else
			{
				hFile				= hFwlFile;
				u32ImageOffset		= m_BaseBoardExpertEntry[i-1].u32ImageOffset;
				u32FpgaImageSize	= m_BaseBoardExpertEntry[i-1].u32ImageSize;
				u32FwVersion		= m_BaseBoardExpertEntry[i-1].u32Version;
				u32FwOption			= m_BaseBoardExpertEntry[i-1].u32Option;
				FriendlyFwVersion( u32FwVersion, NewFwFlVer, sizeof(NewFwFlVer), m_bFwlOld);
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
			i32Status = CS_FWUPDATED_SHUTDOWN_REQUIRED;
		else
		{
			ConfigurationReset(0);
			// Reset the base board also require the reset of the addon board
			ReadCsiFileAndProgramFpga();
			ResetCachedState();

			// Close the handle to kernel driver
			UpdateCardState();
		}
	}

	if ( hFwlFile )
		GageCloseFile( hFwlFile );

	GageCloseFile( hCsiFile );

	GageFreeMemoryX( pBuffer );

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsSplendaDev::NucleonWriteFlash(uInt32 u32StartAddr, void *pBuffer, uInt32 u32Size, bool bBackup )
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
//
//-----------------------------------------------------------------------------

int32	CsSplendaDev::CsReadPlxNvRAM( PPLX_NVRAM_IMAGE nvRAM_image )
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

int32	CsSplendaDev::CsWritePlxNvRam( PPLX_NVRAM_IMAGE nvRAM_image )
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
int32 CsSplendaDev::IoctlGetPciExLnkInfo(PPCIeLINK_INFO pPciExInfo)
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

int32 	CsSplendaDev::CsStmTransferToBuffer( PVOID pVa, uInt32 u32TransferSizeSamples )
{
	int32 i32Status	= CS_SUCCESS;

	if ( ! m_Stream.bEnabled )
		return CS_INVALID_REQUEST;

	// Check if there is data remain
	if ( ! m_Stream.bRunForever && m_Stream.u64DataRemain == 0  )
	{
		i32Status = CS_STM_COMPLETED;
		return i32Status;
	}

	// Validation of the transfer size
	if ( m_Stream.bFifoFull )
	{
		// Fifo overflow has occured. Allow only to transfer valid data
		if ( m_Stream.i64DataInFifo > 0 )
		{
			if ( m_Stream.i64DataInFifo < u32TransferSizeSamples )
				u32TransferSizeSamples = (uInt32) m_Stream.i64DataInFifo;
		}
		else
			i32Status = CS_STM_FIFO_OVERFLOW;
	}
	else if ( ! m_Stream.bRunForever )
	{	
		if ( m_Stream.u64DataRemain < u32TransferSizeSamples )
			u32TransferSizeSamples = (uInt32) m_Stream.u64DataRemain;
	}

	if ( CS_FAILED(i32Status) )
		return i32Status;

	// Only one DMA can be active 
	if ( ::InterlockedCompareExchange( &m_Stream.u32BusyTransferCount, 1, 0 ) ) 
		return CS_INVALID_STATE;

	m_Stream.u32TransferSize = u32TransferSizeSamples;
	i32Status = IoctlStmTransferDmaBuffer( m_Stream.u32TransferSize*m_pCardState->AcqConfig.u32SampleSize, pVa );

	if ( CS_FAILED(i32Status) )
		::InterlockedCompareExchange( &m_Stream.u32BusyTransferCount, 0, 1 );

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsSplendaDev::CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *pu32ErrorFlag, uInt32 *pu32ActualSize, uInt8 *pu8EndOfData )
{
	if ( ! m_Stream.bEnabled )
		return CS_INVALID_REQUEST;

	int32 i32Status = CsStmGetTransferStatus( u32TimeoutMs, pu32ErrorFlag );
	if (CS_SUCCESS == i32Status)
	{
		if ( ! m_Stream.bRunForever )
		{
			m_Stream.u64DataRemain -= m_Stream.u32TransferSize;
			if ( pu8EndOfData )
				*pu8EndOfData	= ( m_Stream.u64DataRemain == 0 );
		}
		if ( pu32ActualSize )
			*pu32ActualSize		= m_Stream.u32TransferSize;		// Return actual DMA data size
	}

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsSplendaDev::CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *pu32ErrorFlag )
{
	// Check if there is any DMA active
	if ( 0 == m_Stream.u32BusyTransferCount )
		return CS_INVALID_STATE;

	// Only one Wait per DMA transfer is allowed
	if ( ::InterlockedCompareExchange( &m_Stream.u32WaitTransferCount, 1, 0 ) ) 
		return CS_INVALID_STATE;

	bool	bDmaCompleted = false;
	uInt32	u32LocalErrorFlag = 0;
	int32 i32Status =  IoctlStmWaitDmaComplete( u32TimeoutMs, &u32LocalErrorFlag, &bDmaCompleted );

	if ( CS_SUCCESS == i32Status )
	{
		if ( pu32ErrorFlag )
			*pu32ErrorFlag = u32LocalErrorFlag;

		if ( ! m_Stream.bFifoFull )
		{	
			if ( u32LocalErrorFlag  & STM_TRANSFER_ERROR_FIFOFULL )
			{
				// Fifo full has occured. From this point all data in memory is still valid
				// We notify the application about the state of FIFO full only when all valid data are upload
				// to application
				m_Stream.bFifoFull = true;
				if ( m_Stream.bRunForever || m_Stream.u64DataRemain > (uint64)m_PlxData->CsBoard.i64MaxMemory )
					m_Stream.i64DataInFifo = m_PlxData->CsBoard.i64MaxMemory;
				else
					m_Stream.i64DataInFifo = m_Stream.u64DataRemain;

				m_Stream.i64DataInFifo -= m_Stream.u32TransferSize;
			}
		}
		else
		{
			// Fifo full has occured. Return error if there is no more valid data to transfer.
			if ( m_Stream.i64DataInFifo > 0 )
				m_Stream.i64DataInFifo -= m_Stream.u32TransferSize;
		}
	}
	
	// The DMA request may completed with success or with error
	if (  bDmaCompleted )
		::InterlockedCompareExchange( &m_Stream.u32BusyTransferCount, 0, 1 );

	::InterlockedCompareExchange( &m_Stream.u32WaitTransferCount, 0, 1 );

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void 	CsSplendaDev::MsStmResetParams()
{
	CsSplendaDev *pDev = m_MasterIndependent;
	
	while( pDev )
	{
		PSTREAMSTRUCT pStreamStr = &pDev->m_Stream;

		pStreamStr->bFifoFull			 = false;
		pStreamStr->u32BusyTransferCount = 0;
		pStreamStr->u32WaitTransferCount = 0;
		pStreamStr->u64DataRemain		 = pStreamStr->u64TotalDataSize/m_pCardState->AcqConfig.u32SampleSize;

		pDev = pDev->m_Next;
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32	CsSplendaDev::ReadConfigBootLocation()
{
	uInt32	u32Val = 0;

	//int32	i32Status = IoctlReadFlash( (0x1FFFFFE<<1), &u32Val, sizeof(u32Val) );
	int32	i32Status = IoctlReadFlash( (0x1FFFFFE<<1), &u32Val, sizeof(u32Val) );
	if ( CS_FAILED(i32Status) )
		return 0;
	
	return u32Val;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsSplendaDev::WriteConfigBootLocation( uInt32	u32Location )
{
	char	szText[256];

	uInt32 Test = (0x1FFFFFE<<1);
	Test = 0x2000000-0x4;

	int32 i32Status = NucleonWriteFlash( Test, &u32Location, sizeof(u32Location) );
	if ( CS_SUCCEEDED(i32Status) )
	{
		uInt32	u32Val = 0xFFFF;
		i32Status = IoctlReadFlash( Test, &u32Val, sizeof(u32Val) );
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
