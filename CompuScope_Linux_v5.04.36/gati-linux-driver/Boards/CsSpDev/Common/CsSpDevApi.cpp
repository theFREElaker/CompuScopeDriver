#include "stdafx.h"
#include "CsSpDevice.h"

int32 CsSpDevice::GetAcquisitionSystemInfo(PCSSYSTEMINFO pSysInfo)
{
	// We can catch structure size errors here.
	if (pSysInfo->u32Size < sizeof(CSSYSTEMINFO))
		return CS_INVALID_STRUCT_SIZE; 
	if( ::IsBadWritePtr( pSysInfo, pSysInfo->u32Size ) )
		return CS_INVALID_STRUCT_SIZE; 
	
	::ZeroMemory(pSysInfo, pSysInfo->u32Size);

	pSysInfo->i64MaxMemory = m_u32MemSize * m_Info.usChanNumber; 
	pSysInfo->u32SampleBits = m_Info.usBits;
	pSysInfo->i32SampleResolution = m_Info.lResolution;	
	pSysInfo->u32SampleSize = CS_SAMPLE_SIZE_2;
	pSysInfo->i32SampleOffset = CS_SAMPLE_OFFSET_16;
	pSysInfo->u32BoardType = CSUSB_BOARDTYPE;			
	
	strcpy_s(pSysInfo->strBoardName, _countof(pSysInfo->strBoardName), m_Info.strCsName );
	if( m_bUsb )
		strcat_s( pSysInfo->strBoardName, _countof(pSysInfo->strBoardName), "U" );
	if( m_bPxi )
		strcat_s( pSysInfo->strBoardName, _countof(pSysInfo->strBoardName), "X" );
	if( m_bDcInput)
		strcat_s( pSysInfo->strBoardName, _countof(pSysInfo->strBoardName), "D" );

	pSysInfo->u32AddonOptions = m_bDcAcInput ? CS_USB_AC_DC_INPUT : 0;
	pSysInfo->u32BaseBoardOptions = 0;	
	pSysInfo->u32TriggerMachineCount = 1;
	pSysInfo->u32ChannelCount = m_Info.usChanNumber;		
	pSysInfo->u32BoardCount = 1;  		
		
	return CS_SUCCESS;
}

int32 CsSpDevice::GetBoardsInfo(PARRAY_BOARDINFO pABoardInfo)
{
	if ( ::IsBadWritePtr( pABoardInfo->aBoardInfo, sizeof(ARRAY_BOARDINFO) ) )
		return CS_INVALID_POINTER_BUFFER;
	else
	{	
		if ( pABoardInfo->aBoardInfo[0].u32Size < sizeof(CSBOARDINFO) )
			return CS_INVALID_STRUCT_SIZE;
		pABoardInfo->aBoardInfo[0].u32BoardIndex = 0;
		pABoardInfo->aBoardInfo[0].u32BoardType = CSUSB_BOARDTYPE;
		pABoardInfo->aBoardInfo[0].u32BaseBoardFirmwareVersion = m_nRevFpga2;  
		pABoardInfo->aBoardInfo[0].u32AddonBoardFirmwareVersion = m_nRevFpga1; 
		strcpy_s( pABoardInfo->aBoardInfo[0].strSerialNumber, _countof(pABoardInfo->aBoardInfo[0].strSerialNumber), m_strSerNum );
		pABoardInfo->aBoardInfo[0].u32BaseBoardVersion = m_u32PcbRev; 
		pABoardInfo->aBoardInfo[0].u32AddonBoardVersion = 0x0100; 

		pABoardInfo->aBoardInfo[0].u32AddonFwOptions = 0x0; 
		pABoardInfo->aBoardInfo[0].u32BaseBoardFwOptions = 0x0; 
		pABoardInfo->aBoardInfo[0].u32AddonHwOptions = 0x0; 
		pABoardInfo->aBoardInfo[0].u32BaseBoardHwOptions = 0x0; 
		
	}
	return CS_SUCCESS;
}

