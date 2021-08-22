//	CsDeereHw.cpp
///	Implementation of CsDeereDevice class
//
//	This file contains all member functions that access to addon hardware including
//	Frontend set up and calibration
//
//


#include "StdAfx.h"
#include "CsDeere.h"
#include "CsDeereFlash.h"

const FRONTEND_DEERE_INFO	CsDeereDevice::c_FrontEnd[] = 
{  	//		        Regsetup	AdcDrv   CalAtten
	CS_GAIN_10_V,	0x8001,		0x00,     0,
	CS_GAIN_4_V,	0x8001,		0x02,     0,
	CS_GAIN_2_V,	0x8001,		0x01,     0,
	CS_GAIN_1_V,	0x8003,		0x00,    10,
	CS_GAIN_400_MV,	0x8003,		0x02,	 27,
	CS_GAIN_200_MV,	0x8003,		0x01,    39, 
};

#define c_JD_FE_INFO_SIZE   (sizeof(CsDeereDevice::c_FrontEnd)/sizeof(FRONTEND_DEERE_INFO))

const uInt16	CsDeereDevice::c_u16CalAttenLowFreq[] = 
{  	
// CalAtten
	0,		//CS_GAIN_10_V,
	0,		//CS_GAIN_4_V,
	3,		//CS_GAIN_2_V,
	15,		//CS_GAIN_1_V,
	32,		//CS_GAIN_400_MV,
	44,		//CS_GAIN_200_MV,
};


// SampleRate	ClkFreq	Decim	TaoD  TaoS
const DEERE_SAMPLERATE_INFO	CsDeereDevice::DeereSRI[] = 
{
	2000000000,  500,	    1,	 0,	    0,
	1000000000,  500,	    2,	 0,	    0,
	 500000000,	 500,	    4,	 0,	    0,
	 250000000,	 500,	    8,	 0,	    0,
	 125000000,  500,	   16,	 0,	    0,
	 100000000,  500,	   20,	 1,	    1,
	  50000000,  500,	   40,	 1,	    1,
	  25000000,  500,	   80,	 1,	    1,
	  10000000,  500,	  200,	 1,	    1,
	   5000000,  500,	  400,	 1,	    1,
	   2500000,	 500,	  800,	 1,	    1,
	   1000000,	 500,	 2000,	 1,	    1,
	    500000,	 500,	 4000,	 1,	    1,
	    250000,	 500,	 8000,	 1,	    1,
	    100000,	 500,   20000,	 1,	    1,
	     50000,	 500,	40000,	 1,	    1,
	     25000,	 500,	80000,	 1,	    1,
		 10000,	 500,  200000,	 1,	    1,
};

const int32 CsDeereDevice::m_i32ExtClkTrigAddrOffsetTable[2] = 
{ 
//	OffsetDual		OffsetSingle
    0,					0,				
};


