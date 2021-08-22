//*******************************************************************************
//@file          CWinEventHandle.cpp
//@copyright     GNU Library General Public License
//@version       $Header$
//@author        R. Dengler
//*
//@language      C++ ANSI V3
//@description   A Wrapper around a pthread_cond_t. Used to emulate win32-events.
//               Note that signal(), subscribe(), unsubscribe() require an
//               (external) critical section.
//******************************************************************************
#include <sys/time.h>
#include <errno.h>
#include "CWinEventHandle.h"

namespace
{
  timespec* getTimeout(struct timespec* spec, unsigned numMs)
  {
    struct timeval current;
    gettimeofday(&current, NULL);
    spec->tv_sec = current.tv_sec + ((numMs + current.tv_usec / 1000) / 1000);
    spec->tv_nsec = ((current.tv_usec / 1000 + numMs) % 1000) * 1000000;
    return spec;
  }
}

/* METHOD *********************************************************************/
/**
  Ctor
@param manualReset: Reset mode
@param        name: Event name, may be NULL
*******************************************************************************/
CWinEventHandle::CWinEventHandle(bool manualReset, bool signaled, const wchar_t* name)
  : CBaseHandle()
  , m_ManualReset(manualReset)
  , m_Signaled(signaled)
  , m_Count(0)
  , m_RefCount(1)
  , m_Name(name == NULL ? L"" : name)
{
  pthread_mutexattr_t attr;

  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
  pthread_mutex_init(&m_Mutex, &attr);
  pthread_mutexattr_destroy(&attr);
  pthread_cond_init(&m_Cond, NULL);
}

CWinEventHandle::~CWinEventHandle()
{
  pthread_cond_destroy(&m_Cond);
  pthread_mutex_destroy(&m_Mutex);
}

void CWinEventHandle::signal()
{
  pthread_mutex_lock(&m_Mutex);

  m_Signaled = true;
  ++m_Count;
  pthread_cond_broadcast(&m_Cond);
  pthread_mutex_unlock(&m_Mutex);//

  // signal all subscribers (used in WaitForMultipleObjects())
  for (std::set<CWinEventHandle*>::iterator iter(m_Subscriber.begin()); iter != m_Subscriber.end(); iter++)
  {
	  (*iter)->signal();
  }
}

bool CWinEventHandle::pulse()
{
  // Only used for shutdown. ToDo !
  signal();
  return true;
}

void CWinEventHandle::reset()
{
  m_Signaled = false;
}

bool CWinEventHandle::wait()
{
# define TIMEOUT_INF ~((unsigned)0)
  return wait(TIMEOUT_INF);
}

bool CWinEventHandle::wait(unsigned numMs)
{
  int rc(0);
  struct timespec spec;

  pthread_mutex_lock(&m_Mutex);
  const long count(m_Count);
  while (!m_Signaled && m_Count == count)
  {
    if (numMs != TIMEOUT_INF)
    {
      rc = pthread_cond_timedwait(&m_Cond, &m_Mutex, getTimeout(&spec, numMs));
    }
    else
    {
      pthread_cond_wait(&m_Cond, &m_Mutex);
    }
    if (rc == ETIMEDOUT)
    {
      break;
    }
  }
  if (!m_ManualReset)
  {
    // autoReset
    m_Signaled = false;
  }
  pthread_mutex_unlock(&m_Mutex);//
  return rc != ETIMEDOUT;
}

/* METHOD *********************************************************************/
/**
  Stores subscriber event to signal instead of signalling "this".
*******************************************************************************/
void CWinEventHandle::subscribe(CWinEventHandle* subscriber)
{
  m_Subscriber.insert(subscriber);
}

/* METHOD *********************************************************************/
/**
  Removes a subscriber event from the subscriber set (external critical section).
*******************************************************************************/
void CWinEventHandle::unSubscribe(CWinEventHandle* subscriber)
{
  m_Subscriber.erase(subscriber);
}

