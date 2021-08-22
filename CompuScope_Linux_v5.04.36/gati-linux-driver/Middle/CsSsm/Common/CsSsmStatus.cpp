
#include "StdAfx.h"
#include "CsSsm.h"

#ifdef __linux__ 
	#include "CsErrorStruct.h"
#endif

//!
//! \ingroup SSM_API
//!
//! \fn int32 SSM_API CsGetStatus(CSHANDLE hSystem);
//!	Returns the current acquisition status of the CompuScope system.
//!
//! This method is used to query the current acquisition status of the CompuScope system.
//! Generally, this method is used to determine whether an acquisition is complete so that further operations may be performed
//!
//! \param hSystem	: Handle of the CompuScope system to be addressed.  Obtained from CsGetSystem.
//!
//!	\return
//!		- \link SYSTEM_STATUS System status \endlink
//!		- Negative value (<0) an error.
//!			Call CsGetErrorString with the error code to obtain a descriptive error string.
//!
//!  \sa CsGetEventHandle
 ////////////////////////////////////////////////////////////////////
int32 SSM_API CsGetStatus(CSHANDLE hSystem)
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	if (FS_BASE_HANDLE & (ULONG_PTR) hSystem)
		return (CS_INVALID_HANDLE);

	int32	i32RetCode = 0;
	uInt32	u32Status = 0;

	i32RetCode = CSystemData::instance().ValidateSystemHandle(hSystem);

	if (CS_INVALID_HANDLE == i32RetCode)
		return (i32RetCode);

	CStateMachine	*pStateMachine = CSystemData::instance().GetStateMachine(hSystem);

	// NULL handle for trigger event represents Disabled Interrupts.
	if (NULL == pStateMachine->GetTriggerEvent())
	{

		// Another state
		i32RetCode = ::CsDrvGetAcquisitionStatus(hSystem, &u32Status);

		Sleep(0);	// Release timeshare

		if (CS_SUCCEEDED(i32RetCode))
			return (u32Status);
		else
			return (i32RetCode);
	}
	else
	{
		// Fixed synchronization problem with SSM Event Thread by return the status
		// based on SSM state instead of the real status of HW
		int32 i32StateID = pStateMachine->RetrieveStateID();
		Sleep(0);	// Release timeshare

		if (SSM_WAIT_ID == i32StateID)
			return ACQ_STATUS_WAIT_TRIGGER;
		else if (SSM_BUSY_ID == i32StateID)
			return ACQ_STATUS_TRIGGERED;
		else if (SSM_DATAREADY_ID == i32StateID)
			return ACQ_STATUS_READY;
		else
		{
			// Other state
			i32RetCode = ::CsDrvGetAcquisitionStatus(hSystem, &u32Status);
			Sleep(0);	// Release timeshare

			if (CS_SUCCEEDED(i32RetCode))
				return (u32Status);
			else
				return (i32RetCode);
		}
	}
}

//!
//! \ingroup SSM_API
//!
//! \fn HANDLE SSM_API CsGetEventHandle(CSHANDLE hSystem, uInt32 u32EventType, HANDLE* hEvent)
//! Returns the requested event handle.
//!
//! This method is useful only for multi-threaded processes. This method may be used to retrieve event notification
//! handles that can be used with standard synchronization primitives rather than continuously polling the hardware.
//! The process that calls this method is responsible for the life cycle management of the event handle.
//!
//! \param hSystem		: Handle of the CompuScope system to be addressed.  Obtained from CsGetSystem.
//! \param u32EventType	: \link EVENT_IDS Requested Event \endlink
//! \param hEvent		: Handle of the requested event
//!
//!	\return
//!		- Positive value (>0) for success.
//!		- Negative value (<0) for error.
//!			Call CsGetErrorString with the error code to obtain a descriptive error string.
//! \sa CsGetStatus
////////////////////////////////////////////////////////////////////
#ifdef _WIN32
int32 SSM_API CsGetEventHandle(CSHANDLE hSystem, uInt32 u32EventType, EVENT_HANDLE* phEvent)
#else
int32 SSM_API CsGetEventHandle(CSHANDLE hSystem, uInt32 u32EventType, int* phEvent)
#endif /* _WIN32 */
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return (CS_NOT_INITIALIZED);

	if (FS_BASE_HANDLE & (ULONG_PTR) hSystem)
		return (CS_INVALID_HANDLE);

	if(phEvent == NULL)
		return CS_NULL_POINTER;

	int32 i32RetCode = CSystemData::instance().ValidateSystemHandle(hSystem);
	if (CS_INVALID_HANDLE == i32RetCode)
		return (i32RetCode);

	// Return event handle for requested system
	*phEvent = CSystemData::instance().GetDrvEvent(hSystem, u32EventType);

	if (0 == (*phEvent))
		return (CS_DRIVER_ASYNC_NOT_SUPPORTED);

	return (CS_SUCCESS);
}