//-----------------------------------------------------------------------------
//----- SendClockSetting
//----- ADD-ON REGISTER
//-----------------------------------------------------------------------------
int32	CsDeereDevice::SendClockSetting(uInt32 u32Mode, LONGLONG llSampleRate, uInt32 u32ExtClk, BOOLEAN bRefClk )
{
	int32	i32Ret = CS_SUCCESS;
	BOOLEAN	bInit = ( DEERE_DEFAULT_SAMPLE_RATE == llSampleRate );

	PDEERE_SAMPLERATE_INFO	 pSrInfo;
	uInt16	u16ClkSelect = m_pCardState->bV10 ? 0 : DEERE_CLKR1_SYNTH_FUNC;
	DEERE_SAMPLERATE_INFO	SrInfo = {0};


	UNREFERENCED_PARAMETER(u32Mode);
	if ( 0 != u32ExtClk )
	{
		if( m_pCardState->u8NumOfAdc > 1 )
			return CS_NO_EXT_CLK;
		// Validation of minimum ext clock frequency
		if ( llSampleRate < DEERE_MIN_EXT_CLK || llSampleRate > m_SrInfoTable[0].llSampleRate )
			return CS_INVALID_SAMPLE_RATE;
		
		SrInfo.u32Decimation = m_SrInfoTable[0].u32Decimation;
		SrInfo.llSampleRate = llSampleRate;
		SrInfo.u32ClockFrequencyMhz = uInt32(llSampleRate / 1000000);;
				
		pSrInfo = &SrInfo;
	}
	else
	{
		if ( bInit )
			llSampleRate = m_SrInfoTable[0].llSampleRate;
		
		if ( !FindSrInfo( llSampleRate, &pSrInfo ) )
			return CS_INVALID_SAMPLE_RATE;
		
		m_pCardState->u32SampleClock = pSrInfo->u32ClockFrequencyMhz * 1000;

		u16ClkSelect |= m_pCardState->bV10 ? DEERE_CLKR1_SYNTH_FUNC : DEERE_CLKR1_PLL_SRC;

		if(!bRefClk )
			u16ClkSelect |= DEERE_CLKR1_REFECI;
	}

	uInt16	u16MsCtrl	= DEERE_MSDAC_RESET | DEERE_MSDAC_PWRDOWN | DEERE_MSTRANSCEIVEROUT_DISABLE;
	uInt16 u16DecimationLow = uInt16(pSrInfo->u32Decimation & 0x7fff);
	uInt16 u16DecimationHigh = uInt16((pSrInfo->u32Decimation>>15) & 0x7fff);

	if ( ermStandAlone == m_pCardState->ermRoleMode )
		u16ClkSelect |= DEERE_CLKR1_LOCALCLK;
	else
	{
		u16MsCtrl &= ~DEERE_MSTRANSCEIVEROUT_DISABLE;
		u16MsCtrl |= DEERE_MSCLKBUFFER_ENABLE;
	}

	if( m_pCardState->AddOnReg.u16MsCtrl != u16MsCtrl )
	{
		i32Ret = WriteRegisterThruNios(u16MsCtrl, DEERE_MS_CTRL);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		m_pCardState->AddOnReg.u16MsCtrl	= u16MsCtrl;
	}
	
	bool	bForceSetClkOut = false;

	//Out0 is disabled, but stil set deviders to 1 for it OUT2 is the same devider as out 1
	if(  m_pCardState->AddOnReg.u32ClkFreq != pSrInfo->u32ClockFrequencyMhz  || 
		 m_pCardState->AddOnReg.u32Decimation != pSrInfo->u32Decimation ||
		 m_pCardState->AddOnReg.u16ClkSelect != u16ClkSelect)
	{
		if ( m_pCardState->AddOnReg.u32ClkFreq != pSrInfo->u32ClockFrequencyMhz ||
		    m_pCardState->AddOnReg.u16ClkSelect != u16ClkSelect )
		{
			i32Ret = WriteRegisterThruNios(u16ClkSelect, DEERE_CLOCK_CTRL);
			if( CS_FAILED(i32Ret) )	return i32Ret;
			
			// Without PLL reset we'll have PDA failure on slave cards in M/S system
			i32Ret = ResetPLL();
			if( CS_FAILED(i32Ret) )	return i32Ret;

			m_pCardState->bDpaNeeded			= true;
			m_pCardState->bMasterSlaveCalib		= true;
			m_pCardState->bExtTrigCalib			= true;

			for( int i = 0; i < m_pCsBoard->NbAnalogChannel; i++ )
				m_pCardState->bCalNeeded[i] = true;

			m_pCardState->AddOnReg.u32ClkFreq    = pSrInfo->u32ClockFrequencyMhz;
			m_pCardState->AddOnReg.u16ClkSelect = u16ClkSelect;
		}

		if ( m_pCardState->AddOnReg.u32Decimation != pSrInfo->u32Decimation ) 
		{
			((CsDeereDevice *)m_MasterIndependent)->m_bMsDecimationReset = true;

			i32Ret = WriteRegisterThruNios(u16DecimationLow, DEERE_DEC_FACTOR_LOW_WR_CMD);
			if( CS_FAILED(i32Ret) )	return i32Ret;
			
			i32Ret = WriteRegisterThruNios(u16DecimationHigh, DEERE_DEC_FACTOR_HIGH_WR_CMD);
			if( CS_FAILED(i32Ret) )	return i32Ret;

			m_pCardState->AddOnReg.u32Decimation = pSrInfo->u32Decimation;
		}
	
		m_pCardState->u32AdcClock = (uInt32) pSrInfo->u32ClockFrequencyMhz * 1000000;
		m_pCardState->AcqConfig.i64SampleRate = pSrInfo->llSampleRate;
		m_pCardState->AcqConfig.u32ExtClk = u32ExtClk;
	}


	if ( 0 != u32ExtClk && !IsFpgaPllLocked() )
		return CS_EXT_CLK_NOT_PRESENT;


	i32Ret = SetClockOut(m_pCardState->eclkoClockOut, bForceSetClkOut );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

int32	CsDeereDevice::SendCaptureModeSetting(uInt32 u32Mode, uInt16 u16ChannelSingle )
{
	uInt16	u16AdcCtrl = 0;

	if ( 1 == m_pCardState->u8NumOfAdc )
		if( 2 == m_pCsBoard->NbAnalogChannel )
			u16AdcCtrl = DEERE_1_ADC_BY_2_CH;		// 1 ADC 2 Ch
		else
			u16AdcCtrl = DEERE_1_ADC_BY_1_CH;		// 1 ADC 1 Ch
	else if ( 2 == m_pCardState->u8NumOfAdc )
		if( 2 == m_pCsBoard->NbAnalogChannel )
			u16AdcCtrl = DEERE_2_ADC_BY_2_CH;		// 2 ADC 2 Ch
		else
			u16AdcCtrl = DEERE_2_ADC_BY_1_CH;		// 2 ADC 1 Ch
	else 
		u16AdcCtrl = DEERE_4_ADC_BY_1_CH;			// 4 ADC

	u16AdcCtrl <<= 2;
		
	if( 0 != (u32Mode & CS_MODE_SINGLE) )
	{
		if ( CS_CHAN_1 == u16ChannelSingle )
			u16AdcCtrl |= DEERE_CHAN_A_ONLY;
		else
			u16AdcCtrl |= DEERE_CHAN_B_ONLY;
	}
	else
		u16AdcCtrl |= DEERE_DUAL_CHAN;
		
	if ( m_pCardState->AcqConfig.u32Mode != u32Mode ) 
		((CsDeereDevice *)m_MasterIndependent)->m_bMsDecimationReset = true;

	int32 i32Ret = WriteRegisterThruNios(u16AdcCtrl, DEERE_DECIMATION_CTRL);

	m_pCardState->AcqConfig.u32Mode	= u32Mode;
	m_pCardState->AddOnReg.u16AdcCtrl = u16AdcCtrl;
	
	return i32Ret;
}


//-----------------------------------------------------------------------------
//-----	SendCalibModeSetting
//----- ADD-ON REGISTER
//-----------------------------------------------------------------------------
int32 CsDeereDevice::SendCalibModeSetting(uInt16 u16HwChannel, eCalMode eMode, uInt32 u32WaitTime )
{
	if( (0 == u16HwChannel) || (u16HwChannel > DEERE_CHANNELS)  )
		return CS_INVALID_CHANNEL;
	
	int32 i32Ret = CS_SUCCESS;
	const uInt16 u16ChanZeroBased = u16HwChannel - 1;
	uInt32 u32FrontEndSetup = m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] & ~(DEERE_SET_USER_INPUT|DEERE_SET_DC_COUPLING);
	uInt16 u16CalibPulse = DEERE_PULSE_DISABLED;
	uInt16 u16CalAc = m_pCardState->AddOnReg.u16CalAc & ~DEERE_CALAC_POWER;
	uInt16 u16ExtTrigIn = m_pCardState->AddOnReg.u16ExtTrigIn | DEERE_ETI_NOT_PULSE | DEERE_ETI_NORMAL;

	//only one channel may be in calibration
	if( eMode != ecmCalModeNormal && eMode != ecmCalModeAdc && eMode != ecmCalModeDual)
	{
		if(  0 == (m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased^1] & DEERE_SET_USER_INPUT) )
			return CS_INVALID_CHANNEL;
	}

	switch ( eMode )
	{
	case ecmCalModeNormal:
		if( CS_COUPLING_DC == m_pCardState->ChannelParams[u16ChanZeroBased].u32Term )
			u32FrontEndSetup |= DEERE_SET_DC_COUPLING;
		u32FrontEndSetup |= DEERE_SET_USER_INPUT;
		break;
	
	case ecmCalModePci:
	case ecmCalModeAc:
		u16CalAc |= DEERE_CALAC_POWER;
		break;

	case ecmCalModeMs:
		u16ExtTrigIn &= ~DEERE_ETI_NOT_PULSE;
		u16CalibPulse &= ~DEERE_MS_PULSE_DISABLED;
		break;
	case ecmCalModeExtTrig:
	case ecmCalModeExtTrigChan:
	case ecmCalModeAdc:
	case ecmCalModeDual:
		u16ExtTrigIn &= ~DEERE_ETI_NOT_PULSE;
		u16CalibPulse &= ~DEERE_OB_PULSE_DISABLED;
		break;

	default:
	case ecmCalModeDc:
		u32FrontEndSetup |= DEERE_SET_DC_COUPLING;
		break;
	}

	bool bChange = false;
	if ( m_pCardState->AddOnReg.u16ExtTrigIn != u16ExtTrigIn )
	{
		i32Ret = WriteRegisterThruNios( u16ExtTrigIn, DEERE_EXTTRIG_INPUT);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		m_pCardState->AddOnReg.u16ExtTrigIn = u16ExtTrigIn;
		bChange = true;
	}

	if ( m_pCardState->AddOnReg.u16CalAc != u16CalAc )
	{
		ConfigureAlarmSource();

		uInt32 u32PLL9511_OUT2 = 0x0A;
		bool bProgram4360 = false;
		if( 0 != (u16CalAc & DEERE_CALAC_POWER ) )
		{
			bProgram4360 = true;
			if( m_pCardState->bV10 )
				u32PLL9511_OUT2 = 0x08; //later revisions use TCXO directly so we disable 9511 output
		}
		
		i32Ret = WriteRegisterThruNios( u32PLL9511_OUT2, DEERE_9511_WR_CMD | DEERE_9511_OUT2);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = WriteRegisterThruNios( u16CalAc, DEERE_SET_CALAC);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		if( bProgram4360 )
		{
			i32Ret = Configure4302(u16HwChannel);
			if( CS_FAILED(i32Ret) )	return i32Ret;
			i32Ret = Configure4360();
			if( CS_FAILED(i32Ret) )	return i32Ret;
		}
		m_pCardState->AddOnReg.u16CalAc = u16CalAc;
		bChange = true;
	}
	
	if ( m_pCardState->AddOnReg.u16CalibPulse != u16CalibPulse )
	{
		i32Ret = WriteRegisterThruNios(u16CalibPulse | DEERE_PULSE_DISABLED, DEERE_PULSE_CALIB);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		m_pCardState->AddOnReg.u16CalibPulse = u16CalibPulse;
		bChange = true;
	}
	if( u32FrontEndSetup != m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] )
	{
		uInt32	u32Cmd = (CS_CHAN_1 != u16HwChannel) ? DEERE_SET_CHAN2 : DEERE_SET_CHAN1;
		i32Ret = WriteRegisterThruNios(u32FrontEndSetup, u32Cmd );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] = u32FrontEndSetup;
		bChange = true;
	}
	m_CalibMode = eMode;
	if( bChange )
	{
		ConfigureAlarmSource(ecmCalModeAc == eMode);
		TraceCalib(TRACE_CAL_INPUT, u16HwChannel,u16CalibPulse,int32(eMode));
	}

	// Waiting for the channel switched between Calib and Input source
	// In the case this function has to be call many times, optimization can be done
	// by setting the waiting time only on the last call.
	BBtiming( u32WaitTime );
	if( CS_SUCCEEDED(i32Ret) )
	{
		GetAlarmStatus();
		if( ( ecmCalModeAc == eMode ) && (0 != (m_u16AlarmSource & m_u16AlarmStatus & DEERE_ALARM_CAL_PLL_UNLOCK ) ) )
		{
			char szError[128];
			sprintf_s(szError, sizeof(szError), " ! # ! ! AC Cal source (card %d) lost lock.\n", m_pCardState->u16CardNumber);
			::OutputDebugString(szError);
			i32Ret = CS_FALSE;
		}
	}
	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDeereDevice::SendChannelSetting(uInt16 u16HwChannel, uInt32 u32InputRange, uInt32 u32Term, uInt32 u32Impendance, int32 i32Position, uInt32 u32Filter, int32 i32ForceCalib)
{
	int32 i32Ret = CS_SUCCESS;
	bool	bCouplingACtoDC	= false;

	if( (CS_CHAN_1 != u16HwChannel) && ((CS_CHAN_2 != u16HwChannel))  )
		return CS_INVALID_CHANNEL;
	if (CS_REAL_IMP_50_OHM != u32Impendance )
		return CS_INVALID_IMPEDANCE;

	if (( CS_COUPLING_DC != (u32Term & CS_COUPLING_MASK) ) && ( CS_COUPLING_AC != (u32Term & CS_COUPLING_MASK) ) )
		return CS_INVALID_COUPLING;

	uInt16 u16ChanZeroBased = u16HwChannel-1;
	uInt32 u32FrontIndex = 0;

	i32Ret = FindFeIndex( u32InputRange, &u32FrontIndex );
	if( CS_FAILED(i32Ret) ) return i32Ret;
	
	uInt32 u32FrontEndSetup = m_FeInfoTable[u32FrontIndex].u32RegSetup;
	uInt16 u16AdcDrvSetup	= m_pCardState->AddOnReg.u16AdcDrv;

	if (CS_CHAN_1 == u16HwChannel)
	{
		u16AdcDrvSetup &= ~0xFF;
		u16AdcDrvSetup |= m_FeInfoTable[u32FrontIndex].u16AdcDrv;
	}
	else
	{
		u16AdcDrvSetup &=  ~0xFF00;
		u16AdcDrvSetup |= (m_FeInfoTable[u32FrontIndex].u16AdcDrv << 8);
	}
	
	if ( 1 == m_pCsBoard->NbAnalogChannel ) 
	{
		u16AdcDrvSetup &=  ~0xFF00;
		u16AdcDrvSetup |= u16AdcDrvSetup << 8;
	}

	if ( CS_COUPLING_DC == (u32Term & CS_COUPLING_MASK) )
	{
		u32FrontEndSetup |= DEERE_SET_DC_COUPLING;
		// Check if the previous setting was AC coupling then we need a delay waiting for relay stable
		if ( CS_COUPLING_AC == (m_pCardState->ChannelParams[u16ChanZeroBased].u32Term & CS_COUPLING_MASK) )
			bCouplingACtoDC = true;
	}

	if ( 0 != u32Filter && !m_bDebugCalibSrc  )
		u32FrontEndSetup |= DEERE_SET_CHAN_FILTER;
	else 
		m_pCardState->ChannelParams[u16ChanZeroBased].u32Filter	= u32Filter;

	int32 i32MaxPositionOffset_mV =  int32(c_u32MaxPositionOffset_mV);
	if( u32InputRange >= CS_GAIN_2_V )
		i32MaxPositionOffset_mV *= 10;

	if( i32Position >  i32MaxPositionOffset_mV || 
		i32Position < -i32MaxPositionOffset_mV )
	{
		return CS_INVALID_POSITION;
	}

	if ( 0 == m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] )
		m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] |= DEERE_SET_USER_INPUT;

	//preserve calibration bit
	u32FrontEndSetup &= ~DEERE_SET_USER_INPUT;
	u32FrontEndSetup |= (m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] & DEERE_SET_USER_INPUT);
		
	if( i32ForceCalib > 0 )
	{
		m_pCardState->bCalNeeded[u16ChanZeroBased] = true;
		m_pCardState->bDpaNeeded = true;
	}
	
	if ( m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] != u32FrontEndSetup )
	{
		uInt32 u32RegCouplingMask = (uInt32) ~DEERE_SET_DC_COUPLING;
		m_pCardState->bCalNeeded[u16ChanZeroBased] |= (m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] & u32RegCouplingMask) != (u32FrontEndSetup & u32RegCouplingMask);

		uInt32	u32Cmd = (CS_CHAN_1 != u16HwChannel) ? DEERE_SET_CHAN2 : DEERE_SET_CHAN1;
		i32Ret = WriteRegisterThruNios(u32FrontEndSetup, u32Cmd );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		// Waiting for relay stable on coupling DC
		if ( bCouplingACtoDC )
			BBtiming(100);	// 10ms

		m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] = u32FrontEndSetup;
	}

	if ( u16AdcDrvSetup != m_pCardState->AddOnReg.u16AdcDrv )
	{
		m_pCardState->bCalNeeded[u16ChanZeroBased] = true;
		i32Ret = WriteRegisterThruNios( u16AdcDrvSetup, DEERE_SET_ADCDRV );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		m_pCardState->AddOnReg.u16AdcDrv = u16AdcDrvSetup;
	}
	
	m_pCardState->ChannelParams[u16ChanZeroBased].u32Term		= u32Term & CS_COUPLING_MASK;
	m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance	= CS_REAL_IMP_50_OHM;
	m_pCardState->ChannelParams[u16ChanZeroBased].u32Filter		= u32Filter;
	m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange = u32InputRange;
	m_pCardState->AddOnReg.u32FeIndex[u16ChanZeroBased]			= u32FrontIndex;

	m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = i32Position;

	return i32Ret;
}




