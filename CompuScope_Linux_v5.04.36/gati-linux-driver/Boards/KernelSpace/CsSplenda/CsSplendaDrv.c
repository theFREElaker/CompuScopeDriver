#include "CsSplendaDrv.h"
#include "NiosApi.h"
#include "CsIoctl.h"
#include "CsTimer.h"
#ifdef _WIN32
#  include "CsDeviceIds.h"
#  ifdef CS_STREAM_SUPPORTED
#     include "Stream.h"
#  endif
#endif

#ifdef __linux__
#define   ASSERT(x)
#endif
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------


uInt32   GetPreTrigDepthLimit ( PFDO_DATA FdoData,  uInt32 u32Segment )
{
   uInt32   u32PostTrig = 0;
   uInt32   u32RecLen = 0;
   uInt32   u32PreTrig = 0;
   uInt8   u8Shift = 0;
   uInt32   u32PreTrigDepthLimit = 0;
   BOOLEAN   bWrapAround = FALSE;
   uInt32   u32TriggerAddress = 0;
   uInt32  u32Mode = FdoData->AcqCfgEx.AcqCfg.u32Mode;

   if ((u32Mode & CS_MODE_QUAD) != 0)
      u8Shift = 1;
   else if ((u32Mode & CS_MODE_DUAL) != 0)
      u8Shift = 2;
   else if ((u32Mode & CS_MODE_SINGLE) != 0)
      u8Shift = 3;

   if ( FdoData->CardState.bPciExpress )
      u8Shift += 1;

   ASSERT( (FdoData->AcqCfgEx.i64HwDepth >> u8Shift) < 0x100000000 );
   ASSERT( (FdoData->AcqCfgEx.i64HwSegmentSize >> u8Shift) < 0x100000000 );
   u32PostTrig   = (uInt32) (FdoData->AcqCfgEx.i64HwDepth >> u8Shift);
   u32RecLen   = (uInt32) (FdoData->AcqCfgEx.i64HwSegmentSize >> u8Shift);

   if ( u32RecLen > u32PostTrig )
      u32PreTrig = u32RecLen - u32PostTrig;
   else
      u32PreTrig = 0;

   u32TriggerAddress = GetHwTriggerAddress( FdoData, u32Segment, &bWrapAround );

   if ( u32TriggerAddress >= u32PreTrig )
      u32PreTrigDepthLimit = u32PreTrig;
   else
   {
      if ( bWrapAround )
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

uInt32   GetHwTriggerAddress( PFDO_DATA FdoData, uInt32 u32Segment, BOOLEAN *bRollOver )
{
   if ( FdoData->CardState.bPciExpress )
   {
      uInt32   u32Mode = FdoData->AcqCfgEx.AcqCfg.u32Mode;
      uInt8   u8Shift = 0;

      if ((u32Mode & CS_MODE_QUAD) != 0)
         u8Shift = 1;
      else if ((u32Mode & CS_MODE_DUAL) != 0)
         u8Shift = 2;
      else if ((u32Mode & CS_MODE_SINGLE) != 0)
         u8Shift = 3;

      u8Shift += 1;

      if ( FdoData->AcqCfgEx.i64HwSegmentSize > FdoData->AcqCfgEx.i64HwDepth )
         FdoData->m_u32HwTrigAddr =  (uInt32) ( (FdoData->AcqCfgEx.i64HwSegmentSize - FdoData->AcqCfgEx.i64HwDepth) >> u8Shift );
      else
         FdoData->m_u32HwTrigAddr = 0;
   }
   else if ( 0 != (FdoData->AcqCfgEx.u32ExpertOption & CS_BBOPTIONS_MULREC_AVG_TD) )
   {
      FdoData->m_u32HwTrigAddr = 0;
      FdoData->m_Etb = 0;
      FdoData->m_bMemRollOver = FALSE;
   }
   else if ( FdoData->m_u32SegmentRead != u32Segment )
   {
      uInt32   u32TriggerAddress   = ReadRegisterFromNios( FdoData, u32Segment - 1, CSPLXBASE_READ_TRIG_ADD_CMD);
      uInt32   u32TaddrAndRecs   = ReadRegisterFromNios( FdoData, u32Segment - 1, CSPLXBASE_READ_TAIL_1 );   //read recs_counter and msb_ta

      FdoData->m_bMemRollOver = ( (u32TaddrAndRecs & 0x2000000) != 0 );
      FdoData->m_Etb    = u32TriggerAddress & 0xF;
      FdoData->m_Etb   += ((u32TriggerAddress >> 4) & 0xF);

      u32TriggerAddress   = u32TriggerAddress >> 8;   // 128 bits resolution
      u32TaddrAndRecs   = (u32TaddrAndRecs >> 3) & 0x1f000000;
      FdoData->m_u32HwTrigAddr = u32TriggerAddress | u32TaddrAndRecs;

      FdoData->m_u32SegmentRead = u32Segment;
   }

   FdoData->m_DecEtb   =
      FdoData->m_MemEtb    =
      FdoData->m_Stb    =
      FdoData->m_SkipEtb    =
      FdoData->m_AddonEtb   =
      FdoData->m_ChannelSkipEtb   = 0;


   if ( NULL != bRollOver )
      *bRollOver = FdoData->m_bMemRollOver;

   return (FdoData->m_u32HwTrigAddr);   // 256 bite resolution
}


#define FORCE_READ_EVEN_ADDRESS   // Patch for firmware bug
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32   CalculateReadingOffset(PFDO_DATA FdoData, int64 i64StartAddressOffset, uInt32 u32Segment, uInt32 *pu32AlignOffset )
{
   uInt32   u32ReadOffset = 0;
   uInt32   u32PostTrig = 0;
   uInt32   u32RecLen = 0;
   uInt32   u32PreTrig = 0;
   int64   i64Offset = 0;
   int32   i32Offsetcopy = 0;
   uInt32   u32Offset = 0;

   uInt8   u8Shift = 0;
   uInt32   u32TriggerAddress = FdoData->m_u32HwTrigAddr;
   uInt32   u32Mode = FdoData->AcqCfgEx.AcqCfg.u32Mode;
   int64   i64TrigDelay = FdoData->AcqCfgEx.AcqCfg.i64TriggerDelay;

   if ((u32Mode & CS_MODE_QUAD) != 0)
      u8Shift = 1;
   else if ((u32Mode & CS_MODE_DUAL) != 0)
      u8Shift = 2;
   else if ((u32Mode & CS_MODE_SINGLE) != 0)
      u8Shift = 3;

   if ( FdoData->CardState.bPciExpress )
   {
      u8Shift += 1;
#ifdef FORCE_READ_EVEN_ADDRESS
      u8Shift += 1;   // firmware bug : Cannot read from Odd address of memory location
#endif
   }

   u32PostTrig = (uInt32) (FdoData->AcqCfgEx.i64HwDepth >> u8Shift);
   u32RecLen = (uInt32)(FdoData->AcqCfgEx.i64HwSegmentSize >> u8Shift);

   if ( u32RecLen > u32PostTrig )
      u32PreTrig = u32RecLen - u32PostTrig;
   else
      u32PreTrig = 0;

   // Compensate for trigger delay if there is any
   if ( FdoData->CardState.bPciExpress || (0 != (FdoData->AcqCfgEx.u32ExpertOption & CS_BBOPTIONS_MULREC_AVG_TD)) )
   {
      // In Square Rec mode , the trigger delay effect does not cause any OFFSETs in memory
      if ( i64StartAddressOffset >= i64TrigDelay )
         i64StartAddressOffset -= i64TrigDelay;
   }

   // Relative offset from beginning of the segment
   i64Offset = u32PreTrig + u32RecLen;
   i64Offset <<= u8Shift;
   i64Offset += i64StartAddressOffset;

   //i64Offset = i64Offset % FdoData->AcqCfgEx.i64HwSegmentSize;
   div_s64_rem (i64Offset, FdoData->AcqCfgEx.i64HwSegmentSize, &i32Offsetcopy);
   i64Offset = i32Offsetcopy;

   if( NULL != pu32AlignOffset )
      *pu32AlignOffset = (uInt32) ( i64Offset & ((1<<u8Shift)-1) );

   u32Offset = (uInt32) (i64Offset >> u8Shift);
   u32Offset = u32Offset % u32RecLen;

#ifdef FORCE_READ_EVEN_ADDRESS
   // Converting trigger address to the same unit count as u32PreTrig
   if ( FdoData->CardState.bPciExpress )
      u32TriggerAddress >>= 1;
#endif

   // Position of Start of segment in memory
   u32ReadOffset = (u32TriggerAddress >=u32PreTrig)?(u32TriggerAddress - u32PreTrig):(u32RecLen + u32TriggerAddress - u32PreTrig);

   u32ReadOffset += u32Offset;
   u32ReadOffset %= u32RecLen;

   if ( sizeof(uInt32) == FdoData->u32SampleSize )
   {
      // 32 bits samples. The offset in memory shoud by 2 times bigger
      u32ReadOffset <<= 1;
   }

#ifdef FORCE_READ_EVEN_ADDRESS
   if ( FdoData->CardState.bPciExpress )
      u32ReadOffset <<= 1;
#endif

   return u32ReadOffset;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 WriteFpgaString(PFDO_DATA FdoData, unsigned char *buffer, int numbytes)
{
   int   count = 0, bytes = numbytes;
   int   status = 0;
   CSTIMER   CsTimeout;

   while(0 != bytes--)
   {
      WriteGageRegister(FdoData, GEN_COMMAND_R_REG, CSPLNDA_CFG_CHIP_SELECT | CSPLNDA_CFG_ENABLE | (((*buffer)&0xff) << 20));
      WriteGageRegister(FdoData, GEN_COMMAND_R_REG, CSPLNDA_CFG_CHIP_SELECT | CSPLNDA_CFG_ENABLE | (((*buffer)&0xff) << 20));
      WriteGageRegister(FdoData, GEN_COMMAND_R_REG, CSPLNDA_CFG_CHIP_SELECT | CSPLNDA_CFG_ENABLE | (((*buffer)&0xff) << 20) | CSPLNDA_CFG_WRITE_SELECT);
      WriteGageRegister(FdoData, GEN_COMMAND_R_REG, CSPLNDA_CFG_CHIP_SELECT | CSPLNDA_CFG_ENABLE | (((*buffer)&0xff) << 20) | CSPLNDA_CFG_WRITE_SELECT);
      WriteGageRegister(FdoData, GEN_COMMAND_R_REG, CSPLNDA_CFG_CHIP_SELECT | CSPLNDA_CFG_ENABLE | (((*buffer)&0xff) << 20));
      WriteGageRegister(FdoData, GEN_COMMAND_R_REG, CSPLNDA_CFG_CHIP_SELECT | CSPLNDA_CFG_ENABLE | (((*buffer)&0xff) << 20));

      CsSetTimer(&CsTimeout, CS_DP_WAIT_TIMEOUT);
      do
      {
         status = ReadGageRegister(FdoData, NIOS_GEN_STATUS);
         if (CsStateTimer(&CsTimeout))
         {
            status = ReadGageRegister(FdoData, NIOS_GEN_STATUS);
            if( 0 != (status & CSPLNDA_CFG_BUSY) )
               return CS_ADDON_FPGA_LOAD_FAILED;
         }
      }
      while( 0 != (status & CSPLNDA_CFG_BUSY));

      count++;
      buffer++;
   }

   return count - numbytes;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 ProgramAddonFpga(PFDO_DATA FdoData, uInt8 *pBuffer, uInt32 u32ImageSize)
{
   CSTIMER   CsTimeout;
   unsigned long count=0;
   unsigned int status = 0;
   int nChunkSize = 0x10000;
   int32 i32Ret = 0;
   uInt8   *fstr = pBuffer;
   int   nRemaining = 0;

   WriteGageRegister( FdoData, GEN_COMMAND_R_REG, 0 );
   CsSetTimer(&CsTimeout, CS_DP_WAIT_TIMEOUT);
   do
   {
      status = ReadGageRegister(FdoData, NIOS_GEN_STATUS);
      if (CsStateTimer(&CsTimeout))
      {
         status = ReadGageRegister(FdoData, NIOS_GEN_STATUS);
         if( 0 != ( status & CSPLNDA_CFG_STATUS) )
         {
            return CS_ADDON_FPGA_LOAD_FAILED;
         }
      }
   }
   while( 0 != (status & CSPLNDA_CFG_STATUS) );

   WriteGageRegister(FdoData, GEN_COMMAND_R_REG, CSPLNDA_NCONFIG | CSPLNDA_CFG_CHIP_SELECT | CSPLNDA_CFG_ENABLE);

   CsSetTimer(&CsTimeout, CS_DP_WAIT_TIMEOUT);
   do
   {
      status = ReadGageRegister(FdoData, NIOS_GEN_STATUS);
      if (CsStateTimer(&CsTimeout))
      {
         status = ReadGageRegister(FdoData, NIOS_GEN_STATUS);
         if( 0 == (status & CSPLNDA_CFG_STATUS) )
         {
            return CS_ADDON_FPGA_LOAD_FAILED;
         }
      }
   }
   while( 0 == (status & CSPLNDA_CFG_STATUS));

   WriteGageRegister(FdoData, GEN_COMMAND_R_REG, CSPLNDA_CFG_CHIP_SELECT | CSPLNDA_CFG_ENABLE);

   CsSetTimer(&CsTimeout, CS_DP_WAIT_TIMEOUT);
   do
   {
      status = ReadGageRegister(FdoData, NIOS_GEN_STATUS);
      if (CsStateTimer(&CsTimeout))
      {
         status = ReadGageRegister(FdoData, NIOS_GEN_STATUS);
         if( 0 != (status & CSPLNDA_CFG_STATUS) )
         {
            return CS_ADDON_FPGA_LOAD_FAILED;
         }
      }
   }
   while(0 != (status & CSPLNDA_CFG_STATUS));

   fstr = pBuffer;
   nRemaining = (int)u32ImageSize;
   do
   {
      count = (nChunkSize < nRemaining) ? nChunkSize : nRemaining;
      i32Ret = WriteFpgaString(FdoData, fstr, count);
      status = ReadGageRegister(FdoData, NIOS_GEN_STATUS);
      if (CS_FAILED(i32Ret) && 0 != (status & CSPLNDA_CFG_STATUS) )
      {
         return CS_ADDON_FPGA_LOAD_FAILED;
      }
      fstr += nChunkSize;
      nRemaining -= nChunkSize;
   }while (nRemaining > 0);

   CsSetTimer(&CsTimeout, CS_DP_WAIT_TIMEOUT);
   do
   {
      status = ReadGageRegister(FdoData, NIOS_GEN_STATUS);
      if (CsStateTimer(&CsTimeout))
      {
         status = ReadGageRegister(FdoData, NIOS_GEN_STATUS);
         if( 0 == (status & CSPLNDA_CFG_DONE) )
         {
            return CS_ADDON_FPGA_LOAD_FAILED;
         }
      }
   }while( 0 == (status & CSPLNDA_CFG_DONE) );

   WriteGageRegister(FdoData, GEN_COMMAND_R_REG, 0);

   return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32   AddonReadFlashByte(PFDO_DATA FdoData, uInt32 Addr)
{
   uInt32 u32Data = 0;

   WriteGIO(FdoData, CSPDR_ADDON_FLASH_ADD, Addr);
   WriteGIO(FdoData, CSPDR_ADDON_FLASH_CMD, CSPDR_ADDON_FCE | CSPDR_ADDON_FOE);

   u32Data = ReadGIO(FdoData, CSPDR_ADDON_FLASH_DATA);

   WriteGIO(FdoData, CSPDR_ADDON_FLASH_CMD, 0);
   return (u32Data);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void   AddonWriteFlashByte(PFDO_DATA FdoData, uInt32 Addr, uInt32 Data)
{
   WriteGIO(FdoData, CSPDR_ADDON_FLASH_ADD, Addr);
   WriteGIO(FdoData, CSPDR_ADDON_FLASH_DATA, Data);

   WriteGIO(FdoData, CSPDR_ADDON_FLASH_CMD, CSPDR_ADDON_FCE | CSPDR_ADDON_FWE);
   WriteGIO(FdoData, CSPDR_ADDON_FLASH_CMD, 0);
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32   SetupPreDataTransfer( PFDO_DATA   FdoData, uInt32 u32Segment, uInt16 u16Channel, uInt32 u32StartAddr, Pin_PREPARE_DATATRANSFER pXferConfig )
{
   FdoData->bDmaDemandMode = FALSE;

   WriteGageRegister( FdoData, MASK_REG, pXferConfig->u32DataMask );

   //----- Set the Segment Number
   StartTransferAtAddress (FdoData, u32Segment);

   SetFifoMode(FdoData, pXferConfig->u32FifoMode);

   //----- Start reading (Start Transfer DDR to FIFO)
   StartTransferDDRtoFIFO(FdoData, u32StartAddr);

   return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
VOID   CsAssignHardwareParams( PFDO_DATA FdoData )
{
#ifdef _WIN32
   ReadWriteConfigSpace( FdoData->PhysicalDevObj, 0, &FdoData->DeviceId,
                         FIELD_OFFSET(PCI_CONFIG_HEADER_0, DeviceID), sizeof(FdoData->DeviceId) );

#else
   FdoData->CardState.u16DeviceId = FdoData->PciDev->device;
#endif
   if ( FdoData->CardState.bPciExpress )
   {
      // Nucleon PciEx base board
      FdoData->BusType   = PCIE_BUS;
      FdoData->NumOfMemoryMap   = 2;
      FdoData->IntFifoRegister   = ALT_INT_FIFO;
      FdoData->FifoDataOutRegister   = ALT_FDATA_OUT;
   }
   else
   {
      // Combine Plx base board
      FdoData->BusType   = PCI_BUS;
      FdoData->NumOfMemoryMap   = 3;
      FdoData->IntFifoRegister   = INT_FIFO;
      FdoData->FifoDataOutRegister   = FDATA_OUT;
   }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
VOID   CsAssignRegisterBaseAddress( PFDO_DATA FdoData )
{
   if ( FdoData->CardState.bPciExpress )
   {
      // Nucleon PciEx base board
      FdoData->PciRegisterBase   = FdoData->MapAddress[1];
      FdoData->GageRegisterBase   = (PUCHAR) FdoData->MapAddress[1] + 0x80;
      FdoData->FlashRegisterBase   = (PUCHAR) FdoData->MapAddress[1] + 0xC4;
   }
   else
   {
      // Combine Plx base board
      FdoData->PciRegisterBase   = FdoData->MapAddress[0];
      FdoData->GageRegisterBase   = FdoData->MapAddress[1];
      FdoData->FlashRegisterBase   = FdoData->MapAddress[2];
   }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
VOID   CsAssignFunctionPointers( PFDO_DATA FdoData )
{
   if ( FdoData->CardState.bPciExpress )
   {
      // Nucleon PciEx base board
      FdoData->pfnStartDmaTransferDescriptor  = AltStartDmaTransferDescriptor;
      FdoData->pfnBuildDmaTransferDescriptor  = AltBuildDmaTransferDescriptor;
      FdoData->pfnAbortDmaTransfer   = AltAbortDmaTransfer;
      FdoData->pfnEnableInterruptDMA   = AltEnableInterruptDMA;
      FdoData->pfnDisableInterruptDMA   = AltDisableInterruptDMA;
      FdoData->pfnCheckBlockTransferDone   = AltCheckBlockTransferDone;

      FdoData->pfnGageFreeMemoryForDmaDescriptor   = AltGageFreeMemoryForDmaDescriptor;
      FdoData->pfnGageAllocateMemoryForDmaDescriptor   = AltGageAllocateMemoryForDmaDescriptor;
   }
   else
   {
      // Combine Plx base board
      FdoData->pfnStartDmaTransferDescriptor  = StartDmaTransferDescriptor;
      FdoData->pfnBuildDmaTransferDescriptor  = BuildDmaTransferDescriptor;
      FdoData->pfnAbortDmaTransfer   = AbortDmaTransfer;
      FdoData->pfnEnableInterruptDMA   = EnableInterruptDMA0;
      FdoData->pfnDisableInterruptDMA   = DisableInterruptDMA0;
      FdoData->pfnCheckBlockTransferDone   = CheckBlockTransferDone;

      FdoData->pfnGageFreeMemoryForDmaDescriptor   = GageFreeMemoryForDmaDescriptor;
      FdoData->pfnGageAllocateMemoryForDmaDescriptor   = GageAllocateMemoryForDmaDescriptor;
   }
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32   SplendaAcquire( PFDO_DATA pMaster, Pin_STARTACQUISITION_PARAMS pAcqParams   )
{
#ifdef CS_STREAM_SUPPORTED
   PCS_STREAM   pStream = &pMaster->Stream;
#endif
   PFDO_DATA   pCard = pMaster;
   int32   i32Status = CS_SUCCESS;
#ifdef _WIN32
   HANDLE   hProcess = IoGetCurrentProcess();
#endif

   // Fixed prefast warnings
   if ( NULL == pMaster || NULL == pAcqParams )
      return CS_MISC_ERROR;

   while ( pCard )
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
   if ( pStream->bEnabled && pAcqParams->u32Param1 != 0 )
#else
   if (is_flag_set_ (pStream, flags, STMF_ENABLED) && pAcqParams->u32Param1 != 0)
#endif
   {
      pCard = pMaster->pNextSlave;
      while ( pCard )
      {
#ifdef _WIN32
         i32Status = StmStartCapture( pCard );
#else
         i32Status = stm_start_capture (pCard, AcqCmdMemory);
#endif
         if ( CS_FAILED(i32Status) )
            break;
         pCard = pCard->pNextSlave;
      }

      if ( CS_SUCCEEDED(i32Status) )
#ifdef _WIN32
         i32Status = StmStartCapture( pMaster );
#else
         i32Status = stm_start_capture (pMaster, AcqCmdMemory);
#endif
   }
   else
#endif
      i32Status = SendStartAcquisition( pMaster, pAcqParams );

   return i32Status;
}

