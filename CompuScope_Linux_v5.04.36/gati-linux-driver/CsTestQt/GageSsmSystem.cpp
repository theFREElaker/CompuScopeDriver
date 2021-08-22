#include "GageSsmSystem.h"
#include <stdlib.h>
#include "gage_drv.h"
#include "CsPrivatePrototypes.h"
#include <QtCore>
#include <QTime>
#include "CsTestFunc.h"

using namespace CsTestFunc;

CGageSsmSystem::CGageSsmSystem(CsTestErrMng* ErrMng)
{

	m_ErrMng = ErrMng;
}

CGageSsmSystem::~CGageSsmSystem(void)
{
	/*
	if ( m_hModule )
	{
		::FreeLibrary(m_hModule);
	}
	*/
	m_i64LastValidHoldoff = 0;
	m_i64LastValidDepth = 0;
}

/*
LPCSSET CGageSsmSystem::GetFunction_CsSet()
{
	return m_pCsSet;
}
*/

int32 CGageSsmSystem::CsGetInfo(CSHANDLE hSystem, int32 nIndex, int32 nConfig, void* pData)
{
    int i32Status;

    i32Status = CsGet(hSystem, nIndex, nConfig, pData);

    return i32Status;
}

int32 CGageSsmSystem::CsSetInfo(CSHANDLE hSystem, int32 nIndex, int32 nConfig, void* pData)
{
    Q_UNUSED(nIndex);
    int i32Status;

    i32Status = CsSet(hSystem, nConfig, pData);

    return i32Status;
}

int32 CGageSsmSystem::InitStructures()
{
	//Initialize arrays for SSM system.
	m_vecChannelConfig.clear();
	CSCHANNELCONFIG csChanConfig;
	csChanConfig.u32Size = sizeof(CSCHANNELCONFIG);
	for (uInt32 i = 0; i < m_SystemInfo.u32ChannelCount; i++)
	{
		csChanConfig.u32ChannelIndex = i + 1;
		m_vecChannelConfig.push_back(csChanConfig);
	}

	m_vecTriggerConfig.clear();
	CSTRIGGERCONFIG csTrigConfig;
	csTrigConfig.u32Size = sizeof(CSTRIGGERCONFIG);
	for (uInt32 i = 0; i < m_SystemInfo.u32TriggerMachineCount; i++)
	{
		csTrigConfig.u32TriggerIndex = i + 1;
		csTrigConfig.i32Source = 0;
		m_vecTriggerConfig.push_back(csTrigConfig);
	}



	return CS_SUCCESS;
}

int32 CGageSsmSystem::Initialize()
{
    int i32Status;

    i32Status = CsInitialize();

    return i32Status;
}

int32 CGageSsmSystem::FreeSystem(CSHANDLE csHandle)
{
    int i32Status;

    i32Status = CsFreeSystem(csHandle);

    return i32Status;
}

int32 CGageSsmSystem::FreeSystem()
{
    int i32Status;

    i32Status = CsFreeSystem(m_hSystem);

    return i32Status;
}

int32 CGageSsmSystem::GetSerialNumber(CSHANDLE hSystem, char *pSerialNumber)
{
    //Get serial number.
    int Res;
    ARRAY_BOARDINFO *pBoardsInfo = (ARRAY_BOARDINFO*)malloc(10 * sizeof(ARRAY_BOARDINFO));
    pBoardsInfo->u32BoardCount = 1;
    pBoardsInfo->aBoardInfo[0].u32Size = sizeof(CSBOARDINFO);
    pBoardsInfo->aBoardInfo[0].u32BoardIndex = 1;
    strcpy(pBoardsInfo->aBoardInfo[0].strSerialNumber, "");
    Res = CsGet(hSystem, CS_BOARD_INFO, 0, pBoardsInfo);
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

int32 CGageSsmSystem::DoAcquisition()
{
	m_bAbort = false;
	m_i32LastStatusCode = CsDo(m_hSystem, ACTION_START);
	if(m_i32LastStatusCode<0)
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(m_i32LastStatusCode), m_i32LastStatusCode);
		m_ErrMng->UpdateErrMsg("Error acquiring.", m_i32LastStatusCode);
		return m_i32LastStatusCode;
	}

	return m_i32LastStatusCode;
}

int32 CGageSsmSystem::CompuscopeGetStatus(CSHANDLE hSystem)
{
	int32 i32Status = CsGetStatus(hSystem);

	return i32Status;
}

int32 CGageSsmSystem::CompuscopeTransferData(CSHANDLE hSystem, IN_PARAMS_TRANSFERDATA *inParams, OUT_PARAMS_TRANSFERDATA *outParams)
{
	int32 i32Status = CsTransfer(hSystem, inParams, outParams);
	
	return i32Status;
}