//! \if DOCUMENTED
//! \fn int32 SSM_API CsLock(CSHANDLE hSystem, BOOL bLock);
//!	Lock the system to improve performance
//!
//! \param hSystem	: Handle of system to lock
//! \param bLock	: Enable (TRUE) or Disable (FALSE) the locking mechanism.
//!
//!	\return
//!		- Positive value (>0) for success.
//!		- Negative value (<0) for error.
//!			Call CsGetErrorString with the error code to obtain a descriptive error string.
//!
//! \sa CsGetSystem, CsFreeSystem
////////////////////////////////////////////////////////////////////
//! \endif
int32 SSM_API CsLock(CSHANDLE hSystem, BOOL bLock)
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return (CS_NOT_INITIALIZED);

	if (FS_BASE_HANDLE &  (ULONG_PTR) hSystem)
		return (CS_INVALID_HANDLE);

	int32 i32RetCode = CSystemData::instance().ValidateSystemHandle(hSystem);
	if (CS_INVALID_HANDLE == i32RetCode)
		return (i32RetCode);

	if (bLock)
		CSystemData::instance().LockSM(hSystem, true);
	else
		CSystemData::instance().LockSM(hSystem, false);

	return (CS_SUCCESS);
}

//!
//! \ingroup SSM_API
//!
//! \fn int32 SSM_API CsRegisterCallbackFnc(CSHANDLE hSystem, uInt32 u32Event, LPCsEventCallback pCallBack)
//!	Register callback a for specific CompuScope system event.
//!
//!	This method registers a user-defined function of type <BR>
//! <b><I>void ( *LPCsEventCallback ) ( PCALLBACK_DATA pData )</I></b> that will be called upon occurrence
//! of the specified event.<BR>
//! The parameter (pData) with which the callback method is called points to a CALLBACK_DATA structure containing
//! information about the CompuScope system.
//! In case of an ACQ_EVENT_END_TXFER event, the structure also contains information to identify the data transfer,
//! namely i32Token and u32ChannelIndex
//!
//! \param hSystem	 : Handle of the CompuScope system to be addressed.  Obtained from CsGetSystem.
//! \param u32Event	 : One of the following:
//!                    - ACQ_EVENT_END_BUSY  - end of the acquisition
//!                    - ACQ_EVENT_END_TXFER - end of the data transfer
//! \param pCallBack : Pointer to callback function
//!
//!	\return
//!		- Positive value (>0) for success.
//!		- Negative value (<0) for error.
//!			Call CsGetErrorString with the error code to obtain a descriptive error string.
//!
//! \remarks To unregister a callback, simply call this function with the given event
//! and set the function pointer as NULL.<BR><BR>
//! The callback function will be called in a different thread context than CsRegisterCallbackFnc method. Therefore,
//! no thread specific information will be preserved.
//!
//! \sa CsGetEventHandle
////////////////////////////////////////////////////////////////////

int32 SSM_API CsRegisterCallbackFnc(CSHANDLE hSystem, uInt32 u32Event, LPCsEventCallback pCallBack)
{
	int32 i32RetCode = CS_SUCCESS;

	if (FS_BASE_HANDLE &  (ULONG_PTR) hSystem)
		return (CS_INVALID_HANDLE);

	i32RetCode = CSystemData::instance().ValidateSystemHandle(hSystem);
	if (CS_INVALID_HANDLE == i32RetCode)
		return (i32RetCode);

	if (FALSE == (BOOL)CSystemData::instance().IsEventSupported(hSystem))
		return (CS_DRIVER_ASYNC_NOT_SUPPORTED);

	switch (u32Event)
	{
		//case ACQ_EVENT_TRIGGERED:
		//	i32RetCode = CSystemData::instance().SetTriggerCallback(hSystem, pCallBack);
		//	break;

		case ACQ_EVENT_END_BUSY:
			i32RetCode = CSystemData::instance().SetEndAcqCallback(hSystem, pCallBack);
			break;

		case ACQ_EVENT_END_TXFER:
			i32RetCode = CSystemData::instance().SetEndTxCallback(hSystem, pCallBack);
			break;

		default:
			return CS_INVALID_EVENT;
	}

	return (i32RetCode);
}
//!
//! \ingroup SSM_API
//!
//! \fn int32 SSM_API CsGetErrorString(int32 i32ErrorCode, LPTSTR lpBuffer, int nBufferMax);
//!	Obtains a descriptive error string corresponding to the provided error code.
//!
//! \param i32ErrorCode		: Error code to look up.
//! \param lpBuffer			: Pointer to the buffer that will be filled with the error string
//! \param nBufferMax		: Specifies the size of the buffer, in TCHARs
//!	\return
//!		- Positive value (>0) for success.
//!		- Negative value (<0) for error.
//!
////////////////////////////////////////////////////////////////////
#ifndef __linux__ //RG ????
int32 SSM_API CsGetErrorString(int32 i32ErrorCode, LPTSTR lpBuffer, int nBufferMax);


