/////////////////////////////////////////////////////////////////////////
//
// GageDeepAcquisition
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
// GageDeepAcquisition demonstrates how to initialize, setup, acquire
// and transfer data using a CompuScope data acquisition system. Data from
// all channels are then saved to ASCII files as voltages.  This example
// also demonstrates how to transfer a large amount of acquired data by
// transferring smaller segments of the data into separate files. These
// segments can then be pasted together.
//
///////////////////////////////////////////////////////////////////////////

#if _WIN32
#include <windows.h>
#include <stdio.h>
#endif

#include "CsPrototypes.h"
#include "CsAppSupport.h"
#include "CsTchar.h"
#include "CsSdkMisc.h"


int _tmain()
{
	int32						i32Status = CS_SUCCESS;
	uInt32						i;
	TCHAR						szFileName[MAX_PATH];
	TCHAR						szFormatString[MAX_PATH];
	int64						i64Depth = 0;
	void*						pBuffer = NULL;
	float*						pVBuffer = NULL;
	uInt32						u32Mode;
	CSHANDLE					hSystem = 0;
	IN_PARAMS_TRANSFERDATA		InData = {0};
	OUT_PARAMS_TRANSFERDATA		OutData = {0};
	CSSYSTEMINFO				CsSysInfo = {0};
	CSAPPLICATIONDATA			CsAppData = {0};
	FileHeaderStruct			stHeader = {0};
	CSACQUISITIONCONFIG			CsAcqCfg = {0};
	CSCHANNELCONFIG				CsChanCfg = {0};
	LPCTSTR						szIniFile = _T("DeepAcquisition.ini");
	int							nMaxPages;
	int							nMaxPageNumber;
	int							nMaxChannelNumber;
	uInt32						u32ChannelIndexIncrement;
	int64						i64TransferPadding = 32; /* Padding to ensure we can transfer what we requested */
	int64						i64MinSA;
	int64						i64MaxLength;
/*
	Initializes the CompuScope boards found in the system. If the
	system is not found a message with the error code will appear.
	Otherwise i32Status will contain the number of systems found.
*/
	i32Status = CsInitialize();

	if (0 == i32Status)
	{
		printf("\nNo CompuScope systems found\n");
		return (-1);
	}

	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		return (-1);
	}

/*
	Get System. This sample program only supports one system. If
	2 systems or more are found, the first system that is found
	will be the system that will be used. hSystem will hold a unique
	system identifier that is used when referencing the system.
*/
	i32Status = CsGetSystem(&hSystem, 0, 0, 0, 0);

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
	i32Status = CsGetSystemInfo(hSystem, &CsSysInfo);

/*
	Display the system name from the driver
*/
	_tprintf(_T("\nBoard Name: %s"), CsSysInfo.strBoardName);

/* 
	In this example we're only using 1 trigger source
*/	
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
			return(-1);
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
			return -1;
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
		return (-1);
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
		return (-1);
	}
/*
	We need to allocate a buffer
	for transferring the data.  In this example we will be
	transferring the data a page at a time.  The page size
	is set in the ini file.
*/
	pBuffer  = VirtualAlloc(NULL, (size_t)((CsAppData.u32PageSize+(uInt32)i64TransferPadding) * CsAcqCfg.u32SampleSize), MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pBuffer)
	{
		_tprintf (_T("\nUnable to allocate memory"));
		CsFreeSystem(hSystem);
		return (-1);
	}

	if (TYPE_FLOAT == CsAppData.i32SaveFormat)
	{
/*
		Allocate another buffer to pass the data that is going to be converted
		into voltages
*/
		pVBuffer  = (float *)VirtualAlloc(NULL, (size_t)((CsAppData.u32PageSize+(uInt32)i64TransferPadding) * sizeof(float)), MEM_COMMIT, PAGE_READWRITE);
		if (NULL == pVBuffer)
		{
			_tprintf (_T("\nUnable to allocate memory\n"));
			CsFreeSystem(hSystem);
			VirtualFree(pBuffer, 0, MEM_RELEASE);
			return (-1);
		}
	}
/*
	Start the data Acquisition
*/
	i32Status = CsDo(hSystem, ACTION_START);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(hSystem);
		return (-1);
	}

	if (!DataCaptureComplete(hSystem))
	{
		CsFreeSystem(hSystem);
		return (-1);
	}
