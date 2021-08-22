/////////////////////////////////////////////////////////////////////////
//
// GageStream2Disk
//
// See the official site at www.gage-applied.com for documentation and
// the latest news.
//
/////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 1994-2012 by Gage Applied Technologies.
// All rights reserved.
//
// This code is free for personal and commercial use, providing this
// notice remains intact in the source files and all eventual changes are
// clearly marked with comments.
//
// No warranty of any kind, expressed or implied, is included with this
// software; use at your own risk, responsibility for damages (if any) to
// anyone resulting from the use of this software rests entirely with the
// user.
//
/////////////////////////////////////////////////////////////////////////
//
// GageStream2Disk demonstrates the Streamming to Disk capabilities of
// CompuScope data acquisition system.  The samples program will setup the Compuscope
// system to multiple records streaming mode. All data captured will be saved
// into hard disk.
//
///////////////////////////////////////////////////////////////////////////

#define _GNU_SOURCE	// for pthread_timedjoin_np

#include "Stream2Disk.h"

LONGLONG            g_llCardTotalSamples[MAX_CARDS_COUNT] = {0};

CSHANDLE            g_hSystem = 0;

CSACQUISITIONCONFIG g_CsAcqCfg = {0};
CSSYSTEMINFO        g_CsSysInfo = {0};
CSSTMCONFIG         g_StreamConfig;
int                 g_ThreadStartStatus = 0;


Event               g_hStreamStarted;
Event               g_hThreadReadyForStream;

BOOL				g_bCtrlAbort = FALSE;
BOOL			   g_bStreamError = FALSE;
BOOL				g_bTransferError = FALSE;
FILE				*g_hFile[MAX_CARDS_COUNT] = {NULL};

