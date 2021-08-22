  /////////////////////////////////////////////////////////////////////////
//
// GageStream2Analysis
//
// See the official site at www.gage-applied.com for documentation and
// the latest news.
//
/////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 1994-2005 by Gage Applied Technologies.
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
// GageStream2Analysis demonstrates the Streamming to Analysis capabilities of 
// CompuScope data acquisition system.  The samples program will setup the Compuscope 
// system to multiple records streaming mode.  A simple function that save the timestamp of s
// each segment is used as the analysis function.
//
///////////////////////////////////////////////////////////////////////////

#define _GNU_SOURCE	/* for pthread_timedjoin_np */

#include "Stream2Analysis.h"


#define TRUE				1
#define FALSE				0

LONGLONG	g_llLastSamples[MAX_CARDS_COUNT]	= {0};
LONGLONG	g_llTotalSamples[MAX_CARDS_COUNT]	= {0};

Event       g_hStreamStarted;
Event       g_hThreadReadyForStream;

BOOL		g_bCtrlAbort	= FALSE;
BOOL		g_bStreamError 	= FALSE;

int			g_ThreadStartStatus	= 0;
int64		g_i64TsFrequency	= 0;
uInt32		g_u32SegmentCounted[MAX_CARDS_COUNT] = {0};
uInt32		g_u32SegmentFileCount	= 1;
int64		g_i64SegmentPerBuffer	= 1;


CSHANDLE		g_hSystem  	= 0;
CSACQUISITIONCONFIG	g_CsAcqCfg 	= {0};
CsStreamConfig		g_StreamConfig;

