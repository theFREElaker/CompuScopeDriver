//	CsDeereCalib.cpp
///	Implementation of CsDeereDevice class
//
//	This file contains all member functions for calibration
//
//

#include "StdAfx.h"
#include "CsDeere.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#endif

const uInt32 CsDeereDevice::c_u32MaxPositionOffset_mV = DEERE_USER_OFFSET_LIMIT;
const DEERE_CAL_LUT	CsDeereDevice::s_CalCfg50[] = 
{ //u32FeSwing_mV i32PosLevel_uV  i32NegLevel_uV u16PosRegMask u16NegRegMask
	    0,            -190,           -190,         0x22F,     0x2EF, //use positive ground to disconnect cal signal 
//	  100,		     97242,         -97296,         0x2EE,     0x3DE,      
	  200,          194484,        -194592,         0x2ED,     0x3DD,      
	  400,          388970,        -389184,         0x2EC,     0x3DC,
	 1000,          926718,        -926772,         0x0FA,     0x1F6,
	 2000,         1853436,       -1853544,         0x0F9,     0x1F5,
     4000,         3706876,       -3707088,         0x0F8,     0x1F4,
	10000,         3706876,       -3707088,         0x0F8,     0x1F4,
};

#define c_JD_50_CAL_LUT_SIZE   (sizeof(CsDeereDevice::s_CalCfg50)/sizeof(CsDeereDevice::s_CalCfg50[0]))

const DEERE_CAL_LUT	CsDeereDevice::s_CalCfg1M[] = 
{ 
 0
};
#define c_JD_1M_CAL_LUT_SIZE   (sizeof(CsDeereDevice::s_CalCfg1M)/sizeof(CsDeereDevice::s_CalCfg1M[0]))


#define	DUAL_CHANNEL_CALIB_LENGTH		128
#define	DUAL_CHANNEL_CALIB_PRETRIG		32
#define	IMPROVED_EDGE_COMPARE_LENGTH	4
#define	MS_CALIB_LENGTH				256


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsDeereDevice::SetupBufferForCalibration( bool bSetup, eCalMode eMode )
{
	int32 i32Ret = CS_SUCCESS;

	if( bSetup )
	{
		if ( NULL != m_pi16CalibBuffer )
			return CS_FALSE;
		uInt32	u32BufferLength = DEERE_CALIB_BUFFER_SIZE*sizeof(uInt16);

		if ( ecmCalModeDual == eMode )
			u32BufferLength *= 2;			// Interleaved data

		m_pi16CalibBuffer = (int16 *)::GageAllocateMemoryX( u32BufferLength );
		if( NULL == m_pi16CalibBuffer)
			return CS_INSUFFICIENT_RESOURCES;

		if( NULL != m_pi32CalibA2DBuffer )
			::GageFreeMemoryX(m_pi32CalibA2DBuffer);
		m_pi32CalibA2DBuffer = (int32 *)::GageAllocateMemoryX( DEERE_CALIB_BLOCK_SIZE*sizeof(int32) );
		if( NULL == m_pi32CalibA2DBuffer)
		{
			::GageFreeMemoryX(m_pi16CalibBuffer);
			m_pi16CalibBuffer = NULL;
			return CS_INSUFFICIENT_RESOURCES;
		}

		if ( ecmCalModeAdc == eMode )
		{
			m_pi16DataAdc[0] = (int16 *)::GageAllocateMemoryX( DEERE_CALIB_BUFFER_SIZE*sizeof(int16) );
			if( NULL == m_pi16DataAdc[0] )
			{
				::GageFreeMemoryX(m_pi16CalibBuffer);
				m_pi16CalibBuffer = NULL;
				
				::GageFreeMemoryX(m_pi32CalibA2DBuffer);
				m_pi32CalibA2DBuffer = NULL;			
				return CS_INSUFFICIENT_RESOURCES;
			}
			for( uInt8 u8 = 1; u8 < DEERE_ADC_COUNT; u8++ )
				m_pi16DataAdc[u8] = m_pi16DataAdc[0] + int(u8)* DEERE_CALIB_BUFFER_SIZE/DEERE_ADC_COUNT;
		}
	}
	else
	{
		::GageFreeMemoryX(m_pi16CalibBuffer);
		m_pi16CalibBuffer = NULL;

		::GageFreeMemoryX(m_pi32CalibA2DBuffer);
		m_pi32CalibA2DBuffer = NULL;

		if ( ecmCalModeAdc == eMode )
		{
			::GageFreeMemoryX(m_pi16DataAdc[0]);
			for( uInt8 u8 = 0; u8 < DEERE_ADC_COUNT; u8++ )
				m_pi16DataAdc[0] = NULL;
		}
	}

	return i32Ret;
}


int32 CsDeereDevice::PrepareForCalibration( bool bSetup )
{
	int32		i32Ret = CS_SUCCESS;

	if( bSetup )	
	{
		m_bCalibActive = true;

		CSACQUISITIONCONFIG CalibAcqConfig;
		CsGetAcquisitionConfig( &m_OldAcqConfig );
		CalibAcqConfig = m_OldAcqConfig;

		// Zero digital delay for this card
		SetDigitalDelay(0);

		//
		// ******   IMPORTANT !!! Always calibrate in CS_SINGLE_MODE **********
		//
		CalibAcqConfig.u32Mode = (m_MasterIndependent->m_OldAcqConfig.u32Mode & ~CS_MASKED_MODE) | CS_MODE_SINGLE;
		CalibAcqConfig.u32SegmentCount = 1;
		CalibAcqConfig.i64TriggerDelay = 0;
		CalibAcqConfig.i64Depth = DEERE_CALIB_DEPTH;

		if ( m_bMulrecAvgTD )
		{
			if ( 0 != (CalibAcqConfig.u32Mode & CS_MODE_DUAL) )
				CalibAcqConfig.i64SegmentSize = CalibAcqConfig.i64Depth + DEERE_AVG_FIXED_PRETRIG/2;
			else
				CalibAcqConfig.i64SegmentSize = CalibAcqConfig.i64Depth + DEERE_AVG_FIXED_PRETRIG;
		}
		else
			CalibAcqConfig.i64SegmentSize = DEERE_CALIB_BUFFER_SIZE;

		CalibAcqConfig.i64TriggerHoldoff = CalibAcqConfig.i64SegmentSize-CalibAcqConfig.i64Depth;
		CalibAcqConfig.i64TriggerTimeout = -1; //ecmCalModeDc == eMode ? 0 : -1;

		// Calib with the current clock rate only in Extclk mode. Otherwise, calib with the highest clock rate
		if ( 0 == CalibAcqConfig.u32ExtClk )
			CalibAcqConfig.i64SampleRate =  m_SrInfoTable[0].llSampleRate;

		m_bSquareRec = m_bCalibSqr;
		i32Ret = CsSetAcquisitionConfig( &CalibAcqConfig );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		if (m_bMulrecAvgTD)
			SetupHwAverageMode(false);

		// Disable Gate triggering for calibration
		ConfigureAuxIn( false );
	}
	else
	{
		m_bSquareRec = IsSquareRec( m_OldAcqConfig.u32Mode, m_OldAcqConfig.u32SegmentCount, (m_OldAcqConfig.i64SegmentSize - m_OldAcqConfig.i64Depth) );
		m_bCalibActive = false;
		i32Ret = CsSetAcquisitionConfig( &m_OldAcqConfig );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		if (m_bMulrecAvgTD)
			SetupHwAverageMode(m_bHwAvg32);
	}

	return i32Ret;
}


//------------------------------------------------------------------------------
// ecmCalModeMs		Calibration for M/S
//		Trigger source should be changed to channel 1 master card
//		Sample rate is the current clock rate without decimation
//
// ecmCalModeExtTrig	Calibration for Ext trigger	
//		Trigger source should be chnaged to channel 1
//		Input Range should be changed to default (2V pk-pk)
//		Sample rate is the current clock rate without decimation
//		
//------------------------------------------------------------------------------
int32 CsDeereDevice::SetupForCalibration( bool bSetup, eCalMode eMode )
{
	int32		i32Ret = CS_SUCCESS;
	char		szErrorStr[128];
	char		szTrace[256];


	if( bSetup )
	{
		i32Ret = SetupBufferForCalibration( true, eMode );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
		
		if ( ecmCalModeMs == eMode || ecmCalModeExtTrig == eMode || ecmCalModeExtTrigChan == eMode || ecmCalModeAdc == eMode || ecmCalModeDual == eMode )
		{
			// Backup the current trigger settings then switch to the default one
			RtlCopyMemory( m_OldChanParams, m_pCardState->ChannelParams, sizeof(m_pCardState->ChannelParams));
			RtlCopyMemory( m_OldTrigParams, m_pCardState->TriggerParams, sizeof(m_pCardState->TriggerParams));
			uInt32 u32Range = (ecmCalModeMs == eMode) ? CS_GAIN_2_V : CS_GAIN_1_V;

			if ( ecmCalModeDual == eMode )
			{
				// In order to minimize error on edge detection algorithm,
				// we have to "polish" the edge of the pulse by enabling filter on both channel 1 ans 2
				i32Ret = SendChannelSetting( CS_CHAN_1, u32Range, CS_COUPLING_DC, CS_REAL_IMP_50_OHM, 0, CS_FILTER_ON );
				if( CS_FAILED(i32Ret) )
					return i32Ret;
				i32Ret = SendChannelSetting( CS_CHAN_2, u32Range, CS_COUPLING_DC, CS_REAL_IMP_50_OHM, 0, CS_FILTER_ON );
				if( CS_FAILED(i32Ret) )
					return i32Ret;

				i32Ret = InCalibrationCalibrate( CS_CHAN_1 );
				if( CS_FAILED(i32Ret) )
					return i32Ret;
				i32Ret = InCalibrationCalibrate( CS_CHAN_2 );
				if( CS_FAILED(i32Ret) )
					return i32Ret;
			}
			else
			{
				i32Ret = SendChannelSetting( CS_CHAN_1, u32Range, CS_COUPLING_DC, CS_REAL_IMP_50_OHM );
				if( CS_FAILED(i32Ret) )	return i32Ret;

				if ( ecmCalModeExtTrig == eMode )
					i32Ret = CalibrateFrontEnd(CS_CHAN_1);
				else if( !FastFeCalibrationSettings(CS_CHAN_1, true ) )
					i32Ret = CalibrateFrontEnd(CS_CHAN_1);
				if( CS_FAILED(i32Ret) )	
				{
					sprintf_s( szErrorStr, sizeof(szErrorStr), "%d, Channel 1 reason %d", i32Ret, i32Ret%CS_ERROR_FORMAT_THRESHOLD );
					if( 0 != (m_u32IgnoreCalibError & 0x01) )
					{
						i32Ret = SetDefaultDacCalib(CS_CHAN_1);
						sprintf_s( szTrace, sizeof(szTrace), "*!*!*   Ignoring calibration error: %s. Using default DAC values.\n", szErrorStr);
					}
					else
					{
						sprintf_s( szTrace, sizeof(szTrace), " ! ! ! Calibration error: %s\n", szErrorStr);
#if !DBG
						CsEventLog EventLog;
						EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );
#endif			
					}
					return i32Ret;
				}
			}

			// In ecmCalModeDual, all triggers should be disabled. Trigger will be enabled later in CalculateDualChannelSkewAdc() function.
			// Otherwise, enable trigger only from the master card
			if ( ecmCalModeDual != eMode &&  ecmCalModeAdc != eMode && this == m_MasterIndependent )
				i32Ret = SendTriggerEngineSetting(CS_CHAN_1, CS_TRIG_COND_NEG_SLOPE );
			else
				i32Ret = SendTriggerEngineSetting(0);

			if( CS_FAILED(i32Ret) )
				return i32Ret;
		}
	}
	else
	{
		if ( ecmCalModeMs == eMode || ecmCalModeExtTrig == eMode || ecmCalModeExtTrigChan == eMode || ecmCalModeAdc == eMode || ecmCalModeDual == eMode )
		{
			// Restore the original trigger settings
			RtlCopyMemory( m_pCardState->ChannelParams, m_OldChanParams, sizeof(m_pCardState->ChannelParams));
			RtlCopyMemory( m_pCardState->TriggerParams, m_OldTrigParams, sizeof(m_pCardState->TriggerParams));

			i32Ret = SendChannelSetting( CS_CHAN_1, m_pCardState->ChannelParams[0].u32InputRange,
										 m_pCardState->ChannelParams[0].u32Term,
										 m_pCardState->ChannelParams[0].u32Impedance,
										 m_pCardState->ChannelParams[0].i32Position,
										 m_pCardState->ChannelParams[0].u32Filter );
			if ( CS_FAILED(i32Ret) )
				return i32Ret;

			i32Ret = SendChannelSetting( CS_CHAN_2, m_pCardState->ChannelParams[1].u32InputRange,
										 m_pCardState->ChannelParams[1].u32Term,
										 m_pCardState->ChannelParams[1].u32Impedance,
										 m_pCardState->ChannelParams[1].i32Position,
										 m_pCardState->ChannelParams[1].u32Filter );
			if ( CS_FAILED(i32Ret) )
				return i32Ret;

			uInt8 u8IndexTrig = INDEX_TRIGENGINE_A1;
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

			i32Ret = SendExtTriggerSetting( CS_TRIG_SOURCE_DISABLE != m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].i32Source,
											m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].i32Level,
											m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].u32Condition,
											m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].u32ExtTriggerRange, 
											m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].u32ExtImpedance,
											m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].u32ExtCoupling );

			if ( CS_FAILED(i32Ret) )
				return i32Ret;

			// Force re-calibrate with user settings all channels affected by skew calibration
			if ( ecmCalModeDual == eMode )
				m_pCardState->bCalNeeded[0] = m_pCardState->bCalNeeded[1] = true;
			else if ( ecmCalModeExtTrig != eMode )
				m_pCardState->bCalNeeded[0] = true;
		}

		i32Ret = SetupBufferForCalibration( false, eMode );
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	return i32Ret;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32	CsDeereDevice::AcquireCalibSignal( uInt16 u16UserChannel, int32 i32StartPos, int32* pi32OffsetAdjust )
{
	// One acquisition ...
	AcquireData();

	CSTIMER	CsTimeout;
	CsTimeout.Set( DEERE_CALIB_TIMEOUT_SHORT );

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
	InXferEx.u32DataMask	= 0; //GetDataMaskModeTransfer(0);
	InXferEx.u32FifoMode	= GetFifoModeTransfer(u16UserChannel);
	InXferEx.bBusMasterDma	= m_BusMasterSupport;
	InXferEx.bIntrEnable	= 0;
	InXferEx.u32Segment		= 1;
	InXferEx.u32SampleSize	= sizeof(int16);

	IoctlPreDataTransfer( &InXferEx, &OutXferEx );
	CardReadMemory( m_pi16CalibBuffer, DEERE_CALIB_BUFFER_SIZE*sizeof(int16) );

	if ( pi32OffsetAdjust )
		*pi32OffsetAdjust =  -1*OutXferEx.i32ActualStartOffset;
	
	return CS_SUCCESS;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32	CsDeereDevice::AcquireAndRead( const uInt16 u16ChanIndex, void* const pBuff, const uInt32 u32ReadLen )
{
	AcquireData();
	ForceTrigger();
	
	uInt32 u32Timing = 100;
	while( STILL_BUSY == ReadBusyStatus() )
	{
		BBtiming(1);
		if( 0 == --u32Timing)
		{
			MsAbort();
			return CS_CAL_BUSY_TIMEOUT;
		}
	}

	in_PREPARE_DATATRANSFER		InXferEx ={0};
	out_PREPARE_DATATRANSFER	OutXferEx = {0};

	InXferEx.i64StartAddr	= 0;
	InXferEx.u16Channel		= u16ChanIndex;
	InXferEx.u32DataMask	= 0;
	InXferEx.u32FifoMode	= GetFifoModeTransfer(u16ChanIndex);
	InXferEx.bBusMasterDma	= m_BusMasterSupport;
	InXferEx.bIntrEnable	= 0;
	InXferEx.u32Segment		= 1;
	InXferEx.u32SampleSize  = m_pCardState->AcqConfig.u32SampleSize;

	IoctlPreDataTransfer( &InXferEx, &OutXferEx );
	CardReadMemory( pBuff, u32ReadLen*m_pCardState->AcqConfig.u32SampleSize );

	return  CS_SUCCESS;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32	CsDeereDevice::AcquireAndXfer(uInt16 u16UserChannel, int16** ppi16Buffer, uInt32* pu32Size)
{
	AcquireData();
	ForceTrigger();
	
	uInt32 u32Timing = 100;
	while( STILL_BUSY == ReadBusyStatus() )
	{
		BBtiming(1);
		if( 0 == --u32Timing)
		{
			MsAbort();
			return CS_CAL_BUSY_TIMEOUT;
		}
	}

	uInt32	u32AverageBufferSize = DEERE_CALIB_DEPTH;

	if( NULL != pu32Size )
		*pu32Size = u32AverageBufferSize;

	if( NULL != ppi16Buffer )
		*ppi16Buffer = m_pi16CalibBuffer;



	in_PREPARE_DATATRANSFER		InXferEx ={0};
	out_PREPARE_DATATRANSFER	OutXferEx = {0};

	InXferEx.i64StartAddr	= 0;
	InXferEx.u16Channel		= u16UserChannel;
	InXferEx.u32DataMask	= GetDataMaskModeTransfer(0);
	InXferEx.u32FifoMode	= GetFifoModeTransfer(u16UserChannel);
	InXferEx.bBusMasterDma	= m_BusMasterSupport;
	InXferEx.bIntrEnable	= 0;
	InXferEx.u32Segment		= 1;
	InXferEx.u32SampleSize	= sizeof(int16);

	IoctlPreDataTransfer( &InXferEx, &OutXferEx );
	CardReadMemory( m_pi16CalibBuffer, DEERE_CALIB_BUFFER_SIZE*sizeof(int16) );

	return  CS_SUCCESS;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32 CsDeereDevice::AcquireAndAverage(uInt16 u16HwChannel, int32& ri32Avg_x10)
{
	int16* pi16Buffer(NULL); uInt32 u32Size(0);
	int32	i32Ret = AcquireAndXfer( u16HwChannel, &pi16Buffer, &u32Size);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	const int32 c32SampleOffset = m_pCardState->AcqConfig.i32SampleOffset;
	LONGLONG llSum = 0;
	for (uInt32 i = 0;   i < u32Size; i++)
		llSum = llSum + pi16Buffer[i] - c32SampleOffset;
	if( 0 == u32Size )
		ri32Avg_x10  = 0;
	else
		ri32Avg_x10  = int32(((llSum*10)+u32Size/2)/u32Size);
		
	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32 CsDeereDevice::AcquireAndAverageCore(uInt16 u16HwChannel, int32* ai32_x10)
{
	int16* pi16Buffer(NULL); uInt32 u32Size(0);
	int32	i32Ret = AcquireAndXfer( u16HwChannel, &pi16Buffer, &u32Size);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	m_SqrEtb = GetSquareEtb();

	if ( 0 != m_SqrEtb )
	{
		pi16Buffer += (m_SqrEtb%(m_pCardState->u8NumOfAdc*DEERE_ADC_CORES));
		u32Size    -= (m_pCardState->u8NumOfAdc*DEERE_ADC_CORES);
	}

	const uInt32 c32SampleOffset = m_pCardState->AcqConfig.i32SampleOffset;
	LONGLONG llSum1(0), llSum2(0), llSum3(0), llSum4(0), llSum5(0), llSum6(0), llSum7(0), llSum8(0);
	u32Size &= ~0x7;
	for (uInt32 i = 0; i < u32Size; )
	{
		llSum1 = llSum1 + pi16Buffer[i++] - c32SampleOffset; llSum2 = llSum2 + pi16Buffer[i++] - c32SampleOffset;
		llSum3 = llSum3 + pi16Buffer[i++] - c32SampleOffset; llSum4 = llSum4 + pi16Buffer[i++] - c32SampleOffset;
		llSum5 = llSum5 + pi16Buffer[i++] - c32SampleOffset; llSum6 = llSum6 + pi16Buffer[i++] - c32SampleOffset;
		llSum7 = llSum7 + pi16Buffer[i++] - c32SampleOffset; llSum8 = llSum8 + pi16Buffer[i++] - c32SampleOffset;
	}
	if( 0 == u32Size )
	{
		ai32_x10[0] = 0;	ai32_x10[1] = 0;
		ai32_x10[2] = 0;	ai32_x10[3] = 0;
		ai32_x10[4] = 0;	ai32_x10[5] = 0;
		ai32_x10[6] = 0;	ai32_x10[7] = 0;
	}
	else
	{
		u32Size = u32Size/8;	// size of each Adc core data
		ai32_x10[0] = int32( ((llSum1*10)+u32Size/2)/u32Size ); ai32_x10[1] = int32( ((llSum2*10)+u32Size/2)/u32Size );
		ai32_x10[2] = int32( ((llSum3*10)+u32Size/2)/u32Size ); ai32_x10[3] = int32( ((llSum4*10)+u32Size/2)/u32Size );
		ai32_x10[4] = int32( ((llSum5*10)+u32Size/2)/u32Size ); ai32_x10[5] = int32( ((llSum6*10)+u32Size/2)/u32Size );
		ai32_x10[6] = int32( ((llSum7*10)+u32Size/2)/u32Size ); ai32_x10[7] = int32( ((llSum8*10)+u32Size/2)/u32Size );
	}
	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 CsDeereDevice::SendPosition(uInt16 u16HwChannel)
{
	if( (0 == u16HwChannel) || (u16HwChannel > DEERE_CHANNELS)  )
		return CS_INVALID_CHANNEL;
	
	const uInt16 u16ChanZeroBased = u16HwChannel - 1;
	int32 i32Position_mV = m_pCardState->ChannelParams[u16ChanZeroBased].i32Position;

	int32 i32MaxPositionOffset_mV =  int32(c_u32MaxPositionOffset_mV);
	if( m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange >= CS_GAIN_2_V )
		i32MaxPositionOffset_mV *= 10;

	if( i32Position_mV >  i32MaxPositionOffset_mV || 
		i32Position_mV < -i32MaxPositionOffset_mV )
	{
		m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = 0;
		return CS_INVALID_POSITION;
	}
	// m_pCardState->DacInfo[u16ChanZeroBased][u32Index]._UserOffsetScale corresponds to 50% of unipolar FS (25% of full InputRange)
	// we calculate Dac control delta assuming linear relationship
	int16 i16Scale = 0; uInt16 u16CentrePos = c_u16CalDacCount/2;
	uInt16* pu16DacInfo = GetDacInfoPointer( CS_CHAN_1 == u16HwChannel?DEERE_U_OFF_SCALE_1:DEERE_U_OFF_SCALE_2); 
	if( NULL != pu16DacInfo)	i16Scale = (int16)*pu16DacInfo;
	pu16DacInfo = GetDacInfoPointer( CS_CHAN_1 == u16HwChannel?DEERE_U_OFF_CENTR_1:DEERE_U_OFF_CENTR_2); 
	if( NULL != pu16DacInfo)	u16CentrePos = *pu16DacInfo;

	uInt16 u16DacData = u16CentrePos;
	if( 0 == i16Scale )
	{
		// tCal.Trace( TraceInfo, "! ! ! Uncalibrated User offet. No position control!\n" );
	}
	else
	{
		int32 i32DacDelta = int32(4*(LONGLONG(i32Position_mV)*i16Scale)/LONGLONG(m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange)); 

		u16DacData = uInt16(Max( 0, Min(c_u16CalDacCount-1, u16CentrePos + i32DacDelta))); 
		int16 i16DacDelta = int16(u16DacData - u16CentrePos);

		if( i32DacDelta != i16DacDelta )
		{
			int32 i32NewPosition_mv = i16DacDelta * m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange;
			i32NewPosition_mv /= 4*i16Scale;
			m_pCardState->ChannelParams[u16ChanZeroBased].i32Position = i32NewPosition_mv;
			
			if( m_bReportOffsetSaturation[u16ChanZeroBased] )
			{
				m_bReportOffsetSaturation[u16ChanZeroBased] = false;
				char szErrorStr[128];
				char szTrace[128];
				sprintf_s( szErrorStr, sizeof(szErrorStr),  "to %d mV on channel %d", i32NewPosition_mv, u16HwChannel );
				CsEventLog EventLog;
				EventLog.Report( CS_WARN_DC_OFFSET, szErrorStr );
				sprintf_s( szTrace, sizeof(szTrace), " !*! User offset changed from %d to %d mV on channel %d\n", i32Position_mV, i32NewPosition_mv, u16HwChannel );
				::OutputDebugString( szTrace );
			}
		}
	}
	int32 i32Ret = CS_SUCCESS;
	if ( m_pCardState->u16NulledUserOffset[u16ChanZeroBased] != u16DacData )
	{
		i32Ret = WriteToDac( CS_CHAN_1 == u16HwChannel ? DEERE_USER_OFFSET_1 : DEERE_USER_OFFSET_2, u16DacData );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		bool bFreeBuffer = false;
		if(NULL == m_pi32CalibA2DBuffer )
		{
			bFreeBuffer = true;
			m_pi32CalibA2DBuffer = (int32 *)::GageAllocateMemoryX( DEERE_CALIB_BLOCK_SIZE*sizeof(int32) );
		}
		eCalMode OldMode = m_CalibMode;
		
		i32Ret = NullOffset( u16HwChannel );
		if( bFreeBuffer )
		{
			::GageFreeMemoryX(m_pi32CalibA2DBuffer);
			m_pi32CalibA2DBuffer = NULL;
		}
		m_pCardState->u16NulledUserOffset[u16ChanZeroBased] = u16DacData;

		SendCalibModeSetting( u16HwChannel, OldMode ); 
	}
	return i32Ret;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32	CsDeereDevice::CalibrateAllChannels()
{
	int32 i32Ret = CS_SUCCESS;
	int32 i32CalibRes = CS_SUCCESS;

	if ( m_bUseDacCalibFromEEprom )
	{	
		if(ForceFastCalibrationSettingsAllChannels() )
			return CS_SUCCESS;
	}

	if( !IsFeCalibrationRequired() || m_bNeverCalibrateDc )
	{
		for( uInt16 i = 1; i <= m_pCsBoard->NbAnalogChannel; i++ )
		{
			i32Ret = SendPosition(i);
			if( CS_FAILED(i32Ret) )	return i32Ret;
		}
		return i32Ret;
	}

	// switch off potential calibration source
	uInt16 i;
	for( i = 1; i <= m_pCsBoard->NbAnalogChannel; i++ )
		SendCalibModeSetting( i );
	
	i32Ret = PrepareForCalibration( true );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SetupForCalibration( true );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = CheckCalibrator();
	if( CS_FAILED(i32Ret) )	
	{
		SetupForCalibration( false );
		PrepareForCalibration( false );
		return 0 == m_u32IgnoreCalibError ? i32Ret : CS_FALSE;
	}

	for( i = 1; i <= m_pCsBoard->NbAnalogChannel; i++ )
	{	
		i32Ret = CalibrateFrontEnd(i);
		if( CS_FAILED(i32Ret) ) 
		{
			char szErrorStr[128];
			char szTrace[256];
			sprintf_s( szErrorStr, sizeof(szErrorStr),  "%d, Channel %d reason %d", i32Ret, i+(m_pCardState->u16CardNumber-1)*m_pCsBoard->NbAnalogChannel, i32Ret%CS_ERROR_FORMAT_THRESHOLD );
			if( 0 != (m_u32IgnoreCalibError & (1<<(i-1))) )
			{
				sprintf_s( szTrace, sizeof(szTrace), "*!*!*   Ignoring calibration error: %s\n", szErrorStr);
				::OutputDebugString(szTrace);
				i32CalibRes = CS_FALSE;
			}
			else
			{
				i32CalibRes = i32Ret;	
				sprintf_s( szTrace, sizeof(szTrace), " ! ! ! Calibration error: %s\n", szErrorStr);
#if !DBG
				CsEventLog EventLog;
				EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );
#endif			
			}
		}
	}

	i32Ret = SetupForCalibration( false );
	i32Ret = PrepareForCalibration( false );
	if( CS_SUCCEEDED(i32CalibRes) && CS_FAILED(i32Ret) )
		return i32CalibRes = i32Ret;

	return i32CalibRes;
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
int32	CsDeereDevice::CalibrateFrontEnd(uInt16 u16HwChannel)
{
	//
	// *****   IMPORTANT !!!!  Always calibrate front end in CS_MODE_SINGLE  ******
	//
	ASSERT( (m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE)  != 0 );	
	if (( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE)  == 0 )
		return CS_INVALID_ACQ_MODE;

	if( (CS_CHAN_1 != u16HwChannel) && ((CS_CHAN_2 != u16HwChannel))  )
		return CS_INVALID_CHANNEL;

	if ( m_bNeverCalibrateDc )
		return CS_SUCCESS;

	const uInt16 cu16ChanZeroBased = u16HwChannel - 1;

	int32 i32Ret = CS_SUCCESS;
	//do not calibrate the second chanel in the single channel mode
	//but do not clear flag since calibration will be needed on this channel in dual mode
	//we cannot use current mode cofiguration for test since calibration is always done in single mode
	if ( (0 != (m_OldAcqConfig.u32Mode & CS_MODE_SINGLE)) && (CS_CHAN_1 != u16HwChannel) )
		return CS_SUCCESS;

	if ( !m_bForceCal[cu16ChanZeroBased] )
	{
		if( !m_pCardState->bCalNeeded[cu16ChanZeroBased] )
		{
			i32Ret = SendPosition(u16HwChannel);
			return i32Ret;
		}
		if( m_bSkipCalib )
		{
			m_bFastCalibSettingsDone[cu16ChanZeroBased] = false;
			if ( FastFeCalibrationSettings(u16HwChannel) )
			{
				TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, 0, 100 );
				return CS_SUCCESS;
			}
		}
	}

	i32Ret = SendCaptureModeSetting(m_pCardState->AcqConfig.u32Mode, u16HwChannel);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel);
	
	m_bReportOffsetSaturation[cu16ChanZeroBased] = true;

	uInt32 u32FrontEndSetup = m_pCardState->AddOnReg.u32FeSetup[cu16ChanZeroBased];
	if( 0 == (u32FrontEndSetup & DEERE_SET_DC_COUPLING) )
	{	//clear AC
		u32FrontEndSetup |= DEERE_SET_DC_COUPLING; 
		i32Ret = WriteRegisterThruNios(u32FrontEndSetup, CSPLXBASE_SETFRONTEND_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}

	// Set all DACs to the default initial value
	SetDefaultDacCalib( u16HwChannel );

	i32Ret = CalibrateOffset(u16HwChannel);
	if( CS_FAILED(i32Ret) )	
	{
		TraceCalib( TRACE_CAL_RESULT, u16HwChannel, 10, i32Ret);
		SendCalibModeSetting( u16HwChannel );
		return i32Ret;
	}

	i32Ret = DetermineIrFactor(u16HwChannel);
	if( CS_FAILED(i32Ret) )	
	{
		TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, 0, -3 );
		TraceCalib( TRACE_CAL_RESULT, u16HwChannel, 20, i32Ret);
		SendCalibModeSetting( u16HwChannel );
		return i32Ret;
	}

	i32Ret = CalibrateGain(u16HwChannel);
	if( CS_FAILED(i32Ret) )	
	{
		TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, 0, -3 );
		TraceCalib( TRACE_CAL_RESULT, u16HwChannel, 20, i32Ret);
		SendCalibModeSetting( u16HwChannel );
		return i32Ret;
	}

	TraceCalib(TRACE_ADC_MATCH, u16HwChannel);
	
	i32Ret = CalibratePosition(u16HwChannel);
	if( CS_FAILED(i32Ret) )	
	{
		TraceCalib(TRACE_CAL_PROGRESS,u16HwChannel, 0, -5 );
		TraceCalib( TRACE_CAL_RESULT, u16HwChannel, 30, i32Ret);
		SendCalibModeSetting( u16HwChannel);
		return i32Ret;
	}

	if( m_pCardState->u8NumOfAdc > 1 )
	{
		m_bAdcMatchNeeded[cu16ChanZeroBased] = !m_bSkipUserCal;
		TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, 0, 200);	
	}

	uInt32 u32Index;
	i32Ret = FindFeIndex( m_pCardState->ChannelParams[cu16ChanZeroBased].u32InputRange, &u32Index );
	if( CS_FAILED(i32Ret) )	return false;

	if( 0 == (m_pCardState->AddOnReg.u32FeSetup[cu16ChanZeroBased] & DEERE_SET_DC_COUPLING) )
	{
		i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u32FeSetup[cu16ChanZeroBased], CSPLXBASE_SETFRONTEND_CMD);
		if( CS_FAILED(i32Ret) )	return i32Ret;
	}
	SetFastCalibration(u16HwChannel);
	m_pCardState->bCalNeeded[cu16ChanZeroBased] = false;
	m_bForceCal[cu16ChanZeroBased] = false;

	i32Ret = SendCalibModeSetting(u16HwChannel);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendPosition(u16HwChannel);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, 0, 0, 1 );
	TraceCalib( TRACE_CAL_RESULT, u16HwChannel, 0, i32Ret);
	return i32Ret;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsDeereDevice::NullOffset(uInt16 u16HwChannel)
{	
	if( m_bNeverCalibrateDc )
		return CS_FALSE;
	const uInt16 u16ChanZeroBased = u16HwChannel - 1;
	const uInt16 u16DacAddr = CS_CHAN_1 == u16HwChannel ? DEERE_OFFSET_NULL_1 : DEERE_OFFSET_NULL_2;
	
	int32 i32Ret = SendCalibModeSetting(u16HwChannel, ecmCalModeDc ); //connect dc cal
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	int32 i32SetLevel_uV;
	i32Ret = SendCalibDC(m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance, 0, true, &i32SetLevel_uV);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	int32 i32Grd;
	i32Ret = ReadCalibA2D(i32SetLevel_uV, eciCalGrd, &i32Grd );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	//Determine polarity and scale of the comtrol
	i32Ret = WriteToDac( u16DacAddr, 3*c_u16CalDacCount/4 );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	int32 i32Chan1;
	i32Ret = ReadCalibA2D(i32SetLevel_uV, eciCalChan, &i32Chan1 );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	i32Ret = WriteToDac( u16DacAddr, c_u16CalDacCount/4 );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	int32 i32Chan2;
	i32Ret = ReadCalibA2D(i32SetLevel_uV, eciCalChan, &i32Chan2 );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	int32 i32Scale = i32Chan1 - i32Chan2;
	if( 0 == i32Scale ) return CS_DAC_CALIB_FAILURE;

	uInt16 u16Code = c_u16CalDacCount/2;
	uInt32 Iter = 0;
	int32 i32Cor = c_u16CalDacCount;
	int32 i32PrevCor = i32Cor;
	uInt16 u16NewCode = u16Code;
	while ( labs(i32Cor) >= 1 && Iter < DEERE_MAX_CAL_ITER  )
	{
		u16Code = u16NewCode;
		i32Ret = WriteToDac( u16DacAddr, u16Code );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = ReadCalibA2D(i32SetLevel_uV, eciCalChan, &i32Chan1 );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		if( 0 == Iter ) i32Chan2 = i32Chan1;

		int32 i32Delta = i32Grd - i32Chan1;
		if( labs(i32Delta) < DEERE_NULL_OFFSET_ERROR )	break;
		LONGLONG llCor = LONGLONG(c_u16CalDacCount/2) * i32Delta;
		llCor += llCor*i32Scale > 0 ? i32Scale/2 : -i32Scale/2;
		llCor /= i32Scale;
		i32Cor = int32(llCor);
		if( labs(labs(i32PrevCor) - labs(i32Cor)) < 2 && labs(i32Cor) > 1 && i32PrevCor*i32Cor < 0 )
			i32Scale = (4*i32Scale)/3;
		i32PrevCor = i32Cor;
		TraceCalib(TRACE_NULL_OFFSET |TRACE_VERB, u16Code, (uInt32)labs(i32Scale), i32Delta, i32Cor);
				
		u16NewCode = (uInt16)Max( 0, Min( c_u16CalDacCount-1, int32(u16Code) + i32Cor ));
		Iter++;
	}

	if( DEERE_MAX_CAL_ITER == Iter )
		return CS_NULL_OFFSET_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * u16HwChannel;

	TraceCalib(TRACE_NULL_OFFSET, u16Code, Iter,i32Grd - i32Chan2, i32Grd - i32Chan1);
	TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, u16Code, 1);
	return CS_SUCCESS;
}



