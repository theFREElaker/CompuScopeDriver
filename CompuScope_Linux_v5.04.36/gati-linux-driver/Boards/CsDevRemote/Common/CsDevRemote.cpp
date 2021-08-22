// CsDevRemote.cpp : Defines the exported functions for the DLL application.

//

#include "stdafx.h"
#include "RawSocket.h"
#include "CsStruct.h"
#include "CsPrivateStruct.h"
#include "CsDrvApi.h"
#include <string>
#include "RemoteSystemInfo.h"
#include "VxiDiscovery.h" 

#ifndef CS_WIN64
	#include "debug.h"
#endif
#include "DllStruct.h"
#include "GageSocket.h"
#include "CsErrors.h"
#include "GageSystemManager.h"
#include "CsEventLog.h"
#include "CsMsgLog.h"
#include "CsSupport.h"


int32 DLL_Initialize();
void DLL_CleanUp();

using namespace std;


#ifdef WIN32
#ifndef UNDER_CE
#pragma comment(lib, "ws2_32.lib")
#else
#pragma comment(lib, "Ws2.lib")
#endif
#endif

// Shared memory for all processes
#define MEMORY_MAPPED_SECTION _T("CsDevRemote-shared")
SHAREDDLLMEM Globals(MEMORY_MAPPED_SECTION);

CGageSystemManager* CGageSystemManager::m_pInstance = NULL;
Critical_Section CGageSystemManager::__key;

HANDLE	g_hFciDllMutex = NULL;

bool g_bFciDllInitialized = false; 

// Variable global / per process basis
#ifndef CS_WIN64
CTraceManager*	g_pTraceMngr= NULL;
#endif

