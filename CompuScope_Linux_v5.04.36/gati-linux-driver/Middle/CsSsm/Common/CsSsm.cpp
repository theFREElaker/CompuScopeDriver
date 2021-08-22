// SSM.cpp : Defines the entry point for the DLL application.
//

#include "StdAfx.h"
#include "CsSsm.h"

bool			g_RMInitialized = false;

#if defined(_WIN32)
CTraceManager*	g_pTraceMngr= NULL;


BOOL APIENTRY DllMain( HANDLE ,
                       DWORD  ul_reason_for_call, 
                       LPVOID 
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			g_RMInitialized = false;
#if defined (_DEBUG) || defined  (RELEASE_TRACE)
			g_pTraceMngr = new CTraceManager();
#endif

			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			delete g_pTraceMngr;
			CSystemData::FreeInstance();
			break;
    }
    return TRUE;
}

#else

void ssm_cleanup(void)
{
	CSystemData::FreeInstance();
}

void __attribute__((constructor)) init_Ssm(void)
{
	g_RMInitialized = false;
	int i = atexit(ssm_cleanup);
}

/*
void __attribute__ ((destructor)) cleanup_Ssm(void)
{
	CSystemData::FreeInstance();
}
*/

#endif