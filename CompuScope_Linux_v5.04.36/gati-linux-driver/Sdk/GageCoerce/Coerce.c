/////////////////////////////////////////////////////////////////////////
//
// GageCoerce
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
// GageCoerce demonstrates how to initialize, setup, acquire
// and transfer data using a CompuScope data acquisition system. Data from
// all channels are then saved to ASCII files as voltages.  This example
// also demonstrates how to coerce any invalid parameters to valid ones
// using CsDo(ACTION_COMMIT_COERCE). In this example, any parameters that
// have been coerced are printed to the console.
//
///////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#endif

#include "CsPrototypes.h"
#include "Coerce.h"

#include "CsAppSupport.h"
#include "CsTchar.h"
#include "CsSdkMisc.h"

int _tmain()
{
	int32						i32Status = CS_SUCCESS;
	int							j;
	uInt32						i;
	TCHAR						szFileName[MAX_PATH];
	int64						i64StartOffset = 0;	
	void*						pBuffer = NULL;
	float*						pVBuffer = NULL;
	uInt32						u32Mode;
	CSHANDLE					hSystem = 0;
	IN_PARAMS_TRANSFERDATA		InData = {0};
	OUT_PARAMS_TRANSFERDATA		OutData = {0};
	CSSYSTEMINFO				CsSysInfo = {0};
	CSAPPLICATIONDATA			CsAppData = {0};
	CSACQUISITIONCONFIG			CsCurrentAcqConfig = {0};
	CSCHANNELCONFIG*			pCsCurrentChanConfig = NULL;
	CSTRIGGERCONFIG				CsCurrentTrigConfig = {0};
	LPCTSTR						szIniFile = _T("Coerce.ini");
	FileHeaderStruct			stHeader = {0};
	CSACQUISITIONCONFIG			CsAcqCfg = {0};
	CSCHANNELCONFIG				CsChanCfg = {0};
	BOOL						bReportAcqSettings = TRUE;
	BOOL						bReportChanSettings = TRUE;
	BOOL						bReportTrigSettings = TRUE;
	uInt32						u32ChannelIndexIncrement;
	int64						i64SavedLength;	

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


	CsCurrentAcqConfig.u32Size = sizeof(CSACQUISITIONCONFIG);
	i32Status = CsGet( hSystem, CS_ACQUISITION, CS_CURRENT_CONFIGURATION, &CsCurrentAcqConfig);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		_tprintf(_T("\nCannot display acquisition settings"));
		bReportAcqSettings = FALSE;
	}

	pCsCurrentChanConfig = (CSCHANNELCONFIG *)VirtualAlloc(NULL, CsSysInfo.u32ChannelCount * sizeof(CSCHANNELCONFIG), MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pCsCurrentChanConfig)
	{
		_tprintf(_T("\nMemory allocation error. Cannot display channel settings"));
		bReportChanSettings = FALSE;
	}

	if (bReportChanSettings)
	{
		for (i = 1, j = 0; i <= CsSysInfo.u32ChannelCount; i++, j++)
		{
			pCsCurrentChanConfig[j].u32Size = sizeof(CSCHANNELCONFIG);
			pCsCurrentChanConfig[j].u32ChannelIndex = (uInt32)(i);
			i32Status = CsGet(hSystem, CS_CHANNEL, CS_CURRENT_CONFIGURATION, &pCsCurrentChanConfig[j]);
			if (CS_FAILED(i32Status))
			{
				DisplayErrorString(i32Status);
				bReportChanSettings = FALSE;
				_tprintf(_T("\nCannot display channel %u settings"), i);
			}
		}
	}
	CsCurrentTrigConfig.u32Size = sizeof(CSTRIGGERCONFIG);
	CsCurrentTrigConfig.u32TriggerIndex = 1;
	i32Status = CsGet( hSystem, CS_TRIGGER, CS_CURRENT_CONFIGURATION, &CsCurrentTrigConfig);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		_tprintf(_T("\nCannot display trigger settings"));
		bReportTrigSettings = FALSE;
	}

