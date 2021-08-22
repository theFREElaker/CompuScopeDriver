
#include "StdAfx.h"
#include "CsSsmStateMachine.h"


CStateMachine::CStateMachine(CSHANDLE hSystem) // : _fsm(*this)
{
	::InitializeCriticalSection(&m_Sec);
	_fsm = new CStateMachineContext(*this);

	m_bSMLock = false; // SM is not locked for performance by default.
	m_bAborted = false;

	m_hSystem = hSystem;

// If in debug mode the events will be named, otherwise they won't
// We're appending the this pointer so the named events will be unique if
// we're using independent systems.
#ifdef __linux__ //RG - CHECK IF WE WANT THESE UNICODE ??
#ifdef _DEBUG
	wchar_t lpReleaseSystemEvent[100];
	wchar_t lpThreadEndEvent[100];
	wchar_t lpThreadEndEventTxfer[100];
	wchar_t lpTriggerEvent[100];
	wchar_t lpAcquisitionEndEvent[100];
	wchar_t lpAppTriggerEvent[100];
	wchar_t lpAppAcquisitionEndEvent[100];
	wchar_t lpEndTransferEvent[100];

	wcscpy(lpReleaseSystemEvent, L"CsReleaseSystemEvent");
	wcscpy(lpThreadEndEvent, L"CsThreadEndEvent");
	wcscpy(lpThreadEndEventTxfer, L"CsThreadEndEventTxfertchThread");
	wcscpy(lpTriggerEvent, L"CsTriggerEventtchThread");
	wcscpy(lpAcquisitionEndEvent, L"CsAcquisitionEndEvent");
	wcscpy(lpAppTriggerEvent, L"CsAppTriggerEvent");
	wcscpy(lpAppAcquisitionEndEvent, L"CsAppAcquisitionEndEvent");
	wcscpy(lpEndTransferEvent, L"CsEndTransferEvent");
#else
	LPCWSTR lpReleaseSystemEvent = NULL;
	LPCWSTR lpThreadEndEvent = NULL;
	LPCWSTR lpThreadEndEventTxfer = NULL;
	LPCWSTR lpTriggerEvent = NULL;
	LPCWSTR lpAcquisitionEndEvent = NULL;
	LPCWSTR lpAppTriggerEvent = NULL;
	LPCWSTR lpAppAcquisitionEndEvent = NULL;
	LPCWSTR lpEndTransferEvent = NULL;
#endif
#else // __linux__
#ifdef _DEBUG
TCHAR lpReleaseSystemEvent[100];
	TCHAR lpThreadEndEvent[100];
	TCHAR lpThreadEndEventTxfer[100];
	TCHAR lpTriggerEvent[100];
	TCHAR lpAcquisitionEndEvent[100];
	TCHAR lpAppTriggerEvent[100];
	TCHAR lpAppAcquisitionEndEvent[100];
	TCHAR lpEndTransferEvent[100];
	DWORD dwPid = ::GetCurrentProcessId();

	_stprintf_s(lpReleaseSystemEvent, _countof(lpReleaseSystemEvent), _T("_CsReleaseSystemEvent_%08x_%08x"), hSystem, dwPid);
	_stprintf_s(lpThreadEndEvent, _countof(lpThreadEndEvent), _T("_CsThreadEndEvent_%08x_%08x"), hSystem, dwPid);
	_stprintf_s(lpThreadEndEventTxfer, _countof(lpThreadEndEventTxfer), _T("_CsThreadEndEventTxfer_%08x"), hSystem, dwPid);
	_stprintf_s(lpTriggerEvent, _countof(lpTriggerEvent), _T("_CsTriggerEvent_%08x_%08x"), hSystem, dwPid);
	_stprintf_s(lpAcquisitionEndEvent, _countof(lpAcquisitionEndEvent), _T("_CsAcquisitionEndEvent_%08x_%08x"), hSystem, dwPid);
	_stprintf_s(lpAppTriggerEvent, _countof(lpAppTriggerEvent), _T("_CsAppTriggerEvent_%08x_%08x"), hSystem, dwPid);
	_stprintf_s(lpAppAcquisitionEndEvent, _countof(lpAppAcquisitionEndEvent), _T("_CsAppAcquisitionEndEvent_%08x_%08x"), hSystem, dwPid);
	_stprintf_s(lpEndTransferEvent, _countof(lpEndTransferEvent), _T("_CsEndTransferEvent_%08x_%08x"), hSystem, dwPid);


#else
	LPSTR lpReleaseSystemEvent = NULL;
	LPSTR lpThreadEndEvent = NULL;
	LPSTR lpThreadEndEventTxfer = NULL;
	LPSTR lpTriggerEvent = NULL;
	LPSTR lpAcquisitionEndEvent = NULL;
	LPSTR lpAppTriggerEvent = NULL;
	LPSTR lpAppAcquisitionEndEvent = NULL;
	LPSTR lpEndTransferEvent = NULL;
#endif
#endif


	// Create Event Handle for Driver synchronisation
#ifdef __linux__
	m_hReleaseSystemEvent = ::CreateEvent(NULL, TRUE, FALSE, (const WCHAR *)lpReleaseSystemEvent);
#else
	m_hReleaseSystemEvent = ::CreateEvent(NULL, TRUE, FALSE, lpReleaseSystemEvent);
#endif
	m_hThreadEndEvent = ::CreateEvent(NULL, TRUE, FALSE, lpThreadEndEvent);
	m_hThreadEndEventTxfer = ::CreateEvent(NULL, TRUE, FALSE, lpThreadEndEventTxfer);
	m_hTriggerEvent = ::CreateEvent(NULL, FALSE, FALSE, lpTriggerEvent);
	m_hAcquisitionEndEvent = ::CreateEvent(NULL, FALSE, FALSE, lpAcquisitionEndEvent);

	m_hEndTransferEvent = ::CreateEvent(NULL, FALSE, FALSE, lpEndTransferEvent);

	// Same event for Application level
#ifdef _WIN32
	m_hApp_TriggerEvent = CreateEvent(NULL, TRUE, FALSE, lpAppTriggerEvent);
	m_hApp_AcquisitionEndEvent = CreateEvent(NULL, TRUE, FALSE, lpAppAcquisitionEndEvent);
#else
	int status;
	
	status = pipe(m_hApp_TriggerEvent);
	status = fcntl(m_hApp_TriggerEvent[0], F_GETFL);
	fcntl(m_hApp_TriggerEvent[0], F_SETFL, status | O_NONBLOCK);
	
	status = pipe(m_hApp_AcquisitionEndEvent);
	status = fcntl(m_hApp_AcquisitionEndEvent[0], F_GETFL);
	fcntl(m_hApp_AcquisitionEndEvent[0], F_SETFL, status | O_NONBLOCK);
#endif /* _WIN32 */

	// Allocate some IN_PARAMS_Structure for AS Transfer
	PIN_PARAMS_TRANSFERDATA pIn_Data = NULL;
	for (int i = 0; i < 4; i++)
	{
		pIn_Data = static_cast<PIN_PARAMS_TRANSFERDATA>(::VirtualAlloc(0, sizeof(IN_PARAMS_TRANSFERDATA), MEM_COMMIT, PAGE_READWRITE));

		m_InDataIndexVector.push_back(0);
		m_InDataStructVector.push_back(pIn_Data);
	}
}

