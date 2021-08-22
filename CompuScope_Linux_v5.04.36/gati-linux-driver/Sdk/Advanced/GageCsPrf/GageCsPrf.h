#pragma once

// Define to support Windows 2000 and higher
//#define _WIN32_WINNT	0x0500

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>	
#include <sys/types.h>	
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <stdint.h>
#include <semaphore.h>
#include <signal.h>
#include <error.h>
#include <errno.h>
#include <time.h>
#endif /* _WIN32 */

#include <CsTypes.h>
#include <CsAppSupport.h>
#include "CsTchar.h"
#include "CsSdkMisc.h"
#include "CsSdkUtil.h"
#include "CsExpert.h"
#include <CsPrototypes.h>

#define START_DEPTH  0x100
#define FINISH_DEPTH 0x1000000
#define LOOP_COUNT   100
#define OUT_FILE     "PrfResults.txt"

typedef struct
{
	int64		i64StartDepth;
	int64		i64FinishDepth;
	uInt32		u32LoopCount;
	BOOL		bUseAsTransfer;
	BOOL		bCsTransferEx;
	BOOL		bInterleaved;
	TCHAR		strResultFile[MAX_PATH];
}CSPRFCONFIG, *PCSPRFCONFIG;

int32 LoadPrfConfiguration(LPCTSTR szIniFile, int32 i32Resolution, PCSPRFCONFIG pConfig);
int64 ClosestPowerOf2(const int64 i64In, BOOL bLower );
int32 GetTriggerResolution(const CSHANDLE csHandle);
int64 AdjustToTriggerResMultiple(const int64 i64In, const int32 i32Res, const BOOL bLower);
int64 AdjustFinishToStartMultiple(const int64 i64Finish, const int64 int64Start, const BOOL	 bLower);
void WaitForAcknowlegment(void);

#ifdef _WIN32
void HandleFailedStatus(CSHANDLE hSystem, int32 iResult, HANDLE hEvent, bool bDisplayErr);
#else
void HandleFailedStatus(CSHANDLE hSystem, int32 iResult, sem_t *hEvent, bool bDisplayErr);
void	QueryPerformanceCounter(LARGE_INTEGER *TickCount);
void	QueryPerformanceFrequency(LARGE_INTEGER *Frq);
DWORD GetFileAttributes(LPCTSTR szFilePath);
DWORD GetFullPathName(LPCTSTR szIniFile, DWORD nBufferLength, LPTSTR szFilePath, LPTSTR* lpFilePart);
DWORD GetPrivateProfileSection(LPCTSTR lpAppName,
							   LPTSTR lpReturnedString,
							   DWORD nSize,
							   LPCTSTR lpFileName);
DWORD GetPrivateProfileString(LPCTSTR lpAppName,
							  LPCTSTR lpKeyName,
							  LPCTSTR lpDefault,
							  LPTSTR lpReturnedString,
							  DWORD nSize,
							  LPCTSTR lpFileName);
DWORD GetPrivateProfileInt(LPCTSTR lpAppName,
							  LPCTSTR lpKeyName,
							  int nDefault,
							  LPCTSTR lpFileName);	
#endif							  