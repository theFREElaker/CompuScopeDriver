
#ifndef _GAGEDRV_H_
#define _GAGEDRV_H_

#ifdef  _WIN32


#include <ntddk.h>
#include <wdf.h>

#include <ntstrsafe.h>

#include "wmilib.h"
#include <initguid.h>

#else   //__linux__

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/pci.h>

#include "gen_kernel_interface.h"
#include "gen_kdebug.h"

#endif

#include "CsTypes.h"
#include "CsDefines.h"
#include "CsIoctlTypes.h"
#include "CsDriverTypes.h"
#include "CsStruct.h"
#include "CsSymLinks.h"
#include "CsErrors.h"
#include "CsCardState.h"
#ifdef _WIN32
#include "TraceEvents.h"
#endif

#define   PRE_ALLOCATE_DMA_DESCIPTOR
#define   STM_USE_ASYNC_REQUEST
#define   MAX_PROCESS   10   // Max allowed process to connect to one device simultaniously

struct _FDO_DATA;
typedef void    (*LPGageFreeMemoryForDmaDescriptor)     (struct _FDO_DATA* FdoData);
typedef BOOLEAN (*LPGageAllocateMemoryForDmaDescriptor) (struct _FDO_DATA* FdoData);
typedef void    (*LPStartDmaTransferDescriptor)         (struct _FDO_DATA* FdoData);
#ifdef _WIN32
typedef void    (*LPBuildDmaTransferDescriptor) (struct _FDO_DATA*, PSCATTER_GATHER_LIST  ScatterGather);
#else
typedef void    (*LPBuildDmaTransferDescriptor) (struct _FDO_DATA*, struct scatterlist *ScatterGather, uInt32 NumberOfElements);
#endif
typedef BOOLEAN (*LPCheckBlockTransferDone) (struct _FDO_DATA* FdoData, BOOLEAN bWaitComplete);
typedef void    (*LPAbortDmaTransfer)       (struct _FDO_DATA* FdoData);
typedef void    (*LPEnableInterruptDMA)     (struct _FDO_DATA* FdoData);
typedef void    (*LPDisableInterruptDMA)    (struct _FDO_DATA* FdoData);

#ifdef _WIN32
//
// Define an Interface Guid to access the proprietary toaster interface.
// This guid is used to identify a specific interface in IRP_MN_QUERY_INTERFACE
// handler.
//

DEFINE_GUID(GUID_GAGE_INTERFACE_STANDARD,
   0xe0b27630, 0x5434, 0x11d3, 0xb8, 0x90, 0x0, 0xc0, 0x4f, 0xad, 0x51, 0x71);
// {E0B27630-5434-11d3-B890-00C04FAD5171}


//
// Define an Interface Guid for toaster device class.
// This GUID is used to register (IoRegisterDeviceInterface)
// an instance of an interface so that user application
// can control the toaster device.
//

DEFINE_GUID (GUID_DEVINTERFACE_COMPUSCOPE,
   0x781EF630, 0x72B2, 0x11d2, 0xB8, 0x52, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);
//{781EF630-72B2-11d2-B852-00C04FAD5171}



//
// Define Interface reference/dereference routines for
//  Interfaces exported by IRP_MN_QUERY_INTERFACE
//

typedef VOID (*PINTERFACE_REFERENCE)(PVOID Context);
typedef VOID (*PINTERFACE_DEREFERENCE)(PVOID Context);

typedef BOOLEAN (*PGAGE_GET_CRISPINESS_LEVEL)(IN   PVOID Context, OUT  PUCHAR Level);

typedef BOOLEAN (*PGAGE_SET_CRISPINESS_LEVEL)(IN   PVOID Context, OUT  UCHAR Level);

typedef BOOLEAN (*PGAGE_IS_CHILD_PROTECTED)(IN PVOID Context);


//
// Interface for getting and setting power level etc.,
//
typedef struct _TOASTER_INTERFACE_STANDARD {
   INTERFACE   InterfaceHeader;
   PGAGE_GET_CRISPINESS_LEVEL   GetCrispinessLevel;
   PGAGE_SET_CRISPINESS_LEVEL   SetCrispinessLevel;
   PGAGE_IS_CHILD_PROTECTED   IsSafetyLockEnabled; //):
} TOASTER_INTERFACE_STANDARD, *PTOASTER_INTERFACE_STANDARD;

