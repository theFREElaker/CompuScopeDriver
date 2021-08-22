/////////////////////////////////////////////////////////////
// CsDevIoCtl.h
/////////////////////////////////////////////////////////////


#ifndef _CSDEVIOCTL_H_
#define _CSDEVIOCTL_H_

#include "CsDefines.h"
#include "CsTypes.h"
#include "CsDriverTypes.h"
#include "CsIoctlTypes.h"
#include "CsErrors.h"
#include "CsCardState.h"

#ifdef __linux__

#define	DEV_IOCTL_ERROR		-1

#else

#define	DEV_IOCTL_ERROR		0

#endif

#define			MAX_OVERLAPPED		10

class CsDevIoCtl;

typedef struct _CS_OVERLAPPED
{
	OVERLAPPED		OverlapStruct;
	BOOL			bBusy;

#ifdef _WINDOWS_
	HANDLE			hDriver;
	HANDLE			hUserNotifyevent;
	PVOID			pUserBuffer;
	uInt32			u32UserBufferSize;
	uInt32			u32CurrentTransferSize;
	int64			i64CompletedTransferSize;	// Driver may have to devide a huge transfer into many smaller transfer 
												// This variable keeps track how  much of data has been transfered
	uInt32			u32TransferRequest;			// Number of Transfer request expected
	int64			i64OriginalBufferSize;		// The original buffer size from application

	HANDLE			hThrWaitAsyncTxComplete;	// Handle of the thread when a big Async transfer is divided by seires of
												// smaller transfers by DLL.
#endif

} CS_OVERLAPPED, *PCS_OVERLAPPED;


class CsDevIoCtl
{
private:
	FILE_HANDLE		m_hDriver;				// DeviceIoControl Handle
	FILE_HANDLE		m_hAsyncDriver;			// DeviceIoControl OVERLAPPED Handle
	ULONG			m_BytesRead;
	DRVHANDLE		m_BoardHandle;			// Board Handle
	CSDEVICE_INFO	m_DevInfo;				// Device info: symbolic link, card handle
	bool			m_bAsyncReadMemory;		// Asynchronous Read 
	bool			m_bAbortReadMemory;
	uInt32			m_u32TransferCount;		// Number of Transfer request sent to kernel driver
	PCS_OVERLAPPED	m_pCurrentOverlaped;

#ifdef _WINDOWS_
	EVENT_HANDLE	m_hAsyncTxEvent;			// Internal Event for Async Data Transfer
#endif

	TRANSFER_DMA_BUFFER_OUTPUT	m_StmTransferOut;
	CS_OVERLAPPED	m_Overlaped[MAX_OVERLAPPED];

	PCS_OVERLAPPED GetFreeOverlapped();
	void SetFreeOverlapped( PCS_OVERLAPPED pCsOverlap = NULL );
	int32   GenericWriteRegister( uInt32 u32IoCtlCode, uInt32 u32Offset, uInt32 u32RegVal);
	uInt32  GenericReadRegister(uInt32 u32IoCtlCode, uInt32 u32Offset);
   void*       m_MapAddr;
	ULONG			m_MapLength;

public:
	uInt32			m_u32Counter;			// Debug Ioctl counter
	CsDevIoCtl();
	~CsDevIoCtl();
	CsDevIoCtl *GetDevIoCtlObj() { return this; };
	FILE_HANDLE	GetIoctlHandle() { return m_hDriver; };

	int32	ReadPciConfigSpace(PVOID pBuffer, uInt32 u32ByteCount, uInt32 u32Offset );
	int32	WritePciConfigSpace( PVOID pBuffer, uInt32 u32ByteCount, uInt32 u32Offset );

	int32	WritePciRegister(uInt32 u32Offset, uInt32 u32RegVal);
	uInt32  ReadPciRegister(uInt32 u32Offset);
	int32	WriteGageRegister(uInt32 u32Offset, uInt32 u32RegVal);
	uInt32  ReadGageRegister(uInt32 u32Offset);
	int32	WriteFlashRegister(uInt32 u32Offset, uInt32 u32RegVal);
	uInt32  ReadFlashRegister(uInt32 u32Offset);
	int32	WriteGIO(uInt32 u32Offset, uInt32 u32RegVal);
	uInt32  ReadGIO(uInt32 u32Offset);
	int32	WriteGioCpld(uInt32 u32Offset, uInt32 u32RegVal);
	uInt32  ReadGioCpld(uInt32 u32Offset);
	int32	WriteCpldRegister(uInt32 u32Offset, uInt32 u32RegVal);
	uInt32  ReadCpldRegister(uInt32 u32Offset);

