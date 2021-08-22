#include "StdAfx.h"
#include "CsSplenda.h"


//-----------------------------------------------------------------------------
//------ CsGetAcquisitionConfig
//-----------------------------------------------------------------------------

int32	CsSplendaDev::CsGetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg )
{
	// Old user struct size my be smaller than driver struct size
	uInt32		u32UserStructSize = pAcqCfg->u32Size;

	RtlCopyMemory( pAcqCfg, &m_pCardState->AcqConfig, pAcqCfg->u32Size ); 
	pAcqCfg->u32Size	= u32UserStructSize;
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//CsGetChannelConfig
//-----------------------------------------------------------------------------
int32	CsSplendaDev::CsGetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg )
{
	CsSplendaDev	*pDevice = this;
	uInt16			NormalizedChanIndex;
	PCSCHANNELCONFIG pChanCfg = pAChanCfg->aChannel;


	for (uInt32 i = 0; i < pAChanCfg->u32ChannelCount; i++ )
	{
		pChanCfg->u32Size = sizeof(CSCHANNELCONFIG);
		NormalizedChanIndex = NormalizedChannelIndex( (uInt16) pChanCfg[i].u32ChannelIndex );
		NormalizedChanIndex =  ConvertToHwChannel(NormalizedChanIndex) - 1;

		pDevice = GetCardPointerFromChannelIndex( (uInt16) pChanCfg[i].u32ChannelIndex );
		if ( NULL != pDevice )
		{
			pChanCfg[i].u32Term = pDevice->m_pCardState->ChannelParams[NormalizedChanIndex].u32Term;
			pChanCfg[i].u32InputRange = pDevice->m_pCardState->ChannelParams[NormalizedChanIndex].u32InputRange;
			pChanCfg[i].u32Impedance = pDevice->m_pCardState->ChannelParams[NormalizedChanIndex].u32Impedance;
			pChanCfg[i].u32Filter = pDevice->m_pCardState->ChannelParams[NormalizedChanIndex].u32Filter;
			pChanCfg[i].i32DcOffset = pDevice->m_pCardState->ChannelParams[NormalizedChanIndex].i32Position;
		}
	}
	
	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//CsGetTriggerConfig
//-----------------------------------------------------------------------------
int32	CsSplendaDev::CsGetTriggerConfig( PARRAY_TRIGGERCONFIG pATrigCfg )
{
	CsSplendaDev		*pDevice = m_MasterIndependent;
	uInt16				NormalizedTrigIndex = 0;
	PCSTRIGGERCONFIG	pTrigCfg = pATrigCfg->aTrigger;
	BOOLEAN				bFoundTrigIndex = FALSE;

	for (uInt32 i = 0; i < pATrigCfg->u32TriggerCount; i++ )
	{

		// Searching for the user trigger index
		pDevice = m_MasterIndependent;
		while( pDevice )
		{
			for ( uInt16 j = 0; j < CSPLNDA_TOTAL_TRIGENGINES; j++ )
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
					pTrigCfg[i].i32Source = pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].i32Source + m_PlxData->CsBoard.NbAnalogChannel*(pDevice->m_pCardState->u16CardNumber - 1);

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
int32	CsSplendaDev::CsGetBoardsInfo( PARRAY_BOARDINFO pABoardInfo )
{
	CsSplendaDev	*pDevice;
	PCSBOARDINFO	BoardInfo = pABoardInfo->aBoardInfo;
	PCS_BOARD_NODE	pDrvBdInfo;


	for (uInt32 i = 0; i < pABoardInfo->u32BoardCount; i++ )
	{
		BoardInfo[i].u32Size = sizeof(CSBOARDINFO);
		BoardInfo[i].u32BoardIndex = (uInt16) (i + 1);

		if ( NULL != (pDevice = GetCardPointerFromBoardIndex( (uInt16) BoardInfo[i].u32BoardIndex ) ) )
		{
			uInt32	u32FwVersion;

			BoardInfo[i].u32BoardType = m_pSystem->SysInfo.u32BoardType;
			pDrvBdInfo = &pDevice->m_PlxData->CsBoard;

			sprintf_s(BoardInfo[i].strSerialNumber, sizeof(BoardInfo[i].strSerialNumber),
								TEXT("%08x/%08x"), pDrvBdInfo->u32SerNum, pDrvBdInfo->u32SerNumEx);
			BoardInfo[i].u32BaseBoardVersion = pDrvBdInfo->u32BaseBoardVersion;

			// Conversion of the firmware version to the standard  xx.xx.xx format
			BoardInfo[i].u32BaseBoardFirmwareVersion = ((pDrvBdInfo->u32RawFwVersionB[0]  & 0xFF00) << 8) |
														((pDrvBdInfo->u32RawFwVersionB[0] & 0xF0) << 4) |
														(pDrvBdInfo->u32RawFwVersionB[0] & 0xF);

			BoardInfo[i].u32AddonBoardVersion = pDrvBdInfo->u32AddonBoardVersion;

			// Conversion of the firmware version to to the standard xx.xx.xx format
			u32FwVersion = pDrvBdInfo->u32RawFwVersionA[0] >> 16;
			BoardInfo[i].u32AddonBoardFirmwareVersion = ((u32FwVersion  & 0x7f00) << 8) |
														((u32FwVersion & 0xF0) << 4) |
														(u32FwVersion & 0xF);

			BoardInfo[i].u32AddonFwOptions		= pDrvBdInfo->u32ActiveAddonOptions;
			BoardInfo[i].u32BaseBoardFwOptions	= pDrvBdInfo->u32BaseBoardOptions[0];	
			BoardInfo[i].u32AddonHwOptions		= pDrvBdInfo->u32AddonHardwareOptions;
			BoardInfo[i].u32BaseBoardHwOptions	= pDrvBdInfo->u32BaseBoardHardwareOptions;	
		}
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//CsSetAcquisitionConfig
//-----------------------------------------------------------------------------

int32	CsSplendaDev::CsSetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg )
{
	int32	i32Ret = CS_SUCCESS;


	if( !m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( pAcqCfg )
	{
		m_pCardState->AcqConfig.u32TrigEnginesEn	= pAcqCfg->u32TrigEnginesEn;
		m_pCardState->AcqConfig.i64TriggerDelay		= pAcqCfg->i64TriggerDelay;
		m_pCardState->AcqConfig.i64TriggerHoldoff	= pAcqCfg->i64TriggerHoldoff;
		m_pCardState->AcqConfig.u32SegmentCount		= pAcqCfg->u32SegmentCount;
		m_pCardState->AcqConfig.i64Depth			= pAcqCfg->i64Depth;
		m_pCardState->AcqConfig.u32TimeStampConfig  = pAcqCfg->u32TimeStampConfig;
		m_pCardState->AcqConfig.i64SegmentSize		= pAcqCfg->i64SegmentSize;
		
		m_pCardState->AcqConfig.u32ExtClkSampleSkip	= CS_SAMPLESKIP_FACTOR;

		// Load appropriate FPGA image depending on the mode
		uInt8	u8LoadImageId = 0;
		if ( (pAcqCfg->u32Mode & CS_MODE_USER1) != 0 )
			u8LoadImageId = 1;
		else if ( (pAcqCfg->u32Mode & CS_MODE_USER2) != 0 )
			u8LoadImageId = 2;
		else
			u8LoadImageId = 0;

		m_DelayLoadImageId = u8LoadImageId;

		// Some algorithm relies on the order of function calls
		// Do not change the order of
		// SendClockSetting
		// SendCaptureModeSetting

		i32Ret = SendClockSetting( pAcqCfg->i64SampleRate,
						           pAcqCfg->u32ExtClk, pAcqCfg->u32ExtClkSampleSkip, 
								   (pAcqCfg->u32Mode & CS_MODE_REFERENCE_CLK) != 0 );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = SendMasterSlaveSetting();
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = ResetMasterSlave();
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = SendCaptureModeSetting( pAcqCfg->u32Mode );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		// Convert TriggerTimeout from 100 ns to number of memory locations
		// Each memory location contains respectively 8 (Single), 2x4 (Dual), 4x2 (Quad) and 8x1 (Octal) Samples
		m_pCardState->AcqConfig.i64TriggerTimeout = pAcqCfg->i64TriggerTimeout;
		m_u32HwTrigTimeout = (uInt32) pAcqCfg->i64TriggerTimeout;
		if( CS_TIMEOUT_DISABLE != m_u32HwTrigTimeout && 0 != m_u32HwTrigTimeout)
		{
			LONGLONG llTimeout = (LONGLONG)m_u32HwTrigTimeout * pAcqCfg->i64SampleRate;
			llTimeout *= (m_pCardState->AcqConfig.u32Mode & CS_MASKED_MODE);
			llTimeout /= 8;
			// llTimeout was calculated using 100ns unit trigger timeout. Convert back to 1 second unit
			llTimeout += 9999999;
			llTimeout /= 10000000; 

			if ( llTimeout >= MAX_HW_DEPTH_COUNTER )
				m_pCardState->AcqConfig.i64TriggerTimeout = (int32) (m_u32HwTrigTimeout = (uInt32) CS_TIMEOUT_DISABLE);
			else
				m_u32HwTrigTimeout = (uInt32)llTimeout;
		}
		
		SetTriggerTimeout (m_u32HwTrigTimeout);

		uInt32 u32HwSegmentCount  = pAcqCfg->u32SegmentCount;
		int64 i64HwSegmentSize    = pAcqCfg->i64SegmentSize;
		int64 i64HwPostDepth      = pAcqCfg->i64Depth;
		int64 i64HwTrigDelay      = pAcqCfg->i64TriggerDelay;
		int64 i64HwTriggerHoldoff = pAcqCfg->i64TriggerHoldoff;

		// Adjust Segemnt Parameters for Hardware
		AdjustedHwSegmentParameters( m_pCardState->AcqConfig.u32Mode, &u32HwSegmentCount, &i64HwSegmentSize, &i64HwPostDepth );

		CsSplendaDev	*pDevice = m_MasterIndependent;
		while (pDevice)
		{
			i32Ret = pDevice->SendSegmentSetting (u32HwSegmentCount, i64HwPostDepth, i64HwSegmentSize, i64HwTriggerHoldoff, i64HwTrigDelay);
			if( CS_FAILED(i32Ret) )
				return i32Ret;

			i32Ret = pDevice->SendTimeStampMode( ((pAcqCfg->u32TimeStampConfig & TIMESTAMP_MCLK) != 0),
					                             ((pAcqCfg->u32TimeStampConfig & TIMESTAMP_FREERUN) != 0) );

			if( CS_FAILED(i32Ret) )
				return i32Ret;
								
			pDevice = pDevice->m_Next;
		}

		if ( !m_bMulrecAvgTD && (( CS_MODE_SW_AVERAGING & pAcqCfg->u32Mode ) != 0 ) )
		{
			m_bSoftwareAveraging = TRUE;
		}
		else
            m_bSoftwareAveraging = FALSE;

		if ( m_bMulrecAvgTD || m_bSoftwareAveraging )
		{
			if ( NULL == m_pAverageBuffer )
			{
				uInt32	u32MaxAverageSamples = 0;
				if ( m_bMulrecAvgTD )
					u32MaxAverageSamples = m_u32MaxHwAvgDepth + AV_ENSURE_POSTTRIG;
				else
					u32MaxAverageSamples = MAX_SW_AVERAGING_SEMGENTSIZE + SW_AVERAGING_BUFFER_PADDING;

				uInt32	u32MaxAverageBufferSize = u32MaxAverageSamples * ( m_bMulrecAvgTD ?  sizeof(int32) : sizeof(int16) );

				m_pAverageBuffer = (int16 *)::GageAllocateMemoryX( u32MaxAverageBufferSize );
				if( NULL == m_pAverageBuffer )
				{
					return CS_INSUFFICIENT_RESOURCES;
				}
			}

			if ( m_bMulrecAvgTD )
				SetupHwAverageMode();
		}
		else if ( NULL != m_pAverageBuffer  )
		{
			::GageFreeMemoryX( m_pAverageBuffer );
			m_pAverageBuffer = NULL;
		}

		if ( m_bNucleon && m_Stream.bEnabled )
			PreCalculateStreamDataSize( pAcqCfg );
	}
	else
	{
		// Default setting
		m_pCardState->AcqConfig.u32ExtClk = 0;
		m_pCardState->AcqConfig.u32ExtClkSampleSkip	= 0;
		m_FirConfig.bEnable = 0;
		m_u32MulrecAvgCount	= 1;
		m_bSoftwareAveraging = FALSE;

		// Release average buffer if there is any
		if ( m_pAverageBuffer )
		{
			::GageFreeMemoryX( m_pAverageBuffer );
			m_pAverageBuffer = NULL;
		}

		// Restore the default FPGA image
		CsLoadBaseBoardFpgaImage( 0 );

		i32Ret = SendClockSetting();
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = SendMasterSlaveSetting();
		if( CS_FAILED(i32Ret) )	return i32Ret;
		
		i32Ret = ResetMasterSlave();
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = SendCaptureModeSetting(  );
		if( CS_FAILED(i32Ret) )	return i32Ret;

		SetTriggerTimeout(m_u32HwTrigTimeout = (uInt32)CS_TIMEOUT_DISABLE);
		CsSetDataFormat();

		m_pCardState->AcqConfig.i64TriggerTimeout	= CS_TIMEOUT_DISABLE;
		m_pCardState->AcqConfig.u32TrigEnginesEn	= CSPLNDA_DEFAULT_TRIG_ENGINES_EN;
		m_pCardState->AcqConfig.i64TriggerDelay		= CSPLNDA_DEFAULT_TRIG_DELAY;
		m_pCardState->AcqConfig.i64TriggerHoldoff	= CSPLNDA_DEFAULT_TRIG_HOLD_OFF;
		m_pCardState->AcqConfig.u32SegmentCount		= CSPLNDA_DEFAULT_SEGMENT_COUNT;
		m_pCardState->AcqConfig.i64Depth			= CSPLNDA_DEFAULT_DEPTH;
		m_pCardState->AcqConfig.i64SegmentSize		= m_pCardState->AcqConfig.i64Depth + m_pCardState->AcqConfig.i64TriggerHoldoff;
		
		m_u32HwSegmentCount		= m_pCardState->AcqConfig.u32SegmentCount;
		m_i64HwSegmentSize		= m_pCardState->AcqConfig.i64SegmentSize;
		m_i64HwPostDepth		= m_pCardState->AcqConfig.i64Depth;
		m_i64HwTriggerHoldoff	= m_pCardState->AcqConfig.i64TriggerHoldoff;

		// Adjust Segemnt Parameters for Hardware
		AdjustedHwSegmentParameters( m_pCardState->AcqConfig.u32Mode, &m_u32HwSegmentCount, &m_i64HwSegmentSize,
									 &m_i64HwPostDepth );

		CsSplendaDev* pDevice = m_MasterIndependent;
		while (pDevice)
		{
			i32Ret = pDevice->SendSegmentSetting (m_u32HwSegmentCount, m_i64HwPostDepth, m_i64HwSegmentSize, m_i64HwTriggerHoldoff);
			if( CS_FAILED(i32Ret) )
				return i32Ret;

			i32Ret = SendTimeStampMode(false, false);
			if( CS_FAILED(i32Ret) )
				return i32Ret;

			pDevice = pDevice->m_Next;
		}
	}

	// Disable all trigger engines to avoid trigger loss during channel configuration
	// Trigger engines will be enable in SetTrigger configuration later
	CsDisableAllTriggers();

	CSACQUISITIONCONFIG_EX AcqCfgEx = {0};
	AcqCfgEx.AcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG); 
	CsGetAcquisitionConfig( &AcqCfgEx.AcqCfg );

	AcqCfgEx.i64HwDepth			= m_i64HwPostDepth;
	AcqCfgEx.i64HwSegmentSize	= m_i64HwSegmentSize;
	AcqCfgEx.u32Decimation		= m_pCardState->AddOnReg.u32Decimation;
	AcqCfgEx.u32ExpertOption	= m_PlxData->CsBoard.u32BaseBoardOptions[m_pCardState->u8ImageId];

	IoctlSetAcquisitionConfigEx( &AcqCfgEx ) ;

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CsSplendaDev *CsSplendaDev::GetCardPointerFromChannelIndex( uInt16 u16ChannelIndex )
{
	CsSplendaDev	*pDevice = static_cast<CsSplendaDev *> (m_MasterIndependent);
	uInt16			u16CardNumber;


	if( 0 == m_PlxData->CsBoard.NbAnalogChannel )
	{
		//Device was not fully initialized therefore no slaves
		return pDevice;
	}
	
	if ( ! IsChannelIndexValid( u16ChannelIndex ) )
		return NULL;

	u16CardNumber = (uInt16) (u16ChannelIndex + m_PlxData->CsBoard.NbAnalogChannel - 1) / m_PlxData->CsBoard.NbAnalogChannel;

	while ( pDevice )
	{
		if (pDevice->m_pCardState->u16CardNumber == u16CardNumber)
			break;
		pDevice = pDevice->m_Next;
	}
	
	return pDevice;
}

//-----------------------------------------------------------------------------
//CsSetChannelConfig
//-----------------------------------------------------------------------------

int32	CsSplendaDev::CsSetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg )
{
	int32				i32Status = CS_SUCCESS;
	
	if( !m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( pAChanCfg )
	{
		CsSplendaDev			*pDevice = (CsSplendaDev *)m_MasterIndependent;
		PCSCHANNELCONFIG	pChanCfg = pAChanCfg->aChannel;
		uInt16 u16NormalizedChanIndex;

		for (uInt32 i = 0; i < pAChanCfg->u32ChannelCount; i++ )
		{
			u16NormalizedChanIndex =  NormalizedChannelIndex( (uInt16) pChanCfg[i].u32ChannelIndex );
			u16NormalizedChanIndex = ConvertToHwChannel( u16NormalizedChanIndex );

			pDevice = GetCardPointerFromChannelIndex( (uInt16) pChanCfg[i].u32ChannelIndex );
			if ( pDevice )
			{
				i32Status = pDevice->SendChannelSetting( u16NormalizedChanIndex, 
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
		for ( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i++ )
		{
			i32Status = SendAdcOffsetAjust(ConvertToHwChannel(i), m_u16AdcOffsetAdjust );
			if( CS_FAILED(i32Status) )	return i32Status;

			i32Status = SendChannelSetting(ConvertToHwChannel(i), GetDefaultRange() );
			if ( CS_FAILED(i32Status) )	return i32Status;
		}

	}

	return i32Status;
}


//-----------------------------------------------------------------------------
//CsSetTriggerConfig
//-----------------------------------------------------------------------------

int32	CsSplendaDev::CsSetTriggerConfig( PARRAY_TRIGGERCONFIG pATrigCfg )
{
	int32	i32Status = CS_SUCCESS;
	uInt16	u16HardwareChannel;
	uInt8	u8IndexTrig;
	uInt32	i;


	if( !m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( pATrigCfg )
	{
		CsSplendaDev			*pDevice = m_MasterIndependent;
		uInt16				u16NormalizedTrigIndex;
		int32				i32NormalizedTrigSource;
		PCSTRIGGERCONFIG	pTrigCfg = pATrigCfg->aTrigger;

		// Use m_pCardState->TriggerParams for new configuration
		while (pDevice)
		{
			GageZeroMemoryX( pDevice->m_CfgTrigParams, sizeof( m_CfgTrigParams ) );
			pDevice = pDevice->m_Next;
		}

		// First Pass, Remember all user params for Trigger Engines ENABLED
		for ( i = 0; i < pATrigCfg->u32TriggerCount; i++ )
		{
			if ( 0 != (pDevice = GetCardPointerFromTriggerSource( pTrigCfg[i].i32Source )) )
			{
				i32NormalizedTrigSource = NormalizedTriggerSource( pTrigCfg[i].i32Source ); 
				i32Status = pDevice->GetFreeTriggerEngine( i32NormalizedTrigSource, &u16NormalizedTrigIndex );
				if ( CS_FAILED(i32Status) )
					return i32Status;
				
				// Keep track of User Trigger Index
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32UserTrigIndex = pTrigCfg[i].u32TriggerIndex;

				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Source = i32NormalizedTrigSource;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32Condition = pTrigCfg[i].u32Condition;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Level = pTrigCfg[i].i32Level;

				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value1 = CSPLNDA_DEFAULT_VALUE;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value2 = CSPLNDA_DEFAULT_VALUE;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32Filter = CSPLNDA_DEFAULT_VALUE;

				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtImpedance = pTrigCfg[i].u32ExtImpedance;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtTriggerRange = pTrigCfg[i].u32ExtTriggerRange;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtCoupling = pTrigCfg[i].u32ExtCoupling;

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
					if ( CS_SUCCESS == pDevice->GetFreeTriggerEngine( pTrigCfg[i].i32Source, &u16NormalizedTrigIndex ) )
					{
						// Keep track of User Trigger Index
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32UserTrigIndex = pTrigCfg[i].u32TriggerIndex;

						// Save all trigger setting for those sources DISABLED
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Source = CS_TRIG_SOURCE_DISABLE;
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32Condition = pTrigCfg[i].u32Condition,
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Level = pTrigCfg[i].i32Level;

						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value1 = CSPLNDA_DEFAULT_VALUE;
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value2 = CSPLNDA_DEFAULT_VALUE;
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32Filter = CSPLNDA_DEFAULT_VALUE;

						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtImpedance = pTrigCfg[i].u32ExtImpedance;
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtTriggerRange = pTrigCfg[i].u32ExtTriggerRange;
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtCoupling = pTrigCfg[i].u32ExtCoupling;
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
			for ( i = 0; i < m_PlxData->CsBoard.NbAnalogChannel; i++ )
			{
				u16HardwareChannel = ConvertToHwChannel((uInt16)(i+1));
				u8IndexTrig = (uInt8) (2*(u16HardwareChannel-1) + 1);

				i32Status = pDevice->SendTriggerEngineSetting( u16HardwareChannel,
																pDevice->m_CfgTrigParams[u8IndexTrig].i32Source,
																pDevice->m_CfgTrigParams[u8IndexTrig].u32Condition,
																pDevice->m_CfgTrigParams[u8IndexTrig].i32Level,
																pDevice->m_CfgTrigParams[u8IndexTrig+1].i32Source,
																pDevice->m_CfgTrigParams[u8IndexTrig+1].u32Condition,
																pDevice->m_CfgTrigParams[u8IndexTrig+1].i32Level );
				if ( CS_FAILED(i32Status) )
					return i32Status;
			}

			pDevice->SendExtTriggerSetting( (BOOLEAN) pDevice->m_CfgTrigParams[CSPLNDA_TRIGENGINE_EXT].i32Source,
											eteAlways,
											pDevice->m_CfgTrigParams[CSPLNDA_TRIGENGINE_EXT].i32Level,
											pDevice->m_CfgTrigParams[CSPLNDA_TRIGENGINE_EXT].u32Condition,
											pDevice->m_CfgTrigParams[CSPLNDA_TRIGENGINE_EXT].u32ExtTriggerRange, 
											pDevice->m_CfgTrigParams[CSPLNDA_TRIGENGINE_EXT].u32ExtCoupling );

			if ( CS_FAILED(i32Status) )
				return i32Status;

			i32Status = pDevice->SendTriggerRelation( CSPLNDA_DEFAULT_RELATION );
			if ( CS_FAILED(i32Status) )
				return i32Status;

			// Update Ext trigger params for all trigger engines
			for(  i = 1; i < CSPLNDA_TOTAL_TRIGENGINES; i++ )
			{
				pDevice->m_pCardState->TriggerParams[i].u32ExtTriggerRange = pDevice->m_CfgTrigParams[CSPLNDA_TRIGENGINE_EXT].u32ExtTriggerRange;
				pDevice->m_pCardState->TriggerParams[i].u32ExtCoupling = pDevice->m_CfgTrigParams[CSPLNDA_TRIGENGINE_EXT].u32ExtCoupling;
				pDevice->m_pCardState->TriggerParams[i].u32ExtImpedance = pDevice->m_CfgTrigParams[CSPLNDA_TRIGENGINE_EXT].u32ExtImpedance;
			}

			// Remember the user trigger index
			for ( i = 0; i < CSPLNDA_TOTAL_TRIGENGINES; i ++ )
			{
				pDevice->m_pCardState->TriggerParams[i].u32UserTrigIndex = pDevice->m_CfgTrigParams[i].u32UserTrigIndex;
			}

			pDevice = pDevice->m_Next;
		}

		return CS_SUCCESS;
	}
	else
	{
		uInt32	u32UserTrigIndex;

		RtlZeroMemory( m_pCardState->TriggerParams, sizeof(m_pCardState->TriggerParams) );
		for ( i = 0; i < m_PlxData->CsBoard.NbAnalogChannel; i++ )
		{
			u16HardwareChannel = ConvertToHwChannel((uInt16)(i+1));
			u8IndexTrig = (uInt8) (2*(u16HardwareChannel-1) + 1);
			u32UserTrigIndex = (uInt8) (2*i + 1);

			// Engine A
			m_pCardState->TriggerParams[u8IndexTrig].u32Relation = CSPLNDA_DEFAULT_RELATION;
			m_pCardState->TriggerParams[u8IndexTrig].i32Value1 = CSPLNDA_DEFAULT_VALUE;
			m_pCardState->TriggerParams[u8IndexTrig].i32Value2 = CSPLNDA_DEFAULT_VALUE;
			m_pCardState->TriggerParams[u8IndexTrig].u32Filter = CSPLNDA_DEFAULT_VALUE;
			m_pCardState->TriggerParams[u8IndexTrig].u32ExtTriggerRange = CSPLNDA_DEFAULT_EXT_GAIN;
			m_pCardState->TriggerParams[u8IndexTrig].u32ExtCoupling = CSPLNDA_DEFAULT_EXT_COUPLING;
			m_pCardState->TriggerParams[u8IndexTrig].u32ExtImpedance = CSPLNDA_DEFAULT_EXT_IMPEDANCE;
			m_pCardState->TriggerParams[u8IndexTrig].u32UserTrigIndex = u32UserTrigIndex + 2*m_PlxData->CsBoard.NbAnalogChannel*(m_pCardState->u16CardNumber-1);

			// Engine B
			m_pCardState->TriggerParams[u8IndexTrig+1].u32Relation = CSPLNDA_DEFAULT_RELATION;
			m_pCardState->TriggerParams[u8IndexTrig+1].i32Value1 = CSPLNDA_DEFAULT_VALUE;
			m_pCardState->TriggerParams[u8IndexTrig+1].i32Value2 = CSPLNDA_DEFAULT_VALUE;
			m_pCardState->TriggerParams[u8IndexTrig+1].u32Filter = CSPLNDA_DEFAULT_VALUE;
			m_pCardState->TriggerParams[u8IndexTrig+1].u32ExtTriggerRange = CSPLNDA_DEFAULT_EXT_GAIN;
			m_pCardState->TriggerParams[u8IndexTrig+1].u32ExtCoupling = CSPLNDA_DEFAULT_EXT_COUPLING;
			m_pCardState->TriggerParams[u8IndexTrig+1].u32ExtImpedance = CSPLNDA_DEFAULT_EXT_IMPEDANCE;
			m_pCardState->TriggerParams[u8IndexTrig+1].u32UserTrigIndex = (u32UserTrigIndex + 1) + 2*m_PlxData->CsBoard.NbAnalogChannel*(m_pCardState->u16CardNumber-1);

			// Default trigger from Channel 1 master
			if ( 1 == u8IndexTrig && 1 == m_pCardState->u16CardNumber )
				m_pCardState->TriggerParams[u8IndexTrig].i32Source = 1;

			i32Status = SendTriggerEngineSetting( u16HardwareChannel,
												  m_pCardState->TriggerParams[u8IndexTrig].i32Source );													
			if ( CS_FAILED(i32Status) )
				return i32Status;
		}

		m_pCardState->TriggerParams[CSPLNDA_TRIGENGINE_EXT].u32Relation = CSPLNDA_DEFAULT_RELATION;
		m_pCardState->TriggerParams[CSPLNDA_TRIGENGINE_EXT].i32Value1 = CSPLNDA_DEFAULT_VALUE;
		m_pCardState->TriggerParams[CSPLNDA_TRIGENGINE_EXT].i32Value2 = CSPLNDA_DEFAULT_VALUE;
		m_pCardState->TriggerParams[CSPLNDA_TRIGENGINE_EXT].u32Filter = CSPLNDA_DEFAULT_VALUE;
		m_pCardState->TriggerParams[CSPLNDA_TRIGENGINE_EXT].u32ExtTriggerRange = CSPLNDA_DEFAULT_EXT_GAIN;
		m_pCardState->TriggerParams[CSPLNDA_TRIGENGINE_EXT].u32ExtCoupling = CSPLNDA_DEFAULT_EXT_COUPLING;
		m_pCardState->TriggerParams[CSPLNDA_TRIGENGINE_EXT].u32ExtImpedance = CSPLNDA_DEFAULT_EXT_IMPEDANCE;
		m_pCardState->TriggerParams[CSPLNDA_TRIGENGINE_EXT].u32UserTrigIndex = m_PlxData->CsBoard.NbTriggerMachine*m_pCardState->u16CardNumber;
		m_pCardState->AcqConfig.u32TrigEnginesEn = CSPLNDA_DEFAULT_TRIG_ENGINES_EN; // also in acquisition

		i32Status = SendExtTriggerSetting();
		if ( CS_FAILED(i32Status) )
			return i32Status;
	
		i32Status = SendTriggerRelation( CSPLNDA_DEFAULT_RELATION	);
		if ( CS_FAILED(i32Status) )
			return i32Status;
	}

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

CsSplendaDev *CsSplendaDev::GetCardPointerFromTriggerSource( int32 i32TriggerSoure )
{
	CsSplendaDev *pDevice = static_cast<CsSplendaDev *> (m_MasterIndependent);

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

CsSplendaDev *CsSplendaDev::GetCardPointerFromTriggerIndex( uInt16 TriggerIndex )
{
	uInt16 CardNumber;
	CsSplendaDev *pDevice = static_cast<CsSplendaDev *> (m_MasterIndependent);

	if( NULL == pDevice )
		return NULL;

	if( 0 == m_PlxData->CsBoard.NbTriggerMachine )
	{
		//Device was not fully initialized therefore no slaves
		return pDevice;
	}

	CardNumber = (uInt16) (TriggerIndex + m_PlxData->CsBoard.NbTriggerMachine - 1) / m_PlxData->CsBoard.NbTriggerMachine;

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

int32	CsSplendaDev::CommitNewConfiguration( PCSACQUISITIONCONFIG pAcqCfg,
											   	PARRAY_CHANNELCONFIG pAChanCfg,
                                                PARRAY_TRIGGERCONFIG pATrigCfg )
{
	int32					i32Status = CS_SUCCESS;
	CsSplendaDev			*pDevice = this;
	int32					i32CalResult(CS_SUCCESS);


	m_bExtraDelayNeeded = false;
	if(NULL == m_pTriggerMaster || NULL == m_MasterIndependent)
		i32Status = CsSetAcquisitionConfig( pAcqCfg );
	else
	{
		pDevice = m_MasterIndependent;
		while (pDevice)
		{
			if( pDevice != m_pTriggerMaster )
			{
				i32Status = pDevice->CsSetAcquisitionConfig( pAcqCfg );
				if( CS_FAILED(i32Status) )
					break;
			}		
			pDevice = pDevice->m_Next;
		}

		if( CS_SUCCEEDED (i32Status) )
			i32Status = m_pTriggerMaster->CsSetAcquisitionConfig( pAcqCfg );
	}

	if( CS_FAILED(i32Status) )
		return i32Status;

	i32Status = CsSetChannelConfig( (PARRAY_CHANNELCONFIG) pAChanCfg );
	if( CS_FAILED(i32Status) )
		return i32Status;


	i32Status = CsSetTriggerConfig( (PARRAY_TRIGGERCONFIG) pATrigCfg );

	// Detect if calibration for at least one channel is required.
	bool	bRequiredCalibrate = false;

	pDevice = m_MasterIndependent;
	while (pDevice != NULL)
	{
		if ( pDevice->IsCalibrationRequired() )
		{
			bRequiredCalibrate = TRUE;
			break;
		}
		pDevice = pDevice->m_Next;
	}

	if ( bRequiredCalibrate )
	{
		// CALIBRATION IS REQUIRED
		if ( 0 != m_pCardState->u8ImageId )
		{
			// ONE OF EXPERT IMAGES THE CURRENT ACTIVE IMAGE
			// switch to the primary image before calibration
			pDevice = m_MasterIndependent;
			while (pDevice != NULL)
			{
				i32Status = CsLoadBaseBoardFpgaImage( 0 );
				if( CS_FAILED(i32Status) )
					break;
				pDevice = pDevice->m_Next;
			}

			pDevice = m_MasterIndependent;
			while (pDevice != NULL)
			{
				pDevice->CsSetDataFormat( formatDefault );

				i32Status = pDevice->CsSetAcquisitionConfig( pAcqCfg );
				if( CS_FAILED(i32Status) )
					break;

				i32Status = pDevice->CsSetChannelConfig( (PARRAY_CHANNELCONFIG) pAChanCfg );
				if( CS_FAILED(i32Status) )
					break;

				i32Status = pDevice->CsSetTriggerConfig( (PARRAY_TRIGGERCONFIG) pATrigCfg );
				if( CS_FAILED(i32Status) )
					break;

				pDevice = pDevice->m_Next;
			}
		}

		if ( m_bExtraDelayNeeded )
			Sleep(400);

		// IMAGE 0 IS THE CURRENT ACITVE IMAGE
		MsClearChannelProtectionStatus();
		i32Status = MsCalibrateAllChannels();
		if( CS_FAILED(i32Status))
			i32CalResult = i32Status;
	
		m_MasterIndependent->ResyncClock();

		// Reset M/S in order to minimize M/S skew.
		// Without this, we may have M/S skew bigger than normal after calibration
		if ( 0 != m_MasterIndependent->m_Next )
			m_MasterIndependent->ResetMasterSlave();
	}


	if ( (m_pCardState->u8ImageId != m_DelayLoadImageId) )
	{
		pDevice = m_MasterIndependent;
		while (pDevice != NULL)
		{
			i32Status = pDevice->CsLoadBaseBoardFpgaImage( m_DelayLoadImageId );
			if( CS_FAILED(i32Status) )break;
			pDevice = pDevice->m_Next;
		}
		if( CS_SUCCEEDED(i32Status) )
		{
			pDevice = m_MasterIndependent;
			while (pDevice != NULL)
			{
				i32Status = pDevice->CsSetAcquisitionConfig( pAcqCfg );
				if( CS_FAILED(i32Status) )break;
				i32Status = pDevice->CsSetChannelConfig( (PARRAY_CHANNELCONFIG) pAChanCfg );

				if( CS_FAILED(i32Status) )break;
				// Changing FPGA image causes the flag m_bCalNeeded is set on CsSetChannelConfig() for all channels
				// But we really do not need to re-calibrate in this case.
				// Clear the flag m_bCalNeeded
				pDevice->ClearFlag_bCalNeeded();
				i32Status = pDevice->CsSetTriggerConfig( (PARRAY_TRIGGERCONFIG) pATrigCfg );

				if( CS_FAILED(i32Status) )break;
				// Use Image0 the calibration values 
				pDevice->ForceFastCalibrationSettingsAllChannels();
				pDevice = pDevice->m_Next;
			}

			// Patch for Stream to Analysis
			// It look like a front end relay or firmware problem. It required some additional time to stabilize. 
			// Without this delay the amplitude of signal captured will change over time
			Sleep(20);
		}
	}

	if( CS_FAILED(i32CalResult) )
		i32Status = i32CalResult;

	pDevice = m_MasterIndependent;
	while (pDevice != NULL)
	{
		if ( m_bMulrecAvgTD )
			pDevice->CsSetDataFormat( formatHwAverage );
		else if ( pDevice->m_bSoftwareAveraging )
			pDevice->CsSetDataFormat( formatSoftwareAverage );
		else // Default image 0
			pDevice->CsSetDataFormat( formatDefault );

		pDevice = pDevice->m_Next;
	}

	// Clear the interrupt register then enable only ones needed for the current acqusition mode
	pDevice = m_MasterIndependent;
	while ( NULL != pDevice )
	{
		pDevice->ClearInterrupts();
		pDevice->DisableInterruptDMA0();
		pDevice = pDevice->m_Next;
	}
	
	return i32Status;
}
