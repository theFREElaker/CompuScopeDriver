#ifndef _CsTimer_h_

#define	_CsTimer_h_


#ifdef _WINDOWS_

// Maccros for conversion of time unit used by moast of DDK functions
#define KTIME_NANOSEC(t)	((t)/100)
#define KTIME_MICROSEC(t)   KTIME_NANOSEC((LONGLONG)1000*(t))
#define KTIME_MILISEC(t)	KTIME_MICROSEC((LONGLONG)1000*(t))
#define KTIME_SECOND(t)		KTIME_MILISEC((LONGLONG)1000*(t))
#define KTIME_RELATIVE(t)	(-1*(t))


#define	CS_DP_WAIT_TIMEOUT	KTIME_SECOND ( 2 )


typedef	struct
{
	LARGE_INTEGER	m_StartTickCount;		// start tick count
	LARGE_INTEGER	m_TickCount;			// current tick count;
	ULONG			m_TimeIncr;				// Time increment unit
	LONGLONG		m_DueTime;

} CSTIMER, *PCSTIMER;


void	CsSetTimer( PCSTIMER CsTimer, LONGLONG DueTime )
{
	CsTimer->m_TimeIncr = KeQueryTimeIncrement();

	if ( DueTime < 0 )
		CsTimer->m_DueTime = -1*DueTime;		// Convert relative timeout
	else
		CsTimer->m_DueTime = DueTime;

	KeQueryTickCount( &CsTimer->m_StartTickCount );
}

BOOLEAN	CsStateTimer( PCSTIMER CsTimer )
{
	KeQueryTickCount( &CsTimer->m_TickCount );
	if ( (( CsTimer->m_TickCount.QuadPart - CsTimer->m_StartTickCount.QuadPart ) * CsTimer->m_TimeIncr)  > CsTimer->m_DueTime )
		return TRUE;
	else
		return FALSE;
}

#else
//__linux__

#include <linux/jiffies.h>

// Maccros for conversion of time unit used by moast of DDK functions
#ifndef KTIME_MILISEC
#define KTIME_MILISEC(t)	(t*HZ/1000)
#endif
#ifndef KTIME_SECOND
#define KTIME_SECOND(t)		(t*HZ)
#endif


#define	CS_DP_WAIT_TIMEOUT	KTIME_SECOND ( 2 )


typedef	struct
{
	ULONG		m_DueTime;

} CSTIMER, *PCSTIMER;


void	CsSetTimer( PCSTIMER CsTimer, uInt32  DueTime )
{
	CsTimer->m_DueTime = jiffies + DueTime;
}


BOOLEAN	CsStateTimer( PCSTIMER CsTimer )
{
	if ( time_after( jiffies, (CsTimer->m_DueTime) ) )
		return TRUE;
	else
		return FALSE;
}

#endif


#endif	//_CsTimer_h_
