/////////////////////////////////////////////////////////////////////////
//
// GageMultipleSystem
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
// MultipleSystem demonstrates how to initialize, setup, acquire
// and transfer data using multiple independent CompuScope data
// acquisition system. The data from all active channels of each
// system are then saved to ASCII files as voltages. The work is done
// by a thread that is created for each CompuScope system found.
//
///////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#else // if __linux__
#include <pthread.h>
#endif

#include "CsPrototypes.h"
#include "CsAppSupport.h"
#include "CsTchar.h"
#include "CsSdkMisc.h"
#include "MultipleSystem.h" // included after CsAppSupport.h so TCHAR is defined

int _tmain()
{
	int32						i32Status = CS_SUCCESS;
	int							nSystemIndex, nSystems;
#ifdef _WIN32
	HANDLE						*phThread = NULL;
	DWORD						dwResult;
#else
	pthread_t					*phThread = NULL;
#endif
	CSHANDLE					*phSystem = NULL;
	TCHAR						szIniFile[20];
	LPCTSTR						szIniFileName = _T("System");
	THREADSTRUCT				*pSysThread = NULL;

/*
	Initializes the CompuScope boards found in the system. If the
	system is not found a message with the error code will appear.
	Otherwise i32Status will contain the number of systems found.
*/
	nSystems = CsInitialize();

	if (0 == i32Status)
	{
		printf("\nNo CompuScope systems found\n");
		return (-1);
	}

	if (CS_FAILED(nSystems))
	{
		DisplayErrorString(nSystems);
		return (-1);
	}
	if (1 == nSystems)
	{
		_tprintf(_T("\nThis example is designed to run with 2 or more CompuScope systems"));
		_tprintf(_T("\nIf you have only one system, please \nuse one of the other example programs.\n"));
		return (-1);
	}
/*
	Allocate memory for an array to hold all our CompuScope system handles
*/
	phSystem = (CSHANDLE *)VirtualAlloc(NULL, nSystems * sizeof(CSHANDLE), MEM_COMMIT, PAGE_READWRITE);
	if (NULL == phSystem)
	{
		_tprintf(_T("\nCould not allocate memory for system handles. Exiting program\n"));
		return 0;
	}
	pSysThread = (THREADSTRUCT *)VirtualAlloc(NULL, nSystems * sizeof(THREADSTRUCT), MEM_COMMIT, PAGE_READWRITE);	
	if (NULL == pSysThread)
	{
		VirtualFree(phSystem, 0, MEM_RELEASE);		
		_tprintf(_T("\nCould not allocate memory for system threads. Exiting program\n"));
		// FREE phSystem
		return 0;
	}	
/*
	Allocate memory for an array to hold all our thread handles
*/
#ifdef _WIN32
	phThread = (HANDLE *)VirtualAlloc(NULL, nSystems * sizeof(HANDLE), MEM_COMMIT, PAGE_READWRITE);
#else
	phThread = (pthread_t *)VirtualAlloc(NULL, nSystems * sizeof(pthread_t), MEM_COMMIT, PAGE_READWRITE);
#endif

	if (NULL == phThread)
	{
		VirtualFree(phSystem, 0, MEM_RELEASE);
		VirtualFree(pSysThread, 0, MEM_RELEASE);
		_tprintf(_T("\nCould not allocate memory for thread handles. Exiting program\n"));
		return 0;
	}

/*
	Call CsGetSystem in a loop to get the first available handle for
	each CompuScope system present in the system. The order the systems
	are found in depends on their position in the registry.
*/
	for (nSystemIndex = 0; nSystemIndex < nSystems; nSystemIndex++)
	{
		i32Status = CsGetSystem(&phSystem[nSystemIndex], 0, 0, 0, 0);
/*
		If we can't get a handle for any of the systems we've found, we'll
		print out a message and continue with the other systems.
*/
		if (CS_FAILED(i32Status))
		{
			DisplayErrorString(i32Status);
			_tprintf(_T("\nCannot get handle for system %d"), nSystemIndex + 1);
			_tprintf(_T("\nContinuing with other systems\n"));
			continue;
		}
	}
	
	for (nSystemIndex = 0; nSystemIndex < nSystems; nSystemIndex++)
	{
/*
		Creating a thread for each system we find and pass it the CompuScope system
		handle, the system number and the name of it's ini file.  In this example the
		name of the base ini file is System. The system number will be appended to the
		base name so the ini files are names System1.ini, System2.ini, etc.
*/
		pSysThread[nSystemIndex].hSystem = phSystem[nSystemIndex];
		pSysThread[nSystemIndex].Sys = nSystemIndex + 1;
/*
		In this example the name of the base ini file is System. The system number
		is appended to the base name so the ini files are names System1.ini, System2.ini, etc.
*/
		_stprintf(szIniFile, _T("%s%d.ini"), szIniFileName, nSystemIndex + 1);

		_tcsncpy(pSysThread[nSystemIndex].lpFilename, szIniFile, 100);
/*
		Start a thread for each system.  Each thread will do the setup, acquisition, transfer
		and save to disk for it's CompuScope system. If any errors occur, or when the thread
		completes it's work, it will clean up it's resource, call CsFreeSystem and exit.
*/
	}
	for (nSystemIndex = 0; nSystemIndex < nSystems; nSystemIndex++)
	{	
#ifdef _WIN32
		phThread[nSystemIndex] = CreateThread(NULL, 0, MultipleSystemThreadFunc, (void *)&(pSysThread[nSystemIndex]), 0, NULL);
#else
		pthread_create(&phThread[nSystemIndex], NULL, MultipleSystemThreadFunc, (void*)&(pSysThread[nSystemIndex]));
#endif
	}
/*
	Wait for the threads to finish.
*/
#ifdef _WIN32
	dwResult = WaitForMultipleObjects(nSystemIndex, phThread, TRUE, INFINITE);
#else
	for (nSystemIndex = 0; nSystemIndex < nSystems; nSystemIndex++)
	{
		pthread_join(phThread[nSystemIndex], NULL);
	}
#endif
	if (NULL == phSystem)
		VirtualFree(phSystem, 0, MEM_RELEASE);
	if (NULL == phThread)
		VirtualFree(phThread, 0, MEM_RELEASE);

	return 0;
}

