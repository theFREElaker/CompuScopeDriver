#include "StdAfx.h"
#include "CsTypes.h"
#include "GageWrapper.h"
#include "CsDefines.h"
#include "CsHexagon.h"

#ifdef _WINDOWS_
	#include "CsMsgLog.h"
#endif
//-----------------------------------------------------------------------------
//------ CsGetAcquisitionConfig
//-----------------------------------------------------------------------------

int32	CsHexagon::CsGetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg )
{
	// User struct size may be smaller than driver struct size
	uInt32		u32UserStructSize = pAcqCfg->u32Size;

	RtlCopyMemory( pAcqCfg, &m_pCardState->AcqConfig, pAcqCfg->u32Size ); 
	pAcqCfg->u32Size = u32UserStructSize;

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
// CsGetChannelConfig
//-----------------------------------------------------------------------------
int32	CsHexagon::CsGetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg )
{
	CsHexagon	*pDevice = this;
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
			pChanCfg[i].i32Calib = pDevice->m_pCardState->ChannelParams[NormalizedChanIndex].bForceCalibration;
		}
	}

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//CsGetTriggerConfig
//-----------------------------------------------------------------------------
int32	CsHexagon::CsGetTriggerConfig( PARRAY_TRIGGERCONFIG pATrigCfg )
{
	CsHexagon		*pDevice = m_MasterIndependent;
	uInt16				NormalizedTrigIndex = 0;
	PCSTRIGGERCONFIG	pTrigCfg = pATrigCfg->aTrigger;
	BOOLEAN				bFoundTrigIndex = FALSE;


	for (uInt32 i = 0; i < pATrigCfg->u32TriggerCount; i++ )
	{
		// Searching for the user trigger index
		pDevice = m_MasterIndependent;
		while( pDevice )
		{
			// required to search for the whole table event for 2 channels boards.
			for ( uInt16 j = 0; j < HEXAGON_TOTAL_TRIGENGINES; j++ )
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
				pTrigCfg[i].u32Condition	= pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].u32Condition;
				pTrigCfg[i].i32Level		= pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].i32Level;
				pTrigCfg[i].u32Relation		= pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].u32Relation;
				pTrigCfg[i].u32Filter		= pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].u32Filter;
				pTrigCfg[i].i32Value1		= pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].i32Value1;
				pTrigCfg[i].i32Value2		= pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].i32Value2;

				// Return extra parameters from external trigger
				pTrigCfg[i].u32ExtImpedance		= pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].u32ExtImpedance;
				pTrigCfg[i].u32ExtCoupling		= pDevice->m_pCardState->TriggerParams[NormalizedTrigIndex].u32ExtCoupling;
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
int32	CsHexagon::CsGetBoardsInfo( PARRAY_BOARDINFO pABoardInfo )
{
	CsHexagon	*pDevice;
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
			pDrvBdInfo = pDevice->GetBoardNode();

			sprintf_s(BoardInfo[i].strSerialNumber, sizeof(BoardInfo[i].strSerialNumber),
								TEXT("%08x/%08x"), pDrvBdInfo->u32SerNum, pDrvBdInfo->u32SerNumEx);
			BoardInfo[i].u32BaseBoardVersion = pDrvBdInfo->u32BaseBoardVersion;

			// Conversion of the firmware version to the standard  xx.xx.xx format
			BoardInfo[i].u32BaseBoardFirmwareVersion = ((pDrvBdInfo->u32RawFwVersionB[0]  & 0xFF00) << 8) |
														((pDrvBdInfo->u32RawFwVersionB[0] & 0xF0) << 4) |
														(pDrvBdInfo->u32RawFwVersionB[0] & 0xF);

			BoardInfo[i].u32AddonBoardVersion = pDrvBdInfo->u32AddonBoardVersion;

			// Conversion of the firmware version to to the standard xx.xx.xx format
			u32FwVersion = pDrvBdInfo->u32RawFwVersionB[0] >> 16;
			BoardInfo[i].u32AddonBoardFirmwareVersion = ((u32FwVersion  & 0x7f00) << 8) |
														((u32FwVersion & 0xF0) << 4) |
														(u32FwVersion & 0xF);

			BoardInfo[i].u32AddonFwOptions		= pDrvBdInfo->u32ActiveAddonOptions;
			BoardInfo[i].u32BaseBoardFwOptions	= pDrvBdInfo->u32BaseBoardOptions[m_pCardState->i32ConfigBootLocation];
			BoardInfo[i].u32AddonHwOptions		= pDrvBdInfo->u32AddonHardwareOptions;
			BoardInfo[i].u32BaseBoardHwOptions	= pDrvBdInfo->u32BaseBoardHardwareOptions;
		}
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//CsSetAcquisitionConfig
//-----------------------------------------------------------------------------

