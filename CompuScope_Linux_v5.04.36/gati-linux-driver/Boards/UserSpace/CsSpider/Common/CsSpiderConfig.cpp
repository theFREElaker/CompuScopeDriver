#include "StdAfx.h"
#include "CsTypes.h"
#include "GageWrapper.h"
#include "CsDefines.h"
#include "CsSpider.h"


//-----------------------------------------------------------------------------
//------ CsGetAcquisitionConfig
//-----------------------------------------------------------------------------

int32	CsSpiderDev::CsGetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg )
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
int32	CsSpiderDev::CsGetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg )
{
	CsSpiderDev	*pDevice = this;
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
int32	CsSpiderDev::CsGetTriggerConfig( PARRAY_TRIGGERCONFIG pATrigCfg )
{
	CsSpiderDev		*pDevice = m_MasterIndependent;
	uInt16				NormalizedTrigIndex = 0;
	PCSTRIGGERCONFIG	pTrigCfg = pATrigCfg->aTrigger;
	BOOLEAN				bFoundTrigIndex = FALSE;


	for (uInt32 i = 0; i < pATrigCfg->u32TriggerCount; i++ )
	{
		// Searching for the user trigger index
		pDevice = m_MasterIndependent;
		while( pDevice )
		{
			for ( uInt16 j = 0; j < 1+2*CSPDR_CHANNELS; j++ )
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
					pTrigCfg[i].i32Source = pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].i32Source + GetBoardNode()->NbAnalogChannel*(pDevice->m_pCardState->u16CardNumber - 1);

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
int32	CsSpiderDev::CsGetBoardsInfo( PARRAY_BOARDINFO pABoardInfo )
{
	CsSpiderDev	*pDevice;
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

int32	CsSpiderDev::CsSetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg )
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

		i32Ret = SendMasterSlaveSetting();
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		// Some algorithm relies on the order of function calls
		// Do not change the order of
		// SendClockSetting
		// SendCaptureModeSetting
		i32Ret = SendClockSetting( pAcqCfg->i64SampleRate,
						           pAcqCfg->u32ExtClk, pAcqCfg->u32ExtClkSampleSkip,
								   (pAcqCfg->u32Mode & CS_MODE_REFERENCE_CLK) != 0 );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = SendCaptureModeSetting( pAcqCfg->u32Mode );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = SetPulseOut();
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		// Convert TriggerTimeout from 100 ns to number of memory locations
		// Each memory location contains 16 Samples (Single) or 2x8 Samples (Dual)
		m_pCardState->AcqConfig.i64TriggerTimeout = pAcqCfg->i64TriggerTimeout;

		m_u32HwTrigTimeout = (uInt32) pAcqCfg->i64TriggerTimeout;
		if( (uInt32) CS_TIMEOUT_DISABLE != m_u32HwTrigTimeout && 0 != m_u32HwTrigTimeout)
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

#ifdef EXPERT_SUPPORTED
		if ( m_bMulrecAvg )
		{
			// Interchange Segment count and Mulrec Acg count
			// From firmware point of view, the SegmentCount is number of averaging
			// and the number of averaging is segment count
			SetMulrecAvgCount( m_u32HwSegmentCount );
			m_u32HwSegmentCount = m_u32MulrecAvgCount;
		}
#endif

		CsSpiderDev	*pDevice = m_MasterIndependent;
		while (pDevice)
		{
			i32Ret = pDevice->SendSegmentSetting (pAcqCfg->u32SegmentCount, m_pCardState->AcqConfig.i64Depth, m_pCardState->AcqConfig.i64SegmentSize, m_pCardState->AcqConfig.i64TriggerHoldoff, m_pCardState->AcqConfig.i64TriggerDelay);
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

				uInt32	u32MaxAverageBufferSize = u32MaxAverageSamples * ( m_bHardwareAveraging ?  sizeof(int32) : sizeof(int16) );

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
		m_u32MulrecAvgCount	= 1;

		m_bSoftwareAveraging = FALSE;
		m_bHardwareAveraging = FALSE;

		// Release average buffer if there is any
		if ( m_pAverageBuffer )
		{
			::GageFreeMemoryX( m_pAverageBuffer );
			m_pAverageBuffer = NULL;
		}

		// Restore the default FPGA image
		CsLoadBaseBoardFpgaImage( 0 );

		i32Ret = SendMasterSlaveSetting();
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = SendClockSetting();
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = SendCaptureModeSetting( m_pCardState->u32DefaultMode );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		SetTriggerTimeout (m_u32HwTrigTimeout = (uInt32) CS_TIMEOUT_DISABLE);
		CsSetDataFormat();

		m_pCardState->AcqConfig.i64TriggerTimeout	= CS_TIMEOUT_DISABLE;
		m_pCardState->AcqConfig.u32TrigEnginesEn	= CSPDR_DEFAULT_TRIG_ENGINES_EN;
		m_pCardState->AcqConfig.i64TriggerDelay		= CSPDR_DEFAULT_TRIG_DELAY;
		m_pCardState->AcqConfig.i64TriggerHoldoff	= CSPDR_DEFAULT_TRIG_HOLD_OFF;
		m_pCardState->AcqConfig.u32SegmentCount		= CSPDR_DEFAULT_SEGMENT_COUNT;
		m_pCardState->AcqConfig.i64Depth			= CSPDR_DEFAULT_DEPTH;
		m_pCardState->AcqConfig.i64SegmentSize		= m_pCardState->AcqConfig.i64Depth + m_pCardState->AcqConfig.i64TriggerHoldoff;

		CsSpiderDev* pDevice = m_MasterIndependent;
		while (pDevice)
		{
			i32Ret = pDevice->SendSegmentSetting (m_pCardState->AcqConfig.u32SegmentCount, m_pCardState->AcqConfig.i64Depth, m_pCardState->AcqConfig.i64SegmentSize, m_pCardState->AcqConfig.i64TriggerHoldoff, m_pCardState->AcqConfig.i64TriggerDelay);
			if( CS_FAILED(i32Ret) )
				return i32Ret;

			i32Ret = SendTimeStampMode(false, false);
			if( CS_FAILED(i32Ret) )
				return i32Ret;

			pDevice = pDevice->m_Next;
		}
	}

	if ( m_pCardState->bSpiderLP )
		SetupHwAverageMode();

	CSACQUISITIONCONFIG_EX AcqCfgEx = {0};

	AcqCfgEx.AcqCfg.u32Size		= sizeof(CSACQUISITIONCONFIG);
	CsGetAcquisitionConfig( &AcqCfgEx.AcqCfg );

	AcqCfgEx.i64HwDepth			= m_i64HwPostDepth;
	AcqCfgEx.i64HwSegmentSize	= m_i64HwSegmentSize;
	AcqCfgEx.u32ExpertOption	= m_PlxData->CsBoard.u32BaseBoardOptions[m_pCardState->u8ImageId];

	IoctlSetAcquisitionConfigEx( &AcqCfgEx ) ;

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CsSpiderDev *CsSpiderDev::GetCardPointerFromChannelIndex( uInt16 u16ChannelIndex )
{
	CsSpiderDev	*pDevice = static_cast<CsSpiderDev *> (m_MasterIndependent);
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

int32	CsSpiderDev::CsSetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg )
{
	int32				i32Status = CS_SUCCESS;

	if( !m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	//m_u32IgnoreCalibErrorMaskLocal = m_u32IgnoreCalibErrorMask;

	if ( pAChanCfg )
	{
		CsSpiderDev			*pDevice = (CsSpiderDev *)m_MasterIndependent;
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
			i32Status = SendAdcOffsetAjust(ConvertToHwChannel(i), m_pCardState->u16AdcOffsetAdjust );
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

int32	CsSpiderDev::CsSetTriggerConfig( PARRAY_TRIGGERCONFIG pATrigCfg )
{
	int32	i32Status = CS_SUCCESS;
	uInt16	u16HardwareChannel;
	uInt8	u8IndexTrig;
	uInt32	i;


	if( !m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( pATrigCfg )
	{
		CsSpiderDev			*pDevice = m_MasterIndependent;
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

				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value1 = CSPDR_DEFAULT_VALUE;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value2 = CSPDR_DEFAULT_VALUE;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32Filter = CSPDR_DEFAULT_VALUE;

				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtImpedance = pTrigCfg[i].u32ExtImpedance;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtTriggerRange = pTrigCfg[i].u32ExtTriggerRange;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32ExtCoupling = pTrigCfg[i].u32ExtCoupling;

				//put the board always in or trigger for now
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32Relation = CS_RELATION_OR;
			}
		}

		// Second Pass, Remember all user params for Trigger Engines DISABLED
		for ( i = 0; i < pATrigCfg->u32TriggerCount; i++ )
		{
			if ( CS_TRIG_SOURCE_DISABLE ==  pTrigCfg[i].i32Source )
			{
				pDevice = m_MasterIndependent;
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

						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value1 = CSPDR_DEFAULT_VALUE;
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value2 = CSPDR_DEFAULT_VALUE;
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32Filter = CSPDR_DEFAULT_VALUE;

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

			pDevice->SendExtTriggerSetting( (BOOLEAN) pDevice->m_CfgTrigParams[CSPDR_TRIGENGINE_EXT].i32Source,
											eteAlways,
											pDevice->m_CfgTrigParams[CSPDR_TRIGENGINE_EXT].i32Level,
											pDevice->m_CfgTrigParams[CSPDR_TRIGENGINE_EXT].u32Condition,
											pDevice->m_CfgTrigParams[CSPDR_TRIGENGINE_EXT].u32ExtTriggerRange, 
											pDevice->m_CfgTrigParams[CSPDR_TRIGENGINE_EXT].u32ExtCoupling );

			if ( CS_FAILED(i32Status) )
				return i32Status;

			i32Status = pDevice->SendTriggerRelation( CSPDR_DEFAULT_RELATION );
			if ( CS_FAILED(i32Status) )
				return i32Status;

			// Update Ext trigger params for all trigger engines
			for(  i = 1; i < CSPDR_TOTAL_TRIGENGINES; i++ )
			{
				pDevice->m_pCardState->TriggerParams[i].u32ExtTriggerRange = pDevice->m_CfgTrigParams[CSPDR_TRIGENGINE_EXT].u32ExtTriggerRange;
				pDevice->m_pCardState->TriggerParams[i].u32ExtCoupling = pDevice->m_CfgTrigParams[CSPDR_TRIGENGINE_EXT].u32ExtCoupling;
				pDevice->m_pCardState->TriggerParams[i].u32ExtImpedance = pDevice->m_CfgTrigParams[CSPDR_TRIGENGINE_EXT].u32ExtImpedance;
			}

			// Remember the user trigger index
			for ( i = 0; i < CSPDR_TOTAL_TRIGENGINES; i ++ )
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

		::GageZeroMemoryX( m_pCardState->TriggerParams, sizeof(m_pCardState->TriggerParams) );
		for ( i = 0; i < m_PlxData->CsBoard.NbAnalogChannel; i++ )
		{
			u16HardwareChannel = ConvertToHwChannel((uInt16)(i+1));
			u8IndexTrig = (uInt8) (2*(u16HardwareChannel-1) + 1);
			u32UserTrigIndex = (uInt8) (2*i + 1);

			// Engine A
			m_pCardState->TriggerParams[u8IndexTrig].u32Relation = CSPDR_DEFAULT_RELATION;
			m_pCardState->TriggerParams[u8IndexTrig].i32Value1 = CSPDR_DEFAULT_VALUE;
			m_pCardState->TriggerParams[u8IndexTrig].i32Value2 = CSPDR_DEFAULT_VALUE;
			m_pCardState->TriggerParams[u8IndexTrig].u32Filter = CSPDR_DEFAULT_VALUE;
			m_pCardState->TriggerParams[u8IndexTrig].u32ExtTriggerRange = CSPDR_DEFAULT_EXT_GAIN;
			m_pCardState->TriggerParams[u8IndexTrig].u32ExtCoupling = CSPDR_DEFAULT_EXT_COUPLING;
			m_pCardState->TriggerParams[u8IndexTrig].u32ExtImpedance = CSPDR_DEFAULT_EXT_IMPEDANCE;
			m_pCardState->TriggerParams[u8IndexTrig].u32UserTrigIndex = u32UserTrigIndex + m_PlxData->CsBoard.NbTriggerMachine*(m_pCardState->u16CardNumber-1);

			// Bngine B
			m_pCardState->TriggerParams[u8IndexTrig+1].u32Relation = CSPDR_DEFAULT_RELATION;
			m_pCardState->TriggerParams[u8IndexTrig+1].i32Value1 = CSPDR_DEFAULT_VALUE;
			m_pCardState->TriggerParams[u8IndexTrig+1].i32Value2 = CSPDR_DEFAULT_VALUE;
			m_pCardState->TriggerParams[u8IndexTrig+1].u32Filter = CSPDR_DEFAULT_VALUE;
			m_pCardState->TriggerParams[u8IndexTrig+1].u32ExtTriggerRange = CSPDR_DEFAULT_EXT_GAIN;
			m_pCardState->TriggerParams[u8IndexTrig+1].u32ExtCoupling = CSPDR_DEFAULT_EXT_COUPLING;
			m_pCardState->TriggerParams[u8IndexTrig+1].u32ExtImpedance = CSPDR_DEFAULT_EXT_IMPEDANCE;
			m_pCardState->TriggerParams[u8IndexTrig+1].u32UserTrigIndex = (u32UserTrigIndex + 1) + m_PlxData->CsBoard.NbTriggerMachine*(m_pCardState->u16CardNumber-1);

			// Default trigger from Channel 1 master
			if ( 1 == u8IndexTrig && 1 == m_pCardState->u16CardNumber )
				m_pCardState->TriggerParams[u8IndexTrig].i32Source = 1;

			i32Status = SendTriggerEngineSetting( u16HardwareChannel,
												  m_pCardState->TriggerParams[u8IndexTrig].i32Source );
			if ( CS_FAILED(i32Status) )
				return i32Status;
		}

		m_pCardState->TriggerParams[CSPDR_TRIGENGINE_EXT].u32Relation = CSPDR_DEFAULT_RELATION;
		m_pCardState->TriggerParams[CSPDR_TRIGENGINE_EXT].i32Value1 = CSPDR_DEFAULT_VALUE;
		m_pCardState->TriggerParams[CSPDR_TRIGENGINE_EXT].i32Value2 = CSPDR_DEFAULT_VALUE;
		m_pCardState->TriggerParams[CSPDR_TRIGENGINE_EXT].u32Filter = CSPDR_DEFAULT_VALUE;
		m_pCardState->TriggerParams[CSPDR_TRIGENGINE_EXT].u32ExtTriggerRange = CSPDR_DEFAULT_EXT_GAIN;
		m_pCardState->TriggerParams[CSPDR_TRIGENGINE_EXT].u32ExtCoupling = CSPDR_DEFAULT_EXT_COUPLING;
		m_pCardState->TriggerParams[CSPDR_TRIGENGINE_EXT].u32ExtImpedance = CSPDR_DEFAULT_EXT_IMPEDANCE;
		m_pCardState->TriggerParams[CSPDR_TRIGENGINE_EXT].u32UserTrigIndex = m_PlxData->CsBoard.NbTriggerMachine*m_pCardState->u16CardNumber;
		m_pCardState->AcqConfig.u32TrigEnginesEn = CSPDR_DEFAULT_TRIG_ENGINES_EN; // also in acquisition

		i32Status = SendExtTriggerSetting();
		if ( CS_FAILED(i32Status) )
			return i32Status;

		i32Status = SendTriggerRelation( CSPDR_DEFAULT_RELATION	);
		if ( CS_FAILED(i32Status) )
			return i32Status;
	}

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

CsSpiderDev *CsSpiderDev::GetCardPointerFromTriggerSource( int32 i32TriggerSoure )
{
	CsSpiderDev *pDevice = static_cast<CsSpiderDev *> (m_MasterIndependent);

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

CsSpiderDev *CsSpiderDev::GetCardPointerFromTriggerIndex( uInt16 TriggerIndex )
{
	uInt16 CardNumber;
	CsSpiderDev *pDevice = static_cast<CsSpiderDev *> (m_MasterIndependent);

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

int32	CsSpiderDev::CommitNewConfiguration( PCSACQUISITIONCONFIG pAcqCfg,
											   	PARRAY_CHANNELCONFIG pAChanCfg,
                                                PARRAY_TRIGGERCONFIG pATrigCfg )
{
	int32					i32Status = CS_SUCCESS;
	int32					i32CalResult(CS_SUCCESS);
	CsSpiderDev				*pDevice = this;


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
	if( CS_FAILED(i32Status) )
		return i32Status;

	// Detect if calibration for at least one channel is required.
	BOOLEAN	bRequiredCalibrate = FALSE;

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

	if ( ! bRequiredCalibrate )
	{
		// CALIBRATION IS NOT REQUIRED

		if ( m_pCardState->u8ImageId != m_DelayLoadImageId )
		{
			pDevice = m_MasterIndependent;
			while (pDevice != NULL)
			{
				// Patch for firmware bug (v8.1.18.157)
				// If we are in OCT mode then switch to Expert AVG TD, there is small offset in trigger adresss
				// The pacth is put the card in mode single then change the image
				if ( m_bNucleon && IsExpertAVG(m_DelayLoadImageId) )
					pDevice->SendCaptureModeSetting(CS_MODE_SINGLE);

				i32Status = pDevice->CsLoadBaseBoardFpgaImage( m_DelayLoadImageId );
				if( CS_FAILED(i32Status) )
					break;
				pDevice = pDevice->m_Next;
			}

			if( CS_SUCCEEDED(i32Status) )
			{
				pDevice = m_MasterIndependent;
				while (pDevice != NULL)
				{
					i32Status = pDevice->CsSetAcquisitionConfig( pAcqCfg );
					if( CS_FAILED(i32Status) )
						break;

					i32Status = pDevice->CsSetChannelConfig( (PARRAY_CHANNELCONFIG) pAChanCfg );
					if( CS_FAILED(i32Status) )
						break;

					// Changing FPGA image causes the flag m_bCalNeeded is set on CsSetChannelConfig() for all channels
					// But we really do not need to re-calibrate in this case.
					// Clear the flag m_bCalNeeded
					pDevice->ClearFlag_bCalNeeded();

					i32Status = pDevice->CsSetTriggerConfig( (PARRAY_TRIGGERCONFIG) pATrigCfg );
					if( CS_FAILED(i32Status) )
						break;

					// Use Image0 the calibration values
					pDevice->ForceFastCalibrationSettingsAllChannels();
					pDevice = pDevice->m_Next;
				}
			}
		}
	}
	else
	{
		// CALIBRATION IS REQUIRED
		if ( 0 != m_pCardState->u8ImageId )
		{
			// ONE OF EXPERT IMAGES THE CURRENT ACTIVE IMAGE

			// switch to the primary image before calibration
			pDevice = m_MasterIndependent;
			while (pDevice != NULL)
			{
				i32Status = pDevice->CsLoadBaseBoardFpgaImage( 0 );
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

			i32CalResult = MsCalibrateAllChannels();

			// switch to the current image after calibration
			pDevice = m_MasterIndependent;
			while (pDevice != NULL)
			{
				// Patch for firmware bug (v8.1.18.157)
				// If we are in OCT mode then switch to Expert AVG TD, there is small offset in trigger adresss
				// The pacth is put the card in mode single then change the image
				if ( m_bNucleon && IsExpertAVG(m_DelayLoadImageId) )
					pDevice->SendCaptureModeSetting(CS_MODE_SINGLE);

				i32Status = pDevice->CsLoadBaseBoardFpgaImage( m_DelayLoadImageId );
				pDevice = pDevice->m_Next;
			}

			pDevice = m_MasterIndependent;
			while (pDevice != NULL)
			{
				i32Status = pDevice->CsSetAcquisitionConfig( pAcqCfg );
				if( CS_FAILED(i32Status) )
					break;

				i32Status = pDevice->CsSetChannelConfig( (PARRAY_CHANNELCONFIG) pAChanCfg );
				if( CS_FAILED(i32Status) )
					break;

				// Changing FPGA image causes the flag m_bCalNeeded is set on CsSetChannelConfig() for all channels
				// But we really do not need to re-calibrate in this case.
				// Clear the flag m_bCalNeeded
				pDevice->ClearFlag_bCalNeeded();

				i32Status = pDevice->CsSetTriggerConfig( (PARRAY_TRIGGERCONFIG) pATrigCfg );
				if( CS_FAILED(i32Status) )
					break;

				// Use Image0 the calibration values
				pDevice->ForceFastCalibrationSettingsAllChannels();

				pDevice = pDevice->m_Next;
			}
		}
		else
		{
			// IMAGE 0 IS THE CURRENT ACITVE IMAGE

			i32CalResult = MsCalibrateAllChannels();

			if ( m_pCardState->u8ImageId != m_DelayLoadImageId )
			{
				// SWITCH FROM IMAGE 0 TO ONE OF EXPERT IMAGES

				// switch to the new image after calibration
				pDevice = m_MasterIndependent;
				while (pDevice != NULL)
				{
					// Patch for firmware bug (v8.1.18.157)
					// If we are in OCT mode then switch to Expert AVG TD, there is small offset in trigger adresss
					// The pacth is put the card in mode single then change the image
					if ( m_bNucleon && IsExpertAVG(m_DelayLoadImageId) )
						pDevice->SendCaptureModeSetting(CS_MODE_SINGLE);
					i32Status = pDevice->CsLoadBaseBoardFpgaImage( m_DelayLoadImageId );
					pDevice = pDevice->m_Next;
				}

				pDevice = m_MasterIndependent;
				while (pDevice != NULL)
				{
					i32Status = pDevice->CsSetAcquisitionConfig( pAcqCfg );
					if( CS_FAILED(i32Status) )
						break;

					i32Status = pDevice->CsSetChannelConfig( (PARRAY_CHANNELCONFIG) pAChanCfg );
					if( CS_FAILED(i32Status) )
						break;

					// Changing FPGA image causes the flag m_bCalNeeded is set on CsSetChannelConfig() for all channels
					// But we really do not need to re-calibrate in this case.
					// Clear the flag m_bCalNeeded
					pDevice->ClearFlag_bCalNeeded();

					i32Status = pDevice->CsSetTriggerConfig( (PARRAY_TRIGGERCONFIG) pATrigCfg );
					if( CS_FAILED(i32Status) )
						break;

					// Use Image0 the calibration values
					pDevice->ForceFastCalibrationSettingsAllChannels();
					pDevice = pDevice->m_Next;
				}
			}
		}
	}

	pDevice = m_MasterIndependent;
	while (pDevice != NULL)
	{
		if ( pDevice->m_bSoftwareAveraging )
			pDevice->CsSetDataFormat( formatSoftwareAverage );
		else if ( pDevice->m_bMulrecAvgTD )
			pDevice->CsSetDataFormat( formatHwAverage );
		else
		{
			// Default image 0
			pDevice->CsSetDataFormat( formatDefault );
		}

		pDevice = pDevice->m_Next;
	}


	// Validation of channel inputs
#ifdef OVERVOLTAGEPROTECTION_v12
	if ( m_bSpiderV12 )
	{
		pDevice = m_MasterIndependent;
		while (pDevice != NULL)
		{
			pDevice->ClearChannelProtectionStatus();
			pDevice->GetChannelProtectionStatus();
			if ( pDevice->m_u16ChanProtectStatus )
				return CS_CHANNEL_PROTECT_FAULT;

			pDevice = pDevice->m_Next;
		}
	}
#endif

	// Fixed channel skew after calibration
	if ( m_MasterIndependent->m_Next != 0 )
		i32Status = m_MasterIndependent->ResetMasterSlave();

	if( CS_FAILED(i32CalResult) )
		return i32CalResult;
	else
		return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
inline void CsSpiderDev::ClearFlag_bCalNeeded()
{
	for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i += GetUserChannelSkip() )
	{
		m_pCardState->bCalNeeded[ConvertToHwChannel(i)-1] = false;
	}
}
