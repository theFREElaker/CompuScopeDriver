/////////////////////////////////////////////////////////////////////////
//
// GageEvents
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

#include "Events.h"

#if 0
int SaveDataToFiles(void *arg);
#endif

#ifdef _WIN32
DWORD WINAPI ThreadWaitForEvents(void *arg)
#else
void* ThreadWaitForEvents(void *arg)
#endif
{
   uInt32			i;
   int32			i32Status = 0;
   void*			pBuffer = NULL;
   float*			pVBuffer = NULL;
   void*			pSrcBuffer = NULL;
   size_t			AllocSize = 0;
   TCHAR			szFileName[MAX_PATH];

   PTHREADDATA		pData = {0};

   IN_PARAMS_TRANSFERDATA		InParams = {0};
   OUT_PARAMS_TRANSFERDATA		OutParams = {0};
   FileHeaderStruct				stHeader = {0};
   CSACQUISITIONCONFIG			CsAcqCfg = {0};
   CSCHANNELCONFIG				CsChanCfg = {0};
   uInt32						u32MaskedMode;
   uInt32						u32ChannelIndexIncrement;
   uInt32						u32ChannelsPerBoard;
   int64						i64Padding = 64; //extra samples to capture to ensure we get what we ask for
   int64						i64SavedLength;

   pData = (PTHREADDATA)arg;

#ifdef _WIN32
   HANDLE			hWaitObjects[3] = {0};
   hWaitObjects[0] = pData->hAborted;
   hWaitObjects[1] = pData->hTriggered;
   hWaitObjects[2] = pData->hEndOfAcquisition;
   DWORD ret = 1;
#else
   struct pollfd pollfds[3];
   char rd_sink;
   long ret = 0;
   pollfds[0].events = POLLIN;
   pollfds[1].events = POLLIN;
   pollfds[2].events = POLLIN;
   pollfds[0].fd = pData->fd_abort_read;
   pollfds[1].fd = pData->fd_triggered_read;
   pollfds[2].fd = pData->fd_end_acq_read;
#endif /* _WIN32 */

   /* Get the current sample size, resolution and offset parameters from the driver
      by calling CsGet for the ACQUISTIONCONFIG structure. These values are used
      when saving the file and allocating the buffer.  */
   CsAcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);
   i32Status = CsGet(g_hSystem, CS_ACQUISITION, CS_ACQUISITION_CONFIGURATION, &CsAcqCfg);
   if (CS_FAILED(i32Status))
   {
      DisplayErrorString(i32Status);
#ifdef _WIN32
      return 1;
#else
      return (void*) 1;
#endif /* _WIN32 */
   }

   AllocSize = (size_t)((pData->TransferLength + i64Padding) * CsAcqCfg.u32SampleSize);
   pBuffer = VirtualAlloc(NULL, AllocSize, MEM_COMMIT, PAGE_READWRITE);
   if (NULL == pBuffer)
   {
      DisplayErrorString(CS_MEMORY_ERROR);
#ifdef _WIN32
      return 1;
#else
      return (void*) 1;
#endif /* _WIN32 */
   }
   if (TYPE_FLOAT == pData->i32SaveFormat)
   {
      /* Allocate another buffer to pass the data that is going to be converted
         into voltages */
      pVBuffer  = (float *)VirtualAlloc(NULL, (size_t)(pData->TransferLength * sizeof(float)), MEM_COMMIT, PAGE_READWRITE);
      if (NULL == pVBuffer)
      {
         DisplayErrorString(CS_MEMORY_ERROR);
         VirtualFree(pBuffer, 0, MEM_RELEASE);
#ifdef _WIN32
         return 1;
#else
         return (void*) 1;
#endif /* _WIN32 */
      }
   }

   for(;;)
   {
      // Wait for events to be signaled ....
#ifdef _WIN32
      DWORD dwWaitRet = WaitForMultipleObjects(3, hWaitObjects, FALSE, INFINITE);
      if ( WAIT_OBJECT_0 == dwWaitRet )
         break;		// Aborted by user

      if ( WAIT_OBJECT_0+1 == dwWaitRet )
      {
         // Triggered Event
         _ftprintf(stdout, _T("\tReceived TRIGGERED Event...\n"));
         ResetEvent(pData->hTriggered); // Trigger event has to be manually resetted.
      }

      if ( WAIT_OBJECT_0+2 == dwWaitRet )
      {
         ResetEvent(pData->hEndOfAcquisition);
#else
         poll(pollfds, 3, -1);
         if (pollfds[0].revents & POLLIN) {
            read(pData->fd_abort_read, &rd_sink, 1);
            ret = 1;
            goto end_free_buffers;
         }
         if (pollfds[1].revents & POLLIN) {
            _ftprintf(stdout, _T("\tReceived TRIGGERED Event...\n"));
            read(pData->fd_triggered_read, &rd_sink, 1);
         }
         if (pollfds[2].revents & POLLIN) {
            read(pData->fd_end_acq_read, &rd_sink, 1);
#endif /* _WIN32 */
            // End of Acquisition Event
            _ftprintf(stdout, _T("\tReceived EndOfAcquisition Event ...\n"));

            u32MaskedMode = max( 1, (CsAcqCfg.u32Mode & CS_MASKED_MODE));
            u32ChannelsPerBoard = pData->u32ChannelCount / pData->u32BoardCount;
            u32ChannelIndexIncrement = max( 1, u32ChannelsPerBoard / u32MaskedMode );


            InParams.u32Segment		= 1;
            InParams.u32Mode		= TxMODE_DEFAULT;
            InParams.i64StartAddress = pData->TransferStart;;
            InParams.i64Length		= pData->TransferLength + i64Padding;
            InParams.hNotifyEvent	= NULL;
            InParams.pDataBuffer	= pBuffer;

            for	(i = 1; i <= pData->u32ChannelCount; i += u32ChannelIndexIncrement)
            {
               int64 i64StartOffset = 0;
               /* Check if the application was aborted between each transfer channel.
                  This way, the abort will be catched at this level and resetted to be catch above */
#ifdef _WIN32
               if (WAIT_OBJECT_0 == WaitForSingleObject(hWaitObjects[0], 0))
                  break;
#else
               poll(&pollfds[0], 1, 0);
               if (pollfds[0].revents & POLLIN) {
                  ret = 1;
                  goto end_free_buffers;
               }
#endif /* _WIN32 */

               /* Transfer the requested channel and perform a quick analysis on the returned buffer */
               InParams.u16Channel = (uInt16)i;

               i32Status = CsTransfer(g_hSystem, &InParams, &OutParams);
               if (CS_FAILED(i32Status))
               {
                  DisplayErrorString(i32Status);
                  break;
               }
               /* Get the channel config so we have the input range for the header
                  or for conversion to volts */
               CsChanCfg.u32Size = sizeof(CSCHANNELCONFIG);
               CsChanCfg.u32ChannelIndex = i;
               CsGet(g_hSystem, CS_CHANNEL, CS_ACQUISITION_CONFIGURATION, &CsChanCfg);

               /* Save the smaller of the requested transfer length or the actual transferred length */
               i64StartOffset = InParams.i64StartAddress - OutParams.i64ActualStart;
               if (i64StartOffset < 0)
               {
                  i64StartOffset = 0;
                  InParams.i64StartAddress = OutParams.i64ActualStart;
               }
               i64SavedLength = pData->TransferLength <= OutParams.i64ActualLength ? pData->TransferLength : OutParams.i64ActualLength;
               /* Because of alignment issues, adjust the length by the difference between the requested and actual start.  This is so we
                  start saving the file at the requested start address (if possible) rather than the actual start.  */
               i64SavedLength -= i64StartOffset;
               if (TYPE_FLOAT == pData->i32SaveFormat)
               {
                  /* Call the ConvertToVolts function. This function will convert the raw
                     data to voltages. We pass the buffer plus the actual start, which will be converted
                     from 0 to actual length in the volts buffer.  */
                  i32Status = CsAs_ConvertToVolts(i64SavedLength, CsChanCfg.u32InputRange, CsAcqCfg.u32SampleSize,
                                                  CsAcqCfg.i32SampleOffset, CsAcqCfg.i32SampleRes,
                                                  CsChanCfg.i32DcOffset, (void *)((unsigned char *)pBuffer + (i64StartOffset * CsAcqCfg.u32SampleSize)) ,
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

               /* Note: to optimize the transfer loop, everything from
                  this point on in the loop could be moved out and done
                  after all the channels are transferred.  */
               /* The driver may have had to change the start address and length
                  due to alignment issues, so we'll get the actual start and length
                  from the driver.  */
               _stprintf(szFileName, _T("%s_CH%d.dat"), pData->lpszSaveFileName, i);
               /*
                  Gather up the information needed for the file header
                  */
               stHeader.i64SampleRate = CsAcqCfg.i64SampleRate;
               stHeader.i64Start = InParams.i64StartAddress;
               stHeader.i64Length = i64SavedLength;
               stHeader.u32SampleSize = CsAcqCfg.u32SampleSize;
               stHeader.i32SampleRes = CsAcqCfg.i32SampleRes;
               stHeader.i32SampleOffset = CsAcqCfg.i32SampleOffset;
               stHeader.u32InputRange = CsChanCfg.u32InputRange;
               stHeader.i32DcOffset = CsChanCfg.i32DcOffset;
               // For sig files we treat each multiple record segment of the capture as a separate file
               stHeader.u32SegmentCount = CsAcqCfg.u32SegmentCount;
               stHeader.u32SegmentNumber = InParams.u32Segment;
               stHeader.dTimeStamp = NO_TIME_STAMP_VALUE;

               /* Ctrl-C (Abort) command will not work inside the following function.
                  The SaveFile function was created to save an ASCII file from a given buffer.
                  Hence, on a big buffer, the app might time some toime to abort until the SaveFile is completed.  */
               i32Status = (int32)CsAs_SaveFile(szFileName, pSrcBuffer, pData->i32SaveFormat, &stHeader);
               if ( 0 > i32Status )
               {
                  if (CS_MISC_ERROR == i32Status)
                  {
                     _ftprintf(stderr, _T("\nFile Error: "));
                     _ftprintf(stderr, "%s", CsAs_GetLastFileError());
                  }
                  else
                  {
                     DisplayErrorString(i32Status);
                  }
               }
            }
            break;
         }
      }

end_free_buffers:
      if (NULL != pBuffer)
      {
         VirtualFree(pBuffer, 0, MEM_RELEASE);
      }
      if (NULL != pVBuffer)
      {
         VirtualFree(pVBuffer, 0, MEM_RELEASE);
      }

#ifdef _WIN32
      SetEvent(pData->hDataProcessingCompleted);
      return ret;
#else
      return (void*) ret;
#endif /* _WIN32 */
   }


#ifdef _WIN32
   int SaveDataToFiles(void *arg)
   {
      uInt32			i;
      HANDLE			hWaitObjects[3] = {0};
      int32			i32Status = 0;
      void*			pBuffer = NULL;
      float*			pVBuffer = NULL;
      void*			pSrcBuffer = NULL;
      size_t			AllocSize = 0;
      TCHAR			szFileName[MAX_PATH];

      PTHREADDATA		pData = {0};

      IN_PARAMS_TRANSFERDATA		InParams = {0};
      OUT_PARAMS_TRANSFERDATA		OutParams = {0};
      FileHeaderStruct			stHeader = {0};
      CSACQUISITIONCONFIG			CsAcqCfg = {0};
      CSCHANNELCONFIG				CsChanCfg = {0};
      uInt32						u32MaskedMode;
      uInt32						u32ChannelIndexIncrement;
      uInt32						u32ChannelsPerBoard;
      int64						i64Padding = 64; //extra samples to capture to ensure we get what we ask for
      int64						i64SavedLength;


      pData = (PTHREADDATA)arg;

      hWaitObjects[0] = pData->hAborted;
      hWaitObjects[1] = pData->hTriggered;
      hWaitObjects[2] = pData->hEndOfAcquisition;
      /* Get the current sample size, resolution and offset parameters from the driver
         by calling CsGet for the ACQUISTIONCONFIG structure. These values are used
         when saving the file and allocating the buffer.  */
      CsAcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);
      i32Status = CsGet(g_hSystem, CS_ACQUISITION, CS_ACQUISITION_CONFIGURATION, &CsAcqCfg);
      if (CS_FAILED(i32Status))
      {
         DisplayErrorString(i32Status);
         return 1;
      }

      AllocSize = (size_t)((pData->TransferLength + i64Padding) * CsAcqCfg.u32SampleSize);
      pBuffer = VirtualAlloc(NULL, AllocSize, MEM_COMMIT, PAGE_READWRITE);
      if (NULL == pBuffer)
      {
         DisplayErrorString(CS_MEMORY_ERROR);
         return 1;
      }
      if (TYPE_FLOAT == pData->i32SaveFormat)
      {
         /* Allocate another buffer to pass the data that is going to be converted
            into voltages */
         pVBuffer  = (float *)VirtualAlloc(NULL, (size_t)(pData->TransferLength * sizeof(float)), MEM_COMMIT, PAGE_READWRITE);
         if (NULL == pVBuffer)
         {
            DisplayErrorString(CS_MEMORY_ERROR);
            VirtualFree(pBuffer, 0, MEM_RELEASE);
            return 1;
         }
      }

      for(;;)
      {
         // Wait for events to be signaled ....
         DWORD dwWaitRet = WaitForMultipleObjects(3, hWaitObjects, FALSE, INFINITE);

         if ( WAIT_OBJECT_0 == dwWaitRet )
            break;		// Aborted by user

         if ( WAIT_OBJECT_0+1 == dwWaitRet )
         {
            // Triggered Event
            _ftprintf(stdout, _T("\tReceived TRIGGERED Event...\n"));
            ResetEvent(pData->hTriggered); // Trigger event has to be manually resetted.
         }

         if ( WAIT_OBJECT_0+2 == dwWaitRet )
         {
            // End of Acquisition Event
            _ftprintf(stdout, _T("\tRecieved EndOfAcquisition Event ...\n"));
            ResetEvent(pData->hEndOfAcquisition);

            u32MaskedMode = max( 1, (CsAcqCfg.u32Mode & CS_MASKED_MODE));
            u32ChannelsPerBoard = pData->u32ChannelCount / pData->u32BoardCount;
            u32ChannelIndexIncrement = max( 1, u32ChannelsPerBoard / u32MaskedMode );


            InParams.u32Segment		= 1;
            InParams.u32Mode		= TxMODE_DEFAULT;
            InParams.i64StartAddress = pData->TransferStart;;
            InParams.i64Length		= pData->TransferLength + i64Padding;
            InParams.hNotifyEvent	= NULL;
            InParams.pDataBuffer	= pBuffer;

            for	(i = 1; i <= pData->u32ChannelCount; i += u32ChannelIndexIncrement)
            {
               int64 i64StartOffset = 0;
               /* Check if the application was aborted between each transfer channel.
                  This way, the abort will be catched at this level and resetted to be catch above */
               if (WAIT_OBJECT_0 == WaitForSingleObject(hWaitObjects[0], 0))
                  break;

               /* Transfer the requested channel and perform a quick analysis on the returned buffer */
               InParams.u16Channel = (uInt16)i;

               i32Status = CsTransfer(g_hSystem, &InParams, &OutParams);
               if (CS_FAILED(i32Status))
               {
                  DisplayErrorString(i32Status);
                  break;
               }
               /* Get the channel config so we have the input range for the header
                  or for conversion to volts */
               CsChanCfg.u32Size = sizeof(CSCHANNELCONFIG);
               CsChanCfg.u32ChannelIndex = i;
               CsGet(g_hSystem, CS_CHANNEL, CS_ACQUISITION_CONFIGURATION, &CsChanCfg);

               /* Save the smaller of the requested transfer length or the actual transferred length */
               i64StartOffset = InParams.i64StartAddress - OutParams.i64ActualStart;
               if (i64StartOffset < 0)
               {
                  i64StartOffset = 0;
                  InParams.i64StartAddress = OutParams.i64ActualStart;
               }
               i64SavedLength = pData->TransferLength <= OutParams.i64ActualLength ? pData->TransferLength : OutParams.i64ActualLength;
               /* Because of alignment issues, adjust the length by the difference between the requested and actual start.  This is so we
                  start saving the file at the requested start address (if possible) rather than the actual start.  */
               i64SavedLength -= i64StartOffset;
               if (TYPE_FLOAT == pData->i32SaveFormat)
               {
                  /* Call the ConvertToVolts function. This function will convert the raw
                     data to voltages. We pass the buffer plus the actual start, which will be converted
                     from 0 to actual length in the volts buffer.  */
                  i32Status = CsAs_ConvertToVolts(i64SavedLength, CsChanCfg.u32InputRange, CsAcqCfg.u32SampleSize,
                                                  CsAcqCfg.i32SampleOffset, CsAcqCfg.i32SampleRes,
                                                  CsChanCfg.i32DcOffset, (void *)((unsigned char *)pBuffer + (i64StartOffset * CsAcqCfg.u32SampleSize)) ,
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
               /* Note: to optimize the transfer loop, everything from
                  this point on in the loop could be moved out and done
                  after all the channels are transferred.  */
               /* The driver may have had to change the start address and length
                  due to alignment issues, so we'll get the actual start and length
                  from the driver.  */
               _stprintf(szFileName, _T("%s_CH%d.dat"), pData->lpszSaveFileName, i);
               /*
                  Gather up the information needed for the file header
                  */
               stHeader.i64SampleRate = CsAcqCfg.i64SampleRate;
               stHeader.i64Start = InParams.i64StartAddress;
               stHeader.i64Length = i64SavedLength;
               stHeader.u32SampleSize = CsAcqCfg.u32SampleSize;
               stHeader.i32SampleRes = CsAcqCfg.i32SampleRes;
               stHeader.i32SampleOffset = CsAcqCfg.i32SampleOffset;
               stHeader.u32InputRange = CsChanCfg.u32InputRange;
               stHeader.i32DcOffset = CsChanCfg.i32DcOffset;
               // For sig files we treat each multiple record segment of the capture as a separate file
               stHeader.u32SegmentCount = CsAcqCfg.u32SegmentCount;
               stHeader.u32SegmentNumber = InParams.u32Segment;
               stHeader.dTimeStamp = NO_TIME_STAMP_VALUE;

               /* Ctrl-C (Abort) command will not work inside the following function.
                  The SaveFile function was created to save an ASCII file from a given buffer.
                  Hence, on a big buffer, the app might time some toime to abort until the SaveFile is completed.  */
               i32Status = (int32)CsAs_SaveFile(szFileName, pSrcBuffer, pData->i32SaveFormat, &stHeader);
               if ( 0 > i32Status )
               {
                  if (CS_MISC_ERROR == i32Status)
                  {
                     _ftprintf(stderr, _T("\nFile Error: "));
                     _ftprintf(stderr, CsAs_GetLastFileError());
                  }
                  else
                  {
                     DisplayErrorString(i32Status);
                  }
               }
            }
            break;
         }
      }

      if (NULL != pBuffer)
      {
         VirtualFree(pBuffer, 0, MEM_RELEASE);
      }
      if (NULL != pVBuffer)
      {
         VirtualFree(pVBuffer, 0, MEM_RELEASE);
      }

      SetEvent(pData->hDataProcessingCompleted);

      return (1);
   }
#endif /* _WIN32 */
