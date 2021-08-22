#include "StdAfx.h"
#include "CsSsm.h"
#include "CsDiskStream.h"
#include "CsDiskChannel.h"


const size_t CsDiskStream::c_zMaxFileSize   = 256*1024*1024;  //256 MB 
const size_t CsDiskStream::c_zMinFileSize   = 1024*1024;      //1 MB
const uInt32 CsDiskStream::c_u32MaxChunkSize = 128*1024*1024; //128 MB
const size_t CsDiskStream::c_zAcqDequeSize = 3;


CsDiskStream::CsDiskStream(CSHANDLE hSystem, string& strBase, deque<uInt16>& deqChanNum, DWORD dwTimeOut_ms,
						   int64 i64XferStart, int64 i64XferLen, uInt32 u32RecStart, uInt32 u32RecCount,
						   uInt32 u32AcqNum, bool bPostTimeStamp,
						   LONG lParallelWrites, LONG lParallelXfers ):
	m_hSystem(hSystem), m_bPostTimeStamp(bPostTimeStamp), m_u32AcqNum(u32AcqNum), m_u32AcqCount(0), 
	m_u32FileCount(0), m_llAcquirePerfCount(0), m_bTimeCode(false)
{
	TCHAR strName[MAX_PATH];
	_stprintf_s( strName, MAX_PATH, _T("CsDskPrfXferSem_%d"), hSystem );
	m_hXferSem  = ::CreateSemaphore( NULL, lParallelXfers, lParallelXfers, strName);
	_stprintf_s( strName, MAX_PATH, _T("CsDskPrfWriteSem_%d"), hSystem );
	m_hWriteSem = ::CreateSemaphore( NULL, lParallelWrites, lParallelWrites, strName);
	_stprintf_s( strName, MAX_PATH, _T("CsDskPrfStopEvt_%d"), hSystem );
	m_hStop     = ::CreateEvent( NULL, TRUE, FALSE, strName );

	::ZeroMemory(&m_AcqConfig, sizeof(CSACQUISITIONCONFIG));
	m_AcqConfig.u32Size = sizeof(CSACQUISITIONCONFIG);
	int32 i32 = ::CsGet(m_hSystem, CS_ACQUISITION, CS_ACQUISITION_CONFIGURATION, &m_AcqConfig);

	if ( CS_FAILED(i32) )
	{
		DiskStreamErrorStruct es;
		es.i32ErrorCode = i32;
		::_tcscpy_s(es.szErrorMessage, _countof(es.szErrorMessage), _T(""));
		throw (&es);		
	}	

	m_u32GroupPadding = 0;
	uInt32 u32RequiredSize = sizeof(m_u32GroupPadding);
	i32 = ::CsGetSystemCaps(m_hSystem, CAPS_MAX_SEGMENT_PADDING, &m_u32GroupPadding, &u32RequiredSize);
	if (CS_FAILED(i32))
	{
		m_u32GroupPadding = 256;
	}

	ValidateFileAndXferParams(i64XferStart, i64XferLen, u32RecStart, u32RecCount);

	m_deqChan.resize(deqChanNum.size());

	::GetLocalTime( &m_BaseSystime );
	::QueryPerformanceCounter( &m_llBaseCount );
	::QueryPerformanceFrequency( &m_llCountFreq );

	if( 0xffffffff == dwTimeOut_ms )
	{
		m_llTimeout.QuadPart = -1;
	}
	else
	{
		m_llTimeout.QuadPart = ( dwTimeOut_ms * m_llCountFreq.QuadPart + 999 ) / 1000;
		if ( m_llTimeout.QuadPart < 0 )
		{
			m_llTimeout.QuadPart = _I64_MAX;
		}
	}
	m_deqTimeStamp.resize(c_zAcqDequeSize);

	for(size_t s = 0; s < deqChanNum.size(); s++ )
	{
		try
		{
			m_deqChan[s] = new CsDiskChannel(this, strBase, deqChanNum[s], m_bTimeCode );
			if ( NULL == m_deqChan[s] )
			{
				DiskStreamErrorStruct es;
				es.i32ErrorCode = CS_MISC_ERROR;
				::_tcscpy_s(es.szErrorMessage, _countof(es.szErrorMessage), _T("Could not create new Disk Channel"));
				throw (&es);		
			}
		}
		catch(DiskStreamErrorStruct* es)
		{
			throw(es);
		}
	}
	
	CSystemData::instance().LockSM(m_hSystem, true);
	
	m_hAcqThread = (HANDLE)::_beginthreadex( NULL, 0, AcquireThread, this, 0, NULL );
	::SetThreadPriority(m_hAcqThread, THREAD_PRIORITY_ABOVE_NORMAL);
	int nHandlesCount = int(m_deqChan.size()) * 4 + 1; //four threads per channel plus an acq thread

	m_pHdl = new HANDLE[nHandlesCount];
	if ( NULL == m_pHdl )
	{
		DiskStreamErrorStruct es;
		es.i32ErrorCode = CS_MEMORY_ERROR;
		::_tcscpy_s(es.szErrorMessage, _countof(es.szErrorMessage), _T(""));
		CSystemData::instance().LockSM(m_hSystem, false);
		throw (&es);		
	}
	::InitializeCriticalSection(&m_CrErrSec);
	int i = 0;
	m_pHdl[i++] = m_hAcqThread;
	for(size_t s = 0; s < m_deqChan.size(); s++ )
	{
		m_pHdl[i++] = m_deqChan[s]->m_hPreThread;
		m_pHdl[i++] = m_deqChan[s]->m_hXferThread;
		m_pHdl[i++] = m_deqChan[s]->m_hWriteThread;
		m_pHdl[i++] = m_deqChan[s]->m_hCloseThread;
	}
}

