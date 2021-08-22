#ifndef __CS_RESOURCE_MANAGER_H__
#define __CS_RESOURCE_MANAGER_H__

#if defined (_WIN32)
#pragma once
#else
#include <pthread.h>
#endif

#include "CsStruct.h"
#include "CsPrivateStruct.h"
#include "misc.h"
#include "CsSharedMemory.h"
#include "ProcessList.h"

#define COMBINE_WORK_DIR __T("SOFTWARE\\Gage\\CombineWorkDir")
#define RESOURCE_MANAGER_KEY _T("SOFTWARE\\Gage\\ResourceManager")

#define REFRESH_VALUE _T("Refresh")
#define FIRMWARE_UPDATE_COUNT _T("FirmwareUpdateCount")

#define MAX_NUMBER_SYSTEMS	16
#define MAX_BOARD_INSYSTEM  16

#define MAX_IMAGE_COUNT	3

typedef int32  (__cdecl* LPDRVAUTOFIRMWAREUPDATE)();
typedef uInt32 (__cdecl* LPDRVOPENSYSTEMFORRM)(CSHANDLE);
typedef uInt32 (__cdecl* LPDRVCLOSESYSTEMFORRM)(CSHANDLE);
typedef int32  (__cdecl* LPDRVGETACQUISITIONSYSTEMCOUNT)(uInt16, uInt16 *);
typedef uInt32 (__cdecl* LPDRVGETACQUISITIONSYSTEMHANDLES) (PDRVHANDLE, uInt32);
typedef int32 (__cdecl* LPDRVGETACQUISITIONSYSTEMINFO) (CSHANDLE, PCSSYSTEMINFO);
typedef int32 (__cdecl* LPDRVUPDATELOOKUPTABLE) (DRVHANDLE, CSHANDLE);
typedef int32 (__cdecl* LPDRVGETBOARDSINFO) (CSHANDLE, PARRAY_BOARDINFO);
typedef int32 (__cdecl* LPDRVGETPARAMS) (CSHANDLE, uInt32, void *);
typedef int32 (__cdecl* LPDRVGETACQUISITIONSYSTEMCAPS) (CSHANDLE, uInt32, PCSSYSTEMCONFIG, void*, uInt32*);

typedef BOOL  (__cdecl* LPISBOARDTYPE)(uInt32);

typedef int32 (__cdecl* LPDRVSETREMOTESYSTEMINUSE)(CSHANDLE, BOOL);
typedef int32 (__cdecl* LPDRVISREMOTESYSTEMINUSE)(CSHANDLE, BOOL*);

typedef int32 (__cdecl* LPDRVGETREMOTEUSER)(CSHANDLE, TCHAR*);


///
/// The SYSTEMHANDLES structure keeps the following information
/// about each system handle.  One structure for each system is
/// kept in a map container.
///
typedef struct _SYSTEMHANDLES
{
	int16			i16SystemNumber; ///< 0 based system number for internal reference
	CSHANDLE		csDriverHandle;  ///< Handle obtained from the driver
	RMHANDLE		rmResourceHandle;///< Resource Manager created handle
	int8			i8HandleActive;  ///< Flag to indicate whether or not the system is in use
	uInt32			u32ProcessID;    ///< ID of process that locked the system
	cschar			strProcessName[MAX_PATH];
	CSSYSTEMINFO	csSystemInfo;    ///< System Info structure
	DWORD			dwTimeActive;	 ///< Time the handle is active(locked) in milliseconds
	uInt32			u32BaseBoardVersion;
	uInt32			u32AddOnVersion;
	cschar			strSerialNumber[MAX_BOARD_INSYSTEM][32];
	CS_FW_VER_INFO  csVersionInfo;
	CSSYSTEMINFOEX	csSystemInfoEx;
	PCIeLINK_INFO   csPCIeInfo[MAX_BOARD_INSYSTEM];
	bool			bCanChangeBootFw;	///!< Flag to determine if a board can boot from it's expert images (Image 1 or 2)
	int				nBootLocation;		///!< Which boot image the system is currently booting from (0, 1, or 2)
	int				nNextBootLocation;	///!< The boot image the system will boot from next (0, 1, or 2)
	CSFWVERSION		fwVersion;		///!< Current version of the boot firmware
	int				nImageCount;	///!< Total number of images
	bool			bIsChecksumValid[MAX_IMAGE_COUNT]; // RG - needs to be changed must account for all the boards in the system
} CSSYSTEMHANDLEINFO, PCSSYSTEMHANDLEINFO;

typedef struct _REFRESHSTRUCT
{
	TIMER_HANDLE hTimer;
	EVENT_HANDLE hRefreshEvent;
	void *pThis;
}REFRESHSTRUCT;

typedef struct _SYSTEMSTRUCT
{
	int16 i16HardReset;
	int32 rmReorganizeFlag;
	int32 i32NumberofSystems;
	int32 i32NumberOfSystemsInUse;
	CSSYSTEMHANDLEINFO System[MAX_NUMBER_SYSTEMS];

#ifdef __linux__
	uInt16 g_RefCount;
#endif
}SYSTEMSTRUCT;

