#include "StdAfx.h"
#include "CsTypes.h"
#include "GageWrapper.h"
#include "CsDefines.h"
#include "CsRabbit.h"


//-----------------------------------------------------------------------------
//------ CsGetAcquisitionConfig
//-----------------------------------------------------------------------------

int32	CsRabbitDev::CsGetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg )
{
	// User struct size may be smaller than driver struct size
	uInt32		u32UserStructSize = pAcqCfg->u32Size;

	RtlCopyMemory( pAcqCfg, &m_pCardState->AcqConfig, pAcqCfg->u32Size ); 
	pAcqCfg->u32Size = u32UserStructSize;

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//CsGetChannelConfig
//-----------------------------------------------------------------------------
int32	CsRabbitDev::CsGetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg )
{
	CsRabbitDev	*pDevice = this;
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
int32	CsRabbitDev::CsGetTriggerConfig( PARRAY_TRIGGERCONFIG pATrigCfg )
{
	CsRabbitDev		*pDevice = m_MasterIndependent;
	uInt16				NormalizedTrigIndex = 0;
	PCSTRIGGERCONFIG	pTrigCfg = pATrigCfg->aTrigger;
	BOOLEAN				bFoundTrigIndex = FALSE;


	for (uInt32 i = 0; i < pATrigCfg->u32TriggerCount; i++ )
	{
		// Searching for the user trigger index
		pDevice = m_MasterIndependent;
		while( pDevice )
		{
			for ( uInt16 j = 0; j < GetBoardNode()->NbTriggerMachine; j++ )
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
int32	CsRabbitDev::CsGetBoardsInfo( PARRAY_BOARDINFO pABoardInfo )
{
	CsRabbitDev	*pDevice;
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

int32	CsRabbitDev::CsSetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg )
{
	int32	i32Ret = CS_SUCCESS;


	if( !m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( pAcqCfg )
	{
		if ( m_bNucleon )
			PreCalculateStreamDataSize( pAcqCfg );

		m_pCardState->AcqConfig.u32TrigEnginesEn	= pAcqCfg->u32TrigEnginesEn;
		m_pCardState->AcqConfig.i64TriggerDelay		= pAcqCfg->i64TriggerDelay;
		m_pCardState->AcqConfig.i64TriggerHoldoff	= pAcqCfg->i64TriggerHoldoff;
		m_pCardState->AcqConfig.u32SegmentCount		= pAcqCfg->u32SegmentCount;
		m_pCardState->AcqConfig.i64Depth			= pAcqCfg->i64Depth;
		m_pCardState->AcqConfig.u32TimeStampConfig	= pAcqCfg->u32TimeStampConfig;
		m_pCardState->AcqConfig.i64SegmentSize		= pAcqCfg->i64SegmentSize;

		// Some algorithm relies on the order of function calls
		// Do not change the order of
		// SendClockSetting
		// SendCaptureModeSetting

		m_bSingleChannel2 = (0 != (pAcqCfg->u32Mode & CS_MODE_SINGLE_CHANNEL2));
		i32Ret = SendClockSetting( pAcqCfg->u32Mode, pAcqCfg->i64SampleRate,
						           pAcqCfg->u32ExtClk, (pAcqCfg->u32Mode & CS_MODE_REFERENCE_CLK) != 0 );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = SendCaptureModeSetting( pAcqCfg->u32Mode, m_bSingleChannel2 ? 1: 2  );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		// Convert TriggerTimeout from 100 ns to number of memory locations
		// Each memory location contains 16 Samples (Single) or 2x8 Samples (Dual)
		m_pCardState->AcqConfig.i64TriggerTimeout = pAcqCfg->i64TriggerTimeout;

		uInt32 u32HwTimeout = (uInt32) pAcqCfg->i64TriggerTimeout;
		if( CS_TIMEOUT_DISABLE != u32HwTimeout && 0 != u32HwTimeout)
		{
			LONGLONG llTimeout = (LONGLONG)u32HwTimeout * pAcqCfg->i64SampleRate;
			llTimeout /= (m_pCardState->AcqConfig.u32Mode & CS_MODE_DUAL) ? 8: 16;
			// llTimeout was calculated using 100ns unit trigger timeout. Convert back to 1 second unit
			llTimeout += 9999999;
			llTimeout /= 10000000; 

			if ( llTimeout >= MAX_HW_DEPTH_COUNTER )
				m_pCardState->AcqConfig.i64TriggerTimeout = (int32) (u32HwTimeout = (uInt32) CS_TIMEOUT_DISABLE);
			else
				u32HwTimeout = (uInt32)llTimeout;
		}

		SetTriggerTimeout (u32HwTimeout);

		m_u32HwSegmentCount = pAcqCfg->u32SegmentCount;
		m_i64HwSegmentSize = m_pCardState->AcqConfig.i64SegmentSize;
		m_i64HwPostDepth = m_pCardState->AcqConfig.i64Depth;
		m_i64HwTriggerHoldoff = m_pCardState->AcqConfig.i64TriggerHoldoff;
		m_i64HwTriggerDelay		= m_pCardState->AcqConfig.i64TriggerDelay;

		// Adjust Segemnt Parameters for Hardware
		AdjustedHwSegmentParameters( m_pCardState->AcqConfig.u32Mode, &m_u32HwSegmentCount, &m_i64HwSegmentSize, &m_i64HwPostDepth );
		if ( m_bNucleon && m_bOneSampleResolution )
			AdjustForOneSampleResolution ( m_pCardState->AcqConfig.u32Mode, &m_i64HwSegmentSize, &m_i64HwPostDepth, &m_i64HwTriggerDelay );

		CsRabbitDev	*pDevice = m_MasterIndependent;
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

		if ( !IsExpertAVG() && (( CS_MODE_SW_AVERAGING & pAcqCfg->u32Mode ) != 0 ) )
			m_bSoftwareAveraging = TRUE;
		else
            m_bSoftwareAveraging = FALSE;

		if ( m_bSoftwareAveraging )
		{
			if ( NULL == m_pAverageBuffer )
			{
				uInt32	u32MaxAverageSamples = MAX_SW_AVERAGING_SEMGENTSIZE + CsGetPostTriggerExtraSamples(pAcqCfg->u32Mode);
				uInt32	u32MaxAverageBufferSize = u32MaxAverageSamples * sizeof(int8);

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

		if ( m_bNucleon && m_Stream.bEnabled )
			PreCalculateStreamDataSize( pAcqCfg );
	}
	else
	{
		// Default setting
		m_pCardState->AcqConfig.u32ExtClk = 0;
		m_pCardState->AcqConfig.u32ExtClkSampleSkip = m_u32DefaultExtClkSkip;
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

		uInt32 u32DefaultMode = CSRB_DEFAULT_MODE;
		if ( 1 == GetBoardNode()->NbAnalogChannel || m_pCardState->bLAB11G )
			u32DefaultMode = CS_MODE_SINGLE;

		i32Ret = SendClockSetting( u32DefaultMode );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = SetClockOut(m_pCardState->ermRoleMode, m_pCardState->eclkoClockOut);
		if( CS_FAILED(i32Ret) )	return i32Ret;

		i32Ret = SendCaptureModeSetting( u32DefaultMode );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		i32Ret = WriteRegisterThruNios( 0, CSPLXBASE_ONE_SAMPLE_RESOLUITON_ADJ );
		if( CS_FAILED(i32Ret) )
			return i32Ret;


		SetTriggerTimeout();
		CsSetDataFormat();

		m_pCardState->AcqConfig.i64TriggerTimeout	= CS_TIMEOUT_DISABLE;
		m_pCardState->AcqConfig.u32TrigEnginesEn	= CSRB_DEFAULT_TRIG_ENGINES_EN;
		m_pCardState->AcqConfig.i64TriggerDelay	= CSRB_DEFAULT_TRIG_DELAY;
		m_pCardState->AcqConfig.i64TriggerHoldoff	= CSRB_DEFAULT_TRIG_HOLD_OFF;
		m_pCardState->AcqConfig.u32SegmentCount	= CSRB_DEFAULT_SEGMENT_COUNT;
		m_pCardState->AcqConfig.i64Depth			= CSRB_DEFAULT_DEPTH;
		m_pCardState->AcqConfig.i64SegmentSize	= m_pCardState->AcqConfig.i64Depth + m_pCardState->AcqConfig.i64TriggerHoldoff;

		m_u32HwSegmentCount	= m_pCardState->AcqConfig.u32SegmentCount;
		m_i64HwSegmentSize	= m_pCardState->AcqConfig.i64SegmentSize;
		m_i64HwPostDepth		= m_pCardState->AcqConfig.i64Depth;
		m_i64HwTriggerHoldoff = m_pCardState->AcqConfig.i64TriggerHoldoff;
		m_i64HwTriggerDelay		= m_pCardState->AcqConfig.i64TriggerDelay;

		// Adjust Segemnt Parameters for Hardware
		AdjustedHwSegmentParameters( m_pCardState->AcqConfig.u32Mode, &m_u32HwSegmentCount, &m_i64HwSegmentSize, &m_i64HwPostDepth );

		CsRabbitDev* pDevice = m_MasterIndependent;
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

	CSACQUISITIONCONFIG_EX AcqCfgEx = {0};

	AcqCfgEx.AcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG); 
	CsGetAcquisitionConfig( &AcqCfgEx.AcqCfg );

	AcqCfgEx.i64HwDepth			= m_i64HwPostDepth;
	AcqCfgEx.i64HwSegmentSize	= m_i64HwSegmentSize;
	AcqCfgEx.i64HwTrigDelay		= m_i64HwTriggerDelay;
	AcqCfgEx.u32ExpertOption	= m_PlxData->CsBoard.u32BaseBoardOptions[m_pCardState->u8ImageId];
	AcqCfgEx.u32Decimation		= m_pCardState->AddOnReg.u32Decimation;
	IoctlSetAcquisitionConfigEx( &AcqCfgEx );

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CsRabbitDev *CsRabbitDev::GetCardPointerFromChannelIndex( uInt16 u16ChannelIndex )
{
	CsRabbitDev		*pDevice = static_cast<CsRabbitDev *> (m_MasterIndependent);
	uInt16			u16CardNumber;

	if ( ! m_bSingleChannel2 )
	{
		if ( ( m_pCardState->AcqConfig.u32Mode & CS_MODE_SINGLE ) &&
			 ( u16ChannelIndex % 2  == 0 ) &&
			 (0 == (GetBoardNode()->u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_1CHANNEL)) )
		{
			// ChannelIndex should be odd in single mode on two channel boards
			return NULL;
		}
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

int32	CsRabbitDev::CsSetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg )
{
	int32				i32Status = CS_SUCCESS;

	if( !m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	//m_u32IgnoreCalibErrorMaskLocal = m_u32IgnoreCalibErrorMask;

	if ( pAChanCfg )
	{
		CsRabbitDev			*pDevice = (CsRabbitDev *)m_MasterIndependent;
		PCSCHANNELCONFIG	pChanCfg = pAChanCfg->aChannel;
		uInt16				u16NormalizedChanIndex = CS_CHAN_1;


		for (uInt32 i = 0; i < pAChanCfg->u32ChannelCount; i++ )
		{
			if ( 0 == (GetBoardNode()->u32AddonHardwareOptions & CSRB_ADDON_HW_OPTION_1CHANNEL) )
				u16NormalizedChanIndex = (pChanCfg[i].u32ChannelIndex % 2) ? CS_CHAN_1: CS_CHAN_2;

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
		i32Status = SendChannelSetting(CS_CHAN_1);
		if ( CS_FAILED(i32Status) )
			return i32Status;

		i32Status = SendChannelSetting(CS_CHAN_2);
		if ( CS_FAILED(i32Status) )
			return i32Status;

		i32Status = SendCalibModeSetting(CS_CHAN_1);
		if ( CS_FAILED(i32Status) )
			return i32Status;

		i32Status = SendCalibModeSetting(CS_CHAN_2);
		if ( CS_FAILED(i32Status) )
			return i32Status;

		i32Status = CalibrateAllChannels();
		if ( CS_FAILED(i32Status) )
			return i32Status;
	}

	return i32Status;
}


//-----------------------------------------------------------------------------
//CsSetTriggerConfig
//-----------------------------------------------------------------------------

int32	CsRabbitDev::CsSetTriggerConfig( PARRAY_TRIGGERCONFIG pATrigCfg )
{
	int32	i32Status = CS_SUCCESS;
	uInt8	u8IndexTrig;
	uInt32	i;

	if( !m_PlxData->InitStatus.Info.u32Nios )
		return CS_FLASH_NOT_INIT;

	if ( pATrigCfg )
	{
		CsRabbitDev			*pDevice = m_MasterIndependent;
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
				i32Status = pDevice->GetFreeTriggerEngine( pTrigCfg[i].i32Source, &u16NormalizedTrigIndex );
				if ( CS_FAILED(i32Status) )
					return i32Status;

				// Keep track of User Trigger Index
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32UserTrigIndex = pTrigCfg[i].u32TriggerIndex;

				i32NormalizedTrigSource = NormalizedTriggerSource( pTrigCfg[i].i32Source );
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Source = i32NormalizedTrigSource;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32Condition = pTrigCfg[i].u32Condition;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Level = pTrigCfg[i].i32Level;

				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value1 = CSRB_DEFAULT_VALUE;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value2 = CSRB_DEFAULT_VALUE;
				pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32Filter = CSRB_DEFAULT_VALUE;

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

						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value1 = CSRB_DEFAULT_VALUE;
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].i32Value2 = CSRB_DEFAULT_VALUE;
						pDevice->m_CfgTrigParams[u16NormalizedTrigIndex].u32Filter = CSRB_DEFAULT_VALUE;

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

			if ( m_pCardState->bLAB11G )
				i32Status = pDevice->SendTriggerEngineSetting( pDevice->m_CfgTrigParams[u8IndexTrig].i32Source,
															   pDevice->m_CfgTrigParams[u8IndexTrig].u32Condition,
															   pDevice->m_CfgTrigParams[u8IndexTrig].i32Level,
															   pDevice->m_CfgTrigParams[u8IndexTrig+1].i32Source,
															   pDevice->m_CfgTrigParams[u8IndexTrig+1].u32Condition,
															   pDevice->m_CfgTrigParams[u8IndexTrig+1].i32Level,
															   (pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].i32Source != 0) ? CS_CHAN_2 : 0,
															   pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32Condition,
															   pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].i32Level);
			else
				i32Status = pDevice->SendTriggerEngineSetting( pDevice->m_CfgTrigParams[u8IndexTrig].i32Source,
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

			// Update Ext trigger params for all trigger engines
			pDevice->m_pCardState->TriggerParams[CSRB_TRIGENGINE_A1].u32ExtTriggerRange = pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange;
			pDevice->m_pCardState->TriggerParams[CSRB_TRIGENGINE_A1].u32ExtCoupling = pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtCoupling;
			pDevice->m_pCardState->TriggerParams[CSRB_TRIGENGINE_A1].u32ExtImpedance = pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance;

			pDevice->m_pCardState->TriggerParams[CSRB_TRIGENGINE_B1].u32ExtTriggerRange = pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange;
			pDevice->m_pCardState->TriggerParams[CSRB_TRIGENGINE_B1].u32ExtCoupling = pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtCoupling;
			pDevice->m_pCardState->TriggerParams[CSRB_TRIGENGINE_B1].u32ExtImpedance = pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance;

			pDevice->m_pCardState->TriggerParams[CSRB_TRIGENGINE_A2].u32ExtTriggerRange = pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange;
			pDevice->m_pCardState->TriggerParams[CSRB_TRIGENGINE_A2].u32ExtCoupling = pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtCoupling;
			pDevice->m_pCardState->TriggerParams[CSRB_TRIGENGINE_A2].u32ExtImpedance = pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance;

			pDevice->m_pCardState->TriggerParams[CSRB_TRIGENGINE_B2].u32ExtTriggerRange = pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange;
			pDevice->m_pCardState->TriggerParams[CSRB_TRIGENGINE_B2].u32ExtCoupling = pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtCoupling;
			pDevice->m_pCardState->TriggerParams[CSRB_TRIGENGINE_B2].u32ExtImpedance = pDevice->m_CfgTrigParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance;

			// Remember the user trigger index
			for ( i = 0; i < GetBoardNode()->NbTriggerMachine; i ++ )
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
		for ( i = CSRB_TRIGENGINE_A1 ; i < GetBoardNode()->NbTriggerMachine; i ++ )
		{
			m_pCardState->TriggerParams[i].u32Relation = CSRB_DEFAULT_RELATION;
			m_pCardState->TriggerParams[i].i32Value1 = CSRB_DEFAULT_VALUE;
			m_pCardState->TriggerParams[i].i32Value2 = CSRB_DEFAULT_VALUE;
			m_pCardState->TriggerParams[i].u32Filter = CSRB_DEFAULT_VALUE;
			m_pCardState->TriggerParams[i].u32ExtTriggerRange = CSRB_DEFAULT_EXT_GAIN;
			m_pCardState->TriggerParams[i].u32ExtCoupling = CSRB_DEFAULT_EXT_COUPLING;
			m_pCardState->TriggerParams[i].u32ExtImpedance = CSRB_DEFAULT_EXT_IMPEDANCE;
			m_pCardState->TriggerParams[i].u32UserTrigIndex = i + GetBoardNode()->NbTriggerMachine*(m_pCardState->u16CardNumber-1);
		}

		// Default trigger from Channel 1 master
		u8IndexTrig = CSRB_TRIGENGINE_A1;
		if ( 1 == m_pCardState->u16CardNumber )
			m_pCardState->TriggerParams[u8IndexTrig].i32Source = 1;
		else
			m_pCardState->TriggerParams[u8IndexTrig].i32Source = 0;

		i32Status = SendTriggerEngineSetting( m_pCardState->TriggerParams[u8IndexTrig].i32Source );
		if ( CS_FAILED(i32Status) )
			return i32Status;

		m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32Relation = CSRB_DEFAULT_RELATION;
		m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].i32Value1 = CSRB_DEFAULT_VALUE;
		m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].i32Value2 = CSRB_DEFAULT_VALUE;
		m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32Filter = CSRB_DEFAULT_VALUE;
		m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtTriggerRange = m_pCardState->u32DefaultExtTrigRange;
		m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtCoupling = m_pCardState->u32DefaultExtTrigCoupling;
		m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32ExtImpedance = m_pCardState->u32DefaultExtTrigImpedance;
		m_pCardState->TriggerParams[CSRB_TRIGENGINE_EXT].u32UserTrigIndex = GetBoardNode()->NbTriggerMachine*m_pCardState->u16CardNumber;
		m_pCardState->AcqConfig.u32TrigEnginesEn = CSRB_DEFAULT_TRIG_ENGINES_EN; // also in acquisition

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

CsRabbitDev *CsRabbitDev::GetCardPointerFromTriggerSource( int32 i32TriggerSoure )
{
	CsRabbitDev *pDevice = static_cast<CsRabbitDev *> (m_MasterIndependent);

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

CsRabbitDev *CsRabbitDev::GetCardPointerFromTriggerIndex( uInt16 TriggerIndex )
{
	uInt16 CardNumber;
	CsRabbitDev *pDevice = static_cast<CsRabbitDev *> (m_MasterIndependent);

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

int32	CsRabbitDev::CommitNewConfiguration( PCSACQUISITIONCONFIG pAcqCfg,
											   	PARRAY_CHANNELCONFIG pAChanCfg,
                                                PARRAY_TRIGGERCONFIG pATrigCfg )
{
	int32					i32Status = CS_SUCCESS;
	CsRabbitDev				*pDevice = this;
	BOOLEAN					bErrorExtClk = FALSE;
	int32					i3EXtClk2Status = CS_SUCCESS;
	int32					i32FeCalResult(CS_SUCCESS);
	int32 					i32MsCalResult(CS_SUCCESS);
	int32 					i32ExtTrigCalResult(CS_SUCCESS);
	int32					i32Ret = CS_SUCCESS;
	uInt8					u8LoadImageId = 0;

	// Validation of Ext Clock
	if ( pAcqCfg->u32ExtClk )
	{
		i3EXtClk2Status = ValidateExtClockFrequency( pAcqCfg->u32Mode, (uInt32) pAcqCfg->i64SampleRate, pAcqCfg->u32ExtClkSampleSkip );
		if( CS_FAILED(i3EXtClk2Status) )
		{
			bErrorExtClk = TRUE;
			goto OnExitCommit;
		}
	}

	// Load appropriate FPGA image depending on the mode
	if ( (pAcqCfg->u32Mode & CS_MODE_USER1) != 0 )
		u8LoadImageId = 1;
	else if ( (pAcqCfg->u32Mode & CS_MODE_USER2) != 0 )
		u8LoadImageId = 2;
	else
		u8LoadImageId = 0;

	pDevice = m_MasterIndependent;
	while (pDevice)
	{
		i32Status = pDevice->CsLoadBaseBoardFpgaImage( u8LoadImageId );
		if( CS_FAILED(i32Status) )
			return i32Status;

		i32Status = pDevice->CsSetAcquisitionConfig( pAcqCfg );
		if( CS_FAILED(i32Status) )
			return i32Status;

		pDevice = pDevice->m_Next;

	}
	if ( m_bCobraMaxPCIe )
	{
		i32Status = SendInternalHwExtTrigAlign();
		if( CS_FAILED(i32Status) )
			return i32Status;
	}	

	i32Status = CsSetTriggerConfig( (PARRAY_TRIGGERCONFIG) pATrigCfg );
	if( CS_FAILED(i32Status) )
		return i32Status;

	i32Status = CsSetChannelConfig( (PARRAY_CHANNELCONFIG) pAChanCfg );
	if( CS_FAILED(i32Status) )
		return i32Status;

	pDevice = m_MasterIndependent;
	while ( pDevice )
	{
		// Restore the default data format
		// Appropriate data format will be set later once calibration is done
		if ( pDevice->IsCalibrationRequired() )
		{
			if ( m_bMulrecAvgTD )
			{
				pDevice->SetupHwAverageMode(false);
				pDevice->CsSetDataFormat( formatHwAverage );
			}
			else
				pDevice->CsSetDataFormat();
		}
		i32FeCalResult = pDevice->CalibrateAllChannels();
		pDevice = pDevice->m_Next;
	}

OnExitCommit:
	
	// Calibration for M/S skew
	if ( m_bNucleon )
		i32MsCalResult = MsCalibrateSkew(IsExtTriggerEnable( pATrigCfg ));
	else
	{
		if ( CS_SUCCEEDED (i32Status) && ( m_pSystem->SysInfo.u32BoardCount > 1 ) )
			i32MsCalResult = CalibrateMasterSlave();

		// Calibration for ext trigger address offset
		if ( IsExtTriggerEnable( pATrigCfg ) )
		{
			i32ExtTrigCalResult = m_MasterIndependent->CalibrateExtTrigger();
			if ( CS_SUCCEEDED(i32ExtTrigCalResult) )
				m_MasterIndependent->m_pCardState->bExtTrigCalib = false;
		}
	}

	if ( ! bErrorExtClk )
	{
		// Restore the current decimation
		pDevice = m_MasterIndependent;
		while ( pDevice )
		{
			pDevice->m_bPreCalib = false;
			pDevice->CsSetAcquisitionConfig( pAcqCfg );
			pDevice = pDevice->m_Next;
		}
	}

 	i32Status = VerifyTriggerOffset();

	if ( CS_FAILED (i3EXtClk2Status) )
		i32Ret = i3EXtClk2Status;
	else if( CS_FAILED(i32FeCalResult) )
		i32Ret = i32FeCalResult;
	else if( CS_FAILED( i32MsCalResult ) )
		i32Ret = i32MsCalResult;
	else if( CS_FAILED(i32ExtTrigCalResult ) )
		i32Ret = i32ExtTrigCalResult;

	if ( m_MasterIndependent->m_Next &&
		 m_MasterIndependent->m_pCardState->bMsDecimationReset )
	{
		m_MasterIndependent->ResetMSDecimationSynch();
	}

	pDevice = m_MasterIndependent;
	while (pDevice)
	{
		if ( CS_SUCCEEDED( i32Status ) )
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

			if ( m_bHistogram )
				pDevice->HistogramReset();
		}

		pDevice->UpdateCardState();
		pDevice = pDevice->m_Next;
	}

	if ( m_bNucleon )
		MsAdjustDigitalDelay();

	i32Status = SetClockOut(m_pCardState->ermRoleMode, m_pCardState->eclkoClockOut);

	if ( CS_FAILED (i32Ret) )
		return i32Ret;
	else
		return i32Status;
}