CStateMachine::~CStateMachine(void)
{
	::DeleteCriticalSection(&m_Sec);
	delete _fsm;

	::CloseHandle(m_hReleaseSystemEvent);
	::CloseHandle(m_hThreadEndEvent);
	::CloseHandle(m_hThreadEndEventTxfer);
	::CloseHandle(m_hTriggerEvent);
	::CloseHandle(m_hAcquisitionEndEvent);

#ifdef _WIN32
	::CloseHandle(m_hApp_TriggerEvent);
	::CloseHandle(m_hApp_AcquisitionEndEvent);
#else
	close(m_hApp_TriggerEvent[0]);
	close(m_hApp_TriggerEvent[1]);
	close(m_hApp_AcquisitionEndEvent[0]);
	close(m_hApp_AcquisitionEndEvent[1]);
#endif /* _WIN32 */

	::CloseHandle(m_hEndTransferEvent);

	m_InDataIndexVector.clear();

	for (int i = 0; i < 4; i++)
	{
		::VirtualFree(m_InDataStructVector[i], 0, MEM_RELEASE);
		m_InDataStructVector[i] = NULL;
	}

	m_InDataStructVector.clear();
}


//!
//! \fn	int32 RegisterDriverEvents(CSHANDLE hSystem);
//!	\brief Register events with low-level driver for synchronism
//!
//! The events are created when the system is added to the singleton object.
//!
//!	\param hSystem : Handle of system to register
//!
//! \return
//!       - CS_SUCCESS: Operation was successful
//!       - CS_INVALID_EVENT_TYPE: Invalid event type (ACQ_EVENT_TRIGGERED, ACQ_EVENT_END_BUSY)
//!		  - CS_INVALID_HANDLE: Handle is insalid with low-level driver
/*************************************************************/
int32 CStateMachine::RegisterDriverEvents(CSHANDLE hSystem)
{
	int32 i32RetVal = CS_SUCCESS;

	i32RetVal = ::CsDrvRegisterEventHandle(hSystem, ACQ_EVENT_TRIGGERED, (HANDLE *)&m_hTriggerEvent);
	if (CS_FAILED(i32RetVal))
	{
		// Disabled Interrupt - Fall back to polling
		::CloseHandle(m_hTriggerEvent);
		::CloseHandle(m_hAcquisitionEndEvent);
		m_hTriggerEvent = m_hAcquisitionEndEvent = NULL;
		
#ifdef _WIN32
		::CloseHandle(m_hApp_TriggerEvent);
		::CloseHandle(m_hApp_AcquisitionEndEvent);
		m_hApp_TriggerEvent = m_hApp_AcquisitionEndEvent = NULL;
#else
		close(m_hApp_TriggerEvent[0]);
		close(m_hApp_TriggerEvent[1]);
		close(m_hApp_AcquisitionEndEvent[0]);
		close(m_hApp_AcquisitionEndEvent[1]);
		m_hApp_TriggerEvent[0] = m_hApp_AcquisitionEndEvent[0] = -1;
		m_hApp_TriggerEvent[1] = m_hApp_AcquisitionEndEvent[1] = -1;
#endif /* _WIN32 */

		return (CS_DRIVER_ASYNC_NOT_SUPPORTED);
	}

	i32RetVal = ::CsDrvRegisterEventHandle(hSystem, ACQ_EVENT_END_BUSY, (HANDLE *)&m_hAcquisitionEndEvent);
	if (CS_FAILED(i32RetVal))
	{
		// Disabled Interrupt - Fall back to polling
		::CloseHandle(m_hTriggerEvent);
		::CloseHandle(m_hAcquisitionEndEvent);
		m_hTriggerEvent = m_hAcquisitionEndEvent = NULL;
		
#ifdef _WIN32
		::CloseHandle(m_hApp_TriggerEvent);
		::CloseHandle(m_hApp_AcquisitionEndEvent);
		m_hApp_TriggerEvent = m_hApp_AcquisitionEndEvent = NULL;
#else
		close(m_hApp_TriggerEvent[0]);
		close(m_hApp_TriggerEvent[1]);
		close(m_hApp_AcquisitionEndEvent[0]);
		close(m_hApp_AcquisitionEndEvent[1]);
		m_hApp_TriggerEvent[0] = m_hApp_AcquisitionEndEvent[0] = -1;
		m_hApp_TriggerEvent[1] = m_hApp_AcquisitionEndEvent[1] = -1;
#endif /* _WIN32 */

		return (CS_DRIVER_ASYNC_NOT_SUPPORTED);
	}

	return (i32RetVal);
}