/*
	Commit the values to the driver.  This is where the values get sent to the
	hardware.  Any invalid parameters will be caught and coerced to the nearest
	valid value.  The only error that can occur now is if and invalid system
	was used.
*/
	i32Status = CsDo(hSystem, ACTION_COMMIT_COERCE);
	if (CS_FAILED(i32Status))
	{
		DisplayErrorString(i32Status);
		CsFreeSystem(hSystem);
		return (-1);
	}

	if (CS_SUCCESS < i32Status)
	{
/*
		Go thru the acquisition, channel and trigger configurations from
		the driver and the hardware and see what are different.  The ones
		that are different have been coerced so print them to the screen.
*/
		if (bReportAcqSettings)
			ReportAcquisitionChanges(hSystem, &CsCurrentAcqConfig);

		if (bReportChanSettings)
			ReportChannelChanges(hSystem, CsSysInfo.u32ChannelCount, pCsCurrentChanConfig);
		if (bReportTrigSettings)
			ReportTriggerChanges(hSystem, &CsCurrentTrigConfig);
	}

	VirtualFree(pCsCurrentChanConfig, 0, MEM_RELEASE);

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
	for transferring the data
*/
	pBuffer  = VirtualAlloc(NULL, (size_t)(CsAppData.i64TransferLength * CsAcqCfg.u32SampleSize), MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pBuffer)
	{
		_tprintf (_T("\nUnable to allocate memory\n"));
		CsFreeSystem(hSystem);
		return (-1);
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
			return (-1);
		}
	}

/*
	<===  If you want to perform a repetitive capture start your loop here.
*/

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
			return (-1);
		}
/*
		Note: to optimize the transfer loop, everything from
		this point on in the loop could be moved out and done
		after all the channels are transferred.
*/

/*
		Assign a file name for each channel that we want to save
*/
		_stprintf(szFileName, _T("%s_CH%u.dat"), CsAppData.lpszSaveFileName, i);

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

		stHeader.i64Start = InData.i64StartAddress;
		stHeader.i64Length = i64SavedLength;
		stHeader.u32SampleSize = CsAcqCfg.u32SampleSize;
		stHeader.i32SampleRes = CsAcqCfg.i32SampleRes;
		stHeader.i32SampleOffset = CsAcqCfg.i32SampleOffset;
		stHeader.u32InputRange = CsChanCfg.u32InputRange;
		stHeader.i32DcOffset = CsChanCfg.i32DcOffset;
		stHeader.u32SegmentCount = CsAcqCfg.u32SegmentCount;
		stHeader.u32SegmentNumber = InData.u32Segment;
		// For sig files we treat each multiple record segment of the capture as a separate file
		stHeader.u32SegmentCount = (TYPE_SIG == CsAppData.i32SaveFormat) ? 1 : CsAcqCfg.u32SegmentCount;
		stHeader.u32SegmentNumber = (TYPE_SIG == CsAppData.i32SaveFormat) ? 1 : InData.u32Segment;		
		stHeader.dTimeStamp = NO_TIME_STAMP_VALUE;
		
		i32Status = (int32)CsAs_SaveFile(szFileName, pSrcBuffer, CsAppData.i32SaveFormat, &stHeader);

		if ( 0 > i32Status )
		{
			if (CS_MISC_ERROR == i32Status)
			{
				_tprintf(_T("\n"));
				_tprintf("%s", CsAs_GetLastFileError());
			}
			else
			{
				DisplayErrorString(i32Status);
			}
			continue;
		}
	}

/*
	<=== End of the repetitive capture loop.
*/


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