int32 CGageSsmSystem::CompuscopeTransferDataEx(CSHANDLE hSystem, IN_PARAMS_TRANSFERDATA_EX *inParams, OUT_PARAMS_TRANSFERDATA_EX *outParams)
{
	int32 i32Status = CsTransferEx(hSystem, inParams, outParams);

	return i32Status;
}

int32 CGageSsmSystem::CompuscopeGetSystem(CSHANDLE *hSystem, int BrdIdx)
{
	int32 i32Status = CsGetSystem(hSystem, 0, 0, 0, BrdIdx);

	return i32Status;
}

int32 CGageSsmSystem::CompuscopeGetSystemInfo(CSHANDLE hSystem, CSSYSTEMINFO *pSystemInfo)
{
	int i32Status;

	i32Status = CsGetSystemInfo(hSystem, pSystemInfo);

	return i32Status;
}

int32 CGageSsmSystem::CompuscopeDo(CSHANDLE hSystem, int16 i16Operation)
{
	int i32Status;

	i32Status = CsDo(hSystem, i16Operation);

	return i32Status;
}

int32 CGageSsmSystem::CompuscopeCommit(CSHANDLE hSystem, bool bCoerce)
{
	int i32Status;

	i32Status = CsDo(hSystem, bCoerce ? ACTION_COMMIT_COERCE : ACTION_COMMIT);

	return i32Status;
}

int32 CGageSsmSystem::CompuscopeSet(CSHANDLE CsHandle, uInt32 ParamID, void *ParamBuffer)
{
	int i32Status;

	i32Status = CsSet(CsHandle, ParamID, ParamBuffer);

	return i32Status;
}

int32 CGageSsmSystem::CompuscopeGet(CSHANDLE hSystem, int32 nIndex, int32 nConfig, void* pData)
{
	int i32Status;

	i32Status = CsGet(hSystem, nIndex, nConfig, pData);

	return i32Status;
}

int32 CGageSsmSystem::CompuscopeSetAcq(CSHANDLE CsHandle, CSACQUISITIONCONFIG *pAcquisitionParms)
{
	int i32Status;

	i32Status = CsSet(CsHandle, CS_ACQUISITION, pAcquisitionParms);

	return i32Status;
}

int32 CGageSsmSystem::CompuscopeSetChan(CSHANDLE CsHandle, int Chan, CSSYSTEMCONFIG *pSystemParms)
{
	int i32Status;

	i32Status = CsSet(CsHandle, CS_CHANNEL, &(pSystemParms->pAChannels->aChannel[Chan - 1]));

	return i32Status;
}

int32 CGageSsmSystem::CompuscopeGetSystemCaps(CSHANDLE CsHandle, uInt32 CapsId, PCSSYSTEMCONFIG pSysCfg, void *pBuffer, uInt32 *BufferSize)
{
    Q_UNUSED(pSysCfg);
	int i32Status;

	i32Status = CsGetSystemCaps(CsHandle, CapsId, pBuffer, BufferSize);

	return i32Status;
}

