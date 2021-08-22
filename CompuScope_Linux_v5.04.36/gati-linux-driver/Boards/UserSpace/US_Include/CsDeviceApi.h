// CsDeviceApi.h
//

#include "CsTypes.h"
#include "CsPrivateStruct.h"
#include "CsDriverTypes.h"
#include "CsIoctlTypes.h"


int32	DeviceGetAcqSystemCount( PVOID pDevice, BOOL bForceInd, uInt16 *pu16SystemFound );
int32	DeviceAcqSystenInitialize( PVOID pDevice, uInt32 u32InitFlag );
void	DeviceAcqSystenCleanup( PVOID pDevice );
uInt32	DeviceGetAcqStatus( PVOID pDevice);
int32	DeviceGetParams( PVOID pDevice, uInt32 u32ParamId, PVOID pOutBuffer, uInt32 u32OutSize = 0 );
int32	DeviceSetParams( PVOID pDevice, uInt32 u32ParamId, PVOID pInBuffer, uInt32 u32InSize  = 0 );
int32	DeviceStartAcquisition( PVOID pDevice );
int32	DeviceAbortAcquisition( PVOID pDevice );
void	DeviceForceTrigger( PVOID pDevice );
int32	DeviceForceCalibration( PVOID pDevice );
void	DeviceResetSystem( PVOID pDevice, BOOL bHardReset = FALSE );
int32	DeviceGetBoardsInfo( PVOID pDevice, PARRAY_BOARDINFO pABoardInfo);
int32	DeviceRegisterEventHandle( PVOID pDevice,in_REGISTER_EVENT_HANDLE Temp );
int32	DeviceValidateAcquisitionConfig( PVOID pDevice, PCSACQUISITIONCONFIG pAcqCfg, uInt32 u32Coerce );
int32	DeviceAutoUpdateFirmware( PVOID pDevice );
int32	DeviceTransferData( PVOID pDevice, in_DATA_TRANSFER *InDataXfer );
int32	DeviceTransferDataEx( PVOID pDevice, io_DATA_TRANSFER_EX *InDataXfer );
int32	DeviceGetAsynTransDataResult( PVOID pDevice, uInt16 u16Channel,
										   CSTRANSFERDATARESULT *pTxDataResult,
                                           BOOL bWait );
PVOID	DeviceGetCardPointerFromNativeIndex( PVOID pDevice, uInt8 u8CardIndex );
void	DeviceBuildCompuScopeSystemLinks();
int32	AllocateProcessDeviceCardList( uInt32 u32CardCount, PCSDEVICE_INFO pDevInfo );
void	DeleteProcessDeviceCardList();

void	CreateKernelLinkDeviceList( PVOID pDevice );
void	DestroyKernelLinkDeviceList( PVOID pDevice );
void	CreateKernelLinkAcqSystem( PVOID pDevice );
void	DestroyKernelLinkAcqSystem( PVOID pDevice );