typedef   BOOLEAN   BOOL;
#define   GEN_INLINE   _inline

#define GAGE_MEM_TAG   (ULONG)'EGAG'
#define GageAllocatePool(type, size)   ExAllocatePoolWithTag(type, size, GAGE_MEM_TAG)
#define GageFreePool(p)   ExFreePoolWithTag(p, GAGE_MEM_TAG)

// Declare a device IO control handler
#define DECLARE_IOCTL_HANDLER(x)   \
   NTSTATUS (x)( IN PFDO_DATA FdoData,   \
   IN WDFQUEUE   Queue,   \
   IN WDFREQUEST   Request,   \
   IN PVOID   InBuffer,   \
   IN size_t   InputBufferLength,   \
   IN PVOID   OutBuffer,   \
   IN size_t   OutputBufferLength )

#else

// Linux
#  define DbgPrint   GEN_PRINT
#  define STAT_MORE_PROCESSING_REQUIRED   0x8000
#  define UNREFERENCED_PARAMETER(a)   (a=a)

#endif

#ifdef   _PLXBASE_
#  define NUM_MEMORYMAP 3
#else
#  define NUM_MEMORYMAP 2
#endif

#define   PCI_NUM_BARS   6
#define   SYMBOLICLINK_LENGTH   50

enum
{
   DMA_INTR_CLEAR =   0x0001,
   DMA_INTR_DISABLE =   0x0002,
};

//! PCI BAR Space information
typedef struct _PCI_BAR_INFO
{
    VOID            *pVa;        // ! BAR Kernel Virtual Address
    PHYSICAL_ADDRESS Physical;   // ! BAR Physical Address
    unsigned long    Size;       // ! BAR size
    BOOLEAN          IsIoMapped; // ! Memory or I/O mapped?

} PCI_BAR_INFO;

typedef struct  _DATA_TRANSFER
{
#ifdef _WIN32
   WDFDMATRANSACTION   DmaTransaction;
#endif
   uInt32   u32TransLength;        // Dma original transaction length
   uInt32   u32CurrentDmaSize;     // Current Dma length
   uInt32   u32CurrentBlocksCount; // Current Dma Blocks count
   uInt32   u32ByteTransfered;
   int32    i32TxStatus;
} DATA_TRANSFER, *PDATA_TRANSFER;

typedef enum
{
   ACQSTATE_READY,
   ACQSTATE_TRIGGERED,
   ACQSTATE_ENDOFBUSY,
   ACQSTATE_DMACOMPLETED,
} CS_ACQSTATE, *PCS_ACQSTATE;

#ifdef _WIN32
typedef struct _CSPROCESS_STRUCT
{
   HANDLE   hProcess;         // Handle of owning process
   PVOID    pCardSatteUserVa; // Card State mapped user-mode virtual address
   PKEVENT  pkEventTriggered;
   PKEVENT  pkEventEndOfBusy;
   uInt32   u32RefCount;      // File Counter

} CSPROCESS_STRUCT, *PCSPROCESS_STRUCT;

typedef struct _KSTMDMABUFFER
{
   WDFCOMMONBUFFER   CommonBuffer;
   PMDL              pMdl;          // Mdl described the buffer
   PHYSICAL_ADDRESS  PhysAddr;      // Logical adddress for DMA
   PVOID             pKernelVa;     // Driver virtual address
   PVOID             pUserVa;       // Mapped user-mode virtual address
   uInt32            u32BufferSize; // Total size in bytes
   HANDLE            hProcess;      // Handle of owning process
   LIST_ENTRY        link;          // Link to next item in list

} KSTM_DMA_BUFFER, *PKSTM_DMA_BUFFER;

#else
#if 0
#define trace_in_()           printk(KERN_WARNING "\n*dbg* : %s : in  ...", __func__)
#define trace_(f,...)         printk(KERN_WARNING "\n*dbg* : %s : " f, __func__, ##__VA_ARGS__)
#else
#define trace_in_()           (void)0
#define trace_(f,...)         (void)0
#endif
#define set_flag_(devp, field, flag)            set_bit(__ffs(flag), &((devp)->field))
#define clr_flag_(devp, field, flag)            clear_bit(__ffs(flag), &((devp)->field))
#define test_and_clear_flag_(devp, field, flag) test_and_clear_bit(__ffs(flag), &((devp)->field))
#define is_flag_set_(devp, field, flag)         test_bit(__ffs(flag), &((devp)->field))