#ifdef _WIN32
DWORD WINAPI MultipleSystemThreadFunc(void *arg)
#else // __linux__
void *MultipleSystemThreadFunc(void *arg)
#endif
{
	IN_PARAMS_TRANSFERDATA		InData = {0};
	OUT_PARAMS_TRANSFERDATA		OutData = {0};
	CSSYSTEMINFO				CsSysInfo = {0};
	CSAPPLICATIONDATA			CsAppData = {0};
	CSHANDLE					hSystem;
	TCHAR						szIniFile[100];
	int							Sys;
	uInt32						i;
	int32						i32Status = CS_SUCCESS;
	void*						pBuffer = NULL;
	float*						pVBuffer = NULL;
	TCHAR						szFileName[MAX_PATH];
	uInt32						u32Mode;
	FileHeaderStruct			stHeader = {0};
	CSACQUISITIONCONFIG			CsAcqCfg = {0};
	CSCHANNELCONFIG				CsChanCfg = {0};
	uInt32						u32ChannelIndexIncrement;
	BOOL						bSuccess = TRUE;
	int64						i64SavedLength;	
	int64						i64StartOffset = 0;	


	THREADSTRUCT *pThreadStruct = (THREADSTRUCT *)arg;
	hSystem = pThreadStruct->hSystem;
	Sys = pThreadStruct->Sys;
	_tcsncpy(szIniFile, pThreadStruct->lpFilename, 100);
/*
	Get the system information. The u32Size field must be filled in
	 prior to calling CsGetSystemInfo
*/
	CsSysInfo.u32Size = sizeof(CSSYSTEMINFO);
	i32Status = CsGetSystemInfo(hSystem, &CsSysInfo);

/*
	Display the system name from the driver
*/
	_tprintf(_T("\nBoard Name: %s"), CsSysInfo.strBoardName);

	i32Status = CsAs_ConfigureSystem(hSystem, (int)CsSysInfo.u32ChannelCount, 1, (LPCTSTR)szIniFile, &u32Mode);

	if (CS_FAILED(i32Status))
	{
		if (CS_INVALID_FILENAME == i32Status)
		{
/*
			Display message but continue on using defaults.
*/
			_tprintf(_T("\nCannot find %s - using default parameters."), szIniFile);
		}
		else
		{
/*
			Otherwise the call failed.  If the call did fail we should free the CompuScope
			system so it's available for another application
*/
			DisplayErrorString(i32Status);
			CsFreeSystem(hSystem);
			return (0);
		}
	}
/*
	If the return value is greater than  1, then either the application,
	acquisition, some of the Channel and / or some of the Trigger sections
	were missing from the ini file and the default parameters were used.
*/
	if (CS_USING_DEFAULT_ACQ_DATA & i32Status)
		_tprintf(_T("\nNo ini entry for acquisition. Using defaults."));

	if (CS_USING_DEFAULT_CHANNEL_DATA & i32Status)
		_tprintf(_T("\nNo ini entry for one or more Channels. Using defaults for missing items."));

	if (CS_USING_DEFAULT_TRIGGER_DATA & i32Status)
		_tprintf(_T("\nNo ini entry for one or more Triggers. Using defaults for missing items."));

	i32Status = CsAs_LoadConfiguration(hSystem, szIniFile, APPLICATION_DATA, &CsAppData);

	if (CS_FAILED(i32Status))
	{
		if (CS_INVALID_FILENAME == i32Status)
			_tprintf(_T("\nUsing default application parameters."));
		else
		{
			DisplayErrorString(i32Status);
			CsFreeSystem(hSystem);
			return (0);
		}
	}
	else if (CS_USING_DEFAULT_APP_DATA & i32Status)
	{
/*
		If the return value is CS_USING_DEFAULT_APP_DATA (defined in ConfigSystem.h)
		then there was no entry in the ini file for Application and we will use
		the application default values, which were set in CsAs_LoadConfiguration.
*/
		_tprintf(_T("\nNo ini entry for application data. Using defaults"));
	}
/*
	Commit the values to the driver.  This is where the values get sent to the
	hardware.  Any invalid parameters will be caught here and an error returned.
*/
	i32Status = CsDo(hSystem, ACTION_COMMIT);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(hSystem);
		return (0);
	}
