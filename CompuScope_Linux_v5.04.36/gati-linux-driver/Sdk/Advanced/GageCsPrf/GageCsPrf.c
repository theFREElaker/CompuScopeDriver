// GageCsPrf.c : Defines the entry point for the console application.
//

#include "GageCsPrf.h"

int _tmain()
{
	int32						i32Status = CS_SUCCESS;
	uInt32						k;
	int							i;
	void*						pBuffer = NULL;
	uInt32						u32Mode;
	CSHANDLE					hSystem = 0;
	IN_PARAMS_TRANSFERDATA		InData = {0};
	OUT_PARAMS_TRANSFERDATA		OutData = {0};
	CSSYSTEMINFO				CsSysInfo = {0};
	CSAPPLICATIONDATA			CsAppData = {0};
	LPCTSTR						szIniFile = _T("Prf.ini");
	CSACQUISITIONCONFIG			AcqConfig;
	CSPRFCONFIG					PfrConfig;

	LONGLONG*                   pllStartTime;
	LONGLONG*                   pllBusyTime;
	LONGLONG*                   pllXferTime;
	LONGLONG*                   pllTotalTime;
#ifdef _WIN32
	HANDLE						hTransferEvent = NULL;
#else
	sem_t						hTransferEvent ;
#endif	

	LARGE_INTEGER				liStart, liEnd;
	LARGE_INTEGER				liBegin, liFinish;
	uInt32                      uIx, j;
	int64						i64Depth = 0;

	int							nDepthCount = 1;
	uInt32                      u32SegmentCount = 1;
	uInt32						u32ChannelIndexIncrement;
	int64						i64BufferSize;
	int32						i32Resolution;

	FILE* pFile = 0;


	i32Status = CsInitialize();

	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		WaitForAcknowlegment();
		return (-1);
	}
	i32Status = CsGetSystem(&hSystem, 0, 0, 0, 0);

	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		WaitForAcknowlegment();
		return (-1);
	}
	CsSysInfo.u32Size = sizeof(CSSYSTEMINFO);
	i32Status = CsGetSystemInfo(hSystem, &CsSysInfo);

	_ftprintf(stdout, _T("\nBoard Name: %s"), CsSysInfo.strBoardName);

	i32Resolution = GetTriggerResolution(hSystem);

/* 
	In this example we're only using 1 trigger source
*/	
	if (!CsAs_ConfigureSystem(hSystem, (int)CsSysInfo.u32ChannelCount, 1, (LPCTSTR)szIniFile, &u32Mode))
	{
		CsFreeSystem(hSystem);
		WaitForAcknowlegment();
		return(-1);
	}

	i32Status = LoadPrfConfiguration(szIniFile, i32Resolution, &PfrConfig);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		WaitForAcknowlegment();
		return FALSE;
	}

	if (CS_USING_DEFAULTS == i32Status)
		_ftprintf(stdout, _T("\nNo ini entry for Prf configuration. Using defaults."));

	if ( PfrConfig.bCsTransferEx )
	{
		i32Status = CsGetSystemCaps( hSystem, CAPS_TRANSFER_EX, NULL, NULL );
		if ( CS_FAILED(i32Status) )
		{
			_ftprintf (stderr, _T("\nThe current system does not support CsTransferEx() and multiple segments transfer.\n"));
			CsFreeSystem(hSystem);
			return (-1);
		}
	}

	nDepthCount = 0;
	i64Depth = PfrConfig.i64FinishDepth;
	while(i64Depth >= PfrConfig.i64StartDepth )
	{
		nDepthCount ++;
		i64Depth >>= 1;
	}

	if(0 == nDepthCount )
	{
		_ftprintf(stderr, _T("\nNo Depth values to benchmark.\n"));
		WaitForAcknowlegment();
		CsFreeSystem(hSystem);
		return FALSE;
	}

	pllStartTime = (LONGLONG*)VirtualAlloc(NULL, nDepthCount*sizeof(LONGLONG), MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pllStartTime)
	{
		_ftprintf (stderr, _T("\nUnable to allocate memory\n"));
		CsFreeSystem(hSystem);
		WaitForAcknowlegment();
		return (-1);
	}

	pllBusyTime  = (LONGLONG*)VirtualAlloc(NULL, nDepthCount*sizeof(LONGLONG), MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pllBusyTime)
	{
		_ftprintf (stderr, _T("\nUnable to allocate memory\n"));
		CsFreeSystem(hSystem);
		WaitForAcknowlegment();
		return (-1);
	}

	pllXferTime  = (LONGLONG*)VirtualAlloc(NULL, nDepthCount*sizeof(LONGLONG), MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pllXferTime)
	{
		_ftprintf (stderr, _T("\nUnable to allocate memory\n"));
		CsFreeSystem(hSystem);
		WaitForAcknowlegment();
		return (-1);
	}

	pllTotalTime  = (LONGLONG*)VirtualAlloc(NULL, nDepthCount*sizeof(LONGLONG), MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pllTotalTime)
	{
		_ftprintf (stderr, _T("\nUnable to allocate memory\n"));
		CsFreeSystem(hSystem);
		WaitForAcknowlegment();
		return (-1);
	}

	// We need to create the event in order to be notified of the completion