int32 CGageSsmSystem::GatherSystemInformation()
{		
	if ( m_hSystem )
	{
		m_SystemInfo.u32Size = sizeof(m_SystemInfo);
		m_i32LastStatusCode = CsGetSystemInfo(m_hSystem, &m_SystemInfo);
		if(m_i32LastStatusCode<0)
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(m_i32LastStatusCode), m_i32LastStatusCode);
			m_ErrMng->UpdateErrMsg("Error getting system information.", m_i32LastStatusCode);
			return m_i32LastStatusCode;
		}

		m_AcquisitionConfig.u32Size = sizeof(m_AcquisitionConfig);
		m_i32LastStatusCode = CsGet(m_hSystem, CS_ACQUISITION, CS_CURRENT_CONFIGURATION, &m_AcquisitionConfig);
		if(m_i32LastStatusCode<0)
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(m_i32LastStatusCode), m_i32LastStatusCode);
			m_ErrMng->UpdateErrMsg("Error getting acquisition configuration.", m_i32LastStatusCode);
			return m_i32LastStatusCode;
		}

		m_AcquisitionConfig.i64TriggerHoldoff = m_AcquisitionConfig.i64SegmentSize - m_AcquisitionConfig.i64Depth;

		m_u32ChannelSkip = getChannelSkip(m_AcquisitionConfig.u32Mode, m_SystemInfo.u32ChannelCount / m_SystemInfo.u32BoardCount);

		for ( uInt32 i = 0; i < m_SystemInfo.u32ChannelCount; i++ )
		{
			CSCHANNELCONFIG csChanConfig;
			csChanConfig.u32Size = sizeof(CSCHANNELCONFIG);
			csChanConfig.u32ChannelIndex = i + 1;
			m_i32LastStatusCode = CsGet(m_hSystem, CS_CHANNEL, CS_CURRENT_CONFIGURATION, &csChanConfig);
			if ( CS_SUCCEEDED(m_i32LastStatusCode) )
			{
				m_vecChannelConfig[i] = csChanConfig;
			}
			else
			{
				m_ErrMng->UpdateErrMsg(GetErrorString(m_i32LastStatusCode), m_i32LastStatusCode);
				m_ErrMng->UpdateErrMsg("Error getting channels configuration.", m_i32LastStatusCode);
				return m_i32LastStatusCode;
			}
		}

		for ( uInt32 i = 0; i < m_SystemInfo.u32TriggerMachineCount; i++ )
		{
			CSTRIGGERCONFIG csTrigConfig;
			csTrigConfig.u32Size = sizeof(CSTRIGGERCONFIG);
			csTrigConfig.u32TriggerIndex = i + 1;
			m_i32LastStatusCode = CsGet(m_hSystem, CS_TRIGGER, CS_CURRENT_CONFIGURATION, &csTrigConfig);
			if ( CS_SUCCEEDED(m_i32LastStatusCode) )
			{
				m_vecTriggerConfig[i] = csTrigConfig;
			}
			else
			{
				m_ErrMng->UpdateErrMsg(GetErrorString(m_i32LastStatusCode), m_i32LastStatusCode);
				m_ErrMng->UpdateErrMsg("Error getting trigger configuration.", m_i32LastStatusCode);
				return m_i32LastStatusCode;
			}
		}

		m_SystemInfoEx.u32Size = sizeof(CSSYSTEMINFOEX);
		m_i32LastStatusCode = CsGet(m_hSystem, CS_PARAMS, CS_SYSTEMINFO_EX, &m_SystemInfoEx);
		if ( CS_FAILED(m_i32LastStatusCode) )
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(m_i32LastStatusCode), m_i32LastStatusCode);
			m_ErrMng->UpdateErrMsg("Error getting expert information.", m_i32LastStatusCode);
			return m_i32LastStatusCode;
		}

		m_i32LastStatusCode = CsGet(m_hSystem, CS_PARAMS, CS_MULREC_AVG_COUNT, &m_MulRecAvgCount);
		if (m_i32LastStatusCode == CS_INVALID_PARAMS_ID)
		{
			m_MulRecAvgCount = 1;
		}
		else
		{
			if (CS_FAILED(m_i32LastStatusCode))
			{
				m_ErrMng->UpdateErrMsg(GetErrorString(m_i32LastStatusCode), m_i32LastStatusCode);
				m_ErrMng->UpdateErrMsg("Error getting MulRec average count.", m_i32LastStatusCode);
				return m_i32LastStatusCode;
			}
		}
	}

	return CS_SUCCESS;
}

int32 CGageSsmSystem::CommitAcq(int *i32CommitTimeMsec)
{
	int32 i32Status;
	QTime CommitTimer;
	int CommitTime;
	CSACQUISITIONCONFIG csCurrentAcqConfig;
	csCurrentAcqConfig.u32Size = sizeof(CSACQUISITIONCONFIG);

	i32Status = CsGet(m_hSystem, CS_ACQUISITION, CS_CURRENT_CONFIGURATION, &csCurrentAcqConfig);
	if ( CS_FAILED(i32Status) )
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error getting acquisition configuration.", i32Status);
		return i32Status;
	}

	i32Status = CsSet(m_hSystem, CS_ACQUISITION, &m_AcquisitionConfig);
	if ( CS_FAILED(i32Status) )
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error setting acquisition configuration.", i32Status);
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
	i32Status = CsDo(m_hSystem, m_bCoerceFlag ? ACTION_COMMIT_COERCE : ACTION_COMMIT);
	if ( CS_FAILED(i32Status) )
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error committing configuration.", i32Status);
		return i32Status;
	}
	CommitTime = CommitTimer.elapsed();
	*i32CommitTimeMsec = CommitTime;

    quSleep(10);

	//if(i32Status==CS_CONFIG_CHANGED)
	if (true)
	{
		i32Status = CsGet(m_hSystem, CS_ACQUISITION, CS_CURRENT_CONFIGURATION, &m_AcquisitionConfig);
		if ( CS_FAILED(i32Status) )
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error getting new acquisition configuration.", i32Status);
			return i32Status;
		}

		m_AcquisitionConfig.i64TriggerHoldoff = m_AcquisitionConfig.i64SegmentSize - m_AcquisitionConfig.i64Depth;

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
	else if ( CS_FAILED(i32Status) ) return i32Status;

	return i32Status;
}