int32 CsDeereDevice::CompensateOffset(uInt16 u16HwChannel, bool bCoarse)
{
	const uInt16 u16ChanZeroBased = u16HwChannel - 1;
	const uInt16 u16DacAddr = bCoarse ? (CS_CHAN_1 == u16HwChannel ? DEERE_USER_OFFSET_1 : DEERE_USER_OFFSET_2) : 
		                                (CS_CHAN_1 == u16HwChannel ? DEERE_OFFSET_COMP_1 : DEERE_OFFSET_COMP_2);

	uInt16* pu16DacInfo = GetDacInfoPointer( CS_CHAN_1 == u16HwChannel?DEERE_U_OFF_CENTR_1:DEERE_U_OFF_CENTR_2); 

	if( bCoarse )
		*pu16DacInfo = c_u16CalDacCount/2;

	int32 i32SetLevel_uV;
	int32 i32Ret = SendCalibDC(m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance, 0, false, &i32SetLevel_uV);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	const int32 ci32Target = -(int32)((LONGLONG(i32SetLevel_uV)*m_pCardState->AcqConfig.i32SampleRes)/(m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange * 50));

	int32 i32Off1, i32Off2;
	//Determine polarity and scale of the control
	uInt16 u16ScaleOffset = c_u16CalDacCount/4;
	if( CS_GAIN_200_MV == m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange ||
		CS_GAIN_2_V == m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange )
		u16ScaleOffset = c_u16CalDacCount/10;

	i32Ret = WriteToDac( u16DacAddr, c_u16CalDacCount/2 + u16ScaleOffset );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = AcquireAndAverage(u16HwChannel, i32Off1);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	i32Ret = WriteToDac( u16DacAddr, c_u16CalDacCount/2 - u16ScaleOffset );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	i32Ret = AcquireAndAverage(u16HwChannel, i32Off2);
	if( CS_FAILED(i32Ret) )	return i32Ret;
#ifdef _WINDOWS_	
	int32 i32Scale = int32((LONGLONG(c_u16CalDacCount)*(i32Off1 - i32Off2))/(4I64*LONGLONG(u16ScaleOffset)));
#else
	/* is I64 really needed? */
	int32 i32Scale = int32((LONGLONG(c_u16CalDacCount)*(i32Off1 - i32Off2))/(4*LONGLONG(u16ScaleOffset)));
#endif
	if( 0 == i32Scale ) return CS_DAC_CALIB_FAILURE;

	uInt16 u16Code = c_u16CalDacCount/2;
	uInt32 Iter = 0;
	int32 i32Cor = c_u16CalDacCount;
	int32 i32PevCor = i32Cor;
	uInt16 u16NewCode = u16Code;
	while ( labs(i32Cor) >= 1 && Iter < DEERE_MAX_CAL_ITER  )
	{
		u16Code = u16NewCode;
		i32Ret = WriteToDac( u16DacAddr, u16Code );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = AcquireAndAverage(u16HwChannel, i32Off1);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		if( 0 == Iter ) i32Off2 = i32Off1;
		
		int32 i32Delta = ci32Target - i32Off1;
		LONGLONG llCor = LONGLONG(c_u16CalDacCount/2) * i32Delta ;
		llCor += llCor*i32Scale > 0 ? i32Scale/2 : -i32Scale/2;
		llCor /= i32Scale;
		i32Cor = int32(llCor);
		if( labs(labs(i32PevCor) - labs(i32Cor)) < 2 && labs(i32Cor) > 1 && i32PevCor*i32Cor < 0 )
			i32Scale = (4*i32Scale)/3;
		i32PevCor = i32Cor;
		TraceCalib(TRACE_CAL_FE_OFFSET |TRACE_VERB, u16Code, (uInt32)labs(i32Scale), i32Delta, i32Cor);
				
		u16NewCode = (uInt16)Max( 0, Min( c_u16CalDacCount-1, int32(u16Code) + i32Cor ));
		Iter++;
	}

	if( DEERE_MAX_CAL_ITER <= Iter )
		return (CS_COARSE_OFFSET_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * u16HwChannel);
	if( bCoarse )
		*pu16DacInfo = u16Code;

	TraceCalib(TRACE_CAL_FE_OFFSET, u16Code, Iter, ci32Target - i32Off2, ci32Target - i32Off1);
	TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, u16Code, bCoarse?12:2);
	return CS_SUCCESS;
}