typedef SharedMem< SYSTEMSTRUCT > SHAREDRMSYSTEMSTRUCT; // shared between 32 and 64 bit versions of RM


class CsResourceManager
{
public:
	CsResourceManager(void);
	~CsResourceManager(void);
//	CsResourceManager(const CsResourceManager& csRM); // Copy Constructor

	EVENT_HANDLE GetRefreshEvent(void) { return m_hRefreshEvent; };
	EVENT_HANDLE GetStopTimerThreadEvent(void) { return m_hStopTimerThreadEvent; };
	MUTEX_HANDLE GetRmMutexHandle(void) { return m_hRmMutex; };


	int32 CsRmInitialize(int16 i16HardReset = 0);
	int32 CsRmGetAvailableSystems(void);
	int32 CsRmLockSystem(RMHANDLE hSystemHandle, uInt32 u32ProcessID);
	int32 CsRmFreeSystem(RMHANDLE hSystemHandle);
	int32 CsRmIsSystemHandleValid(RMHANDLE hSystemHandle, uInt32 u32ProcessID);
	int32 CsRmRefreshAllSystemHandles(void);
	int32 CsRmGetSystem(uInt32 i16BoardType, uInt32 i16ChannelCount, uInt32 i16SampleBits, RMHANDLE *rmHandle, uInt32 u32ProcessID, int16 i16Index);
	int32 CsRmGetSystemInfo(RMHANDLE hSystemHandle, CSSYSTEMINFO* pSystemInfo);
	int32 CsRmGetNumberOfSystems(void);
	int32 CsRmGetSystemStateByIndex(int16 i16Index, CSSYSTEMSTATE* pSystemState);
	int32 CsRmGetSystemStateByHandle(RMHANDLE hSystemHandle, CSSYSTEMSTATE* pSystemState);

	LPDRVAUTOFIRMWAREUPDATE m_pCsDrvAutoFirmwareUpdate;

	void SetUpdateStatus(int32 i32Status) { m_i32FirmwareUpdateStatus = i32Status; }
	int32 GetFirmwareUpdateStatus() { return m_i32FirmwareUpdateStatus; }

#if defined(_WIN32)
	void  CsRmUsbNotify(long lNote);
#endif


private:

	int16 AutoUpdateHandler(int16 i16HardReset);
	BOOL ChangeToCombineDir(void);
	BOOL LoadDriverDll(void);
	BOOL UnLoadDriverDll(void);
	void MakeSystemCaps(CSSYSTEMINFO *pSystemInfo, CSSYSTEMCAPS *pSystemCaps);
	int32 FindSystemIndex(RMHANDLE hSystemHandle);

#if defined(_WIN32)
	BOOL GetSessionUserToken(OUT LPHANDLE  lphUserToken);


	TIMER_HANDLE	m_hTimer;
	TIMER_HANDLE	m_hTimerThreadHandle;
	HANDLE      m_hUsbCommEventHandle;

#endif
	EVENT_HANDLE	m_hStopTimerThreadEvent;
	EVENT_HANDLE	m_hRefreshEvent;
	SYSTEMSTRUCT*	m_pSystem;
	MUTEX_HANDLE    m_hRmMutex;

	CRITICAL_SECTION m_csDriverLoadUnloadSection;

	HMODULE			m_hDispHandle;
	LPDRVGETACQUISITIONSYSTEMCOUNT m_pCsDrvGetAcquisitionSystemCount;
	LPDRVGETACQUISITIONSYSTEMHANDLES m_pCsDrvGetAcquisitionSystemHandles;
	LPDRVGETACQUISITIONSYSTEMINFO m_pCsDrvGetAcquisitionSystemInfo;
	LPDRVUPDATELOOKUPTABLE m_pCsDrvUpdateLookupTable;
	LPDRVGETBOARDSINFO m_pCsDrvGetBoardsInfo;
	LPDRVGETPARAMS m_pCsDrvGetParams;
	LPDRVOPENSYSTEMFORRM m_pCsDrvOpenSystemForRm;
	LPDRVCLOSESYSTEMFORRM m_pCsDrvCloseSystemForRm;
	LPDRVSETREMOTESYSTEMINUSE m_pCsDrvSetRemoteSystemInUse;
	LPDRVISREMOTESYSTEMINUSE m_pCsDrvIsRemoteSystemInUse;
	LPDRVGETREMOTEUSER m_pCsDrvGetRemoteUser;
	LPDRVGETACQUISITIONSYSTEMCAPS m_pCsDrvGetAcquisitionSystemCaps;

	SHAREDRMSYSTEMSTRUCT* m_pSharedMem;
	CProcessList* m_ProcessList;

	BOOL			m_bStartRefreshTimerFlag;
	int32			m_i32FirmwareUpdateStatus;

	CsResourceManager(const CsResourceManager& csRM); // Copy Constructor
};

#endif // #define __CS_RESOURCE_MANAGER_H__
