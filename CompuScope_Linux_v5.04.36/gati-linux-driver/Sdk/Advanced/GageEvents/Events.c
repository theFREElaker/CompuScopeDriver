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
//
// GageEvents demonstrates how to initialize, setup, acquire
// and transfer data using a CompuScope data acquisition system.
// Instead of polling for the status of the system, this sample
// will wait for specific events: Trigger and End of Acquisition.
// This sample contains 1 main thread and 1 events thread.
//
// The event thread will waits for Triggered event and End of Acquisition event
// then transfer the data from the first segment. Data from all valid channels
// are then saved to files.
//
///////////////////////////////////////////////////////////////////////////

#include "Events.h"

CSHANDLE	g_hSystem = 0;

#ifndef _WIN32
int			pipe_fd_abort[2];
pthread_t		thread;
#endif /* ndef _WIN32 */

int _tmain()
{
   int32			i32Status = CS_SUCCESS;
   LPCTSTR			szIniFile = _T("Events.ini");
   uInt32			u32Mode;
#ifdef _WIN32
   DWORD			dwWaitRet = 0;
   HANDLE			hWaitHandles[2]	= {0};
   HANDLE			phThread		= NULL;
   HANDLE			hTriggerEvent	= NULL;
   HANDLE			hEndAcqEvent	= NULL;
   HANDLE			hAborted		= NULL;
   HANDLE			hDataProcessingCompleted = NULL;
#else
   int				thread_exit_status;
#endif /* _WIN32 */
   THREADDATA		ThreadData		= {0};

   CSSYSTEMINFO				CsSysInfo = {0};
   CSAPPLICATIONDATA			CsAppData = {0};
   CSACQUISITIONCONFIG			CsAcqCfg = {0};

   int64			i64MinSA = 0, i64MaxLength = 0;

   /*
      Create manually-resetted events for cancelling and abort the application
      */
#ifdef _WIN32
   hAborted = CreateEvent(NULL, TRUE, FALSE, PGM_ABORTED);
   hDataProcessingCompleted = CreateEvent(NULL, TRUE, FALSE, PGM_COMPLETED);
   if ( NULL == hAborted || NULL == hDataProcessingCompleted )
   {
      _ftprintf(stdout, _T("\nError: Cannot create user events !!!"));
      return (-1);
   }
#else
   /* Create our pipe */
   if (pipe(pipe_fd_abort) != 0) {
      fprintf(stderr, "Error: Cannot create termination pipe\n");
      return -1;
   }
   ThreadData.fd_abort_read = pipe_fd_abort[0];
#endif /* _WIN32 */

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

   /* Get the system. This sample program only supports one system. If
      2 systems or more are found, the first system that is found
      will be the system that will be used. hSystem will hold a unique
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

   /* Display the system name from the driver
      */
   _ftprintf(stdout, _T("\nBoard Name: %s"), CsSysInfo.strBoardName);
   _ftprintf(stdout, "\n\nPress Ctrl-C to abort and quit if needed\n\n");

   /*
      In this example we're only using 1 trigger source
      */
   i32Status = CsAs_ConfigureSystem(g_hSystem, (int)CsSysInfo.u32ChannelCount, 1, (LPCTSTR)szIniFile, &u32Mode);

   if (CS_FAILED(i32Status))
   {
      if (CS_INVALID_FILENAME == i32Status)
      {
         /* Display message but continue on using defaults.  */
         _ftprintf(stdout, _T("\nCannot find %s - using default parameters."), szIniFile);
      }
      else
      {
         /* Otherwise the call failed.  If the call did fail we should free the CompuScope
            system so it's available for another application */
         DisplayErrorString(i32Status);
         CsFreeSystem(g_hSystem);
         return(-1);
      }
   }
   /* If the return value is greater than  1, then either the application, acquisition, some of the Channel and / or some of the Trigger sections
      were missing from the ini file and the default parameters were used.  */
   if (CS_USING_DEFAULT_ACQ_DATA & i32Status)
      _ftprintf(stdout, _T("\nNo ini entry for acquisition. Using defaults."));

   if (CS_USING_DEFAULT_CHANNEL_DATA & i32Status)
      _ftprintf(stdout, _T("\nNo ini entry for one or more Channels. Using defaults for missing items."));

   if (CS_USING_DEFAULT_TRIGGER_DATA & i32Status)
      _ftprintf(stdout, _T("\nNo ini entry for one or more Triggers. Using defaults for missing items."));

   i32Status = CsAs_LoadConfiguration(g_hSystem, szIniFile, APPLICATION_DATA, &CsAppData);

   if (CS_FAILED(i32Status))
   {
      if (CS_INVALID_FILENAME == i32Status)
         _ftprintf(stdout, _T("\nUsing default application parameters."));
      else
      {
         DisplayErrorString(i32Status);
         CsFreeSystem(g_hSystem);
         return -1;
      }
   }
   else if (CS_USING_DEFAULT_APP_DATA & i32Status)
   {
      /* If the return value is CS_USING_DEFAULT_APP_DATA (defined in ConfigSystem.h)
         then there was no entry in the ini file for Application and we will use
         the application default values, which were set in CsAs_LoadConfiguration.  */
      _ftprintf(stdout, _T("\nNo ini entry for application data. Using defaults"));
   }

   /* Commit the values to the driver.  This is where the values get sent to the
      hardware.  Any invalid parameters will be caught here and an error returned.  */
   i32Status = CsDo(g_hSystem, ACTION_COMMIT);
   if (CS_FAILED(i32Status))
   {
      DisplayErrorString(i32Status);
      CsFreeSystem(g_hSystem);
      return (-1);
   }

   /* Handle termination with Ctrl+C */
#ifdef _WIN32
   SetConsoleCtrlHandler( ControlHandler, TRUE );
#else
   signal(SIGINT, signal_handler);
#endif /* _WIN32 */

   /* Get the event Triggered and End of Acquisition from the driver */
#ifdef _WIN32
   i32Status = CsGetEventHandle(g_hSystem, ACQ_EVENT_TRIGGERED, &hTriggerEvent);
#else
   i32Status = CsGetEventHandle(g_hSystem, ACQ_EVENT_TRIGGERED, &ThreadData.fd_triggered_read);
#endif /* _WIN32 */
   if (CS_FAILED(i32Status))
   {
      DisplayErrorString(i32Status);
      CsFreeSystem(g_hSystem);
      return (-11);
   }

#ifdef _WIN32
   i32Status = CsGetEventHandle(g_hSystem, ACQ_EVENT_END_BUSY, &hEndAcqEvent);
#else
   i32Status = CsGetEventHandle(g_hSystem, ACQ_EVENT_END_BUSY, &ThreadData.fd_end_acq_read);
#endif /* _WIN32 */
   if (CS_FAILED(i32Status))
   {
      DisplayErrorString(i32Status);
      CsFreeSystem(g_hSystem);
      return (-11);
   }
   /* Creates the Trigger thread and the Transfer thread.
      The ThreadData structure contains the handle of Wait elements to permit a clean exit
      if using Ctrl-C or the requested number of acquisition has been performed.  */
#ifdef _WIN32
   ThreadData.hAborted				= hAborted;
   ThreadData.hTriggered			= hTriggerEvent;
   ThreadData.hEndOfAcquisition	= hEndAcqEvent;
   ThreadData.hDataProcessingCompleted	= hDataProcessingCompleted;
#endif /* _WIN32 */
   ThreadData.u32Mode			= u32Mode;
   ThreadData.u32BoardCount	= CsSysInfo.u32BoardCount;
   ThreadData.u32ChannelCount	= CsSysInfo.u32ChannelCount;
   ThreadData.SampleSize		= CsSysInfo.u32SampleSize;

   /* Because the Trigger thread will simulate the same behavior as the TriggerTimeOut functionnality,
      we will disable (-1) if it is not already disabled before calling the Commit. Even if the ini file,
      was enabling it for a given amout of time. The Wait period before the trigger can now be set by
      setting USER_DEFINED_TRIGGER_TIMEOUT in Threads.c with the desired value.  */
   CsAcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);

   i32Status = CsGet(g_hSystem, CS_ACQUISITION, CS_CURRENT_CONFIGURATION, &CsAcqCfg);
   if (CS_FAILED(i32Status))
   {
      DisplayErrorString(i32Status);
      CsFreeSystem(g_hSystem);
      return (-1);
   }

   /* Validate the start address and the length.  This is especially necessary if
      trigger delay is being used */
   i64MinSA = CsAcqCfg.i64TriggerDelay + CsAcqCfg.i64Depth - CsAcqCfg.i64SegmentSize;
   if (CsAppData.i64TransferStartPosition < i64MinSA)
   {
      _ftprintf(stdout, _T("\nInvalid Start Address was changed from " F64 " to " F64 "\n"), CsAppData.i64TransferStartPosition, i64MinSA);
      CsAppData.i64TransferStartPosition = i64MinSA;
   }
   i64MaxLength = CsAcqCfg.i64TriggerDelay + CsAcqCfg.i64Depth - i64MinSA;
   if (CsAppData.i64TransferLength > i64MaxLength)
   {
      _ftprintf(stdout, _T("\nInvalid Transfer Length was changed from " F64 " to " F64 "\n"), CsAppData.i64TransferLength, i64MaxLength);
      CsAppData.i64TransferLength = i64MaxLength;
   }

   ThreadData.TransferLength = CsAppData.i64TransferLength;
   ThreadData.TransferStart = CsAppData.i64TransferStartPosition;
   ThreadData.i32SaveFormat = CsAppData.i32SaveFormat;
   _tcsncpy(ThreadData.lpszSaveFileName, CsAppData.lpszSaveFileName, 100);

