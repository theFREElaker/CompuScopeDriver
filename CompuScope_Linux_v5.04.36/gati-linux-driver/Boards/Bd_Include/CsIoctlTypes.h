
#ifndef   _CSTOCTLTYPES_H_

#define _CSTOCTLTYPES_H_

#include "CsTypes.h"
#include "CsStruct.h"
#include "CsPrivateStruct.h"

typedef struct _out_CS_IOCTL_STATUS
{
   int32   i32Status;
} CS_IOCTL_STATUS, *PCS_IOCTL_STATUS;

#ifndef __KERNEL__
typedef struct _in_REGISTER_EVENT_HANDLE
{
   EVENT_HANDLE hUserEventHandle;   // Handle of the event to be signaled.
   // It can be an event created by application (external ) or
   // an event created by Hw Dll (internal event)
   // the bExternalEvent reflects which kind of event.
   uInt32   u32EventType;   // Type of event (TRIGGERED, END_OF_BUSY or END_OF_TRANSFER)
   BOOLEAN   bExternalEvent;   // Internal driver event or event created by application
} in_REGISTER_EVENT_HANDLE;
#endif

#ifdef _WINDOWS_

#ifdef CS_WIN64

typedef struct _in_REGISTER_EVENT_HANDLE32
{
   VOID*POINTER_32 hUserEventHandle;   // Handle of the event to be signaled.
   // It can be an event created by application (external ) or
   // an event created by Hw Dll (internal event)
   // the bExternalEvent reflects which kind of event.
   uInt32   u32EventType;   // Type of event (TRIGGERED, END_OF_BUSY or END_OF_TRANSFER)
   BOOLEAN   bExternalEvent;   // Internal driver event or event created by application
} in_REGISTER_EVENT_HANDLE32;

#endif

#endif

typedef struct _STARTACQUISITION_PARAMS
{
   uInt32   u32IntrMask;   // Interrupt enabled mask
   uInt32   u32Param1;   // Depending on compuscope card, Param1 and Param2 can be use to send more parameters needed for start acquisition
   uInt32   u32Param2;
   BOOLEAN   bRtMode;
   BOOLEAN   bOverload;   // use overload startAcquisition function

} in_STARTACQUISITION_PARAMS, *Pin_STARTACQUISITION_PARAMS;


typedef struct _CSACQUISITIONCONFIG_EX
{
   CSACQUISITIONCONFIG AcqCfg;

   int64   i64HwSegmentSize;
   int64   i64HwDepth;
   int64		i64HwTrigDelay;
   uInt32   u32Decimation;
   uInt32   u32ExpertOption;
   BOOLEAN   bSquareRec;

} CSACQUISITIONCONFIG_EX, *PCSACQUISITIONCONFIG_EX;


typedef struct _in_MASTER_SLAVE_LINK_INFO
{
   DRVHANDLE CardHandle;
   uInt8   u8MsCardId;

} MASTER_SLAVE_LINK_INFO, *PMASTER_SLAVE_LINK_INFO;



typedef struct _in_PREPARE_DATATRANSFER
{
   int64   i64StartAddr;   // Start address relative to trigger position
   uInt32  u32Segment;   // Start segment
   uInt16  u16Channel;   // Channel index 1, 2, 3 ...
   int32   i32OffsetAdj;   // Trigger address offset if there is any
   uInt32  u32TxMode;   // Transfer mode (slave, time_stamp, Tail ... )
   uInt32  u32DataMask;   // Data mask (digital or analog only data )
   uInt32  u32FifoMode;   // Mode select
   uInt32  u32SampleSize;   // Sample Size
   uInt32  u32ReadCount;   // only use on CsTransferEx
   uInt32  u32SkipCount;   // only use on CsTransferEx
   uInt32  u32Extras;   // multipurpose use depending on Hw
   BOOLEAN bBusMasterDma;   // Bus master DMA
   BOOLEAN bXpertFIR;   // FIR
   BOOLEAN bXpertHwAVG;   //
   BOOLEAN bIntrEnable;

} in_PREPARE_DATATRANSFER, *Pin_PREPARE_DATATRANSFER;


typedef struct _out_PREPARE_DATATRANSFER
{
   int32   i32Status;

   uInt32  u32LocalAlignOffset;
   uInt32  u32LocalReadingOffset;

   uInt32  m_u32HwTrigAddr;
   uInt32  m_Etb;
   uInt32  m_MemEtb;

   uInt32  m_Stb;
   uInt32  m_SkipEtb;
   uInt32  m_AddonEtb;
   uInt32  m_ChannelSkipEtb;
   uInt32  m_DecEtb;

   int32   i32ActualStartOffset;
   int64   i64PreTrigLimit;

   // not sure need them
   int64   i64ActualStartAddr;
   int32   i32OffsetAdjust;

   BOOLEAN m_bMemRollOver;

} out_PREPARE_DATATRANSFER, *Pout_PREPARE_DATATRANSFER;