int32	CsHexagon::CsSetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg )
{
	int32	i32Ret = CS_SUCCESS;


	if( !m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	// Reset the flag for Multi Segments Transfer
	m_pCardState->bMultiSegmentTransfer = FALSE;

	if ( pAcqCfg )
	{
		PreCalculateStreamDataSize( pAcqCfg );

		m_pCardState->AcqConfig.u32TrigEnginesEn	= pAcqCfg->u32TrigEnginesEn;
		m_pCardState->AcqConfig.i64TriggerDelay		= pAcqCfg->i64TriggerDelay;
		m_pCardState->AcqConfig.i64TriggerHoldoff	= pAcqCfg->i64TriggerHoldoff;
		m_pCardState->AcqConfig.u32SegmentCount		= pAcqCfg->u32SegmentCount;
		m_pCardState->AcqConfig.i64Depth			= pAcqCfg->i64Depth;
		m_pCardState->AcqConfig.u32TimeStampConfig	= pAcqCfg->u32TimeStampConfig;
		m_pCardState->AcqConfig.i64SegmentSize		= pAcqCfg->i64SegmentSize;
		m_pCardState->AcqConfig.i64TriggerTimeout	= pAcqCfg->i64TriggerTimeout;

		// Some algorithm relies on the order of function calls
		// Do not change the order of
		// SendClockSetting
		// SendCaptureModeSetting

		i32Ret = SendClockSetting( pAcqCfg->i64SampleRate, (0 != pAcqCfg->u32ExtClk), (pAcqCfg->u32Mode & CS_MODE_REFERENCE_CLK) != 0 );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = SendCaptureModeSetting( pAcqCfg->u32Mode );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		SetTriggerTimeout ( ConvertTriggerTimeout(pAcqCfg->i64TriggerTimeout) );

		m_u32HwSegmentCount		= pAcqCfg->u32SegmentCount;
		m_i64HwSegmentSize		= m_pCardState->AcqConfig.i64SegmentSize;
		m_i64HwPostDepth		= m_pCardState->AcqConfig.i64Depth;
		m_i64HwTriggerHoldoff	= m_pCardState->AcqConfig.i64TriggerHoldoff;
		m_i64HwTriggerDelay		= m_pCardState->AcqConfig.i64TriggerDelay;

		CsHexagon	*pDevice = m_MasterIndependent;
		while (pDevice)
		{
			i32Ret = pDevice->SendSegmentSetting (m_u32HwSegmentCount, m_i64HwPostDepth, m_i64HwSegmentSize, m_i64HwTriggerHoldoff, m_i64HwTriggerDelay);
			if( CS_FAILED(i32Ret) )
				return i32Ret;

			i32Ret = pDevice->SendTimeStampMode( ((pAcqCfg->u32TimeStampConfig & TIMESTAMP_MCLK) != 0),
					                             ((pAcqCfg->u32TimeStampConfig & TIMESTAMP_FREERUN) != 0) );
			if( CS_FAILED(i32Ret) )
				return i32Ret;

			pDevice = pDevice->m_Next;
		}

		if ( !IsExpertAVG_enabled() && (( CS_MODE_SW_AVERAGING & pAcqCfg->u32Mode ) != 0 ) )
		{
			m_bSoftwareAveraging = TRUE;

			uInt32  u32MaxAverageSamples = MAX_SW_AVERAGING_SEMGENTSIZE + SW_AVERAGING_BUFFER_PADDING;
			uInt32	u32MaxAverageBufferSize = u32MaxAverageSamples * sizeof(int16);

			if( NULL == m_pAverageBuffer )
			{
				m_pAverageBuffer = (int16 *)::GageAllocateMemoryX( u32MaxAverageBufferSize );
				if( NULL == m_pAverageBuffer )
					return CS_INSUFFICIENT_RESOURCES;
			}
		}
		else
		{
            m_bSoftwareAveraging = FALSE;
			if ( NULL != m_pAverageBuffer  )
			{
				::GageFreeMemoryX( m_pAverageBuffer );
				m_pAverageBuffer = NULL;
			}
		}
	}
	else
	{
		// Default setting
		m_pCardState->AcqConfig.u32ExtClk = 0;
		m_pCardState->AcqConfig.u32ExtClkSampleSkip = m_u32DefaultExtClkSkip;
		m_u32MulrecAvgCount	= 1;

		m_bSoftwareAveraging = FALSE;

		// Release average buffer if there is any
		if ( m_pAverageBuffer )
		{
			::GageFreeMemoryX( m_pAverageBuffer );
			m_pAverageBuffer = NULL;
		}

		i32Ret = SendClockSetting( -1 );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = SendCaptureModeSetting(INFINITE_U32);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		SetTriggerTimeout();
		CsSetDataFormat();

		m_pCardState->AcqConfig.i64TriggerTimeout	= CS_TIMEOUT_DISABLE;
		m_pCardState->AcqConfig.u32TrigEnginesEn	= HEXAGON_DEFAULT_TRIG_ENGINES_EN;
		m_pCardState->AcqConfig.i64TriggerDelay		= HEXAGON_DEFAULT_TRIG_DELAY;
		m_pCardState->AcqConfig.i64TriggerHoldoff	= HEXAGON_DEFAULT_TRIG_HOLD_OFF;
		m_pCardState->AcqConfig.u32SegmentCount		= HEXAGON_DEFAULT_SEGMENT_COUNT;
		m_pCardState->AcqConfig.i64Depth			= HEXAGON_DEFAULT_DEPTH;
		m_pCardState->AcqConfig.i64SegmentSize		= m_pCardState->AcqConfig.i64Depth + m_pCardState->AcqConfig.i64TriggerHoldoff;

		m_u32HwSegmentCount		= m_pCardState->AcqConfig.u32SegmentCount;
		m_i64HwSegmentSize		= m_pCardState->AcqConfig.i64SegmentSize;
		m_i64HwPostDepth		= m_pCardState->AcqConfig.i64Depth;
		m_i64HwTriggerHoldoff	= m_pCardState->AcqConfig.i64TriggerHoldoff;
		m_i64HwTriggerDelay		= m_pCardState->AcqConfig.i64TriggerDelay;

		i32Ret = SendSegmentSetting (m_u32HwSegmentCount, m_i64HwPostDepth, m_i64HwSegmentSize, m_i64HwTriggerHoldoff);
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = SendTimeStampMode(false, false);
		if( CS_FAILED(i32Ret) )
			return i32Ret;
	}

	// Reset the flag for Multi Segments Transfer
	m_pCardState->bMultiSegmentTransfer = FALSE;

	// Send the new infos of acquisition config to kernel
	CSACQUISITIONCONFIG_EX AcqCfgEx = {0};

	AcqCfgEx.AcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG); 
	CsGetAcquisitionConfig( &AcqCfgEx.AcqCfg );

	AcqCfgEx.i64HwDepth			= m_i64HwPostDepth;
	AcqCfgEx.i64HwSegmentSize	= m_i64HwSegmentSize;
	AcqCfgEx.i64HwTrigDelay		= m_i64HwTriggerDelay;
	AcqCfgEx.u32ExpertOption	= m_PlxData->CsBoard.u32BaseBoardOptions[m_pCardState->i32ConfigBootLocation];
	AcqCfgEx.u32Decimation		= m_pCardState->AddOnReg.u32Decimation;
	i32Ret = IoctlSetAcquisitionConfigEx( &AcqCfgEx );
	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CsHexagon *CsHexagon::GetCardPointerFromChannelIndex( uInt16 u16ChannelIndex )
{
	CsHexagon		*pDevice = static_cast<CsHexagon *> (m_MasterIndependent);
	uInt16			u16CardNumber;

	if ( ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE ) &&
		 ( u16ChannelIndex % 2  == 0 ) )
	{
		// ChannelIndex should be odd in single mode on two channel boards
		return NULL;
	}

	if( 0 == GetBoardNode()->NbAnalogChannel )
	{
		//Device was not fully initialized therefore no slaves
		return pDevice;
	}

	u16CardNumber = (uInt16) (u16ChannelIndex + GetBoardNode()->NbAnalogChannel - 1) / GetBoardNode()->NbAnalogChannel;

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

int32	CsHexagon::CsSetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg )
{
	int32				i32Status = CS_SUCCESS;

	if( !m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if (m_u16GainCode_2vpp)
		ForceGainControl(CS_GAIN_2_V, m_u16GainCode_2vpp);
	
	if ( pAChanCfg )
	{
		CsHexagon			*pDevice = (CsHexagon *)m_MasterIndependent;
		PCSCHANNELCONFIG	pChanCfg = pAChanCfg->aChannel;
		uInt16				u16NormalizedChanIndex = CS_CHAN_1;

		for (uInt32 i = 0; i < pAChanCfg->u32ChannelCount; i++ )
		{
			u16NormalizedChanIndex = ConvertToHwChannel( uInt16(CS_CHAN_1 + i) );
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
		uInt32 u32ValidCoupling = CS_COUPLING_DC;
		uInt32 u32InputRange = CS_GAIN_2_V;
		if ( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & HEXAGON_ADDON_HW_OPTION_AC_COUPLING) )
			u32ValidCoupling = CS_COUPLING_AC;

		if (IsBoardLowRange())
			u32InputRange = HEXAGON_GAIN_LR_MV;
		
		for ( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i++ )
		{
			i32Status = SendChannelSetting(ConvertToHwChannel(i), u32InputRange, u32ValidCoupling );
			if ( CS_FAILED(i32Status) )
				return i32Status;
		}
	}

	return i32Status;
}


//-----------------------------------------------------------------------------
//CsSetTriggerConfig
//-----------------------------------------------------------------------------

int32	CsHexagon::CsSetTriggerConfig( PARRAY_TRIGGERCONFIG pATrigCfg )
{
	int32	i32Status = CS_SUCCESS;
	uInt8	u8IndexTrig;
	uInt32	i;

	if( !m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( pATrigCfg )
	{
		CsHexagon			*pDevice = m_MasterIndependent;
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

				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value1 = HEXAGON_DEFAULT_VALUE;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value2 = HEXAGON_DEFAULT_VALUE;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32Filter = HEXAGON_DEFAULT_VALUE;

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

						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value1 = HEXAGON_DEFAULT_VALUE;
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value2 = HEXAGON_DEFAULT_VALUE;
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32Filter = HEXAGON_DEFAULT_VALUE;

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
			u8IndexTrig = CSRB_TRIGENGINE_A1;

			i32Status = pDevice->SendTriggerEngineSetting( 0,
														   pDevice->m_CfgTrigParams[u8IndexTrig].i32Source,
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
			if ( CS_FAILED(i32Status) )
				return i32Status;

			u8IndexTrig = CSRB_TRIGENGINE_A3;
			i32Status = pDevice->SendTriggerEngineSetting( 1,
														   pDevice->m_CfgTrigParams[u8IndexTrig].i32Source,
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
			if ( CS_FAILED(i32Status) )
				return i32Status;

			i32Status = pDevice->SendExtTriggerSetting( (BOOLEAN) pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].i32Source,
											pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].i32Level,
											pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32Condition,
											pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange,
											pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance,
											pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtCoupling );

			if ( CS_FAILED(i32Status) )
				return i32Status;

			// Remember the user trigger index
			for ( i = 0; i < HEXAGON_TOTAL_TRIGENGINES; i ++ )
			{
				pDevice->m_pCardState->TriggerParams[i].u32UserTrigIndex = pDevice->m_CfgTrigParams[i].u32UserTrigIndex;
			}

			pDevice = pDevice->m_Next;
		}

		return CS_SUCCESS;
	}
	else
	{
		RtlZeroMemory( m_pCardState->TriggerParams, sizeof(m_pCardState->TriggerParams) );
		for ( i = CSRB_TRIGENGINE_A1 ; i < HEXAGON_TOTAL_TRIGENGINES; i ++ )
		{
			m_pCardState->TriggerParams[i].u32Relation			= HEXAGON_DEFAULT_RELATION;
			m_pCardState->TriggerParams[i].i32Value1			= HEXAGON_DEFAULT_VALUE;
			m_pCardState->TriggerParams[i].i32Value2			= HEXAGON_DEFAULT_VALUE;
			m_pCardState->TriggerParams[i].u32Filter			= HEXAGON_DEFAULT_VALUE;
			m_pCardState->TriggerParams[i].u32ExtTriggerRange	= HEXAGON_DEFAULT_EXT_GAIN;
			m_pCardState->TriggerParams[i].u32ExtCoupling		= HEXAGON_DEFAULT_EXT_COUPLING;
			m_pCardState->TriggerParams[i].u32ExtImpedance		= HEXAGON_DEFAULT_EXT_IMPEDANCE;
			m_pCardState->TriggerParams[i].u32UserTrigIndex = i + HEXAGON_TOTAL_TRIGENGINES*(m_pCardState->u16CardNumber-1);
		}

		// Default trigger from Channel 1 master
		u8IndexTrig = CSRB_TRIGENGINE_A1;
		if ( 1 == m_pCardState->u16CardNumber )
			m_pCardState->TriggerParams[u8IndexTrig].i32Source = 1;
		else
			m_pCardState->TriggerParams[u8IndexTrig].i32Source = 0;

		i32Status = SendTriggerEngineSetting( 0, m_pCardState->TriggerParams[u8IndexTrig].i32Source );
		if ( CS_FAILED(i32Status) )
			return i32Status;

		i32Status = SendTriggerEngineSetting( 1 );
		if ( CS_FAILED(i32Status) )
			return i32Status;

		m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32Relation		= HEXAGON_DEFAULT_RELATION;
		m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].i32Value1			=
		m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].i32Value2			=
		m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32Filter			= HEXAGON_DEFAULT_VALUE;
		m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange = HEXAGON_DEFAULT_EXT_GAIN;
		m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtCoupling		= HEXAGON_DEFAULT_EXT_COUPLING;
		m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance	= HEXAGON_DEFAULT_EXT_IMPEDANCE;
		m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32UserTrigIndex	= HEXAGON_TOTAL_TRIGENGINES*m_pCardState->u16CardNumber;
		m_pCardState->AcqConfig.u32TrigEnginesEn							= HEXAGON_DEFAULT_TRIG_ENGINES_EN; // also in acquisition

		i32Status = SendExtTriggerSetting();
		if ( CS_FAILED(i32Status) )
			return i32Status;

		RtlCopyMemory( m_CfgTrigParams, m_pCardState->TriggerParams, sizeof( m_CfgTrigParams ) );

	}

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

