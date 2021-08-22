#include "rm.h"
#include "ThreadDispatcher.h"

#if defined (_WIN32)
#include "misc.h"
#include <process.h>

#else
#include <unistd.h>
#endif

///////////////////////////////////////////////////////////////////
// class CWizThreadDispatcher
//*****************************************************************
// Default Constructor
CWizThreadDispatcher::CWizThreadDispatcher(CWizMultiThreadedWorker &rWorker, int MaxThreads)
		: m_nMaxThreads(MaxThreads),
		m_rWorker(rWorker),
		m_ShutDownEvent(TRUE),
		m_HasDataEvent (FALSE),
		m_StartedDataTreatEvent(FALSE),
#if defined (_WIN32)
	m_hThisThread(NULL),
#else
	m_hThisThread(0),
#endif
	m_ahWorkersHandles(NULL)
{
#if defined (_WIN32)
	m_ahStartedTreatmentEvents[0] = m_ShutDownEvent.m_h;
	m_ahStartedTreatmentEvents[1] = m_StartedDataTreatEvent.m_h;
	m_ahWorkersHandles = new HANDLE[m_nMaxThreads + 1];
#else
	m_ahStartedTreatmentEvents[0] = &m_ShutDownEvent.m_h;
	m_ahStartedTreatmentEvents[1] = &m_StartedDataTreatEvent.m_h;
	m_ahWorkersHandles = new pthread_t[m_nMaxThreads + 1];
#endif

	if (m_ahWorkersHandles == NULL)
		throw ("Memory exception");
	for (INDEX i = 0; i < m_nMaxThreads; i++)
#if defined (_WIN32)
		m_ahWorkersHandles[i] = NULL;
#else
		m_ahWorkersHandles[i] = 0;
#endif

}
//*****************************************************************
// Destructor
CWizThreadDispatcher::~CWizThreadDispatcher()
{
#if defined (_WIN32) // RICKY - FIX FOR LINUX
	for (INDEX i = 0; i < m_nMaxThreads; i++) // RICKY - FIX FOR LINUX
		::CloseHandle(m_ahWorkersHandles[i]);
#endif
	delete [] m_ahWorkersHandles;
	Stop(FALSE);
}
//*****************************************************************
void	CWizThreadDispatcher::Start()
{
#if defined (_WIN32)

	unsigned int threadID;
	m_hDispatchThread = (HANDLE)::_beginthreadex(NULL, 0, CWizThreadDispatcherFun, this, 0, &threadID);

#else

	pthread_t threadID;
	pthread_attr_t threadattr;
	pthread_attr_init(&threadattr);
	pthread_attr_setdetachstate(&threadattr, PTHREAD_CREATE_DETACHED);
	int ret = ::pthread_create(&threadID, &threadattr, /*DEC*/ CWizThreadDispatcherFun, (void *)this);
	if (ret != 0)
		throw ("Memory exception");

	(void)pthread_mutex_init(&m_mFinishMutex ,NULL);
	pthread_cond_init(&m_hDispatchThread, NULL);

#endif
}
//*****************************************************************
void	CWizThreadDispatcher::Stop(BOOL bWait)
{
	m_ShutDownEvent.Set();
#if defined (_WIN32)
	::CloseHandle(m_hDispatchThread); // RICKY
#endif

	if (bWait)
		WaitShutDown();
}