#ifdef _WIN32
   phThread = CreateThread(NULL, 0, ThreadWaitForEvents, (void*)&ThreadData, 0, NULL);
#else
   pthread_create(&thread, NULL, ThreadWaitForEvents, (void*)&ThreadData);
#endif /* _WIN32 */

   /* Start the data acquisition */
   i32Status = CsDo(g_hSystem, ACTION_START);
   if (CS_FAILED(i32Status))
   {
      DisplayErrorString(i32Status);
      CsFreeSystem(g_hSystem);
      return (-1);
   }

   /* Waiting on either the aborted command or the completion of the WaitForEvents thread
      Display the program results based on the Wait object returned.  */
#ifdef _WIN32
   hWaitHandles[0] = hAborted;
   hWaitHandles[1] = hDataProcessingCompleted;
   dwWaitRet = WaitForMultipleObjects(2, hWaitHandles, FALSE, INFINITE);
   if (dwWaitRet == WAIT_OBJECT_0)
   {
      _ftprintf(stdout, _T("\tApplication Aborted\n"));
      _ftprintf (stdout, _T("\tSome channels could have been saved in the current working directory.\n"));
   }
   else
   {
      _ftprintf(stdout, _T("\tApplication Completed\n"));
      _ftprintf (stdout, _T("\tAll channels are saved in the current working directory.\n"));
   }
