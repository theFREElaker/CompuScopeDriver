#ifndef __GAGEDEMOSYSTEM_H__
#define __GAGEDEMOSYSTEM_H__

#pragma once

#if defined WIN32 || defined CS_WIN64

	#include "stdafx.h"
#ifndef CS_WIN64
	#include "debug.h"
#endif

#elif defined __linux__

	#include <cstdio>
	#include "CsLinuxPort.h"

#endif

#include "misc.h" // Event logic

#include "CsTypes.h"
#include "CsDefines.h"
#include "CsErrors.h"
#include "CsStruct.h"
#include "CsPrivateStruct.h"
#include "CsDrvConst.h"
#include "CsBaseHwDLL.h"

#include "GageDemo.h"
#include <vector>
#include <string>

using namespace std;

#define DEMO_TRIGGER_RES	64

class CBaseSignal;
class CTrigger;
class CGageDemo;

typedef vector<CBaseSignal*> DEMOSIGNALVECTOR;

typedef struct _THREADTRANSFERSTRUCT
{
	CGageDemoSystem *pSystem;
	IN_PARAMS_TRANSFERDATA InParams;
} ThreadTransferStruct;

class CGageDemoSystem
{
public:
	CGageDemoSystem(DRVHANDLE csHandle, CGageDemo* pGageDemo, PCSSYSTEMINFO pSystemInfo, PCSBOARDINFO pBoardInfo,
					PCSSYSTEMINFOEX pSysInfoEx, PCS_FW_VER_INFO pVerInfo,
					const ChannelInfo& ChanInfo, const SystemCapsInfo& sysCaps, const string& strSysName);
	~CGageDemoSystem(void);

	int32 AcquisitionSystemInit(void);
	int32 AcquisitionSystemCleanup(void);
	int32 GetAcquisitionSystemCaps(uInt32 u32CapsId, PCSSYSTEMCONFIG pSysCfg,
									void *pBuffer, uInt32 *u32BufferSize);

	int32 GetAcquisitionStatus(uInt32 *u32Status);
	int32 GetParams(uInt32 u32ParamID, void *ParamBuffer);
	int32 SetParams(uInt32 u32ParamID, void *ParamBuffer);
	int32 ValidateParams(uInt32 u32ParamID, uInt32 u32Coerce, void *ParamBuffer);

	int32 Do(uInt32 u32ActionID, void *ActionBuffer);
	int32 TransferData(IN_PARAMS_TRANSFERDATA InParams, POUT_PARAMS_TRANSFERDATA OutParams);
	int32 GetAsyncTransferDataResult(uInt8 Channel, CSTRANSFERDATARESULT *pTxDataResult, BOOL bWait);
	int32 RegisterEventHandle(uInt32 u32EventType, EVENT_HANDLE *EventHandle);

	CTrigger* GetTrigger() { return  m_pTrigger; };
	CBaseSignal* GetChannelSource(void);
	bool GetExternalSource(void);
	EVENT_HANDLE GetAcqTriggeredEvent(void) { return m_hAcqTriggeredEvent; }
	EVENT_HANDLE GetAcqEndBusyEvent(void) { return m_hAcqEndBusyEvent; }
	EVENT_HANDLE GetInternalAcqEndBusyEvent(void) { return m_hIntAcqEndBusyEvent; }
	int64 GetSampleRate(void) { return ((NULL == m_csSystemConfig.pAcqCfg) ? 10000 : m_csSystemConfig.pAcqCfg->i64SampleRate); };
	int64 GetTriggerDepth(void) { return NULL == m_csSystemConfig.pAcqCfg ? 0 : m_csSystemConfig.pAcqCfg->i64Depth; };
	unsigned GetSegmentCount(void) { return NULL == m_csSystemConfig.pAcqCfg ? 0 :m_csSystemConfig.pAcqCfg->u32SegmentCount; };
	int64 GetMaxLength(void);
	int64 GetTriggerHoldoff(void);
	int64 GetTriggerDelay(void);
	int64 GetTriggerTimeout(void);
	int GetTriggerLevel(void);
	uInt32 GetTriggerSlope(void);
	uInt32 GetTriggerRange(void);

	void SetTriggerTime(double dTriggerTime, unsigned uSegNum) { if(uSegNum < m_veTriggerTime.size()) m_veTriggerTime[uSegNum] = dTriggerTime; }
	double GetTriggerTime(unsigned uSegNum) { return uSegNum < m_veTriggerTime.size() ? m_veTriggerTime[uSegNum] : 0.;}

#if defined (_WIN32) || defined (CS_WIN64)
	static unsigned __stdcall CaptureThread(void *arg);
	static unsigned __stdcall AsynchronousTransferThread(void *arg);
#elif defined __linux__
	static void* CaptureThread(void *arg);
	// static void* AsynchronousTransferThread(void *arg);
#endif

	CBaseSignal* GetSignal(int nIndex) { return m_vecSignals.at(nIndex - 1); };

private:
	CGageDemoSystem(const CGageDemoSystem& )
	{
		//ASSERT(!_T("Should not be here"));
	}

	void SetAcquisitionDefaults(void);
	void SetChannelDefaults(void);
	void SetTriggerEngineDefaults(void);
	int32 StartAcquisition(void);
	int32 ForceTrigger(void);
	int32 Abort(void);

	void CreateNoise(void);

	int32 GetAcquisitionParams(PCSACQUISITIONCONFIG pAcqConfig);
	int32 SetAcquisitionParams(PCSACQUISITIONCONFIG pAcqConfig);
	int32 GetChannelParams(PCSCHANNELCONFIG pChanConfig);
	int32 SetChannelParams(PCSCHANNELCONFIG pChanConfig);
	int32 GetTriggerParams(PCSTRIGGERCONFIG pTrigConfig);
	int32 SetTriggerParams(PCSTRIGGERCONFIG pTrigConfig);

