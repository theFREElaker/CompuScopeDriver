
#include <linux/interrupt.h>

#include "CsDeviceIDs.h"
#include "CsIoctl.h"
#include "CsIoctlTypes.h"
#include "CsLinuxSignals.h"
#include "GageWrapper.h"
#include "HexagonDrv.h"
#include "HexagonNiosApi.h"
#include "HexagonFlash.h"
#include "HexagonFlashObj.h"

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
#if (LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,33))
int  CsBoardDevIoCtl (struct file *filp, UINT IoctlCmd, ULONG ioctl_arg)
#else
int  CsBoardDevIoCtl (struct inode *inode, struct file *filp, UINT IoctlCmd, ULONG ioctl_arg)
#endif
{
   int  iRet = 0;
   PFDO_DATA  FdoData = (PFDO_DATA) filp->private_data;

   switch (IoctlCmd) {
      case CS_IOCTL_PRE_DATATRANSFER:
         {
            Pio_PREPARE_DATATRANSFER  temp = (Pio_PREPARE_DATATRANSFER) ioctl_arg;
            io_PREPARE_DATATRANSFER  DataXferParams;

            Pin_PREPARE_DATATRANSFER  pInTransferEx = (Pin_PREPARE_DATATRANSFER) &DataXferParams.InParams;
            Pout_PREPARE_DATATRANSFER  pOutTransferEx = (Pout_PREPARE_DATATRANSFER) &DataXferParams.OutParams;
            int64  i64PreDepthLimit;

            uInt32  u32LocalReadingOffset;
            uInt32  u32LocalAlignOffset;
            int64  i64StartAddress;

            if (copy_from_user (&DataXferParams.InParams, (void __user *)&temp->InParams, sizeof(in_PREPARE_DATATRANSFER)))
               return -EFAULT;

            //  GEN_PRINT ("FDO Data = 0x%lx\n", FdoData);
            //  GEN_PRINT ("InParams Channel = %ld\n", pInTransferEx->u16Channel);
            //  GEN_PRINT ("InParams DataMask = %ld\n", pInTransferEx->u32DataMask);

            FdoData->bDataTransferIntrEn  = pInTransferEx->bIntrEnable;
            FdoData->bDataTransferBusMaster = pInTransferEx->bBusMasterDma;

            i64PreDepthLimit = GetPreTrigDepthLimit (FdoData,  pInTransferEx->u32Segment);
            i64PreDepthLimit *= -1;

			if ( pOutTransferEx )
			{
				if ( FdoData->VolatileState.Bits.OverVoltage && pInTransferEx->u32Segment >= FdoData->CardState.u32ActualSegmentCount )
				{
					pOutTransferEx->i32Status		= CS_CHANNEL_PROTECT_FAULT;
				}
				else
				{				
					pOutTransferEx->i64ActualStartAddr = pInTransferEx->i64StartAddr;
					if (pOutTransferEx->i64ActualStartAddr  < 0) {
					   //----- Application requested for pretrigger data
					   //----- Adjust ActualStart address acordingly to pre trigger data
					   if (i64PreDepthLimit > pOutTransferEx->i64ActualStartAddr) {
						  //----- There is less pre trigger data than requested. Re-adjust the start position
						  pOutTransferEx->i64ActualStartAddr = i64PreDepthLimit;
					   }
					}

					i64StartAddress  = pOutTransferEx->i64ActualStartAddr + pInTransferEx->i32OffsetAdj;

					u32LocalReadingOffset = CalculateReadingOffset (FdoData, i64StartAddress, pInTransferEx->u32Segment, &u32LocalAlignOffset);

					SetupPreDataTransfer (FdoData, pInTransferEx->u32Segment,  pInTransferEx->u16Channel, u32LocalReadingOffset, pInTransferEx);

					pOutTransferEx->i32ActualStartOffset  = u32LocalAlignOffset + FdoData->m_MemEtb;
					pOutTransferEx->i32ActualStartOffset *= -1;

					pOutTransferEx->u32LocalAlignOffset   = u32LocalAlignOffset;
					pOutTransferEx->u32LocalReadingOffset = FdoData->u32ReadingOffset = u32LocalReadingOffset;

					// For Debug purpose only. To be removed later in release mode
					pOutTransferEx->m_u32HwTrigAddr       = FdoData->m_u32HwTrigAddr;
					pOutTransferEx->m_Etb                 = FdoData->m_Etb;
					pOutTransferEx->m_MemEtb              = FdoData->m_MemEtb;
					pOutTransferEx->m_Stb                 = FdoData->m_Stb;
					pOutTransferEx->m_SkipEtb             = FdoData->m_SkipEtb;
					pOutTransferEx->m_AddonEtb            = FdoData->m_AddonEtb;
					pOutTransferEx->m_ChannelSkipEtb      = FdoData->m_ChannelSkipEtb;
					pOutTransferEx->m_DecEtb              = FdoData->m_DecEtb;
					pOutTransferEx->i64PreTrigLimit       = i64PreDepthLimit;
					pOutTransferEx->i32OffsetAdjust       = pOutTransferEx->i32ActualStartOffset;
					pOutTransferEx->i32Status             = CS_SUCCESS;
				} //end else
			} // end if 

            iRet = copy_to_user ((void __user *)&temp->OutParams, &DataXferParams.OutParams, sizeof(out_PREPARE_DATATRANSFER));
         }
         break;

      case CS_IOCTL_WRITE_FLASH_REGISTER:
         {
            //  Pio_READWRITE_PCI_REG  PioStruct = (Pio_READWRITE_PCI_REG) ioctl_arg;
            io_READWRITE_PCI_REG  ioStruct;
            if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(io_READWRITE_PCI_REG)))
               return -EFAULT;
            WriteFlashRegister (FdoData, (uInt8) ioStruct.u32Offset, ioStruct.u32RegVal);
            //  WriteFlashRegister (FdoData, PioStruct->u32Offset, PioStruct->u32RegVal);
         }
         break;


      case CS_IOCTL_READ_FLASH_REGISTER:
         {
            io_READWRITE_PCI_REG  ioStruct;
            if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(io_READWRITE_PCI_REG)))
               return -EFAULT;
            ioStruct.u32RegVal = ReadFlashRegister (FdoData, ioStruct.u32Offset);
            iRet = copy_to_user ((void __user *) ioctl_arg, &ioStruct, sizeof(io_READWRITE_PCI_REG));
         }
         break;

      case CS_IOCTL_START_ACQUISITION:
         {
            in_STARTACQUISITION_PARAMS  StartAcqParams;
            if (copy_from_user (&StartAcqParams, (void __user *)ioctl_arg, sizeof(in_STARTACQUISITION_PARAMS)))
               return -EFAULT;
            if (0 == iRet) {
               FdoData->m_u32SegmentRead = 0;
               Abort (FdoData);
               StartAcquire (FdoData, &StartAcqParams);
            }
         }
         break;

      case CS_IOCTL_WRITE_GIO:
         {
            io_READWRITE_PCI_REG ioStruct;
            if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(io_READWRITE_PCI_REG)))
               return -EFAULT;
            if (!FdoData)
               iRet = -EFAULT;
            else
               WriteGIO (FdoData, (uInt8) ioStruct.u32Offset, ioStruct.u32RegVal);
         }
         break;

      case CS_IOCTL_READ_GIO:
         {
            io_READWRITE_PCI_REG  ioStruct;
            if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(io_READWRITE_PCI_REG)))
               return -EFAULT;
            if (!FdoData)
               iRet = -EFAULT;
            else {
               ioStruct.u32RegVal = ReadGIO (FdoData, ioStruct.u32Offset);
               iRet = copy_to_user ((void __user *)ioctl_arg, &ioStruct, sizeof(io_READWRITE_PCI_REG));
            }
         }
         break;

      case CS_IOCTL_WRITEBYTE_TOFLASH:
         {
            io_READWRITE_PCI_REG  ioStruct;
            if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(io_READWRITE_PCI_REG)))
               return -EFAULT;
            /* TBD
             * WriteFlashByte (FdoData, (uInt8) ioStruct.u32Offset, ioStruct.u32RegVal); */
         }
         break;

      case CS_IOCTL_READBYTE_FROMFLASH:
         {
            io_READWRITE_PCI_REG  ioStruct;
            if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(io_READWRITE_PCI_REG)))
               return -EFAULT;
            if (!FdoData)
               iRet = -EFAULT;
            else {
               ioStruct.u32RegVal = 0xA5A5A5A5; /* ReadFlashByte (FdoData, ioStruct.u32Offset); */
               iRet = copy_to_user ((void __user *) ioctl_arg, &ioStruct, sizeof(io_READWRITE_PCI_REG));
            }
         }
         break;

      case CS_IOCTL_ADDON_WRITEBYTE_TOFLASH:
         {
            io_READWRITE_PCI_REG  ioStruct;
            if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(io_READWRITE_PCI_REG)))
               return -EFAULT;
            if (!FdoData)
               iRet = -EFAULT;
            else
               AddonWriteFlashByte (FdoData, ioStruct.u32Offset, ioStruct.u32RegVal);
         }
         break;

      case CS_IOCTL_ADDON_READBYTE_FROMFLASH:
         {
            io_READWRITE_PCI_REG  ioStruct;
            if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(io_READWRITE_PCI_REG)))
               return -EFAULT;
            if (!FdoData)
               iRet = -EFAULT;
            else {
               ioStruct.u32RegVal = AddonReadFlashByte (FdoData, ioStruct.u32Offset);
               iRet = copy_to_user ((void __user *) ioctl_arg, &ioStruct, sizeof(io_READWRITE_PCI_REG));
            }
         }
         break;

      case CS_IOCTL_FLASH_WRITE:
         {
            WRITE_FLASH_STRUCT  ioStruct;
            unsigned char *kbuf;

            iRet = -EFAULT;
            if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(WRITE_FLASH_STRUCT)))
               break;
            if (!(kbuf = kmalloc (ioStruct.u32WriteSize, GFP_KERNEL)))
               break;
            if (copy_from_user (kbuf, (void __user *)ioStruct.pBuffer, ioStruct.u32WriteSize)) {
               kfree (kbuf);
               break;
            }
            if ((uInt32)-1 != ioStruct.u32Sector)
               iRet = HexagonFlashWriteSector (FdoData, ioStruct.u32Sector, ioStruct.u32Offset, kbuf, ioStruct.u32WriteSize);
            else {
               // Indication from user that the FLASH_WRITE is completed
               //HardResetFlash(FdoData);
               TestFlash (FdoData);
            }
            kfree (kbuf);
            break;
         }
         break;

      case CS_IOCTL_FLASH_READ:
         {
            READ_FLASH_STRUCT ioStruct;
            unsigned char *kbuf;

            iRet = -EFAULT;
            if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(READ_FLASH_STRUCT)))
               break;
            if (!(kbuf = kmalloc (ioStruct.u32ReadSize, GFP_KERNEL)))
               break;
            if (HexagonFlashRead (FdoData, ioStruct.u32Addr, kbuf, ioStruct.u32ReadSize) != CS_SUCCESS) {
               kfree (kbuf);
               break;
            }
            if (!copy_to_user ((void __user *) ioStruct.pBuffer, kbuf, ioStruct.u32ReadSize))
               iRet = 0; /* all good */
            kfree (kbuf);
         }
         break;

      case CS_IOCTL_WRITE_GIO_CPLD:
         {
            io_READWRITE_PCI_REG  ioStruct;
            if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(io_READWRITE_PCI_REG)))
               return -EFAULT;
            WriteGioCpld (FdoData, (uInt8) ioStruct.u32Offset, ioStruct.u32RegVal);
         }
         break;

      case CS_IOCTL_READ_GIO_CPLD:
         {
            io_READWRITE_PCI_REG  ioStruct;
            if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(io_READWRITE_PCI_REG)))
               return -EFAULT;
            ioStruct.u32RegVal = ReadGioCpld (FdoData, ioStruct.u32Offset);
            iRet = copy_to_user ((void __user *) ioctl_arg, &ioStruct, sizeof(io_READWRITE_PCI_REG));
         }
         break;

      case CS_IOCTL_ADDON_FLASH_READ:
         {
            READ_FLASH_STRUCT ioStruct;
            unsigned char *kbuf;

            iRet = -EFAULT;
            if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(READ_FLASH_STRUCT)))
               break;
            if (! (kbuf = kmalloc (ioStruct.u32ReadSize, GFP_KERNEL)))
               break;
            if (AddonReadFlash (FdoData, ioStruct.u32Addr, kbuf, ioStruct.u32ReadSize) != CS_SUCCESS) {
               kfree (kbuf);
               break;
            }
            if (!copy_to_user ((void __user *) ioStruct.pBuffer, kbuf, ioStruct.u32ReadSize))
               iRet = 0; /* all good */
            kfree (kbuf);
         }
         break;

      case CS_IOCTL_ADDON_FLASH_WRITE:
         {
            WRITE_FLASH_STRUCT  ioStruct;
            unsigned char *kbuf;

            iRet = -EFAULT;
            if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(WRITE_FLASH_STRUCT)))
               break;
            if (!(kbuf = kmalloc (ioStruct.u32WriteSize, GFP_KERNEL)))
               break;
            if (copy_from_user (kbuf, (void __user *)ioStruct.pBuffer, ioStruct.u32WriteSize)) {
               kfree (kbuf);
               break;
            }
            if (AddonWriteFlash (FdoData, ioStruct.u32Sector, ioStruct.u32Offset, kbuf, ioStruct.u32WriteSize) == CS_SUCCESS)
               iRet = 0;
            kfree (kbuf);
         }
         break;

      case CS_IOCTL_WRITE_PORT_IO:
         {
            io_READWRITE_PCI_REG  ioStruct;
            if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(io_READWRITE_PCI_REG)))
               return -EFAULT;
            outb ((u8) ioStruct.u32Offset, ioStruct.u32RegVal & 0xFF);
         }
         break;

      case CS_IOCTL_WRITE_CPLD_REG:
         {
            io_READWRITE_PCI_REG ioStruct;
            if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(io_READWRITE_PCI_REG)))
               return -EFAULT;
            WriteCpldRegister (FdoData, ioStruct.u32Offset, ioStruct.u32RegVal);
         }
         break;

      case CS_IOCTL_READ_CPLD_REG:
         {
            io_READWRITE_PCI_REG ioStruct;
            if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(io_READWRITE_PCI_REG)))
               return -EFAULT;
            ioStruct.u32RegVal = ReadCpldRegister (FdoData, ioStruct.u32Offset);
            iRet = copy_to_user ((void __user *) ioctl_arg, &ioStruct, sizeof(io_READWRITE_PCI_REG));
         }
         break;

	case CS_IOCTL_GET_KERNEL_MISC_STATE:
		{
		 // OverVoltage	protection
         io_KERNEL_MISC_STATE ioStruct;
         if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(io_KERNEL_MISC_STATE)))
               return -EFAULT;
