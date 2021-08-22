#include "stdafx.h"
#include "CsSpDevice.h"

#define SPD_SN_SIZE            16 
#define HIGH_USB_RESET_LEVEL   16

int32 CsSpDevice::StartAcquisition(void)
{
	int r = m_pAdq->DisarmTrigger();
	r = m_pAdq->ArmTrigger();
	if(m_bDisplayTrace) 
	{
		::OutputDebugString("m_pAdq->DisarmTrigger()\n");
		::OutputDebugString("m_pAdq->ArmTrigger()\n");
	}
	m_bStarted = true;
	return CS_SUCCESS;
}

int32 CsSpDevice::ForceTrigger(void)
{
	m_pAdq->SWTrig();
	if(m_bDisplayTrace) 
		::OutputDebugString("m_pAdq->SWTrig()\n");
	return CS_SUCCESS;
}

int32 CsSpDevice::Abort(void)
{
	int r;
	uInt32 u32Status;
	GetAcquisitionStatus( &u32Status );
	if( u32Status != ACQ_STATUS_READY )
	{
		r = m_pAdq->DisarmTrigger();
		m_bStarted = false;
		if(m_bDisplayTrace) 
			::OutputDebugString("m_pAdq->DisarmTrigger()\n");
	}	
	else 
	{
		//check for transfer
	}
	return CS_SUCCESS;
}

int32 CsSpDevice::Reset(void)
{
	Abort();
	int32 i32Ret = ResetDevice();
	if( CS_FAILED(i32Ret) ) 
		return i32Ret;
	return AcquisitionSystemInit(true);
}

int32 CsSpDevice::GetAcquisitionStatus(uInt32* pu32Status)
{ 
	bool bTrig = true;
	bool bBusy = false;
	bool bOverflow = false;
	if( m_bStarted )
	{
		bTrig = 0 == m_pAdq->GetWaitingForTrigger();
		bBusy = 0 == m_pAdq->GetAcquiredAll();
		bOverflow = 0 != m_pAdq->GetStreamOverflow();
		if(m_bDisplayTrace) 
		{
			char str[128];
			::_stprintf_s( str, 128,  "m_pAdq->GetWaitingForTrigger() returned %s\n", bTrig? "false":"true");
			::OutputDebugString(str);
			::_stprintf_s( str, 128,  "m_pAdq->GetAcquiredAll() returned %s\n", bBusy? "false":"true");
			::OutputDebugString(str);
			::_stprintf_s( str, 128,  "m_pAdq->GetStreamOverflow() returned %s\n", bOverflow? "true":"false");
			::OutputDebugString(str);
		}
		if( bOverflow )
		{
			m_pAdq->DisarmTrigger();
			m_pAdq->ArmTrigger();
			bOverflow = 0 != m_pAdq->GetStreamOverflow();
			bTrig = 0 == m_pAdq->GetWaitingForTrigger();
			bBusy = 0 == m_pAdq->GetAcquiredAll();
			//ResetDevice();
			//AcquisitionSystemInit(false);
			//StartAcquisition();
		}
		
	}
	if( !bBusy )
		m_bStarted = false;
	*pu32Status = bBusy ? (bTrig?ACQ_STATUS_TRIGGERED:ACQ_STATUS_WAIT_TRIGGER) : ACQ_STATUS_READY;
	return CS_SUCCESS;
}

int32 CsSpDevice::ResetTimeStamp(void)
{
	m_pAdq->ResetTrigTimer( 1 );
	if(m_bDisplayTrace) 
		::OutputDebugString("m_pAdq->ResetTrigTimer(1)\n");
	return CS_SUCCESS;
}

int32 CsSpDevice::ResetDevice(void)
{
	if( NULL !=  m_pManager->GetCommEvent() )
		::ResetEvent( m_pManager->GetCommEvent() );
	m_pAdq->ResetDevice(HIGH_USB_RESET_LEVEL);
	if(m_bDisplayTrace) 
		::OutputDebugString("m_pAdq->ResetDevice(16)\n");
	//Communication event will be working only in service mode of RM, so for application mode use 4 sec timeout
	if( NULL !=  m_pManager->GetCommEvent() )
		::WaitForSingleObject( m_pManager->GetCommEvent(), SPD_INIT_DELAY );
	else
		::Sleep( SPD_INIT_DELAY );
	
	int nFoundDev = m_pManager->m_pfFindDevices(m_pManager->GetControl()); 
	m_pAdq = NULL;
	for ( int i = 1; i <= nFoundDev; i++ )
	{
		ADQInterface* pAdq = m_pManager->m_pfGetAdqInterface(m_pManager->GetControl(), i);
		if( 0 == strncmp(m_strSerNum, pAdq->GetBoardSerialNumber(), SPD_SN_SIZE ) )
		{
			m_pAdq = pAdq;
			m_nNum = i;
			break;
		}
	}
	if ( NULL == m_pAdq )
	{
		return CS_INVALID_HANDLE;
	}
	return CS_SUCCESS;
}