CsDiskStream::~CsDiskStream(void)
{
	::SetEvent( m_hStop );
	::WaitForSingleObject( m_hAcqThread, 10000 );
	
	for (size_t i = 0; i < m_deqChan.size(); i++)
	{
		delete m_deqChan[i];
		m_deqChan[i] = NULL;
	}
	m_deqChan.clear();
	::CloseHandle( m_hAcqThread );

	::CloseHandle(m_hXferSem);
	::CloseHandle(m_hWriteSem);
	::CloseHandle(m_hStop);
	delete[] m_pHdl;
	::DeleteCriticalSection(&m_CrErrSec);

	CSystemData::instance().LockSM(m_hSystem, false);
}

UINT __stdcall CsDiskStream::AcquireThread(LPVOID pParms)
{
	CsDiskStream* pThis = static_cast<CsDiskStream*> (pParms);
	DWORD  dwCount = DWORD(pThis->m_deqChan.size());

	HANDLE* pHndls = new HANDLE[dwCount];
	HANDLE  hStop = pThis->m_hStop;
	
	for( size_t s = 0; s < pThis->m_deqChan.size(); s++ )
		pHndls[s] = pThis->m_deqChan[s]->GetAcqEvt();

	
	int32 i32;
	LARGE_INTEGER liStart;
	LARGE_INTEGER liCount = {0};

	LARGE_INTEGER liEnd;
	LONGLONG llAcquirePerfCount = 0;

	for(;;)
	{
		DWORD dw(WAIT_TIMEOUT);
		while(WAIT_TIMEOUT == dw)
		{
			if( WAIT_OBJECT_0 == ::WaitForSingleObject(hStop, 0 ) )
			{
				if ( pThis->m_bTimeCode )
				{
					pThis->SetAcquirePerfCount( llAcquirePerfCount );
				}
				delete[] pHndls;
				_endthreadex(0);
			}
			dw = ::WaitForMultipleObjects(dwCount, pHndls, TRUE, 100 ); 
		}

		::QueryPerformanceCounter( &liStart );

		i32 = ::CsDo(pThis->m_hSystem, ACTION_START);
		if(CS_FAILED(i32))
		{
			::SetEvent(hStop);
			if ( pThis->m_bTimeCode )
			{
				pThis->SetAcquirePerfCount( llAcquirePerfCount );
			}
			delete[] pHndls;
			// put error in the deque
			pThis->QueueError(i32);
			_endthreadex(0);
		}
		::QueryPerformanceCounter(&liCount);
		while ( (ACQ_STATUS_READY != ::CsGetStatus(pThis->m_hSystem) ) )
		{
			if( WAIT_OBJECT_0 == ::WaitForSingleObject(hStop, 0 ) )
			{
				if ( pThis->m_bTimeCode )
				{
					pThis->SetAcquirePerfCount( llAcquirePerfCount );
				}
				delete[] pHndls;
				_endthreadex(0);
			}
			else 
			{
				::QueryPerformanceCounter(&liCount);
				if ( pThis->m_llTimeout.QuadPart > 0 && liCount.QuadPart - liStart.QuadPart > pThis->m_llTimeout.QuadPart )
				{
					i32 = ::CsDo(pThis->m_hSystem, ACTION_FORCE);
					if(CS_FAILED(i32))
					{
						::SetEvent(hStop);
						if ( pThis->m_bTimeCode )
						{
							pThis->SetAcquirePerfCount( llAcquirePerfCount );
						}
						delete[] pHndls;
						// put error in the deque
						pThis->QueueError(i32);
						_endthreadex(0);
					}
				}
			}
		}
		if ( pThis->m_bTimeCode )
		{
			::QueryPerformanceCounter( &liEnd );	
			llAcquirePerfCount += liEnd.QuadPart - liStart.QuadPart;
		}

		pThis->UpdateTimeStamp( liCount );
		pThis->m_u32AcqCount++;

		for( size_t s = 0; s < pThis->m_deqChan.size(); s++ )
			::SetEvent( pThis->m_deqChan[s]->GetDataReadyEvt() );

		if ( pThis->m_u32AcqCount >= pThis->m_u32AcqNum )
		{
			if ( pThis->m_bTimeCode )
			{
				pThis->SetAcquirePerfCount( llAcquirePerfCount );
			}
			delete[] pHndls;
			_endthreadex(0);
		}
	}
}

