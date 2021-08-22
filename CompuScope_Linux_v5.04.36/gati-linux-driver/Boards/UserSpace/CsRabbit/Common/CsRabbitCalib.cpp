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
#include "CsEventLog.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#else
#include "GageCfgParser.h"
#endif

#define	 CSRB_CALIB_TIMEOUT_LONG	KTIME_SECOND(1)			// 1 second timeout
#define	 CSRB_CALIB_TIMEOUT_SHORT	KTIME_MILISEC(300)		// 300 milisecond timeout


#define	 CSRB_CALIBMS_TIMEOUT	KTIME_MILISEC(500)
#define  CSRB_TRIGSENS_PULSE_CALIB				50
#define  EXTTRIGCALIB_SLOPE						CS_TRIG_COND_NEG_SLOPE
//-----------------------------------------------------------------------------
// This function is used only for Cobra PCI card.
//
// M/S calibration algorithm for PCI:
//    Find offset between Master and slave cards
//    Read memory using different offset
//-----------------------------------------------------------------------------

int32	CsRabbitDev::CalibrateMasterSlave()
{
	int32		i32Ret = CS_SUCCESS;
	int32		i32MsCalibRet = CS_SUCCESS;
	int32		MsAlignOffset[20] = {0};
	int32		i32SlaveCount = 0;
	CsRabbitDev *pDevice = m_MasterIndependent;
	CSTIMER		CsTimeout;

	if ( 1 == m_pSystem->SysInfo.u32BoardCount )
		return CS_SUCCESS;

	if ( ! m_MasterIndependent->m_pCardState->bMasterSlaveCalib )
		return CS_SUCCESS;

	m_MasterIndependent->ResetMasterSlave();

	if ( m_bNeverCalibrateMs )
		return CS_SUCCESS;

	while ( pDevice )
	{
		i32SlaveCount++;
		pDevice = pDevice->m_Next;
	}
	i32SlaveCount -=1;

	// Reset Ms align offset before calibration
	IoctlSetMasterSlaveCalibInfo( MsAlignOffset, i32SlaveCount );

	// Switch to Ms calib mode
	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		pDevice->SetupForSinglePulseCalib(TRUE);

		if ( pDevice->m_Next )
		{
			i32Ret = pDevice->SendCalibModeSetting( ConvertToHwChannel( CS_CHAN_1 ), ecmCalModeMs, 0 );
		}
		else
		{	// With default waiting time only on the last card
			i32Ret = pDevice->SendCalibModeSetting( ConvertToHwChannel( CS_CHAN_1 ), ecmCalModeMs );
		}

		if( CS_FAILED(i32Ret) )	
			return i32Ret;

		i32Ret = pDevice->SetupForCalibration(TRUE, ecmCalModeMs);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = pDevice->SetupForMsCalib_SourceChan1();
		if( CS_FAILED(i32Ret) )	goto OnExit;

		pDevice = pDevice->m_Next;
	}

	if ( m_MasterIndependent->m_pCardState->bMsDecimationReset )
		m_MasterIndependent->ResetMSDecimationSynch();

	timing(10000);

	CsTimeout.Set( CSRB_CALIBMS_TIMEOUT );

	// One acquisition ...
	m_MasterIndependent->AcquireData();
	while( STILL_BUSY == m_MasterIndependent->ReadBusyStatus() )
	{
		if ( CsTimeout.State() )
		{
			if( STILL_BUSY == m_MasterIndependent->ReadBusyStatus() )
			{
				::OutputDebugString("\nMaster/Slave Calib error. Trigger timeout");
				m_MasterIndependent->ForceTrigger();
				m_MasterIndependent->MsAbort();
				i32Ret =  CS_MASTERSLAVE_CALIB_FAILURE;
				break;
			}
		}
	}

	// Reset Master/Slave offset
	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		pDevice->m_i16MsAlignOffset = 0;
		pDevice = pDevice->m_Next;
	}

	// Detect zero crossing
	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		i32MsCalibRet = pDevice->CsDetectZeroCrossAddress_PCI();
		if( CS_FAILED(i32MsCalibRet) )	
		{
			// Assume that this card has the same offset as the master one
			pDevice->m_i16ZeroCrossOffset = m_MasterIndependent->m_i16ZeroCrossOffset;

			char	szText[128];
			sprintf_s( szText, sizeof(szText), "\nMaster/Slave Calib error. Zero crossing detection error on card %d", pDevice->m_pCardState->u8MsCardId);
			::OutputDebugString(szText);
			break;
		}

		pDevice = pDevice->m_Next;
	}

	if ( CS_SUCCEEDED(i32MsCalibRet) )
	{
		int32		i = 0;

		// Send M/S calib result down to kernel
		pDevice = m_MasterIndependent;
		while ( pDevice )
		{
			if ( pDevice != m_MasterIndependent )
			{
				MsAlignOffset[i++] = pDevice->m_i16ZeroCrossOffset - m_MasterIndependent->m_i16ZeroCrossOffset;
			}
			pDevice->m_i16MsAlignOffset = pDevice->m_i16ZeroCrossOffset - m_MasterIndependent->m_i16ZeroCrossOffset;
			TraceCalib(TRACE_CAL_MS, pDevice->m_pCardState->u16CardNumber, (uInt32) pDevice->m_i16ZeroCrossOffset,
									 0, pDevice->m_i16MsAlignOffset );	

			pDevice = pDevice->m_Next;
		}

		IoctlSetMasterSlaveCalibInfo( MsAlignOffset, i );

	}

OnExit:
	// Switch back to normal mode
	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		if ( pDevice->m_Next )
			i32Ret = pDevice->SendCalibModeSetting( ConvertToHwChannel( CS_CHAN_1 ), ecmCalModeNormal, 0 );
		else
		{	// With default waiting time only on the last card
			i32Ret = pDevice->SendCalibModeSetting( ConvertToHwChannel( CS_CHAN_1 ), ecmCalModeNormal );
		}
		if( CS_FAILED(i32Ret) )	
			return i32Ret;

		pDevice->SetupForCalibration(FALSE, ecmCalModeMs);
		pDevice->SetupForSinglePulseCalib(FALSE);
		pDevice = pDevice->m_Next;
	}

	return i32MsCalibRet;
}


//-----------------------------------------------------------------------------
// This function is used only for Cobra PCI card.
//-----------------------------------------------------------------------------

int32	CsRabbitDev::CsDetectZeroCrossAddress_PCI()
{
#define	MS_PULSE_VALIDATION_LENGTH	20
#define	MS_CALIB_LENGTH				128

	int32	i32Ret = CS_SUCCESS;
	int32	i32OffsetAdjust;
	uInt32	u32PosCrossing;

	m_i16ZeroCrossOffset = 0;

	in_PREPARE_DATATRANSFER		InXferEx ={0};
	out_PREPARE_DATATRANSFER	OutXferEx = {0};

	InXferEx.i64StartAddr	= MIN_EXTTRIG_ADJUST;
	InXferEx.u16Channel		= CS_CHAN_1;
	InXferEx.u32DataMask	= GetDataMaskModeTransfer(0);
	InXferEx.u32FifoMode	= GetFifoModeTransfer(CS_CHAN_1);
	InXferEx.bBusMasterDma	= m_BusMasterSupport;
	InXferEx.u32Segment		= 1;

	IoctlPreDataTransfer( &InXferEx, &OutXferEx );
	CardReadMemory( m_pu8CalibBuffer, CSRB_CALIB_BUFFER_SIZE );

	i32OffsetAdjust	= -1*OutXferEx.i32ActualStartOffset;

#ifdef DEBUG_MASTERSLAVE
	RtlCopyMemory( m_PulseData, &m_pu8CalibBuffer[i32OffsetAdjust], Min(PAGE_SIZE, CSRB_CALIB_BUFFER_SIZE) );
#endif

	// Calibration signal is a quare wave ~64 samples period (no-pingong) and ~128 sample period (pingpong)

	uInt32	u32SizeForCalib =  MS_CALIB_LENGTH << ( (m_pCardState->AddOnReg.u16PingPong > 0) ? 1 : 0);
	i32Ret = CsDetectLevelCrossAddress( &m_pu8CalibBuffer[i32OffsetAdjust], u32SizeForCalib, &u32PosCrossing );

	if ( CS_FAILED (i32Ret) )
		return CS_MASTERSLAVE_CALIB_FAILURE;

	m_i16ZeroCrossOffset = (int16) u32PosCrossing;

	return CS_SUCCESS;
}

