
#include "StdAfx.h"
#include "CsBaseHwDLL.h"
#include "CsAdvHwDLL.h"
#include "CurrentDriver.h"
#include "CsSymLinks.h"
#ifdef _WINDOWS_
#include "resource.h"
#endif

// Global inter process varIables

#ifdef __linux__
SHAREDDLLMEM		Globals __attribute__ ((init_priority (101))) (MEMORY_MAPPED_SECTION);
#else
SHAREDDLLMEM		Globals(MEMORY_MAPPED_SECTION);
SHAREDDLL_CRITICALSECTION  GlobalCriticalSecion(SHAREDDLL_CRITICALSECTION_NAME, false, true);
#endif


// Global variables
#ifdef _WINDOWS_
	extern	CTraceManager*		g_pTraceMngr;
	HMODULE      g_hDllMod = NULL;
#endif

extern GLOBAL_PERPROCESS	g_GlobalPerProcess;
extern CsBaseHwDLL*			g_pBaseHwDll;
extern CsAdvHwDLL*			g_pAdvanceHwDll;



// Protoypes functions
PCSSYSTEMINFO		CsDllGetSystemInfoPointer( DRVHANDLE DrvHdl, PCSSYSTEMINFOEX *pSysInfoEx = NULL );

#ifdef _WINDOWS_

INT_PTR CALLBACK CfgAuxIoDlg( HWND ,  UINT , WPARAM ,LPARAM );



inline void CsGlobalCriticalSection_Enter()
{
	if( GlobalCriticalSecion.IsMapped() )
		while ( ::InterlockedCompareExchange( &GlobalCriticalSecion.it()->Critical, 1, 0 ) );
}

inline void CsGlobalCriticalSection_Leave()
{
	if( GlobalCriticalSecion.IsMapped() )
		::InterlockedExchange( &GlobalCriticalSecion.it()->Critical, 0 );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL APIENTRY DllMain( HINSTANCE hinst,
                       DWORD  ul_reason_for_call,
                       LPVOID
					 )
{
	BOOL	bSuccess = TRUE;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CsGlobalCriticalSection_Enter();

#if defined (_DEBUG) || defined  (RELEASE_TRACE)
			g_pTraceMngr = new CTraceManager();
#endif
			g_hDllMod = hinst;
			// Do not care about the return value of DLL_Initialize()
			// This will allow the DLL to be load by internal utiliies such as CsiCreator or CsLockSmith
			// even if there is no HW in system
			DLL_Initialize();
			Globals.it()->ProcessCount++;

		CsGlobalCriticalSection_Leave();

			break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
			break;

	case DLL_PROCESS_DETACH:
		CsGlobalCriticalSection_Enter();

			DLL_CleanUp();
			delete g_pTraceMngr;
			g_hDllMod = NULL;
			if (Globals.IsMapped())
			{
				if ( Globals.it()->ProcessCount > 0 )
					Globals.it()->ProcessCount--;
			}

		CsGlobalCriticalSection_Leave();
			break;
	}

	return bSuccess;
}

#else

void CleanupOnExitDll(void)
{
	DLL_CleanUp();

#ifdef _TRACE_ENABLED
	delete g_pTraceMngr;
#endif

	if (Globals.IsMapped())
	{
		if ( Globals.it()->ProcessCount > 0 )
			Globals.it()->ProcessCount--;
	}
}


void __attribute__((constructor)) init_DLL(void)
{
	DLL_Initialize();
	Globals.it()->ProcessCount++;

	openlog("GAGE", LOG_NDELAY, LOG_LOCAL5);

	int i = atexit(CleanupOnExitDll);
	if (i != 0)
		printf("Error\n");
}