void CsDiskStream::UpdateTimeStamp(  LARGE_INTEGER liCount )
{
	if( m_bPostTimeStamp )
	{
		m_deqTimeStamp[m_u32AcqCount % m_deqTimeStamp.size()].QuadPart = liCount.QuadPart - m_llBaseCount.QuadPart;
	}
}

SYSTEMTIME CsDiskStream::GetTimeStamp(int nDequeIndex )
{
	SYSTEMTIME time( m_BaseSystime );
	if( m_bPostTimeStamp )
	{
		LONGLONG llDelta_ms = (1000I64 * m_deqTimeStamp[nDequeIndex].QuadPart) / m_llCountFreq.QuadPart;
		//Subtract the busy time
		LONGLONG llBusyTime_ms = LONGLONG(double(m_AcqConfig.i64Depth) / double(m_AcqConfig.i64SampleRate) * 1000.);
		llDelta_ms -= llBusyTime_ms;
		llDelta_ms += time.wMilliseconds;
		
		LONGLONG llDelta_s(time.wSecond), llDelta_m(time.wMinute);
		if( llDelta_ms < 0 )
		{
			llDelta_s -= (-llDelta_ms + 999)/1000;
			llDelta_ms = 1000 - ((-llDelta_ms)%1000);
		}

		if( llDelta_s < 0 )
		{
			llDelta_m -= (-llDelta_s + 59 )/60;
			llDelta_s = 60 - ((-llDelta_s)%60 );
		}

		if( llDelta_m < 0 )
		{
			time.wHour -= WORD(-llDelta_s + 59 )/60;
			llDelta_m = 60 - ((-llDelta_m)%60 );
		}


		if( llDelta_ms > 1000I64 )
		{
			llDelta_s += llDelta_ms / 1000I64;
			time.wMilliseconds = WORD( llDelta_ms % 1000I64 );
		}
		else
		{
			time.wMilliseconds = WORD( llDelta_ms );
		}
		if( llDelta_s > 60I64 )
		{
			llDelta_m += llDelta_s / 60I64;
			time.wSecond = WORD( llDelta_s % 60I64 );
		}
		else
		{
			time.wSecond = WORD( llDelta_s );
		}
		if( time.wMinute > 60I64 )
		{
			time.wHour +=  WORD( llDelta_m / 60I64);
			time.wMinute = WORD( llDelta_m % 60I64);
		}
		else
		{
			time.wMinute = WORD( llDelta_m );
		}
		//stop at the hour since we are only interested in time not date
	}
	return time;
}