#define		EXTTRIGCALIB_CROSSING_LEVEL		0x70
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::CsDetectLevelCrossAddress( void *pBuffer, uInt32 u32BufferSize, uInt32 *pu32PosCross, uInt8  u8CrossEdge /* = SINGLEPULSECALIB_SLOPE */ )
{
	int32  	i32Status = CS_SUCCESS;
	uInt32 u32Position = 0;
	uInt8* pu8Buffer	= (uInt8 *) pBuffer;
	uInt32 i = 0;
	uInt8  u8CrossLevel = 0x80;
	char	szText[128];

	if ( m_bNucleon && ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_SONOSCAN) ) )
		u8CrossLevel = 0xA0;
	else if ( ( ecmCalModeExtTrig == m_CalibMode || ecmCalModeExtTrigChan == m_CalibMode ) )
		u8CrossLevel = EXTTRIGCALIB_CROSSING_LEVEL;

	// Detection of 1rst raising edge
	if ( u8CrossEdge == CS_TRIG_COND_POS_SLOPE )
	{
		for ( i = 0; i < u32BufferSize; i++ )
		{
			if ( pu8Buffer[i] < u8CrossLevel && pu8Buffer[i+1] >= u8CrossLevel )
			{
				// Find "closest to zero" position
				if ( ( u8CrossLevel-pu8Buffer[i] ) < (pu8Buffer[i+1]-u8CrossLevel) )
					u32Position = i;
				else
					u32Position = i + 1;

				break;
			}
		}

		_ASSERT( i < u32BufferSize );
		if ( i >= u32BufferSize )
		{
			sprintf_s( szText, sizeof(szText), "Failed on detection of the CAL pulse\n");
			i32Status = CS_EXTTRIG_CALIB_FAILURE;		// Zero crossing not found
		}
		else
		{
			// From zero crossing position, double check if it is a square wave or just noise 
			for ( uInt32 n = u32Position+1; n < PULSE_VALIDATION_LENGTH; n++ )
			{
				// All samples must be above crossing level
				if (  pu8Buffer[n] <= u8CrossLevel )
				{
					sprintf_s( szText, sizeof(szText), "The CAL pulse is invalid\n");
					i32Status = CS_EXTTRIG_CALIB_FAILURE;		// Cal signal not present (triggered on noise)
					break;
				}
			}
		}
	}
	else
	{
		for ( i = 0; i < u32BufferSize; i++ )
		{
			if ( pu8Buffer[i] > u8CrossLevel && pu8Buffer[i+1] <= u8CrossLevel )
			{
				// Find "closest to zero" position
				if ( ( pu8Buffer[i]-u8CrossLevel ) < ( u8CrossLevel-pu8Buffer[i+1] ) )
					u32Position = i;
				else
					u32Position = i + 1;

				break;
			}
		}

		if ( i >= u32BufferSize )
		{
			sprintf_s( szText, sizeof(szText), "Failed on detection of the CAL pulse\n");
			i32Status = CS_EXTTRIG_CALIB_FAILURE;		// Zero crossing not found
		}
		else
		{
			// From zero crossing position, double check if it is a square wave or just noise 
			for ( uInt32 n = u32Position+1; n < PULSE_VALIDATION_LENGTH; n++ )
			{
				// All samples must be below crossing level
				if (  pu8Buffer[n] >= u8CrossLevel )
				{
					sprintf_s( szText, sizeof(szText), "The CAL pulse is invalid\n");
					i32Status = CS_EXTTRIG_CALIB_FAILURE;		// Cal signal not present (triggered on noise)
					break;
				}
			}
		}
	}

	if ( CS_FAILED(i32Status) )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_TEXT, szText );
	}

	if ( NULL != pu32PosCross )
		*pu32PosCross = (int16) u32Position;

	return i32Status;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::AcquireCalibSignal( uInt16 u16UserChannel, int32 i32StartPos, int32 *i32OffsetAdjust )
{
	// One acquisition ...
	AcquireData();

	CSTIMER	CsTimeout;
	CsTimeout.Set( CSRB_CALIB_TIMEOUT_SHORT );

	while( STILL_BUSY == ReadBusyStatus() )
	{
		if ( CsTimeout.State() )
		{
			if( STILL_BUSY == ReadBusyStatus() )
			{
				ForceTrigger();
				MsAbort();
				return	CS_EXTTRIG_CALIB_FAILURE;
			}
		}
	}

	in_PREPARE_DATATRANSFER		InXferEx ={0};
	out_PREPARE_DATATRANSFER	OutXferEx = {0};

	InXferEx.i64StartAddr	= i32StartPos;
	InXferEx.u16Channel		= u16UserChannel;
	InXferEx.u32DataMask	= GetDataMaskModeTransfer(0);
	InXferEx.u32FifoMode	= GetFifoModeTransfer(u16UserChannel);
	InXferEx.bBusMasterDma	= m_BusMasterSupport;
	InXferEx.bIntrEnable	= 0;
	InXferEx.u32Segment		= 1;

	IoctlPreDataTransfer( &InXferEx, &OutXferEx );
	CardReadMemory( m_pu8CalibBuffer, CSRB_CALIB_BUFFER_SIZE );

	if ( i32OffsetAdjust )
	{
		*i32OffsetAdjust =  -1*OutXferEx.i32ActualStartOffset;
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsRabbitDev::CsGetAverage(uInt16 u16Channel, uInt8* pu8Buffer, uInt32 u32BufferSize, int16* pi16OddAvg_x10, int16* pi16EvenAvg_x10)
{
	int32	i32Ret = CS_SUCCESS;
	
	in_PREPARE_DATATRANSFER		InXferEx ={0};
	out_PREPARE_DATATRANSFER	OutXferEx = {0};

	InXferEx.i64StartAddr	= m_TrigAddrOffsetAdjust;
	InXferEx.u16Channel		= u16Channel;
	InXferEx.u32DataMask	= GetDataMaskModeTransfer(0);
	InXferEx.u32FifoMode	= GetFifoModeTransfer(u16Channel);
	InXferEx.bBusMasterDma	= m_BusMasterSupport;
	InXferEx.bIntrEnable	= 0;
	InXferEx.u32Segment		= 1;

	i32Ret = IoctlPreDataTransfer( &InXferEx, &OutXferEx );
	if ( CS_FAILED (i32Ret) )
		return i32Ret;

	CardReadMemory( pu8Buffer, u32BufferSize );

	const uInt32 c32SampleOffset = m_pCardState->AcqConfig.i32SampleOffset;
	int32  i32SumEven = 0;
	int32  i32SumOdd = 0;
	int32  i32Count = 0;

	if ( m_bHardwareAveraging )
	{
		// Signed data format
		int8	*pi8Buffer = (int8 * )pu8Buffer;

		if ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE )
		{
			// Data format from HW AVG
			// | Ch1 | Ch1 | Ch1 | Ch1 | Ch1 | Ch1 | Ch1 | Ch1 |
			for (uInt32 u32Addr = CSRB_CALIB_BUF_MARGIN;  u32Addr < u32BufferSize; i32Count++)
			{
				i32SumEven = i32SumEven + int32(pi8Buffer[u32Addr++]) - c32SampleOffset;
				i32SumOdd  = i32SumOdd  + int32(pi8Buffer[u32Addr++]) - c32SampleOffset;
			}
		}
		else
		{
			uInt32	j = 0;

			// Data format from HW AVG
			// | Ch1 | Ch1 | Ch1 | Ch1 | Ch2 | Ch2 | Ch2 | Ch2 |
			j = 8*(CSRB_CALIB_BUF_MARGIN/4) + (CSRB_CALIB_BUF_MARGIN % 4);
			j += ((ConvertToHwChannel(u16Channel)) == CS_CHAN_1) ? 0 : 4;

			for (uInt32 u32Addr = CSRB_CALIB_BUF_MARGIN;  u32Addr < u32BufferSize; )
			{
				i32SumEven = i32SumEven + int32(pi8Buffer[j++]) - c32SampleOffset;
				i32SumOdd  = i32SumOdd  + int32(pi8Buffer[j++]) - c32SampleOffset;

				i32SumEven = i32SumEven + int32(pi8Buffer[j++]) - c32SampleOffset;
				i32SumOdd  = i32SumOdd  + int32(pi8Buffer[j++]) - c32SampleOffset;

				j += 4;
				i32Count += 2;
				u32Addr += 8;
			}
		}
	}
	else
	{
		if ( -1 == m_pCardState->AcqConfig.i32SampleOffset )
		{
			// Signed data format
			int8	*pi8Buffer = (int8 * )pu8Buffer;

			for (uInt32 u32Addr = BUNNY_CALIB_BUF_MARGIN;  u32Addr < u32BufferSize; i32Count++)
			{
				i32SumEven = i32SumEven + int32(pi8Buffer[u32Addr++]);
				i32SumOdd  = i32SumOdd  + int32(pi8Buffer[u32Addr++]);
			}
		}
		else
		{
			// Unsigned data format
			for (uInt32 u32Addr = BUNNY_CALIB_BUF_MARGIN;  u32Addr < u32BufferSize; i32Count++)
			{
				i32SumEven = i32SumEven + int32(pu8Buffer[u32Addr++]) - c32SampleOffset;
				i32SumOdd  = i32SumOdd  + int32(pu8Buffer[u32Addr++]) - c32SampleOffset;
			}
		}
	}

	*pi16OddAvg_x10  = (int16) ( i32SumOdd*10  / i32Count );  
	*pi16EvenAvg_x10 = (int16) ( i32SumEven*10 / i32Count );  
	
	return CS_SUCCESS;
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


const uInt16 CsRabbitDev::c_u16R8   = 1000;
const uInt16 CsRabbitDev::c_u16R10  =  976;
const uInt16 CsRabbitDev::c_u16R19  =   50;
const uInt16 CsRabbitDev::c_u16R26  = 1000;
const uInt16 CsRabbitDev::c_u16R28  = 1000;
const uInt16 CsRabbitDev::c_u16R165 = 1000;
const uInt16 CsRabbitDev::c_u16R171 =  470;
const uInt16 CsRabbitDev::c_u16R212 = 1500;
const uInt16 CsRabbitDev::c_u16R220 = 2000;

const uInt32 CsRabbitDev::c_u32MaxCalVoltage_mV = 7000;
const uInt16 CsRabbitDev::c_u16CalLevelSteps = 256;

const uInt16 CsRabbitDev::c_u16Vref_mV = 2500;
const uInt16 CsRabbitDev::c_u16CalDacCount = 4096;
const uInt32 CsRabbitDev::c_u32RefAdcCount = 65536;
const uInt16 CsRabbitDev::c_u16CalAdcCount = 4096;

const uInt32 CsRabbitDev::c_u32ScaleFactor = 10000;
const uInt32 CsRabbitDev::c_u32K_scaled = uInt32((LONGLONG(CsRabbitDev::c_u32ScaleFactor) * CsRabbitDev::c_u16R165 * (CsRabbitDev::c_u16R171 + CsRabbitDev::c_u16R220)) / ((CsRabbitDev::c_u16R212 + CsRabbitDev::c_u16R165) * CsRabbitDev::c_u16R171));
const uInt32 CsRabbitDev::c_u32D_scaled = uInt32((LONGLONG(CsRabbitDev::c_u32ScaleFactor) * CsRabbitDev::c_u16R220 ) / CsRabbitDev::c_u16R171);
const uInt32 CsRabbitDev::c_u32MaxCalRange_mV = (CsRabbitDev::c_u32MaxCalVoltage_mV*CS_REAL_IMP_50_OHM)/(CsRabbitDev::c_u16R19 + CS_REAL_IMP_50_OHM);
const uInt32 CsRabbitDev::c_u32MinCalRef_uV = 200000;

const uInt32 CsRabbitDev::c_u32OffsetCalibrationRange_mV = 400;
const uInt16 CsRabbitDev::c_u16MaxIterations = 32;

int32 CsRabbitDev::SetCalibrationRange(uInt32 u32Range_mV)
{
	uInt32 u32ActualImpedance = CS_REAL_IMP_50_OHM;

	if( u32Range_mV < c_u32OffsetCalibrationRange_mV )
		u32Range_mV = c_u32OffsetCalibrationRange_mV;

	u32Range_mV /= 2; //make it unipolar
	
	if( u32Range_mV > c_u32MaxCalRange_mV )
		u32Range_mV = c_u32MaxCalRange_mV;

	const LONGLONG	llFactor = 1000;
	const LONGLONG cllU37Swing = (llFactor * u32Range_mV * (c_u16R19 + u32ActualImpedance))/u32ActualImpedance;
	
	//  B = (M * Fe* R10)/(Vref* Min(K, D-K)*(R8+R10))
	ASSERT ( c_u32D_scaled >= c_u32K_scaled);
	uInt32 u32Min_scaled = c_u32K_scaled < c_u32D_scaled - c_u32K_scaled ? c_u32K_scaled : c_u32D_scaled - c_u32K_scaled;

	const uInt32   cu32MinRef_uV = uInt32((cllU37Swing * c_u32ScaleFactor)/u32Min_scaled)>c_u32MinCalRef_uV ? uInt32((cllU37Swing * c_u32ScaleFactor)/u32Min_scaled) : c_u32MinCalRef_uV;
	const LONGLONG cllFeSwing_scaled = cllU37Swing * c_u32ScaleFactor * c_u16CalDacCount *  c_u16R10;
	const LONGLONG cllDenom_scaled = llFactor * u32Min_scaled * c_u16Vref_mV *(c_u16R10 + c_u16R8);

	uInt16 u = 0;
	int32 i32Ret = CS_SUCCESS;
	if( m_u32CalibRange_mV != u32Range_mV )
	{
		//referense should be bigger then swing but just
		uInt16 u16CodeB = uInt16(cllFeSwing_scaled/cllDenom_scaled) <c_u16CalDacCount-1 ? uInt16(cllFeSwing_scaled/cllDenom_scaled):c_u16CalDacCount-1;
		m_u32VarRefLevel_uV = 0;
		for(; (u < c_u16MaxIterations) && (m_u32VarRefLevel_uV < cu32MinRef_uV); u++)
		{
			m_u16CalCodeB = u16CodeB;
			i32Ret = WriteToDac(BUNNY_VAR_REF, m_u16CalCodeB);
			if( CS_FAILED(i32Ret) )	return i32Ret;
		
			i32Ret = WriteRegisterThruNios( CSRB_REF_ADC_CMD_RESET, CSRB_REF_ADC_WR_CMD);
			if( CS_FAILED(i32Ret) )	return i32Ret;

			// MeasVref_u55 = 2 * (CodeU36/Mu36 )*Vref unipolar ADC
			uInt32 u32CodeU1 = ReadRegisterFromNios(CSRB_REF_ADC_CMD_RD, CSRB_REF_ADC_WR_CMD);
			u32CodeU1 &= 0xffff;
			if( 0 == u32CodeU1 )
			{
				return CS_CALIB_MEAS_ADC_FAILURE;
			}
			// Vvr = (2*U1code)*Vr/ MU1
			m_u32VarRefLevel_uV = uInt32(( llFactor * 2 * LONGLONG(c_u16Vref_mV) * LONGLONG(u32CodeU1))/LONGLONG(c_u32RefAdcCount));
			m_u32CalibSwing_uV = uInt32((LONGLONG(m_u32VarRefLevel_uV) * u32Min_scaled * u32ActualImpedance)/(LONGLONG(c_u32ScaleFactor)*(c_u16R19 + u32ActualImpedance)));

			if( m_u32VarRefLevel_uV > 0 )
			{
				uInt32 u32Code = uInt32( (LONGLONG(cu32MinRef_uV)*u16CodeB + m_u32VarRefLevel_uV/2) /LONGLONG(m_u32VarRefLevel_uV));
				u16CodeB = u32Code < c_u16CalDacCount ? uInt16(u32Code) : c_u16CalDacCount-1;
				if( m_u16CalCodeB == u16CodeB )
					u16CodeB++;
				u16CodeB = u16CodeB < c_u16CalDacCount ? u16CodeB : c_u16CalDacCount-1;
				if( m_u16CalCodeB == u16CodeB )
					break;
			}
			else
				return CS_CALIB_MEAS_ADC_FAILURE;
		}
			
		m_u32CalibRange_mV = u32Range_mV;
	}

	TraceCalib(TRACE_CAL_RANGE, u, 0, 0, 0 ); 
	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

bool CsRabbitDev::FastCalibrationSettings(uInt16 u16HwChannel, bool bForceSet )
{
	uInt16 u16ChanZeroBased = u16HwChannel - 1;
	uInt32 u32Index;
	int32 i32Ret = FindFeIndex( m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange, u32Index );
	if( CS_FAILED(i32Ret) )	return false;

	uInt16 u16DacOffset = u16HwChannel == CS_CHAN_1 ? 0 : BUNNY_2_CHAN_DAC;
			
	if ( bForceSet ||
		( m_pCardState->DacInfo[u16ChanZeroBased][u32Index].bValid  && ! m_bFastCalibSettingsDone[u16ChanZeroBased] ) )
	{
		i32Ret = WriteToDac(BUNNY_COARSE_OFF_1 + u16DacOffset, m_pCardState->DacInfo[u16ChanZeroBased][u32Index]._NullOffset);
		if( CS_FAILED(i32Ret) )	return false;

		i32Ret = WriteToDac(BUNNY_POSITION_1 + u16DacOffset, m_pCardState->DacInfo[u16ChanZeroBased][u32Index]._FeOffset);
		if( CS_FAILED(i32Ret) )	return false;

		i32Ret = WriteToDac(BUNNY_ADC_GAIN_1 + u16DacOffset, m_pCardState->DacInfo[u16ChanZeroBased][u32Index]._Gain, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )	return false;

		// in PingPong make the both gains equal
		if ( 0 != m_pCardState->AddOnReg.u16PingPong )
		{
			u16DacOffset = BUNNY_2_CHAN_DAC - u16DacOffset;
			i32Ret = WriteToDac(BUNNY_ADC_GAIN_1 + u16DacOffset, m_pCardState->DacInfo[u16ChanZeroBased][u32Index]._Gain, m_u8CalDacSettleDelay);
			if( CS_FAILED(i32Ret) )	return false;
		}

		i32Ret = SendPosition( u16HwChannel, m_pCardState->ChannelParams[u16ChanZeroBased].i32Position );
		if( CS_FAILED(i32Ret) )	return false;
		
		m_bFastCalibSettingsDone[u16ChanZeroBased] = TRUE;
		return true;
	}
	return false;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void CsRabbitDev::SetFastCalibration(uInt16 u16Channel)
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
		uInt32 u32Index;
		const uInt16 u16ChanZeroBased = u16Channel-1;
		int32 i32Ret = FindFeIndex( m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange, u32Index );
		if( CS_FAILED(i32Ret) )	return;
		m_pCardState->DacInfo[u16ChanZeroBased][u32Index].bValid = true;
	}
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool CsRabbitDev::ForceFastCalibrationSettingsAllChannels()
{
	bool bRetVal = true;

	if ( ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL ) || ( m_pCardState->bLAB11G ) )
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

	// Waiting for relays settled
	BBtiming(500);

	return bRetVal;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsRabbitDev::SetupForExtTrigCalib_SourceChan1()
{
	uInt16 u16Channel		= CS_CHAN_1;
	uInt16 u16TriggerSource = CS_TRIG_SOURCE_CHAN_1;

	if ( m_bSingleChannel2 )
	{
		u16Channel			= CS_CHAN_2;
		u16TriggerSource	= CS_TRIG_SOURCE_CHAN_2;
	}
	int32 i32Ret = SendCalibModeSetting( ConvertToHwChannel( u16Channel ), ecmCalModeExtTrigChan );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	if ( m_bNucleon && ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_SONOSCAN) ) )
	{
		i32Ret = SendTriggerEngineSetting( u16TriggerSource, CS_TRIG_COND_POS_SLOPE, 30 );                                               
	}
	else
	{
		i32Ret = SendTriggerEngineSetting( u16TriggerSource, SINGLEPULSECALIB_SLOPE, EXTTRIGCALIB_LEVEL_CHAN );   
	}
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendExtTriggerSetting( FALSE,
						            0, 
						            SINGLEPULSECALIB_SLOPE,
						            m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange,
						            CS_REAL_IMP_1M_OHM,
						            CS_COUPLING_AC,
						            EXTTRIGCALIB_SENSITIVE_1V);
	if( CS_FAILED(i32Ret) ) return i32Ret;

	timing(2000);

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsRabbitDev::SetupForExtTrigCalib_SourceExtTrig()
{
	int32	i32Ret = CS_SUCCESS;
	int32	i32Level = 0;
	uInt16	u16Channel		= m_bSingleChannel2 ? CS_CHAN_2 : CS_CHAN_1;

	i32Ret = SendCalibModeSetting( ConvertToHwChannel( u16Channel ), ecmCalModeExtTrigChan );
	if( CS_FAILED(i32Ret) )	return i32Ret;

  	i32Ret = SendTriggerEngineSetting( CS_TRIG_SOURCE_DISABLE );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	if ( m_bNucleon && ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_SONOSCAN) ) )
	{	                  
		i32Level = 25;
		i32Ret = SendExtTriggerSetting( TRUE,
										i32Level,
										SINGLEPULSECALIB_SLOPE,		// m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32Condition,
										CS_GAIN_2_V,
										CS_REAL_IMP_1M_OHM, //m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance,
										CS_COUPLING_AC,
										( CS_GAIN_10_V == m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange && !m_bLowExtTrigCalib ) ? EXTTRIGCALIB_SENSITIVE_5V : EXTTRIGCALIB_SENSITIVE_1V);
	}
	else
	{
	i32Level =  ( CS_REAL_IMP_50_OHM == m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance )? EXTTRIGCALIB_LEVEL_EXT_50 : EXTTRIGCALIB_LEVEL_EXT_HiZ;
	if ( CS_GAIN_10_V == m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange  && !m_bLowExtTrigCalib)
		i32Level /= 4;

//   	i32Ret = SendTriggerEngineSetting( CS_TRIG_SOURCE_DISABLE );
//	if( CS_FAILED(i32Ret) )	return i32Ret;
                                   
	i32Ret = SendExtTriggerSetting( TRUE,
						            i32Level,
						            SINGLEPULSECALIB_SLOPE,		// m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32Condition,
						            m_bLowExtTrigCalib? CS_GAIN_2_V:m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange,
						            CS_REAL_IMP_1M_OHM, //m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance,
						            CS_COUPLING_AC,
						            ( CS_GAIN_10_V == m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange && !m_bLowExtTrigCalib ) ? EXTTRIGCALIB_SENSITIVE_5V : EXTTRIGCALIB_SENSITIVE_1V);
	}
	if( CS_FAILED(i32Ret) )	return i32Ret;

	// Waiting for the relay switch
	BBtiming(3000);

	return CS_SUCCESS;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsRabbitDev::SetupForMsCalib_SourceChan1()
{
	SendTriggerEngineSetting( CS_TRIG_SOURCE_CHAN_1, SINGLEPULSECALIB_SLOPE, m_i32MsCalibTrigLevel );             
	BBtiming(2000);
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsRabbitDev::SetupForSinglePulseCalib( bool bSetup )
{
	int32	i32Ret = CS_SUCCESS;
	uInt16 u16Channel		= CS_CHAN_1;
	uInt16 u16TriggerEngine = CSRB_TRIGENGINE_A1;

	if ( m_bSingleChannel2 )
	{
		u16Channel = CS_CHAN_2;
		u16TriggerEngine = CSRB_TRIGENGINE_A2;
	}

	if( bSetup )
	{
		SendDigitalTriggerSensitivity( u16TriggerEngine, SINGLEPULSECALIB_SENSITIVE );
		// Backup the current trigger settings then switch to the default one
		RtlCopyMemory( m_OldChanParams, m_pCardState->ChannelParams, sizeof(m_OldChanParams));

		int32	i32Coupling = CS_COUPLING_AC;
		int32	i32InputRange = 1000;

		if ( m_bNucleon && ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_SONOSCAN) ) )
		{
			i32Coupling = CS_COUPLING_DC;
			i32InputRange = 2000;
		}

		i32Ret = SendChannelSetting( ConvertToHwChannel(u16Channel), i32InputRange, i32Coupling );
	}
	else
	{
		// Backup the current trigger settings then switch to the default one
		RtlCopyMemory( m_pCardState->ChannelParams, m_OldChanParams, sizeof(m_pCardState->ChannelParams));

		// Restore channel setttings on user channel 1
		SendDigitalTriggerSensitivity( u16TriggerEngine, m_u8TrigSensitivity );
		uInt16 n = ConvertToHwChannel(u16Channel) - 1;
		i32Ret = SendChannelSetting( ConvertToHwChannel(u16Channel), m_pCardState->ChannelParams[n].u32InputRange,
												   m_pCardState->ChannelParams[n].u32Term,
												   m_pCardState->ChannelParams[n].u32Impedance,
												   m_pCardState->ChannelParams[n].i32Position,
											       m_pCardState->ChannelParams[n].u32Filter );
	}

	i32Ret = CalibrateAllChannels();

	return i32Ret;
}


