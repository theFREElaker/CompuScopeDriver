#include "StdAfx.h"
#include "CsEventLog.h"
#ifdef _WINDOWS_
	#include "CsMsgLog.h"
	#include "debug.h"
#else
	#include <syslog.h>
#endif

CsEventLog::CsEventLog() : m_hEventLog(NULL), m_dwError(0)
{
#ifdef _WINDOWS_
	m_hEventLog = ::RegisterEventSource(NULL, EVENT_LOG_NAME);

	if ( NULL == m_hEventLog )
	{
		m_dwError = ::GetLastError();
#ifndef CS_WIN64
		TRACE(DRV_TRACE_LEVEL, _T("Could not open event source Error %d"), m_dwError);
#endif
	}
#endif
}

CsEventLog::~CsEventLog(void)
{
#ifdef _WINDOWS_
	if ( NULL != m_hEventLog )
	{
		::DeregisterEventSource(m_hEventLog);
		m_hEventLog = NULL;
	}
#endif
}
#ifdef _WINDOWS_
BOOL CsEventLog::Report(DWORD dwErrorId, LPTSTR lpMsgIn)
{
	BOOL bRet = TRUE;

	LPTSTR lpszStrings[2] = { NULL, NULL };

	WORD wEventType(EVENTLOG_SUCCESS), wMessageCount(1);
	switch ( dwErrorId >> 30 )
	{
			case STATUS_SEVERITY_ERROR:         wEventType = EVENTLOG_ERROR_TYPE; break;
			case STATUS_SEVERITY_WARNING:       wEventType = EVENTLOG_WARNING_TYPE; break;
			case STATUS_SEVERITY_INFORMATIONAL: wEventType = EVENTLOG_INFORMATION_TYPE; break;
			case STATUS_SEVERITY_SUCCESS:       wEventType = EVENTLOG_SUCCESS; wMessageCount = 0; break;
	}


	lpszStrings[0] = lpMsgIn;

	bRet = ::ReportEvent(	m_hEventLog,			// handle of event source
							wEventType,				// event type
							0,						// event category
							dwErrorId,				// event ID
							NULL,					// current user's SID
							wMessageCount,			// strings in lpszStrings
							0,						// no bytes of raw data
							(LPCSTR *)lpszStrings,	// array of error strings
							NULL);					// no raw data
	
	if ( ! bRet )
	{
		// GET THE ERROR, or set m_nError, etc
		m_dwError = ::GetLastError();
#ifndef CS_WIN64
		TRACE(DRV_TRACE_LEVEL, _T("Error %d in ReportEvent"), m_dwError);
#endif
	}
	return bRet;
}
#endif

#ifdef __linux__
BOOL CsEventLog::ReportLinux(const char *dwErrorId, const char* lpMsgIn)
{
	syslog(LOG_INFO, "%s: %s", dwErrorId, lpMsgIn);
}
#endif