int _tmain()
{
	int32           i32Status = CS_SUCCESS;
	uInt32          u32Mode;
	LPCTSTR         szIniFile = _T("Stream2Disk.ini");
	BOOL            bDone = FALSE;
	long long       llSystemTotalSamples = 0;
	uInt16          n;
	uInt16          i;
	LONGLONG        llTotalSamplesConfig = 0;

	pthread_t*      thread;
	pthread_attr_t  attr;
	BOOL*			threadFinished;

	int             ret = 0;
	uInt32          u32TickStart = 0;
	uInt32          u32TickNow = 0;
	struct sigaction act;
	CsDataPackMode	CurrentPackConfig;

	memset (&act, '\0', sizeof(act));

	// Initializes the CompuScope boards found in the system. If the
	// system is not found a message with the error code will appear.
	// Otherwise i32Status will contain the number of systems found.
	i32Status = CsInitialize();

	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		return (-1);
	}

	// Get the system. This sample program only supports one system. If
	// 2 systems or more are found, the first system that is found
	// will be the system that will be used. g_hSystem will hold a unique
	// system identifier that is used when referencing the system.

	i32Status = CsGetSystem(&g_hSystem, 0, 0, 0, 0);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		return (-1);
	}

	// Get System information. The u32Size field must be filled in
	// prior to calling CsGetSystemInfo
	g_CsSysInfo.u32Size = sizeof(CSSYSTEMINFO);
	i32Status = CsGetSystemInfo(g_hSystem, &g_CsSysInfo);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	// Display the system name from the driver
	_ftprintf(stdout, _T("\nBoard Name: %s"), g_CsSysInfo.strBoardName);

	//  We are analysing the ini file to find the number of triggers
	i32Status = CsAs_ConfigureSystem(g_hSystem, (int)g_CsSysInfo.u32ChannelCount,
		(int)CalculateTriggerCountFromConfig(&g_CsSysInfo,(LPCTSTR)szIniFile),
		(LPCTSTR)szIniFile, &u32Mode);

	if (CS_FAILED(i32Status))
	{
		if (CS_INVALID_FILENAME == i32Status)
		{
			// Display message but continue on using defaults.
			_ftprintf(stdout, _T("\nCannot find %s - using default parameters."), szIniFile);
		}
		else
		{

			// Otherwise the call failed.  If the call did fail we should free the CompuScope
			// system so it's available for another application
			DisplayErrorString(i32Status);
			CsFreeSystem(g_hSystem);
			return(-1);
		}
	}

	// If the return value is greater than  1, then either the application,
	// acquisition, some of the Channel and / or some of the Trigger sections
	// were missing from the ini file and the default parameters were used.
	if (CS_USING_DEFAULT_ACQ_DATA & i32Status)
	{
		_ftprintf(stdout, _T("\nNo ini entry for acquisition. Using defaults."));
	}

	if (CS_USING_DEFAULT_CHANNEL_DATA & i32Status)
	{
		_ftprintf(stdout, _T("\nNo ini entry for one or more Channels. Using defaults for missing items."));
	}

	if (CS_USING_DEFAULT_TRIGGER_DATA & i32Status)
	{
		_ftprintf(stdout, _T("\nNo ini entry for one or more Triggers. Using defaults for missing items."));
	}

	// Load application specific information from the ini file
	i32Status = LoadStmConfiguration(szIniFile, &g_StreamConfig);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(g_hSystem);
		return (-1);
	}
	if (CS_USING_DEFAULTS == i32Status)
	{
		_ftprintf(stdout, _T("\nNo ini entry for Stm configuration. Using defaults."));
	}

	// Streaming Configuration.
	// Validate if the board supports hardware streaming. If  it is not supported,
	// we'll exit gracefully.
	i32Status = InitializeStream(g_hSystem);
	if (CS_FAILED(i32Status))
	{
		// Error string was displayed in InitializeStream
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	if (CreateIPCObjects()==-1)
	{
		_ftprintf (stderr, _T("\nUnable to create IPC object for synchronization.\n"));
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	// Detect if the CompuScope card supports Streamming with Data Packing 
	i32Status =	CsGet( g_hSystem, 0, CS_DATAPACKING_MODE, &CurrentPackConfig );
	if ( CS_FAILED(i32Status) )
	{
		if ( CS_INVALID_PARAMS_ID == i32Status )
		{
			// The parameter CS_DATAPACKING_MODE is not supported
			if ( 0 != g_StreamConfig.DataPackCfg )
			{
				_ftprintf (stderr, _T("\nThe current CompuScope system does not support Data Streamming in Packed mode.\n"));
				CsFreeSystem(g_hSystem);
				return (-1);
			}
		}
		else
		{
			// Other errors
			DisplayErrorString(i32Status);
			CsFreeSystem(g_hSystem);
			return (-1);
		}
	}
	else
	{
		// The CompuScope system support Streamming in Packed data format.
		// Set the Data Packing mode for streaming
		// This should be done before ACTION_COMMIT
		i32Status =	CsSet( g_hSystem, CS_DATAPACKING_MODE, &g_StreamConfig.DataPackCfg );
		if (CS_FAILED(i32Status))
		{
			DisplayErrorString(i32Status);
			CsFreeSystem(g_hSystem);
			return (-1);
		}
	}

	// Commit the values to the driver.  This is where the values get sent to the
	// hardware.  Any invalid parameters will be caught here and an error returned.
	i32Status = CsDo(g_hSystem, ACTION_COMMIT);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	// After ACTION_COMMIT, the sample size may change.
	// Get user's acquisition data to use for various parameters for transfer
	g_CsAcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);
	i32Status = CsGet(g_hSystem, CS_ACQUISITION, CS_CURRENT_CONFIGURATION, &g_CsAcqCfg);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	// Get the total amount of data we expect to receive.
	// We can get this value from driver or calculate the following formular
	// llTotalSamplesConfig = (g_CsAcqCfg.i64SegmentSize + SegmentTail_Size) * (g_CsAcqCfg.u32Mode&CS_MASKED_MODE) * g_CsAcqCfg.u32SegmentCount;

	i32Status = CsGet( g_hSystem, 0, CS_STREAM_TOTALDATA_SIZE_BYTES, &llTotalSamplesConfig );
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	// Convert to number of samples
	if ( -1 != llTotalSamplesConfig )
	{
		llTotalSamplesConfig /= g_CsAcqCfg.u32SampleSize;
	}

	//  Create threads for Stream. In M/S system, we have to create one thread per card
	thread = malloc(g_CsSysInfo.u32BoardCount * sizeof(pthread_t));
	threadFinished = malloc(g_CsSysInfo.u32BoardCount * sizeof(BOOL));

	for (n = 1, i = 0; n <= g_CsSysInfo.u32BoardCount; n++, i++ )
	{
		// remove the old file
		if(g_StreamConfig.bSaveToFile)
		{
			struct stat fstat = {0};
			TCHAR szSaveFileName[MAX_PATH] = {0};
			snprintf( szSaveFileName, sizeof (szSaveFileName), "%s_%d.dat", g_StreamConfig.strResultFile, n );
			int result = stat(szSaveFileName,&fstat);
			if (!result)
			{
				_ftprintf (stdout, _T("\nPreparing data file to record <%s> \n"), szSaveFileName);
				remove(szSaveFileName);      
			}

			// open new file
			g_hFile[n-1] = fopen(szSaveFileName,"w+");
			if ( 0 == g_hFile[n-1] )
			{
				_ftprintf (stdout, _T("\n Unable to open file <%s> <errno = %d msg: %s> \n"), szSaveFileName, errno, strerror(errno));
				CsFreeSystem(g_hSystem);
				return (-1);
			}
		}
		fflush(stdout);
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		ret = pthread_create(&thread[i], &attr, CardStreamThread, (void*)&n);
		threadFinished[i] = FALSE;

		if (ret != 0)
		{
			_ftprintf (stderr, _T("Unable to create thread for card %d.\n"), n);
			g_bStreamError = TRUE;
			CsFreeSystem(g_hSystem);
			free(thread);
			free(threadFinished);
			DestroyIPCObjects();
			return (-1);
		}
		else
		{
			// Wait for the g_hThreadReadyForStream to make sure that the thread was successfully created and are ready for stream
			struct timespec wait_timeout ={0};
			pthread_mutex_lock(&g_hThreadReadyForStream.mutex);
			ComputeTimeout(5000, &wait_timeout);
			pthread_cond_timedwait(&g_hThreadReadyForStream.cond, &g_hThreadReadyForStream.mutex, &wait_timeout);
			pthread_mutex_unlock(&g_hThreadReadyForStream.mutex);

			// make sure the CardStreamThread start normally
			if (g_ThreadStartStatus < 0)
			{
       			g_bStreamError = TRUE;
				CsFreeSystem(g_hSystem);
				free(thread);
				free(threadFinished);
				DestroyIPCObjects();
				return (-1);
			}
		}
	} // End for

	/* register signal interrupt handler for contol_c */
	act.sa_handler = &signal_handler;
	sigaction(SIGINT, &act, NULL);


	// Start the streaming data acquisition
	printf ("\nStart streaming. Use CTRL^C to abort\n\n");
	i32Status = CsDo(g_hSystem, ACTION_START);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(g_hSystem);
		free(thread);
		free(threadFinished);
		DestroyIPCObjects();
		return (-1);
	}

	u32TickStart = u32TickNow = GetTickCount();

	pthread_mutex_lock(&g_hStreamStarted.mutex);
	pthread_cond_broadcast(&g_hStreamStarted.cond);
	pthread_mutex_unlock(&g_hStreamStarted.mutex);

	while( !bDone  )
	{
		struct timespec wait_timeout ={0};
		BOOL bAllThreadsFinished = TRUE;

		// Are we being asked to quit?
		if (g_bCtrlAbort)
		{
			printf("\nAborting capture\n");
			break;
		}

		ComputeTimeout(1000, &wait_timeout);

		for (i = 0; i < g_CsSysInfo.u32BoardCount; i++)
		{
			int retVal = pthread_timedjoin_np(thread[i], NULL, &wait_timeout);

			if ( EBUSY == retVal || ETIMEDOUT == retVal )
			{
				break;
			}
			else
			{
				threadFinished[i] = TRUE;
			}
		}

		u32TickNow = GetTickCount();

		// Calculate the sum of all data received so far
		llSystemTotalSamples = 0;
		for (i = 0; i < g_CsSysInfo.u32BoardCount; i++ )
		{
			llSystemTotalSamples += g_llCardTotalSamples[i];
		}

		UpdateProgress(u32TickNow-u32TickStart, llSystemTotalSamples * g_CsSysInfo.u32SampleSize);

		// Check which threads are finished.  If they're all finished set bDone to true
		for (i = 0; i < g_CsSysInfo.u32BoardCount; i++)
		{
			if (!threadFinished[i])
			{
				bAllThreadsFinished = FALSE;
				break;
			}
		}
		bDone = bAllThreadsFinished;
	}


	// Wait for CardStreamThread to finish ..
	sleep(1);
	// Abort the current acquisition
	CsDo(g_hSystem, ACTION_ABORT);

	// Free the CompuScope system and any resources it's been using
	i32Status = CsFreeSystem(g_hSystem);

	if ( !g_bStreamError && !g_bCtrlAbort && !g_bTransferError )
	{
		_ftprintf (stdout, _T("\nStream has finished %u loops.\n"), g_CsAcqCfg.u32SegmentCount);
	}
	else
	{
		if (g_bCtrlAbort)
		{
			_ftprintf (stdout, _T("\nStream aborted by user.\n"));
		}
		else if ( g_bStreamError || g_bTransferError )
		{
			_ftprintf (stdout, _T("\nStream aborted on error.\n"));
		}
	}
	_ftprintf (stdout, _T("\nFreeing resources..\n"));
	free(thread);
	free(threadFinished);
	DestroyIPCObjects();
	return (0);
}