//-----------------------------------------------------------------------------
// This function is used only for Cobra PCI card.
//-----------------------------------------------------------------------------

int32	CsRabbitDev::CalibrateExtTrigger()
{
	int32		i32Ret = CS_SUCCESS;
	int32		i32ExtTrigCalibRet = CS_SUCCESS;

	if ( 0 == (m_pCardState->AddOnReg.u16ExtTrigEnable & CSRB_SET_EXT_TRIGGER_ENABLED ) )
		return CS_SUCCESS;

	if ( ((CsRabbitDev *)m_MasterIndependent)->m_bNeverCalibExtTrig )
		return CS_SUCCESS;

	if ( ((CsRabbitDev *)m_MasterIndependent)->m_pCardState->bExtTrigCalib == FALSE )
		return AdjustOffsetChannelAndExtTrigger();

	SetupForSinglePulseCalib(true);

	i32Ret = SetupForCalibration(true, ecmCalModeExtTrig );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	uInt16	u16RetryCount = 1;
	while ( u16RetryCount > 0 )
	{
		i32ExtTrigCalibRet = CalculateExtTriggerSkew();
		if ( CS_SUCCEEDED(i32ExtTrigCalibRet) )
			break;
		u16RetryCount--;
	}
	i32Ret = SendCalibModeSetting( ConvertToHwChannel( CS_CHAN_1 ));
	if( CS_FAILED(i32Ret) )	
	{
		SetupForCalibration(false, ecmCalModeExtTrig );
		return i32Ret;
	}

	i32Ret = SetupForCalibration(false, ecmCalModeExtTrig );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	SetupForSinglePulseCalib(false);

	i32ExtTrigCalibRet = AdjustOffsetChannelAndExtTrigger();
	if ( CS_SUCCEEDED(i32ExtTrigCalibRet) )
		m_pCardState->bExtTrigCalib = false;

	return i32ExtTrigCalibRet;
}


//-----------------------------------------------------------------------------
// This function is only for new Nucleon PCIe card.
//-----------------------------------------------------------------------------

int32	CsRabbitDev::CalculateExtTriggerSkew()
{
	int32		i32Ret = CS_SUCCESS;
	int32		i32OffsetAdjust;
	int32		i32StartPos;

	// Set ExtTrig address to middle position
	SetDigitalDelay (0);

	if ( IsExpertAVG() )
		i32StartPos = (int32) (-1*(m_i64HwSegmentSize - m_i64HwPostDepth));
	else
		i32StartPos = m_bNucleon ? MIN_DIGITALDELAY_ADJUST : 2*MIN_EXTTRIG_ADJUST;

	// Trigger from Channel 1
	SetupForExtTrigCalib_SourceChan1();

	// Acquire the calib signal
	i32Ret = AcquireCalibSignal( CS_CHAN_1, i32StartPos ,&i32OffsetAdjust );
	if( CS_FAILED(i32Ret) )	
	{
		OutputDebugString("Failed to capture pulse using channel trigger.\n");
		char	szText[128];
		sprintf_s( szText, sizeof(szText), TEXT("Src ch. ExtTrig range %d mV imped. %s."), 
			m_OldTrigParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange, 
			50 == m_OldTrigParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance? TEXT("50 Ohm"):TEXT("HiZ"));
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_EXTTRIG_CALIB_FAILED, szText );
		return i32Ret;
	}

	// Only for Mulrec AVG expert firmware.
	// In calib mode the data is in signed format. All calib bration algorithm assum data is in unsigned format
	// Convert ot unsigned data format so that we can call functions for calibration.
	if ( m_bMulrecAvgTD )
	{
		int8 *pi8Data = (int8 *) m_pu8CalibBuffer;

		for ( int i = 0; i < i32OffsetAdjust+EXTTRIG_CALIB_LENGTH; i ++ )
		{
			m_pu8CalibBuffer[i] = (uInt8)(pi8Data[i] + 0x80);
		}
	}
	
	if ( m_bNucleon && ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_SONOSCAN) ) )
		i32Ret = CsDetectLevelCrossAddress( &m_pu8CalibBuffer[i32OffsetAdjust], EXTTRIG_CALIB_LENGTH, &m_u32ChanPosCross, CS_TRIG_COND_POS_SLOPE );
	else
		i32Ret = CsDetectLevelCrossAddress( &m_pu8CalibBuffer[i32OffsetAdjust], EXTTRIG_CALIB_LENGTH, &m_u32ChanPosCross );
	if( CS_FAILED(i32Ret) )	
		return i32Ret;
	
	RtlCopyMemory( m_pu8DebugBuffer1, &m_pu8CalibBuffer[i32OffsetAdjust], PAGE_SIZE );

	// Switch to Ext Trigger
	SetupForExtTrigCalib_SourceExtTrig();
	
	// For unknown reason, sometime the first acquisition with Ext Trigger does not trigger at all.
	// The second acquisition works. That is why we have double call of AcquireCalibSignal
	i32Ret = AcquireCalibSignal( CS_CHAN_1, i32StartPos, &i32OffsetAdjust );
	if( CS_FAILED(i32Ret) )	
	i32Ret = AcquireCalibSignal( CS_CHAN_1, i32StartPos, &i32OffsetAdjust );

	if( CS_FAILED(i32Ret) )	
	{
		OutputDebugString("Failed to capture pulse using external trigger\n");
		char	szText[128];
		sprintf_s( szText, sizeof(szText), TEXT("Src ext. ExtTrig range %d mV imped. %s."), 
			m_OldTrigParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange, 
			50 == m_OldTrigParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance? TEXT("50 Ohm"):TEXT("HiZ"));
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_EXTTRIG_CALIB_FAILED, szText );
		return i32Ret;
	}

	if ( m_bHardwareAveraging && (0 !=( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL)) )
	{
		// Data in HWAVG mode 
		// | Ch1 | Ch1 | Ch1 | Ch1 | Ch2 | Ch2 | Ch2 | Ch2 |

		int i = 0;
		int j = 0;

		// Removed data from the other channel
		j = ((ConvertToHwChannel(CS_CHAN_1)) == CS_CHAN_1) ? 0 : 4;

		int8	*pi8Channel = (int8 *) &m_pu8CalibBuffer[i32OffsetAdjust];
		int8	*pi8Raw = (int8 *) &m_pu8CalibBuffer[i32OffsetAdjust];

		for ( ; i < CSRB_CALIB_BUF_MARGIN/2; )
		{
			RtlCopyMemory( &pi8Channel[i], &pi8Raw[j], 4);
			i += 4;
			j += 8;
		}
	}
	
	// Only for Mulrec AVG expert firmware.
	// In calib mode the data is in signed format. All calibration algorithm assum data is in unsigned format
	// Convert ot unsigned data format so that we can call functions for calibration.
	if ( m_bMulrecAvgTD )
	{
		int8 *pi8Data = (int8 *) m_pu8CalibBuffer;

		for ( int i = 0; i < i32OffsetAdjust+EXTTRIG_CALIB_LENGTH; i ++ )
		{
			m_pu8CalibBuffer[i] = (uInt8)(pi8Data[i] + 0x80);
		}
	}

	i32Ret = CsDetectLevelCrossAddress( &m_pu8CalibBuffer[i32OffsetAdjust], EXTTRIG_CALIB_LENGTH, &m_u32PosCross );
	if( CS_FAILED(i32Ret) )	
		return i32Ret;

	RtlCopyMemory( m_pu8DebugBufferEx, &m_pu8CalibBuffer[i32OffsetAdjust], PAGE_SIZE );
	m_pCardState->i16ExtTrigSkew = (int16)(m_u32ChanPosCross - m_u32PosCross);
	m_MasterIndependent->m_pCardState->bExtTrigCalib = false;

	TraceCalib(TRACE_CAL_EXTTRIG, m_u32ChanPosCross, m_u32PosCross, m_pCardState->i16ExtTrigSkew, 0 );

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
// This function is used only for Cobra PCI card.
//-----------------------------------------------------------------------------
int32	CsRabbitDev::AdjustOffsetChannelAndExtTrigger()
{
	int32		i32Ret = CS_SUCCESS;
	int16		i16MaxAjust = MAX_EXTTRIG_ADJUST;
	int16		i16MinAjust = MIN_EXTTRIG_ADJUST;
	BOOLEAN		bDoAdjustment = TRUE;

	// In pingpong, we have 32 more samples for the span (16 left, 16 right)
	if ( ! m_pCardState->AddOnReg.u16PingPong )
	{	
		i16MaxAjust /= 2;
		i16MinAjust /= 2;
	}

	int16 i16ExtTrigOffset = (int16)(m_u32PosCross - m_u32ChanPosCross);

	i16ExtTrigOffset += GetDataPathOffsetAdjustment();

	TraceCalib(TRACE_CAL_EXTTRIG, m_u32ChanPosCross, m_u32PosCross, i16ExtTrigOffset, 0 );

	if ( i16ExtTrigOffset < i16MinAjust  || i16ExtTrigOffset > i16MaxAjust )
	{
		char szErrorStr[128];
		sprintf_s(szErrorStr, sizeof(szErrorStr), "%d", i16ExtTrigOffset );
		CsEventLog EventLog;
		EventLog.Report( CS_WARN_EXTTRIG_CALIB_FAILED, szErrorStr );
		OutputDebugString("ExtTrigAddrOffset is out of range supported.");

		// Adjust the best we can ...
		if ( i16ExtTrigOffset  > i16MaxAjust )
			i16ExtTrigOffset = i16MaxAjust;
		else if ( i16ExtTrigOffset  < i16MinAjust )
			i16ExtTrigOffset = i16MinAjust;

		if ( m_MasterIndependent->m_bNoAdjExtTrigFail )
			bDoAdjustment = FALSE;
	}

	// For now the Register for adjusting ExtTrig Address offset accept 64 values.
	// It allows adjutinnf +/- 16(32) position for non-pingpong and pingpong mode
	// Pos 16(32) is the middle position.
	//
	//	settings	shift Normal/Pingpong
	//	0 			16/32
	//	1			16/33
	//	2			17/34
	//	3			17/35
	//	4			18/36
	//	.....
	//  30			31/62
	//	31			31/63
	//	32			00/00
	//	33			00/01
	//	34			01/02
	//	....
	//  62			15/30
	//	63			15/31
	//
	// When send a value < 32 it moves exttrigger address offset to the right side
	// When send a value >= 32 it moves exttrigger address offset to the left side

	if ( bDoAdjustment )
	{
		if ( ! m_pCardState->AddOnReg.u16PingPong )
			i16ExtTrigOffset = (int16) (2*i16ExtTrigOffset);

		SetDigitalDelay( i16ExtTrigOffset );
	}

	return i32Ret;
}




