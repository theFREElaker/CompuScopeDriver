#include "StdAfx.h"
#include "CsDiskChannel.h"	
#include "CsDiskStream.h"

const LPCTSTR CsDiskChannel::c_szDefaultFilePrefix = _T("File");
const LPCTSTR CsDiskChannel::c_szFileExtension = _T(".sig");

UINT __stdcall CsDiskChannel::PreCreateFilesThread(LPVOID pParms)
{
	CsDiskChannel* pThis = static_cast<CsDiskChannel*> (pParms);
	
	HANDLE aHndls[2];
	aHndls[0] = pThis->GetDiskStream()->GetStopEvent();
	aHndls[1] = pThis->m_hPreSem;

	uInt32 u32FileNumber(0);
	TCHAR tName[MAX_PATH];

	LONGLONG llPreCreatePerfThread = 0;
	LARGE_INTEGER liStart = {0};
	LARGE_INTEGER liEnd = {0};


	for(;;)
	{
		DWORD dw = ::WaitForMultipleObjects(2, aHndls, FALSE, INFINITE ); 
		if( WAIT_OBJECT_0 == dw )
		{
			if ( pThis->m_bTimeCode )
			{
				pThis->SetPreCreatePerfCount(llPreCreatePerfThread);
			}
			::ReleaseSemaphore(pThis->m_hWriteSem, 1, 0 );
			::_endthreadex(0);
		}
		if ( pThis->m_bTimeCode )
		{
			QueryPerformanceCounter( &liStart );
		}

		pThis->CreateFileName(u32FileNumber, tName, MAX_PATH);
//		RG test to see if the FILE_FLAG_NO_BUFFERING flag makes a difference. if we do use this in the release
//		we need to call GetFreeDiskSpace to determine the volume sector size (bytes per sector) and must write
//		multiples of that value. also, buffers must be sector aligned (created by Virtuall Alloc)
//		DWORD dwSectPerClust, dwBytesPerSect, dwFreeClust, dwTotal;
//		GetDiskFreeSpace(NULL, &dwSectPerClust, &dwBytesPerSect, &dwFreeClust, &dwTotal);
#define nUSE_THIS_FLAG
#ifdef USE_THIS_FLAG
		HANDLE hFile = ::CreateFile( tName,  GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_NO_BUFFERING, NULL );		
#else
		HANDLE hFile = ::CreateFile( tName,  GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL );
#endif
		if( INVALID_HANDLE_VALUE == hFile )
		{
			::SetEvent(pThis->GetDiskStream()->GetStopEvent());
			if ( pThis->m_bTimeCode )
			{
				pThis->SetPreCreatePerfCount(llPreCreatePerfThread);
			}
			// put error in the deque
			// for now we're only putting errors from threads in the error queue, other parts of the library
			// will throw an exception on error. Also for now we're only going to report the first error in the queue

			TCHAR szErrorMsg[MAX_PATH];
			::_stprintf_s(szErrorMsg, MAX_PATH, _T("Could not create %s"), tName);
			pThis->GetDiskStream()->QueueError(g_nThreadErrorCode, szErrorMsg);
			::ReleaseSemaphore(pThis->m_hWriteSem, 1, 0 );
			::_endthreadex(10);
		}
		
		DWORD dwWritten;
		if( 0 == ::WriteFile( hFile, &pThis->m_header, sizeof(pThis->m_header), &dwWritten, NULL ) )
		{
			::SetEvent(pThis->GetDiskStream()->GetStopEvent());
			if ( pThis->m_bTimeCode )
			{
				pThis->SetPreCreatePerfCount(llPreCreatePerfThread);
			}
			// put error in the deque
			TCHAR szErrorMsg[MAX_PATH];
			::_stprintf_s(szErrorMsg, MAX_PATH, _T("Could not write %s"), tName);
			pThis->GetDiskStream()->QueueError(g_nThreadErrorCode, szErrorMsg);
			::ReleaseSemaphore(pThis->m_hWriteSem, 1, 0 );
			::_endthreadex(30);
		}

		int nDequeIndex = u32FileNumber%pThis->m_deqFile.size();
		pThis->m_deqFile[nDequeIndex].hFile = hFile;
		u32FileNumber++;

		if ( pThis->m_bTimeCode )
		{
			::QueryPerformanceCounter( &liEnd );
			llPreCreatePerfThread += liEnd.QuadPart - liStart.QuadPart;
		}

		::ReleaseSemaphore(pThis->m_hWriteSem, 1, 0 );

		if( u32FileNumber >=  pThis->GetDiskStream()->GetFileNum() )
		{
			if ( pThis->m_bTimeCode )
			{
				pThis->SetPreCreatePerfCount(llPreCreatePerfThread);
			}
			::_endthreadex(100);
		}
	}
}

