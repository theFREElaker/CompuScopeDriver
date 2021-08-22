//

#include "StdAfx.h"
#include "CsDeviceApi.h"
#include "CurrentDriver.h"
#include "DllStruct.h"

extern	SHAREDDLLMEM Globals;
extern	GLOBAL_PERPROCESS g_GlobalPerProcess;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 DeviceGetAcqSystemCount( PVOID pDevice, BOOL bForceInd, uInt16 *pu16SystemFound )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	return pDev->CsGetAcquisitionSystemCount( bForceInd, pu16SystemFound );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	DeviceAcqSystenInitialize( PVOID pDevice, uInt32 u32InitFlag )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	return pDev->CsAcquisitionSystemInitialize( u32InitFlag );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	DeviceAcqSystenCleanup( PVOID pDevice )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	pDev->CsAcquisitionSystemCleanup();
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32	DeviceGetAcqStatus( PVOID pDevice )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	return pDev->CsGetAcquisitionStatus();
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	DeviceGetParams( PVOID pDevice, uInt32 u32ParamId, PVOID pOutBuffer, uInt32 u32OutSize )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	return pDev->CsGetParams( u32ParamId, pOutBuffer, u32OutSize );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	DeviceSetParams( PVOID pDevice, uInt32 u32ParamId, PVOID pInBuffer, uInt32 u32InSize )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	return pDev->CsSetParams( u32ParamId, pInBuffer, u32InSize );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	DeviceStartAcquisition( PVOID pDevice )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	return pDev->CsStartAcquisition();
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	DeviceAbortAcquisition( PVOID pDevice )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	return pDev->CsAbort();
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	DeviceForceTrigger( PVOID pDevice )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	pDev->CsForceTrigger();
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	DeviceResetSystem( PVOID pDevice, BOOL bHardReset )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	pDev->CsSystemReset(bHardReset);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	DeviceForceCalibration( PVOID pDevice )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	return pDev->CsForceCalibration();
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	DeviceGetBoardsInfo( PVOID pDevice, PARRAY_BOARDINFO pABoardInfo)
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	return pDev->CsGetBoardsInfo( pABoardInfo );
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	DeviceRegisterEventHandle( PVOID pDevice,in_REGISTER_EVENT_HANDLE Temp )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	return pDev->CsRegisterEventHandle( Temp );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	DeviceValidateAcquisitionConfig( PVOID pDevice, PCSACQUISITIONCONFIG pAcqCfg, uInt32 u32Coerce )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	return pDev->CsValidateAcquisitionConfig( pAcqCfg, u32Coerce );
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	DeviceAutoUpdateFirmware( PVOID pDevice )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	return pDev->CsAutoUpdateFirmware();
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	CreateKernelLinkDeviceList( PVOID pDevice )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	while( pDev )
	{
		// Open a link to kernel driver
		pDev->InitializeDevIoCtl();
		pDev = pDev->m_HWONext;
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	DestroyKernelLinkDeviceList( PVOID pDevice )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	while( pDev )
	{
		// Close the link to kernel driver
		pDev->UnInitializeDevIoCtl();
		pDev = pDev->m_HWONext;
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	DeviceTransferData( PVOID pDevice, in_DATA_TRANSFER *InDataXfer )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	return pDev->CsDataTransfer( InDataXfer );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	DeviceTransferDataEx( PVOID pDevice, io_DATA_TRANSFER_EX *InDataXfer )
{
#if defined (_FASTBALL_DRV_) || defined (_JDEERE_DRV_)

	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;
	
	return pDev->CsDataTransferEx( InDataXfer );

#else
	UNREFERENCED_PARAMETER(pDevice);
	UNREFERENCED_PARAMETER(InDataXfer);
	return CS_FUNCTION_NOT_SUPPORTED;

#endif
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	DeviceGetAsynTransDataResult( PVOID pDevice, uInt16 u16Channel,
										   CSTRANSFERDATARESULT *pTxDataResult,
                                           BOOL bWait )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	return pDev->CsGetAsyncDataTransferResult( u16Channel, pTxDataResult, bWait );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

PVOID	DeviceGetCardPointerFromNativeIndex( PVOID pDevice, uInt8 u8CardIndex )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	while ( pDev )
	{
		if ( pDev->m_u8CardIndex == u8CardIndex )
			break;

		pDev = pDev->m_HWONext;
	}

	return pDev;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void DeviceBuildCompuScopeSystemLinks()
{
	CSDEVICE_DRVDLL	*pDevice = NULL;
	CSDEVICE_DRVDLL	*pLastCard = NULL;
	CSDEVICE_DRVDLL	*pFirstCard = NULL;
	uInt8			*pu8CardIndexList = NULL;

	for (uInt32 i = 0; i <  Globals.it()->g_StaticLookupTable.u16SystemCount; i ++ )
	{
		pu8CardIndexList = (uInt8 *) &Globals.it()->g_StaticLookupTable.DrvSystem[i].u8CardIndexList;

		for (uInt32 j = 0; j < Globals.it()->g_StaticLookupTable.DrvSystem[i].SysInfo.u32BoardCount; j ++)
		{		
			pDevice = (CSDEVICE_DRVDLL *) g_GlobalPerProcess.hNativeFirstCard;
			while ( pDevice )
			{
				if ( pDevice->m_u8CardIndex == pu8CardIndexList[j] )
				{
					pDevice->m_pSystem = &Globals.it()->g_StaticLookupTable.DrvSystem[i];
					pDevice->m_Next = NULL;
					if ( 0 == j )
					{
						pFirstCard = pDevice;
						pDevice->m_MasterIndependent = pDevice;
						pDevice->m_pTriggerMaster  = pDevice;
					}
					else 
					{
						pDevice->m_MasterIndependent = pFirstCard;
						pDevice->m_pTriggerMaster  = pFirstCard;
						pLastCard->m_Next = pDevice;
					}
					pLastCard = pDevice;
					break;
				}
				pDevice = pDevice->m_HWONext;
			}
		}
	}

	g_GlobalPerProcess.NbSystem = Globals.it()->g_StaticLookupTable.u16SystemCount;

}

//-----------------------------------------------------------------------------
// Allocate card object for the current process then inititalize HW
//-----------------------------------------------------------------------------

int32	AllocateProcessDeviceCardList( uInt32 u32CardCount, PCSDEVICE_INFO pDevInfo )
{
	int32			i32Status = CS_SUCCESS;
	CSDEVICE_DRVDLL	*pLastFoundDev = NULL;
	CSDEVICE_DRVDLL	*pDevice = NULL;
	uInt32			i = 0;
	uInt8			u8CardIndex = 1;

	for (i = 0; i < u32CardCount; i ++ )
	{
		// Create SW Object for every board found 
		pDevice = (CSDEVICE_DRVDLL *) new CSDEVICE_DRVDLL(u8CardIndex, &pDevInfo[i], &g_GlobalPerProcess);
		if ( ! pDevice )
			break;

		if ( pLastFoundDev )
			pLastFoundDev->m_HWONext = pDevice;

		pDevice->m_CardHandle = pDevInfo[i].DrvHdl;

		// Open a link to kernel driver
		i32Status = pDevice->InitializeDevIoCtl();
		// Intialize the card
		if ( CS_SUCCEEDED(i32Status) )
			i32Status = pDevice->DeviceInitialize();

		if ( CS_FAILED(i32Status) )
			break;

		pLastFoundDev = pDevice;

		if ( 0 == i )
			g_GlobalPerProcess.hNativeFirstCard = pDevice;

		u8CardIndex++;
	}
	
	// Destroy all links to kernel driver
	DestroyKernelLinkDeviceList(g_GlobalPerProcess.hNativeFirstCard);

	if ( i < u32CardCount )
	{
		// Some of device object has not been created properly
		DeleteProcessDeviceCardList();
		i32Status = CS_MEMORY_ERROR;		
	}
	else
	{
		g_GlobalPerProcess.pFirstUnCfg	= g_GlobalPerProcess.hNativeFirstCard;
		g_GlobalPerProcess.pLastUnCfg	= pDevice;
	}
	
	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	DeleteProcessDeviceCardList()
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) g_GlobalPerProcess.hNativeFirstCard;
	CSDEVICE_DRVDLL *pDeleteDev;

	while( pDev )
	{
		pDeleteDev = pDev;
		pDev = pDev->m_HWONext;

		delete pDeleteDev;		
	}

	g_GlobalPerProcess.hNativeFirstCard =
	g_GlobalPerProcess.pFirstUnCfg		=
	g_GlobalPerProcess.pLastUnCfg		= NULL;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	CreateKernelLinkAcqSystem ( PVOID pDevice )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	while( pDev )
	{
		// Open a link to kernel driver
		pDev->InitializeDevIoCtl();
		pDev = pDev->m_Next;
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	DestroyKernelLinkAcqSystem( PVOID pDevice )
{
	CSDEVICE_DRVDLL *pDev = (CSDEVICE_DRVDLL *) pDevice;

	while( pDev )
	{
		// Close the link to kernel driver
		pDev->UnInitializeDevIoCtl();
		pDev = pDev->m_Next;
	}
}