int32 CGageSsmSystem::PrepareForCapture(int *i32CommitTimeMsec)
{
	int32 i32Status;
	QTime CommitTimer;
	int CommitTime;

	i32Status = CsSet(m_hSystem, CS_ACQUISITION, &m_AcquisitionConfig);
	if ( CS_FAILED(i32Status) )
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error setting acquisition configuration.", i32Status);
		return i32Status;
	}

	for (uInt32 i = 0; i < m_SystemInfo.u32ChannelCount; i++)
	{
        qDebug() << "PrepareForCapture: Before: DC offset CH" << (i+1) << ": " << m_vecChannelConfig[i].i32DcOffset;
		i32Status = CsSet(m_hSystem, CS_CHANNEL, &m_vecChannelConfig[i]);
		if (CS_FAILED(i32Status))
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error setting channel configuration.", i32Status);
			return i32Status;
		}
	}

	for (uInt32 i = 0; i < m_SystemInfo.u32TriggerMachineCount; i++)
	{
		i32Status = CsSet(m_hSystem, CS_TRIGGER, &m_vecTriggerConfig[i]);
		if (CS_FAILED(i32Status))
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error setting trigger configuration.", i32Status);
			return i32Status;
		}
	}

    //qDebug() << "Set MulRec avg count";
	i32Status = SetMulRecAvgCount(m_MulRecAvgCount);
	if (CS_FAILED(i32Status) && (i32Status != CS_INVALID_PARAMS_ID))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error setting MulRec average count.", i32Status);
		return i32Status;
	}

	CommitTimer.restart();
	i32Status = CsDo(m_hSystem, m_bCoerceFlag ? ACTION_COMMIT_COERCE : ACTION_COMMIT);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error committing configuration.", i32Status);
		return i32Status;
	}
	CommitTime = CommitTimer.elapsed();
	if (i32CommitTimeMsec)
		*i32CommitTimeMsec = CommitTime;

	//if(i32Status==CS_CONFIG_CHANGED)
	if (true)
	{
		i32Status = CsGet(m_hSystem, CS_ACQUISITION, CS_CURRENT_CONFIGURATION, &m_AcquisitionConfig);
		if ( CS_FAILED(i32Status) )
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error getting new acquisition configuration.", i32Status);
			return i32Status;
		}

		for (uInt32 i = 0; i < m_SystemInfo.u32ChannelCount; i++)
		{
			i32Status = CsGet(m_hSystem, CS_CHANNEL, CS_CURRENT_CONFIGURATION, &m_vecChannelConfig[i]);
			if (CS_FAILED(i32Status))
			{
				m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
				m_ErrMng->UpdateErrMsg("Error getting new channel configuration.", i32Status);
				return i32Status;
			}

            qDebug() << "PrepareForCapture: After: DC offset CH" << (i+1) << ": " << m_vecChannelConfig[i].i32DcOffset;
		}

		for (uInt32 i = 0; i < m_SystemInfo.u32TriggerMachineCount; i++)
		{
			i32Status = CsGet(m_hSystem, CS_TRIGGER, CS_CURRENT_CONFIGURATION, &m_vecTriggerConfig[i]);
			if (CS_FAILED(i32Status))
			{
				m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
				m_ErrMng->UpdateErrMsg("Error getting new trigger configuration.", i32Status);
				return i32Status;
			}
		}

        //qDebug() << "Get MulRec avg count";
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

        //qDebug() << "MulRec avg count: " << MulRecAvgCount;
	}
	else if ( CS_FAILED(i32Status) ) return i32Status;

    qDebug() << "Prepare for capture done";

	return i32Status;
}

int32 CGageSsmSystem::SetMulRecAvgCount(uInt32 AvgCount)
{
	int32 i32Status = CsSet(m_hSystem, CS_MULREC_AVG_COUNT, &AvgCount);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error setting MulRec average count.", i32Status);
		return i32Status;
	}
	m_MulRecAvgCount = AvgCount;

    return 0;
}

int32 CGageSsmSystem::GetMulRecAvgCount(uInt32 *AvgCount)
{
	int32 i32Status = CsGet(m_hSystem, CS_PARAMS, CS_MULREC_AVG_COUNT, AvgCount);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error getting MulRec average count.", i32Status);
		return i32Status;
	}
	m_MulRecAvgCount = *AvgCount;

    return 0;
}
