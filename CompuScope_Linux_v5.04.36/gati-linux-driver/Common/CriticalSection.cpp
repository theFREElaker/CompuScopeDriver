#if defined (_WIN32)

#include <crtdbg.h>

#else

#include <assert.h>

#endif
#include "misc.h"
#include "CriticalSection.h"



///////////////////////////////////////////////////////////////////
// class CWizCriticalSection

//*****************************************************************
void	CWizCriticalSection::Init()
{
	_ASSERT(m_bInited == FALSE);
	InitializeCriticalSection(&m_cCriticalSection);
	m_bInited = TRUE;
}
//*****************************************************************
// Default Constructor

CWizCriticalSection::CWizCriticalSection(BOOL bInit)
	: m_bInited (FALSE)
{
	if (bInit)
		Init();
}

//*****************************************************************
// Destructor
CWizCriticalSection::~CWizCriticalSection()
{
	DeleteCriticalSection(&m_cCriticalSection);
}
//*****************************************************************
