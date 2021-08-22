
#include "CsDefines.h"
#include "CsErrors.h"
#include "CsIoctl.h"
#include "CsIoctlTypes.h"
#include "HexagonDrv.h"
#include "HexagonNiosApi.h"

#define  HEXAGON_FDATA_OUT 0xD0
#define  SHIFT_DUAL       3      // 8 Samples
#define  SHIFT_SINGLE     4      // 16 Samples

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 GetPreTrigDepthLimit (PFDO_DATA FdoData,  uInt32 u32Segment)
{
   uInt32  u32PostTrig = 0;
   uInt32  u32RecLen = 0;
   uInt32  u32PreTrig = 0;
   uInt8   u8Shift = 0;
   uInt32  u32PreTrigDepthLimit = 0;
   BOOLEAN bWrapAround = FALSE;
   uInt32  u32TriggerAddress = 0;
   uInt32  u32Mode = FdoData->AcqCfgEx.AcqCfg.u32Mode;

   if ((u32Mode & CS_MODE_DUAL) != 0)
      u8Shift = SHIFT_DUAL;
   else
      u8Shift = SHIFT_SINGLE;

   u8Shift += 1;

#if 0
   ASSERT ((FdoData->AcqCfgEx.i64HwDepth >> u8Shift) < 0x100000000);
   ASSERT ((FdoData->AcqCfgEx.i64HwSegmentSize >> u8Shift) < 0x100000000);
#endif

   u32PostTrig = (uInt32) (FdoData->AcqCfgEx.i64HwDepth >> u8Shift);
   u32RecLen   = (uInt32) (FdoData->AcqCfgEx.i64HwSegmentSize >> u8Shift);

   if (u32RecLen  > u32PostTrig)
      u32PreTrig = u32RecLen - u32PostTrig;
   else
      u32PreTrig = 0;

   u32TriggerAddress = GetHwTriggerAddress (FdoData, u32Segment, &bWrapAround);

   if (u32TriggerAddress >= u32PreTrig)
      u32PreTrigDepthLimit = u32PreTrig;
   else {
      if (bWrapAround)
         u32PreTrigDepthLimit = u32PreTrig;
      else
         u32PreTrigDepthLimit = u32TriggerAddress;
   }

   // Convert to number of samples
   return (u32PreTrigDepthLimit << u8Shift);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 GetHwTriggerAddress (PFDO_DATA FdoData, uInt32 u32Segment, BOOLEAN *bRollOver)
{
   uInt32 u32Mode = FdoData->AcqCfgEx.AcqCfg.u32Mode;
   uInt8  u8Shift = 0;

   if ((u32Mode & CS_MODE_DUAL) != 0)
      u8Shift = 4;      // 16   samples
   else if ((u32Mode & CS_MODE_SINGLE) != 0)
      u8Shift = 5;      // 32   samples

   if (FdoData->AcqCfgEx.i64HwSegmentSize > FdoData->AcqCfgEx.i64HwDepth)
      FdoData->m_u32HwTrigAddr = (uInt32) ((FdoData->AcqCfgEx.i64HwSegmentSize - FdoData->AcqCfgEx.i64HwDepth) >> u8Shift);
   else
      FdoData->m_u32HwTrigAddr = 0;

   FdoData->m_Stb =
      FdoData->m_SkipEtb =
      FdoData->m_AddonEtb =
      FdoData->m_ChannelSkipEtb =
      FdoData->m_MemEtb =
      FdoData->m_DecEtb = 0;

   return (FdoData->m_u32HwTrigAddr);
}

//-----------------------------------------------------------------------------
// This function calculate and convert the i64StartAddressOffset
// to the memory location
//-----------------------------------------------------------------------------
uInt32   CalculateReadingOffset (PFDO_DATA FdoData, int64 i64StartAddressOffset, uInt32 u32Segment, uInt32 *pu32AlignOffset)
{
   uInt32  u32ReadOffsetLocation = 0;
   uInt32  u32ReadOffset = 0;
   /* uInt32  u32PostTrig = 0; */
   /* uInt32  u32RecLen = 0; */
   /* uInt32  u32PreTrig = 0; */
   /* int64   i64Offset = 0; */
   uInt32  u32Offset = 0;
   uInt8   u8Shift = 0;
   uInt8   u8MemUnit = 0;

   /* uInt32  u32TriggerAddress = FdoData->m_u32HwTrigAddr; */
   uInt32  u32Mode = FdoData->AcqCfgEx.AcqCfg.u32Mode;
   int64   i64TrigDelay = FdoData->AcqCfgEx.AcqCfg.i64TriggerDelay;

	switch( u32Mode & CS_MASKED_MODE )
	{
		case CS_MODE_QUAD:
			u8Shift = 3;		// 16 samples unit
			u8MemUnit	= 8;
			break;
		case CS_MODE_DUAL:
			u8Shift = 4;		// 16 samples unit
			u8MemUnit	= 16;
			break;
		case CS_MODE_SINGLE:
			u8Shift = 5;		// 32 samples uint
			u8MemUnit	= 32;
			break;
	}
   
   // Compensate for trigger delay if there is any
   // In Square Rec mode , the trigger delay effect does not cause any OFFSETs in memory
   if (i64StartAddressOffset >= i64TrigDelay)
      i64StartAddressOffset -= i64TrigDelay;

   // Relative offset from beginning of the segment
   u32ReadOffset = (uInt32) (i64StartAddressOffset + FdoData->AcqCfgEx.i64HwSegmentSize - FdoData->AcqCfgEx.i64HwDepth);

   u32ReadOffsetLocation   = u32ReadOffset / u8MemUnit;
   u32Offset      = (uInt32) (u32ReadOffset - (u32ReadOffsetLocation * u8MemUnit));

   if (NULL != pu32AlignOffset)
      *pu32AlignOffset = u32Offset;

   return u32ReadOffsetLocation;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void  IntrDMAInitialize (PFDO_DATA FdoData)
{
   //FdoData->u32RegCSR = _ReadRegister (FdoData, PLDA_DMA_INTR_RD_INFO);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 SetupPreDataTransfer (PFDO_DATA   FdoData, uInt32 u32Segment, uInt16 u16Channel, uInt32 u32StartAddr, Pin_PREPARE_DATATRANSFER pXferConfig)
{
   /* uInt32   u32UserRegVal = 0; */

   FdoData->bDmaDemandMode = FALSE;

   WriteGageRegister (FdoData, MASK_REG, pXferConfig->u32DataMask);

   //----- Set the Segment Number
   StartTransferAtAddress (FdoData, u32Segment);

   SetFifoMode (FdoData, pXferConfig->u32FifoMode);

   //----- Start reading (Start Transfer DDR to FIFO)
   StartTransferDDRtoFIFO (FdoData, u32StartAddr);

   return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
VOID CsAssignHardwareParams (PFDO_DATA FdoData)
{
   FdoData->m_pCardState->u16DeviceId = FdoData->PciDev->device;
   FdoData->BusType               = PCIE_BUS;
   FdoData->NumOfMemoryMap        = 2;
   FdoData->IntFifoRegister       = ALT_INT_FIFO;
   FdoData->FifoDataOutRegister   = HEXAGON_FDATA_OUT;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
VOID CsAssignRegisterBaseAddress (PFDO_DATA FdoData)
{
   FdoData->PciRegisterBase   = FdoData->MapAddress[1];
   FdoData->GageRegisterBase  = FdoData->MapAddress[1];
   FdoData->FlashRegisterBase = (PUCHAR)FdoData->MapAddress[1] + 0xC4;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
VOID CsAssignFunctionPointers( PFDO_DATA FdoData )
{
   FdoData->pfnStartDmaTransferDescriptor         = AltStartDmaTransferDescriptor;
   FdoData->pfnBuildDmaTransferDescriptor         = AltBuildDmaTransferDescriptor;
   FdoData->pfnAbortDmaTransfer                   = AltAbortDmaTransfer;
   FdoData->pfnEnableInterruptDMA                 = AltEnableInterruptDMA;
   FdoData->pfnDisableInterruptDMA                = AltDisableInterruptDMA;
   FdoData->pfnCheckBlockTransferDone             = AltCheckBlockTransferDone;

   FdoData->pfnGageFreeMemoryForDmaDescriptor     = AltGageFreeMemoryForDmaDescriptor;
   FdoData->pfnGageAllocateMemoryForDmaDescriptor = AltGageAllocateMemoryForDmaDescriptor;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32 StartAcquire (PFDO_DATA pMaster, Pin_STARTACQUISITION_PARAMS pAcqParams)
{
#ifdef CS_STREAM_SUPPORTED
   PCS_STREAM  pStream = &pMaster->Stream;
#endif
   PFDO_DATA pCard = pMaster;
   int32     i32Status = CS_SUCCESS;
#if 0
   HANDLE    hProcess = IoGetCurrentProcess();
#endif

   // Fixed prefast warnings
   if (NULL == pMaster || NULL == pAcqParams)
      return CS_MISC_ERROR;

   // Remember the process that sent the Start acquisition request
   while (pCard) {
#if 0
      pCard->hCaptureProcess = hProcess;
#endif
      pCard = pCard->pNextSlave;
   }
   pMaster->m_pCardState->u32AcqMode = pAcqParams->u32Param1;
   // If u32Params1 == 0 then we are in calibration mode.
   // In this case we have to perform a normal acquisition for calibration
#if 0
   if (pStream->bEnabled && pAcqParams->u32Param1 >= AcqCmdStreaming) {
#endif
   if (is_flag_set_ (pStream, flags, STMF_ENABLED) && pAcqParams->u32Param1 >= AcqCmdStreaming) {
      // Acqusition in Streaming mode
      pCard = pMaster->pNextSlave;
      while (pCard) {
#if 0
         i32Status = StmStartCapture (pCard);
#endif
         i32Status = stm_start_capture (pCard, pAcqParams->u32Param1);
         if (CS_FAILED (i32Status))
            break;
         pCard = pCard->pNextSlave;
      }
      if (CS_SUCCEEDED (i32Status))
#if 0
         i32Status = StmStartCapture (pMaster);
#endif
         i32Status = stm_start_capture (pMaster, pAcqParams->u32Param1);
   }
   else {
      // Acqusition in Memory mode
      pMaster->m_pCardState->u32IntConfig = pAcqParams->u32IntrMask;
      //   if (pMaster->m_pCardState->u32IntConfig != pMaster->u32IntBaseMask)
      {
         ClearInterrupts (pMaster);
         ConfigureInterrupts (pMaster, pAcqParams->u32IntrMask);
      }
      pMaster->pTriggerMaster->u32DpcIntrStatus = 0;
      pMaster->bEndOfBusy = FALSE;
      pMaster->bTriggered = FALSE;
#if 0
      pMaster->hCaptureProcess  = hProcess;
#endif
      pMaster->m_u32SegmentRead = 0;
      if (pAcqParams->u32Param1 == AcqCmdMemory_Average) {
         i32Status = SEND_NIOS_COMMAND (pMaster, 0, 0x400000);
      }
      else if (pAcqParams->u32Param1 == AcqCmdMemory_OCT) {
         i32Status = SEND_NIOS_COMMAND (pMaster, 0, 0x1500000);
      }
      else {
         i32Status = StartAcquisition (pMaster, 0);
      }
   }
   return i32Status;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
#define io_read_enable_bit   0x0200000
#define io_write_strobe_bit   0x0100000
#define io_address_mask   0x00F0000
#define io_ready_bit   0x0010000

uInt32 ReadCpldRegister (PFDO_DATA FdoData, uInt32 u32Addr)
{
   uInt32 u32Ret = ReadGageRegister (FdoData, CPLD_REG_OFFSET);
   uInt32 u32Count = 0;
   while (! (u32Ret & io_ready_bit)) {
      u32Ret = ReadGageRegister (FdoData, CPLD_REG_OFFSET);
      if (u32Count++ > 40)
         break;
   }

   WriteGageRegister (FdoData, CPLD_REG_OFFSET, ((u32Addr & 0xf) << 16)| io_read_enable_bit);

   u32Ret = ReadGageRegister (FdoData, CPLD_REG_OFFSET);
   u32Count = 0;
   while (! (u32Ret & io_ready_bit)) {
      u32Ret = ReadGageRegister (FdoData, CPLD_REG_OFFSET);
      if (u32Count++ > 40)
         break;
   }
   /*
      if (u32Count > 40)
      GEN_DEBUG_PRINT (("READ_GIO_CPLD - Error at addr: (0x%x), Command: (0x%x)\n", u32Addr, u32Ret));
      else
      GEN_DEBUG_PRINT (("READ_GIO_CPLD - Cpld offset: (0x%x), data: (0x%x)\n", u32Addr, u32Ret));
      */
   return (u32Ret &0xFFFF);
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void WriteCpldRegister (PFDO_DATA FdoData, uInt32 u32Addr, uInt32 u32Value)
{
   uInt32 u32Ret = ReadGageRegister (FdoData, CPLD_REG_OFFSET);
   uInt32 u32Count = 0;
   while (! (u32Ret & io_ready_bit)) {
      u32Ret = ReadGageRegister (FdoData, CPLD_REG_OFFSET);
      if (u32Count++ > 40)
         break;
   }

   WriteGageRegister (FdoData, CPLD_REG_OFFSET, (u32Value & 0xffff)| ((u32Addr & 0xf) << 16)| io_write_strobe_bit);

   u32Ret = ReadGageRegister (FdoData, CPLD_REG_OFFSET);
   u32Count = 0;
   while (! (u32Ret & io_ready_bit)) {
      u32Ret = ReadGageRegister (FdoData, CPLD_REG_OFFSET);
      if (u32Count++ > 40)
         break;
   }
}

