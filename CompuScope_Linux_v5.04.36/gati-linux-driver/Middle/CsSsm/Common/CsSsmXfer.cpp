#include "StdAfx.h"
#include "CsSsm.h"

#ifdef __linux__
	#include "misc.h"
#endif

//!
//! \ingroup SSM_API
//!
//! \fn int32 SSM_API CsTransfer(CSHANDLE hSystem, PIN_PARAMS_TRANSFERDATA pInData, POUT_PARAMS_TRANSFERDATA pOutData);
//!	Transfers a specified number of samples from CompuScope on-board acquisition memory  to a buffer, from a specified starting address.
//! The method may also be used to transfer other information, such as time-stamp data.
//!
//! This method is used to transfer acquired CompuScope data from on-board acquisition memory.  The method can transfer one acquired
//! segment from one channel of a CompuScope system at a time.<BR>  Settings that specify what data are to be transferred are specified
//! within the structure of type IN_PARAMS_TRANSFERDATA to which pInData points.  The starting address is specified relative to
//! the trigger address so that a starting address of 0 will request data transfer from the trigger address. Negative starting
//! address values are used to download pre-trigger data.<BR> Due to CompuScope memory architecture and transfer buffer alignment issues,
//! the actual start address and the length may be slightly different from those requested. These values may also be adjusted by
//! the driver if the requested data lies outside the valid data range. Consequently, the actual start address and length are
//! returned in the structure to which pOutData  points.  These actual transfer settings must be used in order to correctly interpret
//! the transferred data.
//!
//! \param hSystem	: Handle of the CompuScope system to be addressed.  Obtained from CsGetSystem
//! \param pInData	: Pointer to the \link IN_PARAMS_TRANSFERDATA structure \endlink containing requested data transfer settings and data buffer pointer
//! \param pOutData	: Pointer to the \link OUT_PARAMS_TRANSFERDATA structure \endlink that is filled with actual data transfer settings
//!
//!	\return
//!		- Positive value (>0) for success.
//!		- Negative value (<0) for error.
//!			Call CsGetErrorString with the error code to obtain a descriptive error string.
//!
//!  \sa IN_PARAMS_TRANSFERDATA, OUT_PARAMS_TRANSFERDATA
////////////////////////////////////////////////////////////////////
int32 SSM_API CsTransfer(CSHANDLE   hSystem,
						 PIN_PARAMS_TRANSFERDATA  pInData,
						 POUT_PARAMS_TRANSFERDATA pOutParams)
{
	_ASSERT(g_RMInitialized);


	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	int32 RetCode = 0;

	RetCode = CSystemData::instance().ValidateSystemHandle(hSystem);
	if (CS_INVALID_HANDLE == RetCode)
		return (RetCode);

	// Special case for File handle...
	if (FS_BASE_HANDLE & (ULONG_PTR) hSystem)
	{
		return (CS_INVALID_REQUEST);
	}

	CSystemData::instance().Lock();

	TRACE(SSM_TRACE_LEVEL, "SSM::CsTransfer entry point\n");

	// Check if we have a callback set for end of transfer...
	// If so, set an asynchronous transfer to be sent from the worker thread
	// and wait on its completion to simulate synchronous transfer
	if ((int32)TRUE == CSystemData::instance().HasTransferCallback(hSystem))
	{
		int32 token;

		// Set the transfer with a temporary end event 
#ifdef _WIN32
		EVENT_HANDLE hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		pInData->hNotifyEvent = &hEvent;
#else
		int status;
		int pipefd[2];

		// Set the transfer with a temporary end event
		status = pipe(pipefd);
		status = fcntl(pipefd[0], F_GETFL);
		fcntl(pipefd[0], F_SETFL, status | O_NONBLOCK);
		pInData->hNotifyEvent_read = pipefd[0];
		pInData->hNotifyEvent_write = pipefd[1];
#endif /* _WIN32 */

		// Unlock now to release critical section for further processing
		CSystemData::instance().UnLock();

		RetCode = CsTransferAS(hSystem, pInData, pOutParams, &token);

		// Wait for the transfer to complete before continuing and cleanup
#ifdef _WIN32
		::WaitForSingleObject(hEvent, INFINITE);
		::CloseHandle(hEvent);
#else
		struct pollfd pollfd;
		pollfd.fd = pipefd[0];
		pollfd.events = POLLIN;
		for (;;) {
			poll(&pollfd, 1, -1);
			if (pollfd.revents & POLLIN) {
				return (RetCode);
			}
		}
		close(pipefd[0]);
		close(pipefd[1]);
#endif /* _WIN32 */

		return (RetCode);
	}

	// Validate state to retrieve data
	if (!CSystemData::instance().IsSMLock(hSystem))
	{
		if (!CSystemData::instance().PerformTransition(hSystem, TRANSFER_TRANSITION))
		{
			CSystemData::instance().UnLock();
			TRACE(SSM_TRACE_LEVEL, "SSM::Error Invalid State\n");
			return (CS_INVALID_STATE);
		}

		if (!CSystemData::instance().PerformTransition(hSystem, READ_TRANSITION))
		{
			CSystemData::instance().UnLock();
			TRACE(SSM_TRACE_LEVEL, "SSM::Error Invalid State\n");
			return (CS_INVALID_STATE);
		}
	}
	else
	{	// RG - COMMENTED OUT July 4, 2012 because remote systems would get a buffer full error (10055) when
		// doing multiple record captures.  If needed we can add it back for local systems
/*
		if (ACQ_STATUS_READY != CsGetStatus(hSystem))
		{
			::Sleep(10);
			if (ACQ_STATUS_READY != CsGetStatus(hSystem))
			{
				CSystemData::instance().UnLock();
				return CS_FALSE;
			}
		}
*/
	}

	RetCode = ::CsDrvTransferData(hSystem, *pInData, pOutParams); // Transfer normnally

	if (!CSystemData::instance().IsSMLock(hSystem))
	{
		if (!CSystemData::instance().PerformTransition(hSystem, DONE_TRANSITION))
		{
			CSystemData::instance().UnLock();
			TRACE(SSM_TRACE_LEVEL, "SSM::Error Invalid State\n");
			return (CS_INVALID_STATE);
		}
	}
	CSystemData::instance().UnLock();

	return (RetCode);
}


