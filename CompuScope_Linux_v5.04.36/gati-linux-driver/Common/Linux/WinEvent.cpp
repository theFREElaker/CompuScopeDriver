//*******************************************************************************
//@file          WinEvent.cpp
//@copyright     GNU Library General Public License
//@version       $Header$
//@author        R. Dengler
//*
//@language      C++ ANSI V3
//@description   Emulates odd windows events. Essential classes used for this
//               purpose are CBaseHandle, CWinEventHandle and CWinEventHandleSet
//               (defined in this file).
//******************************************************************************
#include <set>
#include <string>
#include <assert.h>
#include "CWinEventHandle.h"
#include <stdio.h>

#include "misc.h"//RG - temporary for GetTickCount

using std::wstring;

/* CLASS DECLARATION **********************************************************/
/**
  Critical section helper class, encapsulates a (recursive) pthread_mutex_t.
*******************************************************************************/
static class CCriticalSection
{
  pthread_mutex_t m_Mutex;
public:
  CCriticalSection(bool recursive = true)
  {
    if (recursive)
    {
      pthread_mutexattr_t attr;
      pthread_mutexattr_init(&attr);
      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
      pthread_mutex_init(&m_Mutex, &attr);
      pthread_mutexattr_destroy(&attr); //TODO: RG - ADDED Oct 27
    }
    else
    {
      pthread_mutex_init(&m_Mutex, 0);
    }
  }
  ~CCriticalSection()
  {
    pthread_mutex_destroy(&m_Mutex);
  }
  inline void enter() { pthread_mutex_lock(&m_Mutex); }
  inline void leave() { pthread_mutex_unlock(&m_Mutex); }
} s_CritSect;

class CEventLock
{
public:
  CEventLock() { s_CritSect.enter(); }
  ~CEventLock() { s_CritSect.leave(); }
};

/* CLASS DECLARATION **********************************************************/
/**
  Administrates a std::set of CWinEventHandle objects.
  Must be possible to find a CWinEventHandle by name.
  (std::map not very helpful: There are CWinEventHandles without a name, and
  name is stored in CWinEventHandle).
*******************************************************************************/
class CWinEventHandleSet
{
public:
  typedef std::set<CWinEventHandle*> TWinEventHandleSet;

  static CWinEventHandle* createEvent(bool manualReset, bool signaled, const wchar_t* name)
  {
    CEventLock lock;
    
    if (s_pSet == NULL)
    {
       s_pSet = new TWinEventHandleSet;
    }
    
    CWinEventHandle* handle = new CWinEventHandle(manualReset, signaled, name);
    
    s_pSet->insert(handle);
    return handle;
  }
  static void closeHandle(CWinEventHandle* eventHandle)
  {
    CEventLock lock;//
    if (eventHandle->decRefCount() <= 0)
    {
      // ToDo: Inform/wakeup waiting threads ? !

      s_pSet->erase(eventHandle);
      delete eventHandle;
      
      if (s_pSet->empty())
      {
	delete s_pSet;
	s_pSet = NULL;
      }

    }
  }
  static EVENT_HANDLE openEvent(const wchar_t* name)
  {
    CEventLock lock; //
    
    if (s_pSet == NULL)
    {
      return NULL;  
    }
    
    for (TWinEventHandleSet::iterator iter(s_pSet->begin()); iter != s_pSet->end(); iter++)
    {
      if ((*iter)->name() == name)
      {
        (*iter)->incRefCount();
        return *iter;
      }
    }
    return NULL;
  }
private:
  static TWinEventHandleSet* s_pSet; // Set of CWinEventHandle's
};

CWinEventHandleSet::TWinEventHandleSet* CWinEventHandleSet::s_pSet(NULL);

/* METHOD *********************************************************************/
/**
 Called from CloseHandle(HANDLE) when the HANDLE refers to an event.
 Decrements the reference count. If the becomes zero: Removes the handle from
 CWinEventHandleSet, deletes the event.
*
@return true
*******************************************************************************/
bool CWinEventHandle::close()
{
  CWinEventHandleSet::closeHandle(this);
  // Note: "this" may be deleted now!
  return true;
}

/* FUNCTION *******************************************************************/
/**
  See MSDN
*******************************************************************************/
EVENT_HANDLE CreateEvent(LPSECURITY_ATTRIBUTES, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName)
{
  return static_cast<CWinEventHandle*>(CWinEventHandleSet::createEvent(bManualReset, bInitialState, lpName));
}