typedef struct _RW_FLASH_BYTE_STRUCT
{
   uInt32   u32Addr;
   uInt8   u8Value;

} RW_FLASH_BYTE_STRUCT, *PRW_FLASH_BYTE_STRUCT;

typedef struct _WRITE_FLASH_BYTE_STRUCT
{
   uInt32   u32Sector;
   uInt32   u32Offset;
   uInt32   u32WriteSize;
   PVOID   pBuffer;

} WRITE_FLASH_STRUCT, *PWRITE_FLASH_STRUCT;

typedef struct _READ_FLASH_STRUCT
{
   uInt32   u32Addr;
   uInt32   u32ReadSize;
   PVOID   pBuffer;

} READ_FLASH_STRUCT, *PREAD_FLASH_STRUCT;

#ifdef _WINDOWS_

#ifdef CS_WIN64

typedef struct _in_WRITE_PCI_CONFIG_32
{
   uInt32   u32Offset;
   uInt32   u32ByteCount;
   VOID*POINTER_32   ConfigBuffer;

} in_WRITE_PCI_CONFIG_32, *Pin_WRITE_PCI_CONFIG_32;


typedef struct _WRITE_FLASH_STRUCT_32
{
   uInt32   u32Sector;
   uInt32   u32Offset;
   uInt32   u32WriteSize;
   VOID*POINTER_32   pBuffer;

} WRITE_FLASH_STRUCT_32, *PWRITE_FLASH_STRUCT_32;

typedef struct _READ_FLASH_STRUCT_32
{
   uInt32   u32Addr;
   uInt32   u32ReadSize;
   VOID*POINTER_32   pBuffer;

} READ_FLASH_STRUCT_32, *PREAD_FLASH_STRUCT_32;


#endif

//
// CS_IOCTL_GET_ASYNC_DATA_TRANSFER_RESULT
//

typedef struct _in_DATA_TRANSFER_RESULT
{
   uInt16   u16Channel;   // The Channel Index
} in_DATA_TRANSFER_RESULT;


//
// CS_IOCTL_GET_ASYNC_DATA_TRANSFER_RESULT
//

typedef struct _out_DATA_TRANSFER_RESULT
{
   int32   i32Status;   //<! Status of data transfer
   int64   i64BytesTransfered;   //<! Bytes transfered to buffer
} out_DATA_TRANSFER_RESULT;


typedef struct _in_WRITE_REGISTER
{
   uInt32   u32Offset;
   uInt32   u32RegVal;

} in_WRITE_REGISTER, *Pin_WRITE_REGISTER;


typedef struct _in_READ_REGISTER
{
   uInt32   u32Offset;

} in_READ_REGISTER, *Pin_READ_REGISTER;



typedef struct _out_READ_REGISTER
{
   int32   i32Status;
   uInt32   u32RegVal;
} out_READ_REGISTER, *Pout_READ_REGISTER;


typedef struct _in_READWRITE_NIOS_REGISTER
{
   uInt32   u32Command;
   uInt32   u32Data;
   int32   i32Timeout;

} in_READWRITE_NIOS_REGISTER, *Pin_READWRITE_NIOS_REGISTER;


typedef struct _in_READ_PCI_CONFIG
{
   uInt32   u32Offset;
   uInt32   u32ByteCount;

} in_READ_PCI_CONFIG, *Pin_READ_PCI_CONFIG;


typedef struct _in_WRITE_PCI_CONFIG
{
   uInt32   u32Offset;
   uInt32   u32ByteCount;
   PVOID   ConfigBuffer;

} in_WRITE_PCI_CONFIG, *Pin_WRITE_PCI_CONFIG;


#endif



#ifdef __linux__

typedef struct _io_RW_NIOS_REGISTER
{
   uInt32  u32Command;
   uInt32  u32Data;
   int32   i32Timeout;
   uInt32  u32RetVal;
   int32   i32Status;

} io_READWRITE_NIOS_REGISTER, *Pio_READWRITE_NIOS_REGISTER;


typedef struct _io_RW_PCI_REGISTER
{
   uInt32   u32Offset;
   uInt32   u32RegVal;

} io_READWRITE_PCI_REG, *Pio_READWRITE_PCI_REG;

typedef struct _io_RW_CPLD_REGISTER
{
   uInt32   u32Offset;
   uInt32   u32RegVal;

} io_READWRITE_CPLD_REG, *Pio_READWRITE_CPLD_REG;

typedef struct _in_WRITE_PCI_CONFIG
{
   DRVHANDLE   CardHandle;
   uInt32   u32Offset;
   PVOID   ConfigBuffer;
   uInt32   u32ByteCount;

} io_READWRITE_PCI_CONFIG, *Pio_READWRITE_PCI_CONFIG;

