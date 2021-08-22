////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright:      2001/03/01; Andreas Huber <andreas@huber.net>; Zurich; Switzerland
////////////////////////////////////////////////////////////////////////////////////////////////////
// License:        This file is part of the downloadable accompanying material for the article
//                 "Elegant Function Call Wrappers" in the May 2001 issue of the C/C++ Users Journal
//                 (www.cuj.com). The material may be used, copied, distributed, modified and sold
//                 free of charge, provided that this entire notice (copyright, license and
//                 feedback) appears unaltered in all copies of the material.
//                 All material is provided "as is", without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////////////////////////
// Feedback:       Please send comments to andreas@huber.net.
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "Worker.h"

#if defined(_WIN32)
#include <process.h> // for _beginthreadex
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// A CCriticalSection behaves exactly like a mutex. It can be used for in-process thread
// synchronisation only. CMutex, which can also be used across process boundaries, is considerably
// slower.
////////////////////////////////////////////////////////////////////////////////////////////////////
// A CEvent object can be in one of two states: "not set" or "set". A thread locking a "set" CEvent
// object continues executing and the object becomes "not set". A thread locking a "not set"
// object is halted until SetEvent() is called. The thread then continues and the object becomes
// "not set".
////////////////////////////////////////////////////////////////////////////////////////////////////
// CSingleLock - as used throughout this implementation - simply locks the specified resource in the
// constructor and unlocks it in the destructor. This way, we can neither forget to unlock a
// resource nor will we have any problems with exceptions interrupting us when we don't expect it.
// CAUTION: In some places only an unnamed temporary is created, so the lock is destroyed and the
// resource is unlocked before executing the next statement.
////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////
Worker::Worker() : _IsDestructing( false )
{
	::InitializeCriticalSection(&_pCriticalSection);

#ifdef _DEBUG
#ifdef __linux__
	WCHAR tchThread[32];

	swprintf(tchThread, 32, L"_CsThreadHasStarted_%p", this);
	LPCWSTR lpStartedThread = tchThread;
	swprintf(tchThread, 32, L"_CsWork_%p", this);
	LPCWSTR lpWorkThread = tchThread;
	swprintf(tchThread, 32, L"_CsThreadHasTerminated_%p", this);
	LPCWSTR lpTerminatedThread = tchThread;

#else
	TCHAR lpStartedThread[100];
	TCHAR lpWorkThread[100];
	TCHAR lpTerminatedThread[100];

	DWORD dwPid = ::GetCurrentProcessId();

	_stprintf_s(lpStartedThread, _countof(lpStartedThread), _T("_CsThreadHasStarted_%08x"), dwPid);
	_stprintf_s(lpWorkThread, _countof(lpWorkThread), _T("_CsWork_%08x"), dwPid);
	_stprintf_s(lpTerminatedThread, _countof(lpTerminatedThread),  _T("_CsThreadHasTerminated_%08x"), dwPid);
#endif
#else
#ifdef __linux__
	LPCWSTR lpStartedThread = NULL; 	//TODO: RG - CHANGED FROM LPSTR
	LPCWSTR lpWorkThread = NULL;		//TODO: RG - CHANGED FROM LPSTR
	LPCWSTR lpTerminatedThread = NULL;	//TODO: RG - CHANGED FROM LPSTR
#else

	LPSTR lpStartedThread = NULL;
	LPSTR lpWorkThread = NULL;
	LPSTR lpTerminatedThread = NULL;
#endif

#endif

	_pThreadHasStarted = ::CreateEvent(NULL, FALSE, FALSE, lpStartedThread);
	_pWork = ::CreateEvent(NULL, FALSE, FALSE, lpWorkThread);
	_pThreadHasTerminated = ::CreateEvent(NULL, FALSE, FALSE, lpTerminatedThread);


  // create a new thread, pass the this pointer to controlling function
#ifdef __linux__ //TODO: Make this generic or wrap in a class
	pthread_create(&_pThread, NULL, &ControllingFunction, this);
#else
	_pThread = (HANDLE)::_beginthreadex(NULL, 0, &ControllingFunction, this, 0, NULL);
#endif
	::WaitForSingleObject(_pThreadHasStarted, INFINITE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Worker::~Worker()
{
	_IsDestructing = true;

#if defined(_WIN32)

	for(;;)
	{
	 ::SetEvent(_pWork);
	// ensure the thread has terminated before we continue
	if( WAIT_OBJECT_0 == ::WaitForSingleObject(_pThreadHasTerminated, 10) )
		break;
	}
	// Wait 10 seconds until the thread is released
	::WaitForSingleObject(_pThread, 10000);

#else

	SetEvent(_pWork);

	// ensure the thread has terminated before we continue
	::WaitForSingleObject(_pThreadHasTerminated, INFINITE);

	// Wait 10 seconds until the thread is released
	void*	thread_result;
	pthread_join( _pThread, &thread_result);//TODO: NEED TO IMPLEMENT A TIMED pthread_joins

#endif

  // we must not delete the thread, the thread deletes itself automatically 
  ::CloseHandle(_pThreadHasStarted);
  ::CloseHandle(_pWork);
  ::CloseHandle(_pThreadHasTerminated);

#if defined(_WIN32)
  ::CloseHandle(_pThread);
#endif

  ::DeleteCriticalSection(&_pCriticalSection);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
void Worker::Do(
  Functor * pPieceOfWork
)
{
  ::EnterCriticalSection(&_pCriticalSection);

  _Queue.push( pPieceOfWork );
  ::SetEvent(_pWork);

  ::LeaveCriticalSection(&_pCriticalSection);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
void Worker::Work()
{
  while ( !_IsDestructing )
  {
	::SetEvent(_pThreadHasStarted);

	::WaitForSingleObject(_pWork, INFINITE);

    Functor * pPieceOfWork = GetNextPieceOfWork();

    while ( pPieceOfWork != NULL )
    {
      try
      {
        ( *pPieceOfWork )(); // execute the deferred call
      }
      catch ( ... ) // ensure that no exception is going to interrupt us and terminate the thread
      {
      }

      delete pPieceOfWork;
      pPieceOfWork = GetNextPieceOfWork();
    }
  }

  ::SetEvent(_pThreadHasTerminated);

#if defined(_WIN32)
  ::_endthreadex(0); // Quit elegantly
#else
  ::pthread_exit(0);
#endif

}


////////////////////////////////////////////////////////////////////////////////////////////////////
Functor * const Worker::GetNextPieceOfWork()
{
  ::EnterCriticalSection(&_pCriticalSection);
  Functor * pPieceOfWork = NULL;

  if ( !_Queue.empty() )
  {
    pPieceOfWork = _Queue.front();
    _Queue.pop();
  }
  ::LeaveCriticalSection(&_pCriticalSection);
  return pPieceOfWork;
}


////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __linux__//TODO: Put typedef in CsLinuxPort.h
void* WINAPI ControllingFunction( void * pParam )
#else
unsigned int WINAPI ControllingFunction( void * pParam )
#endif
{
  static_cast< Worker * >( pParam )->Work();
  // ::_endthreadex(0); // Quit elegantly
  return 0;
}