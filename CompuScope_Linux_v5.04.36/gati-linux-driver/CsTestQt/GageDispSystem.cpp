#include "GageDispSystem.h"
#include "gage_drv.h"
#include "CsPrivatePrototypes.h"
#include <QtCore>
#include <QTime>

CGageDispSystem::CGageDispSystem(CsTestErrMng* ErrMng)
{

	m_ErrMng = ErrMng;
	m_phDrvHdl = (DRVHANDLE*)malloc(MAX_NB_HANDLES * sizeof(CSHANDLE));
	m_CsHdl = 0;
	m_hSystem = 0;
	m_i64LastValidHoldoff = 0;
	m_i64LastValidDepth = 0;
    m_bCoerceFlag = false;
}


CGageDispSystem::~CGageDispSystem(void)
{
	/*
	if ( m_hModule )
	{
		::FreeLibrary(m_hModule);
	}
	*/
	free(m_phDrvHdl);
}


int32 CGageDispSystem::InitStructures()
{
	// Initialize arrays for DISP system.
	// CHANNELCONFIG array
	if (m_pChannelConfigArray) free(m_pChannelConfigArray);
	size_t tSize = ((m_SystemInfo.u32ChannelCount - 1) * sizeof(CSCHANNELCONFIG)) + sizeof(ARRAY_CHANNELCONFIG);
	m_pChannelConfigArray = static_cast<PARRAY_CHANNELCONFIG>(::malloc(tSize));
	if (!m_pChannelConfigArray)
	{
//		m_ErrMng->UpdateErrMsg("Error allocating memory for the channels.", i32Status);
		return -1;
	}

	// TRIGGERCONFIG array
	if (m_pTriggerConfigArray) free(m_pTriggerConfigArray);
	tSize = ((m_SystemInfo.u32TriggerMachineCount - 1) * sizeof(CSTRIGGERCONFIG)) + sizeof(ARRAY_TRIGGERCONFIG);
	m_pTriggerConfigArray = static_cast<PARRAY_TRIGGERCONFIG>(::malloc(tSize));
	if (!m_pTriggerConfigArray)
	{
//		m_ErrMng->UpdateErrMsg("Error allocating memory for the triggers.", i32Status);
		return -1;
	}

	// BOARDINFO array
	tSize = (m_SystemInfo.u32BoardCount) * sizeof(CSBOARDINFO) + sizeof(ARRAY_BOARDINFO);
	m_pBoardInfoArray = static_cast<PARRAY_BOARDINFO>(::malloc(tSize));
	if (!m_pBoardInfoArray)
	{
//		m_ErrMng->UpdateErrMsg("Error allocating memory for board information.", i32Status);
		return -1;
	}
	m_pBoardInfoArray->u32BoardCount = m_SystemInfo.u32BoardCount;
	for (uInt32 i = 0; i < m_SystemInfo.u32BoardCount; i++)
	{
		m_pBoardInfoArray->aBoardInfo[i].u32Size = sizeof(CSBOARDINFO);
	}

	m_pChannelConfigArray->u32ChannelCount = m_SystemInfo.u32ChannelCount;
	for (uInt32 i = 0; i < m_SystemInfo.u32ChannelCount; i++)
	{
		m_pChannelConfigArray->aChannel[i].u32Size = sizeof(CSCHANNELCONFIG);
		m_pChannelConfigArray->aChannel[i].u32ChannelIndex = i + 1;
	}

	m_pTriggerConfigArray->u32TriggerCount = m_SystemInfo.u32TriggerMachineCount;
	for (uInt32 i = 0; i < m_SystemInfo.u32TriggerMachineCount; i++)
	{
		m_pTriggerConfigArray->aTrigger[i].u32Size = sizeof(CSTRIGGERCONFIG);
		m_pTriggerConfigArray->aTrigger[i].u32TriggerIndex = i + 1;
		m_pTriggerConfigArray->aTrigger[i].i32Source = 0;
	}

	m_AcquisitionConfig.u32Size = sizeof(m_AcquisitionConfig);
	m_SystemConfig.u32Size = sizeof(CSSYSTEMCONFIG);
	m_SystemConfig.pAcqCfg = &m_AcquisitionConfig;
	m_SystemConfig.pAChannels = m_pChannelConfigArray;
	m_SystemConfig.pATriggers = m_pTriggerConfigArray;


	return CS_SUCCESS;
}