#ifdef _WIN32	
	hTransferEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
#else
	sem_init(&hTransferEvent, 0, 0);
#endif	

	AcqConfig.u32Size = sizeof(CSACQUISITIONCONFIG);
	i32Status = CsGet( hSystem, CS_ACQUISITION, CS_CURRENT_CONFIGURATION, &AcqConfig);
	if (CS_FAILED(i32Status))
	{
		HandleFailedStatus(hSystem, i32Status, &hTransferEvent, TRUE );		
		return (-1);
	}

	u32SegmentCount = AcqConfig.u32SegmentCount;
	u32ChannelIndexIncrement = CsAs_CalculateChannelIndexIncrement(&AcqConfig, &CsSysInfo );
		
	CsAppData.i64TransferLength = AcqConfig.i64Depth = AcqConfig.i64SegmentSize = PfrConfig.i64FinishDepth;
	CsAppData.u32TransferSegmentCount = AcqConfig.u32SegmentCount;

	// Recalculate the size of the buffer for waveform data if necessary
	i64BufferSize = CsAppData.i64TransferLength * CsSysInfo.u32SampleSize;
	if ( PfrConfig.bCsTransferEx )
	{	
		i64BufferSize *= CsAppData.u32TransferSegmentCount;
		if ( PfrConfig.bInterleaved )
			i64BufferSize  *= (CsSysInfo.u32ChannelCount/CsSysInfo.u32BoardCount);
	}

	// Allocate buffer for waveform data
	pBuffer  = VirtualAlloc(NULL, (size_t)i64BufferSize, MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pBuffer)
	{
		_ftprintf (stderr, _T("\nUnable to allocate memory for waveform data\n"));
		HandleFailedStatus(hSystem, i32Status, &hTransferEvent, FALSE );
		return (-1);
	}

	_ftprintf(stdout, _T("\nStart PRF test. It might take sometime to complete ...\n"));

	for( i = 0; i < nDepthCount; i++ )
	{
		i32Status = CsSet( hSystem, CS_ACQUISITION, &AcqConfig);
		if (CS_FAILED(i32Status))
		{
			HandleFailedStatus(hSystem, i32Status, &hTransferEvent, TRUE );
			return (-1);
		}

		i32Status = CsDo(hSystem, ACTION_COMMIT);
		if (CS_FAILED(i32Status))
		{
			HandleFailedStatus(hSystem, i32Status, &hTransferEvent, TRUE );
			return (-1);
		}

		pllStartTime[i] = pllBusyTime[i] = pllXferTime[i] = 0;

		InData.u32Segment = 1;
		InData.u32Mode = TxMODE_DEFAULT;
		InData.i64StartAddress = 0;
		InData.i64Length =  CsAppData.i64TransferLength;
		InData.pDataBuffer = pBuffer;

		i32Status = CsDo(hSystem, ACTION_START);
		if (CS_FAILED(i32Status))
		{
			HandleFailedStatus(hSystem, i32Status, &hTransferEvent, TRUE );
			return (-1);
		}

		if (!DataCaptureComplete(hSystem))
		{
			HandleFailedStatus(hSystem, i32Status, &hTransferEvent, FALSE );
			return (-1);
		}

		QueryPerformanceCounter(&liBegin);

		for(uIx = 0; uIx < PfrConfig.u32LoopCount; uIx++)
		{
			QueryPerformanceCounter(&liStart);

			i32Status = CsDo(hSystem, ACTION_START);

			QueryPerformanceCounter(&liEnd);

			if (CS_FAILED(i32Status))
			{
				HandleFailedStatus(hSystem, i32Status, &hTransferEvent, TRUE );		
				return (-1);
			}
			pllStartTime[i] += liEnd.QuadPart - liStart.QuadPart;

			QueryPerformanceCounter(&liStart);
			if (!DataCaptureComplete(hSystem))
			{
				HandleFailedStatus(hSystem, i32Status, &hTransferEvent, FALSE );		
				return (-1);
			}
			QueryPerformanceCounter(&liEnd);

			pllBusyTime[i] += liEnd.QuadPart - liStart.QuadPart;

			QueryPerformanceCounter(&liStart);

			if ( PfrConfig.bCsTransferEx )
			{
				IN_PARAMS_TRANSFERDATA_EX		InDataEx = {0};
				OUT_PARAMS_TRANSFERDATA_EX		OutDataEx = {0};
				uInt16							u16ChanPerCard = (uInt16) (CsSysInfo.u32ChannelCount/CsSysInfo.u32BoardCount);

				InDataEx.u16Channel		 = 1;
				InDataEx.i64Length		 = CsAppData.i64TransferLength;
				InDataEx.u32StartSegment = 1;
				InDataEx.u32SegmentCount = u32SegmentCount;
				InDataEx.pDataBuffer	 = pBuffer;
				InDataEx.i64BufferLength = i64BufferSize;
	
				if ( PfrConfig.bInterleaved )
				{
					InDataEx.u32Mode = TxMODE_DATA_INTERLEAVED;

					for	(k = 0; k < CsSysInfo.u32BoardCount; k++)
					{
						InData.u16Channel = (uInt16) (1 + k*u16ChanPerCard);
						i32Status = CsTransferEx(hSystem, &InDataEx, &OutDataEx);
					}
				}
				else
				{
					for	(k = 1; k <= CsSysInfo.u32ChannelCount; k += u32ChannelIndexIncrement)
					{
						InData.u16Channel = (uInt16)k;
						i32Status = CsTransferEx(hSystem, &InDataEx, &OutDataEx);
					}
				}
			}
			else
			{
				for	(k = 1; k <= CsSysInfo.u32ChannelCount; k += u32ChannelIndexIncrement)
				{
					for( j = 1; j <= u32SegmentCount; j++ )
					{
						InData.u16Channel = (uInt16)k;
						InData.u32Segment = (uInt32)j;

						if (PfrConfig.bUseAsTransfer)
						{
							int32	i32Token = 0;

							InData.hNotifyEvent = &hTransferEvent;
							i32Status = CsTransferAS(hSystem, &InData, &OutData, &i32Token);
#ifdef _WIN32							
							WaitForSingleObject(hTransferEvent, INFINITE);
#else
                                                        // in window, the lower layer call SetEvent.
                                                        // so what should be used in Linux. TBD.
                                                        sem_wait(&hTransferEvent);
#endif

							// i32Status = CsGetTransferASResult(hSystem, i32Token, &i64Bytes);
							i32Status = CS_SUCCESS;
						}
						else
						{
							i32Status = CsTransfer(hSystem, &InData, &OutData);
						}

						if (CS_FAILED(i32Status))
						{
							HandleFailedStatus(hSystem, i32Status, &hTransferEvent, FALSE );		
							VirtualFree(pBuffer, 0, MEM_RELEASE);
							return (-1);
						}
					}
				}
			}

			QueryPerformanceCounter(&liEnd);
#ifdef _DEBUG
			_ftprintf(stdout, _T("\t\tCurrent depth: %09ld    Loop count %5d\r"), AcqConfig.i64Depth, uIx+1);
#endif
			pllXferTime[i] += liEnd.QuadPart - liStart.QuadPart;
		}
		QueryPerformanceCounter(&liFinish);

		pllTotalTime[i] = liFinish.QuadPart - liBegin.QuadPart;

		AcqConfig.i64Depth >>= 1;
		AcqConfig.i64SegmentSize = CsAppData.i64TransferLength = AcqConfig.i64Depth;
	} // End nDepthCount

	QueryPerformanceFrequency(&liStart);

	i64Depth = PfrConfig.i64FinishDepth;

	if (!strncmp(PfrConfig.strResultFile,"stdout", 6))
		pFile = stdout;
	else	
	pFile = _tfopen(PfrConfig.strResultFile, "w");

	_ftprintf(pFile, _T("PRF results for  %s %d MS\n"), CsSysInfo.strBoardName, (int)(CsSysInfo.i64MaxMemory >> 20));
	_ftprintf(pFile, _T("Acquisition mode: %04x hex \tChannel count: %d\n"), AcqConfig.u32Mode, CsSysInfo.u32ChannelCount / u32ChannelIndexIncrement );
	_ftprintf(pFile, _T("SampleRate:  %09ld \t\tSegment count: %d\n"), AcqConfig.i64SampleRate, u32SegmentCount );
	_ftprintf(pFile, _T("Loop count: %d\n"), PfrConfig.u32LoopCount);
	_ftprintf(pFile, _T("Transfer function: "));

	if ( PfrConfig.bCsTransferEx )
	{
		 if ( PfrConfig.bInterleaved )
			 _ftprintf(pFile, _T("CsTransferEx | Interleaved\n"));
		 else
			 _ftprintf(pFile, _T("CsTransferEx\n"));
	}
	else
	{
		if ( PfrConfig.bUseAsTransfer )
			_ftprintf(pFile, _T("CsTransferAS\n"));
		else
			_ftprintf(pFile, _T("CsTransfer\n"));
	}

	_ftprintf(pFile, _T("\n   Depth   \t Total time,ms\t   PRF, Hz\tStart time, ms\tBusy time, ms\tTransfer time, ms\tRate MBytes/s\n"));
	
	for(i = 0; i < nDepthCount; i++)
	{
		double dStartTime, dBusyTime, dXferTime, dTotalTime;
		double dDepthFactor = (double)(CsSysInfo.u32ChannelCount / u32ChannelIndexIncrement) * (double)u32SegmentCount * (double)CsSysInfo.u32SampleSize / 1048.576;

		dStartTime = (double)pllStartTime[i];
		dStartTime /= (double)liStart.QuadPart;
		dStartTime /= (double)PfrConfig.u32LoopCount;
		dStartTime *= 1.E3;

		dBusyTime = (double)pllBusyTime[i];
		dBusyTime /= (double)liStart.QuadPart;
		dBusyTime /= (double)PfrConfig.u32LoopCount;
		dBusyTime *= 1.E3;

		dXferTime = (double)pllXferTime[i];
		dXferTime /= (double)liStart.QuadPart;
		dXferTime /= (double)PfrConfig.u32LoopCount;
		dXferTime *= 1.E3;

		dTotalTime = (double)pllTotalTime[i];
		dTotalTime /= (double)liStart.QuadPart;
		dTotalTime /= (double)PfrConfig.u32LoopCount;
		dTotalTime *= 1.E3;

		_ftprintf(pFile, _T("%10lld   \t  %8.3f    \t%9.2f \t  %8.3f    \t   %8.3f  \t   %8.3f      \t  %8.3f\n"), (long long)i64Depth, dTotalTime, 1.e3/dTotalTime, dStartTime, dBusyTime, dXferTime, (double)i64Depth * dDepthFactor /dXferTime );

		i64Depth >>= 1;
	}
	_ftprintf(stdout, _T("\nPRF test completed."));

	if ( (pFile) && (pFile!=stdout) )
	fclose(pFile);

	VirtualFree(pllStartTime, 0, MEM_RELEASE);
	VirtualFree(pllBusyTime, 0, MEM_RELEASE);
	VirtualFree(pllXferTime, 0, MEM_RELEASE);
	VirtualFree(pllTotalTime, 0, MEM_RELEASE);
	VirtualFree(pBuffer, 0, MEM_RELEASE);