int32 CsSpDevice::AcquisitionSystemInit(BOOL bResetDefaultSetting)
{
	if( bResetDefaultSetting )
	{
		//Set Acq parameters
		m_ReqAcq.i64SampleRate = GetMaxSr();
		m_ReqAcq.u32ExtClk = 0;
		m_ReqAcq.u32Mode = 1 == m_Info.usChanNumber ? CS_MODE_SINGLE : CS_MODE_DUAL;
		m_ReqAcq.u32SegmentCount = 1;	
		m_ReqAcq.i64Depth = 0x2000; //8k
		m_ReqAcq.i64SegmentSize = 0x2000; //8k
		m_ReqAcq.i64TriggerDelay = 0;
		m_ReqAcq.i64TriggerHoldoff = 0;
		
		m_ReqAcq.i32SampleOffset= CS_SAMPLE_OFFSET_16;		
		m_ReqAcq.i64TriggerTimeout = -1;	// infinite
		m_ReqAcq.u32TimeStampConfig = TIMESTAMP_GCLK | TIMESTAMP_FREERUN;	
		m_ReqAcq.u32TrigEnginesEn = 0;
		m_ReqAcq.u32SampleBits = m_Info.usBits;
		m_ReqAcq.i32SampleRes = m_Info.lResolution;
		m_ReqAcq.u32SampleSize = CS_SAMPLE_SIZE_2;
		m_ReqAcq.u32ExtClkSampleSkip = 1 == m_Info.usChanNumber ? CS_SAMPLESKIP_FACTOR/2 : CS_SAMPLESKIP_FACTOR;
		
		m_aReqChan[0].u32ChannelIndex = CS_CHAN_1;
		m_aReqChan[1].u32ChannelIndex = CS_CHAN_2; 
		m_aReqChan[0].i32Calib = m_aReqChan[1].i32Calib = 0;
		m_aReqChan[0].i32DcOffset = m_aReqChan[1].i32DcOffset = 0;
		m_aReqChan[0].u32Filter = m_aReqChan[1].u32Filter = 0;
		m_aReqChan[0].u32Impedance = m_aReqChan[1].u32Impedance = CS_REAL_IMP_50_OHM;
		m_aReqChan[0].u32Term = m_aReqChan[1].u32Term = CS_COUPLING_AC;
		m_aReqChan[0].u32InputRange = m_aReqChan[1].u32InputRange = m_bDcInput? SPD_DC_INPUT_RANGE : SPD_INPUT_RANGE;
		
		m_ReqTrig.u32TriggerIndex = 1;
		m_ReqTrig.u32Condition = CS_TRIG_COND_POS_SLOPE;
		m_ReqTrig.i32Level = 0;
		m_ReqTrig.i32Source = CS_TRIG_SOURCE_CHAN_1;
		m_ReqTrig.u32ExtCoupling = CS_COUPLING_DC;
		m_ReqTrig.u32ExtTriggerRange = CS_GAIN_2_V;
		m_ReqTrig.u32ExtImpedance = CS_REAL_IMP_50_OHM;
		m_ReqTrig.i32Value1 = 0;			
		m_ReqTrig.i32Value2 = 0;			
		m_ReqTrig.u32Filter = 0;			
		m_ReqTrig.u32Relation = 0;  
	}

	int32 i32 = ConfigureAcq( &m_ReqAcq, true, false);
	if( CS_FAILED( i32 ) ) return i32;
	for(uInt32 u = 0; u < m_Info.usChanNumber; u++ )
	{
		i32 = ConfigureChan( m_aReqChan + u, true, false);
		if( CS_FAILED( i32 ) ) return i32;
	}

	i32 = ConfigureTrig( &m_ReqTrig, true, false);
	if( CS_FAILED( i32 ) ) return i32;

	return i32;
}
int32 CsSpDevice::AcquisitionSystemCleanup(void)
{
	Abort();
	return CS_SUCCESS;
}
int32 CsSpDevice::GetParams(uInt32 u32ParamID, void* pvParamBuffer)
{
	// Validate Buffer for Write Access
	int32 i32Status = ValidateBufferForReadWrite(u32ParamID, pvParamBuffer, false);
	if (CS_FAILED(i32Status))return i32Status;
	switch (u32ParamID)
	{
		case CS_SYSTEM:
		{
			PCSSYSTEMCONFIG pSysConfig = static_cast<PCSSYSTEMCONFIG>(pvParamBuffer);
			if (0 == pSysConfig->u32Size)	return CS_INVALID_STRUCT_SIZE;
			
			GetParams(CS_ACQUISITION, pSysConfig->pAcqCfg);
			GetParams(CS_CHANNEL, pSysConfig->pAChannels);
			GetParams(CS_TRIGGER, pSysConfig->pATriggers);
			return CS_SUCCESS;
		}
		case CS_ACQUISITION:
		{
			::CopyMemory( pvParamBuffer, &m_Acq, sizeof( m_Acq ) );  
			return CS_SUCCESS;
		}
		
		case CS_TRIGGER:
		{	
			PARRAY_TRIGGERCONFIG pTrigCfg = static_cast<PARRAY_TRIGGERCONFIG>(pvParamBuffer);
			if( 1 != pTrigCfg->aTrigger[0].u32TriggerIndex ) return CS_INVALID_TRIGGER;
			pTrigCfg->u32TriggerCount = 1;
			pTrigCfg->aTrigger[0] = m_Trig;
			return CS_SUCCESS;
		}

		case CS_CHANNEL:
		{	
			PARRAY_CHANNELCONFIG pChanArrayConfig = static_cast<PARRAY_CHANNELCONFIG>(pvParamBuffer);
			if( pChanArrayConfig->u32ChannelCount > m_Info.usChanNumber ) return CS_INVALID_CHANNEL_COUNT;
			for( uInt32 i = 0; i < pChanArrayConfig->u32ChannelCount; i++ )
			{
				if( pChanArrayConfig->aChannel[i].u32ChannelIndex > m_Info.usChanNumber ) return CS_INVALID_CHANNEL;
				pChanArrayConfig->aChannel[i] = m_aChan[pChanArrayConfig->aChannel[i].u32ChannelIndex-1];
			}
			return CS_SUCCESS;
		}	

		case CS_EXTENDED_BOARD_OPTIONS:
			*(static_cast<int64*>(pvParamBuffer)) = 0;
			return CS_SUCCESS;

		case CS_SYSTEMINFO_EX:
		{
			CSSYSTEMINFOEX SysInfoEx = {0};
			SysInfoEx.u32Size = sizeof(CSSYSTEMINFOEX);
			SysInfoEx.u32BaseBoardFwVersions[0].Version.uMinor = (m_nRevFpga1 & 0xff00)>>8;
			SysInfoEx.u32BaseBoardFwVersions[0].Version.uRev = m_nRevFpga1 & 0x00ff;
			SysInfoEx.u32AddonFwVersions[0].Version.uMinor = (m_nRevFpga2 & 0xff00)>>8;
			SysInfoEx.u32AddonFwVersions[0].Version.uRev = m_nRevFpga2 & 0x00ff;
			
			*static_cast<PCSSYSTEMINFOEX>(pvParamBuffer) = SysInfoEx;
			return CS_SUCCESS;
		}

		case CS_READ_VER_INFO:
		{
			CS_FW_VER_INFO VerInfo = {0};
			VerInfo.u32Size = sizeof(CS_FW_VER_INFO);
			VerInfo.AddonFpgaFwInfo.Version.uMinor = (m_nRevFpga2 & 0xff00)>>8;
			VerInfo.AddonFpgaFwInfo.Version.uRev = m_nRevFpga2 & 0x00ff;
			VerInfo.BbFpgaInfo.Version.uMinor = (m_nRevFpga1 & 0xff00)>>8;
			VerInfo.BbFpgaInfo.Version.uRev = m_nRevFpga1 & 0x00ff;
			*static_cast<PCS_FW_VER_INFO>(pvParamBuffer) = VerInfo;

			return CS_SUCCESS;
		}
		case CS_TIMESTAMP_TICKFREQUENCY:
			*static_cast<int64*>(pvParamBuffer) = m_Acq.i64SampleRate * m_Info.usChanNumber;
			return CS_SUCCESS;
		default:
			return CS_INVALID_PARAMS_ID;
	}
}
int32 CsSpDevice::SetParams(uInt32 u32ParamID, void* pvParamBuffer)
{
	int32 i32Status = ValidateBufferForReadWrite(u32ParamID, pvParamBuffer, true);
	if (CS_FAILED(i32Status))return i32Status;
	int32 i32(0);
	uInt32 u(0);

	switch (u32ParamID)
	{
		case CS_ACQUISITION:
			m_ReqAcq = *static_cast<PCSACQUISITIONCONFIG>(pvParamBuffer);
			break;
		
		case CS_TRIGGER:
			m_ReqTrig = (static_cast<PARRAY_TRIGGERCONFIG>(pvParamBuffer))->aTrigger[0];
		
		case CS_CHANNEL:
		{
			PARRAY_CHANNELCONFIG pChanCfg = static_cast<PARRAY_CHANNELCONFIG>(pvParamBuffer);
			for(u = 0; u < pChanCfg->u32ChannelCount; u++ )
			{
				if( pChanCfg->aChannel[u].u32ChannelIndex <= m_Info.usChanNumber )
					m_aReqChan[pChanCfg->aChannel[u].u32ChannelIndex-1] = pChanCfg->aChannel[u];
			}
			return CS_SUCCESS;
		}

		case CS_SYSTEM:
		{
			PCSSYSTEMCONFIG pSysConfig = static_cast<PCSSYSTEMCONFIG>(pvParamBuffer);
			if (0 == pSysConfig->u32Size)	return CS_INVALID_STRUCT_SIZE;
			
			i32 = SetParams(CS_ACQUISITION, pSysConfig->pAcqCfg);
			if( CS_FAILED( i32 ) ) return i32;
			i32 = SetParams(CS_CHANNEL, pSysConfig->pAChannels);
			if( CS_FAILED( i32 ) ) return i32;
			i32 = SetParams(CS_TRIGGER, pSysConfig->pATriggers);
			if( CS_FAILED( i32 ) ) return i32;
			return CS_SUCCESS;
		}
		default:
			return CS_INVALID_PARAMS_ID;

	}
	return CS_SUCCESS;
}
int32 CsSpDevice::ValidateParams(uInt32 u32ParamID, uInt32 u32Coerce, void* pvParamBuffer)
{
	int32 i32Status = ValidateBufferForReadWrite(u32ParamID, pvParamBuffer, true);
	if (CS_FAILED(i32Status))return i32Status;
	int32 i32;
	switch (u32ParamID)
	{
		case CS_SYSTEM:
		{
			PCSSYSTEMCONFIG pSysConfig = static_cast<PCSSYSTEMCONFIG>(pvParamBuffer);
			if (0 == pSysConfig->u32Size)	return CS_INVALID_STRUCT_SIZE;
			
			i32 = ConfigureAcq( pSysConfig->pAcqCfg, u32Coerce != 0, true);
			if( CS_FAILED( i32 ) ) return i32;
			for(uInt32 u = 0; u < pSysConfig->pAChannels->u32ChannelCount; u++ )
			{
				i32 = ConfigureChan( pSysConfig->pAChannels->aChannel + u, u32Coerce != 0, true);
				if( CS_FAILED( i32 ) ) return i32;
			}
			i32 = ConfigureTrig( pSysConfig->pATriggers->aTrigger, u32Coerce != 0, true);
			if( CS_FAILED( i32 ) ) return i32;

			return CS_SUCCESS;
		}
		default:
			return CS_INVALID_PARAMS_ID;
	}
}

