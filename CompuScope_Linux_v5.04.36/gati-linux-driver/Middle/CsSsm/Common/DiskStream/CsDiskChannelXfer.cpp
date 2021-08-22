#include "StdAfx.h"
#include "CsDiskChannel.h"	
#include "CsDiskStream.h"

UINT __stdcall CsDiskChannel::XferDataThread(LPVOID pParms)
{
	CsDiskChannel* pThis = static_cast<CsDiskChannel*> (pParms);

	int nBufferNumber(0);
	uInt32 u32AcqNumber(0);
	
	HANDLE aHndlAcq[2];
	aHndlAcq[0] = pThis->GetDiskStream()->GetStopEvent();
	aHndlAcq[1] = pThis->m_hNewDataEvt;

	HANDLE aHndlXfer[2];
	aHndlXfer[0] = pThis->GetDiskStream()->GetXferSem();
	aHndlXfer[1] = pThis->m_hBufSem;

	int64  i64CurAddr = pThis->GetDiskStream()->GetXferStart();
	uInt32 u32CurRec = pThis->GetDiskStream()->GetRecStart();
	uInt32 u32CurrentLength = pThis->GetDiskStream()->GetAddrInc();

	IN_PARAMS_TRANSFERDATA TransferIn;
	OUT_PARAMS_TRANSFERDATA TransferOut;	

	TransferIn.u32Mode = TxMODE_DEFAULT;
	TransferIn.u16Channel = pThis->m_u16ChanNum; 
	TransferIn.hNotifyEvent = NULL;

	LONGLONG llXferPerfCount = 0;
	LARGE_INTEGER liStart = {0};
	LARGE_INTEGER liEnd = {0};

	for(;;)
	{
		DWORD dw = ::WaitForMultipleObjects(2, aHndlAcq, FALSE, INFINITE ); 
		if( WAIT_OBJECT_0 == dw )
		{
			if ( pThis->m_bTimeCode )
			{
				pThis->SetXferPerfCount(llXferPerfCount);
			}
			::_endthreadex(0);
		}
		//New data is ready
		uInt32 u32AcqIx = u32AcqNumber % pThis->GetDiskStream()->GetAcqDequeSize();
		for(;;)
		{
			dw = WAIT_TIMEOUT;
			while(WAIT_TIMEOUT == dw)
			{
				if( WAIT_OBJECT_0 == ::WaitForSingleObject(aHndlAcq[0], 0 ) )
				{
					if ( pThis->m_bTimeCode )
					{
						pThis->SetXferPerfCount(llXferPerfCount);
					}
					::_endthreadex(0);
				}
				dw = ::WaitForMultipleObjects(2, aHndlXfer, TRUE, 100 ); 
			}
			//acquired Xfer sems

			if ( pThis->m_bTimeCode )
			{
				::QueryPerformanceCounter( &liStart );
			}

			int nBufIx = nBufferNumber % pThis->m_deqBuffer.size();
			TransferIn.i64StartAddress = i64CurAddr;
			TransferIn.i64Length = u32CurrentLength+ pThis->GetDiskStream()->GetGroupPadding();
			TransferIn.pDataBuffer = pThis->m_deqBuffer[nBufIx].pBuffer;	
			TransferIn.u32Segment = u32CurRec;
			int32 i32 = ::CsTransfer(pThis->GetDiskStream()->GetSystem(), &TransferIn, &TransferOut);
			pThis->m_deqBuffer[nBufIx].i32Off = int32(TransferIn.i64StartAddress - TransferOut.i64ActualStart)*pThis->GetDiskStream()->GetSampleSize();
			pThis->m_deqBuffer[nBufIx].u32Len = u32CurrentLength*pThis->GetDiskStream()->GetSampleSize();
			pThis->m_deqBuffer[nBufIx].Ts = pThis->GetDiskStream()->GetTimeStamp(u32AcqIx);

			if( CS_FAILED(i32) || 0 > pThis->m_deqBuffer[nBufIx].i32Off )
			{
				::SetEvent(pThis->GetDiskStream()->GetStopEvent());
				if ( pThis->m_bTimeCode )
				{
					pThis->SetXferPerfCount(llXferPerfCount);
				}
				// put error in the deque
				pThis->GetDiskStream()->QueueError(i32, _T(""));
				::_endthreadex(0);
			}
			bool bNewAcq = false;
			bool bNewRec = false;
			if( i64CurAddr + u32CurrentLength >= pThis->GetDiskStream()->GetXferEnd() )
			{
				bNewRec = true;
				u32CurrentLength = pThis->GetDiskStream()->GetAddrInc();
				i64CurAddr = pThis->GetDiskStream()->GetXferStart();
				u32CurRec++;
				if( u32CurRec >= pThis->GetDiskStream()->GetRecEnd() )
				{
					u32CurRec = pThis->GetDiskStream()->GetRecStart();
					bNewAcq = true;
				}
			}
			else
			{
				i64CurAddr += u32CurrentLength;
				if( i64CurAddr + u32CurrentLength > pThis->GetDiskStream()->GetXferEnd() )
					u32CurrentLength = uInt32(pThis->GetDiskStream()->GetXferEnd() - i64CurAddr);
			}		
			if ( pThis->m_bTimeCode )
			{
				::QueryPerformanceCounter( &liEnd );
				llXferPerfCount += liEnd.QuadPart - liStart.QuadPart;
			}

			if( bNewAcq )
			{
				::SetEvent(pThis->m_hAcqEvt);
				u32AcqNumber++;
			}

			//The buffer is final only if not stacking multiple acquisitions in one file
			if( 1 == pThis->GetDiskStream()->GetSegPerFile() )
				pThis->m_deqBuffer[nBufIx].bFinal = bNewRec;
			else if( 1 == pThis->GetDiskStream()->GetAcqConfig().u32SegmentCount )
				pThis->m_deqBuffer[nBufIx].bFinal = u32AcqNumber >= pThis->GetDiskStream()->GetAcqNum();
			else
				pThis->m_deqBuffer[nBufIx].bFinal = bNewAcq;

			nBufferNumber++;

			::ReleaseSemaphore(pThis->GetDiskStream()->GetXferSem(), 1, NULL );
			::ReleaseSemaphore(pThis->m_hBufRdySem, 1, NULL );

			if( bNewAcq )
			{
				if( u32AcqNumber >= pThis->GetDiskStream()->GetAcqNum() )
				{
					if ( pThis->m_bTimeCode )
					{
						pThis->SetXferPerfCount(llXferPerfCount);
					}
					::_endthreadex(0);
				}
				break;
			}
		}
	}
}