
#ifndef  _PLXSUPPORT_H_
#define _PLXSUPPORT_H_

#ifdef __linux__
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/scatterlist.h>
#endif


#define	MAX_DMA_DESC_BLOCKS				512

typedef struct _DMA_DESCRIPTOR_BLOCK
{
	uInt32		PciAddress;
	uInt32		LocalAddress;
	uInt32		u32TransferSize;
	uInt32		NextDescPtr;

#ifdef USE_DMA_64
	uInt32		PciAddressHigh;
	uInt32		Padding[3];			// Ensure structure size is 32-byte aligned
#endif

} PLX_DMA_DESCRIPTOR, *PPLX_DMA_DESCRIPTOR;


void	PlxInterruptEnable( PFDO_DATA	FdoData, BOOLEAN bDmaIntr );
void	PlxInterruptDisable( PFDO_DATA	FdoData, BOOLEAN bDmaIntr );
void	EnableInterrupts(PFDO_DATA FdoData, uInt32 IntType);
void	DisableInterrupts(PFDO_DATA FdoData, uInt32 IntType);
void	ConfigureInterrupts( PFDO_DATA FdoData, uInt32 u32IntrMask );
void	ClearInterrupts(PFDO_DATA	FdoData);

void	StartDmaTransferDescriptor( PFDO_DATA	FdoData );
void	AbortDmaTransfer( PFDO_DATA	FdoData );
BOOLEAN	CheckBlockTransferDone( PFDO_DATA	FdoData, BOOLEAN bWaitComplete );


void	WriteGIO( PFDO_DATA	FdoData, uInt8 u8Addr, uInt32 u32Data );
uInt32	ReadGIO( PFDO_DATA	FdoData, uInt8 u8Addr );
void	WriteFlashByte( PFDO_DATA	FdoData, uInt32 Addr, uInt32 Data );
uInt32	ReadFlashByte( PFDO_DATA	FdoData, uInt32 Addr );

void	GageFreeMemoryForDmaDescriptor( PFDO_DATA	FdoData );
BOOLEAN	GageAllocateMemoryForDmaDescriptor( PFDO_DATA	FdoData );
#ifdef _WINDOWS_
void BuildDmaTransferDescriptor( PFDO_DATA	FdoData, PSCATTER_GATHER_LIST  ScatterGather );

#else
void 	BuildDmaTransferDescriptor( PFDO_DATA	FdoData,
								 struct scatterlist *ScatterGather, uInt32 NumberOfElements );
#endif


#endif
