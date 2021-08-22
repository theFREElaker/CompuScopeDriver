#include "GageDrv.h"
#include "CsDefines.h"
#include "CsErrors.h"
#include "CsIoctl.h"
#include "CsIoctlTypes.h"
#include "NiosApi.h"
#include "CsPlxDefines.h"
#include "PlxDrvConsts.h"
#include "PlxSupport.h"
#include "AlteraSupport.h"

#define    nDEBUG_DESCRIPTOR

#define  ALTERA_MAX_DMA_DESC_BLOCKS   256
#define  ALTERA_MAX_DMA_DESC_MAX_SIZE (0x80000-0x1000)      // -PAGE_SIZE to ensure the size is multiple of 4 bytes

BOOLEAN AltWriteBlockDescriptor( PFDO_DATA FdoData, uInt32   Address, uInt32 Length, BOOLEAN bLastBlock );

#ifdef _WINDOWS_
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void    AltGageFreeMemoryForDmaDescriptor( IN PFDO_DATA   FdoData )
{
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
BOOLEAN   AltGageAllocateMemoryForDmaDescriptor( IN PFDO_DATA   FdoData )
{
   return TRUE;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void AltBuildDmaTransferDescriptor( PFDO_DATA   FdoData,
                                    PSCATTER_GATHER_LIST  ScatterGather )
{
   uInt32               i = 0;
   uInt32               Addr = 0;
   uInt32               Size = 0;
   uInt32               Offset = 0;

   uInt32   u32DescCount = ScatterGather->NumberOfElements;

   if ( u32DescCount > ALTERA_MAX_DMA_DESC_BLOCKS )
      u32DescCount = ALTERA_MAX_DMA_DESC_BLOCKS;

   FdoData->XferRead.u32CurrentDmaSize      = 0;
   FdoData->XferRead.u32CurrentBlocksCount = 0;

#ifdef    DEBUG_DESCRIPTOR
   KdPrint(("DMA Descriptor:\n"));
   KdPrint(("Block         Addr         Length\n"));
#endif

   // Firmware requirement
   // If the buffer is not aligned on 4K boundary then its size should not cross 4K boundary
   Offset = (ScatterGather->Elements[0].Address.LowPart & 0xFFF);
   if ( 0 != Offset && ScatterGather->Elements[0].Length > (0x1000-Offset) )
   {
      Addr = ScatterGather->Elements[0].Address.LowPart;
      Size = 0x1000-Offset;

#ifdef    DEBUG_DESCRIPTOR
      KdPrint(("%03i      0x%x      0x%x\n", FdoData->XferRead.u32CurrentBlocksCount, Addr, Size ));
#endif
      _WriteRegister( FdoData, ALTERA_REG_DMAWRA, Addr );
      _WriteRegister( FdoData, ALTERA_REG_DMASIZE, Size );

      FdoData->XferRead.u32CurrentDmaSize      += Size;
      FdoData->XferRead.u32CurrentBlocksCount++;

      Addr += Size;
      Size = ScatterGather->Elements[0].Length - Size;

      if ( AltWriteBlockDescriptor( FdoData, Addr, Size, 1 == u32DescCount ) )
         return;

      i++;
      Addr = ScatterGather->Elements[i].Address.LowPart;
      Size = ScatterGather->Elements[i].Length;
   }
   else
   {
      Addr = ScatterGather->Elements[0].Address.LowPart;
      Size = ScatterGather->Elements[0].Length;
   }

   //----- build the descriptor array
   while( i < u32DescCount )
   {
      if ( AltWriteBlockDescriptor( FdoData, Addr, Size, i == u32DescCount-1 ) )
         break;

      i++;
      if  (i < u32DescCount)
      {
         Addr = ScatterGather->Elements[i].Address.LowPart;
         Size = ScatterGather->Elements[i].Length;
      }
   }
}

#else // __linux__
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
void AltBuildDmaTransferDescriptor (PFDO_DATA FdoData,
                                    struct scatterlist *ScatterGather,
                                    uInt32 NumberOfElements )
{
   uInt32  i = 0;
   uInt32  Addr = 0;
   uInt32  Size = 0;
   uInt32  Offset = 0;
   uInt32  pcilen;

   uInt32 u32DescCount = NumberOfElements;

   if (u32DescCount > ALTERA_MAX_DMA_DESC_BLOCKS)
      u32DescCount = ALTERA_MAX_DMA_DESC_BLOCKS;

   FdoData->XferRead.u32CurrentDmaSize     = 0;
   FdoData->XferRead.u32CurrentBlocksCount = 0;

#ifdef    DEBUG_DESCRIPTOR
   KdPrint(("DMA Descriptor:\n"));
   KdPrint(("Block         Addr         Length\n"));
#endif

   // Firmware requirement
   // If the buffer is not aligned on 4K boundary then its size should not cross 4K boundary
   Addr = (uInt32)sg_dma_address(&ScatterGather[0]);
   pcilen = sg_dma_len(&ScatterGather[0]);
   Offset = (Addr & 0xFFF);
   if ( 0 != Offset && pcilen > (0x1000-Offset) )
   {
      Size = 0x1000-Offset;

#ifdef    DEBUG_DESCRIPTOR
      KdPrint(("%03i      0x%x      0x%x\n", FdoData->XferRead.u32CurrentBlocksCount, Addr, Size ));
#endif
      _WriteRegister( FdoData, ALTERA_REG_DMAWRA, Addr );
      _WriteRegister( FdoData, ALTERA_REG_DMASIZE, Size );

      FdoData->XferRead.u32CurrentDmaSize      += Size;
      FdoData->XferRead.u32CurrentBlocksCount++;

      Addr += Size;
      Size = pcilen - Size;

      if ( AltWriteBlockDescriptor( FdoData, Addr, Size, 1 == u32DescCount ) )
         return;

      i++;
      Addr = (uInt32)sg_dma_address(&ScatterGather[i]);
      Size = sg_dma_len(&ScatterGather[i]);
   }
   else
   {
      Size = pcilen;
   }

   //----- build the descriptor array
   while( i < u32DescCount )
   {
      if ( AltWriteBlockDescriptor( FdoData, Addr, Size, i == u32DescCount-1 ) )
         break;

      i++;
      if  (i < u32DescCount)
      {
         Addr = (uInt32)sg_dma_address(&ScatterGather[i]);
         Size = sg_dma_len(&ScatterGather[i]);
      }
   }
}

#endif

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

uInt32 AltBuildDmaDescContiguous( PFDO_DATA FdoData, uInt32 PhysAddr, uInt32 u32Size )
{
   uInt32   i = 0;
   uInt32   u32DescCount = 1;         // One big contigous memory
   uInt32   u32SizeRemain = u32Size;
   uInt32   u32CurrentSize = ALTERA_MAX_DMA_DESC_MAX_SIZE;

   // The contiguous memory is too big that the firmware cannot handle
   // Break one big contiguous memory in many smaller one
   if ( u32Size > ALTERA_MAX_DMA_DESC_MAX_SIZE )
   {
      u32DescCount = u32Size/ALTERA_MAX_DMA_DESC_MAX_SIZE;
      if ( (u32Size % ALTERA_MAX_DMA_DESC_MAX_SIZE) > 0 )
         u32DescCount += 1;
   }

   // Contiguous memory too big !!!!
   if ( u32DescCount > ALTERA_MAX_DMA_DESC_BLOCKS )
      u32DescCount = ALTERA_MAX_DMA_DESC_BLOCKS;

   if ( u32SizeRemain < ALTERA_MAX_DMA_DESC_MAX_SIZE )
      u32CurrentSize = u32SizeRemain;

   while ( u32DescCount != 0 )
   {
      _WriteRegister( FdoData, ALTERA_REG_DMAWRA, PhysAddr + i*ALTERA_MAX_DMA_DESC_MAX_SIZE );
      if ( 1 == u32DescCount )
      {
         // Last chunk
         _WriteRegister( FdoData, ALTERA_REG_DMASIZE, u32CurrentSize | (1 << 19) );
      }
      else
         _WriteRegister( FdoData, ALTERA_REG_DMASIZE, u32CurrentSize );

      u32SizeRemain -= u32CurrentSize;
      if ( u32SizeRemain < ALTERA_MAX_DMA_DESC_MAX_SIZE )
         u32CurrentSize = u32SizeRemain;

      i++;
      u32DescCount--;
   }

   FdoData->XferRead.u32CurrentDmaSize = u32Size - u32SizeRemain;
   FdoData->XferRead.u32CurrentBlocksCount = u32DescCount;

   return (u32Size - u32SizeRemain);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void   AltStartDmaTransferDescriptor( PFDO_DATA   FdoData )
{
   _WriteRegister( FdoData, ALTERA_REG_DMACSR, 0x2 );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

BOOLEAN   AltCheckBlockTransferDone( PFDO_DATA   FdoData, BOOLEAN bWaitComplete )
{
   int32    i32RetruCount = 20000;
   volatile uInt32 u32Value;

   u32Value = _ReadRegister( FdoData, ALTERA_REG_DMASTAT );
   u32Value = (0xF & u32Value);
   while( 7 != u32Value )
   {
      u32Value = _ReadRegister( FdoData, ALTERA_REG_DMASTAT );
      u32Value = (0xF & u32Value);

      if ( i32RetruCount-- < 0 )
      {
         AltAbortDmaTransfer(FdoData);
         return FALSE;
      }
   }

   AltAbortDmaTransfer(FdoData);
#ifdef GEN_DEBUG_DMA
   printk(KERN_INFO "AltCheckBlockTransferDone u32Value = 0x%x, i32RetruCount = %d\n", u32Value, i32RetruCount);
#endif

   return TRUE;;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void   AltAbortDmaTransfer( PFDO_DATA   FdoData )
{
   _WriteRegister(FdoData, ALTERA_REG_DMACSR, 0x1);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void  AltEnableInterruptDMA( PFDO_DATA FdoData )
{
   //   FdoData->u32RegCSR &= ~DMA_INTR_DISABLE;
   FdoData->u32IntBaseMask |= ALT_DMA_DONE;
   WriteGageRegister(FdoData, INT_CFG, FdoData->u32IntBaseMask);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void  AltDisableInterruptDMA( PFDO_DATA FdoData )
{
   //   FdoData->u32RegCSR |= DMA_INTR_DISABLE;
   FdoData->u32IntBaseMask &= ~ALT_DMA_DONE;
   WriteGageRegister(FdoData, INT_CFG, FdoData->u32IntBaseMask);
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
BOOLEAN AltWriteBlockDescriptor( PFDO_DATA FdoData, uInt32   Address, uInt32 Length, BOOLEAN bLastBlock )
{
   BOOLEAN   bFull = FALSE;

   if ( Length > ALTERA_MAX_DMA_DESC_MAX_SIZE )
   {
      // The contiguous memory is too big that the firmware cannot handle
      // Break one big contiguous memory in many smaller one

      uInt32   u32StartAddr = Address;
      uInt32   u32SizeRemain = Length;
      uInt32   u32CurrentSize = ALTERA_MAX_DMA_DESC_MAX_SIZE;
      uInt32   j = 0;

#ifdef DEBUG_DESCRIPTOR
      KdPrint(("Break Descriptor ------  (Size = 0x%x)\n", Length));
#endif

      while ( u32SizeRemain > 0 && FdoData->XferRead.u32CurrentBlocksCount < (ALTERA_MAX_DMA_DESC_BLOCKS-1) )
      {
         _WriteRegister( FdoData, ALTERA_REG_DMAWRA, u32StartAddr + j*ALTERA_MAX_DMA_DESC_MAX_SIZE );
         _WriteRegister( FdoData, ALTERA_REG_DMASIZE, u32CurrentSize );

#ifdef    DEBUG_DESCRIPTOR
         KdPrint(("%03i      0x%x      0x%x\n", FdoData->XferRead.u32CurrentBlocksCount, u32StartAddr + j*ALTERA_MAX_DMA_DESC_MAX_SIZE, u32CurrentSize ));
#endif

         FdoData->XferRead.u32CurrentDmaSize      += u32CurrentSize;
         FdoData->XferRead.u32CurrentBlocksCount++;
         j++;

         u32SizeRemain -= u32CurrentSize;
         if ( u32SizeRemain <= ALTERA_MAX_DMA_DESC_MAX_SIZE )
         {
            u32CurrentSize = u32SizeRemain;
            break;
         }
      }

      _WriteRegister( FdoData, ALTERA_REG_DMAWRA, u32StartAddr + j*ALTERA_MAX_DMA_DESC_MAX_SIZE );

#ifdef    DEBUG_DESCRIPTOR
      KdPrint(("%03i      0x%x      0x%x\n", FdoData->XferRead.u32CurrentBlocksCount, u32StartAddr + j*ALTERA_MAX_DMA_DESC_MAX_SIZE, u32CurrentSize ));
#endif
      FdoData->XferRead.u32CurrentDmaSize  += u32CurrentSize;
      FdoData->XferRead.u32CurrentBlocksCount++;

      if ( bLastBlock || FdoData->XferRead.u32CurrentBlocksCount == (ALTERA_MAX_DMA_DESC_BLOCKS-1) )
      {
         bFull = TRUE;
         _WriteRegister( FdoData, ALTERA_REG_DMASIZE, u32CurrentSize | (1 << 19) );
      }
      else
         _WriteRegister( FdoData, ALTERA_REG_DMASIZE, u32CurrentSize );
   }
   else
   {
      _WriteRegister( FdoData, ALTERA_REG_DMAWRA, Address );

#ifdef    DEBUG_DESCRIPTOR
      KdPrint(("%03i      0x%x      0x%x\n", FdoData->XferRead.u32CurrentBlocksCount, Address, Length ));
#endif
      if ( bLastBlock )
         _WriteRegister( FdoData, ALTERA_REG_DMASIZE, Length | (1 << 19) );
      else
         _WriteRegister( FdoData, ALTERA_REG_DMASIZE, Length );

      FdoData->XferRead.u32CurrentDmaSize  += Length;
      FdoData->XferRead.u32CurrentBlocksCount++;
   }

   // return TRUE when descriptor is full or completed with the last block.
   return ( bLastBlock || bFull );
}