int32 CGageDispSystem::Initialize()
{
	uInt16 u16SystemsFound;

    int32 i32Status = CsDrvGetAcquisitionSystemCount(FALSE, &u16SystemsFound);
    if(i32Status<0) return i32Status;

	return static_cast<int32>(u16SystemsFound);
}

int32 CGageDispSystem::FreeSystem(CSHANDLE csHandle)
{
    int i32Status;

    i32Status = CsDrvAcquisitionSystemCleanup(csHandle);

    return i32Status;
}

int32 CGageDispSystem::FreeSystem()
{
    int i32Status;

    i32Status = CsDrvAcquisitionSystemCleanup(m_hSystem);

    return i32Status;
}

int32 CGageDispSystem::GetSerialNumber(CSHANDLE hSystem, char *pSerialNumber)
{    
    //Get serial number.
    int Res;
    ARRAY_BOARDINFO *pBoardsInfo = (ARRAY_BOARDINFO*)malloc(10 * sizeof(ARRAY_BOARDINFO));
    pBoardsInfo->u32BoardCount = 1;
    pBoardsInfo->aBoardInfo[0].u32Size = sizeof(CSBOARDINFO);
    pBoardsInfo->aBoardInfo[0].u32BoardIndex = 1;
    strcpy(pBoardsInfo->aBoardInfo[0].strSerialNumber, "");
    Res = CsDrvGetBoardsInfo(hSystem, pBoardsInfo);
    if(Res<0)
    {
        strcpy(pSerialNumber, "Sernum error");
        m_ErrMng->UpdateErrMsg(GetErrorString(Res), Res);
        m_ErrMng->UpdateErrMsg("Error getting serial number.", Res);
        free(pBoardsInfo);
        return Res;
    }

    strcpy(pSerialNumber, pBoardsInfo->aBoardInfo[0].strSerialNumber);

    free(pBoardsInfo);

    return 0;


}

int32 CGageDispSystem::DoAcquisition()
{
	QMutexLocker lockSystem(&m_MutexSystemHandle); // RG ????

	QMutexLocker lockStatusCode(&m_MutexLastStatusCode);
	m_bAbort = false;
	m_i32LastStatusCode = CsDrvDo(m_hSystem, ACTION_START, &m_SystemConfig);
	if(m_i32LastStatusCode<0)
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(m_i32LastStatusCode), m_i32LastStatusCode);
		m_ErrMng->UpdateErrMsg("Error acquiring.", m_i32LastStatusCode);
		return m_i32LastStatusCode;
	}

	return m_i32LastStatusCode;
}

int32 CGageDispSystem::CompuscopeGetStatus(CSHANDLE hSystem)
{
	uInt32 u32AcqStatus;

	int32 i32Status = CsDrvGetAcquisitionStatus(hSystem, &u32AcqStatus);

	if(i32Status<0) return i32Status;
	else return u32AcqStatus;
}

int32 CGageDispSystem::CompuscopeTransferData(CSHANDLE hSystem, IN_PARAMS_TRANSFERDATA *inParams, OUT_PARAMS_TRANSFERDATA *outParams)
{
	int32 i32Status = CsDrvTransferData(hSystem, *inParams, outParams);

	return i32Status;
}

int32 CGageDispSystem::CompuscopeTransferDataEx(CSHANDLE hSystem, IN_PARAMS_TRANSFERDATA_EX *inParams, OUT_PARAMS_TRANSFERDATA_EX *outParams)
{
	int32 i32Status = CsDrvTransferDataEx(hSystem, inParams, outParams);

	return i32Status;
}

int32 CGageDispSystem::CompuscopeGetSystem(CSHANDLE *hSystem, int BrdIdx)
{
	int32 i32Status;

	DRVHANDLE hDriverHandle[MAX_NB_HANDLES];
	CsDrvGetAcquisitionSystemHandles(&hDriverHandle[0], sizeof(hDriverHandle));
	*hSystem = hDriverHandle[BrdIdx - 1];
	i32Status = CsDrvUpdateLookupTable(hDriverHandle[BrdIdx - 1], *hSystem);
	if (CS_FAILED(i32Status)) return i32Status;

	i32Status = CsDrvAcquisitionSystemInit(*hSystem, true);

	return i32Status;
}

