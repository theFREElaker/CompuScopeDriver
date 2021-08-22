#include "StdAfx.h"
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "CsDiskChannel.h"	
#include "CsDiskStream.h"
#include "CsFs.h"


const size_t CsDiskChannel::c_zFileDequeSize = 4;
const size_t CsDiskChannel::c_zBufferDequeSize = 3;
const int CsDiskChannel::c_nFilesPerFolder = 16000; // to be complient with older file systems
const LPCTSTR CsDiskChannel::c_szDefaultChanDir = _T("XX");
const int CsDiskChannel::c_nMinSizeFormatString = 5;

CsDiskChannel::CsDiskChannel(CsDiskStream* pDisk, string& strBase, uInt16 u16Chan, bool bTimeCode):
m_pDisk(pDisk), m_strBase(strBase), m_u16ChanNum(u16Chan), m_u32FolderCount(1),
m_llXferPerfCount(0), m_llWritePerfCount(0), m_llUpdateClosePefCount(0), m_llPreCreatePerfCount(0),
m_bTimeCode(bTimeCode)
{
	::ZeroMemory(&m_ChanConfig, sizeof(CSCHANNELCONFIG));
	m_ChanConfig.u32Size = sizeof(CSCHANNELCONFIG);
	m_ChanConfig.u32ChannelIndex = m_u16ChanNum;
	int32 i32RetValue = ::CsGet(m_pDisk->GetSystem(), CS_CHANNEL, CS_ACQUISITION_CONFIGURATION, &m_ChanConfig);
	if ( CS_FAILED(i32RetValue) )
	{
		DiskStreamErrorStruct es;
		es.i32ErrorCode = i32RetValue;
		::_tcscpy_s(es.szErrorMessage, _countof(es.szErrorMessage), _T(""));
		throw (&es);
	}
	i32RetValue = FilloutHeader();
	if ( CS_FAILED(i32RetValue) )
	{
		DiskStreamErrorStruct es;
		es.i32ErrorCode = i32RetValue;
		::_tcscpy_s(es.szErrorMessage, _countof(es.szErrorMessage), _T(""));
		throw (&es);
	}

	CreateDeques();

	TCHAR strName[MAX_PATH];
	_stprintf_s( strName, MAX_PATH, _T("CsDskPrfChanPreSem_%d_%02d"), m_pDisk->GetSystem(), m_u16ChanNum );
	m_hPreSem = ::CreateSemaphore( NULL, c_zFileDequeSize, c_zFileDequeSize, strName);
	_stprintf_s( strName, MAX_PATH, _T("CsDskPrfChanWrtSem_%d_%02d"), m_pDisk->GetSystem(), m_u16ChanNum );
	m_hWriteSem = ::CreateSemaphore( NULL, 0, c_zFileDequeSize, strName);
	_stprintf_s( strName, MAX_PATH, _T("CsDskPrfChanClsSem_%d_%02d"), m_pDisk->GetSystem(), m_u16ChanNum );
	m_hCloseSem = ::CreateSemaphore( NULL, 0, c_zFileDequeSize, strName);
	
	_stprintf_s( strName, MAX_PATH, _T("CsDskPrfChanBufSem_%d_%02d"), m_pDisk->GetSystem(), m_u16ChanNum );
	m_hBufSem = ::CreateSemaphore( NULL, c_zBufferDequeSize, c_zBufferDequeSize, strName);
	_stprintf_s( strName, MAX_PATH, _T("CsDskPrfChanBufRdySem_%d_%02d"), m_pDisk->GetSystem(), m_u16ChanNum );
	m_hBufRdySem = ::CreateSemaphore( NULL, 0, c_zBufferDequeSize, strName);

	_stprintf_s( strName, MAX_PATH, _T("CsDskPrfChanAcqEvt_%d_%02d"), m_pDisk->GetSystem(), m_u16ChanNum );
	m_hAcqEvt = ::CreateEvent( NULL, FALSE, FALSE, strName );
	_stprintf_s( strName, MAX_PATH, _T("CsDskPrfChanRdyEvt_%d_%02d"), m_pDisk->GetSystem(), m_u16ChanNum );
	m_hNewDataEvt = ::CreateEvent( NULL, FALSE, FALSE, strName );

	if ( !CreateDirectoryStructure() )
	{
		DiskStreamErrorStruct es;
		es.i32ErrorCode = CS_INVALID_DIRECTORY;
		::_tcscpy_s(es.szErrorMessage, _countof(es.szErrorMessage), _T("Could not create directory structure"));
		throw (&es);
	}

	m_hPreThread   = (HANDLE)::_beginthreadex( NULL, 0, PreCreateFilesThread, this, 0, NULL );
	m_hXferThread  = (HANDLE)::_beginthreadex( NULL, 0, XferDataThread, this, 0, NULL );
	m_hWriteThread = (HANDLE)::_beginthreadex( NULL, 0, WriteDataThread, this, 0, NULL );
	m_hCloseThread = (HANDLE)::_beginthreadex( NULL, 0, UpdateHeaderAndCloseThread, this, 0, NULL );
}