/*
	Acquisition is now complete.
	Fill in the InData structure for transferring the data
*/
/*
	Non multiple record captures should have the segment set to 1.
	InData.u32Mode refers to the transfer mode. Regular transfer is 0
*/
	InData.u32Segment = 1;
	InData.u32Mode = TxMODE_DEFAULT;
	InData.pDataBuffer = pBuffer;

/*  
	Validate the start address and the length.  This is especially necessary if
	trigger delay is being used
*/
	i64MinSA = CsAcqCfg.i64TriggerDelay + CsAcqCfg.i64Depth - CsAcqCfg.i64SegmentSize;
	if (CsAppData.i64TransferStartPosition < i64MinSA)
	{
		_ftprintf(stdout, _T("\nInvalid Start Address was changed from %lld to %lld\n"), (long long)CsAppData.i64TransferStartPosition, (long long)i64MinSA);
		CsAppData.i64TransferStartPosition = i64MinSA;
	}
	i64MaxLength = CsAcqCfg.i64TriggerDelay + CsAcqCfg.i64Depth - i64MinSA;
	if (CsAppData.i64TransferLength > i64MaxLength)
	{
		_ftprintf(stdout, _T("\nInvalid Transfer Length was changed from %lld to %lld\n"), (long long)CsAppData.i64TransferLength, (long long)i64MaxLength);
		CsAppData.i64TransferLength = i64MaxLength;
	}

	u32ChannelIndexIncrement = CsAs_CalculateChannelIndexIncrement(&CsAcqCfg, &CsSysInfo );
