

#include "StdAfx.h"
#include "CsSpider.h"
#include "CsEventLog.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#endif
#include "CsPrivatePrototypes.h"
#include "CsIoctl.h"
#include "CsNucleonBaseFlash.h"



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsSpiderDev::DetectMasterSlave(int16 *i16NumMasterSlaveSystems, int16 *i16NumStandAloneSystems)
{
	CsSpiderDev* pHead	= (CsSpiderDev *) m_ProcessGlblVars->pFirstUnCfg;
	CsSpiderDev* pCurrent = pHead;
	bool	         bFoundMaster = false;

	uInt32	u32Data;

	*i16NumStandAloneSystems = 0;
	*i16NumMasterSlaveSystems = 0;

	// First pass. Search for master cards
	while( NULL != pCurrent )
	{
		// Skip all boards that have errors on Nios init or AddOn connection
		if( ! (pCurrent->BoardInitOK() && pCurrent->m_bNucleon) )
		{
			pCurrent = pCurrent->m_Next;
			continue;
		}

		// Search for Master card
		u32Data = pCurrent->ReadRegisterFromNios( 0, CSPDR_MS_STATUS_RD );
		u32Data = (u32Data >>11) & 0x7;
		int32 i32Status = pCurrent->WriteRegisterThruNios( u32Data, CSPDR_MSCTRL_REG_WR );
		if ( CS_SUCCEEDED(i32Status ) )
		{
			pCurrent->m_pCardState->u8MsCardId = (uInt8) u32Data;

			if ( 0 == pCurrent->m_pCardState->u8MsCardId )
			{
				bFoundMaster = true;
				pCurrent->m_pCardState->ermRoleMode = ermMaster;
			}
		}
		pCurrent = pCurrent->m_Next;
	}
	// If Master card not found, it is not worth to go further
	// All cards will behave as stand alone cards
	if ( !bFoundMaster )
		goto DetectMasterSlaveExit;

	// Second pass. Enable MS BIST
	pCurrent = pHead;
	while( NULL != pCurrent )
	{
		// Skip all boards that have errors on Nios init or AddOn connection
		if( ! (pCurrent->BoardInitOK() && pCurrent->m_bNucleon) )
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
		// Skip all boards that have errors on Nios init or AddOn connection
		if( ! (pCurrent->BoardInitOK() && pCurrent->m_bNucleon) )
		{
			pCurrent = pCurrent->m_Next;
			continue;
		}

		if( ermMaster == pCurrent->m_pCardState->ermRoleMode  )
		{
			if( CS_SUCCEEDED(pCurrent->DetectSlaves()) )
				(*i16NumMasterSlaveSystems)++;
			else
				pCurrent->m_pCardState->ermRoleMode = ermStandAlone;
		}
		pCurrent = pCurrent->m_Next;
	}

DetectMasterSlaveExit:
	// Third pass. Count remaining stand alone cards
	// and disable the bist
	pCurrent = pHead;
	while( NULL != pCurrent )
	{
		// Skip all boards that have errors on Nios init or AddOn connection
		if( ! (pCurrent->BoardInitOK() && pCurrent->m_bNucleon) )
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

int32	CsSpiderDev::DetectSlaves()
{
	CsSpiderDev* pHead	= (CsSpiderDev *) m_ProcessGlblVars->pFirstUnCfg;
	CsSpiderDev* pCurrent = pHead;
	CsSpiderDev* pMS = this;
	CsSpiderDev* pMSTable[8] = {NULL};
	char			 szText[128] = {0};
	int				 char_i = 0;
	uInt32           u32Data;

	if ( 0 != m_pCardState->u8MsCardId )
		return CS_FALSE;

	pMSTable[0] = this;			// Master candiate

#ifdef _DEBUG
	sprintf_s( szText, sizeof (szText), "Found slave %d", m_pCardState->u8MsCardId);
	OutputDebugString(szText);
#endif

	//Configure master tranceivers
	m_pCardState->AddOnReg.u16MsBist1 = CSPDR_BIST_EN|CSPDR_BIST_DIR1;
	int32 i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist1, CSPDR_MS_BIST_CTL1_WR);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	m_pCardState->AddOnReg.u16MsBist2 = 0;
	i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSPDR_MS_BIST_CTL2_WR);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Detect all slave cards conntected to the master
	pCurrent = pHead;
	while( NULL != pCurrent )
	{
		if( ( pCurrent->m_DeviceId == pMS->m_DeviceId ) && pCurrent->m_pCardState->u8MsCardId != 0 )
		{
			u32Data = pCurrent->ReadRegisterFromNios(pCurrent->m_pCardState->AddOnReg.u16MsBist1, CSPDR_MS_BIST_ST1_RD);
			if(  0 == ((u32Data & CSPDR_BIST_MS_SLAVE_ACK_MASK) & (1 << (pCurrent->m_pCardState->u8MsCardId-1))) )
			{
				m_pCardState->AddOnReg.u16MsBist2 = 0x7FFF;
				i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSPDR_MS_BIST_CTL2_WR);
				if( CS_FAILED(i32Ret) )
					return i32Ret;

				u32Data = pCurrent->ReadRegisterFromNios(pCurrent->m_pCardState->AddOnReg.u16MsBist1, CSPDR_MS_BIST_ST1_RD);
				if(  0 != ((u32Data & CSPDR_BIST_MS_SLAVE_ACK_MASK) & (1 << (pCurrent->m_pCardState->u8MsCardId-1))) )
				{
					m_pCardState->AddOnReg.u16MsBist2 = 0;
					i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSPDR_MS_BIST_CTL2_WR);
					if( CS_FAILED(i32Ret) )
						return i32Ret;

					u32Data = pCurrent->ReadRegisterFromNios(pCurrent->m_pCardState->AddOnReg.u16MsBist1, CSPDR_MS_BIST_ST1_RD);
					if(  0 == ((u32Data & CSPDR_BIST_MS_SLAVE_ACK_MASK) & (1 << (pCurrent->m_pCardState->u8MsCardId-1))) )
					{
						pCurrent->m_pCardState->ermRoleMode = ermSlave;
						pMSTable[pCurrent->m_pCardState->u8MsCardId] = pCurrent;
						pCurrent = pCurrent->m_Next;
						continue;
					}
#ifdef _DEBUG
					else
						OutputDebugString("   Test of CSPDR_MS_BIST_ST1_RD  u16MsBist2 = 0 (Step1) -  FAILED");
				}
				else
				{
					OutputDebugString("   Test of CSPDR_MS_BIST_ST1_RD  u16MsBist2 = 0x7FFF -  FAILED");
				}
			}
			else
				OutputDebugString("   Test of CSPDR_MS_BIST_ST1_RD  u16MsBist2 = 0 (Step2) -  FAILED");
#else
				}
			}