#define STMDBUF_INIT          0x00000001     /* stmdmabuf_t.flags */
#define STMDBUF_DRIVER        0x00000002
#define STMDBUF_PAGE_RESERVED 0x00000004

#define STMFDB_UADDR          0x00000001     /* stmfdb_t.how */
#define STMFDB_KADDR          0x00000002
#define STMFDB_FILE           0x00000004
#define STMFDB_ALL            0x00000008
#define STMFDB_ALL_UADDR      0x00000010

#define STMF_ENABLED          0x00000001
#define STMF_FIFOFULL         0x00000002
#define STMF_DMA_XFER         0x00000004
#define STMF_DMA_ABORT        0x00000008
#define STMF_STARTED          0x00000010

typedef struct _stmfdb_t
{
   union {
      uint         bytes;           /* length freed */
      ulong        uaddr;           /* user-space address */
//    void        *proc_mmp;        /* address space */
      void        *kaddrp;          /* kernel-space address */
      struct file *filp;            /* file (device) handle */
   };
   int how;                         /* STMFDB_* */
}  stmfdb_t;

typedef struct _stmdmabuf_t
{
   struct list_head  list;
   struct mm_struct *owner_mmp;     /* owner's address space */
   struct file      *owner_filp;    /* file handle that performed allocation */
   ulong             flags;         /* ... */
   uint              bytes;         /* buffer size in bytes */
   ulong             uaddr;         /* __mapped__ user-space virtual address */
   dma_addr_t        baddr;         /* bus address for dma operations */
   void             *kaddrp;        /* kernel-space address */
}  stmdmabuf_t;
#endif

#define CS_STREAM_SUPPORTED

#ifdef CS_STREAM_SUPPORTED
typedef struct _STREAM1
{
#ifdef _WIN32
   uInt32     u32ActualDmaSize;     // Actual Dma transfer size
   uInt32     u32BufferOffset;      // Offset from the begining of of the Dma buffer
   int32      i32Status;
   WDFREQUEST StmRequest;           // Current Stream transfer request
   KEVENT     kEventDmaDone;
   BOOLEAN    bEnabled;             // Streamming is enabled
   BOOLEAN    bWdfRequestAsync;     // Use the Dma request asynchronous
   BOOLEAN    bFixedSizeFifo;       // Fixed size fifo
   BOOLEAN    bRunForever;          // Capture forever
   BOOLEAN    bStarted;             // Streaming acquisition is in progress
   BOOLEAN    bFifoFull;            // Fifo for stream was full. Possibility loss data
   BOOLEAN    bBusyTransfer;        // Current transfer is not complted
   uInt32     u32DebugTranferCount;
   PTRANSFER_DMA_BUFFER_OUTPUT   pUserOutput;
   uInt32   u32UserOutputSize;

#else
   ulong             flags;
   struct completion dma_done;

#endif
}  CS_STREAM, *PCS_STREAM;
#endif

