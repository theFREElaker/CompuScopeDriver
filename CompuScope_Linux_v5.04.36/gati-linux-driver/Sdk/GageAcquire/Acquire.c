/////////////////////////////////////////////////////////////////////////
//
// GageAcquire
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
// GageAcquire demonstrates how to initialize, setup, acquire
// and transfer data using a CompuScope data acquisition system. Data from
// all channels are then saved to ASCII files as voltages.
//
///////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#endif

#include "CsTypes.h"
#include "CsPrototypes.h"
#include "CsAppSupport.h"
#include "CsTchar.h"
#include "CsSdkMisc.h"

int _tmain()
{
   int32                   	i32Status = CS_SUCCESS;
   uInt32                  	i;
   TCHAR                   	szFileName[MAX_PATH];
   int64					i64StartOffset = 0;
   void*                   	pBuffer = NULL;
   float*                  	pVBuffer = NULL;
   uInt32                  	u32Mode;
   CSHANDLE                	hSystem = 0;
   IN_PARAMS_TRANSFERDATA  	InData = {0};
   OUT_PARAMS_TRANSFERDATA 	OutData = {0};
   CSSYSTEMINFO            	CsSysInfo = {0};
   CSAPPLICATIONDATA       	CsAppData = {0};
   LPCTSTR                 	szIniFile = _T("Acquire.ini");
   FileHeaderStruct        	stHeader = {0};
   CSACQUISITIONCONFIG     	CsAcqCfg = {0};
   CSCHANNELCONFIG         	CsChanCfg = {0};
   uInt32                  	u32ChannelIndexIncrement;
   ARRAY_BOARDINFO         	*pArrayBoardInfo=NULL;

   int64					i64Padding = 64; //extra samples to capture to ensure we get what we ask for
   int64					i64SavedLength;
   int64					i64MaxLength;
   int64					i64MinSA;
   int64             		i64Status = 0;

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
      Get the system. This sample program only supports one system. If
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

   pArrayBoardInfo = VirtualAlloc (NULL, ((CsSysInfo.u32BoardCount - 1) * sizeof(CSBOARDINFO)) + sizeof(ARRAY_BOARDINFO), MEM_COMMIT, PAGE_READWRITE);
   if (!pArrayBoardInfo)
   {
      printf (_T("\nUnable to allocate memory\n"));
      CsFreeSystem(hSystem);
      return (-1);
   }
   pArrayBoardInfo->u32BoardCount = CsSysInfo.u32BoardCount;
   for (i = 0; i < pArrayBoardInfo->u32BoardCount; i++)
   {
      pArrayBoardInfo->aBoardInfo[i].u32BoardIndex = i + 1;
      pArrayBoardInfo->aBoardInfo[i].u32Size = sizeof(CSBOARDINFO);
   }
   i32Status = CsGet(hSystem, CS_BOARD_INFO, CS_ACQUISITION_CONFIGURATION, pArrayBoardInfo);

   /*
   Display the system name from the driver
   */
   printf(_T("\nBoard Name: %s"), CsSysInfo.strBoardName);
   for (i = 0; i < pArrayBoardInfo->u32BoardCount; i++)
   {
      printf(_T("\n\tSerial[%d]: %s"), i, pArrayBoardInfo->aBoardInfo[i].strSerialNumber);
   }
   printf(_T("\n"));

   i32Status = CsAs_ConfigureSystem(hSystem, (int)CsSysInfo.u32ChannelCount, 1, (LPCTSTR)szIniFile, &u32Mode);
   if (CS_FAILED(i32Status))
   {
      if (CS_INVALID_FILENAME == i32Status)
      {
         /*
            Display message but continue on using defaults.
            */
         printf(_T("\nCannot find %s - using default parameters."), szIniFile);
      }
      else
      {
         /*
            Otherwise the call failed.  If the call did fail we should free the CompuScope
            system so it's available for another application
            */
         DisplayErrorString(i32Status);
         CsFreeSystem(hSystem);
         VirtualFree (pArrayBoardInfo, 0, MEM_RELEASE);
         return(-1);
      }
   }
   /*
      If the return value is greater than  1, then either the application,
      acquisition, some of the Channel and / or some of the Trigger sections
      were missing from the ini file and the default parameters were used.
      */
   if (CS_USING_DEFAULT_ACQ_DATA & i32Status)
      printf(_T("\nNo ini entry for acquisition. Using defaults."));
   if (CS_USING_DEFAULT_CHANNEL_DATA & i32Status)
      printf(_T("\nNo ini entry for one or more Channels. Using defaults for missing items."));

   if (CS_USING_DEFAULT_TRIGGER_DATA & i32Status)
      printf(_T("\nNo ini entry for one or more Triggers. Using defaults for missing items."));

   i32Status = CsAs_LoadConfiguration(hSystem, szIniFile, APPLICATION_DATA, &CsAppData);
   if (CS_FAILED(i32Status))
   {
      if (CS_INVALID_FILENAME == i32Status)
      {
         printf(_T("\nUsing default application parameters."));
      }
      else
      {
         DisplayErrorString(i32Status);
         CsFreeSystem(hSystem);
         VirtualFree (pArrayBoardInfo, 0, MEM_RELEASE);
         return -1;
      }
   }
   else if (CS_USING_DEFAULT_APP_DATA & i32Status)
   {
      /*
         If the return value is CS_USING_DEFAULT_APP_DATA (defined in ConfigSystem.h)
         then there was no entry in the ini file for Application and we will use
         the application default values, which have already been set.
         */
      printf(_T("\nNo ini entry for application data. Using defaults."));
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
      VirtualFree (pArrayBoardInfo, 0, MEM_RELEASE);
      return (-1);
   }
   /*
      Get the current sample size, resolution and offset parameters from the driver
      by calling CsGet for the ACQUISTIONCONFIG structure. These values are used
      when saving the file.
   */
   CsAcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);
   i32Status = CsGet(hSystem, CS_ACQUISITION, CS_ACQUISITION_CONFIGURATION, &CsAcqCfg);
   if (CS_FAILED(i32Status))
   {
      DisplayErrorString(i32Status);
      CsFreeSystem(hSystem);
      VirtualFree (pArrayBoardInfo, 0, MEM_RELEASE);
      return (-1);
   }

   /*
      We need to allocate a buffer
      for transferring the data
   */
   pBuffer  = VirtualAlloc(NULL, (size_t)((CsAppData.i64TransferLength + i64Padding) * CsAcqCfg.u32SampleSize), MEM_COMMIT, PAGE_READWRITE);
   if (NULL == pBuffer)
   {
      printf (_T("\nUnable to allocate memory\n"));
      CsFreeSystem(hSystem);
      VirtualFree (pArrayBoardInfo, 0, MEM_RELEASE);
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
         printf (_T("\nUnable to allocate memory\n"));
         CsFreeSystem(hSystem);
         VirtualFree(pBuffer, 0, MEM_RELEASE);
         VirtualFree (pArrayBoardInfo, 0, MEM_RELEASE);
         return (-1);
      }
   }

   /*
      <===  If you want to perform a repetitive capture start your loop here.
      */


   /*
      Start the data acquisition
   */
   i32Status = CsDo(hSystem, ACTION_START);
   if (CS_FAILED(i32Status))
   {
      DisplayErrorString(i32Status);
      CsFreeSystem(hSystem);
      VirtualFree (pArrayBoardInfo, 0, MEM_RELEASE);
      return (-1);
   }

   if (!DataCaptureComplete(hSystem))
   {
      CsFreeSystem(hSystem);
      VirtualFree (pArrayBoardInfo, 0, MEM_RELEASE);
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

   /*
   Validate the start address and the length.  This is especially necessary if
   trigger delay is being used
   */
   i64MinSA = CsAcqCfg.i64TriggerDelay + CsAcqCfg.i64Depth - CsAcqCfg.i64SegmentSize;
   if (CsAppData.i64TransferStartPosition < i64MinSA)
   {
	   _ftprintf(stdout, _T("\nInvalid Start Address was changed from %lli to %lli\n"), (long long)CsAppData.i64TransferStartPosition, (long long)i64MinSA);
	   CsAppData.i64TransferStartPosition = i64MinSA;
   }

   i64MaxLength = CsAcqCfg.i64TriggerDelay + CsAcqCfg.i64Depth - i64MinSA;
   if (CsAppData.i64TransferLength > i64MaxLength)
   {
	   _ftprintf(stdout, _T("\nInvalid Transfer Length was changed from %lli to %lli\n"), (long long)CsAppData.i64TransferLength, (long long)i64MaxLength);
	   CsAppData.i64TransferLength = i64MaxLength;
   }

   InData.i64StartAddress = CsAppData.i64TransferStartPosition;
   /*
   	We transfer a little more than we need so we're sure to get what we requested, regardless of any hw alignment issuses
   */
   InData.i64Length =  CsAppData.i64TransferLength + i64Padding;
   InData.pDataBuffer = pBuffer;

   u32ChannelIndexIncrement = CsAs_CalculateChannelIndexIncrement(&CsAcqCfg, &CsSysInfo );

   for   (i = 1; i <= CsSysInfo.u32ChannelCount; i += u32ChannelIndexIncrement)
   {
      /*
         Variable that will contain either raw data or data in Volts depending on requested format
         */
      void* pSrcBuffer = NULL;

      ZeroMemory(pBuffer,(size_t)((CsAppData.i64TransferLength + i64Padding) * CsAcqCfg.u32SampleSize));
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
         VirtualFree (pArrayBoardInfo, 0, MEM_RELEASE);
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

         pSrcBuffer = (void *)pVBuffer;
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

      i64Status = (int64)CsAs_SaveFile(szFileName, pSrcBuffer, CsAppData.i32SaveFormat, &stHeader);
      if ( 0 > i64Status )
      {
         if (CS_MISC_ERROR == i64Status)
         {
            printf(_T("\n"));
            printf(_T("%s\n"), CsAs_GetLastFileError());
         }
         else
         {
            DisplayErrorString(i64Status);
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
   VirtualFree (pArrayBoardInfo, 0, MEM_RELEASE);
   return 0;
}