/***************************************************************************************************
****************************************************************************************************/

uInt32 CalculateTriggerCountFromConfig(CSSYSTEMINFO* pCsSysInfo, const LPCTSTR szIniFile)
{
	TCHAR   szFilePath[MAX_PATH];
	TCHAR   szTrigger[100];
	TCHAR   szString[100];
	uInt32  i = 0;

	GetFullPathName(szIniFile, MAX_PATH, szFilePath, NULL);

	for( ; i < pCsSysInfo->u32TriggerMachineCount; ++i)
	{
		_stprintf(szTrigger, _T("Trigger%i"), i+1);

		if (0 == GetPrivateProfileSection(szTrigger, szString, 100, szFilePath))
		{
			break;
		}
	}
	return i;
}

/***************************************************************************************************
****************************************************************************************************/


BOOL isChannelValid(uInt32 u32ChannelIndex, uInt32 u32mode, uInt16 u16cardIndex, CSSYSTEMINFO* pCsSysInfo)
{
	uInt32 mode = u32mode & CS_MASKED_MODE;
	uInt32 channelsPerCard = pCsSysInfo->u32ChannelCount / pCsSysInfo->u32BoardCount;
	uInt32 min = ((u16cardIndex-1) * channelsPerCard) + 1;
	uInt32 max = (u16cardIndex * channelsPerCard);

	if((u32ChannelIndex-1) % (pCsSysInfo->u32ChannelCount / mode) != 0)
	{
		return FALSE;
	}
	return (u32ChannelIndex >= min && u32ChannelIndex <= max);
}