//         ioStruct.u32Value = FdoData->MiscState.u32Value;
         ioStruct.u32Value = FdoData->VolatileState.u32Value;
         iRet = copy_to_user ((void __user *) ioctl_arg, &ioStruct, sizeof(io_KERNEL_MISC_STATE));
		}
		break;

	case CS_IOCTL_SET_KERNEL_MISC_STATE:
		{
		 // OverVoltage	protection
         io_KERNEL_MISC_STATE ioStruct;
         if (copy_from_user (&ioStruct, (void __user *)ioctl_arg, sizeof(io_KERNEL_MISC_STATE)))
               return -EFAULT;
//        FdoData->MiscState.u32Value = ioStruct.u32Value;
         FdoData->VolatileState.u32Value = ioStruct.u32Value;
         iRet = copy_to_user ((void __user *) ioctl_arg, &ioStruct, sizeof(io_KERNEL_MISC_STATE));
		}
		break;

      default:
         return STAT_MORE_PROCESSING_REQUIRED;
   }
   return iRet;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void  GetDeviceIdList (uInt16* u16DevIdTable, uInt32 u32TableSize)
{
   GageZeroMemory (u16DevIdTable, u32TableSize);
   u16DevIdTable[0] = CS_DEVID_HEXAGON;
   u16DevIdTable[1] = CS_DEVID_HEXAGON_PXI;
   u16DevIdTable[2] = CS_DEVID_HEXAGON_A3;
   u16DevIdTable[3] = CS_DEVID_HEXAGON_A3_PXI;
}