//*****************************************************************
void	CWizThreadDispatcher::WaitShutDown()
{
#if defined (_WIN32)
	DWORD dwWaitRes = ::WaitForSingleObject(m_hDispatchThread, INFINITE);
	// Wait for all destructors to be called.
	if (dwWaitRes != WAIT_OBJECT_0)
		::DisplayErrorMessage(::GetLastError());
#else
	pthread_mutex_lock(&m_mFinishMutex);

	// pthread_cond_wait never returns an error code
	::pthread_cond_wait(&m_hDispatchThread, &m_mFinishMutex);
	pthread_mutex_unlock(&m_mFinishMutex);
#endif
	return;
}
//*****************************************************************
#ifdef __linux__
void* CWizThreadDispatcherFun(LPVOID pParam)
#else
UINT __stdcall CWizThreadDispatcherFun(LPVOID pParam)
#endif
{
	_ASSERT(pParam != NULL);
#ifdef __linux__
	((CWizThreadDispatcher *)pParam)->Run();
	pthread_exit(NULL);
#else
	return ((CWizThreadDispatcher *)pParam)->Run();
#endif
}
//*****************************************************************
UINT 	CWizThreadDispatcher::Run()
{
	CWizMultiThreadedWorker::Stack stack(m_rWorker);
#if defined (_WIN32)
	m_hThisThread = ::GetCurrentThread();
	m_ahWorkersHandles[m_nMaxThreads] = m_hThisThread; //RICKY
#else
	m_hThisThread = ::pthread_self();
	m_ahWorkersHandles[m_nMaxThreads] = m_hThisThread;
#endif // _WIN32
	//---------------------------------------------------------
	// Start all working threads

	for (INDEX i = 0; i < m_nMaxThreads; i++)
	{
		WorkerThread* pTr = new WorkerThread(m_rWorker,
			m_HasDataEvent.m_h,
			m_StartedDataTreatEvent.m_h,
			m_ShutDownEvent.m_h);

		if (pTr == NULL)
			throw("Memory exception");

#if defined (_WIN32)
		unsigned int threadID;
		HANDLE hThr = (HANDLE)::_beginthreadex(NULL, 0, CWizThreadDispatcher_WorkerThread_Fun, pTr, 0, &threadID);
		if (hThr == NULL)
			throw ("Memory exception");
		m_ahWorkersHandles[i] = hThr;
#else
//#define DEC (void *(*)(void *))
		pthread_t threadID;
		pthread_attr_t threadattr;
		pthread_attr_init(&threadattr);
		pthread_attr_setdetachstate(&threadattr, PTHREAD_CREATE_DETACHED);
		int hThr = ::pthread_create(&threadID, &threadattr, CWizThreadDispatcher_WorkerThread_Fun, (void *)pTr);
		if (hThr != 0)
			throw ("Memory exception");
		m_ahWorkersHandles[i] = threadID;
#endif
	}
	//---------------------------------------------------------
	for (;;)
	{
		if (!m_rWorker.WaitForData(m_ShutDownEvent.m_h))
		{
			m_ShutDownEvent.Set();
			goto end;
		}

		m_HasDataEvent.Set();

#if defined (_WIN32) // RICKY - FIX FOR LINUX
		const DWORD res =
			::WaitForMultipleObjects(StartedTreatmentEventsCount, m_ahStartedTreatmentEvents, FALSE, INFINITE);
		if (res == WAIT_FAILED)
		{
			int	ret = ::GetLastError();
			::DisplayErrorMessage(ret);
		}

		switch (res)
		{
			case WAIT_OBJECT_0: goto end;
			case (WAIT_OBJECT_0 + 1): break;// Worker thread started to treat data

			case WAIT_FAILED:
				throw XWaitFailed();// something wrong!

			default:		_ASSERT(0);
		}
#else

//		pthread_mutex_t hShutDownMutex = PTHREAD_MUTEX_INITIALIZER;			// RG
//		pthread_mutex_t hDataTreatmentStartedMutex = PTHREAD_MUTEX_INITIALIZER; // RG

		pthread_mutex_t hShutDownMutex;;
		pthread_mutex_t hDataTreatmentStartedMutex;
		pthread_mutexattr_t mutexattr;
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);		
		pthread_mutexattr_init(&mutexattr);	
		pthread_mutex_init(&hShutDownMutex, &mutexattr);		
		pthread_mutex_init(&hDataTreatmentStartedMutex, &mutexattr);	
		pthread_mutexattr_destroy(&mutexattr);			
		
		
		int nRetVal = 0;
		struct timeval now;
		struct timezone tz;
		struct timespec timeout;
		::pthread_mutex_lock(&hShutDownMutex);
		::pthread_mutex_lock(&hDataTreatmentStartedMutex);

		gettimeofday(&now, &tz);
		timeout.tv_sec = now.tv_sec + 2;
		timeout.tv_nsec = now.tv_usec * 1000;

		nRetVal = ::pthread_cond_timedwait(m_ahStartedTreatmentEvents[0], &hShutDownMutex, &timeout);

		if (nRetVal != ETIMEDOUT)
		{
			// Should be a manual event for shutdown, signal it again...
			pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

			::pthread_mutex_lock(&mut);
			::pthread_cond_broadcast(m_ahStartedTreatmentEvents[0]);
			::pthread_mutex_unlock(&mut);

			goto end;
		}

#endif // _WIN32
	} // while 1
	//---------------------------------------------------------
end:
	//---------------------------------------------------------

#if defined (_WIN32) // RICKY - FIX FOR LINUX
	for (INDEX i = 0; i < m_nMaxThreads; i++)
		::WaitForSingleObject(m_ahWorkersHandles[i], INFINITE);
