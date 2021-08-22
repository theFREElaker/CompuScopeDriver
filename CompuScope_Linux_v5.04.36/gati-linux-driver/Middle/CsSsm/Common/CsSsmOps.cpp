#include "StdAfx.h"
#include "CsSsm.h"

#if defined(_WIN32)
#include "CsExpert.h"

#include <io.h>   // For access().
#include <sys/types.h>  // For stat().
#include <sys/stat.h>   // For stat().
#include <direct.h>		// for _getdrive, etc
#include "SSMmsglog.h"

const TCHAR* szEventLogName = _T("GageDiskStream");

const int InvalidAsciiChars[36] = { 34, 60, 62, 124, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	       						  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
								  25, 26, 27, 28, 29, 30, 31 };


VOID AddToMessageLog(DWORD dwErrorID, TCHAR* szErrorMsg)
{
	LPTSTR lpszStrings[2];
	WORD wEventType(EVENTLOG_SUCCESS), wMessageCount(1);
	
	switch ( dwErrorID>>30 )
	{
		case STATUS_SEVERITY_ERROR:         wEventType = EVENTLOG_ERROR_TYPE; break;
		case STATUS_SEVERITY_WARNING:       wEventType = EVENTLOG_WARNING_TYPE; break;
		case STATUS_SEVERITY_INFORMATIONAL: wEventType = EVENTLOG_INFORMATION_TYPE; break;
		case STATUS_SEVERITY_SUCCESS:       wEventType = EVENTLOG_SUCCESS; wMessageCount = 0; break;
	}
	lpszStrings[0] = szErrorMsg;

	HANDLE hEventSource = ::RegisterEventSource(NULL, TEXT(szEventLogName));
	if (hEventSource != NULL) 
	{
		::ReportEvent(hEventSource,				// handle of event source
					  wEventType,				// event type
					  0,						// event category
					  dwErrorID,				// event ID
					  NULL,						// current user's SID
					  wMessageCount,			// strings in lpszStrings
					  0,						// no bytes of raw data
					  (LPCSTR *)lpszStrings,	// array of error strings
					  NULL);					// no raw data

		::DeregisterEventSource(hEventSource);
	}
}

BOOL IsValidPathName(TCHAR* szName)
{
	if (::_access(szName, 0) == 0)
	{
		// if folder exists and is not a file, then it must be valid
		struct stat status;
		::stat(szName, &status);
		return (status.st_mode & S_IFDIR) ? TRUE : FALSE;
	}
	else 
	{
#ifdef _WIN32						
		if (NULL != ::_tcspbrk(szName, _T(":")))
		{
			// then the drive letter should be the 1st letter, check if it's valid
					
			int drive = ::_totupper(szName[0]) - 'A' + 1;
			int curdrive = ::_getdrive(); 
			if ( -1 == ::_chdrive(drive))
			{
				_chdrive(curdrive);
				return FALSE;
			}
			// restore original drive
			_chdrive(curdrive);
		}
#endif 
		// folder doesn't exist so lets do the best we can do determine if 
		// there are invalid characters in the string. Note: an empty string is valid
		
		// any colon has to be the second character. so the return value of _tcscpn should be either 1 if there's
		// a colon (absolute path) or the length of the string (relative path so no colon).
		int nPos = int(::_tcscspn(szName, _T(":")));
		if (1 != nPos && lstrlen(szName) != nPos)
		{
			return FALSE;
		}

		// check if any of these ascii codes are in the directory string.
		for ( int i = 0; i < 36; i++)
		{
			TCHAR* p = ::_tcschr(szName, InvalidAsciiChars[i]);
			// note that \0 can appear at the end of a string and this is legal
			if (NULL != p && p != (szName + lstrlen(szName)))
			{
				return FALSE;
			}
		}
		return TRUE;
	}
}

#endif

//!
//! \ingroup SSM_API
//!