/***************************************************************************************************
****************************************************************************************************/

uInt32 GetSectorSize()
{
	return 4096;
}

/***************************************************************************************************
****************************************************************************************************/

int32 InitializeStream(CSHANDLE g_hSystem)
{
	int32   i32Status = CS_SUCCESS;
	int64   i64ExtendedOptions = 0;
	char    szExpert[64];
	CSACQUISITIONCONFIG CsAcqCfg = {0};

	strncpy(szExpert, "Stream", sizeof(szExpert) );

	CsAcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);

	// Get user's acquisition Data
	i32Status = CsGet(g_hSystem, CS_ACQUISITION, CS_CURRENT_CONFIGURATION, &CsAcqCfg);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		return (i32Status);
	}

	// Check if selected system supports Expert Stream
	// And set the correct image to be used.
	CsGet(g_hSystem, CS_PARAMS, CS_EXTENDED_BOARD_OPTIONS, &i64ExtendedOptions);

	if (i64ExtendedOptions & CS_BBOPTIONS_STREAM)
	{
		_ftprintf(stdout, _T("\nSelecting Expert %s from image 1."), szExpert);
		CsAcqCfg.u32Mode |= CS_MODE_USER1;
	}
	else if ((i64ExtendedOptions >> 32) & CS_BBOPTIONS_STREAM)
	{
		_ftprintf(stdout, _T("\nSelecting Expert %s from image 2."), szExpert);
		CsAcqCfg.u32Mode |= CS_MODE_USER2;
	}
	else
	{
		_ftprintf(stdout, _T("\nCurrent system does not support Expert %s."), szExpert);
		_ftprintf(stdout, _T("\nApplication terminated."));
		return CS_MISC_ERROR;
	}

	// Sets the Acquisition values down the driver, without any validation,
	// for the Commit step which will validate system configuration.
	i32Status = CsSet(g_hSystem, CS_ACQUISITION, &CsAcqCfg);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		return CS_MISC_ERROR;
	}

	return CS_SUCCESS;
}