int32 CsSpDevice::Do(uInt32 u32ActionID, void* pActionBuffer)
{
	switch (u32ActionID)
	{
		case ACTION_START:
			return StartAcquisition();

		case ACTION_FORCE:
			return ForceTrigger();

		case ACTION_ABORT:
			return Abort();

		case ACTION_RESET:
			return Reset();

		case ACTION_COMMIT:	
		{
			PCSSYSTEMCONFIG pSysConfig = static_cast<PCSSYSTEMCONFIG>(pActionBuffer);
			if (0 == pSysConfig->u32Size)	return CS_INVALID_STRUCT_SIZE;
			
			int32 i32 = ConfigureAcq( pSysConfig->pAcqCfg, false, false);
			if( CS_FAILED( i32 ) ) return i32;
			for(uInt32 u = 0; u < m_Info.usChanNumber; u++ )
			{
				i32 = ConfigureChan( pSysConfig->pAChannels->aChannel + u, false, false);
				if( CS_FAILED( i32 ) ) return i32;
			}
			i32 = ConfigureTrig( pSysConfig->pATriggers->aTrigger, false, false);
			if( CS_FAILED( i32 ) ) return i32;
			return i32;
		}

	case ACTION_TIMESTAMP_RESET:
		return ResetTimeStamp();

	case ACTION_CALIB:
		return CS_SUCCESS;
	default:
		return CS_INVALID_REQUEST;
	}
}

