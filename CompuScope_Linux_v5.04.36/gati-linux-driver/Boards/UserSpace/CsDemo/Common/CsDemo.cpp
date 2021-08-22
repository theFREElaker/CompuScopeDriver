/*
 * CsDemo.cpp
 *
 *  Created on: 7-Aug-2008
 *      Author: egiard
 */

#if defined WIN32 || defined CS_WIN64
	#include "stdafx.h"
	#include "debug.h"
#elif defined __linux__
	#include <cstdio>
	#include <CsLinuxPort.h>
#endif // WIN32 or CS_WIN64

#include <misc.h> // Mutex logic

#include <math.h>
#include <stdlib.h>

#include <CsBaseHwDLL.h>
#include <CsIoctlTypes.h>
#include <CsTypes.h>

#include "CsDemo.h"
#include "GageDemo.h"
#include "GageDemoSystem.h"
#include "CsErrors.h"
// Shared memory for all processes

#define MEMORY_MAPPED_SECTION _T("CsDemo-shared")

#ifdef __linux__
	SHAREDDLLMEM Globals __attribute__ ((init_priority (101))) (MEMORY_MAPPED_SECTION);
#else
	SHAREDDLLMEM Globals(MEMORY_MAPPED_SECTION);
#endif

MUTEX_HANDLE g_hDemoDllMutex = NULL;

bool g_bDemoDllInitialized = false;

#if defined (_WIN32) || defined (CS_WIN64)

// Variable global / per process basis
CTraceManager*	g_pTraceMngr= NULL;

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
#if defined (_DEBUG) || defined  (RELEASE_TRACE)
			g_pTraceMngr = new CTraceManager();
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
			delete g_pTraceMngr;
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

#elif  defined __linux__

void demo_cleanup(void)
{
    DLL_CleanUp();

	if (Globals.IsMapped())
	{
		if ( Globals.it()->ProcessCount > 0 )
			Globals.it()->ProcessCount--;
	}
	if (NULL != g_hDemoDllMutex) //shouldn't happen, should be cleaned up in DLL_Cleanup()
	{
		free(g_hDemoDllMutex);
		g_hDemoDllMutex = NULL;
	}
	closelog();
}

