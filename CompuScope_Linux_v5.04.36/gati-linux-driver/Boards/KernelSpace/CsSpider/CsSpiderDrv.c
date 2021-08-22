
#include "CsSpiderDrv.h"
#include "CsDefines.h"
#include "CsErrors.h"
#include "CsIoctl.h"
#include "CsIoctlTypes.h"
#include "CsDeviceIDs.h"
#include "NiosApi.h"
#if defined(_WIN32) && defined(CS_STREAM_SUPPORTED)
#include "Stream.h"
#endif

#ifdef __linux__
#define   ASSERT(x)
#endif
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32   GetPreTrigDepthLimit (PFDO_DATA FdoData,  uInt32 u32Segment)
{
   uInt32   u32PostTrig = 0;
   uInt32   u32RecLen = 0;
   uInt32   u32PreTrig = 0;
   uInt8    u8Shift = 0;
   uInt32   u32PreTrigDepthLimit = 0;
   BOOLEAN  bWrapAround = FALSE;
   uInt32   u32TriggerAddress = 0;
   uInt32   u32Mode = FdoData->AcqCfgEx.AcqCfg.u32Mode;

   if ((u32Mode & CS_MODE_QUAD) != 0)
      u8Shift = 1;
   else if ((u32Mode & CS_MODE_DUAL) != 0)
      u8Shift = 2;
   else if ((u32Mode & CS_MODE_SINGLE) != 0)
      u8Shift = 3;

   if (FdoData->CardState.bPciExpress)
      u8Shift += 1;

   ASSERT ((FdoData->AcqCfgEx.i64HwDepth >> u8Shift) < 0x100000000);
   ASSERT ((FdoData->AcqCfgEx.i64HwSegmentSize >> u8Shift) < 0x100000000);
   u32PostTrig = (uInt32) (FdoData->AcqCfgEx.i64HwDepth >> u8Shift);
   u32RecLen   = (uInt32) (FdoData->AcqCfgEx.i64HwSegmentSize >> u8Shift);

   if (u32RecLen > u32PostTrig)
      u32PreTrig = u32RecLen - u32PostTrig;
   else
      u32PreTrig = 0;

   u32TriggerAddress = GetHwTriggerAddress (FdoData, u32Segment, &bWrapAround);

   if (u32TriggerAddress >= u32PreTrig)
      u32PreTrigDepthLimit = u32PreTrig;
   else
   {
      if (bWrapAround)
         u32PreTrigDepthLimit = u32PreTrig;
      else
         u32PreTrigDepthLimit = u32TriggerAddress>>1;
   }

   // Convert to number of samples
   return (u32PreTrigDepthLimit << u8Shift);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32   GetHwTriggerAddress (PFDO_DATA FdoData, uInt32 u32Segment, BOOLEAN *bRollOver)
{
   if (FdoData->CardState.bPciExpress)
   {
      uInt32  u32Mode = FdoData->AcqCfgEx.AcqCfg.u32Mode;
      uInt8   u8Shift = 0;

      if ((u32Mode & CS_MODE_QUAD) != 0)
         u8Shift = 1;
      else if ((u32Mode & CS_MODE_DUAL) != 0)
         u8Shift = 2;
      else if ((u32Mode & CS_MODE_SINGLE) != 0)
         u8Shift = 3;

      u8Shift += 1;

      if (FdoData->AcqCfgEx.i64HwSegmentSize > FdoData->AcqCfgEx.i64HwDepth)
         FdoData->m_u32HwTrigAddr =  (uInt32) ((FdoData->AcqCfgEx.i64HwSegmentSize - FdoData->AcqCfgEx.i64HwDepth) >> u8Shift);
      else
         FdoData->m_u32HwTrigAddr = 0;
   }
   else
   {
      if (0 != (FdoData->AcqCfgEx.u32ExpertOption & CS_BBOPTIONS_MULREC_AVG_TD))
      {
         FdoData->m_u32HwTrigAddr = 0;
         FdoData->m_bMemRollOver = FALSE;
         FdoData->m_MemEtb = 0;
      }
      else if (FdoData->m_u32SegmentRead != u32Segment)
      {

         uInt32  u32TriggerAddress = ReadRegisterFromNios (FdoData, u32Segment - 1, CSPLXBASE_READ_TRIG_ADD_CMD);
         uInt32  u32TaddrAndRecs   = ReadRegisterFromNios (FdoData, u32Segment - 1, CSPLXBASE_READ_TAIL_1);      //read recs_counter and msb_ta

         FdoData->m_bMemRollOver   = ((u32TaddrAndRecs & 0x2000000) != 0);
         FdoData->m_MemEtb         = (u32TriggerAddress >> 4) & 0xF;

         u32TriggerAddress         = u32TriggerAddress >> 8;      // 128 bits resolution
         u32TaddrAndRecs           = (u32TaddrAndRecs >> 3) & 0x1f000000;
         FdoData->m_u32HwTrigAddr  = u32TriggerAddress | u32TaddrAndRecs;

         FdoData->m_u32SegmentRead = u32Segment;
      }
   }

   FdoData->m_Etb =
      FdoData->m_Stb =
      FdoData->m_SkipEtb =
      FdoData->m_AddonEtb =
      FdoData->m_ChannelSkipEtb =
      FdoData->m_DecEtb = 0;

   if (NULL != bRollOver)
      *bRollOver = FdoData->m_bMemRollOver;

   return (FdoData->m_u32HwTrigAddr);      // 256 bite resolution
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32   CalculateReadingOffset (PFDO_DATA FdoData, int64 i64StartAddressOffset, uInt32 u32Segment, uInt32 *pu32AlignOffset)
{
   uInt32  u32ReadOffset = 0;
   uInt32  u32PostTrig = 0;
   uInt32  u32RecLen = 0;
   uInt32  u32PreTrig = 0;
   int64   i64Offset = 0;
   int32   i32Offsetcopy = 0;
   uInt32  u32Offset = 0;

   uInt8   u8Shift = 0;
   uInt32  u32TriggerAddress = FdoData->m_u32HwTrigAddr;
   uInt32  u32Mode = FdoData->AcqCfgEx.AcqCfg.u32Mode;
   int64   i64TrigDelay = FdoData->AcqCfgEx.AcqCfg.i64TriggerDelay;


   if ((u32Mode & CS_MODE_QUAD) != 0)
      u8Shift = 1;
   else if ((u32Mode & CS_MODE_DUAL) != 0)
      u8Shift = 2;
   else if ((u32Mode & CS_MODE_SINGLE) != 0)
      u8Shift = 3;

   if (FdoData->CardState.bPciExpress)
      u8Shift += 1;

   u32PostTrig = (uInt32) (FdoData->AcqCfgEx.i64HwDepth >> u8Shift);
   u32RecLen = (uInt32) (FdoData->AcqCfgEx.i64HwSegmentSize >> u8Shift);

   if (u32RecLen > u32PostTrig)
      u32PreTrig = u32RecLen - u32PostTrig;
   else
      u32PreTrig = 0;

   // Compensate for trigger delay if there is any
   if (FdoData->CardState.bPciExpress || (0 != (FdoData->AcqCfgEx.u32ExpertOption & CS_BBOPTIONS_MULREC_AVG_TD)))
   {
      // In Square Rec mode , the trigger delay effect does not cause any OFFSETs in memory
      if (i64StartAddressOffset >= i64TrigDelay)
         i64StartAddressOffset -= i64TrigDelay;
   }

   // Relative offset from beginning of the segment
   i64Offset = u32PreTrig + u32RecLen;
   i64Offset <<= u8Shift;
   i64Offset += i64StartAddressOffset;

   //i64Offset = i64Offset % FdoData->AcqCfgEx.i64HwSegmentSize;
   div_s64_rem (i64Offset, FdoData->AcqCfgEx.i64HwSegmentSize, &i32Offsetcopy);
   i64Offset = i32Offsetcopy;
   i64Offset = i64Offset + FdoData->m_Etb;

   if (NULL != pu32AlignOffset)
      *pu32AlignOffset = (uInt32) (i64Offset & ((1<<u8Shift)-1));

   u32Offset = (uInt32) (i64Offset >> u8Shift);
   u32Offset = u32Offset % u32RecLen;

   // Position of Start of segment in memory
   u32ReadOffset = (u32TriggerAddress >=u32PreTrig)? (u32TriggerAddress - u32PreTrig): (u32RecLen + u32TriggerAddress - u32PreTrig);

   u32ReadOffset += u32Offset;
   u32ReadOffset %= u32RecLen;

   if (sizeof (uInt32) == FdoData->u32SampleSize)
   {
      // 32 bits samples. The offset in memory shoud by 2 times bigger
      u32ReadOffset <<= 1;
   }

   return u32ReadOffset;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void  ControlSpiClock (PFDO_DATA    FdoData, BOOLEAN bStart)
{
   uInt32   u32Ad7450CtlReg = 0;

   if (bStart)
   {
      // turn on all clocks
      u32Ad7450CtlReg &= ~CSPDR_START_SPI_CLK;
   }
   else
   {
      // turn off all clocks
      u32Ad7450CtlReg |= CSPDR_START_SPI_CLK;
   }
   WriteGIO (FdoData, CSPDR_AD7450_WRITE_REG, u32Ad7450CtlReg);
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

int32   SetupPreDataTransfer (PFDO_DATA   FdoData, uInt32 u32Segment, uInt16 u16Channel, uInt32 u32StartAddr, Pin_PREPARE_DATATRANSFER pXferConfig)
{
   FdoData->bDmaDemandMode = FALSE;

   WriteGageRegister (FdoData, MASK_REG, pXferConfig->u32DataMask);

   SetFifoMode (FdoData, pXferConfig->u32FifoMode);
   //----- Set the Segment Number
   StartTransferAtAddress (FdoData, u32Segment);

   //----- Start reading (Start Transfer DDR to FIFO)
   StartTransferDDRtoFIFO (FdoData, u32StartAddr);

   return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
VOID    CsAssignHardwareParams (PFDO_DATA FdoData)
{
#ifdef _WIN32
   ReadWriteConfigSpace (FdoData->PhysicalDevObj, 0, &FdoData->DeviceId,
                         FIELD_OFFSET (PCI_CONFIG_HEADER_0, DeviceID), sizeof (FdoData->DeviceId));
#else
   FdoData->CardState.u16DeviceId = FdoData->PciDev->device;
#endif

   FdoData->CardState.u16DeviceId = FdoData->DeviceId;
   if (FdoData->CardState.bPciExpress)
   {
      FdoData->BusType             = PCIE_BUS;
      FdoData->NumOfMemoryMap      = 2;         // NUCLEON BASEBOARD
      FdoData->IntFifoRegister     = ALT_INT_FIFO;
      FdoData->FifoDataOutRegister = ALT_FDATA_OUT;
   }
   else
   {
      FdoData->BusType             = PCI_BUS;
      FdoData->NumOfMemoryMap      = 3;
      FdoData->IntFifoRegister     = INT_FIFO;
      FdoData->FifoDataOutRegister = FDATA_OUT;
   }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
VOID    CsAssignRegisterBaseAddress (PFDO_DATA FdoData)
{
   if (FdoData->CardState.bPciExpress)
   {
      FdoData->PciRegisterBase   = FdoData->MapAddress[1];
      FdoData->GageRegisterBase  = (PUCHAR) FdoData->MapAddress[1] + 0x80;
      FdoData->FlashRegisterBase = (PUCHAR) FdoData->MapAddress[1] + 0xC4;
   }
   else
   {
      FdoData->PciRegisterBase   = FdoData->MapAddress[0];
      FdoData->GageRegisterBase  = FdoData->MapAddress[1];
      FdoData->FlashRegisterBase = FdoData->MapAddress[2];
   }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
VOID    CsAssignFunctionPointers (PFDO_DATA FdoData)
{
   if (FdoData->CardState.bPciExpress)
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
   else
   {
      FdoData->pfnStartDmaTransferDescriptor         = StartDmaTransferDescriptor;
      FdoData->pfnBuildDmaTransferDescriptor         = BuildDmaTransferDescriptor;
      FdoData->pfnAbortDmaTransfer                   = AbortDmaTransfer;
      FdoData->pfnEnableInterruptDMA                 = EnableInterruptDMA0;
      FdoData->pfnDisableInterruptDMA                = DisableInterruptDMA0;
      FdoData->pfnCheckBlockTransferDone             = CheckBlockTransferDone;

      FdoData->pfnGageFreeMemoryForDmaDescriptor     = GageFreeMemoryForDmaDescriptor;
      FdoData->pfnGageAllocateMemoryForDmaDescriptor = GageAllocateMemoryForDmaDescriptor;
   }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32   SpiderAcquire (PFDO_DATA pMaster, Pin_STARTACQUISITION_PARAMS pAcqParams  )
{
#ifdef CS_STREAM_SUPPORTED
   PCS_STREAM  pStream = &pMaster->Stream;
#endif
   PFDO_DATA   pCard = pMaster;
   int32       i32Status = CS_SUCCESS;
#ifdef _WIN32
   HANDLE      hProcess = IoGetCurrentProcess ();
#endif

   // Fixed prefast warnings
   if (NULL == pMaster || NULL == pAcqParams)
      return CS_MISC_ERROR;

   while (pCard)
   {
#ifdef _WIN32
      pCard->hCaptureProcess = hProcess;
#endif
      pCard = pCard->pNextSlave;
   }
#ifdef CS_STREAM_SUPPORTED
   // If u32Params1 == 0 then we are in calibration mode.
   // In this case we have to perform a normal acquisition for calibration
#ifdef _WIN32
   if (pStream->bEnabled && pAcqParams->u32Param1 != 0)
#else
   if (is_flag_set_ (pStream, flags, STMF_ENABLED) && pAcqParams->u32Param1 != 0)
#endif
   {
      pCard = pMaster->pNextSlave;
      while (pCard)
      {
#ifdef _WIN32
         i32Status = StmStartCapture (pCard);
#else
         i32Status = stm_start_capture (pCard, AcqCmdMemory);
#endif
         if (CS_FAILED (i32Status))
            break;
         pCard = pCard->pNextSlave;
      }

      if (CS_SUCCEEDED (i32Status))
#ifdef _WIN32
         i32Status = StmStartCapture (pMaster);
#else
         i32Status = stm_start_capture (pMaster, AcqCmdMemory);
#endif
   }
   else
#endif
      i32Status = SendStartAcquisition (pMaster, pAcqParams);

   return i32Status;
}
