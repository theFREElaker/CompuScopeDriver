#include <linux/interrupt.h>

#include "CsRabbitDrv.h"
#include "CsTypes.h"
#include "CsDeviceIDs.h"
#include "GageWrapper.h"

#include "CsDefines.h"
#include "CsErrors.h"
#include "CsIoctl.h"
#include "NiosApi.h"
#include "PlxSupport.h"
#include "CsLinuxSignals.h"
#include "CsRabbitFlashObj.h"
#include "CsPlxFlash.h"
#include "CsNucleonFlash.h"

//-----------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
int		CsBoardDevIoCtl(struct file *filp, UINT IoctlCmd, ULONG ioctl_arg)
#else
int		CsBoardDevIoCtl(struct inode *inode, struct file *filp, UINT IoctlCmd, ULONG ioctl_arg)
#endif
{
	int			iRet = 0;
	PFDO_DATA 	FdoData = (PFDO_DATA) filp->private_data;

	switch (IoctlCmd)
	{
	case CS_IOCTL_PRE_DATATRANSFER:
	{
		Pio_PREPARE_DATATRANSFER 	temp = (Pio_PREPARE_DATATRANSFER) ioctl_arg;
		io_PREPARE_DATATRANSFER		DataXferParams;

		Pin_PREPARE_DATATRANSFER	pInTransferEx = (Pin_PREPARE_DATATRANSFER) &DataXferParams.InParams;
		Pout_PREPARE_DATATRANSFER	pOutTransferEx = (Pout_PREPARE_DATATRANSFER) &DataXferParams.OutParams;
		int64						i64PreDepthLimit;

		uInt32	u32LocalReadingOffset;
		uInt32	u32LocalAlignOffset;
		int64   i64StartAddress;

		iRet = copy_from_user ( &DataXferParams.InParams, (void __user *)&temp->InParams, sizeof (in_PREPARE_DATATRANSFER));

      //   GEN_PRINT ("FDO Data = 0x%lx\n", FdoData );
      //   GEN_PRINT ("InParams Channel = %ld\n", pInTransferEx->u16Channel );
      //   GEN_PRINT ("InParams DataMask = %ld\n", pInTransferEx->u32DataMask );

		FdoData->bDataTransferIntrEn 	= pInTransferEx->bIntrEnable;
		FdoData->bDataTransferBusMaster = pInTransferEx->bBusMasterDma;
      FdoData->u32SampleSize  = pInTransferEx->u32SampleSize;

#if 0
		if ( FdoData->bDataTransferIntrEn )
			EnableInterruptDMA0( FdoData );
		else
			DisableInterruptDMA0( FdoData );
#endif

		i64PreDepthLimit = GetPreTrigDepthLimit ( FdoData,  pInTransferEx->u32Segment );
		i64PreDepthLimit *= -1;

      //	GEN_PRINT ("Pretrigger limit = %lld\n", i64PreDepthLimit );

		pOutTransferEx->i64ActualStartAddr = pInTransferEx->i64StartAddr;
		if ( pOutTransferEx->i64ActualStartAddr  < 0 )
		{
			//----- Application requested for pretrigger data
			//----- Adjust ActualStart address acordingly to pre trigger data
			if ( i64PreDepthLimit > pOutTransferEx->i64ActualStartAddr )
			{
				//----- There is less pre trigger data than requested. Re-adjust the start position
				pOutTransferEx->i64ActualStartAddr = i64PreDepthLimit;
			}
		}
		i64StartAddress  = pOutTransferEx->i64ActualStartAddr + pInTransferEx->i32OffsetAdj;
		u32LocalReadingOffset = CalculateReadingOffset( FdoData, (int32) i64StartAddress, pInTransferEx->u32Segment, &u32LocalAlignOffset );

		SetupPreDataTransfer( FdoData, pInTransferEx->u32Segment,  pInTransferEx->u16Channel, u32LocalReadingOffset, pInTransferEx );

		pOutTransferEx->i32ActualStartOffset = u32LocalAlignOffset + FdoData->m_Etb;
		pOutTransferEx->i32ActualStartOffset *= -1;


		pOutTransferEx->u32LocalAlignOffset = u32LocalAlignOffset;
		pOutTransferEx->u32LocalReadingOffset = FdoData->u32ReadingOffset = u32LocalReadingOffset;

		// For Debug purpose only. To be removed later in release mode
		pOutTransferEx->m_u32HwTrigAddr = FdoData->m_u32HwTrigAddr;
		pOutTransferEx->m_Etb			= FdoData->m_Etb;
		pOutTransferEx->m_MemEtb		= FdoData->m_MemEtb;
		pOutTransferEx->m_Stb			= FdoData->m_Stb;
		pOutTransferEx->m_SkipEtb		= FdoData->m_SkipEtb;
		pOutTransferEx->m_AddonEtb		= FdoData->m_AddonEtb;
		pOutTransferEx->m_ChannelSkipEtb	= FdoData->m_ChannelSkipEtb;
		pOutTransferEx->m_DecEtb		= FdoData->m_DecEtb;
		pOutTransferEx->i64PreTrigLimit = i64PreDepthLimit;
		pOutTransferEx->i32OffsetAdjust = pOutTransferEx->i32ActualStartOffset;
		pOutTransferEx->m_bMemRollOver = 0;
      pOutTransferEx->i32Status   = CS_SUCCESS;

		iRet = copy_to_user ( (void __user *)&temp->OutParams, &DataXferParams.OutParams, sizeof (out_PREPARE_DATATRANSFER));

	}
	break;

	case CS_IOCTL_WRITE_FLASH_REGISTER:
	{
		io_READWRITE_PCI_REG	ioStruct;
		iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_PCI_REG));
		WriteFlashRegister( FdoData, (uInt8) ioStruct.u32Offset, ioStruct.u32RegVal);
	}
	break;

	case CS_IOCTL_READ_FLASH_REGISTER:
	{
		io_READWRITE_PCI_REG	ioStruct;

		iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_PCI_REG));
		ioStruct.u32RegVal = ReadFlashRegister(FdoData, ioStruct.u32Offset);
		iRet = copy_to_user ((void __user *) ioctl_arg, &ioStruct, sizeof (io_READWRITE_PCI_REG));
	}
	break;

	case CS_IOCTL_START_ACQUISITION:
	{
		in_STARTACQUISITION_PARAMS		StartAcqParams;

		iRet = copy_from_user ( &StartAcqParams, (void __user *)ioctl_arg, sizeof (in_STARTACQUISITION_PARAMS));
		if ( 0 == iRet )
		{
			FdoData->m_u32SegmentRead = 0;

			Abort( FdoData );
			RabbitAcquire( FdoData->pMasterIndependent, &StartAcqParams );
		}
	}
	break;

	case CS_IOCTL_WRITE_GIO:
		{
			io_READWRITE_PCI_REG	ioStruct;
			iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_PCI_REG));

			WriteGIO ( FdoData, (uInt8) ioStruct.u32Offset, ioStruct.u32RegVal);
		}
		break;

	case CS_IOCTL_READ_GIO:
		{
			io_READWRITE_PCI_REG	ioStruct;

			iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_PCI_REG));
			ioStruct.u32RegVal = ReadGIO(FdoData, ioStruct.u32Offset);
			iRet = copy_to_user ((void __user *) ioctl_arg, &ioStruct, sizeof (io_READWRITE_PCI_REG));
		}
		break;

	case CS_IOCTL_WRITEBYTE_TOFLASH:
		{
			io_READWRITE_PCI_REG	ioStruct;
			iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_PCI_REG));

			WriteFlashByte( FdoData, (uInt8) ioStruct.u32Offset, ioStruct.u32RegVal);
		}
		break;

	case CS_IOCTL_READBYTE_FROMFLASH:
		{
			io_READWRITE_PCI_REG	ioStruct;

			iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_PCI_REG));
			ioStruct.u32RegVal = ReadFlashByte(FdoData, ioStruct.u32Offset);
			iRet = copy_to_user ((void __user *) ioctl_arg, &ioStruct, sizeof (io_READWRITE_PCI_REG));
		}
		break;

	case CS_IOCTL_ADDON_WRITEBYTE_TOFLASH:
		{
			io_READWRITE_PCI_REG	ioStruct;
			iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_PCI_REG));

			AddonWriteFlashByte( FdoData, ioStruct.u32Offset, ioStruct.u32RegVal);
		}
		break;

	case CS_IOCTL_ADDON_READBYTE_FROMFLASH:
		{
			io_READWRITE_PCI_REG	ioStruct;

			iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_PCI_REG));
			ioStruct.u32RegVal = AddonReadFlashByte(FdoData, ioStruct.u32Offset);
			iRet = copy_to_user ((void __user *) ioctl_arg, &ioStruct, sizeof (io_READWRITE_PCI_REG));
		}
		break;

      case CS_IOCTL_FLASH_WRITE:
         {
            WRITE_FLASH_STRUCT  ioStruct;
            unsigned char *kbuf;

            iRet = -EFAULT;

            if (copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (WRITE_FLASH_STRUCT)))
               break;

            if (!(kbuf = kmalloc(ioStruct.u32WriteSize, GFP_KERNEL)))
               break;

            if (copy_from_user ( kbuf, (void __user *)ioStruct.pBuffer, ioStruct.u32WriteSize))
            {
               kfree(kbuf);
               break;
            }
            if ( PCIE_BUS == FdoData->BusType )
            {
               if ( (uInt32)-1 != ioStruct.u32Sector )
               {
                  if (NucleonFlashWrite( FdoData, ioStruct.u32Sector, ioStruct.u32Offset, kbuf, ioStruct.u32WriteSize ) == CS_SUCCESS)
                     iRet = 0;
               }
               else
               {
                  // Indication from user that the FLASH_WRITE is completed
                  HardResetFlash(FdoData);
                  iRet = 0;
               }
            }
            else
            {
               if (PlxFlashWrite( FdoData, ioStruct.u32Sector, ioStruct.u32Offset, kbuf, ioStruct.u32WriteSize ) == CS_SUCCESS)
                  iRet = 0;
            }

            kfree(kbuf);
            break;
         }
         break;

      case CS_IOCTL_FLASH_READ:
         {
            READ_FLASH_STRUCT ioStruct;
            unsigned char *kbuf;

            iRet = -EFAULT;

            if (copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (READ_FLASH_STRUCT)))
               break;

            if (!(kbuf = kmalloc(ioStruct.u32ReadSize, GFP_KERNEL)))
               break;

            if ( PCIE_BUS == FdoData->BusType )
            {
               if (NucleonFlashRead( FdoData, ioStruct.u32Addr, kbuf, ioStruct.u32ReadSize ) != CS_SUCCESS) {
                  kfree(kbuf);
                  break;
               }
            }
            else
            {
               if (PlxFlashRead( FdoData, ioStruct.u32Addr, kbuf, ioStruct.u32ReadSize ) != CS_SUCCESS) {
                  kfree(kbuf);
                  break;
               }
            }

            if (!copy_to_user ((void __user *) ioStruct.pBuffer, kbuf, ioStruct.u32ReadSize))
               iRet = 0; /* all good */

            kfree(kbuf);
            break;
         }
         break;

      case CS_IOCTL_WRITE_GIO_CPLD:
         {
            io_READWRITE_PCI_REG   ioStruct;
            iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_PCI_REG));

            WriteGioCpld( FdoData, (uInt8) ioStruct.u32Offset, ioStruct.u32RegVal);
         }
         break;

      case CS_IOCTL_READ_GIO_CPLD:
         {
            io_READWRITE_PCI_REG   ioStruct;

            iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_PCI_REG));
            ioStruct.u32RegVal = ReadGioCpld(FdoData, ioStruct.u32Offset);
            iRet = copy_to_user ((void __user *) ioctl_arg, &ioStruct, sizeof (io_READWRITE_PCI_REG));
         }
         break;

      case CS_IOCTL_ADDON_FLASH_READ:
         {
            READ_FLASH_STRUCT ioStruct;
            unsigned char *kbuf;

            iRet = -EFAULT;

            if (copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (READ_FLASH_STRUCT)))
               break;

            if (!(kbuf = kmalloc(ioStruct.u32ReadSize, GFP_KERNEL)))
               break;

            if (AddonReadFlash( FdoData, ioStruct.u32Addr, kbuf, ioStruct.u32ReadSize ) != CS_SUCCESS)
            {
               kfree(kbuf);
               break;
            }

            if (!copy_to_user ((void __user *) ioStruct.pBuffer, kbuf, ioStruct.u32ReadSize))
               iRet = 0; /* all good */

            kfree(kbuf);
            break;
         }
         break;
         
      case CS_IOCTL_ADDON_FLASH_WRITE:
         {
            WRITE_FLASH_STRUCT  ioStruct;
            unsigned char *kbuf;

            iRet = -EFAULT;

            if (copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (WRITE_FLASH_STRUCT)))
               break;

            if (!(kbuf = kmalloc(ioStruct.u32WriteSize, GFP_KERNEL)))
               break;

            if (copy_from_user ( kbuf, (void __user *)ioStruct.pBuffer, ioStruct.u32WriteSize))
            {
               kfree(kbuf);
               break;
            }

            if (AddonWriteFlash( FdoData, ioStruct.u32Sector, ioStruct.u32Offset, kbuf, ioStruct.u32WriteSize ) == CS_SUCCESS)
               iRet = 0;

            kfree(kbuf);
         }
         break;

      case CS_IOCTL_WRITE_PORT_IO:
         {
            io_READWRITE_PCI_REG  ioStruct;
            iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_PCI_REG));

            outb( (u8) ioStruct.u32Offset, ioStruct.u32RegVal & 0xFF );
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
void	GetDeviceIdList( uInt16* u16DevIdTable, uInt32 u32TableSize )
{
	GageZeroMemory( u16DevIdTable, u32TableSize );
   if (u32TableSize < 3)
      return;

#ifdef _RABBIT_DRV_
	u16DevIdTable[0] = CS_DEVID_RABBIT;
	u16DevIdTable[1] = CS_DEVID_COBRA_PCIE;
#else
	u16DevIdTable[0] = CS_DEVID_COBRA_MAX_PCIE;
#endif
}