void __attribute__((constructor)) init_Demo(void)
{
	openlog("CsDemo", LOG_PID | LOG_NDELAY, LOG_USER);

#if defined (_DEBUG) || defined  (RELEASE_TRACE)
	g_pTraceMngr = new CTraceManager();
#endif

	if (CS_SUCCESS == DLL_Initialize())
		Globals.it()->ProcessCount++;

	int i = atexit(demo_cleanup);
	if (i != 0)
		TRACE(DEMO_TRACE_LEVEL, "error creating demo_cleanup");
}
#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	DLL_Initialize()
{

	if (!Globals.IsCreated())
		return CS_SHARED_MAP_UNAVAILABLE;

	if (!Globals.IsMapped())
		return CS_INVALID_SHARED_REGION;

	g_bDemoDllInitialized = true;
	return 1;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void DLL_CleanUp()
{

	if (NULL != g_hDemoDllMutex)
	{
		DestroyMutex(g_hDemoDllMutex);
		g_hDemoDllMutex = NULL;
	}

	if (Globals.it()->ProcessCount == 0)
		g_bDemoDllInitialized = false;

	CGageDemo::RemoveDemoInstance();

	return;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAcquisitionSystemCount(uInt16, uInt16* pu16SystemFound )
{
	if (!Globals.IsCreated())
		return CS_SHARED_MAP_UNAVAILABLE;

	if (!Globals.IsMapped())
		return CS_INVALID_SHARED_REGION;

	if (NULL == g_hDemoDllMutex)
	{
		g_hDemoDllMutex = MakeMutex(g_hDemoDllMutex, TRUE, "");
	}

	int32 i32Status = CGageDemo::GetDemoInstance().GetAcquisitionSystemCount(pu16SystemFound);

	Globals.it()->g_StaticLookupTable.u16SystemCount = *pu16SystemFound;
	if (CS_SUCCEEDED(i32Status) && 0 != Globals.it()->g_StaticLookupTable.u16SystemCount)
		g_bDemoDllInitialized = true;
	else
		g_bDemoDllInitialized = false;

	ReleaseMutex(g_hDemoDllMutex);

	return i32Status;

}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32	CsDrvGetAcquisitionSystemHandles(PDRVHANDLE pDrvHdl, uInt32 Size)
{
	uInt32 u32Status = 0;

	if (g_bDemoDllInitialized)
	{
		GetMutex(g_hDemoDllMutex);
		u32Status = CGageDemo::GetDemoInstance().GetAcquisitionSystemHandles(pDrvHdl, Size);
		ReleaseMutex(g_hDemoDllMutex);
	}

	return u32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvAcquisitionSystemInit(DRVHANDLE DrvHdl, BOOL)
{
	int32 i32Status = 0;
	// The mutex gets created when CsDrvAcquisitionSystemCount is called, which
	// happens from Resource Manager or when direct driver calls (thru CsDisp)
	// are used. If we're using CsSsm and the public API we need to create the
	// mutex here only if it is NULL which means it wasn't created before in this
	// process.
	if (NULL == g_hDemoDllMutex)
	{
		g_hDemoDllMutex = MakeMutex(g_hDemoDllMutex, TRUE, "");
	}

	i32Status = CGageDemo::GetDemoInstance().AcquisitionSystemInit(DrvHdl);

	ReleaseMutex(g_hDemoDllMutex);
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvAcquisitionSystemCleanup(DRVHANDLE DrvHdl)
{
	int32 i32Status = 0;
	if (g_bDemoDllInitialized)
	{
		GetMutex(g_hDemoDllMutex);

		CGageDemoSystem *pSystem = CGageDemo::GetDemoInstance().GetDemoSystem(DrvHdl);
		if (NULL == pSystem)
			i32Status = CS_INVALID_HANDLE;
		else
			i32Status = pSystem->AcquisitionSystemCleanup();

		ReleaseMutex(g_hDemoDllMutex); // we'll destroy the mutex when we exit the dll

	}
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAcquisitionSystemInfo(DRVHANDLE DrvHdl, PCSSYSTEMINFO pSysInfo)
{
	int32 i32Status = 0;
	if (g_bDemoDllInitialized)
	{
		GetMutex(g_hDemoDllMutex);

		i32Status = CGageDemo::GetDemoInstance().GetAcquisitionSystemInfo(DrvHdl, pSysInfo);
		ReleaseMutex(g_hDemoDllMutex);
	}
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAcquisitionSystemCaps(DRVHANDLE DrvHdl, uInt32 CapsId, PCSSYSTEMCONFIG pSysCfg,
									 void *pBuffer, uInt32 *BufferSize)
{
	int32 i32Status = 0;
	if (g_bDemoDllInitialized)
	{
		GetMutex(g_hDemoDllMutex);

		CGageDemoSystem *pSystem = CGageDemo::GetDemoInstance().GetDemoSystem(DrvHdl);
		if (NULL == pSystem)
			i32Status = CS_INVALID_HANDLE;
		else
			i32Status = pSystem->GetAcquisitionSystemCaps(CapsId, pSysCfg, pBuffer, BufferSize);

		ReleaseMutex(g_hDemoDllMutex);
	}
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetBoardsInfo(DRVHANDLE DrvHdl, PARRAY_BOARDINFO pABoardInfo)
{
	int32 i32Status = 0;
	if (g_bDemoDllInitialized)
	{
		GetMutex(g_hDemoDllMutex);

		i32Status = CGageDemo::GetDemoInstance().GetBoardsInfo(DrvHdl, pABoardInfo);

		ReleaseMutex(g_hDemoDllMutex);
	}
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAcquisitionStatus(DRVHANDLE DrvHdl, uInt32 *Status)
{
	int32 i32Status = 0;
	if (g_bDemoDllInitialized)
	{
		GetMutex(g_hDemoDllMutex);

		CGageDemoSystem *pSystem = CGageDemo::GetDemoInstance().GetDemoSystem(DrvHdl);
		if (NULL == pSystem)
			i32Status = CS_INVALID_HANDLE;
		else
			i32Status = pSystem->GetAcquisitionStatus(Status);

		ReleaseMutex(g_hDemoDllMutex);
	}
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
//--------------------------------------------------
//----- SM calls this function to retrieve the current configuration of acquisition system
int32	CsDrvGetParams(DRVHANDLE DrvHdl, uInt32 u32ParamID, void *ParamBuffer)
{
	// CsDrvGetParams is called from CsRm with CS_SYSTEM_INFO_EX and
	// CS_READ_VER_INFO before AcquisitionSystemInit is ever called, so CGageDemoSystem
	// is not initialized yet. For these 2 cases we'll handle the call in the
	// CGageDemo class. All other parameters should go to the CGageDemoSystem class
	int32 i32Status = 0;
	if (g_bDemoDllInitialized)
	{
		GetMutex(g_hDemoDllMutex);

		i32Status = CGageDemo::GetDemoInstance().GetParams(DrvHdl, u32ParamID, ParamBuffer);
		ReleaseMutex(g_hDemoDllMutex);
	}
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvSetParams(DRVHANDLE DrvHdl, uInt32 ParamID, void *ParamBuffer)
{
	int32 i32Status = 0;
	if (g_bDemoDllInitialized)
	{
		GetMutex(g_hDemoDllMutex);

		CGageDemoSystem *pSystem = CGageDemo::GetDemoInstance().GetDemoSystem(DrvHdl);
		if (NULL == pSystem)
			i32Status = CS_INVALID_HANDLE;
		else
			i32Status = pSystem->SetParams(ParamID, ParamBuffer);

		ReleaseMutex(g_hDemoDllMutex);
	}
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvValidateParams(DRVHANDLE DrvHdl, uInt32 ParamID, uInt32 u32Coerce, void *ParamBuffer)
{
	int32 i32Status = 0;
	if (g_bDemoDllInitialized)
	{
		GetMutex(g_hDemoDllMutex);

		CGageDemoSystem *pSystem = CGageDemo::GetDemoInstance().GetDemoSystem(DrvHdl);
		if (NULL == pSystem)
			i32Status = CS_INVALID_HANDLE;
		else
			i32Status = pSystem->ValidateParams(ParamID, u32Coerce, ParamBuffer);

		ReleaseMutex(g_hDemoDllMutex);
	}
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvDo(DRVHANDLE DrvHdl, uInt32 ActionID, void *ActionBuffer )
{
	int32 i32Status = 0;
	if (g_bDemoDllInitialized)
	{
		GetMutex(g_hDemoDllMutex);
		CGageDemoSystem *pSystem = CGageDemo::GetDemoInstance().GetDemoSystem(DrvHdl);
		if (NULL == pSystem)
			i32Status = CS_INVALID_HANDLE;
		else
			i32Status = pSystem->Do(ActionID, ActionBuffer);

		ReleaseMutex(g_hDemoDllMutex);
	}
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvTransferData(DRVHANDLE DrvHdl, IN_PARAMS_TRANSFERDATA InParams,
							POUT_PARAMS_TRANSFERDATA OutParams)
{
	int32 i32Status = 0;
	if (g_bDemoDllInitialized)
	{
		GetMutex(g_hDemoDllMutex);

		CGageDemoSystem *pSystem = CGageDemo::GetDemoInstance().GetDemoSystem(DrvHdl);
		if (NULL == pSystem)
			i32Status = CS_INVALID_HANDLE;
		else
			i32Status = pSystem->TransferData(InParams, OutParams);

		ReleaseMutex(g_hDemoDllMutex);
	}
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAsyncTransferDataResult(DRVHANDLE DrvHdl, uInt8 Channel, CSTRANSFERDATARESULT *pTxDataResult,
											BOOL bWait )
{
	int32 i32Status = 0;
	if (g_bDemoDllInitialized)
	{
		GetMutex(g_hDemoDllMutex);

		CGageDemoSystem *pSystem = CGageDemo::GetDemoInstance().GetDemoSystem(DrvHdl);
		if (NULL == pSystem)
			i32Status = CS_INVALID_HANDLE;
		else
			i32Status = pSystem->GetAsyncTransferDataResult(Channel, pTxDataResult, bWait);

		ReleaseMutex(g_hDemoDllMutex);
	}
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvRegisterEventHandle(DRVHANDLE DrvHdl, uInt32 u32EventType, HANDLE *EHandle)
{
	int32 i32Status = 0;
	if (g_bDemoDllInitialized)
	{
		GetMutex(g_hDemoDllMutex);
#ifdef __linux
		EVENT_HANDLE *EventHandle = (EVENT_HANDLE *)EHandle;
#else
		HANDLE *EventHandle = EHandle;
#endif
		CGageDemoSystem *pSystem = CGageDemo::GetDemoInstance().GetDemoSystem(DrvHdl);
		if (NULL == pSystem)
			i32Status = CS_INVALID_HANDLE;
		else
			i32Status = pSystem->RegisterEventHandle(u32EventType, EventHandle);

		ReleaseMutex(g_hDemoDllMutex);
	}
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvRegisterStreamingBuffers(DRVHANDLE, uInt8, CSFOLIOARRAY*, HANDLE*, PSTMSTATUS*)
{
	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvReleaseStreamingBuffers(DRVHANDLE, uInt8)
{
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvOpenSystemForRm(DRVHANDLE)
{
	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvCloseSystemForRm(DRVHANDLE)
{
	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvAutoFirmwareUpdate()
{
	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvExpertCall(CSHANDLE, void *)
{
	return CS_FUNCTION_NOT_SUPPORTED;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 _CsDrvGetBoardType(uInt32 *u32MinBoardType, uInt32 *u32MaxBoardType)
{
#ifdef _WINDOWS
	if ( IsBadReadPtr( u32MaxBoardType, sizeof(uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( IsBadReadPtr( u32MinBoardType, sizeof(uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;
#endif

	*u32MinBoardType = CSDEMO_BT_FIRST_BOARD;
	*u32MaxBoardType = CSDEMO_BT_LAST_BOARD;
	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 _CsDrvGetHwOptionText(PCS_HWOPTIONS_CONVERT2TEXT)
{
	return CS_FUNCTION_NOT_SUPPORTED;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 _CsDrvGetFwOptionText(PCS_FWOPTIONS_CONVERT2TEXT)
{
	return CS_FUNCTION_NOT_SUPPORTED;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 _CsDrvGetBoardCaps(uInt32, uInt32, void*, uInt32 *)
{
	return CS_FUNCTION_NOT_SUPPORTED;//TODO: RG - FILL THIS FUNCTION IN
}


int32 CsDrvCfgAuxIo(CSHANDLE){return CS_FUNCTION_NOT_SUPPORTED;}


