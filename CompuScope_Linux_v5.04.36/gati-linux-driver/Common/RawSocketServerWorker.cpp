/////////////////////////////////////////////////////////////////////
// Class Creator Version 2.0.000 Copyrigth (C) Poul A. Costinsky 1994
///////////////////////////////////////////////////////////////////
// Implementation File RawSocketServerWorker.cpp
// class CWizRawSocketServerWorker
//
// 16/07/1996 17:53                             Author: Poul
///////////////////////////////////////////////////////////////////
 
#if defined (_WIN32)
#include <crtdbg.h>
#else

#include <pthread.h>
#include <sys/time.h>
#include <errno.h>

#ifdef _DEBUG
#include <stdio.h> // for printf and sprintf
#endif

#endif

#include "rm.h"
#include "misc.h"
#include "RawSocketServerWorker.h"

#if defined (_WIN32)
#include "Debug.h"
#endif

#if defined(GAGE_FCI)
#define MODULE_NAME _T("GageServer")
#else
#define MODULE_NAME _T("RM")
#endif


#if defined (_WIN32) // RICKY - CHECK THIS
// Events to interrupt blocking socket functions.
__declspec( thread ) static const int HOOK_CANCEL_EVENTS = 2;
__declspec( thread ) static HANDLE sh_HookCanselEvents[HOOK_CANCEL_EVENTS] = { NULL, NULL };
__declspec( thread ) static int nHookEventsInstalled = 0;
#endif
///////////////////////////////////////////////////////////////////
//*****************************************************************
inline void ThrowIfNull(void* p)
{
	if (p == NULL)	
		throw ("Memory exception"); 
}
//*****************************************************************
#if defined (_WIN32) // RICKY - CHECK ABOUT LINUX
// Function called from Windows Sockets blocking hook function
inline BOOL TestCancelEvents (DWORD TimeOut = 0)
{
	_ASSERT(nHookEventsInstalled <= HOOK_CANCEL_EVENTS);
	for(INDEX i = 0; i < nHookEventsInstalled; i++)
		_ASSERT(sh_HookCanselEvents[i] != NULL);
	// Tests events - if one signalled, should cancel blocking function.
	return (::WaitForMultipleObjects (nHookEventsInstalled, sh_HookCanselEvents, FALSE, TimeOut) != WAIT_TIMEOUT);
}
//*****************************************************************

// Windows Sockets blocking hook function
BOOL WINAPI BlockingHook(void) 
{
	// As simple as...
	if (::TestCancelEvents(0))
		WSACancelBlockingCall();
	return 0;
}
#endif // _WIN32 RICKY - NEED TO CHECK THESE FOR LINUX
///////////////////////////////////////////////////////////////////
// class CWizRawSocketListener
///////////////////////////////////////////////////////////////////
//**********************************************************//
// Default Constructor										//
CWizRawSocketListener::CWizRawSocketListener(int nPort)     //
	: m_pListenSocket (NULL),m_nPort(nPort) //
{	
	m_pListenSocket = NULL;
	m_nPort = nPort;
}

CWizRawSocketListener::CWizRawSocketListener()
{
}

//
//**********************************************************//
// Destructor												//
CWizRawSocketListener::~CWizRawSocketListener()				//
{															//
	_ASSERT(m_pListenSocket == NULL);						//
	delete m_pListenSocket;									//
}															//
//**********************************************************//
// Method called from dispath thread.
void CWizRawSocketListener::Prepare ()
{
	// Create listening socket
	_ASSERT(m_pListenSocket == NULL);
	delete m_pListenSocket;
	
	m_pListenSocket = new CWizSyncSocket (m_nPort);
	ThrowIfNull (m_pListenSocket);
#if defined (_WIN32) // RICKY - CHECK FOR LINUX
	_ASSERT(nHookEventsInstalled == 0);
#endif

#ifdef _DEBUG
	TCHAR buff[100];

	unsigned int nP;
	if (m_pListenSocket->GetHostName(buff, 100, nP))
	{
		TRACE(SOCKET_TRACE_LEVEL, "%s::Listening at %s:%u\n", MODULE_NAME, buff, nP);
	}
	else
		TRACE(SOCKET_TRACE_LEVEL, "%s::Can't get host name\n", MODULE_NAME);
#endif
}
//*****************************************************************
// Method called from dispath thread.
void CWizRawSocketListener::CleanUp()
{
	// close and destroy listening socket
	delete m_pListenSocket;
	m_pListenSocket = NULL;
}

