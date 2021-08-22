#ifndef __WORKER_H__
#define __WORKER_H__

#if defined (_WIN32)
#pragma once
#else
#include <pthread.h>
#endif

#include "RawSocketServerWorker.h"
#include "CriticalSection.h"


class CsResourceManager;

class CWorker :	public CWizRawSocketListener
{
public:
	CWorker();
	virtual ~CWorker();
#if defined (_WIN32)
	virtual BOOL TreatData (HANDLE hShutDownEvent, HANDLE hDataTakenEvent);
	CsResourceManager* GetRm(void){ return m_pRM;}
#else
	virtual BOOL TreatData (pthread_cond_t* hShutDownEvent, pthread_cond_t& hDataTakenEvent);
#endif
	virtual BOOL ReadWrite (CWizReadWriteSocket& socket);
	int m_nCurrThreads;
	int m_nMaxThreads;
	int	m_nRequests;
	CWizCriticalSection cs;
	
protected:
	CsResourceManager* m_pRM;
};

#endif // __WORKER_H__
