
#include "StdAfx.h"
#include "CsSsm.h"

CSHANDLE SSM_API CsGetDemoSystem(void)
{
	_ASSERT(g_RMInitialized);

	if (g_RMInitialized)
		return (CsRmGetDemoSystem());
	else
		return CS_NOT_INITIALIZED;
}

CSHANDLE SSM_API CsCreateDemoSystem(void)
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	int32 nRetCode = 0;

	nRetCode = CsRmCreateDemoSystem();
	return (CS_SUCCESS);
}