/***************************************************************************************************
****************************************************************************************************/

void UpdateProgress( uInt32 u32Elapsed, LONGLONG llTotaBytes )
{
	uInt32  h = 0;
	uInt32  m = 0;
	uInt32  s = 0;
	double  dRate;
	double	dTotal;

	if ( u32Elapsed > 0 )
	{
		dRate = (llTotaBytes / 1000000.0) / (u32Elapsed / 1000.0);

		if (u32Elapsed >= 1000)
		{
			if ((s = u32Elapsed / 1000) >= 60)  // Seconds
			{
				if ((m = s / 60) >= 60)         // Minutes
				{
					if ((h = m / 60) > 0)       // Hours
					{
						m %= 60;
					}
				}
				s %= 60;
			}
		}
		dTotal = 1.0*llTotaBytes/1000000.0;		// Mega samples
		_ftprintf(stdout, _T("\rTotal: %0.2f MB, Rate: %6.2f MB/s, Elapsed time: %u:%02u:%02u  "), dTotal, dRate, h, m, s);
		fflush(stdout);
	}
}

#define	MIN_STREAM_BUFFER_SIZE		0x2000		// 4K

/***************************************************************************************************
****************************************************************************************************/

int32 LoadStmConfiguration(LPCTSTR szIniFile, PCSSTMCONFIG pConfig)
{
	TCHAR   szDefault[MAX_PATH];
	TCHAR   szString[MAX_PATH];
	TCHAR   szFilePath[MAX_PATH];
	int     nDummy;
	CSSTMCONFIG CsStmCfg;

	// Set defaults in case we can't read the ini file
	CsStmCfg.u32BufferSizeBytes = STREAM_BUFFERSZIZE;
	CsStmCfg.u32TransferTimeout = TRANSFER_TIMEOUT;
	strcpy(CsStmCfg.strResultFile, _T(OUT_FILE));

	if (NULL == pConfig)
	{
		return (CS_INVALID_PARAMETER);
	}

	GetFullPathName(szIniFile, MAX_PATH, szFilePath, NULL);

	if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(szFilePath))
	{
		*pConfig = CsStmCfg;
		return (CS_USING_DEFAULTS);
	}

	if (0 == GetPrivateProfileSection(STM_SECTION, szString, 100, szFilePath))
	{
		*pConfig = CsStmCfg;
		return (CS_USING_DEFAULTS);
	}

	nDummy = 0;
	CsStmCfg.bSaveToFile = (0 != GetPrivateProfileInt(STM_SECTION, _T("SaveToFile"), nDummy, szFilePath));

	nDummy = 0;
	CsStmCfg.bSaveHeader = (0 != GetPrivateProfileInt(STM_SECTION, _T("Header"), nDummy, szFilePath));

	nDummy = 0;
	CsStmCfg.bFileFlagNoBuffering = (0 != GetPrivateProfileInt(STM_SECTION, _T("FileFlagNoBuffering"), nDummy, szFilePath));

	nDummy = CsStmCfg.u32TransferTimeout;
	CsStmCfg.u32TransferTimeout = GetPrivateProfileInt(STM_SECTION, _T("TimeoutOnTransfer"), nDummy, szFilePath);

	nDummy = CsStmCfg.u32BufferSizeBytes;
	CsStmCfg.u32BufferSizeBytes =  GetPrivateProfileInt(STM_SECTION, _T("BufferSize"), nDummy, szFilePath);

   if (CsStmCfg.u32BufferSizeBytes < MIN_STREAM_BUFFER_SIZE)
      CsStmCfg.u32BufferSizeBytes = MIN_STREAM_BUFFER_SIZE;

	nDummy = 0;
	CsStmCfg.bErrorHandling = (0 != GetPrivateProfileInt(STM_SECTION, _T("ErrorHandlingMode"), nDummy, szFilePath));

	nDummy = 0;
	CsStmCfg.DataPackCfg = GetPrivateProfileInt(STM_SECTION, _T("DataPackMode"), nDummy, szFilePath);

	_stprintf(szDefault, _T("%s"), CsStmCfg.strResultFile);
	GetPrivateProfileString(STM_SECTION, _T("DataFile"), szDefault, szString, MAX_PATH, szFilePath);
	_tcscpy(CsStmCfg.strResultFile, szString);

	*pConfig = CsStmCfg;
	return (CS_SUCCESS);
}