	int32	WriteByteToFlash(uInt32 u32Offset, uInt32 u32RegVal);
	uInt32  ReadByteFromFlash(uInt32 u32Offset);
	int32	AddonWriteByteToFlash(uInt32 u32Offset, uInt32 u32RegVal);
	uInt32	AddonReadByteFromFlash(uInt32 u32Offset);

	int32	WriteRegisterThruNios(uInt32 u32Data, uInt32 u32Command, int32 i32Timeout_100us = -1, uInt32 *pu32NiosReturn = 0);
	uInt32	ReadRegisterFromNios(uInt32 Data, uInt32 u32Command, int32 i32Timeout = -1);
	uInt32  ReadRegisterFromNios_Slow(uInt32 u32Data, uInt32 u32Command, uInt32 u32TimeoutMs = 10000 );
	int32	RegisterEventHandle( EVENT_HANDLE hHamdle, uInt32 u32EventType );
	int32	TransferDataFromOnBoardMemory( PVOID pBuffer, int64 i64ByteCount, EVENT_HANDLE hAsyncEvent = 0, uInt32 *pu32TransferCount = 0 );
	int32	CardReadMemory( PVOID pBuffer, uInt32 u32ByteCount, EVENT_HANDLE hUserAsynEvent = 0, uInt32 *pu32TransferCount = 0 );
	void	AbortCardReadMemory();
	uInt32	GetIoctlDebugCounter() {return m_u32Counter;};

#ifdef _WINDOWS_
	int32	TransferDataKernel( PVOID pBuffer, uInt32 u32ByteCount, PCS_OVERLAPPED pCsOverlapped = NULL );
	static  void  ThreadWaitForAsyncTransferComplete( LPVOID  lpParams );


#endif

	int32	InitializeDevIoCtl();
	void	InitializeDevIoCtl( PCSDEVICE_INFO	pDevInfo );
	void	UnInitializeDevIoCtl();

	int32	IoctlDrvStartAcquisition( Pin_STARTACQUISITION_PARAMS pAcqParams );
	int32	IoctlSetMasterSlaveLink( MASTER_SLAVE_LINK_INFO *SlaveInfoArray, uInt32 u32NumOfSlaves );
	int32	IoctlSetMasterSlaveCalibInfo( int32	*i32AlignOffset, uInt32 u32NumOfSlaves );
	int32	IoctlSetTriggerMasterLink( DRVHANDLE TrigMasterHdl );
	int32	IoctlSetAcquisitionConfigEx( PCSACQUISITIONCONFIG_EX pAcqCfgEx );
	int32	IoctlPreDataTransfer( Pin_PREPARE_DATATRANSFER pInParams, Pout_PREPARE_DATATRANSFER pOutParams );
	int32	IoctlGetAsynDataTransferResult( CSTRANSFERDATARESULT	*pTxResult );
	int32	IoctlSetCardState( PCSCARD_STATE pCardState );
	int32	IoctlGetCardState( PCSCARD_STATE pCardState );
	int32	IoctlReadFlash( uInt32 u32Addr, PVOID pBuffer, uInt32 u32ReadSize );
	int32	IoctlWriteFlash( uInt32 u32Sector, uInt32 Offset, PVOID pBuffer, uInt32 u32WriteSize );
	int32	IoctlSendFlashCommand( uInt32 u32Addr, uInt8 u8Value );
	int32	IoctlAddonReadFlash( uInt32 u32Addr, PVOID pBuffer, uInt32 u32ReadSize );
	int32	IoctlAddonWriteFlash( uInt32 u32Sector, uInt32 u32Offset, PVOID pBuffer, uInt32 u32WriteSize );
	int32	IoctlAbortAsyncTransfer();
	uInt32	IoctlFlashOpRead(uInt32 u32Offset);
	int32	IoctlFlashOpWrite(uInt32 u32Offset, uInt32 u32Data);

	int32	IoctlStmAllocateDmaBuffer( uInt32 u32SizeInBytes, PVOID *pVA );
	int32	IoctlStmFreeDmaBuffer( PVOID pVA );
	int32	IoctlStmTransferDmaBuffer( uInt32 u32TransferSize, PVOID pVA );
	int32	IoctlStmSetStreamMode( int32	i32Mode );
	int32	IoctlStmWaitDmaComplete( uInt32 u32TimeoutMs, uInt32 *u32ErrorFlag, bool *bDmaComplete );
	int32	IoctlGetCardStatePointer( PCSCARD_STATE *pCardState );
	int32	IoctlMapCardStatePointer( uInt32 CardIndex, uInt32 u32SizeInBytes, PVOID *pMapAddr );
	int32	IoctlGetMiscState( PKERNEL_MISC_STATE pMiscState );
	int32	IoctlSetMiscState( PKERNEL_MISC_STATE pMiscState );

};



#endif   // _CSDEVIOCTL_H_