#define  INTR_COUNT_MASK  0x3F

static void  GageEvtInterruptDpc (ULONG p);
DECLARE_TASKLET (ISR_Tasklet, GageEvtInterruptDpc, 0);

#if (LINUX_VERSION_CODE < KERNEL_VERSION (2,6,26))
irqreturn_t Isr_CsIrq (int irq, void *pData, struct pt_regs *regs)
#else
irqreturn_t Isr_CsIrq (int irq, void *pData)
#endif
{
   PFDO_DATA FdoData = 0;
   PFDO_DATA FdoDataMaster = NULL;

   uInt32 IntIntCtrlStatus = 0;
   uInt32 IntFromNiosCount = 0;
   uInt32 IntFromBoard = 0;
   uInt32 IntEndOfTransfer = 0;

#ifdef TRACE_DEBUG_ISR
   static uInt32 m_u32IntrCount = 0;
#endif

   FdoData = (PFDO_DATA) pData;
   FdoDataMaster = FdoData->pTriggerMaster;

   IntIntCtrlStatus = ReadGageRegister (FdoData, INT_STATUS);
   IntFromNiosCount = ReadGageRegister (FdoData, INT_COUNT);

   if (0 != (IntFromNiosCount & 0x80))   // interrupt Fifo full
      IntFromNiosCount = 64;
   else
      IntFromNiosCount = IntFromNiosCount & INTR_COUNT_MASK;

   IntFromBoard = IntFromNiosCount | IntEndOfTransfer;

#ifdef TRACE_DEBUG_ISR
   if (IntFromNiosCount > 0) {
      GEN_DEBUG_PRINT ("\n----  Received interrupt %d  ---- ", m_u32IntrCount);
      GEN_DEBUG_PRINT ("  IntrStatus = 0x%x, IntrCount = %d, ", IntIntCtrlStatus, IntFromNiosCount);
   }
#endif

   if (IntFromBoard) {
      uInt32 IntFromAcqDone  = 0;
      uInt32 IntFromTrigger  = 0;
      uInt32 i = 0;
      uInt32 IntFifo = 0;

#ifdef TRACE_DEBUG_ISR
      m_u32IntrCount++;
#endif

      // Reading INT_FIFO will decrease the interrupts count
      for (i = 0; i < IntFromNiosCount; ++i)
         IntFifo |=  ReadGageRegister (FdoData, FdoData->IntFifoRegister);

      IntFromNiosCount = ReadGageRegister (FdoData, INT_COUNT);
      IntFromNiosCount = IntFromNiosCount & INTR_COUNT_MASK;

      IntFifo &= FdoData->u32IntBaseMask;

      // Determine what interrupts has been used for the acquisition
      if (FdoData->m_pCardState->u32AcqMode == AcqCmdMemory_Average) {
         IntFromTrigger   = IntFifo & FIRST_TRIGGER_AVG_INTR;
         IntFromAcqDone   = IntFifo & MULREC_AVG_DONE_INTR;
         // Patch for FW bug
         if (0 != IntFromAcqDone && 0 == IntFromTrigger)
            IntFromTrigger   = FIRST_TRIGGER_AVG_INTR;
      }
      else {
         IntFromAcqDone   = IntFifo & ACQUISITION_DONE_INTR;
         IntFromTrigger   = IntFifo & (TRIGGER_INTR | FIRST_TRIGGER_INTR);
      }

      if (PCIE_BUS == FdoData->BusType) {
         IntEndOfTransfer = IntFifo & DMA_DONE_INTR;
         if (IntEndOfTransfer) {
            // Clear status of trigger and busy that still int status register
            IntFromTrigger = IntFromAcqDone = 0;
            FdoData->bDmaInterrupt = TRUE;
         }
      }
      if (IntFromTrigger) {
         FdoData->bTriggered = TRUE;
         // Temporary fixed for problem with interrupt
         // If interrupt FIFO is full (>=64 interrupts) we will not receive any interrupt included
         // N_MBUSY_REC_DONE.
         // Semaphore for trigger scheme need to be modified later when we want to added that feature...
         DisableInterrupts (FdoData, IntFromTrigger);
      }
      if (true/* 0 != (FdoData->AcqCfgEx.u32ExpertOption & CS_BBOPTIONS_MULREC_AVG_TD) */) {
         if (0 != (IntFifo & MULREC_AVG_DONE_INTR)) {
            DisableInterrupts (FdoData, IntFromAcqDone);
            FdoData->bEndOfBusy = TRUE;
         }
      }
      if (IntFromAcqDone) {
         DisableInterrupts (FdoData, IntFromAcqDone);
         FdoData->bEndOfBusy = TRUE;
      }
#ifdef CS_STREAM_SUPPORTED
      if (is_flag_set_ (&FdoData->Stream, flags, STMF_ENABLED)) {
         // The status of DMA completed remain set until we send abort.
         // If we do not send abort Transfer here, the status DMA done may cause confusion for driver later.
         // For example, When we have FIFO_OVERLOW interrupt. the interrupt fifo register will have both bit DMA_Done and Fifo_Overlow set
         if (IntEndOfTransfer)
            AltAbortDmaTransfer (FdoData);

         if (0 != (IntFifo & STREAM_OVERFLOW_INTR)) {
            set_flag_ (&FdoData->Stream, flags, STMF_FIFOFULL);
            DisableInterrupts (FdoData, STREAM_OVERFLOW_INTR);
         }
      }
#endif
      if (IntFifo & OVER_VOLTAGE_INTR) {
         FdoData->m_pCardState->bOverVoltage = TRUE;
 		 FdoData->VolatileState.Bits.OverVoltage = 1;
         // Abort the current acquisition and signal events
         Abort (FdoDataMaster);
         FdoData->bEndOfBusy = TRUE;
         FdoData->bTriggered = TRUE;

         DisableInterrupts (FdoDataMaster, TRIGGER_INTR|FIRST_TRIGGER_INTR);
         DisableInterrupts (FdoDataMaster, ACQUISITION_DONE_INTR);
         //DisableInterrupts (FdoData, OVER_VOLTAGE);
      }
   }
   else {
#if 0
      // Return FALSE to indicate that this device did not cause the interrupt.
      return FALSE;
#endif
      return IRQ_NONE;
   }
   atomic_inc (&FdoData->tsk_count);
   tasklet_schedule (&ISR_Tasklet);
   return IRQ_HANDLED;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
#if 0

VOID GageEvtInterruptDpc (IN WDFINTERRUPT WdfInterrupt, IN WDFOBJECT  WdfDevice)
{
   PFDO_DATA  FdoData = GageFdoGetData (WdfInterruptGetDevice (WdfInterrupt));

   if (FdoData->bDmaInterrupt) {
      FdoData->bDmaInterrupt = FALSE;
      GageHandleDmaInterrupt (FdoData);
   }
   else
      SignalEvents (FdoData);
}

#else  // _linux_

VOID GageEvtInterruptDpc (ULONG pIn)
{
   uInt32 size=0;
   PFDO_DATA *fdotablepp=0;
   PFDO_DATA fdodatap=0;

   GageGetDeviceList (&fdotablepp, &size);

   for (; size; --size) {
      fdodatap = *fdotablepp++;
      if (!atomic_read (&fdodatap->tsk_count))
         continue;
      if (fdodatap->bDmaInterrupt) {
         fdodatap->bDmaInterrupt = FALSE;
         GageHandleDmaInterrupt (fdodatap);
      }
      else {
         GEN_DEBUG_PRINT ("DPC Called!!!!!!!\n");
         SignalEvents (fdodatap);
      }
      atomic_dec (&fdodatap->tsk_count);
   }
}
#endif

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void  GageHandleDmaInterrupt (PFDO_DATA FdoData)
{
   uInt32  u32ByteLeft = 0; // Assume all bytes has been transfered
   FdoData->XferRead.u32ByteTransfered = FdoData->XferRead.u32CurrentDmaSize - u32ByteLeft;

   if (0 == u32ByteLeft) {
#ifdef CS_STREAM_SUPPORTED
      CS_STREAM *streamp = &FdoData->Stream;
      if (test_and_clear_flag_ (streamp, flags, STMF_DMA_XFER)) {
         complete_all (&streamp->dma_done);
         return;
      }
#endif
      FdoData->bDmaIntrWait = FALSE;
      wake_up_interruptible (&FdoData->DmaWaitQueue);
   }
}