int _tmain()
{
	int32			i32Status = CS_SUCCESS;
	uInt32			u32Mode;
	CSSYSTEMINFO	CsSysInfo = {0};
	LPCTSTR			szIniFile = _T("Stream2Analysis.ini");
	BOOL			bDone = FALSE;
	long long		llTotalSamples = 0;
	uInt16			n;
	uInt16			i;
	uInt32			u32TickStart = 0;
	uInt32			u32TickNow = 0;
	int				ret;
	pthread_t*		thread = NULL;
	pthread_attr_t  attr;
	BOOL*			threadFinished;	
	

	struct sigaction act;

	memset (&act, '\0', sizeof(act));	

	/*
	Initializes the CompuScope boards found in the system. If the
	system is not found a message with the error code will appear.
	Otherwise i32Status will contain the number of systems found.
	*/
	i32Status = CsInitialize();

	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		return (-1);
	}
	/*
	Get the system. This sample program only supports one system. If
	2 systems or more are found, the first system that is found
	will be the system that will be used. g_hSystem will hold a unique
	system identifier that is used when referencing the system.
	*/
	i32Status = CsGetSystem(&g_hSystem, 0, 0, 0, 0);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		return (-1);
	}
	/*
	Get System information. The u32Size field must be filled in
	prior to calling CsGetSystemInfo
	*/
	CsSysInfo.u32Size = sizeof(CSSYSTEMINFO);
	i32Status = CsGetSystemInfo(g_hSystem, &CsSysInfo);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		return (-1);
	}

	/*
	Display the system name from the driver
	*/
	_ftprintf(stdout, _T("\nBoard Name: %s"), CsSysInfo.strBoardName);

	/* 
	In this example we're only using 1 trigger source
	*/	
	i32Status = CsAs_ConfigureSystem(g_hSystem, (int)CsSysInfo.u32ChannelCount, 1, (LPCTSTR)szIniFile, &u32Mode);

	if (CS_FAILED(i32Status))
	{
		if (CS_INVALID_FILENAME == i32Status)
		{
			/*
			Display message but continue on using defaults.
			*/
			_ftprintf(stdout, _T("\nCannot find %s - using default parameters."), szIniFile);
		}
		else
		{	
			/*
			Otherwise the call failed.  If the call did fail we should free the CompuScope
			system so it's available for another application
			*/
			DisplayErrorString(i32Status);
			CsFreeSystem(g_hSystem);
			return(-1);
		}
	}
	/*
	If the return value is greater than  1, then either the application, 
	acquisition, some of the Channel and / or some of the Trigger sections
	were missing from the ini file and the default parameters were used. 
	*/
	if (CS_USING_DEFAULT_ACQ_DATA & i32Status)
		_ftprintf(stdout, _T("\nNo ini entry for acquisition. Using defaults."));

	if (CS_USING_DEFAULT_CHANNEL_DATA & i32Status)
		_ftprintf(stdout, _T("\nNo ini entry for one or more Channels. Using defaults for missing items."));

	if (CS_USING_DEFAULT_TRIGGER_DATA & i32Status)
		_ftprintf(stdout, _T("\nNo ini entry for one or more Triggers. Using defaults for missing items."));

	/*
	Load application specific information from the ini file
	*/
	i32Status = LoadStreamConfiguration(szIniFile, &g_StreamConfig);
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
	/*
	Streaming Configuration

	Validate if the board supports hardware streaming  If  it is not supported, 
	we'll exit gracefully.
	*/
	i32Status = InitializeStream(g_hSystem);
	if (CS_FAILED(i32Status))
	{
		// Error string was displayed in InitializeStream
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	/*
	Create events for streams
	*/	
	if (-1 == CreateIPCObjects())
	{
		_ftprintf(stderr, _T("\nUnable to create IPC Objects for synchonization.\n"));
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	g_CsAcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);
	/*
	Get user's acquisition data to use for various parameters for transfer
	*/
	i32Status = CsGet(g_hSystem, CS_ACQUISITION, CS_CURRENT_CONFIGURATION, &g_CsAcqCfg);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	g_i64SegmentPerBuffer = g_StreamConfig.u32BufferSizeBytes / ((g_CsAcqCfg.i64SegmentSize * g_CsAcqCfg.u32SampleBits / 8) + 64);

	/*
	For this sample program only, check for a limited segment size
	*/
	if ( g_CsAcqCfg.i64SegmentSize < 0 || g_CsAcqCfg.i64Depth < 0 )
	{
		_ftprintf (stderr, _T("\n\nThis sample program does not support acquisitions with infinite segment size.\n"));
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	/*
	Commit the values to the driver.  This is where the values get sent to the
	hardware.  Any invalid parameters will be caught here and an error returned.
	*/
	i32Status = CsDo(g_hSystem, ACTION_COMMIT);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(g_hSystem);
		return (-1);
	}
	/*
	Get the timestamp frequency. The frequency will be changed depending on the channel mode (SINGLE, DUAL ...)
	*/
	i32Status = CsGet( g_hSystem, CS_PARAMS, CS_TIMESTAMP_TICKFREQUENCY, &g_i64TsFrequency );
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	/*
	Create threads for Stream. In M/S system, we have to create one thread per card
	*/
	thread = malloc(CsSysInfo.u32BoardCount * sizeof(pthread_t));
	threadFinished = malloc(CsSysInfo.u32BoardCount * sizeof(BOOL));	

	for (n = 1, i = 0; n <= CsSysInfo.u32BoardCount; n++, i++ )
	{
		fflush(stdout);
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		ret = pthread_create(&thread[i], &attr, CardStream, (void*)&n);
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
			/*
			Wait for the g_hThreadReadyForStream to make sure that the thread was successfully created and are ready for stream
			*/
			struct timespec wait_timeout ={0};
			pthread_mutex_lock(&g_hThreadReadyForStream.mutex);
			ComputeTimeout(5000, &wait_timeout);
		
			pthread_cond_timedwait(&g_hThreadReadyForStream.cond, &g_hThreadReadyForStream.mutex, &wait_timeout);
			pthread_mutex_unlock(&g_hThreadReadyForStream.mutex);

			/*
			make sure the CardStream thread start normally
			*/
			if (g_ThreadStartStatus < 0)
			{
				_ftprintf (stderr, _T("Thread initialization error on card %d.\n"), n);
       			g_bStreamError = TRUE;
				CsFreeSystem(g_hSystem);
				free(thread);
				free(threadFinished);
				DestroyIPCObjects();				
				return (-1);
			}
		}
	} /* end for */
	/* register signal interrupt handler for contol_c */
	act.sa_handler = &signal_handler;
	sigaction(SIGINT, &act, NULL);	


	/*
	Start the data acquisition
	*/
	printf ("\nStart streaming. Press CTRL-C to abort\n\n");
	i32Status = CsDo(g_hSystem, ACTION_START);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(g_hSystem);
		return (-1);
	}

	u32TickStart = u32TickNow = GetTickCount();

	/*
	Set the event g_hStreamStarted so that the other threads can start to transfer data
	*/
	pthread_mutex_lock(&g_hStreamStarted.mutex);
	pthread_cond_broadcast(&g_hStreamStarted.cond);
	pthread_mutex_unlock(&g_hStreamStarted.mutex);    	

	/*
	loop until either we've done the number of segments we want, or
	the CRTL-C was pressed to abort. While we loop we transfer into
	pCurrentBuffer and do our analysis on pWorkBuffer
	*/
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

		for (i = 0; i < CsSysInfo.u32BoardCount; i++)
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
		
		/*
		Calcaulate the sum of all data received so far
		*/
		llTotalSamples = 0;
		for (i = 0; i < CsSysInfo.u32BoardCount; i++ )
		{
			llTotalSamples += g_llTotalSamples[i];
		}
		UpdateProgress(u32TickNow-u32TickStart, llTotalSamples * CsSysInfo.u32SampleSize);
		/*
		Check which threads are finished.  If they're all finished set bDone to true
		*/
		for (i = 0; i < CsSysInfo.u32BoardCount; i++)
		{
			if (!threadFinished[i])
			{
				bAllThreadsFinished = FALSE;
				break;
			}
		}
		bDone = bAllThreadsFinished;		
	}

	/*
	Wait for CardStreamThread to finish ..
	*/
	sleep(1);

	//	Abort the current acquisition 
	CsDo(g_hSystem, ACTION_ABORT);

	/*
	Free the CompuScope system and any resources it's been using
	*/
	i32Status = CsFreeSystem(g_hSystem);

	if ( !g_bStreamError && !g_bCtrlAbort )
	{
		/*
		g_u32SegmentCounted[0] is the number of segments processed, in the case of a master / slave system
		it is the number of segments processed by the master board.  The slave boards will have
		processed the same amount.
		*/

		if(g_u32SegmentCounted[0] != g_CsAcqCfg.u32SegmentCount)
		{
			_ftprintf (stdout, _T("\nStream has finished with %u loops insted of %u.\n"), g_u32SegmentCounted[0], g_CsAcqCfg.u32SegmentCount);
		}
		else
		{
			_ftprintf (stdout, _T("\nStream has finished with %u loops.\n"), g_u32SegmentCounted[0]);
		}
	}
	else
	{ 
		if (g_bCtrlAbort)
		{
			_ftprintf (stdout, _T("\nStream aborted by user.\n"));
		}
		else if (g_bStreamError)
		{
			_ftprintf (stdout, _T("\nStream aborted on error.\n"));
		}
	}
	free(thread);
	free(threadFinished);					
	DestroyIPCObjects();
	return 0;
}

