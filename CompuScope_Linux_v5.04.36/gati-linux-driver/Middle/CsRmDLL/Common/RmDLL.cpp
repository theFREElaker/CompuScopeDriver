// RmDLL.cpp : Defines the entry point for the DLL application.
//

#include "rm.h"

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include "debug.h"

CTraceManager*	g_pTraceMngr= NULL;

BOOL APIENTRY DllMain( HANDLE , 
                       DWORD  ul_reason_for_call, 
                       LPVOID 
					 )
{
	switch(ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
#if defined (_DEBUG) || defined  (RELEASE_TRACE)
			g_pTraceMngr = new CTraceManager();
#endif
			break;
	
		case DLL_PROCESS_DETACH:
		{
			// If the DLL is exiting we can set the GageResourceManagerEvent
			// in case the application has exited without freeing the system.
			// First we check if the process that's exiting is not Rm.exe.  If
			// it is not, we set the event.  In Rm.exe the event being set will
			// cause Rm to call CsRefreshAllSystemHandles.
			HMODULE hModule = ::GetModuleHandle(_T("Rm.exe"));
			if (hModule != ::GetCurrentProcess())
			{
				TRACE(RM_TRACE_LEVEL, "RmDLL::Process is not CsRm.exe\n");

				HANDLE hEvent = ::CreateEvent(NULL, FALSE, FALSE, GAGERESOURCEMANAGERREFRESHEVENT_NAME);
				
				if (NULL != hEvent)
				{
					// The event should already exist because it was created in 
					// Rm.exe.  If not there's no point in setting it as there'll
					// be no one to act on it.
					DWORD dwError = ::GetLastError();
					if (ERROR_ALREADY_EXISTS == dwError)
					{
						::SetEvent(hEvent);
						TRACE(RM_TRACE_LEVEL, "RmDLL::GageResourceManagerRefreshEvent has been set\n");
					}
					else
					{
						TRACE(RM_TRACE_LEVEL, "RmDLL::GageResourceManagerRefreshEvent did not exist thus was not set\n");
					}
					::CloseHandle(hEvent);	
				}
				else
				{
					TRACE(RM_TRACE_LEVEL, "RmDLL::GageResourceManagerRefreshEvent was not been created\n");
				}
			}
			::WSACleanup();

			TRACE(SOCKET_TRACE_LEVEL, "RmDLL::Cleaning up WSA\n");
			delete g_pTraceMngr;
		}
		break;
		
		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;
	}
    return TRUE;
}

#else


/* TODO: Not sure how to check if an event already exists
void __attribute__ ((destructor)) cleanup_RmDll(void)
{
	// If the shared library is exiting we can set the GageResourceManagerEvent
	// in case the application has exited without freeing the system.
	// First we check if the process that's exiting is not RM.  If
	// it is not, we set the event.  In RM the event being set will
	// cause RM to call CsRefreshAllSystemHandles.

	TCHAR tstrProcessName[_MAX_PATH] = '\n';

	::GetProcessName(getpid(), tstrProcessName);
	char *strName = strrchr(tstrProcessName, '/');
	if (NULL != strName)
		strName = strName + 1; // skip over the /
	else
		strName = tstrProcessName; // there's no path, only the process name
	if (0 != _tcsicmp( strName, _T("csrmd") ) )
	{
		EVENT_HANDLE hEvent = ::OpenEvent(NULL, FALSE, FALSE, GAGERESOURCEMANAGERREFRESHEVENT_NAME);

		if (NULL != hEvent)
		{
			// The event should already exist because it was created in
			// Rm.exe.  If not there's no point in setting it as there'll
			// be no one to act on it.
			DWORD dwError = ::GetLastError();
			if (ERROR_ALREADY_EXISTS == dwError)
			{
				::SetEvent(hEvent);
				TRACE(RM_TRACE_LEVEL, "RmDLL::GageResourceManagerRefreshEvent has been set\n");
			}
			else
			{
				TRACE(RM_TRACE_LEVEL, "RmDLL::GageResourceManagerRefreshEvent did not exist thus was not set\n");
			}
			::CloseHandle(hEvent);
		}
		else
		{
			TRACE(RM_TRACE_LEVEL, "RmDLL::GageResourceManagerRefreshEvent was not been created\n");
		}
	}
}
*/
#endif
