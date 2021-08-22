#ifdef _WIN32
//#include "CSFastBall.h"
#include "PldaSupport.h"
#else
#include "GageDrv.h"
#include "PldaSupport.h"
#endif

#include "NiosApi.h"
#include "CsErrors.h"
#include "CsPlxDefines.h"

#ifdef _WIN32

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
BOOLEAN	GageAllocateMemoryForDmaDescriptor( PFDO_DATA	FdoData )
{
	PHYSICAL_ADDRESS	PhysAddr;

	// 32 bit address
	PhysAddr.QuadPart = 0xFFFFFFFF;

	// Allocate memory for Dma descriptor
	FdoData->u32MaxDesc = MAX_DMA_DESC_BLOCKS;
	FdoData->pDmaDescriptor = MmAllocateContiguousMemory( FdoData->u32MaxDesc * sizeof(PLDA_DMA_DESCRIPTOR), PhysAddr);
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
void	 GageFreeMemoryForDmaDescriptor( PFDO_DATA	FdoData )
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

void BuildDmaTransferDescriptor( IN PFDO_DATA	FdoData,
								 PSCATTER_GATHER_LIST  ScatterGather )
{
	uInt32					i = 0;
	PPLDA_DMA_DESCRIPTOR	pDesc = (PPLDA_DMA_DESCRIPTOR) FdoData->pDmaDescriptor;
	PHYSICAL_ADDRESS		PhysDmaNextDescriptor = FdoData->PhysAddrDmaDesc;

	uInt32	u32DescCount = ScatterGather->NumberOfElements;

	if ( u32DescCount > FdoData->u32MaxDesc )
		 u32DescCount = FdoData->u32MaxDesc;

	FdoData->XferRead.u32CurrentDmaSize = 0;
	//----- build the descriptor array
	for (i = 0; i < u32DescCount; i++)
	{
		pDesc[i].PhysAddr.QuadPart	= ScatterGather->Elements[i].Address.QuadPart;
		pDesc[i].u32SizeBytes		= ScatterGather->Elements[i].Length;

//        KdPrint(("<-- PhysAddr 0x%x\n", pDesc[i].PhysAddr.LowPart));
//        KdPrint(("<-- Size %d\n", pDesc[i].u32SizeBytes));

		pDesc[i].NextDescPtr	= PhysDmaNextDescriptor.LowPart + sizeof(PLDA_DMA_DESCRIPTOR);
		PhysDmaNextDescriptor.LowPart += sizeof(PLDA_DMA_DESCRIPTOR);

		FdoData->XferRead.u32CurrentDmaSize  += pDesc[i].u32SizeBytes;
	}

    KdPrint(("DMA DescCount =  %d, Total Bytes = 0x%08x\n", u32DescCount, FdoData->XferRead.u32CurrentDmaSize));

	//----- Write the end of the chain in the last descriptor
	pDesc[u32DescCount-1].NextDescPtr |= 1;			// End of chain

}

#else

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
BOOLEAN	GageAllocateMemoryForDmaDescriptor( PFDO_DATA	FdoData )
{
	BOOLEAN	 bRet = TRUE;

TRACEIN;
	// Allocate memory for Dma descriptor
	FdoData->u32MaxDesc = MAX_DMA_DESC_BLOCKS;
	FdoData->pDmaDescriptor = dma_alloc_coherent (FdoData->Device, FdoData->u32MaxDesc,
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
		dma_free_coherent(FdoData->Device, FdoData->u32MaxDesc,
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
	uInt32					i = 0;
	PPLDA_DMA_DESCRIPTOR	pDesc = (PPLDA_DMA_DESCRIPTOR) FdoData->pDmaDescriptor;
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
		pDesc[i].PhysAddr.HighPart 	 = 0;
		pDesc[i].PhysAddr.LowPart	 = sg_dma_address(&ScatterGather[i]);
		pDesc[i].u32SizeBytes		 = sg_dma_len(&ScatterGather[i]);

#ifdef GEN_DEBUG_DMA
		GEN_DEBUG_PRINT("\nBuffer%d Addr = 0x%lx,  Size = 0x%lx", (int)i, pDesc[i].PhysAddr.LowPart, pDesc[i].u32SizeBytes);
#endif

		pDesc[i].NextDescPtr	= PhysDmaNextDescriptor.LowPart + sizeof(PLDA_DMA_DESCRIPTOR);
		PhysDmaNextDescriptor.QuadPart += sizeof(PLDA_DMA_DESCRIPTOR);

		FdoData->XferRead.u32CurrentDmaSize  += pDesc[i].u32SizeBytes;
	}
#else
	{
		struct scatterlist *sg;// = ScatterGather;
		//----- build the descriptor array
		//----- use new macro (for_each_sg) to loop thru the scatterlist segments
		for_each_sg(ScatterGather, sg, u32DescCount, i)
		{
			pDesc[i].PhysAddr.HighPart 	 = 0;
			pDesc[i].PhysAddr.LowPart	 = sg_dma_address(sg);
			pDesc[i].u32SizeBytes		 = sg_dma_len(sg);

#ifdef GEN_DEBUG_DMA
		GEN_DEBUG_PRINT("\nBuffer%d Addr = 0x%lx,  Size = 0x%lx",(int)i,pDesc[i].PhysAddr.LowPart,pDesc[i].u32SizeBytes);
#endif
			pDesc[i].NextDescPtr	= PhysDmaNextDescriptor.LowPart + sizeof(PLDA_DMA_DESCRIPTOR);
			PhysDmaNextDescriptor.QuadPart += sizeof(PLDA_DMA_DESCRIPTOR);

			FdoData->XferRead.u32CurrentDmaSize  += pDesc[i].u32SizeBytes;
		}
	}

#endif // LINUX_VERSION < 2.6.24


#ifdef GEN_DEBUG_DMA
	GEN_DEBUG_PRINT("\nTotal transferSize = %ld", FdoData->XferRead.u32CurrentDmaSize);
#endif

	//----- Write the end of the chain in the last descriptor
	pDesc[u32DescCount-1].NextDescPtr |= 1;			// End of chain

//TRACEOUT;
}



#endif

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void	StartDmaTransferDescriptor( PFDO_DATA	FdoData )
{

#ifdef _WINDOWS_
	_WriteRegister( FdoData, PLDA_DMA_WR_LSB_ADDR, FdoData->PhysAddrDmaDesc.LowPart );
	_WriteRegister( FdoData, PLDA_DMA_WR_MSB_ADDR, FdoData->PhysAddrDmaDesc.HighPart );
#else
	{
	PHYSICAL_ADDRESS		PhysDmaDesc;

	PhysDmaDesc.LowPart = (uInt32) (FdoData->PhysAddrDmaDesc & 0xFFFFFFFF);
	PhysDmaDesc.HighPart = (int32) ((FdoData->PhysAddrDmaDesc >> 16) >> 16);

	AbortDmaTransfer( FdoData );

	_WriteRegister( FdoData, PLDA_DMA_WR_LSB_ADDR, PhysDmaDesc.LowPart );
	_WriteRegister( FdoData, PLDA_DMA_WR_MSB_ADDR, PhysDmaDesc.HighPart );
	}
#endif

	_WriteRegister( FdoData, PLDA_DMA_WR_SIZE, FdoData->XferRead.u32CurrentDmaSize );

	// Start transfer
	_WriteRegister( FdoData, PLDA_DMA_WR_INFO, PLDA_DMA_START_CMD | PLDA_DMASG_CMD);
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void	AbortDmaTransfer( PFDO_DATA	FdoData )
{
	_WriteRegister( FdoData, PLDA_DMA_WR_INFO, PLDA_DMA_ABORT_CMD );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

BOOLEAN	CheckBlockTransferDone( PFDO_DATA FdoData, BOOLEAN bWaitComplete )
{
	int32	 i32RetruCount = 20000;
	volatile uInt32 u32Value;

	u32Value = _ReadRegister( FdoData, PLDA_DMA_RD_INFO );
	if ( bWaitComplete )
	{
		while( u32Value & 0xF04 )
		{
			u32Value = _ReadRegister( FdoData, PLDA_DMA_RD_INFO );
			if ( i32RetruCount-- < 0 )
			{
				AbortDmaTransfer(FdoData);
				return FALSE;
			}
		}
		return TRUE;
	}
	else
		return( 0 == (u32Value & 0xF04) );
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
BOOLEAN	IsDmaInterrupt( PFDO_DATA	FdoData )
{
	FdoData->u32RegCSR = _ReadRegister( FdoData, PLDA_DMA_INTR_RD_INFO );

	return  (BOOLEAN)(1 & FdoData->u32RegCSR);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void ClearInterrupts( PFDO_DATA FdoData )
{
	FdoData->u32IntBaseMask  = 0;
	WriteGageRegister( FdoData, INT_CFG, 0 );

	//----- Clear INT_FIFO and INT_COUNT
	WriteGageRegister(FdoData, MODESEL, INT_CLR);
	WriteGageRegister(FdoData, MODESEL, 0);
}


//------------------------------------------------------------------------------
//       EnableInterrupts: all the interrupts except DMA int
//------------------------------------------------------------------------------

void EnableInterrupts(PFDO_DATA FdoData, uInt32 IntType)
{
	FdoData->u32IntBaseMask |= IntType;
	WriteGageRegister( FdoData, INT_CFG, FdoData->u32IntBaseMask );
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void DisableInterrupts(PFDO_DATA FdoData, uInt32 IntType)
{
	FdoData->u32IntBaseMask = ReadGageRegister( FdoData, INT_CFG );
	FdoData->u32IntBaseMask &= ~IntType;
	WriteGageRegister( FdoData, INT_CFG, FdoData->u32IntBaseMask );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void ConfigureInterrupts( PFDO_DATA FdoData,  uInt32 u32IntrMask )
{
	FdoData->u32IntBaseMask = u32IntrMask;
	WriteGageRegister(FdoData, INT_CFG, u32IntrMask);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void  ClearInterruptDMA0( PFDO_DATA	FdoData )
{
	uInt32	u32RegCSR = _ReadRegister( FdoData, PLDA_DMA_INTR_RD_INFO );

	u32RegCSR |= DMA_INTR_CLEAR;

	_WriteRegister( FdoData, PLDA_DMA_INTR_RD_INFO, u32RegCSR );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void  EnableInterruptDMA0( PFDO_DATA FdoData )
{	
	FdoData->u32RegCSR &= ~DMA_INTR_DISABLE;

	_WriteRegister( FdoData, PLDA_DMA_INTR_RD_INFO, FdoData->u32RegCSR );
}



//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void  DisableInterruptDMA0( PFDO_DATA FdoData )
{
	FdoData->u32RegCSR |= DMA_INTR_DISABLE;

	_WriteRegister( FdoData, PLDA_DMA_INTR_RD_INFO, FdoData->u32RegCSR );
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void  IntrDMAInitialize( PFDO_DATA FdoData )
{

	FdoData->u32RegCSR = _ReadRegister( FdoData, PLDA_DMA_INTR_RD_INFO );
}

