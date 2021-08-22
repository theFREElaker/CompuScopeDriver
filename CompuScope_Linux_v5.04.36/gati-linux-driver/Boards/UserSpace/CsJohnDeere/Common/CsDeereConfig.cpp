#include "StdAfx.h"
#include "CsDeere.h"


//-----------------------------------------------------------------------------
//------ CsGetAcquisitionConfig
//-----------------------------------------------------------------------------

int32	CsDeereDevice::CsGetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg )
{
	pAcqCfg->u32Size			= sizeof(CSACQUISITIONCONFIG);
	pAcqCfg->i64SampleRate		= m_pCardState->AcqConfig.i64SampleRate;
	pAcqCfg->u32ExtClk			= m_pCardState->AcqConfig.u32ExtClk;
	pAcqCfg->u32ExtClkSampleSkip= m_pCardState->AcqConfig.u32ExtClkSampleSkip;
	pAcqCfg->u32Mode			= m_pCardState->AcqConfig.u32Mode;
	pAcqCfg->u32SampleBits		= m_pCardState->AcqConfig.u32SampleBits;
	pAcqCfg->i32SampleRes		= m_pCardState->AcqConfig.i32SampleRes;
	pAcqCfg->u32SampleSize		= m_pCardState->AcqConfig.u32SampleSize;
	pAcqCfg->u32TrigEnginesEn	= m_pCardState->AcqConfig.u32TrigEnginesEn;
	pAcqCfg->i64TriggerDelay	= m_pCardState->AcqConfig.i64TriggerDelay;
	pAcqCfg->i64TriggerHoldoff	= m_pCardState->AcqConfig.i64TriggerHoldoff;
	pAcqCfg->u32SegmentCount	= m_pCardState->AcqConfig.u32SegmentCount;
	pAcqCfg->i64Depth			= m_pCardState->AcqConfig.i64Depth;
	pAcqCfg->i64SegmentSize		= m_pCardState->AcqConfig.i64SegmentSize;
	pAcqCfg->i32SampleOffset	= m_pCardState->AcqConfig.i32SampleOffset;
	pAcqCfg->i64TriggerTimeout	= m_pCardState->AcqConfig.i64TriggerTimeout;
	pAcqCfg->u32TimeStampConfig = m_pCardState->AcqConfig.u32TimeStampConfig;

	return CS_SUCCESS;
}



//-----------------------------------------------------------------------------
//CsGetChannelConfig
//-----------------------------------------------------------------------------
int32	CsDeereDevice::CsGetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg )
{
	CsDeereDevice* pDevice = this;
	PCSCHANNELCONFIG pChanCfg = pAChanCfg->aChannel;
	for (uInt32 i = 0; i < pAChanCfg->u32ChannelCount; i++ )
	{
		pChanCfg->u32Size = sizeof(CSCHANNELCONFIG);
		uInt16 NormalizedChanIndex = CS_CHAN_1;
		if ( 2 == m_pCsBoard->NbAnalogChannel )
			NormalizedChanIndex = (pChanCfg[i].u32ChannelIndex % 2) ? CS_CHAN_1: CS_CHAN_2;
		
		pDevice = GetCardPointerFromChannelIndex( (uInt16) pChanCfg[i].u32ChannelIndex );
		if ( NULL != pDevice )
		{
			pChanCfg[i].u32Term = pDevice->m_pCardState->ChannelParams[NormalizedChanIndex-1].u32Term;
			pChanCfg[i].u32InputRange = pDevice->m_pCardState->ChannelParams[NormalizedChanIndex-1].u32InputRange;
			pChanCfg[i].u32Impedance = pDevice->m_pCardState->ChannelParams[NormalizedChanIndex-1].u32Impedance;
			pChanCfg[i].u32Filter = pDevice->m_pCardState->ChannelParams[NormalizedChanIndex-1].u32Filter;
			pChanCfg[i].i32DcOffset = pDevice->m_pCardState->ChannelParams[NormalizedChanIndex-1].i32Position;
		}
	}
	
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//CsGetTriggerConfig
//-----------------------------------------------------------------------------
int32	CsDeereDevice::CsGetTriggerConfig( PARRAY_TRIGGERCONFIG pATrigCfg )
{
	CsDeereDevice		*pDevice = m_MasterIndependent;
	uInt16				NormalizedTrigIndex = 0;
	PCSTRIGGERCONFIG	pTrigCfg = pATrigCfg->aTrigger;
	BOOLEAN				bFoundTrigIndex = FALSE;


	for (uInt32 i = 0; i < pATrigCfg->u32TriggerCount; i++ )
	{
		// Searching for the user trigger index
		pDevice = m_MasterIndependent;
		while( pDevice )
		{

			for ( uInt16 j = 0; j < m_pCsBoard->NbTriggerMachine; j++ )
			{
				if ( pTrigCfg[i].u32TriggerIndex == pDevice->m_pCardState->TriggerParams[j].u32UserTrigIndex )
				{
					bFoundTrigIndex = TRUE;
					NormalizedTrigIndex = j;
					break;						
				}
			}

			if ( bFoundTrigIndex )
			{
				pTrigCfg[i].u32Condition = pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].u32Condition;
				pTrigCfg[i].i32Level = pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].i32Level;
				pTrigCfg[i].u32Relation = pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].u32Relation;
				pTrigCfg[i].u32Filter = pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].u32Filter;
				pTrigCfg[i].i32Value1 = pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].i32Value1;
				pTrigCfg[i].i32Value2 = pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].i32Value2;

				// Return extra parameters from external trigger
				pTrigCfg[i].u32ExtImpedance = pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].u32ExtImpedance;
				pTrigCfg[i].u32ExtCoupling	= pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].u32ExtCoupling;
				pTrigCfg[i].u32ExtTriggerRange	= pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].u32ExtTriggerRange;

				// Un normalized trigger sources
				if ( pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].i32Source ==  0 )
					pTrigCfg[i].i32Source = pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].i32Source;
				else if ( pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].i32Source < 0 )
					pTrigCfg[i].i32Source = pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].i32Source * pDevice->m_pCardState->u16CardNumber;
				else 
					pTrigCfg[i].i32Source = pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].i32Source + m_pCsBoard->NbAnalogChannel*(pDevice->m_pCardState->u16CardNumber - 1);

				break;
			}
			else
				pDevice = pDevice->m_Next;
		}

		// Reset flag
		bFoundTrigIndex = FALSE;
	}

	return CS_SUCCESS;
}