UINT __stdcall CsDiskChannel::UpdateHeaderAndCloseThread(LPVOID pParms)
{
	CsDiskChannel* pThis = static_cast<CsDiskChannel*> (pParms);
	
	HANDLE pHndls[2];
	pHndls[0] = pThis->GetDiskStream()->GetStopEvent();
	pHndls[1] = pThis->m_hCloseSem;

	disk_file_header header;
	uInt32 u32FileNumber(0);

	LONGLONG llUpdateClosePerfCount = 0;
	LARGE_INTEGER liStart = {0};
	LARGE_INTEGER liEnd = {0};

	for(;;)
	{

		DWORD dw = ::WaitForMultipleObjects(2, pHndls, FALSE, INFINITE ); 
		if( WAIT_OBJECT_0 == dw )
		{
			if ( pThis->m_bTimeCode )
			{
				pThis->SetUpdateClosePerfCount(llUpdateClosePerfCount);
			}
			int nFileDequeIndex = u32FileNumber %  pThis->m_deqFile.size();
			::CloseHandle(pThis->m_deqFile[nFileDequeIndex].hFile);
			pThis->m_deqFile[nFileDequeIndex].hFile = NULL;
			::ReleaseSemaphore(pThis->m_hPreSem, 1, 0 );
			::_endthreadex(0);
		}
		if ( pThis->m_bTimeCode )
		{
			::QueryPerformanceCounter( &liStart );
		}
		int nFileDequeIndex = u32FileNumber %  pThis->m_deqFile.size();
		if( pThis->UpdateHeader(nFileDequeIndex, &header) )
		{
			DWORD dwWritten;
			::SetFilePointer( pThis->m_deqFile[nFileDequeIndex].hFile, 0, 0, FILE_BEGIN );
			if( 0 == ::WriteFile( pThis->m_deqFile[nFileDequeIndex].hFile, &header, sizeof(header), &dwWritten, NULL ) )
			{
				::SetEvent(pThis->GetDiskStream()->GetStopEvent());
				if ( pThis->m_bTimeCode )
				{
					pThis->SetUpdateClosePerfCount(llUpdateClosePerfCount);
				}

				// put error in the deque
				TCHAR szErrorMsg[MAX_PATH];
				::_stprintf_s(szErrorMsg, MAX_PATH, _T("Could not write file #%d"), u32FileNumber);
				pThis->GetDiskStream()->QueueError(g_nThreadErrorCode, szErrorMsg);
				::CloseHandle(pThis->m_deqFile[nFileDequeIndex].hFile);
				pThis->m_deqFile[nFileDequeIndex].hFile = NULL;
				::ReleaseSemaphore(pThis->m_hPreSem, 1, 0 );
				::_endthreadex(20);
			}
		}
		::CloseHandle(pThis->m_deqFile[nFileDequeIndex].hFile);
		pThis->m_deqFile[nFileDequeIndex].hFile = NULL;
		u32FileNumber++;
		::InterlockedIncrement((volatile LONG *)&(pThis->GetDiskStream()->m_u32FileCount));
		::ReleaseSemaphore(pThis->m_hPreSem, 1, 0 );
	
		if ( pThis->m_bTimeCode )
		{
			::QueryPerformanceCounter( &liEnd );
			llUpdateClosePerfCount += liEnd.QuadPart - liStart.QuadPart;
		}
		if( u32FileNumber >=  pThis->GetDiskStream()->GetFileNum() )
		{
			if ( pThis->m_bTimeCode )
			{
				pThis->SetUpdateClosePerfCount(llUpdateClosePerfCount);
			}
			_endthreadex(100);
		}
	}
}


