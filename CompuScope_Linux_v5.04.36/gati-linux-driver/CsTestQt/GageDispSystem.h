#pragma once
#include "cstestqttypes.h"
#include "GageSystem.h"
#include "CsDisp.h"

/*
typedef int32	(* LPCSDRVGETACQUISITIONSYSTEMCOUNT) (uInt16, uInt16*);
typedef int32	(* LPCSDRVGETACQUISITIONSYSTEMHANDLES) (DRVHANDLE*, uInt32);
typedef int32	(* LPCSDRVUPDATELOOKUPTABLE) (DRVHANDLE, CSHANDLE);
typedef int32	(* LPCSDRVACQUISITIONSYSTEMINIT) (CSHANDLE, BOOL);
typedef int32	(* LPCSDRVACQUISITIONSYSTEMCLEANUP) (CSHANDLE);
typedef int32	(* LPCSDRVDO) (CSHANDLE, uInt32, void*);
typedef int32	(* LPCSDRVGETACQUISITIONSTATUS) (CSHANDLE, uInt32*);
typedef int32	(* LPCSDRVGETPARAMS) (CSHANDLE, int32, void*);
typedef int32	(* LPCSDRVSETPARAMS) (CSHANDLE, uInt32, void*);
typedef int32	(* LPCSDRVVALIDATEPARAMS) (CSHANDLE, uInt32, uInt32, void*);
typedef int32	(* LPCSDRVGETACQUISITIONSYSTEMINFO) (CSHANDLE, PCSSYSTEMINFO);
typedef int32	(* LPCSDRVGETACQUISITIONSYSTEMCAPS) (CSHANDLE, uInt32, PCSSYSTEMCONFIG, void*, uInt32*);
typedef int32	(* LPCSDRVGETBOARDSINFO) (CSHANDLE, PARRAY_BOARDINFO);
typedef int32	(* LPCSDRVTRANSFERDATA) (CSHANDLE, IN_PARAMS_TRANSFERDATA, POUT_PARAMS_TRANSFERDATA);
typedef int32	(* LPCSDRVREGISTEREVENTHANDLE) (CSHANDLE, uInt32, HANDLE*);
typedef int32	(* LPCSDRVGETERRORSTRING) (int32, LPSTR, int);
*/

#define MAX_NB_HANDLES	10

class CGageDispSystem : 	public CGageSystem
{
	Q_OBJECT

public:
    CGageDispSystem(CsTestErrMng* ErrMng);
	~CGageDispSystem(void);

protected:	
//	QString				GetErrorString(int32 i32Status);	

//	QString				GetBoardName(){  return QString(m_SystemInfo.strBoardName); }
//	int64				GetMemorySize() { return m_SystemInfo.i64MaxMemory; }

private:

	int32				InitStructures();
	int32				Initialize();
	int32				ForceTrigger();
	int32				Abort();
	int32				FreeSystem(CSHANDLE csHandle);	
	int32				FreeSystem();
    int32               GetSerialNumber(CSHANDLE hSystem, char *pSerialNumber);
	int32				DoAcquisition();
	int32				CompuscopeGetStatus(CSHANDLE hSystem);
	int32				CompuscopeTransferData(CSHANDLE hSystem, IN_PARAMS_TRANSFERDATA *inParams, OUT_PARAMS_TRANSFERDATA *outParams);
	int32				CompuscopeTransferDataEx(CSHANDLE hSystem, IN_PARAMS_TRANSFERDATA_EX *inParams, OUT_PARAMS_TRANSFERDATA_EX *outParams);
	int32				CompuscopeGetSystem(CSHANDLE *hSystem, int BrdIdx);
	int32				CompuscopeGetSystemInfo(CSHANDLE hSystem, CSSYSTEMINFO *pSystemInfo);
	int32				CompuscopeDo(CSHANDLE hSystem, int16 i16Operation);
	int32				CompuscopeCommit(CSHANDLE hSystem, bool bCoerce);
	int32				CompuscopeSet(CSHANDLE CsHandle, uInt32 ParamID, void *ParamBuffer);
	int32				CompuscopeGet(CSHANDLE hSystem, int32 nIndex, int32 nConfig, void* pData);
	int32				CompuscopeSetAcq(CSHANDLE CsHandle, CSACQUISITIONCONFIG *pAcquisitionParms);
    int32				CompuscopeSetChan(CSHANDLE CsHandle, int Chan, CSSYSTEMCONFIG *pSystemParms);
	int32				CompuscopeGetSystemCaps(CSHANDLE CsHandle, uInt32 CapsId, PCSSYSTEMCONFIG pSysCfg, void *pBuffer, uInt32 *BufferSize);
	int32				CsGetInfo(CSHANDLE hSystem, int32 nIndex, int32 nConfig, void* pData);
	int32				CsSetInfo(CSHANDLE hSystem, int32 nIndex, int32 nConfig, void* pData);
	int32				GatherSystemInformation();
	int32				CommitAcq(int *i32CommitTimeMsec);
	int32				PrepareForCapture(int *i32CommitTimeMsec);
	int32				SetMulRecAvgCount(uInt32 AvgCount);
	int32				GetMulRecAvgCount(uInt32 *AvgCount);

