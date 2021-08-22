
#include "CsDefines.h"
#include "CsErrors.h"
#include "CsIoctl.h"
#include "CsIoctlTypes.h"
#include "CsPlxDefines.h"
#include "DecadeGageDrv.h"
#include "DecadeNiosApi.h"
#include "DecadeAlteraSupport.h"

#define  nDEBUG_DESCRIPTOR

#define  DECADE_MAX_DMA_DESC_BLOCKS   1024
#define  DECADE_MAX_DMA_DESC_MAX_SIZE (0x80000-0x1000)

BOOLEAN AltWriteBlockDescriptor (PFDO_DATA FdoData, uInt64 Address, uInt32 Length, BOOLEAN bLastBlock);

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void AltGageFreeMemoryForDmaDescriptor (PFDO_DATA FdoData)
{
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
BOOLEAN AltGageAllocateMemoryForDmaDescriptor (PFDO_DATA FdoData)
{
   return TRUE;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void AltStartDmaTransferDescriptor (PFDO_DATA FdoData)
{
   // AltAbortDmaTransfer (FdoData);
   _WriteRegister (FdoData, ALTERA_REG_DMACSR, 0x2);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void AltAbortDmaTransfer (PFDO_DATA FdoData)
{
   _WriteRegister (FdoData, ALTERA_REG_DMACSR, 0x1);
   _WriteRegister (FdoData, ALTERA_REG_DMACSR, 0x0);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
uInt32 AltBuildDmaDescContiguous (PFDO_DATA FdoData, uInt64 PhysAddr, uInt32 u32Size)
{
   uInt32 desc_count = 1;      // One big contigous memory
   uInt32 size_remain = u32Size;
   uInt32 current_size = DECADE_MAX_DMA_DESC_MAX_SIZE;
   UPHYSICAL_ADDRESS addr;

   // The contiguous memory is too big that the firmware cannot handle
   // Break one big contiguous memory in many smaller one
   if (u32Size > DECADE_MAX_DMA_DESC_MAX_SIZE) {
      desc_count = u32Size/DECADE_MAX_DMA_DESC_MAX_SIZE;
      if ((u32Size % DECADE_MAX_DMA_DESC_MAX_SIZE) > 0)
         desc_count += 1;
   }

   // Contiguous memory too big !!!!
   if (desc_count > DECADE_MAX_DMA_DESC_BLOCKS)
      desc_count = DECADE_MAX_DMA_DESC_BLOCKS;

   if (size_remain < DECADE_MAX_DMA_DESC_MAX_SIZE)
      current_size = size_remain;

   addr.QuadPart = PhysAddr;
   FdoData->XferRead.u32CurrentBlocksCount = desc_count;
   while (desc_count != 0) {
      _WriteRegister (FdoData, ALTERA_REG_DMAWRHA, addr.HighPart);
      _WriteRegister (FdoData, ALTERA_REG_DMAWRA,  addr.LowPart);
      if (1 == desc_count)
         _WriteRegister (FdoData, ALTERA_REG_DMASIZE, current_size | (1 << 19)); // Last chunk
      else
         _WriteRegister (FdoData, ALTERA_REG_DMASIZE, current_size);

      size_remain -= current_size;
      if (size_remain < DECADE_MAX_DMA_DESC_MAX_SIZE)
         current_size = size_remain;

      addr.QuadPart += DECADE_MAX_DMA_DESC_MAX_SIZE;
      desc_count--;
   }
   FdoData->XferRead.u32CurrentDmaSize = u32Size - size_remain;
   return (u32Size - size_remain);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void AltBuildDmaTransferDescriptor (PFDO_DATA FdoData,
                                    struct scatterlist *ScatterGather,
                                    uInt32 NumberOfElements)
{
	uInt32					i = 0;
	uInt32					Size = 0;				// Size of contiguous memory
	PHYSICAL_ADDRESS		PhysAddress;			// Address of contigous memory
	uInt32					Offset = 0;
   uInt32 pcilen;
   UPHYSICAL_ADDRESS addr;

	uInt32	u32DescCount = NumberOfElements;

	if ( u32DescCount > DECADE_MAX_DMA_DESC_BLOCKS )
		 u32DescCount = DECADE_MAX_DMA_DESC_BLOCKS;

	// Reset counters
   FdoData->XferRead.u32CurrentDmaSize     = 0;
   FdoData->XferRead.u32CurrentBlocksCount = 0;

   // Firmware requirement
   // If the buffer is not aligned on 4K boundary then its size should not cross 4K boundary
   addr.QuadPart = sg_dma_address (&ScatterGather[0]);
   pcilen = sg_dma_len (&ScatterGather[0]);
   Offset = (addr.QuadPart & 0xFFF);
   if (0 != Offset && pcilen > (0x1000-Offset)) {
		Size = 0x1000-Offset;									// Size to align at 4K bounday

		// Program the first block of memory for DMA. 
      _WriteRegister (FdoData, ALTERA_REG_DMAWRHA, addr.HighPart);
      _WriteRegister (FdoData, ALTERA_REG_DMAWRA,  addr.LowPart);
      _WriteRegister (FdoData, ALTERA_REG_DMASIZE, Size);

      FdoData->XferRead.u32CurrentDmaSize += Size;
      FdoData->XferRead.u32CurrentBlocksCount++;

      addr.QuadPart += Size;
      Size = pcilen - Size;

		// Program the remaining blocks of  memory for DMA. 
      if (AltWriteBlockDescriptor (FdoData, addr.QuadPart, Size, 1 == u32DescCount))
         return;

      i++;
   }

   //----- build the descriptor array
   while (i < u32DescCount)
	{
      addr.QuadPart = sg_dma_address (&ScatterGather[i]);
      Size = sg_dma_len (&ScatterGather[i]);

		// Program the blocks of  memory for DMA. 
      if (AltWriteBlockDescriptor (FdoData, addr.QuadPart, Size, i == u32DescCount-1))
         break;

      i++;
   }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
BOOLEAN AltCheckBlockTransferDone (PFDO_DATA FdoData, BOOLEAN bWaitComplete)
{
   int32    i32RetryCount = 100000;
   volatile uInt32 u32Value;

   u32Value = _ReadRegister (FdoData, ALTERA_REG_DMASTAT);
   u32Value = (0xF & u32Value);
   while (7 != u32Value) {
      u32Value = _ReadRegister (FdoData, ALTERA_REG_DMASTAT);
      u32Value = (0xF & u32Value);

      if (i32RetryCount-- < 0) {
         GEN_DEBUG_PRINT (("Dma polling time out !!! (DMAstat=0x%x)\n", u32Value));
         AltAbortDmaTransfer (FdoData);
         return TRUE;
      }
   }

   //   GEN_DEBUG_PRINT (("Dma Polling Completed (DMAstat=0x%x)\n", u32Value));
   // AltAbortDmaTransfer (FdoData);
   return TRUE;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void  AltEnableInterruptDMA (PFDO_DATA FdoData)
{
   FdoData->u32IntBaseMask |= DMA_DONE_INTR;
   WriteGageRegister (FdoData, INT_CFG, FdoData->u32IntBaseMask);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void  AltDisableInterruptDMA (PFDO_DATA FdoData)
{
   FdoData->u32IntBaseMask &= ~DMA_DONE_INTR;
   WriteGageRegister (FdoData, INT_CFG, FdoData->u32IntBaseMask);
}


//------------------------------------------------------------------------------
// This function programs the descriptor for one contiguous block of memory
// It take into account the limitation of firmware. It the contiguous memory is too
// big for firmware then it will divide the big contigous memory into many smaller ones
//------------------------------------------------------------------------------
BOOLEAN AltWriteBlockDescriptor (PFDO_DATA FdoData, uInt64 Address, uInt32 Length, BOOLEAN bLastChunk)
{
	UPHYSICAL_ADDRESS	ChunkAddr = (UPHYSICAL_ADDRESS)Address;
	uInt32				u32ChunkSize	= DECADE_MAX_DMA_DESC_MAX_SIZE;
	uInt32				u32SizeRemain	= Length;
	uInt32				EndDescriptorMark = 0;			// Mark the end of the DMA descriptor (for one DMA transfer)
	BOOLEAN				bDescriptorFull = FALSE;

	if ( Length > DECADE_MAX_DMA_DESC_MAX_SIZE )
	{
		// The contiguous memory is too big and  the firmware cannot handle
		// Break one big contiguous memory in many smaller ones (Chunks)

      /* GEN_DEBUG_PRINT (("Break Descriptor ------  (size = 0x%x)\n", Length)); */

		while ( u32SizeRemain > 0 && FdoData->XferRead.u32CurrentBlocksCount < (DECADE_MAX_DMA_DESC_BLOCKS-1) )
		{
			_WriteRegister( FdoData, ALTERA_REG_DMAWRHA, ChunkAddr.HighPart );
			_WriteRegister( FdoData, ALTERA_REG_DMAWRA, ChunkAddr.LowPart );	//u32StartAddr + j*DECADE_MAX_DMA_DESC_MAX_SIZE );
         _WriteRegister (FdoData, ALTERA_REG_DMASIZE, u32ChunkSize);

         /* GEN_DEBUG_PRINT (("%03i   0x%x   0x%x\n", FdoData->XferRead.u32CurrentBlocksCount, startaddr.QuadPart, current_size)); */

			u32SizeRemain							-= u32ChunkSize;
         FdoData->XferRead.u32CurrentDmaSize += u32ChunkSize;
         FdoData->XferRead.u32CurrentBlocksCount++;

			// Next Chunk
			ChunkAddr.QuadPart += u32ChunkSize;
			if ( u32SizeRemain <= u32ChunkSize )
			{
            u32ChunkSize = u32SizeRemain;
            break;
         }
      }

		// The last chunk
		if ( bLastChunk || FdoData->XferRead.u32CurrentBlocksCount == (DECADE_MAX_DMA_DESC_BLOCKS-1) )
		{
			// Mark the end of DMA descriptor
			EndDescriptorMark |= (1 << 19);
			bDescriptorFull = TRUE;
		}

		_WriteRegister( FdoData, ALTERA_REG_DMAWRHA, ChunkAddr.HighPart );
		_WriteRegister( FdoData, ALTERA_REG_DMAWRA, ChunkAddr.LowPart );	
		_WriteRegister( FdoData, ALTERA_REG_DMASIZE, u32ChunkSize | EndDescriptorMark );		

		/* GEN_DEBUG_PRINT(("WriteBlkEnd: %03i		0x%08x|%08x		%ld   EndDescMark 0x%x\n", 
            FdoData->XferRead.u32CurrentBlocksCount, ChunkAddr.HighPart, ChunkAddr.LowPart, u32ChunkSize, EndDescriptorMark )); */

		FdoData->XferRead.u32CurrentDmaSize		+= u32ChunkSize;
		FdoData->XferRead.u32CurrentBlocksCount++;
   }
	else
	{
		// Mark the end of DMA descriptor
		if ( bLastChunk || FdoData->XferRead.u32CurrentBlocksCount == (DECADE_MAX_DMA_DESC_BLOCKS-1) ) 
		{
			EndDescriptorMark  |= (1 << 19);		
			bDescriptorFull = TRUE;
		}

		_WriteRegister( FdoData, ALTERA_REG_DMAWRHA, ChunkAddr.HighPart );
		_WriteRegister( FdoData, ALTERA_REG_DMAWRA, ChunkAddr.LowPart );
		_WriteRegister( FdoData, ALTERA_REG_DMASIZE, Length | EndDescriptorMark );

#ifdef DEBUG_DESCRIPTOR
      if (EndDescriptorMark)
		KdPrint(("WriteBlkOne: %03i		0x%08x|%08x		%ld\n", FdoData->XferRead.u32CurrentBlocksCount, PhysAddress.HighPart, PhysAddress.LowPart, Length ));
#endif

		FdoData->XferRead.u32CurrentDmaSize  += Length;
		FdoData->XferRead.u32CurrentBlocksCount++;
	}

   // return TRUE when descriptor is full or completed with the last block.
	return ( bDescriptorFull );
}
