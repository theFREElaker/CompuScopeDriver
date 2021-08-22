
#include "StdAfx.h"
#include "CsDeere.h"

#define DEERE_CORE_CALIB_SIZE		0x2000
#define DEERE_ADC_CALIB_DEPTH   (DEERE_CORE_CALIB_SIZE*DEERE_ADC_COUNT + 0x100)
#define DEERE_CALIB_RANGE   CS_GAIN_1_V
#define DEERE_CALIB_IMPED   CS_REAL_IMP_50_OHM
#define DEERE_CALIB_LOOP_COUNT 10
#define DEERE_CALIB_MAX_ITER   16
#define DEERE_ADC_SKEWSPAN_LIMIT    4 //ps
#define	MHz				1000000
#define	GHz				(1000*MHz)

#define _HalfDacValue        2048
#define _PhaseDacSpan       (_HalfDacValue/8)
#define _HiPhaseDacLimit    (_HalfDacValue + _PhaseDacSpan)
#define _LoPhaseDacLimit    (_HalfDacValue - _PhaseDacSpan)
#define _MaxAdcRegValue      255
#define _AmpMatchTolerance   2.9  //lsb
#define _OffMatchTolerance   29.9  //lsb

#define _OffMatchSpanTolerance   0.06 // 6% FS
#define _AmpMatchSpanTolerance   0.02 // 2% Aplitude


bool  g_bCalibInSqr = true;

#ifdef __linux
#include <math.h>
#include <stdarg.h>

#ifdef __try
#undef __try
#endif
#define	__try
#define	__finally	Finally:
#define	__leave		{goto Finally;}

#define	fabs(x)		(((x) > 0) ? (x) : (-1*(x)))