//! \fn int32 SSM_API CsDo(CSHANDLE hSystem, int16 i16Operation);
//!	Performs an operation on the CompuScope system
//!
//! Except for data transfer methods, this is the only API method that actually communicates with the CompuScope hardware.
//! Aside from data transfer, therefore, all control of the CompuScope hardware is effected using this method.
//! Different actions are implemented on the CompuScope system by assigning different constants to i16Operation.
//! Setting i16Operation to ACTION_COMMIT or ACTION_COMMIT_COERCE will actually pass configuration setting values to the CompuScope hardware.
//! Configuration setting values should have been previously assigned in the driver using CsSet().  In the event of invalid
//! setting values, ACTION_COMMIT will cause CsDo() to return an error code. By contrast, in the event of invalid setting values,
//! ACTION_COMMIT_COERCE will cause CsDo() to coerce all invalid setting values to valid values before passing them to the CompuScope hardware.
//! If coercion of setting values occurs, then CS_CONFIG_CHANGED value is returned.
//! CsDo() may also be used to start acquisition, to abort acquisition, or to emulate a trigger event.
//! These actions are implemented by setting i16Operation to corresponding \link CSDO_ACTIONS actions constants \endlink
//!
//! \param hSystem		: Handle of CompuScope system
//! \param i16Operation	: Requested Operation \link CSDO_ACTIONS actions constants \endlink
//!
//!	\return
//!		- Positive value (>0) for success.
//!		- Negative value (<0) for error.
//!			Use #CsGetErrorString with error code to retrieve its definition.
//!
////////////////////////////////////////////////////////////////////
int32 SSM_API CsDo(CSHANDLE hSystem, int16 i16Operation)
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	if (FS_BASE_HANDLE & (ULONG_PTR) hSystem)
		return (CS_INVALID_HANDLE);

	int32 RetCode = 0;
	int32 i32Coerse = 0;

	RetCode = CSystemData::instance().ValidateSystemHandle(hSystem);
	if (CS_INVALID_HANDLE == RetCode)
		return (RetCode);

	CSystemData::instance().Lock();

	switch(i16Operation)
	{
		case ACTION_START: // Start Capture
		{
#if defined(_WIN32)
			TRACE(SSM_TRACE_LEVEL, "SSM::Action>>Start Capture %x\n",  GetCurrentThreadId() );
#else
			TRACE(SSM_TRACE_LEVEL, "SSM::Action>>Start Captur\n");
#endif

			CStateMachine	*pStateMachine = CSystemData::instance().GetStateMachine(hSystem);

			// Check if configuration has been changed. If changed, Revert to commited configuration
			int32 i32CurrentStateId = pStateMachine->RetrieveStateID();
			if (SSM_VALIDCONFIGURATION_ID == i32CurrentStateId)
			{
				CSystemData::instance().RevertConfiguration(hSystem);
			}

			if (true == CSystemData::instance().GetCommittedFlag(hSystem))
			{
				CSystemData::instance().UpdateAcquiredConfig(hSystem);
				CSystemData::instance().SetCommittedFlag(hSystem, false); // Reset the flag until next commit
			}

			if (!CSystemData::instance().IsSMLock(hSystem))
			{
				if (!CSystemData::instance().PerformTransition(hSystem, START_TRANSITION)) // Update State Machine
				{
					// Signal events for application to satisfy any WaitForSingleObject ..
					pStateMachine->SignalEvent(ACQ_EVENT_TRIGGERED);
					pStateMachine->SignalEvent(ACQ_EVENT_END_BUSY);

					TRACE(SSM_TRACE_LEVEL, "SSM::Error Invalid State\n");
					RetCode = CS_INVALID_STATE;
					break;
				}
			}

			// Reset events for application
			pStateMachine->ClearEvent(ACQ_EVENT_TRIGGERED);
			pStateMachine->ClearEvent(ACQ_EVENT_END_BUSY);

			// Start System Acquisition
			RetCode = ::CsDrvDo(hSystem, ACTION_START, NULL);
		}
		break;

		case ACTION_COMMIT_COERCE: // Commit Configuration
			i32Coerse = 1;
		case ACTION_COMMIT: // Commit Configuration
		{
			CSSYSTEMCONFIG			SystemConfig;
			int32					i32CoerceRetCode;

#if defined(_WIN32)
			TRACE(SSM_TRACE_LEVEL, "SSM::Action>>Commit Configuration %x\n", GetCurrentThreadId());
#else
			TRACE(SSM_TRACE_LEVEL, "SSM::Action>>Commit Configuration\n");
#endif


			SystemConfig.u32Size = sizeof(CSSYSTEMCONFIG);
			CSystemData::instance().GetSystemConfig(hSystem, &SystemConfig);

			// SystemConfig might be modified if coerce flag are set.
			i32CoerceRetCode = ::CsDrvValidateParams(hSystem, CS_SYSTEM, i32Coerse, &SystemConfig);

			if (CS_FAILED(i32CoerceRetCode)) // No failure under coerce flag
			{
				CSystemData::instance().PerformTransition(hSystem, BADMODIFY_TRANSITION); // Update State Machine
				RetCode = i32CoerceRetCode;
				break;
			}

			if (!CSystemData::instance().PerformTransition(hSystem, MODIFY_TRANSITION)) // Update State Machine
			{
				TRACE(SSM_TRACE_LEVEL, "SSM::Error Invalid State\n");
				RetCode = CS_INVALID_STATE;
				break;
			}

			// Commit Configuration
			CSystemData::instance().PerformTransition(hSystem, COMMIT_TRANSITION); // Update State Machine
			if (CSystemData::instance().IsSMLock(hSystem))
			{
				if (ACQ_STATUS_READY != CsGetStatus(hSystem))
				{
					::Sleep(10);
					if (ACQ_STATUS_READY != CsGetStatus(hSystem))
					{
						// Set the state machine to InvalidConfiguration since CANNOT call Commit
						CSystemData::instance().PerformTransition(hSystem, ABORT_TRANSITION);
						CSystemData::instance().UnLock();
						return CS_MISC_ERROR;
					}
				}
			}

			RetCode = ::CsDrvDo(hSystem, ACTION_COMMIT, &SystemConfig);

			if (CS_SUCCEEDED(RetCode))
			{
				int32 CoerceCheck = 0;

				// Update Committed structure...
				// Overwrite the Acquisition config with a Get since Read-Only value are only updated
				// when called by Get (not the validate)
				//CSACQUISITIONCONFIG AcqConfig = {0};
				CSACQUISITIONCONFIG AcqConfig;

				// ZeroMemory will initialize padding section while = {0} will not. 
				// Memcmp will fail in SetAcquisitionConfig
				ZeroMemory(&AcqConfig, sizeof(CSACQUISITIONCONFIG));
				AcqConfig.u32Size = sizeof(CSACQUISITIONCONFIG);

				RetCode = ::CsDrvGetParams(hSystem, CS_ACQUISITION, &AcqConfig);
				if (CS_FAILED(RetCode))
				{
					CSystemData::instance().UnLock();
					return (RetCode);
				}

				CoerceCheck = CSystemData::instance().SetAcquisitionConfig(hSystem, &AcqConfig, true);
/*
				Lines removed Feb 18 2015
				if (i32CoerceRetCode == CS_CONFIG_CHANGED || CS_CONFIG_CHANGED == CoerceCheck)
					i32CoerceRetCode = CS_CONFIG_CHANGED;
*/

				CSSYSTEMINFO SystemInfo;
				ZeroMemory(&SystemInfo, sizeof(CSSYSTEMINFO));
				SystemInfo.u32Size = sizeof(CSSYSTEMINFO);
				RetCode = ::CsDrvGetAcquisitionSystemInfo(hSystem, &SystemInfo);
				if (CS_FAILED(RetCode))
				{
					CSystemData::instance().UnLock();
					return (RetCode);
				}

				// Also update channels
				size_t ChannelStructSize = sizeof(ARRAY_CHANNELCONFIG) + ((SystemInfo.u32ChannelCount-1)* sizeof(CSCHANNELCONFIG));
				PARRAY_CHANNELCONFIG pChannelConfig = (PARRAY_CHANNELCONFIG)::VirtualAlloc(NULL, ChannelStructSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

				pChannelConfig->u32ChannelCount = SystemInfo.u32ChannelCount;
				for (uInt16 i = 0; i < pChannelConfig->u32ChannelCount; i++)
				{
					ZeroMemory(&pChannelConfig->aChannel[i], sizeof(CSCHANNELCONFIG));
					pChannelConfig->aChannel[i].u32Size = sizeof(CSCHANNELCONFIG);
					pChannelConfig->aChannel[i].u32ChannelIndex = i+1; // Channel is 1-based
				}

				RetCode = ::CsDrvGetParams(hSystem, CS_CHANNEL, pChannelConfig);
				if (CS_FAILED(RetCode))
				{
					CSystemData::instance().UnLock();
					return (RetCode);
				}

				for (uInt16 i = 0; i < pChannelConfig->u32ChannelCount; i++)
				{
					// Only update channel config where data is valid. Input range should always be different than 0.
					if (0 != pChannelConfig->aChannel[i].u32InputRange)
					{
						CoerceCheck = CSystemData::instance().SetChannelConfig(hSystem, &pChannelConfig->aChannel[i], true);
/*
						Lines removed Feb 18 2015
						if (i32CoerceRetCode != CS_CONFIG_CHANGED && CS_CONFIG_CHANGED == CoerceCheck)
							i32CoerceRetCode = CS_CONFIG_CHANGED;
*/
					}

				}

				::VirtualFree(pChannelConfig, 0, MEM_RELEASE);

				// And don't forget to update triggers
				size_t TriggerStructSize = sizeof(ARRAY_TRIGGERCONFIG)+((SystemInfo.u32TriggerMachineCount-1)* sizeof(CSTRIGGERCONFIG));
				PARRAY_TRIGGERCONFIG pTriggerConfig = (PARRAY_TRIGGERCONFIG)VirtualAlloc(NULL, TriggerStructSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

				pTriggerConfig->u32TriggerCount = SystemInfo.u32TriggerMachineCount;

				for (uInt16 i = 0; i < pTriggerConfig->u32TriggerCount; i++)
				{
					ZeroMemory(&pTriggerConfig->aTrigger[i], sizeof(CSTRIGGERCONFIG));
					pTriggerConfig->aTrigger[i].u32Size = sizeof(CSTRIGGERCONFIG);
					pTriggerConfig->aTrigger[i].u32TriggerIndex = i+1; // Channel is 1-based
				}

				RetCode = ::CsDrvGetParams(hSystem, CS_TRIGGER, pTriggerConfig);
				if (CS_FAILED(RetCode))
				{
					CSystemData::instance().UnLock();
					return (RetCode);
				}

				for (uInt16 i = 0; i < pTriggerConfig->u32TriggerCount; i++)
				{
					CoerceCheck = CSystemData::instance().SetTriggerConfig(hSystem, &pTriggerConfig->aTrigger[i], true);
/*
					Lines removed Feb 18 2015					
					if (i32CoerceRetCode != CS_CONFIG_CHANGED && CS_CONFIG_CHANGED == CoerceCheck)
						i32CoerceRetCode = CS_CONFIG_CHANGED;
*/
				}

				::VirtualFree(pTriggerConfig, 0, MEM_RELEASE);

				CSystemData::instance().UpdateCommittedConfig(hSystem);
				CSystemData::instance().PerformTransition(hSystem, DONE_TRANSITION); // Update State Machine

				CSystemData::instance().SetCommittedFlag(hSystem, true);

				// Commit success, return the return code from CsDrvValidateParams
				// This will tell the application if there is any corrections has been done
				RetCode = i32CoerceRetCode;
			}
			else
			{
				// Set the state machine to InvalidConfiguration since Commit failed
				CSystemData::instance().PerformTransition(hSystem, ABORT_TRANSITION);
			}
		}
		break;

		case ACTION_FORCE: // Force Trigger
		{
			TRACE(SSM_TRACE_LEVEL, "SSM::Action>>Force Trigger\n");

			// Validate Operation
			//if (!CSystemData::instance().IsActionValid(hSystem, FORCETRIGGER_TRANSITION))
			//	return (CS_INVALID_STATE);

			// Start System Acquisition
			RetCode = ::CsDrvDo(hSystem, ACTION_FORCE, NULL);

			// No modification at the StateMachine at this point, since a Trigger event will occurred
			// from the low-level driver and the state will be transition towards the BUSY state in the
			// event thread function.
//			return (RetCode);
		}
		break;

		case ACTION_ABORT: // Abort Operation
		{
			TRACE(SSM_TRACE_LEVEL, "SSM::Action>>Abort Operation\n");

			// Validate Operation
			//if (!CSystemData::instance().IsActionValid(hSystem, ABORT_TRANSITION))
			//	return (CS_INVALID_STATE);
			bool bDiskStream = false;
			if (SSM_DISKSTREAM_ID == CSystemData::instance().GetStateMachine(hSystem)->RetrieveStateID())
			{
				bDiskStream = true;
			}
			if (CSystemData::instance().PerformTransition(hSystem, ABORT_TRANSITION))
			{
				// Abort System at the low driver level
				RetCode = ::CsDrvDo(hSystem, ACTION_ABORT, NULL);

				if (CS_SUCCEEDED(RetCode))
				{
					// Flag the State Machine for Abort.
					// Event thread will update the state machine.
					CSystemData::instance().Abort(hSystem);

#if defined(_WIN32)
					// This is dangerous and a sledge hammer approach. DiskStream should be calling Stop rather than abort.
					if ( bDiskStream && (NULL != CSystemData::instance().GetDiskStreamPtr(hSystem)) )
					{
						CSystemData::instance().GetDiskStreamPtr(hSystem)->Stop();
						::Sleep(200); // just to be safe
						if ( !CSystemData::instance().GetDiskStreamPtr(hSystem)->IsFinished(500) )
						{
							CSystemData::instance().GetDiskStreamPtr(hSystem)->TerminateAllThreads();
						}
					}
#endif
				}
			}
			else
				RetCode = CS_INVALID_STATE;
		}
		break;

		case ACTION_RESET: // Reset Operation
		{
			TRACE(SSM_TRACE_LEVEL, "SSM::Action>>Reset Operation\n");

			CStateMachine	*pStateMachine = CSystemData::instance().GetStateMachine(hSystem);

			pStateMachine->SignalEvent(ACQ_EVENT_TRIGGERED);
			pStateMachine->SignalEvent(ACQ_EVENT_END_BUSY);

			// Reset System Acquisition
			RetCode = ::CsDrvDo(hSystem, ACTION_RESET, NULL);
			
			// Release time slice for Event Thread 
			Sleep(0);

			if (CS_SUCCEEDED(RetCode))
			{
				CSystemData::instance().PerformTransition(hSystem, RESET_TRANSITION); // Reset State Machine
				CSystemData::instance().PerformTransition(hSystem, INIT_TRANSITION); 

				CSystemData::instance().InitializeSystem( hSystem  );
			}
		}
		break;

		case ACTION_CALIB:
		{
			TRACE(SSM_TRACE_LEVEL, "SSM::Action>>Calibration Operation\n");
			
			// Validate Operation
			//if (!CSystemData::instance().IsActionValid(hSystem, ABORT_TRANSITION))
			//	return (CS_INVALID_STATE);
			if (CSystemData::instance().PerformTransition(hSystem, CALIBRATE_TRANSITION))
			{
				// Abort System at the low driver level
				RetCode = ::CsDrvDo(hSystem, ACTION_CALIB, NULL);
				CSystemData::instance().PerformTransition(hSystem, DONE_TRANSITION); // Update State Machine				
			}
			else
				RetCode = CS_INVALID_STATE;
		}
		break;
		
		default:
			// These actions will not affect the SSM state machine
			TRACE(SSM_TRACE_LEVEL, "SSM::Other Actions>> OperationId = %d\n", i16Operation);
			RetCode = ::CsDrvDo(hSystem, i16Operation, NULL);
			break;
	}

	CSystemData::instance().UnLock();

	return (RetCode);
}

//!
//! \ingroup SSM_API
//!


//! \fn int32 SSM_API CsExpertCall(CSHANDLE hSystem, PVOID pFunctionParams);
//! Generic function for access to eXpert functionality
//!
//! \param hSystem	        : Handle of the CompuScope system to be addressed. Obtained from CsGetSystem
//! \param pFunctionParams	: Pointer to the call-specific structure. See eXpert mode documentation for details.
//!
//!	\return
//!		- Positive value (>0) for success.
//!		- Negative value (<0) for error.
//!			Call CsGetErrorString with the error code to obtain a descriptive error string.
//!
////////////////////////////////////////////////////////////////////

int32 SSM_API CsExpertCall(CSHANDLE hSystem, PVOID pFunctionParams)
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	int32 i32RetCode = CSystemData::instance().ValidateSystemHandle(hSystem);
	if (CS_INVALID_HANDLE == i32RetCode)
		return (i32RetCode);

	PCSEXPERTPARAM   pCsInParam = (PCSEXPERTPARAM) pFunctionParams;

#if defined(_WIN32)
	if ((pCsInParam->u32ActionId & EXFX_DISK_STREAM_MASK) != 0)
	{
		CsDiskStream* pStream = CSystemData::instance().GetDiskStreamPtr(hSystem);

		switch (pCsInParam->u32ActionId)
		{
			case EXFN_DISK_STREAM_INITIALIZE:

				if (NULL == pStream) // pStream should be NULL here because there should only be 1 instance of it
				{
					PCSCONFIG_DISKSTREAM pParam = (PCSCONFIG_DISKSTREAM)pFunctionParams;

					if (::IsBadReadPtr (pParam, sizeof( CSCONFIG_DISKSTREAM)))
						return CS_INVALID_POINTER_BUFFER;

					if (pParam->u32Size != sizeof(CSCONFIG_DISKSTREAM))
						return CS_INVALID_STRUCT_SIZE;					
					
					//RG - MOVE THE VALIDATION STUFF (ESPECIALLY THE FILE NAME TO A SEPARATE FILE ??
					// check if szBaseFolder has a valid directory name and valid filenames, an empty string
					// can be considered valid.  Add error codes to CsError.h as you need them

					if (!IsValidPathName(pParam->szBaseFolder))
					{
						return CS_INVALID_DIRECTORY;
					}
					
					if (pParam->u32AcqNum < 0)
					{
						return CS_INVALID_DISK_STREAM_ACQ_COUNT;
					}
					// more validation and error checking
					// check that XferStart and XferLen are valid compared to TriggerHoldoff and SegmentSize
					// and that u32RecStart and u32RecCount are valid compared to SegmentCount 
					// check that channels are valid compared to the mode and the system

					CSACQUISITIONCONFIG csAcqConfig = {0};
					csAcqConfig.u32Size = sizeof(CSACQUISITIONCONFIG);
					i32RetCode = CSystemData::instance().GetAcquisitionConfig(hSystem, CS_CURRENT_CONFIGURATION, &csAcqConfig);
					if (CS_FAILED(i32RetCode))
					{
						return i32RetCode;
					}
					if (pParam->i64XferStart < -csAcqConfig.i64TriggerHoldoff)
					{
						return CS_INVALID_START;
					}
					if ((pParam->i64XferStart + pParam->i64XferLen) > (csAcqConfig.i64TriggerHoldoff + csAcqConfig.i64SegmentSize + csAcqConfig.i64TriggerDelay))
					{
						return CS_INVALID_LENGTH;
					}

					if (pParam->u32RecStart < 1 || pParam->u32RecStart > csAcqConfig.u32SegmentCount)
					{
						return CS_INVALID_SEGMENT;
					}
					if (((pParam->u32RecStart + pParam->u32RecCount - 1) > csAcqConfig.u32SegmentCount) ||
						(pParam->u32RecCount > csAcqConfig.u32SegmentCount) ||
						(pParam->u32RecCount < 1))
					{
						return CS_INVALID_SEGMENT_COUNT;
					}
					// check channels to see if they're valid
					CSSYSTEMINFO csSysInfo = {0};
					csSysInfo.u32Size = sizeof(CSSYSTEMINFO);
					i32RetCode = CSystemData::instance().GetSystemInfo(hSystem, &csSysInfo);
					if (CS_FAILED(i32RetCode))
					{
						return i32RetCode;
					}
					if (pParam->u32ChannelCount < 1 || pParam->u32ChannelCount > csSysInfo.u32ChannelCount)
					{
						return CS_INVALID_CHANNEL_COUNT;
					}

					uInt32 u32MaskedMode = csAcqConfig.u32Mode & CS_MASKED_MODE;
					uInt32 u32ChannelsPerBoard = csSysInfo.u32ChannelCount / csSysInfo.u32BoardCount;
					uInt32 u32ChannelSkip = u32ChannelsPerBoard / u32MaskedMode;

					bool bValidChannel = true;
					for (uInt32 i = 0; i < pParam->u32ChannelCount && bValidChannel; i++)
					{
						bool bFound = false;
						for (uInt32 u32Channel = 1; u32Channel <= csSysInfo.u32ChannelCount; u32Channel += u32ChannelSkip)
						{
							if (u32Channel == pParam->ChanList[i])
							{
								bFound = true;
								break;
							}
						}
						bValidChannel = bFound;
					}
					if ( !bValidChannel )
					{
						return CS_INVALID_CHANNEL;
					}


					string strBase = pParam->szBaseFolder;
					deque<uInt16> deqChan;
					for (uInt32 i = 0; i < pParam->u32ChannelCount; i++)
					{
						deqChan.push_front(pParam->ChanList[i]);
					}
					bool bTimeStamp = pParam->bTimeStamp ? true : false;

					try 
					{
						pStream = new CsDiskStream(hSystem, 
			       								   strBase,
												   deqChan, 
												   pParam->dwTimeout, 
												   pParam->i64XferStart, 
												   pParam->i64XferLen, 
												   pParam->u32RecStart, 
												   pParam->u32RecCount, 
												   pParam->u32AcqNum, 
												   bTimeStamp);

									
						// set the actual static data member
						CSystemData::instance().SetDiskStreamPtr(hSystem, pStream);
					}
					catch (PDiskStreamErrorStruct es)
					{
						delete pStream;
						pStream = NULL; 
						
						i32RetCode = es->i32ErrorCode;
						if ( 0 != ::_tcscmp(es->szErrorMessage, _T("")) )
						{
							// Report to event log
							AddToMessageLog(MSG_DISKSTREAM_ERROR, es->szErrorMessage);
						}
						break;
					}
					pParam->u32FilesPerChannel = 0;
					if( NULL != pStream )
						pParam->u32FilesPerChannel = pStream->GetFileNum();

					AddToMessageLog(MSG_DISKSTREAM_INITIALIZED, _T(""));
					::Sleep(100); // to allow precreation of files
				}
				break;

			case EXFN_DISK_STREAM_START:
				
				if (NULL == pStream)
				{
					return CS_DISK_STREAM_NOT_INITIALIZED;
				}
				try
				{
					i32RetCode = pStream->Start();
				}
				catch (PDiskStreamErrorStruct es)
				{
					if (NULL != pStream)
					{
						delete pStream;
						pStream = NULL;
					}
					i32RetCode = es->i32ErrorCode;
					if ( 0 != ::_tcscmp(es->szErrorMessage, _T("")) )
					{
						// Report to event log
						AddToMessageLog(MSG_DISKSTREAM_ERROR, es->szErrorMessage);
					}
				}
				break;

			case EXFN_DISK_STREAM_STATUS:
				{
					if (NULL == pStream)
					{
						return CS_DISK_STREAM_NOT_INITIALIZED;
					}

					PCSCONFIG_DISKSTREAM pParam = (PCSCONFIG_DISKSTREAM)pFunctionParams;

					if (::IsBadReadPtr (pParam, sizeof( CSCONFIG_DISKSTREAM)))
						return CS_INVALID_POINTER_BUFFER;

					if (pParam->u32Size != sizeof(CSCONFIG_DISKSTREAM))
						return CS_INVALID_STRUCT_SIZE;									
					
					try
					{
						i32RetCode = pStream->IsFinished(pParam->dwStatusTimeout); 
					}
					catch (PDiskStreamErrorStruct es)
					{
						if (NULL != pStream)
						{
							delete pStream;
							pStream = NULL;
						}
						i32RetCode = es->i32ErrorCode;
						if ( 0 != ::_tcscmp(es->szErrorMessage, _T("")) )
						{
							// Report to event log
							AddToMessageLog(MSG_DISKSTREAM_ERROR, es->szErrorMessage);
						}
					}
				}
				break;

			case EXFN_DISK_STREAM_STOP:
				
				if (NULL == pStream)
				{
					return CS_DISK_STREAM_NOT_INITIALIZED;
				}
				try
				{
					i32RetCode = pStream->Stop();
				}
				catch (PDiskStreamErrorStruct es)
				{
					if (NULL != pStream) 
					{
						delete pStream;
						pStream = NULL;
					}
					i32RetCode = es->i32ErrorCode;
					if ( 0 != ::_tcscmp(es->szErrorMessage, _T("")) )
					{
						// Report to event log
						AddToMessageLog(MSG_DISKSTREAM_ERROR, es->szErrorMessage);
					}
				}
				break;

			case EXFN_DISK_STREAM_CLOSE:
				
				delete pStream;
				pStream = NULL; 
				// set the actual static data member to NULL
				CSystemData::instance().SetDiskStreamPtr(hSystem, pStream);
				AddToMessageLog(MSG_DISKSTREAM_DESTROYED, _T(""));
				break;

			case EXFN_DISK_STREAM_ACQ_COUNT:
				
				if (NULL == pStream)
				{
					return CS_DISK_STREAM_NOT_INITIALIZED;
				}
				try
				{
					i32RetCode = pStream->GetAcquiredCount();
				}
				catch (PDiskStreamErrorStruct es)
				{
					if (NULL != pStream)
					{
						delete pStream;
						pStream = NULL;
					}
					i32RetCode = es->i32ErrorCode;
					if ( 0 != ::_tcscmp(es->szErrorMessage, _T("")) )
					{
						// Report to event log
						AddToMessageLog(MSG_DISKSTREAM_ERROR, es->szErrorMessage);
					}
				}				
				break;

			case EXFN_DISK_STREAM_WRITE_COUNT:
				if (NULL == pStream)
				{
					return CS_DISK_STREAM_NOT_INITIALIZED;
				}				
				try
				{
					i32RetCode = pStream->GetWriteCount();
				}
				catch (PDiskStreamErrorStruct es)
				{
					if (NULL != pStream) 
					{
						delete pStream;
						pStream = NULL;
					}
					i32RetCode = es->i32ErrorCode;
					if ( 0 != ::_tcscmp(es->szErrorMessage, _T("")) )
					{
						// Report to event log
						AddToMessageLog(MSG_DISKSTREAM_ERROR, es->szErrorMessage);
					}
				}
				break;

			case EXFN_DISK_STREAM_ERRORS:
				if (NULL == pStream)
				{
					return CS_DISK_STREAM_NOT_INITIALIZED;
				}
				try
				{
					deque<DiskStreamErrorStruct> deqErrors;
					pStream->GetErrorQueue(deqErrors);
					if ( deqErrors.empty() )
					{
						return CS_SUCCESS;
					}
					else
					{
						if ( 0 != lstrcmp(deqErrors[0].szErrorMessage, _T("")) )
						{
							// Report to event log
							AddToMessageLog(MSG_DISKSTREAM_ERROR, deqErrors[0].szErrorMessage);
						}
						return deqErrors[0].i32ErrorCode;
					}
				}
				catch (PDiskStreamErrorStruct es)
				{
					if (NULL != pStream) 
					{
						delete pStream;
						pStream = NULL;
					}
					i32RetCode = es->i32ErrorCode;
					if ( 0 != ::_tcscmp(es->szErrorMessage, _T("")) )
					{
						// Report to event log
						AddToMessageLog(MSG_DISKSTREAM_ERROR, es->szErrorMessage);
					}
				}
				break;

			case EXFN_DISK_STREAM_TIMING_FLAG:
				{
					if ( NULL == pStream )
					{
						return CS_DISK_STREAM_NOT_INITIALIZED;
					}

					PCSTIMEFLAG_DISKSTREAM pParam = (PCSTIMEFLAG_DISKSTREAM)pFunctionParams;
					if ( ::IsBadReadPtr (pParam, sizeof(CSTIMESTAT_DISKSTREAM)) )
						return CS_INVALID_POINTER_BUFFER;
					if ( ::IsBadWritePtr (pParam, sizeof(CSTIMESTAT_DISKSTREAM)) )
						return CS_INVALID_POINTER_BUFFER;

					if ( pParam->u32Size != sizeof(CSTIMESTAT_DISKSTREAM) )
						return CS_INVALID_STRUCT_SIZE;												

					try
					{
						pStream->SetTimingFlag(pParam->bFlag ? true : false);
					}
					catch (PDiskStreamErrorStruct es)
					{
						if (NULL != pStream) 
						{
							delete pStream;
							pStream = NULL;
						}
						i32RetCode = es->i32ErrorCode;
						if ( 0 != ::_tcscmp(es->szErrorMessage, _T("")) )
						{
							// Report to event log
							AddToMessageLog(MSG_DISKSTREAM_ERROR, es->szErrorMessage);
						}
					}
				}
				break;

			case EXFN_DISK_STREAM_TIMING_STATS: 

				if ( NULL == pStream )
				{
					return CS_DISK_STREAM_NOT_INITIALIZED;
				}

				PCSTIMESTAT_DISKSTREAM pParam = (PCSTIMESTAT_DISKSTREAM)pFunctionParams;

				if ( ::IsBadReadPtr (pParam, sizeof(CSTIMESTAT_DISKSTREAM)) )
					return CS_INVALID_POINTER_BUFFER;
				if ( ::IsBadWritePtr (pParam, sizeof(CSTIMESTAT_DISKSTREAM)) )
					return CS_INVALID_POINTER_BUFFER;

				if ( pParam->in.u32Size != sizeof(CSTIMESTAT_DISKSTREAM) )
					return CS_INVALID_STRUCT_SIZE;									

				try
				{
					pStream->GetPerformanceCount(pParam);
				}
				catch (PDiskStreamErrorStruct es)
				{
					if (NULL != pStream) 
					{
						delete pStream;
						pStream = NULL;
					}
					i32RetCode = es->i32ErrorCode;
					if ( 0 != ::_tcscmp(es->szErrorMessage, _T("")) )
					{
						// Report to event log
						AddToMessageLog(MSG_DISKSTREAM_ERROR, es->szErrorMessage);
					}
				}
				break;
		}
	}
	else
	{
		i32RetCode = ::CsDrvExpertCall(hSystem, pFunctionParams);
	}
#endif

	return (i32RetCode);
}