//
// The device extension for the device object
//
typedef struct _FDO_DATA
{
   uInt16           DeviceId;
   BUSTYPE          BusType;
   uInt32           u32CardIndex;
   PCIeLINK_INFO    PciExLinkInfo;
   PHYSICAL_ADDRESS MemPhysAddress[PCI_NUM_BARS];
   ULONG            MemLength[PCI_NUM_BARS];
   PVOID            MapAddress[PCI_NUM_BARS];
   uInt16           NumOfMemoryMap;
   PVOID            PciRegisterBase;
   PVOID            GageRegisterBase;
   PVOID            FlashRegisterBase;
   uInt32           IntFifoRegister;
   uInt32           FifoDataOutRegister;

#ifdef _WIN32
   PCWSTR                     RegistryPath;
   TOASTER_INTERFACE_STANDARD BusInterface;

   WDFDRIVER         WdfDriver;
   PDEVICE_OBJECT    PhysicalDevObj;
   WDFDEVICE         WdfDevice;
   WDFSPINLOCK       DmaLock;
   WDFSPINLOCK       DmaTransactionComplete;
   WDFTIMER          Timer;

   WDFINTERRUPT    WdfInterrupt;
   PKEVENT         pkEventAlarm;
   HANDLE          hCaptureProcess;    // The process that sent StartAcquisition request

   WDFDMAENABLER   WdfDmaEnabler;

   FAST_MUTEX          fMutex;
   PHYSICAL_ADDRESS    PhysAddrDmaDesc;
   KSPIN_LOCK          DeviceLock;

   uInt8         u8StmDmaBufferIndex;
   uInt8         u8StmDmaBufferCounter;
   LIST_ENTRY    lstDmaBuffers;            // List of KSTM_DMA_BUFFER
   BOOLEAN       bStmTransfer;             // Stream Dma transfer vs Regular Dma transfer

   PMDL                pCardStateMdl;
   CSPROCESS_STRUCT    CsProcess[MAX_PROCESS];

#else
   struct pci_dev    *PciDev;
   struct device     *Device;
   int               IrqLine;
   PCI_BAR_INFO      m_PciBar[PCI_NUM_BARS];

   dma_addr_t   PhysAddrDmaDesc;

   struct proc_dir_entry    *ProcEntry;
   struct task_struct       *tskEvent;
   int                       pid;
   int                       refcount;

   // DMA data transfer variables
   struct page           **PageList;
   struct page           *Page;
   struct scatterlist    *SgList;
   uInt32                u32PageLocked;
   wait_queue_head_t     DmaWaitQueue;
   BOOLEAN               bDmaIntrWait;

   /* streaming context variables */
   struct list_head  dmabuf_list;
   /* tasklet work count */
   atomic_t tsk_count;

#endif

#ifdef CS_STREAM_SUPPORTED
   CS_STREAM   Stream;
#endif

   BOOLEAN   bDmaInterrupt;

   DRVHANDLE      PseudoHandle;
   CS_ACQSTATE    AcqState;
   uInt32         u32IntBaseMask;

   BOOLEAN   bTriggered;
   BOOLEAN   bEndOfBusy;
   BOOLEAN   bTriggerNotification;
   BOOLEAN   bDmaDemandMode;

   ULONG   u32MaxDmaLength;

   DATA_TRANSFER    XferRead;
   uInt8            u8OpenCounter;     // CreateFile counter
   PVOID            pDmaDescriptor;
   ULONG            u32MaxDesc;

   char                      szSymbolLink[SYMBOLICLINK_LENGTH];
   CSACQUISITIONCONFIG_EX    AcqCfgEx;
   uInt32                    m_u32SegmentRead;
   uInt32                    u32ReadingOffset;
   uInt32                    u32SampleSize;

   // Variables related to Trigger address
   BOOLEAN    m_bMemRollOver;
   uInt32     m_u32HwTrigAddr;
   uInt32     m_Etb;
   uInt32     m_MemEtb;
   uInt8      u8TrigCardIndex;
   uInt32     m_Stb;
   uInt32     m_SkipEtb;
   uInt32     m_AddonEtb;
   uInt32     m_ChannelSkipEtb;
   uInt32     m_DecEtb;

   // DMA
   uInt32     u32RegCSR;
   BOOLEAN    bDataTransferIntrEn;
   BOOLEAN    bDataTransferBusMaster;

   uInt32    u32DpcIntrStatus;
   uInt16    u16AlarmSource;
   uInt16    u16AlarmStatus;

   uInt8   u8MsCardId;
   int32   i32MsAlignOffset;

   LPGageFreeMemoryForDmaDescriptor      pfnGageFreeMemoryForDmaDescriptor;
   LPGageAllocateMemoryForDmaDescriptor  pfnGageAllocateMemoryForDmaDescriptor;
   LPStartDmaTransferDescriptor          pfnStartDmaTransferDescriptor;
   LPBuildDmaTransferDescriptor          pfnBuildDmaTransferDescriptor;
   LPCheckBlockTransferDone              pfnCheckBlockTransferDone;
   LPAbortDmaTransfer                    pfnAbortDmaTransfer;
   LPEnableInterruptDMA                  pfnEnableInterruptDMA;
   LPDisableInterruptDMA                 pfnDisableInterruptDMA;

   PVOID     ContiguousAddr;
   uInt32    u32DebugReadLength;

   struct _FDO_DATA   *pMasterIndependent;
   struct _FDO_DATA   *pTriggerMaster;
   struct _FDO_DATA   *pNextSlave;

   CSCARD_STATE   CardState;

}  FDO_DATA, *PFDO_DATA;