#define EXTTRIG_DATAPATH_ADJ_2V_50OHM		-2
#define EXTTRIG_DATAPATH_ADJ_2V_HiZ			0
#define EXTTRIG_DATAPATH_ADJ_10V_50OHM		-8
#define EXTTRIG_DATAPATH_ADJ_10V_HiZ		-1

//-----------------------------------------------------------------------------
//  Adjustment for different data path between PULSE Calib and REAL signal input
//
//	
//-----------------------------------------------------------------------------

int16 CsRabbitDev::GetDataPathOffsetAdjustment()
{
	if ( m_bZeroExtTrigDataPathAdjust )
		return 0;

	if ( CS_GAIN_2_V == m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange )
	{
		if ( CS_REAL_IMP_50_OHM == m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance )
			return EXTTRIG_DATAPATH_ADJ_2V_50OHM;
		else
			return EXTTRIG_DATAPATH_ADJ_2V_HiZ;
	}
	else
	{
		if ( CS_REAL_IMP_50_OHM == m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance )
			return EXTTRIG_DATAPATH_ADJ_10V_50OHM;
		else
			return EXTTRIG_DATAPATH_ADJ_10V_HiZ;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsRabbitDev::TraceCalib(uInt32 u32Id , uInt32 u32V1, uInt32 u32V2, int32 i32V3 , int32 i32V4)
{
	if( 0 == m_u32DebugCalibTrace )
		return;

	char  szTemp[250];
	char  szT[25] = "";

	u32Id &= m_u32DebugCalibTrace;
	switch( u32Id )
	{
		case TRACE_ZERO_OFFSET:
		{
			sprintf_s( szTemp, sizeof(szTemp),  ":: Initial Amp offset %d uV Final %d uV Code %4d (0x%03x)\n",i32V3, i32V4, u32V1, u32V1);
			OutputDebugString( szTemp );
		}
		break;
		case TRACE_ZERO_OFFSET_IT:
		{
			sprintf_s( szTemp, sizeof(szTemp),  "   ::: Amp offset %+5d uV ( %+5d uV ) Code %4d (%4d)\n",i32V3, i32V4, u32V1, u32V2);
			OutputDebugString( szTemp );
		}
		break;
		case TRACE_CAL_FE_OFFSET:
		{
			sprintf_s( szTemp, sizeof(szTemp),  "== Initial Fe offset %c%ld.%ld lsb Final %c%ld.%ld lsb Code %4d (0x%03x)\n",
				i32V3<0?'-':' ',labs(i32V3)/10, labs(i32V3)%10, i32V3<0?'-':' ',labs(i32V4)/10, labs(i32V4)%10, u32V1, u32V1);
			OutputDebugString( szTemp );
		}
		break;
		case TRACE_CAL_GAIN:
		{
			sprintf_s( szTemp, sizeof(szTemp),  "~~ Gain cal. Pos target %d ( %4d ) Neg Target %d ( %4d ) Result code %4d\n",i32V3, u32V1, i32V4, u32V2, (u32V1 + u32V2)/2);
			OutputDebugString( szTemp );
		}
		break;
		case TRACE_CAL_POS:
		{
			sprintf_s( szTemp, sizeof(szTemp),  "^^ Position cal. P.avg %d.%d code %4d N.avg -%d.%d code %4d  Result code %4d\n", 
				i32V3/10, i32V3%10, u32V1, -i32V4/10, (-i32V4)%10, u32V2, (int16(u32V2)-int16(u32V1))/2);
			OutputDebugString( szTemp );
		}
		break;
		case TRACE_CAL_PINGPONG_IT:
		{
			static int32 si32Dif;
			if( 1 == u32V1 )
			{
				si32Dif = i32V4 - i32V3;
				sprintf_s( szTemp, sizeof(szTemp),  "  -- Ping pong. Code %4d Even %c%ld.%ld. Odd %c%ld.%ld\n", u32V2, i32V3<0?'-':' ',labs(i32V3)/10, labs(i32V3)%10, i32V4<0?'-':' ',labs(i32V4)/10, labs(i32V4)%10);
			}
			else if( 2 == u32V1 )
			{
				si32Dif += i32V3 - i32V4;
				sprintf_s( szTemp, sizeof(szTemp),  "  ++ Ping pong. Code %4d Even %c%ld.%ld. Odd %c%ld.%ld  Dif %ld\n", u32V2, i32V3<0?'-':' ',labs(i32V3)/10, labs(i32V3)%10, i32V4<0?'-':' ',labs(i32V4)/10, labs(i32V4)%10, labs(si32Dif));
			}
			else
				strcpy_s( szTemp, sizeof(szTemp),  "       Ping pong. Empty trace\n" );
			OutputDebugString( szTemp );
		}
		break;
		case TRACE_CAL_PINGPONG:
		{
			sprintf_s( szTemp, sizeof(szTemp),  " -/+ Ping pong. Code %4d   Dif %d\n", u32V2, i32V3);
			OutputDebugString( szTemp );
		}
		break;
		case TRACE_CAL_RESULT:
		{
			if( CS_SUCCEEDED(i32V3) )
			{
				uInt32 u32Index;
				FindFeIndex( m_pCardState->ChannelParams[u32V1-1].u32InputRange, u32Index );
				sprintf_s( szTemp, sizeof(szTemp),  "Channel %d cal results:\n\t\tNull\t%4d\tFe  \t%4d\n\t\tGain\t%4d\tPos \t%4d ( %4d - %4d )\n",
					ConvertToUserChannel(uInt16(u32V1)), m_pCardState->DacInfo[u32V1-1][u32Index]._NullOffset, m_pCardState->DacInfo[u32V1-1][u32Index]._FeOffset,m_pCardState->DacInfo[u32V1-1][u32Index]._Gain, m_pCardState->DacInfo[u32V1-1][u32Index]._Pos,
					int16(m_pCardState->DacInfo[u32V1-1][u32Index]._FeOffset) - 2 *m_pCardState->DacInfo[u32V1-1][u32Index]._Pos, int16(m_pCardState->DacInfo[u32V1-1][u32Index]._FeOffset) + 2 *m_pCardState->DacInfo[u32V1-1][u32Index]._Pos);
			}
			else
			{
				sprintf_s( szTemp, sizeof(szTemp),  "Channel cal failed: Error %d\n", i32V3);
			}
			OutputDebugString( szTemp );
		}
		break;
		case TRACE_DAC:
		{
			sprintf_s( szTemp, sizeof(szTemp),  "Writing to DAC %2d Code %04x\n",u32V2, u32V1);
			OutputDebugString( szTemp );
		}
		break;
		case TRACE_CAL_SIGNAL_ITER: strcpy_s(szT, sizeof(szT), "    ");
		case TRACE_CAL_SIGNAL:
		{
			sprintf_s( szTemp, sizeof(szTemp),  "%sSetting calibration DAC to %+07d uV Code %4d Res %+07d uV (i=%d)\n", 
				szT,i32V3, u32V1, i32V4, u32V2);
			OutputDebugString( szTemp );
		}
		break;
		case TRACE_CAL_RANGE:
		{
			sprintf_s( szTemp, sizeof(szTemp),  "* * * Calibration range %04d mV  Calibration swing %d.%d mV Refference %d.%d mV CodeB %4d (iter = %d)\n", 
				m_u32CalibRange_mV, (m_u32CalibSwing_uV+500)/1000, ((m_u32CalibSwing_uV+50)/100)%10,
			   (m_u32VarRefLevel_uV+500)/1000, ((m_u32VarRefLevel_uV+50)/100)%10, m_u16CalCodeB, u32V1);
			OutputDebugString( szTemp );
		}
		break;
		case TRACE_CAL_PROGRESS:
		{
			switch ( i32V3 )
			{
			case 0:
				sprintf_s( szTemp, sizeof(szTemp),  "Calibration of channel %d   Range: %d mV   Coupling: %s  Filter: %s\n", 
					ConvertToUserChannel(uInt16(u32V1)), m_pCardState->ChannelParams[u32V1-1].u32InputRange, CS_COUPLING_AC == m_pCardState->ChannelParams[u32V1-1].u32Term ?"AC":"DC", m_pCardState->ChannelParams[u32V1-1].u32Filter!=0?"On":"Off");
				OutputDebugString( "\n" );
				break;
			case 1:
				sprintf_s( szTemp, sizeof(szTemp),  " ## ## Channel %d switched into %s mode\n", ConvertToUserChannel(uInt16(u32V1)), 0==u32V2? "input": "calibration"); 
				break;
			case -1:
				switch ( i32V4 )
				{
					default: return;
					case 1: sprintf_s( szTemp, sizeof(szTemp),  "\n!!!  Null amp. offset cal failed. channel %d. Code %d (0x%3x)\n\n", ConvertToUserChannel(uInt16(u32V1)), u32V2, u32V2); break;
					case 2: sprintf_s( szTemp, sizeof(szTemp),  "\n!!!  Front end offset cal failed. channel %d. Code %d (0x%3x)\n\n", ConvertToUserChannel(uInt16(u32V1)), u32V2, u32V2); break;
					case 3: sprintf_s( szTemp, sizeof(szTemp),  "\n!!!              Gain cal failed. channel %d. Code %d (0x%3x)\n\n", ConvertToUserChannel(uInt16(u32V1)), u32V2, u32V2); break;
					case 4: sprintf_s( szTemp, sizeof(szTemp),  "\n!!!          Position cal failed. channel %d. Pos Code %d (0x%3x)\n\n", ConvertToUserChannel(uInt16(u32V1)), u32V2, u32V2); break;
					case 5: sprintf_s( szTemp, sizeof(szTemp),  "\n!!!          Position cal failed. channel %d. Neg Code %d (0x%3x)\n\n", ConvertToUserChannel(uInt16(u32V1)), u32V2, u32V2); break;
				}
				break;
			default: return;
			}
			OutputDebugString( szTemp );
		}
		break;

		case TRACE_CAL_MS:
		{
			int32	i32Temp = (int32) u32V2;
			sprintf_s(szTemp, sizeof(szTemp), "\nCard %d: PosZeroCrossOffset1 = %d, MsCalibOffset= %d", u32V1, i32Temp, i32V4 );
			OutputDebugString( szTemp );
		}
		break;

		case TRACE_CAL_EXTTRIG:
			sprintf_s(szTemp, sizeof(szTemp), "\nChannelZeroCrossOffset= %d, ExtTrigZeroCrossOffset= %d", u32V1, u32V2 );
			OutputDebugString( szTemp );
			break;

		default: return;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsRabbitDev::VerifyTriggerOffset()
{
	int32 i32Ret = CS_SUCCESS;
	CsRabbitDev	*pDevice = (CsRabbitDev *) m_MasterIndependent;

	if( !VerifyTriggerOffsetNeeded() )return i32Ret;

	while ( pDevice )
	{
//		i32Ret = pDevice->SetupForSinglePulseCalib(true);
//		if( CS_FAILED(i32Ret) )return i32Ret;

		i32Ret = pDevice->SetupForCalibration(true, ecmCalModeTAO );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		pDevice = (CsRabbitDev *) (pDevice->m_Next);
	}
	
	// Trigger from Channel 1
	i32Ret = SetupForExtTrigCalib_SourceChan1();
	if( CS_FAILED(i32Ret) )	return i32Ret;

	int32 i32CalibRet = CalibTriggerOffset();
		
	i32Ret = SendCalibModeSetting( ConvertToHwChannel( CS_CHAN_1 ));
	if( CS_FAILED(i32Ret) )	return i32Ret;

	pDevice = (CsRabbitDev *) m_MasterIndependent;
	while ( pDevice )
	{
//		i32Ret = pDevice->SetupForSinglePulseCalib(false);
//		if( CS_FAILED(i32Ret) )return i32Ret;

		i32Ret = pDevice->SetupForCalibration(false, ecmCalModeTAO );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		pDevice = (CsRabbitDev *) (pDevice->m_Next);
	}

	return i32CalibRet;
}

//-----------------------------------------------------------------------------
// This function is only for Cobra PCI card.
//-----------------------------------------------------------------------------

int32   CsRabbitDev::CalibTriggerOffset()
{
	int32 i32Ret = CS_SUCCESS;
	int8 i8StartPos;
	int32 i32OffsetAdjust;
	bool  bOldSeting = m_bZeroTrigAddrOffset;

	if ( IsExpertAVG() )
		i8StartPos = 0;
	else
		i8StartPos = MIN_EXTTRIG_ADJUST;

	// Acquire the calib signal
	m_bZeroTrigAddrOffset = true;
	i32Ret = AcquireCalibSignal( CS_CHAN_1, i8StartPos ,&i32OffsetAdjust );
	m_bZeroTrigAddrOffset = bOldSeting;

	if( CS_FAILED(i32Ret) )	
	{
		::OutputDebugString("AcquireCalibSignal failed in VerifyTriggerOffset\n");
		return CS_FALSE;
	}
	
	if ( m_bHardwareAveraging && (0 !=( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL)) )
	{
		// Data in HWAVG mode 
		// | Ch1 | Ch1 | Ch1 | Ch1 | Ch2 | Ch2 | Ch2 | Ch2 |

		int i = 0;
		int j = 0;

		// Removed data from the other channel
		j = ((ConvertToHwChannel(CS_CHAN_1)) == CS_CHAN_1) ? 0 : 4;

		int8	*pi8Channel = (int8 *) &m_pu8CalibBuffer[i32OffsetAdjust];
		int8	*pi8Raw = (int8 *) &m_pu8CalibBuffer[i32OffsetAdjust];

		for ( ; i < CSRB_CALIB_BUF_MARGIN/2; )
		{
			RtlCopyMemory( &pi8Channel[i], &pi8Raw[j], 4);
			i += 4;
			j += 8;
		}
		// Magic number !!!!
		i32OffsetAdjust = 0x2A;
	}

	uInt32 u32ChanPosCross;

	i32Ret = CsDetectLevelCrossAddress( &m_pu8CalibBuffer[i32OffsetAdjust], EXTTRIG_CALIB_LENGTH, &u32ChanPosCross );
	if( CS_FAILED(i32Ret) )
	{
		OutputDebugString("CsDetectZeroCrossAddress failed in VerifyTriggerOffset\n");
		return CS_FALSE;	
	}

	int8 i8NewAddrOff = int8(u32ChanPosCross) + i8StartPos;
	SetTriggerAddressOffsetAdjust( i8NewAddrOff );

#if _DEBUG
	char szText[128];
	sprintf_s(szText, sizeof(szText), "New TrigAddrOffset = %d\n", i8NewAddrOff );
	::OutputDebugString(szText);
#endif

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

bool CsRabbitDev::VerifyTriggerOffsetNeeded()
{

	if( m_bNucleon || m_bZeroTrigAddrOffset || (0 != m_pCardState->u8ImageId) )
		return false;

	// Below 5 M/S, acquisition of the pulse for calib is not reliable or impossible
	if ( m_pCardState->AcqConfig.i64SampleRate < 5000000 )
		return false;

	uInt8	u8ModeIndex = ((m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) != 0) ? 0 : 1;
	uInt32	u32RateIndex = 0;

	if ( (m_pCardState->AcqConfig.u32Mode & CS_MODE_REFERENCE_CLK ) != 0 )
		u8ModeIndex += 4;

	if ( 0 != m_pCardState->AcqConfig.u32ExtClk )
	{
		u8ModeIndex += 2;
		u32RateIndex = (m_pCardState->AddOnReg.u16PingPong != 0) ? 0 : 1;
		if ( NULL != m_MasterIndependent->m_Next )
		u32RateIndex += 2;
	}
	else
		FindSRIndex( m_pCardState->AcqConfig.i64SampleRate, &u32RateIndex ); 
	
	bool bRet = (u8ModeIndex != m_pCardState->u8VerifyTrigOffsetModeIx) || (u32RateIndex != m_pCardState->u32VerifyTrigOffsetRateIx );
	m_pCardState->u8VerifyTrigOffsetModeIx = u8ModeIndex;
	m_pCardState->u32VerifyTrigOffsetRateIx = u32RateIndex;
	return bRet;
}


//------------------------------------------------------------------------------
// ecmCalModeMs		Calibration for M/S
//		Trigger source should be chnaged to channel 1 master card
//		Sample rate is the current clock rate without decimation
//
// ecmCalModeExtTrig	Calibration for Ext trigger	
//		Trigger source should be chnaged to channel 1
//		Input Range should be changed to default (2V pk-pk)
//		Sample rate is the current clock rate without decimation
//		
//------------------------------------------------------------------------------
int32 CsRabbitDev::SetupForCalibration( bool bSetup, eCalMode eMode )
{
	int32	i32Ret = CS_SUCCESS;
	uInt16	u16HwChannel;		
	CsRabbitDev* pDevice(NULL);


	i32Ret = SetupBufferForCalibration( bSetup, eMode );
	if( CS_FAILED(i32Ret) )	
		return i32Ret;

	if( bSetup )
	{
		CSACQUISITIONCONFIG CalibAcqConfig;

		m_OldAcqConfig.u32Size = sizeof(CSACQUISITIONCONFIG);
		CsGetAcquisitionConfig( &m_OldAcqConfig );
			
		CalibAcqConfig = m_OldAcqConfig;		
		
		if ( ecmCalModeMs == eMode || ecmCalModeExtTrig == eMode || ecmCalModeExtTrigChan == eMode || ecmCalModeTAO == eMode )
		{
			// Backup the current trigger settings then switch to the default one
			RtlCopyMemory( m_OldTrigParams, m_pCardState->TriggerParams, sizeof(m_pCardState->TriggerParams));
			CsSetTriggerConfig();
			if ( this == m_MasterIndependent )
			{
				// we're using m_ValidConfig to store the master's settings because m_OldAcqConfig gets overwritten
				// each time this routine gets called we go thru the master and the slaves
				m_ValidConfig = m_OldAcqConfig;
			}
		}
	
		CalibAcqConfig.u32SegmentCount = 1;
		CalibAcqConfig.i64TriggerDelay = 0;
		CalibAcqConfig.i64TriggerHoldoff = 0;
		CalibAcqConfig.i64Depth = CSRB_CALIB_DEPTH + CSRB_CALIB_BUF_MARGIN;

		if ( m_bNucleon )
		{
			CalibAcqConfig.u32Mode &= ~(CS_MASKED_MODE);
			CalibAcqConfig.u32Mode |= CS_MODE_SINGLE;
		}
		else
			CalibAcqConfig.i64Depth = CSRB_CALIB_DEPTH + CSRB_CALIB_BUF_MARGIN;


		if ( IsExpertAVG() )
			CalibAcqConfig.i64SegmentSize = CalibAcqConfig.i64Depth + 128;
		else
			CalibAcqConfig.i64SegmentSize = CSRB_CALIB_BUFFER_SIZE;

		if ( ecmCalModeMs == eMode || ecmCalModeExtTrig == eMode || ecmCalModeExtTrigChan == eMode || ecmCalModeTAO == eMode )
		{
			CalibAcqConfig.i64TriggerTimeout = -1;
			CalibAcqConfig.i64TriggerHoldoff = (m_bNucleon) ? EXTTRIG_CALIB_LENGTH/2 : 64;
			CalibAcqConfig.i64Depth			 = 8192;
			CalibAcqConfig.i64SegmentSize	= CalibAcqConfig.i64Depth + CalibAcqConfig.i64TriggerHoldoff;
		}
		else
		{
			CalibAcqConfig.i64TriggerTimeout = 0;
			CalibAcqConfig.i64TriggerHoldoff = 0;
		}

		if( 0 == CalibAcqConfig.u32ExtClk && ecmCalModeTAO != eMode )
		{
			// Calibrate at highest sample clock		
			LONGLONG llMaxSr = GetMaxSampleRate( false );

			if ( m_pCardState->bLAB11G )
			{
				CalibAcqConfig.u32Mode = CS_MODE_DUAL;
				CalibAcqConfig.i64SampleRate = llMaxSr;
			}
			else if( CalibAcqConfig.i64SampleRate <= llMaxSr  )
				CalibAcqConfig.i64SampleRate = llMaxSr;
			else
				CalibAcqConfig.i64SampleRate = GetMaxSampleRate( true );
		}

		if ( ecmCalModeMs == eMode || ecmCalModeExtTrig == eMode || ecmCalModeExtTrigChan == eMode || ecmCalModeTAO == eMode )
		{
			// Reset all channel offsets to 0
			for( uInt16 i = 1; i <= GetBoardNode()->NbAnalogChannel; i ++ )
			{
				u16HwChannel = ConvertToHwChannel(i);
				i32Ret = SendPosition( u16HwChannel, 0 );
					if( CS_FAILED(i32Ret) )	break;
			}
		}

		if(NULL == m_pTriggerMaster || NULL == m_MasterIndependent)
			i32Ret = CsSetAcquisitionConfig( &CalibAcqConfig );
		else
		{
			pDevice = m_MasterIndependent;
			while (pDevice)
			{
				pDevice->m_bCalibActive = true;

				if( pDevice != m_pTriggerMaster )
				{
					i32Ret = pDevice->CsSetAcquisitionConfig( &CalibAcqConfig );
					if( CS_FAILED(i32Ret) )	break;

					if (pDevice->IsExpertAVG())
						pDevice->SetupHwAverageMode( FALSE );
				}
				pDevice = pDevice->m_Next;
			}
			if( CS_SUCCEEDED (i32Ret) )
			{
				i32Ret = m_pTriggerMaster->CsSetAcquisitionConfig( &CalibAcqConfig );

				if (m_pTriggerMaster->IsExpertAVG())
					m_pTriggerMaster->SetupHwAverageMode( FALSE );
			}
		}

		if( CS_FAILED(i32Ret) )	
			return i32Ret;
	}
	else
	{
		if ( ecmCalModeMs == eMode || ecmCalModeExtTrig == eMode || ecmCalModeExtTrigChan == eMode || ecmCalModeTAO == eMode )
		{
			// Restore the original trigger settings
			RtlCopyMemory( m_pCardState->TriggerParams, m_OldTrigParams, sizeof(m_pCardState->TriggerParams));

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


			// Restore all channel offsets
			for( uInt16 i = 1; i <= GetBoardNode()->NbAnalogChannel; i ++ )
			{
				u16HwChannel = ConvertToHwChannel(i);
				i32Ret = SendPosition( u16HwChannel, m_pCardState->ChannelParams[u16HwChannel-1].i32Position );
				if( CS_FAILED(i32Ret) )	break;
			}
			if ( this != m_MasterIndependent )
			{
				m_OldAcqConfig = m_MasterIndependent->m_ValidConfig;
			}
		}

		if(NULL == m_pTriggerMaster || NULL == m_MasterIndependent)
		{
			m_bCalibActive = false;
			i32Ret = CsSetAcquisitionConfig( &m_OldAcqConfig );
		}
		else
		{
			pDevice = m_MasterIndependent;
			while (pDevice)
			{
				pDevice->m_bCalibActive = false;
				if( pDevice != m_pTriggerMaster )
				{
					i32Ret = pDevice->CsSetAcquisitionConfig( &m_OldAcqConfig );
					if( CS_FAILED(i32Ret) )	break;
				}

				pDevice = pDevice->m_Next;
			}
			if( CS_SUCCEEDED (i32Ret) )
			{
				i32Ret = m_pTriggerMaster->CsSetAcquisitionConfig( &m_OldAcqConfig );
			}
		}
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}

	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsRabbitDev::SetupBufferForCalibration( bool bSetup, eCalMode eMode )
{
	int32 i32Ret = CS_SUCCESS;

	UNREFERENCED_PARAMETER(eMode);

	if( bSetup )
	{
		m_pu8CalibBuffer = (uInt8 *)::GageAllocateMemoryX( CSRB_CALIB_BUFFER_SIZE*sizeof(uInt8) );
		if( NULL == m_pu8CalibBuffer)
			return CS_INSUFFICIENT_RESOURCES;

		m_pi32CalibA2DBuffer = (int32 *)::GageAllocateMemoryX( CSRB_CALIB_BLOCK_SIZE*sizeof(int32) );
		if( NULL == m_pi32CalibA2DBuffer)
			return CS_INSUFFICIENT_RESOURCES;

		// Master/Slave and Ext Trigger calibration, change the trigger sensitivity on the default triggger
		// engine so that it can be triggered on 2V and 5V ranges with the Calib pulse signal
//		if ( ecmCalModeMs == eMode || ecmCalModeExtTrig == eMode )
//			i32Ret =  SendDigitalTriggerSensitivity( CSRB_TRIGENGINE_A1, 3 );
	}
	else
	{
		::GageFreeMemoryX(m_pu8CalibBuffer);
		m_pu8CalibBuffer = NULL;

		::GageFreeMemoryX(m_pi32CalibA2DBuffer);
		m_pi32CalibA2DBuffer = NULL;

//		if ( ecmCalModeMs == eMode || ecmCalModeExtTrig == eMode )
//			i32Ret =  SendDigitalTriggerSensitivity( CSRB_TRIGENGINE_A1, m_pCardState->u8TrigSensitivity );
	}

	return i32Ret;
}

#ifdef _WINDOWS_

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void CsRabbitDev::ReadCalibrationSettings(void *Params)
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

	if( ! m_pCardState->bHighBandwidth )
	{
		ul = m_pCardState->bUseDacCalibFromEEprom ? 1 : 0;
		::RegQueryValueEx( hKey, _T("UseDacCalibFromEeprom"), NULL, NULL, (LPBYTE)&ul, &DataSize );
		m_pCardState->bUseDacCalibFromEEprom = 0 != ul;
	}

	ul = m_u8CalDacSettleDelay;
	::RegQueryValueEx( hKey, _T("CalDacSettleDelay"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u8CalDacSettleDelay = (uInt8)ul;

	ul = (uInt32)m_i32CalAdcOffset_uV;
	::RegQueryValueEx( hKey, _T("CalAdcOffset"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_i32CalAdcOffset_uV = (int32)ul;

	ul = m_u32IgnoreCalibError;
	::RegQueryValueEx( hKey, _T("IgnoreCalibError"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u32IgnoreCalibError = ul;

	ul = m_bNeverCalibrateMs ? 1 : 0;
	::RegQueryValueEx( hKey, _T("NeverCalibrateMs"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bNeverCalibrateMs = ( ul !=0 );

	ul = m_bDebugCalibSrc ? 1 : 0;
	::RegQueryValueEx( hKey, _T("DebugCalSrc"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bDebugCalibSrc = 0 != ul;

	ul = m_bNeverValidateExtClk ? 1 : 0;
	::RegQueryValueEx( hKey, _T("NeverValidateExtClk"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_bNeverValidateExtClk = 0 != ul;


	ul = m_u32DebugCalibTrace;
	::RegQueryValueEx( hKey, _T("TraceCalib"), NULL, NULL, (LPBYTE)&ul, &DataSize );
	m_u32DebugCalibTrace = ul;
}

#else

void CsRabbitDev::ReadCalibrationSettings(char *szIniFile, void *Params)
{
	ULONG	ul = 0;
	DWORD	DataSize = sizeof(ul);
   
   char *szSectionName = (char *)Params;
	ul = m_bSkipCalib ? 1 : 0;
   
   if (!szIniFile)
      return;
   
   ul = GetCfgKeyInt(szSectionName, _T("FastDcCal"),  ul,  szIniFile);	
	m_bSkipCalib = 0 != ul;
   //fprintf(stdout, "FastDcCal return %d \n",ul);

	ul = m_u16SkipCalibDelay;
   ul = GetCfgKeyInt(szSectionName, _T("NoCalibrationDelay"),  ul,  szIniFile);	

	if(ul > 24 * 60 ) //limitted to 1 day
		ul = 24*60;
	m_u16SkipCalibDelay = (uInt16)ul;
   //fprintf(stdout, "NoCalibrationDelay return %d \n",ul);

	ul = m_bNeverCalibrateDc ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("NeverCalibrateDc"),  ul,  szIniFile);	
	m_bNeverCalibrateDc = 0 != ul;
   //fprintf(stdout, "NeverCalibrateDc return %d \n",ul);
  
	if( ! m_pCardState->bHighBandwidth )
	{
		ul = m_pCardState->bUseDacCalibFromEEprom ? 1 : 0;
      ul = GetCfgKeyInt(szSectionName, _T("UseDacCalibFromEeprom"),  ul,  szIniFile);	
		m_pCardState->bUseDacCalibFromEEprom = 0 != ul;
      //fprintf(stdout, "UseDacCalibFromEeprom return %d \n",ul);
	}

	ul = m_u8CalDacSettleDelay;
   ul = GetCfgKeyInt(szSectionName, _T("CalDacSettleDelay"),  ul,  szIniFile);	
	m_u8CalDacSettleDelay = (uInt8)ul;
   //fprintf(stdout, "CalDacSettleDelay return %d \n",ul);

	ul = (uInt32)m_i32CalAdcOffset_uV;
   ul = GetCfgKeyInt(szSectionName, _T("CalAdcOffset"),  ul,  szIniFile);	
	m_i32CalAdcOffset_uV = (int32)ul;
   //fprintf(stdout, "CalAdcOffset return %d \n",ul);

	ul = m_u32IgnoreCalibError;
   ul = GetCfgKeyInt(szSectionName, _T("IgnoreCalibError"),  ul,  szIniFile);	
	m_u32IgnoreCalibError = ul;
   //fprintf(stdout, "IgnoreCalibError return %d \n",ul);

	ul = m_bNeverCalibrateMs ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("NeverCalibrateMs"),  ul,  szIniFile);	
	m_bNeverCalibrateMs = ( ul !=0 );
   //fprintf(stdout, "NeverCalibrateMs return %d \n",ul);

	ul = m_bDebugCalibSrc ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("DebugCalSrc"),  ul,  szIniFile);	
	m_bDebugCalibSrc = 0 != ul;
   //fprintf(stdout, "DebugCalSrc return %d \n",ul);

	ul = m_bNeverValidateExtClk ? 1 : 0;
   ul = GetCfgKeyInt(szSectionName, _T("NeverValidateExtClk"),  ul,  szIniFile);	
	m_bNeverValidateExtClk = 0 != ul;
   //fprintf(stdout, "NeverValidateExtClk return %d \n",ul);

	ul = m_u32DebugCalibTrace;
   ul = GetCfgKeyInt(szSectionName, _T("TraceCalib"),  ul,  szIniFile);	
	m_u32DebugCalibTrace = ul;
   //fprintf(stdout, "TraceCalib return %d \n",ul);
   
}   

#endif



//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsRabbitDev::CalibrateOffset(uInt16 u16HwChannel)
{	
	int32 i32Ret = CS_SUCCESS;
	const uInt16 u16ChanZeroBased = u16HwChannel - 1;
	
	i32Ret = SetCalibrationRange(c_u32OffsetCalibrationRange_mV);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	//Write initial values
	i32Ret = WriteToCalibCtrl(eccFEOffset, u16HwChannel, c_u16CalDacCount/2);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = WriteToCalibCtrl(eccAdcGain, u16HwChannel, BUNNY_ADC_REG_SPAN/2, m_u8CalDacSettleDelay);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	int32 i32uV;
	i32Ret = SendCalibDC( 0, false, &i32uV );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendCalibModeSetting( u16HwChannel, ecmCalModeDc );
	if( CS_FAILED(i32Ret) )	return i32Ret;


	//Adjust Null offset to compensate bias current
	int32 i32uV_init(0);
	uInt16 u16Code(0), u16Delta(c_u16CalDacCount/2);
	if( 0 == (CSRB_SET_CHAN_AMP & m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] ) )
	{
#ifdef DBG
		i32Ret = WriteToCalibCtrl(eccNullOffset, u16HwChannel, c_u16CalDacCount/2, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = ReadCalibA2D(&i32uV_init);
		if( CS_FAILED(i32Ret) )	return i32Ret;
#endif
		
		int32 i32 = i32uV_init;
		uInt16 u16 = c_u16CalDacCount/2;
		while(u16Delta > 0 )
		{
			i32Ret = WriteToCalibCtrl(eccNullOffset, u16HwChannel, u16Code|u16Delta, m_u8CalDacSettleDelay);
			if( CS_FAILED(i32Ret) )	return i32Ret;

			i32Ret = ReadCalibA2D(&i32uV);
			if( CS_FAILED(i32Ret) )	return i32Ret;
			TraceCalib( TRACE_ZERO_OFFSET_IT, u16Code|u16Delta, u16, i32uV, i32 );
			i32 = i32uV;
			u16 = u16Code|u16Delta;

			if( i32uV > 0 )
				u16Code |= u16Delta;

			u16Delta >>= 1;
		}

		if( 0 == u16Code || (c_u16CalDacCount-1) == u16Code )
		{
			TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, u16Code,-1,1);
			char szErrorStr[128];
			sprintf_s( szErrorStr, sizeof(szErrorStr), "%d, Channel %d DAC code %d", CS_COARSE_OFFSET_CAL_FAILED, ConvertToUserChannel(u16HwChannel), u16Code );
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );

			if( 0 == m_u32IgnoreCalibError )
				return CS_COARSE_OFFSET_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16HwChannel);
		}
	}
	else
	{
		u16Code = 0;
	}

	//Write final value
	i32Ret = WriteToCalibCtrl(eccNullOffset, u16HwChannel, u16Code, m_u8CalDacSettleDelay);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	TraceCalib( TRACE_ZERO_OFFSET, u16Code, 0, i32uV_init, i32uV );

	i32Ret = CalibrateCoreOffset();
	if( CS_FAILED(i32Ret) )	return i32Ret;

	//Adjust Front end offset to compensate fo FE offset
	int16 i16Odd(0), i16Even(0);
#ifdef DBG
	i32Ret = AcquireAndAverage(u16HwChannel, &i16Odd, &i16Even);
	if( CS_FAILED(i32Ret) )	return i32Ret;
#endif
	i32uV_init = (i16Odd + i16Even)/2;

	//Substruct a c_u16CalDacCount/32 from midle value to calibrate �c_u16CalDacCount/32 limits
	//we do not go further then c_u16CalDacCount/32 so not diminish user offset range
	u16Code = 0;
	u16Delta = c_u16CalDacCount/8;
	const uInt16 cu16Base = c_u16CalDacCount/2 - u16Delta;
	
	while(u16Delta > 0 )
	{
		i32Ret = WriteToCalibCtrl(eccFEOffset, u16HwChannel, cu16Base + u16Code|u16Delta, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = AcquireAndAverage(u16HwChannel, &i16Odd, &i16Even);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32uV = (i16Odd + i16Even)/2;
		
		if( i32uV > 0 )
			u16Code |= u16Delta;
		u16Delta >>= 1;
	}
	
	i32Ret = WriteToCalibCtrl(eccFEOffset, u16HwChannel, cu16Base + u16Code, m_u8CalDacSettleDelay);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	TraceCalib( TRACE_CAL_FE_OFFSET, cu16Base + u16Code, 0, i32uV_init, i32uV );

	if( 0 == u16Code || (c_u16CalDacCount/16-1) == u16Code )
	{
		TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, u16Code,-1,2);
		char szErrorStr[128];
		sprintf_s( szErrorStr, sizeof(szErrorStr), "%d, Channel %d DAC code %d", CS_FINE_OFFSET_CAL_FAILED, ConvertToUserChannel(u16HwChannel), cu16Base + u16Code );
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );
		if( 0 == m_u32IgnoreCalibError )
			return CS_FINE_OFFSET_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16HwChannel);
	}

	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
const uInt16 CsRabbitDev::c_u16CalGainWnd = 2;

int32	CsRabbitDev::CalibrateGain(uInt16 u16HwChannel)
{
	int32 i32Ret = CS_SUCCESS;
	const uInt16 u16ChanZeroBased = u16HwChannel - 1;

	// SONOSCAN -RFS
	bool	bSonoscanPatchforGain = true;

	i32Ret = SetCalibrationRange(m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	uInt32 u32LevelRange = 0;
	
	if ((m_u32CalibSwing_uV+500)/1000 > m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange/2)
	{
		u32LevelRange = m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange/2;
	}
	else
	{
		bSonoscanPatchforGain = true;		// FW or HW problem. We need more attenuation
		u32LevelRange = (m_u32CalibSwing_uV+500)/1000;
	}

	// SONOSCAN -RFS
	int32 i32Level = (u32LevelRange * 9)/10;
	if ( m_bNucleon && ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_SONOSCAN) ) )
	{
		// FW or HW problem. We need more attenuation
		if ( bSonoscanPatchforGain )
			i32Level = (u32LevelRange * 30)/100;
		else
			i32Level = (u32LevelRange * 40)/100;
	}

	//Calibrate positive side
	uInt16 u16PosCode(0), u16NegCode(0);
	int32 i32uV;
	uInt16 u16PosDacCode(c_u16CalDacCount+1), u16NegDacCode(c_u16CalDacCount+1);
	i32Ret = SendCalibDC( i32Level, true, &i32uV, &u16PosDacCode );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	int16 i16Odd, i16Even;

	i32uV *= -m_pCardState->AcqConfig.i32SampleRes;

	BOOLEAN		bOddSamples = FALSE;
	// Find out which samples (odd or even) is affected by Adc control
	if  ( !m_pCardState->b14G8 && 0 != m_pCardState->AddOnReg.u16PingPong )
	{
		int16 i16OddA, i16OddB;
		int16 i16EvenA, i16EvenB;

		i32Ret = WriteToCalibCtrl(eccAdcGain, u16HwChannel, 0, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = AcquireAndAverage(u16HwChannel, &i16OddA, &i16EvenA);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = WriteToCalibCtrl(eccAdcGain, u16HwChannel, BUNNY_ADC_REG_SPAN, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = AcquireAndAverage(u16HwChannel, &i16OddB, &i16EvenB);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		if ( Abs(i16OddB - i16OddA ) > Abs(i16EvenB - i16EvenA ) )
			bOddSamples = TRUE;		
	}

	//Sample res value corresponds to the full scale
	//if we devide voltage level by unipolar range in �V (* 500 ) 
	//and multiply by 10 (factor of the AcquireAndAverage) we will get the target value

	//First calibrate even samples
	int16 i16TargetPos = int16(i32uV/int32(m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange * 50));
	uInt16 u16Delta = (BUNNY_ADC_REG_SPAN+1)/2;
	
	while(u16Delta > 0 )
	{
		i32Ret = WriteToCalibCtrl(eccAdcGain, u16HwChannel, u16PosCode|u16Delta, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = AcquireAndAverage(u16HwChannel, &i16Odd, &i16Even);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		if ( bOddSamples )
		{
			if( i16Odd > i16TargetPos )
				u16PosCode |= u16Delta;
			u16Delta >>= 1;
		}
		else
		{
			if( i16Even > i16TargetPos )
				u16PosCode |= u16Delta;
			u16Delta >>= 1;
		}
	}

	i32Ret = SendCalibDC( -i32Level, true, &i32uV, &u16NegDacCode );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	i32uV *= -m_pCardState->AcqConfig.i32SampleRes;
	//Sample res value corresponds to the full scale
	//if we devide voltage level by unipolar range in �V (* 500 ) 
	//and multiply by 10 (factor of the AcquireAndAverage) we will get the target value
	int16 i16TargetNeg = int16(i32uV/int32(m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange * 50));
	
	u16Delta = (BUNNY_ADC_REG_SPAN+1)/2;
	while(u16Delta > 0 )
	{
		i32Ret = WriteToCalibCtrl(eccAdcGain, u16HwChannel, u16NegCode|u16Delta, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = AcquireAndAverage(u16HwChannel, &i16Odd, &i16Even);
		if( CS_FAILED(i32Ret) )	return i32Ret;
				
		if ( bOddSamples )
		{
			if( i16Odd < i16TargetNeg )
				u16NegCode |= u16Delta;
			u16Delta >>= 1;
		}
		else
		{
			if( i16Even < i16TargetNeg )
				u16NegCode |= u16Delta;
			u16Delta >>= 1;
		}
	}

	uInt16 u16Code = (u16NegCode + u16PosCode)/2;
	i32Ret = WriteToCalibCtrl(eccAdcGain, u16HwChannel, u16Code, m_u8CalDacSettleDelay);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	// in PingPong make the both gains equal
	if ( !m_pCardState->b14G8 && 0 != m_pCardState->AddOnReg.u16PingPong )
	{
		const uInt16 cu16SecondChannel = CS_CHAN_1 == u16HwChannel ? CS_CHAN_2 : CS_CHAN_1;
		
		int16 i16Dif, i16MinDif = 0x7fff;
		uInt16 u16StartCode(u16Code - c_u16CalGainWnd), u16EndCode(u16Code + c_u16CalGainWnd), u16MinCode(u16Code);

		for( u16Code = u16StartCode; u16Code <= u16EndCode; u16Code++)
		{
			i32Ret = WriteToCalibCtrl(eccAdcGain, cu16SecondChannel, u16Code, m_u8CalDacSettleDelay);
			if( CS_FAILED(i32Ret) )	return i32Ret;

			i32Ret = SendCalibDC( -i32Level, true, &i32uV, &u16NegDacCode );
			if( CS_FAILED(i32Ret) )	return i32Ret;
			i32Ret = AcquireAndAverage(u16HwChannel, &i16Odd, &i16Even);
			if( CS_FAILED(i32Ret) )	return i32Ret;
			i16Dif = i16Even - i16Odd;
			TraceCalib(TRACE_CAL_PINGPONG_IT, 1, u16Code,i16Odd,i16Even);

			i32Ret = SendCalibDC( i32Level, true, &i32uV, &u16PosDacCode );
			if( CS_FAILED(i32Ret) )	return i32Ret;
			i32Ret = AcquireAndAverage(u16HwChannel, &i16Odd, &i16Even);
			if( CS_FAILED(i32Ret) )	return i32Ret;
			i16Dif += i16Odd - i16Even;
			TraceCalib(TRACE_CAL_PINGPONG_IT, 2, u16Code,i16Odd,i16Even);
			
			if( labs(i16Dif) < i16MinDif )
			{
				i16MinDif = (int16) labs(i16Dif);
				u16MinCode = u16Code;
			}
		}
		
		i32Ret = WriteToCalibCtrl(eccAdcGain, cu16SecondChannel, u16MinCode, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )	return i32Ret;
#ifdef DBG
		i32Ret = AcquireAndAverage(u16HwChannel, &i16Odd, &i16Even);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i16Dif = i16Odd - i16Even;
		i32Ret = SendCalibDC( -i32Level, true, &i32uV, &u16NegDacCode );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = AcquireAndAverage(u16HwChannel, &i16Odd, &i16Even);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i16Dif += i16Even - i16Odd;
		TraceCalib(TRACE_CAL_PINGPONG, 0, u16MinCode, i16Dif,0);
#endif
	}

	TraceCalib(TRACE_CAL_GAIN, u16PosCode, u16NegCode, i16TargetPos, i16TargetNeg);

	if( 0 == u16Code || (c_u16CalDacCount-1) == u16Code )
	{
		TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, u16Code,-1,3);
		char szErrorStr[128];
		sprintf_s( szErrorStr, sizeof(szErrorStr), "%d, Channel %d DAC code %d", CS_GAIN_CAL_FAILED, ConvertToUserChannel(u16HwChannel), u16Code );
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );
		if( 0 == m_u32IgnoreCalibError )
			return CS_GAIN_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16HwChannel);
	}
	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsRabbitDev::CalibratePosition(uInt16 u16HwChannel)
{
	int32 i32Ret = CS_SUCCESS;
	const uInt16 u16ChanZeroBased = u16HwChannel - 1;
//	uInt32 u32LevelRange = (m_u32CalibSwing_uV+500)/1000 > m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange/2 ? 
//		                m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange/2 : (m_u32CalibSwing_uV+500)/1000;
//	int32 i32Level = u32LevelRange/2;
	
	uInt32 u32Index;
	i32Ret = FindFeIndex( m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange, u32Index );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	//Calibrate positive side
	int16 i16PosCode(0), i16NegCode(0);
	int32 i32uV;
	i32Ret = SendCalibDC( 0, true, &i32uV );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	int16 i16Pos(0), i16Neg(0), i16Odd(0), i16Even(0);
	int16 i16Target = int16(labs(m_pCardState->AcqConfig.i32SampleRes) * 5);  // (labs(m_pCardState->AcqConfig.i32SampleRes)/2)*10

	bool bSuccses = true;
	int16 i16Delta = -c_u16CalDacCount/2;
	while(i16Delta < 0 )
	{
		i32Ret = WriteToCalibCtrl(eccPosition, u16HwChannel, i16PosCode+i16Delta, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = AcquireAndAverage(u16HwChannel, &i16Odd, &i16Even);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i16Pos = (i16Odd + i16Even)/2;
		
		if( i16Pos < i16Target )
			i16PosCode += i16Delta;
		i16Delta /= 2;
	}

	if(0 == i16PosCode || c_u16CalDacCount-1 == -i16PosCode)
	{
		bSuccses = false;
		TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, i16PosCode,-1,4);
		char szErrorStr[128];
		sprintf_s( szErrorStr, sizeof(szErrorStr), "%d, Channel %d Pos. code %d", CS_POSITION_CAL_FAILED, ConvertToUserChannel(u16HwChannel), i16PosCode);
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );
	}

	//Calibrate negative side
	i16Target *= -1;
	i16Delta = c_u16CalDacCount/2;
	while(i16Delta > 0 )
	{
		i32Ret = WriteToCalibCtrl(eccPosition, u16HwChannel, i16NegCode+i16Delta, m_u8CalDacSettleDelay);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = AcquireAndAverage(u16HwChannel, &i16Odd, &i16Even);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i16Neg = (i16Odd + i16Even)/2;
		
		if( i16Neg > i16Target)
			i16NegCode += i16Delta;
		i16Delta /= 2;
	}

	if(0 == i16NegCode || c_u16CalDacCount-1 == i16NegCode)
	{
		bSuccses = false;
		TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, i16NegCode,-1,5);
		char szErrorStr[128];
		sprintf_s( szErrorStr, sizeof(szErrorStr), "%d, Channel %d Neg. code %d", CS_POSITION_CAL_FAILED, ConvertToUserChannel(u16HwChannel), i16NegCode);
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );
	}

	i32Ret = WriteToCalibCtrl(eccPosition, u16HwChannel, 0, m_u8CalDacSettleDelay);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	m_pCardState->DacInfo[u16ChanZeroBased][u32Index]._Pos = bSuccses ? (i16NegCode - i16PosCode)/2 : 0;
	
	TraceCalib(TRACE_CAL_POS, i16PosCode, i16NegCode, i16Pos, i16Neg);

	if( !bSuccses ) 
	{
		if( 0 == m_u32IgnoreCalibError )
			return CS_POSITION_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16HwChannel);
	}
	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsRabbitDev::CalibrateAllChannels()
{
	int32 i32Ret = CS_SUCCESS;
	int32 i32Res = CS_SUCCESS;


	if ( m_pCardState->bUseDacCalibFromEEprom )
	{	
		ForceFastCalibrationSettingsAllChannels();
		return CS_SUCCESS;
	}

	if( !IsCalibrationRequired() || m_bNeverCalibrateDc )
	{
		for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i ++ )
		{
			i32Ret = SendPosition(ConvertToHwChannel(i), m_pCardState->ChannelParams[ConvertToHwChannel(i)-1].i32Position);
			if( CS_FAILED(i32Ret) )	return i32Ret;
		}
		return i32Ret;
	}

	// SetFlag for calib ExtTrig or M/S Trig Addr Offset
//	SetFlagCalibRequired( CalibSET, CalibSET );

	i32Ret = SetupForCalibration( true );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i ++ )
	{
		i32Ret = CalibrateFrontEnd( ConvertToHwChannel(i) );

		if( m_bDebugCalibSrc )
		{
			i32Ret = SendCalibModeSetting(ConvertToHwChannel(i), m_pCardState->ChannelParams[ConvertToHwChannel(i)-1].u32Filter != 0 ? ecmCalModeDc : ecmCalModeNormal );
			if( CS_FAILED(i32Ret) )	return i32Ret;
		}

		if( CS_FAILED(i32Ret) ) 
		{
			char szErrorStr[128];
			sprintf_s( szErrorStr, sizeof(szErrorStr), "%d, Channel %d reason %d", i32Ret, i, i32Ret%CS_ERROR_FORMAT_THRESHOLD );
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );

			if( 0 == m_u32IgnoreCalibError )
				i32Res = i32Ret;
		}
	}

	i32Ret = SetupForCalibration( false );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	return i32Res;
}



//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsRabbitDev::CalibrateFrontEnd(uInt16 u16HwChannel)
{
	int32 i32Ret = CS_SUCCESS;
	const uInt16 u16ChanZeroBased = u16HwChannel - 1;

	// Do not calibrate the second channel if the user setting is single channel mode,
	// but do not clear flag since calibration will be needed on this channel in dual mode
	if ( m_bNucleon && !m_pCardState->bLAB11G )
	{
		// For PCI Express card we always calibrate in single channel mode
		if ( 0 != (m_OldAcqConfig.u32Mode & CS_MODE_SINGLE) )
		{
			if ( m_bSingleChannel2 && (CS_CHAN_2 != ConvertToUserChannel(u16HwChannel))  )
				return CS_SUCCESS;
			else if ( ! m_bSingleChannel2 &&( CS_CHAN_1 != ConvertToUserChannel(u16HwChannel))  )
				return CS_SUCCESS;
		}
	}
	else
	{
		// For PCI we calibrate with the same channel mode as user setting
		if ( (0 != (m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE)) && (CS_CHAN_1 != ConvertToUserChannel(u16HwChannel))  )
			return CS_SUCCESS;
	}

	if ( (m_bSkipCalib || !m_pCardState->bCalNeeded[u16ChanZeroBased]) && !m_bForceCal[u16ChanZeroBased])
	{
		m_bFastCalibSettingsDone[u16ChanZeroBased] = FALSE;
		if ( FastCalibrationSettings(u16HwChannel) )
			return CS_SUCCESS;
	}

	if (m_bNucleon)
	{
		SendCaptureModeSetting(m_pCardState->AcqConfig.u32Mode, u16HwChannel );
	}

	TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, 0,0,0);

	m_bErrorOnOffsetSaturation[u16ChanZeroBased] = true;

	uInt32 u32FrontEndSetup = m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased];
	if( 0 != (u32FrontEndSetup & CSRB_SET_AC_COUPLING) )
	{//clear AC
		u32FrontEndSetup &= ~CSRB_SET_AC_COUPLING; 
		i32Ret = WriteRegisterThruNios(u32FrontEndSetup, CSPLXBASE_SETFRONTEND_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}

	i32Ret = CalibrateOffset(u16HwChannel);
	if( CS_FAILED(i32Ret) )	
	{
		TraceCalib( TRACE_CAL_RESULT, u16HwChannel, 0, i32Ret, 0);
		SendCalibModeSetting( u16HwChannel );
		return i32Ret;
	}

	i32Ret = CalibrateGain(u16HwChannel);
	if( CS_FAILED(i32Ret) )	
	{
		TraceCalib( TRACE_CAL_RESULT, u16HwChannel, 0, i32Ret, 0);
		SendCalibModeSetting( u16HwChannel );
		return i32Ret;
	}

	i32Ret = CalibratePosition(u16HwChannel);
	if( CS_FAILED(i32Ret) )	
	{
		TraceCalib( TRACE_CAL_RESULT, u16HwChannel, 0, i32Ret, 0);
		SendCalibModeSetting( u16HwChannel);
		return i32Ret;
	}


	uInt32 u32Index;
	i32Ret = FindFeIndex( m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange, u32Index );
	if( CS_FAILED(i32Ret) )	return false;

	m_pCardState->DacInfo[u16ChanZeroBased][u32Index].bValid = true;

	if( 0 != (m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased] & CSRB_SET_AC_COUPLING) )
	{
		i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u32FeSetup[u16ChanZeroBased], CSPLXBASE_SETFRONTEND_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}
	SetFastCalibration(u16HwChannel);
	m_pCardState->bCalNeeded[u16ChanZeroBased] = false;
	m_bForceCal[u16ChanZeroBased] = false;

	i32Ret = SendCalibModeSetting(u16HwChannel);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendPosition(u16HwChannel, m_pCardState->ChannelParams[u16ChanZeroBased].i32Position);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	TraceCalib( TRACE_CAL_RESULT, u16HwChannel, 0, i32Ret, 0);

	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32	CsRabbitDev::AcquireAndAverage(uInt16	u16HwChannel, int16* pi16Odd_x10, int16* pi16Even_x10)
{
	CSTIMER		CsTimeout;
	m_MasterIndependent->AcquireData();
	m_MasterIndependent->ForceTrigger();

	CsTimeout.Set( CSRB_CALIB_TIMEOUT_SHORT );
	while( STILL_BUSY == m_MasterIndependent->ReadBusyStatus() )
	{
		ForceTrigger();
		if ( CsTimeout.State() )
		{
			if( STILL_BUSY == m_MasterIndependent->ReadBusyStatus() )
			{
				MsAbort();
				return CS_CAL_BUSY_TIMEOUT;
			}
		}
		BBtiming(1);
	}

	uInt32	u32AverageBufferSize = CSRB_CALIB_DEPTH;
	 if ( m_bMulrecAvgTD )
		u32AverageBufferSize = Min( m_u32MaxHwAvgDepth, CSRB_CALIB_DEPTH );

	int32 i32Ret = CsGetAverage( ConvertToUserChannel(u16HwChannel), m_pu8CalibBuffer, u32AverageBufferSize, pi16Odd_x10, pi16Even_x10);
	return  i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsRabbitDev::CheckSinglePulseForCalib()
{
	int32			i32Ret;
	DRVSYSTEM		TempSystem;			// Dummy System Node

	// LAB11G cards do not need Pulse calib signal working
	if ( m_pCardState->bLAB11G )
		return CS_SUCCESS;

	m_pSystem = &TempSystem;

	CalibrateAllChannels();

	i32Ret = SetupForCalibration(TRUE, ecmCalModeExtTrig );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendCalibModeSetting( ConvertToHwChannel( CS_CHAN_1 ), ecmCalModeExtTrig );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	// Trigger from Channel 1
	SetupForExtTrigCalib_SourceChan1();

	BBtiming(2000);
	// Acquire the calib signal
	// One acquisition ...
	AcquireData();

	CSTIMER	CsTimeout;
	CsTimeout.Set( CSRB_CALIB_TIMEOUT_SHORT );

	while( STILL_BUSY == ReadBusyStatus() )
	{
		if ( CsTimeout.State() )
		{
			if( STILL_BUSY == ReadBusyStatus() )
			{
				char szText[128] = {0};

				ForceTrigger();
				MsAbort();
				sprintf_s( szText, sizeof(szText), TEXT("(Board S/N %08x/%08x)"),m_PlxData->CsBoard.u32SerNum, m_PlxData->CsBoard.u32SerNumEx);
				CsEventLog EventLog;
				EventLog.Report( CS_PULSECALIB_TEST_FAILED, szText );
				goto OnExit;
			}
		}
	}
	 m_bPulseCalib = TRUE;

OnExit:
	i32Ret = SendCalibModeSetting( ConvertToHwChannel( CS_CHAN_1 ));
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SetupForCalibration(FALSE, ecmCalModeExtTrig );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	m_pSystem = NULL;

	return i32Ret;
}




#define	CSFB_CALIBMS_TIMEOUT	KTIME_MILISEC(500)

//-----------------------------------------------------------------------------
// This function is only for new Nucleon PCIe card.
//
// M/S calibration algorithm for PCIe:
//    Find offset between Master and slave cards
//    Using digital delay register to align the master and slave cards
//-----------------------------------------------------------------------------

int32	CsRabbitDev::MsCalibrateSkew(bool bExtTrigEn)
{
	int32 i32Ret = CS_SUCCESS;
	int32 i32MsCalibRet = CS_SUCCESS;
	CsRabbitDev* pDevice = m_MasterIndependent;
	bool	bAddonResetNeeded = false;

	bool	bCalibMs =  ( 0 != m_MasterIndependent->m_Next ) &&  m_MasterIndependent->m_pCardState->bMasterSlaveCalib && ! m_bNeverCalibrateMs;
	bool	bCalibExtTrig = (m_u8DebugCalibExtTrig == DBG_ALWAYS_CALIB) ? true : m_MasterIndependent->m_pCardState->bExtTrigCalib && (m_u8DebugCalibExtTrig != DBG_NEVER_CALIB) && bExtTrigEn;

	if ( m_pCardState->bLAB11G )
		bCalibExtTrig = false;			// No Ext Trigger calib for LAB11G

	if ( ! bCalibExtTrig && ! bCalibMs )
		return CS_SUCCESS;

	// Addon reset should be called only once after changing the clock settings.
	if ( 0 != m_MasterIndependent->m_Next )
	{
		if ( bCalibMs )
			bAddonResetNeeded = true;
	}
	else
		bAddonResetNeeded = true;

	// Set neutral position for all cards before calibration
	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		if ( bAddonResetNeeded )
			pDevice->AddonReset();
		pDevice->SetDigitalDelay(0);
		pDevice = pDevice->m_Next;
	}

	// Switch to Ms calib mode
	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		i32Ret = pDevice->SetupForSinglePulseCalib(true);
		if( CS_FAILED(i32Ret) )return i32Ret;

		i32Ret = pDevice->SetupForCalibration(true, ecmCalModeMs);
		if( CS_FAILED(i32Ret) )return i32Ret;
	
		pDevice = pDevice->m_Next;
	}

	if ( m_MasterIndependent->m_pCardState->bMsDecimationReset )
		m_MasterIndependent->ResetMSDecimationSynch();


	bool	bTemp = m_bZeroTrigAddrOffset;
	m_bZeroTrigAddrOffset = true;
	
	if ( bCalibMs )
	{
		ResetMasterSlave();
		i32Ret = CalculateMasterSlaveSkew();
	}

	if ( bCalibExtTrig && CS_SUCCEEDED(i32Ret) )
		i32Ret = CalculateExtTriggerSkew();

	i32MsCalibRet = i32Ret;

	m_bZeroTrigAddrOffset = bTemp;

	// Switch back to normal mode
	uInt16	u16Channel = m_bSingleChannel2 ? CS_CHAN_2 : CS_CHAN_1;
	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		if ( pDevice->m_Next )
			pDevice->SendCalibModeSetting( ConvertToHwChannel(u16Channel), ecmCalModeNormal, 0 );
		else
			pDevice->SendCalibModeSetting( ConvertToHwChannel(u16Channel), ecmCalModeNormal ); // With default waiting time only on the last card

		pDevice = pDevice->m_Next;
	}

	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		i32Ret = pDevice->SetupForCalibration(false, ecmCalModeMs);
		i32Ret = pDevice->SetupForSinglePulseCalib(false);

		pDevice = pDevice->m_Next;
	}

	BBtiming(1000);

	if ( m_MasterIndependent->m_pCardState->bMsDecimationReset )
		m_MasterIndependent->ResetMSDecimationSynch();

	return i32MsCalibRet;
}



#define	CSFB_CALIBMS_TIMEOUT	KTIME_MILISEC(500)
//-----------------------------------------------------------------------------
// This function is only for new Nucleon PCIe card.
//-----------------------------------------------------------------------------

int32	CsRabbitDev::CalculateMasterSlaveSkew()
{
	int32 i32Ret = CS_SUCCESS;
	int32 i32MsCalibRet = CS_SUCCESS;
	CsRabbitDev* pDevice = m_MasterIndependent;
	char  szTemp[250];

	// Switch to Ms calib mode
	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		i32Ret = pDevice->SendCalibModeSetting( ConvertToHwChannel(CS_CHAN_1), ecmCalModeMs, 0 );
		if( CS_FAILED(i32Ret) )return i32Ret;
		
		pDevice = pDevice->m_Next;
	}

	// Enable trigger only from channel 1 of Master cards
	SendTriggerEngineSetting( CS_TRIG_SOURCE_CHAN_1, EXTTRIGCALIB_SLOPE, MSCALIB_LEVEL_CHAN );
	// Change the trigger sensitivity on the default triggger engine so that
	// it can be triggered on 2V and 5V ranges with the Calib pulse signal
	SendDigitalTriggerSensitivity( CSRB_TRIGENGINE_A1, CSRB_TRIGSENS_PULSE_CALIB );

	BBtiming(5000);

	CSTIMER		CsTimeout;
	CsTimeout.Set( CSFB_CALIBMS_TIMEOUT );

	// One acquisition ...
	m_MasterIndependent->AcquireData();
	while( STILL_BUSY == m_MasterIndependent->ReadBusyStatus() )
	{
		if ( CsTimeout.State() )
		{
			if( STILL_BUSY == m_MasterIndependent->ReadBusyStatus() )
			{
				::OutputDebugString("\nMaster/Slave Calib error. Trigger timeout");
				m_MasterIndependent->ForceTrigger();
				m_MasterIndependent->MsAbort();
				i32Ret =  CS_MASTERSLAVE_CALIB_FAILURE;
				break;
			}
		}
	}

	// Detect zero crossing
	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		i32MsCalibRet = pDevice->DetectMsZeroCrossAddress();
		if( CS_FAILED(i32MsCalibRet) )	
		{
			// Assume that this card has the same offset as the master one
			pDevice->m_i16ZeroCrossOffset = m_MasterIndependent->m_i16ZeroCrossOffset;
			sprintf_s( szTemp, sizeof(szTemp), "Master/Slave Calib error. Zero crossing detection error on card %d\n", pDevice->m_pCardState->u8MsCardId);
			::OutputDebugString(szTemp);
			break;
		}
		if( pDevice != m_MasterIndependent )
			TraceCalib(TRACE_CAL_MS, pDevice->m_pCardState->u16CardNumber, 0, pDevice->m_i16ZeroCrossOffset, m_MasterIndependent->m_i16ZeroCrossOffset);

		pDevice->m_pCardState->i16MasterSlaveSkew = m_MasterIndependent->m_i16ZeroCrossOffset - pDevice->m_i16ZeroCrossOffset;
		pDevice = pDevice->m_Next;
	}	

	if ( CS_SUCCEEDED(i32MsCalibRet) )
		m_MasterIndependent->m_pCardState->bMasterSlaveCalib = false;

	return i32MsCalibRet;
}