#else
   pthread_join(thread, (void*) &thread_exit_status);
   if (!thread_exit_status) {
      _ftprintf(stdout, _T("\tApplication Completed\n"));
      _ftprintf (stdout, _T("\tAll channels are saved in the current working directory.\n"));
   } else {
      _ftprintf(stdout, _T("\tApplication Aborted\n"));
      _ftprintf (stdout, _T("\tSome channels could have been saved in the current working directory.\n"));
      //CsDo(g_hSystem, ACTION_ABORT);
   }
#endif /* _WIN32 */


   /* Close handle that have been created with CreateEvent
      Close allocated resources: threads
      Free the CompuScope system and any resources it's been using */
#ifdef _WIN32
   CloseHandle(hAborted);
   CloseHandle(hDataProcessingCompleted);
   CloseHandle(phThread);
#else
   /* Close our pipe's file descriptors */
   close(pipe_fd_abort[0]);
   close(pipe_fd_abort[1]);
#endif /* _WIN32 */

   CsFreeSystem(g_hSystem);
   return (1);
}

#ifdef _WIN32
BOOL WINAPI ControlHandler ( DWORD dwCtrlType )
{
   switch( dwCtrlType )
   {
      case CTRL_BREAK_EVENT:
      case CTRL_C_EVENT:
         {
            HANDLE hAbort = NULL;
            /* Retrieve the aborted pgm event and signaled it
               in order to kill all running threads */
            hAbort = OpenEvent(EVENT_ALL_ACCESS, TRUE, PGM_ABORTED);
            if (hAbort)
            {
               SetEvent(hAbort);
               CsDo(g_hSystem, ACTION_ABORT);
            }
            else
            {
               DWORD dwError = GetLastError();
               _ftprintf(stdout, _T("\tError encountered:%i\n"), dwError);
            }

            return (TRUE);
         }
   }

   return (FALSE);
}
#else
void signal_handler(int sig)
{
   /* Handles Ctrl+C in terminal */
   write(pipe_fd_abort[1], "!", 1);
   CsDo(g_hSystem, ACTION_ABORT);
   pthread_cancel(thread);
}
#endif /* _WIN32 */