#ifdef _WIN32
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DATA, GageFdoGetData)


   DRIVER_INITIALIZE DriverEntry;

   EVT_WDF_DRIVER_DEVICE_ADD GageEvtDeviceAdd;

   EVT_WDF_DEVICE_CONTEXT_CLEANUP GageEvtDeviceContextCleanup;
   EVT_WDF_DEVICE_D0_ENTRY GageEvtDeviceD0Entry;
   EVT_WDF_DEVICE_D0_EXIT GageEvtDeviceD0Exit;
   EVT_WDF_DEVICE_PREPARE_HARDWARE GageEvtDevicePrepareHardware;
   EVT_WDF_DEVICE_RELEASE_HARDWARE GageEvtDeviceReleaseHardware;

   EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT GageEvtDeviceSelfManagedIoInit;

   //
   // Io events callbacks.
   //
   EVT_WDF_IO_QUEUE_IO_READ GageEvtIoRead;
   EVT_WDF_IO_QUEUE_IO_WRITE GageEvtIoWrite;
   EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL GageEvtIoDeviceControl;
   EVT_WDF_DEVICE_FILE_CREATE GageEvtDeviceFileCreate;
   EVT_WDF_FILE_CLOSE GageEvtFileClose;
   EVT_WDF_FILE_CLEANUP GageEvtFileCleanup;
   EVT_WDF_INTERRUPT_ISR GageEvtInterruptIsr;
   EVT_WDF_INTERRUPT_DPC GageEvtInterruptDpc;
   EVT_WDF_PROGRAM_DMA GageEvtProgramDmaFunction;
   EVT_WDF_INTERRUPT_SYNCHRONIZE GageEvtForceTrigger;
   EVT_WDF_TIMER  GageDmaTimeOutFunc;
   EVT_WDF_REQUEST_CANCEL  GageEvtDmaRequestCancel;
   EVT_WDF_REQUEST_CANCEL  GageEvtStmRequestCancel;


   //
   // Power events callbacks
   //
   EVT_WDF_DEVICE_ARM_WAKE_FROM_S0 GageEvtDeviceArmWakeFromS0;
   EVT_WDF_DEVICE_ARM_WAKE_FROM_SX GageEvtDeviceArmWakeFromSx;
   EVT_WDF_DEVICE_DISARM_WAKE_FROM_S0 GageEvtDeviceDisarmWakeFromS0;
   EVT_WDF_DEVICE_DISARM_WAKE_FROM_SX GageEvtDeviceDisarmWakeFromSx;
   EVT_WDF_DEVICE_WAKE_FROM_S0_TRIGGERED GageEvtDeviceWakeFromS0Triggered;
   EVT_WDF_DEVICE_WAKE_FROM_SX_TRIGGERED GageEvtDeviceWakeFromSxTriggered;

   PCHAR
   DbgDevicePowerString(
   IN WDF_POWER_DEVICE_STATE Type
   );

   //
   // WMI event callbacks
   //
   EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE EvtWmiInstanceStdDeviceDataQueryInstance;
   EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE EvtWmiInstanceToasterControlQueryInstance;
   EVT_WDF_WMI_INSTANCE_SET_INSTANCE EvtWmiInstanceStdDeviceDataSetInstance;
   EVT_WDF_WMI_INSTANCE_SET_INSTANCE EvtWmiInstanceToasterControlSetInstance;
   EVT_WDF_WMI_INSTANCE_SET_ITEM EvtWmiInstanceToasterControlSetItem;
   EVT_WDF_WMI_INSTANCE_SET_ITEM EvtWmiInstanceStdDeviceDataSetItem;
   EVT_WDF_WMI_INSTANCE_EXECUTE_METHOD EvtWmiInstanceToasterControlExecuteMethod;


   NTSTATUS
   GageMapHWResources(
   IN OUT PFDO_DATA FdoData,
   WDFCMRESLIST   ResourcesTranslated
   );

   NTSTATUS
   GageUnmapHWResources(
   IN OUT PFDO_DATA FdoData
   );

   NTSTATUS  GageCreateDeviceSymbolicLink( PFDO_DATA FdoData );


   // Maccros for conversion of time unit used by moast of DDK functions