//-----------------------------------------------------------------------------
//----- SendTriggerEngineSetting
//----- ADD-ON REGISTER
//-----------------------------------------------------------------------------
int32 CsDeereDevice::SendTriggerEngineSetting(int32	i32SourceA1,
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
											int32   i32LevelB2 ,
											int16	i16AdcSelect )
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

	// Remember the card that has trigger engine enabled
	if (  0 < i32SourceA1  || 0 < i32SourceB1 || 0 < i32SourceA2 || 0 < i32SourceB2  )
		m_MasterIndependent->m_pVirtualMaster = this;
	else
		m_MasterIndependent->m_pVirtualMaster = m_MasterIndependent;

	switch ( i16AdcSelect )
	{
		default:		// Trigger from all Adcs
		{
			if ( 0 < i32SourceA1  )
			{
				u16Data |= DEERE_SET_DIG_TRIG_A1_ENABLE;
				
				if ( CS_TRIG_SOURCE_CHAN_2 == i32SourceA1 )
					u16Data |= DEERE_SET_DIG_TRIG_A1_CHAN_SEL;

				if ( u32ConditionA1 == CS_TRIG_COND_POS_SLOPE )
					u16Data |= DEERE_SET_DIG_TRIG_A1_POS_SLOPE;
			}
			if ( 0 < i32SourceB1  )
			{
				u16Data |= DEERE_SET_DIG_TRIG_B1_ENABLE;
				
				if ( CS_TRIG_SOURCE_CHAN_1 == i32SourceB1 )
					u16Data |= DEERE_SET_DIG_TRIG_B1_CHAN_SEL;

				if ( u32ConditionB1 == CS_TRIG_COND_POS_SLOPE )
					u16Data |= DEERE_SET_DIG_TRIG_B1_POS_SLOPE;
			}
			if ( 0 < i32SourceA2  )
			{
				u16Data |= DEERE_SET_DIG_TRIG_A2_ENABLE;
				
				if ( CS_TRIG_SOURCE_CHAN_2 == i32SourceA2 )
					u16Data |= DEERE_SET_DIG_TRIG_A2_CHAN_SEL;

				if ( u32ConditionA2 == CS_TRIG_COND_POS_SLOPE )
					u16Data |= DEERE_SET_DIG_TRIG_A2_POS_SLOPE;
			}
			if ( 0 < i32SourceB2  )
			{
				u16Data |= DEERE_SET_DIG_TRIG_B2_ENABLE;
				
				if ( CS_TRIG_SOURCE_CHAN_1 == i32SourceB2 )
					u16Data |= DEERE_SET_DIG_TRIG_B2_CHAN_SEL;

				if ( u32ConditionB2 == CS_TRIG_COND_POS_SLOPE )
					u16Data |= DEERE_SET_DIG_TRIG_B2_POS_SLOPE;
			}
		}
		break;

		case 0:			// Trigger from Adc 0
			u16Data |= DEERE_SET_DIG_TRIG_TRIGEN3_EN;
			break;
		case 1:			// Trigger from Adc 90
			u16Data |= DEERE_SET_DIG_TRIG_TRIGEN3_EN;
			u16Data |= DEERE_SET_DIG_TRIG_A1_CHAN_SEL;
			break;
		case 2:			// Trigger from Adc 180
			u16Data |= DEERE_SET_DIG_TRIG_TRIGEN3_EN;
			u16Data |= DEERE_SET_DIG_TRIG_B1_CHAN_SEL;
			break;
		case 3:			// Trigger from Adc 270
			u16Data |= DEERE_SET_DIG_TRIG_TRIGEN3_EN;
			u16Data |= DEERE_SET_DIG_TRIG_A1_CHAN_SEL;
			u16Data |= DEERE_SET_DIG_TRIG_B1_CHAN_SEL;
			break;
	}


	int32 i32Ret = CS_SUCCESS;

	i32Ret = WriteRegisterThruNios(u16Data, DEERE_DTE_SETTINGS);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret =  SendDigitalTriggerLevel( INDEX_TRIGENGINE_A1, i32LevelA1 );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret =  SendDigitalTriggerLevel( INDEX_TRIGENGINE_B1, i32LevelB1 );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	i32Ret =  SendDigitalTriggerLevel( INDEX_TRIGENGINE_A2, i32LevelA2 );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret =  SendDigitalTriggerLevel( INDEX_TRIGENGINE_B2, i32LevelB2 );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	for ( uInt16 i = INDEX_TRIGENGINE_A1 ; i < m_pCsBoard->NbTriggerMachine; i ++ )
	{
		i32Ret =  SendDigitalTriggerSensitivity( i, m_u8TrigSensitivity );
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}

	m_pCardState->TriggerParams[INDEX_TRIGENGINE_A1].i32Source	= i32SourceA1;
	m_pCardState->TriggerParams[INDEX_TRIGENGINE_A1].i32Level	= i32LevelA1;
	m_pCardState->TriggerParams[INDEX_TRIGENGINE_A1].u32Condition = u32ConditionA1;

	m_pCardState->TriggerParams[INDEX_TRIGENGINE_B1].i32Source	= i32SourceB1;
	m_pCardState->TriggerParams[INDEX_TRIGENGINE_B1].i32Level	= i32LevelB1;
	m_pCardState->TriggerParams[INDEX_TRIGENGINE_B1].u32Condition = u32ConditionB1;

	m_pCardState->TriggerParams[INDEX_TRIGENGINE_A2].i32Source	= i32SourceA2;
	m_pCardState->TriggerParams[INDEX_TRIGENGINE_A2].i32Level	= i32LevelA2;
	m_pCardState->TriggerParams[INDEX_TRIGENGINE_A2].u32Condition = u32ConditionA2;

	m_pCardState->TriggerParams[INDEX_TRIGENGINE_B2].i32Source	= i32SourceB2;
	m_pCardState->TriggerParams[INDEX_TRIGENGINE_B2].i32Level	= i32LevelB2;
	m_pCardState->TriggerParams[INDEX_TRIGENGINE_B2].u32Condition = u32ConditionB2;

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDeereDevice::SendExtTriggerSetting( bool  bExtTrigEnabled,
										   int32  i32ExtLevel,
										   uInt32 u32ExtCondition,
										   uInt32 u32ExtTriggerRange, 
										   uInt32 u32ExtTriggerImped, 
										   uInt32 u32ExtCoupling )
{
	uInt16	u16ExtTrigIn = m_pCardState->AddOnReg.u16ExtTrigIn & (DEERE_ETI_NORMAL | DEERE_ETI_NOT_PULSE);
	uInt16	u16ExtTrigSet = 0;
	int32	i32Ret = CS_SUCCESS;
	
	if ( -1 == u32ExtTriggerRange )
		u32ExtTriggerRange = m_u32DefaultExtTrigRange;

	if ( -1 == u32ExtTriggerImped )
		u32ExtTriggerImped = m_u32DefaultExtTrigImpedance;

	if ( -1 == u32ExtCoupling )
		u32ExtCoupling = m_u32DefaultExtTrigCoupling;

	if  ( ( CS_GAIN_2_V != u32ExtTriggerRange ) && ( CS_GAIN_10_V != u32ExtTriggerRange ) )
		return CS_INVALID_GAIN;

	if  ( (CS_COUPLING_DC != u32ExtCoupling) && (CS_COUPLING_AC != u32ExtCoupling) )
		return CS_INVALID_COUPLING;

	if  ( (CS_REAL_IMP_50_OHM != u32ExtTriggerImped) && (CS_REAL_IMP_1M_OHM != u32ExtTriggerImped) )
		return CS_INVALID_IMPEDANCE;

	if ( bExtTrigEnabled )
	{
		u16ExtTrigSet |= DEERE_SET_EXT_TRIGGER_ENABLED;

		if (  CS_TRIG_COND_POS_SLOPE == u32ExtCondition )
			u16ExtTrigSet |=  DEERE_SET_EXT_TRIGGER_POS_SLOPE;

		if ( CS_GAIN_2_V == u32ExtTriggerRange && !m_pCardState->bV10 )
			u16ExtTrigIn |=  DEERE_ETI_HIGH_GAIN;

		if  ( CS_COUPLING_AC == u32ExtCoupling) 
			u16ExtTrigIn |=  DEERE_ETI_AC;

		if  ( CS_REAL_IMP_50_OHM == u32ExtTriggerImped )
			u16ExtTrigIn |=  DEERE_ETI_50_OHM;

		if ( m_pCardState->AddOnReg.u16ExtTrigIn != u16ExtTrigIn )
		{
			i32Ret = WriteRegisterThruNios( u16ExtTrigIn, DEERE_EXTTRIG_INPUT);
			if( CS_FAILED(i32Ret) )	return i32Ret;
			m_pCardState->AddOnReg.u16ExtTrigIn = u16ExtTrigIn;
		}
		i32Ret = SendExtTriggerLevel( i32ExtLevel, CS_TRIG_COND_POS_SLOPE == u32ExtCondition );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].i32Source	= -1*m_pCardState->u16CardNumber;
	}
	else
		m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].i32Source	= 0;

	if ( m_pCardState->AddOnReg.u16ExtTrigSet != u16ExtTrigSet )
	{
		i32Ret = WriteRegisterThruNios(u16ExtTrigSet, DEERE_EXT_TRIGGER_SETTING);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = ExtTrigEnableReset();
		if( CS_FAILED(i32Ret) )	return i32Ret;
		m_pCardState->AddOnReg.u16ExtTrigSet = u16ExtTrigSet;
	}
	
	m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].i32Level	= i32ExtLevel;
	m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].u32Condition = u32ExtCondition;
	m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].u32ExtTriggerRange = u32ExtTriggerRange;
	m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].u32ExtCoupling = u32ExtCoupling;
	m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].u32ExtImpedance = u32ExtTriggerImped;

	return CS_SUCCESS;
}