#ifdef _WIN32
	CloseHandle(hTransferEvent);
#else
	sem_destroy(&hTransferEvent);
#endif	
	i32Status = CsFreeSystem(hSystem);
	return 0;
}

#define PRF_SECTION _T("PrfConfig")

int32 LoadPrfConfiguration(LPCTSTR szIniFile, int32 i32Res, PCSPRFCONFIG pConfig)
{
	TCHAR	szDefault[MAX_PATH];
	TCHAR	szString[MAX_PATH];
	TCHAR	szFilePath[MAX_PATH];
	int		nDummy;

	CSPRFCONFIG CsPrfCfg;	
	CsPrfCfg.i64StartDepth	= START_DEPTH;
	CsPrfCfg.i64FinishDepth = FINISH_DEPTH;              
	CsPrfCfg.u32LoopCount	= LOOP_COUNT; 
	CsPrfCfg.bUseAsTransfer = FALSE;
	CsPrfCfg.bCsTransferEx  = FALSE;
	CsPrfCfg.bInterleaved   = FALSE;

	strcpy(CsPrfCfg.strResultFile, _T(OUT_FILE));
	
	if (NULL == pConfig)
		return (CS_INVALID_PARAMETER);


	GetFullPathName(szIniFile, MAX_PATH, szFilePath, NULL);

	if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(szFilePath))
	{
		*pConfig = CsPrfCfg;
		// return (CS_INVALID_FILENAME);
		return (CS_USING_DEFAULTS);
	}

	if (0 == GetPrivateProfileSection(PRF_SECTION, szString, 100, szFilePath))
	{
		*pConfig = CsPrfCfg;
		return (CS_USING_DEFAULTS);
	}

	nDummy = (int)CsPrfCfg.i64StartDepth;
	CsPrfCfg.i64StartDepth = GetPrivateProfileInt(PRF_SECTION, _T("StartDepth"), nDummy, szFilePath);
	CsPrfCfg.i64StartDepth = AdjustToTriggerResMultiple(CsPrfCfg.i64StartDepth, i32Res, FALSE);

	nDummy = (int)CsPrfCfg.i64FinishDepth;
	CsPrfCfg.i64FinishDepth = GetPrivateProfileInt(PRF_SECTION, _T("FinishDepth"), nDummy, szFilePath);

	CsPrfCfg.i64FinishDepth = AdjustFinishToStartMultiple(CsPrfCfg.i64FinishDepth,  CsPrfCfg.i64StartDepth, FALSE);

	nDummy = (int)CsPrfCfg.u32LoopCount;
	CsPrfCfg.u32LoopCount = GetPrivateProfileInt(PRF_SECTION, _T("LoopCount"), nDummy, szFilePath);

	_stprintf(szDefault, _T("%s"), CsPrfCfg.strResultFile);
	GetPrivateProfileString(PRF_SECTION, _T("ResultsFile"), szDefault, szString, MAX_PATH, szFilePath);
	_tcscpy(CsPrfCfg.strResultFile, szString);

	nDummy = CsPrfCfg.bUseAsTransfer;
	nDummy = GetPrivateProfileInt(PRF_SECTION, _T("AsynTransfer"), nDummy, szFilePath);
	CsPrfCfg.bUseAsTransfer = (nDummy != 0);

	nDummy = CsPrfCfg.bCsTransferEx;
	nDummy = GetPrivateProfileInt(PRF_SECTION, _T("CsTransferEx"), nDummy, szFilePath);
	CsPrfCfg.bCsTransferEx = (nDummy != 0);

	nDummy = CsPrfCfg.bInterleaved;
	nDummy = GetPrivateProfileInt(PRF_SECTION, _T("Interleaved"), nDummy, szFilePath);
	CsPrfCfg.bInterleaved = (nDummy != 0);

	*pConfig = CsPrfCfg;
	return (CS_SUCCESS);
}