CsDiskChannel::~CsDiskChannel(void)
{
	HANDLE aHdl[4];
	aHdl[0] = m_hPreThread;
	aHdl[1] = m_hXferThread;
	aHdl[2] = m_hWriteThread;
	aHdl[3] = m_hCloseThread;

	::WaitForMultipleObjects( 4, aHdl, TRUE, 10000 );

	::CloseHandle( m_hPreThread );
	::CloseHandle( m_hXferThread );
	::CloseHandle( m_hWriteThread );
	::CloseHandle( m_hCloseThread );
	
	::CloseHandle( m_hPreSem );
	::CloseHandle( m_hBufSem );
	::CloseHandle( m_hBufRdySem );
	::CloseHandle( m_hWriteSem );
	::CloseHandle( m_hCloseSem );
	::CloseHandle( m_hAcqEvt );
	::CloseHandle( m_hNewDataEvt );

	size_t i(0);
	for( i = 0; i < c_zBufferDequeSize; i++ )
	{
		::VirtualFree( m_deqBuffer[i].pBuffer, 0 , MEM_RELEASE );
	}
	for(i = 0; i < c_zFileDequeSize; i++ )
	{
		if( NULL != m_deqFile[i].hFile )
			::CloseHandle( m_deqFile[i].hFile );
	}

	m_deqFile.clear();
	m_deqBuffer.clear();
}

void CsDiskChannel::MakeFormatString(TCHAR *szFormatString, DWORD dwSize, uInt32 u32Count) 
{
	TCHAR szTmp[25];
	size_t tFormatSize;

	_stprintf_s(szTmp, _countof(szTmp), _T("%d"), u32Count);
	// have the number of leading 0's be at least MIN_SIZE_FORMAT_STRING
	tFormatSize = ::_tcslen(szTmp);
	tFormatSize = (tFormatSize < c_nMinSizeFormatString) ? c_nMinSizeFormatString : tFormatSize;
	::_stprintf_s(szFormatString, dwSize, _T("%%0%dd"), tFormatSize); 
}

