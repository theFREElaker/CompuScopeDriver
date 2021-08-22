#include <QtGlobal>

#include "cstestqttypes.h"

#ifdef Q_OS_WIN
    #include "qt_windows.h"
#endif

#include "CsPrototypes.h"
#include "GageSystem.h"

/*
typedef int32	(__stdcall* LPCSINITIALIZE) ();
typedef int32	(__stdcall* LPCSGETSYSTEM) (CSHANDLE*, uInt32, uInt32, uInt32, int16);
typedef int32	(__stdcall* LPCSFREESYSTEM) (CSHANDLE);
typedef int32	(__stdcall* LPCSDO) (CSHANDLE, int16);
typedef int32	(__stdcall* LPCSGETSTATUS) (CSHANDLE);
typedef int32	(__stdcall* LPCSGET) (CSHANDLE, int32, int32, void*);
typedef int32	(__stdcall* LPCSSET) (CSHANDLE, int32, const void*);
typedef int32	(__stdcall* LPCSGETSYSTEMINFO) (CSHANDLE, PCSSYSTEMINFO);
typedef int32	(__stdcall* LPCSGETSYSTEMCAPS) (CSHANDLE, uInt32, void*, uInt32*);
typedef int32	(__stdcall* LPCSTRANSFER) (CSHANDLE, PIN_PARAMS_TRANSFERDATA, POUT_PARAMS_TRANSFERDATA);
typedef int32	(__stdcall* LPCSTRANSFERAS) (CSHANDLE, PIN_PARAMS_TRANSFERDATA, POUT_PARAMS_TRANSFERDATA, int32*);
typedef int32	(__stdcall* LPCSGETEVENTHANDLE) (CSHANDLE, uInt32, EVENT_HANDLE*);
typedef int32	(__stdcall* LPCSGETERRORSTRING) (int32, LPSTR, int);
*/

class CSystemCaps;

class CGageSsmSystem : public CGageSystem
{
	Q_OBJECT

public:
    CGageSsmSystem(CsTestErrMng* ErrMng);
	~CGageSsmSystem(void);

//	QString				GetBoardName(){  return QString(m_SystemInfo.strBoardName); }
//	int64				GetMemorySize() { return m_SystemInfo.i64MaxMemory; }

	//LPCSSET				GetFunction_CsSet();
	//LPCSGET				GetFunction_CsGet();
	//LPCSGETERRORSTRING	GetFunction_CsGetErrString();

	int32				ForceTrigger();
	int32				Abort();
	
protected:	
//	QString				GetErrorString(int32 i32Status);


private:
	int32				InitStructures();
	int32				Initialize();
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
	int32				SetMulRecAvgCount(uInt32 AvgCount);
	int32				GetMulRecAvgCount(uInt32 *AvgCount);
		
	int32				GatherSystemInformation();
	int32				CommitAcq(int *i32CommitTimeMsec);
	int32				PrepareForCapture(int *i32CommitTimeMsec);

	int32				GetActiveChannelCount();

	// Channel parameters get and set
	uInt32				GetImpedance(uInt32 u32Channel)	{ return m_vecChannelConfig[u32Channel-1].u32Impedance; }
	void				SetImpedance(uInt32 u32Channel, uInt32 u32Impedance)	{ m_vecChannelConfig[u32Channel-1].u32Impedance = u32Impedance; }

	uInt32				GetCoupling(uInt32 u32Channel)	{ return m_vecChannelConfig[u32Channel-1].u32Term; }
	void				SetCoupling(uInt32 u32Channel, uInt32 u32Coupling)	{ m_vecChannelConfig[u32Channel-1].u32Term = u32Coupling; }  //RG ???

	uInt32				GetFilter(uInt32 u32Channel)	{ return m_vecChannelConfig[u32Channel-1].u32Filter; }
	void				SetFilter(uInt32 u32Channel, uInt32 u32Filter)	{ m_vecChannelConfig[u32Channel-1].u32Filter = u32Filter; }

	int32				GetDcOffset(uInt32 u32Channel)	{ return m_vecChannelConfig[u32Channel-1].i32DcOffset; }
	void				SetDcOffset(uInt32 u32Channel, int32 i32DcOffset)	{ m_vecChannelConfig[u32Channel-1].i32DcOffset = i32DcOffset; }

	uInt32				GetInputRange(uInt32 u32Channel){ return m_vecChannelConfig[u32Channel-1].u32InputRange; }
	void				SetInputRange(uInt32 u32Channel, uInt32 u32InputRange) { m_vecChannelConfig[u32Channel-1].u32InputRange = u32InputRange; }

	// Trigger parameters get and set
	int32				GetSource(uInt32 u32Trigger) { return m_vecTriggerConfig[u32Trigger-1].i32Source; }
	void				SetSource(uInt32 u32Trigger, int32 i32Source) { m_vecTriggerConfig[u32Trigger-1].i32Source = i32Source; }

	int32				GetLevel(uInt32 u32Trigger) { return m_vecTriggerConfig[u32Trigger-1].i32Level; }
	void				SetLevel(uInt32 u32Trigger, int32 i32Level) { m_vecTriggerConfig[u32Trigger-1].i32Level = i32Level; }

	uInt32				GetCondition(uInt32 u32Trigger) { return m_vecTriggerConfig[u32Trigger-1].u32Condition; }
	void				SetCondition(uInt32 u32Trigger, uInt32 u32Condition) { m_vecTriggerConfig[u32Trigger-1].u32Condition = u32Condition; }

	uInt32				GetExtCoupling(uInt32 u32Trigger) { return m_vecTriggerConfig[u32Trigger-1].u32ExtCoupling; }
	void				SetExtCoupling(uInt32 u32Trigger, uInt32 u32ExtCoupling) { m_vecTriggerConfig[u32Trigger-1].u32ExtCoupling = u32ExtCoupling; }

	uInt32				GetExtImpedance(uInt32 u32Trigger) { return m_vecTriggerConfig[u32Trigger-1].u32ExtImpedance; }
	void				SetExtImpedance(uInt32 u32Trigger, uInt32 u32ExtImpedance) { m_vecTriggerConfig[u32Trigger-1].u32ExtImpedance = u32ExtImpedance; }

	uInt32				GetExtTriggerRange(uInt32 u32Trigger) { return m_vecTriggerConfig[u32Trigger-1].u32ExtTriggerRange; }
	void				SetExtTriggerRange(uInt32 u32Trigger, uInt32 u32ExtTriggerRange) { m_vecTriggerConfig[u32Trigger-1].u32ExtTriggerRange = u32ExtTriggerRange; }

	CSystemCaps*		m_pCaps;
	vector<CSCHANNELCONFIG> m_vecChannelConfig;
	vector<CSTRIGGERCONFIG> m_vecTriggerConfig;
};
