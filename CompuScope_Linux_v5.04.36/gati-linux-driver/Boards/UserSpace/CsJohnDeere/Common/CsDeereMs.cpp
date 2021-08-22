
#include "StdAfx.h"
#include "CsDeere.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsDeereDevice::DetectMasterSlave(int16 *i16NumMasterSlaveSystems, int16 *i16NumStandAloneSystems)
{
	CsDeereDevice	*pHead = (CsDeereDevice *) m_ProcessGlblVars->pFirstUnCfg;
	CsDeereDevice*  pCurrent = pHead;
	bool	         bFoundMaster = false;

	uInt32	u32Data;

	*i16NumStandAloneSystems = 0;
	*i16NumMasterSlaveSystems = 0;

	INITSTATUS *pInitStatus = 0;
	// First pass. Search for master cards
	while( NULL != pCurrent )
	{
		pInitStatus = pCurrent->GetInitStatus();

		// Skip all boards that have errors on Nios init or AddOn connection
		if( pInitStatus->Info.u32Nios == 0 || pInitStatus->Info.u32AddOnPresent == 0)
		{
			pCurrent = pCurrent->m_Next;
			continue;
		}
		
		// Search for Master card
		u32Data = pCurrent->ReadRegisterFromNios( 0, DEERE_MS_REG_RD );
		pCurrent->m_pCardState->u8MsCardId = ((uInt8) (u32Data >>10)) & 0x7;

		if ( 0 == pCurrent->m_pCardState->u8MsCardId )
		{
			bFoundMaster = true;
			pCurrent->SetRoleMode(ermMaster);
		}
		pCurrent = pCurrent->m_Next;
	}
	// If Master card not found, it is not worth to go further
	// All cards will behave as stand alone cards
	if ( !bFoundMaster )
	{
		pCurrent = pHead;
		while( NULL != pCurrent )
		{
			pInitStatus = pCurrent->GetInitStatus();

			// Skip all boards that have errors on Nios init or AddOn connection
			if( pInitStatus->Info.u32Nios != 0 && pInitStatus->Info.u32AddOnPresent != 0 )
			{
				(*i16NumStandAloneSystems)++;
			}
			pCurrent = pCurrent->m_Next;
		}
		return;
	}

	// Second pass. Enable MS BIST
	pCurrent = pHead;
	while( NULL != pCurrent )
	{
		pInitStatus = pCurrent->GetInitStatus();

		// Skip all boards that have errors on Nios init or AddOn connection
		if( pInitStatus->Info.u32Nios == 0 || pInitStatus->Info.u32AddOnPresent == 0 )
		{
			pCurrent = pCurrent->m_Next;
			continue;
		}
		pCurrent->EnableMsBist(true);
		pCurrent = pCurrent->m_Next;
	}
	
	// Third pass. Find slaves that belong to the master
	pCurrent = pHead;
	while( NULL != pCurrent )
	{
		pInitStatus = pCurrent->GetInitStatus();

		// Skip all boards that have errors on Nios init or AddOn connection
		if( pInitStatus->Info.u32Nios == 0 || pInitStatus->Info.u32AddOnPresent == 0 )
		{
			pCurrent = pCurrent->m_Next;
			continue;
		}

		if( ermMaster == pCurrent->m_pCardState->ermRoleMode  )
		{
			if( CS_SUCCEEDED(pCurrent->DetectSlaves()) )
				(*i16NumMasterSlaveSystems)++;
			else
				pCurrent->SetRoleMode(ermStandAlone);
		}
		pCurrent = pCurrent->m_Next;
	}
	// Third pass. Count remaining stand alone cards
	// and disable the bist
	pCurrent = pHead;
	while( NULL != pCurrent )
	{
		pInitStatus = pCurrent->GetInitStatus();

		// Skip all boards that have errors on Nios init or AddOn connection
		if( pInitStatus->Info.u32Nios == 0 || pInitStatus->Info.u32AddOnPresent == 0 )
		{
			pCurrent = pCurrent->m_Next;
			continue;
		}
		if( pCurrent->m_pCardState->ermRoleMode == ermStandAlone )
			(*i16NumStandAloneSystems)++;
		pCurrent->EnableMsBist(false);
		pCurrent = pCurrent->m_Next;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::DetectSlaves()
{
	CsDeereDevice	*pHead = (CsDeereDevice *) m_ProcessGlblVars->pFirstUnCfg;
	CsDeereDevice* pCurrent = pHead;
	CsDeereDevice* pMS = this;
	CsDeereDevice* pMSTable[8] = {NULL};
	char			 szText[128] = {0};
	int				 char_i = 0;
	uInt32           u32Data;

	if ( 0 != m_pCardState->u8MsCardId )
		return CS_FALSE;

	pMSTable[0] = this;			// Master candiate
	szText[char_i++] = '0';
	//Configure master tranceivers
	m_pCardState->AddOnReg.u16MsBist1 = DEERE_BIST_EN|DEERE_BIST_DIR1;
	int32 i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist1, DEERE_MS_BISTCTRL_REG1);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	m_pCardState->AddOnReg.u16MsBist2 = 0;
	i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, DEERE_MS_BISTCTRL_REG2);
	if( CS_FAILED(i32Ret) )	return i32Ret;
	
	// Detect all slave card conntected to the master
	pCurrent = pHead;
	while( NULL != pCurrent )
	{
		if(pCurrent->m_pCardState->u8MsCardId  != 0)
		{
			i32Ret = pCurrent->SetRoleMode(ermSlave);
			if( CS_FAILED(i32Ret) )	break;

			u32Data = pCurrent->ReadRegisterFromNios(pCurrent->m_pCardState->AddOnReg.u16MsBist1, DEERE_MS_BITSTSTAT_REG1);
			if(  0 == ((u32Data & DEERE_BIST_MS_SLAVE_ACK_MASK) & (1 << (pCurrent->m_pCardState->u8MsCardId-1))) )
			{
				m_pCardState->AddOnReg.u16MsBist2 = 0x7FFF;
				i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, DEERE_MS_BISTCTRL_REG2);
				if( CS_FAILED(i32Ret) )	break;
	
				u32Data = pCurrent->ReadRegisterFromNios(pCurrent->m_pCardState->AddOnReg.u16MsBist1, DEERE_MS_BITSTSTAT_REG1);
				if(  0 != ((u32Data & DEERE_BIST_MS_SLAVE_ACK_MASK) & (1 << (pCurrent->m_pCardState->u8MsCardId-1))) )
				{
					m_pCardState->AddOnReg.u16MsBist2 = 0;
					i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, DEERE_MS_BISTCTRL_REG2);
					if( CS_FAILED(i32Ret) )	break;
					
					u32Data = pCurrent->ReadRegisterFromNios(pCurrent->m_pCardState->AddOnReg.u16MsBist1, DEERE_MS_BITSTSTAT_REG1);
					if(  0 == ((u32Data & DEERE_BIST_MS_SLAVE_ACK_MASK) & (1 << (pCurrent->m_pCardState->u8MsCardId-1))) )
					{
						pMSTable[pCurrent->m_pCardState->u8MsCardId] = pCurrent;
						pCurrent = pCurrent->m_Next;
						continue;
					}
				}
				else
				{
					// A stand alone card could be a slave card with a bad contection with M/S bridge
					i32Ret = pCurrent->SetRoleMode(ermStandAlone);
					if( CS_FAILED(i32Ret) )	return i32Ret;

					m_pCardState->AddOnReg.u16MsBist2 = 0;
					i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, DEERE_MS_BISTCTRL_REG2);
					if( CS_FAILED(i32Ret) )	return i32Ret;
				}
			}
			else
			{
				// A stand alone card could be a slave card with a bad contection with M/S bridge
				i32Ret = pCurrent->SetRoleMode(ermStandAlone);
				if( CS_FAILED(i32Ret) )	return i32Ret;
			}
		}
		pCurrent = pCurrent->m_Next;
	}


	if( CS_FAILED(i32Ret) && ( NULL != pCurrent ) )
	{
		// Error on Master card. Abort M/S detection
		sprintf_s( szText, sizeof(szText), TEXT("Error on card Master (%d). M/S detection aborted\n"),i32Ret );
		::OutputDebugString(szText);
		pCurrent->SetRoleMode(ermStandAlone);
		return i32Ret;
	}

	// Re-organize slave cards
	pMS =this;
	for (int i = 1; i <= 7; i++)
	{
		if ( NULL != pMSTable[i] )
		{
			pMS->m_pNextInTheChain = pMSTable[i];
			pMS = pMSTable[i];
			m_pCardState->u8SlaveConnected |= (1 << (pMS->m_pCardState->u8MsCardId-1));

			szText[char_i++] = ',';
			szText[char_i++] = (char)('0' + i );
		}
	}

	int32 i32Status = TestMsBridge();
	if (CS_FAILED(i32Status))
	{
		CsDeereDevice* pTemp;
		// Undo master/slave
		m_pCardState->u8SlaveConnected = 0;
		pCurrent = this;
		while( pCurrent )
		{
			pCurrent->SetRoleMode(ermStandAlone);
			pTemp		= pCurrent;
			pCurrent	= pCurrent->m_pNextInTheChain;
			pTemp->m_pNextInTheChain = pTemp->m_Next;
		}
		return i32Status;
	}

	// A Master slave system should have at least one slave connected
	if ( m_pCardState->u8SlaveConnected  )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_INFO_MSDETECT, szText );
		return CS_SUCCESS;
	}
	else
		return CS_MISC_ERROR;
}