int32 SSM_API CsTransferEx(CSHANDLE   hSystem, PIN_PARAMS_TRANSFERDATA_EX  pInData, POUT_PARAMS_TRANSFERDATA_EX pOutParams)
{
	_ASSERT(g_RMInitialized);


	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	int32 RetCode = 0;

	RetCode = CSystemData::instance().ValidateSystemHandle(hSystem);
	if (CS_INVALID_HANDLE == RetCode)
		return (RetCode);

	// Special case for File handle...
	if (FS_BASE_HANDLE & (ULONG_PTR) hSystem)
	{
		return (CS_INVALID_REQUEST);
	}

	CSystemData::instance().Lock();

	TRACE(SSM_TRACE_LEVEL, "SSM::CsTransferEx entry point\n");

	// Check if we have a callback set for end of transfer...
	// If so, set an asynchronous transfer to be sent from the worker thread
	// and wait on its completion to simulate synchronous transfer
	if ((int32)TRUE == CSystemData::instance().HasTransferCallback(hSystem))
	{
		// Set the transfer with a temporary end event 
#ifdef _WIN32
		EVENT_HANDLE hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		pInData->hNotifyEvent = &hEvent;
#else
		int status;
		int pipefd[2];

		// Set the transfer with a temporary end event
		status = pipe(pipefd);
		status = fcntl(pipefd[0], F_GETFL);
		fcntl(pipefd[0], F_SETFL, status | O_NONBLOCK);
		pInData->hNotifyEvent_read = pipefd[0];
		pInData->hNotifyEvent_write = pipefd[1];
#endif /* _WIN32 */

		// Unlock now to release critical section for further processing
		CSystemData::instance().UnLock();

		// Wait for the transfer to complete before continuing and cleanup
#ifdef _WIN32
		::WaitForSingleObject(hEvent, INFINITE);
		::CloseHandle(hEvent);
#else
		struct pollfd pollfd;
		pollfd.fd = pipefd[0];
		pollfd.events = POLLIN;
		for (;;) {
			poll(&pollfd, 1, -1);
			if (pollfd.revents & POLLIN) {
				return (RetCode);
			}
		}
		close(pipefd[0]);
		close(pipefd[1]);
#endif /* _WIN32 */
		return (RetCode);
	}

	// Validate state to retrieve data
	if (!CSystemData::instance().IsSMLock(hSystem))
	{
		if (!CSystemData::instance().PerformTransition(hSystem, TRANSFER_TRANSITION))
		{
			CSystemData::instance().UnLock();
			TRACE(SSM_TRACE_LEVEL, "SSM::Error Invalid State\n");
			return (CS_INVALID_STATE);
		}

		if (!CSystemData::instance().PerformTransition(hSystem, READ_TRANSITION))
		{
			CSystemData::instance().UnLock();
			TRACE(SSM_TRACE_LEVEL, "SSM::Error Invalid State\n");
			return (CS_INVALID_STATE);
		}
	}
	else
	{
		if (ACQ_STATUS_READY != CsGetStatus(hSystem))
		{
			::Sleep(10);
			if (ACQ_STATUS_READY != CsGetStatus(hSystem))
			{
				CSystemData::instance().UnLock();
				return CS_FALSE;
			}
		}
	}

	RetCode = ::CsDrvTransferDataEx(hSystem, pInData, pOutParams); // Transfer normnally

	if (!CSystemData::instance().IsSMLock(hSystem))
	{
		if (!CSystemData::instance().PerformTransition(hSystem, DONE_TRANSITION))
		{
			CSystemData::instance().UnLock();
			TRACE(SSM_TRACE_LEVEL, "SSM::Error Invalid State\n");
			return (CS_INVALID_STATE);
		}
	}
	CSystemData::instance().UnLock();

	return (RetCode);
}

