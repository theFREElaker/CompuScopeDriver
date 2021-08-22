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
#ifndef Worker_h
#define Worker_h 1

#include <queue>
#include "Functor.h"
#include "CsTypes.h"
#ifdef __linux__
	#include <CsLinuxPort.h>
	#include <misc.h>
#endif

//class CCriticalSection;
//class CEvent;
//class CWinThread;

#ifdef __linux__//TODO: Put typedef in CsLinuxPort.h
void* WINAPI ControllingFunction( void * pParam );
#else
unsigned int WINAPI ControllingFunction( void * pParam );
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// A simple example of a worker class for deferred calls. Each instance owns a thread, which is
// waked up as soon as there is at least one piece of work in the queue.
// The thread then continuously pops a piece of work out of the queue, works on it and deletes it
// until the queue is empty. The thread then sleeps again until there is more work to do.
////////////////////////////////////////////////////////////////////////////////////////////////////
class Worker
{
  public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
  	Worker();

    // non-virtual destructor!! DO NOT DERIVE FROM THIS CLASS
    // The destructor waits until the queue is empty before cleaning up and returning.
    ~Worker();

    // Puts the functor into a fifo queue and returns immediately.
    void Do( Functor * pPieceOfWork );

  public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void Work();

    Functor * const GetNextPieceOfWork();

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //static unsigned int ControllingFunction( void * pParam );

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // CWinThread * _pThread;
    // CCriticalSection * _pCriticalSection;
    // CEvent * _pThreadHasStarted; | RTLD_NODELETE
    // CEvent * _pWork;
    // CEvent * _pThreadHasTerminated;
private:
#ifdef __linux__ //TODO: Put into CsLinuxPort.h
	pthread_t _pThread;
#else
	HANDLE _pThread;
#endif
	CRITICAL_SECTION _pCriticalSection;
    EVENT_HANDLE _pThreadHasStarted;
    EVENT_HANDLE _pWork;
    EVENT_HANDLE _pThreadHasTerminated;

    std::queue< Functor * > _Queue;
    bool _IsDestructing;
};



#endif