#define KTIME_NANOSEC(t)   ((t)/100)
#define KTIME_MICROSEC(t)   KTIME_NANOSEC((LONGLONG)1000*(t))
#define KTIME_MILISEC(t)   KTIME_MICROSEC((LONGLONG)1000*(t))
#define KTIME_SECOND(t)   KTIME_MILISEC((LONGLONG)1000*(t))
#define KTIME_RELATIVE(t)   (-1*(t))

   typedef ULONG   uInt32;
   typedef LONG   int32;
   typedef BYTE   uInt8;

#define Max(a,b)  (((a) > (b)) ? (a) : (b))
#define Min(a,b)  (((a) < (b)) ? (a) : (b))


   NTSTATUS ReadWriteConfigSpace(   __in PDEVICE_OBJECT DeviceObject,
   __in ULONG   ReadOrWrite, // 0 for read 1 for write
   __in PVOID   Buffer,
   __in ULONG   Offset,
   __in ULONG   Length );



#ifdef USE_POWER_POLICY
NTSTATUS
ToasterWmiRegistration(
   __in WDFDEVICE Device
   );

NTSTATUS
ToasterFireArrivalEvent(
   __in WDFDEVICE  Device
   );
#endif

#endif   // _WIN32


#define   MAX_CARDS   20


GEN_INLINE  VOID _WriteRegister( PFDO_DATA FdoData, ULONG u32Offset, ULONG u32Value )
{
   WRITE_REGISTER_ULONG( (PULONG) ( (PUCHAR) FdoData->PciRegisterBase + u32Offset ),  u32Value );
};

GEN_INLINE  ULONG _ReadRegister( PFDO_DATA FdoData, ULONG u32Offset )
{
   return READ_REGISTER_ULONG( (PULONG) ( (PUCHAR) FdoData->PciRegisterBase + u32Offset ) );
};

GEN_INLINE   VOID  WriteGageRegister( PFDO_DATA FdoData, uInt32 u32Offset, uInt32 u32Data )
{
   WRITE_REGISTER_ULONG( (PULONG) ( (PUCHAR) FdoData->GageRegisterBase + u32Offset ),  u32Data );
}

GEN_INLINE uInt32   ReadGageRegister( PFDO_DATA FdoData, uInt32 u32Offset )
{
   return READ_REGISTER_ULONG( (PULONG) ( (PUCHAR) FdoData->GageRegisterBase + u32Offset ) );
}

GEN_INLINE  VOID WriteFlashRegister( PFDO_DATA FdoData, ULONG u32Offset, ULONG u32Value )
{
   WRITE_REGISTER_ULONG( (PULONG) ( (PUCHAR) FdoData->FlashRegisterBase + u32Offset ),  u32Value );
};

GEN_INLINE uInt32   ReadFlashRegister( PFDO_DATA FdoData, uInt32 u32Offset )
{
   return READ_REGISTER_ULONG( (PULONG) ( (PUCHAR) FdoData->FlashRegisterBase + u32Offset ) );
}

void   WriteGioCpld( PFDO_DATA FdoData, uInt32 u32Addr, uInt32 u32Data );
uInt32   ReadGioCpld( PFDO_DATA FdoData, uInt32 u32Addr );

#ifdef _WIN32

BOOLEAN CsProbeForRead( void *DataPtr, uInt32 DataLength );
BOOLEAN CsProbeForWrite( void *DataPtr, uInt32 DataLength );

NTSTATUS GageInitiateDmaTransfer(IN PFDO_DATA FdoData, IN WDFREQUEST Request );

NTSTATUS
GageReadDataDma(
   IN  PFDO_DATA   FdoData,
   IN  WDFDMATRANSACTION   DmaTransaction,
   IN  PSCATTER_GATHER_LIST   SGList
   );

NTSTATUS   GageHandleDmaInterrupt( IN PFDO_DATA   FdoData );

NTSTATUS   GageAllocateSoftwareResources( IN OUT PFDO_DATA FdoData );
void   GageFreeSoftwareResources( IN OUT PFDO_DATA FdoData );
BOOLEAN   GageAllocateMemoryForDmaDescriptor( IN PFDO_DATA   FdoData );
void   GageFreeMemoryForDmaDescriptor( IN PFDO_DATA   FdoData );