//*****************************************************************
#if defined (_WIN32) // RICKY - CHECK IN LINUX
struct EventInstall
{
	EventInstall (HANDLE h, int &n): m_n (n)
	{
		_ASSERT(nHookEventsInstalled <= m_n);
		if (nHookEventsInstalled > m_n)
			throw CWizRawSocketListener::XCannotSetHookEvent();
		sh_HookCanselEvents [m_n] = h;
		nHookEventsInstalled = m_n + 1;
		n++;
	}
	~EventInstall ()
	{
		_ASSERT(nHookEventsInstalled == m_n + 1);
		sh_HookCanselEvents [m_n] = NULL;
		nHookEventsInstalled = m_n;
	}
	int m_n;
};
#endif // _WIN32 - RICKY - CHECK IN LINUX
//*****************************************************************
// Method called from dispath thread.
#if defined (_WIN32)
BOOL CWizRawSocketListener::WaitForData (HANDLE hShutDownEvent)
#else
BOOL CWizRawSocketListener::WaitForData (pthread_cond_t& hShutDownEvent)
#endif
{
	m_hAcceptedSocket = INVALID_SOCKET;
#if defined (_WIN32)
	TRACE(SOCKET_TRACE_LEVEL, "%s::Waiting for data thread %lu\n", MODULE_NAME, ::GetCurrentThread());

	// Install shutdown event.
	int n = 0;
	EventInstall ei (hShutDownEvent, n);
#endif // _WIN32 RICKY - HAVE TO CHECK FOR LINUX
	// Maybe set hook.
	// Accept pending connections or wait.
	for (;;)
	{
		// Get accepted socket.
		SOCKET h = m_pListenSocket->Accept();
		// Shutdown?
#if defined (_WIN32)
		if (::WaitForSingleObject(hShutDownEvent,0) != WAIT_TIMEOUT)
			return FALSE;
#else
		int nRetVal = 0;
		struct timeval now;
		struct timezone tz;
		struct timespec timeout;
		//RICKY - CHECK IF I'VE ALREADY GOT A MUTEX AVAILABLE
//		pthread_mutex_t hShutDownMutex = PTHREAD_MUTEX_INITIALIZER; // RG

		pthread_mutex_t hShutDownMutex;
		pthread_mutexattr_t mutexattr;
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);		
		pthread_mutexattr_init(&mutexattr);	
		pthread_mutex_init(&hShutDownMutex, &mutexattr);	
		pthread_mutexattr_destroy(&mutexattr);	


		::pthread_mutex_lock(&hShutDownMutex);
		gettimeofday(&now, &tz);
		timeout.tv_sec = now.tv_sec;
		timeout.tv_nsec = now.tv_usec * 1000;
		nRetVal = ::pthread_cond_timedwait(&hShutDownEvent, &hShutDownMutex, &timeout);
		::pthread_mutex_unlock(&hShutDownMutex);
		if (nRetVal != ETIMEDOUT)
		{
			// Should be a manual event for shutdown, signal it again...
			::pthread_mutex_lock(&hShutDownMutex);
			::pthread_cond_broadcast(&hShutDownEvent);
			::pthread_mutex_unlock(&hShutDownMutex);

			return FALSE;
		}
#endif
		// If it's connected client, go to serve it.
		if (h != INVALID_SOCKET)
		{
			m_hAcceptedSocket = h;
			return TRUE;
		}
	} // for (;;)
}
//*****************************************************************
// Method called from dispath thread.
#if defined (_WIN32)
BOOL CWizRawSocketListener::TreatData(HANDLE hShutDownEvent, HANDLE hDataTakenEvent)
#else
BOOL CWizRawSocketListener::TreatData(pthread_cond_t* hShutDownEvent, pthread_cond_t& hDataTakenEvent)
#endif
{

#if defined (_WIN32) // RICKY - HAVE TO CHECK IN LINUX
	TRACE(SOCKET_TRACE_LEVEL, "%s::Treat data thread %lu\n", MODULE_NAME, ::GetCurrentThread());

	int n = 0;
	// Install shutdown event.
	EventInstall ei (hShutDownEvent,n);
#endif // _WIN32 HAVE TO CHECK
	// Create client side socket to communicate with client.
	CWizReadWriteSocket clientSide (m_hAcceptedSocket);
#if defined (_WIN32)
	// Signal dispather to continue waiting.
	::SetEvent(hDataTakenEvent);
#else
//	pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER; //RG
	
	pthread_mutex_t mut;
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);	
	pthread_mutexattr_init(&mutexattr);	
	pthread_mutex_init(&mut, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);
		
	pthread_mutex_lock(&mut);
	::pthread_cond_broadcast(&hDataTakenEvent);
	pthread_mutex_unlock(&mut);
#endif
	// Maybe set hook.
#ifdef _DEBUG

	TCHAR buff[100];
	unsigned int nP = 0;
	clientSide.GetPeerName (buff,100,nP);
	TRACE(SOCKET_TRACE_LEVEL, "%s::Connected to %s:%d\n", MODULE_NAME, buff, 0);
#endif

	for(;;)
	{
		// Shutdown?
#if defined (_WIN32)
		if (::WaitForSingleObject(hShutDownEvent, 0) == WAIT_OBJECT_0)
			return FALSE;
#else
		int nRetVal = 0;
		struct timeval now;
		struct timezone tz;
		struct timespec timeout;
		//RICKY - CHECK IF I'VE ALREADY GOT A MUTEX AVAILABLE

//		pthread_mutex_t hShutDownMutex = PTHREAD_MUTEX_INITIALIZER; // RG

		pthread_mutex_t hShutDownMutex;
		pthread_mutexattr_t mutexattr;
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);		
		pthread_mutexattr_init(&mutexattr);	
		pthread_mutex_init(&hShutDownMutex, &mutexattr);
		pthread_mutexattr_destroy(&mutexattr);


		::pthread_mutex_lock(&hShutDownMutex);
		gettimeofday(&now, &tz);
		timeout.tv_sec = now.tv_sec;
		timeout.tv_nsec = now.tv_usec * 1000;
		nRetVal = ::pthread_cond_timedwait(hShutDownEvent, &hShutDownMutex, &timeout);
		::pthread_mutex_unlock(&hShutDownMutex);
		if (nRetVal != ETIMEDOUT)
		{
			// Should be a manual event for shutdown, signal it again...
			::pthread_mutex_lock(&hShutDownMutex);
			::pthread_cond_broadcast(hShutDownEvent);
			::pthread_mutex_unlock(&hShutDownMutex);

			return FALSE;
		}
#endif
		// Exchange with client.
		if (!ReadWrite(clientSide))
			break;

	}
	return TRUE;
}
//*****************************************************************