//-----------------------------------------------------------------------------
//CsGetBoardsInfo
//-----------------------------------------------------------------------------
int32	CsDeereDevice::CsGetBoardsInfo( PARRAY_BOARDINFO pABoardInfo )
{
	CsDeereDevice	*pDevice;
	PCSBOARDINFO	BoardInfo = pABoardInfo->aBoardInfo;

	for (uInt32 i = 0; i < pABoardInfo->u32BoardCount; i++ )
	{
		BoardInfo[i].u32Size = sizeof(CSBOARDINFO);
		BoardInfo[i].u32BoardIndex = (uInt16) (i + 1);

		if ( NULL != (pDevice = GetCardPointerFromBoardIndex( (uInt16) BoardInfo[i].u32BoardIndex ) ) )
		{
			BoardInfo[i].u32BoardType = m_pSystem->SysInfo.u32BoardType;
			sprintf_s(BoardInfo[i].strSerialNumber, sizeof(BoardInfo[i].strSerialNumber), TEXT("%08x/%08x"), pDevice->m_pCsBoard->u32SerNum, pDevice->m_pCsBoard->u32SerNum);
			BoardInfo[i].u32BaseBoardVersion = pDevice->m_pCsBoard->u32BaseBoardVersion;

			// Conversion of the firmware version to the standard  xx.xx.xx format
			BoardInfo[i].u32BaseBoardFirmwareVersion = ((pDevice->m_pCsBoard->u32RawFwVersionB[0]  & 0xFF00) << 8) |
														((pDevice->m_pCsBoard->u32RawFwVersionB[0] & 0xF0) << 4) |
														(pDevice->m_pCsBoard->u32RawFwVersionB[0] & 0xF);

			BoardInfo[i].u32BaseBoardFwOptions	= pDevice->m_pCsBoard->u32BaseBoardOptions[0];	
			BoardInfo[i].u32BaseBoardHwOptions	= pDevice->m_pCsBoard->u32BaseBoardHardwareOptions;	
			BoardInfo[i].u32AddonHwOptions		= pDevice->m_pCsBoard->u32AddonHardwareOptions;

			// No addon for JohnDeere
			BoardInfo[i].u32AddonBoardVersion = 0;
			BoardInfo[i].u32AddonBoardFirmwareVersion = 0;
			BoardInfo[i].u32AddonFwOptions = 0;
		}
	}

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//CsSetAcquisitionConfig
//-----------------------------------------------------------------------------

int32 CsDeereDevice::CsSetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg )
{
	int32	i32Ret = CS_SUCCESS;


	if( !GetInitStatus()->Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( pAcqCfg )
	{
		m_pCardState->AcqConfig.u32TrigEnginesEn	= pAcqCfg->u32TrigEnginesEn;
		m_pCardState->AcqConfig.i64TriggerDelay	= pAcqCfg->i64TriggerDelay;
		m_pCardState->AcqConfig.i64TriggerHoldoff	= pAcqCfg->i64TriggerHoldoff;
		m_pCardState->AcqConfig.u32SegmentCount	= pAcqCfg->u32SegmentCount;
		m_pCardState->AcqConfig.i64Depth			= pAcqCfg->i64Depth;
		m_pCardState->AcqConfig.u32TimeStampConfig = pAcqCfg->u32TimeStampConfig;
		m_pCardState->AcqConfig.i64SegmentSize	= pAcqCfg->i64SegmentSize;
		
		// Some algorithm relies on the order of function calls
		// Do not change the order of
		// SendClockSetting
		// SendCaptureModeSetting

		i32Ret = SendMasterSlaveSetting();
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = SendClockSetting( pAcqCfg->u32Mode, pAcqCfg->i64SampleRate, 
						           pAcqCfg->u32ExtClk, (pAcqCfg->u32Mode & CS_MODE_REFERENCE_CLK) != 0 );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		if ( m_bCh2Single )
			i32Ret = SendCaptureModeSetting( pAcqCfg->u32Mode, CS_CHAN_2 );
		else
			i32Ret = SendCaptureModeSetting( pAcqCfg->u32Mode );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		
		m_pCardState->AcqConfig.u32ExtClkSampleSkip	= CS_SAMPLESKIP_FACTOR;

		m_pCardState->AcqConfig.i64TriggerTimeout = pAcqCfg->i64TriggerTimeout;
		SetTriggerTimeout( ConvertToHwTimeout((uInt32) pAcqCfg->i64TriggerTimeout, (uInt32)pAcqCfg->i64SampleRate ) );

		m_u32HwSegmentCount		= pAcqCfg->u32SegmentCount;
		m_i64HwSegmentSize		= m_pCardState->AcqConfig.i64SegmentSize;
		m_i64HwPostDepth		= m_pCardState->AcqConfig.i64Depth;
		m_i64HwTriggerHoldoff	= m_pCardState->AcqConfig.i64TriggerHoldoff;

		// Adjust Segemnt Parameters for Hardware
		AdjustedHwSegmentParameters( m_pCardState->AcqConfig.u32Mode, &m_u32HwSegmentCount, &m_i64HwSegmentSize, &m_i64HwPostDepth );

		CsDeereDevice	*pDevice = m_MasterIndependent;
		while (pDevice)
		{
			i32Ret = pDevice->SendSegmentSetting (m_u32HwSegmentCount, m_i64HwPostDepth, m_i64HwSegmentSize, m_i64HwTriggerHoldoff);
			if( CS_FAILED(i32Ret) )
				return i32Ret;

			i32Ret = pDevice->SendTimeStampMode( ((pAcqCfg->u32TimeStampConfig & TIMESTAMP_MCLK) != 0),
					                             ((pAcqCfg->u32TimeStampConfig & TIMESTAMP_FREERUN) != 0) );

			if( CS_FAILED(i32Ret) )
				return i32Ret;
								
			pDevice = pDevice->m_Next;
		}

		if ( ( CS_MODE_SW_AVERAGING & pAcqCfg->u32Mode ) != 0 ) 
			m_bSoftwareAveraging = TRUE;
		else
            m_bSoftwareAveraging = FALSE;

		if ( m_bSoftwareAveraging )
		{
			if ( NULL == m_pAverageBuffer )
			{
				uInt32	u32MaxAverageSamples = MAX_SW_AVERAGING_SEMGENTSIZE + CsGetPostTriggerExtraSamples(pAcqCfg->u32Mode);
				uInt32	u32MaxAverageBufferSize = u32MaxAverageSamples * sizeof(int16);

				m_pAverageBuffer = (int16 *)::GageAllocateMemoryX( u32MaxAverageBufferSize );
				if( NULL == m_pAverageBuffer )
				{
					return CS_INSUFFICIENT_RESOURCES;
				}
			}
		}
		else if ( NULL != m_pAverageBuffer  )
		{
			::GageFreeMemoryX( m_pAverageBuffer );
			m_pAverageBuffer = NULL;
		}
	}
	else
	{
		// Default setting
		m_pCardState->AcqConfig.u32ExtClk = 0;
		m_pCardState->AcqConfig.u32ExtClkSampleSkip	= CS_SAMPLESKIP_FACTOR;
		m_FirConfig.bEnable = 0;		 
		m_u32MulrecAvgCount	 = 1;
		m_bSoftwareAveraging = FALSE;

		// Release average buffer if there is any
		if ( m_pAverageBuffer )
		{
			::GageFreeMemoryX( m_pAverageBuffer );
			m_pAverageBuffer = NULL;
		}

		// Restore the default FPGA image
		CsLoadFpgaImage( 0 );

		i32Ret = SendMasterSlaveSetting();
		if( CS_FAILED(i32Ret) )	return i32Ret;

		uInt32 u32DefaultMode = DEERE_DEFAULT_MODE;	
		if ( 1 == m_pCsBoard->NbAnalogChannel ) 
			u32DefaultMode = CS_MODE_SINGLE;

		m_bSquareRec = IsSquareRec(u32DefaultMode);

		i32Ret = SendClockSetting( u32DefaultMode );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = SendCaptureModeSetting( u32DefaultMode );
		if( CS_FAILED(i32Ret) )	return i32Ret;
		
		m_pCardState->AcqConfig.i64TriggerTimeout = CS_TIMEOUT_DISABLE;
		SetTriggerTimeout( ConvertToHwTimeout((uInt32) m_pCardState->AcqConfig.i64TriggerTimeout, (uInt32) m_pCardState->AcqConfig.i64SampleRate ) );
//		SetTriggerTimeout();

		CsSetDataFormat(formatDefault);
		m_pCardState->AcqConfig.u32TrigEnginesEn = DEERE_DEFAULT_TRIG_ENGINES_EN;
		m_pCardState->AcqConfig.i64TriggerDelay = DEERE_DEFAULT_TRIG_DELAY;
		m_pCardState->AcqConfig.i64TriggerHoldoff = DEERE_DEFAULT_TRIG_HOLD_OFF;
		m_pCardState->AcqConfig.u32SegmentCount = DEERE_DEFAULT_SEGMENT_COUNT;
		m_pCardState->AcqConfig.i64Depth = DEERE_DEFAULT_DEPTH;
		m_pCardState->AcqConfig.i64SegmentSize	= m_pCardState->AcqConfig.i64Depth + m_pCardState->AcqConfig.i64TriggerHoldoff;
		
		m_u32HwSegmentCount		= m_pCardState->AcqConfig.u32SegmentCount;
		m_i64HwSegmentSize		= m_pCardState->AcqConfig.i64SegmentSize;
		m_i64HwPostDepth		= m_pCardState->AcqConfig.i64Depth;
		m_i64HwTriggerHoldoff	= m_pCardState->AcqConfig.i64TriggerHoldoff;

		// Adjust Segemnt Parameters for Hardware
		AdjustedHwSegmentParameters( m_pCardState->AcqConfig.u32Mode, &m_u32HwSegmentCount, &m_i64HwSegmentSize, &m_i64HwPostDepth );

		CsDeereDevice* pDevice = m_MasterIndependent;
		while (pDevice)
		{
			i32Ret = pDevice->SendSegmentSetting (m_u32HwSegmentCount, m_i64HwPostDepth, m_i64HwSegmentSize, m_i64HwTriggerHoldoff);
			if( CS_FAILED(i32Ret) )	return i32Ret;

			i32Ret = SendTimeStampMode(false, false);
			if( CS_FAILED(i32Ret) )	return i32Ret;

			pDevice = pDevice->m_Next;
		}

	}

	// Inform kernel driver about acquisition config
	CSACQUISITIONCONFIG_EX AcqCfgEx = {0};
	CsGetAcquisitionConfig( &AcqCfgEx.AcqCfg );

	AcqCfgEx.i64HwDepth			= m_i64HwPostDepth;
	AcqCfgEx.i64HwSegmentSize	= m_i64HwSegmentSize;
	AcqCfgEx.u32Decimation		= m_pCardState->AddOnReg.u32Decimation;
	AcqCfgEx.u32ExpertOption	= m_pCsBoard->u32BaseBoardOptions[m_pCardState->u8ImageId ];
	AcqCfgEx.bSquareRec			= m_bSquareRec;
	IoctlSetAcquisitionConfigEx( &AcqCfgEx );

	return i32Ret;
}


//-----------------------------------------------------------------------------
//CsSetChannelConfig
//-----------------------------------------------------------------------------

int32	CsDeereDevice::CsSetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg, bool bResetDefaultParams )
{
	int32	i32Status = CS_SUCCESS;
	
	if( !GetInitStatus()->Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( pAChanCfg )
	{
		CsDeereDevice*   pDevice = (CsDeereDevice *)m_MasterIndependent;
		PCSCHANNELCONFIG pChanCfg = pAChanCfg->aChannel;

		for (uInt32 i = 0; i < pAChanCfg->u32ChannelCount; i++ )
		{
			uInt16 NormalizedChanIndex = CS_CHAN_1;

			if ( 2 == m_pCsBoard->NbAnalogChannel )
				NormalizedChanIndex = (pChanCfg[i].u32ChannelIndex % 2) ? CS_CHAN_1: CS_CHAN_2;

			pDevice = GetCardPointerFromChannelIndex( (uInt16) pChanCfg[i].u32ChannelIndex );
			if ( NULL != pDevice )
			{
				i32Status = pDevice->SendChannelSetting( NormalizedChanIndex, 
														 pChanCfg[i].u32InputRange,
														 pChanCfg[i].u32Term, 
														 pChanCfg[i].u32Impedance,
														 pChanCfg[i].i32DcOffset,
														 pChanCfg[i].u32Filter,
														 pChanCfg[i].i32Calib);
				if( CS_FAILED(i32Status) )
				{
					return i32Status;
				}
			}
		}
	}
	else
	{	
		if (bResetDefaultParams)
		{	
		i32Status = SendChannelSetting(CS_CHAN_1, m_u32DefaultRange);
		if ( CS_FAILED(i32Status) )	return i32Status;
	
		i32Status = SendCalibModeSetting(CS_CHAN_1, ecmCalModeNormal, 0);
		if ( CS_FAILED(i32Status) )	return i32Status;

		i32Status = SendChannelSetting(CS_CHAN_2, m_u32DefaultRange);
		if ( CS_FAILED(i32Status) )	return i32Status;

		i32Status = SendCalibModeSetting(CS_CHAN_2);
		if ( CS_FAILED(i32Status) )	return i32Status;
		}
		else
		{
			i32Status = SendCalibModeSetting(CS_CHAN_1, ecmCalModeNormal, 0);
			if ( CS_FAILED(i32Status) )	return i32Status;
			i32Status = SendCalibModeSetting(CS_CHAN_2);
			if ( CS_FAILED(i32Status) )	return i32Status;
		}
	}

	return i32Status;
}

//-----------------------------------------------------------------------------
//CsSetTriggerConfig
//-----------------------------------------------------------------------------

int32	CsDeereDevice::CsSetTriggerConfig( PARRAY_TRIGGERCONFIG pATrigCfg )
{
	int32	i32Status = CS_SUCCESS;
	uInt8	u8IndexTrig;
	uInt32	i;

	if( !GetInitStatus()->Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( pATrigCfg )
	{
		CsDeereDevice*	    pDevice = m_MasterIndependent;
		uInt16				u16NormalizedTrigIndex;
		int32				i32NormalizedTrigSource;
		PCSTRIGGERCONFIG	pTrigCfg = pATrigCfg->aTrigger;

		// Use m_CfgTrigParams for new configuration
		while (pDevice)
		{
			RtlZeroMemory( pDevice->m_CfgTrigParams, sizeof( m_CfgTrigParams ) );
			for ( i = 0; i < sizeof(m_pCardState->TriggerParams)/sizeof(CS_TRIGGER_PARAMS); i++ )
			{
				pDevice->m_CfgTrigParams[i].u32ExtTriggerRange	= m_u32DefaultExtTrigRange;
				pDevice->m_CfgTrigParams[i].u32ExtCoupling		= m_u32DefaultExtTrigCoupling;
				pDevice->m_CfgTrigParams[i].u32ExtImpedance		= m_u32DefaultExtTrigImpedance;
			}

			pDevice = pDevice->m_Next;
		}



		// First Pass, Remember all user params for Trigger Engines ENABLED
		for ( i = 0; i < pATrigCfg->u32TriggerCount; i++ )
		{
			pDevice = GetCardPointerFromTriggerSource( pTrigCfg[i].i32Source );
			if ( 0 != pDevice  )
			{
				i32NormalizedTrigSource = NormalizedTriggerSource( pTrigCfg[i].i32Source ); 

				i32Status = pDevice->GetFreeTriggerEngine( pTrigCfg[i].i32Source, &u16NormalizedTrigIndex );
				if ( CS_FAILED(i32Status) )	return i32Status;
				
				// Keep track of User Trigger Index
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32UserTrigIndex = pTrigCfg[i].u32TriggerIndex;

				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Source = i32NormalizedTrigSource;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32Condition = pTrigCfg[i].u32Condition;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Level = pTrigCfg[i].i32Level;

				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value1 = DEERE_DEFAULT_VALUE;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value2 = DEERE_DEFAULT_VALUE;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32Filter = DEERE_DEFAULT_VALUE;

				if ( CS_TRIG_SOURCE_EXT == i32NormalizedTrigSource )
				{
					pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtImpedance = pTrigCfg[i].u32ExtImpedance;
					pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtTriggerRange = pTrigCfg[i].u32ExtTriggerRange;
					pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtCoupling = pTrigCfg[i].u32ExtCoupling;
				}
				else
				{
					pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtTriggerRange = m_u32DefaultExtTrigRange;
					pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtCoupling = m_u32DefaultExtTrigCoupling;
					pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtImpedance = m_u32DefaultExtTrigImpedance;
				}
				//put the board always in or trigger for now
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32Relation = CS_RELATION_OR;
			}
		}

		pDevice = m_MasterIndependent;
		// Second Pass, Remember all user params for Trigger Engines DISABLED
		for ( i = 0; i < pATrigCfg->u32TriggerCount; i++ )
		{
			if ( CS_TRIG_SOURCE_DISABLE ==  pTrigCfg[i].i32Source )
			{
				while( pDevice )
				{
					i32NormalizedTrigSource = NormalizedTriggerSource( pTrigCfg[i].i32Source ); 
					if ( CS_SUCCESS == pDevice->GetFreeTriggerEngine( pTrigCfg[i].i32Source, &u16NormalizedTrigIndex ) )
					{
						// Keep track of User Trigger Index
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32UserTrigIndex = pTrigCfg[i].u32TriggerIndex;

						// Save all trigger setting for those sources DISABLED
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Source = CS_TRIG_SOURCE_DISABLE;
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32Condition = pTrigCfg[i].u32Condition,
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Level = pTrigCfg[i].i32Level;

						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value1 = DEERE_DEFAULT_VALUE;
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value2 = DEERE_DEFAULT_VALUE;
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32Filter = DEERE_DEFAULT_VALUE;

						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtTriggerRange = m_u32DefaultExtTrigRange;
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtCoupling = m_u32DefaultExtTrigCoupling;
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtImpedance = m_u32DefaultExtTrigImpedance;
						break;
					}
					else
						pDevice = pDevice->m_Next;
				}		
			}
		}
		// Third Pass, Set all Trigger Engines configurations
		pDevice = m_MasterIndependent;
		while( pDevice )
		{
			u8IndexTrig = INDEX_TRIGENGINE_A1;
			i32Status = pDevice->SendTriggerEngineSetting( m_bCh2Single ? CS_CHAN_2 : pDevice->m_CfgTrigParams[u8IndexTrig].i32Source,
														   pDevice->m_CfgTrigParams[u8IndexTrig].u32Condition,
                                                           pDevice->m_CfgTrigParams[u8IndexTrig].i32Level,
                                                           pDevice->m_CfgTrigParams[u8IndexTrig+1].i32Source,
                                                           pDevice->m_CfgTrigParams[u8IndexTrig+1].u32Condition,
                                                           pDevice->m_CfgTrigParams[u8IndexTrig+1].i32Level,
														   pDevice->m_CfgTrigParams[u8IndexTrig+2].i32Source,
														   pDevice->m_CfgTrigParams[u8IndexTrig+2].u32Condition,
                                                           pDevice->m_CfgTrigParams[u8IndexTrig+2].i32Level,
                                                           pDevice->m_CfgTrigParams[u8IndexTrig+3].i32Source,
                                                           pDevice->m_CfgTrigParams[u8IndexTrig+3].u32Condition,
                                                           pDevice->m_CfgTrigParams[u8IndexTrig+3].i32Level );

			if ( CS_FAILED(i32Status) )	return i32Status;

			i32Status = pDevice->SendExtTriggerSetting( CS_TRIG_SOURCE_DISABLE != pDevice->m_CfgTrigParams[INDEX_TRIGENGINE_EXT].i32Source,
											pDevice->m_CfgTrigParams[INDEX_TRIGENGINE_EXT].i32Level,
											pDevice->m_CfgTrigParams[INDEX_TRIGENGINE_EXT].u32Condition,
											pDevice->m_CfgTrigParams[INDEX_TRIGENGINE_EXT].u32ExtTriggerRange, 
											pDevice->m_CfgTrigParams[INDEX_TRIGENGINE_EXT].u32ExtImpedance,
											pDevice->m_CfgTrigParams[INDEX_TRIGENGINE_EXT].u32ExtCoupling );

			if ( CS_FAILED(i32Status) )	return i32Status;

			// Remember the user trigger index
			for ( i = 0; i < m_pCsBoard->NbTriggerMachine; i ++ )
				pDevice->m_pCardState->TriggerParams[i].u32UserTrigIndex = pDevice->m_CfgTrigParams[i].u32UserTrigIndex;

			pDevice = pDevice->m_Next;
		}
		return CS_SUCCESS;
	}
	else
	{
		RtlZeroMemory( m_pCardState->TriggerParams, sizeof(m_pCardState->TriggerParams) );

		for ( i = INDEX_TRIGENGINE_A1 ; i < m_pCsBoard->NbTriggerMachine; i ++ )
		{
			m_pCardState->TriggerParams[i].u32Relation = DEERE_DEFAULT_RELATION;
			m_pCardState->TriggerParams[i].i32Value1 = DEERE_DEFAULT_VALUE;
			m_pCardState->TriggerParams[i].i32Value2 = DEERE_DEFAULT_VALUE;
			m_pCardState->TriggerParams[i].u32Filter = DEERE_DEFAULT_VALUE;
			m_pCardState->TriggerParams[i].u32ExtTriggerRange = m_u32DefaultExtTrigRange;
			m_pCardState->TriggerParams[i].u32ExtCoupling = m_u32DefaultExtTrigCoupling;
			m_pCardState->TriggerParams[i].u32ExtImpedance = m_u32DefaultExtTrigImpedance;
			m_pCardState->TriggerParams[i].u32UserTrigIndex = i + m_pCsBoard->NbTriggerMachine*(m_pCardState->u16CardNumber-1);
		}
	
		// Default trigger from Channel 1 master
		u8IndexTrig = INDEX_TRIGENGINE_A1;

		if ( 1 == m_pCardState->u16CardNumber )
			m_pCardState->TriggerParams[u8IndexTrig].i32Source = 1;
		else 
			m_pCardState->TriggerParams[u8IndexTrig].i32Source = 0;

		i32Status = SendTriggerEngineSetting( m_pCardState->TriggerParams[u8IndexTrig].i32Source );
		if ( CS_FAILED(i32Status) )	return i32Status;

		m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].u32Relation = DEERE_DEFAULT_RELATION;
		m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].i32Value1 = DEERE_DEFAULT_VALUE;
		m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].i32Value2 = DEERE_DEFAULT_VALUE;
		m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].u32Filter = DEERE_DEFAULT_VALUE;
		m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].u32ExtTriggerRange = m_u32DefaultExtTrigRange;
		m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].u32ExtCoupling = m_u32DefaultExtTrigCoupling;
		m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].u32ExtImpedance = m_u32DefaultExtTrigImpedance;

		m_pCardState->TriggerParams[INDEX_TRIGENGINE_EXT].u32UserTrigIndex = m_pCsBoard->NbTriggerMachine*m_pCardState->u16CardNumber;

		m_pCardState->AcqConfig.u32TrigEnginesEn = DEERE_DEFAULT_TRIG_ENGINES_EN; // also in acquisition

		i32Status = SendExtTriggerSetting();
		if ( CS_FAILED(i32Status) )	return i32Status;
	
		RtlCopyMemory( m_CfgTrigParams, m_pCardState->TriggerParams, sizeof( m_CfgTrigParams ) );
	}

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