void __attribute__ ((destructor)) cleanup_DLL(void)
{
	closelog();

/*	DLL_CleanUp();

	cout << "Cleanup Gage 2 DLL called\n";

#ifdef _TRACE_ENABLED
	delete g_pTraceMngr;
#endif

	if (Globals.IsMapped())
	{
		cout << "Cleanup Gage 3 DLL called\n";
		if ( Globals.it()->ProcessCount > 0 )
			Globals.it()->ProcessCount--;
	}

	cout << "Cleanup FastBall DLL Done\n";
*/
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
	g_pBaseHwDll = new CsBaseHwDLL( (char *)THIS_LINK_NAME, &Globals.it()->g_StaticLookupTable );
	if ( ! g_pBaseHwDll )
		return CS_MEMORY_ERROR;

	// Check constructor status
	int32	CreationStatus = g_pBaseHwDll->GetStatus();
	if ( CreationStatus != CS_SUCCESS )
	{
		// No HW found
		delete g_pBaseHwDll;
		g_pBaseHwDll = NULL;
	}

#ifdef QQQWDK
	else
	{
		g_pAdvanceHwDll = new CsAdvHwDLL( g_pBaseHwDll, Globals.it()->g_StaticLookupTable.DrvSystem, g_LookupTable, (uInt8 *)THIS_LINK_NAME );
		if ( ! g_pAdvanceHwDll)
		{
			delete g_pBaseHwDll;
			g_pBaseHwDll = NULL;

			return CS_MEMORY_ERROR;
		}
	}
#endif

	return CreationStatus;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void DLL_CleanUp()
{
	delete  g_pBaseHwDll;
	g_pBaseHwDll = NULL;

#ifdef QQQWDK
	delete  g_pAdvanceHwDll;
	g_pAdvanceHwDll = NULL;
#endif
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAcquisitionSystemCount( uInt16 bForceIndependantMode, uInt16* pu16SystemFound )
{
	if (!Globals.IsCreated())
		return CS_SHARED_MAP_UNAVAILABLE;
	if (!Globals.IsMapped())
		return CS_INVALID_SHARED_REGION;
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	int32	i32Status;
#ifdef _WINDOWS_
	CsGlobalCriticalSection_Enter();
#endif
	// Get number of Cs acquisition system detected by kernel drivers
	i32Status = g_pBaseHwDll->CsDrvGetAcquisitionSystemCount( bForceIndependantMode, pu16SystemFound );
#ifdef _WINDOWS_
	CsGlobalCriticalSection_Leave();
#endif
	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CsDrvGetAcquisitionSystemHandles(PDRVHANDLE pDrvHdl, uInt32 Size)
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	uInt32 RetCode;
	PDRVSYSTEM	pDrvSystem = Globals.it()->g_StaticLookupTable.DrvSystem;

	RetCode = g_pBaseHwDll->CsDrvGetAcquisitionSystemHandles( pDrvHdl, Size );
	if ( RetCode )
	{
		for (uInt32 i = 0; i< RetCode; i ++ )
		{
			pDrvSystem[i].DrvHandle = pDrvHdl[i];
		}
	}

	return RetCode;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAcquisitionSystemInfo(DRVHANDLE DrvHdl, PCSSYSTEMINFO pSysInfo)
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	return g_pBaseHwDll->CsDrvGetAcquisitionSystemInfo( DrvHdl, pSysInfo );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetBoardsInfo(DRVHANDLE DrvHdl, PARRAY_BOARDINFO pABoardInfo)
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	uInt32 i32Status = g_pBaseHwDll->CsValidateBufferForReadWrite( CS_BOARD_INFO,
												pABoardInfo, FALSE );
	if (i32Status != CS_SUCCESS)
		return i32Status;

	return g_pBaseHwDll->CsDrvGetBoardsInfo( DrvHdl, pABoardInfo );

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAcquisitionStatus(DRVHANDLE DrvHdl, uInt32 *pu32Status)
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	if ( IsBadWritePtr( pu32Status, sizeof(uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	return g_pBaseHwDll->CsDrvGetAcquisitionStatus( DrvHdl, pu32Status );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
PCSSYSTEMINFO CsDllGetSystemInfoPointer( DRVHANDLE DrvHdl, PCSSYSTEMINFOEX *pSysInfoEx )
{
	PDRVSYSTEM	pDrvSystem = Globals.it()->g_StaticLookupTable.DrvSystem;

	// Search for SystemInfo of the current system based on DrvHdl
	for (uInt32 i = 0; i < Globals.it()->g_StaticLookupTable.u16SystemCount; i++)
	{
		if ( pDrvSystem[i].DrvHandle ==  DrvHdl )
		{
			if ( pSysInfoEx != NULL )
				*pSysInfoEx = &pDrvSystem[i].SysInfoEx;

			return (&pDrvSystem[i].SysInfo);
		}
	}
	_ASSERT(0);
	return NULL;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
PCSACQUISITIONCONFIG CsDllGetAcqSystemConfigPointer( DRVHANDLE DrvHdl )
{
	PDRVSYSTEM	pDrvSystem = Globals.it()->g_StaticLookupTable.DrvSystem;

	// Search for SystemInfo of the current system based on DrvHdl
	for (uInt32 i = 0; i < Globals.it()->g_StaticLookupTable.u16SystemCount; i++)
	{
		if ( pDrvSystem[i].DrvHandle ==  DrvHdl )
		{
			return (&pDrvSystem[i].AcqConfig);
		}
	}
	_ASSERT(0);
	return NULL;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvDo(DRVHANDLE DrvHdl, uInt32 ActionID, void *ActionBuffer )
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	PCSSYSTEMINFO	pSysInfo = CsDllGetSystemInfoPointer( DrvHdl );

	if ( ! pSysInfo )
		return CS_INVALID_HANDLE;

	switch( ActionID )
	{
	case ACTION_COMMIT:
	case ACTION_START:
	case ACTION_FORCE:
	case ACTION_ABORT:
	case ACTION_CALIB:
	case ACTION_RESET:
	case ACTION_TIMESTAMP_RESET:
	case ACTION_HISTOGRAM_RESET:
		return g_pBaseHwDll->CsDrvDo( DrvHdl, ActionID, ActionBuffer );
	default:
		return CS_INVALID_REQUEST;
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAsyncTransferDataResult(DRVHANDLE DrvHdl, uInt8 Channel, CSTRANSFERDATARESULT *pTxDataResult,
											BOOL bWait )
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	return g_pBaseHwDll->CsDrvGetAsyncTransferDataResult( DrvHdl, Channel, pTxDataResult, bWait );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvRegisterEventHandle(DRVHANDLE DrvHdl, uInt32 EventType, HANDLE *EventHandle)
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	return g_pBaseHwDll->CsDrvRegisterEventHandle( DrvHdl, EventType, EventHandle );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvOpenSystemForRm(DRVHANDLE DrvHdl)
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	return g_pBaseHwDll->CsDrvOpenSystemForRm( DrvHdl );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvCloseSystemForRm(DRVHANDLE DrvHdl)
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	return g_pBaseHwDll->CsDrvCloseSystemForRm( DrvHdl );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvAutoFirmwareUpdate()
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;
	return g_pBaseHwDll->CsDrvAutoFirmwareUpdate();
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
#ifdef AUX_CFG_DLG
	#include "CsRcInfo.h"
#endif

int32 CsDrvCfgAuxIo(DRVHANDLE DrvHdl)
{
#ifdef AUX_CFG_DLG

	PDRVSYSTEM	pDrvSystem = Globals.it()->g_StaticLookupTable.DrvSystem;

	// Check to see if this functionality is supported. Currently only BASE-8 (among real CompuScope boards)
	// does not support Aux I/O
	for (uInt32 i = 0; i < Globals.it()->g_StaticLookupTable.u16SystemCount; i++)
	{
		if ( pDrvSystem[i].DrvHandle ==  DrvHdl )
		{
			if ( CS_BASE8_BOARDTYPE == pDrvSystem[i].SysInfo.u32BoardType )
			{
				return ( CS_FUNCTION_NOT_SUPPORTED );
			}
		}
	}

	INT_PTR nRet = ::DialogBoxParam(g_hDllMod, (LPCSTR)MAKEINTRESOURCE(IDD_AUX_IO_DLG), NULL, CfgAuxIoDlg, DrvHdl);
	if( nRet == IDOK ||nRet == IDCANCEL )
		return CS_SUCCESS;

	return CS_MISC_ERROR;
#else
	UNREFERENCED_PARAMETER(DrvHdl);
	return CS_FUNCTION_NOT_SUPPORTED;
#endif
}