/*
	Get the current sample size, resolution and offset parameters from the driver
	by calling CsGet for the ACQUISTIONCONFIG structure. These values are used
	when saving the file.
*/
	CsAcqCfg.u32Size = sizeof(CsAcqCfg);
	i32Status = CsGet(hSystem, CS_ACQUISITION, CS_ACQUISITION_CONFIGURATION, &CsAcqCfg);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(hSystem);
		return (0);
	}
/*
	Start the data Acquisition
*/
	i32Status = CsDo(hSystem, ACTION_START);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(hSystem);
		return (0);
	}
	if (!DataCaptureComplete(hSystem))
	{
		CsFreeSystem(hSystem);
		return (0);
	}
/*
	Aquisition is now complete. We need to allocate a buffer
	for transferring the data
*/
	pBuffer  = VirtualAlloc(NULL, (size_t)(CsAppData.i64TransferLength * CsAcqCfg.u32SampleSize), MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pBuffer)
	{
		_tprintf (_T("\nUnable to allocate memory\n"));
		CsFreeSystem(hSystem);
		return (0);
	}

	if (TYPE_FLOAT == CsAppData.i32SaveFormat)
	{
/*
		Allocate another buffer to pass the data that is going to be converted
		into voltages
*/
		pVBuffer  = (float *)VirtualAlloc(NULL, (size_t)(CsAppData.i64TransferLength * sizeof(float)), MEM_COMMIT, PAGE_READWRITE);
		if (NULL == pVBuffer)
		{
			_tprintf (_T("\nUnable to allocate memory\n"));
			CsFreeSystem(hSystem);
			VirtualFree(pBuffer, 0, MEM_RELEASE);
			return (0);
		}
	}