//!
//! \ingroup SSM_API
//!
//! \fn int32 SSM_API CsTransferAS(CSHANDLE hSystem, PIN_PARAMS_TRANSFERDATA pInData, POUT_PARAMS_TRANSFERDATA pOutData, int32* pToken);
//!	Asynchronously transfers a specified number of samples from CompuScope on-board acquisition memory to a buffer from a specified starting address.
//!
//! This method is used to asynchronously transfer acquired CompuScope data from on-board acquisition memory.
//! The method returns right away after initiating the transfer. The token returned by this method may be used
//! to check the status of the transfer via CsGetTransferASResult method.
//! Otherwise, operation of this method is identical to CsTransfer one.<BR>
//! Note: Asynchronous transfers are usually used with event notifications.
//!
//! \param hSystem	: Handle of the CompuScope system to be addressed.  Obtained from CsGetSystem
//! \param pInData	: Pointer to the \link IN_PARAMS_TRANSFERDATA structure \endlink containing requested data transfer settings and data buffer pointer
//! \param pOutData	: Pointer to the \link OUT_PARAMS_TRANSFERDATA structure \endlink that is filled with actual data transfer settings
//! \param pToken	: Pointer to the int32 variable that is filled with a unique value used in subsequent calls to CsGetTransferASResult
//!
//!	\return
//!		- Positive value (>0) for success.
//!		- Negative value (<0) for error.
//!			Call CsGetErrorString with the error code to obtain a descriptive error string.
//!
//!  \sa IN_PARAMS_TRANSFERDATA, OUT_PARAMS_TRANSFERDATA, CsTransfer, CsGetTransferASResult, CsGetEventHandle
////////////////////////////////////////////////////////////////////

int32 SSM_API CsTransferAS(CSHANDLE   hSystem,
					  	   PIN_PARAMS_TRANSFERDATA pInData,
						   POUT_PARAMS_TRANSFERDATA	pOutParams,
						   int32* pToken)
{
	_ASSERT(g_RMInitialized);

	if (::IsBadWritePtr(pToken, sizeof(int32)))
		return (CS_INVALID_PARAMETER);

	*pToken = 0;

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	// Special case for File handle...
	if (FS_BASE_HANDLE & (ULONG_PTR) hSystem)
	{
		return (CS_INVALID_REQUEST);
	}

	// NULL handle for trigger event represents Disabled Interrupts.
	if (NULL == CSystemData::instance().GetStateMachine(hSystem)->GetTriggerEvent())
		return (CS_DRIVER_ASYNC_NOT_SUPPORTED);

	int32 i32RetCode = CSystemData::instance().ValidateSystemHandle(hSystem);
	if (CS_INVALID_HANDLE == i32RetCode)
		return (i32RetCode);

#ifdef _WIN32
	TRACE(SSM_TRACE_LEVEL, "SSM::CsTransferAS entry point\n");
	CSystemData::instance().GetStateMachine(hSystem)->SetAppEndTransferEvent(*(pInData->hNotifyEvent));
#else
	CSystemData::instance().GetStateMachine(hSystem)->SetAppEndTransferEvent(pInData->hNotifyEvent_read, pInData->hNotifyEvent_write);
#endif /* _WIN32 */

    // Make a copy of the PIN_PARAMS_TRANSFERDATA in order for the app not to see the modification that
	// will be applied. The strucutre will be freed once it has been executed in the queue.
	//PIN_PARAMS_TRANSFERDATA pSSM_In_Data = static_cast<PIN_PARAMS_TRANSFERDATA>(::VirtualAlloc(NULL, sizeof(IN_PARAMS_TRANSFERDATA), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	PIN_PARAMS_TRANSFERDATA pSSM_In_Data = NULL;
	pSSM_In_Data = CSystemData::instance().GetStateMachine(hSystem)->AllocateInStruct();

	_ASSERT(NULL != pSSM_In_Data);

	memcpy(pSSM_In_Data, pInData, sizeof(IN_PARAMS_TRANSFERDATA));

	pSSM_In_Data->hNotifyEvent = CSystemData::instance().GetStateMachine(hSystem)->GetTransferEndEvent();

	// Create token for Asynchronous transfer
	int32 i32Token = CSystemData::instance().GetToken(hSystem, pSSM_In_Data->u16Channel);

	// Queued calls
	Worker* pWorker = CSystemData::instance().GetWorkerThread(hSystem);
	pWorker->Do(DeferCall(&PerformQueued, hSystem, pSSM_In_Data, pOutParams, i32Token));

	*pToken = i32Token;

	// Return token to user
	return (CS_SUCCESS);
}