//!
//! \fn	void SignalEvent(int16 EventType);
//!	\brief Signal user application on driver event
//!
//!	\param EventType : ACQ_EVENT_END_BUSY or ACQ_EVENT_TRIGGERED
//!
//! \return
//!       - void
/*************************************************************/
void CStateMachine::SignalEvent(int16 EventType)
{
#ifdef __linux__
   int len = 0;
#endif   
   
	switch (EventType)
	{
		case ACQ_EVENT_END_BUSY:
#ifdef _WIN32
			::SetEvent(m_hApp_AcquisitionEndEvent);
#else
			len = write(m_hApp_AcquisitionEndEvent[1], "!", 1);
#endif /* _WIN32 */
			break;

		case ACQ_EVENT_TRIGGERED:
#ifdef _WIN32
			::SetEvent(m_hApp_TriggerEvent);
#else
			len = write(m_hApp_TriggerEvent[1], "!", 1);
#endif /* _WIN32 */
			break;

		case ACQ_EVENT_END_TXFER:
#ifdef _WIN32
			::SetEvent(m_hAppEndTransferEvent);
#else
			len = write(m_hAppEndTransferEvent_write, "!", 1);
#endif /* _WIN32 */
			break;
	}
}

#ifdef __linux__
static void empty_pipe(int fd) {
	static char buf[1024];
	int len;
	
	do {
		len = read(fd, buf, 1024);
	} while (len > 0);
}
#endif // __linux__ 