int32 CGageDispSystem::CompuscopeGetSystemInfo(CSHANDLE hSystem, CSSYSTEMINFO *pSystemInfo)
{
	int i32Status;

	i32Status = CsDrvGetAcquisitionSystemInfo(hSystem, pSystemInfo);

	return i32Status;
}

int32 CGageDispSystem::CompuscopeDo(CSHANDLE hSystem, int16 i16Operation)
{
	int i32Status;

	i32Status = CsDrvDo(hSystem, i16Operation, NULL);

	return i32Status;
}

int32 CGageDispSystem::CompuscopeCommit(CSHANDLE hSystem, bool bCoerce)
{
	int i32Status;
    int Action;
    if(bCoerce) Action = ACTION_COMMIT_COERCE;
    else Action = ACTION_COMMIT;

    i32Status = CsDrvDo(hSystem, Action, &m_SystemConfig);

	return i32Status;
}

int32 CGageDispSystem::CompuscopeSet(CSHANDLE CsHandle, uInt32 ParamID, void *ParamBuffer)
{
	int i32Status;

	i32Status = CsDrvSetParams(CsHandle, ParamID, ParamBuffer);

	return i32Status;
}

int32 CGageDispSystem::CompuscopeGet(CSHANDLE hSystem, int32 nIndex, int32 nConfig, void* pData)
{
	int i32Status;

	if (nIndex==CS_PARAMS) i32Status = CsDrvGetParams(hSystem, nConfig, pData);
	else i32Status = CsDrvGetParams(hSystem, nIndex, pData);

	return i32Status;
}

int32 CGageDispSystem::CompuscopeSetAcq(CSHANDLE CsHandle, CSACQUISITIONCONFIG *pAcquisitionParms)
{
	int i32Status = CS_SUCCESS;

	Q_UNUSED(CsHandle);
	memcpy(m_SystemConfig.pAcqCfg, pAcquisitionParms, sizeof(CSACQUISITIONCONFIG));
//	i32Status = CsDrvValidateParams(CsHandle, CS_SYSTEM, FALSE, pSystemParms);

	return i32Status;
}

int32 CGageDispSystem::CompuscopeSetChan(CSHANDLE CsHandle, int Chan, CSSYSTEMCONFIG *pSystemParms)
{
    Q_UNUSED(Chan);
	int i32Status;

	i32Status = CsDrvValidateParams(CsHandle, CS_SYSTEM, FALSE, pSystemParms);

	return i32Status;
}

int32 CGageDispSystem::CompuscopeGetSystemCaps(CSHANDLE CsHandle, uInt32 CapsId, PCSSYSTEMCONFIG pSysCfg, void *pBuffer, uInt32 *BufferSize)
{
	int i32Status;

	Q_UNUSED(pSysCfg);
	i32Status = CsDrvGetAcquisitionSystemCaps(CsHandle, CapsId, &m_SystemConfig, pBuffer, BufferSize);

	return i32Status;
}

int32 CGageDispSystem::CsGetInfo(CSHANDLE hSystem, int32 nIndex, int32 nConfig, void* pData)
{
    Q_UNUSED(nIndex);
    int i32Status;

    i32Status = CsDrvGetParams(hSystem, nConfig, pData);

    return i32Status;
}

int32 CGageDispSystem::CsSetInfo(CSHANDLE hSystem, int32 nIndex, int32 nConfig, void* pData)
{
    Q_UNUSED(nIndex);
    int i32Status;

    i32Status = CsDrvSetParams(hSystem, nConfig, pData);

    return i32Status;
}