//-----------------------------------------------------------------------------
// Input signal is scaled to � c_u32ExtTrigRange
// Vl = Vref * COUNTl/ DacCount;
// Vo = Vref * COUNTo/ DacCount;
// Vmid = -Vl * (R100/R150) + Vref*((R150+R100)/R150)*(R136/(R136+R134))
// Vll = - Vmid * (R158/R118) - Vo*(R158/R116)
// Vlh = - Vmid * (R160/R120) + Vo*(R124/(R122+R124))*((R120++R160)/R120)
// Vlh-Vll = Vo*( R158/R116 + (R124/(R122+R124))*((R120+R160)/R120) ) + Vmid * (R158/R118 - R160/R120)
// if R158*R120 == R160*R118
// Vlh-Vll = Vo*( R158/R116 + (R124/(R122+R124))*((R120+R160)/R120) )
// Vlh-Vll = c_u32ExtTrigRange * m_u16ExtTrigSensitivity / 1000 (m_u16ExtTrigSensitivity in 0.1% )
// c_u32ExtTrigRange * m_u16ExtTrigSensitivity / 1000 =  Vref * COUNTo/ DacCount * ( R158/R116 + (R124/(R122+R124))*((R120+R160)/R120) )
// COUNTo = DacCount * c_u32ExtTrigRange * m_u16ExtTrigSensitivity / ( 1000 * Vref *( R158/R116 + (R124/(R122+R124))*((R120+R160)/R120) ) )
//   A = R120*R122*R158+R120*R124*R158+R116*R120*R124+R116*R124*R160
//   B = R116*R120*R122+R166*R12**R124
// COUNTo = DacCount * c_u32ExtTrigRange * m_u16ExtTrigSensitivity * B / ( 1000 * Vref * A )
// If Slope is positive 
//      TrigLevel = Vlh
//  TrigLevel = - Vmid * (R160/R120) + Vo*(R124/(R122+R124))*((R120++R160)/R120)
//  Vmid = Vref*COUNTo*(R120++R160)*R124/(DacCount*R160*(R122+R124)) - TrigLevel*R120/R160
// If Slope is negative 
//      TrigLevel = Vll
// TrigLevel = - Vmid * (R158/R118) - Vo*(R158/R116)
// Vmid = - TrigLevel*R118/R158 - Vref*COUNTo*R118/(R116*DacCount)
// Vmid = Vref*R136*(R100+R150)/((R134+R136)*R150) - Vref*COUNTl*R100/(DacDount*R150)
// COUNTl = DacCount*R136*(R100+R150)/((R134+R136)*R100) - DacCount*Vmid*R150/(Vref*R100)
//-----------------------------------------------------------------------------


const LONGLONG CsDeereDevice::c_llR100 =  2000;
const LONGLONG CsDeereDevice::c_llR116 = 10000;
const LONGLONG CsDeereDevice::c_llR118 =  4990;
const LONGLONG CsDeereDevice::c_llR120 =  4990;
const LONGLONG CsDeereDevice::c_llR122 =  4990;
const LONGLONG CsDeereDevice::c_llR124 =   820;
const LONGLONG CsDeereDevice::c_llR134 = 15000;
const LONGLONG CsDeereDevice::c_llR136 =  4990;
const LONGLONG CsDeereDevice::c_llR150 =  2000;
const LONGLONG CsDeereDevice::c_llR158 =  2000;
const LONGLONG CsDeereDevice::c_llR160 =  2000;

const LONGLONG CsDeereDevice::c_llExtTrigRangeScaled = 1000;  //mV
const LONGLONG CsDeereDevice::c_llVref               = 2500;  //mV
const uInt16 CsDeereDevice::c_u16CalDacCount	     = 4096;
const uInt8  CsDeereDevice::c_u8AdcCoarseGainSpan    = 7;
const uInt8  CsDeereDevice::c_u8AdcCalRegSpan	     = 255;
const uInt16 CsDeereDevice::c_u16CalAdcCount	     = 4096;
const uInt16 CsDeereDevice::c_u16DacPhaseLimit       = 600;