/*
	Fill in the InData structure for transferring the data
*/
/*
	Non multiple record captures should have the segment set to 1.
	InData.u32Mode refers to the transfer mode. Regular transfer is 0
*/
	InData.u32Segment = 1;
	InData.u32Mode = TxMODE_DEFAULT;
	InData.i64StartAddress = CsAppData.i64TransferStartPosition;
	InData.i64Length =  CsAppData.i64TransferLength;
	InData.pDataBuffer = pBuffer;

	u32ChannelIndexIncrement = CsAs_CalculateChannelIndexIncrement(&CsAcqCfg, &CsSysInfo );

	for	(i = 1; i <= CsSysInfo.u32ChannelCount; i += u32ChannelIndexIncrement)
	{
/*
		Variable that will contain either raw data or data in Volts depending on requested format
*/
		void* pSrcBuffer = NULL;

		ZeroMemory(pBuffer,(size_t)(CsAppData.i64TransferLength * CsAcqCfg.u32SampleSize));
		InData.u16Channel = (uInt16)i;
/*
		Transfer the captured data
*/
		i32Status = CsTransfer(hSystem, &InData, &OutData);
		if (CS_FAILED(i32Status))
		{
			DisplayErrorString(i32Status);
			if (NULL != pBuffer)
				VirtualFree(pBuffer, 0, MEM_RELEASE);
			if (NULL != pVBuffer)
				VirtualFree(pVBuffer, 0, MEM_RELEASE);
			CsFreeSystem(hSystem);
			return (0);
		}
/*
		Note: to optimize the transfer loop, everything from
		this point on in the loop could be moved out and done
		after all the channels are transferred.
*/

/*
		Assign a file name for each channel that we want to save
*/
		_stprintf(szFileName, _T("%s_Sys%d_CH%02u.dat"), CsAppData.lpszSaveFileName, Sys, i);

/*
		Gather up the information needed for the volt conversion and/or file header
*/
		CsChanCfg.u32Size = sizeof(CSCHANNELCONFIG);
		CsChanCfg.u32ChannelIndex = i;
		CsGet(hSystem, CS_CHANNEL, CS_ACQUISITION_CONFIGURATION, &CsChanCfg);
		i64StartOffset = InData.i64StartAddress - OutData.i64ActualStart;
		if (i64StartOffset < 0)
		{
			i64StartOffset = 0;
			InData.i64StartAddress = OutData.i64ActualStart;
		}		
		
/*
		Save the smaller of the requested transfer length or the actual transferred length
 */
		OutData.i64ActualLength -= i64StartOffset;
		i64SavedLength = CsAppData.i64TransferLength <= OutData.i64ActualLength ? CsAppData.i64TransferLength : OutData.i64ActualLength;		

		if (TYPE_FLOAT == CsAppData.i32SaveFormat)
		{
/*
            Call the ConvertToVolts function. This function will convert the raw
            data to voltages. We pass the saved length, which will be converted
            from 0 to actual length.  Invalid samples at the beginning are
            skipped over here.
*/

			i32Status = CsAs_ConvertToVolts(i64SavedLength, CsChanCfg.u32InputRange, CsAcqCfg.u32SampleSize,
                                         CsAcqCfg.i32SampleOffset, CsAcqCfg.i32SampleRes,
                                         CsChanCfg.i32DcOffset, (void *)((unsigned char *)pBuffer + (i64StartOffset * CsAcqCfg.u32SampleSize)),
										 pVBuffer);

			if (CS_FAILED(i32Status))
			{
				DisplayErrorString(i32Status);
				bSuccess = FALSE;
				continue;
			}

			pSrcBuffer = pVBuffer;
		}
		else
		{
			pSrcBuffer = (void *)((unsigned char *)pBuffer + (i64StartOffset * CsAcqCfg.u32SampleSize));
		}
/*
		The driver may have had to change the start address and length
		due to alignment issues, so we'll get the actual start and length
		from the driver.
*/

		stHeader.i64SampleRate = CsAcqCfg.i64SampleRate;
		stHeader.i64Start = InData.i64StartAddress;
		stHeader.i64Length = i64SavedLength;
		stHeader.u32SampleSize = CsAcqCfg.u32SampleSize;
		stHeader.i32SampleRes = CsAcqCfg.i32SampleRes;
		stHeader.i32SampleOffset = CsAcqCfg.i32SampleOffset;
		stHeader.u32InputRange = CsChanCfg.u32InputRange;
		stHeader.i32DcOffset = CsChanCfg.i32DcOffset;
		stHeader.u32SegmentCount = CsAcqCfg.u32SegmentCount;
		stHeader.u32SegmentNumber = InData.u32Segment;
		stHeader.dTimeStamp = NO_TIME_STAMP_VALUE;
		// For sig files we treat each multiple record segment of the capture as a separate file
		stHeader.u32SegmentCount = (TYPE_SIG == CsAppData.i32SaveFormat) ? 1 : CsAcqCfg.u32SegmentCount;
		stHeader.u32SegmentNumber = (TYPE_SIG == CsAppData.i32SaveFormat) ? 1 : InData.u32Segment;

		i32Status = (int32)CsAs_SaveFile(szFileName, pSrcBuffer, CsAppData.i32SaveFormat, &stHeader);
		if ( 0 > i32Status )
		{
			if (CS_MISC_ERROR == i32Status)
			{
				_tprintf(_T("\n"));
				_tprintf(_T("%s"), CsAs_GetLastFileError());
			}
			else
			{
				DisplayErrorString(i32Status);
			}
		}
	}

/*
	Free any buffers that have been allocated
*/
	if ( NULL != pVBuffer )
	{
		VirtualFree(pVBuffer, 0, MEM_RELEASE);
		pVBuffer = NULL;
	}

	if ( NULL != pBuffer)
	{
		VirtualFree(pBuffer, 0, MEM_RELEASE);
		pBuffer = NULL;
	}

/*
	Free the CompuScope system and any resources it's been using
*/
	i32Status = CsFreeSystem(hSystem);
	if (CS_FAILED(i32Status))
		DisplayErrorString(i32Status);

	if (bSuccess)
		_tprintf (_T("\n[%s] Acquisition completed. \nAll channels are saved in the current working directory.\n"), CsSysInfo.strBoardName);
	else
		_tprintf (_T("\n[%s] An error has occurred.\n"), CsSysInfo.strBoardName);

	return (0);
}