//!
//! \fn	void ClearEvent(int16 EventType);
//!	\brief Reset user application on driver event
//!
//!	\param EventType : ACQ_EVENT_END_BUSY or ACQ_EVENT_TRIGGERED
//!
//! \return
//!       - void
/*************************************************************/
void CStateMachine::ClearEvent(int16 EventType)
{
	switch (EventType)
	{
		case ACQ_EVENT_END_BUSY:
#ifdef _WIN32
			::ResetEvent(m_hApp_AcquisitionEndEvent);
#else
			empty_pipe(m_hApp_AcquisitionEndEvent[0]);
#endif /* _WIN32 */
			break;

		case ACQ_EVENT_TRIGGERED:
#ifdef _WIN32
			::ResetEvent(m_hApp_TriggerEvent);
#else
			empty_pipe(m_hApp_TriggerEvent[0]);
#endif /* _WIN32 */
			break;

		case ACQ_EVENT_END_TXFER:
#ifdef _WIN32
			::ResetEvent(m_hAppEndTransferEvent);
#else
			empty_pipe(m_hAppEndTransferEvent_read);
#endif /* _WIN32 */
			break;
	}
}

//!
//! \fn	int32 RetrieveStateID(void) const ;
//!	\brief Return the ID of the current state
//!
//! \return
//!       - State ID
/*************************************************************/
int32 CStateMachine::RetrieveStateID(void)
{
	::EnterCriticalSection(&m_Sec);

	int32 i32Id = _fsm->getState().getId();

	::LeaveCriticalSection(&m_Sec);

	return i32Id;
}


//!
//! \fn	int32 RetrieveStateName(LPTSTR szBuffer, int16 nMaxCount) const ;
//!	\brief Return the name of the current state
//!
//!	\param szBuffer : Buffer to output name to
//!	\param nMaxCount: size of buffer
//!
//! \return
//!       - Number of Bytes copied to buffer
/*************************************************************/
int32 CStateMachine::RetrieveStateName(LPTSTR szBuffer, int16 nMaxCount)
{
	int32	nBytesCopied = 0;
	TCHAR	szName[36];

	::EnterCriticalSection(&m_Sec);

	lstrcpyn(szName, _fsm->getState().getName(), 35);

	::LeaveCriticalSection(&m_Sec);

#if defined(_WIN32)
	nBytesCopied = _cpp_min(nMaxCount-1, lstrlen(szName));
#else
	nBytesCopied = MIN(nMaxCount-1, int(lstrlen(szName)));
#endif
	lstrcpyn(szBuffer, szName, nBytesCopied+1);

	return (nBytesCopied);
}