BOOL ReportAcquisitionChanges(CSHANDLE hSystem, CSACQUISITIONCONFIG *pAcqConfig)
{
	CSACQUISITIONCONFIG CsHardwareAcqConfig;
	int32 i32Status;

	CsHardwareAcqConfig.u32Size = sizeof(CSACQUISITIONCONFIG);

	i32Status = CsGet(hSystem, CS_ACQUISITION, CS_ACQUISITION_CONFIGURATION, &CsHardwareAcqConfig);
	if (CS_FAILED(i32Status))
	{
		_tprintf(_T("\nCannot compare acquisition structures"));
		return FALSE;
	}

	if (pAcqConfig->i64Depth != CsHardwareAcqConfig.i64Depth)
		_tprintf(_T("\nTrigger depth coerced from "F64" to "F64""), pAcqConfig->i64Depth, CsHardwareAcqConfig.i64Depth);

	if (pAcqConfig->i64SampleRate != CsHardwareAcqConfig.i64SampleRate)
		_tprintf(_T("\nSample rate coerced from "F64" to "F64""), pAcqConfig->i64SampleRate, CsHardwareAcqConfig.i64SampleRate);

	if (pAcqConfig->i64SegmentSize != CsHardwareAcqConfig.i64SegmentSize)
		_tprintf(_T("\nSegment size coerced from "F64" to "F64""), pAcqConfig->i64SegmentSize, CsHardwareAcqConfig.i64SegmentSize);

	if (pAcqConfig->i64TriggerDelay != CsHardwareAcqConfig.i64TriggerDelay)
		_tprintf(_T("\nTrigger delay coerced from "F64" to "F64""), pAcqConfig->i64TriggerDelay, CsHardwareAcqConfig.i64TriggerDelay);

	if (pAcqConfig->i64TriggerHoldoff != CsHardwareAcqConfig.i64TriggerHoldoff)
		_tprintf(_T("\nTrigger holdoff coerced from "F64" to "F64""), pAcqConfig->i64TriggerHoldoff, CsHardwareAcqConfig.i64TriggerHoldoff);

	if (pAcqConfig->i64TriggerTimeout != CsHardwareAcqConfig.i64TriggerTimeout)
		_tprintf(_T("\nTrigger timeout coerced from "F64" to "F64""), pAcqConfig->i64TriggerTimeout, CsHardwareAcqConfig.i64TriggerTimeout);

	if (pAcqConfig->u32Mode != CsHardwareAcqConfig.u32Mode)
		_tprintf(_T("\nMode coerced from %u to %u"), pAcqConfig->u32Mode, CsHardwareAcqConfig.u32Mode);

	if (pAcqConfig->u32SegmentCount != CsHardwareAcqConfig.u32SegmentCount)
		_tprintf(_T("\nSegment count coerced from %u to %u"), pAcqConfig->u32SegmentCount, CsHardwareAcqConfig.u32SegmentCount);

	return TRUE;
}

BOOL ReportChannelChanges(CSHANDLE hSystem, uInt32 u32ChannelCount, CSCHANNELCONFIG *pChanConfig)
{
	CSCHANNELCONFIG CsHardwareChanConfig;
	int32 i32Status;
	uInt32 i;
	int j;

	for (i = 1, j = 0; i <= u32ChannelCount; i++, j++)
	{
		CsHardwareChanConfig.u32Size = sizeof(CSCHANNELCONFIG);
		CsHardwareChanConfig.u32ChannelIndex = (uInt32)(i);
		i32Status = CsGet(hSystem, CS_CHANNEL, CS_ACQUISITION_CONFIGURATION, &CsHardwareChanConfig);
		if (CS_FAILED(i32Status))
		{
			_tprintf(_T("\nCannot compare channel structures for channel %u"), i);
			return FALSE;
		}
		if (pChanConfig[j].u32InputRange != CsHardwareChanConfig.u32InputRange)
			_tprintf(_T("\nChannel %u gain coerced from %u to %u"), i, pChanConfig[j].u32InputRange, CsHardwareChanConfig.u32InputRange);

		if (pChanConfig[j].u32Impedance != CsHardwareChanConfig.u32Impedance)
			_tprintf(_T("\nChannel %u impedance coerced from %u to %u"), i, pChanConfig[j].u32Impedance, CsHardwareChanConfig.u32Impedance);

		if (pChanConfig[j].i32DcOffset != CsHardwareChanConfig.i32DcOffset)
			_tprintf(_T("\nChannel %u DC offset coerced from %d to %d"), i, pChanConfig[j].i32DcOffset, CsHardwareChanConfig.i32DcOffset);

		if (pChanConfig[j].u32Filter != CsHardwareChanConfig.u32Filter)
			_tprintf(_T("\nChannel %u filter coerced from %u to %u"), i, pChanConfig[j].u32Filter, CsHardwareChanConfig.u32Filter);

		if (pChanConfig[j].u32Term != CsHardwareChanConfig.u32Term)
		{
			if ((pChanConfig[j].u32Term & CS_COUPLING_MASK) != (CsHardwareChanConfig.u32Term & CS_COUPLING_MASK))
				_tprintf(_T("\nChannel %u coupling coerced from %u to %u"), i, pChanConfig[j].u32Term & CS_COUPLING_MASK, CsHardwareChanConfig.u32Term & CS_COUPLING_MASK);

			if ((pChanConfig[j].u32Term & CS_DIFFERENTIAL_INPUT) != (CsHardwareChanConfig.u32Term & CS_DIFFERENTIAL_INPUT))
				_tprintf(_T("\nChannel %u diff input coerced from %u to %u"), i, pChanConfig[j].u32Term & CS_DIFFERENTIAL_INPUT, CsHardwareChanConfig.u32Term & CS_DIFFERENTIAL_INPUT);

			if ((pChanConfig[j].u32Term & CS_DIRECT_ADC_INPUT) != (CsHardwareChanConfig.u32Term & CS_DIRECT_ADC_INPUT))
				_tprintf(_T("\nChannel %u direct ADC coerced from %u to %u"), i, pChanConfig[j].u32Term & CS_DIRECT_ADC_INPUT, CsHardwareChanConfig.u32Term & CS_DIRECT_ADC_INPUT);
		}
	}
	return TRUE;
}