int32 CsDeereDevice::SendExtTriggerLevel( int32 i32Level, bool bRising )
{
	ASSERT( c_llR158*c_llR120 == c_llR160*c_llR118 );

	const LONGLONG cllFactor = 0x10000;
	const LONGLONG cllExtSenseScale = 1000;
	const LONGLONG cllA = c_llR120*c_llR158*(c_llR122+c_llR124) + c_llR116*c_llR124*(c_llR120+c_llR160);
	const LONGLONG cllB = c_llR116*c_llR120*(c_llR122+c_llR124);

	LONGLONG llCodeOff = cllB*cllFactor + cllA/2;
	llCodeOff /= cllA;
	llCodeOff *= c_llExtTrigRangeScaled*m_u16ExtTrigSensitivity*c_u16CalDacCount;
	llCodeOff += c_llVref*cllExtSenseScale*cllFactor/2;
	llCodeOff /= c_llVref*cllExtSenseScale*cllFactor;

	if( llCodeOff > (LONGLONG)(c_u16CalDacCount-1) )
		llCodeOff = (LONGLONG)(c_u16CalDacCount-1);
	uInt16 u16CodeOff = uInt16(llCodeOff);

	const LONGLONG cllLevelSpan = 200;
	LONGLONG llMid_factored;
	if( bRising )
	{
		//  Vmid = Vref*COUNTo*(R120++R160)*R124/(DacCount*R160*(R122+R124)) - TrigLevel*R120/R160
		const LONGLONG cllE = c_llR124*(c_llR120+c_llR160)*llCodeOff*c_llVref*cllFactor;
		const LONGLONG cllF = c_llR160*(c_llR122+c_llR124)*c_u16CalDacCount;
		const LONGLONG cllG = i32Level * c_llExtTrigRangeScaled * c_llR120*cllFactor;
		const LONGLONG cllH = cllLevelSpan * c_llR160; 

		llMid_factored = (cllE + cllF/2)/cllF - (cllG+cllH/2)/cllH;
	}
	else
	{
		// Vmid = - TrigLevel*R118/R158 - Vref*COUNTo*R118/(R116*DacCount)
		const LONGLONG cllE = c_llR118*llCodeOff*c_llVref*cllFactor;
		const LONGLONG cllF = c_llR116*c_u16CalDacCount;
		const LONGLONG cllG = i32Level * c_llExtTrigRangeScaled * c_llR118*cllFactor; 
		const LONGLONG cllH = cllLevelSpan * c_llR158; 

		llMid_factored = - (cllG+cllH/2)/cllH - (cllE + cllF/2)/cllF;
	}

	// COUNTl = DacCount*R136*(R100+R150)/((R134+R136)*R100) - DacCount*Vmid*R150/(Vref*R100)
	const LONGLONG cllX = c_u16CalDacCount*c_llR136*(c_llR100+c_llR150);
	const LONGLONG cllY = (c_llR134+c_llR136)*c_llR100;
	const LONGLONG cllS = c_u16CalDacCount*llMid_factored*c_llR150;
	const LONGLONG cllT = c_llVref*c_llR100*cllFactor;
	LONGLONG llCodeLevel = (cllX+cllY/2)/cllY - (cllS+cllT/2)/cllT;

	if( llCodeLevel > (LONGLONG)(c_u16CalDacCount-1) )
		llCodeLevel = (LONGLONG)(c_u16CalDacCount-1);
	if( llCodeLevel < 0 )
		llCodeLevel = 0;
	uInt16 u16CodeLevel = uInt16(llCodeLevel);

	int32 i32Ret = WriteToDac(DEERE_EXTRIG_LEVEL, u16CodeLevel);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	i32Ret = WriteToDac(DEERE_EXTRIG_OFFSET, u16CodeOff);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	TraceCalib( TRACE_EXTTRIG_LEVEL, u16CodeLevel, u16CodeOff, i32Level, bRising?1:-1);
	
	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDeereDevice::SendDigitalTriggerLevel( uInt16  u16TrigEngine, int32 i32Level )
{
	uInt16	u16Data = DEERE_SPI;
	int32	i32Ret = CS_SUCCESS;		
	int32	i32LevelConverted = i32Level;

	//----- Convert from (-100, 99) range to ADC range
	i32LevelConverted = i32Level + 100;
	i32LevelConverted *= DEERE_DIG_TRIGLEVEL_SPAN;
	i32LevelConverted /= 200L;

	if( DEERE_DIG_TRIGLEVEL_SPAN <= i32LevelConverted )
		i32LevelConverted = DEERE_DIG_TRIGLEVEL_SPAN - 1;
	else if( 0 > i32LevelConverted )
		i32LevelConverted = 0;

	int16 i16LevelDAC = (int16)i32LevelConverted;

	u16Data |= i16LevelDAC;

	switch ( u16TrigEngine )
	{
	case INDEX_TRIGENGINE_A1:
		i32Ret = WriteRegisterThruNios(u16Data, DEERE_DLA1_SETTING_ADDR);
		break;
	case INDEX_TRIGENGINE_B1:
		i32Ret = WriteRegisterThruNios(u16Data, DEERE_DLB1_SETTING_ADDR);
		break;
	case INDEX_TRIGENGINE_A2:
		i32Ret = WriteRegisterThruNios(u16Data, DEERE_DLA2_SETTING_ADDR);
		break;
	case INDEX_TRIGENGINE_B2:
		i32Ret = WriteRegisterThruNios(u16Data, DEERE_DLB2_SETTING_ADDR);
		break;

	}

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDeereDevice::SendDigitalTriggerSensitivity( uInt16  u16TrigEngine, uInt8 u8Sensitive )
{
	uInt32 u32Addr = DEERE_DSA1_SETTING_ADDR;
	switch ( u16TrigEngine )
	{
		case INDEX_TRIGENGINE_A1: u32Addr = DEERE_DSA1_SETTING_ADDR; break;
		case INDEX_TRIGENGINE_B1: u32Addr = DEERE_DSB1_SETTING_ADDR; break;
		case INDEX_TRIGENGINE_A2: u32Addr = DEERE_DSA2_SETTING_ADDR; break;
		case INDEX_TRIGENGINE_B2: u32Addr = DEERE_DSB2_SETTING_ADDR; break;	
	}
	return WriteRegisterThruNios(DEERE_SPI | u8Sensitive, u32Addr);
}

//-----------------------------------------------------------------------------
//----- WriteToSelfTestRegister
//----- ADD-ON REGISTER
//-----------------------------------------------------------------------------
int32 CsDeereDevice::WriteToSelfTestRegister( uInt16 u16SelfTestMode )
{
	uInt16	u16Data = 0;

	switch( u16SelfTestMode )
	{
	case CSST_DISABLE:
	case CSST_CAPTURE_DATA:
		u16Data = 0;
		break;
	case CSST_BIST:
		u16Data = DEERE_SELF_BIST;
		break;
	case CSST_COUNTER_DATA:
		u16Data = DEERE_SELF_COUNTER;
		break;
	default:
		return CS_INVALID_SELF_TEST;
	}

	int32 i32Ret = WriteRegisterThruNios(u16Data, DEERE_DATA_SEL_WR_CMD);
	if( CS_FAILED(i32Ret) )
	{
		return i32Ret;
	}

	m_pCardState->AddOnReg.u16DataSelect = u16Data;
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//----- InitRegisters
//-----------------------------------------------------------------------------
int32	CsDeereDevice::InitRegisters(bool bForce)
{
	int32 i32Ret = InitRegisters0(bForce);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	if( !bForce && m_pCardState->bAddOnRegistersInitialized )
		return CS_SUCCESS;

	if( !GetInitStatus()->Info.u32Nios )
		return i32Ret;

	// Disable TrigOut and BusyOut 
	ConfiureTriggerBusyOut();

	//Initialize DAC Info
	for( int i = 0; i < DEERE_CHANNELS; i++ )
		for ( int k = 0; k < DEERE_MAX_RANGES; k++ )
		{
			m_pCardState->DacInfo[i][k]._OffsetNull       = c_u16CalDacCount/2;
			m_pCardState->DacInfo[i][k]._OffsetComp       = c_u16CalDacCount/2;
			m_pCardState->DacInfo[i][k]._UserOffset       = c_u16CalDacCount/2;
			m_pCardState->DacInfo[i][k]._UserOffsetScale  = 0;
			m_pCardState->DacInfo[i][k]._UserOffsetCentre = c_u16CalDacCount/2;
			m_pCardState->DacInfo[i][k]._InputRangeScale  = DEERE_IR_SCALE_FACTOR;
			m_pCardState->DacInfo[i][k]._AdcDrvOffset_LoP = c_u16CalDacCount/2;
			m_pCardState->DacInfo[i][k]._AdcDrvOffset_HiP = c_u16CalDacCount/2;
			m_pCardState->DacInfo[i][k]._ClkPhaseOff_LoP  = c_u16CalDacCount/2;			
			m_pCardState->DacInfo[i][k]._ClkPhaseOff_HiP  = c_u16CalDacCount/2;			
			m_pCardState->DacInfo[i][k]._RefPhaseOff      = c_u16CalDacCount/2;               
			m_pCardState->DacInfo[i][k].bValid = false;
		}

	i32Ret = SendChannelSetting(CS_CHAN_1, m_u32DefaultRange);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendChannelSetting(CS_CHAN_2, m_u32DefaultRange);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	//initialize dacs
	uInt16 u16DacAddr = DEERE_OFFSET_NULL_1;
	for( uInt16 u16 = DEERE_OFFSET_NULL_1; u16 <= DEERE_CLKPHASE_2; u16++ )
	{
		i32Ret = WriteToDac( u16DacAddr + u16, c_u16CalDacCount/2 );
		if( CS_FAILED(i32Ret) )return i32Ret;
	}
	u16DacAddr = DEERE_OFFSET_NULL_2;
	for( uInt16 u16 = DEERE_OFFSET_NULL_1; u16 <= DEERE_CLKPHASE_2; u16++ )
	{
		i32Ret = WriteToDac( u16DacAddr + u16, c_u16CalDacCount/2 );
		if( CS_FAILED(i32Ret) )return i32Ret;
	}
		
	// Enabled All trigger engines
	i32Ret = WriteRegisterThruNios(  DEERE_GLB_TRIG_ENG_ENABLE, DEERE_TRIG_SETTING);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendTriggerEngineSetting( CS_TRIG_SOURCE_DISABLE );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendExtTriggerSetting( false );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = WriteToSelfTestRegister();
	if( CS_FAILED(i32Ret) )	return i32Ret;

	uInt32 u32DefaultMode = 0;	
	if ( 1 == m_pCsBoard->NbAnalogChannel ) 
		u32DefaultMode = CS_MODE_SINGLE;
	else
		u32DefaultMode = CS_MODE_DUAL;

	// These functions have algorithm depend on Channel settings 
	// they should be called after SendChannelSetting()
	DefaultAdcCalibInfoStructure();
	i32Ret = ReadCalibTableFromEeprom();
	if ( CS_FAILED(i32Ret) )return i32Ret;

	i32Ret = SendClockSetting( u32DefaultMode );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendCaptureModeSetting(u32DefaultMode);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendCalibModeSetting(CS_CHAN_1);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendCalibModeSetting(CS_CHAN_2);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	m_pCardState->bAddOnRegistersInitialized = true;

	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsDeereDevice::GetFEIndex(uInt16 u16Channel)
{
	int32 i32(0);

	if( (CS_CHAN_1 != u16Channel) && (CS_CHAN_2 != u16Channel) )
	{
		return i32;
	}

	switch (m_pCardState->ChannelParams[u16Channel].u32InputRange)
	{
		case CS_GAIN_10_V:     i32 = 7; break;
		case CS_GAIN_4_V:      i32 = 6; break;
		case CS_GAIN_2_V:      i32 = 5; break;
		case CS_GAIN_1_V:      i32 = 4; break;
		case CS_GAIN_400_MV:   i32 = 3; break;
		case CS_GAIN_200_MV:   i32 = 2; break;
		case CS_GAIN_100_MV:   i32 = 1; break;
		default:               break;
	}

	return i32;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int32 CsDeereDevice::SendAdcSelfCal(bool bRestore)
{
	if( ! m_pCardState->bDpaNeeded || m_bSkipAdcSelfCal )
		return CS_SUCCESS;

	// Set Flag ADC Align calib required
	m_pCardState->bAdcAlignCalibReq = true;
	// In dual channel cards, Adc reset causes channel 1 and 2 mis alligned
	m_pCardState->bDualChanAligned = false;

	uInt16 u16AdcMask = 0xF; // Skip all by default
	for( uInt8 u8 = 0; u8 < m_pCardState->u8NumOfAdc; u8++ )
	{
		u16AdcMask &= ~( 1 << AdcForChannel(0, u8) );

		if ( m_pCsBoard->NbAnalogChannel > 1 )
			u16AdcMask &= ~( 1 << AdcForChannel(1, u8) );
	}
		
	int32 i32Ret = WriteRegisterThruNios(u16AdcMask, DEERE_ADC_RESET, DEERE_ADC_RESET_TIMEOUT/**m_pCardState->u8NumOfAdc*/);

	TraceCalib(TRACE_CAL_PROGRESS, 0, u16AdcMask, 50);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SetAdcDllRange();
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	uInt16	i = 0;
	for( i = 1; i <= m_pCsBoard->NbAnalogChannel; i++ )
	{
		if ( ! bRestore )
			m_bCoreMatchNeeded[i-1] = true;

		m_bUpdateCoreOrder[i-1] = true;

		i32Ret = SendAcdCorrection( i, m_u8DataCorrection );
		if( CS_FAILED(i32Ret) )break;;

		if( m_bGrayCode )
		{
			uInt8 u8Data = ReadAdcRegDirect( AdcForChannel(i-1, 0), DEERE_ADCREG_OUTPUTMODEA );
			u8Data &= 0xf8;
			u8Data |= 0x02;//Gray code
			for( uInt8 u8 = 0; u8 < m_pCardState->u8NumOfAdc; u8++ )
			{
				i32Ret = WriteToAdcRegDirect( AdcForChannel(i-1, u8), DEERE_ADCREG_OUTPUTMODEA, u8Data);   
				if( CS_FAILED(i32Ret) )	break;
			}
		}

		DefaultAdcCalibInfoRange(i);
		if( CS_FAILED(i32Ret) )break;;
	}

	if ( CS_FAILED(i32Ret) )
		TraceCalib( TRACE_CAL_RESULT, i, 1, i32Ret);

	m_pCardState->bDpaNeeded = false;

	return i32Ret;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool CsDeereDevice::ForceFastCalibrationSettingsAllChannels()
{
	bool bRetVal = true;

	if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL )
	{
		for( uInt16 i = 1; i <= m_pCsBoard->NbAnalogChannel; i ++ )
		{
			if( ! FastFeCalibrationSettings( i, true ) )
				bRetVal = false;
		}
	}
	else
	{
		if( ! FastFeCalibrationSettings( CS_CHAN_1, true ) )
			bRetVal = false;
	}
	return bRetVal;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool CsDeereDevice::FindSrInfo( const LONGLONG llSr, PDEERE_SAMPLERATE_INFO *pSrInfo )
{
	for( unsigned i = 0; i < m_u32SrInfoSize; i++ )
	{
		if ( llSr == m_SrInfoTable[i].llSampleRate )
		{
			*pSrInfo = &m_SrInfoTable[i];
			return TRUE;
		}
	}
	return FALSE;
}



//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void	CsDeereDevice::InitBoardCaps()
{
	uInt32 u32MinDecimation = 1;

	m_u32SrInfoSize = sizeof(DeereSRI) / sizeof(DEERE_SAMPLERATE_INFO);

	if ( 1 == m_pCardState->u8NumOfAdc )
		u32MinDecimation = 4;
	else if ( 2 == m_pCardState->u8NumOfAdc )
		u32MinDecimation = 2;

	// Search for min decimation required 
	uInt32 i = 0;
	for (i = 0; i < m_u32SrInfoSize; i++)
		if ( DeereSRI[i].u32Decimation == u32MinDecimation )
			break;

	m_u32SrInfoSize -= i;
	
	RtlCopyMemory( (void *)m_SrInfoTable, &DeereSRI[i], m_u32SrInfoSize*sizeof(DEERE_SAMPLERATE_INFO) ); 
	if( m_pCardState->b14Bits )
		for( uInt32 u = 0;  u < m_u32SrInfoSize; u++ )
		{
			m_SrInfoTable[u].llSampleRate /= 2;
			m_SrInfoTable[u].u32ClockFrequencyMhz /= 2;
		}

	if( 2 == m_pCsBoard->NbAnalogChannel )
		for( uInt32 u = 0;  u < m_u32SrInfoSize; u++ )
		{
			m_SrInfoTable[u].u32Decimation /= 2;
			if( 0 == m_SrInfoTable[u].u32Decimation )	
				m_SrInfoTable[u].u32Decimation = 1;
		}


	PFRONTEND_DEERE_INFO pFeInfoTable = (PFRONTEND_DEERE_INFO) c_FrontEnd;
	for (uInt32 i = 0; i < c_JD_FE_INFO_SIZE; i++)
	{
		RtlCopyMemory( &m_FeInfoTable[i], &pFeInfoTable[i], sizeof(FRONTEND_DEERE_INFO));
		if ( m_pCardState->bLowCalFreq )
			m_FeInfoTable[i].u16CalAtten = c_u16CalAttenLowFreq[i];
	}

	m_u32FeInfoSize = c_JD_FE_INFO_SIZE;
	m_pCardState->u32Rcount = m_pCardState->bV10 ? DEERE_4360_CLK_1V0 : DEERE_4360_CLK_1V1; // 4360 clock is 250 MHz in 1V0 and 10 MHz in 1V1
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDeereDevice::GeneratePulseCalib()
{
	// Toggle DEERE_PULSE_GENERATE bit together with OE bits ( possible race condition)  
	int32 i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16CalibPulse | DEERE_PULSE_GENERATE, DEERE_PULSE_CALIB);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	return WriteRegisterThruNios(m_pCardState->AddOnReg.u16CalibPulse | DEERE_PULSE_DISABLED, DEERE_PULSE_CALIB);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32	CsDeereDevice::ResetPLL()
{
	m_pCardState->bAdcAlignCalibReq = true;
	m_pCardState->bDualChanAligned = (1 == m_pCsBoard->NbAnalogChannel);
	m_pCardState->AddOnReg.u16ClkOut = 0x8000;
	m_pCardState->AddOnReg.u16CalAc = 0x8000;
	return WriteRegisterThruNios(m_pCardState->b14Bits, DEERE_PLL_RESET);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
uInt8	CsDeereDevice::KenetCoarseGainConvert( uInt8 u8Val, bool bWrite )
{
	//  D3 D2 D1 D0				i16Val
	//	1  1  0  0				6
	//  1  0  0  0				5
	//  0  1  0  0				4
	//  0  0  0  0				3
	//  0  0  0  1				2
	//  0  0  1  0				1
	//  0  0  1  1				0

	if ( bWrite )
	{
		switch(u8Val)
		{
			case 0: return 0x3;  //  0  0  1  1	  -4.2%  
			case 1: return 0x2;  //  0  0  1  0	  -2.8% 
			case 2: return 0x1;  //  0  0  0  1	  -1.4%  
			default:
			case 3: return 0x0;  //  0  0  0  0	   0

			case 4: return 0x4;  //  0  1  0  0	  +1.4%
			case 5: return 0x8;  //  1  0  0  0	  +2.8%
			case 6: return 0xC;  //  1  1  0  0   +4.2%
		}
	}
	else
	{
		u8Val &= 0xf;
		switch(u8Val)
		{   
			
			case 0x3:  return 0;  //  0  0  1  1	-4.2%   
			case 0x2:  return 1;  //  0  0  1  0	-2.8%  
			case 0x1:  return 2;  //  0  0  0  1	-1.4%   
			default:								
			case 0x0:  return 3;  //  0  0  0  0	 0 

			case 0x4:  return 4;  //  0  1  0  0	+1.4% 
			case 0x8:  return 5;  //  1  0  0  0	+2.8% 
			case 0XC:  return 6;  //  1  1  0  0	+4.2% 
		}											
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDeereDevice::BaseBoardConfigReset(uInt8 u8NewImageId)
{

	uInt32	u32Addr = 0;
	switch( u8NewImageId )
		{
		case 2:
			u32Addr = FIELD_OFFSET( DEERE_FLASH_LAYOUT, FlashImage3 );
			break;
		case 1:
			u32Addr = FIELD_OFFSET( DEERE_FLASH_LAYOUT, FlashImage2 );
			break;
		case 0:
		default:
			u8NewImageId = 0;
			u32Addr = FIELD_OFFSET( DEERE_FLASH_LAYOUT, FlashImage1 );
			break;
	}

	//---- write flash location address[7..0]
	WriteFlashRegister(FLASH_ADD_REG_1,u32Addr & 0xFF);
	//---- write flash location address[15..8]
	WriteFlashRegister(FLASH_ADD_REG_2, (u32Addr >> 8) & 0xFF);
	//---- write flash location address[24..16]
	WriteFlashRegister(FLASH_ADD_REG_3, (u32Addr >> 16) & 0xFF);
	//---- send the read command to the PLD
	WriteFlashRegister(COMMAND_REG_PLD,CONFIG);

	//----- release control
	WriteFlashRegister(COMMAND_REG_PLD,0);

	// Wait for Hw stable
	CSTIMER		CsTimeout;
	CsTimeout.Set( KTIME_SECOND(2) );
	while( !CsTimeout.State() );

	// Wait for baseboard ready
	if( !IsNiosInit() )
		return CS_NIOS_FAILED;	// timeout !!!

	m_pCardState->u8ImageId = u8NewImageId;

	// After ConfigReset all settings on the card is lost. By reseting these parameters, we make sure
	// that the next call of CssetAcquisitionCfg, CsSetChannelCfg ... will reconfigure HW even if the
	// configuration has not been changed.
	ResetCachedState();

	return CS_SUCCESS;
}

#if DBG

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#define MSBIST_FPGACLK_MASK				0x7F
#define MSBIST_SAMPLECLK_MASK			0x3F10
#define	MSBIST_SLAVECONNECT_MASK		0x7F
#define	MSBIST_READBIT_MASK				0x8000

int32	CsDeereDevice::Debug_MsSelfTest()
{
	int32			i32Status = CS_SUCCESS;
	CsDeereDevice*  pDevice = m_MasterIndependent;
	uInt32			u16TestData1 = 0;
	uInt32			u16TestData2 = 0;
	char			szText[128];
	BOOLEAN			bError = FALSE;
	uInt16			u16Mask = ~(MSBIST_SLAVECONNECT_MASK | MSBIST_READBIT_MASK) | m_MasterIndependent->m_u8SlaveConnected;


	// Do not need this test on a stand alone card
	if ( ! m_MasterIndependent->m_pNextInTheChain )
		return CS_SUCCESS;

	// Do not need this test if we do not want to
	if ( 0 == (TRACE_MS_BIST_TEST & m_u32DebugCalibTrace) )
		return CS_SUCCESS;

	// Eanble MS_BIST_EN bit on all cards
	while( pDevice )
	{
		pDevice->WriteRegisterThruNios(1, DEERE_MS_BISTCTRL_REG1);
		pDevice = pDevice->m_pNextInTheChain;
	}
	
	pDevice = m_MasterIndependent;
	while( pDevice )
	{
		u16TestData1 = pDevice->ReadRegisterFromNios(0, DEERE_MS_BITSTSTAT_REG2);
		u16TestData2 = pDevice->ReadRegisterFromNios(0, DEERE_MS_BITSTSTAT_REG2);

		if ( 0 == (u16TestData1 & 0x4000) )	
		{
			bError = TRUE;
			t << "Bad Sampling clock (bit 14)";
		}
		if ((u16TestData1 & MSBIST_FPGACLK_MASK) == (u16TestData2 & MSBIST_FPGACLK_MASK))
		{
			bError = TRUE;
			t << "Bad FPGA clock (bits 0-6)";
		}

		if ((u16TestData1 & MSBIST_SAMPLECLK_MASK) == (u16TestData2 & MSBIST_SAMPLECLK_MASK))
		{
			bError = TRUE;
			t << "Bad Sampling clock (bits 7-13)";
		}

		pDevice = pDevice->m_pNextInTheChain;
	}


	//
	// Test direction Master ---> Slave
	//
	
	// Set Buffer direction   A -> B
	m_MasterIndependent->WriteRegisterThruNios(0x7, DEERE_MS_BISTCTRL_REG1);

	uInt16	u16Pattern = 0x2000;
	while (u16Pattern != 0)
	{
		if ( MSBIST_SLAVECONNECT_MASK & u16Pattern )
		{
			// Skip bit positions ->slave card is not connected
			if ( 0 == (u16Pattern & m_MasterIndependent->m_u8SlaveConnected) )
			{
				u16Pattern >>= 1;
				continue;
			}
		}

		// Send a pattern from master card
		m_MasterIndependent->WriteRegisterThruNios(u16Pattern, DEERE_MS_BISTCTRL_REG2);
		pDevice = m_MasterIndependent->m_pNextInTheChain;

		// Check if all slave cards receive the same pattern
		while( pDevice )
		{
			u16TestData1 = pDevice->ReadRegisterFromNios(0, DEERE_MS_BITSTSTAT_REG1);
			u16TestData1 &= u16Mask;
			if ( u16Pattern != u16TestData1 )
			{
				bError = TRUE;
				sprintf(szText, "\nPattern mismatch detected (Master -> Slave). Pat = 0x%08x, CardNum = %d",
								u16Pattern, pDevice->m_ShJdData->u16CardNumber );
				t << szText;
			}

			pDevice = pDevice->m_pNextInTheChain;
		}

		u16Pattern >>= 1;
	}


	//
	// Test direction Slave ---> Master
	//

	// Set Buffer direction   B -> A
	m_MasterIndependent->WriteRegisterThruNios(0x1, DEERE_MS_BISTCTRL_REG1);
	pDevice = m_MasterIndependent->m_pNextInTheChain;
	while( pDevice )
	{
		// Set Buffer direction   A -> B
		pDevice->WriteRegisterThruNios(0x7, DEERE_MS_BISTCTRL_REG1);
		pDevice = pDevice->m_pNextInTheChain;
	}

	pDevice = m_MasterIndependent->m_pNextInTheChain;
	while( pDevice )
	{	
		u16Pattern = 0x2000;
		while (u16Pattern != 0)
		{
			if ( MSBIST_SLAVECONNECT_MASK & u16Pattern )
			{
				// Skip bit positions ->slave card is not connected
				if ( 0 == (u16Pattern & m_MasterIndependent->m_u8SlaveConnected) )
				{
					u16Pattern >>= 1;
					continue;
				}
			}

			pDevice->WriteRegisterThruNios(u16Pattern, DEERE_MS_BISTCTRL_REG2);
			u16TestData1 = m_MasterIndependent->ReadRegisterFromNios(0, DEERE_MS_BITSTSTAT_REG1);
			u16TestData1 &= u16Mask;

			if ( u16Pattern != u16TestData1 )
			{
				bError = TRUE;
				sprintf(szText, "Pattern mismatch detected (Slave -> Master). Pat = 0x%08x, CardNum = %d",
								u16Pattern, pDevice->m_ShJdData->u16CardNumber );
				t << szText;
			}
			u16Pattern >>= 1;
		}

		pDevice = pDevice->m_pNextInTheChain;
	}


	// Disable MS_BIST_EN bit on all cards
	pDevice = m_MasterIndependent;
	while( pDevice )
	{
		pDevice->WriteRegisterThruNios(0, DEERE_MS_BISTCTRL_REG1);
		pDevice = pDevice->m_pNextInTheChain;
	}

	if ( ! bError )
		t << "Master/Slave BIST test success";

	return i32Status;
}

#endif


int32 CsDeereDevice::Configure4360()
{
	if( 0 == m_pCardState->u32Ncount || 0 == m_pCardState->u32Rcount )
		return CS_SUCCESS;

	int32 i32Ret = CS_SUCCESS;

	const uInt32 cu32RCLR = 0x10000; //LPD high
	uInt32 u32RData = cu32RCLR | m_pCardState->u32Rcount;
	i32Ret = WriteRegisterThruNios( u32RData, DEERE_4360_WR_CMD | DEERE_4360_RCLR);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	uInt32 u32CData = 0x3FF49;
	i32Ret = WriteRegisterThruNios( u32CData, DEERE_4360_WR_CMD | DEERE_4360_CCLR);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	BBtiming(150); //15 ms

	const uInt32 cu32NCLR = 0x0;
	uInt32 u32NData = cu32NCLR | (m_pCardState->u32Ncount<<6);
	i32Ret = WriteRegisterThruNios( u32NData, DEERE_4360_WR_CMD | DEERE_4360_NCLR);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	m_u32CalFreq = m_pCardState->bV10 ? DEERE_4360_CLK_1V0 : DEERE_4360_CLK_1V1; 
	m_u32CalFreq = uInt32((LONGLONG(m_u32CalFreq) * m_pCardState->u32Ncount * 1000000)/ m_pCardState->u32Rcount); 

	TraceCalib( TRACE_CONF_4360, u32RData, u32CData, (int32)u32NData );
	
	return i32Ret;
}

int32 CsDeereDevice::Configure4302()
{
	int32 i32Ret = WriteRegisterThruNios( m_u16Atten4302, DEERE_4302_WR_CMD );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	BBtiming(150); //15 ms
	return i32Ret;
}

int32 CsDeereDevice::Configure4302(uInt16 u16HwChannel)
{
	uInt32 u32Index = 0;
	int32 i32Ret = FindFeIndex( m_pCardState->ChannelParams[u16HwChannel-1].u32InputRange, &u32Index );
	if( CS_FAILED(i32Ret) ) return i32Ret;

	m_u16Atten4302 = m_FeInfoTable[u32Index].u16CalAtten;
	return Configure4302();
}

int32 CsDeereDevice::SendAcdCorrection(uInt16 u16HwChannel, uInt8 u8Cor)
{
	uInt16 u16Data = u8Cor;
	u16Data <<= 7;
	u16Data |= u8Cor;  //we are sending the same correction value for both adc, in case of one adc per channel second one will be moot
	uInt16 u16Adc = AdcForChannel(u16HwChannel-1, 0 );
	uInt32 u32Cmd;
	if( (0 == u16Adc % 2) || 4 == m_pCardState->u8NumOfAdc )
		u32Cmd = DEERE_ADC_0_2_COR;
	else
		u32Cmd = DEERE_ADC_1_3_COR;

	int32 i32Ret = WriteRegisterThruNios( u16Data, u32Cmd );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	if( 4 == m_pCardState->u8NumOfAdc )
		i32Ret = WriteRegisterThruNios( u16Data, DEERE_ADC_1_3_COR );
	
	return i32Ret;
}


uInt16 CsDeereDevice::ReadTriggerStatus()	
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


uInt16 CsDeereDevice::ReadBusyStatus()
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

bool CsDeereDevice::IsOnBoardPllLocked()
{
	uInt32 u32Data = ReadRegisterFromNios(0, DEERE_FPGA_STATUS);
	return 0 == (u32Data & DEERE_STATUS_LOSS_OF_LOCK);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool CsDeereDevice::IsFpgaPllLocked()
{
	// Wait for Fpga PLL locked
	CSTIMER		CsTimeout;
	CsTimeout.Set( KTIME_SECOND(2) );
	uInt32 u32Data = ReadRegisterFromNios(0, DEERE_MS_REG_RD);

	while( 0 == (u32Data & DEERE_MS_CLK_LOCKED) )
	{
		u32Data = ReadRegisterFromNios(0, DEERE_MS_REG_RD);
		if ( CsTimeout.State() )
			break;
	}

	return 0 != (u32Data & DEERE_MS_CLK_LOCKED);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool	CsDeereDevice::IsCalPllLocked()
{
	if( m_u16AlarmSource & m_u16AlarmStatus & DEERE_ALARM_CAL_PLL_UNLOCK )
		return false;
	else
		return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDeereDevice::SetDigitalDelay( int16 i16Offset )
{
	int32			i32Ret = CS_SUCCESS;
	uInt16			u16Mask = 0x3F;		// 7 bits
	int16			i16ConvOffset;

	// Convert from (32, -32) range to (64, 0) range
	i16ConvOffset = i16Offset + (MAX_EXTTRIG_ADJUST_SPAN/2);
	i16ConvOffset = (i16ConvOffset & u16Mask);

	if ( -1 == m_u16DigitalDelay || ( m_u16DigitalDelay != (uInt16) i16ConvOffset ) )
	{
		i32Ret = WriteRegisterThruNios( (uInt16) i16ConvOffset, DEERE_EXTTRIG_ALIGNMENT);
		if (CS_FAILED( i32Ret ))
			return i32Ret;

//		int16 i16CoreAdj = ((uInt16) i16ConvOffset) - m_u16DigitalDelay;

		//t << "$$$$$$   New Core Order .....\n";
//		UpdateCoreOrder(CS_CHAN_1, i16CoreAdj);
//		UpdateCoreOrder(CS_CHAN_2, i16CoreAdj);
		m_u16DigitalDelay = (uInt16) i16ConvOffset;
	}
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDeereDevice::ConfigureAuxIn(bool bExtTrigEnabled)
{
	uInt16 u16Data = 0;

	if( bExtTrigEnabled && eAuxInExtTrigEnable == m_pCardState->AuxIn && 0 != m_pCardState->u16ExtTrigEnCfg )
	{
		bool	bEdge			= 0 == (m_pCardState->u16ExtTrigEnCfg & CS_EXTTRIGEN_LIVE);
		bool	bPolarity		= 0 != (m_pCardState->u16ExtTrigEnCfg & (CS_EXTTRIGEN_POS_LATCH | CS_EXTTRIGEN_POS_LATCH_ONCE));
		bool	bResetOnStart	= 0 != (m_pCardState->u16ExtTrigEnCfg & (CS_EXTTRIGEN_NEG_LATCH | CS_EXTTRIGEN_POS_LATCH));

		u16Data |= DEERE_EXTTRIG_EN_ENABLE;
		u16Data |= bEdge ?  0 : DEERE_EXTTRIG_EN_GATE_MODE;
		u16Data |= bPolarity ?  DEERE_EXTTRIG_EN_POS_SLOPE : 0;
		u16Data |= bResetOnStart ? DEERE_EXTTRIG_EN_RST_START : 0;
	}

	m_pCardState->AddOnReg.u16AuxCfg = u16Data;
	return WriteRegisterThruNios(u16Data, DEERE_EXT_TRIG_EN_CFG);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDeereDevice::ExtTrigEnableReset(void)
{
	WriteRegisterThruNios( m_pCardState->AddOnReg.u16AuxCfg | DEERE_EXTTRIG_EN_FORCE_RST, DEERE_EXT_TRIG_EN_CFG);
	return WriteRegisterThruNios( m_pCardState->AddOnReg.u16AuxCfg , DEERE_EXT_TRIG_EN_CFG);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDeereDevice::ConfiureTriggerBusyOut(bool bTrigOutEn, bool bBusyOutEn)
{
	int32	i32Ret = CS_SUCCESS;
	uInt16	u16Data = m_pCardState->AddOnReg.u16TbOutCfg & ~(DEERE_TRIGOUTENABLED | DEERE_BUSYOUTENABLED);

	// Do not change the default hysteresis value from firmware
	u16Data |= (0x7 <<4);

	if ( bTrigOutEn ) 
		u16Data |= DEERE_TRIGOUTENABLED;

	if ( bBusyOutEn )
		u16Data |= DEERE_BUSYOUTENABLED;

	if ( m_pCardState->AddOnReg.u16TbOutCfg != u16Data )
	{
		m_pCardState->AddOnReg.u16TbOutCfg = u16Data;
		i32Ret = WriteRegisterThruNios(u16Data, DEERE_EXTTRIG_REG);
	}
	
	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

eClkOutMode CsDeereDevice::GetClockOut()
{
	return m_pCardState->eclkoClockOut;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDeereDevice:: ReadTriggerTimeStampEtb( uInt32 StartSegment, int64 *pBuffer, uInt32 u32NumOfSeg )
{
	if ( m_bSquareRec )
	{
		for (uInt32 i = 0; i < u32NumOfSeg; i ++)
		{
			pBuffer[i] = ReadRegisterFromNios( StartSegment, CSPLXBASE_READ_TIMESTAMP0_CMD );
			pBuffer[i] <<= 32;
			pBuffer[i] |= ReadRegisterFromNios( StartSegment++, CSPLXBASE_READ_TIMESTAMP1_CMD );
		}
	}
	else
	{
		for (uInt32 i = 0; i < u32NumOfSeg; i ++)
		{
			pBuffer[i] = ReadRegisterFromNios( StartSegment, CSPLXBASE_READ_TIMESTAMP1_CMD );
			pBuffer[i] <<= 32;
			pBuffer[i] |= ReadRegisterFromNios( StartSegment++, CSPLXBASE_READ_TIMESTAMP0_CMD );
		}
	}

	if ( 0 == (m_pCardState->AcqConfig.u32TimeStampConfig & TIMESTAMP_MCLK) )
	{
		uInt8	u8Shift = 0;
		if ( 0 != ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL ))
			u8Shift = SHIFT_DUAL;		// DUAL: One memory location has 8 samples each of channels
		else
			u8Shift = SHIFT_SINGLE;		// SINGLE: One memory location has 16 samples 

		for (uInt32 i = 0; i < u32NumOfSeg; i ++ )
		{
			uInt8 u8Etb = (uInt8) ((ReadRegisterFromNios(i, CSPLXBASE_READ_TRIG_ADD_CMD) >> 4) & 0xf);
			// Timestamp is the count of memory locations
			// Convert to number of samples
			pBuffer[i] = pBuffer[i] >> 4;
			pBuffer[i] = pBuffer[i]  << u8Shift;
			pBuffer[i] += u8Etb;
		}
	}
	return CS_SUCCESS;
}