int32 SSM_API CsGetErrorStringA(int32 i32ErrorCode, LPSTR lpBuffer, int nBufferMax)
{
	return (CsGetErrorString(i32ErrorCode, lpBuffer, nBufferMax));
}

int32 SSM_API CsGetErrorStringW(int32 i32ErrorCode, wchar_t* lpBuffer, int nBufferMax)
{
	LPSTR	pszString = NULL;
	int32	i32Ret = CS_SUCCESS;

	// normalize the buffer size from wchar_t to char
	int nBufferLength = (nBufferMax/sizeof(wchar_t)) * sizeof(char);
	pszString =  (LPSTR)::VirtualAlloc(NULL, nBufferLength, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	i32Ret = CsGetErrorString(i32ErrorCode, pszString, nBufferLength);

	if (CS_FAILED(i32Ret))
		return (i32Ret);

	::MultiByteToWideChar(CP_ACP, 0, pszString, -1, lpBuffer, nBufferMax);
	::VirtualFree(pszString, 0, MEM_RELEASE);
	return (CS_SUCCESS);
}

// Prototype from ErrorString.cpp
int GetErrorString(int32 i32ErrorCode, LPSTR lpBuffer, int nBufferMax);

int32 SSM_API CsGetErrorString(int32 i32ErrorCode, LPSTR lpBuffer, int nBufferMax)
{
	int32	i32RetVal = 0;
	int32   i32Param = 0;

	bool bFormat = false;

	if( CS_ERROR_FORMAT_THRESHOLD < -i32ErrorCode )
	{
		// It is a calibration error with additional information
		bFormat = true;
		i32Param = (-i32ErrorCode) / CS_ERROR_FORMAT_THRESHOLD;
		i32ErrorCode =  i32ErrorCode % CS_ERROR_FORMAT_THRESHOLD;
	}

	if( !bFormat )
	{
		i32RetVal = GetErrorString(i32ErrorCode, lpBuffer, nBufferMax);
	}
	else
	{
		TCHAR szFrmt[MAX_PATH];
		i32RetVal = GetErrorString(i32ErrorCode, szFrmt, MAX_PATH);
		if( 0 != i32RetVal )
		{
			_sntprintf_s(lpBuffer, nBufferMax, nBufferMax, szFrmt, i32Param);
		}
	}

	if( 0 == i32RetVal)
	{
		if( i32ErrorCode >= 0 )
		{
			_tcsncpy_s( lpBuffer, nBufferMax, _T("Unrecognized success code"), nBufferMax );
		}
		else
		{
			_snprintf_s( lpBuffer, nBufferMax, nBufferMax, _T("Unrecognized error code [ %d ]"), i32ErrorCode );
		}
	}

	return (CS_SUCCESS);
}

#else

int32 SSM_API CsGetErrorString(int32 i32ErrorCode, LPSTR lpBuffer, int nBufferMax)
{
	int32 i32RetVal = 0;
	for (int i = 0; i < g_arrErrorCodeSize; i++)
	{
		if ( i32ErrorCode == g_arrErrorCodes[i].nErrorCode )
		{
			strncpy(lpBuffer, g_arrErrorCodes[i].strText, nBufferMax);
			i32RetVal = CS_SUCCESS;
			break;
		}
	}
	if ( 0 == i32RetVal )
	{
		if ( i32ErrorCode >= 0 )
			strncpy(lpBuffer, "Unrecognized success code", nBufferMax);
		else
			snprintf( lpBuffer, nBufferMax, _T("Unrecognized error code [ %d ]"), i32ErrorCode );
	}
	return CS_SUCCESS;
}

#endif //__linux__