/***************************************************************************************************
****************************************************************************************************/

int32 InitializeStream(CSHANDLE g_hSystem)
{
	int32	i32Status = CS_SUCCESS;
	int64	i64ExtendedOptions = 0;	

	CSACQUISITIONCONFIG CsAcqCfg = {0};

	CsAcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);

	/*
	Get user's acquisition Data
	*/
	i32Status = CsGet(g_hSystem, CS_ACQUISITION, CS_CURRENT_CONFIGURATION, &CsAcqCfg);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		return (i32Status);
	}
	/*
	Check if selected system supports Expert Stream
	And set the correct image to be used.
	*/
	CsGet(g_hSystem, CS_PARAMS, CS_EXTENDED_BOARD_OPTIONS, &i64ExtendedOptions);

	if (i64ExtendedOptions & CS_BBOPTIONS_STREAM)
	{
		_ftprintf(stdout, _T("\nSelecting Expert Stream from image 1."));
		CsAcqCfg.u32Mode |= CS_MODE_USER1;
	}
	else if ((i64ExtendedOptions >> 32) & CS_BBOPTIONS_STREAM)
	{
		_ftprintf(stdout, _T("\nSelecting Expert Stream from image 2."));
		CsAcqCfg.u32Mode |= CS_MODE_USER2;
	}
	else
	{
		_ftprintf(stdout, _T("\nCurrent system does not support Expert Stream."));
		_ftprintf(stdout, _T("\nApplication terminated."));
		return CS_MISC_ERROR;
	}

	/*
	Sets the Acquisition values down the driver, without any validation, 
	for the Commit step which will validate system configuration.
	*/

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
	uInt32	h = 0;
	uInt32	m = 0;
	uInt32	s = 0;
	double	dRate;
	double  dTotal;

	if ( u32Elapsed > 0 )
	{
		dRate = (llTotaBytes / 1000000.0) / (u32Elapsed / 1000.0);

		if (u32Elapsed >= 1000)
		{
			if ((s = u32Elapsed / 1000) >= 60)	// Seconds
			{
				if ((m = s / 60) >= 60)			// Minutes
				{
					if ((h = m / 60) > 0)		// Hours
						m %= 60;
				}

				s %= 60;
			}
		}
		dTotal = 1.0*llTotaBytes/1000000.0;		// Mega samples
		_ftprintf (stdout, "\rTotal data: %0.2f MB, Rate: %6.2f MB/s, Elapsed Time: %u:%02u:%02u", 
                                 dTotal, dRate, h, m, s);
		fflush(stdout);
	}
}