//!
//! \fn	bool PerformTransition(int16 nTransition, LPTSTR szNewState = NULL, int16 nMaxCount = 0);
//!	\brief Perfrom the requested transition on the state machine
//!
//!	\param nTransition : Transition to be performed (from TRANSITIONS)
//!	\param szNewState : Buffer to output name to if desired
//!	\param nMaxCount: size of buffer
//!
//! \return
//!       - Always true
/*************************************************************/
bool CStateMachine::PerformTransition(int16 nTransition)
{
	bool bRet = true;
	
	::EnterCriticalSection(&m_Sec);
	try
	{
		switch(nTransition)
		{
			case INIT_TRANSITION:
				_fsm->init();
				break;

			case  ABORT_TRANSITION:
				_fsm->abort();
				break;

			case  BADMODIFY_TRANSITION:
				_fsm->bad_modify();

				break;

			case  COERCE_TRANSITION:
				_fsm->coerce();
				break;

			case  DONE_TRANSITION:
				_fsm->done();
				break;

			case  FORCETRIGGER_TRANSITION:
				_fsm->force_Trigger();
				break;

			case  HWEVENT_TRANSITION:
				_fsm->hw_event();
				break;

			case  INVALID_TRANSITION:
				_fsm->invalid();
				break;

			case  MODIFY_TRANSITION:
				_fsm->modify();
				break;

			case  READ_TRANSITION:
				_fsm->read();
				break;

			case  START_TRANSITION:
				_fsm->start();
				break;

			case  TRANSFER_TRANSITION:
				_fsm->transfer();
				break;

			case  VALID_TRANSITION:
				_fsm->valid();
				break;

			case COMMIT_TRANSITION:
				_fsm->commit();
				break;

			case RESET_TRANSITION:
				_fsm->reset();
				break;

			case CALIBRATE_TRANSITION:
				_fsm->calibrate();
				break;

#if defined(_WIN32)
			case DISKSTREAM_TRANSITION:
				_fsm->start_stream();
				break;
#endif
			default: // Unknow transition
				bRet = false;
				break;
		}

		if( bRet )
		{
//RG		TRACE(STATE_TRACE_LEVEL, _fsm->getState().getName());
		}
	}
	catch( int nError )
	{
		bRet = false;
		if (ERROR_INVALID_TRANSITION != nError)
		{
			_ASSERT(!"Unhandled nError code");
		}
	}

#if defined(_WIn32)
	catch(...)
	{
		_ASSERT(!"Unhandled exception");
		bRet = false;
	}
#endif

	::LeaveCriticalSection(&m_Sec);
	return bRet;
}

void CStateMachine::SetFlag(int32 )
{
}

void CStateMachine::ResetFlags(void)
{
}

LPCTSTR CStateMachine::Dump(void)
{
	return (NULL);
}

void CStateMachine::Set_Debug_Trace(int8 bEnable, ostream& debug_stream)
{
	if (bEnable)
	{
		_fsm->setDebugFlag(true);
		_fsm->setDebugStream(debug_stream);
	}
	else
		_fsm->setDebugFlag(false);
}

void CStateMachine::LockSM(bool bEnable)
{
	m_bSMLock = bEnable;
}

bool CStateMachine::IsSMLock()
{
	return (m_bSMLock);
}

void CStateMachine::Abort(bool bAbort)
{
	m_bAborted = bAbort;
}

bool CStateMachine::IsAborted()
{
	return (m_bAborted);
}

void CStateMachine::RaiseError(int nError)
{
	throw (nError);
}

PIN_PARAMS_TRANSFERDATA CStateMachine::AllocateInStruct(void)
{
	// Find the first available index
	size_t nSize = m_InDataIndexVector.size();

	for (size_t i = 0; i < nSize; i++)
	{
		if (0 == m_InDataIndexVector[i])
		{
			m_InDataIndexVector[i] = 1;
			return (m_InDataStructVector[i]);
		}
	}

	return (NULL);
}

void CStateMachine::ReleaseInStruct(PIN_PARAMS_TRANSFERDATA pData)
{
	// Find the matching pointer
	size_t nSize = m_InDataStructVector.size();

	for (size_t i = 0; i < nSize; i++)
	{
		if (pData == m_InDataStructVector[i])
		{
			m_InDataIndexVector[i] = 0;
			ZeroMemory(pData, sizeof(IN_PARAMS_TRANSFERDATA));
		}
	}
}