typedef struct _io_PREPARE_DATATRANSFER
{
   in_PREPARE_DATATRANSFER   InParams;
   out_PREPARE_DATATRANSFER   OutParams;

} io_PREPARE_DATATRANSFER, *Pio_PREPARE_DATATRANSFER;

typedef struct _in_MS_LINK_INFO
{
   uInt32   u32NumOfSlave;
   MASTER_SLAVE_LINK_INFO   MsLinkInfo[20];   // Max 20 slave cards

} MS_LINK_INFO, *PMS_LINK_INFO;


typedef struct _in_MS_CALIB_INFO
{
   uInt32   u32NumOfSlave;
   int32   MsAlignOffset[20];   // Max 20 slave cards

} MS_CALIB_INFO, *PMS_CALIB_INFO;

typedef struct _in_ADDON_FPGA_DESC
{
   PVOID   pFpgaBuffer;
   uInt32   u32BufferSize;

   int32   i32Status;
} ADDON_FPGA_DESC, *PADDON_FPGA_DESC;
#endif


typedef union _ALLOCATE_DMA_BUFFER
{
   struct
   {
      uInt32   u32BufferSize;   // size in bytes
   } in;

   struct
   {
      int32   i32Status;
      PVOID   pVa;
   } out;

}  ALLOCATE_DMA_BUFFER, *PALLOCATE_DMA_BUFFER;

typedef union _MAP_CARDSTATE_BUFFER
{
   struct
   {
      uInt32  CardIndex;       // Card index
      PVOID   pVa;             // user buffer
      uInt32  u32BufferSize;   // size in bytes
      uInt32  u32ProcId;       // processid
   }  in;
   struct
   {
      int32 i32Status;
      int32 kaddr;            /* kernel address */
   }  out;

}  MAP_CARDSTATE_BUFFER, *PMAP_CARDSTATE_BUFFER;

typedef struct _TRANSFER_DMA_BUFFER_OUTPUT
{
   int32   i32Status;
   uInt32   u32ErrorFlag;   // Any error flag returned from kernel driver
   uInt32   u32ActualSize;   // Actual valid samples in buffer
   uInt8   u8EndOfData;   // Timeout in miliseconD
}  TRANSFER_DMA_BUFFER_OUTPUT, *PTRANSFER_DMA_BUFFER_OUTPUT;

typedef union _TRANSFER_DMA_BUFFER
{
   struct
   {
      PVOID   pBuffer;   // point to the buffer created by AllocatedStmbuffer()
      uInt32   u32TransferSize;   // TransferSize in bytes
   } in;

   TRANSFER_DMA_BUFFER_OUTPUT out;

} TRANSFER_DMA_BUFFER, *PTRANSFER_DMA_BUFFER;


typedef union _FREE_DMA_BUFFER
{
   struct
   {
      PVOID pVa;
      int32 kaddr; /* _must_ be set to '1' if mmap failed */
   }  in;
   struct
   {
      int32 i32Status;
   }  out;

}  FREE_DMA_BUFFER, *PFREE_DMA_BUFFER;

#ifdef CS_WIN64
typedef union _ALLOCATE_DMA_BUFFER_32
{
   struct
   {
      uInt32   u32BufferSize;   // size in bytes
   } in;

   struct
   {
      int32   i32Status;
      VOID*POINTER_32   pVa;
   } out;

} ALLOCATE_DMA_BUFFER32, *PALLOCATE_DMA_BUFFER32;


typedef union _TRANSFER_DMA_BUFFER_32
{
   struct
   {
      VOID*POINTER_32   pBuffer;   // point to the buffer created by AllocatedStmbuffer()
      uInt32   u32TransferSize;   // TransferSzie in bytes
   } in;

   TRANSFER_DMA_BUFFER_OUTPUT out;

} TRANSFER_DMA_BUFFER32, *PTRANSFER_DMA_BUFFER32;


typedef union _FREE_DMA_BUFFER_32
{
   struct
   {
      VOID*POINTER_32   pVa;
   } in;

   struct
   {
      int32   i32Status;
   } out;

} FREE_DMA_BUFFER32, *PFREE_DMA_BUFFER32;
#endif

typedef union _SET_STREAM_MODE
{
   struct
   {
      int32   i32Mode;
   } in;

   struct
   {
      int32   i32Status;
   } out;

} SET_STREAM_MODE, *PSET_STREAM_MODE;


typedef union _WAIT_STREAM_DMA_DONE
{
   struct
   {
      uInt32   u32TimeoutMs;   // Timeout in milisecond
   } in;

   TRANSFER_DMA_BUFFER_OUTPUT out;

} WAIT_STREAM_DMA_DONE, *PWAIT_STREAM_DMA_DONE;


#endif   // _CSTOCTLTYPES_H_