//-----------------------------------------------------------------------------
// This function is only for new Nucleon PCIe card.
//-----------------------------------------------------------------------------
#define		EXTTRIGCALIB_CROSSING_LEVEL_50		0x80
#define		EXTTRIGCALIB_CROSSING_LEVEL_HiZ		0x60
#define		PULSE_VALIDATION_LENGTH				20

int32	CsRabbitDev::NucleonDetectZeroCrossAddress( void* pBuffer, uInt32 u32BufferSize, uInt8	u8CrossLevel, uInt32* pu32PosCross )
{
	uInt32 u32Position = 0;
	uInt8* pu8Buffer = (uInt8*) pBuffer;
	uInt32 i = 0;
	uInt8  u8Convert = 0;

	if ( -1 == m_pCardState->AcqConfig.i32SampleOffset )
		u8Convert = 0x80;		// Convert from signed to unsigned data format

	// Detection of the falling edge
	for ( i = 0; i < u32BufferSize; i++ )
	{
		if ( ((uInt8) (pu8Buffer[i] + u8Convert)) >= u8CrossLevel && ((uInt8) (pu8Buffer[i+1] + u8Convert)) < u8CrossLevel )
		{
			u32Position = i + 1;
			break;
		}
	}

	if ( i >= u32BufferSize )
	{
		::OutputDebugString("Failed on detection of zero crossing\n");
		return CS_EXTTRIG_CALIB_FAILURE;		// Zero crossing not found
	}


	uInt32	u32ValidationLength  = PULSE_VALIDATION_LENGTH;

	if ( u32Position < PULSE_VALIDATION_LENGTH )
		u32ValidationLength = u32Position;

	for ( i = 1; i <= u32ValidationLength; i++ )
	{	// All samples must be above 0 level
		if ( ((uInt8)(pu8Buffer[u32Position - i] + u8Convert)) < u8CrossLevel )
		{
			::OutputDebugString("Failed on validation of zero crossing (pre trigger)");
			return CS_MISC_ERROR;		// Cal signal not present
		}
	}

	// From zero crossing position, double check if it is a square wave or just noise 
	for ( i = 1; i < PULSE_VALIDATION_LENGTH; i++ )
	{	// All samples must be below 0 level
		if ( ((uInt8)(pu8Buffer[u32Position + i] + u8Convert)) > u8CrossLevel )
		{
			::OutputDebugString("Failed on validation of zero crossing (post trigger)");
			return CS_MISC_ERROR;		// Cal signal not present
		}
	}
	if ( NULL != pu32PosCross )
		*pu32PosCross = u32Position;
	
	return CS_SUCCESS;
}



