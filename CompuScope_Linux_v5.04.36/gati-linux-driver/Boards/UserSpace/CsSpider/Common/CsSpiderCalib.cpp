//	CsSpiderCalib.cpp
///	Implementation of CsSpiderDev class
//
//	This file contains all member functions that access to addon hardware including
//	Frontend set up and calibration
//
//

#include "StdAfx.h"
#include "CsSpider.h"
#include "CsSpiderCal.h"
#include "CsEventLog.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#endif

#define	 CSPDR_CALIB_TIMEOUT_LONG	KTIME_SECOND(1)			// 1 second timeout
#define	 CSPDR_CALIB_TIMEOUT_SHORT	KTIME_MILISEC(300)		// 300 milisecond timeout

//	VCAL_DC voltage is
//
//	Vcal = V3 - (R918/R921)*(Vb-V2),
//
//	where V2 and V3 are voltages on U70 pin 2 and 3 respectively
//	and Va and Vb are voltages of the corresponding outputs of U76 (all references by 1V1 schematics).
//
// V3 = V2 = Va*R923/(R922+R923)
// Va = Vr * A/M, where Vr is 2.5V reference, A is DAC code of U76, M is 4096
// Vb = Vfr* B/M, where Vfr is flexible reference (DACVARREF_DACDC_VARREF)
// since Vfr = Vr * I/M, where I is input code of U77,
// Vb = Vr * B * I /(M * M)
// let assign k as R923/(R922+R923) and D as R918/R921, then
//
// Vcal(I) = (D+1)*k * Vr * A/M - D * Vr * (B*I)/(M*M), of if K = k*(D+1)
// Vcal(I) = K * Vr * A/M - D * Vr * (B*I)/(M*M)
// Vcal shoul cover all front end span with some margin (let's say 32/31), then
// A = M * (Sw / Vr) * 1/(2*K)
// B = M * (Sw / Vr) * 1/D
//
////////////////////////////////////////////////////////////////////
// Modified board pin 6 of u76 to r922
// then
//
//	Vcal = V3 - (R918/R921)*(Vb-V2),
//  V3 = V2 = Vfr*R923/(R922+R923)
//  Vb = Vfr * B/M
//  let assign k as R923/(R922+R923) and D as R918/R921, then

//  Vcal = Vfr*( k - D * (B/M-k) ) = Vfr * ( K - D*B/M )
//  Max Vcal = Vfr * K   Min Vcal = Vfr * ( K - D )
//  Absolute values of Min amd max Vcal should exceed front end span
//  Vfr * Min(K, D-K) = Fe
//  Vfr = Fe/Min(K, D-K)
//  Vfr = RefCode / M * Vref
//  RefCode = M * Fe/(Vref* Min(K, D-K))


//
// However, due to rounding errors resulting Input code to voltage conversion has a non zero constant term z
// Vcal(I) = K * Vr * A/M - D * Vr * (B*(I+z))/(M*M)
// z = Round(M * Vcal(2048)/Sw).
// All of the above allows to calculate A, B and z for any given range and then use the same input code for the
// given calibration level for all ranges.
// In calibration seven levels are used +/- 124/128 (31/32), +/- 64/128 (1/2), +/- 1/128, 0
//
///////////////
//
// Measurment loop
// Reference voltage of U55 depends on range of calibration voltages , we assume that swing of the calibration signal
// is equal to front end swing.
// For U55:
// V+ = Vin * R1637 / (R980+R1637) + Vref_u55 * R980 / (R980+R1637)
// V- = Vref_u55 * R1639 / (R1638+R1639)
// Designate Alpha = R1639 / (R1638+R1639) and Beta = R1637 / (R980+R1637)
// DeltaV = V+ - V- = Vin*Beta + Vref_u55* ( 1 - Alpha - Beta ), asuuming that Alpha + Beta = 1
// DeltaV = Vin * Beta
// CodeU55 = 2* DeltaV / (Vref_u55*Alpha)*M = Vin/Vref_u55 * Beta/Alpha * M/2, or
// Vin = Vref_u55 * CodeU55/M * 2*Alpha/Beta
// Maximum Code coresponding to maximum cal signal is M/2, so
// MaxVin = Vref_u55 * Alpha/Beta = Sw/2, so
// Vref_u55 = Sw/2 * Beta/Alpha
// On the other hand Vref_u55 = Vref * CodeB_u77 / M * (R978+R97)/R77, therefore if Gamma = (R978+R97)/R77
// CodeB_u77 = Vref_u55/Vref * M = Sw/(2*Vref) * Beta/Alpha * M/Gamma
// Let's further designate M*Beta/(Alpha*Gamma) as F, then
// CodeB_u77 =  Sw/(2*Vref) * F
// For better presision Vref_u55 is measured by U36
// MeasVref_u55 = 2 * (CodeU36/Mu36 )*Vref, where Mu36 is the code range of u36 (2^16)
//
// Note. Vref_u55 * Alpha is stored in  m_u32VarRefLevel_uV
// Therefore
// Vin = 2 * m_u32VarRefLevel_uV * CodeU55/(M * Beta)


const uInt16 CsSpiderDev::c_u16R918 = 4990;
const uInt16 CsSpiderDev::c_u16R921 = 1200;
const uInt16 CsSpiderDev::c_u16R922 = 1200;
const uInt16 CsSpiderDev::c_u16R922_v2 = 1820;
const uInt16 CsSpiderDev::c_u16R923 = 1200;

const uInt16 CsSpiderDev::c_u16R977 =  976;
const uInt16 CsSpiderDev::c_u16R978 = 1000;
const uInt16 CsSpiderDev::c_u16R980 = 1000;
const uInt16 CsSpiderDev::c_u16R1637= 1000;
const uInt16 CsSpiderDev::c_u16R1638= 1000;
const uInt16 CsSpiderDev::c_u16R1639= 1000;

const uInt16 CsSpiderDev::c_u16L6 = 50;

const uInt32  CsSpiderDev::c_u32MaxCalVoltage_uV = 4000000;
const uInt32  CsSpiderDev::c_u32MaxCalVoltageBoosted_uV = 5000000;
const uInt16 CsSpiderDev::c_u16CalLevelSteps = 256;

const uInt16 CsSpiderDev::c_u16Vref_mV = 2500;
const uInt16 CsSpiderDev::c_u16CalDacCount = 4096;
const uInt32 CsSpiderDev::c_u32RefAdcCount = 65536;
const uInt16 CsSpiderDev::c_u16CalAdcCount = 4096;

const uInt32 CsSpiderDev::c_u32ScaleFactor = 10000;
const uInt32 CsSpiderDev::c_u32K_scaled = uInt32((LONGLONG(CsSpiderDev::c_u32ScaleFactor) * CsSpiderDev::c_u16R923 * (CsSpiderDev::c_u16R918 + CsSpiderDev::c_u16R921)) / ((CsSpiderDev::c_u16R922 + CsSpiderDev::c_u16R923) * CsSpiderDev::c_u16R921));
const uInt32 CsSpiderDev::c_u32K_scaled_v2 = uInt32((LONGLONG(CsSpiderDev::c_u32ScaleFactor) * CsSpiderDev::c_u16R923 * (CsSpiderDev::c_u16R918 + CsSpiderDev::c_u16R921)) / ((CsSpiderDev::c_u16R922_v2 + CsSpiderDev::c_u16R923) * CsSpiderDev::c_u16R921));
const uInt32 CsSpiderDev::c_u32D_scaled = CsSpiderDev::c_u32ScaleFactor * CsSpiderDev::c_u16R918 / CsSpiderDev::c_u16R921;
const uInt32 CsSpiderDev::c_u32A_scaled = CsSpiderDev::c_u32ScaleFactor * CsSpiderDev::c_u16R1639 / ( CsSpiderDev::c_u16R1639 + CsSpiderDev::c_u16R1638 );
const uInt32 CsSpiderDev::c_u32B_scaled = CsSpiderDev::c_u32ScaleFactor * CsSpiderDev::c_u16R1637 / ( CsSpiderDev::c_u16R980  + CsSpiderDev::c_u16R1637 );
const uInt32 CsSpiderDev::c_u32C_scaled = CsSpiderDev::c_u32ScaleFactor * (CsSpiderDev::c_u16R978 + CsSpiderDev::c_u16R977) / CsSpiderDev::c_u16R977;

const uInt16 CsSpiderDev::c_u32MarginNom = 32;
const uInt16 CsSpiderDev::c_u32MarginDenomHiRange = 28;
const uInt16 CsSpiderDev::c_u32MarginDenomLoRange = 16;
const uInt16 CsSpiderDev::c_u32MarginDenomHiRange_v2 = 30;
const uInt16 CsSpiderDev::c_u32MarginDenomLoRange_v2 = 24;

const uInt16 CsSpiderDev::c_u32MarginLoRangeBoundary = 800;


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsSpiderDev::CalibrateAllChannels()
{
	int32 i32Ret = CS_SUCCESS;

#ifdef	SPIDER_v12_TEST
	uInt16	u16HwChan;
	for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i++ )
	{
		u16HwChan = ConvertToHwChannel(i);
		if ( m_pCardState->ChannelParams[u16HwChan-1].u32Impedance == CS_REAL_IMP_1M_OHM )
			Test1MCalib( ConvertToHwChannel(i) );
	}