void PerformQueued(CSHANDLE hSystem, PIN_PARAMS_TRANSFERDATA pInData, POUT_PARAMS_TRANSFERDATA	pOutParams, int32 i32Token)
{
	int32 RetCode = 0;

	TRACE(SSM_TRACE_LEVEL, "SSM::Queued CsTransferAS entry point\n");

	RetCode = CSystemData::instance().PerformTransition(hSystem, TRANSFER_TRANSITION);

	if (CS_SUCCEEDED(RetCode))
	{
		RetCode = CSystemData::instance().PerformTransition(hSystem, READ_TRANSITION);

		if (CS_SUCCEEDED(RetCode))
		{
			// Retrieve requested data: ASYNCHRONE MODE
			RetCode = ::CsDrvTransferData(hSystem, *pInData, pOutParams);

			// Activate cookie for CsGetTransferASResult
			CSystemData::instance().ValidateToken(hSystem, i32Token, NULL, ACTIVATE_TOKEN);
		}
	}

	if (CS_FAILED(RetCode))
	{
		CSystemData::instance().SetTokenStatus(hSystem, i32Token, RetCode);
		SetEvent((EVENT_HANDLE)(pInData->hNotifyEvent));
	}

	// Deallocation of structure allocated in CsTransferAS before the Queue
	//::VirtualFree(pInData, 0, MEM_RELEASE);
	CSystemData::instance().GetStateMachine(hSystem)->ReleaseInStruct(pInData);
}

//!
//! \ingroup SSM_API
//!
//! \fn int32 SSM_API CsGetTransferASResult(CSHANDLE hSystem, int32 i32Token, int64* pResults)
//! Query the result of the transfer specified by the i32Token
//!
//! This method is used to  query the result of an asynchronous transfer initiated by a call
//! to the CsGetTransferAS API method.
//!
//! \param hSystem	: Handle of the CompuScope system to be addressed.  Obtained from CsGetSystem
//! \param i32Token	: Token specifying the transfer to be queried
//! \param pResults	: Pointer to the int64 variable that is filled with the number of bytes already transferred
//!
//!	\return
//!		- Positive value (>0) for success.
//!		- Negative value (<0) for error.
//!			Call CsGetErrorString with the error code to obtain a descriptive error string.
//!
//!  \sa CsGetTransferAS
////////////////////////////////////////////////////////////////////

int32 SSM_API CsGetTransferASResult(CSHANDLE hSystem, int32 i32Token, int64* pResults)
{
	int32 i32RetCode = CS_SUCCESS;
	int32 i32Status = 0;

	i32RetCode = CSystemData::instance().ValidateSystemHandle(hSystem);
	if (CS_FAILED(i32RetCode))
		return (i32RetCode);

	i32RetCode = CSystemData::instance().ValidateToken(hSystem, i32Token, &i32Status);
	if (CS_FAILED(i32RetCode))
		return (i32RetCode);

	// Validate if cookie is in progress
	if (TX_PROGRESS == i32Status)
	{
		CSTRANSFERDATARESULT TxResults = {0};

		i32RetCode = ::CsDrvGetAsyncTransferDataResult(hSystem, (uInt8)(HIWORD(i32Token)), &TxResults, FALSE);

		if (CS_SUCCESS == i32RetCode)
			i32RetCode = TxResults.i32Status;

		*pResults = TxResults.i64BytesTransfered;
	}
	else if (TX_COMPLETED == i32Status)
	{
		*pResults = CSystemData::instance().GetTokenResult(hSystem, i32Token);
	}
	else
	{
		*pResults = 0;
	}

	i32RetCode =  CSystemData::instance().GetTokenStatus(hSystem, i32Token);
	return (i32RetCode);
}