//-----------------------------------------------------------------------------
// This function is only for new Nucleon PCIe card.
//-----------------------------------------------------------------------------

int32	CsRabbitDev::DetectMsZeroCrossAddress()
{
	int32	i32Ret = CS_SUCCESS;
	int32	i32OffsetAdjust = 0;
	uInt32	u32PosCrossing;

	m_i16ZeroCrossOffset = 0;

	in_PREPARE_DATATRANSFER		InXferEx ={0};
	out_PREPARE_DATATRANSFER	OutXferEx;

	InXferEx.i64StartAddr	= MIN_DIGITALDELAY_ADJUST;
	InXferEx.u16Channel		= ConvertToHwChannel(CS_CHAN_1);
	InXferEx.u32DataMask	= GetDataMaskModeTransfer(0);
	InXferEx.u32Segment		= 1;
	InXferEx.u32FifoMode	= GetFifoModeTransfer(CS_CHAN_1);
	InXferEx.bBusMasterDma	= m_BusMasterSupport;
	InXferEx.bIntrEnable	= 0;
	InXferEx.u32SampleSize  = sizeof(uInt8);

	IoctlPreDataTransfer( &InXferEx, &OutXferEx );

	OutXferEx.i64ActualStartAddr += OutXferEx.i32ActualStartOffset;
	if ( InXferEx.i64StartAddr >= OutXferEx.i64ActualStartAddr )
		i32OffsetAdjust = (int32)(InXferEx.i64StartAddr - OutXferEx.i64ActualStartAddr);

	CardReadMemory( m_pu8CalibBuffer, BUNNY_CALIB_BUFFER_SIZE );

#ifdef DEBUG_MASTERSLAVE
	RtlCopyMemory( m_PulseData, &m_pu8CalibBuffer[i32OffsetAdjust], Min(PAGE_SIZE, BUNNY_CALIB_BUFFER_SIZE) );
#endif

	// Calibration signal is a quare wave ~64 samples period (no-pingong) and ~128 sample period (pingpong)

	int j = i32OffsetAdjust;
	uInt32	u32SizeForCalib = EXTTRIG_CALIB_LENGTH;
	i32Ret = NucleonDetectZeroCrossAddress( &m_pu8CalibBuffer[j], u32SizeForCalib, 0x80, &u32PosCrossing );
	if ( CS_FAILED (i32Ret) )
		return CS_MASTERSLAVE_CALIB_FAILURE;

	m_i16ZeroCrossOffset = (int16) u32PosCrossing;

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
// This function is only for new Nucleon PCIe card.
//-----------------------------------------------------------------------------

int32	CsRabbitDev::MsAdjustDigitalDelay()
{
	CsRabbitDev	*pDevice = m_MasterIndependent;
	int16		i16StartAdjOffset = 0;


	if ( DBG_NEVER_CALIB == m_MasterIndependent->m_u8DebugCalibExtTrig )
		i16StartAdjOffset = 0;
	else if (m_MasterIndependent->m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].i32Source != 0 )
		i16StartAdjOffset = m_pCardState->i16ExtTrigSkew;

	while ( pDevice )
	{
		int16	i16AdjustOffset = i16StartAdjOffset;

		// Total adj offsett = M/S offset + Ext trigger offset
		if ( ! m_MasterIndependent->m_bNeverCalibrateMs && pDevice->m_bMsOffsetAdjust )
			i16AdjustOffset += pDevice->m_pCardState->i16MasterSlaveSkew;

		if ( 0 == m_pCardState->AddOnReg.u16PingPong ) // To move X samples,in nonpingpong, we have to send 2*X
			i16AdjustOffset *= 2;
	
		pDevice->SetDigitalDelay( i16AdjustOffset );
		pDevice = pDevice->m_Next;
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
// This function is only for new CobraMax PCIe card.
// CobraMax E14G8 always works in pingpong mode but ADC registers do not support
// offset adjustment between 2 cores. Here we adjust offset beween 2 cores using
// digital offset registers provided by firmware
//-----------------------------------------------------------------------------

int32	CsRabbitDev::CalibrateCoreOffset()
{
#define CSFB_OFFSET_Q_CMD				0x00815701  // cmd = 0x008, scope = 0x1, add = 0x57  Model/Cs = 01
#define CSFB_OFFSET_I_CMD				0x00815801  // cmd = 0x008, scope = 0x1, add = 0x58  Model/Cs = 01
#define	CSFB_OFFSET_MAX					7

	int32		i32Ret = CS_SUCCESS;
	int16		i32Diff = 0;
	int16		i16Odd = 0;
	int16		i16Even = 0;
	int16		i16LastVal = 0;


	if ( ! m_bCobraMaxPCIe && ! m_pCardState->b14G8 )
		return CS_SUCCESS;

	WriteRegisterThruNios( 0, CSFB_OFFSET_Q_CMD );
	WriteRegisterThruNios( 0, CSFB_OFFSET_I_CMD );
	
	i32Ret = AcquireAndAverage(CS_CHAN_1, &i16Odd, &i16Even);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Diff = Abs(i16Odd - i16Even);

	i16LastVal = 0;
	for ( int16 i = 1; i < CSFB_OFFSET_MAX; i++ )
	{
		WriteRegisterThruNios( i, CSFB_OFFSET_Q_CMD );
		i32Ret = AcquireAndAverage(CS_CHAN_1, &i16Odd, &i16Even);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		if ( Abs(i16Odd - i16Even) > i32Diff )
		{
			WriteRegisterThruNios( i16LastVal, CSFB_OFFSET_Q_CMD );
			break;
		}
		else if ( Abs(i16Odd - i16Even) <= i32Diff )
		{
			i32Diff = Abs(i16Odd - i16Even);
			i16LastVal = i;
		}
	}

	i16LastVal = 0;
	for ( int16 i = 1; i < CSFB_OFFSET_MAX; i++ )
	{
		WriteRegisterThruNios( i, CSFB_OFFSET_I_CMD );
		i32Ret = AcquireAndAverage(CS_CHAN_1, &i16Odd, &i16Even);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		if ( Abs(i16Odd - i16Even) > i32Diff )
		{
			WriteRegisterThruNios( i16LastVal, CSFB_OFFSET_I_CMD );
			break;
		}
		else if ( Abs(i16Odd - i16Even) <= i32Diff )
		{
			i32Diff = Abs(i16Odd - i16Even);
			i16LastVal = i;
		}
	}

	return CS_SUCCESS;
}