PFDO_DATA   GetFdoData( DRVHANDLE CardHandle );
int32   SetupPreDataTransfer( PFDO_DATA   FdoData, uInt32 u32Segment, uInt16 u16Channel, uInt32 u32StartAddr, Pin_PREPARE_DATATRANSFER pXferConfig );

// DMA related function
void   IntrDMAInitialize( PFDO_DATA FdoData );
void   ClearInterruptDMA0( IN PFDO_DATA   FdoData );
BOOLEAN   IsDmaInterrupt( IN PFDO_DATA   FdoData );
void   AbortDmaTransfer( IN PFDO_DATA   FdoData );
void   SlaveModeTransferData( PFDO_DATA FdoData, void* pvBuffer, uInt32 u32LengthInBytes );

BOOLEAN   CsBoardEvtIoDeviceControl(
   IN WDFQUEUE   Queue,
   IN WDFREQUEST   Request,
   IN size_t   OutputBufferLength,
   IN size_t   InputBufferLength,
   IN ULONG   IoControlCode
   );

#else   // __linux__

int   GageFindDevice (void);
int   GageReleaseDevice(void);
int   GageMapHWResource(PFDO_DATA FdoData);
int   GageUnMapHWResource(PFDO_DATA FdoData);
int   GageCreateDeviceSymbolicLink( PFDO_DATA FdoData );
void  GageDestroyDeviceSymbolicLink( PFDO_DATA FdoData );
void  GageGetDeviceList (PFDO_DATA **listpp, uInt32 *countp);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
irqreturn_t Isr_CsIrq (int irq, void *pDevice, struct pt_regs *regs);
#else
irqreturn_t Isr_CsIrq (int irq, void *pDevice);
#endif

int   GageCreate   (struct inode *inode, struct file *filp);
int   GageClose   (struct inode *inode, struct file *filp);
ssize_t   GageRead (struct file *filp, char __user *buffer, size_t Size, loff_t *Offset );
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
long   GageDeviceControl(struct file *filp, UINT IoctlCmd, ULONG ioctl_arg);
int   CsBoardDevIoCtl(struct file *filp, UINT ioctl_cmd, ULONG ioctl_arg);
#else
int   GageDeviceControl(struct inode *inode, struct file *filp, UINT ioctl_cmd, ULONG ioctl_arg);
int   CsBoardDevIoCtl(struct inode *inode, struct file *filp, UINT ioctl_cmd, ULONG ioctl_arg);
#endif

uInt32  GetPreTrigDepthLimit ( PFDO_DATA FdoData,  uInt32 u32Segment );
uInt32  GetHwTriggerAddress( PFDO_DATA FdoData, uInt32 u32Segment, BOOLEAN *bRollOver );
int32   SetupPreDataTransfer( PFDO_DATA   FdoData, uInt32 u32Segment, uInt16 u16Channel, uInt32 u32StartAddr, Pin_PREPARE_DATATRANSFER pXferConfig );

int32   GageLockUserBuffer( PFDO_DATA FdoData, void *pBuffer, uInt32 u32BuffSize );
void    GageUnlockUserBuffer( PFDO_DATA FdoData );

int     GageAllocateSoftwareResource( PFDO_DATA FdoData );
void    GageFreeSoftwareResource( PFDO_DATA FdoData );
BOOLEAN GageAllocateMemoryForDmaDescriptor( PFDO_DATA FdoData );
void    GageFreeMemoryForDmaDescriptor( PFDO_DATA FdoData );
void    GageHandleDmaInterrupt( PFDO_DATA FdoData );
VOID    SignalEvents( PFDO_DATA FdoData );

void    EnableInterrupts(PFDO_DATA FdoData, uInt32 IntType);
void    DisableInterrupts(PFDO_DATA FdoData, uInt32 IntType);
void    ConfigureInterrupts( PFDO_DATA FdoData,  uInt32 u32IntrMask );
void    ClearInterruptDMA0( PFDO_DATA   FdoData );
BOOLEAN IsDmaInterrupt( PFDO_DATA   FdoData );

