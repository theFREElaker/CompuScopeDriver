
#include "StdAfx.h"
#include "CurrentDriver.h"


#ifdef _WINDOWS_
	#include <process.h>			// for _endthreadex
#endif

#ifdef __linux__

	#include <sys/resource.h>
	#include <pthread.h>
	#include "CsLinuxSignals.h"


        /* This is the critical section object (statically allocated). */
        static pthread_mutex_t cs_mutex = PTHREAD_MUTEX_INITIALIZER;

#if __GNUC_PREREQ(2,16)
	void	HWEventHandler( int sig, siginfo_t * info, void * data );
#else
	void	HWEventHandler( int sig, struct siginfo * info, void * data );
#endif

#endif

#ifdef _WINDOWS_

unsigned int WINAPI ThreadFuncIntr( LPVOID lpParam )
{
	HANDLE			pEvents[4] = {0};
	uInt32			u32EventCount = 0;
	CSDEVICE_DRVDLL*	pDevice = static_cast<CSDEVICE_DRVDLL*>(lpParam);


	pEvents[u32EventCount++] = pDevice->GetEventSystemCleanup();
	pEvents[u32EventCount++] = pDevice->GetEventTriggered();
	pEvents[u32EventCount++] = pDevice->GetEventEndOfBusy();

	if ( 0 != pDevice->GetEventAlarm() )
		pEvents[u32EventCount++] = pDevice->GetEventAlarm();

	for(;;)
	{
		DWORD dwRet = ::WaitForMultipleObjects(u32EventCount, pEvents, FALSE, INFINITE);

		if (WAIT_OBJECT_0 == dwRet)
			break;

		// Is Hardware Transition possible 
		if (WAIT_OBJECT_0+1 == dwRet)
		{
			pDevice->SetStateTriggered();
		}

		if (WAIT_OBJECT_0+2 == dwRet)
		{
			pDevice->SetStateAcqDone();
		}
		
		if (WAIT_OBJECT_0+3 == dwRet)
		{
			pDevice->OnAlarmEvent();
			pDevice->SetStateTriggered();
			pDevice->SetStateAcqDone();
		}

		pDevice->SignalEvents();
	}
	
	::_endthreadex(1); // Quit elegantly
	return 0;
}

#else

static CSDEVICE_DRVDLL* g_pEventTrigMaster;


void	CSDEVICE_DRVDLL::InitEventHandlers()
{
	g_pEventTrigMaster = this;


	struct sigaction sAction;
	sigset_t mask ;

	memset(&sAction, 0, sizeof(sAction));

	sigemptyset( &mask ) ;

       	sAction.sa_mask = mask ;
       	sAction.sa_flags = SA_SIGINFO;

	sAction.sa_sigaction = &HWEventHandler;

	if (sigaction(CS_EVENT_SIGNAL, &sAction, NULL) < 0)
	{
		perror("sigaction");
		TRACE(DISP_TRACE_LEVEL, "Failure: sigaction", SIGRTMAX-14);
	}
	else
	{
		TRACE(DISP_TRACE_LEVEL, "sigaction created SIGRTMAX : %i", SIGRTMAX-14);
	}
}

void	*gThreadFuncIntr( LPVOID lpParam )
{
	CSDEVICE_DRVDLL* pDevice = static_cast<CSDEVICE_DRVDLL*>(lpParam);
	int iPriority = -18;

	setpriority(PRIO_PROCESS, 0, iPriority);


	SetEvent(pDevice->m_pSystem->hEventTriggered);
	cout << "Thread Intr Event enter\n";

	g_pEventTrigMaster = pDevice;

	//signal(SIGUSR1, TriggerEventHandler);
	//signal(SIGUSR2, EndOfAcqEventHandler);
	struct sigaction sAction;
	sigset_t mask ;

	memset(&sAction, 0, sizeof(sAction));

	sigemptyset( &mask ) ;

       	sAction.sa_mask = mask ;
       	sAction.sa_flags = SA_SIGINFO;

	sAction.sa_sigaction = &HWEventHandler;

	if (sigaction(SIGRTMIN+1, &sAction, NULL) < 0)
	{
		perror("sigaction");
		TRACE(DISP_TRACE_LEVEL, "Failure: sigaction", SIGRTMIN+1);
	}
	else
	{
		TRACE(DISP_TRACE_LEVEL, "sigaction created SIGRTMIN+1 : %i", SIGRTMIN+1);
	}

	cout << "Thread Intr Event exit\n";
	pthread_exit(NULL);
}

