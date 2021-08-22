#include "GageDrv.h"
#include "CsDeereDrv.h"
#include "CsDefines.h"
#include "CsErrors.h"
#include "CsIoctl.h"
#include "CsIoctlTypes.h"
#include "NiosApi.h"
#include "PlxSupport.h"
#include "CsPlxDefines.h"
#include "CsExpert.h"

#ifndef __x86_64__
   #include <linux/math64.h>
#endif

#ifdef __linux__
#define	ASSERT(x)
#endif

#define	SHIFT_DUAL		3			// 8 Samples
#define	SHIFT_SINGLE	4			// 16 Samples
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32	GetPreTrigDepthLimit ( PFDO_DATA FdoData,  uInt32 u32Segment )
{
   uInt32	u32PostTrig = 0;
   uInt32	u32RecLen = 0;
   uInt32	u32PreTrig = 0;
   uInt8	u8Shift = 0;
   uInt32	u32PreTrigDepthLimit = 0;
   BOOLEAN	bWrapAround = FALSE;
   uInt32	u32TriggerAddress = 0;
   uInt32  u32Mode = FdoData->AcqCfgEx.AcqCfg.u32Mode;

   if ((u32Mode & CS_MODE_DUAL) != 0)
      u8Shift = SHIFT_DUAL;
   else
      u8Shift = SHIFT_SINGLE;

   ASSERT( (FdoData->AcqCfgEx.i64HwDepth >> u8Shift) < 0x100000000 );
   ASSERT( (FdoData->AcqCfgEx.i64HwSegmentSize >> u8Shift) < 0x100000000 );
   u32PostTrig	= (uInt32) (FdoData->AcqCfgEx.i64HwDepth >> u8Shift);
   u32RecLen	= (uInt32) (FdoData->AcqCfgEx.i64HwSegmentSize >> u8Shift);

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
         u32PreTrigDepthLimit = u32TriggerAddress;
   }

   // Convert to number of samples
   return (u32PreTrigDepthLimit << u8Shift);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32	GetHwTriggerAddress( PFDO_DATA FdoData, uInt32 u32Segment, BOOLEAN *bRollOver )
{
   //	if( m_bZeroTrigAddress )
   //	{
   //		m_Etb = m_MemEtb = 0;
   //		return 0;
   //	}
   uInt8	u8Shift;

   if ( 0 != ( FdoData->AcqCfgEx.AcqCfg.u32Mode & CS_MODE_DUAL ))
      u8Shift = SHIFT_DUAL;		// DUAL: One memory location has 8 samples each of channels
   else
      u8Shift = SHIFT_SINGLE;		// SINGLE: One memory location has 16 samples


#define DEERE_AVG_FIXED_PRETRIG		32

   if ( FdoData->m_u32SegmentRead != u32Segment )
   {
      if ( 0 != (FdoData->AcqCfgEx.u32ExpertOption & CS_BBOPTIONS_MULREC_AVG_TD) )
      {
         FdoData->m_u32HwTrigAddr = DEERE_AVG_FIXED_PRETRIG;
         if ( 0 != ( FdoData->AcqCfgEx.AcqCfg.u32Mode & CS_MODE_DUAL ))
            FdoData->m_u32HwTrigAddr >>= 1;

         FdoData->m_u32HwTrigAddr >>= u8Shift;
         FdoData->m_Etb = FdoData->m_MemEtb = 0;
      }
      else if ( FdoData->AcqCfgEx.bSquareRec )
      {
         if ( FdoData->AcqCfgEx.i64HwSegmentSize > FdoData->AcqCfgEx.i64HwDepth )
            FdoData->m_u32HwTrigAddr = (uInt32) ( (FdoData->AcqCfgEx.i64HwSegmentSize - FdoData->AcqCfgEx.i64HwDepth) >> u8Shift );
         else
            FdoData->m_u32HwTrigAddr = 0;

         FdoData->m_Etb = FdoData->m_MemEtb = 0;
      }
      else
      {
         uInt32	u32TriggerAddress	= ReadRegisterFromNios( FdoData, u32Segment - 1, CSPLXBASE_READ_TRIG_ADD_CMD);
         uInt32	u32TaddrAndRecs		= ReadRegisterFromNios( FdoData, u32Segment - 1, CSPLXBASE_READ_TAIL_1 );		//read recs_counter and msb_ta

         // Retreive the information which card has TRIGGERED
         FdoData->u8TrigCardIndex  = (uInt8) u32TriggerAddress & 0xF;
         FdoData->m_bMemRollOver = ( (u32TaddrAndRecs & 0x2000000) != 0 );
         FdoData->m_Etb = (u32TriggerAddress >> 4) & 0xF;

         u32TriggerAddress	= u32TriggerAddress >> 8;		// 128 bits resolution
         u32TaddrAndRecs		= (u32TaddrAndRecs >> 3) & 0x1f000000;
         FdoData->m_u32HwTrigAddr = u32TriggerAddress | u32TaddrAndRecs;

         FdoData->m_u32SegmentRead = u32Segment;
      }
   }

   FdoData->m_Stb =
      FdoData->m_SkipEtb =
      FdoData->m_AddonEtb =
      FdoData->m_ChannelSkipEtb =
      FdoData->m_MemEtb =
      FdoData->m_DecEtb = 0;

   if ( NULL != bRollOver )
      *bRollOver = FdoData->m_bMemRollOver;

   return (FdoData->m_u32HwTrigAddr);		// 256 bite resolution
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32	CalculateReadingOffset(PFDO_DATA FdoData, int64 i64StartAddressOffset, uInt32 u32Segment, uInt32 *pu32AlignOffset )
{
   uInt32 u32ReadOffset = 0;
   uInt32 u32PostTrig = 0;
   uInt32 u32RecLen = 0;
   uInt32 u32PreTrig = 0;
   int64 i64Offset = 0;
   uInt32 u32Offset = 0;

   uInt8	 u8Shift = SHIFT_SINGLE;
   uInt32 u32TriggerAddress = GetHwTriggerAddress( FdoData, u32Segment, NULL );
   uInt32 u32AlignMask = 0;

   if ((FdoData->AcqCfgEx.AcqCfg.u32Mode & CS_MODE_DUAL) != 0)
      u8Shift = SHIFT_DUAL;

   u32AlignMask = (1<<u8Shift) - 1;

   u32PostTrig = (uInt32) (FdoData->AcqCfgEx.i64HwDepth >> u8Shift);
   u32RecLen	= (uInt32) (FdoData->AcqCfgEx.i64HwSegmentSize >> u8Shift);

   if ( u32RecLen  > u32PostTrig )
      u32PreTrig = u32RecLen - u32PostTrig;

   // Relative offset from beginning of the segment
   i64Offset = u32PreTrig;
   i64Offset <<= u8Shift;
   i64Offset += i64StartAddressOffset;
   if ( FdoData->AcqCfgEx.bSquareRec )
   {
      // In Square Rec mode , the trigger delay effect does not cause any OFFSETs in memory
      i64Offset -= FdoData->AcqCfgEx.AcqCfg.i64TriggerDelay;
   }
#ifdef __x86_64__
   i64Offset = i64Offset % FdoData->AcqCfgEx.i64HwSegmentSize;
#else
   {
      int64 i64dividend, i64divisor;
      i64dividend = i64Offset;
      i64divisor  = FdoData->AcqCfgEx.i64HwSegmentSize;
      /* printk ("\n--> div (%lld, %lld)\n", u64dividend, u64divisor); */
      i64Offset   = i64Offset - (div64_s64 (i64dividend, i64divisor)*FdoData->AcqCfgEx.i64HwSegmentSize);
      /* printk ("\n--> remainder = %lld\n", u64Offset); */
   }
#endif
   if ( pu32AlignOffset )
      *pu32AlignOffset = (uInt32) (i64Offset & u32AlignMask);

   u32Offset = (uInt32) (i64Offset >> u8Shift);
   u32Offset = u32Offset % u32RecLen;

   // Position of Start of segment in memory
   u32ReadOffset = (u32TriggerAddress >=u32PreTrig)?(u32TriggerAddress - u32PreTrig):(u32RecLen + u32TriggerAddress - u32PreTrig);
   u32ReadOffset += u32Offset;
   u32ReadOffset %= u32RecLen;

   if ( sizeof(uInt32) == FdoData->u32SampleSize )
   {
      // 32 bits samples. The offset in memory shoud by 2 times bigger
      u32ReadOffset *= 2;
   }

   return u32ReadOffset;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void  IntrDMAInitialize( PFDO_DATA FdoData )
{

   //FdoData->u32RegCSR = _ReadRegister( FdoData, PLDA_DMA_INTR_RD_INFO );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	SetupPreDataTransfer( PFDO_DATA	FdoData, uInt32 u32Segment, uInt16 u16Channel, uInt32 u32StartAddr, Pin_PREPARE_DATATRANSFER pXferConfig )
{
   FdoData->bDmaDemandMode = FALSE;

   WriteGageRegister( FdoData, MASK_REG, pXferConfig->u32DataMask );

   //----- Set the Segment Number
   StartTransferAtAddress (FdoData, u32Segment);

   //----- Start reading (Start Transfer DDR to FIFO)
   StartTransferDDRtoFIFO(FdoData, u32StartAddr);

   SetFifoMode(FdoData, pXferConfig->u32FifoMode);

   return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	SetupPreDataTransferEx( PFDO_DATA	FdoData, uInt32 u32Segment, uInt32 u32StartAddr, uInt32 u32ReadCount, uInt32 u32SkipCount, Pin_PREPARE_DATATRANSFER pXferConfig )
{
   FdoData->bDmaDemandMode = TRUE;

   WriteRegisterThruNios(FdoData, u32SkipCount, CSPLXBASE_SQR_READSKIPCOUNT);
   WriteRegisterThruNios(FdoData, u32ReadCount, CSPLXBASE_SQR_READVALIDCOUNT);

   WriteGageRegister( FdoData, MASK_REG, pXferConfig->u32DataMask );

   WriteRegisterThruNios(FdoData, u32StartAddr, CSPLXBASE_SET_START_ADD_CMD);
   WriteRegisterThruNios(FdoData, 0, CSPLXBASE_SQR_AQC_SKIP_READ);

   SetFifoMode(FdoData, pXferConfig->u32FifoMode);

   return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void	GetAlarmStatus( PFDO_DATA	FdoData )
{
   if ( DEERE_ALARM_INVALID == FdoData->u16AlarmStatus )
      FdoData->u16AlarmStatus = (uInt16)ReadRegisterFromNios( FdoData, 0, DEERE_ALARM_CMD );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32  ClearAlarmStatus( PFDO_DATA	FdoData, uInt16 u16AlarmSource )
{
   int32 i32Ret = WriteRegisterThruNios( FdoData, u16AlarmSource | DEERE_ALARM_CLEAR, DEERE_ALARM_CMD | SPI_WRITE_CMD_MASK );
   if( CS_SUCCEEDED(i32Ret) )// Reset channels protection status This will force updatting this status on the next Data transfer
      FdoData->u16AlarmStatus = DEERE_ALARM_INVALID;
   return i32Ret;
}
/*
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int32  ReportAlarm( PFDO_DATA	FdoData )
{
int32 i32Ret = CS_SUCCESS;
//MAsked read value by allowed alarm sources
uInt16 u16ReportedStatus = FdoData->u16AlarmSource & FdoData-?u16AlarmStatus;
if( 0 == u16ReportedStatus )
return i32Ret;

char ChannelText[50] = {0};
char TracelChar = ' ';
int i = 0;
if( u16ReportedStatus & DEERE_ARARM_PROTECT_CH1 )
{
TracelChar = '1';
ChannelText[i++] = '#';
ChannelText[i++] = '1';
i32Ret = CS_CHANNEL_PROTECT_FAULT;
}

if( u16ReportedStatus & DEERE_ARARM_PROTECT_CH2 )
{
TracelChar = '2';
if( 0 != i )
{
ChannelText[i++] = ',';
TracelChar++;
}
ChannelText[i++] = '#';
ChannelText[i++] = '2';
if ( 0 != ( m_SwCfg.u32Mode & CS_MODE_DUAL) )
i32Ret = CS_CHANNEL_PROTECT_FAULT;
}

if( u16ReportedStatus & DEERE_ARARM_CAL_PLL_UNLOCK )
{
if( CS_SUCCEEDED(i32Ret) ) i32Ret = CS_FALSE;
t.Trace(TraceInfo, " ! # ! AC Cal source lost lock.\n");
}
else
{
ChannelText[i] = 0;
#if DBG
t.Trace(TraceInfo, " ! %c - %02x (%02x) ! Protection fault on channel %s Return value %d.\n", TracelChar, m_u16AlarmStatus, u16ReportedStatus, ChannelText, i32Ret );
#else
m_EventLog.ReportEventString( CS_INFO_OVERVOLTAGE,  ChannelText);
#endif
}
return i32Ret;
}
*/


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	DeereAcquire( PFDO_DATA pMaster, Pin_STARTACQUISITION_PARAMS pAcqParams   )
{
   PFDO_DATA	pCard = pMaster;
   int32		i32Status = CS_SUCCESS;
#ifdef _WIN32
   HANDLE		hProcess = IoGetCurrentProcess();
#endif

   // Alarm source was sent as Param1
   uInt16		u16AlarmSource = (uInt16) pAcqParams->u32Param1;
   uInt32		u32IntrEnabled = MAOINT;

   // Remember the process that sent the Start acquisition request
   while ( pCard )
   {
#ifdef _WIN32
      pCard->hCaptureProcess = hProcess;
#endif
      pCard = pCard->pNextSlave;
   }

   pCard = pMaster->pNextSlave;
   while ( pCard )
   {
      ClearAlarmStatus( pCard, u16AlarmSource );
      ConfigureInterrupts( pCard, u32IntrEnabled );

      pCard = pCard->pNextSlave;
   }

   ClearAlarmStatus( pMaster, u16AlarmSource );
   ConfigureInterrupts( pMaster->pTriggerMaster, pAcqParams->u32IntrMask | u32IntrEnabled );

   pCard = pMaster;
   while ( pCard )
   {
      pCard->m_u32SegmentRead = 0;

      if( pCard != pCard->pTriggerMaster )
      {
         i32Status = StartAcquisition( pCard, pAcqParams->bRtMode );
         if ( CS_FAILED(i32Status) )
            break;
      }

      pCard = pCard->pNextSlave;
   }

   if ( CS_SUCCEEDED(i32Status) )
      i32Status = StartAcquisition( pMaster->pTriggerMaster, pAcqParams->bRtMode );
   return i32Status;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VOID    CsAssignHardwareParams( PFDO_DATA FdoData )
{
#ifdef _WIN32
   ReadWriteConfigSpace( FdoData->PhysicalDevObj, 0, &FdoData->CardState.u16DeviceId,
                         FIELD_OFFSET(PCI_CONFIG_HEADER_0, DeviceID), sizeof(FdoData->CardState.u16DeviceId) );
#else
   FdoData->CardState.u16DeviceId = FdoData->PciDev->device;
#endif

   FdoData->NumOfMemoryMap	= 3;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

VOID    CsAssignRegisterBaseAddress( PFDO_DATA FdoData )
{
   FdoData->PciRegisterBase	= FdoData->MapAddress[0];
   FdoData->GageRegisterBase	= FdoData->MapAddress[1];
   FdoData->FlashRegisterBase	= FdoData->MapAddress[2];
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
VOID    CsAssignFunctionPointers( PFDO_DATA FdoData )
{
   FdoData->pfnStartDmaTransferDescriptor  = StartDmaTransferDescriptor;
   FdoData->pfnBuildDmaTransferDescriptor  = BuildDmaTransferDescriptor;
   FdoData->pfnAbortDmaTransfer			= AbortDmaTransfer;
   FdoData->pfnEnableInterruptDMA			= EnableInterruptDMA0;
   FdoData->pfnDisableInterruptDMA			= DisableInterruptDMA0;
   FdoData->pfnCheckBlockTransferDone		= CheckBlockTransferDone;

   FdoData->pfnGageFreeMemoryForDmaDescriptor		= GageFreeMemoryForDmaDescriptor;
   FdoData->pfnGageAllocateMemoryForDmaDescriptor	= GageAllocateMemoryForDmaDescriptor;
}