#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDeereDevice::CommitNewConfiguration( PCSACQUISITIONCONFIG pAcqCfg,
											   	PARRAY_CHANNELCONFIG pAChanCfg,
                                                PARRAY_TRIGGERCONFIG pATrigCfg )
{
	int32					i32Status = CS_SUCCESS;
	CsDeereDevice			*pDevice = this;

	// Load appropriate FPGA image depending on the mode
	uInt8	u8LoadImageId = 0;
	if ( (pAcqCfg->u32Mode & CS_MODE_USER1) != 0 )
		u8LoadImageId = 1;
	else if ( (pAcqCfg->u32Mode & CS_MODE_USER2) != 0 )
		u8LoadImageId = 2;
	else
		u8LoadImageId = 0;

	pDevice = m_MasterIndependent;
	while (NULL != pDevice)
	{
		if ( m_pCardState->u8ImageId != u8LoadImageId )
		{
			i32Status = pDevice->CsLoadFpgaImage( u8LoadImageId );
			if( CS_FAILED(i32Status) )	return i32Status;
		}

		pDevice = pDevice->m_Next;
	}

	// Detect if square tec mode will be used
	m_bSquareRec = IsSquareRec( pAcqCfg->u32Mode, pAcqCfg->u32SegmentCount, (pAcqCfg->i64SegmentSize - pAcqCfg->i64Depth) ); 
	pDevice = m_MasterIndependent;
	while (NULL != pDevice)
	{
		pDevice->m_bSquareRec = m_bSquareRec;
		pDevice = pDevice->m_Next;
	}

	pDevice = m_MasterIndependent;
	while (NULL != pDevice)
	{
		i32Status = pDevice->CsSetAcquisitionConfig( pAcqCfg );
		if( CS_FAILED(i32Status) )
			return i32Status;

		pDevice = pDevice->m_Next;
	}

	i32Status = CsSetChannelConfig( (PARRAY_CHANNELCONFIG) pAChanCfg );
	if( CS_FAILED(i32Status) )return i32Status;

	i32Status = CsSetTriggerConfig( (PARRAY_TRIGGERCONFIG) pATrigCfg );
	if( CS_FAILED(i32Status) )return i32Status;


	int32 i32SystemResult(CS_SUCCESS);
	int32 i32LockResult(CS_SUCCESS);
	int32 i32Result(CS_SUCCESS);

	pDevice = m_MasterIndependent;
	while ( NULL != pDevice )
	{
		i32Result = pDevice->SendAdcSelfCal();
		if ( CS_FAILED(i32Result) )
			return i32Result;

		pDevice = pDevice->m_Next;
	}

	if ( CS_SUCCEEDED(i32SystemResult) )
	{
		i32Result = MsCalibrateAdcAlign();
		if ( CS_FAILED(i32Result)  )
			i32SystemResult = i32Result;
	}

	if ( CS_SUCCEEDED(i32SystemResult) )
	{
		i32Result = MsCalibrateSkew();
		if ( CS_FAILED(i32Result)  )
			i32SystemResult = i32Result;
	}

	pDevice = m_MasterIndependent;
	while ( NULL != pDevice )
	{
		i32Result = pDevice->CalibrateAllChannels();
		if( (pDevice == m_MasterIndependent) && !pDevice->IsOnBoardPllLocked() ) 
			i32LockResult = CS_CLK_NOT_LOCKED;

		if ( CS_FAILED(i32Result) && CS_SUCCEEDED(i32SystemResult) )
			i32SystemResult = i32Result;

		pDevice->ConfigureDebugCalSrc();

		pDevice = pDevice->m_Next;
	}

	int32 i32CoerceResult(CS_SUCCESS);
	pDevice = m_MasterIndependent;
	while ( NULL != pDevice )
	{
		pDevice->CheckCoerceResult(i32CoerceResult);
		pDevice->ConfigureDebugCalSrc();
		pDevice = pDevice->m_Next;
	}

	if ( CS_SUCCEEDED(i32SystemResult) )
		if( CS_FAILED(i32LockResult) )
			i32SystemResult = CS_FALSE;
		else 
			i32SystemResult = i32CoerceResult;

	pDevice = m_MasterIndependent;
	while ( NULL != pDevice )
	{
		if ( m_bMulrecAvgTD )
		{
			pDevice->SetupHwAverageMode(m_bHwAvg32);
			pDevice->CsSetDataFormat( formatHwAverage );
		}
		else if ( m_bSoftwareAveraging )
			pDevice->CsSetDataFormat( formatSoftwareAverage );
		else 
			pDevice->CsSetDataFormat();

		pDevice->ConfigureAlarmSource();

		pDevice->UpdateCardState();

		pDevice = pDevice->m_Next;
	}

	return i32SystemResult;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDeereDevice::MsMatchAdcPhaseAllChannels()
{
	CsDeereDevice *pDevice = m_MasterIndependent;

	CSACQUISITIONCONFIG CalibAcq;
	CsGetAcquisitionConfig( &m_OldAcqConfig );
	CalibAcq = m_OldAcqConfig;

	int64 i64Sr = 500*MHz;
	switch( m_pCsBoard->u32BoardType )
	{
		default: 
		case CS12500_BOARDTYPE: i64Sr = 500*MHz; break;
		case CS14250_BOARDTYPE: i64Sr = 250*MHz; break;
		case CS121G_BOARDTYPE:  i64Sr = 1*GHz;   break;
		case CS14500_BOARDTYPE: i64Sr = 500*MHz; break;
		case CS122G_BOARDTYPE:  i64Sr = 2*GHz;   break;
		case CS141G_BOARDTYPE:  i64Sr = 1*GHz;   break;
	}

	bool	bOldSqaureRec = m_bSquareRec;

	m_bSquareRec = g_bCalibInSqr;		  // Force circular record
	CalibAcq.i64TriggerTimeout = 0;

	if ( ! g_bCalibInSqr )
	{
		CalibAcq.i64Depth = DEERE_ADC_CALIB_DEPTH;
		CalibAcq.i64SegmentSize = DEERE_ADC_CALIB_DEPTH+DEERE_MAX_SQUARE_PRETRIGDEPTH+0x100;  // Force circular record to ignore etb
		CalibAcq.i64TriggerHoldoff = DEERE_MAX_SQUARE_PRETRIGDEPTH+0x100;
	}
	else
	{
		CalibAcq.i64Depth = DEERE_ADC_CALIB_DEPTH+0x100;
		CalibAcq.i64SegmentSize = CalibAcq.i64Depth;
		CalibAcq.i64TriggerHoldoff = 0;
	}

	CalibAcq.i64TriggerDelay = 0;
	CalibAcq.u32SegmentCount = 1;
	CalibAcq.i64SampleRate = i64Sr;
	CalibAcq.u32ExtClk = 0;

	int32 i32Status = CS_SUCCESS;
	int32 i32CalibStatus = CS_SUCCESS;
	pDevice = m_MasterIndependent;
	while( pDevice )
	{
		// Backup old trigger settings
		RtlCopyMemory( pDevice->m_OldTrigParams, pDevice->m_pCardState->TriggerParams, sizeof(m_pCardState->TriggerParams));

		pDevice->m_bCalibActive = true;

		i32Status = pDevice->CsSetAcquisitionConfig(&CalibAcq);
		if( CS_FAILED(i32Status) )
			return i32Status;

		// Disable trigger
		i32Status = pDevice->SendTriggerEngineSetting(0);
		if( CS_FAILED(i32Status) )
			return i32Status;

		i32Status = pDevice->SendExtTriggerSetting(0);
		if( CS_FAILED(i32Status) )
			return i32Status;

		pDevice = pDevice->m_Next;
	}

	pDevice = m_MasterIndependent;
	while( pDevice )
	{
		// If we have changed the clock setting. It may require to run Dpa again
		i32Status = pDevice->SendAdcSelfCal();

		for( uInt16 i = 0; i < pDevice->m_pCsBoard->NbAnalogChannel; i++ )
		{
			bool	bAdcMatch = pDevice->m_bAdcMatchNeeded[i];
			bool	bCoreMatch = pDevice->m_bCoreMatchNeeded[i];
			
			bCoreMatch = bCoreMatch && isAdcPingPong() && !m_bNeverMatchAdcPhase;

			if ( bAdcMatch || bCoreMatch )
			{
				// Backup old front end settings
				RtlCopyMemory( pDevice->m_OldChanParams, pDevice->m_pCardState->ChannelParams, sizeof(m_pCardState->ChannelParams));

				// Change into AC coupling
				i32Status = pDevice->SendChannelSetting( i+1, pDevice->m_pCardState->ChannelParams[i].u32InputRange,
														 CS_COUPLING_AC,
														 pDevice->m_pCardState->ChannelParams[i].u32Impedance,
														 pDevice->m_pCardState->ChannelParams[i].i32Position,
														 pDevice->m_pCardState->ChannelParams[i].u32Filter );

				if ( CS_FAILED(i32Status) )
					return i32Status;

				uInt32 u32CalFreq= 0;
				bool bUseBuildInCal = true;

#ifdef _WINDOWS_
				if( m_bUseUserCalSignal )
				{
					bool QuerryUserCalFrequency(uInt32& u32CalFreq);
					bUseBuildInCal = !QuerryUserCalFrequency(u32CalFreq);
				}
#endif

				if( bUseBuildInCal )
				{
					i32Status = pDevice->SendCalibModeSetting( i+1, ecmCalModeAc );
					if ( CS_FAILED(i32Status) )
						return i32Status;
				}

				double dFreq = pDevice->m_u32CalFreq;
				// Do actual matching
				if ( bCoreMatch )
				{
					i32CalibStatus = pDevice->MatchAllAdcCores( i+1, double(CalibAcq.i64SampleRate), dFreq );
					if ( CS_FAILED( i32CalibStatus ) ) goto FailedExit;
				}

				if ( bAdcMatch )
				{
					i32CalibStatus = pDevice->RematchGainAndOffset( i+1, double(CalibAcq.i64SampleRate), dFreq );
					if ( CS_FAILED( i32CalibStatus ) )goto FailedExit;
				}

				i32CalibStatus = pDevice->VerifyCalibration( i+1, double(CalibAcq.i64SampleRate), dFreq );
				if ( CS_FAILED( i32CalibStatus ) )goto FailedExit;

				// Clear calibration required flag on success
				pDevice->m_bCoreMatchNeeded[i] = false;
				pDevice->m_bAdcMatchNeeded[i] = false;

FailedExit:
				i32Status = pDevice->SendCalibModeSetting( i+1, ecmCalModeNormal );

				// Restore coupling
				i32Status = pDevice->SendChannelSetting( i+1, pDevice->m_OldChanParams[i].u32InputRange,
														 pDevice->m_OldChanParams[i].u32Term,
														 pDevice->m_OldChanParams[i].u32Impedance,
														 pDevice->m_OldChanParams[i].i32Position,
														 pDevice->m_OldChanParams[i].u32Filter );
			}

			if( 0 != (m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE) )
				break;
		}
		pDevice = pDevice->m_Next;
	}

	// Restore all user settings
	m_bSquareRec = bOldSqaureRec;
	pDevice = m_MasterIndependent;
	while( pDevice )
	{
		// Restore acquisition config
		pDevice->m_bCalibActive = false;

		i32Status = pDevice->CsSetAcquisitionConfig(&m_OldAcqConfig);
		if( CS_FAILED(i32Status) )
			return i32Status;

		// Restore trigger settings
		i32Status = pDevice->SendExtTriggerSetting((pDevice->m_OldTrigParams[INDEX_TRIGENGINE_EXT].i32Source != 0),
								pDevice->m_OldTrigParams[INDEX_TRIGENGINE_EXT].i32Level,
								pDevice->m_OldTrigParams[INDEX_TRIGENGINE_EXT].u32Condition,
								pDevice->m_OldTrigParams[INDEX_TRIGENGINE_EXT].u32ExtTriggerRange, 
								pDevice->m_OldTrigParams[INDEX_TRIGENGINE_EXT].u32ExtImpedance,
								pDevice->m_OldTrigParams[INDEX_TRIGENGINE_EXT].u32ExtCoupling );
		if( CS_FAILED(i32Status) )
			return i32Status;

		i32Status = pDevice->SendTriggerEngineSetting( pDevice->m_OldTrigParams[INDEX_TRIGENGINE_A1].i32Source,
													   pDevice->m_OldTrigParams[INDEX_TRIGENGINE_A1].u32Condition,
                                                       pDevice->m_OldTrigParams[INDEX_TRIGENGINE_A1].i32Level,
                                                       pDevice->m_OldTrigParams[INDEX_TRIGENGINE_B1].i32Source,
                                                       pDevice->m_OldTrigParams[INDEX_TRIGENGINE_B1].u32Condition,
                                                       pDevice->m_OldTrigParams[INDEX_TRIGENGINE_B1].i32Level,
													   pDevice->m_OldTrigParams[INDEX_TRIGENGINE_A2].i32Source,
													   pDevice->m_OldTrigParams[INDEX_TRIGENGINE_A2].u32Condition,
                                                       pDevice->m_OldTrigParams[INDEX_TRIGENGINE_A2].i32Level,
                                                       pDevice->m_OldTrigParams[INDEX_TRIGENGINE_B2].i32Source,
                                                       pDevice->m_OldTrigParams[INDEX_TRIGENGINE_B2].u32Condition,
                                                       pDevice->m_OldTrigParams[INDEX_TRIGENGINE_B2].i32Level );
		if( CS_FAILED(i32Status) )
			return i32Status;

		pDevice = pDevice->m_Next;
	}

	
	if ( CS_FAILED( i32CalibStatus ) )
		return i32CalibStatus;
	else
		return i32Status;
}

#define DEERE_SINEFIT_SNR_LIMIT     30  //30 dB

int32 CsDeereDevice::MeasureCalResult( const uInt16 u16ChanIndex, const double dSr, const double dFreq, void* pBuffer, SINEFIT3* pSol, double* pdOff, double* pdAmp, double* pdPh, double* pdSnr)
{
	if( NULL == pBuffer) { return CS_INVALID_CALIB_BUFFER;}
	if( NULL == pSol) { return CS_INVALID_CALIB_BUFFER;}
	if( NULL == pdOff) { return CS_INVALID_CALIB_BUFFER;}
	if( NULL == pdAmp) { return CS_INVALID_CALIB_BUFFER;}
	if( NULL == pdPh) { return CS_INVALID_CALIB_BUFFER;}
	
	if( ::IsBadWritePtr( pBuffer, DEERE_ADC_CALIB_DEPTH * m_pCardState->AcqConfig.u32SampleSize ) ) { return CS_INVALID_CALIB_BUFFER;}
	const uInt8 u8Cores = m_pCardState->u8NumOfAdc*DEERE_ADC_CORES;
	if( ::IsBadWritePtr( pSol, u8Cores * sizeof(SINEFIT3) ) ) { return CS_INVALID_CALIB_BUFFER;}
	if( ::IsBadWritePtr( pdOff, u8Cores * sizeof(double) ) ) { return CS_INVALID_CALIB_BUFFER;}
	else ::ZeroMemory( pdOff, u8Cores * sizeof(double) );
	if( ::IsBadWritePtr( pdAmp, u8Cores * sizeof(double) ) ) { return CS_INVALID_CALIB_BUFFER;}
	else ::ZeroMemory( pdAmp, u8Cores * sizeof(double) );
	if( ::IsBadWritePtr( pdPh,  u8Cores * sizeof(double) ) ) { return CS_INVALID_CALIB_BUFFER;}
	else ::ZeroMemory( pdPh, u8Cores * sizeof(double) );
	if( NULL != pdSnr) 
		if( ::IsBadWritePtr( pdSnr,  u8Cores * sizeof(double) ) )
		{
			return CS_INVALID_CALIB_BUFFER;
		}
		else
			::ZeroMemory( pdSnr, u8Cores * sizeof(double) );

	int32 i32Ret = CS_SUCCESS;
	uInt8 u8Core;
	int nFitCount = 0;
	int32 iSqrAdjust = 0;
	uInt32 TransferDataSize	= (DEERE_CORE_CALIB_SIZE+32)*m_pCardState->u8NumOfAdc;		// + 32 for Max Sqr Etb Adjustment
	uInt32 SineFitDataSize	= DEERE_CORE_CALIB_SIZE*m_pCardState->u8NumOfAdc;

	for( int i = 0; i < DEERE_CALIB_LOOP_COUNT; i++ )
	{
		i32Ret = AcquireAndRead(u16ChanIndex, pBuffer, TransferDataSize );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		// Get the Etb
		iSqrAdjust = GetSquareEtb();
		
		i32Ret = SplitSineFit3(dSr/(u8Cores*dFreq), pBuffer, iSqrAdjust, m_pCardState->AcqConfig.u32SampleSize, SineFitDataSize, u8Cores, pSol);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
		
		bool bGoodFit = true;
		for( u8Core = 0; u8Core < u8Cores; u8Core ++ )
			if( pSol[u8Core].dSnr < DEERE_SINEFIT_SNR_LIMIT )
			{
				bGoodFit = false;
#ifdef  _DEBUG
				::OutputDebugString("Error Sinefit!!!!\n");
				///MessageBox(0, "Error Sinefit!!!!", "Error", MB_OK);
#endif
				break;
			}

		if( !bGoodFit )		
		{
			TraceCalibEx( TRACE_CORE_SKEW_MEAS|TRACE_VERB, -1, u16ChanIndex, m_pCardState->u8NumOfAdc, &i, pSol, &dFreq);
			continue;
		}
		nFitCount++;

		for( u8Core = 0; u8Core < u8Cores; u8Core ++ )
		{
			double dPh = pSol[u8Core].dPhase - pSol[0].dPhase;
			if( dPh >  M_PI ) dPh -= 2*M_PI;
			if( dPh < -M_PI ) dPh += 2*M_PI;
			dPh = 1.e12*dPh/ ( 2.0 * M_PI * dFreq ); // convert to time in ps unnecessary but easier to debug
			pdPh[u8Core] += dPh;
			pdAmp[u8Core] += pSol[u8Core].dAmp;
			pdOff[u8Core] += pSol[u8Core].dOff;
			if( NULL != pdSnr ) pdSnr[u8Core] += pSol[u8Core].dSnr;
		}
		TraceCalibEx( TRACE_CORE_SKEW_MEAS|TRACE_VERB, 0, u16ChanIndex, m_pCardState->u8NumOfAdc, &i, pSol, &dFreq);
	}

	if( nFitCount > 0 )
	{
		for( u8Core = 0; u8Core < u8Cores; u8Core ++ )
		{
			pdPh[u8Core] /= nFitCount;
			pdAmp[u8Core] /= nFitCount;
			pdOff[u8Core] /= nFitCount;
			if( NULL != pdSnr ) pdSnr[u8Core] /= nFitCount;
		}
	}
	else
	{
		TraceCalibEx( TRACE_CORE_SKEW_MEAS, -1, u16ChanIndex, m_pCardState->u8NumOfAdc );
		return CS_ACCAL_NOT_LOCKED;
	}

	TraceCalibEx( TRACE_CORE_SKEW_MEAS, 0, u16ChanIndex, m_pCardState->u8NumOfAdc, pdPh, pdAmp, pdOff);
	return i32Ret;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDeereDevice::MatchAllAdcCores( const uInt16 u16ChanIndex, const double dSr,  const double dFreq  )
{
	int32	i32Ret = CS_SUCCESS;
	const uInt8	u8AdcPerChan = m_pCardState->u8NumOfAdc;
	const uInt16 u16SysChanIndex = u16ChanIndex + ((m_pCardState->u16CardNumber-1) * m_pCsBoard->NbAnalogChannel );


	TraceCalibEx( TRACE_CORE_SKEW_MATCH, 0, u16SysChanIndex, u8AdcPerChan);
	void* pBuffer = ::GageAllocateMemoryX(DEERE_ADC_CALIB_DEPTH * m_pCardState->AcqConfig.u32SampleSize);
	if( NULL == pBuffer) { return CS_INVALID_CALIB_BUFFER;}
	
	const uInt8 u8Cores = m_pCardState->u8NumOfAdc*DEERE_ADC_CORES;
	double* pdPh  = new double[u8Cores];
	double* pdAmp = new double[u8Cores];
	double* pdOff = new double[u8Cores];
	SINEFIT3* pSol = new SINEFIT3[u8Cores];

	uInt8 u8Skew[DEERE_ADC_COUNT] = {0};
	uInt8 u8Iter[DEERE_ADC_COUNT] = {0};
	double adSkewDelta[DEERE_ADC_COUNT]={0};

	__try
	{
		TraceCalibEx( TRACE_CAL_PROGRESS, 10, u16SysChanIndex, u8AdcPerChan);
		//Determine Scales
		uInt8 u8AdcIx;
		for( u8AdcIx = 0; u8AdcIx < u8AdcPerChan; u8AdcIx++ )
			if( CS_FAILED(i32Ret = SendCoreSkewAdjust( AdcForChannel( u16ChanIndex-1, u8AdcIx ), u8Skew[u8AdcIx] )) )
				__leave;
		double adScale[DEERE_ADC_COUNT]={0};
		if( CS_FAILED( i32Ret = MeasureCalResult( u16ChanIndex, dSr, dFreq, pBuffer, pSol, pdOff, pdAmp, pdPh) ) )
			__leave;
		for( u8AdcIx = 0; u8AdcIx < u8AdcPerChan; u8AdcIx++ )
			adScale[u8AdcIx] = pdPh[u8AdcIx] - pdPh[u8AdcIx + u8AdcPerChan];

  		for( u8AdcIx = 0; u8AdcIx < u8AdcPerChan; u8AdcIx++ )
			if( CS_FAILED(i32Ret = SendCoreSkewAdjust( AdcForChannel( u16ChanIndex-1, u8AdcIx ), u8Skew[u8AdcIx] = _MaxAdcRegValue )) )
				__leave;

		if( CS_FAILED( i32Ret = MeasureCalResult( u16ChanIndex, dSr, dFreq, pBuffer, pSol, pdOff, pdAmp, pdPh) ) )
			__leave;
		for( u8AdcIx = 0; u8AdcIx < u8AdcPerChan; u8AdcIx++ )
			adSkewDelta[u8AdcIx] = pdPh[u8AdcIx] - pdPh[u8AdcIx + u8AdcPerChan];

		TraceCalibEx( TRACE_CORE_SKEW_MATCH, 1, u16ChanIndex, u8AdcPerChan, adScale, adSkewDelta); 
		for( u8AdcIx = 0; u8AdcIx < u8AdcPerChan; u8AdcIx++ )
		{
			if( fabs(adSkewDelta[u8AdcIx] - adScale[u8AdcIx]) < DEERE_ADC_SKEWSPAN_LIMIT )
			{
				i32Ret = CS_ADC_SKEW_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * u16ChanIndex;
				__leave;
			}
			adScale[u8AdcIx] = 255. /(adSkewDelta[u8AdcIx] - adScale[u8AdcIx]);
		}
		
		//Adjust skews
		bool bDoit = false;
		bool abDone[DEERE_ADC_COUNT], abFailed[DEERE_ADC_COUNT] = {0};
		do
		{
			if( CS_FAILED( i32Ret = MeasureCalResult( u16ChanIndex, dSr, dFreq, pBuffer, pSol, pdOff, pdAmp, pdPh) ) )
				__leave;
			for( u8AdcIx = 0; u8AdcIx < u8AdcPerChan; u8AdcIx++ )
				adSkewDelta[u8AdcIx] = pdPh[u8AdcIx] - pdPh[u8AdcIx + u8AdcPerChan];
			TraceCalibEx( TRACE_CORE_SKEW_MATCH|TRACE_VERB, 0, u16ChanIndex, u8AdcPerChan, u8Iter, adSkewDelta, u8Skew );

			bDoit = false;
			for( u8AdcIx = 0; u8AdcIx < u8AdcPerChan; u8AdcIx++ )
			{
				double dCor = -adSkewDelta[u8AdcIx]*adScale[u8AdcIx];
				dCor += dCor > 0. ? .5 : -.5;
				if( ++u8Iter[u8AdcIx] > DEERE_CALIB_MAX_ITER || fabs(dCor) < 1. )
					abDone[u8AdcIx] = true;
				else
				{
					abDone[u8AdcIx] = false;
					bDoit = true;
				}
		
				if( !abDone[u8AdcIx] )
				{
					int16 i16Skew = u8Skew[u8AdcIx];
					i16Skew +=  int16(dCor);
					if( 0 > i16Skew  )
					{
						if( abFailed[u8AdcIx] )
							__leave;
						i16Skew = 0;
						abFailed[u8AdcIx] = true;
					}
					else if( _MaxAdcRegValue < i16Skew  )
					{
						if( abFailed[u8AdcIx] )
							__leave;
						i16Skew = _MaxAdcRegValue;
						abFailed[u8AdcIx] = true;
					}
					else
						abFailed[u8AdcIx] = false;
					u8Skew[u8AdcIx] = uInt8(i16Skew);
					
					if( CS_FAILED(i32Ret = SendCoreSkewAdjust( AdcForChannel( u16ChanIndex-1, u8AdcIx ), u8Skew[u8AdcIx] )) )
						__leave;
				}
			}
		}while( bDoit );
	}
	__finally
	{
		delete[] pSol;
		delete[] pdPh; 
		delete[] pdAmp;
		delete[] pdOff;

		TraceCalibEx( TRACE_CORE_SKEW_MATCH, 10, u16ChanIndex, u8AdcPerChan, u8Iter, u8Skew, adSkewDelta );
		::VirtualFree( pBuffer, 0, MEM_RELEASE );
		uInt8 u8MaxIter = 0;
		for( uInt8 u8AdcIx = 0; u8AdcIx < u8AdcPerChan; u8AdcIx++ )
			u8MaxIter = max( u8MaxIter, u8Iter[u8AdcIx] );
		TraceCalibEx( TRACE_CAL_PROGRESS, 20, u16SysChanIndex, u8AdcPerChan, &u8MaxIter);
	}
	return i32Ret;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDeereDevice::DetermineCoreOrder( const uInt16 u16ChanIndex, const double dSr,  const double dFreq, void* pBuffer, SINEFIT3* pSol, double* pdOff, double* pdAmp, double* pdPh, uInt8 u8AdcIx[DEERE_ADC_COUNT][DEERE_ADC_CORES])
{
	int32 i32Ret = CS_SUCCESS;
	const uInt8 u8Cores = m_pCardState->u8NumOfAdc*DEERE_ADC_CORES;
	double adAmpOld[DEERE_ADC_COUNT*DEERE_ADC_CORES];

	for( uInt8 u8Adc = 0; u8Adc < m_pCardState->u8NumOfAdc; u8Adc++ )
	{
		for( uInt8 u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
		{
			if ( CS_FAILED(i32Ret = SendCoreGainOffsetAdjust(u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, true, false, 0 ) ) )
				return i32Ret;
		}
	}

	if( CS_FAILED( i32Ret = MeasureCalResult( u16ChanIndex, dSr, dFreq, pBuffer, pSol, pdOff, pdAmp, pdPh) ) )
		return i32Ret;
	GageCopyMemoryX( adAmpOld, pdAmp, sizeof(adAmpOld) );

	for( uInt8 u8Adc = 0; u8Adc < m_pCardState->u8NumOfAdc; u8Adc++ )
	{
		for( uInt8 u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
		{
			if ( CS_FAILED(i32Ret = SendCoreGainOffsetAdjust( u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, true, false, _MaxAdcRegValue ) ) )
				return i32Ret;
			if( CS_FAILED( i32Ret = MeasureCalResult( u16ChanIndex, dSr, dFreq, pBuffer, pSol, pdOff, pdAmp, pdPh) ) )
				return i32Ret;

			double dMax = 0.; uInt8 u8IxAdc = 0;
			for( uInt8 u8Ix = 0; u8Ix < u8Cores; u8Ix++ )
			{
				if( dMax < fabs( adAmpOld[u8Ix] - pdAmp[u8Ix]) )
				{
					u8IxAdc = u8Ix;
					dMax = fabs( adAmpOld[u8Ix] - pdAmp[u8Ix]);
				}
			}

			u8AdcIx[u8Adc][u8Core] = u8IxAdc;
			GageCopyMemoryX( adAmpOld, pdAmp, sizeof(adAmpOld) );
		}
	}

	TraceCalibEx(TRACE_ADC_CORE_ORDER,0, u16ChanIndex, m_pCardState->u8NumOfAdc, u8AdcIx );

	return i32Ret;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDeereDevice::RematchGainAndOffset( const uInt16 u16ChanIndex, const double dSr,  const double dFreq )
{
	if( m_pCardState->u8NumOfAdc <= 1 ) return CS_FALSE; //only in JD senior

	void* pBuffer = ::GageAllocateMemoryX( DEERE_ADC_CALIB_DEPTH * m_pCardState->AcqConfig.u32SampleSize );
	if( NULL == pBuffer) { return CS_INVALID_CALIB_BUFFER;}

	const uInt16 u16SysChanIndex = u16ChanIndex + ((m_pCardState->u16CardNumber-1) * m_pCsBoard->NbAnalogChannel );
	const uInt8 u8AdcPerChan = m_pCardState->u8NumOfAdc;
	const uInt8 u8Cores = m_pCardState->u8NumOfAdc*DEERE_ADC_CORES;
	double dOffTarget(0.), dAmpTarget(0.);
	double* pdPh  = new double[u8Cores];
	double* pdAmp = new double[u8Cores];
	double* pdOff = new double[u8Cores];
	double* pdPh_2 = new double[u8Cores];
	double* pdAmp_2 = new double[u8Cores];
	double* pdOff_2 = new double[u8Cores];
	SINEFIT3* pSol = new SINEFIT3[u8Cores];
	int32 i32Ret = CS_SUCCESS;

	uInt16 u16PhDac[DEERE_ADC_COUNT];
	bool bDoOff(true), bDoAmp(true);
	uInt16 u16OldOffCoarse[DEERE_ADC_COUNT][DEERE_ADC_CORES], u16OldOffFine[DEERE_ADC_COUNT][DEERE_ADC_CORES];
	uInt16 u16OldGainMed[DEERE_ADC_COUNT][DEERE_ADC_CORES], u16OldGainFine[DEERE_ADC_COUNT][DEERE_ADC_CORES];
	bool bRestoreOffCoarse(false), bRestoreOffFine(false), bRestoreGainCoarse(false), bRestoreGainFine(false);
	uInt8 u8Adc, u8Core;
	
	uInt8 u8AdcIx [DEERE_ADC_COUNT][DEERE_ADC_CORES];
	for( u8Adc = 0; u8Adc < u8AdcPerChan; u8Adc++ ) 
	{
		uInt8 u8Ix = AdcForChannel(u16ChanIndex-1, u8Adc );
		for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
			u8AdcIx[u8Adc][u8Core] = m_u8AdcIndex[u8Ix][u8Core];
	}

	__try
	{
		int32 i32Err = CS_ERROR_FORMAT_THRESHOLD * u16ChanIndex;

		//Use kernel mode result as target this the only absolute reference we have
		//However target may be dedefined later to ensure succesful calibration. The absolute accuracy would be then overriden by adc matching.
		TraceCalibEx( TRACE_CAL_PROGRESS, 30, u16SysChanIndex );
		if( CS_FAILED( i32Ret = MeasureCalResult( u16ChanIndex, dSr, dFreq, pBuffer, pSol, pdOff, pdAmp, pdPh) ) )
			__leave;
		uInt8 u8Core;
		for( u8Core = 0; u8Core < u8Cores; u8Core ++ )
		{
			dAmpTarget += pdAmp[u8Core];
			dOffTarget += pdOff[u8Core];
		}
		dAmpTarget /= u8Cores;
		dOffTarget /= u8Cores;

		//Store results of kernel DC cal
		for( u8Adc = 0; u8Adc < u8AdcPerChan; u8Adc++ )
		{
			for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
			{
				if( CS_FAILED(i32Ret=ReadCoreGainOffsetAdjust(u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, false, false, &u16OldOffCoarse[u8Adc][u8Core] ) ) )
					__leave;
				if( CS_FAILED(i32Ret=ReadCoreGainOffsetAdjust(u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, false, true,  &u16OldOffFine[u8Adc][u8Core]   ) ) )
					__leave;
				if( CS_FAILED(i32Ret=ReadCoreGainOffsetAdjust(u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, true,  false, &u16OldGainMed[u8Adc][u8Core]   ) ) )
					__leave;
				if( CS_FAILED(i32Ret=ReadCoreGainOffsetAdjust(u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, true,  true,  &u16OldGainFine[u8Adc][u8Core]  ) ) )
					__leave;
			}
		}
		bRestoreOffCoarse = bRestoreOffFine = bRestoreGainCoarse = bRestoreGainFine = true;

		//Put controls in the middle
		for( u8Adc = 0; u8Adc < u8AdcPerChan; u8Adc++ )
		{
			if( CS_FAILED(i32Ret=SendAdcPhaseAdjust( AdcForChannel( u16ChanIndex-1, u8Adc ), u16PhDac[u8Adc] = _HalfDacValue ) ) )
				__leave;
			for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
			{
				if ( bDoAmp && CS_FAILED(i32Ret = SendCoreGainOffsetAdjust( u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, true, true, _MaxAdcRegValue/2 ) ) )
					__leave;
				if ( bDoOff && CS_FAILED(i32Ret = SendCoreGainOffsetAdjust( u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, false, true, _MaxAdcRegValue/2 ) ) )
					__leave;
			}
		}

//		if( CS_FAILED( i32Ret = DetermineCoreOrder( u16ChanIndex, dSr, dFreq, pBuffer, pSol, pdOff, pdAmp, pdPh, u8AdcIx) ) )
//			__leave;

		uInt8 u8Adc0 = 0;
		for( u8Adc = 0; u8Adc < u8AdcPerChan; u8Adc++ ) 
		{
			for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
			{
				if( 0 == u8AdcIx[u8Adc][u8Core] )
					u8Adc0 = u8Adc;
			}
		}
			
		// Move first ADC0 so the rest are clustered around it
		double dSum1(0.), dSum2(0.);
		if( CS_FAILED(i32Ret=SendAdcPhaseAdjust(AdcForChannel( u16ChanIndex-1, u8Adc0 ), _LoPhaseDacLimit ) ) )
			__leave;
		if( CS_FAILED( i32Ret = MeasureCalResult( u16ChanIndex, dSr, dFreq, pBuffer, pSol, pdOff_2, pdAmp_2, pdPh_2) ) )
			__leave;
		for( u8Adc = 0; u8Adc < u8AdcPerChan; u8Adc++ )
			dSum1 += pdPh_2[u8Adc];

		if( CS_FAILED(i32Ret=SendAdcPhaseAdjust(AdcForChannel( u16ChanIndex-1, u8Adc0 ), _HiPhaseDacLimit ) ) )
			__leave;
		if( CS_FAILED( i32Ret = MeasureCalResult( u16ChanIndex, dSr, dFreq, pBuffer, pSol, pdOff_2, pdAmp_2, pdPh) ) )
			__leave;
		for( u8Adc = 0; u8Adc < u8AdcPerChan; u8Adc++ )
			dSum2 += pdPh[u8Adc];
		
		if( dSum2 != dSum1 )
		{
			u16PhDac[u8Adc0] = uInt16((_LoPhaseDacLimit*dSum2 - _HiPhaseDacLimit*dSum1)/(dSum2 - dSum1) + 0.5 );
			u16PhDac[u8Adc0] = Min( Max( u16PhDac[0], _LoPhaseDacLimit), _HiPhaseDacLimit );
			if( CS_FAILED(i32Ret=SendAdcPhaseAdjust(AdcForChannel( u16ChanIndex-1, u8Adc0 ), u16PhDac[u8Adc0] ) ) )
				__leave;
		}
		else
		{
			i32Ret = i32Err + CS_ADC_PHASE_CAL_FAILED;
			__leave;
		}

		TraceCalibEx( TRACE_ADC_PHASE_MATCH, 1, u16SysChanIndex, u8AdcPerChan, u16PhDac, pdPh_2, pdPh, &u8Adc0);
		
		//Main matching loop.
		//Done twice for medioum and fine controls.
		bool bPhDone[DEERE_ADC_COUNT] = {0};
		for( int nLoop = 0; nLoop < 2; nLoop++ )
		{
			bool bFine = 0 != nLoop;
			TraceCalibEx( TRACE_CAL_PROGRESS, bFine?70:50, u16SysChanIndex );
			double dOffScale[DEERE_ADC_COUNT][DEERE_ADC_CORES], dAmpScale[DEERE_ADC_COUNT][DEERE_ADC_CORES], dPhScale[DEERE_ADC_COUNT];
			
			//Determine Scales
			//set controls to the maximum
			for( u8Adc = 0; u8Adc < u8AdcPerChan; u8Adc++ )
			{
				if( u8Adc0 != u8Adc && !bPhDone[u8Adc])
					if( CS_FAILED(i32Ret=SendAdcPhaseAdjust(AdcForChannel( u16ChanIndex-1, u8Adc ), u16PhDac[u8Adc] = _LoPhaseDacLimit ) ) )
						__leave;
				for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
				{
					if ( bDoAmp && CS_FAILED(i32Ret = SendCoreGainOffsetAdjust(u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, true, bFine, _MaxAdcRegValue)  ) )
						__leave;
					if ( bDoOff && CS_FAILED(SendCoreGainOffsetAdjust(u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, false, bFine, _MaxAdcRegValue ) ) )
						__leave;
				}
			}

			//Mesure result
			if( CS_FAILED( i32Ret = MeasureCalResult( u16ChanIndex, dSr, dFreq, pBuffer, pSol, pdOff_2, pdAmp_2, pdPh_2) ) )
				__leave;

			//set controls to the minimum
			for( u8Adc = 0; u8Adc < u8AdcPerChan; u8Adc++ )
			{
				if( u8Adc0 != u8Adc && !bPhDone[u8Adc])
					if( CS_FAILED(i32Ret=SendAdcPhaseAdjust(AdcForChannel( u16ChanIndex-1, u8Adc ), u16PhDac[u8Adc] = _HiPhaseDacLimit ) ) )
						__leave;
				for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
				{
					if ( bDoAmp && CS_FAILED(i32Ret = SendCoreGainOffsetAdjust(u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, true, bFine, 0)  ) )
						__leave;
					if ( bDoOff && CS_FAILED(SendCoreGainOffsetAdjust(u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, false, bFine, 0 ) ) )
						__leave;
				}
			}
			//Mesure result
			if( CS_FAILED( i32Ret = MeasureCalResult( u16ChanIndex, dSr, dFreq, pBuffer, pSol, pdOff, pdAmp, pdPh) ) )
				__leave;

			//Verify that the span covers target values
			double dLoLimitOff = min( pdOff[u8AdcIx[0][0]], pdOff_2[u8AdcIx[0][0]] ); double dLoLimitAmp = min( pdAmp[u8AdcIx[0][0]], pdAmp_2[u8AdcIx[0][0]] );
			double dHiLimitOff = max( pdOff[u8AdcIx[0][0]], pdOff_2[u8AdcIx[0][0]] ); double dHiLimitAmp = max( pdAmp[u8AdcIx[0][0]], pdAmp_2[u8AdcIx[0][0]] );
			for( u8Adc = 0; u8Adc < u8AdcPerChan; u8Adc++ )
				for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
				{
					dLoLimitOff = Max( dLoLimitOff, min( pdOff[u8AdcIx[u8Adc][u8Core]], pdOff_2[u8AdcIx[u8Adc][u8Core]] ) );
					dHiLimitOff = Min( dHiLimitOff, max( pdOff[u8AdcIx[u8Adc][u8Core]], pdOff_2[u8AdcIx[u8Adc][u8Core]] ) );
					dLoLimitAmp = Max( dLoLimitAmp, min( pdAmp[u8AdcIx[u8Adc][u8Core]], pdAmp_2[u8AdcIx[u8Adc][u8Core]] ) );
					dHiLimitAmp = Min( dHiLimitAmp, max( pdAmp[u8AdcIx[u8Adc][u8Core]], pdAmp_2[u8AdcIx[u8Adc][u8Core]] ) );
				}	

			double dOffSpan = dHiLimitOff - dLoLimitOff;
			double dAmpSpan = dHiLimitAmp - dLoLimitAmp;
			// reduse span 5% to avoid ubmiguity due to measurment errors
			if( dOffSpan > 0.)
			{
				dLoLimitOff += dOffSpan*0.05;
				dHiLimitOff -= dOffSpan*0.05;
			}
			if( dAmpSpan > 0.)
			{
				dLoLimitAmp += dAmpSpan*0.05;	
				dHiLimitAmp -= dAmpSpan*0.05;
			}

			// Redefine target for the coase adjustment if needed
			if( !bFine )
			{
				if( dOffSpan < 0.) 	// Span of the controls do not overlap
				{
					if( 0 != (m_u32IgnoreTargetMismatch & (1<<u16SysChanIndex) ) )
					{ //If g_IgnoreTargetMismatch flag is set (default)  set the target in the middle of the dead zone. It will give chance to calibrate other parameres.
						TraceCalibEx( TRACE_CAL_PROGRESS, -104, u16SysChanIndex, u8AdcPerChan, &i32Ret, &dLoLimitOff, &dHiLimitOff );
						dOffTarget = (dLoLimitOff + dHiLimitOff)/2.;
					}
					else
					{
						i32Ret = i32Err + CS_COARSE_OFFSET_CAL_FAILED;
						TraceCalibEx( TRACE_CAL_PROGRESS, -102, u16SysChanIndex, u8AdcPerChan, &i32Ret, &dLoLimitOff, &dHiLimitOff );
						__leave;
					}
				}
				else if( dOffTarget != min( max( dOffTarget, dLoLimitOff), dHiLimitOff) )
				{
					double dNewTarget = min( max( dOffTarget, dLoLimitOff ), dHiLimitOff);
					TraceCalibEx( TRACE_CAL_PROGRESS, -100, u16SysChanIndex, u8AdcPerChan, &dOffTarget, &dNewTarget );
					dOffTarget = dNewTarget;
				}
				if( dAmpSpan < 0.) // Spans of the controls do not overlap
				{
					if( 0 != (m_u32IgnoreTargetMismatch & (1<<u16SysChanIndex) ) )
					{  //If g_IgnoreTargetMismatch flag is set (default)  set the target in the middle of the dead zone. It will give chance to calibrate other parameres.
						TraceCalibEx( TRACE_CAL_PROGRESS, -105, u16SysChanIndex, u8AdcPerChan, &i32Ret, &dLoLimitAmp, &dHiLimitAmp );
						dAmpTarget = (dLoLimitAmp + dHiLimitAmp)/2.;
					}
					else
					{
						i32Ret = i32Err + CS_GAIN_CAL_FAILED;
						TraceCalibEx( TRACE_CAL_PROGRESS, -103, u16SysChanIndex, u8AdcPerChan, &i32Ret, &dLoLimitAmp, &dHiLimitAmp );
						__leave;
					}
				}
				else if( dAmpTarget != min( max( dAmpTarget, dLoLimitAmp ), dHiLimitAmp) )
				{
					double dNewTarget = min( max( dAmpTarget, dLoLimitAmp ), dHiLimitAmp);
					TraceCalibEx( TRACE_CAL_PROGRESS, -101, u16SysChanIndex, u8AdcPerChan, &dAmpTarget, &dNewTarget );
					dAmpTarget = dNewTarget;
				}
				TraceCalibEx( TRACE_CAL_PROGRESS, 101, u16SysChanIndex, u8AdcPerChan, &dOffTarget, &dAmpTarget );
			}

			//Calculate the control responnse coefiisients and determine weather the fine targets need adjustments
			bool bRedefineOff(false), bRedefineAmp(false);
			for( u8Adc = 0; u8Adc < u8AdcPerChan; u8Adc++ )
			{
				if( u8Adc0 != u8Adc && !bPhDone[u8Adc])
				{
					if( pdPh_2[u8Adc] == pdPh[u8Adc] )
					{ 
						i32Ret = i32Err + CS_ADC_PHASE_CAL_FAILED;
						__leave;
					}
					dPhScale[u8Adc] = (_HiPhaseDacLimit - _LoPhaseDacLimit) / (pdPh_2[u8Adc] - pdPh[u8Adc]);
				}
				else
					dPhScale[u8Adc] = 0.;
				for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
				{
					if( pdOff_2[u8AdcIx[u8Adc][u8Core]] == pdOff[u8AdcIx[u8Adc][u8Core]] )
					{
						i32Ret = i32Err + (bFine ? CS_FINE_OFFSET_CAL_FAILED : CS_COARSE_OFFSET_CAL_FAILED);
						__leave;
					}
					dOffScale[u8Adc][u8Core] = _MaxAdcRegValue/(pdOff[u8AdcIx[u8Adc][u8Core]] - pdOff_2[u8AdcIx[u8Adc][u8Core]] );
					if( (dOffTarget - pdOff[u8AdcIx[u8Adc][u8Core]])*(dOffTarget - pdOff_2[u8AdcIx[u8Adc][u8Core]]) >= 0 )
						if( bFine )
							bRedefineOff = true;

					if( pdAmp_2[u8AdcIx[u8Adc][u8Core]] == pdAmp[u8AdcIx[u8Adc][u8Core]] )
					{
						i32Ret = i32Err + CS_GAIN_CAL_FAILED;
						__leave;
					}
					dAmpScale[u8Adc][u8Core] = _MaxAdcRegValue/(pdAmp[u8AdcIx[u8Adc][u8Core]] - pdAmp_2[u8AdcIx[u8Adc][u8Core]] );
					if( (dAmpTarget - pdAmp[u8AdcIx[u8Adc][u8Core]])*(dAmpTarget - pdAmp_2[u8AdcIx[u8Adc][u8Core]]) >= 0 )
						if( !bFine )
							bRedefineAmp = true;
				}
			}
			if( CS_FAILED(i32Ret) )
				__leave;
			//Redefine fine targets if needed
			if(bRedefineOff)
			{
				double dNewTarget = (dLoLimitOff + dHiLimitOff)/2.;
				TraceCalibEx( TRACE_CAL_PROGRESS, -120, u16SysChanIndex, u8AdcPerChan, &dOffTarget, &dNewTarget );
				dOffTarget = dNewTarget;
			}
			if(bRedefineAmp)
			{
				double dNewTarget = (dLoLimitAmp + dHiLimitAmp)/2.;
				TraceCalibEx( TRACE_CAL_PROGRESS, -121, u16SysChanIndex, u8AdcPerChan, &dAmpTarget, &dNewTarget );
				dAmpTarget = dNewTarget;
			}

			//Actual matching
			// Matching is done in ittereative manner on all there parameters (phase, gain, offset) at the same time.
			// This way any interdependancy should be taken care off assuming they are relativly weak.

			uInt16 u16Off[DEERE_ADC_COUNT][DEERE_ADC_CORES] = {0}, u16Amp[DEERE_ADC_COUNT][DEERE_ADC_CORES] = {0};
			uInt8 u8Iter[DEERE_ADC_COUNT][DEERE_ADC_CORES] = {0};
			bool  bOffDone[DEERE_ADC_COUNT][DEERE_ADC_CORES] = {0}, bAmpDone[DEERE_ADC_COUNT][DEERE_ADC_CORES] = {0};
			bool  bOffFailed[DEERE_ADC_COUNT][DEERE_ADC_CORES] = {0}, bAmpFailed[DEERE_ADC_COUNT][DEERE_ADC_CORES] = {0}, bPhFailed[DEERE_ADC_COUNT] = {0};
			uInt32 u32TraceIdOff  = bFine ? TRACE_MATCH_OFF_FINE : TRACE_MATCH_OFF_MED;
			uInt32 u32TraceIdAmp  = bFine ? TRACE_MATCH_GAIN_FINE : TRACE_MATCH_GAIN_MED;
			bool bDone = false;

			do
			{
				if( CS_FAILED( i32Ret = MeasureCalResult( u16ChanIndex, dSr, dFreq, pBuffer, pSol, pdOff, pdAmp, pdPh) ) )
					__leave;
				TraceCalibEx( TRACE_ADC_PHASE_MATCH|TRACE_VERB, 0, u16ChanIndex, u8AdcPerChan, u8Iter, u16PhDac, pdPh);
				TraceCalibEx(         u32TraceIdOff|TRACE_VERB, 0, u16ChanIndex, u8AdcPerChan, pdOff, u16Off, &dOffTarget, u8Iter);
				TraceCalibEx(         u32TraceIdAmp|TRACE_VERB, 0, u16ChanIndex, u8AdcPerChan, pdAmp, u16Amp, &dAmpTarget, u8Iter);
				bDone = true;
				for( u8Adc = 0; u8Adc < u8AdcPerChan; u8Adc++ )
				{
					if( !bPhDone[u8Adc])
					{
						double dCor = pdPh[u8Adc]*dPhScale[u8Adc];
						dCor += dCor > 0 ? .5 : -.5;
						if( fabs(dCor) < 1.  )
							bPhDone[u8Adc] = true;
						else
						{
							u16PhDac[u8Adc] += int16(dCor);
							if( u16PhDac[u8Adc] > _HiPhaseDacLimit )
							{
								if( bPhFailed[u8Adc] ) 
									bPhDone[u8Adc] = true;
								bPhFailed[u8Adc] = true; 
								u16PhDac[u8Adc] = _HiPhaseDacLimit;
							}
							else if( u16PhDac[u8Adc] < _LoPhaseDacLimit )
							{
								if( bPhFailed[u8Adc] ) 
									bPhDone[u8Adc] = true;
								bPhFailed[u8Adc] = true; 
								u16PhDac[u8Adc] = _LoPhaseDacLimit;
							}
							else
								 bPhFailed[u8Adc] = false; 
							if( CS_FAILED(i32Ret = SendAdcPhaseAdjust(AdcForChannel( u16ChanIndex-1, u8Adc ), u16PhDac[u8Adc]) ) )
								__leave;
						}
					}
					for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
					{
						if( !bOffDone[u8Adc][u8Core] )
						{
							double dCor = (pdOff[u8AdcIx[u8Adc][u8Core]] - dOffTarget)*dOffScale[u8Adc][u8Core];
							dCor += dCor > 0 ? .5 : -.5;
							if( fabs(dCor) < 1. || fabs(pdOff[u8AdcIx[u8Adc][u8Core]] - dOffTarget) < _OffMatchTolerance )
								bOffDone[u8Adc][u8Core] = true;
							else
							{
								int32 i32 = u16Off[u8Adc][u8Core];
								i32 += int16(dCor);
								if( i32 > _MaxAdcRegValue )
								{
									if( bOffFailed[u8Adc][u8Core] ) 
										bOffDone[u8Adc][u8Core] = true;
									bOffFailed[u8Adc][u8Core] = true; 
									u16Off[u8Adc][u8Core] = _MaxAdcRegValue;
								}
								else if( i32 < 0 )
								{
									if( bOffFailed[u8Adc][u8Core] ) 
										bOffDone[u8Adc][u8Core] = true;
									bOffFailed[u8Adc][u8Core] = true; 
									u16Off[u8Adc][u8Core] = 0;
								}
								else
								{
									bOffFailed[u8Adc][u8Core] = false; 
									u16Off[u8Adc][u8Core] = (uInt16)i32;
								}
								if ( CS_FAILED(i32Ret = SendCoreGainOffsetAdjust(u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, false, bFine, u16Off[u8Adc][u8Core] ) ) )
									__leave;
							}
						}

						if( !bAmpDone[u8Adc][u8Core] )
						{
							double dCor = (pdAmp[u8AdcIx[u8Adc][u8Core]] - dAmpTarget)*dAmpScale[u8Adc][u8Core];
							dCor += dCor > 0 ? .5 : -.5;
							if( fabs(dCor) < 1.  || fabs(pdAmp[u8AdcIx[u8Adc][u8Core]] - dAmpTarget) < _AmpMatchTolerance )
								bAmpDone[u8Adc][u8Core] = true;
							else
							{
								int32 i32 = u16Amp[u8Adc][u8Core];
								i32 += int16(dCor);
								if( i32 > _MaxAdcRegValue )
								{
									if( bAmpFailed[u8Adc][u8Core] ) 
										bAmpDone[u8Adc][u8Core] = true;
									bAmpFailed[u8Adc][u8Core] = true; 
									u16Amp[u8Adc][u8Core] = _MaxAdcRegValue;
								}
								else if( i32 < 0 )
								{
									if( bAmpFailed[u8Adc][u8Core] ) 
										bAmpDone[u8Adc][u8Core] = true;
									bAmpFailed[u8Adc][u8Core] = true; 
									u16Amp[u8Adc][u8Core] = 0;
								}
								else
								{
									bAmpFailed[u8Adc][u8Core] = false; 
									u16Amp[u8Adc][u8Core] = (uInt16)i32;
								}
								if ( CS_FAILED(i32Ret = SendCoreGainOffsetAdjust(u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, true, bFine, u16Amp[u8Adc][u8Core] ) ) )
									__leave;
							}
						}
					
						if( !bOffDone[u8Adc][u8Core] || !bAmpDone[u8Adc][u8Core] || !bPhDone[u8Adc] )
						{
							if( ++u8Iter[u8Adc][u8Core] > DEERE_CALIB_MAX_ITER )
								bOffDone[u8Adc][u8Core] = bAmpDone[u8Adc][u8Core] = bPhDone[u8Adc]  = true;
							bDone = false;
						}
					}
				}
			}while(!bDone);

			if( bFine )
				bRestoreOffFine = bRestoreGainFine = false;
			else
				bRestoreOffCoarse = bRestoreGainCoarse = false;
					
			uInt8 u8MaxIter = 0;
			for( uInt8 u8Adc = 0; u8Adc < u8AdcPerChan; u8Adc++ )
				for( uInt8 u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
					u8MaxIter = max( u8MaxIter, u8Iter[u8Adc][u8Core] );
			TraceCalibEx( TRACE_CAL_PROGRESS, bFine?80:60, u16SysChanIndex, u8AdcPerChan, &u8MaxIter);
			TraceCalibEx( TRACE_CAL_RESULT, 0, u16SysChanIndex, u8AdcPerChan, u16Amp, u16Off, &bFine );
		}
		TraceCalibEx( TRACE_ADC_PHASE_MATCH, 2, u16SysChanIndex, u8AdcPerChan, u16PhDac, pdPh, &u8Adc0);
		TraceCalibEx( TRACE_CAL_PROGRESS, 40, u16SysChanIndex, u8AdcPerChan);
	}

	__finally
	{
		for( u8Adc = 0; u8Adc < u8AdcPerChan; u8Adc++ )
			for( u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
			{
				if( bRestoreOffCoarse ) 	                
					SendCoreGainOffsetAdjust(u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, false,  false, u16OldOffCoarse[u8Adc][u8Core] );
				if( bRestoreOffCoarse || bRestoreOffFine)   
					SendCoreGainOffsetAdjust(u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, false,  true,  u16OldOffFine[u8Adc][u8Core]   );
				
				if( bRestoreGainCoarse )   				   
					SendCoreGainOffsetAdjust(u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, true,   false, u16OldGainMed[u8Adc][u8Core]   );
				if( bRestoreGainCoarse || bRestoreGainFine)	
					SendCoreGainOffsetAdjust(u16ChanIndex, AdcForChannel( u16ChanIndex-1, u8Adc ), u8Core, true,   true,  u16OldGainFine[u8Adc][u8Core]  );
			}

		delete[] pSol;
		delete[] pdPh; 
		delete[] pdAmp;
		delete[] pdOff;
		delete[] pdPh_2;
		delete[] pdAmp_2;
		delete[] pdOff_2;
		::VirtualFree( pBuffer, 0, MEM_RELEASE );
	}

	return i32Ret;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDeereDevice::VerifyCalibration( const uInt16 u16ChanIndex, const double dSr,  const double dFreq )
{
	if( 0 == (TRACE_FINAL_RESULT & m_u32DebugCalibTrace ) )
		return CS_FALSE;

	void* pBuffer = ::GageAllocateMemoryX( DEERE_ADC_CALIB_DEPTH * m_pCardState->AcqConfig.u32SampleSize );
	if( NULL == pBuffer) { return CS_INVALID_CALIB_BUFFER;}
		
	const uInt8 u8Cores = m_pCardState->u8NumOfAdc*DEERE_ADC_CORES;
	double* pdPh  = new double[u8Cores];
	double* pdAmp = new double[u8Cores];
	double* pdOff = new double[u8Cores];
	double* pdSnr = new double[u8Cores];
	SINEFIT3* pSol = new SINEFIT3[u8Cores];

	int32 i32Ret = CS_SUCCESS;
	__try
	{
		if( CS_FAILED( i32Ret = MeasureCalResult( u16ChanIndex, dSr, dFreq, pBuffer, pSol, pdOff, pdAmp, pdPh, pdSnr) ) ) __leave;
	
		double dMeanAmp(0.), dMeanOff(0.);
		uInt8 u8;
		for( u8 = 0; u8 < u8Cores; u8++ )
		{
			dMeanAmp += pdAmp[u8];
			dMeanOff += pdOff[u8];
		}
		dMeanAmp /= u8Cores;
		dMeanOff /= u8Cores;
		char str[250];
		sprintf_s( str, sizeof(str), "\n               Calibration result @ %5.1f MHz:\n", dFreq/1.e6 );
		OutputDebugString(str);
		OutputDebugString("            Ix Core    Offset,ppm  Amp. mdB   Phase,fs    SINAD\n");
		for( u8 = 0; u8 < u8Cores; u8++ )
		{
			sprintf_s( str, sizeof(str), "                %d      %+6.1f     %+6.1f      %+6.1f    %5.2f dB\n",
				u8, 
				1.e6*(pdOff[u8]-dMeanOff)/labs(m_pCardState->AcqConfig.i32SampleRes), 
				2.e4*log10(fabs(pdAmp[u8]/dMeanAmp)),
				1.e3*pdPh[u8], 
				pdSnr[u8]);
			OutputDebugString(str);
		}

		OutputDebugString("            ----------------------------------------------------------\n");
		sprintf_s( str, sizeof(str), "                      %+6.1f%%      %+6.1f%%\n",100.*dMeanOff/labs(m_pCardState->AcqConfig.i32SampleRes), 100.*dMeanAmp/labs(m_pCardState->AcqConfig.i32SampleRes) );
		OutputDebugString(str);
	}

	__finally
	{
		delete[] pSol;
		delete[] pdPh; 
		delete[] pdAmp;
		delete[] pdOff;
		delete[] pdSnr;
		::VirtualFree( pBuffer, 0, MEM_RELEASE );
	}
	return i32Ret;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int32 CsDeereDevice::SendCoreGainOffsetAdjust( const uInt16 u16ChanIndex, const uInt8 u8Adc, const uInt8 u8Core, bool bGain, bool bFine, const uInt16 u16Value )
{
	uInt8 u8Dac;
	const uInt16 u16SysChanIndex = u16ChanIndex + ((m_pCardState->u16CardNumber-1) * m_pCsBoard->NbAnalogChannel );

	if( bGain )
		u8Dac = bFine? DEERE_ADC_FINE_GAIN : DEERE_ADC_MEDIUM_GAIN ;
	else
		u8Dac = bFine? DEERE_ADC_FINE_OFFSET : DEERE_ADC_COARSE_OFFSET ;

	int32 i32Status = WriteToAdcReg( u8Core, u8Adc, u8Dac, (uInt8) u16Value );
	if ( CS_FAILED(i32Status) ) 
		TraceCalibEx( TRACE_CAL_PROGRESS, -4, u16SysChanIndex, u8Adc, &i32Status );

	return i32Status;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int32 CsDeereDevice::ReadCoreGainOffsetAdjust(const uInt16 u16ChanIndex, const uInt8 u8Adc, const uInt8 u8Core, bool bGain, bool bFine, uInt16 *u16Value )
{
	uInt8  u8DacAddress;
	if( bGain )
		u8DacAddress = bFine? DEERE_ADC_FINE_GAIN : DEERE_ADC_MEDIUM_GAIN ;
	else
		u8DacAddress = bFine? DEERE_ADC_FINE_OFFSET : DEERE_ADC_COARSE_OFFSET ;

	uInt8	u8DacValue = 0;
	int32 i32RetCode = ReadAdcReg(u8Core, u8Adc, u8DacAddress, &u8DacValue );
	if ( CS_FAILED(i32RetCode) ) 
	{
		uInt16 u16SysChanIndex = u16ChanIndex + ((m_pCardState->u16CardNumber-1) * m_pCsBoard->NbAnalogChannel );
		TraceCalibEx( TRACE_CAL_PROGRESS, -6, u16SysChanIndex, u8Adc, &i32RetCode );
	}

	*u16Value = u8DacValue;
	return i32RetCode;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CsDeereDevice::TraceCalibEx( DWORD dwTraceId, int nTraceIdMod, uInt16 u16Chan, uInt16 u16AdcPerChan, const void* const pBuf1, const void* const pBuf2, const void* const pBuf3, const void* const pBuf4 )
{
	if( dwTraceId != (m_u32DebugCalibTrace & dwTraceId) )
		return;

	if( 0 != (m_u32DebugCalibTrace&TRACE_USAGE) )
	{
		OutputCalibTraceUsage();
		m_u32DebugCalibTrace &= ~TRACE_USAGE;
	}

	bool bVerbRequest = 0 != (m_u32DebugCalibTrace & TRACE_VERB);
	bool bVerbReport = 0 != (dwTraceId & TRACE_VERB);
	bool bFine = false;
	char str1[128] = "", str2[128] = "", str3[128] = "", str4[128] = "";
	char strTrace[1024];

	dwTraceId &= ~TRACE_VERB;
	switch( dwTraceId )
	{
		case TRACE_CAL_PROGRESS:
		{
			int32* pi32 = (int32*)pBuf1; 
			switch ( nTraceIdMod )
			{
			case    0: strcpy_s( strTrace, 1024, "  >> Starting user mode calibration.\n" ); break;
			case    1: strcpy_s( strTrace, 1024, "\n~ ~ ~ Skipping user mode calibration. Loss of reference lock.\n\n" ); break;
			case    2: sprintf_s( strTrace, 1024, "\n~ ~ ~ Skipping user mode calibration Chan %d. Loss of Cal signal lock.\n\n", u16Chan ); break;
			case  100: strcpy_s( strTrace, 1024, "  << Finished user mode calibration.\n" ); break;
			case   10: sprintf_s( strTrace, 1024, "    -> Starting core matching for Chan %d (#adc = %d).\n",  u16Chan, u16AdcPerChan ); break;
			case   20: sprintf_s( strTrace, 1024, "    <- Finished core matching for Chan %d (#adc = %d). Iter = %d\n",  u16Chan, m_pCardState->u8NumOfAdc, *((uInt8*)pBuf1) ); break;
			case   30: sprintf_s( strTrace, 1024, "    => Starting Adc matching for Chan %d (#adc = %d).\n",  u16Chan, m_pCardState->u8NumOfAdc ); break;
			case   40: sprintf_s( strTrace, 1024, "    <= Finished Adc matching for Chan %d (#adc = %d).\n",  u16Chan, m_pCardState->u8NumOfAdc ); break;
			case   50: sprintf_s( strTrace, 1024, "     ==>  Starting med. user space matching for Chan %d (#adc = %d).\n",  u16Chan, m_pCardState->u8NumOfAdc ); break;
			case   60: sprintf_s( strTrace, 1024, "     <==  Finished med. user space matching for Chan %d (#adc = %d). Iter = %d\n",  u16Chan, m_pCardState->u8NumOfAdc, *((uInt8*)pBuf1) ); break;
			case   70: sprintf_s( strTrace, 1024, "     -=>  Starting fine user space matching for Chan %d (#adc = %d).\n",  u16Chan, u16AdcPerChan ); break;
			case   80: sprintf_s( strTrace, 1024, "     <=-  Finished fine user space matching for Chan %d (#adc = %d). Iter = %d\n",  u16Chan, m_pCardState->u8NumOfAdc, *((uInt8*)pBuf1) ); break;
			
			case   -1: sprintf_s( strTrace, 1024, "! ! ! CsSet Chan for Calib failed. Chan %d Error %d\n", u16Chan, *pi32 ); break;
			case   -2: sprintf_s( strTrace, 1024, "! ! ! CsSet Calib mode. Chan %d Error %d\n", u16Chan, *pi32 ); break;
			case   -3: sprintf_s( strTrace, 1024, "! ! ! CsSet restore cal mode. Chan %d Error %d\n", u16Chan, *pi32 ); break;
			case   -4: sprintf_s( strTrace, 1024, "! ! ! CsSet SEND_DAC. Chan %d Error %d\n", u16Chan, *pi32 ); break;
			case   -5: sprintf_s( strTrace, 1024, "! ! ! CsSet CS_AC_CAL_FREQ  Error %d \n", *pi32 ); break;
			case   -6: sprintf_s( strTrace, 1024, "! ! ! CsGet READ_DAC  Error %d \n", *pi32 ); break;
			default:   sprintf_s( strTrace, 1024, "* * * Undefined Trace TRACE_CAL_PROGRESS %d\n", nTraceIdMod ); break;
			}
			break;
		}
		case TRACE_ADC_CORE_ORDER:
		{
			uInt8 u8Ix[DEERE_ADC_COUNT][DEERE_ADC_CORES];
			memcpy( u8Ix, pBuf1, sizeof(u8Ix) );
			char str[DEERE_ADC_CORES*DEERE_ADC_COUNT*3+2] = {0};
			for( uInt8 u8AdcIx = 0; u8AdcIx < u16AdcPerChan; u8AdcIx++ )
			{
				uInt8 u8Adc = AdcForChannel( u16Chan-1, u8AdcIx );
				for( uInt8 u8Core = 0; u8Core < DEERE_ADC_CORES; u8Core++ )
				{
					str[u8Ix[u8Adc][u8Core]*3] = '0' + u8Adc; 
					str[u8Ix[u8Adc][u8Core]*3+1] = 'a' + u8Core; 
					str[u8Ix[u8Adc][u8Core]*3+2] = ' '; 
				}
			}
			sprintf_s( strTrace, 1024, "  1 2 3 User space:    Ch %d Adc core order %s\n", u16Chan, str );
			break;
		}
		case TRACE_CORE_SKEW_MATCH:
		{
			if( bVerbReport )
				if( bVerbRequest )
				{
					for( uInt8 u8 = 0; u8 < u16AdcPerChan; u8++ )
					{
						char strTmp[15];
						sprintf_s(strTmp, 15, " %2d", ((uInt8*)pBuf1)[u8] );  //iter
						strcat_s( str1, 128, strTmp );
						sprintf_s(strTmp, 15, " %+5.2f", ((double*)pBuf2)[u8] ); //skew
						strcat_s( str2, 128, strTmp );
						sprintf_s(strTmp, 15, " %3d", ((uInt8*)pBuf3)[u8] ); //reg
						strcat_s( str4, 128, strTmp );
					}
					sprintf_s( strTrace, 1024, "    ~ ~ ~ ~ %s Core skew matching chan %d Skew%s Reg%s\n", str1, u16Chan, str2, str4 );
					break;
				}
				else
					return;
			else
			{
				switch( nTraceIdMod )
				{
					default:   sprintf_s( strTrace, 1024, "* * * Undefined Trace TRACE_CORE_SKEW_MATCH %d\n", nTraceIdMod ); break;
					case  0:   sprintf_s( strTrace, 1024, "    \\ \\     Starting Core skew match chan %d\n", u16Chan ); break;
					case 10:   
						{
							for( uInt8 u8 = 0; u8 < u16AdcPerChan; u8++ )
							{
								char strTmp[15];
								sprintf_s(strTmp, 15, " %2d", ((uInt8*)pBuf1)[u8] );  //iter
								strcat_s( str1, 128, strTmp );
								sprintf_s(strTmp, 15, " %3d", ((uInt8*)pBuf2)[u8] );  //Skew
								strcat_s( str2, 128, strTmp );
								sprintf_s(strTmp, 15, " %+5.2f", ((double*)pBuf3)[u8] );  //Skew
								strcat_s( str3, 128, strTmp );
							}
							sprintf_s( strTrace, 1024, "    / /     Finished Core skew match chan %d Steps %s Reg %s Skew %s\n", u16Chan, str1, str2, str3 );
							break;
						}
					case  1:
						{
							for( uInt8 u8 = 0; u8 < u16AdcPerChan; u8++ )
							{
								char strTmp[15];
								sprintf_s(strTmp, 15, " %+5.2f", ((double*)pBuf1)[u8] );  //Skew 0
								strcat_s( str1, 128, strTmp );
								sprintf_s(strTmp, 15, " %+5.2f", ((double*)pBuf2)[u8]);  //Skew 255
								strcat_s( str2, 128, strTmp );
								sprintf_s(strTmp, 15, " %+5.2f", ((double*)pBuf1)[u8] - ((double*)pBuf2)[u8]);  //Span
								strcat_s( str3, 128, strTmp );
							}
							sprintf_s( strTrace, 1024, "    - -     Core skew span chan %d Skew0 %s Skew255 %s Span %s\n", 
								u16Chan, str1, str2, str3 ); break;
							break;
						}
				}
				break;
			}
		}
		case TRACE_CORE_SKEW_MEAS:
		{
			if( bVerbReport )
				if( bVerbRequest )
				{
					for( uInt8 u8 = 0; u8 < u16AdcPerChan*2; u8++ )
					{
						char strTmp[15];
						sprintf_s(strTmp, 15, " %+8.3f", 1.e12*(((SINEFIT3*)pBuf2)[u8].dPhase - ((SINEFIT3*)pBuf2)[0].dPhase)/ ( 2.0 * M_PI * *((double*)pBuf3) ));
						strcat_s( str1, 128, strTmp );

						sprintf_s(strTmp, 15, " %8.1f", ((SINEFIT3*)pBuf2)[u8].dAmp);
						strcat_s( str2, 128, strTmp );
						
						sprintf_s(strTmp, 15, " %+8.3f", ((SINEFIT3*)pBuf2)[u8].dOff);
						strcat_s( str3, 128, strTmp );
						
						sprintf_s(strTmp, 15, " %4.1f", ((SINEFIT3*)pBuf2)[u8].dSnr);
						strcat_s( str4, 128, strTmp );
					}
					sprintf_s( strTrace, 1024, " =*=*=*= Ch %d Loop %2d Core params Skews %s Amps %s Off %s  Snr %s\n", u16Chan, *((uInt8*)pBuf1), str1, str2, str3, str4 );
					break;
				}
				else
					return;
			else
			{
				for( uInt8 u8 = 0; u8 < u16AdcPerChan*2; u8++ )
				{
					char strTmp[30];
					sprintf_s(strTmp, 15, " %+8.3f", ((double*)pBuf1)[u8]);
					strcat_s( str1, 128, strTmp );

					sprintf_s(strTmp, 15, " %+8.1f", ((double*)pBuf2)[u8]);
					strcat_s( str2, 128, strTmp );
					
					sprintf_s(strTmp, 15, " %+8.3f", ((double*)pBuf3)[u8]);
					strcat_s( str3, 128, strTmp );
				}
				sprintf_s( strTrace, 1024, " = = = = Ch %d         Core params Skew %s Amps %s Off %s\n", u16Chan, str1, str2, str3 ); break;
				break;
			}
		}

		case TRACE_ADC_PHASE_MATCH:
		{
			if( bVerbReport )
				if( bVerbRequest )
				{
					for( uInt8 u8 = 0; u8 < u16AdcPerChan; u8++ )
					{
						char strTmp[15];
						sprintf_s(strTmp, 15, " %2d", ((uInt8*)pBuf1)[u8] );  //iter
						strcat_s( str1, 128, strTmp );
						sprintf_s(strTmp, 15, " %4d", ((uInt16*)pBuf2)[u8] ); //DAC
						strcat_s( str2, 128, strTmp );
						sprintf_s(strTmp, 15, " %+8.3f", ((double*)pBuf3)[u8] ); //Phase
						strcat_s( str3, 128, strTmp );
					}
					sprintf_s( strTrace, 1024, "    ~ ~ ~ ~ %s Phase delta matching chan %d Deltas %s DAC values %s\n", str1, u16Chan, str3,  str2);
					break;
				}
				else
					return;
			else
			{
				switch( nTraceIdMod )
				{
					default:   sprintf_s( strTrace, 1024, "* * * Undefined Trace TRACE_ADC_PHASE_MATCH %d\n", nTraceIdMod ); break;
					case  1:
					{
						for( uInt8 u8 = 0; u8 < u16AdcPerChan; u8++ )
						{
							char strTmp[15];
							sprintf_s(strTmp, 15, " %+5.2f", ((double*)pBuf2)[u8] );  //Phase 1
							strcat_s( str1, 128, strTmp );
							sprintf_s(strTmp, 15, " %+5.2f", ((double*)pBuf3)[u8]);  //Phase 2
							strcat_s( str2, 128, strTmp );
							sprintf_s(strTmp, 15, " %+5.2f", ((double*)pBuf3)[u8] - ((double*)pBuf2)[u8]);  //Span
							strcat_s( str3, 128, strTmp );
						}
						sprintf_s( strTrace, 1024, "    - -   PhDac[%d] %4d chan %d  Spans %s: ( PhDac %d: %s  PhDac %d: %s )\n", 
							*((uInt8*)pBuf4), *((uInt16*)pBuf1+*((uInt8*)pBuf4)), u16Chan, str3, _LoPhaseDacLimit, str1, _HiPhaseDacLimit, str2 ); break;
						break;
					}
					case  2:
					{
						for( uInt8 u8 = 0; u8 < u16AdcPerChan; u8++ )
						{
							char strTmp[15];
							sprintf_s(strTmp, 15, " %4d", ((uInt16*)pBuf1)[u8] );  //Dac
							strcat_s( str1, 128, strTmp );
							sprintf_s(strTmp, 15, " %+5.2f", ((double*)pBuf2)[u8] - ((double*)pBuf2)[*((uInt8*)pBuf3)]);  // pdPh
							strcat_s( str2, 128, strTmp );
						}
						sprintf_s( strTrace, 1024, "    - -   PhDac %s Phase %s\n", str1, str2); 
							
						break;
					}
				}
				break;
			}
		}	
		case TRACE_CAL_RESULT: 
			{
				for( uInt8 u8 = 0; u8 < u16AdcPerChan*2; u8++ )
				{
					char strTmp[15];
					sprintf_s(strTmp, 15, " %4d", ((uInt16*)pBuf1)[u8] );  //Amp
					strcat_s( str1, 128, strTmp );
					sprintf_s(strTmp, 15, " %4d", ((uInt16*)pBuf2)[u8] );  //Off
					strcat_s( str2, 128, strTmp );
				}
				sprintf_s( strTrace, 1024, "          Resulting Reg values for %s Calibration\n                 Off %s\n                 Amp %s", 
					*((bool*)pBuf3)?"fine":"med.",str2, str1); 
			}

				
		case TRACE_MATCH_OFF_FINE: 
			bFine = true;
		case TRACE_MATCH_OFF_MED:
		{
			if( bVerbReport && bVerbRequest )
			{
				for( uInt8 u8 = 0; u8 < u16AdcPerChan*DEERE_ADC_CORES; u8++ )
				{
					char strTmp[15];
					sprintf_s(strTmp, 15, " %+5.1f", ((double*)pBuf1)[u8] - *((double*)pBuf3) );  //pdOff
					strcat_s( str1, 128, strTmp );
					sprintf_s(strTmp, 15, " %3d", ((uInt16*)pBuf2)[u8] ); //u16Off
					strcat_s( str2, 128, strTmp );
					sprintf_s(strTmp, 15, " %2d", ((uInt8*)pBuf4)[u8] ); //Iter
					strcat_s( str3, 128, strTmp );
				}
				sprintf_s( strTrace, 1024, "    = = = = %s User space %s offset matching chan %d Target %+8.1f Err %s Dacs %s\n", 
					str3, bFine?"fine":"med.", u16Chan, *((double*)pBuf3), str1, str2);
			}
			break;
		}
		case TRACE_MATCH_GAIN_FINE: 
			bFine = true;
		case TRACE_MATCH_GAIN_MED:
		{
			if( bVerbReport && bVerbRequest )
			{
				for( uInt8 u8 = 0; u8 < u16AdcPerChan*DEERE_ADC_CORES; u8++ )
				{
					char strTmp[15];
					sprintf_s(strTmp, 15, " %+5.1f", ((double*)pBuf1)[u8] - *((double*)pBuf3) );  //pdAmp
					strcat_s( str1, 128, strTmp );
					sprintf_s(strTmp, 15, " %3d", ((uInt16*)pBuf2)[u8] ); //u16Amp
					strcat_s( str2, 128, strTmp );
					sprintf_s(strTmp, 15, " %2d", ((uInt8*)pBuf4)[u8] ); //Iter
					strcat_s( str3, 128, strTmp );
				}
				sprintf_s( strTrace, 1024, "    _ - _ - %s User space %s gain   matching chan %d Target %+8.1f Err %s Dacs %s\n", 
					str3, bFine?"fine":"med.", u16Chan, *((double*)pBuf3), str1, str2);
			}
			break;
		}		
	}
	OutputDebugString( strTrace );
}




int32 CsDeereDevice::SendCoreSkewAdjust( const uInt8 u8Adc, const uInt8 u8Skew )
{
	switch ( u8Adc )
	{
		default: return CS_INVALID_PARAMETER;
		case 0:
		case 1:
		case 2:
		case 3:
			return WriteToAdcReg(0, u8Adc, DEERE_ADC_SKEW, u8Skew );						
	}
}

int32 CsDeereDevice::SendAdcPhaseAdjust( const uInt8 u8Adc, const uInt16 u16Val )
{
	uInt8	u8DacAddress = 0;
	switch ( u8Adc )
	{
		default: return CS_INVALID_PARAMETER;
		case 0: u8DacAddress = DEERE_CLKPHASE_0; break;
		case 1: u8DacAddress = DEERE_CLKPHASE_1; break;
		case 2: u8DacAddress = DEERE_CLKPHASE_2; break;
		case 3: u8DacAddress = DEERE_CLKPHASE_3; break;
	}

	return WriteToDac( u8DacAddress , u16Val );
}

void trace(const char* format, ...)
{
   char buffer[1000];

   va_list argptr;
   va_start(argptr, format);
#ifdef _WINDOWS_
   wvsprintf(buffer, format, argptr);
#else
   vsprintf(buffer, format, argptr);
#endif
   va_end(argptr);

   OutputDebugString(buffer);
}