	// Channel parameters get and set
	int32				GetActiveChannelCount();

	uInt32				GetImpedance(uInt32 u32Channel)	{ return m_pChannelConfigArray->aChannel[u32Channel-1].u32Impedance; }
	void				SetImpedance(uInt32 u32Channel, uInt32 u32Impedance) { m_pChannelConfigArray->aChannel[u32Channel - 1].u32Impedance = u32Impedance; }

	uInt32				GetCoupling(uInt32 u32Channel)	{ return m_pChannelConfigArray->aChannel[u32Channel - 1].u32Term; }
	void				SetCoupling(uInt32 u32Channel, uInt32 u32Coupling)	{ m_pChannelConfigArray->aChannel[u32Channel - 1].u32Term = u32Coupling; }  //RG ???

	uInt32				GetFilter(uInt32 u32Channel)	{ return m_pChannelConfigArray->aChannel[u32Channel - 1].u32Filter; }
	void				SetFilter(uInt32 u32Channel, uInt32 u32Filter)	{ m_pChannelConfigArray->aChannel[u32Channel - 1].u32Filter = u32Filter; }

	int32				GetDcOffset(uInt32 u32Channel)	{ return m_pChannelConfigArray->aChannel[u32Channel - 1].i32DcOffset; }
	void				SetDcOffset(uInt32 u32Channel, int32 i32DcOffset)	{ m_pChannelConfigArray->aChannel[u32Channel - 1].i32DcOffset = i32DcOffset; }

	uInt32				GetInputRange(uInt32 u32Channel){ return m_pChannelConfigArray->aChannel[u32Channel - 1].u32InputRange; }
	void				SetInputRange(uInt32 u32Channel, uInt32 u32InputRange){ m_pChannelConfigArray->aChannel[u32Channel - 1].u32InputRange = u32InputRange; }

	// Trigger parameters get and set
	int32				GetSource(uInt32 u32Trigger) { return m_pTriggerConfigArray->aTrigger[u32Trigger - 1].i32Source; }
	void				SetSource(uInt32 u32Trigger, int32 i32Source) { m_pTriggerConfigArray->aTrigger[u32Trigger-1].i32Source = i32Source; }

	int32				GetLevel(uInt32 u32Trigger) { return m_pTriggerConfigArray->aTrigger[u32Trigger - 1].i32Level; }
	void				SetLevel(uInt32 u32Trigger, int32 i32Level) { m_pTriggerConfigArray->aTrigger[u32Trigger - 1].i32Level = i32Level; }

	uInt32				GetCondition(uInt32 u32Trigger) { return m_pTriggerConfigArray->aTrigger[u32Trigger - 1].u32Condition; }
	void				SetCondition(uInt32 u32Trigger, uInt32 u32Condition) { m_pTriggerConfigArray->aTrigger[u32Trigger - 1].u32Condition = u32Condition; }

	uInt32				GetExtCoupling(uInt32 u32Trigger) { return m_pTriggerConfigArray->aTrigger[u32Trigger - 1].u32ExtCoupling; }
	void				SetExtCoupling(uInt32 u32Trigger, uInt32 u32ExtCoupling) { m_pTriggerConfigArray->aTrigger[u32Trigger - 1].u32ExtCoupling = u32ExtCoupling; }

	uInt32				GetExtImpedance(uInt32 u32Trigger) { return m_pTriggerConfigArray->aTrigger[u32Trigger - 1].u32ExtImpedance; }
	void				SetExtImpedance(uInt32 u32Trigger, uInt32 u32ExtImpedance) { m_pTriggerConfigArray->aTrigger[u32Trigger - 1].u32ExtImpedance = u32ExtImpedance; }

	uInt32				GetExtTriggerRange(uInt32 u32Trigger) { return m_pTriggerConfigArray->aTrigger[u32Trigger - 1].u32ExtTriggerRange; }
	void				SetExtTriggerRange(uInt32 u32Trigger, uInt32 u32ExtTriggerRange) { m_pTriggerConfigArray->aTrigger[u32Trigger - 1].u32ExtTriggerRange = u32ExtTriggerRange; }

	PARRAY_CHANNELCONFIG	m_pChannelConfigArray;
	PARRAY_TRIGGERCONFIG	m_pTriggerConfigArray;
	PARRAY_BOARDINFO		m_pBoardInfoArray;
	CSSYSTEMCONFIG			m_SystemConfig;
	
	PDRVHANDLE			m_phDrvHdl;
	CSHANDLE			m_CsHdl;
};