CsHexagon *CsHexagon::GetCardPointerFromTriggerSource( int32 i32TriggerSoure )
{
	CsHexagon *pDevice = static_cast<CsHexagon *> (m_MasterIndependent);

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

CsHexagon *CsHexagon::GetCardPointerFromTriggerIndex( uInt16 TriggerIndex )
{
	uInt16 CardNumber;
	CsHexagon *pDevice = static_cast<CsHexagon *> (m_MasterIndependent);

	if( NULL == pDevice )
		return NULL;

	if( 0 == GetBoardNode()->NbTriggerMachine )
	{
		//Device was not fully initialized therefore no slaves
		return pDevice;
	}

	CardNumber = (uInt16) (TriggerIndex + GetBoardNode()->NbTriggerMachine - 1) / GetBoardNode()->NbTriggerMachine;

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

int32	CsHexagon::CommitNewConfiguration( PCSACQUISITIONCONFIG pAcqCfg,
											   	PARRAY_CHANNELCONFIG pAChanCfg,
                                                PARRAY_TRIGGERCONFIG pATrigCfg )
{
	int32					i32Status = CS_SUCCESS;
	CsHexagon				*pDevice = this;
	BOOLEAN					bErrorExtClk = FALSE;
	int32					i3EXtClk2Status = CS_SUCCESS;
	int32					i32FeCalResult(CS_SUCCESS);
	int32 					i32MsCalResult(CS_SUCCESS);
	int32 					i32ExtTrigCalResult(CS_SUCCESS);
	int32					i32Ret = CS_SUCCESS;
	uInt8					u8ImageId = 0;
	bool					bExpertStream = false;
	bool					bStreamSetupChanged = false;


   // Detect over voltage
	i32Status = CheckOverVoltage();
	if ( CS_FAILED(i32Status) )
   {
		m_EventLog.Report( CS_INFO_OVERVOLTAGE_DETECTED, "");
   		return i32Status;
   }
	// Load appropriate FPGA image depending on the mode
	if ( (pAcqCfg->u32Mode & CS_MODE_USER1) != 0 )
	{
		// Convert from user index to the one we use in the driver
		u8ImageId = EXPERT_INDEX_OFFSET;
	}

	bExpertStream	= IsExpertSupported(u8ImageId, DataStreaming );
	m_bMulrecAvgTD	= IsExpertSupported(u8ImageId, Averaging);
	m_bFFTsupported	= IsExpertSupported(u8ImageId, FFT);

	if ( m_Stream.bEnabled != bExpertStream )
	{
		m_Stream.bEnabled = bExpertStream;
		bStreamSetupChanged = true;
	}

	i32Status = CsSetAcquisitionConfig( pAcqCfg );
	if( CS_FAILED(i32Status) )
		return i32Status;

	i32Status = CsSetTriggerConfig( (PARRAY_TRIGGERCONFIG) pATrigCfg );
	if( CS_FAILED(i32Status) )
		return i32Status;

	i32Status = CsSetChannelConfig( (PARRAY_CHANNELCONFIG) pAChanCfg );
	if( CS_FAILED(i32Status) )
		return i32Status;

	if ( IsExtTriggerEnable( pATrigCfg ) )
	SendExtTriggerAlign();

	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		// Restore the default data format
		// Appropriate data format will be set later once calibration is done
		if ( pDevice->IsCalibrationRequired() )
		{
			pDevice->CsSetDataFormat();
			i32FeCalResult = pDevice->CalibrateAllChannels();
		}
		pDevice = pDevice->m_Next;
	}

	 if( CS_FAILED(i32FeCalResult) )
		i32Ret = i32FeCalResult;

	pDevice = m_MasterIndependent;
	while (pDevice)
	{
		if ( CS_SUCCEEDED( i32Status ) )
		{
			if ( m_bMulrecAvgTD )
				pDevice->CsSetDataFormat( formatHwAverage );
			else if ( m_bSoftwareAveraging )
				pDevice->CsSetDataFormat( formatSoftwareAverage );
			else 
				pDevice->CsSetDataFormat();
		}

		pDevice = pDevice->m_Next;
	}

	if ( bStreamSetupChanged )
	{
		m_CaptureMode = m_Stream.bEnabled ? StreamingMode : MemoryMode;
		i32Ret = IoctlStmSetStreamMode( bExpertStream ? 1: 0 );
	}

	UpdateCardState();

	if ( CS_FAILED (i32Ret) )
		return i32Ret;
	else
		return i32Status;
}