void CsDiskStream::ValidateFileAndXferParams(int64 i64XferStart, int64 i64XferLen, uInt32 u32RecStart, uInt32 u32RecCount)
{
	m_u32SampleSize = m_AcqConfig.u32SampleSize;
	
	int64 i64AcqStart = max(-m_AcqConfig.i64TriggerHoldoff + m_AcqConfig.i64TriggerDelay, m_AcqConfig.i64Depth - m_AcqConfig.i64SegmentSize + m_AcqConfig.i64TriggerDelay);
	int64 i64AcqEnd   = m_AcqConfig.i64Depth + m_AcqConfig.i64TriggerDelay;

	m_i64XferStart = min( max(i64XferStart, i64AcqStart), i64AcqEnd - 4 );

	m_i64XferLen = max( 4, min(i64XferLen, i64AcqEnd - m_i64XferStart));

	m_u32RecStart = min( m_AcqConfig.u32SegmentCount, u32RecStart );
	
	m_u32RecCount = min(u32RecCount, m_AcqConfig.u32SegmentCount - m_u32RecStart + 1 );

	int64 i64FileSize = m_u32RecCount * m_i64XferLen * m_u32SampleSize;
	m_zFileSize = size_t(min(c_zMaxFileSize, max(c_zMinFileSize, i64FileSize)));

	m_u32SegPerFile = uInt32(int64(m_zFileSize) / (m_i64XferLen*m_u32SampleSize));
	m_u32ChunkSize = min( uInt32(m_i64XferLen*m_u32SampleSize), c_u32MaxChunkSize - m_u32GroupPadding*m_u32SampleSize );

	if( 0 == m_u32SegPerFile )
	{
		m_u32SegPerFile = 1;
		m_u32FileNum = uInt32(m_u32AcqNum*m_u32RecCount*((m_i64XferLen*m_u32SampleSize + m_zFileSize - 1)/m_zFileSize));
	}
	else if( m_u32SegPerFile >= m_u32RecCount )
	{
		if( 1 < m_u32RecCount )
		{
			m_u32SegPerFile = m_u32RecCount;
			m_zFileSize = size_t(m_u32RecCount*m_i64XferLen*m_u32SampleSize);
			m_u32FileNum = m_u32AcqNum;
		}
		else
		{
			m_u32SegPerFile = min( m_u32AcqNum, m_u32SegPerFile);
			m_zFileSize = uInt32( m_u32SegPerFile*(m_i64XferLen*m_u32SampleSize));
			m_u32FileNum = (m_u32AcqNum+m_u32SegPerFile-1)/m_u32SegPerFile;
		}
	}	
	else
	{
		m_zFileSize = size_t(m_u32SegPerFile*m_i64XferLen*m_u32SampleSize);
		m_u32FileNum = m_u32AcqNum * ((m_u32RecCount +  m_u32SegPerFile - 1) / m_u32SegPerFile);
	}
}


int32 CsDiskStream::Start(void)
{
	m_u32AcqCount = 0;
	if (!CSystemData::instance().PerformTransition(m_hSystem, DISKSTREAM_TRANSITION))
		return (CS_INVALID_STATE);

	for( size_t s = 0; s < m_deqChan.size(); s++ )
		m_deqChan[s]->PreCreateOnStart();

	return CS_SUCCESS;
}