BOOL ReportTriggerChanges(CSHANDLE hSystem, CSTRIGGERCONFIG *pTrigConfig)
{
	CSTRIGGERCONFIG CsHardwareTrigConfig;
	int32 i32Status;

	CsHardwareTrigConfig.u32Size = sizeof(CSTRIGGERCONFIG);
	CsHardwareTrigConfig.u32TriggerIndex = 1;

	i32Status = CsGet(hSystem, CS_TRIGGER, CS_ACQUISITION_CONFIGURATION, &CsHardwareTrigConfig);
	if (CS_FAILED(i32Status))
	{
		_tprintf(_T("\nCannot compare trigger structures for trigger 1"));
		return FALSE;
	}

	if (pTrigConfig->i32Level != CsHardwareTrigConfig.i32Level)
		_tprintf(_T("\nTrigger level coerced from %d to %d"), pTrigConfig->i32Level, CsHardwareTrigConfig.i32Level);

	if (pTrigConfig->i32Source != CsHardwareTrigConfig.i32Source)
		_tprintf(_T("\nTrigger source coerced from %d to %d"), pTrigConfig->i32Source, CsHardwareTrigConfig.i32Source);

	if (pTrigConfig->u32Condition != CsHardwareTrigConfig.u32Condition)
		_tprintf(_T("\nTrigger condition coerced from %u to %u"), pTrigConfig->u32Condition, CsHardwareTrigConfig.u32Condition);

	if (pTrigConfig->u32ExtCoupling != CsHardwareTrigConfig.u32ExtCoupling)
		_tprintf(_T("\nTrigger coupling coerced from %u to %u"), pTrigConfig->u32ExtCoupling, CsHardwareTrigConfig.u32ExtCoupling);

	if (pTrigConfig->u32ExtTriggerRange != CsHardwareTrigConfig.u32ExtTriggerRange)
		_tprintf(_T("\nTrigger gain coerced from %u to %u"), pTrigConfig->u32ExtTriggerRange, CsHardwareTrigConfig.u32ExtTriggerRange);

	if (pTrigConfig->u32ExtImpedance != CsHardwareTrigConfig.u32ExtImpedance)
		_tprintf(_T("\nTrigger impedance coerced from %u to %u"), pTrigConfig->u32ExtImpedance, CsHardwareTrigConfig.u32ExtImpedance);

	if (pTrigConfig->u32Relation != CsHardwareTrigConfig.u32Relation)
		_tprintf(_T("\nTrigger relation coerced from %u to %u"), pTrigConfig->u32Relation, CsHardwareTrigConfig.u32Relation);

	return TRUE;
}
