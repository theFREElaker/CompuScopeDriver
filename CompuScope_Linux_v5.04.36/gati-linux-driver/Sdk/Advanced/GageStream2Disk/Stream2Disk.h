
#pragma once

// Define to support Windows 2000 and higher
#define _WIN32_WINNT	0x0500


#include <unistd.h>
#include <pthread.h>

#include <stdio.h>

#include <signal.h>
#include <errno.h>
#include <sys/stat.h>

#include <CsAppSupport.h>
#include "CsTchar.h"
#include "CsSdkMisc.h"
#include "CsSdkUtil.h"
#include "CsExpert.h"
#include <CsPrototypes.h>

#define  MAX_CARDS_COUNT	10				// Max number of cards supported in a M/S Compuscope system 
#define	SEGMENT_TAIL_ADJUST	64				// number of bytes at end of data which holds the timestamp values
#define OUT_FILE			"Data"			// name of the output file 
#define LOOP_COUNT			1000
#define TRANSFER_TIMEOUT	10000				
#define STREAM_BUFFERSZIZE	0x200000
#define STM_SECTION 		_T("StmConfig")	// section name in ini file

#define MAX_SEGMENTS_COUNT 25000

// User configuration variables
typedef struct
{
	uInt32		u32BufferSizeBytes;
	uInt32		u32TransferTimeout;
	TCHAR		strResultFile[MAX_PATH];
	BOOL		bSaveToFile;			// Save data to file or not
	BOOL		bSaveHeader;			// Save data with or wihtour the header 
	BOOL		bFileFlagNoBuffering;	// Should be 1 for better disk performance
	BOOL		bErrorHandling;			// How to handle the FIFO full error
	CsDataPackMode		DataPackCfg;
}CSSTMCONFIG, *PCSSTMCONFIG;

typedef struct
{
	pthread_mutex_t mutex;
	pthread_cond_t	cond;
}Event, *pEvent;

int32 InitializeStream(CSHANDLE g_hSystem);
uInt32 CalculateTriggerCountFromConfig(CSSYSTEMINFO* pCsSysInfo, const LPCTSTR szIniFile);

BOOL isChannelValid(uInt32 u32ChannelIndex, uInt32 u32mode, uInt16 u16cardIndex, CSSYSTEMINFO* pCsSysInfo);
uInt32 GetSectorSize();
void  UpdateProgress( uInt32 u32Elapsed, LONGLONG llSamples );
int32 LoadStmConfiguration(LPCTSTR szIniFile, PCSSTMCONFIG pConfig);

void* CardStreamThread(void *arg);
void signal_handler(int sig);
uInt32 GetTickFuntion();
int CreateIPCObjects();
int DestroyIPCObjects();
int ComputeTimeout(int timeOutms, struct timespec *tm );
DWORD GetFileAttributes(LPCTSTR szFilePath);
DWORD GetFullPathName(LPCTSTR szIniFile, DWORD nBufferLength, LPTSTR szFilePath, LPTSTR* lpFilePart);
DWORD GetPrivateProfileSection(	LPCTSTR lpAppName,
								LPTSTR lpReturnedString,
								DWORD nSize,
								LPCTSTR lpFileName);
DWORD GetPrivateProfileString(	LPCTSTR lpAppName,
								LPCTSTR lpKeyName,
								LPCTSTR lpDefault,
								LPTSTR lpReturnedString,
								DWORD nSize,
								LPCTSTR lpFileName);
DWORD GetPrivateProfileInt(	LPCTSTR lpAppName,
							LPCTSTR lpKeyName,
							int nDefault,
							LPCTSTR lpFileName);					  