/***************************************************************************************************
****************************************************************************************************/

void UpdateResultFile(FILE** pFile, uInt16 nCardIndex, uInt32* pu32NbrFiles)
{
	TCHAR strFileName[MAX_PATH];
	++(*pu32NbrFiles);

	if(*pFile != NULL)
		fclose(*pFile);

	sprintf(strFileName, "%s_%d_%d.txt", g_StreamConfig.strResultFile, nCardIndex, *pu32NbrFiles);
	*pFile = _tfopen(strFileName, "w");
	_ftprintf(*pFile, _T("Segment start with segment offset: %d x 25000 segments per file\n\nSegment\t\tTimestamp (H/L)\t\tDelta(ms)\tDelta(Samples)\n"), ((*pu32NbrFiles)-1));
}

/***************************************************************************************************
****************************************************************************************************/

void SaveResults(PSTREAM_INFO streamInfo, 
		 uInt32 u32Count, 
		 uInt32* pu32SegmentFileCount, 
		 uInt32* pu32nbFiles, 
		 FILE **pFile, 
		 uInt16 u16CardIndex, 
		 uInt32* pu32TotalSegmentCount)
{
	int32	i32TsLow;
	int32	i32TsHigh;
	int64	i64Delta;
	double	fDeltaTime;
	uInt32	i = 0;

	for ( ; i < u32Count; i++, streamInfo->u32Segment++)
	{
		/*
		If the file is full, open a new file
		*/
		if((*pu32SegmentFileCount)+1 > MAX_SEGMENTS_COUNT)
		{
			UpdateResultFile(pFile, u16CardIndex, pu32nbFiles);
			*pu32SegmentFileCount = 0;
		}

		i32TsLow	= (int32) streamInfo->pi64TimeStamp[i];
		i32TsHigh	= (int32) (streamInfo->pi64TimeStamp[i]>>32);
		streamInfo->i64DeltaTime = streamInfo->pi64TimeStamp[i] - streamInfo->i64LastTs;
		fDeltaTime	= 1000.0 * streamInfo->i64DeltaTime / g_i64TsFrequency;
		i64Delta    = g_CsAcqCfg.i64SampleRate * fDeltaTime / 1000;

		if ( 1 == streamInfo->u32Segment )
			_ftprintf(*pFile, "%d\t\t0x%08x %08x\r\n", (*pu32TotalSegmentCount)++, i32TsHigh, i32TsLow );
		else
			_ftprintf(*pFile, "%d\t\t0x%08x %08x\t%.05f\t\t%08lld \r\n", (*pu32TotalSegmentCount)++  , i32TsHigh, i32TsLow, fDeltaTime, (long long int)i64Delta );
		
		streamInfo->i64LastTs = streamInfo->pi64TimeStamp[i];
		(*pu32SegmentFileCount)++;
	}
}

/***************************************************************************************************
****************************************************************************************************/

int32 LoadStreamConfiguration(LPCTSTR szIniFile, PCsStreamConfig pConfig)
{
	TCHAR	szDefault[MAX_PATH];
	TCHAR	szString[MAX_PATH];
	TCHAR	szFilePath[MAX_PATH];
	int		nDummy;

	CsStreamConfig CsStmCfg;	
	/*
	Set defaults in case we can't read the ini file
	*/
	CsStmCfg.u32BufferSizeBytes = STREAM_BUFFERSIZE;
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
	CsStmCfg.bDoAnalysis = (0 != GetPrivateProfileInt(STM_SECTION, _T("DoAnalysis"), nDummy, szFilePath));

	nDummy = CsStmCfg.u32TransferTimeout;
	CsStmCfg.u32TransferTimeout = GetPrivateProfileInt(STM_SECTION, _T("TimeoutOnTransfer"), nDummy, szFilePath);

	nDummy = CsStmCfg.u32BufferSizeBytes;
	CsStmCfg.u32BufferSizeBytes =  GetPrivateProfileInt(STM_SECTION, _T("BufferSize"), nDummy, szFilePath);

	_stprintf(szDefault, _T("%s"), CsStmCfg.strResultFile);
	GetPrivateProfileString(STM_SECTION, _T("ResultsFile"), szDefault, szString, MAX_PATH, szFilePath);
	_tcscpy(CsStmCfg.strResultFile, szString);

	*pConfig = CsStmCfg;
	return (CS_SUCCESS);
}

