
#include "CsRabbitDrv.h"
#include "CsDefines.h"
#include "CsErrors.h"
#include "CsIoctl.h"
#include "CsIoctlTypes.h"
#include "NiosApi.h"
#include "PlxSupport.h"

#define	SHIFT_DUAL		3			// 8 Samples
#define	SHIFT_SINGLE	4			// 16 Samples

#ifdef __linux__
#define	ASSERT(x)
#endif
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

	if ( FdoData->CardState.bPciExpress )
		u8Shift += 1;

	ASSERT( (FdoData->AcqCfgEx.i64HwDepth >> u8Shift) < 0x100000000 );
	ASSERT( (FdoData->AcqCfgEx.i64HwSegmentSize >> u8Shift) < 0x100000000 );

	u32PostTrig	= (uInt32) (FdoData->AcqCfgEx.i64HwDepth >> u8Shift);
	u32RecLen	= (uInt32) (FdoData->AcqCfgEx.i64HwSegmentSize >> u8Shift);

	if ( u32RecLen  > u32PostTrig )
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
	if ( FdoData->CardState.bPciExpress )
	{
		uInt32		u32Mode = FdoData->AcqCfgEx.AcqCfg.u32Mode;
		uInt8		u8Shift = 0;

		if ((u32Mode & CS_MODE_DUAL) != 0)
			u8Shift = 3;
		else if ((u32Mode & CS_MODE_SINGLE) != 0)
			u8Shift = 4;

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
	uInt32		u32ReadOffset = 0;
	uInt32		u32PostTrig = 0;
	uInt32		u32RecLen = 0;
	uInt32		u32PreTrig = 0;
	int64		i64Offset = 0;
	uInt32		u32Offset = 0;
	uInt8		u8Shift = 0;

	uInt32		u32TriggerAddress = FdoData->m_u32HwTrigAddr;
	uInt32		u32Mode = FdoData->AcqCfgEx.AcqCfg.u32Mode;
	int64		i64TrigDelay = FdoData->AcqCfgEx.AcqCfg.i64TriggerDelay;


	if ((u32Mode & CS_MODE_DUAL) != 0)
		u8Shift = 3;
	else
		u8Shift = 4;

	if ( FdoData->CardState.bPciExpress )
		u8Shift += 1;

	ASSERT( (FdoData->AcqCfgEx.i64HwDepth >> u8Shift) < 0x100000000 );
	ASSERT( (FdoData->AcqCfgEx.i64HwSegmentSize >> u8Shift) < 0x100000000 );

	u32PostTrig = (uInt32) (FdoData->AcqCfgEx.i64HwDepth >> u8Shift);
	u32RecLen	= (uInt32)(FdoData->AcqCfgEx.i64HwSegmentSize >> u8Shift);

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
	i64Offset = i64Offset % FdoData->AcqCfgEx.i64HwSegmentSize;

	if( NULL != pu32AlignOffset )
		*pu32AlignOffset = (uInt32) ( i64Offset & ((1<<u8Shift)-1) );

	u32Offset = (uInt32) (i64Offset >> u8Shift);
	u32Offset = u32Offset % u32RecLen;

	// Position of Start of segment in memory
	u32ReadOffset = (u32TriggerAddress >=u32PreTrig)?(u32TriggerAddress - u32PreTrig):(u32RecLen + u32TriggerAddress - u32PreTrig);

	u32ReadOffset += u32Offset;
	u32ReadOffset %= u32RecLen;

	return u32ReadOffset;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	GetMsCalibOffset( PFDO_DATA FdoData )
{
	PFDO_DATA	pCard = FdoData->pMasterIndependent;

	// Find the card that have the trigger event
	while ( pCard )
	{
		if ( FdoData->pMasterIndependent->u8TrigCardIndex == pCard->u8MsCardId )
			break;

		pCard = pCard->pNextSlave;
	}

	if ( pCard )
	{
		int32	i32Offset = FdoData->i32MsAlignOffset - pCard->i32MsAlignOffset;
		//DbgPrint("MsOffset return %d", (FdoData->i32MsAlignOffset - pCard->i32MsAlignOffset));

		if ( 1 != pCard->AcqCfgEx.u32Decimation )
		{
			if( i32Offset > 0)
			{
				i32Offset = ((2*i32Offset + pCard->AcqCfgEx.u32Decimation) /(2*pCard->AcqCfgEx.u32Decimation));
			}
			else
			{
				i32Offset = -1*i32Offset;
				i32Offset = ((2*i32Offset + pCard->AcqCfgEx.u32Decimation) /(2*pCard->AcqCfgEx.u32Decimation));
				i32Offset = -1*i32Offset;
			}
		}

		return i32Offset;
	}
	else
	{
		DbgPrint("Error Get MsOffset");
		return 0;
	}
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
	uInt32	u32UserRegVal = 0;

	FdoData->bDmaDemandMode = FALSE;

	WriteGageRegister( FdoData, MASK_REG, pXferConfig->u32DataMask );

	//----- Set the Segment Number
	StartTransferAtAddress (FdoData, u32Segment);

	// Reset FIR data path if needed
	if ( pXferConfig->bXpertFIR )
	{
		u32UserRegVal = ReadGageRegister( FdoData, USER_REG_INDEX );
		WriteGageRegister( FdoData, USER_REG_INDEX, u32UserRegVal | 0x200 );
	}

	SetFifoMode(FdoData, pXferConfig->u32FifoMode);

	if ( pXferConfig->bXpertFIR )
	{
		WriteRegisterThruNios(FdoData, 0, CSPLXBASE_READ_ACQ_CMD);
	}
	else
	{
		//----- Start reading (Start Transfer DDR to FIFO)
		StartTransferDDRtoFIFO(FdoData, u32StartAddr);
	}

	// Clear reset bit FIR data path if needed
	if ( pXferConfig->bXpertFIR )
	{
		WriteGageRegister( FdoData, USER_REG_INDEX, u32UserRegVal & ~(0x200) );
	}

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
VOID    CsAssignHardwareParams( PFDO_DATA FdoData )
{
#ifdef _WIN32
	ReadWriteConfigSpace( FdoData->PhysicalDevObj, 0, &FdoData->DeviceId,
							FIELD_OFFSET(PCI_CONFIG_HEADER_0, DeviceID), sizeof(FdoData->DeviceId) );
#else
	FdoData->CardState.u16DeviceId = FdoData->PciDev->device;
#endif

	FdoData->CardState.u16DeviceId = FdoData->DeviceId;
	if ( FdoData->CardState.bPciExpress )
	{
		// Nucleon PciEx base board
		FdoData->BusType				= PCIE_BUS;
		FdoData->NumOfMemoryMap			= 2;
		FdoData->IntFifoRegister		= ALT_INT_FIFO;
		FdoData->FifoDataOutRegister	= ALT_FDATA_OUT;
	}
	else
	{
		// Combine Plx base board
		FdoData->BusType				= PCI_BUS;
		FdoData->NumOfMemoryMap			= 3;
		FdoData->IntFifoRegister		= INT_FIFO;
		FdoData->FifoDataOutRegister	= FDATA_OUT;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
VOID    CsAssignRegisterBaseAddress( PFDO_DATA FdoData )
{
	if ( FdoData->CardState.bPciExpress )
	{
		// Nucleon PciEx base board
		FdoData->PciRegisterBase	= FdoData->MapAddress[1];
		FdoData->GageRegisterBase	= (PUCHAR) FdoData->MapAddress[1] + 0x80;
		FdoData->FlashRegisterBase	= (PUCHAR) FdoData->MapAddress[1] + 0xC4;
	}
	else
	{
		// Combine Plx base board
		FdoData->PciRegisterBase	= FdoData->MapAddress[0];
		FdoData->GageRegisterBase	= FdoData->MapAddress[1];
		FdoData->FlashRegisterBase	= FdoData->MapAddress[2];
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
VOID    CsAssignFunctionPointers( PFDO_DATA FdoData )
{
	if ( FdoData->CardState.bPciExpress )
	{
		// Nucleon PciEx base board
		FdoData->pfnStartDmaTransferDescriptor  = AltStartDmaTransferDescriptor;
		FdoData->pfnBuildDmaTransferDescriptor  = AltBuildDmaTransferDescriptor;
		FdoData->pfnAbortDmaTransfer			= AltAbortDmaTransfer;
		FdoData->pfnEnableInterruptDMA			= AltEnableInterruptDMA;
		FdoData->pfnDisableInterruptDMA			= AltDisableInterruptDMA;
		FdoData->pfnCheckBlockTransferDone		= AltCheckBlockTransferDone;

		FdoData->pfnGageFreeMemoryForDmaDescriptor		= AltGageFreeMemoryForDmaDescriptor;
		FdoData->pfnGageAllocateMemoryForDmaDescriptor	= AltGageAllocateMemoryForDmaDescriptor;
	}
	else
	{
		// Combine Plx base board
		FdoData->pfnStartDmaTransferDescriptor  = StartDmaTransferDescriptor;
		FdoData->pfnBuildDmaTransferDescriptor  = BuildDmaTransferDescriptor;
		FdoData->pfnAbortDmaTransfer			= AbortDmaTransfer;
		FdoData->pfnEnableInterruptDMA			= EnableInterruptDMA0;
		FdoData->pfnDisableInterruptDMA			= DisableInterruptDMA0;
		FdoData->pfnCheckBlockTransferDone		= CheckBlockTransferDone;

		FdoData->pfnGageFreeMemoryForDmaDescriptor		= GageFreeMemoryForDmaDescriptor;
		FdoData->pfnGageAllocateMemoryForDmaDescriptor	= GageAllocateMemoryForDmaDescriptor;
	}
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32	RabbitAcquire( PFDO_DATA pMaster, Pin_STARTACQUISITION_PARAMS pAcqParams )
{
#ifdef CS_STREAM_SUPPORTED
   PCS_STREAM	pStream = &pMaster->Stream;
#endif
   PFDO_DATA	pCard = pMaster;
   int32		   i32Status = CS_SUCCESS;
#ifdef _WIN32
   HANDLE		hProcess = IoGetCurrentProcess();
#endif

   // Fixed prefast warnings
   if ( NULL == pMaster || NULL == pAcqParams )
      return CS_MISC_ERROR;

   // Remember the process that sent the Start acquisition request
   while ( pCard )
   {
#ifdef _WIN32
      pCard->hCaptureProcess = hProcess;
#endif
      pCard = pCard->pNextSlave;
   }
#ifdef CS_STREAM_SUPPORTED
   // If u32Params1 != 0 then we are in calibration mode.
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