/***************************************************************************************************
****************************************************************************************************/
void* CardStreamThread( void *CardIndex )
{
	uInt16              nCardIndex = *((uInt16 *) CardIndex);
	void*               pBuffer1 = NULL;
	void*               pBuffer2 = NULL;
	void*               pCurrentBuffer = NULL;
	void*               pWorkBuffer = NULL;

	uInt32              u32TransferSizeSamples = 0;
	uInt32              u32SectorSize = 4096;         //should be 512 or 4K for Linux
	uInt32				u32DmaBoundary = 16;
	uInt32              u32WriteSize;
	int32               i32Status;

	BOOL                bDone = FALSE;
	uInt32              u32LoopCount = 0;
	uInt32              u32ErrorFlag = 0;
	DWORD               dwBytesSave = 0;
	FILE				*hFile;
	TCHAR               szSaveFileName[MAX_PATH];
	uInt32              u32ActualLength = 0;
	uInt8               u8EndOfData = 0;
	BOOL                bStreamCompletedSuccess = FALSE;

	if (g_StreamConfig.bSaveToFile)
	{
		TCHAR szSaveFileName[MAX_PATH] = {0};
		snprintf( szSaveFileName, sizeof (szSaveFileName), "%s_%d.dat", g_StreamConfig.strResultFile, nCardIndex );
		if (g_hFile[nCardIndex-1])
			hFile = g_hFile[nCardIndex-1];
		else
		{
			_ftprintf (stdout, _T("\nfile (%s) open error!  \n"), szSaveFileName);
			CsFreeSystem(g_hSystem);
			return NULL;
		}
	}

/*
	We need to allocate a buffer for transferring the data. Buffer is allocated as void with
	a size of length * number of channels * sample size. All channels in the mode are transferred
	within the same buffer, so the mode tells us the number of channels.  Currently, TAIL_ADJUST
	samples are placed at the end of each segment. These samples contain timestamp information for the
	segemnt.  The buffer must be allocated by a call to CsStmAllocateBuffer.  This routine will
	allocate a buffer suitable for streaming.  In this program we're allocating 2 streaming buffers
	so we can transfer to one while doing analysis on the other.
*/
	u32SectorSize = GetSectorSize();

	if ( g_StreamConfig.bFileFlagNoBuffering )
	{
		// If bFileFlagNoBuffering is set, the buffer size should be multiple of the sector size of the Hard Disk Drive.
		// Most of HDDs have the sector size equal 512 or 1024.
		// Round up the buffer size into the sector size boundary
		u32DmaBoundary = u32SectorSize;
	}

	// Round up the DMA buffer size to DMA boundary (required by the Streaming data transfer)
	if ( g_StreamConfig.u32BufferSizeBytes % u32DmaBoundary )
		g_StreamConfig.u32BufferSizeBytes += (u32DmaBoundary - g_StreamConfig.u32BufferSizeBytes % u32DmaBoundary);

	_ftprintf (stderr, _T("\n(Actual buffer size used for data streaming = %u Bytes)\n"), g_StreamConfig.u32BufferSizeBytes );

	i32Status = CsStmAllocateBuffer(g_hSystem, nCardIndex, g_StreamConfig.u32BufferSizeBytes, &pBuffer1);
	if (CS_FAILED(i32Status))
	{
		_ftprintf (stderr, _T("\nUnable to allocate memory for stream buffer 1. i32Status %d \n"),i32Status);
		fclose(hFile);
		remove(szSaveFileName);
		g_ThreadStartStatus = -1;
		return NULL;
	}
	i32Status = CsStmAllocateBuffer(g_hSystem, nCardIndex, g_StreamConfig.u32BufferSizeBytes, &pBuffer2);
	if (CS_FAILED(i32Status))
	{
		_ftprintf (stderr, _T("\nUnable to allocate memory for stream buffer 2.\n"));
		CsStmFreeBuffer(g_hSystem, nCardIndex, pBuffer1);
		fclose(hFile);
		remove(szSaveFileName);
		g_ThreadStartStatus = -1;
		return NULL;
	}

	// So far so good ...
	// Let the main thread know that this thread is ready for stream

	pthread_mutex_lock(&g_hThreadReadyForStream.mutex);
	pthread_cond_signal(&g_hThreadReadyForStream.cond);
	pthread_mutex_unlock(&g_hThreadReadyForStream.mutex);

	// we wait for Thread Ready. Abort will be treat later

	struct timespec wait_timeout ={0};
	memset(&wait_timeout, 0, sizeof(struct timespec));
	ComputeTimeout(5000, &wait_timeout);

	pthread_mutex_lock(&g_hStreamStarted.mutex);
	pthread_cond_wait(&g_hStreamStarted.cond, &g_hStreamStarted.mutex);

	pthread_mutex_unlock(&g_hStreamStarted.mutex);

	// Convert the transfer size to BYTEs or WORDs depending on the card.
	u32TransferSizeSamples = g_StreamConfig.u32BufferSizeBytes/g_CsSysInfo.u32SampleSize;

	// Steam acqusition has started.
	// loop until either we've done the number of segments we want, or
	// the ESC key was pressed to abort. While we loop, we transfer data into
	// pCurrentBuffer and save pWorkBuffer to hard diskinfo

	ComputeTimeout(0, &wait_timeout);

	while( ! ( bDone || bStreamCompletedSuccess) )
	{
		if (g_bCtrlAbort || g_bStreamError)
		{
			break;
		}

		// Determine where new data transfer data will go. We alternate
		// between our 2 streaming buffers
		if (u32LoopCount & 1)
		{
			pCurrentBuffer = pBuffer2;
		}
		else
		{
			pCurrentBuffer = pBuffer1;
		}

		i32Status = CsStmTransferToBuffer(g_hSystem, nCardIndex, pCurrentBuffer, u32TransferSizeSamples);

		if (CS_FAILED(i32Status))
		{
			if ( CS_STM_COMPLETED == i32Status )
			{
				bStreamCompletedSuccess = TRUE;
			}
			else
			{
            DisplayErrorString(i32Status);
            g_bTransferError = TRUE;
			}
			break;
		}

		if ( g_StreamConfig.bSaveToFile && NULL != pWorkBuffer )
		{
			// Save the data from pWorkBuffer to hard disk
			dwBytesSave = 0;
			dwBytesSave = fwrite( pWorkBuffer, 1, g_StreamConfig.u32BufferSizeBytes, hFile );
			fflush(hFile);
			if ( dwBytesSave != g_StreamConfig.u32BufferSizeBytes )
			{
				_ftprintf (stdout, _T("\fwrite() on card %d return: <errno = %d msg: %s> \n"), nCardIndex, errno, strerror(errno));
				g_bStreamError = TRUE;
				bDone = TRUE;
			}
		}
		// Wait for the DMA transfer on the current buffer to complete so we can loop back around to start a new one.
		// The calling thread will sleep until the transfer completes

		i32Status = CsStmGetTransferStatus( g_hSystem, nCardIndex, g_StreamConfig.u32TransferTimeout, &u32ErrorFlag, &u32ActualLength, &u8EndOfData );
		if ( CS_SUCCEEDED(i32Status) )
		{

			// Calculate the total of data transfered so far for this card

			g_llCardTotalSamples[nCardIndex-1] += u32ActualLength;
			bStreamCompletedSuccess = (0 != u8EndOfData);

			if ( STM_TRANSFER_ERROR_FIFOFULL & u32ErrorFlag )
			{
				// The Fifo full error has occured at the card level which results data lost.
				// This error occurs when the application is not fast enough to transfer data.
				if ( 0 != g_StreamConfig.bErrorHandling )
				{
					// Stop as soon as we recieve the FIFO full error from the card
					_ftprintf (stdout, _T("\nFifo full detected on card %d !!!\n"), nCardIndex);
					g_bStreamError = TRUE;
					bDone = TRUE;
				}
				else
				{
					// Transfer all valid dat a into the PC RAM

					// Althought the Fifo full has occured, there is valid data available on the On-board memory.
					// To transfer these data into the PC RAM, we can keep calling CsStmTransferToBuffer() then CsStmGetTransferStatus()
					// until the function CsStmTransferToBuffer() returns the error CS_STM_FIFO_OVERFLOW.
					// The error CS_STM_FIFO_OVERFLOW indicates that all valid data has been transfered to the PC RAM

					// Do thing here, go backto the loop CsStmTransferToBuffer() CsStmGetTransferStatus()
				}
			}
		}
		else
		{
			bDone = TRUE;
			g_bStreamError = TRUE;
			if ( CS_STM_TRANSFER_TIMEOUT == i32Status )
			{
				//  Timeout on CsStmGetTransferStatus().
				//  Data transfer has not yet completed. We could repeat calling CsStmGetTransferStatus() until we get the completion status.
				//  In this sample program, we consider this as an  error
				_ftprintf (stdout, _T("\nStream transfer timeout on card %d !!!\n"), nCardIndex);
			}
			else // some other error
			{
				char szErrorString[255];
				CsGetErrorString(i32Status, szErrorString, sizeof(szErrorString));
				_ftprintf (stdout, _T("\n%s on card %d !!!\n"), szErrorString, nCardIndex);
			}
		}

		pWorkBuffer = pCurrentBuffer;
		u32LoopCount++;
	} /* end while */

	if ( bStreamCompletedSuccess && g_StreamConfig.bSaveToFile && NULL != pWorkBuffer )
	{
		u32WriteSize = u32ActualLength*g_CsAcqCfg.u32SampleSize;

		// Save the data from pWorkBuffer to hard disk
		dwBytesSave = fwrite( pWorkBuffer, 1, u32WriteSize, hFile );
		fflush(hFile);
		if ( dwBytesSave != u32WriteSize )
		{
			_ftprintf (stdout, _T("\fwrite() on card %d return: <errno = %d msg: %s> \n"), nCardIndex, errno, strerror(errno));
			g_bStreamError = TRUE;
		}
	}

	// Close the data file and free all streaming buffers
	if ( g_StreamConfig.bSaveToFile )
	{
		// sync to disc
		if (g_bCtrlAbort==FALSE)
			fsync(fileno(hFile));
		fclose(hFile);
	}
	CsStmFreeBuffer(g_hSystem, nCardIndex, pBuffer1);
	CsStmFreeBuffer(g_hSystem, nCardIndex, pBuffer2);

	return NULL;
}