/***************************************************************************************************
****************************************************************************************************/

void *CardStream( void *CardIndex )
{
	void*		pBuffer1		= NULL;
	void*		pBuffer2		= NULL;
	void*		pCurrentBuffer	= NULL;

	BOOL		bDone					= FALSE;
	BOOL		bStreamCompletedSuccess = FALSE;

	FILE*		pFile = 0;

	uInt8		u8EndOfData = 0;

	uInt16		nCardIndex = *((uInt16 *) CardIndex);

	uInt32		u32LoopCount			= 0;	
	uInt32		u32TransferSize_Samples	= 0;	
	uInt32		u32ProcessedSegments	= 0;
	uInt32		u32SegmentCount			= 0;
	uInt32		u32NbrFiles				= 0;
	uInt32		u32ErrorFlag			= 0;
	uInt32		u32ActualLength			= 0;
	uInt32		u32TailLeftOver			= 0;
	int32		i32Status;
	int64		i64SegmentSizeBytes 	= 0;	
	int64		i64DataInSegment_Samples= 0;
	uInt32		u32SegmentTail_Bytes	= 0;
	uInt32		u32SegmentFileCount		= 1;

	STREAM_INFO	streamInfo = {0};

	/* 
	In Streaming, all channels are transferred within the same buffer. Calculate the size of data in a segment
	(Segment size * number of channels)
	*/
	i64DataInSegment_Samples = (g_CsAcqCfg.i64SegmentSize*(g_CsAcqCfg.u32Mode & CS_MASKED_MODE));

	/* 
	In Streaming, some hardware related information are placed at the end of each segment. These samples contain also timestamp information for the 
	segemnt. The number of extra bytes may be different depending on CompuScope card model.
	*/
	i32Status = CsGet(g_hSystem, 0, CS_SEGMENTTAIL_SIZE_BYTES, &u32SegmentTail_Bytes);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		g_bStreamError = TRUE;
		return NULL;
	}

	/*
	We are gonna need the size of the segment with his tail for the analysis function.

	To simplify the algorithm in the analysis function, the buffer size used for transfer should be multiple or a fraction of the size of segment
	Resize the buffer size.
	*/
	i64SegmentSizeBytes = i64DataInSegment_Samples*g_CsAcqCfg.u32SampleSize;

	u32TransferSize_Samples = g_StreamConfig.u32BufferSizeBytes/g_CsAcqCfg.u32SampleSize;
	_ftprintf (stderr, _T("\n(Actual buffer size used for data streaming = %u Bytes)\n"), g_StreamConfig.u32BufferSizeBytes );

	/*
	We adjust the size of the memory allocated for the timeStamp buffer if we can fit more segment + tail in the buffer than the SegmentCount
	*/
	if(g_i64SegmentPerBuffer > MAX_SEGMENTS_COUNT)
	{
		streamInfo.pi64TimeStamp = (int64 *)calloc(g_i64SegmentPerBuffer + 1, sizeof(int64));
	}
	else
	{
		streamInfo.pi64TimeStamp = (int64 *)calloc(MIN(g_CsAcqCfg.u32SegmentCount,MAX_SEGMENTS_COUNT), sizeof(int64));
	}
	
	if (NULL == streamInfo.pi64TimeStamp)
	{
		_ftprintf (stderr, _T("\nUnable to allocate memory to hold analysis results.\n"));
		g_bStreamError = TRUE;
		return NULL;
	}

	/*
	We need to allocate a buffer for transferring the data. The buffer must be allocated by a call to CsStmAllocateBuffer.
	This routine will allocate a buffer suitable for streaming.  In this program we're allocating 2 streaming buffers
	so we can transfer to one while doing analysis on the other.
	*/
	i32Status = CsStmAllocateBuffer(g_hSystem, nCardIndex, g_StreamConfig.u32BufferSizeBytes, &pBuffer1);
	if (CS_FAILED(i32Status))
	{
		_ftprintf (stderr, _T("\nUnable to allocate memory for stream buffer 1.\n"));
		free(streamInfo.pi64TimeStamp);
		g_bStreamError = TRUE;
		g_ThreadStartStatus = -1;
		return NULL;
	}

	i32Status = CsStmAllocateBuffer(g_hSystem, nCardIndex, g_StreamConfig.u32BufferSizeBytes, &pBuffer2);
	if (CS_FAILED(i32Status))
	{
		_ftprintf (stderr, _T("\nUnable to allocate memory for stream buffer 2.\n"));
		CsStmFreeBuffer(g_hSystem, nCardIndex, pBuffer1);
		free(streamInfo.pi64TimeStamp);
		g_bStreamError = TRUE;
		g_ThreadStartStatus = -1;		
		return NULL;
	}

	/*
	So far so good ...
	Let the main thread know that this thread is ready for stream
	*/

	pthread_mutex_lock(&g_hThreadReadyForStream.mutex);
	pthread_cond_signal(&g_hThreadReadyForStream.cond);
	pthread_mutex_unlock(&g_hThreadReadyForStream.mutex);    	

	/*
	Wait for the start acquisition event from the main thread
	*/
	struct timespec wait_timeout ={0};
	memset(&wait_timeout, 0, sizeof(struct timespec));
	ComputeTimeout(5000, &wait_timeout);

	pthread_mutex_lock(&g_hStreamStarted.mutex);
	pthread_cond_wait(&g_hStreamStarted.cond, &g_hStreamStarted.mutex);
	pthread_mutex_unlock(&g_hStreamStarted.mutex);
	
	if (g_bCtrlAbort || g_bStreamError)
	{
		/*
		Aborted from user or error
		*/
		CsStmFreeBuffer(g_hSystem, nCardIndex, pBuffer1);
		CsStmFreeBuffer(g_hSystem, nCardIndex, pBuffer2);
		free(streamInfo.pi64TimeStamp);
		return NULL;
	}

	/*
	Init first result file
	*/
	UpdateResultFile(&pFile, nCardIndex, &u32NbrFiles);

	/*
	Steam acqusition has started.

	Loop until either we've done the number of segments we want, or
	the ESC key was pressed to abort. While we loop we transfer into
	pCurrentBuffer and do our analysis on pWorkBuffer
	*/

	if (NULL == streamInfo.pi64TimeStamp )
	{
		_ftprintf (stderr, _T("\nUnable to allocate memory to hold analysis results.\n"));
		g_bStreamError = TRUE;
		return NULL;
	}

	/*
	Variable declaration for streaming
	*/

	streamInfo.u32BufferSize	= g_StreamConfig.u32BufferSizeBytes;
	streamInfo.i64SegmentSize	= i64SegmentSizeBytes;
	streamInfo.u32TailSize		= u32SegmentTail_Bytes;

	streamInfo.i64ByteToEndSegment	= i64SegmentSizeBytes;
	streamInfo.u32ByteToEndTail	= u32SegmentTail_Bytes;
	streamInfo.u32LeftOverSize	= u32TailLeftOver;

	streamInfo.i64LastTs		= 0;
	streamInfo.u32Segment		= 1;

	streamInfo.u32SegmentCountDown  = g_CsAcqCfg.u32SegmentCount;

	streamInfo.bSplitTail		= FALSE;	/* Default value */

	while( ! ( bDone || bStreamCompletedSuccess) )
	{
		/*
		Check if user has aborted or an error has occured
		*/
		if (g_bCtrlAbort || g_bStreamError)
		{
			break;
		}
		/*
		Determine where new data transfer data will go. We alternate
		between our 2 DMA buffers
		*/
		if (u32LoopCount & 1)
		{
			pCurrentBuffer = pBuffer2;
		}
		else
		{
			pCurrentBuffer = pBuffer1;
		}

		i32Status = CsStmTransferToBuffer(g_hSystem, nCardIndex, pCurrentBuffer, u32TransferSize_Samples);
		if (CS_FAILED(i32Status))
		{
			g_bStreamError = TRUE;
			DisplayErrorString(i32Status);
			break;
		}

		if ( g_StreamConfig.bDoAnalysis )
		{
			if ( NULL != streamInfo.pWorkBuffer )
			{
				/*
				Do analysis on the previous buffer
				*/

				u32ProcessedSegments = AnalysisFunc(&streamInfo);
				/*
				Save data in the file
				*/
				if ( u32ProcessedSegments > 0 )
				{
					SaveResults(&streamInfo, u32ProcessedSegments, &u32SegmentCount, &u32NbrFiles, &pFile, nCardIndex, &u32SegmentFileCount);
				}
				g_u32SegmentCounted[nCardIndex-1] += u32ProcessedSegments;
				/*
				Note: g_i64TsFrequency holds the frequency of the timestamp clock. This is needed to convert the timestamp
				counter values into actual time values. If you need it, you should make not of it or save it to a file.
				*/
			}
		}
		/*
		Wait for the DMA transfer on the current buffer to complete so we can loop 
		back around to start a new one. Calling thread will sleep until
		the transfer completes
		*/
			
		i32Status = CsStmGetTransferStatus( g_hSystem, nCardIndex, g_StreamConfig.u32TransferTimeout, &u32ErrorFlag, &u32ActualLength, &u8EndOfData );
		if ( CS_SUCCEEDED(i32Status) )
		{
			/*
			Calculate the total of data for this card
			*/
			g_llTotalSamples[nCardIndex-1] += u32TransferSize_Samples;
			bStreamCompletedSuccess = (0 != u8EndOfData);
		}
		else
		{
			g_bStreamError = TRUE;
			bDone = TRUE;
			if (CS_STM_TRANSFER_TIMEOUT == i32Status)
			{
				/* 
				Timeout on CsStmGetTransferStatus().
				Data transfer has not yet completed. We can repeat calling CsStmGetTransferStatus() until we get the status success (ie data transfer is completed)
				In this sample program, we consider the timeout as an error
				*/
				DisplayErrorString(i32Status);
			}
			else
			{
				DisplayErrorString(i32Status);				
			}
			break;
		}

		streamInfo.pWorkBuffer = pCurrentBuffer;

		u32LoopCount++;

	}
	if (g_StreamConfig.bDoAnalysis)
	{
		/*
		Do analysis on the last buffer
		*/
		u32ProcessedSegments = AnalysisFunc(&streamInfo);
		SaveResults(&streamInfo, u32ProcessedSegments,  &u32SegmentCount, &u32NbrFiles, &pFile, nCardIndex, &u32SegmentFileCount);
		g_u32SegmentCounted[nCardIndex-1] += u32ProcessedSegments;
		fclose(pFile);
	}

	free(streamInfo.pi64TimeStamp);
	CsStmFreeBuffer(g_hSystem, 0, pBuffer1);
	CsStmFreeBuffer(g_hSystem, 0, pBuffer2);

	return NULL;

}
/***************************************************************************************************
****************************************************************************************************/
uInt32 AnalysisFunc(PSTREAM_INFO pStreamInfo)
{
	/*
	The tail of every segment is compose of 8 identicals timeStamps. This function Analyse only the first ones
	*/

	uInt8	*pu8Buffer	= (uInt8 *)pStreamInfo->pWorkBuffer;
	int64	*pi64Ts		= NULL;
	uInt32	i		= 0;
	uInt32	ByteToEndBuffer	= g_StreamConfig.u32BufferSizeBytes;
	
	/*
	If tail was split in last buffer
	*/
	if(pStreamInfo->bSplitTail)
	{
		/*
		Save leftOver from last buffer
		*/
		pStreamInfo->u32LeftOverSize = pStreamInfo->u32ByteToEndTail;
		ByteToEndBuffer				-= pStreamInfo->u32LeftOverSize;

		/*
		Goto next segment
		*/
		pu8Buffer					 += pStreamInfo->u32ByteToEndTail;
		pStreamInfo->u32ByteToEndTail = pStreamInfo->u32TailSize;

		/*
		Reset flag for tail split
		*/
		pStreamInfo->bSplitTail = FALSE;
	}

	/*
	Case Buffer < Segment
	*/
	if(ByteToEndBuffer < (pStreamInfo->i64SegmentSize + pStreamInfo->u32TailSize))
	{
		for(;;)
		{
			/*
			if there is a tail in the present segment
			*/
			if(ByteToEndBuffer >= pStreamInfo->i64ByteToEndSegment + pStreamInfo->u32ByteToEndTail)
			{
				/*
				Goto Tail
				*/
				pu8Buffer		+= pStreamInfo->i64ByteToEndSegment;
				ByteToEndBuffer -= pStreamInfo->i64ByteToEndSegment;

				/*
				Process Tail
				*/
				pi64Ts							  = (int64*) pu8Buffer;
				pStreamInfo->pi64TimeStamp[i++]	  = (*pi64Ts & 0xFFFFFFFFFFFF);
				pStreamInfo->u32SegmentCountDown -= 1;
				ByteToEndBuffer					 -= pStreamInfo->u32ByteToEndTail;

				/*
				Reset counts
				*/
				pStreamInfo->i64ByteToEndSegment = pStreamInfo->i64SegmentSize;
				pStreamInfo->u32ByteToEndTail	 = pStreamInfo->u32TailSize;

				if(!pStreamInfo->u32SegmentCountDown)
				{
					return i;
				}

			}
			
			/*
			If there is only part of a tail in the present segment
			*/
			else if (ByteToEndBuffer > pStreamInfo->i64ByteToEndSegment)
			{
				/*
				Goto Tail
				*/
				pu8Buffer		+= pStreamInfo->i64ByteToEndSegment;
				ByteToEndBuffer -= pStreamInfo->i64ByteToEndSegment;

				/*
				Process part tail
				*/
				pi64Ts = (int64*) pu8Buffer;
				pStreamInfo->pi64TimeStamp[i++]	  = (*pi64Ts & 0xFFFFFFFFFFFF);
				pStreamInfo->u32SegmentCountDown -= 1;
				pStreamInfo->u32ByteToEndTail	 -= ByteToEndBuffer;

				/*
				Tail is split between this buffer and next one
				*/
				pStreamInfo->bSplitTail = TRUE;

				pStreamInfo->i64ByteToEndSegment = pStreamInfo->i64SegmentSize;

				if(!pStreamInfo->u32SegmentCountDown)
				{
					return i;
				}
				break;
			}
			
			pStreamInfo->i64ByteToEndSegment -= ByteToEndBuffer;
			break;
		}
	}

	/*
	Case Buffer >= Segment
	*/
	else
	{
		for(;;)
		{
			/*
			Goto tail
			*/
			pu8Buffer		+= pStreamInfo->i64ByteToEndSegment;
			ByteToEndBuffer -= pStreamInfo->i64ByteToEndSegment;

			/*
			Process Tail
			*/
			pi64Ts							  = (int64*) pu8Buffer;
			pStreamInfo->pi64TimeStamp[i++]	  = (*pi64Ts & 0xFFFFFFFFFFFF);
			pStreamInfo->u32SegmentCountDown -= 1;

			/*
			Goto next segment
			*/
			pu8Buffer		 += pStreamInfo->u32ByteToEndTail;
			ByteToEndBuffer  -= pStreamInfo->u32ByteToEndTail;

			/*
			Reset counts
			*/
			pStreamInfo->i64ByteToEndSegment = pStreamInfo->i64SegmentSize;
			pStreamInfo->u32ByteToEndTail	 = pStreamInfo->u32TailSize;

			/*
			If we reach the amount of segment to analyse
			*/
			if (!pStreamInfo->u32SegmentCountDown)
			{
				return i;
			}


			/*
			No more tail in buffer
			*/
			if (ByteToEndBuffer <= pStreamInfo->i64ByteToEndSegment)
			{
				pStreamInfo->i64ByteToEndSegment -= ByteToEndBuffer;
				pStreamInfo->u32LeftOverSize = 0;
				break;
			}

			/*
			No more full tail in buffer
			*/
			else
			{ 
				if (ByteToEndBuffer < (pStreamInfo->i64ByteToEndSegment + pStreamInfo->u32ByteToEndTail))
				{
					/*
					goto Tail
					*/
					pu8Buffer		+= pStreamInfo->i64ByteToEndSegment;
					ByteToEndBuffer -= pStreamInfo->i64ByteToEndSegment;

					/*
					Process part of tail
					*/
					pi64Ts = (int64*) pu8Buffer;
					pStreamInfo->pi64TimeStamp[i++]	  = (*pi64Ts & 0xFFFFFFFFFFFF);
					pStreamInfo->u32SegmentCountDown -= 1;
					pStreamInfo->u32ByteToEndTail	 -= ByteToEndBuffer - pStreamInfo->u32LeftOverSize;
					
					/*
					Tail is split between this buffer and next one
					*/
					pStreamInfo->bSplitTail = TRUE;

					if (!pStreamInfo->u32SegmentCountDown)
					{
						return i;
					}

					break;
				}
			}
		}
	}
	return i;
}
/***************************************************************************************************
****************************************************************************************************/
void signal_handler(int sig)
{
	/*
	Handles Ctrl+C in terminal 
	*/
	g_bCtrlAbort = TRUE;
}
/***************************************************************************************************
****************************************************************************************************/
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
/***************************************************************************************************
****************************************************************************************************/
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
/***************************************************************************************************
****************************************************************************************************/
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