int32 CGageDispSystem::GatherSystemInformation()
{		
	if ( m_hSystem )
	{
		QMutexLocker lockStatusCode(&m_MutexLastStatusCode);

		m_AcquisitionConfig.u32Size = sizeof(m_AcquisitionConfig);
		m_i32LastStatusCode = CsDrvGetParams(m_hSystem, CS_ACQUISITION, &m_AcquisitionConfig);
		if (m_i32LastStatusCode<0)
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(m_i32LastStatusCode), m_i32LastStatusCode);
			m_ErrMng->UpdateErrMsg("Error getting acquisition configuration.", m_i32LastStatusCode);
			return m_i32LastStatusCode;
		}
		m_AcquisitionConfig.i64TriggerHoldoff = m_AcquisitionConfig.i64SegmentSize - m_AcquisitionConfig.i64Depth;
		m_u32ChannelSkip = getChannelSkip(m_AcquisitionConfig.u32Mode, m_SystemInfo.u32ChannelCount / m_SystemInfo.u32BoardCount);

		m_i32LastStatusCode = CsDrvGetParams(m_hSystem, CS_CHANNEL, m_pChannelConfigArray);
		if (m_i32LastStatusCode<0)
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(m_i32LastStatusCode), m_i32LastStatusCode);
			m_ErrMng->UpdateErrMsg("Error getting channel configuration.", m_i32LastStatusCode);
			return m_i32LastStatusCode;
		}

		m_i32LastStatusCode = CsDrvGetParams(m_hSystem, CS_TRIGGER, m_pTriggerConfigArray);
		if (m_i32LastStatusCode<0)
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(m_i32LastStatusCode), m_i32LastStatusCode);
			m_ErrMng->UpdateErrMsg("Error getting trigger configuration.", m_i32LastStatusCode);
			return m_i32LastStatusCode;
		}

		m_i32LastStatusCode = CsDrvGetBoardsInfo(m_hSystem, m_pBoardInfoArray);
		if(m_i32LastStatusCode<0)
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(m_i32LastStatusCode), m_i32LastStatusCode);
			m_ErrMng->UpdateErrMsg("Error getting board information.", m_i32LastStatusCode);
			return m_i32LastStatusCode;
		}

		m_SystemInfoEx.u32Size = sizeof(CSSYSTEMINFOEX);
		m_i32LastStatusCode = CsDrvGetParams(m_hSystem, CS_SYSTEMINFO_EX, &m_SystemInfoEx);
		if(m_i32LastStatusCode<0)
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(m_i32LastStatusCode), m_i32LastStatusCode);
			m_ErrMng->UpdateErrMsg("Error getting expert information.", m_i32LastStatusCode);
			return m_i32LastStatusCode;
		}

		m_i32LastStatusCode = CsDrvGetParams(m_hSystem, CS_MULREC_AVG_COUNT, &m_MulRecAvgCount);
		if (m_i32LastStatusCode == CS_INVALID_PARAMS_ID)
		{
			m_MulRecAvgCount = 1;
		}
		else
		{
			if (m_i32LastStatusCode < 0)
			{
				m_ErrMng->UpdateErrMsg(GetErrorString(m_i32LastStatusCode), m_i32LastStatusCode);
				m_ErrMng->UpdateErrMsg("Error getting expert information.", m_i32LastStatusCode);
				return m_i32LastStatusCode;
			}
		}
	}

	return CS_SUCCESS;
}

int32 CGageDispSystem::CommitAcq(int *i32CommitTimeMsec)
{
	int32 i32Status;
	QTime CommitTimer;
	CSACQUISITIONCONFIG csCurrentAcqConfig;
	csCurrentAcqConfig.u32Size = sizeof(CSACQUISITIONCONFIG);

	i32Status = CsDrvGetParams(m_hSystem, CS_ACQUISITION, &csCurrentAcqConfig);
	if ( CS_FAILED(i32Status) )
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error getting acquisition configuration.", i32Status);
		return i32Status;
	}

	i32Status = SetMulRecAvgCount(m_MulRecAvgCount);
	if (CS_FAILED(i32Status) && (i32Status != CS_INVALID_PARAMS_ID))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error setting MulRec average count.", i32Status);
		return i32Status;
	}

	CommitTimer.restart();
	i32Status = CsDrvValidateParams(m_hSystem, CS_SYSTEM, m_bCoerceFlag, &m_SystemConfig);
    if ( CS_FAILED(i32Status) )
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error setting system parameters.", i32Status);
		return i32Status;
	}

	int i32Coerce = m_bCoerceFlag ? ACTION_COMMIT_COERCE : ACTION_COMMIT;
    i32Status = CsDrvDo(m_hSystem, i32Coerce, &m_SystemConfig);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error committing configuration.", i32Status);
		return i32Status;
	}

	*i32CommitTimeMsec = CommitTimer.elapsed();
	//if(i32Status==CS_CONFIG_CHANGED)
	if (true)
	{
		i32Status = CsDrvGetParams(m_hSystem, CS_ACQUISITION, &m_AcquisitionConfig);
		if ( CS_FAILED(i32Status) )
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error getting new acquisition configuration.", i32Status);
			return i32Status;
		}

		i32Status = CsDrvGetParams(m_hSystem, CS_CHANNEL, m_pChannelConfigArray);
		if (CS_FAILED(i32Status))
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error getting new channel configuration.", i32Status);
			return i32Status;
		}

		i32Status = CsDrvGetParams(m_hSystem, CS_TRIGGER, m_pTriggerConfigArray);
		if (CS_FAILED(i32Status))
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error getting new trigger configuration.", i32Status);
			return i32Status;
		}

		uInt32 MulRecAvgCount;
		i32Status = GetMulRecAvgCount(&MulRecAvgCount);
		if (i32Status == CS_INVALID_PARAMS_ID)
		{
			m_MulRecAvgCount = 1;
			i32Status = 1;
		}
		else
		{
			if (CS_FAILED(i32Status))
			{
				m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
				m_ErrMng->UpdateErrMsg("Error getting MulRec average count.", i32Status);
				return i32Status;
			}
		}
	}

	return i32Status;
}