int32 CsDeereDevice::CalibrateOffset(uInt16 u16HwChannel)
{	
	int32 i32Ret = NullOffset(u16HwChannel);
	if( CS_FAILED(i32Ret) )	{TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, 0, -1);return i32Ret;}
	
	i32Ret = CompensateOffset(u16HwChannel, true);
	if( CS_FAILED(i32Ret) )	{TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, 0, -12); return i32Ret;}
	i32Ret = CompensateOffset(u16HwChannel, false);
	if( CS_FAILED(i32Ret) )	{TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, 0, -2); return i32Ret; }

	i32Ret = DetermineAdcDataOrder(u16HwChannel);
	if( CS_FAILED(i32Ret) )	{TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, 0, -10);return i32Ret;}

	i32Ret = MatchAdcOffset(u16HwChannel, false);
	if( CS_FAILED(i32Ret) )	{TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, 0, -6);return i32Ret;}
	
	i32Ret = MatchAdcOffset(u16HwChannel, true);
	if( CS_FAILED(i32Ret) ){TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, 0, -7);return i32Ret;}
	
	return i32Ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32 CsDeereDevice::DetermineAdcDataOrder(uInt16 u16HwChannel)
{
	const uInt16 u16ChanZeroBased = u16HwChannel - 1;
	int32 i32[8], i32_N[8]; //Dummy variables
	int32 i32Ret; 

	if ( ! m_bUpdateCoreOrder[u16ChanZeroBased] )
		return CS_SUCCESS;

	//Determine core and adc aligment
	i32Ret = WriteToAdcRegAllChannelCores(u16HwChannel, DEERE_ADC_COARSE_OFFSET, c_u8AdcCalRegSpan/2 );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	i32Ret = AcquireAndAverageCore(u16HwChannel, i32_N);
	if( CS_FAILED(i32Ret) )	return i32Ret;

//	RtlZeroMemory( m_u8AdcIndex, sizeof(m_u8AdcIndex) );
	for( uInt8 u8 = 0; u8 < m_pCardState->u8NumOfAdc; u8++ )
	{
		uInt8 u8Adc = AdcForChannel(u16ChanZeroBased, u8 );
		for( uInt8 u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
		{
			i32Ret = WriteToAdcReg(u8Core, u8Adc, DEERE_ADC_COARSE_OFFSET, 0 );
			if( CS_FAILED(i32Ret) )	return i32Ret;
			i32Ret = AcquireAndAverageCore(u16HwChannel, i32);
			if( CS_FAILED(i32Ret) )	return i32Ret;
			
			int32 MaxDif = 0;
			int nIx = 0;
			for( int i = 0; i < DEERE_ADC_CORES*m_pCardState->u8NumOfAdc; i++ )
			{
				if( labs( i32[i] - i32_N[i] ) > MaxDif )
				{
					MaxDif = labs( i32[i] - i32_N[i] );
					nIx = i;
				}
			}
			m_u8AdcIndex[u8Adc][u8Core] = (uInt8) nIx;
			i32_N[nIx] = i32[nIx];
		}
	}
	TraceCalib(TRACE_ADC_CORE_ORDER, u16HwChannel );
	i32Ret = WriteToAdcRegAllChannelCores(u16HwChannel, DEERE_ADC_COARSE_OFFSET, c_u8AdcCalRegSpan/2 );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	m_bUpdateCoreOrder[u16ChanZeroBased] = FALSE;

	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32 CsDeereDevice::DetermineIrFactor(uInt16 u16HwChannel)
{
	const uInt16 u16ChanZeroBased = u16HwChannel - 1;
	int32 i32SetLevel_uV;
	int32 i32Ret = SendCalibDC(m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance, m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange, true, &i32SetLevel_uV);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	const int32 ci32Target = -(int32)((LONGLONG(i32SetLevel_uV)*m_pCardState->AcqConfig.i32SampleRes)/(m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange * 50));

	int32 i32GMin[DEERE_ADC_COUNT*DEERE_ADC_CORES], i32GMax[DEERE_ADC_COUNT*DEERE_ADC_CORES];;

	i32Ret = WriteToAdcRegAllChannelCores(u16HwChannel, DEERE_ADC_COARSE_GAIN, 0 );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	i32Ret = AcquireAndAverageCore(u16HwChannel, i32GMin);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = WriteToAdcRegAllChannelCores(u16HwChannel, DEERE_ADC_COARSE_GAIN, c_u8AdcCoarseGainSpan - 1 );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	i32Ret = AcquireAndAverageCore(u16HwChannel, i32GMax);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = WriteToAdcRegAllChannelCores(u16HwChannel, DEERE_ADC_COARSE_GAIN, c_u8AdcCoarseGainSpan/2 );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	bool bMin = false,  bMax = false;
	int32 i32TargetModifiedMin = ci32Target;
	int32 i32TargetModifiedMax = ci32Target;
	for( uInt8 u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
	{
		uInt8 u8Adc = AdcForChannel( u16ChanZeroBased, u8AdcCount );
		for( uInt8 u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
		{
			if( i32GMin[m_u8AdcIndex[u8Adc][u8Core]] > ci32Target )
			{
				i32TargetModifiedMin = Max(i32TargetModifiedMin, i32GMin[m_u8AdcIndex[u8Adc][u8Core]] );
				bMin = true;
			}
			if( i32GMax[m_u8AdcIndex[u8Adc][u8Core]] < ci32Target )
			{
				i32TargetModifiedMax = Min(i32TargetModifiedMax, i32GMax[m_u8AdcIndex[u8Adc][u8Core]] );
				bMax = true;
			}
		}
	}

	uInt16* pu16DacInfo = GetDacInfoPointer( CS_CHAN_1 == u16HwChannel?DEERE_IR_SCALE_1:DEERE_IR_SCALE_2 );
	*pu16DacInfo = DEERE_IR_SCALE_FACTOR;
	
	if( bMin )
	{
		if( bMax )
		{
			::OutputDebugString("! ! ! ! Adc cores are on opposite ends of adjustment range\n");
			return CS_GAIN_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * u16HwChannel;
		}
		else
		{
			*pu16DacInfo = (uInt16) ((LONGLONG(i32TargetModifiedMin) * DEERE_IR_SCALE_FACTOR)/ ci32Target); 
			
		}
	}
	else if( bMax)
	{
		*pu16DacInfo = (uInt16) ((LONGLONG(i32TargetModifiedMax) * DEERE_IR_SCALE_FACTOR)/ ci32Target); 
	}

	TraceCalib(TRACE_CAL_SCALE_GAIN, u16HwChannel,*pu16DacInfo);
	
	return CS_SUCCESS;
}



void CsDeereDevice::CheckCoerceResult(int32& rResult )
{
	if( CS_SUCCESS == rResult )
	{
		for( uInt32 i = 0; i < (m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE); i++ )
		{
			if( DEERE_IR_SCALE_FACTOR != *GetDacInfoPointer( 0 == i?DEERE_IR_SCALE_1:DEERE_IR_SCALE_2 ) )
				rResult = CS_CONFIG_CHANGED;
		}
	}
}		
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32 CsDeereDevice::CalibrateGain(uInt16 u16HwChannel)
{
	const uInt16 u16ChanZeroBased = u16HwChannel - 1;

	//Find the best coarse gain setting
	int32 i32SetLevel_uV;
	int32 i32Ret = SendCalibDC(m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance, m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange, true, &i32SetLevel_uV);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	int32 i32TargetPos = -(int32)((LONGLONG(i32SetLevel_uV)*m_pCardState->AcqConfig.i32SampleRes)/(m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange * 50));
	uInt16* pu16DacInfo = GetDacInfoPointer( CS_CHAN_1 == u16HwChannel?DEERE_IR_SCALE_1:DEERE_IR_SCALE_2 );
	i32TargetPos = int32((LONGLONG(i32TargetPos)* (*pu16DacInfo))/ DEERE_IR_SCALE_FACTOR);
			
	int32 i32_P[DEERE_ADC_COUNT*DEERE_ADC_CORES];
	
	uInt8 u8Adc, u8Core, u8AdcCount;
	int32 i32Delta[DEERE_ADC_COUNT][DEERE_ADC_CORES];
	for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
	{
		u8Adc = AdcForChannel( u16ChanZeroBased, u8AdcCount );
		for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
			i32Delta[u8Adc][u8Core]= 0x7fffffff;
	}

	uInt8 u8CoarseGainBest[DEERE_ADC_COUNT][DEERE_ADC_CORES] = {0};
	for( uInt8 u8CoarseGain = 0; u8CoarseGain < c_u8AdcCoarseGainSpan; u8CoarseGain++ )
	{
		i32Ret = WriteToAdcRegAllChannelCores(u16HwChannel, DEERE_ADC_COARSE_GAIN, u8CoarseGain );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = AcquireAndAverageCore(u16HwChannel, i32_P);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
		{
			u8Adc = AdcForChannel( u16ChanZeroBased, u8AdcCount );
			for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
			{
				if( i32Delta[u8Adc][u8Core] > labs( i32_P[m_u8AdcIndex[u8Adc][u8Core]] - i32TargetPos ) )
				{
					i32Delta[u8Adc][u8Core] = labs( i32_P[m_u8AdcIndex[u8Adc][u8Core]] - i32TargetPos );
					u8CoarseGainBest[u8Adc][u8Core] = u8CoarseGain;
				}
				TraceCalib(TRACE_CAL_GAIN|TRACE_VERB, u8CoarseGain, u8Adc*10+u8Core, i32TargetPos, i32_P[m_u8AdcIndex[u8Adc][u8Core]]);
			}
		}
	}
	//Write the best coarse gain setting
	for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
	{
		u8Adc = AdcForChannel( u16ChanZeroBased, u8AdcCount );
		for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
		{
			i32Ret = WriteToAdcReg( u8Core, u8Adc, DEERE_ADC_COARSE_GAIN, u8CoarseGainBest[u8Adc][u8Core] );
			if( CS_FAILED(i32Ret) )	return i32Ret;
		}
	}
	i32Ret = AcquireAndAverageCore(u16HwChannel, i32_P);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
	{
		u8Adc = AdcForChannel( u16ChanZeroBased, u8AdcCount );
		for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
		{
			TraceCalib(TRACE_CAL_GAIN, u8CoarseGainBest[u8Adc][u8Core], u8Adc, u8Core, i32_P[m_u8AdcIndex[u8Adc][u8Core]] - i32TargetPos );
		}
	}

	TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, 0, 3);

	i32Ret = MatchAdcGain(u16HwChannel, false);
	if( CS_FAILED(i32Ret) )
	{
		TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, 0, -8);
		return i32Ret;
	}
	
	i32Ret = MatchAdcGain(u16HwChannel, true);
	if( CS_FAILED(i32Ret) )
	{
		TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, 0, -9);
		return i32Ret;
	}

	return i32Ret;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32 CsDeereDevice::CalibratePosition(uInt16 u16HwChannel)
{
	const uInt16 u16ChanZeroBased = u16HwChannel - 1;
	const uInt16 u16DacAddr = CS_CHAN_1 == u16HwChannel ? DEERE_USER_OFFSET_1 : DEERE_USER_OFFSET_2;
	
	int32 i32SetLevel_uV;
	int32 i32Ret = SendCalibDC(m_pCardState->ChannelParams[u16ChanZeroBased].u32Impedance, 0, false, &i32SetLevel_uV);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	int32 i32TargetPos = (int32)(-(LONGLONG(i32SetLevel_uV)*m_pCardState->AcqConfig.i32SampleRes)/(m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange * 50)) + (m_pCardState->AcqConfig.i32SampleRes*5)/2;
	int32 i32TargetNeg = (int32)(-(LONGLONG(i32SetLevel_uV)*m_pCardState->AcqConfig.i32SampleRes)/(m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange * 50)) - (m_pCardState->AcqConfig.i32SampleRes*5)/2;
	uInt16* pu16DacInfo = GetDacInfoPointer( CS_CHAN_1 == u16HwChannel?DEERE_IR_SCALE_1:DEERE_IR_SCALE_2 );
	i32TargetPos = int32((LONGLONG(i32TargetPos)* (*pu16DacInfo))/ DEERE_IR_SCALE_FACTOR);
	i32TargetNeg = int32((LONGLONG(i32TargetNeg)* (*pu16DacInfo))/ DEERE_IR_SCALE_FACTOR);

	uInt16 u16CentrePos = c_u16CalDacCount/2;
	pu16DacInfo = GetDacInfoPointer( CS_CHAN_1 == u16HwChannel?DEERE_U_OFF_CENTR_1:DEERE_U_OFF_CENTR_2); 
	if( NULL != pu16DacInfo) u16CentrePos = *pu16DacInfo;

	int32 i32Off1, i32Off2;
	//in Low gain the offset at half DAC scale will saturate signal
	bool bLowRange = (CS_GAIN_200_MV == m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange) || (CS_GAIN_2_V == m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange);
	const uInt16 cu16ControlOffset = bLowRange? c_u16CalDacCount/8 : c_u16CalDacCount/4;
	//Determine polarity and scale of the comtrol
	i32Ret = WriteToDac( u16DacAddr, u16CentrePos + cu16ControlOffset );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	i32Ret = AcquireAndAverage(u16HwChannel, i32Off1);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = WriteToDac( u16DacAddr, u16CentrePos - cu16ControlOffset );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	i32Ret = AcquireAndAverage(u16HwChannel, i32Off2);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	LONGLONG llScale = LONGLONG(c_u16CalDacCount)*LONGLONG(i32Off1 - i32Off2)/LONGLONG(4*cu16ControlOffset);
	int32 i32Scale = int32(llScale);
	if( 0 == i32Scale ) return CS_DAC_CALIB_FAILURE;

	//Positive Side
	uInt16 u16CodePos = i32Scale > 0 ? u16CentrePos - cu16ControlOffset : u16CentrePos + cu16ControlOffset;
	uInt32 Iter = 0;
	int32  i32Cor = c_u16CalDacCount;
	int32 i32PevCor = i32Cor;
	uInt16 u16NewCode = u16CodePos;
	while ( labs(i32Cor) >= 1 && Iter < DEERE_MAX_CAL_ITER )
	{
		u16CodePos = u16NewCode;
		i32Ret = WriteToDac( u16DacAddr, u16CodePos );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = AcquireAndAverage(u16HwChannel, i32Off1);
		if( CS_FAILED(i32Ret) )	return i32Ret;
				
		int32 i32Delta = i32TargetPos - i32Off1;
		LONGLONG llCor = LONGLONG(c_u16CalDacCount/2) * i32Delta;
		llCor += llCor*i32Scale > 0 ? i32Scale/2 : -i32Scale/2;
		llCor /= i32Scale;
		i32Cor = int32(llCor);
		if( labs(labs(i32PevCor) - labs(i32Cor)) < 2 && labs(i32Cor) > 1 && i32PevCor*i32Cor < 0 )
			i32Scale = (4*i32Scale)/3;
		i32PevCor = i32Cor;
		TraceCalib(TRACE_CAL_POS |TRACE_VERB, u16NewCode, (uInt32)labs(i32Scale), i32Delta, i32Cor);
		
		int32 i32New = u16CodePos + i32Cor;
		u16NewCode = (uInt16)Max( 0, Min( c_u16CalDacCount - 1, i32New) );
		Iter++;
	}

	if( DEERE_MAX_CAL_ITER == Iter )
	{
		i32Ret = WriteToDac( u16DacAddr, u16CentrePos );
		pu16DacInfo = GetDacInfoPointer( CS_CHAN_1 == u16HwChannel?DEERE_U_OFF_SCALE_1:DEERE_U_OFF_SCALE_2); 
		if( NULL != pu16DacInfo)
			*pu16DacInfo = 0;
		TraceCalib(TRACE_CAL_POS, u16CodePos, 0, i32Off1);
		return CS_POSITION_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * u16HwChannel;
	}

	//Negative Side
	uInt16 u16CodeNeg = i32Scale > 0 ? u16CentrePos + cu16ControlOffset : u16CentrePos - cu16ControlOffset;
	Iter = 0;
	i32Cor = c_u16CalDacCount;
	u16NewCode = u16CodeNeg;
	while ( labs(i32Cor) >= 1 && Iter < DEERE_MAX_CAL_ITER  )
	{
		u16CodeNeg = u16NewCode;
		i32Ret = WriteToDac( u16DacAddr, u16CodeNeg );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		i32Ret = AcquireAndAverage(u16HwChannel, i32Off2);
		if( CS_FAILED(i32Ret) )	return i32Ret;
				
		int32 i32Delta =  i32TargetNeg - i32Off2;
		i32Cor = (int32(c_u16CalDacCount/2) * i32Delta + i32Scale/2)/i32Scale;
		TraceCalib(TRACE_CAL_POS |TRACE_VERB, u16CodeNeg, 3, i32Delta, i32Cor);
				
		int32 i32New = u16CodeNeg + i32Cor;
		u16NewCode = (uInt16)Max( 0, Min( c_u16CalDacCount - 1, i32New) );
		Iter++;
	}
	if( DEERE_MAX_CAL_ITER == Iter )
	{
		i32Ret = WriteToDac( u16DacAddr, u16CentrePos);
		pu16DacInfo = GetDacInfoPointer( CS_CHAN_1 == u16HwChannel?DEERE_U_OFF_SCALE_1:DEERE_U_OFF_SCALE_2); 
		if( NULL != pu16DacInfo) *pu16DacInfo = 0;
		TraceCalib(TRACE_CAL_POS, u16CodePos, u16CodeNeg, i32Off1, i32Off2);
		return CS_POSITION_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * u16HwChannel;
	}
	WriteToDac( u16DacAddr, u16CentrePos);
	m_pCardState->u16NulledUserOffset[u16ChanZeroBased] = c_u16CalDacCount; //Force renulling after position controle is set
	pu16DacInfo = GetDacInfoPointer( CS_CHAN_1 == u16HwChannel?DEERE_U_OFF_SCALE_1:DEERE_U_OFF_SCALE_2); 
	if( NULL != pu16DacInfo) *pu16DacInfo = uInt16(int16(u16CodePos) - int16(u16CodeNeg));

	TraceCalib(TRACE_CAL_POS, u16CodePos, u16CodeNeg, i32Off1, i32Off2);
	TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, u16CodePos, 5);
	return CS_SUCCESS;
}

bool CsDeereDevice::isAdcPingPong()
{
	if( 0 == m_MasterIndependent->m_pCsBoard->NbAnalogChannel )
		return false;

	return  (m_MasterIndependent->m_pCardState->AddOnReg.u32Decimation <= 4UL/(m_MasterIndependent->m_pCsBoard->NbAnalogChannel));
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32 CsDeereDevice::MatchAdcOffset(uInt16 u16HwChannel, bool bFine)
{
	if( m_bNeverMatchAdcOffset || bFine )
		return CS_FALSE;
	
	const uInt16 cu16ChanZeroBased = u16HwChannel - 1;

	//Match offset
	int32 i32SetLevel_uV;
	int32 i32Ret = SendCalibDC(m_pCardState->ChannelParams[cu16ChanZeroBased].u32Impedance, 0, false, &i32SetLevel_uV );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	const int32 ci32Target = -(int32)((LONGLONG(i32SetLevel_uV)*m_pCardState->AcqConfig.i32SampleRes)/(m_pCardState->ChannelParams[cu16ChanZeroBased].u32InputRange * 50));

	uInt16 u16Addr = bFine ? DEERE_ADC_FINE_OFFSET : DEERE_ADC_COARSE_OFFSET;
	uInt32 u32TraceId = bFine ? TRACE_MATCH_OFF_FINE : TRACE_MATCH_OFF_MED;
	
	int32 i32_1[DEERE_ADC_COUNT*DEERE_ADC_CORES], i32_2[DEERE_ADC_COUNT*DEERE_ADC_CORES];
	
	uInt8 u8Step1 = (uInt8) (3*(1+c_u8AdcCalRegSpan)/4);
	uInt8 u8Step2 = (uInt8) (1*(1+c_u8AdcCalRegSpan)/4);
	uInt8 u8StepDelta = u8Step1-u8Step2;

	i32Ret = WriteToAdcRegAllChannelCores( u16HwChannel, u16Addr, u8Step1);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	i32Ret = AcquireAndAverageCore(u16HwChannel, i32_1);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = WriteToAdcRegAllChannelCores( u16HwChannel, u16Addr, u8Step2);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = AcquireAndAverageCore(u16HwChannel, i32_2);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	int32 i32Scale[DEERE_ADC_COUNT][DEERE_ADC_CORES]; //separate scale for all cores
	uInt8 u8[DEERE_ADC_COUNT][DEERE_ADC_CORES];  
	int32 i32Cor;
	uInt8 u8Adc, u8Core, u8AdcCount;

	for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
	{
		u8Adc = AdcForChannel( cu16ChanZeroBased, u8AdcCount );
		for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
		{
			i32Scale[u8Adc][u8Core] = i32_1[m_u8AdcIndex[u8Adc][u8Core]] - i32_2[m_u8AdcIndex[u8Adc][u8Core]];
			if( 0 == i32Scale[u8Adc][u8Core] )
				return (CS_FINE_OFFSET_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * u16HwChannel);
			i32Cor = (int32(u8StepDelta) * (ci32Target - i32_2[m_u8AdcIndex[u8Adc][u8Core]]) + i32Scale[u8Adc][u8Core]/2)/i32Scale[u8Adc][u8Core];
			u8[u8Adc][u8Core] = (uInt8)Max(0, Min(int32(c_u8AdcCalRegSpan), int32(u8Step2) + i32Cor));
		}
	}

	bool bDoit[DEERE_ADC_COUNT][DEERE_ADC_CORES];
	for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
	{
		u8Adc = AdcForChannel( cu16ChanZeroBased, u8AdcCount );
		for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
			bDoit[u8Adc][u8Core] = true;
	}
	uInt8 u8Iter = 0;
	while (  u8Iter < DEERE_MAX_CAL_ITER )
	{
		for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
		{
			u8Adc = AdcForChannel( cu16ChanZeroBased, u8AdcCount );
			for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
			{
				if( !bDoit[u8Adc][u8Core] ) continue;
				i32Ret = WriteToAdcReg( u8Core, u8Adc, u16Addr, u8[u8Adc][u8Core]);
				if( CS_FAILED(i32Ret) )	return i32Ret;
			}
		}
		i32Ret = AcquireAndAverageCore(u16HwChannel, i32_1);
		if( CS_FAILED(i32Ret) )	return i32Ret;
		bool bStop = true;
		for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
		{
			u8Adc = AdcForChannel( cu16ChanZeroBased, u8AdcCount );
			for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
			{
				if( !bDoit[u8Adc][u8Core] )
					continue;
				i32Cor = (int32(u8StepDelta) * (ci32Target - i32_1[m_u8AdcIndex[u8Adc][u8Core]]) + i32Scale[u8Adc][u8Core]/2)/i32Scale[u8Adc][u8Core];
				TraceCalib(u32TraceId|TRACE_VERB, u8[u8Adc][u8Core], u8Adc*10 + u8Core, ci32Target-i32_1[m_u8AdcIndex[u8Adc][u8Core]], i32Cor);
				u8[u8Adc][u8Core] = (uInt8)Max(0, Min(int32(c_u8AdcCalRegSpan), int32(u8[u8Adc][u8Core]) + i32Cor));

				// Strict in Coarse Offset but flexible in Fine Offset with Tolerance
				if ( bFine )
				{
					if( m_u32MatchAdcOffsetTolerance < (uInt32) labs(i32Cor) )
						bStop = false;
					else
						bDoit[u8Adc][u8Core] = false;
				}
				else
				{
					if( 0 != i32Cor )
						bStop = false;
					else
						bDoit[u8Adc][u8Core] = false;
				}
			}
		}
		if ( bStop )
			break;

		u8Iter++;
	}

	for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
	{
		u8Adc = AdcForChannel( cu16ChanZeroBased, u8AdcCount );
		for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
			TraceCalib( u32TraceId, u16HwChannel, u8Adc*10+u8Core, u8[u8Adc][u8Core], i32Scale[u8Adc][u8Core] );
	}

	TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, u8[AdcForChannel( cu16ChanZeroBased, 0 )][0], bFine?7:6, u8[AdcForChannel( cu16ChanZeroBased, 0 )][0] - u8[AdcForChannel( cu16ChanZeroBased, 0 )][1] );
	if( u8Iter == DEERE_MAX_CAL_ITER )
		return CS_FINE_OFFSET_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * u16HwChannel;
	return CS_SUCCESS;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32 CsDeereDevice::MeasureAdcGain(uInt16 u16HwChannel, int32* ai32Gain, int32* pi32Target)
{
	int32 i32Ret, i32P[DEERE_ADC_COUNT*DEERE_ADC_CORES], i32N[DEERE_ADC_COUNT*DEERE_ADC_CORES]; 
	const uInt16 cu16ChanZeroBased = u16HwChannel - 1;

	int32 i32P_uV, i32N_uV;
	i32Ret = SendCalibDC(m_pCardState->ChannelParams[cu16ChanZeroBased].u32Impedance, m_pCardState->ChannelParams[cu16ChanZeroBased].u32InputRange, true, &i32P_uV);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	i32Ret = AcquireAndAverageCore(u16HwChannel, i32P);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
//	sprintf(szDebugTxt, "P_uV[0 1 ...] = %x, %x, %x, %x, %x, %x, %x, %x\n", i32P[0], i32P[1], i32P[2], i32P[3], i32P[4],
//													i32P[5], i32P[6], i32P[7]);
//	t << szDebugTxt;


	i32Ret = SendCalibDC(m_pCardState->ChannelParams[cu16ChanZeroBased].u32Impedance, m_pCardState->ChannelParams[cu16ChanZeroBased].u32InputRange, false, &i32N_uV);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	i32Ret = AcquireAndAverageCore(u16HwChannel, i32N);
	if( CS_FAILED(i32Ret) )	return i32Ret;

//	sprintf(szDebugTxt, "N_uV[0 1 ...] = %x, %x, %x, %x, %x, %x, %x, %x\n", i32N[0], i32N[1], i32N[2], i32N[3], i32N[4],
//													i32N[5], i32N[6], i32N[7]);
//	t << szDebugTxt;

	if( NULL != pi32Target )
	{
		i32P_uV -= i32N_uV;
		int32 i32Target = -(int32)((LONGLONG(i32P_uV)*m_pCardState->AcqConfig.i32SampleRes)/(m_pCardState->ChannelParams[cu16ChanZeroBased].u32InputRange * 50));
		uInt16* pu16DacInfo = GetDacInfoPointer( CS_CHAN_1 == u16HwChannel?DEERE_IR_SCALE_1:DEERE_IR_SCALE_2 );
		*pi32Target = int32((LONGLONG(i32Target)* (*pu16DacInfo))/ DEERE_IR_SCALE_FACTOR);
	}

	for( uInt8 u8 = 0; u8 < DEERE_ADC_COUNT*DEERE_ADC_CORES; u8++ )
		ai32Gain[u8] = i32P[u8] - i32N[u8];

//	sprintf(szDebugTxt, "Gain[0 1 ...] = %x, %x, %x, %x, %x, %x, %x, %x\n", ai32Gain[0], ai32Gain[1], ai32Gain[2], ai32Gain[3], ai32Gain[4],
//													ai32Gain[5], ai32Gain[6], ai32Gain[7]);
//	t << szDebugTxt;

	return CS_SUCCESS;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32 CsDeereDevice::MatchAdcGain(uInt16 u16HwChannel, bool bFine)
{
	if( m_bNeverMatchAdcGain || bFine )	return CS_FALSE;

	const uInt16 cu16ChanZeroBased = u16HwChannel - 1;
	uInt16 u16Addr = bFine ? DEERE_ADC_FINE_GAIN : DEERE_ADC_MEDIUM_GAIN;
	uInt32 u32TraceId = bFine ? TRACE_MATCH_GAIN_FINE : TRACE_MATCH_GAIN_MED;

	int32 i32Ret = WriteToAdcRegAllChannelCores( u16HwChannel, u16Addr, 3*c_u8AdcCalRegSpan/4);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	int32 i32Target, i32G1[DEERE_ADC_COUNT*DEERE_ADC_CORES], i32G2[DEERE_ADC_COUNT*DEERE_ADC_CORES]; 
	i32Ret = MeasureAdcGain(u16HwChannel, i32G1, &i32Target);
	if( CS_FAILED(i32Ret) )
		return i32Ret;
	
	i32Ret = WriteToAdcRegAllChannelCores( u16HwChannel, u16Addr, c_u8AdcCalRegSpan/4);
	if( CS_FAILED(i32Ret) )
		return i32Ret;
	i32Ret = MeasureAdcGain(u16HwChannel, i32G2);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	int32 i32Scale[DEERE_ADC_COUNT][DEERE_ADC_CORES]; //separate scale for all cores
	uInt8 u8[DEERE_ADC_COUNT][DEERE_ADC_CORES], u8AdcCount, u8Adc, u8Core;
	int32 i32Cor;
	for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
	{
		u8Adc = AdcForChannel( cu16ChanZeroBased, u8AdcCount );
		for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
		{
			i32Scale[u8Adc][u8Core] = i32G1[m_u8AdcIndex[u8Adc][u8Core]] - i32G2[m_u8AdcIndex[u8Adc][u8Core]];
			if( 0 == i32Scale[u8Adc][u8Core] )
				return (CS_GAIN_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * u16HwChannel);
			i32Cor = (int32(c_u8AdcCalRegSpan/2) * (i32G2[m_u8AdcIndex[u8Adc][u8Core]] - i32Target) + i32Scale[u8Adc][u8Core]/2)/i32Scale[u8Adc][u8Core];

			TraceCalib(u32TraceId|TRACE_VERB, c_u8AdcCalRegSpan/4, u8Adc*10 + u8Core, i32G1[m_u8AdcIndex[u8Adc][u8Core]]-i32Target, -i32Cor);
			u8[u8Adc][u8Core] = (uInt8)Max(0, Min(int32(c_u8AdcCalRegSpan), int32(c_u8AdcCalRegSpan/4) - i32Cor));
		}		
	}
	
	bool bDoit[DEERE_ADC_COUNT][DEERE_ADC_CORES];
	for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
	{
		u8Adc = AdcForChannel( cu16ChanZeroBased, u8AdcCount );
		for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
			bDoit[u8Adc][u8Core] = true;
	}
	uInt8 u8Iter = 0;
	while (  u8Iter < DEERE_MAX_CAL_ITER )
	{
		for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
		{
			u8Adc = AdcForChannel( cu16ChanZeroBased, u8AdcCount );
			for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
			{
				if( !bDoit[u8Adc][u8Core] )
					continue;
				i32Ret = WriteToAdcReg( u8Core, u8Adc, u16Addr, u8[u8Adc][u8Core]);
				if( CS_FAILED(i32Ret) )
					return i32Ret;
			}
		}

		i32Ret = MeasureAdcGain(u16HwChannel, i32G1);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		bool bStop = true;
		for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
		{
			u8Adc = AdcForChannel( cu16ChanZeroBased, u8AdcCount );
			for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
			{
				if( !bDoit[u8Adc][u8Core] )
					continue;
				i32Cor = (int32(c_u8AdcCalRegSpan/2) * (i32G1[m_u8AdcIndex[u8Adc][u8Core]] - i32Target) + i32Scale[u8Adc][u8Core]/2)/i32Scale[u8Adc][u8Core];
				TraceCalib(u32TraceId|TRACE_VERB, u8[u8Adc][u8Core], u8Adc*10 + u8Core, i32G1[m_u8AdcIndex[u8Adc][u8Core]]-i32Target, -i32Cor);
				u8[u8Adc][u8Core] = (uInt8)Max(0, Min(int32(c_u8AdcCalRegSpan), int32(u8[u8Adc][u8Core]) - i32Cor));

				// Strict in Medium Gain but flexible in Fine gain with Tolerance
				if ( bFine )
				{
					if ( m_u32MatchAdcGainTolerance < (uInt32) labs(i32Cor) ) 
						bStop = false;
					else
						bDoit[u8Adc][u8Core] = false;
				}
				else
				{
					if ( 0 != i32Cor )
						bStop = false;
					else
						bDoit[u8Adc][u8Core] = false;
				}
			}
		}
		if ( bStop )
			break;

		u8Iter++;
	}

	for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
	{
		u8Adc = AdcForChannel( cu16ChanZeroBased, u8AdcCount );
		for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
			TraceCalib( u32TraceId, u16HwChannel, u8Adc*10+u8Core, u8[u8Adc][u8Core], i32Scale[u8Adc][u8Core] );
	}

	TraceCalib(TRACE_CAL_PROGRESS, u16HwChannel, u8[AdcForChannel( cu16ChanZeroBased, 0 )][0], bFine?9:8, u8[AdcForChannel( cu16ChanZeroBased, 0 )][0] - u8[AdcForChannel( cu16ChanZeroBased, 0 )][1] );
	if( u8Iter == DEERE_MAX_CAL_ITER )
		return (CS_GAIN_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * u16HwChannel);

	return CS_SUCCESS;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32 CsDeereDevice::TraceCalibrationMatch(uInt16 u16HwChannel )
{
	const uInt16 cu16ChanZeroBased = u16HwChannel - 1;
	if( ecmCalModeDc != m_CalibMode )
	{
		::OutputDebugString( "!*!*!*!*!*!*!* Cannot display matching results the Calibration mode is wrong\n" );
		return CS_FALSE;
	}
	
	int32 i32SetLevel_uV;
	int32 i32Ret = SendCalibDC(m_pCardState->ChannelParams[cu16ChanZeroBased].u32Impedance, 0, false, &i32SetLevel_uV );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	int32 i32Target = -(int32)((LONGLONG(i32SetLevel_uV)*m_pCardState->AcqConfig.i32SampleRes)/(m_pCardState->ChannelParams[cu16ChanZeroBased].u32InputRange * 50));

	int32 ai32[DEERE_ADC_COUNT*DEERE_ADC_CORES];
	i32Ret = AcquireAndAverageCore(u16HwChannel, ai32);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	uInt8 u8AdcCount, u8Core;
	int32 i32Avg = 0;
	for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
		for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
			i32Avg += ai32[m_u8AdcIndex[AdcForChannel(cu16ChanZeroBased, u8AdcCount)][u8Core]];
	
	i32Avg /= m_pCardState->u8NumOfAdc*DEERE_ADC_CORES;

	char szTrace[128];
	sprintf_s( szTrace, sizeof(szTrace),  "   ±   Ch %d Offset matching results: Target %+8d            Average %+8d           Delta %+6d (%c%ld.%03ld pph)\n",  
		u16HwChannel, i32Target, i32Avg, 
		i32Avg-i32Target, i32Avg-i32Target>0?' ':'-', (labs(i32Target-i32Avg)*10)/labs(m_pCardState->AcqConfig.i32SampleRes), 
		((labs(i32Target-i32Avg)*10000)/labs(m_pCardState->AcqConfig.i32SampleRes))%1000  );
	
	if( 0 != (m_u32DebugCalibTrace & TRACE_VERB) )
		for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
			for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
				sprintf_s( szTrace, sizeof(szTrace), "                             Adc %d%c:        %+8d (%c%ld.%03ld pph)       %+8d (%c%ld.%03ld pph)\n", 
					AdcForChannel(cu16ChanZeroBased, u8AdcCount), 'a'+ u8Core,
					ai32[m_u8AdcIndex[AdcForChannel(cu16ChanZeroBased, u8AdcCount)][u8Core]] - i32Target, 
					ai32[m_u8AdcIndex[AdcForChannel(cu16ChanZeroBased, u8AdcCount)][u8Core]] - i32Target>0?' ':'-',
					labs(ai32[m_u8AdcIndex[AdcForChannel(cu16ChanZeroBased, u8AdcCount)][u8Core]] - i32Target)*10/labs(m_pCardState->AcqConfig.i32SampleRes),
					((labs(ai32[m_u8AdcIndex[AdcForChannel(cu16ChanZeroBased, u8AdcCount)][u8Core]] - i32Target)*10000)/labs(m_pCardState->AcqConfig.i32SampleRes))%1000,

					ai32[m_u8AdcIndex[AdcForChannel(cu16ChanZeroBased, u8AdcCount)][u8Core]] - i32Avg,
					ai32[m_u8AdcIndex[AdcForChannel(cu16ChanZeroBased, u8AdcCount)][u8Core]] - i32Avg>0?' ':'-',
					labs(ai32[m_u8AdcIndex[AdcForChannel(cu16ChanZeroBased, u8AdcCount)][u8Core]] - i32Avg)*10/labs(m_pCardState->AcqConfig.i32SampleRes),
					((labs(ai32[m_u8AdcIndex[AdcForChannel(cu16ChanZeroBased, u8AdcCount)][u8Core]] - i32Avg)*10000)/labs(m_pCardState->AcqConfig.i32SampleRes))%1000 );
				
	i32Ret = MeasureAdcGain(u16HwChannel, ai32, &i32Target);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	i32Avg = 0;
	for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
		for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
			i32Avg += ai32[m_u8AdcIndex[AdcForChannel( cu16ChanZeroBased, u8AdcCount )][u8Core]];

	i32Avg /= m_pCardState->u8NumOfAdc*DEERE_ADC_CORES;

	sprintf_s( szTrace, sizeof(szTrace), "   ±   Ch %d Gain matching results:   Target %+8d            Average %+8d           Delta %+6d (%c%ld.%03ld pph)\n",  
		u16HwChannel, i32Target, i32Avg, 
		i32Avg-i32Target, i32Avg-i32Target>0?' ':'-',(labs(i32Target-i32Avg)*10)/labs(m_pCardState->AcqConfig.i32SampleRes), 
		((labs(i32Target-i32Avg)*10000)/labs(m_pCardState->AcqConfig.i32SampleRes))%1000   );
	
	if( 0 != (m_u32DebugCalibTrace & TRACE_VERB) )
		for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
			for( u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
				sprintf_s( szTrace, sizeof(szTrace),  "                             Adc %d%c:        %+8d (%c%ld.%03ld pph)       %+8d (%c%ld.%03ld pph)\n", 
					AdcForChannel(cu16ChanZeroBased, u8AdcCount), 'a'+ u8Core,  
					ai32[m_u8AdcIndex[AdcForChannel(cu16ChanZeroBased, u8AdcCount)][u8Core]] - i32Target, 
					ai32[m_u8AdcIndex[AdcForChannel(cu16ChanZeroBased, u8AdcCount)][u8Core]] - i32Target>0?' ':'-',
					labs(ai32[m_u8AdcIndex[AdcForChannel(cu16ChanZeroBased, u8AdcCount)][u8Core]] - i32Target)*10/labs(m_pCardState->AcqConfig.i32SampleRes),
					((labs(ai32[m_u8AdcIndex[AdcForChannel(cu16ChanZeroBased, u8AdcCount)][u8Core]] - i32Target)*10000)/labs(m_pCardState->AcqConfig.i32SampleRes))%1000,

					ai32[m_u8AdcIndex[AdcForChannel(cu16ChanZeroBased, u8AdcCount)][u8Core]] - i32Avg,
					ai32[m_u8AdcIndex[AdcForChannel(cu16ChanZeroBased, u8AdcCount)][u8Core]] - i32Avg>0?' ':'-',
					labs(ai32[m_u8AdcIndex[AdcForChannel(cu16ChanZeroBased, u8AdcCount)][u8Core]] - i32Avg)*10/labs(m_pCardState->AcqConfig.i32SampleRes),
					((labs(ai32[m_u8AdcIndex[AdcForChannel(cu16ChanZeroBased, u8AdcCount)][u8Core]] - i32Avg)*10000)/labs(m_pCardState->AcqConfig.i32SampleRes))%1000 );
	
	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool CsDeereDevice::FastFeCalibrationSettings(uInt16 u16HwChannel, bool bForceSet )
{
	const uInt16	cu16ChanZeroBased = u16HwChannel - 1;
	uInt32	u32Index;
	uInt16	u16DacOffset = ( CS_CHAN_1 == u16HwChannel ) ? 0 : DEERE_CHAN2_DAC_CTRL;

	int32 i32Ret = FindFeIndex( m_pCardState->ChannelParams[cu16ChanZeroBased].u32InputRange, &u32Index );
	if( CS_FAILED(i32Ret) )	return false;

	PDEERE_CHANCALIBINFO pDacInfo = &m_pCardState->DacInfo[cu16ChanZeroBased][u32Index];
	if( !pDacInfo->bValid )	return false;

	if ( bForceSet || !m_bFastCalibSettingsDone[cu16ChanZeroBased] )
	{
		i32Ret = WriteToDac(DEERE_OFFSET_NULL_1 + u16DacOffset, pDacInfo->_OffsetNull, 0);
		if( CS_FAILED(i32Ret) )	return false;

		i32Ret = WriteToDac(DEERE_OFFSET_COMP_1 + u16DacOffset, pDacInfo->_OffsetComp, 0);
		if( CS_FAILED(i32Ret) )	return false;

		i32Ret = WriteToDac(DEERE_USER_OFFSET_1 + u16DacOffset, pDacInfo->_UserOffset, 0);
		if( CS_FAILED(i32Ret) )	return false;

		i32Ret = WriteToDac( DEERE_CLKPHASE_0 + u16DacOffset, pDacInfo->_ClkPhaseOff_LoP, 0);
		if( CS_FAILED(i32Ret) )	return false;

		i32Ret = WriteToDac( DEERE_CLKPHASE_2 + u16DacOffset, pDacInfo->_ClkPhaseOff_HiP, 0);
		if( CS_FAILED(i32Ret) )	return false;

		i32Ret = SendPosition( u16HwChannel );
		if( CS_FAILED(i32Ret) )	return false;

		for( uInt8 u8Adc = 0; u8Adc < m_pCardState->u8NumOfAdc; u8Adc++ )
		{
			if( isAdcForChannel( u8Adc, cu16ChanZeroBased ) )
			{
				i32Ret = WriteToAdcReg(0, u8Adc, DEERE_ADC_COARSE_OFFSET,m_pCardState->AdcReg[u8Adc][u32Index]._CoarseOffsetA, 0);
				if( CS_FAILED(i32Ret) )	return false;
				i32Ret = WriteToAdcReg(1, u8Adc, DEERE_ADC_COARSE_OFFSET, m_pCardState->AdcReg[u8Adc][u32Index]._CoarseOffsetB, 0);
				if( CS_FAILED(i32Ret) )	return false;

				i32Ret = WriteToAdcReg(0, u8Adc, DEERE_ADC_FINE_OFFSET, m_pCardState->AdcReg[u8Adc][u32Index]._FineOffsetA, 0);
				if( CS_FAILED(i32Ret) )	return false;
				i32Ret = WriteToAdcReg(1, u8Adc, DEERE_ADC_FINE_OFFSET, m_pCardState->AdcReg[u8Adc][u32Index]._FineOffsetB, 0);
				if( CS_FAILED(i32Ret) )	return false;

				i32Ret = WriteToAdcReg(0, u8Adc, DEERE_ADC_COARSE_GAIN, m_pCardState->AdcReg[u8Adc][u32Index]._CoarseGainA, 0);
				if( CS_FAILED(i32Ret) )	return false;
				i32Ret = WriteToAdcReg(1, u8Adc, DEERE_ADC_COARSE_GAIN, m_pCardState->AdcReg[u8Adc][u32Index]._CoarseGainB, 0);
				if( CS_FAILED(i32Ret) )	return false;

				i32Ret = WriteToAdcReg(0, u8Adc, DEERE_ADC_MEDIUM_GAIN, m_pCardState->AdcReg[u8Adc][u32Index]._MedGainA, 0);
				if( CS_FAILED(i32Ret) )	return false;
				i32Ret = WriteToAdcReg(1, u8Adc, DEERE_ADC_MEDIUM_GAIN, m_pCardState->AdcReg[u8Adc][u32Index]._MedGainB, 0);
				if( CS_FAILED(i32Ret) )	return false;

				i32Ret = WriteToAdcReg(0, u8Adc, DEERE_ADC_FINE_GAIN, m_pCardState->AdcReg[u8Adc][u32Index]._FineGainA, 0);
				if( CS_FAILED(i32Ret) )	return false;
				i32Ret = WriteToAdcReg(1, u8Adc, DEERE_ADC_FINE_GAIN, m_pCardState->AdcReg[u8Adc][u32Index]._FineGainB, 0);
				if( CS_FAILED(i32Ret) )	return false;

				i32Ret = WriteToAdcReg(0, u8Adc, DEERE_ADC_SKEW, m_pCardState->AdcReg[u8Adc][0]._Skew, 0);
				if( CS_FAILED(i32Ret) )	return false;
			}
		}
		BBtiming(m_u16AdcRegDelay);
		m_bFastCalibSettingsDone[cu16ChanZeroBased] = true;
		return true;
	}

	return false;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void CsDeereDevice::SetFastCalibration(uInt16 u16Channel)
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
		int32 i32Ret = FindFeIndex( m_pCardState->ChannelParams[u16Channel-1].u32InputRange, &u32Index );
		if( CS_FAILED(i32Ret) )	return;
		m_pCardState->DacInfo[u16Channel-1][u32Index].bValid = true;
	}
}


// For Adc Calibration.
// The Pulse for Adc calibration is not perfect. Nearby the trigger position, below 0V level is a turbulant zone. The signal goes up and down.
// In order to properly detect the level crossing position, we have to move the detection level away from turbulant zone.
// The DEFAULT_ADC_CALIB_CROSS_LEVEL is level in stable zone of the calib adc pulse. It is determined by testing experience.

#define DEFAULT_ADC_CALIB_CROSS_LEVEL 0x2000   // 1/4 of full scale above 0 Level
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int16	CsDeereDevice::CalculateLevelForPulseDetection( int16 *pi16CalibPulseBuffer, uInt32 u32FlatLevelSize )
{
	LONGLONG llSumLevel = 0;

	// Assume that pi16CalibPulseBuffer is the pulse data and u32FlatLevelSize is the size of the flat zone
	// above 0 level just before trigger position.
	// Calculate and return the average of flatness level.
	for (uInt32	i = 0; i < u32FlatLevelSize; i ++)
	{
		llSumLevel += pi16CalibPulseBuffer[i];
	}

	// The 80% of the average
	llSumLevel = llSumLevel*80;
	llSumLevel /= (100*u32FlatLevelSize);

	if ( llSumLevel <= (-1*m_pCardState->AcqConfig.i32SampleRes /4) )
	{
		// The level is too low. It is posiible that there is no pulse at all ...
		// Just use the default level.
		return (int16)(-1*m_pCardState->AcqConfig.i32SampleRes/4);
	}
	else 
		return (int16)( llSumLevel);
	/*

	if ( llSumLevel <= DEFAULT_ADC_CALIB_CROSS_LEVEL )
	{
		// The level is too low. It is posiible that there is no pulse at all ...
		// Just use the default level.
		return DEFAULT_ADC_CALIB_CROSS_LEVEL;
	}
	else 
		return (int16)( llSumLevel);
		*/
}


// Based on the shape of the calib pulse, the PreTrigger is number of samples just before trigger position whose value are all positive.
#define ADC_CALIB_PRETRIG_SAMPLES_ABOVE_0V			80		// based on 2G/s sample rate

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32	CsDeereDevice::DetectAdcCrossings( uInt16 u16Channel )
{
	uInt32	u32PreTrigger = (uInt32)(m_i64HwSegmentSize - m_i64HwPostDepth );
	if ( u32PreTrigger > ADC_CALIB_PRETRIG_SAMPLES_ABOVE_0V )
		u32PreTrigger = ADC_CALIB_PRETRIG_SAMPLES_ABOVE_0V;

	u32PreTrigger = (uInt32) (m_pCardState->AcqConfig.i64SampleRate * u32PreTrigger / 2000000000);

	in_PREPARE_DATATRANSFER		InXferEx ={0};
	out_PREPARE_DATATRANSFER	OutXferEx = {0};

	InXferEx.i64StartAddr	= -1*u32PreTrigger;
	InXferEx.u16Channel		= u16Channel;
	InXferEx.u32DataMask	= GetDataMaskModeTransfer(0);
	InXferEx.u32FifoMode	= GetFifoModeTransfer(CS_CHAN_1);
	InXferEx.bBusMasterDma	= m_BusMasterSupport;
	InXferEx.bIntrEnable	= 0;
	InXferEx.u32Segment		= 1;
	InXferEx.u32SampleSize	= sizeof(int16);

	IoctlPreDataTransfer( &InXferEx, &OutXferEx );
	CardReadMemory( m_pi16CalibBuffer, DEERE_CALIB_BUFFER_SIZE*sizeof(int16) );
	
#ifdef DEBUG_MASTERSLAVE
	RtlCopyMemory( m_PulseData, m_pi16CalibBuffer, Min(sizeof(m_PulseData), DEERE_CALIB_BUFFER_SIZE*sizeof(int16)) );
#endif

	int16 ai16CrossLevel[DEERE_ADC_COUNT] = {0};
	uInt8 u8 = 0;
	int16 *pi16Buff = &m_pi16CalibBuffer[0];
	int32 i32Ret = CS_SUCCESS;


	if ( 4 == m_pCardState->u8NumOfAdc )
	{
		// Spliting data from each ADC
		for (int i = 0, k = 0; i < DEERE_CALIB_BUFFER_SIZE; k++)
			for( u8 = 0; u8 < m_pCardState->u8NumOfAdc; u8 ++ )
				m_pi16DataAdc[u8][k] = pi16Buff[i++];
		
		// Calculate level for pulse detection. -5 because we want to remove samples near trigger position.
		int16 i16DetectLevel = CalculateLevelForPulseDetection( m_pi16DataAdc[0], u32PreTrigger/4-5 );

		for( u8 = 0; u8 < m_pCardState->u8NumOfAdc; u8++ )
		{
			if ( ! CsDetectLevelCrossPosition(m_pi16DataAdc[u8], DEERE_CALIB_BUFFER_SIZE/4, (uInt32 *)(m_ai32AdcAlignment + u8), i16DetectLevel, ai16CrossLevel + u8 ) )
			{
				i32Ret =  CS_ADCALIGN_CALIB_FAILURE;
				break;
			}
		}
	}
	else if ( 2 == m_pCardState->u8NumOfAdc )
	{	
		int i = 0, k = 0;
		uInt8 u8Indx  = 0;
		for( u8 = 0; u8 < m_pCardState->u8NumOfAdc; u8++ )
		{
			// Spliting data from each ADC

			u8Indx  = AdcForChannel( u16Channel-1, u8 );
			for (i = u8, k = 0; i < DEERE_CALIB_BUFFER_SIZE/2; k++, i+=2)
			{
				m_pi16DataAdc[u8Indx][k] = pi16Buff[i];
			}
		}

		// Calculate level for pulse detection. -5 because we want to remove samples near trigger position.
		u8Indx  = AdcForChannel( u16Channel-1, 0 );
		int16 i16DetectLevel = CalculateLevelForPulseDetection( m_pi16DataAdc[u8Indx], u32PreTrigger/2- 5 );

		for( u8 = 0; u8 < m_pCardState->u8NumOfAdc; u8++ )
		{
			u8Indx = AdcForChannel( u16Channel-1, u8 );
#if DBG
			if ( 2 == u16Channel)
			{
				tCal.Trace(TraceInfo, "Pulse shape (phase index %d).....(Detection Level =0x%x)\n", u8Indx, Temp );
				DebugTraceMemory(m_pi16DataAdc[u8Indx], 48);
			}
#endif
			if ( ! CsDetectLevelCrossPosition(m_pi16DataAdc[u8Indx], DEERE_CALIB_BUFFER_SIZE/4, (uInt32 *)(m_ai32AdcAlignment + u8Indx), i16DetectLevel, ai16CrossLevel + u8Indx ) )
			{
				i32Ret =  CS_ADCALIGN_CALIB_FAILURE;
				break;
			}
		}
	}

	if ( CS_FAILED(i32Ret) )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_ADC_ALIGN_ERROR, "Edge detection error in Adc aligment" );
		char szTrace[128];
		sprintf_s(szTrace, sizeof(szTrace), "! ! ! Edge detection error on Adc aligment %d\n", u8);
		::OutputDebugString(szTrace);

		return i32Ret;
	}

	FindAdcOrder( u16Channel, ai16CrossLevel );
	return i32Ret;
}

//-----------------------------------------------------------------------------
//	
//-----------------------------------------------------------------------------
void CsDeereDevice::FindAdcOrder( uInt16 u16Channel, int16* pArrayLevel )
{
	// pArrayLevel is a array of 4 samples.
	// Because we are triggered on negative slope, if all ADCs are properly aligned
	// the data would be
	//
	// 0                  *
	// 90                   *  
	// 180                    *  
	// 270                      *
	//     pArrayLevel    0 1 2 3
	//
	// And the result of this function call would be
	// m_au8AdcOrder[0]=3, m_au8AdcOrder[1]=2, m_au8AdcOrder[2]=1, m_au8AdcOrder[3]=0


	// Example of ADC order (fine) problem
	//
	//	0		         *
	//	90                * 
	//	180            *         
	//  270             *   


	// Example of ADC delay (coarse) problem
	//
	// 0                  *
	// 90		           *
	// 180     *				    
	// 270                   *

	if ( 4 == m_pCardState->u8NumOfAdc )
	{
		for( uInt8 u8Order = 0; u8Order < m_pCardState->u8NumOfAdc; u8Order++ )
		{
			int16	i16Min = pArrayLevel[0];
			uInt8	u8PosMin = 0;
			for ( uInt8 i = 1; i < m_pCardState->u8NumOfAdc; i++ )
				if ( pArrayLevel[i] < i16Min )
					i16Min = pArrayLevel[u8PosMin = i];

			m_au8AdcOrder[u8PosMin] = u8Order;

			// Removed this data from next search iteration.
			// Because we are looking for min value, let's put it in max value.
			pArrayLevel[u8PosMin] = 0x7fff;
		}
	}
	else if ( 2 == m_pCardState->u8NumOfAdc )
	{
		// pArrayLevel is a array of 4 samples in interleaved.
		// Chan1 Chan2 Chan1 Chan2
		// 0     90    180   270  
		// if all ADCs are properly aligned, the result of this function call would be
		// m_au8AdcOrder[0]=1, m_au8AdcOrder[1]=1, m_au8AdcOrder[2]=0, m_au8AdcOrder[3]=0

		uInt8	u8Index  = AdcForChannel( u16Channel-1, 0 );
		int16	i16Min = pArrayLevel[u8Index];

		if ( pArrayLevel[u8Index+2] < i16Min )
		{
			m_au8AdcOrder[u8Index+2] = 0;
			m_au8AdcOrder[u8Index] = 1;
		}
		else
		{
			m_au8AdcOrder[u8Index] = 0;
			m_au8AdcOrder[u8Index+2] = 1;
		}
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDeereDevice::AdjustAdcAlign()
{
	int32 i32Ret = CS_SUCCESS;
	uInt8 u8 = 0;

	// Combine Adc delay and order
	if ( 4 == m_pCardState->u8NumOfAdc )
	{
		for( u8 = 0; u8 < m_pCardState->u8NumOfAdc; u8 ++ )
			m_i16DelayAdc[u8]	= (int16) (m_ai32AdcAlignment[0] - m_ai32AdcAlignment[u8]);

		// Each byte of u32RealOrder is a phase order
		// From MSB to LSB ---> Phase 0, 90, 180 and 270
		uInt32	u32RealOrder = m_au8AdcOrder[0];
		for( u8 = 1; u8 < m_pCardState->u8NumOfAdc; u8 ++ )
		{
			u32RealOrder <<= 8;
			u32RealOrder |= m_au8AdcOrder[u8];
		}
		
		switch (u32RealOrder)
		{
			case 0x03020100: break;											// Adc order: 3 2 1 0 - do nothing
			case 0x02010003: m_i16DelayAdc[3]--; break;                     // Adc order: 2 1 0 3 - move fourth ADC data one step forward (reduce the delay) 
			case 0x01000302: m_i16DelayAdc[0]++; m_i16DelayAdc[1]++; break; // Adc order: 1 0 3 2 - move first two ADC one step back (increase the delay) 
			case 0x00030201: m_i16DelayAdc[0]++; break;                     // Adc order: 0 3 2 1 - move first ADC one step back (increase the delay) 
			default:
				{
					char	szErrorStr[128];
					sprintf_s( szErrorStr, sizeof(szErrorStr), " ! ! ! ERROR: Unexpected ADC order (u32RealOrser = %08x)\n", u32RealOrder);
					::OutputDebugString(szErrorStr);
#if !DBG
					sprintf_s( szErrorStr, sizeof(szErrorStr), "%08x", u32RealOrder );
					CsEventLog EventLog;
					EventLog.Report( CS_ADC_ORDER_ERROR, szErrorStr );
#endif			

				}

				i32Ret = CS_ADCALIGN_CALIB_FAILURE;
		}
	}
	else if ( 2 == m_pCardState->u8NumOfAdc )
	{
		m_i16DelayAdc[0] = 
		m_i16DelayAdc[1] = 0;
		m_i16DelayAdc[2] = (int16) (m_ai32AdcAlignment[0] - m_ai32AdcAlignment[2]);
		m_i16DelayAdc[3] = (int16) (m_ai32AdcAlignment[1] - m_ai32AdcAlignment[3]);


		// Channel 1
		// if ( m_au8AdcOrder[0] > m_au8AdcOrder[2] ) then nothing. Otherwise ...
		if ( m_au8AdcOrder[0] < m_au8AdcOrder[2] )
			m_i16DelayAdc[2]--;

		// Channel 2
		// if ( m_au8AdcOrder[1] > m_au8AdcOrder[3] ) then nothing. Otherwise ...
		if ( m_au8AdcOrder[1] < m_au8AdcOrder[3] )
			m_i16DelayAdc[3]--;
	}

	if( m_bSkipCalibAdcAlign )
		return CS_SUCCESS;
	else
	{
		if ( CS_SUCCEEDED(i32Ret) )
			return 	SendAdcAlign( ecmCalModeAdc );
		else
			SendAdcAlign( ecmCalModeAdc );
	}

	return i32Ret;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDeereDevice::MsCalibrateAdcAlign()
{
	int32			i32Ret = CS_SUCCESS;
	CsDeereDevice	*pDevice = m_MasterIndependent;

	// Do not need calibrate for one ADC one channel card
	if ( (1 == m_pCardState->u8NumOfAdc) && (1 == m_pCsBoard->NbAnalogChannel) )
		return CS_SUCCESS;

	// Check if we need to go through this Adc Calib process
	bool bAdcAlignCalibReq = false;
	pDevice = m_MasterIndependent;
	while ( !m_bNeverCalibAdcAlign && pDevice )
	{
		if ( pDevice->m_pCardState->bAdcAlignCalibReq || 
			 ((2 == m_pCsBoard->NbAnalogChannel) && !pDevice->m_pCardState->bDualChanAligned ) )
		{
			bAdcAlignCalibReq = true;
			break;
		}
		pDevice = pDevice->m_Next;
	}

	if( !bAdcAlignCalibReq ) return CS_SUCCESS;

	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		i32Ret = pDevice->PrepareForCalibration( true );
		if( CS_FAILED(i32Ret) ) goto SafeExit;
		pDevice = pDevice->m_Next;
	}			

	// First, calibrate ADCs alignment for each of channels
	i32Ret = MsCalibrateAdcAlignForChannels();

	// Second, calibrate 2 channels alignment if it is a dual channel card
	if ( CS_SUCCEEDED(i32Ret) &&  2 == m_pCsBoard->NbAnalogChannel )
		i32Ret = MsCalibrateAdcAlign2ChannelsDual();

SafeExit:
	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		i32Ret = pDevice->PrepareForCalibration( false );
		if( CS_FAILED(i32Ret) ) break;
		pDevice = pDevice->m_Next;
	}				

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDeereDevice::MsCalibrateAdcAlignForChannels()
{
	// This function should be called only on the master card
	if( this != m_MasterIndependent ) 
		return CS_SUCCESS;

	// Do not need calibrate for one ADC
	if ( 1 == m_pCardState->u8NumOfAdc ) 
		return CS_SUCCESS;
	
	int32 i32Ret = CS_SUCCESS;
	CsDeereDevice* pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		i32Ret = pDevice->SetupForCalibration(true, ecmCalModeAdc);
		if( CS_FAILED(i32Ret) ) break;
		pDevice = pDevice->m_Next;
	}
	
	int32 i32Res = CS_SUCCESS;
	if ( CS_SUCCEEDED(i32Ret) )
	{
		pDevice = m_MasterIndependent;
		while ( pDevice )
		{
			if ( pDevice->m_pCardState->bAdcAlignCalibReq )
			{
				RtlZeroMemory( pDevice->m_au8AdcOrder, sizeof(m_au8AdcOrder) );
				RtlZeroMemory( pDevice->m_ai32AdcAlignment, sizeof(m_ai32AdcAlignment) );

				// Set all ADC alignment to neutral position
				RtlZeroMemory( pDevice->m_i16DelayAdc, sizeof( m_i16DelayAdc )); 
				i32Ret = pDevice->WriteRegisterThruNios( 0, DEERE_ADC_ALIGN_WR_CMD );
				if( CS_SUCCEEDED(i32Ret) )
				{
					for (uInt16 u16Chan = 0; u16Chan < m_pCsBoard->NbAnalogChannel; u16Chan ++ )
					{
						// Calibrate Adc alignment for Channel
						i32Res = pDevice->CalibrateAdcAlignSingleChan(CS_CHAN_1 + u16Chan);
						if ( CS_FAILED(i32Res) ) break;
					}

					pDevice->m_pCardState->bAdcAlignCalibReq = CS_FAILED(i32Res);
				}
			}

			pDevice = pDevice->m_Next;
		}
	}
	
	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		pDevice->SetupForCalibration(false, ecmCalModeAdc);
		pDevice = pDevice->m_Next;
	}

	return i32Res;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDeereDevice::CalibrateAdcAlignSingleChan( uInt16 u16Channel )
{
	int32 i32Res  = CS_SUCCESS;
	int16	i16AdcSelect = (int16)(u16Channel-1);
	CSTIMER		CsTimeout;

	ASSERT( (m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE) != 0 );	
	if (( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE)  == 0 )
		return CS_INVALID_ACQ_MODE;

	int32 i32Ret = SendChannelSetting( u16Channel, CS_GAIN_1_V, CS_COUPLING_DC, CS_REAL_IMP_50_OHM );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = InCalibrationCalibrate( u16Channel );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	i32Ret = SendCaptureModeSetting(m_pCardState->AcqConfig.u32Mode, u16Channel);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	// Switch to Adc calid mode
	i32Ret = SendCalibModeSetting( u16Channel, ecmCalModeAdc );
	if( CS_FAILED(i32Ret) )	goto SafeExit;

	// Enable trigger only from this card and only from one of ADCs dedicated for this channel
	i32Ret = SendTriggerEngineSetting( u16Channel, CS_TRIG_COND_NEG_SLOPE, EXTTRIGCALIB_LEVEL_CHAN, 0, 0, 0, 0, 0, 0, 0, 0, 0, i16AdcSelect );
	if( CS_FAILED(i32Ret) )	goto SafeExit;

	AcquireData();
	CsTimeout.Set( DEERE_CALIB_TIMEOUT_SHORT );

	while( STILL_BUSY == ReadBusyStatus() )
	{
		if ( CsTimeout.State() )
		{
			if( STILL_BUSY == ReadBusyStatus() )
			{
				MsAbort();
				i32Res =  CS_CAL_BUSY_TIMEOUT;
				break;
			}
		}
	}

	char szError[128];
	if ( CS_FAILED(i32Res) )
	{
		sprintf_s(szError, sizeof(szError), "ADC ALIGN FAILED (No trigger)!  Board %d\n", m_pCardState->u8MsCardId);
		::OutputDebugString(szError);
#if !DBG
		CsEventLog EventLog;
		EventLog.Report( CS_ADC_ALIGN_ERROR, "No trigger" );
#endif
		goto SafeExit;
	}
	else
	{
		i32Res = DetectAdcCrossings(u16Channel);
		if ( CS_FAILED( i32Res ) )
		{
			sprintf_s(szError, sizeof(szError), "ADC ALIGN FAILED (No Pulse detected)!  Board %d\n", m_pCardState->u8MsCardId);
			::OutputDebugString(szError);
#if !DBG
			CsEventLog EventLog;
			EventLog.Report( CS_ADC_ALIGN_ERROR, "No Pulse detected" );
#endif
			goto SafeExit;
		}
		else
		{
			i32Res = AdjustAdcAlign();
			if( CS_FAILED(i32Ret) )
				{ i32Ret = CS_ADCALIGN_CALIB_FAILURE; }
		}
	}

SafeExit:
	// Disable trigger from this card
	i32Ret = SendTriggerEngineSetting( 0 );
	if( CS_FAILED(i32Ret) )
		i32Res = i32Ret;
	
	// Switch back to normal mode
	i32Ret = SendCalibModeSetting( u16Channel );
	if( CS_FAILED(i32Ret) )
		i32Res = i32Ret;

	return i32Res;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsDeereDevice::FindCalLutItem( const uInt32 u32Swing, const uInt32 u32Imped, DEERE_CAL_LUT& Item )
{
	const DEERE_CAL_LUT*  pLut;
	int nCount(0);
	if (CS_REAL_IMP_50_OHM == u32Imped)
	{
		pLut = s_CalCfg50;
		nCount = c_JD_50_CAL_LUT_SIZE;
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
//------------------------------------------------------------------------------
int32 CsDeereDevice::SendCalibDC(const uInt32 u32Imped, const uInt32 u32InputRange, bool bPolarity, int32* pi32SetLevel_uV)
{
	DEERE_CAL_LUT CalInfo;
	uInt32 u32CalRange = u32InputRange;
	
	int32 i32Ret = FindCalLutItem(u32CalRange, u32Imped, CalInfo);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	uInt16 u16Old = m_pCardState->AddOnReg.u16CalDc;
	//Connect ground to discharge feedback capacitor
	i32Ret = WriteRegisterThruNios(u16Old|3, DEERE_SET_CALDC);
	BBtiming(m_u32CalSrcRelayDelay);
	m_pCardState->AddOnReg.u16CalDc = bPolarity ? CalInfo.u16PosRegMask : CalInfo.u16NegRegMask;

	i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16CalDc, DEERE_SET_CALDC);
	BBtiming(m_u32CalSrcRelayDelay);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	TraceCalib( TRACE_CAL_SIGNAL, u32Imped, u32InputRange, bPolarity ? CalInfo.i32PosLevel_uV : CalInfo.i32NegLevel_uV, m_pCardState->AddOnReg.u16CalDc );
	
	if( NULL != pi32SetLevel_uV )
	{
		int32 i32SetLevel_uV = bPolarity ? CalInfo.i32PosLevel_uV : CalInfo.i32NegLevel_uV;
		i32SetLevel_uV *= CS_REAL_IMP_50_OHM;
		i32SetLevel_uV /= int32(CS_REAL_IMP_50_OHM + u32Imped);
		*pi32SetLevel_uV = i32SetLevel_uV;
	}
	return CS_SUCCESS;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32 CsDeereDevice::ReadCalibA2D(int32& i32Expected_uV, eCalInputJD eInput, int32* pi32V_uV/*=NULL*/, int32* pi32Noise_01LSB/* = NULL*/)
{
	bool bFreeBuffer = false;
	if(NULL == m_pi32CalibA2DBuffer )
	{
		bFreeBuffer = true;
		m_pi32CalibA2DBuffer = (int32 *)::GageAllocateMemoryX( DEERE_CALIB_BLOCK_SIZE*sizeof(int32) );
	}
	if( eciDirectRef == eInput )
	{
		switch(m_pCardState->AddOnReg.u16CalDc & DEERE_CALDC_REF_MASK )
		{
			default:
			case 0x0: i32Expected_uV = DEERE_CALREF0 * 1000; break;
			case 0x1: i32Expected_uV = DEERE_CALREF1 * 1000; break;
			case 0x2: i32Expected_uV = DEERE_CALREF2 * 1000; break;
			case 0x3: i32Expected_uV = DEERE_CALREF3 * 1000; break;
		}
	}
	else if ( eciCalGrd == eInput )
		i32Expected_uV = 0;

//Set Range	 and input
	uInt32 u32DataCtl = DEERE_ADC7321_CTRL | DEERE_ADC7321_CHAN;
	uInt32 u32DataRange = 0, u32Range_mV = 0;;
	int32 i32Ret;
	if( labs(i32Expected_uV) < DEERE_CALA2D_2p5_V_RANGE * 1000 )
	{
		u32Range_mV  = DEERE_CALA2D_2p5_V_RANGE;
		u32DataRange = DEERE_ADC7321_CH1_2p5_V;
	}
	else if( labs(i32Expected_uV) < DEERE_CALA2D_5_V_RANGE * 1000)
	{
		u32Range_mV = DEERE_CALA2D_5_V_RANGE;
		u32DataRange = DEERE_ADC7321_CH1_5_V;
	}
	else 
	{
		u32Range_mV = DEERE_CALA2D_10_V_RANGE;
		u32DataRange = DEERE_ADC7321_CH1_10_V;
	}
	switch( eInput )
	{
		case eciDirectRef: u32DataRange <<= 4; //Channel 0 range is the same decoding but 4 bits higher
		                   u32DataCtl &= ~DEERE_ADC7321_CHAN;
						   break;
		case eciCalChan:   m_pCardState->AddOnReg.u16CalDc &= ~DEERE_CALDC_SRC_GRD; break;
		case eciCalGrd:    m_pCardState->AddOnReg.u16CalDc |= DEERE_CALDC_SRC_GRD;break;
	}
	i32Ret = WriteRegisterThruNios(u32DataCtl, DEERE_CALADC_CTL);
	if( CS_FAILED(i32Ret) )	
	{
		if( bFreeBuffer )
			::GageFreeMemoryX(m_pi32CalibA2DBuffer);
		return i32Ret;
	}
	u32DataRange |= DEERE_ADC7321_RANGE;
	i32Ret = WriteRegisterThruNios(u32DataRange, DEERE_CALADC_RNG);

	i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16CalDc, DEERE_SET_CALDC);
	BBtiming(m_u32CalSrcRelayDelay);
	if( CS_FAILED(i32Ret) )	
	{
		if( bFreeBuffer )
			::GageFreeMemoryX(m_pi32CalibA2DBuffer);
		return i32Ret;
	}	
	 
	LONGLONG llSum = 0;
	int i = 0;
	for( i = 0; i < DEERE_CALIB_BLOCK_SIZE; i++ )
	{
		uInt32 u32Code = ReadRegisterFromNios(0, DEERE_CALADC_DATA);
		int16 i16Code = int16(u32Code & 0x1fff);
		i16Code <<= 3; i16Code >>= 3; //sign extention
		TraceCalib( TRACE_CAL_SIG_MEAS|TRACE_VERB, i, 0, i16Code);
		m_pi32CalibA2DBuffer[i] = i16Code;
		llSum += LONGLONG(i16Code);
	}
	//Switch back  source relay
	if( 0 == (m_pCardState->AddOnReg.u16CalDc & DEERE_CALDC_SRC_GRD) )
	{
		m_pCardState->AddOnReg.u16CalDc |= DEERE_CALDC_SRC_GRD;
		i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16CalDc, DEERE_SET_CALDC);
		if( CS_FAILED(i32Ret) )	
		{
			if( bFreeBuffer )
				::GageFreeMemoryX(m_pi32CalibA2DBuffer);
			return i32Ret;
		}
	}

	LONGLONG llAvg_x10 = (llSum*10)/DEERE_CALIB_BLOCK_SIZE;
	llSum *= LONGLONG(1000*u32Range_mV);
	llSum += llSum > 0 ? LONGLONG(DEERE_CALIB_BLOCK_SIZE*DEERE_AD7321_CODE_SPAN)/2 : -LONGLONG(DEERE_CALIB_BLOCK_SIZE*DEERE_AD7321_CODE_SPAN)/2;
	llSum /= LONGLONG(DEERE_CALIB_BLOCK_SIZE*DEERE_AD7321_CODE_SPAN);
	if( NULL != pi32V_uV)
	{
		*pi32V_uV = int32(llSum);

#if !DBG
		if( NULL != pi32Noise_01LSB )
#endif
		{
			LONGLONG llSum2 = 0;
			int32 i32Final = m_pi32CalibA2DBuffer[DEERE_CALIB_BLOCK_SIZE-1];
			uInt16 u16SettleCount = 0xffff;
			for( i = 0; i < DEERE_CALIB_BLOCK_SIZE; i++ )
			{
				int32 i32Err = m_pi32CalibA2DBuffer[i] - i32Final;

				if( 0xffff == u16SettleCount && (labs(i32Err) < 2) )
					u16SettleCount = uInt16(i);
#ifdef _WINDOWS_
				llSum2 += _abs64(llAvg_x10 - m_pi32CalibA2DBuffer[i]*10);
#else
				/* abs is a macro capable of auto-detecting the argument type and choosing right expression */
				llSum2 += abs(llAvg_x10 - m_pi32CalibA2DBuffer[i]*10);
#endif
			}

			llSum2 /= DEERE_CALIB_BLOCK_SIZE;
			if( NULL != pi32Noise_01LSB )
				*pi32Noise_01LSB = int32(llSum2);
			TraceCalib( TRACE_CAL_SIG_MEAS, u16SettleCount, uInt32(eInput), int32(llSum), int32(llSum2));
		}
	}
	if( bFreeBuffer )
	{
		::GageFreeMemoryX(m_pi32CalibA2DBuffer);
		m_pi32CalibA2DBuffer = NULL;
	}
	return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32 CsDeereDevice::CheckCalibrator()
{
	if( !m_bVerifyDcCalSrc ) return CS_SUCCESS;
	
	int32 i32Ret = CS_SUCCESS, i32RetErr = CS_SUCCESS;
	char szErrorStr[128];
	char szTrace[128];

	i32Ret = SendCalibModeSetting(CS_CHAN_1, ecmCalModeExtTrigChan ); //connect channel to ground via pulse path to dissipate any remaining charge
	if( CS_FAILED(i32Ret) )	
	{
		sprintf_s( szErrorStr, sizeof(szErrorStr),  "SendCalibModeSetting ExtTrig return %d.",  i32Ret);
		sprintf_s( szTrace, sizeof(szTrace),  " ! ! ! CheckCalibrator failure: %s\n", szErrorStr );
#if !DBG
		CsEventLog EventLog;
		EventLog.Report( CS_SENDCALIBDC_FAILED, szErrorStr );
#endif
	}
	BBtiming(100);
	i32Ret = SendCalibModeSetting(CS_CHAN_1, ecmCalModeDc ); //connect dc cal input
	if( CS_FAILED(i32Ret) )	
	{
		sprintf_s( szErrorStr, sizeof(szErrorStr),  "SendCalibModeSetting ModeDc return %d.",  i32Ret);
		sprintf_s( szTrace, sizeof(szTrace),  " ! ! ! CheckCalibrator failure: %s\n", szErrorStr );
#if !DBG
		CsEventLog EventLog;
		EventLog.Report( CS_SENDCALIBDC_FAILED, szErrorStr );
#endif
	}
	
	//Check 50 Ohm
	uInt32 u32Imped = CS_REAL_IMP_50_OHM;
	for( int i = 0; i < c_JD_50_CAL_LUT_SIZE; i++ )
	{
		for( int u = 0; u < 2; u++ )
		{
			int32 i32SetLevel_uV;
			i32Ret = SendCalibDC(u32Imped, s_CalCfg50[i].u32FeSwing_mV, 0 == u, &i32SetLevel_uV);
			if( CS_FAILED(i32Ret) )	
			{
				i32RetErr = i32Ret;
				sprintf_s( szErrorStr, sizeof(szErrorStr),  "%c%d.", 0 == u?'+':'-', s_CalCfg50[i].u32FeSwing_mV );
				sprintf_s( szTrace, sizeof(szTrace),  " ! ! ! DC cal source failure: SendCalibDC %s\n", szErrorStr );
#if !DBG
				CsEventLog EventLog;
				EventLog.Report( CS_SENDCALIBDC_FAILED, szErrorStr );
#endif
				continue;
			}
			int32 i32SetLevelCache = i32SetLevel_uV;
			for( int eI = int(eciDirectRef); eI <= int(eciCalGrd); eI++ )
			{
				int32 i32V_uV;
				i32SetLevel_uV = i32SetLevelCache;
				if( eciCalChan == eI ){ i32SetLevel_uV *= int32(CS_REAL_IMP_50_OHM + u32Imped); i32SetLevel_uV /= CS_REAL_IMP_50_OHM;}
		
				i32Ret = ReadCalibA2D(i32SetLevel_uV, eCalInputJD(eI), &i32V_uV );
				if( CS_FAILED(i32Ret) )	
				{
					i32RetErr = i32Ret; 
					sprintf_s( szErrorStr, sizeof(szErrorStr),  "%d @ %s.", i32SetLevel_uV, eciDirectRef==eCalInputJD(eI)?"Ref.":(eciCalChan==eCalInputJD(eI)?"Ch. ":"Grd.") );
					sprintf_s( szTrace, sizeof(szTrace),  " ! ! ! DC cal source failure: ReadCalibA2D %s\n", szErrorStr );
#if !DBG
					CsEventLog EventLog;
					EventLog.Report( CS_READCALIBA2D_FAILED, szErrorStr );
#endif
					continue;
				}
				int32 i32ErrorMax = DEERE_AD7321_FIXED_ERR + (labs(i32SetLevel_uV)*100)/DEERE_AD7321_GAIN_ERR;
				int32 i32Error = labs(i32V_uV-i32SetLevel_uV);
				if( i32Error > i32ErrorMax )
				{
					i32RetErr = CS_DAC_CALIB_FAILURE;
					sprintf_s( szErrorStr, sizeof(szErrorStr),  "Expected %d uV measured %d uV @%s\n", i32SetLevel_uV, i32V_uV, 
						 eciDirectRef==eI?"Ref.":(eciCalChan==eI?"Ch. ":"Grd.") ); 

					sprintf_s( szTrace, sizeof(szTrace),  " ! ! ! DC cal source failure! %s\n", szErrorStr);
#if !DBG
					CsEventLog EventLog;
					EventLog.Report( CS_CALSRC_FAILED, szErrorStr );
#endif
				}
			}
		}
	}
	//Check 1M Ohm
	// not ready yet
	SendCalibModeSetting(CS_CHAN_1 );
	SendCalibDC(CS_REAL_IMP_50_OHM, 0, true);
	if( CS_SUCCEEDED(i32RetErr) )
		m_bVerifyDcCalSrc = false;
	
	return i32RetErr;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CsDeereDevice::TraceCalib(uInt32 u32Id, uInt32 u32V1, uInt32 u32V2, int32 i32V3 , int32 i32V4)
{
	if( 0 == m_u32DebugCalibTrace )
		return;
	if( 0 != (m_u32DebugCalibTrace&TRACE_USAGE) )
	{
		OutputCalibTraceUsage();
		m_u32DebugCalibTrace &= ~TRACE_USAGE;
	}

	char  szTrace[250] = "";
	char  szT[250] = "";

	bool bVerboseReq = 0 != (m_u32DebugCalibTrace & TRACE_VERB);
	bool bVerboseRep = 0 != (u32Id & TRACE_VERB);
	u32Id &= (m_u32DebugCalibTrace & ~TRACE_VERB);
	
	switch( u32Id )
	{
		case TRACE_NULL_OFFSET:
			if( bVerboseRep )
			{
				if( bVerboseReq )
					sprintf_s( szTrace, sizeof(szTrace), "   ~~~ Code %4d     Delta %+8d  Cor %+5d Scale %d.\n", u32V1, i32V3, i32V4, u32V2 );
			}
			else
				sprintf_s( szTrace, sizeof(szTrace), ":: Initial Amp offset %d �V Final %d �V Code %4d (Iter %d)\n",i32V3, i32V4, u32V1, u32V2);
			break;
		
		case TRACE_CAL_FE_OFFSET:
			if( bVerboseRep )
			{
				if( bVerboseReq )
					sprintf_s( szTrace, sizeof(szTrace), "   --- Code %4d     Delta %+8d  Cor %+5d Scale %d\n", u32V1, i32V3, i32V4, u32V2 );
			}
			else
				sprintf_s( szTrace, sizeof(szTrace), "== Initial Fe offset %c%ld.%ld LSB Final %c%ld.%ld LSB  Code %4d (Iter %d)\n",
				i32V3<0?'-':' ',labs(i32V3)/10,labs(i32V3)%10, i32V4<0?'-':' ',labs(i32V4)/10, labs(i32V4)%10, u32V1, u32V2);
			break;

		case TRACE_CAL_GAIN:
			{
				static uInt16 s_u16ZeroBasedChannel(0);
				if( bVerboseRep )
				{
					if( bVerboseReq )
						sprintf_s( szTrace, sizeof(szTrace), "   *** Coarse Code %d   Adc %d%c Delta %+8d Target %+8d  Real %+8d \n",
						u32V1, u32V2/10, 'a'+ u32V2%10, i32V4-i32V3, i32V3, i32V4); 
				}
				else
				{
					sprintf_s( szTrace, sizeof(szTrace), "[] Best Coarse Gain Adc %d%c  %d Delta %+6d\n", u32V2, 'a'+i32V3, u32V1, i32V4 ); break; //coarse
				}
				break;
			}

		case TRACE_CAL_POS:
			if( bVerboseRep )
			{
				if( bVerboseReq )
					sprintf_s( szTrace, sizeof(szTrace), "  %s Code %+5d (%+5d)    Delta %+8d  Cor %+5d Scale %d\n", 
					u32V2/3==0?"+-+ Pos":"-+- Neg", u32V1,
					u32V2/3==0 ? int32(u32V1) - c_u16CalDacCount/2 : c_u16CalDacCount/2 - int32(u32V1),
					i32V3, i32V4, u32V2 );
			}
			else
				sprintf_s( szTrace, sizeof(szTrace), "|| Position scale %+4d (Pos code %4d (%4d) Offset %+8d  Neg code %4d (%4d)   Offset %+8d)\n", 
				   u32V1-u32V2, u32V1, u32V1 - c_u16CalDacCount/2, i32V3, u32V2, c_u16CalDacCount/2 - u32V2, i32V4);
			break;
		case TRACE_ADC_CORE_ORDER:
			{
				char str[DEERE_ADC_CORES*DEERE_ADC_COUNT*3+2];
				GageSetMemoryX( str, ' ', sizeof(str));
				GageZeroMemoryX( str + sizeof(str) - 2, 2 );
				for( uInt8 u8AdcCount = 0; u8AdcCount < m_pCardState->u8NumOfAdc; u8AdcCount++ )
				{
					uInt8 u8Adc = AdcForChannel( uInt16(u32V1)-1, u8AdcCount );
					for( uInt8 u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
					{
						str[m_u8AdcIndex[u8Adc][u8Core]*3] = '0' + u8Adc; 
						str[m_u8AdcIndex[u8Adc][u8Core]*3+1] = 'a' + u8Core; 
					}
				}
				sprintf_s( szTrace, sizeof(szTrace),  "** 0 1 2 3   Ch %d Adc core order %s\n", u32V1, str ); 
			}
			break;
		case TRACE_MATCH_OFF_MED:
			if( bVerboseRep )
			{
				if( bVerboseReq )
				{
					bool bMatched = (0 == i32V4);
					sprintf_s( szTrace, sizeof(szTrace), "  ### Med offset Adc %d%c =  %4d   Delta %+6d Cor %+6d %s%c%c\n", 
					u32V2/10, u32V2%10 + 'a', u32V1, i32V3, i32V4, bMatched? "---": "", bMatched? '0' + u32V2/10 : ' ', bMatched? 'a'+u32V2%10 : ' ');
				}
			}
			else
				sprintf_s( szTrace, sizeof(szTrace), " = = Match med  offset ch %d  Adc %d%c   value: %+6d (scale %+6d)\n", 
								u32V1, u32V2/10, u32V2%10 + 'a', i32V3, i32V4); 
			break;
		case TRACE_MATCH_OFF_FINE:
			if( bVerboseRep )
			{
				if( bVerboseReq )
				{
					bool bMatched = (m_u32MatchAdcOffsetTolerance >= (uInt32) labs(i32V4));
					sprintf_s( szTrace, sizeof(szTrace), "  \"\"\" Fine offset Adc %d%c = %4d   Delta %+6d Cor %+6d %s%c%c\n", 
					u32V2/10, u32V2%10 + 'a', u32V1, i32V3, i32V4, bMatched? "---": "", bMatched? '0' + u32V2/10 : ' ', bMatched? 'a'+u32V2%10 : ' ');
				}
			}
			else
				sprintf_s( szTrace, sizeof(szTrace), " -=- Match fine offset ch %d  Adc %d%c   value: %+6d (scale %+6d)\n", 
								u32V1, u32V2/10, u32V2%10 + 'a', i32V3, i32V4); 
	
			break;
		case TRACE_MATCH_GAIN_MED:
			if( bVerboseRep )
			{
				if( bVerboseReq )
				{
					bool bMatched = (0 == i32V4);
					sprintf_s( szTrace, sizeof(szTrace), "  ::: Med gain  Adc %d%c = %4d   Delta %+6d Cor %+6d %s%c%c\n", 
					u32V2/10, u32V2%10 + 'a', u32V1, i32V3, i32V4, bMatched? "---": "", bMatched? '0' + u32V2/10 : ' ', bMatched? 'a'+u32V2%10 : ' ');
				}
			}
			else
				sprintf_s( szTrace, sizeof(szTrace), " ^:^ Match med  gain ch %d  Adc %d%c   value: %+6d (scale %+6d)\n", 
								u32V1, u32V2/10, u32V2%10 + 'a', i32V3, i32V4); 
			break;
		case TRACE_MATCH_GAIN_FINE:
			if( bVerboseRep )
			{
				if( bVerboseReq )
				{
					bool bMatched = (m_u32MatchAdcGainTolerance >= (uInt32) labs(i32V4));
					sprintf_s( szTrace, sizeof(szTrace), "  > < Fine gain  Adc %d%c = %4d   Delta %+6d Cor %+6d %s%c%c\n", 
					u32V2/10, u32V2%10 + 'a', u32V1, i32V3, i32V4, bMatched? "---": "", bMatched? '0' + u32V2/10 : ' ', bMatched? 'a'+u32V2%10 : ' ');
				}
			}
			else
				sprintf_s( szTrace, sizeof(szTrace), " ~:~ Match fine gain ch %d  Adc %d%c   value: %+6d (scale %+6d)\n", 
								u32V1, u32V2/10, u32V2%10 + 'a', i32V3, i32V4);
			break;
		case TRACE_CAL_RESULT:
		{
			uInt32 u32Index;
			FindFeIndex( m_pCardState->ChannelParams[u32V1-1].u32InputRange, &u32Index );
			switch( u32V2 )
			{
				case 0:  strcpy_s( szT, sizeof(szT), "."  ); break;
				case 1:  strcpy_s( szT, sizeof(szT), " at adc selfcal."  ); break;
				case 4:	 strcpy_s( szT, sizeof(szT), " at DAC initialization."  ); break;
				case 5:  strcpy_s( szT, sizeof(szT), " at Adc initialization."  ); break;
				case 10: strcpy_s( szT, sizeof(szT), " at offset calibration."  ); break;
				case 20: strcpy_s( szT, sizeof(szT), " at gain calibration."  ); break;
				case 30: strcpy_s( szT, sizeof(szT), " at position calibration."  ); break;
				case 40: strcpy_s( szT, sizeof(szT), " at adc matching."  ); break;
				case 50: strcpy_s( szT, sizeof(szT), " at adc alignment."  ); break;
				default: strcpy_s( szT, sizeof(szT), " at somewhere."  ); break;
			}
			sprintf_s( szTrace, sizeof(szTrace),  "\n       � � Calibration of channel %d (IR %5d) has %s%s\n", 
				u32V1, m_pCardState->ChannelParams[u32V1-1].u32InputRange, CS_FAILED(i32V3)?"failed" : "succeeded", szT );
			sprintf_s( szTrace, sizeof(szTrace),  "\t\t  Null   %3d        Comp %3d       Pos centre %3d scale %d\n", 
				m_pCardState->DacInfo[u32V1-1][u32Index]._OffsetNull, m_pCardState->DacInfo[u32V1-1][u32Index]._OffsetComp, 
				m_pCardState->DacInfo[u32V1-1][u32Index]._UserOffsetCentre, m_pCardState->DacInfo[u32V1-1][u32Index]._UserOffsetScale);
			for( uInt8 u = 0; u < m_pCardState->u8NumOfAdc; u++ )
			{
				uInt16 u16Adc = AdcForChannel( uInt16(u32V1-1), u );
				sprintf_s( szTrace, sizeof(szTrace),  "\t\tAdc %d Core1  Gc %3d (%+4d) Gm %3d (%+4d) Gf %3d (%+4d)\n", u16Adc,
					m_pCardState->AdcReg[u16Adc][u32Index]._CoarseGainA, int16(m_pCardState->AdcReg[u16Adc][u32Index]._CoarseGainA) - c_u8AdcCoarseGainSpan/2, 
					m_pCardState->AdcReg[u16Adc][u32Index]._MedGainA, int16(m_pCardState->AdcReg[u16Adc][u32Index]._MedGainA) - (c_u8AdcCalRegSpan+1)/2, 
					m_pCardState->AdcReg[u16Adc][u32Index]._FineGainA, int16(m_pCardState->AdcReg[u16Adc][u32Index]._FineGainA) - (c_u8AdcCalRegSpan+1)/2 );
				sprintf_s( szTrace, sizeof(szTrace),  "\t\t             Oc %3d (%+4d) Of %3d (%+4d)\n", 
					m_pCardState->AdcReg[u16Adc][u32Index]._CoarseOffsetA, int16(m_pCardState->AdcReg[u16Adc][u32Index]._CoarseOffsetA) - (c_u8AdcCalRegSpan+1)/2, 
					m_pCardState->AdcReg[u16Adc][u32Index]._FineOffsetA, int16(m_pCardState->AdcReg[u16Adc][u32Index]._FineOffsetA) - (c_u8AdcCalRegSpan+1)/2 );
				sprintf_s( szTrace, sizeof(szTrace),  "\t\t      Core2  Gc %3d (%+4d) Gm %3d (%+4d) Gf %3d (%+4d)\n",
					m_pCardState->AdcReg[u16Adc][u32Index]._CoarseGainB, int16(m_pCardState->AdcReg[u16Adc][u32Index]._CoarseGainB) - c_u8AdcCoarseGainSpan/2, 
					m_pCardState->AdcReg[u16Adc][u32Index]._MedGainB, int16(m_pCardState->AdcReg[u16Adc][u32Index]._MedGainB) - (c_u8AdcCalRegSpan+1)/2, 
					m_pCardState->AdcReg[u16Adc][u32Index]._FineGainB, int16(m_pCardState->AdcReg[u16Adc][u32Index]._FineGainB) - (c_u8AdcCalRegSpan+1)/2  );
				sprintf_s( szTrace, sizeof(szTrace),  "\t\t             Oc %3d (%+4d) Of %3d (%+4d)\n",
					m_pCardState->AdcReg[u16Adc][u32Index]._CoarseOffsetB, int16(m_pCardState->AdcReg[u16Adc][u32Index]._CoarseOffsetB) - (c_u8AdcCalRegSpan+1)/2, 
					m_pCardState->AdcReg[u16Adc][u32Index]._FineOffsetB, int16(m_pCardState->AdcReg[u16Adc][u32Index]._FineOffsetB) - (c_u8AdcCalRegSpan+1)/2 );
				sprintf_s( szTrace, sizeof(szTrace),  "\t\t      Skew %3d (%+4d)\n\n",
					m_pCardState->AdcReg[u16Adc][0]._Skew, int16(m_pCardState->AdcReg[u16Adc][0]._Skew) - (c_u8AdcCalRegSpan+1)/2);
			}
		}
		break;
		case TRACE_CAL_PROGRESS:
		{
			switch( labs(i32V3) )
			{
				case   0: if( 0 == i32V4 ) strcpy_s( szT, sizeof(szT), "Start"  ); else strcpy_s( szT, sizeof(szT), "Finish"  ); break;
				case   1: strcpy_s( szT, sizeof(szT), "        Null offset "  ); break;
				case  12: strcpy_s( szT, sizeof(szT), "        Offset compensation coarse"  ); break;
				case   2: strcpy_s( szT, sizeof(szT), "        Offset compensation"  ); break;
				case   3: strcpy_s( szT, sizeof(szT), "        Coarse gain"  ); break;
				case   4: strcpy_s( szT, sizeof(szT), "        Medium gain"  ); break;
				case   5: strcpy_s( szT, sizeof(szT), "        Position"  ); break;
				case   6: strcpy_s( szT, sizeof(szT), "        Medium offset match"  ); break;
				case   7: strcpy_s( szT, sizeof(szT), "        Fine offset match"  ); break;
				case   8: strcpy_s( szT, sizeof(szT), "        Medium gain match"  ); break;
				case   9: strcpy_s( szT, sizeof(szT), "        Fine gain match"  ); break;
				case  10: strcpy_s( szT, sizeof(szT), "        Adc core order determination"  ); break;
				case  11: strcpy_s( szT, sizeof(szT), "        Adc aligment"  ); break;
				case  50: strcpy_s( szT, sizeof(szT), "        Adc DPA "  ); break;
				case 100: strcpy_s( szT, sizeof(szT), "   Fast settings"  ); break;
				case 200: sprintf_s( szT, sizeof(szT), "   Matching in user space %s", m_bSkipUserCal? "skipped.":"." ); break;
				case 300: strcpy_s( szT, sizeof(szT), "   Phase matching in user space"); break;
			}
			if( i32V3 > 0 )
				sprintf_s( szTrace, sizeof(szTrace),  " %s  calibration of %d channel done. Code %d (delta %d)\n", szT, u32V1, u32V2, i32V4);
			else if( i32V3 < 0 )
				sprintf_s( szTrace, sizeof(szTrace),  " %s  calibration of %d channel failed.\n", szT, u32V1);
			else
				sprintf_s( szTrace, sizeof(szTrace),  " %s  calibration of %d channel IR %d\n", szT, u32V1, m_pCardState->ChannelParams[u32V1-1].u32InputRange);
			break;
		}
		case TRACE_CAL_INPUT:
			{
				char strMode[10];
				switch(i32V3)
				{
				case ecmCalModeNormal:  strcpy_s(strMode, sizeof(strMode), "Normal"); break;
				case ecmCalModeDc:      strcpy_s(strMode, sizeof(strMode), "DC cal"); break;  
				case ecmCalModePci:    
				case ecmCalModeAc:      strcpy_s(strMode, sizeof(strMode), "AC cal"); break; 
				case ecmCalModeMs:      strcpy_s(strMode, sizeof(strMode), "MS cal"); break; 
				case ecmCalModeExtTrigChan:
				case ecmCalModeExtTrig: strcpy_s(strMode, sizeof(strMode), "ExtTrg"); break; 
				case ecmCalModeAdc:     strcpy_s(strMode, sizeof(strMode), "AdcCal"); break; 
				case ecmCalModeTAO:     strcpy_s(strMode, sizeof(strMode), "TAOCal"); break; 
				
				}
				sprintf_s( szTrace, sizeof(szTrace),  "Channel %d switched to %s mode  (Pulse %s duration %d us).\n",
					u32V1, strMode,
					0 == (u32V2 & DEERE_OB_PULSE_DISABLED) ?"on-board" : (0 == (u32V2 & DEERE_MS_PULSE_DISABLED) ?"   M/S  " : "disabled"),
					0 == (u32V2 & DEERE_PULSE_DURATION) ? 5 : 200 );
				break;
			}
		case TRACE_ADC_REG:
			sprintf_s( szTrace, sizeof(szTrace),  "\t � Writing to ADC %d core %d addr %d Code %3d\n",u32V2, u32V1, i32V3, i32V4 );
			break;
		case TRACE_ADC_REG_DIR:
			sprintf_s( szTrace, sizeof(szTrace),  "\t %s 0x%03x ADC %d  addr 0x%02x\n", 0 == i32V4?"�  � Reading":" ��  Writing", u32V1, u32V2, i32V3>>8 );
			break;
		case TRACE_CAL_DAC:
			sprintf_s( szTrace, sizeof(szTrace),  "\t � Writing to DAC %2d (DacAddr %03x) Code %04d (delay %3d)\n",i32V4, u32V2, i32V3, u32V1 );
			break;
		case TRACE_CAL_SIG_MEAS:
			if( bVerboseRep )
			{
				if ( bVerboseReq )
					sprintf_s( szTrace, sizeof(szTrace),  "-=-=    Code %+4d read %2d \n", i32V3, u32V1);
			}
			else
			{
				sprintf_s( szTrace, sizeof(szTrace),  "=-=- Measured %c%ld.%03ld mV at %s Settled by %2d sample noise %d.%d lsb\n", i32V3<0?'-':' ',labs(i32V3)/1000, labs(i32V3)%1000,
				                                                                                               0==u32V2?"Ref.":(1==u32V2?"Ch. ":"Grd."), 
																										       u32V1,
																										       i32V4/10, i32V4%10);
			}
			break;

		case TRACE_CAL_SIGNAL:
			sprintf_s( szTrace, sizeof(szTrace),  "= = CalDC %c%4ld.%03ld mV for �%4d mV range @ %s Ohm.  %sonnected (reg 0x%04x).\n", i32V3<0?'-':' ',labs(i32V3)/1000, labs(i32V3)%1000, 
                                                                                      u32V2/2,
																					  50 == u32V1 ? "50" : "1M",
				                                                                      ( 0 == (m_pCardState->AddOnReg.u16CalDc & 0x80) )?"Disc":"C",
																					  i32V4);
			break;
		case TRACE_CAL_MS:		
			sprintf_s( szTrace, sizeof(szTrace),  "^ ^ Card %d: PosZeroCrossOffset1 = %d,  MsCalibOffset= %d\n", u32V1, i32V3, (i32V3-i32V4) );
			break;

		case TRACE_CAL_EXTTRIG:
			sprintf_s( szTrace, sizeof(szTrace),  "X X ChannelZeroCrossOffset = %d, ExtTrigZeroCrossOffset = %d i16ExtTrigOffset = %d Sig.Path delta = %d\n", u32V1, u32V2, i32V3, i32V4 );
			break;

		case TRACE_CAL_ADCALIGN:
			if( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL )
			{
				if( 1 == m_pCardState->u8NumOfAdc )
				{
					sprintf_s( szTrace, sizeof(szTrace),  " 1 2   Dual channel skew (Channel2 (%d)  vs Channel1 (%d) ) = %d\n", u32V2, u32V1, int16(u32V2 - u32V1));
					break;
				}
				else
					break;
			}
			else
			{
				if( 1 == m_pCardState->u8NumOfAdc )
					break; //NoAligment
				else if( 2 == m_pCardState->u8NumOfAdc )
				{
					break; // Not implemented yet
				}
				else
				{
					if( bVerboseRep )
					{
						if( bVerboseReq )
						{
							char str1[DEERE_ADC_COUNT*3+2];
							char str2[DEERE_ADC_COUNT*3+2];
							char strTmp1[5], strTmp2[5];
							GageSetMemoryX( str1, ' ', sizeof(str1) );
							GageZeroMemoryX( str1 + sizeof(str1) - 2, 2 );
							GageSetMemoryX( str2, ' ', sizeof(str2) );
							GageZeroMemoryX( str2 + sizeof(str2) - 2, 2 );

							uInt8 u8Adc;
							for( u8Adc = 0; u8Adc< m_pCardState->u8NumOfAdc; u8Adc++ )
							{
								sprintf_s( strTmp1, sizeof(strTmp1), "%2d", m_ai32AdcAlignment[u8Adc] );
								sprintf_s( strTmp2, sizeof(strTmp2), "%2d", m_ai32AdcAlignment[u8Adc] - m_ai32AdcAlignment[0] );
								GageCopyMemoryX( str1 + u8Adc * 3, strTmp1, 2 );
								GageCopyMemoryX( str2 + u8Adc * 3, strTmp2, 2 );
							}
							int16 i16Del[4];
							i16Del[0] = int16(u32V1); i16Del[1] = int16(u32V2); i16Del[2] = int16(i32V3); i16Del[3] = int16(i32V4);
							char str3[DEERE_ADC_COUNT*3+2];
							GageSetMemoryX( str3, ' ', sizeof(str2) );
							GageZeroMemoryX( str3 + sizeof(str2) - 2, 2 );
							int16 i16 = 10;
							for( u8Adc = 0; u8Adc < m_pCardState->u8NumOfAdc; u8Adc ++ )
							{
								i16Del[u8Adc] += (int16) (m_ai32AdcAlignment[0] - m_ai32AdcAlignment[u8Adc]);
								i16 = Min( i16, i16Del[u8Adc] );
								sprintf_s( strTmp1, sizeof(strTmp1), "%2d", i16Del[u8Adc] );
								GageCopyMemoryX( str3 + u8Adc * 3, strTmp1, 2 );
							}
							for( u8Adc = 0; u8Adc < m_pCardState->u8NumOfAdc; u8Adc ++ )
								i16Del[u8Adc] = i16Del[u8Adc] - i16;
							
							sprintf_s( szTrace, sizeof(szTrace),  "0 1 2 3 AdcAlignment  %s (%s) AdcOrder %d %d %d %d (%d %d %d %d) AdcAlingment %s => %d %d %d %d\n", 
								str1, str2, m_au8AdcOrder[0], m_au8AdcOrder[1], m_au8AdcOrder[2], m_au8AdcOrder[3], u32V1, u32V2, i32V3, i32V4,
								str3, i16Del[0],i16Del[1], i16Del[2], i16Del[3]);
						}
					}
					else
						sprintf_s( szTrace, sizeof(szTrace),  "~ ~ ~ ~ AdcAlignment = %d %d %d %d\n", u32V1, u32V2, i32V3, i32V4 );
					break;
				}
			}

		case TRACE_CONF_4360:
			sprintf_s( szTrace, sizeof(szTrace),  "   # # # AD4360 R = 0x%06x C = 0x%06x N = 0x%06x (Rcount = %4d Ncount = %4d) Atten = %d Freq %d\n", u32V1, u32V2, i32V3, m_pCardState->u32Rcount, m_pCardState->u32Ncount, m_u16Atten4302, m_u32CalFreq);
			break;
		case TRACE_EXTTRIG_LEVEL:
			sprintf_s( szTrace, sizeof(szTrace),  "   Ext.Trig Level %s Slope Level %+3d CodeLevel %4d CodeOffset %4d\n", i32V4>0?"Rising ":"Falling", i32V3, u32V1, u32V2);
			break;

		case TRACE_ADC_MATCH:
			TraceCalibrationMatch( uInt16(u32V1) );
			break;
		case TRACE_CAL_SCALE_GAIN:
			if( DEERE_IR_SCALE_FACTOR != u32V2 )
				sprintf_s( szTrace, sizeof(szTrace), "! ! ! ! Ch %d IR �%-4d mV Filter %s adjusting IR to �%d mV (%c%ld.%02ld pph) \n", 
						u32V1, m_pCardState->ChannelParams[u32V1-1].u32InputRange/2, m_pCardState->ChannelParams[u32V1-1].u32Filter == 0 ? "off" : "on ",
						uInt32((LONGLONG(m_pCardState->ChannelParams[u32V1-1].u32InputRange)* u32V2)/(DEERE_IR_SCALE_FACTOR*2)),
						u32V2 > DEERE_IR_SCALE_FACTOR ? '+':'-',(100*labs(int32(u32V2 -  DEERE_IR_SCALE_FACTOR)))/DEERE_IR_SCALE_FACTOR,
						(100*((100*labs(int32(u32V2 -  DEERE_IR_SCALE_FACTOR)))%DEERE_IR_SCALE_FACTOR))/DEERE_IR_SCALE_FACTOR);
			else
				sprintf_s( szTrace, sizeof(szTrace), "        Ch %d IR �%-4d mV Filter %s no adjusments needed \n",
				u32V1, m_pCardState->ChannelParams[u32V1-1].u32InputRange/2, m_pCardState->ChannelParams[u32V1-1].u32Filter == 0 ? "off" : "on " );
			break;
		default: return;
	}

	::OutputDebugString(szTrace);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDeereDevice::SetupForExtTrigCalib_SourceExtTrig()
{
	int32 i32Ret = SendCalibModeSetting( CS_CHAN_1, ecmCalModeExtTrig );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	// Disable channel trigger egines
   	i32Ret = SendTriggerEngineSetting( CS_TRIG_SOURCE_DISABLE );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	// Enable Ext trigger egines
	//Use fixed configuration that is guaranteed to trigger skew between different 
	//ExtTrig configurations has been deemed insignificant
	i32Ret = SendExtTriggerSetting( TRUE, 0, EXTTRIGCALIB_SLOPE, CS_GAIN_2_V, CS_REAL_IMP_1M_OHM, CS_COUPLING_AC );
	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDeereDevice::SetupForExtTrigCalib_SourceChan1()
{
	uInt16 u16Chan = CS_CHAN_1;
	if ( m_bCh2Single )
		u16Chan = CS_CHAN_2;

	int32 i32Ret = SendCalibModeSetting( u16Chan, ecmCalModeExtTrigChan );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	// Enable channel trigger egines
	i32Ret = SendTriggerEngineSetting( u16Chan, EXTTRIGCALIB_SLOPE, EXTTRIGCALIB_LEVEL_CHAN );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	// Disable Ext trigger egines
	i32Ret = SendExtTriggerSetting( FALSE,
						            0,
						            EXTTRIGCALIB_SLOPE,
						            m_CfgTrigParams[INDEX_TRIGENGINE_EXT].u32ExtTriggerRange,
						            CS_REAL_IMP_1M_OHM, //m_CfgTrigParams[INDEX_TRIGENGINE_EXT].u32ExtImpedance,
						            CS_COUPLING_AC );
	return i32Ret;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// Ajustment for offset error when changing from ExtTrig calib mode to normal mode
#define	EXTTRIGCALIB_DATAPATH_ADJUST		0

int32	CsDeereDevice::CalculateExtTriggerSkew()
{
	int32		i32Ret = CS_SUCCESS;
	int32		i32OffsetAdjust;
	int32		i32StartPos;
    uInt32		u32ChanPosCross = 0;


	if ( DBG_NEVER_CALIB == m_MasterIndependent->m_u8DebugCalibExtTrig )
		return CS_SUCCESS;

	if ( m_bMulrecAvgTD )
		i32StartPos = (int32) (-1*(m_i64HwSegmentSize - m_i64HwPostDepth));
	else
		i32StartPos = (-3*MAX_EXTTRIG_ADJUST_SPAN/2);

	SetupForExtTrigCalib_SourceChan1();
	i32Ret = AcquireCalibSignal( CS_CHAN_1, i32StartPos, &i32OffsetAdjust );
	if( CS_FAILED(i32Ret) )	
	{
		::OutputDebugString("Failed to capture pulse using channel trigger.\n" );
		char  szTemp[100];
		sprintf_s( szTemp, sizeof(szTemp), TEXT("Ch. Ext range %d mV. Ext imped. %s"), 
			    m_CfgTrigParams[INDEX_TRIGENGINE_EXT].u32ExtTriggerRange, 
				CS_REAL_IMP_50_OHM == m_CfgTrigParams[INDEX_TRIGENGINE_EXT].u32ExtImpedance? TEXT("50 Ohm"):TEXT("HiZ"));
		::OutputDebugString(szTemp );
#if !DBG
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_EXTTRIG_CALIB_FAILED, szTemp );
#endif
		return i32Ret;
	}

	if ( !CsDetectLevelCrossPosition( &m_pi16CalibBuffer[i32OffsetAdjust], EXTTRIG_CALIB_LENGTH, &u32ChanPosCross) )
	{
		::OutputDebugString("Failed to detect edge using channel trigger.\n");
#if !DBG
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_EXTTRIG_CALIB_FAILED, TEXT("No edge on channel") );
#endif
		return CS_EXTTRIG_CALIB_FAILURE;
	}


#ifdef DEBUG_EXTTRIGCALIB	
	RtlCopyMemory( m_pu8DebugBuffer1, &m_pi16CalibBuffer[i32OffsetAdjust], PAGE_SIZE );
#endif
	int16	i16ExtTrigOffset = 0;
	int16	i16MaxAjust = MAX_EXTTRIG_ADJUST_SPAN/2;
	int16	i16MinAjust = -1*MAX_EXTTRIG_ADJUST_SPAN/2;


	// Switch to Ext Trigger
	SetupForExtTrigCalib_SourceExtTrig();
	
	// Patch for Firmware bug
	// With Ext Trigger souece, it does not trigger on the first acquisition.???????
	// Do a dummy accquisition
	AcquireCalibSignal( CS_CHAN_1, i32StartPos, &i32OffsetAdjust );

	i32Ret = AcquireCalibSignal( CS_CHAN_1, i32StartPos, &i32OffsetAdjust );
	if( CS_FAILED(i32Ret) )	
	{
		::OutputDebugString("Failed on Capturing Calib Signal (Ext Trig).");
		char  szTemp[100];
		sprintf_s( szTemp, sizeof(szTemp), TEXT("Ext. Ext range %d mV. Ext imped. %s"),
			m_CfgTrigParams[INDEX_TRIGENGINE_EXT].u32ExtTriggerRange, 
			CS_REAL_IMP_50_OHM == m_CfgTrigParams[INDEX_TRIGENGINE_EXT].u32ExtImpedance? TEXT("50 Ohm"):TEXT("HiZ"));
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_EXTTRIG_CALIB_FAILED, szTemp );
		::OutputDebugString(szTemp);
		return i32Ret;
	}

	uInt32 u32PosCross(0);
	if ( ! CsDetectLevelCrossPosition( &m_pi16CalibBuffer[i32OffsetAdjust], EXTTRIG_CALIB_LENGTH, &u32PosCross) )
	{
		::OutputDebugString("Failed to detect edge using external trigger.\n");
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_EXTTRIG_CALIB_FAILED, TEXT("No edge on ExtTrig") );
		return CS_EXTTRIG_CALIB_FAILURE;
	}
#ifdef DEBUG_EXTTRIGCALIB
	RtlCopyMemory( m_pu8DebugBufferEx, &m_pi16CalibBuffer[i32OffsetAdjust], PAGE_SIZE );
#endif
	i16ExtTrigOffset = (int16)(u32PosCross - u32ChanPosCross);
	i16ExtTrigOffset += EXTTRIGCALIB_DATAPATH_ADJUST;

	TraceCalib(TRACE_CAL_EXTTRIG, u32ChanPosCross, u32PosCross, i16ExtTrigOffset);

	i16ExtTrigOffset *= (DEERE_ADC_COUNT/m_pCardState->u8NumOfAdc);

	if ( i16ExtTrigOffset < i16MinAjust  || i16ExtTrigOffset > i16MaxAjust )
	{
		char szText[256];
		sprintf_s( szText, sizeof(szText), "**ERROR** ExtTrigAddrOffset=%d is out of range supported\n", i16ExtTrigOffset);

		::OutputDebugString(szText);
		m_pCardState->i16ExtTrigSkew = 0;
		return CS_EXTTRIG_CALIB_FAILURE;
	}
	else
	{
#if 0
		// Debug ext trigger clib
		char szText[256];
		sprintf_s( szText, sizeof(szText), "ExtTrigAddrOffset=%d\n", i16ExtTrigOffset);
		::OutputDebugString(szText);
#endif
		m_pCardState->i16ExtTrigSkew = i16ExtTrigOffset;
		m_pCardState->bExtTrigCalib = false;
		return CS_SUCCESS;
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::MsCalculateMasterSlaveSkew()
{
	int32 i32Ret = CS_SUCCESS;
	int32 i32MsCalibRet = CS_SUCCESS;
	CsDeereDevice* pDevice = m_MasterIndependent;

	if (( DBG_ALWAYS_CALIB != m_u8NeverCalibrateMs ) && ((0 == m_MasterIndependent->m_Next || !m_MasterIndependent->m_pCardState->bMasterSlaveCalib)) )
		return CS_SUCCESS;

	// Switch to Ms calib mode
	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		// Waiting for relay settled onluy on the last card.
		if ( pDevice->m_Next != 0 )
			i32Ret = pDevice->SendCalibModeSetting( CS_CHAN_1, ecmCalModeMs, 0 );
		else
			i32Ret = pDevice->SendCalibModeSetting( CS_CHAN_1, ecmCalModeMs );
		if( CS_FAILED(i32Ret) )return i32Ret;
		
		pDevice = pDevice->m_Next;
	}

	// Enable trigger only from channel 1 of Master cards
	SendTriggerEngineSetting( CS_TRIG_SOURCE_CHAN_1, CS_TRIG_COND_NEG_SLOPE, MSCALIB_TRIGLEVEL );

	CSTIMER		CsTimeout;
	CsTimeout.Set( DEERE_CALIBMS_TIMEOUT );

	// One acquisition ...
	m_MasterIndependent->AcquireData();
	while( STILL_BUSY == m_MasterIndependent->ReadBusyStatus() )
	{
		if ( CsTimeout.State() )
		{
			if( STILL_BUSY == m_MasterIndependent->ReadBusyStatus() )
			{
				::OutputDebugString("Channel skew Calib error. Trigger timeout\n");
				m_MasterIndependent->ForceTrigger();
				m_MasterIndependent->MsAbort();
				i32Ret =  CS_MASTERSLAVE_CALIB_FAILURE;
				break;
			}
		}
	}

	int16 i16MasterCrossPos = 0;
	int16 i16SlaveCrossPos = 0;

	// Detect zero crossing
	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		if( pDevice == m_MasterIndependent )
			i32MsCalibRet = pDevice->CsDetectMsZeroCrossAddress( &i16MasterCrossPos );
		else
			i32MsCalibRet = pDevice->CsDetectMsZeroCrossAddress( &i16SlaveCrossPos, m_MasterIndependent->m_i16ZeroCrossLevel );

		if( CS_FAILED(i32MsCalibRet) )	
		{
			// Assume that this card has the same offset as the master one
			char szTrace[128];
			pDevice->m_pCardState->i16MasterSlaveSkew = 0;
			sprintf_s( szTrace, sizeof(szTrace), "Master/Slave Calib error. Zero crossing detection error on card %d\n", pDevice->m_pCardState->u8MsCardId);
			::OutputDebugString(szTrace);
			break;		}

		if( pDevice != m_MasterIndependent )
		{
			TraceCalib(TRACE_CAL_MS, pDevice->m_pCardState->u16CardNumber, 0, i16SlaveCrossPos, i16MasterCrossPos);
			// Sometimes Etb from Master and Slave cards are not the same.
			// Should we bother about this and consider as an error ?????????
			if( m_MasterIndependent->m_Etb != pDevice->m_Etb )
				::OutputDebugString( "Master/Slave Calib error. Error M/S Etb\n");

			pDevice->m_pCardState->i16MasterSlaveSkew = i16SlaveCrossPos - i16MasterCrossPos;
		}
		else
			pDevice->m_pCardState->i16MasterSlaveSkew = 0;

		pDevice->m_pCardState->i16MasterSlaveSkew *= DEERE_ADC_COUNT/(m_pCardState->u8NumOfAdc);

		pDevice = pDevice->m_Next;
	}	

	if ( CS_SUCCEEDED(i32MsCalibRet) )
		m_MasterIndependent->m_pCardState->bMasterSlaveCalib = false;

	return i32MsCalibRet;
}




#define	CALIB_SKEW_RETRY		3

//-----------------------------------------------------------------------------
// Calibration for Master/Slave and ExtTrigger skew
//-----------------------------------------------------------------------------
int32 CsDeereDevice::MsCalibrateSkew()
{
	//Only run on the first card
	if( this != m_MasterIndependent ) 
		return CS_SUCCESS;

	bool	bCalibMs =  (m_u8NeverCalibrateMs == DBG_ALWAYS_CALIB) ? true : ( 0 != m_MasterIndependent->m_Next ) &&  m_MasterIndependent->m_pCardState->bMasterSlaveCalib && (m_u8NeverCalibrateMs != DBG_NEVER_CALIB);
	bool	bCalibExtTrig = (m_u8DebugCalibExtTrig == DBG_ALWAYS_CALIB) ? true : m_MasterIndependent->m_pCardState->bExtTrigCalib && (m_u8DebugCalibExtTrig != DBG_NEVER_CALIB) && (m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].i32Source != 0 );
	
	if ( ! bCalibExtTrig && ! bCalibMs )
		return CS_SUCCESS;

	int32 i32Ret = CS_SUCCESS;
	int32 i32CalibRet = CS_SUCCESS;
	CsDeereDevice* 	pDevice = m_MasterIndependent;

	// Enter calibration mode
	while ( pDevice )
	{
		i32Ret = pDevice->PrepareForCalibration( true );
		if( CS_FAILED(i32Ret) ) break;
		pDevice = pDevice->m_Next;
	}					

	uInt32	u32Retry = 0 == m_MasterIndependent->m_Next ? 0 : CALIB_SKEW_RETRY;
	bool	bTemp = m_bZeroTrigAddrOffset;

	// Master/Slave alignment calibration
	if ( bCalibMs )
	{		
		pDevice = m_MasterIndependent;
		while ( pDevice )
		{
			// Switch to Ms calib mode
			i32Ret = pDevice->SetupForCalibration(true, ecmCalModeMs);
			if( CS_FAILED(i32Ret) ) goto ExitMsCalib;
			pDevice = pDevice->m_Next;
		}

		if ( m_MasterIndependent->m_bMsDecimationReset )
			m_MasterIndependent->ResetMSDecimationSynch();

		m_bZeroTrigAddrOffset = true;

		for (uInt32 i = 0; i <= u32Retry; i ++ )
		{
			m_MasterIndependent->ResetMasterSlave();
			i32CalibRet = MsCalculateMasterSlaveSkew();
			if( CS_SUCCEEDED(i32CalibRet) ) break;
		}

ExitMsCalib:
		pDevice = m_MasterIndependent;
		while ( pDevice )
		{
			i32Ret = pDevice->SetupForCalibration(false, ecmCalModeMs);
			pDevice = pDevice->m_Next;
		}
	}

	if ( CS_FAILED(i32CalibRet) )
		bCalibExtTrig = false;

	// External trigger alignment calibration
	if ( bCalibExtTrig )
	{
		i32Ret = SetupForCalibration(true, ecmCalModeExtTrig);
		if( CS_SUCCEEDED(i32Ret) )
			i32CalibRet = m_MasterIndependent->CalculateExtTriggerSkew();

		i32Ret = SetupForCalibration(false, ecmCalModeExtTrig);
	}

	m_bZeroTrigAddrOffset = bTemp;

	// Switch back to normal mode
	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		if ( NULL != pDevice->m_Next )
			pDevice->SendCalibModeSetting( CS_CHAN_1, ecmCalModeNormal, 0 );
		else
			pDevice->SendCalibModeSetting( CS_CHAN_1, ecmCalModeNormal ); // With default waiting time only on the last card
		pDevice = pDevice->m_Next;
	}

	// Exit calibration mode
	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		i32Ret = pDevice->PrepareForCalibration( false );
		if( CS_FAILED(i32Ret) ) break;
		pDevice = pDevice->m_Next;
	}				

	if ( m_MasterIndependent->m_bMsDecimationReset )
		m_MasterIndependent->ResetMSDecimationSynch();

	if ( CS_FAILED(i32CalibRet) )
		return i32CalibRet;
	else
		return i32Ret;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define EDGECHECK_LENGTH		16  //Points
#define EDGECHECK_HIGHT  		5  // % of FS

bool CsDeereDevice::CsDetectLevelCrossPosition( int16* pi16Buffer, uInt32 u32BufferSize, uInt32* pu32PosCross, int16 i16DetectLevel, int16* pi16CrossLevel )
{
	bool	bEdgeDetected = false;
	uInt32	j;
	uInt32	u32Position = 0;
	int16	i16Pol = -1;			// for negative slope (Detection of 1rst falling edge)

	if ( u32BufferSize < 2*(EDGECHECK_LENGTH + 2) )
		return false;			// Buffer size is too short

	for ( uInt32 i = EDGECHECK_LENGTH + 1; i < (u32BufferSize - EDGECHECK_LENGTH - 2 ); i++ )
	{
		if ( (i16Pol*(i16DetectLevel-pi16Buffer[i]) >= 0) && (i16Pol*(i16DetectLevel - pi16Buffer[i+1]) < 0) )
		{
			u32Position = i+1;
			bEdgeDetected = true;
			const int16 ci16EddgeHight = (int16)labs(m_pCardState->AcqConfig.i32SampleRes * EDGECHECK_HIGHT)/100;
			// Validation of the left and right side of zero crossing
			for (j = 1; j <= EDGECHECK_LENGTH; j++)
				if ( (i16Pol*(pi16Buffer[i-j] - i16DetectLevel) > ci16EddgeHight) || (i16Pol*(pi16Buffer[i+1+j] - i16DetectLevel) < -ci16EddgeHight) )
				{
					bEdgeDetected = false; break;
				}
			if ( bEdgeDetected )break;
		}
	}

	if ( !bEdgeDetected )
			return false;		// Zero crossing not found
	else if ( ecmCalModeMs == m_CalibMode )
		if ( (i16DetectLevel-i16Pol*pi16Buffer[u32Position-1])  < (i16Pol*pi16Buffer[u32Position]-i16DetectLevel) )
			u32Position--; // Pick the position which has the ADC value closest to the detection level

	if ( NULL != pu32PosCross )
		*pu32PosCross = u32Position;

	if ( NULL != pi16CrossLevel )
		*pi16CrossLevel = pi16Buffer[u32Position];

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int32 CsDeereDevice::MsAdjustDigitalDelay()
{
	CsDeereDevice	*pDevice = m_MasterIndependent;
	int16			 i16AdjustOffset = 0;


	if ( DBG_NEVER_CALIB == m_MasterIndependent->m_u8DebugCalibExtTrig )
		i16AdjustOffset = 0;
	else if (m_MasterIndependent->m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].i32Source != 0 )
		i16AdjustOffset = m_pCardState->i16ExtTrigSkew;

	while ( pDevice )
	{
		if ( pDevice->m_bMsOffsetAdjust )
			pDevice->SetDigitalDelay( i16AdjustOffset + pDevice->m_pCardState->i16MasterSlaveSkew );
		else
			pDevice->SetDigitalDelay( i16AdjustOffset );
		pDevice = pDevice->m_Next;
	}
	return CS_SUCCESS;
}

int32 CsDeereDevice::ConfigureDebugCalSrc()
{
	if( m_bDebugCalibSrc )
	{
		int32 i32Ret;
		uInt16 u16DebugChannel = 0;
		for( uInt16 i = 1; i <= m_pCsBoard->NbAnalogChannel; i++ )
		{
			i32Ret = SendCalibModeSetting(i, ecmCalModeNormal );
			if( CS_FAILED(i32Ret) )	return i32Ret;

			if ( 0 != m_pCardState->ChannelParams[i-1].u32Filter )
				if( ecmCalModeAdc == m_ecmDebugCalSrcMode )
					SendCalibModeSetting(i, m_ecmDebugCalSrcMode );
				else if( 0 == u16DebugChannel )
					u16DebugChannel = i;
				else
				{
					char szTrace[128];
					sprintf_s( szTrace, sizeof(szTrace),  "! ! ! ! ! ! More then one DebugCalSrc channels. %d and %d\n", u16DebugChannel, i );
					::OutputDebugString(szTrace);
				}
		}
		if( 0 != u16DebugChannel )
			return SendCalibModeSetting(u16DebugChannel, m_ecmDebugCalSrcMode );
	}
	return CS_SUCCESS;
}

void CsDeereDevice::OutputCalibTraceUsage(void)
{
	char szTrace[128];

	::OutputDebugString("\nTraceCalib flags:\n");
	sprintf_s( szTrace, sizeof(szTrace),  "0x%08x -  TRACE_NULL_OFFSET     \n", TRACE_NULL_OFFSET     );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace),  "0x%08x -  TRACE_CAL_FE_OFFSET   \n", TRACE_CAL_FE_OFFSET   );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace),  "0x%08x -  TRACE_CAL_GAIN        \n", TRACE_CAL_GAIN        );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace),  "0x%08x -  TRACE_CAL_POS         \n", TRACE_CAL_POS         );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace),  "0x%08x -  TRACE_CAL_SIGNAL      \n", TRACE_CAL_SIGNAL      );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_CAL_DAC         \n", TRACE_CAL_DAC         );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_CAL_RESULT      \n", TRACE_CAL_RESULT      );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_CAL_SIG_MEAS    \n", TRACE_CAL_SIG_MEAS    );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_VERB            \n", TRACE_VERB            );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_CAL_INPUT       \n", TRACE_CAL_INPUT       );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_ADC_REG         \n", TRACE_ADC_REG         );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_ADC_CORE_ORDER  \n", TRACE_ADC_CORE_ORDER  );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_CAL_PROGRESS    \n", TRACE_CAL_PROGRESS    );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_CAL_MS	       \n", TRACE_CAL_MS	       );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_CAL_EXTTRIG	   \n", TRACE_CAL_EXTTRIG	   );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_CAL_ADCALIGN	   \n", TRACE_CAL_ADCALIGN	  );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_MS_BIST_TEST	   \n", TRACE_MS_BIST_TEST	  );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_ADC_REG_DIR     \n", TRACE_ADC_REG_DIR     );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_CONF_4360       \n", TRACE_CONF_4360       );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_EXTTRIG_LEVEL   \n", TRACE_EXTTRIG_LEVEL   );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_MATCH_OFF_MED   \n", TRACE_MATCH_OFF_MED   );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_MATCH_OFF_FINE  \n", TRACE_MATCH_OFF_FINE  );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_MATCH_GAIN_MED  \n", TRACE_MATCH_GAIN_MED  );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_MATCH_GAIN_FINE \n", TRACE_MATCH_GAIN_FINE );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_ADC_MATCH       \n", TRACE_ADC_MATCH       );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_CORE_SKEW_MATCH \n", TRACE_CORE_SKEW_MATCH );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_ADC_PHASE_MATCH \n", TRACE_ADC_PHASE_MATCH );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_CORE_SKEW_MEAS  \n", TRACE_CORE_SKEW_MEAS  );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_CAL_SCALE_GAIN  \n", TRACE_CAL_SCALE_GAIN  );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_RES_20000000    \n", TRACE_RES_20000000    );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_FINAL_RESULT    \n", TRACE_FINAL_RESULT    );
	::OutputDebugString(szTrace);;
	sprintf_s( szTrace, sizeof(szTrace), "0x%08x -  TRACE_USAGE           \n", TRACE_USAGE);           
	::OutputDebugString(szTrace);

	OutputDacInfo();

}

