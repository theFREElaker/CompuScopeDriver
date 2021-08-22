// CsRabbiEvtThread.cpp
//

#include "StdAfx.h"
#include "CsTypes.h"
#include "CsRabbit.h"

//------------------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------------------

void	CsRabbitDev::SignalEvents( BOOL bAquisitionEvent )
{
#ifdef __linux__// only do this for Linux if we're not recovering from crash	
	if ( m_pSystem->bCrashRecovery	)
	{
		return;
	}
#endif // __linux__		
	if ( bAquisitionEvent )
	{
		_ASSERT( this == m_pTriggerMaster );
		if ( this != m_pTriggerMaster )
			return;

		if ( m_PlxData->CsBoard.bTriggered ) //received trigger interrupt
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

