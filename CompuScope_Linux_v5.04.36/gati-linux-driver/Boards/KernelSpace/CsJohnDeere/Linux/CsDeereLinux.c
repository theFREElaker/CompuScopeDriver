#include <linux/interrupt.h>
#ifndef __x86_64__
   #include <linux/sched.h>
#endif

#include "CsDeereDrv.h"
#include "CsTypes.h"
#include "CsDeviceIDs.h"
#include "GageWrapper.h"

#include "CsDefines.h"
#include "CsErrors.h"
#include "CsIoctl.h"
#include "NiosApi.h"
#include "PlxSupport.h"
#include "CsLinuxSignals.h"
#include "CsDeereFlash.h"
#include "CsDeereFlashObj.h"

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
			Abort( FdoData );
			DeereAcquire( FdoData->pMasterIndependent, &StartAcqParams );
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
	case CS_IOCTL_FLASH_READ:
		{
			READ_FLASH_STRUCT ioStruct;
			unsigned char *kbuf;

			iRet = -EFAULT;

			if (copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (READ_FLASH_STRUCT)))
				break;

			if (ioStruct.u32ReadSize > DEERE_FLASH_SIZE)
				break;

			if (!(kbuf = kmalloc(ioStruct.u32ReadSize, GFP_KERNEL)))
				break;

			if (FlashRead( FdoData, ioStruct.u32Addr, kbuf, ioStruct.u32ReadSize ) != CS_SUCCESS)
			{
				kfree(kbuf);
				break;
			}

			if (!copy_to_user ((void __user *) ioStruct.pBuffer, kbuf, ioStruct.u32ReadSize))
				iRet = 0; /* all good */

			kfree(kbuf);
			break;
		}
	case CS_IOCTL_FLASH_WRITE:
		{
			WRITE_FLASH_STRUCT	ioStruct;
			unsigned char *kbuf;

			iRet = -EFAULT;

			if (copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (WRITE_FLASH_STRUCT)))
				break;

			if (ioStruct.u32WriteSize > DEERE_INTEL_FLASH_SECTOR_SIZE)
				break;
			
			if (!(kbuf = kmalloc(ioStruct.u32WriteSize, GFP_KERNEL)))
				break;

			if (copy_from_user ( kbuf, (void __user *)ioStruct.pBuffer, ioStruct.u32WriteSize))
			{
				kfree(kbuf);
				break;
			}
			if (FlashWrite( FdoData, ioStruct.u32Sector, ioStruct.u32Offset, kbuf, ioStruct.u32WriteSize ) == CS_SUCCESS)
				iRet = 0; /* all good */

			kfree(kbuf);
			break;
		}
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

//		GEN_DEBUG_PRINT ("FDO Data = 0x%lx\n", FdoData );
		GEN_DEBUG_PRINT ("FDO Data = %p\n", FdoData );
		GEN_DEBUG_PRINT ("InParams Channel = %d\n", pInTransferEx->u16Channel );
		GEN_DEBUG_PRINT ("InParams DataMask = %d\n", pInTransferEx->u32DataMask );

		FdoData->bDataTransferIntrEn	= pInTransferEx->bIntrEnable;
		FdoData->bDataTransferBusMaster = pInTransferEx->bBusMasterDma;
		FdoData->u32SampleSize			= pInTransferEx->u32SampleSize;

		if ( pInTransferEx->u32ReadCount != 0 )
		{
			SetupPreDataTransferEx( FdoData, pInTransferEx->u32Segment, (uInt32) pInTransferEx->i64StartAddr, 
									pInTransferEx->u32ReadCount, pInTransferEx->u32SkipCount, pInTransferEx );
		}
		else if ( pOutTransferEx != 0)
		{
			i64PreDepthLimit = GetPreTrigDepthLimit ( FdoData,  pInTransferEx->u32Segment );
			i64PreDepthLimit *= -1;

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

			u32LocalReadingOffset = CalculateReadingOffset( FdoData, i64StartAddress, pInTransferEx->u32Segment, &u32LocalAlignOffset );

			SetupPreDataTransfer( FdoData, pInTransferEx->u32Segment,  pInTransferEx->u16Channel, u32LocalReadingOffset, pInTransferEx );

			pOutTransferEx->i32ActualStartOffset = u32LocalAlignOffset + FdoData->m_Etb;
			pOutTransferEx->i32ActualStartOffset *= -1;

			pOutTransferEx->u32LocalAlignOffset			= u32LocalAlignOffset;
			pOutTransferEx->u32LocalReadingOffset		= FdoData->u32ReadingOffset = u32LocalReadingOffset;

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
		}

		iRet = copy_to_user ( (void __user *)&temp->OutParams, &DataXferParams.OutParams, sizeof (out_PREPARE_DATATRANSFER));

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
	u16DevIdTable[0] = CS_DEVID_JOHNDEERE;
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

//    GEN_PRINT ("RECIEVED INTERRUPT !!!!!!!!\n");

	IntIntCtrlStatus = ReadGageRegister(FdoData, INT_STATUS);
	IntFromNiosCount = ReadGageRegister(FdoData, INT_COUNT);


//	uInt32	u32DmaReg = _ReadRegister( FdoData, PLDA_DMA_INTR_RD_INFO );
	IntEndOfTransfer = IsDmaInterrupt( FdoData );
	IntFromNiosCount = IntFromNiosCount & INTR_COUNT_MASK;

	IntFromBoard = IntFromNiosCount | IntEndOfTransfer;

#ifdef TRACE_DEBUG_ISR
	m_u32IntrCount++;
	if ( IntFromNiosCount > 0 )
	{
		sprintf(szText, "\n----   Received interrupt %d  ---- ", m_u32IntrCount);
		KdPrint((szText));
		sprintf(szText, "   IntrStatus = 0x%x, IntrCount = %d, ", IntIntCtrlStatus, IntFromNiosCount);
		KdPrint((szText));
	}