CsDeereDevice* CsDeereDevice::GetCardPointerFromTriggerSource( int32 i32TriggerSoure )
{
	CsDeereDevice* pDevice = m_MasterIndependent;

	if( NULL == pDevice )
		return NULL;

	if ( i32TriggerSoure == CS_TRIG_SOURCE_DISABLE )
	{
		// Cannot determine card pointer from trigger Source disbaled
		return NULL;
	}

	if ( i32TriggerSoure < 0 )	// External Trigger
	{
		while ( pDevice )
		{
			if ( pDevice->m_pCardState->u16CardNumber == Abs(i32TriggerSoure) )
				break;
			pDevice = pDevice->m_Next;
		}
	
		return pDevice;
	}
	else
	{	
		return GetCardPointerFromChannelIndex( (uInt16) i32TriggerSoure );
	}

}



//-----------------------------------------------------------------------------
//GetCardPointerFromTriggerIndex
//-----------------------------------------------------------------------------

CsDeereDevice* CsDeereDevice::GetCardPointerFromTriggerIndex( uInt16 TriggerIndex )
{
	uInt16 CardNumber;
	CsDeereDevice *pDevice = static_cast<CsDeereDevice *> (m_MasterIndependent);

	if( NULL == pDevice )
		return NULL;

	if( 0 == m_pCsBoard->NbTriggerMachine )
	{
		//Device was not fully initialized therefore no slaves
		return pDevice;
	}

	CardNumber = (uInt16) (TriggerIndex + m_pCsBoard->NbTriggerMachine - 1) / m_pCsBoard->NbTriggerMachine;

	while ( pDevice )
	{
		if (pDevice->m_pCardState->u16CardNumber == CardNumber)
			break;
		pDevice = pDevice->m_Next;
	}

	return pDevice;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CsDeereDevice* CsDeereDevice::GetCardPointerFromChannelIndex( uInt16 ChannelIndex )
{
	if ( ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE ) && 
		 (2==m_pCsBoard->NbAnalogChannel) &&
		 ( ChannelIndex % 2  == 0 ) )
	{
		// ChannelIndex should be odd in single mode on two channel boards
		return NULL;
	}

	if( 0 == m_pCsBoard->NbAnalogChannel )
	{
		//Device was not fully initialized therefore no slaves
		return this;
	}
	const uInt16 cu16CardNumber = (uInt16) (ChannelIndex + m_pCsBoard->NbAnalogChannel - 1) / m_pCsBoard->NbAnalogChannel;
	CsDeereDevice* pDevice = static_cast<CsDeereDevice *> (m_MasterIndependent);
	while ( pDevice )
	{
		if (pDevice->m_pCardState->u16CardNumber == cu16CardNumber)
			break;
		pDevice = pDevice->m_Next;
	}
	return pDevice;
}