BOOL APIENTRY DllMain( HANDLE, 
                       DWORD  ul_reason_for_call, 
                       LPVOID
					 )
{
	BOOL	bSuccess = TRUE;

	if( Globals.IsMapped() )
		while ( ::InterlockedCompareExchange( &Globals.it()->g_Critical, 1, 0 ) );

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
#ifndef CS_WIN64
#if defined (_DEBUG) || defined  (RELEASE_TRACE)
			g_pTraceMngr = new CTraceManager();
#endif
#endif
			if (CS_SUCCESS == DLL_Initialize())
				Globals.it()->ProcessCount++;
			else
				bSuccess = FALSE;
			break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
			break;

	case DLL_PROCESS_DETACH:
		    DLL_CleanUp();
#ifndef CS_WIN64
			delete g_pTraceMngr;
#endif
			if (Globals.IsMapped())
			{
				if ( Globals.it()->ProcessCount > 0 )
					Globals.it()->ProcessCount--;
			}
			break;
	}

	if( Globals.IsMapped() )
		::InterlockedExchange( &Globals.it()->g_Critical, 0 );

    return bSuccess;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	DLL_Initialize()
{
	if (!Globals.IsCreated())
	{
		return CS_SHARED_MAP_UNAVAILABLE;
	}
	if (!Globals.IsMapped())
	{
		return CS_INVALID_SHARED_REGION;
	}
	g_bFciDllInitialized = true;
	return 1;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void DLL_CleanUp()
{
	if (NULL != g_hFciDllMutex)
	{
		::CloseHandle(g_hFciDllMutex);
	}

	if (Globals.it()->ProcessCount == 0)
	{
		g_bFciDllInitialized = false;
	}
	CGageSystemManager::RemoveRemoteInstance();
	return;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#define GAGE_REMOTE_SYSTEMS_KEY _T("SOFTWARE\\Gage\\RemoteSystems")
#define USE_REMOTE_FLAG _T("RemoteActive")

bool  UseRemoteSystems()
{
	bool bUseRemote = false; // default

	char Subkey[50] = GAGE_REMOTE_SYSTEMS_KEY;

	HKEY	hKey;
	char	ValueBuffer[50];
	uInt32 	ValueBuffSize;
	DWORD	DataBuffer;
	uInt32 	DataBuffSize;
	DWORD	DataType;

	DWORD dwFlags = KEY_QUERY_VALUE;

	if (Is32on64())
		dwFlags |= KEY_WOW64_64KEY;
	
	long lRetVal = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, Subkey, 0, dwFlags, &hKey);

	if (ERROR_SUCCESS == lRetVal)
	{
		ValueBuffSize = sizeof(ValueBuffer);
		DataBuffSize = sizeof(DWORD);

		long lRetVal = ::RegQueryValueEx(hKey, USE_REMOTE_FLAG, NULL, &DataType, 
								(LPBYTE) &DataBuffer, &DataBuffSize);
		
		if ((ERROR_SUCCESS == lRetVal) && (DataType == REG_DWORD))
		{
			bUseRemote = ( DataBuffer == 0 ) ? false : true;
		}
		::RegCloseKey(hKey);
	}
	return bUseRemote;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvAutoFirmwareUpdate()
{
	return 1;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32  CsDrvGetAcquisitionSystemCount( uInt16 bForceIndependantMode, uInt16* pu16SystemFound )
{
	// this is the first call made to the drivers
	if ( !UseRemoteSystems() )
	{
		*pu16SystemFound = 0;
		return CS_SUCCESS;
	}

	if (!Globals.IsCreated())
	{
		return CS_SHARED_MAP_UNAVAILABLE;
	}
	if (!Globals.IsMapped())
	{
		return CS_INVALID_SHARED_REGION;
	}
	if (NULL == g_hFciDllMutex)
	{
		g_hFciDllMutex = ::CreateMutex(NULL, TRUE, "GageFciDllMutex");
	}

#ifdef xxWIN32
	// This starts up WinSock for the CsRm process
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD( 2, 2 );
	nErrorCode = ::WSAStartup(wVersionRequested, &wsaData);
	if ( 0 != nErrorCode )
	{
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.         */
		nErrorCode = ::GetLastSocketError();
		DisplayErrorMessage(nErrorCode);

		// also report it to the event log
		TCHAR szText[128];
		_stprintf_s( szText, sizeof(szText), TEXT("Could not start WinSock. Error code: %d"), nErrorCode);
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_TEXT, szText );

		return CS_SOCKET_NOT_FOUND;
	}
#endif
	int32 i32Status = CGageSystemManager::GetRemoteInstance().GetAcquisitionSystemCount(bForceIndependantMode, pu16SystemFound);
	Globals.it()->g_StaticLookupTable.u16SystemCount = *pu16SystemFound;
	if ( CS_SUCCEEDED(i32Status) && 0 != *pu16SystemFound )
	{
		g_bFciDllInitialized = true;
	}
	else
	{
		g_bFciDllInitialized = false;
	}
	::ReleaseMutex(g_hFciDllMutex);
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CsDrvGetAcquisitionSystemHandles( PDRVHANDLE pdrvHandle, uInt32 u32Size )
{
	uInt32 u32Status = 0;
	if ( g_bFciDllInitialized )
	{
		::WaitForSingleObject(g_hFciDllMutex, INFINITE);
		u32Status = CGageSystemManager::GetRemoteInstance().GetAcquisitionSystemHandles(pdrvHandle, u32Size);
		::ReleaseMutex(g_hFciDllMutex);
	}

	return u32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvAcquisitionSystemInit( CSHANDLE csHandle, BOOL bResetDefaultSetting )
{
	// The mutex gets created when CsDrvAcquisitionSystemCount is called, which
	// happens from Resource Manager or when direct driver calls (thru CsDisp) 
	// are used. If we're using CsSsm and the public API we need to create the
	// mutex here only if it it NULL which means it wasn't created before in this
	// process.

	int32 i32Status = 0;	

	if (NULL == g_hFciDllMutex)
	{
		g_hFciDllMutex = ::CreateMutex(NULL, TRUE, "GageFciDllMutex");
	}
	i32Status = CGageSystemManager::GetRemoteInstance().AcquisitionSystemInit(csHandle, bResetDefaultSetting);
	::ReleaseMutex(g_hFciDllMutex);
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvCloseSystemForRm( DRVHANDLE drvHandle )
{
	uInt32 u32Status = 0;
	if ( g_bFciDllInitialized )
	{
		::WaitForSingleObject(g_hFciDllMutex, INFINITE);
		u32Status = CGageSystemManager::GetRemoteInstance().CloseSystemForRm(drvHandle);
		::ReleaseMutex(g_hFciDllMutex);
	}
	return u32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvOpenSystemForRm( DRVHANDLE drvHandle )
{
	uInt32 u32Status = 0;
	if ( g_bFciDllInitialized )
	{
		::WaitForSingleObject(g_hFciDllMutex, INFINITE);
		u32Status = CGageSystemManager::GetRemoteInstance().OpenSystemForRm(drvHandle);	
		::ReleaseMutex(g_hFciDllMutex);
	}
	return u32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvGetAcquisitionSystemInfo( DRVHANDLE drvHandle, PCSSYSTEMINFO pSysInfo )
{
	int32 i32Status = 0;
	if (g_bFciDllInitialized)
	{
		::WaitForSingleObject(g_hFciDllMutex, INFINITE);
		i32Status = CGageSystemManager::GetRemoteInstance().GetAcquisitionSystemInfo(drvHandle, pSysInfo);
		::ReleaseMutex(g_hFciDllMutex);
	}
	return i32Status;			
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvGetBoardsInfo( DRVHANDLE drvHandle, PARRAY_BOARDINFO pBoardInfo )
{
	int32 i32Status = 0;
	if (g_bFciDllInitialized)
	{
		::WaitForSingleObject(g_hFciDllMutex, INFINITE);
		i32Status = CGageSystemManager::GetRemoteInstance().GetBoardsInfo(drvHandle, pBoardInfo);
		::ReleaseMutex(g_hFciDllMutex);
	}
	return i32Status;	
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvGetAcquisitionSystemCaps( DRVHANDLE drvHandle, uInt32 u32CapsId, PCSSYSTEMCONFIG pSysCfg, void* pBuffer, uInt32* pu32BufferSize )
{
	int32 i32Status = 0;
	if (g_bFciDllInitialized)
	{
		::WaitForSingleObject(g_hFciDllMutex, INFINITE);
		i32Status = CGageSystemManager::GetRemoteInstance().GetAcquisitionSystemCaps(drvHandle, u32CapsId, pSysCfg, pBuffer, pu32BufferSize);
		::ReleaseMutex(g_hFciDllMutex);
	}
	return i32Status;		
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvAcquisitionSystemCleanup(DRVHANDLE drvHandle)
{
	int32 i32Status = 0;
	if (g_bFciDllInitialized)
	{
		::WaitForSingleObject(g_hFciDllMutex, INFINITE);
		i32Status = CGageSystemManager::GetRemoteInstance().AcquisitionSystemCleanup(drvHandle);
		::ReleaseMutex(g_hFciDllMutex);
	}
	if ( NULL != g_hFciDllMutex )
	{
		::ReleaseMutex(g_hFciDllMutex);
		::CloseHandle(g_hFciDllMutex);
		g_hFciDllMutex = NULL;
	}
	return i32Status;		
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvGetParams( DRVHANDLE drvHandle, uInt32 u32ParamID, void* pParamBuffer )
{
	int32 i32Status = 0;
	if (g_bFciDllInitialized)
	{
		::WaitForSingleObject(g_hFciDllMutex, INFINITE);
		i32Status = CGageSystemManager::GetRemoteInstance().GetParams(drvHandle, u32ParamID, pParamBuffer);
		::ReleaseMutex(g_hFciDllMutex);
	}
	return i32Status;	
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvSetParams( DRVHANDLE drvHandle, uInt32 u32ParamID, void* pParamBuffer )
{
	int32 i32Status = 0;
	if (g_bFciDllInitialized)
	{
		::WaitForSingleObject(g_hFciDllMutex, INFINITE);
		i32Status = CGageSystemManager::GetRemoteInstance().SetParams(drvHandle, u32ParamID, pParamBuffer);
		::ReleaseMutex(g_hFciDllMutex);
	}
	return i32Status;		
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvValidateParams( DRVHANDLE drvHandle, uInt32 u32ParamID, uInt32 u32Coerce, void* pParamBuffer )
{
	int32 i32Status = 0;
	if (g_bFciDllInitialized)
	{
		::WaitForSingleObject(g_hFciDllMutex, INFINITE);
		i32Status = CGageSystemManager::GetRemoteInstance().ValidateParams(drvHandle, u32ParamID, u32Coerce, pParamBuffer);
		::ReleaseMutex(g_hFciDllMutex);
	}
	return i32Status;	
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvDo( DRVHANDLE drvHandle, uInt32 u32ActionID, void* pActionBuffer )
{
	int32 i32Status = 0;
	if (g_bFciDllInitialized)
	{
		::WaitForSingleObject(g_hFciDllMutex, INFINITE);
		i32Status = CGageSystemManager::GetRemoteInstance().Do(drvHandle, u32ActionID, pActionBuffer);
		::ReleaseMutex(g_hFciDllMutex);
	}
	return i32Status;	
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvGetAcquisitionStatus( DRVHANDLE drvHandle, uInt32* pu32Status )
{
	int32 i32Status = 0;
	if (g_bFciDllInitialized)
	{
		::WaitForSingleObject(g_hFciDllMutex, INFINITE);
		i32Status = CGageSystemManager::GetRemoteInstance().GetAcquisitionStatus(drvHandle, pu32Status);
		::ReleaseMutex(g_hFciDllMutex);
	}
	return i32Status;	
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvTransferData( DRVHANDLE drvHandle, IN_PARAMS_TRANSFERDATA inParams, POUT_PARAMS_TRANSFERDATA pOutParams )
{
	int32 i32Status = 0;
	if (g_bFciDllInitialized)
	{
		::WaitForSingleObject(g_hFciDllMutex, INFINITE);
		i32Status = CGageSystemManager::GetRemoteInstance().TransferData(drvHandle, inParams, pOutParams);
		::ReleaseMutex(g_hFciDllMutex);
	}
	return i32Status;	
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvSetRemoteSystemInUse( DRVHANDLE drvHandle, BOOL bSet )
{
	int32 i32Status = 0;
	if (g_bFciDllInitialized)
	{
		::WaitForSingleObject(g_hFciDllMutex, INFINITE);
		i32Status = CGageSystemManager::GetRemoteInstance().SetSystemInUse(drvHandle, bSet);
		::ReleaseMutex(g_hFciDllMutex);
	}
	return i32Status;	
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvIsRemoteSystemInUse( DRVHANDLE drvHandle, BOOL* bSet )
{
	int32 i32Status = 0;
	if (g_bFciDllInitialized)
	{
		::WaitForSingleObject(g_hFciDllMutex, INFINITE);
		i32Status = CGageSystemManager::GetRemoteInstance().IsSystemInUse(drvHandle, bSet);
		::ReleaseMutex(g_hFciDllMutex);
	}
	return i32Status;	
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvRegisterEventHandle( DRVHANDLE drvHandle, uInt32 u32EvenType, HANDLE* phEventHandle )
{
	UNREFERENCED_PARAMETER(drvHandle);
	UNREFERENCED_PARAMETER(u32EvenType);
	UNREFERENCED_PARAMETER(phEventHandle);	
	return CS_FUNCTION_NOT_SUPPORTED;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvGetAsyncTransferDataResult( DRVHANDLE drvHandle, uInt8 Channel, CSTRANSFERDATARESULT* pTxDataResult, BOOL bWait )
{
	UNREFERENCED_PARAMETER(drvHandle);
	UNREFERENCED_PARAMETER(Channel);
	UNREFERENCED_PARAMETER(pTxDataResult);	
	UNREFERENCED_PARAMETER(bWait);	
	return CS_FUNCTION_NOT_SUPPORTED;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvTransferDataEx( DRVHANDLE drvHandle, PIN_PARAMS_TRANSFERDATA_EX InParams, POUT_PARAMS_TRANSFERDATA_EX OutParams )
{
	UNREFERENCED_PARAMETER(drvHandle);
	UNREFERENCED_PARAMETER(InParams);
	UNREFERENCED_PARAMETER(OutParams);
	return CS_FUNCTION_NOT_SUPPORTED;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvExpertCall( DRVHANDLE drvHandle, void* pFuncParams )
{
	UNREFERENCED_PARAMETER(drvHandle);
	UNREFERENCED_PARAMETER(pFuncParams);
	return CS_FUNCTION_NOT_SUPPORTED;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvCfgAuxIo( DRVHANDLE drvHandle )
{
	UNREFERENCED_PARAMETER(drvHandle);
	return CS_FUNCTION_NOT_SUPPORTED; //RG - FOR NOW AT LEAST
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  _CsDrvGetBoardCaps( uInt32 u32ProductId, uInt32 u32Param, void* pBuffer, uInt32* pu32BufferSize )
{
	UNREFERENCED_PARAMETER(u32ProductId);
	UNREFERENCED_PARAMETER(u32Param);
	UNREFERENCED_PARAMETER(pBuffer);
	UNREFERENCED_PARAMETER(pu32BufferSize);
	return 1;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvStmAllocateBuffer( DRVHANDLE drvHandle, int32 nCardIndex, uInt32 u32BufferSize, PVOID *pVa )
{
	UNREFERENCED_PARAMETER(drvHandle);
	UNREFERENCED_PARAMETER(nCardIndex);
	UNREFERENCED_PARAMETER(u32BufferSize);
	UNREFERENCED_PARAMETER(pVa);
	return CS_FUNCTION_NOT_SUPPORTED; //RG - FOR NOW AT LEAST
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvStmFreeBuffer( DRVHANDLE drvHandle, int32 nCardIndex, PVOID pVa )
{
	UNREFERENCED_PARAMETER(drvHandle);
	UNREFERENCED_PARAMETER(nCardIndex);
	UNREFERENCED_PARAMETER(pVa);
	return CS_FUNCTION_NOT_SUPPORTED; //RG - FOR NOW AT LEAST
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvStmTransferToBuffer( DRVHANDLE drvHandle, int32 nCardIndex, PVOID pBuffer, uInt32 u32TransferSize )
{
	UNREFERENCED_PARAMETER(drvHandle);
	UNREFERENCED_PARAMETER(nCardIndex);
	UNREFERENCED_PARAMETER(pBuffer);
	UNREFERENCED_PARAMETER(u32TransferSize);
	return CS_FUNCTION_NOT_SUPPORTED; //RG - FOR NOW AT LEAST
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvStmGetTransferStatus( DRVHANDLE drvHandle, int32 nCardIndex, uInt32 u32TimeoutMs, uInt32 *u32ErrorFlag, uInt32 *u32ActualSize, uInt8 *u8EndOfData )
{
	UNREFERENCED_PARAMETER(drvHandle);
	UNREFERENCED_PARAMETER(nCardIndex);
	UNREFERENCED_PARAMETER(u32TimeoutMs);
	UNREFERENCED_PARAMETER(u32ErrorFlag);
	UNREFERENCED_PARAMETER(u32ActualSize);
	UNREFERENCED_PARAMETER(u8EndOfData);
	return CS_FUNCTION_NOT_SUPPORTED; //RG - FOR NOW AT LEAST
}