#endif
	for(INDEX i = 0; i < 10 ; i++)
		Sleep(100);
	//---------------------------------------------------------
#if !defined (_WIN32) // for linux we have to broadcast the thread finish condition
        pthread_cond_broadcast(&m_hDispatchThread);
#endif // _WIN32
	return 0;
}
//*****************************************************************
///////////////////////////////////////////////////////////////////
#if defined (_WIN32)
CWizThreadDispatcher::WorkerThread::WorkerThread(CWizMultiThreadedWorker &rWorker,
				HANDLE hDataReadyEvent,
				HANDLE hStartedTreatEvent,
				HANDLE hShutDownEvent)
	: m_rWorker (rWorker),
	  m_hStartedTreatEvent (hStartedTreatEvent)
{
	m_hDataWait[0] = hShutDownEvent;
	m_hDataWait[1] = hDataReadyEvent;
}
#else
CWizThreadDispatcher::WorkerThread::WorkerThread(CWizMultiThreadedWorker &rWorker,
				pthread_cond_t& hDataReadyEvent,
				pthread_cond_t& hStartedTreatEvent,
				pthread_cond_t& hShutDownEvent)
	: m_rWorker (rWorker),
	m_hStartedTreatEvent (hStartedTreatEvent)
{
	m_hDataWait[0] = &hShutDownEvent;
	m_hDataWait[1] = &hDataReadyEvent;
}
#endif // _WIN32
//*****************************************************************
UINT CWizThreadDispatcher::WorkerThread::Run()
{
	for (;;)
	{
#if defined (_WIN32) // try using WaitForSingleObject like in Linux
		const DWORD res = ::WaitForMultipleObjects(DataWaitHCount, m_hDataWait, FALSE, INFINITE);
		switch (res)
		{
			case WAIT_FAILED: // something wrong!
				throw CWizThreadDispatcher::XWaitFailed();
			case WAIT_OBJECT_0: // Shut down!
				return 0;
			case (WAIT_OBJECT_0 + 1): // Has data to treat
				if (!m_rWorker.TreatData(m_hDataWait[0], m_hStartedTreatEvent))
					return 0;
				break;
			default:
				_ASSERT(0);
		}
#else
//		pthread_mutex_t m_hShutDownMutex = PTHREAD_MUTEX_INITIALIZER;	// RG
//		pthread_mutex_t m_hDataReadyMutex = PTHREAD_MUTEX_INITIALIZER;  // RG
		

		pthread_mutex_t m_hShutDownMutex;;
		pthread_mutex_t m_hDataReadyMutex;
		pthread_mutexattr_t mutexattr;
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);		
		pthread_mutexattr_init(&mutexattr);	
		pthread_mutex_init(&m_hShutDownMutex, &mutexattr);		
		pthread_mutex_init(&m_hDataReadyMutex, &mutexattr);				
		pthread_mutexattr_destroy(&mutexattr);
			
		
		
		
		int nRetVal = 0;

		::pthread_mutex_lock(&m_hShutDownMutex);
		::pthread_mutex_lock(&m_hDataReadyMutex);

		nRetVal = ::pthread_cond_wait(m_hDataWait[1], &m_hDataReadyMutex);

		if (0 == nRetVal)
		{
			if (!m_rWorker.TreatData(m_hDataWait[0], m_hStartedTreatEvent))
			{
				// Should be a manual event for shutdown, signal it again...
		        pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

		        ::pthread_mutex_lock(&mut);
		        ::pthread_cond_broadcast(m_hDataWait[0]);
		        ::pthread_mutex_unlock(&mut);

				::pthread_mutex_unlock(&m_hDataReadyMutex);
				::pthread_mutex_unlock(&m_hShutDownMutex);

				return 0;
			}
		}
		else if(EINTR == nRetVal)
			throw CWizThreadDispatcher::XWaitFailed();

#endif // _WIN32
	} // for (;;)
}
//*****************************************************************
#ifdef __linux__
void *CWizThreadDispatcher_WorkerThread_Fun(LPVOID pParam)
#else
UINT __stdcall CWizThreadDispatcher_WorkerThread_Fun(LPVOID pParam)
#endif
{

	CWizThreadDispatcher::WorkerThread* pWorker = (CWizThreadDispatcher::WorkerThread*)pParam;
	_ASSERT(pWorker != NULL);
	UINT res = 0;
	try
	{
		res = pWorker->Run();
	}
	catch(...)
	{
		delete pWorker;
		throw;
	}
	delete pWorker;
#ifndef __linux__
	return res;
#else
	pthread_exit(NULL);
#endif
}
//*****************************************************************
