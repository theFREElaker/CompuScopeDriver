// CsUsb.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#include "CsBaseHwDLL.h"
#include "CsSpDevice.h"
#include "CsUsbmsglog.h"
#ifndef CS_WIN64
#include "Debug.h"
#endif

CsSpDeviceManager* CsSpDeviceManager::m_pInstance = NULL;
Critical_Section CsSpDeviceManager::__key;

// Shared memory for all processes
#define MEMORY_MAPPED_SECTION _T("CsUsb-shared")
SHAREDDLLMEM Globals(MEMORY_MAPPED_SECTION);

HANDLE	g_hCsUsbMutex = NULL;

bool g_bUsbDllInitialized = false; 

// Variable global / per process basis
#ifndef CS_WIN64
CTraceManager*	g_pTraceMngr= NULL;
#endif

BOOL APIENTRY DllMain( HANDLE,DWORD  ul_reason_for_call, LPVOID )
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
//-----------------------------------------------------------------------------
int32	DLL_Initialize()
{
	if (!Globals.IsCreated())
		return CS_SHARED_MAP_UNAVAILABLE;
	if (!Globals.IsMapped())
		return CS_INVALID_SHARED_REGION;
	
	g_bUsbDllInitialized = true;
	return 1;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void DLL_CleanUp()
{
	if (NULL != g_hCsUsbMutex)
		::CloseHandle(g_hCsUsbMutex);

	if (Globals.it()->ProcessCount == 0)
		g_bUsbDllInitialized = false;
	CsSpDeviceManager::RemoveInstance();
	return;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDrvGetAcquisitionSystemCount(uInt16 u16ResetDevices, uInt16* pu16SystemFound ) //RG
{
	if (u16ResetDevices)
	{
		CsSpDeviceManager::RemoveInstance(); //RG - what happens to other systems
	} 
	if (!Globals.IsCreated())
		return CS_SHARED_MAP_UNAVAILABLE;
	if (!Globals.IsMapped())
		return CS_INVALID_SHARED_REGION;
	if (NULL == g_hCsUsbMutex)
		g_hCsUsbMutex = ::CreateMutex(NULL, TRUE, "GageUsbDllMutex");

	int32 i32Status = CsSpDeviceManager::GetInstance().GetAcquisitionSystemCount(pu16SystemFound);
	Globals.it()->g_StaticLookupTable.u16SystemCount = *pu16SystemFound;
	if (CS_SUCCEEDED(i32Status) && 0 != Globals.it()->g_StaticLookupTable.u16SystemCount)
		g_bUsbDllInitialized = true;
	else
		g_bUsbDllInitialized = false;
	::ReleaseMutex(g_hCsUsbMutex);
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32	CsDrvGetAcquisitionSystemHandles(PDRVHANDLE pDrvHdl, uInt32 Size)
{
	uInt32 u32Status = 0;
	if (g_bUsbDllInitialized)
	{
		::WaitForSingleObject(g_hCsUsbMutex, INFINITE);
		u32Status = CsSpDeviceManager::GetInstance().GetAcquisitionSystemHandles(pDrvHdl, Size);
		::ReleaseMutex(g_hCsUsbMutex);
	}
	return u32Status;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDrvAcquisitionSystemInit(DRVHANDLE DrvHdl, BOOL bResetDefaultSetting)
{
	int32 i32Status = CS_FALSE;
	// The mutex gets created when CsDrvAcquisitionSystemCount is called, which
	// happens from Resource Manager or when direct driver calls (thru CsDisp) 
	// are used. If we're using CsSsm and the public API we need to create the
	// mutex here only if it it NULL which means it wasn't created before in this
	// process.
	if (NULL == g_hCsUsbMutex)
		g_hCsUsbMutex = ::CreateMutex(NULL, TRUE, "GageUsbDllMutex");

	CsSpDevice* pDevice = CsSpDeviceManager::GetInstance().GetDevice(DrvHdl);
	if (NULL == pDevice)
		i32Status = CS_INVALID_HANDLE;
	else		
		i32Status = pDevice->AcquisitionSystemInit(bResetDefaultSetting);

	::ReleaseMutex(g_hCsUsbMutex);
	return i32Status;	
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDrvAcquisitionSystemCleanup(DRVHANDLE DrvHdl)
{
	int32 i32Status = CS_FALSE;
	if (g_bUsbDllInitialized)
	{
		::WaitForSingleObject(g_hCsUsbMutex, INFINITE);
		CsSpDevice* pDevice = CsSpDeviceManager::GetInstance().GetDevice(DrvHdl);
		if (NULL == pDevice)
			i32Status = CS_INVALID_HANDLE;
		else		
			i32Status = pDevice->AcquisitionSystemCleanup();

		if (NULL != g_hCsUsbMutex)
		{
			::ReleaseMutex(g_hCsUsbMutex);
			::CloseHandle(g_hCsUsbMutex);
			g_hCsUsbMutex = NULL;
		}
	}
	return i32Status;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDrvGetAcquisitionSystemInfo(DRVHANDLE DrvHdl, PCSSYSTEMINFO pSysInfo)
{
	int32 i32Status = CS_FALSE;
	if (g_bUsbDllInitialized)
	{
		::WaitForSingleObject(g_hCsUsbMutex, INFINITE);
		CsSpDevice* pDevice = CsSpDeviceManager::GetInstance().GetDevice(DrvHdl);
		if (NULL == pDevice)
			i32Status = CS_INVALID_HANDLE;
		else		
			i32Status = pDevice->GetAcquisitionSystemInfo(pSysInfo);
		::ReleaseMutex(g_hCsUsbMutex);
	}
	return i32Status;		
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDrvGetAcquisitionSystemCaps(DRVHANDLE DrvHdl, uInt32 CapsId, PCSSYSTEMCONFIG pSysCfg, void* pBuffer, uInt32* BufferSize)
{
	int32 i32Status = CS_FALSE;
	if (g_bUsbDllInitialized)
	{
		::WaitForSingleObject(g_hCsUsbMutex, INFINITE);
		CsSpDevice* pDevice = CsSpDeviceManager::GetInstance().GetDevice(DrvHdl);
		if (NULL == pDevice)
			i32Status = CS_INVALID_HANDLE;
		else		
			i32Status = pDevice->GetAcquisitionSystemCaps(CapsId, pSysCfg, pBuffer, BufferSize);
		::ReleaseMutex(g_hCsUsbMutex);
	}
	return i32Status;		
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDrvGetBoardsInfo(DRVHANDLE DrvHdl, PARRAY_BOARDINFO paBoardInfo)
{
	int32 i32Status = CS_FALSE;
	if (g_bUsbDllInitialized)
	{
		::WaitForSingleObject(g_hCsUsbMutex, INFINITE);
		CsSpDevice* pDevice = CsSpDeviceManager::GetInstance().GetDevice(DrvHdl);
		if (NULL == pDevice)
			i32Status = CS_INVALID_HANDLE;
		else		
			i32Status = pDevice->GetBoardsInfo(paBoardInfo);
		::ReleaseMutex(g_hCsUsbMutex);
	}
	return i32Status;		
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDrvGetAcquisitionStatus(DRVHANDLE DrvHdl, uInt32* pStatus)
{
	int32 i32Status = CS_FALSE; 
	if (g_bUsbDllInitialized)
	{
		::WaitForSingleObject(g_hCsUsbMutex, INFINITE);
		CsSpDevice* pDevice = CsSpDeviceManager::GetInstance().GetDevice(DrvHdl);
		if (NULL == pDevice)
			i32Status = CS_INVALID_HANDLE;
		else		
			i32Status = pDevice->GetAcquisitionStatus(pStatus);
		::ReleaseMutex(g_hCsUsbMutex);
	}
	return i32Status;		
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32	CsDrvGetParams(DRVHANDLE DrvHdl, uInt32 u32ParamID, void *ParamBuffer)
{
	// CsDrvGetParams is called from CsRm with CS_SYSTEM_INFO_EX and 
	// CS_READ_VER_INFO before AcquisitionSystemInit is ever called, so CGageDemoSystem
	// is not initialized yet. For these 2 cases we'll handle the call in the
	// CGageDemo class. All other parameters should go to the CGageDemoSystem class
	int32 i32Status = CS_FALSE;
	if (g_bUsbDllInitialized)
	{
		::WaitForSingleObject(g_hCsUsbMutex, INFINITE);
		CsSpDevice* pDevice = CsSpDeviceManager::GetInstance().GetDevice(DrvHdl);
		if (NULL == pDevice)
			i32Status = CS_INVALID_HANDLE;
		else		
			i32Status = pDevice->GetParams(u32ParamID, ParamBuffer);
		::ReleaseMutex(g_hCsUsbMutex);
	}
	return i32Status;		
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDrvSetParams(DRVHANDLE DrvHdl, uInt32 ParamID, void *ParamBuffer)
{
	int32 i32Status = CS_FALSE;
	if (g_bUsbDllInitialized)
	{
		::WaitForSingleObject(g_hCsUsbMutex, INFINITE);
		CsSpDevice* pDevice = CsSpDeviceManager::GetInstance().GetDevice(DrvHdl);
		if (NULL == pDevice)
			i32Status = CS_INVALID_HANDLE;
		else		
			i32Status = pDevice->SetParams(ParamID, ParamBuffer);
		::ReleaseMutex(g_hCsUsbMutex);
	}
	return i32Status;		
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDrvValidateParams(DRVHANDLE DrvHdl, uInt32 ParamID, uInt32 u32Coerce, void* pvParamBuffer)
{
	int32 i32Status = CS_FALSE;
	if (g_bUsbDllInitialized)
	{
		::WaitForSingleObject(g_hCsUsbMutex, INFINITE);
		CsSpDevice* pDevice = CsSpDeviceManager::GetInstance().GetDevice(DrvHdl);
		if (NULL == pDevice)
			i32Status = CS_INVALID_HANDLE;
		else		
			i32Status = pDevice->ValidateParams(ParamID, u32Coerce, pvParamBuffer);

		::ReleaseMutex(g_hCsUsbMutex);
	}
	return i32Status;		
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDrvDo(DRVHANDLE DrvHdl, uInt32 ActionID, void* pvActionBuffer )
{
	int32 i32Status = CS_FALSE;
	if (g_bUsbDllInitialized)
	{
		::WaitForSingleObject(g_hCsUsbMutex, INFINITE);
		CsSpDevice* pDevice = CsSpDeviceManager::GetInstance().GetDevice(DrvHdl);
		if (NULL == pDevice)
			i32Status = CS_INVALID_HANDLE;
		else		
			i32Status = pDevice->Do(ActionID, pvActionBuffer);

		::ReleaseMutex(g_hCsUsbMutex);
	}
	return i32Status;		
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvTransferData(DRVHANDLE DrvHdl, IN_PARAMS_TRANSFERDATA InParams, POUT_PARAMS_TRANSFERDATA pOutParams)
{
	int32 i32Status = CS_FALSE;
	if (g_bUsbDllInitialized)
	{
		::WaitForSingleObject(g_hCsUsbMutex, INFINITE);
		CsSpDevice* pDevice = CsSpDeviceManager::GetInstance().GetDevice(DrvHdl);
		if (NULL == pDevice)
			i32Status = CS_INVALID_HANDLE;
		else		
			i32Status = pDevice->TransferData(InParams, pOutParams);

		::ReleaseMutex(g_hCsUsbMutex);
	}
	return i32Status;		
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int32  CsDrvTransferDataEx(DRVHANDLE DrvHdl, PIN_PARAMS_TRANSFERDATA_EX pInParamsEx, POUT_PARAMS_TRANSFERDATA_EX pOutParamsEx)
{
	int32 i32Status = CS_FALSE;
	if (g_bUsbDllInitialized)
	{
		::WaitForSingleObject(g_hCsUsbMutex, INFINITE);
		CsSpDevice* pDevice = CsSpDeviceManager::GetInstance().GetDevice(DrvHdl);
		if (NULL == pDevice)
			i32Status = CS_INVALID_HANDLE;
		else		
			i32Status = pDevice->TransferDataEx(pInParamsEx, pOutParamsEx);

		::ReleaseMutex(g_hCsUsbMutex);
	}
	return i32Status;		
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int32 CsDrvRegisterEventHandle(DRVHANDLE DrvHdl, uInt32 u32EventType, HANDLE *EventHandle)
{
	int32 i32Status = CS_FALSE;
	if (g_bUsbDllInitialized)
	{
		::WaitForSingleObject(g_hCsUsbMutex, INFINITE);
		CsSpDevice* pDevice = CsSpDeviceManager::GetInstance().GetDevice(DrvHdl);
		if (NULL == pDevice)
			i32Status = CS_INVALID_HANDLE;
		else		
			i32Status = pDevice->RegisterEventHandle(u32EventType, EventHandle);

		::ReleaseMutex(g_hCsUsbMutex);
	}
	return i32Status;		
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int32  CsDrvAutoFirmwareUpdate(void)
{
	uInt32 u32Status = 0;
	// we don't check for g_bUsbDllInitialized because if we're here because of
	// an invalid firmeware error, it won't be initialized
	::WaitForSingleObject(g_hCsUsbMutex, INFINITE);
	u32Status = CsSpDeviceManager::GetInstance().AutoFirmwareUpdate();
	::ReleaseMutex(g_hCsUsbMutex);

	return u32Status;
}