/*
	format a string with the number of pages  and channels so all filenames will have
	the same number of characters.
*/
	i64Depth = CsAppData.i64TransferLength;
	nMaxPages = (int)(i64Depth / CsAppData.u32PageSize);
	if (0 != (i64Depth % CsAppData.u32PageSize))
		nMaxPages++;

	_stprintf(szFormatString, _T("%d"), nMaxPages);
	nMaxPageNumber = (int)_tcslen(szFormatString);

	_stprintf(szFormatString, _T("%u"), CsSysInfo.u32ChannelCount);
	nMaxChannelNumber = (int)_tcslen(szFormatString);

	_stprintf(szFormatString, _T("%%s_CH%%0%dd-%%0%dd.dat"), nMaxChannelNumber, nMaxPageNumber);

	for	(i = 1; i <= CsSysInfo.u32ChannelCount; i += u32ChannelIndexIncrement)
	{
/*
		Variable that will contain either raw data or data in Volts depending on requested format
*/
		void* pSrcBuffer = NULL;

		uInt32 u32PageNumber = 1;
		ZeroMemory(pBuffer,(size_t)((CsAppData.u32PageSize+i64TransferPadding) * CsAcqCfg.u32SampleSize));
		InData.u16Channel = (uInt16)i;

		i64Depth = CsAppData.i64TransferLength;
		InData.i64StartAddress = CsAppData.i64TransferStartPosition;
		InData.i64Length =  (int64)CsAppData.u32PageSize;

/*
		Gather up the information needed for the volt conversion and/or file header
*/
		CsChanCfg.u32Size = sizeof(CSCHANNELCONFIG);
		CsChanCfg.u32ChannelIndex = i;
		CsGet(hSystem, CS_CHANNEL, CS_ACQUISITION_CONFIGURATION, &CsChanCfg);


		InData.pDataBuffer = pBuffer;
/*
		Transfer the captured data
*/
		while (i64Depth > 0)
		{
			int64 i64TransferredDepth = 0;
			int64 i64StartOffset = 0;
			if (i64Depth >= CsAppData.u32PageSize)
				InData.i64Length =  (int64)CsAppData.u32PageSize;
			else
				InData.i64Length =  i64Depth;
			
			InData.i64Length += i64TransferPadding;
			
/*
			Check if our start address is valid. CsAcqCfg.i64Depth is
			the last valid post-trigger sample, so the start address
			cannot be beyond that.
*/
			if (InData.i64StartAddress >= (CsAcqCfg.i64Depth + CsAcqCfg.i64TriggerDelay))
				break;

			i32Status = CsTransfer(hSystem, &InData, &OutData);
			if (CS_FAILED(i32Status))
			{
				DisplayErrorString(i32Status);
				if (NULL != pBuffer)
					VirtualFree(pBuffer, 0, MEM_RELEASE);
				if (NULL != pVBuffer)
					VirtualFree(pVBuffer, 0, MEM_RELEASE);
				CsFreeSystem(hSystem);
				return (-1);
			}
/*

			Check to see if what's remaining to tranfer is less than our page size
*/
			i64TransferredDepth = (OutData.i64ActualLength < CsAppData.u32PageSize) ? OutData.i64ActualLength : CsAppData.u32PageSize;
			i64StartOffset = InData.i64StartAddress - OutData.i64ActualStart;
			if (i64StartOffset < 0 )
			{
				i64StartOffset = 0;
				InData.i64StartAddress = OutData.i64ActualStart;
			}
/*
	    	Adjust depth for alignment of Requested Start and Actual Start addresses on the last page. This is
			the only one that the transfer size will be less than the page size. If they're the same then we 
			shouldn't need the adjustment.
*/
			if (i64TransferredDepth < CsAppData.u32PageSize)
			{
				i64TransferredDepth -= i64StartOffset; 
			}
/*
			Note: to optimize the transfer loop, everything from
			this point on in the loop could be moved out and done
			after all the channels are transferred.
*/

/*
			Assign a file name for each channel that we want to save
*/
			_stprintf(szFileName, szFormatString, CsAppData.lpszSaveFileName, i, u32PageNumber);
/*
			Note: because this sample is saving the captured data in chunks to a file, we're 
			handling the adjustment of the starting address and length ourselves for each chunk. We 
			request a transfer of more than our chunksize to be sure that we get our chunksize and then
			adjust pSrcBuffer to start at the right address.
*/
			if (TYPE_FLOAT == CsAppData.i32SaveFormat)
			{
/*
				Call the ConvertToVolts function. This function will convert the raw
				data to voltages. We pass the buffer plus the actual start, which will be converted
				from reqested start to actual length in the volts buffer.  
*/




				i32Status = CsAs_ConvertToVolts(i64TransferredDepth, CsChanCfg.u32InputRange, CsAcqCfg.u32SampleSize,
												CsAcqCfg.i32SampleOffset, CsAcqCfg.i32SampleRes,
												CsChanCfg.i32DcOffset, (void *)((unsigned char *)pBuffer + (i64StartOffset * CsAcqCfg.u32SampleSize)), pVBuffer);
												
				if (CS_FAILED(i32Status))
				{
					DisplayErrorString(i32Status);
					continue;
				}

				pSrcBuffer = (void *)(pVBuffer);
			}
			else
			{
				pSrcBuffer = (void *)((unsigned char *)pBuffer + (i64StartOffset * CsAcqCfg.u32SampleSize));
			}
/*
			We can put the requested Start Address in the header because any alignment issues have been
			handled in the buffer.



*/
			stHeader.i64SampleRate = CsAcqCfg.i64SampleRate;
			stHeader.i64Start = InData.i64StartAddress; 
			stHeader.i64Length = i64TransferredDepth;
			stHeader.u32SampleSize = CsAcqCfg.u32SampleSize;
			stHeader.i32SampleRes = CsAcqCfg.i32SampleRes;
			stHeader.i32SampleOffset = CsAcqCfg.i32SampleOffset;
			stHeader.u32InputRange = CsChanCfg.u32InputRange;
			stHeader.i32DcOffset = CsChanCfg.i32DcOffset;
			// For sig files we treat each multiple record segment of the capture as a separate file
			stHeader.u32SegmentCount = (TYPE_SIG == CsAppData.i32SaveFormat) ? 1 : CsAcqCfg.u32SegmentCount;
			stHeader.u32SegmentNumber = (TYPE_SIG == CsAppData.i32SaveFormat) ? 1 : InData.u32Segment;
			stHeader.dTimeStamp = NO_TIME_STAMP_VALUE;

			i32Status = (int32)CsAs_SaveFile(szFileName, pSrcBuffer, CsAppData.i32SaveFormat, &stHeader);
			if ( 0 > i32Status )
			{
				if (CS_MISC_ERROR == i32Status)
				{
					_tprintf(_T("\nFile Error: "));
					
					_tprintf(_T("%s"), CsAs_GetLastFileError());
				}
				else
				{
					DisplayErrorString(i32Status);
				}
				continue;
			}

			i64Depth -= i64TransferredDepth;
			InData.i64StartAddress += (int64)CsAppData.u32PageSize;
			u32PageNumber++;
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

	DisplayFinishString(CsAppData.i32SaveFormat);
	return 0;
}