//!
//! \}
//!

int32 SSM_API CsCfgAuxIo(CSHANDLE hSystem)
{
	_ASSERT(g_RMInitialized);

	if (g_RMInitialized)
	{
		int32 i32RetCode = CSystemData::instance().ValidateSystemHandle(hSystem);
		if (CS_INVALID_HANDLE == i32RetCode)return (i32RetCode);

		return (::CsDrvCfgAuxIo(hSystem));
	}
	else
		return CS_NOT_INITIALIZED;
}


// Streaming operation
int32 SSM_API CsStmAllocateBuffer( CSHANDLE hSystem, uInt16 nCardIndex, uInt32 u32SizeInBytes, PVOID *pVa )
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	return ::CsDrvStmAllocateBuffer( hSystem, nCardIndex, u32SizeInBytes, pVa );
}

int32 SSM_API CsStmFreeBuffer( CSHANDLE hSystem, uInt16 nCardIndex, PVOID pVa )
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	return ::CsDrvStmFreeBuffer( hSystem, nCardIndex, pVa );
}

int32 SSM_API CsStmTransferToBuffer( CSHANDLE hSystem, uInt16 nCardIndex, PVOID pBuffer, uInt32 u32TransferSize )
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	return ::CsDrvStmTransferToBuffer( hSystem, nCardIndex, pBuffer, u32TransferSize );
}

int32 SSM_API CsStmGetTransferStatus( CSHANDLE hSystem, uInt16 nCardIndex, uInt32 u32WaitTimeoutMs, uInt32 *u32ErrorFlag, uInt32 *u32ActualSize, PVOID Reserved )
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	return ::CsDrvStmGetTransferStatus( hSystem, nCardIndex, u32WaitTimeoutMs, u32ErrorFlag, u32ActualSize, Reserved );
}