//-----------------------------------------------------------------------------
// Step 1. All 0 one bit 1 DEERE_BIST_MS_MULTIDROP_MASK
// Step 2. All 1 one bit 0
// Step 3. Clock counters
//-----------------------------------------------------------------------------


int32 CsDeereDevice::TestMsBridge()
{
	int32 i32Ret = CS_SUCCESS;

	if (! m_bTestBridgeEnabled)
		return CS_SUCCESS;

	if ( ermMaster != m_pCardState->ermRoleMode )
		return CS_FALSE;
	
	uInt16 u16(0);
	uInt32 u32Data(0);
	CsDeereDevice* pSlave(NULL);
	char szText[128] = {0};
	
	//Configure master tranceivers
	m_pCardState->AddOnReg.u16MsBist1 = DEERE_BIST_EN|DEERE_BIST_DIR2;
	WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist1, DEERE_MS_BISTCTRL_REG1);
		
	//Step 1.
	m_pCardState->AddOnReg.u16MsBist2 = 0;
	WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, DEERE_MS_BISTCTRL_REG2);
	for( u16 = DEERE_BIST_MS_MULTIDROP_FIRST; u16 <= DEERE_BIST_MS_MULTIDROP_LAST; u16 <<= 1 )
	{
		m_pCardState->AddOnReg.u16MsBist2 = u16;
		WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, DEERE_MS_BISTCTRL_REG2);
		pSlave = m_pNextInTheChain;
		while( NULL != pSlave )
		{
			u32Data = pSlave->ReadRegisterFromNios(pSlave->m_pCardState->AddOnReg.u16MsBist1, DEERE_MS_BITSTSTAT_REG1);
			if( (u32Data & DEERE_BIST_MS_MULTIDROP_MASK) != u16 )
			{
				sprintf_s( szText, sizeof(szText), TEXT("Bit 0x%x card %d low\n"), u16, pSlave->m_pCardState->u8MsCardId );
				::OutputDebugString(szText);
				CsEventLog EventLog;
				EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
				i32Ret =CS_MS_BRIDGE_FAILED;
			}
			pSlave = pSlave->m_pNextInTheChain;
		}			
	}
	//Step 2.
	m_pCardState->AddOnReg.u16MsBist2 = DEERE_BIST_MS_MULTIDROP_MASK;
	WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, DEERE_MS_BISTCTRL_REG2);
	for( u16 = DEERE_BIST_MS_MULTIDROP_FIRST; u16 <= DEERE_BIST_MS_MULTIDROP_LAST; u16 <<= 1 )
	{
		m_pCardState->AddOnReg.u16MsBist2 = (DEERE_BIST_MS_MULTIDROP_MASK & ~u16);
		WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, DEERE_MS_BISTCTRL_REG2);
		pSlave = m_pNextInTheChain;
		while( NULL != pSlave )
		{
			u32Data = pSlave->ReadRegisterFromNios(pSlave->m_pCardState->AddOnReg.u16MsBist1, DEERE_MS_BITSTSTAT_REG1);
			if( (u32Data & DEERE_BIST_MS_MULTIDROP_MASK) != (uInt32)(DEERE_BIST_MS_MULTIDROP_MASK & ~u16) )
			{
				sprintf_s( szText, sizeof(szText),  TEXT("Bit 0x%x card %d high\n"), u16, pSlave->m_pCardState->u8MsCardId );
				::OutputDebugString(szText);
				CsEventLog EventLog;
				EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
				i32Ret =CS_MS_BRIDGE_FAILED;
			}
			pSlave = pSlave->m_pNextInTheChain;
		}			
	}
	//Step 3
	uInt32 u32FpgaClkCnt(0), u32AdcClkCnt(0);
	u32Data = ReadRegisterFromNios(m_pCardState->AddOnReg.u16MsBist1, DEERE_MS_BITSTSTAT_REG2);
	u32FpgaClkCnt = u32Data & DEERE_BIST_MS_FPGA_COUNT_MASK;
	u32AdcClkCnt  = u32Data & DEERE_BIST_MS_ADC_COUNT_MASK;

	u32Data = ReadRegisterFromNios(m_pCardState->AddOnReg.u16MsBist1, DEERE_MS_BITSTSTAT_REG2);
	if( u32FpgaClkCnt == (u32Data & DEERE_BIST_MS_FPGA_COUNT_MASK) )
	{
		sprintf_s( szText, sizeof(szText), TEXT("Fpga count failed. Card %d\n"), m_pCardState->u8MsCardId );
		::OutputDebugString(szText);
		CsEventLog EventLog;
		EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );		
		i32Ret = CS_MS_BRIDGE_FAILED;
	}
	if( u32AdcClkCnt == (u32Data & DEERE_BIST_MS_ADC_COUNT_MASK) )
	{
		sprintf_s( szText, sizeof(szText), TEXT("ADC count failed. Card %d\n"), m_pCardState->u8MsCardId );
		::OutputDebugString(szText);
		CsEventLog EventLog;
		EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
		i32Ret = CS_MS_BRIDGE_FAILED;
	}


	pSlave = m_pNextInTheChain;
	while( NULL != pSlave )
	{
		u32Data = pSlave->ReadRegisterFromNios(pSlave->m_pCardState->AddOnReg.u16MsBist1, DEERE_MS_BITSTSTAT_REG2);
		u32FpgaClkCnt = u32Data & DEERE_BIST_MS_FPGA_COUNT_MASK;
		u32AdcClkCnt  = u32Data & DEERE_BIST_MS_ADC_COUNT_MASK;

		u32Data = pSlave->ReadRegisterFromNios(pSlave->m_pCardState->AddOnReg.u16MsBist1, DEERE_MS_BITSTSTAT_REG2);
		if( u32FpgaClkCnt == (u32Data & DEERE_BIST_MS_FPGA_COUNT_MASK) )
		{
			sprintf_s( szText, sizeof(szText), TEXT("Fpga count failed. Card %d\n"), pSlave->m_pCardState->u8MsCardId );
			::OutputDebugString(szText);
			CsEventLog EventLog;
			EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
			i32Ret =CS_MS_BRIDGE_FAILED;
		}
		if( u32AdcClkCnt == (u32Data & DEERE_BIST_MS_ADC_COUNT_MASK) )
		{
			sprintf_s( szText, sizeof(szText), TEXT("ADC count failed. Card %d\n"), pSlave->m_pCardState->u8MsCardId );
			::OutputDebugString(szText);
			CsEventLog EventLog;
			EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
			i32Ret = CS_MS_BRIDGE_FAILED;
		}

		pSlave = pSlave->m_pNextInTheChain;
	}

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::SendMasterSlaveSetting()
{
	uInt16 u16Data = 0;

	uInt8	u8MsMode = 0; 
	switch(m_pCardState->ermRoleMode)
	{
		default: return CS_INVALID_PARAMETER;
		case ermStandAlone:
			break;
		case ermMaster:
		case ermSlave:
		case ermLast:
			u8MsMode = m_pCardState->u8MsTriggMode;
			break;
	}
	u16Data = (u8MsMode) << 11 | (m_pCardState->u8SlaveConnected << 2);

	int32 i32Ret = WriteRegisterThruNios(u16Data, DEERE_MS_REG_WR);
	if( CS_FAILED(i32Ret) )
	{
		return i32Ret;
	}
	
	m_pCardState->AddOnReg.u16MasterSlave = u16Data;
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDeereDevice::EnableMsBist(bool bEnable)
{
	if(bEnable)
	{
		m_pCardState->AddOnReg.u16MsBist1 |= DEERE_BIST_EN;
		m_pCardState->AddOnReg.u16MasterSlave = (DEERE_DEFAULT_MSTRIGMODE) << 11 ;
	}
	else
	{
		if( ermStandAlone == m_pCardState->ermRoleMode )
			m_pCardState->AddOnReg.u16MasterSlave = 0;

		m_pCardState->AddOnReg.u16MsBist1 &=~DEERE_BIST_EN;
	}
	int32 i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MasterSlave, DEERE_MS_REG_WR);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	return WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist1, DEERE_MS_BISTCTRL_REG1);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::ResetMasterSlave()
{
	int32 i32Ret = CS_SUCCESS;

	// This function is only for master card ...
	if ( ermMaster != m_pCardState->ermRoleMode )
		return i32Ret;

	uInt16	u16Data = m_pCardState->AddOnReg.u16MasterSlave;

	u16Data |=DEERE_MS_RESET;
	i32Ret = WriteRegisterThruNios(u16Data, DEERE_MS_REG_WR);
	if( CS_FAILED(i32Ret) )
		return i32Ret;
		
	u16Data &= ~DEERE_MS_RESET;
	i32Ret = WriteRegisterThruNios(u16Data, DEERE_MS_REG_WR);

	BBtiming(1000);

	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	CsDeereDevice::CheckMasterSlaveFirmwareVersion()
{
	int32	i32Status = CS_SUCCESS;

	if ( 0 != m_pCardState->u8BadFirmware )
		return CS_INVALID_FW_VERSION;

	if( this != m_pTriggerMaster )
	{
		if ( m_pCardState->VerInfo.BbCpldInfo.u8Reg      != m_pTriggerMaster->m_pCardState->VerInfo.BbCpldInfo.u8Reg       ||
			 m_pCardState->VerInfo.BbFpgaInfo.u32Reg     != m_pTriggerMaster->m_pCardState->VerInfo.BbFpgaInfo.u32Reg      ||
			 m_pCardState->VerInfo.AddonFpgaFwInfo.u32Reg!= m_pTriggerMaster->m_pCardState->VerInfo.AddonFpgaFwInfo.u32Reg ||
			 m_pCardState->VerInfo.AnCpldFwInfo.u32Reg   != m_pTriggerMaster->m_pCardState->VerInfo.AnCpldFwInfo.u32Reg    ||
			 m_pCardState->VerInfo.TrigCpldFwInfo.u32Reg != m_pTriggerMaster->m_pCardState->VerInfo.TrigCpldFwInfo.u32Reg  ||
			 m_pCardState->VerInfo.ConfCpldInfo.u32Reg   != m_pTriggerMaster->m_pCardState->VerInfo.ConfCpldInfo.u32Reg    )
		{
			CsEventLog EventLog;
			EventLog.Report( CS_WARN_FW_DISCREPANCY, "" );
			i32Status = CS_MASTERSLAVE_DISCREPANCY;
		}

	}

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsDeereDevice::ResetMSDecimationSynch()
{
	int32 i32Ret = CS_SUCCESS;

	// This function is only for master card ...
	if ( ermMaster != m_pCardState->ermRoleMode )
		return i32Ret;

	uInt16	u16Data = m_pCardState->AddOnReg.u16MasterSlave;

	u16Data |= DEERE_MS_DECSYNCH_RESET;
	i32Ret = WriteRegisterThruNios(u16Data, DEERE_MS_REG_WR);
	if( CS_FAILED(i32Ret) )
		return i32Ret;
		
	u16Data &= ~DEERE_MS_DECSYNCH_RESET;
	i32Ret = WriteRegisterThruNios(u16Data, DEERE_MS_REG_WR);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	BBtiming(1000);

	m_MasterIndependent->m_bMsDecimationReset = false;
	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDeereDevice::MsInitializeDevIoCtl()
{
	int32		 i32Status = CS_SUCCESS;
	CsDeereDevice* pDevice = m_MasterIndependent;

	while ( pDevice )
	{
		// Open handle to kernel driver
		i32Status = pDevice->InitializeDevIoCtl();

#if !defined( USE_SHARED_CARDSTATE )
		i32Status = pDevice->IoctlGetCardState(pDevice->m_pCardState);
#endif
		if ( CS_FAILED(i32Status) )
			break;

		pDevice = pDevice->m_Next;
	}

	if ( CS_FAILED(i32Status) )
	{
		pDevice = m_MasterIndependent;
		while ( pDevice )
		{
			// Close handle to kernel driver
			pDevice->UnInitializeDevIoCtl();
			pDevice = pDevice->m_Next;
		}
	}

	return i32Status;
}