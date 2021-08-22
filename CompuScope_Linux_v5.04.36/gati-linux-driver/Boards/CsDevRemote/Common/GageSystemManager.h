#pragma once
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#elif BSD_SOCKET
#include "platform.h"
#endif

#include <string>
#include <vector>
#include <map>
#include "CsStruct.h"
#include "CsPrivateStruct.h"
#include "CsThreadLock.h"
#include "CsSharedMemory.h"


using namespace std;


class CGageSocket;

typedef struct _systemInfo
{
	CGageSocket* pSystem;
	string strIPAddress;
	DWORD dwPort;
} systemInfo;


typedef vector<systemInfo> vecGageSystem;
typedef map<CSHANDLE, systemInfo> SystemMap;


class CGageSystemManager
{

private:
	CGageSystemManager(void);
	~CGageSystemManager(void);

	static CGageSystemManager* m_pInstance;
	static Critical_Section __key;

public:
	static CGageSystemManager& GetRemoteInstance()
	{
		if(NULL == m_pInstance )
		{
			Lock_Guard<Critical_Section> __lock(__key);
			if(NULL == m_pInstance )
			{
				m_pInstance = new CGageSystemManager();
			}
		}
		return *m_pInstance;
	}
	static void RemoveRemoteInstance()
	{
		Lock_Guard<Critical_Section> __lock(__key);
		delete m_pInstance;
		m_pInstance = NULL;
	}


	int32 GetAcquisitionSystemCount( uInt16 bForceIndependantMode, uInt16* pu16SystemFound );
	uInt32 GetAcquisitionSystemHandles( PDRVHANDLE pDrvHdl, uInt32 u32Size );

	int32 CloseSystemForRm( CSHANDLE csHandle );
	int32 OpenSystemForRm( CSHANDLE csHandle );
	int32 AcquisitionSystemInit( CSHANDLE csHandle, BOOL bResetDefaultSetting );
	int32 GetAcquisitionSystemInfo( CSHANDLE csHandle, PCSSYSTEMINFO pSysInfo );
	int32 GetBoardsInfo( CSHANDLE csHandle, PARRAY_BOARDINFO pBoardInfo );
	int32 GetAcquisitionSystemCaps( CSHANDLE csHandle, uInt32 u32CapsId, PCSSYSTEMCONFIG pSysCfg, void* pBuffer, uInt32* pu32BufferSize );
	int32 AcquisitionSystemCleanup( CSHANDLE csHandle);
	int32 GetParams( CSHANDLE csHandle, uInt32 u32ParamID, void* pParamBuffer );
	int32 SetParams( CSHANDLE csHandle, uInt32 u32ParamID, void* pParamBuffer );
	int32 ValidateParams( CSHANDLE csHandle, uInt32 u32ParamID, uInt32 u32Coerce, void* pParamBuffer );
	int32 Do( CSHANDLE csHandle, uInt32 u32ActionID, void* pActionBuffer );
	int32 GetAcquisitionStatus( CSHANDLE csHandle, uInt32* pu32Status );
	int32 TransferData( CSHANDLE csHandle, IN_PARAMS_TRANSFERDATA inParams, POUT_PARAMS_TRANSFERDATA pOutParams );
	int32 SetSystemInUse( CSHANDLE csHandle, BOOL bSet );
	int32 IsSystemInUse( CSHANDLE csHandle, BOOL* bSet );

// Implementation
//protected:
private:
	string m_strPort;
	string m_szIPAddress;
    int m_nMode;
    int m_nSockType;

private:
	int32 GetSystemIndex( CSHANDLE csHandle );
//	CGageSocket* m_pGageSocket;
//	vecGageSystem m_vSystem;
	SystemMap m_deviceMap;
};
