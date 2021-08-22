
#include "GageDrv.h"
#include "CsDefines.h"
#include "CsErrors.h"
#include "CsIoctl.h"
#include "CsIoctlTypes.h"
#include "NiosApi.h"
#include "CsPlxDefines.h"
#include "PlxDrvConsts.h"
#include "PlxSupport.h"

#define USE_DMA_CHANNEL0

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	PlxInterruptEnable( PFDO_DATA	FdoData, BOOLEAN bDmaIntr )
{
    unsigned long IntCsr;
    unsigned long RegValue;

    //---- Interrupt Control/Status Register
    IntCsr = _ReadRegister(FdoData, PCI9056_INT_CTRL_STAT);

    if ( bDmaIntr )
    {
#ifdef USE_DMA_CHANNEL0
		//  Use PciDmaChannel0
        IntCsr |= (1 << 18);

        //----- Make sure DMA interrupt is routed to PCI
        RegValue = _ReadRegister(FdoData, PCI9056_DMA0_MODE);
        _WriteRegister(FdoData, PCI9056_DMA0_MODE, RegValue | (1 << 17));

#else
		//  Use PciDmaChannel1
		IntCsr |= (1 << 19);

		/* Make sure DMA interrupt is routed to PCI */
		RegValue = _ReadRegister(FdoData, PCI9056_DMA1_MODE);
		_WriteRegister(FdoData, PCI9056_DMA1_MODE, RegValue | (1 << 17));
#endif
    }

	//  pPlxInterrupt->PciMainInt
    IntCsr |= (1 << 8);

	// pPlxInterrupt->IopToPciInt
    IntCsr |= (1 << 11);

	//----- Write Status registers
    _WriteRegister(FdoData, PCI9056_INT_CTRL_STAT, IntCsr);

}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void	PlxInterruptDisable( PFDO_DATA	FdoData, BOOLEAN bDmaIntr )
{
    unsigned long IntCsr;
    unsigned long RegValue;

    //----- Interrupt Control/Status Register
    IntCsr = _ReadRegister(FdoData, PCI9056_INT_CTRL_STAT);

    if ( bDmaIntr )
    {
#ifdef USE_DMA_CHANNEL0
		//  Use PciDmaChannel0
        //----- Check if DMA interrupt is routed to PCI
        RegValue = _ReadRegister(FdoData, PCI9056_DMA0_MODE);

        if (RegValue & (1 << 17))
        {
            IntCsr &= ~(1 << 18);
            //----- Make sure DMA interrupt is routed to Local
            _WriteRegister(FdoData, PCI9056_DMA0_MODE, RegValue & ~(1 << 17));
        } //----- if (RegValue & (1 << 17)){

		//------ clear the interrupt
		RegValue = _ReadRegister(FdoData, PCI9056_DMA_COMMAND_STAT);
        _WriteRegister(FdoData, PCI9056_DMA_COMMAND_STAT, RegValue & ~(1 << 3));
#else
		//  Use PciDmaChannel1
        //----- Check if DMA interrupt is routed to PCI
        RegValue = _ReadRegister(FdoData, PCI9056_DMA1_MODE);

        if (RegValue & (1 << 17))
        {
            IntCsr &= ~(1 << 18);
            //----- Make sure DMA interrupt is routed to Local
            _WriteRegister(FdoData, PCI9056_DMA1_MODE, RegValue & ~(1 << 17));
        } //----- if (RegValue & (1 << 17)){

		//------ clear the interrupt
		RegValue = _ReadRegister(FdoData, PCI9056_DMA_COMMAND_STAT);
        _WriteRegister(FdoData, PCI9056_DMA_COMMAND_STAT, RegValue & ~(1 << 11));
#endif
    }

	// pPlxInterrupt->IopToPciInt
    IntCsr &= ~(1 << 11);

    // pPlxInterrupt->PciMainInt
    IntCsr &= ~(1 << 8);

    //----- Write Status registers
    _WriteRegister(FdoData, PCI9056_INT_CTRL_STAT, IntCsr);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void ClearInterrupts(PFDO_DATA	FdoData)
{
	//if( m_InitStatus.Info.u32Nios )
	{
	//----- Clear INT_CFG => disable all the interrupts
	WriteGageRegister(FdoData, INT_CFG, 0);
	FdoData->u32IntBaseMask = 0;

	//----- Clear INT_FIFO and INT_COUNT
	WriteGageRegister(FdoData, MODESEL, INT_CLR);
	WriteGageRegister(FdoData, MODESEL, 0);
	}

	//---- Disable the local interrupt
	PlxInterruptDisable( FdoData, FALSE );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void EnableInterrupts(PFDO_DATA FdoData, uInt32 IntType)
{
	FdoData->u32IntBaseMask |= IntType;
	WriteGageRegister(FdoData, INT_CFG, FdoData->u32IntBaseMask);

	PlxInterruptEnable( FdoData, FALSE );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void DisableInterrupts(PFDO_DATA FdoData, uInt32 IntType)
{
	FdoData->u32IntBaseMask &= ~IntType;
	WriteGageRegister( FdoData, INT_CFG, FdoData->u32IntBaseMask );
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void ConfigureInterrupts( PFDO_DATA FdoData,  uInt32 u32IntrMask )
{
	FdoData->u32IntBaseMask = 0;
	EnableInterrupts( FdoData, u32IntrMask );
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
BOOLEAN	IsDmaInterrupt( PFDO_DATA	FdoData )
{
	uInt32 	IntIntCtrlStatus = _ReadRegister(FdoData, PCI9056_INT_CTRL_STAT);

	return  ((IntIntCtrlStatus & DMA_CH0_INT_ACTIVE) != 0);
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void  ClearInterruptDMA0( PFDO_DATA	FdoData )
{
	uInt32 u32Value = _ReadRegister(FdoData, PCI9056_DMA_COMMAND_STAT);

	// Set Dma Clear interrupt bit
	u32Value |= 0x8;

	_WriteRegister(FdoData, PCI9056_DMA_COMMAND_STAT, u32Value);

}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void  EnableInterruptDMA0( PFDO_DATA FdoData )
{
	uInt32		u32Config = 0x00020FC3;

	if ( FdoData->bDmaDemandMode )
		u32Config |= (1<< 12);

#ifdef USE_DMA_64
	// Enable the descriptor to load the PCI Dual Adsress Cycles value
	u32Config |= (1<< 18);
#endif

	_WriteRegister(FdoData, PCI9056_DMA0_MODE, u32Config);

	PlxInterruptEnable( FdoData, TRUE );
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void  DisableInterruptDMA0( PFDO_DATA FdoData )
{
	uInt32		u32Config = 0x00020BC3;

	if ( FdoData->bDmaDemandMode )
		u32Config |= (1<< 12);

#ifdef USE_DMA_64
	// Enable the descriptor to load the PCI Dual Adsress Cycles value
	u32Config |= (1<< 18);
#endif

	_WriteRegister(FdoData, PCI9056_DMA0_MODE, u32Config);
}

#ifdef _WINDOWS_

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void	 GageFreeMemoryForDmaDescriptor( IN PFDO_DATA	FdoData )
{
	if ( FdoData->pDmaDescriptor )
	{
		MmFreeContiguousMemory(FdoData->pDmaDescriptor);
		FdoData->pDmaDescriptor = NULL;
	}
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
BOOLEAN	GageAllocateMemoryForDmaDescriptor( IN PFDO_DATA	FdoData )
{
	PHYSICAL_ADDRESS	PhysAddr;

	// 32 bit address
	PhysAddr.QuadPart = 0xFFFFFFFF;

	// Allocate memory for Dma descriptor
	FdoData->u32MaxDesc = MAX_DMA_DESC_BLOCKS;
	FdoData->pDmaDescriptor = MmAllocateContiguousMemory( FdoData->u32MaxDesc * sizeof(PLX_DMA_DESCRIPTOR), PhysAddr);
	if ( ! FdoData->pDmaDescriptor )
	{
		KdPrint(("Cannot allocate memory for Dma descriptor\n"));
		return FALSE;
	}
	FdoData->PhysAddrDmaDesc = MmGetPhysicalAddress(FdoData->pDmaDescriptor);

	return TRUE;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void BuildDmaTransferDescriptor( PFDO_DATA	FdoData,
								 PSCATTER_GATHER_LIST  ScatterGather )
{
	BOOLEAN					PciMemory = TRUE;
	uInt32					i = 0;
	PPLX_DMA_DESCRIPTOR		pDesc = (PPLX_DMA_DESCRIPTOR) FdoData->pDmaDescriptor;
	PHYSICAL_ADDRESS		PhysDmaNextDescriptor = FdoData->PhysAddrDmaDesc;

	uInt32	u32DescCount = ScatterGather->NumberOfElements;

	if ( u32DescCount > FdoData->u32MaxDesc )
		 u32DescCount = FdoData->u32MaxDesc;

	FdoData->XferRead.u32CurrentDmaSize = 0;

	//----- build the descriptor array
	for (i = 0; i < u32DescCount; i++)
	{
		pDesc[i].PciAddress			= ScatterGather->Elements[i].Address.LowPart;
		pDesc[i].LocalAddress		= FIFO_READ_ADD; //----- Constant Address
		pDesc[i].u32TransferSize	= ScatterGather->Elements[i].Length;
		pDesc[i].NextDescPtr		= (PhysDmaNextDescriptor.LowPart + sizeof(PLX_DMA_DESCRIPTOR)) | (1<<3) | // Direction (local => PCI)
																		( PciMemory <<0 );
#ifdef USE_DMA_64
			pDesc[i].PciAddressHigh	=ScatterGather->Elements[i].Address.HighPart;
#endif

		PhysDmaNextDescriptor.LowPart += sizeof(PLX_DMA_DESCRIPTOR);
		FdoData->XferRead.u32CurrentDmaSize  += pDesc[i].u32TransferSize;
	}

	//----- Write the end of the chain in the last descriptor
	pDesc[u32DescCount-1].NextDescPtr = ( (1<<3) | (1<<1) | ( PciMemory <<0 ));	// End of chain

}

#else // __linux__

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
BOOLEAN	GageAllocateMemoryForDmaDescriptor( PFDO_DATA	FdoData )
{
	BOOLEAN	 bRet = TRUE;

TRACEIN;
	// Allocate memory for Dma descriptor
	FdoData->u32MaxDesc = MAX_DMA_DESC_BLOCKS;
	FdoData->pDmaDescriptor = dma_alloc_coherent (FdoData->Device, FdoData->u32MaxDesc * sizeof(PLX_DMA_DESCRIPTOR),
	                                                                 &FdoData->PhysAddrDmaDesc, GFP_KERNEL);
	if ( ! FdoData->pDmaDescriptor )
	{
		DbgPrint("Cannot allocate memory for Dma descriptor\n");
		bRet = FALSE;
		goto errorAllocate;
	}

errorAllocate:

TRACEOUT;
	return bRet;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void	GageFreeMemoryForDmaDescriptor( PFDO_DATA	FdoData )
{
TRACEIN;
	if ( FdoData->pDmaDescriptor )
	{
		dma_free_coherent(FdoData->Device, FdoData->u32MaxDesc * sizeof(PLX_DMA_DESCRIPTOR),
						  FdoData->pDmaDescriptor,
						  FdoData->PhysAddrDmaDesc );
		FdoData->pDmaDescriptor = NULL;
		FdoData->PhysAddrDmaDesc = 0;

	}

TRACEOUT;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void BuildDmaTransferDescriptor( PFDO_DATA	FdoData,
								 struct scatterlist *ScatterGather, uInt32 NumberOfElements )
{
	BOOLEAN					PciMemory = TRUE;
	uInt32					i = 0;
	PPLX_DMA_DESCRIPTOR	pDesc = (PPLX_DMA_DESCRIPTOR) FdoData->pDmaDescriptor;
	PHYSICAL_ADDRESS		PhysDmaNextDescriptor;
	uInt32					u32DescCount = NumberOfElements;

//TRACEIN;
	PhysDmaNextDescriptor.LowPart = (uInt32) (FdoData->PhysAddrDmaDesc & 0xFFFFFFFF);
	PhysDmaNextDescriptor.HighPart = (int32) ((FdoData->PhysAddrDmaDesc >> 16) >> 16);

	if ( u32DescCount > FdoData->u32MaxDesc )
		 u32DescCount = FdoData->u32MaxDesc;

	FdoData->XferRead.u32CurrentDmaSize = 0;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)) // changes to how the scatterlist is used 
	//----- build the descriptor array
	for (i = 0; i < u32DescCount; i++)
	{
		dma_addr_t pciaddr = sg_dma_address(&ScatterGather[i]);
		pDesc[i].PciAddress				= (uInt32)pciaddr;
		pDesc[i].LocalAddress			= FIFO_READ_ADD; //----- Constant Address
		pDesc[i].u32TransferSize		= sg_dma_len(&ScatterGather[i]);
		pDesc[i].NextDescPtr			= (PhysDmaNextDescriptor.LowPart + sizeof(PLX_DMA_DESCRIPTOR)) | (1<<3) | // Direction (local => PCI)
																		( PciMemory <<0 );

#ifdef USE_DMA_64
		pDesc[i].PciAddressHigh	=(uInt32)(pciaddr>>32);
#endif

#ifdef GEN_DEBUG_DMA
#ifdef USE_DMA_64
		GEN_DEBUG_PRINT("\nBuffer%d Addr Low = 0x%x, High = 0x%x,  Size = 0x%x",
						(int)i, pDesc[i].PciAddress, pDesc[i].PciAddressHigh, pDesc[i].u32TransferSize);
#else
		GEN_DEBUG_PRINT("\nBuffer%d Addr = 0x%x,  Size = 0x%x", (int)i, pDesc[i].PciAddress, pDesc[i].u32TransferSize);
#endif
#endif

		PhysDmaNextDescriptor.LowPart += sizeof(PLX_DMA_DESCRIPTOR);
		FdoData->XferRead.u32CurrentDmaSize  += pDesc[i].u32TransferSize;
	}

#else
	{
		struct scatterlist *sg;// = ScatterGather;
		//----- build the descriptor array
		//----- use new macro (for_each_sg) to loop thru the scatterlist segments
		for_each_sg(ScatterGather, sg, u32DescCount, i)
		{
			dma_addr_t pciaddr = sg_dma_address(sg);
			pDesc[i].PciAddress			= pciaddr;
			pDesc[i].LocalAddress			= FIFO_READ_ADD; //----- Constant Address
			pDesc[i].u32TransferSize		= sg_dma_len(sg);
			pDesc[i].NextDescPtr			= (PhysDmaNextDescriptor.LowPart + sizeof(PLX_DMA_DESCRIPTOR)) | (1<<3) | // Direction (local => PCI)
																		( PciMemory <<0 );
#ifdef USE_DMA_64
			pDesc[i].PciAddressHigh	=(uInt32)(pciaddr>>32);
#endif

#ifdef GEN_DEBUG_DMA
#ifdef USE_DMA_64
		GEN_DEBUG_PRINT("\nBuffer%d Addr Low = 0x%x, High = 0x%x,  Size = 0x%x",
						(int)i, pDesc[i].PciAddress, pDesc[i].PciAddressHigh, pDesc[i].u32TransferSize);
#else
		GEN_DEBUG_PRINT("\nBuffer%d Addr = 0x%x,  Size = 0x%x", (int)i, pDesc[i].PciAddress, pDesc[i].u32TransferSize);
#endif
#endif

			PhysDmaNextDescriptor.LowPart += sizeof(PLX_DMA_DESCRIPTOR);
			FdoData->XferRead.u32CurrentDmaSize  += pDesc[i].u32TransferSize;
		}
	}
#endif // LINUX_VERSION_CODE

#ifdef GEN_DEBUG_DMA
	GEN_DEBUG_PRINT("\nTotal transferSize = %d", FdoData->XferRead.u32CurrentDmaSize);
#endif

	//----- Write the end of the chain in the last descriptor
	pDesc[u32DescCount-1].NextDescPtr = ( (1<<3) | (1<<1) | ( PciMemory <<0 ));	// End of chain

//TRACEOUT;
}

#endif


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void	StartDmaTransferDescriptor( PFDO_DATA	FdoData )
{
	BOOLEAN		bPciMemory = TRUE;
	uInt32		u32Value;

#ifdef _WINDOWS_
	//----- Address of the first descriptor
	_WriteRegister( FdoData, PCI9056_DMA0_DESC_PTR, (FdoData->PhysAddrDmaDesc.LowPart) | (1<<3) | ( bPciMemory<<0 ) );
#else
	PHYSICAL_ADDRESS		PhysDmaDesc;

	PhysDmaDesc.LowPart 	= (uInt32) (FdoData->PhysAddrDmaDesc & 0xFFFFFFFF);
	PhysDmaDesc.HighPart 	= (int32) ((FdoData->PhysAddrDmaDesc >> 16) >> 16);

	//----- Address of the first descriptor
	_WriteRegister( FdoData, PCI9056_DMA0_DESC_PTR, PhysDmaDesc.LowPart | (1<<3) | ( bPciMemory<<0 ) );
#endif

	//----- Start DMA Engine
	u32Value = _ReadRegister( FdoData, PCI9056_DMA_COMMAND_STAT );
	//----- enable the DMA Channel
	u32Value |= 1<<0;
	//----- start the transfer
	u32Value |= 1<<1;
	_WriteRegister( FdoData, PCI9056_DMA_COMMAND_STAT, u32Value );
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void	AbortDmaTransfer( PFDO_DATA	FdoData )
{
	uInt32 u32Value = _ReadRegister( FdoData, PCI9056_DMA_COMMAND_STAT );

	if ( u32Value & 0x10 )
		return;				// Dma done !! no need to abort

	// Clear Dma enable and start bits
	u32Value &= 0xFFFC;
	// set Abort bit
	u32Value |= 1 << 2;

	_WriteRegister(FdoData, PCI9056_DMA_COMMAND_STAT, u32Value);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

BOOLEAN	CheckBlockTransferDone( PFDO_DATA	FdoData, BOOLEAN bWaitComplete )
{
	int32	 i32RetruCount = 20000;
	volatile uInt32 u32Value;

	u32Value = (_ReadRegister( FdoData, PCI9056_DMA_COMMAND_STAT )) & DMA_CH0_DONE;
	while( 0 == u32Value  )
	{
		u32Value = (_ReadRegister( FdoData, PCI9056_DMA_COMMAND_STAT )) & DMA_CH0_DONE;
		if ( i32RetruCount-- < 0 )
		{
			AbortDmaTransfer(FdoData);
			return FALSE;
		}
	}
#ifdef GEN_DEBUG_DMA
	printk(KERN_INFO "CheckBlockTransferDone u32Value = 0x%x, i32RetruCount = %d\n", u32Value, i32RetruCount);
#endif
	return TRUE;;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void SetWatchDogTimer(PFDO_DATA	FdoData, uInt32 Time, BOOLEAN Run)
{
	WriteGageRegister(FdoData, GEN_COMMAND, 0);
	WriteGageRegister(FdoData, WATCH_DOG_REG, Time);
	if (Run)
		WriteGageRegister(FdoData, GEN_COMMAND, WATCH_DOG_RUN_MASK);
	else
		WriteGageRegister(FdoData, GEN_COMMAND,0);
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void	WriteGIO( PFDO_DATA	FdoData, uInt8 u8Addr, uInt32 u32Data )
{
	uInt32 Status = 1;

	WriteGageRegister(FdoData, GIO_READ_DATA_REG, u32Data);
	WriteGageRegister(FdoData, GIO_CMD_ADDR_REG, ((u8Addr << 8)& GIO_HOST_LOC_ADD_MASK) | GIO_HOST_LOC_REQ_MASK | GIO_HOST_LOC_RW_MASK);

	SetWatchDogTimer(FdoData, 0x80000, TRUE);

	while (!((ReadGageRegister(FdoData, GEN_COMMAND)) & GIO_HOST_LOC_ACK_MASK))
	{
		if ((ReadGageRegister(FdoData, GEN_COMMAND)) & WATCH_DOG_EXPIRE_Q_MASK)
		{
			Status = 0;
			break;
		}
	}

	WriteGageRegister(FdoData, GIO_CMD_ADDR_REG, 0);
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

uInt32  ReadGIO( PFDO_DATA	FdoData, uInt8 Addr)
{
	uInt32 Data = 0;

	WriteGageRegister(FdoData, GIO_CMD_ADDR_REG, ((Addr << 8)& GIO_HOST_LOC_ADD_MASK) | GIO_HOST_LOC_REQ_MASK);
	SetWatchDogTimer(FdoData, 0x80000, TRUE);

	while (!((ReadGageRegister(FdoData, GEN_COMMAND)) & GIO_HOST_LOC_ACK_MASK))
	{
		if ((ReadGageRegister(FdoData, GEN_COMMAND)) & WATCH_DOG_EXPIRE_Q_MASK)
			break;
	}

	WriteGageRegister(FdoData, GIO_CMD_ADDR_REG, 0);
	Data = ReadGageRegister(FdoData, GIO_READ_DATA_REG);

	return Data;
}

