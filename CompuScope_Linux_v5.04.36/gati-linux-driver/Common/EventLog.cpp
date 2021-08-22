#include "StdAfx.h"
#include "EventLog.h"
#include "CsMsgLog.h"

#ifdef _WIN32_
	#include <windows.h>
#endif


VOID CsEventLog::ReportEventString(DWORD dwEventID, TCHAR* szMsg1, TCHAR* szMsg2 )
{
	LPTSTR lpszStrings[4] = {0};
	WORD wEventType(EVENTLOG_SUCCESS), wMessageCount(0);
	
	if ( 0 != szMsg1)
		lpszStrings[wMessageCount++] = szMsg1;
	if ( 0 != szMsg2)
		lpszStrings[wMessageCount++] = szMsg2;


#ifdef _WINDOWS_
	switch ( dwEventID>>30 )
	{
		case STATUS_SEVERITY_ERROR:         wEventType = EVENTLOG_ERROR_TYPE; break;
		case STATUS_SEVERITY_WARNING:       wEventType = EVENTLOG_WARNING_TYPE; break;
		case STATUS_SEVERITY_INFORMATIONAL: wEventType = EVENTLOG_INFORMATION_TYPE; break;
		case STATUS_SEVERITY_SUCCESS:       wEventType = EVENTLOG_SUCCESS; break;
	}
	HANDLE hEventSource = ::RegisterEventSource(NULL, TEXT(m_szSourceName));
	if (hEventSource != NULL) 
	{
		::ReportEvent(hEventSource,				// handle of event source
					  wEventType,				// event type
					  0,						// event category
					  dwEventID,				// event ID
					  NULL,						// current user's SID
					  wMessageCount,			// strings in lpszStrings
					  0,						// no bytes of raw data
					  (LPCSTR *)lpszStrings,	// array of error strings
					  NULL);					// no raw data

		::DeregisterEventSource(hEventSource);
	}

#else		// __linux__



#endif
}