int64 ClosestPowerOf2(const int64 i64In, BOOL bLower )
{
	int64 i64Ret = 1;

	if(bLower)
	{
		while( i64Ret <= i64In )
		{
			i64Ret <<= 1;
		}

		i64Ret >>= 1;
	}
	else
	{
		while( i64Ret < i64In )
		{
			i64Ret <<= 1;
		}
	}

	return i64Ret;
}

int32 GetTriggerResolution(const CSHANDLE csHandle)
{
	int32 i32Res;
	uInt32 u32RequiredSize = sizeof(i32Res);

	int32 i32Status = CsGetSystemCaps(csHandle, CAPS_TRIGGER_RES, &i32Res, &u32RequiredSize);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		WaitForAcknowlegment();
		return (-1);
	}
	i32Res *= 2; /* so the resolution will be valid for all modes */
	return i32Res;
}

int64 AdjustToTriggerResMultiple(const int64 i64In, const int32 i32Res, const BOOL bLower)
{
	/*
		Adjusts the value in i64In so it's a multiple of the board's trigger resolution.
		The value is adjusted to the closest multiple greater than the original value. If
		bLower is true, the value is adjusted to the closest multiple less than the original
		value.
	*/
	int64 i64Ret = i32Res;

	if(bLower)
	{
		while( i64Ret <= i64In )
		{
			i64Ret += i32Res;
		}

		i64Ret -= i32Res;
	}
	else
	{
		while( i64Ret < i64In )
		{
			i64Ret += i32Res;
		}
	}
	return i64Ret;
}