void signal_handler(int sig)
{
	/* Handles Ctrl+C in terminal */
	g_bCtrlAbort = TRUE;
}

int CreateIPCObjects()
{
	if ( 0 != pthread_mutex_init(&g_hStreamStarted.mutex, NULL) )
	{
		return (-1);
	}

	if ( 0 != pthread_cond_init(&g_hStreamStarted.cond, NULL) )
	{
		return (-1);
	}

	if ( 0 != pthread_mutex_init(&g_hThreadReadyForStream.mutex, NULL) )
	{
		return (-1);
	}

	if ( 0 != pthread_cond_init(&g_hThreadReadyForStream.cond, NULL) )
	{
		return (-1);
	}

	return(0);
}

int DestroyIPCObjects()
{
	if ( 0 != pthread_mutex_destroy(&g_hStreamStarted.mutex) )
	{
		return (-1);
	}

	if ( 0 != pthread_cond_destroy(&g_hStreamStarted.cond) )
	{
		return (-1);
	}

	if ( 0 != pthread_mutex_destroy(&g_hThreadReadyForStream.mutex) )
	{
		return (-1);
	}

	if ( 0 != pthread_cond_destroy(&g_hThreadReadyForStream.cond) )
	{
		return (-1);
	}

	return(0);
}

int ComputeTimeout(int timeOutms, struct timespec *tm )
{
	long sec, millisec;
	struct timespec ts;
	if ( clock_gettime(CLOCK_REALTIME, &ts) == 0)
	{
		sec = timeOutms / 1000;
		millisec = timeOutms % 1000;

		tm->tv_sec = ts.tv_sec + sec;
		tm->tv_nsec = ts.tv_nsec + (millisec * 1000000) ;
		return(0);
	}
	return(-1);
}