//-----------------------------------------------------------------------------
//GetCardPointerFromBoardIndex
//-----------------------------------------------------------------------------

CsDeereDevice* CsDeereDevice::GetCardPointerFromBoardIndex( uInt16 BoardIndex )
{	
	CsDeereDevice* pDevice = m_MasterIndependent;

	while ( pDevice )
	{
		if (pDevice->m_pCardState->u16CardNumber == BoardIndex)
			break;
		pDevice = pDevice->m_Next;
	}
	
	return pDevice;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsDeereDevice::NormalizedChannelIndex( uInt16 u16ChannelIndex )
{
	if ( ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) == CS_MODE_DUAL )
		return	( u16ChannelIndex % 2) ? CS_CHAN_1: CS_CHAN_2;
	else
		return CS_CHAN_1;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDeereDevice::NormalizedTriggerSource( int32 i32TriggerSource )
{
	if ( i32TriggerSource ==  CS_TRIG_SOURCE_DISABLE )
		return CS_TRIG_SOURCE_DISABLE;
	else if ( i32TriggerSource < 0 )
	{
		return CS_TRIG_SOURCE_EXT;
	}
	else
	{
		if ( ( m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) == CS_MODE_DUAL )
			return	( i32TriggerSource % 2) ? CS_TRIG_SOURCE_CHAN_1: CS_TRIG_SOURCE_CHAN_2;
		else
			return CS_TRIG_SOURCE_CHAN_1;
	}
}

#if 0
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDeereDevice::SendTimeStampMode(bool bMclk, bool bFreeRun)
{
	uInt32 u32Data = _ReadRegister( GEN_COMMAND_R_REG );
	if(bMclk)
		u32Data |= TS_MCLK_SRC;
	else
		u32Data &= ~TS_MCLK_SRC;

	if(bFreeRun)
		u32Data |= TS_FREE_RUN;
	else
		u32Data &= ~TS_FREE_RUN;

	if( eAuxInTimestampReset == m_AuxIn  )
		u32Data |= TS_EXT_RESET;
	else
		u32Data &= ~TS_EXT_RESET;
	
	_WriteRegister(GEN_COMMAND_R_REG, u32Data);
	m_pCardState->AcqConfig.u32TimeStampMode = u32Data & (TS_FREE_RUN | TS_MCLK_SRC | TS_EXT_RESET);
	return CS_SUCCESS;
}
#endif