int64 AdjustFinishToStartMultiple(const int64 i64Finish, const int64 i64Start, const BOOL bLower)
{
	/*
		Adjusts the finish value so that we can keep dividing it by 2 to get to 
		the start value. It stops at the first valid value more than the original
		start value. If bLower is true it will do a divide by 2 so we're less 
		than the original value.
	*/
	int64 i64Value = i64Start;
	while ( i64Value < i64Finish )
	{
		i64Value *= 2;
	}
	if ( bLower )
	{
		i64Value /= 2;
	}
	return i64Value;
}



void WaitForAcknowlegment()
{
	TCHAR tch;

	_ftprintf(stderr, _T("Press enter to continue...\n"));
	
	scanf(_T("%c"), &tch);
}
#ifdef _WIN32
void HandleFailedStatus(CSHANDLE hSystem, int32 iResult, HANDLE hEvent, bool bDisplayErr)
{
	if (bDisplayErr)
		DisplayErrorString(iResult);
	CsFreeSystem(hSystem);
	if (hEvent)
		CloseHandle(hEvent);	
	WaitForAcknowlegment();	
}	
#else
void HandleFailedStatus(CSHANDLE hSystem, int32 iResult, sem_t *hEvent, bool bDisplayErr)
{
	if (bDisplayErr)
		DisplayErrorString(iResult);
	CsFreeSystem(hSystem);
	if (hEvent)
		sem_destroy(hEvent);
	WaitForAcknowlegment();
}
#endif