int32  CsDiskStream::Stop(void)
{
	::SetEvent(m_hStop);
	if (!CSystemData::instance().PerformTransition(m_hSystem, DONE_TRANSITION))
		return (CS_INVALID_STATE);

	return CS_SUCCESS;

}

bool CsDiskStream::IsFinished(DWORD dwTimeout /* = 0 */)
{
	int nHandlesCount = int(m_deqChan.size()) * 4 + 1; //four threads per channel plus an acq thread
	bool bRunning = WAIT_TIMEOUT == ::WaitForMultipleObjects(nHandlesCount, m_pHdl, TRUE, dwTimeout );
	return !bRunning;
}

void CsDiskStream::GetErrorQueue(deque<DiskStreamErrorStruct>& deqErrors)
{
	DiskStreamErrorStruct es;
	for (size_t i = 0; i < m_deqErrors.size(); i++)
	{
		es.i32ErrorCode = m_deqErrors[i].i32ErrorCode;
		_tcsncpy_s(es.szErrorMessage, _countof(deqErrors[i].szErrorMessage), m_deqErrors[i].szErrorMessage, _TRUNCATE);
		deqErrors.push_back(es);
	}
}

void CsDiskStream::QueueError(int32 i32ErrorCode, TCHAR* szErrorMessage /* = _T("")*/)
{
	DiskStreamErrorStruct es;

	es.i32ErrorCode = i32ErrorCode;
	_tcsncpy_s(es.szErrorMessage, _countof(es.szErrorMessage), szErrorMessage, _TRUNCATE);

	::EnterCriticalSection(&m_CrErrSec);		
	m_deqErrors.push_front(es);
	::LeaveCriticalSection(&m_CrErrSec);
}

void CsDiskStream::TerminateAllThreads(void)
{
	DWORD dwExitCode;
	HANDLE hThread;

	::GetExitCodeThread(m_hAcqThread, &dwExitCode);
	::TerminateThread(m_hAcqThread, dwExitCode);

	for (size_t i = 0; i < m_deqChan.size(); i++)
	{
		hThread = m_deqChan[i]->GetPreCreateThread();
		::GetExitCodeThread(hThread, &dwExitCode);
		::TerminateThread(hThread, dwExitCode);

		hThread = m_deqChan[i]->GetCloseThread();
		::GetExitCodeThread(hThread, &dwExitCode);
		::TerminateThread(hThread, dwExitCode);

		hThread = m_deqChan[i]->GetXferThread();
		::GetExitCodeThread(hThread, &dwExitCode);
		::TerminateThread(hThread, dwExitCode);

		hThread = m_deqChan[i]->GetWriteThread();
		::GetExitCodeThread(hThread, &dwExitCode);
		::TerminateThread(hThread, dwExitCode);
	}
}

void CsDiskStream::GetPerformanceCount(PCSTIMESTAT_DISKSTREAM pTimingStats)
{
	LONGLONG llPreCreateCount = 0;
	LONGLONG llUpdateCloseCount = 0;
	LONGLONG llXferCount = 0;
	LONGLONG llWriteCount = 0;

	pTimingStats->out.llAcquirePerfCount = m_llAcquirePerfCount;

	for (size_t i = 0; i < m_deqChan.size(); i++)
	{
		llPreCreateCount += m_deqChan[i]->GetPreCreatePerfCount();
		llUpdateCloseCount += m_deqChan[i]->GetUpdateClosePerfCount();
		llXferCount += m_deqChan[i]->GetXferPerfCount();
		llWriteCount += m_deqChan[i]->GetWritePerfCount();
	}
	pTimingStats->out.llPreCreatePerfCount = llPreCreateCount;
	pTimingStats->out.llUpdateClosePerfCount = llUpdateCloseCount;
	pTimingStats->out.llXferPerfCount = llXferCount;
	pTimingStats->out.llWritePerfCount = llWriteCount;
}

void CsDiskStream::SetTimingFlag(bool bFlag)
{
	m_bTimeCode = bFlag;
	for (size_t i = 0; i < m_deqChan.size(); i++)
	{
		m_deqChan[i]->SetTimingFlag(bFlag);
	}
}