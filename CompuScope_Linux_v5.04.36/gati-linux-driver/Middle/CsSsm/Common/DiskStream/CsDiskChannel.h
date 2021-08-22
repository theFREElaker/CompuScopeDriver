#pragma once

#include "CsStruct.h"
#include "diskhead.h"
#include <string>
#include <deque>

using namespace std;


class CsDiskStream;

typedef struct _DiskBuffer
{
	void*    pBuffer;
	int32    i32Off;
	uInt32   u32Len;
	bool     bFinal;
	SYSTEMTIME Ts;
}DiskBuffer;

typedef struct _DiskFile
{
	HANDLE   hFile;
	int32    i32StartAddr;
	DWORD	 dwTotalWritten;
	SYSTEMTIME Ts;
}DiskFile;

class CsDiskChannel
{
public:
	CsDiskChannel(CsDiskStream* pDisk, string& strBase, uInt16 u16Chan, bool bTimeCode);
	virtual ~CsDiskChannel(void);

	LONGLONG GetXferPerfCount() { return m_llXferPerfCount; }	
	LONGLONG GetWritePerfCount() { return m_llWritePerfCount; }	
	LONGLONG GetUpdateClosePerfCount() { return m_llUpdateClosePefCount; }	
	LONGLONG GetPreCreatePerfCount() { return m_llPreCreatePerfCount; }	

protected:
	static UINT __stdcall PreCreateFilesThread(LPVOID pParms);
	static UINT __stdcall XferDataThread(LPVOID pParms);
	static UINT __stdcall WriteDataThread(LPVOID pParms);
	static UINT __stdcall UpdateHeaderAndCloseThread(LPVOID pParms);

	CsDiskStream* GetDiskStream(void){return m_pDisk;}
	
	HANDLE GetAcqEvt(void){return m_hAcqEvt;}
	HANDLE GetDataReadyEvt(void){return m_hNewDataEvt;}
	void PreCreateOnStart(void){::SetEvent(m_hAcqEvt);}  

	HANDLE GetPreCreateThread() { return m_hPreThread; }
	HANDLE GetCloseThread() { return m_hCloseThread; }
	HANDLE GetXferThread() { return m_hXferThread; }
	HANDLE GetWriteThread() { return m_hWriteThread; }


	void SetXferPerfCount(LONGLONG llPerfCount) { m_llXferPerfCount = llPerfCount; }	
	void SetWritePerfCount(LONGLONG llPerfCount) { m_llWritePerfCount = llPerfCount; }	
	void SetUpdateClosePerfCount(LONGLONG llPerfCount) { m_llUpdateClosePefCount = llPerfCount; }	
	void SetPreCreatePerfCount(LONGLONG llPerfCount) { m_llPreCreatePerfCount = llPerfCount; }

	void SetTimingFlag(bool bFlag) { m_bTimeCode = bFlag; }

private:
	void CreateFileName(uInt32 u32FileNumber, TCHAR* tName, DWORD dwSize);
	BOOL CreateDirectoryStructure();
	void MakeFormatString(TCHAR *szFormatString, DWORD dwSize, uInt32 u32Count);
	int32 FilloutHeader();
	bool UpdateHeader( int nDequeIndex, disk_file_header* pHeader );
	bool UpdateXferParams(int nIndex);

	deque<DiskBuffer>  m_deqBuffer;
	deque<DiskFile>    m_deqFile;
			
	CsDiskStream* m_pDisk;
	uInt16        m_u16ChanNum;

	uInt32		  m_u32FolderCount;

	static const size_t c_zFileDequeSize;
	static const size_t c_zBufferDequeSize;
	void CreateDeques(void);

	static const int c_nMinSizeFormatString;
	static const int c_nFilesPerFolder;
	static const LPCTSTR c_szDefaultChanDir;
	static const LPCTSTR c_szDefaultFilePrefix;
	static const LPCTSTR c_szFileExtension;

	string m_strPath;
	string m_strDirFormatString;
	HANDLE m_hPreSem;
	HANDLE m_hBufSem;
	HANDLE m_hBufRdySem;
	HANDLE m_hWriteSem;
	HANDLE m_hCloseSem;
	HANDLE m_hAcqEvt;
	HANDLE m_hNewDataEvt;

	HANDLE m_hPreThread;
	HANDLE m_hXferThread;
	HANDLE m_hWriteThread;
	HANDLE m_hCloseThread;

	string m_strBase;
	disk_file_header m_header;
	CSCHANNELCONFIG m_ChanConfig;

	// variables to hold performance counter totals
	LONGLONG m_llXferPerfCount;
	LONGLONG m_llWritePerfCount;
	LONGLONG m_llUpdateClosePefCount;
	LONGLONG m_llPreCreatePerfCount;

	bool m_bTimeCode;

	friend class CsDiskStream;
};