	int32 CsDrvValidateAcquisitionConfig(PCSSYSTEMCONFIG pSysCfg, PCSSYSTEMINFO pSysInfo, uInt32 u32Coerce);
	int32 CsDrvValidateChannelConfig(PCSSYSTEMCONFIG pSysCfg, PCSSYSTEMINFO pSysInfo, uInt32 u32Coerce);
	int32 CsDrvValidateTriggerConfig(PCSSYSTEMCONFIG pSysCfg, PCSSYSTEMINFO pSysInfo, uInt32 u32Coerce);
	int32 ValidateNumberOfTriggerEnabled(PARRAY_TRIGGERCONFIG pATriggers, PCSSYSTEMINFO pSysInfo, uInt32 u32Coerce);
	int32 ValidateTriggerSources(PARRAY_TRIGGERCONFIG pATriggers, PCSSYSTEMINFO pSysInfo, uInt32 u32Coerce);
	bool ValidateTriggerChannelSourceAvailable(int32 i32TriggerSource, uInt32 u32Mode, uInt32 u32ChannelCount);

	int32 ValidateBufferForReadWrite(uInt32 u32ParamID, void *ParamBuffer, BOOL bRead);
	int64 CoerceSampleRate(int64 i64AnySampleRate, uInt32 u32ExtClk, uInt32 u32Mode);
	int64 ValidateSampleRate(PCSSYSTEMCONFIG pSysCfg);
	int32 GetAvailableSampleRates(uInt32 CapsId, PCSSYSTEMCONFIG pSystemConfig,
									PCSSAMPLERATETABLE pSRTable, uInt32 *BufferSize);
	int32 GetAvailableInputRanges(uInt32 CapsId, PCSSYSTEMCONFIG pSystemConfig,
									   PCSRANGETABLE pRangesTbl, uInt32 *BufferSize);
	int32 GetAvailableImpedances(uInt32 CapsId, PCSSYSTEMCONFIG pSystemConfig,
									   PCSIMPEDANCETABLE pImpedancesTbl, uInt32 *BufferSize);
	int32 GetAvailableCouplings(uInt32 CapsId, PCSSYSTEMCONFIG pSystemConfig,
								PCSCOUPLINGTABLE pCouplingsTbl, uInt32 *BufferSize);
	int32 GetAvailableAcqModes(uInt32 *u32AcqModes, uInt32 *BufferSize);
	int32 GetAvailableTerminations(uInt32 *u32Terminations, uInt32 *BufferSize);

	uInt32 GetValidTerminations(void);
	int32 GetAvailableFlexibleTrigger(uInt32 *u32FlexibleTrigger, uInt32 *u32BufferSize);
	int32 GetBoardTriggerEngines(uInt32 *u32BoardEngineCount, uInt32 *u32BufferSize);
	int32 GetAvailableTriggerSources(uInt32 SubCapsId, int32 *pi32ArrayTriggerSources, uInt32 *u32BufferSize);
	int32 GetMaxSegmentPadding(uInt32 *pBuffer,  uInt32 *BufferSize);
	int32 CsDrvGetTriggerResolution(uInt32 *pBuffer,  uInt32 *u32BufferSize);

	void AdjustAcqModesToAvailable(uInt32 *u32AcqModes);

	CSHANDLE m_csHandle;
	uInt32 m_u32SampleBits;

	CSSYSTEMINFO m_csSystemInfo;
	CSBOARDINFO m_csBoardInfo;
	CSSYSTEMINFOEX m_csSysInfoEx;
	CS_FW_VER_INFO m_csVerInfo;
	CSSYSTEMCONFIG m_csSystemConfig;

	EVENT_HANDLE m_hAcqTriggeredEvent;
	EVENT_HANDLE m_hAcqEndBusyEvent;
	EVENT_HANDLE m_hIntAcqEndBusyEvent;
	SEM_HANDLE m_hTransferSemaphore;

	EVENT_HANDLE m_hForceTriggerEvent;
	EVENT_HANDLE m_hAbortAcquisitionEvent;
	EVENT_HANDLE m_hAbortTransferEvent;
	EVENT_HANDLE m_hPerformAcquisitionEvent;
	EVENT_HANDLE m_hTerminateThreadEvent;

	CGageDemo* m_pGageDemo;

	EVENT_HANDLE GetAbortAcquisitionEvent(void){ return m_hAbortAcquisitionEvent;}
	EVENT_HANDLE GetAbortTransferEvent(void){ return m_hAbortTransferEvent;}
	SEM_HANDLE GetTransferSemaphore(void){ return m_hTransferSemaphore;}
	EVENT_HANDLE GetForceTriggerEvent(void){ return m_hForceTriggerEvent;}
	EVENT_HANDLE GetPerformAcquisitionEvent(void){ return m_hPerformAcquisitionEvent;}
	EVENT_HANDLE GetTerminateThreadEvent(void){ return m_hTerminateThreadEvent;}

	void InitHandles(void);

#if defined (_WIN32) || defined (CS_WIN64)
	HANDLE 		m_hCaptureThreadHandle;
#elif defined __linux__
	pthread_t	m_hCaptureThreadHandle;
#endif

	DEMOSIGNALVECTOR m_vecSignals;
	vector<double> m_veTriggerTime;

	SystemCapsInfo m_CapsInfo;
	CTrigger* m_pTrigger;
	string m_strSysName;

	CSTRANSFERDATARESULT m_csAsyncTransferResult;

	bool m_bManualResetTimeStamp;

public:
	static const int64 c_i64MemSize;
	static const uInt32 c_u32MaxSegmentCount;
};

#endif // __GAGEDEMOSYSTEM_H__