#endif
		}
		pCurrent = pCurrent->m_Next;
	}

	// Re-organize slave cards
	szText[char_i++] = '0';
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
		CsSpiderDev* pTemp;
		// Undo master/slave
		m_pCardState->u8SlaveConnected = 0;
		pCurrent = this;
		while( pCurrent )
		{
			pCurrent->m_pCardState->ermRoleMode = ermStandAlone;
			pTemp		= pCurrent;
			pCurrent	= pCurrent->m_pNextInTheChain;
			pTemp->m_pNextInTheChain = pTemp->m_Next;
		}

#ifdef _DEBUG
		OutputDebugString("   TestMsBridge() -  FAILED");
#endif

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
//
//-----------------------------------------------------------------------------

int32	CsSpiderDev::SendMasterSlaveSetting()
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
			u8MsMode = CSPDR_DEFAULT_MSTRIGMODE;
			break;
	}
	u16Data = (u8MsMode) << 11 | (m_pCardState->u8SlaveConnected << 2);

	int32 i32Ret = WriteRegisterThruNios(u16Data, CSPDR_MS_REG_WR);
	if( CS_SUCCEEDED(i32Ret) )
		m_pCardState->AddOnReg.u16MasterSlave = u16Data;
	
	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsSpiderDev::EnableMsBist(bool bEnable)
{
	if(bEnable)
	{
		m_pCardState->AddOnReg.u16MsBist1 |= CSPDR_BIST_EN;
		m_pCardState->AddOnReg.u16MasterSlave = (CSPDR_DEFAULT_MSTRIGMODE) << 11 ;
	}
	else
	{
		if( ermStandAlone == m_pCardState->ermRoleMode )
			m_pCardState->AddOnReg.u16MasterSlave = 0;

		m_pCardState->AddOnReg.u16MsBist1 &=~CSPDR_BIST_EN;
	}
	int32 i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MasterSlave, CSPDR_MS_REG_WR);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	return WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist1, CSPDR_MS_BIST_CTL1_WR);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsSpiderDev::SetMsTriggerEnable(bool bEnable)
{
	if( ! bEnable)
		m_pCardState->AddOnReg.u16MasterSlave = 0;

	int32 i32Ret = WriteRegisterThruNios(m_pCardState->AddOnReg.u16MasterSlave, CSPDR_MS_REG_WR);
	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsSpiderDev::ResetMasterSlave(bool bForce)
{
	int32 i32Ret = CS_SUCCESS;

	// This function is only for master card ...
	if ( ermMaster != m_pCardState->ermRoleMode )
		return i32Ret;

	if ( !bForce && !m_bMsResetNeeded )
		return i32Ret;

	uInt16	u16Data = m_pCardState->AddOnReg.u16MasterSlave;

	u16Data |= CSPDR_MS_RESET;
	i32Ret = WriteRegisterThruNios(u16Data, CSPDR_MS_REG_WR);
	if( CS_FAILED(i32Ret) )
		return i32Ret;
	BBtiming(3000);

	u16Data &= ~CSPDR_MS_RESET;
	i32Ret = WriteRegisterThruNios(u16Data, CSPDR_MS_REG_WR);
	BBtiming(3000);

	m_bMsResetNeeded = false;

	return i32Ret;
}


//-----------------------------------------------------------------------------
// Step 1. All 0 one bit 1 CSPDR_BIST_MS_MULTIDROP_MASK
// Step 2. All 1 one bit 0
// Step 3. Clock counters
//-----------------------------------------------------------------------------


int32 CsSpiderDev::TestMsBridge()
{
	int32 i32Ret = CS_SUCCESS;
	if ( ermMaster != m_pCardState->ermRoleMode )
		return CS_FALSE;

	uInt16 u16(0);
	uInt32 u32Data(0);
	CsSpiderDev* pSlave(NULL);
	char szText[128] = {0};

	//Configure master tranceivers
	m_pCardState->AddOnReg.u16MsBist1 = CSPDR_BIST_EN|CSPDR_BIST_DIR2;
	WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist1, CSPDR_MS_BIST_CTL1_WR);

	//Step 1.
	m_pCardState->AddOnReg.u16MsBist2 = 0;
	WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSPDR_MS_BIST_CTL2_WR);
	for( u16 = CSPDR_BIST_MS_MULTIDROP_FIRST; u16 <= CSPDR_BIST_MS_MULTIDROP_LAST; u16 <<= 1 )
	{
		m_pCardState->AddOnReg.u16MsBist2 = u16;
		WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSPDR_MS_BIST_CTL2_WR);
		pSlave = m_pNextInTheChain;
		while( NULL != pSlave )
		{
			u32Data = pSlave->ReadRegisterFromNios(pSlave->m_pCardState->AddOnReg.u16MsBist1, CSPDR_MS_BIST_ST1_RD);
			if( (u32Data & CSPDR_BIST_MS_MULTIDROP_MASK) != u16 )
			{
				sprintf_s( szText, sizeof(szText), TEXT("Bit 0x%x card %d low"), u16, pSlave->m_pCardState->u8MsCardId );
				CsEventLog EventLog;
				EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
				i32Ret =CS_MS_BRIDGE_FAILED;
			}
			pSlave = pSlave->m_pNextInTheChain;
		}
	}
	//Step 2.
	m_pCardState->AddOnReg.u16MsBist2 = CSPDR_BIST_MS_MULTIDROP_MASK;
	WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSPDR_MS_BIST_CTL2_WR);
	for( u16 = CSPDR_BIST_MS_MULTIDROP_FIRST; u16 <= CSPDR_BIST_MS_MULTIDROP_LAST; u16 <<= 1 )
	{
		m_pCardState->AddOnReg.u16MsBist2 = (CSPDR_BIST_MS_MULTIDROP_MASK & ~u16);
		WriteRegisterThruNios(m_pCardState->AddOnReg.u16MsBist2, CSPDR_MS_BIST_CTL2_WR);
		pSlave = m_pNextInTheChain;
		while( NULL != pSlave )
		{
			u32Data = pSlave->ReadRegisterFromNios(pSlave->m_pCardState->AddOnReg.u16MsBist1, CSPDR_MS_BIST_ST1_RD);
			if( (u32Data & CSPDR_BIST_MS_MULTIDROP_MASK) != (uInt32)(CSPDR_BIST_MS_MULTIDROP_MASK & ~u16) )
			{
				sprintf_s( szText, sizeof(szText), TEXT("Bit 0x%x card %d high"), u16, pSlave->m_pCardState->u8MsCardId );
				CsEventLog EventLog;
				EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
				i32Ret =CS_MS_BRIDGE_FAILED;
			}
			pSlave = pSlave->m_pNextInTheChain;
		}
	}
	// Step 3
	// Validation for FPGA count and ADC count
	pSlave = this;
	while( NULL != pSlave )
	{
		i32Ret = pSlave->TestFpgaCounter();
		if ( CS_FAILED(i32Ret) )
			break;

		pSlave = pSlave->m_pNextInTheChain;
	}

	return i32Ret;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsSpiderDev::TestFpgaCounter()
{
	int32	i32Ret = CS_SUCCESS;
	char	szText[128] = {0};
	uInt32	u32Data = ReadRegisterFromNios(m_pCardState->AddOnReg.u16MsBist1, CSPDR_MS_BIST_ST2_RD);
	uInt32	u32FpgaClkCnt = u32Data & CSPDR_BIST_MS_FPGA_COUNT_MASK;
	uInt32	u32AdcClkCnt  = u32Data & CSPDR_BIST_MS_ADC_COUNT_MASK;

	BOOL bPassed = FALSE;

	// Some of spider boatds have slow clock (10MS/s). With these board, we have to read the register many times to detect the FPGA counter changed
	for ( int i = 0; i < 200; i++ )
	{
		u32Data = ReadRegisterFromNios(m_pCardState->AddOnReg.u16MsBist1, CSPDR_MS_BIST_ST2_RD);
		if ( u32FpgaClkCnt != (u32Data & CSPDR_BIST_MS_FPGA_COUNT_MASK) && u32AdcClkCnt != (u32Data & CSPDR_BIST_MS_ADC_COUNT_MASK))
		{
			bPassed = TRUE;
			break;
		}
	}

	if ( ! bPassed )
	{
		if( u32FpgaClkCnt == (u32Data & CSPDR_BIST_MS_FPGA_COUNT_MASK) )
		{
			sprintf_s( szText, sizeof(szText), TEXT("Fpga count failed. Card %d"), m_pCardState->u8MsCardId );
			CsEventLog EventLog;
			EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
		}

		if( u32AdcClkCnt == (u32Data & CSPDR_BIST_MS_ADC_COUNT_MASK) )
		{
			sprintf_s( szText, sizeof(szText), TEXT("ADC count failed. Card %d"), m_pCardState->u8MsCardId );
			CsEventLog EventLog;
			EventLog.Report( CS_MS_BRIDGE_TEST_FAILED, szText );
		}
		i32Ret = CS_MS_BRIDGE_FAILED;
	}

	return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt16 CsSpiderDev::BuildMasterSlaveSystem( CsSpiderDev** ppFirstCard )
{
	CsSpiderDev*	pCurUnCfg	= (CsSpiderDev*) m_ProcessGlblVars->pFirstUnCfg;
	CsSpiderDev*	pMaster = NULL;

	//Look for the Master
	while( NULL != pCurUnCfg )
	{
		//if ( ermMaster == pCurUnCfg->m_pCardState->ermRoleMode )
		if (  0 == pCurUnCfg->m_pCardState->u8MsCardId )
		{
			pMaster = pCurUnCfg;
			break;
		}

		pCurUnCfg = pCurUnCfg->m_Next;
	}

	if( NULL == pMaster )
	{
		*ppFirstCard  = NULL;
		return 0;
	}

	uInt16 u16CardInSystem = 0;

	*ppFirstCard  = pMaster;
	CsSpiderDev* pCard = pMaster;

	LONGLONG llMaxMem = pMaster->m_PlxData->CsBoard.i64MaxMemory;

	while ( pCard )
	{
		//remove card from unconfigured list
		pCard->RemoveDeviceFromUnCfgList();

		pCard->m_pTriggerMaster = pMaster;
		pCard->m_Next = pCard->m_pNextInTheChain;
		pCard->m_MasterIndependent = pMaster;

		pCard->m_pCardState->u16CardNumber = ++u16CardInSystem;

		llMaxMem = llMaxMem < pCard->m_PlxData->CsBoard.i64MaxMemory ? llMaxMem : pCard->m_PlxData->CsBoard.i64MaxMemory;

		pCard = pCard->m_pNextInTheChain;
	}

	if( 0 == llMaxMem )
	{
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_TEXT, "Zero memory" );
		return 0;
	}

	bool bRemoveSystem = false;

	// Check for Master/|Slave discrepancy
	pCard = pMaster;
	while ( pCard )
	{
		if( pMaster->m_PlxData->CsBoard.NbAnalogChannel != pCard->m_PlxData->CsBoard.NbAnalogChannel ||
		    pMaster->m_PlxData->CsBoard.u32BoardType != pCard->m_PlxData->CsBoard.u32BoardType )
		{
			char szErrorStr[128];
			sprintf_s( szErrorStr, sizeof(szErrorStr), "M %d ch. %d MS/s - S%d %d ch. %d MS/s",
					pMaster->m_PlxData->CsBoard.NbAnalogChannel, int(pMaster->m_pCardState->SrInfo[0].llSampleRate/1000000),
					pCard->m_pCardState->u8MsCardId, pCard->m_PlxData->CsBoard.NbAnalogChannel, int(pCard->m_pCardState->SrInfo[0].llSampleRate/1000000));

			::OutputDebugString(szErrorStr);
			CsEventLog EventLog;
			EventLog.Report( CS_ERROR_TEXT, "Master/Slave descrepancy." );
			bRemoveSystem = true;
			break;
		}

		pCard = pCard->m_pNextInTheChain;
	}

	if( bRemoveSystem )
		return 0;
	else
	{
		MASTER_SLAVE_LINK_INFO	SlaveInfoArray[20] = {0};
		uInt16					u16SlaveCount = 0;

		pCard = pMaster;
		while ( pCard )
		{
			pCard->m_PlxData->CsBoard.i64MaxMemory = llMaxMem;
			pCard->UpdateCardState();

			// Build Master/Slave card info to be sent to kernel driver
			if ( pCard != pCard->m_MasterIndependent )
			{
				SlaveInfoArray[u16SlaveCount].CardHandle	= pCard->m_CardHandle;
				SlaveInfoArray[u16SlaveCount++].u8MsCardId	= pCard->m_pCardState->u8MsCardId;
			}
			pCard = pCard->m_pNextInTheChain;
		}

		// Send Master/Slave link info to kernel driver
		pMaster->IoctlSetMasterSlaveLink( SlaveInfoArray, u16SlaveCount );

		char	szText[128];
		sprintf_s( szText, sizeof(szText), TEXT("Octopus Master/Slave system of %d boards initialized"), u16CardInSystem);
		CsEventLog EventLog;
		EventLog.Report( CS_INFO_TEXT, szText );
	}

	return u16CardInSystem;
}

