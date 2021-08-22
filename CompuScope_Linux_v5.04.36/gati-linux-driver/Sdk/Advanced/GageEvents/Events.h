
#pragma once

// Define to support Windows 2000 and higher
#define _WIN32_WINNT	0x0500

#ifdef _WIN32
#include <Windows.h>
#else
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <poll.h>
#endif /* _WIN32 */

#include <stdio.h>

#include <CsPrototypes.h>
#include <CsAppSupport.h>
#include "CsTchar.h"
#include "CsSdkMisc.h"

extern CSHANDLE	g_hSystem;
extern short	g_nbCaptureCount;

#ifdef _WIN32
#define PGM_ABORTED		"CS_Abort_Execution"
#define PGM_COMPLETED	"CS_Execution_Completed"
#define PGM_ACQUIRING	"CS_Acquiring_Event"
#endif /* _WIN32 */

typedef struct _THREADDATA
{
#ifdef _WIN32
	HANDLE		hAborted;
	HANDLE		hTriggered;
	HANDLE		hEndOfAcquisition;
	HANDLE		hDataProcessingCompleted;
#else
	int			fd_abort_read;
	int			fd_triggered_read;
	int			fd_end_acq_read;
#endif /* _WIN32 */
	uInt32		u32Mode;
	uInt32		SampleSize;
	uInt32		u32BoardCount;
	uInt32		u32ChannelCount;
	int64		TransferStart;
	int64		TransferLength;
	int32		i32SaveFormat;
	TCHAR		lpszSaveFileName[100];
} THREADDATA, *PTHREADDATA;

#ifdef _WIN32
BOOL WINAPI ControlHandler (DWORD dwCtrlType);
DWORD WINAPI ThreadWaitForEvents(void *arg);
#else
void signal_handler(int sig);
void* ThreadWaitForEvents(void *arg);
#endif /* _WIN32 */
