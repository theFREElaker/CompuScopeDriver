#include "StdAfx.h"
#include "CsDiskChannel.h"	
#include "CsDiskStream.h"


UINT __stdcall CsDiskChannel::WriteDataThread(LPVOID pParms)
{
	CsDiskChannel* pThis = static_cast<CsDiskChannel*> (pParms);

	int nBufferNumber(0);
	uInt32 u32FileNumber(0);
	
	HANDLE aHndl[3];
	HANDLE  hStop = pThis->GetDiskStream()->GetStopEvent();
	aHndl[0] = pThis->GetDiskStream()->GetWriteSem();
	aHndl[1] = pThis->m_hBufRdySem;
	aHndl[2] = pThis->m_hWriteSem;

	DWORD dwTotalWritten(0);
	int32 i32StartAddr = int32(pThis->GetDiskStream()->GetXferStart());

	LONGLONG llWritePerfCount = 0;
	LARGE_INTEGER liStart = {0};
	LARGE_INTEGER liEnd = {0};

	for(;;)
	{
		DWORD dw = WAIT_TIMEOUT;
		while(WAIT_TIMEOUT == dw)
		{
			if( WAIT_OBJECT_0 == ::WaitForSingleObject(hStop, 0 ) )
			{
				if ( pThis->m_bTimeCode )
				{
					pThis->SetWritePerfCount(llWritePerfCount);
				}
				::_endthreadex(0);
			}
			dw = ::WaitForMultipleObjects(3, aHndl, TRUE, 100 ); 
		}

		if ( pThis->m_bTimeCode )
		{
			::QueryPerformanceCounter( &liStart );
		}

		uInt32 u32FileIndex = u32FileNumber % pThis->m_deqFile.size();
		HANDLE hFile = pThis->m_deqFile[u32FileIndex].hFile;

		int nBufIndex = nBufferNumber % pThis->m_deqBuffer.size();
		char* ptr = static_cast<char*>(pThis->m_deqBuffer[nBufIndex].pBuffer);
		uInt32 u32BufLen = pThis->m_deqBuffer[nBufIndex].u32Len;
		uInt32 u32SpaceInFile = pThis->GetDiskStream()->GetFileSize() - dwTotalWritten;
		if( 0 == dwTotalWritten )
			pThis->m_deqFile[u32FileIndex].Ts = pThis->m_deqBuffer[nBufIndex].Ts;
		ptr += pThis->m_deqBuffer[nBufIndex].i32Off;
		bool bFreeBuffer = true;
		bool bFinalBuffer = pThis->m_deqBuffer[nBufIndex].bFinal;
		if( u32SpaceInFile < u32BufLen )
		{
			//Mulrec files should fit exactly
			ASSERT( 1 == pThis->GetDiskStream()->GetSegPerFile()  );
			u32BufLen = u32SpaceInFile;
			pThis->m_deqBuffer[nBufIndex].i32Off += u32BufLen;
			pThis->m_deqBuffer[nBufIndex].u32Len -= u32BufLen;
			bFreeBuffer = false;
			bFinalBuffer = false;
		}
		
		DWORD dwWritten;
		if( 0 == ::WriteFile(hFile, ptr, u32BufLen, &dwWritten, NULL) )
		{
			::SetEvent(hStop);
			if ( pThis->m_bTimeCode )
			{
				pThis->SetWritePerfCount(llWritePerfCount);
			}
			// put error in the deque
			TCHAR szErrorMsg[MAX_PATH];
			::_stprintf_s(szErrorMsg, MAX_PATH, _T("Could not write file #%d"), u32FileNumber);
			pThis->GetDiskStream()->QueueError(g_nThreadErrorCode, szErrorMsg);
			::_endthreadex(0);
		}

		dwTotalWritten += dwWritten;
		
		pThis->m_deqFile[u32FileIndex].i32StartAddr = i32StartAddr;
		pThis->m_deqFile[u32FileIndex].dwTotalWritten = dwTotalWritten;
		
		if( bFreeBuffer )
		{
			nBufferNumber++;
			::ReleaseSemaphore(pThis->m_hBufSem, 1, NULL );
		}
		else
			::ReleaseSemaphore(pThis->m_hBufRdySem, 1, NULL );

		::ReleaseSemaphore(pThis->GetDiskStream()->GetWriteSem(), 1, NULL );

		if( dwTotalWritten >= pThis->GetDiskStream()->GetFileSize() || bFinalBuffer )
		{
			if( bFinalBuffer || 1 != pThis->GetDiskStream()->GetSegPerFile() )
				i32StartAddr = int32(pThis->GetDiskStream()->GetXferStart());
			else
				i32StartAddr += int32(dwTotalWritten/pThis->GetDiskStream()->GetSampleSize());
			dwTotalWritten = 0;
			u32FileNumber++;
			if ( pThis->m_bTimeCode )
			{
				::QueryPerformanceCounter( &liEnd );
				llWritePerfCount += liEnd.QuadPart - liStart.QuadPart;
			}
			::ReleaseSemaphore(pThis->m_hCloseSem, 1, NULL);
			if( u32FileNumber >=  pThis->GetDiskStream()->GetFileNum() )
			{
				if ( pThis->m_bTimeCode )
				{
					pThis->SetWritePerfCount(llWritePerfCount);
				}
				::_endthreadex(100);
			}
		}
		else
		{
			if ( pThis->m_bTimeCode )
			{
				::QueryPerformanceCounter( &liEnd );
				llWritePerfCount += liEnd.QuadPart - liStart.QuadPart;
			}
			::ReleaseSemaphore(pThis->m_hWriteSem, 1, NULL );
		}
	}
}
