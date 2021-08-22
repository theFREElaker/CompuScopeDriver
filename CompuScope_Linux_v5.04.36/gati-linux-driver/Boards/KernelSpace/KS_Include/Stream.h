
#include "CsDefines.h"
#include "CsErrors.h"
#include "CsIoctl.h"
#include "CsIoctlTypes.h"
#include "NiosApi.h"
#include "PlxSupport.h"
#include "CsPlxFlash.h"
#include "CsExpert.h"


// STREAM Data transfer State
#define	STM_TRANSFER_ERROR_NOT_STARTED			0
#define	STM_TRANSFER_STARTED					1
#define	STM_TRANSFER_COMPLETED					2
#define	STM_TRANSFER_ABORTED					3



DECLARE_IOCTL_HANDLER(DoStmAllocateDmaBuffer);
DECLARE_IOCTL_HANDLER(DoStmFreeDmaBuffer);
DECLARE_IOCTL_HANDLER(DoStmTransferBuffer);
DECLARE_IOCTL_HANDLER(DoStmSetStreamMode);
DECLARE_IOCTL_HANDLER(DoStmWaitTransferCompleted);

NTSTATUS StmInitiateDmaTransfer(IN PFDO_DATA FdoData,  IN WDFREQUEST   Request, PKSTM_DMA_BUFFER pStmBuffer, uInt32 u32Offset, uInt32 u32TransferSize );

void	StmInitializeStruct( PCS_STREAM  pStream );
KSTART_ROUTINE StmWaitForStreamThread;
void	StmTerminateAsyncThread( IN PFDO_DATA FdoData );

int32 StmFreeDmaBuffers( IN PFDO_DATA FdoData, PVOID VirAddr, BOOLEAN bFreeAll );
void  StmFreeAllDmaBuffers( IN PFDO_DATA FdoData );
int32 StmFindDmaBufferDesc( IN PFDO_DATA FdoData, IN PVOID VirAddr, OUT PKSTM_DMA_BUFFER *pStmBuffer );
int32 StmStartCapture( IN PFDO_DATA FdoData );
void  StmAbortDmaTrransfer( IN PFDO_DATA FdoData );
void  StmCompleteWdfRequestWithStatus( IN PFDO_DATA FdoData, int32 i32StmStatus );