void CsDeereDevice::OutputDacInfo(void)
{
	::OutputDebugString("\nDAC controls: \n\n");
	::OutputDebugString(  "\tCHAN_BASE_1        0\n");
	::OutputDebugString(  "\tCHAN_BASE_2       17\n\n");

	::OutputDebugString(  "\tOFFSET_NULL_1      0\n");
	::OutputDebugString(  "\tOFFSET_COMP_1      1\n");
	::OutputDebugString(  "\tUSER_OFFSET_1      2\n\n");	
	
	::OutputDebugString(  "\tCLKPHASE_0         3\n");
	::OutputDebugString(  "\tCLKPHASE_2         4\n");

	::OutputDebugString(  "\tADC_BASE_0         5\n");
	::OutputDebugString(  "\tADC_BASE_2	       11\n");
	::OutputDebugString(  "\tADC_BASE_1	       22\n");
	::OutputDebugString(  "\tADC_BASE_3	       28\n\n");

	::OutputDebugString(  "\tADC_COARSE_OFFSET  0\n");
	::OutputDebugString(  "\tADC_FINE_OFFSET    1\n");
	::OutputDebugString(  "\tADC_COARSE_GAIN    2\n" );
	::OutputDebugString(  "\tADC_MEDIUM_GAIN    3\n" );
	::OutputDebugString(  "\tADC_FINE_GAIN      4\n");
	::OutputDebugString(  "\tADC_SKEW           5\n\n");

	::OutputDebugString(  "\tMS_CLK_PHASE_ADJ  34\n");	
	::OutputDebugString(  "\tEXTRIG_LEVEL      35\n");	
	::OutputDebugString(  "\tEXTRIG_OFFSET     36\n\n");	

	::OutputDebugString(  "\tR_COUNT           60\n");  
	::OutputDebugString(  "\tN_COUNT           61\n");  
	::OutputDebugString(  "\tAC_ATTEN          62\n");  
	::OutputDebugString(  "\tEXT_TRIG_SHIFT    63\n");  
	::OutputDebugString(  "\tADC_CORRECTION    64\n");  
	::OutputDebugString(  "\t\n\n");
}													   


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDeereDevice::CsDetectMsZeroCrossAddress(int16 *i16CrossPos, int16 i16BestMatchLevel )
{
	int64	i64StartPos = {0};			// Relative to trigger position
	int32	i32OffsetAdjust;
	uInt32	u32PosCrossing;


	if ( m_bMulrecAvgTD )
		i64StartPos  =  (-1*(m_i64HwSegmentSize - m_i64HwPostDepth));
	else
		i64StartPos = -64;

	in_PREPARE_DATATRANSFER		InXferEx ={0};
	out_PREPARE_DATATRANSFER	OutXferEx = {0};

	InXferEx.i64StartAddr	= i64StartPos;
	InXferEx.u16Channel		= CS_CHAN_1;
	InXferEx.u32DataMask	= GetDataMaskModeTransfer(0);
	InXferEx.u32FifoMode	= GetFifoModeTransfer(CS_CHAN_1);
	InXferEx.bBusMasterDma	= m_BusMasterSupport;
	InXferEx.bIntrEnable	= 0;
	InXferEx.u32Segment		= 1;
	InXferEx.u32SampleSize	= sizeof(int16);

	IoctlPreDataTransfer( &InXferEx, &OutXferEx );
	CardReadMemory( m_pi16CalibBuffer, DEERE_CALIB_BUFFER_SIZE*sizeof(int16) );
	
	i32OffsetAdjust = -1*OutXferEx.i32ActualStartOffset;

#ifdef DEBUG_MASTERSLAVE
	// Keep a copy of the pulse for debug purpose
	RtlCopyMemory( m_PulseData, &m_pi16CalibBuffer[i32OffsetAdjust], Min(PAGE_SIZE, DEERE_CALIB_BUFFER_SIZE) );
#endif

	int16 i16DetectLevel =  (int16)(m_pCardState->AcqConfig.i32SampleRes/2);	// Half of full scale below 0
	// M/S calib pulse is a unipole pulse, that's why the detectton level is != 0
	if ( ! CsDetectLevelCrossPosition( &m_pi16CalibBuffer[i32OffsetAdjust], MS_CALIB_LENGTH, &u32PosCrossing, i16DetectLevel, &m_i16ZeroCrossLevel ) )
	{
#if !DBG
		CsEventLog EventLog;
		EventLog.Report( CS_ADC_ALIGN_ERROR, "Edge detection error on M/S cal" );
#endif	
		::OutputDebugString(" ! ! ! Edge detection error on M/S cal\n");		
		return CS_MASTERSLAVE_CALIB_FAILURE;
	}
	else
	{
		if ( i16BestMatchLevel != 0 )
		{
			// Improved edge detection algorithm
			int16* pi16Buf = m_pi16CalibBuffer + i32OffsetAdjust + u32PosCrossing - IMPROVED_EDGE_COMPARE_LENGTH/2;
			int32 i32MinDiff = m_i16ZeroCrossLevel - i16BestMatchLevel;
		
			// Lookking for a sample which has level closest with the reference level
			for (int i = 0; i  < IMPROVED_EDGE_COMPARE_LENGTH;  i++ )
			{
				int32	i32Diff = pi16Buf[i] - i16BestMatchLevel;

				if ( Abs(i32Diff) < Abs(i32MinDiff) )
				{
					i32MinDiff = i32Diff;
					u32PosCrossing = u32PosCrossing - IMPROVED_EDGE_COMPARE_LENGTH/2 + i;
				}	
			}			
		}
		*i16CrossPos = (int16) u32PosCrossing;
		return CS_SUCCESS;
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::MsCalibrateAdcAlign2ChannelsDual()
{
	// This function should be called only on the master card
	if( this != m_MasterIndependent  || m_pCardState->bDualChanAligned || ( 1 == m_pCsBoard->NbAnalogChannel ) )
	{
		m_pCardState->bDualChanAligned = true;
		return CS_SUCCESS;
	}

	int32 i32Ret = CS_SUCCESS;
	int32 i32CalibRet = CS_SUCCESS;
	CsDeereDevice* pDevice = m_MasterIndependent;
	{
		pDevice = m_MasterIndependent;
		// Prepare both Channels 1 and 2 with filters
		while ( pDevice )
		{
			i32Ret = pDevice->SetupForCalibration(true, ecmCalModeDual);
			if( CS_FAILED(i32Ret) ) break;
			pDevice = pDevice->m_Next;
		}
	}

	// Force to Dual channel acquisition mode
	CSACQUISITIONCONFIG CalibAcqConfig;
	CsGetAcquisitionConfig( &CalibAcqConfig );

	CalibAcqConfig.u32Mode &= ~CS_MODE_SINGLE;
	CalibAcqConfig.u32Mode |= CS_MODE_DUAL;

	if ( CS_SUCCEEDED(i32Ret) )
	{
		pDevice = m_MasterIndependent;
		while ( pDevice )
		{
			i32Ret = pDevice->CsSetAcquisitionConfig(&CalibAcqConfig);
			if( CS_FAILED(i32Ret) )return i32Ret;

			// After changing acquisition mode we have to reset the AVG mode
			if ( m_bMulrecAvgTD )
				pDevice->SetupHwAverageMode(false);

			pDevice = pDevice->m_Next;
		}
	}

	// Calibrate one card at time
	if ( CS_SUCCEEDED(i32Ret) )
	{
		pDevice = m_MasterIndependent;
		while ( pDevice )
		{
			i32CalibRet = pDevice->CalculateDualChannelSkewAdc();
			if( CS_FAILED(i32CalibRet) ) break;
			pDevice = pDevice->m_Next;
		}
	}
	else
		i32CalibRet = i32Ret;

	// Force to Single channel acquisition mode
	CalibAcqConfig.u32Mode &= ~CS_MODE_DUAL;
	CalibAcqConfig.u32Mode |= CS_MODE_SINGLE;

	if ( CS_SUCCEEDED(i32Ret) )
	{
		pDevice = m_MasterIndependent;
		while ( pDevice )
		{
			i32Ret = pDevice->CsSetAcquisitionConfig(&CalibAcqConfig);
			if( CS_FAILED(i32Ret) ) return i32Ret;

			// After changing acquisition mode we have to reset the AVG mode
			if ( m_bMulrecAvgTD )
				pDevice->SetupHwAverageMode(false);

			pDevice = pDevice->m_Next;
		}
	}

	// Switch back all cards to normal mode
	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		i32Ret = pDevice->SetupForCalibration(false, ecmCalModeDual);
		if( CS_FAILED(i32Ret) ) break;
		pDevice = pDevice->m_Next;
	}

	if ( CS_FAILED(i32CalibRet) )
		return i32CalibRet;
	else
		return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::CalculateDualChannelSkewAdc()
{
	int32	i32Ret = CS_SUCCESS;
	int32	i32RetDualCal = CS_SUCCESS;
	char	szErrorInfo[128];

	int16	i16LevelCross1 = 0, i16LevelCross2 = 0;
	uInt32  u32ChanPosCross1 = 0, u32ChanPosCross2 = 0;
	char	szTrace[256];
	int32	i32OffsetAdjust = 0;

	in_PREPARE_DATATRANSFER		InXferEx ={0};
	out_PREPARE_DATATRANSFER	OutXferEx = {0};

	if ( m_pCardState->bDualChanAligned || ( 1 == m_pCsBoard->NbAnalogChannel ) )
	{
		m_pCardState->bDualChanAligned = true;
		return CS_SUCCESS;
	}
	else if ( 1 == m_pCardState->u8NumOfAdc )
	{
		// Set all ADC alignment to neutral position
		RtlZeroMemory( m_i16DelayAdc, sizeof( m_i16DelayAdc )); 
		i32Ret = WriteRegisterThruNios( 0, DEERE_ADC_ALIGN_WR_CMD );
	}
	
	SendCalibModeSetting( CS_CHAN_1, ecmCalModeDual, 0 );
	SendCalibModeSetting( CS_CHAN_2, ecmCalModeDual );    // With default waiting time

	// Trigger from channel 1
	i32Ret = SendTriggerEngineSetting( CS_TRIG_SOURCE_CHAN_1, CS_TRIG_COND_NEG_SLOPE, EXTTRIGCALIB_LEVEL_CHAN );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	// One acquisition ...
	m_MasterIndependent->AcquireData();

	if ( (DEERE_PULSE_DISABLED & m_pCardState->AddOnReg.u16CalibPulse) != DEERE_PULSE_DISABLED )
		GeneratePulseCalib();

	CSTIMER		CsTimeout;
	CsTimeout.Set( DEERE_CALIB_TIMEOUT_SHORT );

	while( STILL_BUSY == m_MasterIndependent->ReadBusyStatus() )
	{
		if ( CsTimeout.State() )
		{
			if( STILL_BUSY == m_MasterIndependent->ReadBusyStatus() )
			{
#if !DBG
				CsEventLog EventLog;
				EventLog.Report( CS_ADC_ALIGN_ERROR, "(Dual Chan) Trigger timeout" );
#endif	
				m_MasterIndependent->ForceTrigger();
				m_MasterIndependent->MsAbort();
				i32Ret =  CS_ADCALIGN_CALIB_FAILURE;
				goto safeExit;
			}
		}
	}

	int64	i64StartPos;
	
	ASSERT( (m_i64HwSegmentSize - m_i64HwPostDepth) >= DUAL_CHANNEL_CALIB_PRETRIG );
	i64StartPos = -1*DUAL_CHANNEL_CALIB_PRETRIG;

	if ( m_bMulrecAvgTD )
	{
		// ReadData in interleaved mode
		// In order to avoid allocation of another buffer for non-interleaved data, we use the same buffer but remove unwanted data.
		// Draw back is we have to do the same transfer twice.
		// Adter the first transfer, we renoved data from channel 2
		// Adter the second transfer, we renoved data from channel 1.

		m_pCardState->AcqConfig.u32Mode  &= ~CS_MODE_DUAL;
		m_pCardState->AcqConfig.u32Mode  |= CS_MODE_SINGLE;

		InXferEx.i64StartAddr	= i64StartPos;
		InXferEx.u16Channel		= CS_CHAN_1;
		InXferEx.u32DataMask	= GetDataMaskModeTransfer(0);
		InXferEx.u32FifoMode	= 0;	// Interleaved
		InXferEx.bBusMasterDma	= m_BusMasterSupport;
		InXferEx.bIntrEnable	= 0;
		InXferEx.u32Segment		= 1;
		InXferEx.u32SampleSize	= sizeof(int16);

		IoctlPreDataTransfer( &InXferEx, &OutXferEx );
		CardReadMemory( m_pi16CalibBuffer, DEERE_CALIB_BUFFER_SIZE*sizeof(int16) );

		i32OffsetAdjust = -1*OutXferEx.i32ActualStartOffset;

		// Interleaved data format:  Ch1 Ch2 Ch1 Ch2 Ch1 Ch2 ....
		// Removed Ch2 Data
		int16* pu16Buff = m_pi16CalibBuffer;
		int j;
		int i;
		for (i = 0, j = 0; i < 2*DUAL_CHANNEL_CALIB_LENGTH; i+=2, j++)
		{
			pu16Buff[j] = pu16Buff[i];
		}

		if ( ! CsDetectLevelCrossPosition( &m_pi16CalibBuffer[i32OffsetAdjust], DUAL_CHANNEL_CALIB_LENGTH, &u32ChanPosCross1, 0, &i16LevelCross1) )
		{
			sprintf_s(szErrorInfo, sizeof(szErrorInfo), "Edge detection error on Chan 1, Card %d",  m_pCardState->u16CardNumber );
#if !DBG
			CsEventLog EventLog;
			EventLog.Report( CS_ADC_ALIGN_ERROR, szErrorInfo );
#endif	
			sprintf_s( szTrace, sizeof(szTrace),  "Channel skew Calib error. %s\n", szErrorInfo);
			::OutputDebugString(szTrace);
			i32RetDualCal = CS_ADCALIGN_CALIB_FAILURE;
			goto safeExit;
		}

		// ReadData in interlevaed mode again ...
		IoctlPreDataTransfer( &InXferEx, &OutXferEx );
		CardReadMemory( m_pi16CalibBuffer, DEERE_CALIB_BUFFER_SIZE*sizeof(int16) );

		// Interleaved data format:  Ch1 Ch2 Ch1 Ch2 Ch1 Ch2 ....
		// Removed Ch1 Data
		for (i = 1, j = 0; i < 2*DUAL_CHANNEL_CALIB_LENGTH; i+=2, j++)
		{
			pu16Buff[j] = pu16Buff[i];
		}


		if ( ! CsDetectLevelCrossPosition( &m_pi16CalibBuffer[i32OffsetAdjust], DUAL_CHANNEL_CALIB_LENGTH, &u32ChanPosCross2, 0, &i16LevelCross2) )
		{
			sprintf_s(szErrorInfo, sizeof(szErrorInfo),  "Edge detection error on Chan 2, Card %d",  m_pCardState->u16CardNumber );
#if !DBG
			CsEventLog EventLog;
			EventLog.Report( CS_ADC_ALIGN_ERROR, szErrorInfo );
#endif	
			sprintf_s( szTrace, sizeof(szTrace),  "Channel skew Calib error. %s\n", szErrorInfo);
			::OutputDebugString(szTrace);
			i32RetDualCal = CS_ADCALIGN_CALIB_FAILURE;
			goto safeExit;
		}

		m_pCardState->AcqConfig.u32Mode  &= ~CS_MODE_SINGLE;
		m_pCardState->AcqConfig.u32Mode  |= CS_MODE_DUAL;
	}
	else
	{
		InXferEx.i64StartAddr	= i64StartPos;
		InXferEx.u16Channel	= CS_CHAN_1;
		InXferEx.u32DataMask	= GetDataMaskModeTransfer(0);
		InXferEx.bBusMasterDma	= m_BusMasterSupport;
		InXferEx.bIntrEnable	= 0;
		InXferEx.u32Segment		= 1;
		InXferEx.u32SampleSize	= sizeof(int16);

		// Get data from each of channel and search for zero crossing position
		for (uInt16 u16Chan = CS_CHAN_1; u16Chan <= CS_CHAN_2; u16Chan ++ )
		{
			InXferEx.u16Channel	= u16Chan;
			InXferEx.u32FifoMode	= GetFifoModeTransfer(u16Chan);

			IoctlPreDataTransfer( &InXferEx, &OutXferEx );
			CardReadMemory( m_pi16CalibBuffer, DEERE_CALIB_BUFFER_SIZE*sizeof(int16) );

			i32OffsetAdjust = -1*OutXferEx.i32ActualStartOffset;

			bool bSuccess;
			if ( CS_CHAN_1 == u16Chan )
				bSuccess = CsDetectLevelCrossPosition( &m_pi16CalibBuffer[i32OffsetAdjust], EXTTRIG_CALIB_LENGTH, &u32ChanPosCross1, 0, &i16LevelCross1);
			else
				bSuccess = CsDetectLevelCrossPosition( &m_pi16CalibBuffer[i32OffsetAdjust], EXTTRIG_CALIB_LENGTH, &u32ChanPosCross2, 0, &i16LevelCross2);

			if ( ! bSuccess )
			{
				char szTrace[256];

				sprintf_s( szErrorInfo, sizeof(szErrorInfo), "Edge detection error on Chan %d, Card %d", u16Chan, m_pCardState->u16CardNumber );
				::OutputDebugString("Pulse shape\n");
#if 0
				DebugTraceMemory(&m_pi16CalibBuffer[i32OffsetAdjust], 48);
#endif
#if !DBG
				CsEventLog EventLog;
				EventLog.Report( CS_ADC_ALIGN_ERROR, szErrorInfo );
#endif	
				sprintf_s( szTrace, sizeof(szTrace), "! ! ! Channel skew Calib error. %s\n", szErrorInfo);
				::OutputDebugString(szTrace);
				i32RetDualCal = CS_ADCALIGN_CALIB_FAILURE;
				goto safeExit;
			}
		}
	}

	if ( CS_SUCCEEDED(i32RetDualCal) )
	{
		//					-------------------
		//				   /
		//				  /
		//				 /
		//		  /\	/
		//		 /	 \/
		//		/
		//		
		// Because this shape of the pulse, sometimes, the trigger position detected aboved may not be correct. We may have up to +/- 2 samples error
		// We have to do this extra step to improved edge detection algorithm (reduced to +/- 1 samples error)
		int16* pi16Buf = m_pi16CalibBuffer + i32OffsetAdjust + u32ChanPosCross2 - IMPROVED_EDGE_COMPARE_LENGTH/2;
		uInt32 u32MinDiff = Abs(i16LevelCross2 - i16LevelCross1);
	
		// From channel 2, lookking for a sample which has level closest with the one from channel 1
		for (int i = 0; i  < IMPROVED_EDGE_COMPARE_LENGTH;  i++ )
		{
			uInt32	u32Diff = Abs(pi16Buf[i] - i16LevelCross1);
			if ( u32Diff < u32MinDiff )
			{
				u32MinDiff = u32Diff;
				u32ChanPosCross2 = u32ChanPosCross2 - IMPROVED_EDGE_COMPARE_LENGTH/2 + i;
			}	
		}
		
		if ( 0 != (m_u32DebugCalibTrace & TRACE_CAL_ADCALIGN ) )
		{
			char szTrace[256];
			sprintf_s( szTrace, sizeof(szTrace), "(Card %d) DelayAdc before alignment dual channel (0 1 2 3) = %d, %d, %d, %d\n", m_pCardState->u16CardNumber, m_i16DelayAdc[0], m_i16DelayAdc[1], m_i16DelayAdc[2], m_i16DelayAdc[3]);
			::OutputDebugString(szTrace);
			sprintf_s( szTrace, sizeof(szTrace), "          Dual channel skew (Channel2 (%d)  vs Channel1 (%d) ) = %d\n", u32ChanPosCross2, u32ChanPosCross1, (u32ChanPosCross2 - u32ChanPosCross1));
			::OutputDebugString(szTrace);
		}

		i32RetDualCal = AlignDualChannelAdc(u32ChanPosCross1, u32ChanPosCross2);
		m_pCardState->bDualChanAligned = CS_SUCCEEDED(i32RetDualCal);
	}

safeExit:
	SendCalibModeSetting( CS_CHAN_1, ecmCalModeNormal, 0 ); // With default waiting time only on the last card
	SendCalibModeSetting( CS_CHAN_2, ecmCalModeNormal ); // With default waiting time only on the last card

	// Disable trigger from this card
	i32Ret = SendTriggerEngineSetting( 0 );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	if ( CS_FAILED(i32RetDualCal) )
		return i32RetDualCal;
	else
		return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::AlignDualChannelAdc(uInt32 u32Pos1, uInt32 u32Pos2)
{
	if ( m_pCardState->bDualChanAligned ||  ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) == 0 )
		return CS_SUCCESS;

	int32 i32Ret = CS_SUCCESS;
	if ( u32Pos2 != u32Pos1 )
	{
		int16 i16ChanDelay;

		// The DEERE_ADC_ALIGN_WR_CMD register does not allow to shift data from one ADC to left
		// In order to align both channels, we have to move data from one or more ADC to the right
		if ( 1 == m_pCardState->u8NumOfAdc )
		{
			if ( u32Pos2 > u32Pos1 ) // Channel 2 (ADC 270) is shifted right versus Channel 1 (ADC 180)
			{
				i16ChanDelay = (int16) (u32Pos2-u32Pos1);
				// Align 2 channels
				if ( m_i16DelayAdc[3] > i16ChanDelay )
					m_i16DelayAdc[3] -= i16ChanDelay;	// Move Channel 2 (ADC 270) to the left
				else
					m_i16DelayAdc[2] += i16ChanDelay;	// Move Channel 1 (ADC 180) to the right
			}
			else					// Channel 2 (ADC 270) is shifted left versus Channel 1 (ADC 180)
			{
				i16ChanDelay = (int16) (u32Pos1-u32Pos2);
				// Align 2 channels
				if ( m_i16DelayAdc[2] > i16ChanDelay )
					m_i16DelayAdc[2] -= i16ChanDelay;	// Move Channel 1 (ADC 180) to the left
				else
					m_i16DelayAdc[3] += i16ChanDelay;	// Move Channel 2 (ADC 270) to the right
			}
		}
		else
		{
			// We can only adjust the delay by multiple of 2 samples. That's why i16ChanDelay = (int16) (u32Pos2-u32Pos1)/2;
			if ( u32Pos2 > u32Pos1 ) // Channel 2 (ADC 90-270) is shifted right versus Channel 1 (ADC 0-180)
			{
				i16ChanDelay = (int16) (u32Pos2-u32Pos1)/2;

				// Align 2 channels
				if ( m_i16DelayAdc[1] >= i16ChanDelay && m_i16DelayAdc[3] >= i16ChanDelay )
				{
					// Move Channel 2 (ADC 90-270) to the left
					m_i16DelayAdc[1] -= i16ChanDelay;		// ADC 90
					m_i16DelayAdc[3] -= i16ChanDelay;		// ADC 270
				}
				else
				{
					// Move Channel 1 (ADC 0-180) to the right
					m_i16DelayAdc[0] += i16ChanDelay;		// ADC 0
					m_i16DelayAdc[2] += i16ChanDelay;		// ADC 180
				}
			}
			else					// Channel 2 (ADC 90-270) is shifted left versus Channel 1 (ADC 0-180)
			{
				i16ChanDelay = (int16) (u32Pos1-u32Pos2)/2;

				// Align 2 channels
				if ( m_i16DelayAdc[0] >= i16ChanDelay && m_i16DelayAdc[2] >= i16ChanDelay )
				{
					// Move Channel 1 (ADC 0-180) to the left
					m_i16DelayAdc[0] -= i16ChanDelay;		// ADC 0
					m_i16DelayAdc[2] -= i16ChanDelay;		// ADC 180
				}
				else
				{
					// Move Channel 2 (ADC 90-270) to the right
					m_i16DelayAdc[1] += i16ChanDelay;		// ADC 90
					m_i16DelayAdc[3] += i16ChanDelay;		// ADC 270
				}
			}
		}

		i32Ret = SendAdcAlign( ecmCalModeDual );

	}

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::SetDefaultDacCalib( uInt16 u16HwChannel )
{
	int32	i32Ret = CS_SUCCESS;

	// Set all DACs to the default initial value
	uInt16 u16DacAddr = CS_CHAN_1 == u16HwChannel ? DEERE_OFFSET_NULL_1 : DEERE_OFFSET_NULL_2;
	i32Ret = WriteToDac( u16DacAddr + DEERE_OFFSET_NULL_1, c_u16CalDacCount/2 );
	if( CS_FAILED(i32Ret) )	{TraceCalib( TRACE_CAL_RESULT, u16HwChannel, 4, i32Ret); SendCalibModeSetting( u16HwChannel ); return i32Ret;}
	i32Ret = WriteToDac( u16DacAddr + DEERE_OFFSET_COMP_1, c_u16CalDacCount/2 );
	if( CS_FAILED(i32Ret) )	{TraceCalib( TRACE_CAL_RESULT, u16HwChannel, 4, i32Ret); SendCalibModeSetting( u16HwChannel ); return i32Ret;}
	i32Ret = WriteToDac( u16DacAddr + DEERE_USER_OFFSET_1, c_u16CalDacCount/2 );
	if( CS_FAILED(i32Ret) )	{TraceCalib( TRACE_CAL_RESULT, u16HwChannel, 4, i32Ret); SendCalibModeSetting( u16HwChannel ); return i32Ret;}
	
	i32Ret = WriteToAdcRegAllChannelCores(u16HwChannel, DEERE_ADC_COARSE_OFFSET, c_u8AdcCalRegSpan/2 );
	if( CS_FAILED(i32Ret) )	{TraceCalib( TRACE_CAL_RESULT, u16HwChannel, 5, i32Ret); SendCalibModeSetting( u16HwChannel ); return i32Ret;}
	i32Ret = WriteToAdcRegAllChannelCores(u16HwChannel, DEERE_ADC_FINE_OFFSET, c_u8AdcCalRegSpan/2 );
	if( CS_FAILED(i32Ret) )	{TraceCalib( TRACE_CAL_RESULT, u16HwChannel, 5, i32Ret); SendCalibModeSetting( u16HwChannel ); return i32Ret;}
	
	i32Ret = WriteToAdcRegAllChannelCores(u16HwChannel, DEERE_ADC_COARSE_GAIN, c_u8AdcCoarseGainSpan/2 );
	if( CS_FAILED(i32Ret) )	{TraceCalib( TRACE_CAL_RESULT, u16HwChannel, 5, i32Ret); SendCalibModeSetting( u16HwChannel ); return i32Ret;}
	i32Ret = WriteToAdcRegAllChannelCores(u16HwChannel, DEERE_ADC_MEDIUM_GAIN, c_u8AdcCalRegSpan/2 );
	if( CS_FAILED(i32Ret) )	{TraceCalib( TRACE_CAL_RESULT, u16HwChannel, 5, i32Ret); SendCalibModeSetting( u16HwChannel ); return i32Ret;}
	i32Ret = WriteToAdcRegAllChannelCores(u16HwChannel, DEERE_ADC_FINE_GAIN, c_u8AdcCalRegSpan/2 );
	if( CS_FAILED(i32Ret) )	{TraceCalib( TRACE_CAL_RESULT, u16HwChannel, 5, i32Ret); SendCalibModeSetting( u16HwChannel ); return i32Ret;}

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::SendAdcAlign( eCalMode eMode )
{
	// Value of DelayAdc should be normalized (0-7)
	int16	i16Min = m_i16DelayAdc[0];
	for( uInt8 u8 = 0; u8 < DEERE_ADC_COUNT; u8 ++ )
		i16Min = Min( i16Min, m_i16DelayAdc[u8] );

	// convert to positive value (0-7)

	int16 i16NewDelayAdc[DEERE_ADC_COUNT] = {0};
	for (int i = 0; i < DEERE_ADC_COUNT; i ++)
	{
		i16NewDelayAdc[i] = m_i16DelayAdc[i] - i16Min;
		if ( i16NewDelayAdc[i] > 7 )
		{
			char szErrorText[128];

			if ( ecmCalModeDual == eMode )
				sprintf_s( szErrorText, sizeof(szErrorText), "(Adc%d, ModeCalibDual, Val=%d)\n", i*90, i16NewDelayAdc[i]);
			else 
				sprintf_s( szErrorText, sizeof(szErrorText), "(Adc%d, ModeCalibAdc, Val=%d)\n", i*90, i16NewDelayAdc[i]);

			::OutputDebugString(" ! ! ! ERROR: ADC Alignment out of range. \n");
			::OutputDebugString( szErrorText );
#if !DBG
			CsEventLog EventLog;
			EventLog.Report( CS_ADC_ALIGN_OUTOFRANGE, szErrorText );
#endif
			return CS_ADCALIGN_CALIB_FAILURE;
		}
		else
			m_i16DelayAdc[i] = i16NewDelayAdc[i];
	}

	uInt16 u16AdcAlign = m_i16DelayAdc[3] & 0x7; // 270	
	u16AdcAlign <<= 3;
	u16AdcAlign |= m_i16DelayAdc[2] & 0x7;		 // 180
	u16AdcAlign <<= 3;
	u16AdcAlign |= m_i16DelayAdc[1] & 0x7;		// 90
	u16AdcAlign <<= 3;
	u16AdcAlign |= m_i16DelayAdc[0] & 0x7;		// 0
	
	for( uInt16	i = 1; i <= m_pCsBoard->NbAnalogChannel; i++ )
		m_bUpdateCoreOrder[i-1] = TRUE;

	TraceCalib( TRACE_CAL_ADCALIGN | TRACE_VERB, m_i16DelayAdc[0],	m_i16DelayAdc[1],m_i16DelayAdc[2], m_i16DelayAdc[3] );
	return WriteRegisterThruNios( u16AdcAlign, DEERE_ADC_ALIGN_WR_CMD );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::	InCalibrationCalibrate( uInt16 Channel )
{
	int i32Ret = CS_SUCCESS;

	// Improved calibration time by skipping front end calibration
	if( !FastFeCalibrationSettings(Channel, true ) )
		i32Ret = CalibrateFrontEnd(Channel);
	if( CS_FAILED(i32Ret) )	
	{
		char szErrorStr[128];
		char szTrace[256];
		sprintf_s( szErrorStr, sizeof(szErrorStr), "%d, Channel 1 reason %d", i32Ret, i32Ret%CS_ERROR_FORMAT_THRESHOLD );
		if( 0 != (m_u32IgnoreCalibError & 0x01) )
		{
			i32Ret = SetDefaultDacCalib(Channel);
			sprintf_s(  szTrace, sizeof(szTrace), "*!*!*   Ignoring calibration error: %s. Using default DAC values.\n", szErrorStr);
			::OutputDebugString(szTrace);
		}
		else
		{
			sprintf_s( szTrace, sizeof(szTrace),  " ! ! ! Calibration error: %s\n", szErrorStr);
			::OutputDebugString(szTrace);
#if !DBG
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_CALIBRATION_LOG, szErrorStr );
#endif			
		}
	}

	return i32Ret;
}


//-----------------------------------------------------------------------------
// Update core order (they have been changed because of M/S or Ext trigger calibration )
//-----------------------------------------------------------------------------
void CsDeereDevice::UpdateCoreOrder( uInt16 u16HwChannel, int16 i16DigitalDelay )
{
	uInt8	Temp[DEERE_ADC_CORES*DEERE_ADC_COUNT] = {0};
	uInt8	NewOrder[DEERE_ADC_CORES*DEERE_ADC_COUNT] = {0};
	uInt8	u8Idx = 0;

	if ( CS_CHAN_2 == u16HwChannel && 1 == m_pCsBoard->NbAnalogChannel )
		return;
 
	// Adc cores to be shifted in samples
	int16 i16Shift = i16DigitalDelay*m_pCardState->u8NumOfAdc/DEERE_ADC_COUNT;
	i16Shift = i16Shift%(DEERE_ADC_CORES*m_pCardState->u8NumOfAdc);

	if ( 0 == i16Shift )
		return;

	if ( i16Shift < 0 )
		i16Shift += DEERE_ADC_CORES*m_pCardState->u8NumOfAdc;

	for( uInt8 u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
	{
		for( uInt8 u8 = 0; u8 < m_pCardState->u8NumOfAdc; u8++ )
		{
			uInt8 u8Adc = AdcForChannel(u16HwChannel-1, u8 );
			Temp[u8Idx++] = m_u8AdcIndex[u8Adc][u8Core];
		}
	}

	for( u8Idx = 0; u8Idx < (DEERE_ADC_CORES*m_pCardState->u8NumOfAdc); u8Idx++ )
	{
		NewOrder[u8Idx] = Temp[i16Shift];
		i16Shift ++;
		i16Shift = i16Shift%(DEERE_ADC_CORES*m_pCardState->u8NumOfAdc);
	}

	u8Idx = 0;
	for( uInt8 u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
	{
		for( uInt8 u8 = 0; u8 < m_pCardState->u8NumOfAdc; u8++ )
		{
			uInt8 u8Adc = AdcForChannel(u16HwChannel-1, u8 );
			m_u8AdcIndex[u8Adc][u8Core] = NewOrder[u8Idx++];
		}
	}

	TraceCalib(TRACE_ADC_CORE_ORDER, u16HwChannel );
}




#if DBG

void	CsDeereDevice::DebugTraceMemory( int16 *pi16Buffer, uInt32 u32Size )
{
	char szTrace[256];

	szTrace[0] = 0;
	int j = 0;
	tCal.Trace(TraceInfo, "Memory dump (card %d).....\n", m_pCardState->u16CardNumber );
	for (uInt32 i = 0; i < u32Size;)
	{
		uInt16 u16Temp = (uInt16) pi16Buffer[i];
		sprintf(&szTrace[j], "%04x ", u16Temp);
		j+=5; i++;
		// 16 WORDS per line
		if ( i%16 == 0)
		{
			sprintf(&szTrace[j], "\n");
			tCal.Trace(TraceInfo, szTrace);
			j = 0;
		}
	}
}

#endif