void CsDiskChannel::CreateFileName(uInt32 u32FileNumber, TCHAR* tName, DWORD dwSize )
{
	// m_strPath holds the path, except for the folder numbers
	// m_u32FolderCount holds the number of folders in the channel directory
	TCHAR szFileName[MAX_PATH];
	TCHAR szTmp[25];
	TCHAR szFileFormatString[25];
	TCHAR szFolderNumber[MAX_PATH];
	
	string strPath = m_strPath;
	if ( strPath[strPath.length()] != '\\' )
	{
		strPath.append(_T("\\"));
	}
	
	uInt32 u32Folder = (u32FileNumber / c_nFilesPerFolder) + 1;

	_stprintf_s(szTmp, _countof(szTmp), "Folder.%s", m_strDirFormatString.c_str());
	_stprintf_s(szFolderNumber, _countof(szFolderNumber), szTmp, u32Folder);
	
	strPath.append(szFolderNumber);
	if ( strPath[strPath.length()] != '\\' )
	{
		strPath.append(_T("\\"));
	}

	MakeFormatString(szFileFormatString, _countof(szFileFormatString), m_pDisk->GetFileNum());
	_stprintf_s(szTmp, _countof(szTmp), "%s-%s", c_szDefaultFilePrefix, szFileFormatString);
	// add 1 to filenumber so we start at 1
	_stprintf_s(szFileName, _countof(szFileName), szTmp, u32FileNumber+1);

	strPath.append(szFileName);
	strPath.append(c_szFileExtension);
	::_tcsncpy_s(tName, dwSize, strPath.c_str(), strPath.length());
}

bool CsDiskChannel::UpdateHeader( int nFileDequeIndex, disk_file_header* pHeader )
{
	::CopyMemory( pHeader, &m_header, sizeof(m_header) );

	if( GetDiskStream()->GetPostTimeStamp() )
	{
		SYSTEMTIME sys = m_deqFile[nFileDequeIndex].Ts;

		//convert TimeStamp to header fields. have to change milliseconds to  of a second
		pHeader->trigger_time = 0; // if trigger time is 0, the extended trigger time is used
		uInt32 u32TrigTime = ((sys.wMilliseconds + 5)/10) & 0x7f; //round to a 100ths of the sec
		u32TrigTime |= (sys.wSecond & 0x3f) << 7;
		u32TrigTime |= (sys.wMinute & 0x3f) << 13;
		u32TrigTime |= (sys.wHour & 0x1f) << 19;
		pHeader->extended_trigger_time = u32TrigTime;
		
		// since we've filled in extended_trigger_time we'll zero out trigger_time to avoid confusion
		pHeader->trigger_time = 0;
		// store the trigger time as a string (complete with milliseconds as a string in the comment field)
		TCHAR szDateTime[MAX_PATH];
		::_stprintf_s(szDateTime, _countof(szDateTime), _T("Time stamp: %4u/%02u/%02u %02u:%02u:%02u:%03u"), 
								sys.wYear, sys.wMonth, sys.wDay, sys.wHour, sys.wMinute, sys.wSecond, sys.wMilliseconds);
		::_tcsncpy_s(pHeader->comment, _countof(pHeader->comment), szDateTime, _TRUNCATE);
	}
	// Update Start address - in the sig file the start address is always 0 and the trigger address is always -start
	pHeader->trigger_address = -m_deqFile[nFileDequeIndex].i32StartAddr;
	pHeader->sample_depth = m_deqFile[nFileDequeIndex].dwTotalWritten / GetDiskStream()->GetSampleSize();//RG
	pHeader->multiple_record_count = pHeader->sample_depth / pHeader->record_depth;
	pHeader->starting_address = 0;
	if ( pHeader->multiple_record_count > 1 )
		pHeader->ending_address = pHeader->record_depth - 1;
	else
		pHeader->ending_address = pHeader->sample_depth - 1;
	
	pHeader->trigger_depth = pHeader->ending_address - pHeader->trigger_address + 1;

	if (pHeader->trigger_depth < 0) // RG added Sept 21, 2009
		pHeader->trigger_depth = 0;
	
	return  0 != ::memcmp( pHeader, &m_header, sizeof(m_header) );
}