BOOL CsDiskChannel::CreateDirectoryStructure()
{
	struct _stat buf;
	string strPath;
	TCHAR szCurrentPath[MAX_PATH];
	DWORD dwSize = ::GetCurrentDirectory(MAX_PATH, szCurrentPath);
	if ( 0 == dwSize || dwSize > (MAX_PATH - 1) )
	{
		DiskStreamErrorStruct es;
		es.i32ErrorCode = CS_INVALID_DIRECTORY;
		::_tcscpy_s(es.szErrorMessage, _countof(es.szErrorMessage), _T("Could not get current directory"));
		throw (&es);
	}

	int nResult = ::_tstat(m_strBase.c_str(), &buf);
	if ( -1 == nResult || !(buf.st_mode & _S_IFDIR) )
	{
		BOOL bAbsolutePath = (NULL != ::_tcsstr(m_strBase.c_str(), _T(":\\"))) ? TRUE : FALSE;
		if ( bAbsolutePath )
		{
			if ( !::CreateDirectory(m_strBase.c_str(), NULL) )
			{
				if (ERROR_ALREADY_EXISTS != ::GetLastError())
				{
					DiskStreamErrorStruct es;
					es.i32ErrorCode = CS_INVALID_DIRECTORY;
					string strError("Could not create ");
					strError.append(m_strBase);
					::_tcscpy_s(es.szErrorMessage, _countof(es.szErrorMessage), strError.c_str());
					throw (&es);
				}
			}
			strPath = m_strBase;
		}
		else // create full path, even if it's relative
		{
			strPath = szCurrentPath;
			if ( strPath[strPath.length()] != '\\' )
			{
				strPath.append(_T("\\"));
			}
			strPath.append(m_strBase);
			if ( !::CreateDirectory(strPath.c_str(), NULL) )
			{
				if (ERROR_ALREADY_EXISTS != ::GetLastError())
				{
					DiskStreamErrorStruct es;
					es.i32ErrorCode = CS_INVALID_DIRECTORY;
					string strError("Could not create ");
					strError.append(strPath);
					::_tcscpy_s(es.szErrorMessage, _countof(es.szErrorMessage), strError.c_str());
					throw (&es);
				}
			}
		}
	}
	else // directory exists
	{
		BOOL bAbsolutePath = (NULL != ::_tcsstr(m_strBase.c_str(), _T(":\\"))) ? TRUE : FALSE;
		if ( bAbsolutePath )
		{
			strPath = m_strBase;
		}
		else
		{
			strPath = szCurrentPath;
			if ( strPath[strPath.length()] != '\\' )
			{
				strPath.append(_T("\\"));
			}
			strPath.append(m_strBase);
		}
	}
	
	if ( !::SetCurrentDirectory(strPath.c_str()) )
	{
		DiskStreamErrorStruct es;
		es.i32ErrorCode = CS_INVALID_DIRECTORY;
		string strError("Could not set current directory to ");
		strError.append(strPath);
		::_tcscpy_s(es.szErrorMessage, _countof(es.szErrorMessage), strError.c_str());
		throw (&es);		
	}
	if ( strPath[strPath.length()] != '\\' )
	{
		strPath.append(_T("\\"));
	}

	// Create channel directory
	TCHAR szChanFolder[MAX_PATH];
	
	struct tm tm;
	time_t long_time;
	
	_time64(&long_time);

	errno_t err = ::_localtime64_s( &tm, &long_time ); 
	if (err)
	{
		::_stprintf_s(szChanFolder, _T("%s_CHAN%02d"), c_szDefaultChanDir, m_u16ChanNum);
	}
	else
	{
		::_tcsftime(szChanFolder, MAX_PATH, _T("%Y-%m-%d_%H-%M-%S"), &tm);
		TCHAR szChanString[30];
		::_stprintf_s(szChanString, 30, _T("_CHAN%02d"), m_u16ChanNum);
		::_tcscat_s(szChanFolder, MAX_PATH, szChanString);
	}
	if ( !::CreateDirectory(szChanFolder, NULL) )
	{
		DiskStreamErrorStruct es;
		es.i32ErrorCode = CS_INVALID_DIRECTORY;
		string strError("Could not create ");
		strError.append(szChanFolder);
		::_tcscpy_s(es.szErrorMessage, _countof(es.szErrorMessage), strError.c_str());
		throw (&es);		
	}
	strPath.append(szChanFolder);
	if ( !::SetCurrentDirectory(strPath.c_str()) )
	{
		DiskStreamErrorStruct es;
		es.i32ErrorCode = CS_INVALID_DIRECTORY;
		string strError("Could not set current directory to ");
		strError.append(strPath);
		::_tcscpy_s(es.szErrorMessage, _countof(es.szErrorMessage), strError.c_str());
		throw (&es);		
	}
	m_strPath = strPath;

	// create folder directories
	uInt32 u32FolderCount = m_pDisk->GetFileNum() / c_nFilesPerFolder;
	if ( ((m_pDisk->GetFileNum() % c_nFilesPerFolder) != 0) || (0 == u32FolderCount) )
	{
		u32FolderCount++;
	}

	m_u32FolderCount = u32FolderCount;

	TCHAR szDirFormatString[30];
	TCHAR szFolderNumber[MAX_PATH];

	MakeFormatString(szDirFormatString, _countof(szDirFormatString), u32FolderCount);
	m_strDirFormatString = szDirFormatString;

	TCHAR szTmp[20];
	for (uInt32 u32Folder = 1; u32Folder <= u32FolderCount; u32Folder++)
	{
		::_stprintf_s(szTmp, _countof(szTmp), "Folder.%s", szDirFormatString);
		::_stprintf_s(szFolderNumber, _countof(szFolderNumber), szTmp, u32Folder);
		if ( !::CreateDirectory(szFolderNumber, NULL) )
		{
			DiskStreamErrorStruct es;
			es.i32ErrorCode = CS_INVALID_DIRECTORY;
			string strError("Could not create ");
			strError.append(szFolderNumber);
			::_tcscpy_s(es.szErrorMessage, _countof(es.szErrorMessage), strError.c_str());
			throw (&es);		
		}
	}
	::SetCurrentDirectory(szCurrentPath);
	return TRUE;
}