EVENT_HANDLE WINAPI OpenEvent(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName)
{
  return static_cast<CWinEventHandle*>(CWinEventHandleSet::openEvent(lpName));
}

BOOL CloseHandle(EVENT_HANDLE handle)
{
  bool ret(false);
  if (handle != NULL)
  {
    CBaseHandle* baseHandle(static_cast<CBaseHandle*>(handle));
    if (!baseHandle->close())
    {
      printf("Closing unknown HANDLE type\n");
    }
    ret = true;
  }
  return ret;
}

BOOL WINAPI SetEvent(EVENT_HANDLE hEvent)
{
  CEventLock lock; //The lock avoids a race condition with subscribe() in WaitForMultipleObjects()//
  hEvent->signal();
  return true;
}

BOOL ResetEvent(EVENT_HANDLE hEvent)
{
  hEvent->reset();
  return true;
}

BOOL PulseEvent(EVENT_HANDLE hEvent)
{
   return hEvent->pulse();
}

/* FUNCTION *******************************************************************/
/**
  See MSDN
*******************************************************************************/
DWORD WINAPI WaitForSingleObject(EVENT_HANDLE obj, DWORD timeMs)
{
  CBaseHandle* handle(static_cast<CBaseHandle*>(obj));
  if (handle->wait(timeMs))
  {
    return WAIT_OBJECT_0;
  }
  // Might be handle of wrong type?
  return WAIT_TIMEOUT;
}

/* FUNCTION *******************************************************************/
/**
  See MSDN.
*******************************************************************************/
DWORD WINAPI WaitForMultipleObjects(DWORD numObj, const EVENT_HANDLE* objs, BOOL waitAll, DWORD timeMs)
{
  static const DWORD MAXIMUM_WAIT_OBJECTS(32);
  CWinEventHandle* eventHandle[MAXIMUM_WAIT_OBJECTS];
  assert(numObj <= MAXIMUM_WAIT_OBJECTS);
  if (waitAll)
  {
    const DWORD startMs(GetTickCount());
    for (unsigned ix(0); ix < numObj; ix++)
    {
      // Wait for all events, one after the other.
      CWinEventHandle* event(objs[ix]);
      assert(event);
      DWORD usedMs(GetTickCount() - startMs);
      if (usedMs > timeMs)
      {
        return WAIT_TIMEOUT;
      }
      if (!event->wait(timeMs - usedMs))
      {
        return WAIT_TIMEOUT;
      }
    }
    return WAIT_OBJECT_0;
  }
  s_CritSect.enter();//
  // Check whether an event is already signaled
  for (unsigned ix(0); ix < numObj; ix++)
  {
    CWinEventHandle* event(objs[ix]);
    assert(event);

    if (event->signaled())
    {
    	if (!event->isManualReset())
    	{
    	    // autoReset
    		event->reset();
    	}
      s_CritSect.leave();//
      return ix;
    }
    eventHandle[ix] = event;
  }
  if (timeMs == 0)
  {
    // Only check, do not wait. Has already failed, see above.
    s_CritSect.leave();//
    return WAIT_TIMEOUT;
  }
  /***************************************************************************
  Main use case: No event signaled yet, create a subscriber event.
  ***************************************************************************/
  CWinEventHandle subscriberEvent(false, 0);
  // Subscribe at the original objects
  for (unsigned ix(0); ix < numObj; ix++)
  {
    eventHandle[ix]->subscribe(&subscriberEvent);
  }
  s_CritSect.leave();// This re-enables SetEvent(). OK since the subscription is done $

  bool success(subscriberEvent.wait(timeMs));

  // Unsubscribe and determine return value
  DWORD ret(WAIT_TIMEOUT);
  s_CritSect.enter();//
  for (unsigned ix(0); ix < numObj; ix++)
  {
    if (success && eventHandle[ix]->signaled())
    {
      success = false;
      ret = ix;
      if (!eventHandle[ix]->isManualReset())
      {
        // autoReset
        eventHandle[ix]->reset();
      }
    }
    eventHandle[ix]->unSubscribe(&subscriberEvent);
  }
  s_CritSect.leave();//
  return ret;
}

/* FUNCTION *******************************************************************/
/**
  See MSDN. Requires realtime library, -lrt.
*******************************************************************************/
/*
DWORD GetTickCount()
{
	  struct timespec ts;
	  clock_gettime(CLOCK_REALTIME, &ts); // or CLOCK_PROCESS_CPUTIME_ID
	  DWORD ret(ts.tv_sec);
	  return ret * 1000 + ts.tv_nsec / 1000000;
}
*/