#endif

	if( !IsCalibrationRequired() || m_bNeverCalibrateDc )
		return i32Ret;

	// switch off potential calibration source
	if( 0 != m_u32DebugCalibSrc )
	{
		for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i++ )
		{
			SendCalibModeSetting( ConvertToHwChannel(i) );
		}
	}

	TraceCalib( 0, TRACE_CAL_PROGRESS, 0, 0, 0);

	uInt16 u16CalibSrcChan = 0;
	char szErrorStr[128];
	bool bError = false;
	int32 i32Res = CS_SUCCESS;

	for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i += GetUserChannelSkip() )
	{
		i32Ret = CalibrateFrontEnd( ConvertToHwChannel(i) );
		if( CS_FAILED(i32Ret) )
		{
			uInt16 u16UserChan = i + (m_u8CardIndex -1) * m_PlxData->CsBoard.NbAnalogChannel;
			sprintf_s( szErrorStr, sizeof(szErrorStr), "%d (channel %d)", i32Ret, u16UserChan );
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );
			if ( 0 == ( m_u32IgnoreCalibErrorMaskLocal  & (1 << (i-1)) ) )
			{
				if( !bError )
					i32Res = i32Ret;
				bError = true;
			}
			else
			{
				m_pCardState->bCalNeeded[ConvertToHwChannel(i)-1] = false;
			    m_bForceCal[ConvertToHwChannel(i)-1] = false;
			}
		}

		if( 0 != (m_u32DebugCalibSrc & 1 << (ConvertToHwChannel(i) - 1)) )
		{
			if ( CS_COUPLING_AC == m_pCardState->ChannelParams[ConvertToHwChannel(i) - 1].u32Term )
				u16CalibSrcChan = ConvertToHwChannel(i);
		}
	}

	TraceCalib( 0, TRACE_CAL_PROGRESS, 1, 0, 0);

	if( 0 != u16CalibSrcChan )
		SendCalibModeSetting( u16CalibSrcChan, true );

	return i32Res;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsSpiderDev::MsSetupAcquireAverage( bool bSetup)
{
	CsSpiderDev	*pCard = m_MasterIndependent;
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
int32	CsSpiderDev::SetupAcquireAverage( bool bSetup )
{
	int32 i32Ret = CS_SUCCESS;

	// in eXpert images there is no calibration capability
	if( 0 != m_pCardState->u8ImageId )
		return CS_SUCCESS;

	m_bCalibActive = bSetup;
	
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
		AcqCfg.i64SampleRate	 = m_pCardState->SrInfo[0].u32ClockFrequencyMhz*1000000;
		AcqCfg.i64SegmentSize	 = CSPDR_CALIB_BUFFER_SIZE;
		AcqCfg.i64Depth			 = CSPDR_CALIB_BUFFER_SIZE;
		AcqCfg.u32SegmentCount   = 1;
		AcqCfg.u32ExtClk		 = 0;
		AcqCfg.u32Mode			 &= ~(CS_MODE_USER1|CS_MODE_USER2|CS_MODE_REFERENCE_CLK);

		m_pi16CalibBuffer = (int16 *)::GageAllocateMemoryX( CSPDR_CALIB_BUFFER_SIZE*sizeof(int16) );
		if( NULL == m_pi16CalibBuffer)
			return CS_INSUFFICIENT_RESOURCES;

		m_pi32CalibA2DBuffer = (int32 *)::GageAllocateMemoryX( CSPDR_CALIB_BLOCK_SIZE*sizeof(int32) );
		if( NULL == m_pi32CalibA2DBuffer)
			return CS_INSUFFICIENT_RESOURCES;
	}
	else
	{
		::GageFreeMemoryX(m_pi16CalibBuffer);
		m_pi16CalibBuffer = NULL;

		::GageFreeMemoryX(m_pi32CalibA2DBuffer);
		m_pi32CalibA2DBuffer = NULL;
		
		AcqCfg = m_OldAcqCfg;
	}

	//Set timeout
	WriteGageRegister(TRIG_TIMEOUT_REG, bSetup ? 0 : ((uInt32) m_pCardState->AcqConfig.i64TriggerTimeout) );

	if ( m_pCardState->bZap )
	{
		// Zap boards are special. It use 50Mhz Clock setting to produce 10MHz sampling clock. This will cause error on validation of clock frequency.
		// Use this function instead of SendClockSetting() to set clock for calibration in order to bypass validation of maximum clock frequency.
		// This will fixed problem of calibration error with Zap boards. 
		i32Ret = SetClockForCalibration( bSetup );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}
	else
	{
		i32Ret = SendClockSetting( AcqCfg.i64SampleRate, AcqCfg.u32ExtClk, AcqCfg.u32ExtClkSampleSkip, (AcqCfg.u32Mode&CS_MODE_REFERENCE_CLK) != 0 );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	// Send the new config to hardware
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

int32 CsSpiderDev::SetClockForCalibration(const bool bSet)
{
	int32 i32Ret = CS_SUCCESS;
	if( bSet )
	{
		bool bExtClock = 0 != (m_pCardState->AddOnReg.u32ClkFreq & CSPDR_EXT_CLK_BYPASS);
		if( bExtClock )
		{ //set clock to highest internal
			i32Ret = SetClockThruNios( m_pCardState->SrInfo[0].u32ClockFrequencyMhz );
			if( CS_FAILED(i32Ret) ) return i32Ret;

			i32Ret = WriteRegisterThruNios(CSPRD_SET_PLL_CLK | CSPRD_SET_STRAIGHT_CLK, CSPDR_CLK_SET_WR_CMD);
			if( CS_FAILED(i32Ret) )	return i32Ret;
		}
		// 1 as decimation factor
		i32Ret = WriteRegisterThruNios(1, CSPDR_DEC_FACTOR_LOW_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = WriteRegisterThruNios(0, CSPDR_DEC_FACTOR_HIGH_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}
	else
	{
		i32Ret = SetClockThruNios( m_pCardState->AddOnReg.u32ClkFreq );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16ClkSelect, CSPDR_CLK_SET_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = WriteRegisterThruNios( (m_pCardState->AddOnReg.u32Decimation&0x7fff), CSPDR_DEC_FACTOR_LOW_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = WriteRegisterThruNios( ((m_pCardState->AddOnReg.u32Decimation>>15)&0x7fff), CSPDR_DEC_FACTOR_HIGH_WR_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}

	// Clock has changed. Fixed channels skew
	if ( m_bNucleon )
		AddonReset();

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSpiderDev::CsGetAverage(uInt16 u16Channel, int16* pi16Buffer, uInt32 u32BufferSizeSamples, int32* pi32Average)
{
	int64	i64StartPos = {0};			// Relative to trigger position

	in_PREPARE_DATATRANSFER		InXferEx;
	out_PREPARE_DATATRANSFER	OutXferEx;

	::GageZeroMemoryX( &InXferEx, sizeof(InXferEx));
	InXferEx.u32Segment		= 1;
	InXferEx.u16Channel		= u16Channel;
	InXferEx.i64StartAddr	= i64StartPos;
	InXferEx.i32OffsetAdj	= GetTriggerAddressOffsetAdjust(  m_pCardState->AcqConfig.u32Mode, (uInt32) m_pCardState->AcqConfig.i64SampleRate );
	InXferEx.u32DataMask	= GetDataMaskModeTransfer (0);
	InXferEx.u32FifoMode	= GetFifoModeTransfer(u16Channel);
	InXferEx.bBusMasterDma	= m_BusMasterSupport;

	IoctlPreDataTransfer( &InXferEx, &OutXferEx );
	CardReadMemory( pi16Buffer, u32BufferSizeSamples * m_pCardState->AcqConfig.u32SampleSize );

	int32 i32Sum = 0;
	int32 i32Count = 0;
	const int32 c32SampleOffset = m_pCardState->AcqConfig.i32SampleOffset;

	for (uInt32 u32Addr = CSPDR_CALIB_BUF_MARGIN;  u32Addr < u32BufferSizeSamples; i32Count++)
	{
		i32Sum = i32Sum + pi16Buffer[u32Addr++] - c32SampleOffset;
	}

	*pi32Average  = ( i32Sum/ i32Count );

	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsSpiderDev::AcquireAndAverage(uInt16	u16Channel, int32* pi32Avrage)
{
	int32		i32Ret = CS_SUCCESS;
	CSTIMER		CsTimeout;

	in_STARTACQUISITION_PARAMS AcqParams = {0};

	i32Ret = m_MasterIndependent->IoctlDrvStartAcquisition( &AcqParams );
	if ( CS_FAILED(i32Ret ) )
		return i32Ret;
	ForceTrigger();

	CsTimeout.Set( CSPDR_CALIB_TIMEOUT_LONG );
	while( STILL_BUSY == ReadBusyStatus() )
	{
		ForceTrigger();
		if ( CsTimeout.State() )
		{
			if( STILL_BUSY == ReadBusyStatus() )
			{
				Abort();
				ControlSpiClock(true);
				return CS_CAL_BUSY_TIMEOUT;
			}
		}
		BBtiming(1);
	}
	ControlSpiClock(true);

	//Calculate logical channel
	uInt16 u16UserChannel = ConvertToUserChannel(u16Channel);
	i32Ret = CsGetAverage( u16UserChannel, m_pi16CalibBuffer, CSPDR_CALIB_DEPTH, pi32Avrage);
	return  i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsSpiderDev::CalibrateFrontEnd(uInt16 u16HwChannel)
{
	if( !m_pCardState->bCalNeeded[u16HwChannel-1] )
		return CS_SUCCESS;

	if ( 0 != m_pCardState->u8ImageId )
	{
		// Never calibrate with expert images.
		m_pCardState->bCalNeeded[u16HwChannel-1] = false;
		return CS_SUCCESS;
	}

	if ( m_bSkipCalib && !m_bForceCal[u16HwChannel-1] )
	{
		m_pCardState->bFastCalibSettingsDone[u16HwChannel-1] = FALSE;
		if ( FastCalibrationSettings(u16HwChannel) )
			return CS_SUCCESS;
	}

	TraceCalib( u16HwChannel, TRACE_CAL_PROGRESS, 0, 0, 0);
	int32 i32Ret = CS_SUCCESS;

	i32Ret = SetCalibrationRange(m_pCardState->ChannelParams[u16HwChannel-1].u32InputRange, m_pCardState->ChannelParams[u16HwChannel-1].u32Impedance);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	i32Ret = SendCalibSource( ecmCalModeDc, 0);
	if( CS_FAILED(i32Ret) )
	{
		TraceCalib( u16HwChannel, TRACE_CAL_PROGRESS, 1, i32Ret, 0);
		SendCalibModeSetting(u16HwChannel, false);
		return i32Ret;
	}

	i32Ret = CalibrateOffset(u16HwChannel);
	if( CS_FAILED(i32Ret) )
	{
		SendCalibDC_128( 0, m_pCardState->ChannelParams[u16HwChannel-1].u32Impedance );
		TraceCalib( u16HwChannel, TRACE_CAL_RESULT, 0, 0, 0);
		TraceCalib( u16HwChannel, TRACE_CAL_PROGRESS, 1, i32Ret, 0);
		SendCalibModeSetting(u16HwChannel, false);

		m_pCardState->bCalNeeded[u16HwChannel-1] = false;
		return i32Ret;
	}
	else
	{
		TraceCalib( u16HwChannel, TRACE_CAL_RESULT, 0, 1, 0);
	}

	i32Ret = CalibrateGain(u16HwChannel);
	if( CS_FAILED(i32Ret) )
	{
		SendCalibDC_128( 0, m_pCardState->ChannelParams[u16HwChannel-1].u32Impedance );
		TraceCalib( u16HwChannel, TRACE_CAL_RESULT, 1, 0, 0);
		TraceCalib( u16HwChannel, TRACE_CAL_PROGRESS, 1, i32Ret, 0);
		SendCalibModeSetting(u16HwChannel, false);

		m_pCardState->bCalNeeded[u16HwChannel-1] = false;
		return i32Ret;
	}
	else
	{
		TraceCalib( u16HwChannel, TRACE_CAL_RESULT, 1, 1, 0);
	}

	i32Ret = CalibratePosition(u16HwChannel);
	if( CS_FAILED(i32Ret) )
	{
		SendCalibDC_128( 0, m_pCardState->ChannelParams[u16HwChannel-1].u32Impedance );
		TraceCalib( u16HwChannel, TRACE_CAL_RESULT, 2, 0, 0);
		TraceCalib( u16HwChannel, TRACE_CAL_PROGRESS, 1, i32Ret, 0);
		SendCalibModeSetting(u16HwChannel, false);

		m_pCardState->bCalNeeded[u16HwChannel-1] = false;
		return i32Ret;
	}
	else
	{
		TraceCalib( u16HwChannel, TRACE_CAL_RESULT, 2, 1, 0);
	}

	SendCalibDC_128( 0, m_pCardState->ChannelParams[u16HwChannel-1].u32Impedance );

	SetFastCalibration(u16HwChannel);

	m_pCardState->bCalNeeded[u16HwChannel-1] = false;
	m_bForceCal[u16HwChannel-1] = false;

	i32Ret = SendCalibModeSetting(u16HwChannel, false);
	TraceCalib( u16HwChannel, TRACE_CAL_PROGRESS, 1, 0, 0);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

#define CSPDR_COARSE_CAL_LIMIT    16384

int32	CsSpiderDev::CalibrateOffset(uInt16 u16Channel)
{
	int32 i32Ret = CS_SUCCESS;
	if( (0 == u16Channel) || (u16Channel > CSPDR_CHANNELS)  )
	{
		return CS_INVALID_CHANNEL;
	}

	//set offset controls to default positions
	i32Ret = UpdateCalibDac(u16Channel, ecdOffset, c_u16CalDacCount/2);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = UpdateCalibDac(u16Channel, ecdPosition, c_u16CalDacCount/2 );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = UpdateCalibDac(u16Channel, ecdGain, c_u16CalDacCount/2);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	// first disconect calibration circuitry and set cal voltage to Zero
	i32Ret = SendCalibModeSetting(u16Channel, false);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	int32  i32Voltage_uV;
	i32Ret = SendCalibDC_128( 0, CS_REAL_IMP_1M_OHM, &i32Voltage_uV );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	int32 i32TestZeroCal_uV;
	i32Ret = ReadCalibA2D(&i32TestZeroCal_uV);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	// Connect calibration circuitry
	i32Ret = SendCalibModeSetting(u16Channel, true);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	// and measure offset voltage
	int32 i32TestZeroFe_uV;
	i32Ret = ReadCalibA2D(&i32TestZeroFe_uV);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	// Invert the target to compencate the leakage current of
	// first stage single ended amplifier (u145 on channel 1)
	i32TestZeroFe_uV *= -1;

	//Target should be zero
	int32 i32Target = 0;
	TraceCalib( 0, TRACE_ZERO_OFFSET, uInt16(i32Target), i32TestZeroCal_uV, i32TestZeroFe_uV );

	// First adjust coarse offset
	// Zero offset control
	int32 i32Avg = 0;
	uInt16 u16Pos = 0;
	uInt16 u16PosDelta = c_u16CalDacCount/2;

	while( u16PosDelta > 0 )
	{
		i32Ret = UpdateCalibDac(u16Channel, ecdPosition, u16Pos | u16PosDelta );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = AcquireAndAverage(u16Channel, &i32Avg);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		if( i32Avg < 0 )
			u16Pos |= u16PosDelta;

		u16PosDelta >>= 1;
	}
	TraceCalib( u16Channel, TRACE_CAL_COARSE_OFF, u16Pos, 0, i32Avg );

	if ( i32Avg > CSPDR_COARSE_CAL_LIMIT || i32Avg < -CSPDR_COARSE_CAL_LIMIT )
	{
		return CS_COARSE_OFFSET_CAL_FAILED;
	}

	// Then adjust fine offset to match measured offset of the initial front end
	uInt16 u16Offset = 0;
	uInt16 u16OffsetDelta = c_u16CalDacCount/2;

	while( u16OffsetDelta > 0 )
	{
		i32Ret = UpdateCalibDac(u16Channel, ecdOffset, u16Offset | u16OffsetDelta );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = AcquireAndAverage(u16Channel, &i32Avg);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		if( i32Avg <= i32Target )
			u16Offset |= u16OffsetDelta;

		u16OffsetDelta >>= 1;
	}

	TraceCalib( u16Channel, TRACE_CAL_FINE_OFF, u16Offset, i32Target, i32Avg );

	if( 0 == u16Offset || (c_u16CalDacCount - 1) == u16Offset )
	{
		// Calibration fails
		// Attempt to use the default values from Eeprom
		if( ( 0 != (m_CalibInfoTable.u32Valid & CSPDR_CAL_DAC_VALID) ) &&
			( 0 == (m_CalibInfoTable.u32Valid & CSPDR_CAL_DAC_INVALID) ) )
		{
			uInt16	u16DataPosition;
			uInt16	u16DataOffset;

			i32Ret = GetEepromCalibInfoValues(u16Channel, ecdPosition, u16DataPosition );
			if( CS_FAILED(i32Ret) )	return i32Ret;

			i32Ret = GetEepromCalibInfoValues(u16Channel, ecdOffset, u16DataOffset );
			if( CS_FAILED(i32Ret) )	return i32Ret;

			i32Ret = UpdateCalibDac(u16Channel, ecdPosition, u16DataPosition );
			if( CS_FAILED(i32Ret) )	return i32Ret;

			i32Ret = UpdateCalibDac(u16Channel, ecdOffset, u16DataOffset );
			if( CS_FAILED(i32Ret) )	return i32Ret;
		}
		else
			return CS_FINE_OFFSET_CAL_FAILED;
	}

	return	CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//----- ReadCalibA2D
//		returns in uV
//-----------------------------------------------------------------------------

int32 CsSpiderDev::ReadCalibA2D(int32* pi32V_uv)
{
	int32 i32MeanSquare; //for debug
	return ReadCalibA2D_MeanSquare(pi32V_uv, &i32MeanSquare);
}

//-----------------------------------------------------------------------------
//----- ReadCalibA2D block
//		returns in uV
//-----------------------------------------------------------------------------

int32 CsSpiderDev::ReadCalibA2D_MeanSquare(int32* pi32Mean, int32* pi32MeanSquare )
{
	WriteGIO(CSPDR_AD7450_WRITE_REG,m_u32Ad7450CtlReg | 1);
	WriteGIO(CSPDR_AD7450_WRITE_REG,m_u32Ad7450CtlReg );

	uInt32 u32Code(0);
	CSTIMER	CsTimeout;
	CsTimeout.Set( CSPDR_CALIB_TIMEOUT_LONG );
	for(;;)
	{
		u32Code = ReadGIO(CSPDR_AD7450_READ_REG);
		if( 0 != (u32Code & CSPDR_AD7450_FIFO_FULL) )
			break;
		if ( CsTimeout.State() )
		{
			u32Code = ReadGIO(CSPDR_AD7450_READ_REG);
			if( 0 != (u32Code & CSPDR_AD7450_FIFO_FULL) )
				break;
			else
				return CS_CALIB_MEAS_ADC_FAILURE;
		}
		else
			BBtiming(1);
	}

	int		nValidSamples = 0;
	int		i = 0;

	for( i = 0, nValidSamples = 0; i < CSPDR_CALIB_BLOCK_SIZE; i++ )
	{
		WriteGIO(CSPDR_AD7450_WRITE_REG,m_u32Ad7450CtlReg | 2);
		WriteGIO(CSPDR_AD7450_WRITE_REG,m_u32Ad7450CtlReg );
		u32Code = ReadGIO(CSPDR_AD7450_READ_REG);
		if( 0 == (u32Code & CSPDR_AD7450_DATA_READ) )
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

	i32Avg += i32Avg > 0 ? nValidSamples/2 : - nValidSamples/2;
    i32Avg /= nValidSamples;

	// Vin = 2 * m_u32VarRefLevel_uV * CodeU55/(M * Beta)
	LONGLONG llCode = i32Avg;
	llCode = llCode * LONGLONG(m_u32VarRefLevel_uV) * LONGLONG(2);
	llCode = llCode / LONGLONG(c_u16CalAdcCount);
	llCode = (llCode * LONGLONG(c_u32ScaleFactor) ) / LONGLONG(c_u32B_scaled);

	*pi32Mean = int32(llCode);

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

		TraceCalib( 0, TRACE_CAL_NOISE, 0, *pi32Mean, int32(llMeanSquare) );

		llCode = llMeanSquare/10;
		llCode *= LONGLONG(m_u32VarRefLevel_uV) * LONGLONG(m_u32VarRefLevel_uV) * LONGLONG(4);
		llCode /=  LONGLONG(c_u16CalAdcCount) * LONGLONG(c_u16CalAdcCount);
		llCode *=  LONGLONG(c_u32ScaleFactor) * LONGLONG(c_u32ScaleFactor);
		llCode /=  LONGLONG(c_u32B_scaled) * LONGLONG(c_u32B_scaled);

		*pi32MeanSquare = int32(llCode);
	}
	return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSpiderDev::SetCalibrationRange(uInt32 u32FrontEndSwing_mV, uInt32 u32Imped)
{
	if( m_pCardState->bSpiderV12 || (0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSPDR_AOHW_OPT_MODIFIED_CAL )) )
		return SetCalibrationRange_v2(u32FrontEndSwing_mV, u32Imped);

	// Due to votage divider in between calibration part and regular front end we need
	// to increase required swing of calibration signal by ratio of the divider.
	LONGLONG llFeSwing = u32FrontEndSwing_mV;
	llFeSwing *= LONGLONG(u32Imped + c_u16L6);
	llFeSwing /= LONGLONG(u32Imped);

	//Store value for trace
	int32 i32IR = u32FrontEndSwing_mV/2;

	m_u32MarginDenom = c_u32MarginDenomHiRange;

	if( uInt32(llFeSwing) < c_u32MarginLoRangeBoundary )
		m_u32MarginDenom = c_u32MarginDenomLoRange;

	llFeSwing *= c_u32MarginNom;
	llFeSwing /= m_u32MarginDenom;

	u32FrontEndSwing_mV = uInt32(llFeSwing);

	// Swing cannot exceed capacity of the calibration circuitry
	uInt32 u32MaxCalOutput_uV = c_u32MaxCalVoltage_uV;
	if( m_pCardState->bSpiderV12 || (0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSPDR_AOHW_OPT_BOOSTED_CAL_AMP ) ) )
		u32MaxCalOutput_uV = c_u32MaxCalVoltageBoosted_uV;

    if( u32FrontEndSwing_mV >  2 * u32MaxCalOutput_uV / 1000 )
		u32FrontEndSwing_mV = 2 * u32MaxCalOutput_uV / 1000;

	int32 i32Ret = CS_SUCCESS;
	if( m_u32CalibRange_uV != u32FrontEndSwing_mV * 500 )
	{
		LONGLONG llSwing = LONGLONG(u32FrontEndSwing_mV) *  c_u16CalDacCount * c_u32ScaleFactor;

		LONGLONG llDenom = LONGLONG(c_u16Vref_mV) * c_u32D_scaled;
		LONGLONG llCode = (llSwing  + llDenom/2 )/llDenom;

		m_u16CalCodeB = llCode > c_u16CalDacCount - 1 ? c_u16CalDacCount - 1 : uInt16(llCode);

		llDenom = LONGLONG(c_u16Vref_mV) * 2 * c_u32K_scaled;
		llCode = (llSwing + llDenom/2 )/llDenom;

		m_u16CalCodeA = llCode > c_u16CalDacCount - 1 ? c_u16CalDacCount - 1 : uInt16(llCode);

	// Vcal(I) = K * Vr * A/M - D * Vr * (B*I)/(M*M)
	// Vcal(I) = Vr * (K * A * M - D * (B*I)) /(M*M)
	// Vcal(M/2) = Vr * (K * A * 2 - D * B) /M

		LONGLONG llZero =  LONGLONG( 2* c_u32K_scaled) * LONGLONG(m_u16CalCodeA) - LONGLONG(c_u32D_scaled) * LONGLONG(m_u16CalCodeB);
	// z = Round(M * Vcal(M/2)/Sw).
	// z = Round(Vr * (K * A * 2 - D * B)/Sw).
		llZero *= LONGLONG(c_u16Vref_mV);

		llDenom = LONGLONG(u32FrontEndSwing_mV) * LONGLONG(2) * LONGLONG(c_u32ScaleFactor);

		if( llZero >= 0 )
			llCode = (llZero + llDenom/2)/llDenom;
		else
			llCode = (llZero - llDenom/2)/llDenom;
		m_i16CalCodeZ = llCode > c_u16CalDacCount - 1 ? c_u16CalDacCount - 1 : uInt16(llCode);

#ifdef _WINDOWS_
		ASSERT( uInt32(c_u16R1639) * (c_u16R980 + c_u16R1637) == uInt32(c_u16R1637) * (c_u16R1638 + c_u16R1639) );
#endif

		LONGLONG llF_scaled = LONGLONG(c_u16CalAdcCount) * LONGLONG(c_u32ScaleFactor) *
							LONGLONG(c_u32ScaleFactor) * LONGLONG(c_u32B_scaled) /
							( LONGLONG(c_u32A_scaled) * LONGLONG(c_u32C_scaled) );

	// CodeB_u77 =  Sw/(2*Vref) * F
		LONGLONG llCodeB_u77 = LONGLONG(u32FrontEndSwing_mV) * llF_scaled;
		llDenom = LONGLONG( 2 * c_u16Vref_mV) * LONGLONG(c_u32ScaleFactor);
		llCodeB_u77 = (llCodeB_u77 + llDenom/2)/llDenom;
		m_u16VarRefCode = llCodeB_u77 > c_u16CalDacCount - 1 ? c_u16CalDacCount - 1 : uInt16(llCodeB_u77);

		i32Ret = WriteToCalibDac(CSPDR_CAL_CODE, c_u16CalDacCount/2);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteToCalibDac(CSPDR_CAL_VA, m_u16CalCodeA);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteToCalibDac(CSPDR_CAL_VB, m_u16CalCodeB);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteToCalibDac(CSPDR_CAL_REF, m_u16VarRefCode, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	i32Ret = WriteRegisterThruNios( SPDR_REF_ADC_CMD_RESET, CSPDR_REF_ADC_WR_CMD);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// MeasVref_u55 = 2 * (CodeU36/Mu36 )*Vref unipolar ADC
	uInt32 u32CodeU36 = ReadRegisterFromNios(SPDR_REF_ADC_CMD_RD, CSPDR_REF_ADC_WR_CMD);
	u32CodeU36 &= 0xffff;
	if( 0 == u32CodeU36 )
	{
		return CS_CALIB_MEAS_ADC_FAILURE;
	}

	LONGLONG llVarVref_uV = ( LONGLONG(2) * LONGLONG(c_u16Vref_mV) * LONGLONG(1000) * LONGLONG(u32CodeU36))/LONGLONG(c_u32RefAdcCount);

	LONGLONG llBase = (llVarVref_uV * LONGLONG(c_u32A_scaled) ) / LONGLONG(c_u32ScaleFactor);
	m_u32VarRefLevel_uV = uInt32(llBase);

	m_u32CalibRange_uV = u32FrontEndSwing_mV * 500;

	TraceCalib( 0, TRACE_CAL_HEADER, 0, i32IR, u32Imped);

	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsSpiderDev::SetCalibrationRange_v2(uInt32 u32FrontEndSwing_mV, uInt32 u32Imped)
{
	// Due to votage divider in between calibration part and regular front end we need
	// to increase required swing of calibration signal by ratio of the divider.
	LONGLONG llFeSwing = u32FrontEndSwing_mV;
	llFeSwing *= LONGLONG(u32Imped + c_u16L6);
	llFeSwing /= LONGLONG(u32Imped);

	//Store value for trace
	int32 i32IR = u32FrontEndSwing_mV/2;

	m_u32MarginDenom = c_u32MarginDenomHiRange_v2;

	if( uInt32(llFeSwing) < c_u32MarginLoRangeBoundary )
	{
		llFeSwing = c_u32MarginLoRangeBoundary;
		m_u32MarginDenom = c_u32MarginDenomLoRange_v2;
	}

	llFeSwing *= c_u32MarginNom;
	llFeSwing /= m_u32MarginDenom;

	u32FrontEndSwing_mV = uInt32(llFeSwing);

	// Swing cannot exceed capacity of the calibration circuitry
	uInt32 u32MaxCalOutput_uV = c_u32MaxCalVoltage_uV;
	if( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSPDR_AOHW_OPT_BOOSTED_CAL_AMP ) )
		u32MaxCalOutput_uV = c_u32MaxCalVoltageBoosted_uV;

    if( u32FrontEndSwing_mV >  2 * u32MaxCalOutput_uV / 1000 )
		u32FrontEndSwing_mV = 2 * u32MaxCalOutput_uV / 1000;

	int32 i32Ret = CS_SUCCESS;
	if( m_u32CalibRange_uV != u32FrontEndSwing_mV * 500 )
	{
		m_i16CalCodeZ = 0;
		m_u16CalCodeB = c_u16CalDacCount/2;

		//  RefCode = M * Fe/(Vref* Min(K, D-K))
		uInt32 u32MinFactor = c_u32K_scaled_v2 < c_u32D_scaled - c_u32K_scaled_v2 ? c_u32K_scaled_v2 : c_u32D_scaled - c_u32K_scaled_v2;
		LONGLONG llNom  = LONGLONG(u32FrontEndSwing_mV) *  c_u16CalDacCount * c_u32ScaleFactor;
		LONGLONG llDenom = LONGLONG(c_u16Vref_mV) * u32MinFactor * 2;

		LONGLONG llCode = (llNom  + llDenom/2 )/llDenom;

		m_u16CalCodeA = llCode > c_u16CalDacCount - 1 ? c_u16CalDacCount - 1 : uInt16(llCode);

		LONGLONG llF_scaled = LONGLONG(c_u16CalAdcCount) * LONGLONG(c_u32ScaleFactor) *
							LONGLONG(c_u32ScaleFactor) * LONGLONG(c_u32B_scaled) /
							( LONGLONG(c_u32A_scaled) * LONGLONG(c_u32C_scaled) );

	// CodeB_u77 =  Sw/(2*Vref) * F
		LONGLONG llCodeB_u77 = LONGLONG(u32FrontEndSwing_mV) * llF_scaled;
		llDenom = LONGLONG( 2 * c_u16Vref_mV) * LONGLONG(c_u32ScaleFactor);
		llCodeB_u77 = (llCodeB_u77 + llDenom/2)/llDenom;
		m_u16VarRefCode = llCodeB_u77 > c_u16CalDacCount - 1 ? c_u16CalDacCount - 1 : uInt16(llCodeB_u77);


		i32Ret = WriteToCalibDac(CSPDR_CAL_CODE, m_u16CalCodeA);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteToCalibDac(CSPDR_CAL_VA, c_u16CalDacCount/2);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteToCalibDac(CSPDR_CAL_VB, c_u16CalDacCount/2);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteToCalibDac(CSPDR_CAL_REF, m_u16VarRefCode, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	i32Ret = WriteRegisterThruNios( SPDR_REF_ADC_CMD_RESET, CSPDR_REF_ADC_WR_CMD);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// MeasVref_u55 = 2 * (CodeU36/Mu36 )*Vref unipolar ADC
	uInt32 u32CodeU36 = ReadRegisterFromNios(SPDR_REF_ADC_CMD_RD, CSPDR_REF_ADC_WR_CMD);
	u32CodeU36 &= 0xffff;
	if( 0 == u32CodeU36 )
	{
		return CS_CALIB_MEAS_ADC_FAILURE;
	}

	LONGLONG llVarVref_uV = ( LONGLONG(2) * LONGLONG(c_u16Vref_mV) * LONGLONG(1000) * LONGLONG(u32CodeU36))/LONGLONG(c_u32RefAdcCount);

	LONGLONG llBase = (llVarVref_uV * LONGLONG(c_u32A_scaled) ) / LONGLONG(c_u32ScaleFactor);
	m_u32VarRefLevel_uV = uInt32(llBase);

	m_u32CalibRange_uV = u32FrontEndSwing_mV * 500;

	TraceCalib( 0, TRACE_CAL_HEADER, 0, i32IR, u32Imped);

	return i32Ret;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

#define MAX_ITER 32

int32 CsSpiderDev::SendCalibDC_128(int32 i32Target_uV, uInt32 u32Imped, int32*  pVoltage_uV, int16* pi16InitialCode )
{
	if( m_pCardState->bSpiderV12 || (0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSPDR_AOHW_OPT_MODIFIED_CAL )) )
		return SendCalibDC_128_v2(i32Target_uV, u32Imped, pVoltage_uV, pi16InitialCode );

	int32 i32Ret = CS_SUCCESS;

	LONGLONG llTar = i32Target_uV;
	llTar *= LONGLONG(u32Imped + c_u16L6);
	llTar /= LONGLONG(u32Imped);
	int32 i32TargetDac_uV = int32(llTar);

	LONGLONG llMaxTarget_uV =  m_u32CalibRange_uV;
	llMaxTarget_uV *= m_u32MarginDenom;
	llMaxTarget_uV /= c_u32MarginNom;
	int32 i32MaxTar = int32(llMaxTarget_uV);

	if( i32TargetDac_uV > i32MaxTar )
		i32TargetDac_uV = i32MaxTar;
	else if( i32TargetDac_uV < -i32MaxTar )
		i32TargetDac_uV = -i32MaxTar;

	llTar = i32TargetDac_uV;
	llTar *= LONGLONG(u32Imped);
	llTar /= LONGLONG(u32Imped + c_u16L6);
	i32Target_uV = int32(llTar);

	int16 i16Code;
	if ( NULL != pi16InitialCode && *pi16InitialCode > 0 && *pi16InitialCode < c_u16CalDacCount - 1 )
		i16Code = *pi16InitialCode;
	else
	{
		LONGLONG llCode = c_u16CalDacCount/2; //Middle point

		llCode *= -LONGLONG(i32TargetDac_uV);
		llCode /= LONGLONG(m_u32CalibRange_uV);

		i16Code = int16(llCode + c_u16CalDacCount/2 + m_i16CalCodeZ );
		i16Code = i16Code < 0 ? 0 : ( i16Code > c_u16CalDacCount-1 ? c_u16CalDacCount - 1 : i16Code );
	}

	i32Ret = WriteToCalibDac(CSPDR_CAL_CODE, i16Code, m_u8CalDacSettleDelay);
	if( CS_FAILED(i32Ret) )
		return i32Ret;


	int32 i32Signal_uV = i32Target_uV;
	const int32 ci32MaxError = int32( 3 * m_u32CalibRange_uV / (2*c_u16CalDacCount) ) ;

	if( !m_bSkipTrim )
	{
		i32Ret = ReadCalibA2D(&i32Signal_uV);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		TraceCalib( 0, TRACE_CAL_SIGNAL_ITER, i16Code, i32Target_uV, i32Signal_uV );

		int32 i32Diff = i32Signal_uV - i32Target_uV;

		int16 i16Correction = 0;
		uInt8 u8Iter = 0;

		while ( (labs(i32Diff) > ci32MaxError ) && u8Iter < MAX_ITER )
		{
			LONGLONG llCor = LONGLONG(i32Diff) * c_u16CalDacCount;
			llCor *= LONGLONG( m_u32MarginDenom );
			llCor *= LONGLONG(u32Imped + c_u16L6);
			llCor /= LONGLONG(u32Imped);
			llCor /= LONGLONG(c_u32MarginNom);
			llCor /= LONGLONG(2*m_u32CalibRange_uV);

			i16Correction = int16(llCor);

			if( 0 == i16Correction )
			{
				i16Correction = i32Diff > 0 ? 1 : -1;
			}

			int16 i16OldCode = i16Code;

			i16Code += i16Correction;

			i16Code = i16Code < 0 ? 0 : ( i16Code > c_u16CalDacCount-1 ? c_u16CalDacCount - 1 : i16Code );

			if( i16OldCode == i16Code ) //Only if we are at the limits
				return CS_CALIB_DAC_OUT_OF_RANGE;

			i32Ret = WriteToCalibDac(CSPDR_CAL_CODE, i16Code, m_u8CalDacSettleDelay);
			if( CS_FAILED(i32Ret) )
				return i32Ret;

			i32Ret = ReadCalibA2D(&i32Signal_uV);
			if( CS_FAILED(i32Ret) )
				return i32Ret;

			u8Iter++;

			TraceCalib( u8Iter, TRACE_CAL_SIGNAL_ITER, i16Code, i32Target_uV, i32Signal_uV );
			i32Diff = i32Signal_uV - i32Target_uV;
		}

		TraceCalib( u8Iter, TRACE_CAL_SIGNAL, i16Code, i32Target_uV, i32Signal_uV );
		if ( labs(i32Diff) > ci32MaxError )
		{
			return CS_DAC_CALIB_FAILURE;
		}
	}

	if( 0 == i32Target_uV )
		m_i16CalCodeZ = i16Code - c_u16CalDacCount/2;

	if( NULL != pi16InitialCode )
		*pi16InitialCode = i16Code;

	if( NULL != pVoltage_uV )
		*pVoltage_uV = i32Signal_uV;

	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSpiderDev::SendCalibDC_128_v2(int32 i32Target_uV, uInt32 u32Imped, int32* pVoltage_uV, int16* pi16InitialCode )
{
	int32 i32Ret = CS_SUCCESS;

	LONGLONG llTar = i32Target_uV;
	llTar *= LONGLONG(u32Imped + c_u16L6);
	llTar /= LONGLONG(u32Imped);
	int32 i32TargetDac_uV = int32(llTar);

	LONGLONG llMaxTarget_uV =  m_u32CalibRange_uV;
	llMaxTarget_uV *= m_u32MarginDenom;
	llMaxTarget_uV /= c_u32MarginNom;
	int32 i32MaxTar = int32(llMaxTarget_uV);

	if( i32TargetDac_uV > i32MaxTar )
		i32TargetDac_uV = i32MaxTar;
	else if( i32TargetDac_uV < -i32MaxTar )
		i32TargetDac_uV = -i32MaxTar;

	llTar = i32TargetDac_uV;
	llTar *= LONGLONG(u32Imped);
	llTar /= LONGLONG(u32Imped + c_u16L6);
	i32Target_uV = int32(llTar);

	int16 i16Code;
	if ( NULL != pi16InitialCode && *pi16InitialCode > 0 && *pi16InitialCode < c_u16CalDacCount - 1 )
		i16Code = *pi16InitialCode;
	else
	{
		LONGLONG llCode = c_u16CalDacCount/2; //Middle point

		llCode *= -LONGLONG(i32TargetDac_uV);
		llCode /= LONGLONG(m_u32CalibRange_uV);

		i16Code = int16(llCode + c_u16CalDacCount/2 );
		i16Code = i16Code < 0 ? 0 : ( i16Code > c_u16CalDacCount-1 ? c_u16CalDacCount - 1 : i16Code );
	}

	i32Ret = WriteToCalibDac(CSPDR_CAL_VB, i16Code, m_u8CalDacSettleDelay);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	int32 i32Signal_uV = i32Target_uV;
	const int32 ci32MaxError = int32( 3 * m_u32CalibRange_uV / (2*c_u16CalDacCount) ) ;

	if( !m_bSkipTrim )
	{
		i32Ret = ReadCalibA2D(&i32Signal_uV);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		TraceCalib( 0, TRACE_CAL_SIGNAL_ITER, i16Code, i32Target_uV, i32Signal_uV );

		int32 i32Diff = i32Signal_uV - i32Target_uV;

		int16 i16Correction = 0;
		uInt8 u8Iter = 0;

		while ( (labs(i32Diff) > ci32MaxError ) && u8Iter < MAX_ITER )
		{
			LONGLONG llCor = LONGLONG(i32Diff) * c_u16CalDacCount;
			llCor *= LONGLONG( m_u32MarginDenom );
			llCor *= LONGLONG(u32Imped + c_u16L6);
			llCor /= LONGLONG(u32Imped);
			llCor /= LONGLONG(c_u32MarginNom);
			llCor /= LONGLONG(2*m_u32CalibRange_uV);

			i16Correction = int16(llCor);

			if( 0 == i16Correction )
			{
				i16Correction = i32Diff > 0 ? 1 : -1;
			}

			int16 i16OldCode = i16Code;

			i16Code += i16Correction;

			i16Code = i16Code < 0 ? 0 : ( i16Code > c_u16CalDacCount-1 ? c_u16CalDacCount - 1 : i16Code );

			if( i16OldCode == i16Code ) //Only if we are at the limits
				return CS_CALIB_DAC_OUT_OF_RANGE;

			i32Ret = WriteToCalibDac(CSPDR_CAL_VB, i16Code, m_u8CalDacSettleDelay);
			if( CS_FAILED(i32Ret) )
				return i32Ret;

			i32Ret = ReadCalibA2D(&i32Signal_uV);
			if( CS_FAILED(i32Ret) )
				return i32Ret;

			u8Iter++;

			TraceCalib( u8Iter, TRACE_CAL_SIGNAL_ITER, i16Code, i32Target_uV, i32Signal_uV );
			i32Diff = i32Signal_uV - i32Target_uV;
		}

		TraceCalib( u8Iter, TRACE_CAL_SIGNAL, i16Code, i32Target_uV, i32Signal_uV );
		if ( labs(i32Diff) > ci32MaxError )
		{
			return CS_DAC_CALIB_FAILURE;
		}
	}

	if( 0 == i32Target_uV )
		m_i16CalCodeZ = i16Code - c_u16CalDacCount/2;

	if( NULL != pi16InitialCode )
		*pi16InitialCode = i16Code;

	if( NULL != pVoltage_uV )
		*pVoltage_uV = i32Signal_uV;

	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
#define GAIN_CAL_LEVEL		120
int32	CsSpiderDev::CalibrateGain(uInt16 u16Channel)
{
	int32 i32Ret = CS_SUCCESS;
	if( (0 == u16Channel) || (u16Channel > CSPDR_CHANNELS)  )
	{
		return CS_INVALID_CHANNEL;
	}

	uInt16 u16ChanZeroBased = u16Channel-1;

	int32  i32VoltageHigh_uV, i32VoltageLow_uV;
	int32  i32CalLevel_uV = int32((LONGLONG(m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange) * GAIN_CAL_LEVEL * LONGLONG(1000) ) / c_u16CalLevelSteps);
	uInt16 u16Gain = 0;
	uInt16 u16GainDelta = c_u16CalDacCount/2;
	int32  i32AvgHi(0), i32AvgLo(0), i32Avg(0);
	int32  i32Target_uV, i32Target(0);
	int16  i16CodeHi(c_u16CalDacCount), i16CodeLo(c_u16CalDacCount);

	while( u16GainDelta > 0 )
	{
		i32Ret = UpdateCalibDac(u16Channel, ecdGain, u16Gain | u16GainDelta );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = SendCalibDC_128( i32CalLevel_uV, m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance, &i32VoltageHigh_uV, &i16CodeHi );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = AcquireAndAverage(u16Channel, &i32AvgHi);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = SendCalibDC_128( -i32CalLevel_uV, m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance, &i32VoltageLow_uV, &i16CodeLo );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = AcquireAndAverage(u16Channel, &i32AvgLo);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Avg = i32AvgHi - i32AvgLo;
		i32Target_uV = i32VoltageHigh_uV - i32VoltageLow_uV;

		i32Target = int32(( LONGLONG( -2 * m_pCardState->AcqConfig.i32SampleRes) * LONGLONG(i32Target_uV) )/ LONGLONG(m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange*1000));

		if( i32Avg < i32Target )
			u16Gain |= u16GainDelta;

		u16GainDelta >>= 1;
	}

	TraceCalib( u16Channel, TRACE_CAL_GAIN, u16Gain, i32Target, i32Avg );

	if( 0 == u16Gain || (c_u16CalDacCount - 1) == u16Gain )
	{
		// Calibration fails
		// Attempt to use the default values from Eeprom
		if( ( 0 != (m_CalibInfoTable.u32Valid & CSPDR_CAL_DAC_VALID) ) &&
			( 0 == (m_CalibInfoTable.u32Valid & CSPDR_CAL_DAC_INVALID) ) )
		{
			uInt16	u16DataGain = 0;

			i32Ret = GetEepromCalibInfoValues(u16Channel, ecdGain, u16DataGain );
			if( CS_FAILED(i32Ret) )	return i32Ret;

			i32Ret = UpdateCalibDac(u16Channel, ecdGain, u16DataGain );
			if( CS_FAILED(i32Ret) )	return i32Ret;
		}
		else
			return CS_GAIN_CAL_FAILED;
	}

	return	CS_SUCCESS;
}

///------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsSpiderDev::CalibratePosition(uInt16 u16Channel)
{
	if( (0 == u16Channel) || (u16Channel > CSPDR_CHANNELS)  )
		return CS_INVALID_CHANNEL;

	if ( m_pCardState->bSpiderV12 && !m_bDisableUserOffet )
	{
		uInt16 u16ChanZeroBased = u16Channel-1;
		int32  i32Index = GetFEIndex(u16Channel);
		uInt32 u32RangeIndex = m_pCardState->AddOnReg.u32FeIndex[u16ChanZeroBased];
		uInt16 u16PositionDac  = GetControlDacId( ecdPosition, u16Channel );

		int32 i32Voltage_uV;
		int32 i32Ret = SendCalibDC_128( 0, m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance, &i32Voltage_uV );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		int32 i32PosCode(0), i32PosCodePlus(0), i32PosCodeMinus(0);
		int32 i32PosCodeDelta = c_u16CalDacCount/2;
		int32 i32Avg(0), i32Target = labs(m_pCardState->AcqConfig.i32SampleRes)/2;

		uInt16 u16BaseOffset = m_pCardState->DacInfo[u16ChanZeroBased][i32Index][u32RangeIndex].u16CoarseOffset;

		while( i32PosCodeDelta > 0 )
		{
			int32 i32NewPosCode = i32PosCodePlus + i32PosCodeDelta + u16BaseOffset;

			uInt16 u16Code = i32NewPosCode > 0 ? ( i32NewPosCode < int32(c_u16CalDacCount-1) ? uInt16(i32NewPosCode) : c_u16CalDacCount-1 ) : 0;
			i32Ret = WriteToCalibDac(u16PositionDac, u16Code, m_u8CalDacSettleDelay);
			if( CS_FAILED(i32Ret) )	return i32Ret;

			i32Ret = AcquireAndAverage(u16Channel, &i32Avg);
			if( CS_FAILED(i32Ret) )	return i32Ret;

			if( i32Avg < i32Target )
				i32PosCodePlus += i32PosCodeDelta;

			i32PosCodeDelta /= 2;
		}

		if( 0 != i32PosCodePlus && (c_u16CalDacCount - 1 ) > i32PosCodePlus )
		{
			TraceCalib( u16Channel, TRACE_CAL_POS, uInt16(i32PosCodePlus), i32Target, i32Avg);
			i32PosCode = i32PosCodePlus;
		}

		i32PosCodeDelta =  -c_u16CalDacCount/2;
		i32Target *= -1;
		while( i32PosCodeDelta < 0 )
		{
			int32 i32NewPosCode = i32PosCodeMinus + i32PosCodeDelta + u16BaseOffset;

			uInt16 u16Code = i32NewPosCode > 0 ? ( i32NewPosCode < int32(c_u16CalDacCount-1) ? uInt16(i32NewPosCode) : c_u16CalDacCount-1 ) : 0;
			i32Ret = WriteToCalibDac(u16PositionDac, u16Code, m_u8CalDacSettleDelay);
			if( CS_FAILED(i32Ret) )	return i32Ret;

			i32Ret = AcquireAndAverage(u16Channel, &i32Avg);
			if( CS_FAILED(i32Ret) )	return i32Ret;

			if( i32Avg > i32Target )
				i32PosCodeMinus += i32PosCodeDelta;

			i32PosCodeDelta /= 2;
		}

		if( 0 != i32PosCodeMinus && (1 - c_u16CalDacCount) < i32PosCodeMinus  )
		{
			TraceCalib( u16Channel, TRACE_CAL_POS, uInt16(-i32PosCodeMinus), i32Target, i32Avg);
			if ( 0 != i32PosCode )
			{
				i32PosCode -= i32PosCodeMinus;
				i32PosCode /= 2;
			}
			else
				i32PosCode = -i32PosCodeMinus;
		}
		else if ( 0 == i32PosCode )
		{
			return CS_POSITION_CAL_FAILED;
		}
		else
		{
			TraceCalib( u16Channel, TRACE_CAL_POS, uInt16(i32PosCode), i32Target, i32Avg);
		}

		m_pCardState->DacInfo[u16ChanZeroBased][i32Index][u32RangeIndex].u16CodeDeltaForHalfIR = uInt16(i32PosCode);

		return SendPosition( u16Channel, m_pCardState->ChannelParams[u16ChanZeroBased].i32Position );
	}
	else
		return CS_SUCCESS;

}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsSpiderDev::GetEepromCalibInfoValues(uInt16 u16Channel, eCalDacId ecdId, uInt16 &u16Data)
{
	int32 i32Ret = CS_SUCCESS;
	if( (0 == u16Channel) || (u16Channel > CSPDR_CHANNELS)  )
	{
		return CS_INVALID_CHANNEL;
	}

	uInt16 u16ChanZeroBased = u16Channel-1;

	int32 i32Index = GetFEIndex(u16Channel);
	uInt32 u32RangeIndex = m_pCardState->AddOnReg.u32FeIndex[u16ChanZeroBased];

	switch( ecdId )
	{
		case ecdPosition:
			u16Data = m_CalibInfoTable.DacInfo[u16ChanZeroBased][i32Index][u32RangeIndex].u16CoarseOffset;
			break;
		case ecdOffset:
			u16Data = m_CalibInfoTable.DacInfo[u16ChanZeroBased][i32Index][u32RangeIndex].u16FineOffset;
			break;
		case ecdGain:
			u16Data = m_CalibInfoTable.DacInfo[u16ChanZeroBased][i32Index][u32RangeIndex].u16Gain;
			break;
	}

	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsSpiderDev::UpdateCalibInfoStrucure()
{
	uInt16	u16HwChannel;
	uInt16	u16ChanZeroBased;
	uInt32	u32MaxRangeIndex;

	uInt32	u32MaxCouplingIndex = CSPDR_AC_COUPLING_INDEX | CSPDR_FILTER_INDEX | CSPDR_IMPEDANCE_50_INDEX;

	// Verify that all channels has been calibrated
	for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i++ )
	{
		u16HwChannel = ConvertToHwChannel(i);
		u16ChanZeroBased = u16HwChannel-1;

		for ( uInt16 j = 0; j < u32MaxCouplingIndex; j ++ )
		{
			// Skip checkking calibration with Filter
			if ( (j & CSPDR_FILTER_INDEX) != 0 )
				continue;

			if ( j & CSPDR_IMPEDANCE_50_INDEX )
				u32MaxRangeIndex = m_pCardState->u32SwingTableSize[0];
			else
				u32MaxRangeIndex = m_pCardState->u32SwingTableSize[1];

			for ( uInt16 k = 0; k < u32MaxRangeIndex; k++ )
			{
				if ( m_pCardState->DacInfo[u16ChanZeroBased][j][k].bCalibrated == false )
					return CS_CHANNELS_NOTCALIBRATED;
			}
		}
	}

	::GageCopyMemoryX( m_CalibInfoTable.DacInfo, m_pCardState->DacInfo, sizeof( m_CalibInfoTable.DacInfo ) );

	return CS_SUCCESS;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

BOOLEAN	CsSpiderDev::IsCalibrationRequired()
{
	// Detect if calibration for at least one channel is required.
	for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i += GetUserChannelSkip() )
	{
		uInt16 u16ChanZeroBased = ConvertToHwChannel(i) - 1;
		if(  m_pCardState->bCalNeeded[u16ChanZeroBased] )
		{
			return TRUE;
		}
	}

	return FALSE;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

bool CsSpiderDev::FastCalibrationSettings(uInt16 u16Channel, BOOLEAN bForceSet )
{
	int32 i32Index = GetFEIndex(u16Channel);
	uInt16 u16ChanZeroBased = u16Channel - 1;
	uInt32 u32RangeIndex = m_pCardState->AddOnReg.u32FeIndex[u16ChanZeroBased];

	if ( bForceSet ||
		( m_pCardState->DacInfo[u16ChanZeroBased][i32Index][u32RangeIndex].bCalibrated  && ! m_pCardState->bFastCalibSettingsDone[u16ChanZeroBased] ) )
	{
		uInt16 u16Dac = GetControlDacId( ecdPosition, u16Channel );
		int32 i32Ret = WriteToCalibDac(u16Dac, m_pCardState->DacInfo[u16ChanZeroBased][i32Index][u32RangeIndex].u16CoarseOffset);
		if( CS_FAILED(i32Ret) )
			return false;

		u16Dac = GetControlDacId( ecdOffset, u16Channel );
		i32Ret = WriteToCalibDac(u16Dac, m_pCardState->DacInfo[u16ChanZeroBased][i32Index][u32RangeIndex].u16FineOffset);
		if( CS_FAILED(i32Ret) )
			return false;

		u16Dac = GetControlDacId( ecdGain, u16Channel );
		i32Ret = WriteToCalibDac(u16Dac, m_pCardState->DacInfo[u16ChanZeroBased][i32Index][u32RangeIndex].u16Gain, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )
			return false;

		i32Ret = SendPosition( u16Channel, m_pCardState->ChannelParams[u16ChanZeroBased].i32Position );
		if( CS_FAILED(i32Ret) )
			return false;

		m_pCardState->bFastCalibSettingsDone[u16ChanZeroBased] = TRUE;

		return true;
	}

	return false;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
BOOLEAN	CsSpiderDev::ForceFastCalibrationSettingsAllChannels()
{
	BOOLEAN	bRetVal = TRUE;

	for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i += GetUserChannelSkip() )
	{
		if( ! FastCalibrationSettings( ConvertToHwChannel(i), TRUE ) )
			bRetVal = FALSE;
	}

	return bRetVal;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void CsSpiderDev::SetFastCalibration(uInt16 u16Channel)
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
		int32 i32Index = GetFEIndex(u16Channel);
		uInt32 u32RangeIndex = m_pCardState->AddOnReg.u32FeIndex[u16Channel -1];

	    m_pCardState->DacInfo[u16Channel-1][i32Index][u32RangeIndex].bCalibrated = true;
	}
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsSpiderDev::UpdateCalibDac(uInt16 u16Channel, eCalDacId ecdId, uInt16 u16Data)
{
	int32 i32Ret = CS_SUCCESS;
	if( (0 == u16Channel) || (u16Channel > CSPDR_CHANNELS)  )
	{
		return CS_INVALID_CHANNEL;
	}

	uInt16 u16ChanZeroBased = u16Channel-1;

	int32 i32Index = GetFEIndex(u16Channel);
	uInt32 u32RangeIndex = m_pCardState->AddOnReg.u32FeIndex[u16ChanZeroBased];

	uInt16 u16Dac = GetControlDacId( ecdId, u16Channel );

	TraceCalib( u16Channel, TRACE_DAC, u16Data, u16Dac, 0);

	i32Ret = WriteToCalibDac(u16Dac, u16Data, m_u8CalDacSettleDelay);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	switch( ecdId )
	{
		case ecdPosition: m_pCardState->DacInfo[u16ChanZeroBased][i32Index][u32RangeIndex].u16CoarseOffset = u16Data; break;
		case ecdOffset:   m_pCardState->DacInfo[u16ChanZeroBased][i32Index][u32RangeIndex].u16FineOffset = u16Data; break;
		case ecdGain:     m_pCardState->DacInfo[u16ChanZeroBased][i32Index][u32RangeIndex].u16Gain = u16Data; break;
	}

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsSpiderDev::GetFreeTriggerEngine( int32 i32TriggerSource, uInt16 *u16TriggerEngine )
{
	uInt16	u16TrigEngineIndex;

	// A Free trigger engine is the one that is disabled and not yet assigned to any user
	// trigger settings

	if ( i32TriggerSource < 0 )		// Trigger from external input
	{
		u16TrigEngineIndex = CSPDR_TRIGENGINE_EXT;
		if ( 0 == m_CfgTrigParams[u16TrigEngineIndex].u32UserTrigIndex )
		{
			*u16TriggerEngine = u16TrigEngineIndex;
			return CS_SUCCESS;
		}
		else
			return CS_ALLTRIGGERENGINES_USED;
	}
	else
	{
			uInt16	u16Start = 0;
		uInt16	u16End = CSPDR_TOTAL_TRIGENGINES;

		if ( i32TriggerSource > 0 ) // disable may be in any position for channel trigger need to define appropriate engine
		{
			uInt16 u16Hw = ConvertToHwChannel(uInt16(i32TriggerSource));
		
			u16End = u16Hw * 2; 
			u16Start = u16End-1;

			for ( uInt16 i = u16Start; i <= u16End; i ++ )
			{
				if ( 0 == m_CfgTrigParams[i].u32UserTrigIndex )
				{
					*u16TriggerEngine = i;
					return CS_SUCCESS;
				}
			}
		}
		else
		{
			if ( 0 == m_CfgTrigParams[0].u32UserTrigIndex )
			{
				*u16TriggerEngine = 0;
				return CS_SUCCESS;
			}

			for ( uInt16 i = 0; i < m_PlxData->CsBoard.NbAnalogChannel; i ++ )
			{
				uInt16 u16Hw = ConvertToHwChannel(i+1);
				uInt16 u16Index = 2*u16Hw-1;
				if ( 0 == m_CfgTrigParams[u16Index].u32UserTrigIndex )
				{
					*u16TriggerEngine = u16Index;
					return CS_SUCCESS;
				}
				u16Index ++;
				if ( 0 == m_CfgTrigParams[u16Index].u32UserTrigIndex )
				{
					*u16TriggerEngine = u16Index;
					return CS_SUCCESS;
				}
			}
		}
	}
	return CS_ALLTRIGGERENGINES_USED;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void CsSpiderDev::TraceCalib(uInt16 u16Channel, uInt32 u32TraceFlag, uInt16 u16Code, int32 i32Target, int32 i32Real)
{
	if( m_u32DebugCalibTrace != 0 )
	{
		char  szTemp[250];
		int32 i32Error = labs(i32Real - i32Target);

		i32Error *= 10000;
		i32Error /= labs(m_pCardState->AcqConfig.i32SampleRes);

		if( TRACE_CAL_HEADER  == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			OutputDebugString("\n");
			sprintf_s( szTemp, sizeof(szTemp), "Calibration range = �%d mV\tInput range = �%d mV Impedance %d\n\t\t\t\tCodeA: %4d\tCodeB: %4d\tCodeRef %4d\tVarRef %d uV Code Offset %d\n",
				m_u32CalibRange_uV/1000, i32Target, i32Real, m_u16CalCodeA, m_u16CalCodeB, m_u16VarRefCode, m_u32VarRefLevel_uV, m_pCardState->u16AdcOffsetAdjust);
			OutputDebugString(szTemp);
		}

		if( TRACE_CAL_COARSE_OFF  == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			i32Error = labs(i32Real) * 10000;
			i32Error /= labs(m_pCardState->AcqConfig.i32SampleRes);

			sprintf_s( szTemp, sizeof(szTemp), "== Coarse offset: Code %4d Real = %d\t Error = %d\n", u16Code, i32Real, i32Error);
			OutputDebugString(szTemp);
		}
		else if( TRACE_CAL_FINE_OFF  == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			sprintf_s( szTemp, sizeof(szTemp), "~~ Fine offset:   Code %4d Target = %d\t Real = %d\t Error = %d\n", u16Code, i32Target, i32Real, i32Error);
			OutputDebugString(szTemp);
		}
		else if( TRACE_CAL_GAIN  == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			sprintf_s( szTemp, sizeof(szTemp), "** Gain:          Code %4d Target = %d\t Real = %d\t Error = %d\n", u16Code, i32Target, i32Real, i32Error);
			OutputDebugString(szTemp);
		}
		else if( TRACE_CAL_POS  == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			sprintf_s( szTemp, sizeof(szTemp), "^^ Position:      Code %4d Target = %d\t Real = %d\t Error = %d\n", u16Code, i32Target, i32Real, i32Error);
			OutputDebugString(szTemp);
		}
		else if( TRACE_CAL_SIGNAL  == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			i32Error = labs(i32Real - i32Target);
			i32Error *= 10000;
			i32Error += 5000;
			i32Error /= m_u32CalibRange_uV;

			sprintf_s( szTemp, sizeof(szTemp),  "     Set level:     Code %4d Target = %+5d\t Real = %+d\t Error = %d ( Itter = %d)\n", u16Code, i32Target, i32Real, i32Error, u16Channel);
			OutputDebugString(szTemp);
		}
		else if( TRACE_DAC == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			sprintf_s( szTemp, sizeof(szTemp),  "Writing DAC %d with %d\n", i32Target, u16Code);
			OutputDebugString(szTemp);
		}
		else if( TRACE_CAL_RESULT == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			char szCode[50];
			switch (u16Code)
			{
				case 0: strcpy_s( szCode, sizeof(szCode), "Offset"); break;
				case 1: strcpy_s( szCode, sizeof(szCode), "Gain"); break;
				case 2: strcpy_s( szCode, sizeof(szCode), "Position"); break;
				default: strcpy_s( szCode, sizeof(szCode), "-----"); break;
			}
			if( 0 == i32Target )
				sprintf_s( szTemp, sizeof(szTemp), "!! %s calibration failed on channel %d\n", szCode, u16Channel);
			else
				sprintf_s( szTemp, sizeof(szTemp), "   %s calibration succeeded on channel %d\n", szCode, u16Channel);

			OutputDebugString(szTemp);
		}
		else if ( TRACE_CAL_SIGNAL_ITER == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			sprintf_s( szTemp, sizeof(szTemp), "== Cal: Target = %+7d uV  Iteration  %2d Code %4d Real = %+7d uV (Dif %+6d uV)\n", i32Target, u16Channel, u16Code, i32Real, i32Real - i32Target);
			OutputDebugString(szTemp);
		}
		else if ( TRACE_ZERO_OFFSET == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			sprintf_s( szTemp, sizeof(szTemp), "   Zero offset: Calib %d uV Front end %d uv (Target %d)\n", i32Target, i32Real, u16Code);
			OutputDebugString(szTemp);
		}
		else if ( TRACE_CAL_NOISE == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			sprintf_s( szTemp, sizeof(szTemp), "Cal noise:  Level %+8d uV  Mean square noise: %d.%d LSB\n", i32Target, i32Real/10, i32Real%10);
			OutputDebugString(szTemp);
		}
		else if( TRACE_CAL_PROGRESS == (u32TraceFlag &  m_u32DebugCalibTrace) )
		{
			if( 0 == u16Channel )
			{
				OutputDebugString("\n");
				sprintf_s( szTemp, sizeof(szTemp), "--- All channels calibration has %s\n", 0 == u16Code? "started." : "finished.\n");
			}
			else
			{
				if( 0 == u16Code )
				{
					OutputDebugString("\n");
					sprintf_s( szTemp, sizeof(szTemp), "\tChannel %d calibration has started.\n", u16Channel);
				}
				else
				{
					int32 i32Index = GetFEIndex(u16Channel);
					uInt32 u32RangeIndex = m_pCardState->AddOnReg.u32FeIndex[u16Channel-1];

					sprintf_s( szTemp, sizeof(szTemp), "\t%sChannel %d calibration has finished (code %d).\n\t\t\t\tCoarse: %4d (0x%04x)\tFine: %4d (0x%04x)\tGain: %d (0x%04x)\tPos: %4d (0x%04x)\n",
						0 == i32Target ? "" : "** ",
						(ConvertToUserChannel(u16Channel) + (m_u8CardIndex -1) * m_PlxData->CsBoard.NbAnalogChannel),
						i32Target > 0 ? i32Target : (-i32Target)%CS_ERROR_FORMAT_THRESHOLD,
						m_pCardState->DacInfo[u16Channel-1][i32Index][u32RangeIndex].u16CoarseOffset,
						m_pCardState->DacInfo[u16Channel-1][i32Index][u32RangeIndex].u16CoarseOffset,
						m_pCardState->DacInfo[u16Channel-1][i32Index][u32RangeIndex].u16FineOffset,
						m_pCardState->DacInfo[u16Channel-1][i32Index][u32RangeIndex].u16FineOffset,
						m_pCardState->DacInfo[u16Channel-1][i32Index][u32RangeIndex].u16Gain,
						m_pCardState->DacInfo[u16Channel-1][i32Index][u32RangeIndex].u16Gain,
						m_pCardState->DacInfo[u16Channel-1][i32Index][u32RangeIndex].u16CodeDeltaForHalfIR,
						m_pCardState->DacInfo[u16Channel-1][i32Index][u32RangeIndex].u16CodeDeltaForHalfIR);
				}
			}
			OutputDebugString(szTemp);
		}

	}
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsSpiderDev::MsCalibrateAllChannels()
{
	CsSpiderDev	*pDevice = m_MasterIndependent;
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

	// return the last error found
	return i32ErrorStatus;
}