//#define			TRACE_DEBUG_ISR
//#define			KdPrint(x)		GEN_PRINT(x)

#define		INTR_COUNT_MASK		0x3F

static void	GageEvtInterruptDpc( ULONG p );
DECLARE_TASKLET (ISR_Tasklet, GageEvtInterruptDpc, 0);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
 irqreturn_t Isr_CsIrq(int irq, void *pData, struct pt_regs *regs)
#else
 irqreturn_t Isr_CsIrq(int irq, void *pData)
#endif
 {
	 PFDO_DATA	FdoData = 0;
   PFDO_DATA  FdoDataMaster = NULL;

	 uInt32 IntIntCtrlStatus = 0;
	 uInt32	IntFromNiosCount = 0;
	 uInt32	IntFromBoard = 0;
	 uInt32	IntEndOfTransfer = 0;

#ifdef TRACE_DEBUG_ISR
	static  uInt32	m_u32IntrCount = 0;
	char	szText[256];
#endif

#ifdef _WINODWS_
	FdoData = GageFdoGetData(WdfInterruptGetDevice(Interrupt));
#else
	FdoData = (PFDO_DATA) pData;
#endif
   FdoDataMaster = FdoData->pTriggerMaster;

   // GEN_PRINT ("RECEIVED INTERRUPT !!!!!!!!\n");

	IntIntCtrlStatus = ReadGageRegister(FdoData, INT_STATUS);
	IntFromNiosCount = ReadGageRegister(FdoData, INT_COUNT);

   //	uInt32	u32DmaReg = _ReadRegister( FdoData, PLDA_DMA_INTR_RD_INFO );
   if ( PCI_BUS == FdoData->BusType )
      IntEndOfTransfer = IsDmaInterrupt( FdoData );

   if (0 != (IntFromNiosCount & 0x80))   // interrupt Fifo full
      IntFromNiosCount = 64;
   else
      IntFromNiosCount = IntFromNiosCount & INTR_COUNT_MASK;

	IntFromBoard = IntFromNiosCount | IntEndOfTransfer;

#ifdef TRACE_DEBUG_ISR
	m_u32IntrCount++;
	if ( IntFromNiosCount > 0 )
	{
      GEN_PRINT("\n----  Received interrupt %d  ---- ", m_u32IntrCount);
      GEN_PRINT("  IntrStatus = 0x%x, IntrCount = %d, ", IntIntCtrlStatus, IntFromNiosCount);
	}
#endif

	if ( IntFromBoard )
	{
   //	GEN_PRINT ("INTERRUPT FROM CARD !!!!!!!!\n");

      if (( PCI_BUS == FdoData->BusType ) && IntEndOfTransfer)
		{
			// Acknowledge DMA interrupt
			ClearInterruptDMA0(FdoData);
			FdoData->bDmaInterrupt = TRUE;
			GEN_DEBUG_PRINT ("DMA DONE INTR  !!!!!!!!\n");
		}
		else
		{
			uInt32	IntFromAcqDone	= 0;
			uInt32	IntFromTrigger	= 0;
			uInt32	IntRecordDone = 0;
         uInt32   IntProtection = 0;
         uInt32 IntFifo = 0;
         uInt32 i;

         // Reading INT_FIFO will decrease the interrupts count
         for (i=0; i<IntFromNiosCount; ++i)
            IntFifo |= ReadGageRegister (FdoData, FdoData->IntFifoRegister);

         IntFromNiosCount = ReadGageRegister (FdoData, INT_COUNT);
         IntFromNiosCount = IntFromNiosCount & INTR_COUNT_MASK;

         IntFifo &= FdoData->u32IntBaseMask;
			IntFromAcqDone =  IntFifo & N_MBUSY_REC_DONE;
			IntFromTrigger =  IntFifo & MTRIG;
			IntRecordDone = IntFifo & RECORD_DONE;
         IntProtection   = IntFifo & MAOINT;

         if ( PCIE_BUS == FdoData->BusType )
         {
            IntEndOfTransfer = IntFifo & ALT_DMA_DONE;
            if ( IntEndOfTransfer )
            {
               // Clear status of trigger and busy that still int status register
               IntFromTrigger = IntFromAcqDone = IntRecordDone = 0;
               FdoData->bDmaInterrupt = TRUE;
               //      KdPrint(("DMA Interrupt\n"));
            }
         }
#ifdef TRACE_DEBUG_ISR
         GEN_PRINT("  IntFifo = 0x%x,", IntFifo);
#endif

         if ( 0 != IntProtection )
         {
            FdoDataMaster->u32DpcIntrStatus |= MAOINT;
            DisableInterrupts(FdoData, MAOINT);

            DisableInterrupts(FdoDataMaster, MTRIG);
            DisableInterrupts(FdoDataMaster, N_MBUSY_REC_DONE);
         }

			if (IntFromTrigger)
			{
#ifdef TRACE_DEBUG_ISR
            GEN_DEBUG_PRINT("  TRIGGERED interrupt");
#endif
				FdoData->bTriggered = TRUE;

				// Temporary fixed for problem with interrupt
				// If interrupt FIFO is full (>=64 interrupts) we will not receive any interrupt included
				// N_MBUSY_REC_DONE.
				// Semaphore for trigger scheme need to be modified later when we want to added that feature...
				DisableInterrupts(FdoData, MTRIG);
			}

         if ( 0 != (FdoData->AcqCfgEx.u32ExpertOption & CS_BBOPTIONS_MULREC_AVG_TD))
         {
            if ( 0 != (IntFifo & MULREC_AVG_DONE))
            {
               DisableInterrupts(FdoData, MULREC_AVG_DONE);
               FdoData->bEndOfBusy = TRUE;
            }
         }
         else if ( IntFromAcqDone )
         {
#ifdef TRACE_DEBUG_ISR
            GEN_DEBUG_PRINT("   END OF ACQUISITION interrupt");
#endif
            DisableInterrupts(FdoData, N_MBUSY_REC_DONE);
            FdoData->bEndOfBusy = TRUE;
            DisableInterrupts(FdoData, MTRIG);   //----- Disable the trigger anyway
         }
#ifdef CS_STREAM_SUPPORTED
         if (is_flag_set_ (&FdoData->Stream, flags, STMF_ENABLED))
         {
            // The status of DMA completed remain set until we send abort.
            // If we do not send abort Transfer here, the status DMA done may cause confusion for driver later.
            // For example, When we have FIFO_OVERLOW interrupt. the interrupt fifo register will have both bit DMA_Done and Fifo_Overlow set
            if (IntEndOfTransfer)
               AltAbortDmaTransfer (FdoData);

            if (0 != (IntFifo & STREAM_OVERFLOW))
            {
               set_flag_ (&FdoData->Stream, flags, STMF_FIFOFULL);
               DisableInterrupts (FdoData, STREAM_OVERFLOW);
            }
         }
#endif
		}
	}
	else
	{
#ifdef _WINDOWS_
		// Return FALSE to indicate that this device did not cause the interrupt.
		return FALSE;
#else
		return IRQ_NONE;
#endif
	}
   atomic_inc (&FdoData->tsk_count);
	tasklet_schedule(&ISR_Tasklet);
	return IRQ_HANDLED;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
#ifdef _WINDOWS_

VOID GageEvtInterruptDpc(IN WDFINTERRUPT WdfInterrupt, IN WDFOBJECT    WdfDevice )
{
 PFDO_DATA  FdoData = GageFdoGetData(WdfInterruptGetDevice(WdfInterrupt));

	if ( FdoData->bDmaInterrupt )
	{
		FdoData->bDmaInterrupt = FALSE;
		GageHandleDmaInterrupt( FdoData );
	}
	else
		SignalEvents( FdoData );
}

#else	// _linux_

VOID GageEvtInterruptDpc(ULONG	pIn)
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

void	GageHandleDmaInterrupt( PFDO_DATA FdoData )
{
   uInt32   u32ByteLeft = 0; // Assume all bytes has been transfered
	FdoData->XferRead.u32ByteTransfered = FdoData->XferRead.u32CurrentDmaSize - u32ByteLeft;

#ifdef TRACE_DEBUG_ISR
		GEN_PRINT ("DPC Called!!!!!!!\n");
#endif  
   
	if (0 == u32ByteLeft)
	{
#ifdef CS_STREAM_SUPPORTED
      CS_STREAM *streamp = &FdoData->Stream;
      if (test_and_clear_flag_ (streamp, flags, STMF_DMA_XFER)) {
         complete_all (&streamp->dma_done);
         return;
      }
#endif
		FdoData->bDmaIntrWait = FALSE;
		wake_up_interruptible( &FdoData->DmaWaitQueue );
	}
}