#endif

	if ( IntFromBoard )
	{
//		GEN_PRINT ("INTERRUPT FROM CARD !!!!!!!!\n");

		if (IntEndOfTransfer)
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

			// Reading INT_FIFO will decrease the interrupts count
			uInt32	IntFifo =  ReadGageRegister(FdoData, INT_FIFO);

			IntFromAcqDone =  IntFifo & N_MBUSY_REC_DONE;
			IntFromTrigger =  IntFifo & MTRIG;
			IntRecordDone = IntFifo & RECORD_DONE;

#ifdef TRACE_DEBUG_ISR
			sprintf(szText, "   IntFifo = 0x%x,", IntFifo);
			KdPrint((szText));
#endif

			if ( 0 != (IntFifo & MAOINT) )
			{
				FdoData->u32DpcIntrStatus |= MAOINT;
				DisableInterrupts(FdoData, MAOINT);
#ifdef TRACE_DEBUG_ISR
				KdPrint(("ALARM interrupt\n"));
#endif
			}
			
			if (IntFromTrigger)
			{
#ifdef TRACE_DEBUG_ISR
				KdPrint(("   TRIGGERED interrupt"));
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
	//				m_CsBoard.DpcIntrStatus |= MULREC_AVG_DONE;
					DisableInterrupts(FdoData, MULREC_AVG_DONE);
					FdoData->bEndOfBusy = TRUE;
				}
			}
			else if ( IntFromAcqDone )
			{
#ifdef TRACE_DEBUG_ISR
				KdPrint(("   END OF ACQUISITION interrupt"));
#endif
				DisableInterrupts(FdoData, N_MBUSY_REC_DONE);
				FdoData->bEndOfBusy = TRUE;
				DisableInterrupts(FdoData, MTRIG);		//----- Disable the trigger anyway
			}
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





#ifdef _WINDOWS_
	// Request deferred procedure call
	WdfInterruptQueueDpcForIsr( Interrupt );
	// Return TRUE to indicate that our device caused the interrupt
	return TRUE;

#else
	ISR_Tasklet.data = (ULONG) FdoData;
	tasklet_schedule(&ISR_Tasklet);
	return IRQ_HANDLED;
#endif


}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
#ifdef _WINDOWS_

VOID GageEvtInterruptDpc(IN WDFINTERRUPT WdfInterrupt, IN WDFOBJECT    WdfDevice )
{
    PFDO_DATA  FdoData = GageFdoGetData(WdfInterruptGetDevice(WdfInterrupt));
	
	if ( 0 != (FdoData->u32DpcIntrStatus & MAOINT) )
	{
		// Alarm interrupt. Channel protection activated. Read the actual segment size captured so far ...
		FdoData->pMasterIndependent->CardState.u32ActualSegmentCount = ReadRegisterFromNios( FdoData->pMasterIndependent, 0, CSPLXBASE_READ_REC_COUNT_CMD ) & PLX_REC_COUNT_MASK;
		Abort( FdoData->pTriggerMaster );
//		if( ! FdoData->pMasterIndependent->bEndOfBusy )
//		{
//			DisableInterrupts(FdoData->pMasterIndependent, N_MBUSY_REC_DONE);
//			DisableInterrupts(FdoData->pMasterIndependent, MTRIG);
//		}
	}

	if ( FdoData->bDmaInterrupt )
	{
		FdoData->bDmaInterrupt = FALSE;

	    WdfSpinLockAcquire(FdoData->DmaLock);
		GageHandleDmaInterrupt( FdoData );
		WdfSpinLockRelease(FdoData->DmaLock);
	}
	else
		SignalEvents( FdoData );
}

#else	// _linux_

VOID GageEvtInterruptDpc(ULONG	pIn)
{
	PFDO_DATA  FdoData = (PFDO_DATA) pIn;

	if ( 0 != (FdoData->u32DpcIntrStatus & MAOINT) )
	{
		// Alarm interrupt. Channel protection activated. Read the actual segment size captured so far ...
		FdoData->pMasterIndependent->CardState.u32ActualSegmentCount = ReadRegisterFromNios( FdoData->pMasterIndependent, 0, CSPLXBASE_READ_REC_COUNT_CMD ) & PLX_REC_COUNT_MASK;
		Abort( FdoData->pTriggerMaster );
//		if( ! FdoData->pMasterIndependent->bEndOfBusy )
//		{
//			DisableInterrupts(FdoData->pMasterIndependent, N_MBUSY_REC_DONE);
//			DisableInterrupts(FdoData->pMasterIndependent, MTRIG);
//		}
	}

	if ( FdoData->bDmaInterrupt )
	{
		FdoData->bDmaInterrupt = FALSE;
		GageHandleDmaInterrupt( FdoData );
	}
	else
	{
		GEN_DEBUG_PRINT ("DPC Called!!!!!!!\n");
		SignalEvents( FdoData );
	}
}

#endif



//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void	GageHandleDmaInterrupt( PFDO_DATA FdoData )
{
	uInt32 u32ByteLeft = 0;	//_ReadRegister( FdoData, PLDA_DMA_WR_SIZE );
	FdoData->XferRead.u32ByteTransfered = FdoData->XferRead.u32CurrentDmaSize - u32ByteLeft;
	GEN_DEBUG_PRINT("DmaCount remain = %d\n", u32ByteLeft);

	if (0 == u32ByteLeft)
	{
		FdoData->bDmaIntrWait = FALSE;
		wake_up_interruptible( &FdoData->DmaWaitQueue );
	}
}
