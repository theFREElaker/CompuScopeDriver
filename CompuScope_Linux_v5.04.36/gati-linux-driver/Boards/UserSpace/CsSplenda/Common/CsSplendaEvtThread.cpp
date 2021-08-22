
#include "StdAfx.h"
#include "CsSplenda.h"


//------------------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------------------

void	CsSplendaDev::SignalEvents( BOOL bAquisitionEvent )
{
#ifdef __linux__// only do this for Linux if we're not recovering from crash	
	if ( m_pSystem->bCrashRecovery	)
	{
		return;
	}
#endif // __linux__	
	if ( bAquisitionEvent )
	{
		// These events only valid for the master card
		_ASSERT( this == m_pTriggerMaster );
		if ( this != m_pTriggerMaster )
			return;

		if ( m_PlxData->CsBoard.bTriggered )	//received trigger interrupt
		{
			if (  m_pSystem->u32TriggerNotification == 0 )
			{
				m_pSystem->u16AcqStatus = ACQ_STATUS_TRIGGERED;
				m_pSystem->u32TriggerNotification ++;

				//----- Signal registered events for trigger
				if ( m_pSystem->hUserEventTriggered )		// Application created event
					SetEvent( m_pSystem->hUserEventTriggered );
			}
		}

		if ( ! m_PlxData->CsBoard.bBusy )	//received End of busy Interrupt
		{
			if ( ! m_pSystem->bEndAcquisitionNotification )
			{
				m_pSystem->u16AcqStatus = ACQ_STATUS_READY;
				m_pSystem->bEndAcquisitionNotification = TRUE;

				//----- Signal registered events for busy
				if ( m_pSystem->hUserEventEndOfBusy )		// Application created event
					SetEvent( m_pSystem->hUserEventEndOfBusy );
			}
		}
	}
	else
	{
		// Asyn data transfer event
		if ( m_DataTransfer.hUserAsynEvent )
			SetEvent( m_DataTransfer.hUserAsynEvent );
	}

}

//------------------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------------------

void	CsSplendaDev::OnAlarmEvent()
{
	CsSplendaDev *pDevice = m_MasterIndependent;

#if !defined(USE_SHARED_CARDSTATE)
	SPLENDA_CARDSTATE			tempCardState = {0};
	
	pDevice->IoctlGetCardState(&tempCardState);
	pDevice->m_pCardState->u32ActualSegmentCount = tempCardState.u32ActualSegmentCount;
#endif

	if ( pDevice->m_pCardState->u32ActualSegmentCount > 2 )
		pDevice->m_pCardState->u32ActualSegmentCount -= 2;
	else
		pDevice->m_pCardState->u32ActualSegmentCount = 0;

	pDevice->m_pCardState->bAlarm = TRUE;

	while (pDevice)
	{
		if ( pDevice->GetChannelProtectionStatus() )
			pDevice->ReportProtectionFault();
		
		pDevice = pDevice->m_Next;
	}
}
