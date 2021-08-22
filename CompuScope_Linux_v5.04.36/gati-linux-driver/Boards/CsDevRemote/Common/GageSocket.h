#pragma once
#include <vector>
#include <string>
#include "CsTypes.h"
#include "CsStruct.h"
#include "CsPrivateStruct.h"
#include "CsThreadLock.h"
#include "RawSocket.h"// added

#include <string>

using namespace std;

class CGageSocket
{
public:
	CGageSocket(void);
	CGageSocket(string strIPAddress, DWORD dwPort);
	~CGageSocket(void);

	int32 CloseSystemForRm( DRVHANDLE drvHandle );
	int32 OpenSystemForRm( DRVHANDLE drvHandle );
	int32 GetAcquisitionSystemCount( uInt16 bForceIndependantMode, uInt16* pu16SystemFound );
	uInt32 GetAcquisitionSystemHandles( PDRVHANDLE pDrvHdl, uInt32 u32Size );
	int32 AcquisitionSystemInit( DRVHANDLE drvHandle, BOOL bResetDefaultSetting );
	int32 GetAcquisitionSystemInfo( DRVHANDLE drvHandle, PCSSYSTEMINFO pSysInfo );
	int32 GetBoardsInfo( DRVHANDLE drvHandle, PARRAY_BOARDINFO pBoardInfo );
	int32 GetAcquisitionSystemCaps( DRVHANDLE drvHandle, uInt32 u32CapsId, PCSSYSTEMCONFIG pSysCfg, void* pBuffer, uInt32* pu32BufferSize );
	int32 AcquisitionSystemCleanup( DRVHANDLE drvHandle );
	int32 GetParams( DRVHANDLE drvHandle, uInt32 u32ParamID, void* pParamBuffer );
	int32 SetParams( DRVHANDLE drvHandle, uInt32 u32ParamID, void* pParamBuffer );
	int32 ValidateParams( DRVHANDLE drvHandle, uInt32 u32ParamID, uInt32 u32Coerce, void* pParamBuffer );
	int32 Do( DRVHANDLE drvHandle, uInt32 u32ActionID, void* pActionBuffer );
	int32 GetAcquisitionStatus( DRVHANDLE drvHandle, uInt32* pu32Status );
	int32 TransferData( DRVHANDLE drvHandle, IN_PARAMS_TRANSFERDATA inParams, POUT_PARAMS_TRANSFERDATA pOutParams, uInt32* u32SampleLength = NULL );
	int32 SetSystemInUse( CSHANDLE csHandle, BOOL bSet );
	int32 IsSystemInUse( CSHANDLE csHandle, BOOL* bSet );


private:
	DWORD m_dwCount;
	DRVHANDLE m_DriverHandle[10];

	string m_strIPAddress;
	DWORD  m_dwPort;

	CWizReadWriteSocket socket;
	CRITICAL_SECTION	m_CriticalSection;

};