int32 CGageDispSystem::PrepareForCapture(int *i32CommitTimeMsec)
{
	int32 i32Status;
	QTime CommitTimer;
	CSACQUISITIONCONFIG csCurrentAcqConfig;
	csCurrentAcqConfig.u32Size = sizeof(CSACQUISITIONCONFIG);

	i32Status = CsDrvGetParams(m_hSystem, CS_ACQUISITION, &csCurrentAcqConfig);
	if ( CS_FAILED(i32Status) )
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error getting acquisition configuration.", i32Status);
		return i32Status;
	}

	i32Status = SetMulRecAvgCount(m_MulRecAvgCount);
	if (CS_FAILED(i32Status) && (i32Status != CS_INVALID_PARAMS_ID))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error setting MulRec average count.", i32Status);
		return i32Status;
	}

	CommitTimer.restart();
	i32Status = CsDrvValidateParams(m_hSystem, CS_SYSTEM, m_bCoerceFlag, &m_SystemConfig);
	if ( CS_FAILED(i32Status) )
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error setting acquisition configuration.", i32Status);
		return i32Status;
	}

	int i32Coerce = m_bCoerceFlag ? ACTION_COMMIT_COERCE : ACTION_COMMIT;
    i32Status = CsDrvDo(m_hSystem, i32Coerce, &m_SystemConfig);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error committing configuration.", i32Status);
		return i32Status;
	}

	*i32CommitTimeMsec = CommitTimer.elapsed();
	//if(i32Status==CS_CONFIG_CHANGED)
	if (true)
	{
		i32Status = CsDrvGetParams(m_hSystem, CS_ACQUISITION, &m_AcquisitionConfig);
		if ( CS_FAILED(i32Status) )
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error getting new acquisition configuration.", i32Status);
			return i32Status;
		}

		i32Status = CsDrvGetParams(m_hSystem, CS_CHANNEL, m_pChannelConfigArray);
		if ( CS_FAILED(i32Status) )
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error getting new channel configuration.", i32Status);
			return i32Status;
		}
		
		i32Status = CsDrvGetParams(m_hSystem, CS_TRIGGER, m_pTriggerConfigArray);
		if ( CS_FAILED(i32Status) )
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error getting new trigger configuration.", i32Status);
			return i32Status;
		}

		uInt32 MulRecAvgCount;
		i32Status = GetMulRecAvgCount(&MulRecAvgCount);
		if (i32Status == CS_INVALID_PARAMS_ID)
		{
			m_MulRecAvgCount = 1;
			i32Status = 1;
		}
		else
		{
			if (CS_FAILED(i32Status))
			{
				m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
				m_ErrMng->UpdateErrMsg("Error getting MulRec average count.", i32Status);
				return i32Status;
			}
		}
	}

	return i32Status;
}

int32 CGageDispSystem::SetMulRecAvgCount(uInt32 AvgCount)
{
	int i32Status;

	i32Status = CsDrvSetParams(m_hSystem, CS_MULREC_AVG_COUNT, &AvgCount);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error setting MulRec average count.", i32Status);
		return i32Status;
	}
	m_MulRecAvgCount = AvgCount;

	return i32Status;
}

int32 CGageDispSystem::GetMulRecAvgCount(uInt32 *AvgCount)
{
	int i32Status;

	i32Status = CsDrvGetParams(m_hSystem, CS_MULREC_AVG_COUNT, AvgCount);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error setting MulRec average count.", i32Status);
		return i32Status;
	}
	m_MulRecAvgCount = *AvgCount;

	return i32Status;
}
