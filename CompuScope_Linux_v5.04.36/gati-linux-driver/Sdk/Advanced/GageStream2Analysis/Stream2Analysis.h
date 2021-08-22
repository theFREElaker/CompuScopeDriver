
#pragma once


#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>


#include <CsAppSupport.h>
#include "CsTchar.h"
#include "CsSdkMisc.h"
#include "CsSdkUtil.h"
#include "CsExpert.h"
#include <CsPrototypes.h>


#define MAX_SEGMENTS_COUNT	25000			/* Max number of segments in a single file */
#define STREAM_BUFFERSIZE	0x200000
#define TRANSFER_TIMEOUT	10000
#define OUT_FILE			"Result.txt"	/* name of the output file */
#define STM_SECTION			"StmConfig"		/* section name in ini file */
#define MAX_CARDS_COUNT		10				/* Max number of cards supported in a M/S Compuscope system */

typedef struct
{
	uInt32		u32BufferSizeBytes;
	uInt32		u32TransferTimeout;
	char		strResultFile[MAX_PATH];
	BOOL		bDoAnalysis;				/* Turn on or off data analysis */
}CsStreamConfig, *PCsStreamConfig;

typedef struct _STREAM_INFO
{
	//Buffers
	void*		pWorkBuffer;
	int64*		pi64TimeStamp;

	//Buffer size in bytes
	uInt32		u32BufferSize;
	int64		i64SegmentSize;
	uInt32		u32TailSize;
	uInt32		u32LeftOverSize;

	//Number of bytes till end of 
	int64		i64ByteToEndSegment;
	uInt32		u32ByteToEndTail;

	//Informations for saving
	int64		i64DeltaTime;
	int64		i64LastTs;
	uInt32		u32Segment;

	uInt32		u32SegmentCountDown;
	BOOL		bSplitTail;					/* Flag to signify if the tail is split betweentwo buffers	*/

}STREAM_INFO, *PSTREAM_INFO;

typedef struct
{
	pthread_mutex_t mutex;
	pthread_cond_t	cond;
}Event, *pEvent;


int32	InitializeStream(CSHANDLE g_hSystem);
int CreateIPCObjects();
int DestroyIPCObjects();
int ComputeTimeout(int timeOutms, struct timespec *tm );
void signal_handler(int sig);

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

uInt32 AnalysisFunc(PSTREAM_INFO pStreamInfo);

void	UpdateProgress( uInt32 u32Elapsed, LONGLONG llSamples );
void	UpdateResultFile(FILE** pInit, uInt16 nCardIndex, uInt32* pu32NbrFiles);
void	SaveResults(PSTREAM_INFO streamInfo,
					uInt32 u32Count,
					uInt32* pu32SegmentFileCount,
					uInt32* pu32nbFiles,
					FILE **pFile,
					uInt16 u16CardIndex,
					uInt32* pu32TotalSegmentCount);
					
int32	LoadStreamConfiguration(LPCTSTR szIniFile, PCsStreamConfig pConfig);

void* CardStream( void *CardIndex );