int32 CsDiskChannel::FilloutHeader()
{
	// Fillout the header with values from the current system.
	// This is a generic header, the individual channel headers will need
	// to be updated with that channels input range and dc offset. For now
	// we'll just use the values from the channel in m_u16ChanNum

	CSSIGSTRUCT SigHeader;
	SigHeader.u32Size = sizeof(CSSIGSTRUCT);
	SigHeader.i64SampleRate = m_pDisk->GetSampleRate();
	SigHeader.i64RecordStart = m_pDisk->GetXferStart();
	SigHeader.i64RecordLength = m_pDisk->GetXferLength();
	SigHeader.u32RecordCount = m_pDisk->GetSegPerFile();
	SigHeader.u32SampleBits = m_pDisk->GetSampleBits();
	SigHeader.u32SampleSize = m_pDisk->GetSampleSize();
	SigHeader.i32SampleOffset = m_pDisk->GetSampleOffset();
	SigHeader.i32SampleRes = m_pDisk->GetSampleRes();
	SigHeader.u32Channel = m_u16ChanNum;
	SigHeader.u32InputRange = m_ChanConfig.u32InputRange;
	SigHeader.i32DcOffset = m_ChanConfig.i32DcOffset;
	
	// this will get overwritten when we update the header if bTimeStamp is true
	SYSTEMTIME stSysTime = {0};
	::GetLocalTime(&stSysTime);
	SigHeader.TimeStamp.u16Hour = stSysTime.wHour;
	SigHeader.TimeStamp.u16Minute = stSysTime.wMinute;
	SigHeader.TimeStamp.u16Second = stSysTime.wSecond;
	SigHeader.TimeStamp.u16Point1Second = stSysTime.wMilliseconds;

	CSDISKFILEHEADER csHeader = {0};
	TCHAR szName[DISK_FILE_CHANNAMESIZE];
	_stprintf_s(szName, DISK_FILE_CHANNAMESIZE, _T("Ch %02d"), m_u16ChanNum);
	int32 i32RetCode = ConvertToSigHeader(&csHeader, &SigHeader, _T("GageScope SIG File"), szName);
	if ( CS_SUCCEEDED(i32RetCode) )
	{
		::CopyMemory(&m_header, &csHeader, sizeof(CSDISKFILEHEADER));
	}
	return i32RetCode;
}


void CsDiskChannel::CreateDeques(void)
{
	m_deqFile.resize(c_zFileDequeSize);
	m_deqBuffer.resize(c_zBufferDequeSize);
	size_t i(0);
	DWORD dwBufferSize = GetDiskStream()->GetBufferSize();
	for( i = 0; i < c_zBufferDequeSize; i++ )
	{
		m_deqBuffer[i].pBuffer = ::VirtualAlloc( NULL, dwBufferSize, MEM_COMMIT, PAGE_READWRITE );
		if ( NULL == m_deqBuffer[i].pBuffer )
		{
//			TCHAR str[100];
//			_stprintf_s(str, 100, _T("CreateDeques: Allocation of %i bytes failed\n"), dwBufferSize);
//			OutputDebugString(str);
			
			for (int j = 0; j < (int)i-1; j++)
			{
				::VirtualFree(m_deqBuffer[j].pBuffer, 0, MEM_RELEASE);
			}
			DiskStreamErrorStruct es;
			es.i32ErrorCode = CS_MEMORY_ERROR;
			::_tcscpy_s(es.szErrorMessage, _countof(es.szErrorMessage), _T(""));
			throw (&es);		
		}
	}
	for(i = 0; i < c_zFileDequeSize; i++ )
	{
		m_deqFile[i].hFile = NULL;
	}
}