void   GetDeviceIdList( uInt16* u16DevIdTable, uInt32 u32TableSize );

void   WriteGIO( PFDO_DATA   FdoData, uInt8 u8Addr, uInt32 u32Data );
uInt32 ReadGIO( PFDO_DATA   FdoData, uInt8 u8Addr );
void   WriteFlashByte( PFDO_DATA   FdoData, uInt32 u32Addr, uInt32 u32Data );
uInt32 ReadFlashByte( PFDO_DATA   FdoData, uInt32 u32Addr );

int          stm_init            (PFDO_DATA fdodatap);
void         stm_cleanup         (PFDO_DATA fdodatap);
int          stm_dma_mmap        (struct file *filp, struct vm_area_struct *vmap);
stmdmabuf_t* stm_dmabuf_allocate (PFDO_DATA fdodatap, ALLOCATE_DMA_BUFFER *ctxp);
void         stm_dmabuf_free     (PFDO_DATA fdodatap, struct list_head *dmabuf_listp);
int          stm_dmabuf_find     (PFDO_DATA fdodatap, stmfdb_t *fdbctxp, struct list_head *dmabuf_listp);
int          stm_dmabuf_size     (PFDO_DATA fdodatap, stmfdb_t *fdbctxp, uint *sizep);
int          stm_dma_start       (PFDO_DATA fdodatap, TRANSFER_DMA_BUFFER *trfbufp);
void         stm_dma_abort       (PFDO_DATA fdodatap);
int          stm_dma_wait        (PFDO_DATA fdodatap, uint msec);
int          stm_start_capture   (PFDO_DATA fdodatap, AcquisitionCommand cmd);

#endif

void   EnableInterruptDMA0( PFDO_DATA FdoData );
void   DisableInterruptDMA0( PFDO_DATA FdoData );

uInt32   CalculateReadingOffset(PFDO_DATA FdoData, int64 i32StartAddressOffset, uInt32 u32Segment, uInt32 *pu32AlignOffset );
int32   SetMasterSlaveLink( PFDO_DATA pMaster, uInt32 u32NumOfSlaves, MASTER_SLAVE_LINK_INFO *pSlaveHandles );
int32   SetTriggerMasterLink( PFDO_DATA pMaster, DRVHANDLE TrigMasterHdl );
int32   SetMasterSlaveCalibInfo( PFDO_DATA pMaster, uInt32 u32NumOfSlaves, int32 *MsAlignOffset );
int32   SendStartAcquisition( PFDO_DATA pMaster, Pin_STARTACQUISITION_PARAMS pAcqParams   );
void   Abort( PFDO_DATA pMaster );
void   ForceTrigger( PFDO_DATA pMaster );
void   SetAcquisitionConfig( PFDO_DATA pMaster , PCSACQUISITIONCONFIG_EX pAcqCfgEx );
uInt32   GetPreTrigDepthLimit ( PFDO_DATA FdoData,  uInt32 u32Segment );
uInt32   GetHwTriggerAddress( PFDO_DATA FdoData, uInt32 u32Segment, BOOLEAN *bRollOver );
VOID   SignalEvents( PFDO_DATA  FdoData );
VOID   GageDeviceCleanupOnFileClose( PFDO_DATA FdoData );
VOID   CsAssignHardwareParams( PFDO_DATA FdoData );
VOID   CsAssignRegisterBaseAddress( PFDO_DATA FdoData );
VOID   CsAssignFunctionPointers( PFDO_DATA FdoData );


//#ifdef CS_STREAM_SUPPORTED
void   KeepTrackProcessOpen( PFDO_DATA FdoData, HANDLE hProcess );
void   KeepTrackProcessClose( PFDO_DATA FdoData, HANDLE hProcess );
PVOID   SearchCardStateUserVa( PFDO_DATA FdoData, HANDLE hProcess );
PVOID   MapCardStateUserVa( PFDO_DATA FdoData, HANDLE hProcess );
#ifdef _WIN32
void   ReadCommonRegistryValues( PFDO_DATA FdoData );
void   ReadDeviceRegistryValues( PFDO_DATA FdoData, WDFKEY hKey );

PCSPROCESS_STRUCT GetCsProcessPointer( PFDO_DATA FdoData, HANDLE hProcess );
#endif

//#endif

#endif  // _GAGEDRV_H_