int32 CsSpDevice::ValidateBufferForReadWrite(uInt32 u32ParamID, void* pvParamBuffer, bool bRead)
{
	BOOL	bBadPtr = FALSE;
	uInt32	u32ParamBuffSize;

	switch (u32ParamID)
	{
		case CS_SYSTEM:
		{
			PCSSYSTEMCONFIG pSysCfg = static_cast<PCSSYSTEMCONFIG>(pvParamBuffer);

			if ( NULL == pSysCfg->pAcqCfg || NULL == pSysCfg->pAChannels || NULL == pSysCfg->pATriggers)
				return CS_NULL_POINTER;
			if (bRead)
			{
				// Validate all pointers to application buffer for Read operation
				bBadPtr |= ::IsBadReadPtr(pSysCfg->pAcqCfg, sizeof(CSACQUISITIONCONFIG));
				bBadPtr |= ::IsBadReadPtr(pSysCfg->pAChannels, sizeof(ARRAY_CHANNELCONFIG) + (pSysCfg->pAChannels->u32ChannelCount-1)* sizeof(CSCHANNELCONFIG));
				bBadPtr |= ::IsBadReadPtr(pSysCfg->pATriggers, sizeof(ARRAY_TRIGGERCONFIG) + (pSysCfg->pATriggers->u32TriggerCount-1)* sizeof(CSTRIGGERCONFIG));
			}
			else
			{
				// Validate all pointers to application buffer for Read operation
				bBadPtr |= ::IsBadWritePtr(pSysCfg->pAcqCfg, sizeof(CSACQUISITIONCONFIG));
				bBadPtr |= ::IsBadWritePtr(pSysCfg->pAChannels, sizeof(ARRAY_CHANNELCONFIG) + (pSysCfg->pAChannels->u32ChannelCount-1)* sizeof(CSCHANNELCONFIG));
				bBadPtr |= ::IsBadWritePtr(pSysCfg->pATriggers, sizeof(ARRAY_TRIGGERCONFIG) + (pSysCfg->pATriggers->u32TriggerCount-1)* sizeof(CSTRIGGERCONFIG));
			}

			if (!bBadPtr)
			{
				if (pSysCfg->u32Size < sizeof(CSSYSTEMCONFIG))
					return CS_INVALID_STRUCT_SIZE;

				for (uInt32 i = 0; i< pSysCfg->pATriggers->u32TriggerCount; i++)
					if (pSysCfg->pATriggers->aTrigger[i].u32Size != sizeof(CSTRIGGERCONFIG))
						return CS_INVALID_STRUCT_SIZE;
				for (uInt32 i = 0; i< pSysCfg->pAChannels->u32ChannelCount; i++)
					if (pSysCfg->pAChannels->aChannel[i].u32Size != sizeof(CSCHANNELCONFIG))
						return CS_INVALID_STRUCT_SIZE;
				return CS_SUCCESS;
			}
			else
				return CS_INVALID_POINTER_BUFFER;
		}

		case CS_ACQUISITION:
		{
			PCSACQUISITIONCONFIG pAcqCfg = static_cast<PCSACQUISITIONCONFIG>(pvParamBuffer);
			if (bRead)
				bBadPtr = ::IsBadReadPtr(pvParamBuffer, sizeof(CSACQUISITIONCONFIG));
			else
				bBadPtr = ::IsBadWritePtr(pvParamBuffer, sizeof(CSACQUISITIONCONFIG));

			if (bBadPtr)
				return CS_INVALID_POINTER_BUFFER;
			else if (pAcqCfg->u32Size < sizeof(CSACQUISITIONCONFIG)) 
				return CS_INVALID_STRUCT_SIZE;
			else
				return CS_SUCCESS;
		}

		case CS_TRIGGER:
		{
			PARRAY_TRIGGERCONFIG pATrigCfg = static_cast<PARRAY_TRIGGERCONFIG>(pvParamBuffer);
			u32ParamBuffSize = (pATrigCfg->u32TriggerCount-1) * sizeof(CSTRIGGERCONFIG) + sizeof(ARRAY_TRIGGERCONFIG);
			// Validate pointers to the application buffer for Read/Write operation
			if (bRead)
				bBadPtr = ::IsBadReadPtr(pATrigCfg->aTrigger, u32ParamBuffSize);
			else
				bBadPtr = ::IsBadWritePtr(pATrigCfg->aTrigger, u32ParamBuffSize);

			if (bBadPtr)
				return CS_INVALID_POINTER_BUFFER;
			else
				for (uInt32 i = 0; i< pATrigCfg->u32TriggerCount; i++)
					if (pATrigCfg->aTrigger[i].u32Size != sizeof(CSTRIGGERCONFIG))
						return CS_INVALID_STRUCT_SIZE;
			return CS_SUCCESS;
		}

		case CS_CHANNEL:
		{
			PARRAY_CHANNELCONFIG pAChanCfg = static_cast<PARRAY_CHANNELCONFIG>(pvParamBuffer);
			u32ParamBuffSize = (pAChanCfg->u32ChannelCount-1) * sizeof(CSCHANNELCONFIG) + sizeof(ARRAY_CHANNELCONFIG);
			// Validate pointers to the application buffer for Read/Write operation
			if ( bRead )
				bBadPtr = ::IsBadReadPtr(pAChanCfg->aChannel, u32ParamBuffSize);
			else
				bBadPtr = ::IsBadWritePtr(pAChanCfg->aChannel, u32ParamBuffSize);

			if (bBadPtr)
				return CS_INVALID_POINTER_BUFFER;
			else
				for (uInt32 i = 0; i< pAChanCfg->u32ChannelCount; i++)
					if (pAChanCfg->aChannel[i].u32Size != sizeof(CSCHANNELCONFIG))
						return CS_INVALID_STRUCT_SIZE;
			return CS_SUCCESS;
		}

		case CS_BOARD_INFO:
		{
			PARRAY_BOARDINFO pABoardInfo = static_cast<PARRAY_BOARDINFO>(pvParamBuffer);
			u32ParamBuffSize = (pABoardInfo->u32BoardCount-1) * sizeof(CSBOARDINFO) + sizeof(ARRAY_BOARDINFO);
			// Validate pointers to the application buffer for Read/Write operation
			if (bRead)
				bBadPtr = ::IsBadReadPtr(pABoardInfo->aBoardInfo, u32ParamBuffSize);
			else
				bBadPtr = ::IsBadWritePtr(pABoardInfo->aBoardInfo, u32ParamBuffSize);

			if (bBadPtr)
				return CS_INVALID_POINTER_BUFFER;
			else
				for (uInt32 i = 0; i< pABoardInfo->u32BoardCount; i++)
					if (pABoardInfo->aBoardInfo[i].u32Size != sizeof(CSBOARDINFO))
						return CS_INVALID_STRUCT_SIZE;
			return CS_SUCCESS;
		}
		case CS_SYSTEMINFO_EX:
		{
			PCSSYSTEMINFOEX pBdInfoEx = static_cast<PCSSYSTEMINFOEX>(pvParamBuffer);
			u32ParamBuffSize = sizeof(CSSYSTEMINFOEX);

			if (bRead)
				bBadPtr = ::IsBadReadPtr(pvParamBuffer, sizeof(CSSYSTEMINFOEX));
			else
				bBadPtr = ::IsBadWritePtr(pvParamBuffer, sizeof(CSSYSTEMINFOEX));

			if (bBadPtr)
				return CS_INVALID_POINTER_BUFFER;
			else if (pBdInfoEx->u32Size < sizeof(CSSYSTEMINFOEX)) 
				return CS_INVALID_STRUCT_SIZE;
			else
				return CS_SUCCESS;
		}
		case CS_READ_VER_INFO:
		{
			bBadPtr = ::IsBadWritePtr( pvParamBuffer, sizeof(CS_FW_VER_INFO) );
			if (bBadPtr)
				return CS_INVALID_POINTER_BUFFER;
			if ( static_cast<PCS_FW_VER_INFO>(pvParamBuffer)->u32Size != sizeof( CS_FW_VER_INFO ) )
				return CS_INVALID_STRUCT_SIZE;
		}
		case CS_EXTENDED_BOARD_OPTIONS:
			if (bRead)
				bBadPtr = ::IsBadReadPtr(pvParamBuffer, sizeof(int64));
			else
				bBadPtr = ::IsBadWritePtr(pvParamBuffer, sizeof(int64));

			if (bBadPtr)
				return CS_INVALID_POINTER_BUFFER;
			else
				return CS_SUCCESS;

		case CS_TIMESTAMP_TICKFREQUENCY:
			if ( IsBadWritePtr( (int64*) pvParamBuffer, sizeof(int64) ) )
				return CS_INVALID_POINTER_BUFFER;
			else
				return CS_SUCCESS;

		default:
			return CS_INVALID_REQUEST;
	}
}

