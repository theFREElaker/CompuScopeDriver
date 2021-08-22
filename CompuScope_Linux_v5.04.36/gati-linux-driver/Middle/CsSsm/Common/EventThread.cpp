
#include "StdAfx.h"
#include "CsSsm.h"

#if defined (_WIN32)
	#include <process.h> // for _endthreadex
#endif


#ifdef __linux__	//TODO: FIX
void* ThreadFunc( void * lpParam )
#else
unsigned int WINAPI ThreadFunc( LPVOID lpParam )
#endif
{
	EVENT_HANDLE	pEvents[3] = {0};

	CStateMachine* pStateMachine = static_cast<CStateMachine*>(lpParam);
	CSHANDLE hSystem = pStateMachine->GetSystemHandle();

	pEvents[0] = pStateMachine->GetSystemReleaseEvent();
	pEvents[1] = pStateMachine->GetTriggerEvent();
	pEvents[2] = pStateMachine->GetAcquisitionEndEvent();

	for(;;)
	{
		DWORD dwRet = ::WaitForMultipleObjects(3, pEvents, FALSE, INFINITE);
		if (WAIT_OBJECT_0 == dwRet)
		{
			// Set the event so CSystemData::ClearSystem knows the thread has ended
			::SetEvent(pStateMachine->GetThreadEndEvent());
#if defined (_WIN32)
			::_endthreadex(1); // Quit elegantly
#else
			void *value;
			::pthread_exit(value); // RG for now
#endif
		}

		// Is Hardware Transition possible
		if (WAIT_OBJECT_0+1 == dwRet)
		{
			CSystemData::instance().Lock();
			if (!pStateMachine->IsSMLock())
			{
				if (pStateMachine->PerformTransition(HWEVENT_TRANSITION))
					pStateMachine->SignalEvent(ACQ_EVENT_TRIGGERED);
				else
					TRACE(SSM_TRACE_LEVEL, "SSM::Error Processing Triggered Event\n");
			}
			else
				pStateMachine->SignalEvent(ACQ_EVENT_TRIGGERED);

			CSystemData::instance().UnLock();
		}

		if (WAIT_OBJECT_0+2 == dwRet)
		{
			CSystemData::instance().Lock();
			if (!pStateMachine->IsSMLock())
			{
				if (pStateMachine->PerformTransition(HWEVENT_TRANSITION))
				{
					pStateMachine->SignalEvent(ACQ_EVENT_END_BUSY);
					CSystemData::instance().UnLock();
					CSystemData::instance().EndAcqCallback(hSystem);
				}
				else
				{
					TRACE(SSM_TRACE_LEVEL, "SSM::Error Processing EndBusy Event\n");
					CSystemData::instance().UnLock();
				}
			}
			else
			{
				pStateMachine->SignalEvent(ACQ_EVENT_END_BUSY);
				CSystemData::instance().UnLock();
			}
		}
	}
}
#ifdef __linux__//TODO: FIX
void* ThreadFuncTxfer( void * lpParam )
#else
unsigned int WINAPI ThreadFuncTxfer( LPVOID lpParam )
#endif
{
	EVENT_HANDLE	pEvents[2] = {0};

	CStateMachine* pStateMachine = static_cast<CStateMachine*>(lpParam);
	CSHANDLE hSystem = pStateMachine->GetSystemHandle();

	pEvents[0] = pStateMachine->GetSystemReleaseEvent();
	pEvents[1] = *(pStateMachine->GetTransferEndEvent());

	for(;;)
	{
		DWORD dwRet = ::WaitForMultipleObjects(2, pEvents, FALSE, INFINITE);

		if (WAIT_OBJECT_0 == dwRet)
		{
			// Set the event so CSystemData::ClearSystem knows the thread has ended
			::SetEvent(pStateMachine->GetThreadEndEventTxfer());
#ifndef __linux__ // for now
			::_endthreadex(1); // Quit elegantly
#else
			void *value;
			::pthread_exit(value);
#endif
		}

		if (WAIT_OBJECT_0+1 == dwRet)
		{
			CSystemData::instance().Lock();

			// Code fix. Since event has been set, it implies the transfer was completed.
			// So we need to complete the transition for the state machine and close current acquisition
			// token. Validate token will store the status of the transfer.
			CSystemData::instance().ValidateToken(hSystem, -1, NULL, COMPLETE_TOKEN);
			CSystemData::instance().PerformTransition(hSystem, DONE_TRANSITION);

			pStateMachine->SignalEvent(ACQ_EVENT_END_TXFER);

			CSystemData::instance().UnLock();
			CSystemData::instance().EndTxCallback(hSystem);

		}
	}
}