#endif


//------------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------------

#ifdef _WINDOWS_
unsigned int WINAPI ThreadFuncTxfer( LPVOID lpParam )
#else
void	*gThreadFuncTxfer( LPVOID lpParam )
#endif
{
	CSDEVICE_DRVDLL*	pCardMaster = static_cast<CSDEVICE_DRVDLL*>(lpParam);

	EVENT_HANDLE	Events[20] = {0};
	uInt32			i = 0;

	Events[i++] = pCardMaster->GetEventSystemCleanup();

	CSDEVICE_DRVDLL*	pCard = pCardMaster;
	while ( pCard )
	{
		if ( 0 !=  pCard->GetEventTxDone() )
			Events[i++] =  pCard->GetEventTxDone();
		pCard = pCard->m_Next;
	}

	for(;;)
	{
		DWORD dwRet = ::WaitForMultipleObjects( i, Events, FALSE, INFINITE);

		if (WAIT_OBJECT_0 == dwRet)		//	System cleanup event
			break;
		else
		{
			// One of events for async data transfer completed
			pCard = pCardMaster;
			while( pCard )
			{
				// Find the card which has the event signaled
				if ( pCard->GetEventTxDone() == Events[(dwRet-WAIT_OBJECT_0)] )
				{
					pCard->AsynDataTransferDone();
					pCard->SignalEvents(FALSE);
					break;
				}
				else
					pCard = pCard->m_Next;
			}
		}
	}

#ifdef _WINDOWS_
	::_endthreadex(1); // Quit elegantly
	return 0;
#else
	pthread_exit(NULL);
#endif


}


#ifdef __linux__

//------------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------------

void	CSDEVICE_DRVDLL::ThreadFuncIntr()
{
	EVENT_HANDLE	pEvents[3] = {0};

	pEvents[0] = GetEventSystemCleanup();
	pEvents[1] = GetEventTriggered();
	pEvents[2] = GetEventEndOfBusy();

	for(;;)
	{
		DWORD dwRet = ::WaitForMultipleObjects(3, pEvents, FALSE, INFINITE);

		if (WAIT_OBJECT_0 == dwRet)
		{
			cout << "CLEANUP THREAD !!!!\n";
			break;
		}

		// Is Hardware Transition possible
		if (WAIT_OBJECT_0+1 == dwRet)
		{
			cout << "TRIGGERED !!!!\n";
			SetStateTriggered();
		}

		if (WAIT_OBJECT_0+2 == dwRet)
		{
			cout << "END OF ACQ !!!!\n";
			SetStateAcqDone();
		}

		SignalEvents();
	}

}

#if __GNUC_PREREQ(2,16)
void	HWEventHandler( int sig, siginfo_t * info, void * data )
#else
void	HWEventHandler( int sig, struct siginfo * info, void * data )
#endif
{
	/* Enter the critical section -- other threads are locked out */
	pthread_mutex_lock( &cs_mutex );

	if (SIGNAL_ENDOFBUSY == info->si_int)
	{
	//	cout << "YOUPI   EOA SIGNAL\n";
		g_pEventTrigMaster->SetStateAcqDone();
		g_pEventTrigMaster->SignalEvents();
	
	}
	else if (SIGNAL_TRIGGER == info->si_int)
	{
	//	cout << "YOUPI   TRIGGERED SIGNAL\n";
		g_pEventTrigMaster->SetStateTriggered();
		g_pEventTrigMaster->SignalEvents();
	}
	else
	{
		TRACE(DISP_TRACE_LEVEL, "Unhandled Type of signal (%i)", info->si_int);
	}

	/*Leave the critical section -- other threads can now pthread_mutex_lock()  */
	pthread_mutex_unlock( &cs_mutex );

}

#endif
