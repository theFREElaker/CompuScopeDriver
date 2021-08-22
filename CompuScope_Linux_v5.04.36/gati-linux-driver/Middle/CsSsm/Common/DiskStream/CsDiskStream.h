#pragma once

#include "CsPrototypes.h"
#include <process.h>
#include <string>
#include <deque>

#include "CsExpert.h"
#include "CsPrivateStruct.h" // for CSTIMESTAT_DISKSTREAM

using namespace std;

const int g_nThreadErrorCode = -9999;

typedef struct _DiskStreamErrorStruct
{
	int32    i32ErrorCode;
	TCHAR	 szErrorMessage[MAX_PATH];
}DiskStreamErrorStruct, *PDiskStreamErrorStruct;

class CsDiskChannel;
class CsDiskStream
{
public:
	CsDiskStream(CSHANDLE hSystem, string& strBase, deque<uInt16>& deqChanNum, DWORD dwTimeOut_ms,
		         int64 i64XferStart, int64 i64XferLen, uInt32 u32RecStart, uInt32 u32RecCount,
				 uInt32 u32AcqNum,
		         bool bPostTimeStamp = false, LONG uParallelWrites = 1, LONG uParallelXfers = 1 );
	virtual ~CsDiskStream(void);
	
	int32 Start(void);
	int32 Stop(void);
	bool IsFinished(DWORD dwTimeout = 0);
	uInt32 GetFileNum(void){return m_u32FileNum;}
	uInt32 GetAcquiredCount(void){return m_u32AcqCount;}
	uInt32 GetWriteCount(void){return m_u32FileCount;} 
	void GetErrorQueue(deque<DiskStreamErrorStruct>& deqErrors);
	void TerminateAllThreads(void);
	void SetTimingFlag(bool bFlag);


	void GetPerformanceCount(PCSTIMESTAT_DISKSTREAM pTimingStats);
	
protected:

	CSHANDLE GetSystem(void) {return m_hSystem;}
	HANDLE   GetXferSem(void){return m_hXferSem;}
	HANDLE   GetWriteSem(void){return m_hWriteSem;}
	string&  GetBaseNAme(void){return m_strBase;}
	
	HANDLE   GetStopEvent(void){return m_hStop;}

	static UINT __stdcall AcquireThread(LPVOID pParms);
	
	bool    GetPostTimeStamp(void){return m_bPostTimeStamp;}
	void    UpdateTimeStamp( LARGE_INTEGER llCount );
	SYSTEMTIME GetTimeStamp(int nDequeIndex );

	uInt32  GetAddrInc(void){return m_u32ChunkSize/m_u32SampleSize;}
	int64   GetXferStart(void){return m_i64XferStart;}
	int64   GetXferEnd(void){return m_i64XferStart + m_i64XferLen;}
	int64	GetXferLength(void){return m_i64XferLen;}
	uInt32  GetRecStart(void){return m_u32RecStart;}
	int64   GetRecEnd(void){return m_u32RecStart + m_u32RecCount;}
	uInt32  GetFileSize(void){return uInt32(m_zFileSize);}
	int     GetAcqDequeSize(void){return int(m_deqTimeStamp.size());}
	uInt32  GetSampleSize(void){return m_u32SampleSize;}
	uInt32  GetGroupPadding(void){return m_u32GroupPadding;}
	DWORD   GetBufferSize(void){return (m_u32GroupPadding*m_u32SampleSize + m_u32ChunkSize);}
	uInt32  GetAcqNum(void){return m_u32AcqNum;}
	uInt32	GetSegPerFile(void){return m_u32SegPerFile;}
	
	int64	GetSampleRate(void){return m_AcqConfig.i64SampleRate;}
	int32	GetSampleRes(void){return m_AcqConfig.i32SampleRes;}
	int32	GetSampleOffset(void){return m_AcqConfig.i32SampleOffset;}
	uInt32	GetSampleBits(void){return m_AcqConfig.u32SampleBits;}
	CSACQUISITIONCONFIG& GetAcqConfig() { return m_AcqConfig;}

	void	QueueError(int32 i32ErrorCode, TCHAR* szErrorMsg = _T(""));
	void	SetAcquirePerfCount(LONGLONG llPerfCount) { m_llAcquirePerfCount = llPerfCount; }

	HANDLE  m_hXferSem;
	HANDLE  m_hWriteSem;
	string  m_strBase;
	uInt32	m_u32FileCount;
		
	static const size_t c_zMaxFileSize;
	static const size_t c_zMinFileSize;
	static const uInt32 c_u32MaxChunkSize;
	static const size_t c_zAcqDequeSize;

private:
	void ValidateFileAndXferParams(int64 i64XferStart, int64 i64XferLen, uInt32 u32RecStart, uInt32 u32RecCount);
	void EnsureThatBaseDirectoryExists(string str);

	HANDLE   m_hStop;
	CSHANDLE m_hSystem;
	HANDLE   m_hAcqThread;

	CSACQUISITIONCONFIG m_AcqConfig;
	SYSTEMTIME    m_BaseSystime;
	LARGE_INTEGER m_llBaseCount;
	LARGE_INTEGER m_llCountFreq;
	LARGE_INTEGER m_llTimeout;

	LONGLONG m_llAcquirePerfCount;

	bool          m_bPostTimeStamp;
	
	size_t        m_zFileSize;
	int64         m_i64XferStart;
	int64         m_i64XferLen;
	uInt32        m_u32RecStart;
	uInt32        m_u32RecCount;
	uInt32        m_u32ChunkSize;
	uInt32        m_u32SampleSize;
	uInt32        m_u32GroupPadding;
	uInt32        m_u32AcqNum;
	uInt32        m_u32FileNum;
	uInt32        m_u32AcqCount;
	uInt32		  m_u32SegPerFile;
		
	HANDLE* m_pHdl;
	CRITICAL_SECTION m_CrErrSec;
	
	deque<CsDiskChannel*> m_deqChan;
	deque<LARGE_INTEGER> m_deqTimeStamp;
	deque<DiskStreamErrorStruct> m_deqErrors; 

	bool m_bTimeCode;

	friend class CsDiskChannel;
};