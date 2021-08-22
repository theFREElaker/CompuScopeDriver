// AltCommonDefs.h : Common Definitions header file
//

#ifndef altcommondefs_h
#define altcommondefs_h


#define ALT_INT_FIFO			0x48	//----- READ
//#define ALT_FIFO_DATA			0x200	//----- READ
#define ALT_FDATA_OUT			0x4C
#define STREAM_STATUS0			0x7E
//#define STREAM_STATUS1			0x64

// ALTERADRIVER Interrupt Structure
typedef struct _ALT_INTR
{
    unsigned int INTERUPT_PENDING   :1;
    unsigned int ERROR_PENDING		:1;
    unsigned int LOCAL_REQ			:1;
    unsigned int TX_COMP			:1;
    unsigned int ACR_LOADED			:1;
    unsigned int CHAIN_STARTED		:1;
    unsigned int GAGE_LOCAL_IRQ     :1;
    unsigned int SwInterrupt		:1;
} ALT_INTR;

#define ALT_DMA_PAGES 256
#define DMA_MAP_OFFSET 0x0400

enum {
	ALTERA_REG_DMACSR  = 0x0 | DMA_MAP_OFFSET, /* Control Status register */
	ALTERA_REG_DMAWRA  = 0x4 | DMA_MAP_OFFSET, /* DMA DT WR ADDRESS */
	ALTERA_REG_DMASIZE = 0x8 | DMA_MAP_OFFSET, /* DMA DT WR SIZE + CONTROL */

	ALTERA_REG_DMASTAT	 = 0x0 | DMA_MAP_OFFSET, /* 0xbc DMA Status */
	ALTERA_REG_PCCONTROL = 0xC | DMA_MAP_OFFSET, /* Performance counter control */
	ALTERA_REG_PCTOTAL   = 0x10 | DMA_MAP_OFFSET, /* Performance counter total bytes */
	ALTERA_REG_DMAWRHA   = 0x14 | DMA_MAP_OFFSET, /* DMA DT WR SIZE HIGH */
};

enum {
 	INTERRUPT_ENABLE =	0x0001,
	FLASH			=   0x0002,
	FIFO_FLASH		=   0x0004,
	DMA_ENABLE		=	0x0010,
	TRXCOMPINTDIS	=	0x0020,
	CHAINEN			=	0x0100,
	DIRECTION		=	0x0008,
};
// ISR
enum {
	INTERUPT_PENDING	= 0x01,
	ERROR_PENDING		= 0x02,
	LOCAL_REQ			= 0x04,
	TX_COMP				= 0x08,
	ACR_LOADED			= 0x10,
	CHAIN_STARTED		= 0x20,
	GAGE_LOCAL_IRQ		= 0x40,
};

void	AltGageFreeMemoryForDmaDescriptor( PFDO_DATA	FdoData );
BOOLEAN	AltGageAllocateMemoryForDmaDescriptor( PFDO_DATA	FdoData );
void	AltStartDmaTransferDescriptor( PFDO_DATA	FdoData );
#ifdef _WINDOWS_
void	AltBuildDmaTransferDescriptor( PFDO_DATA	FdoData, PSCATTER_GATHER_LIST  ScatterGather );
#else
void	AltBuildDmaTransferDescriptor( PFDO_DATA	FdoData, struct scatterlist *ScatterGather, uInt32 NumberOfElements );
#endif
uInt32	AltBuildDmaDescContiguous( PFDO_DATA FdoData, uInt64 PhysAddr, uInt32 u32Size );
BOOLEAN	AltCheckBlockTransferDone( PFDO_DATA FdoData, BOOLEAN bWaitComplete );
void	AltAbortDmaTransfer( PFDO_DATA	FdoData );
void	AltEnableInterruptDMA( PFDO_DATA FdoData );
void	AltDisableInterruptDMA( PFDO_DATA FdoData );


#endif // !defined(_ALTCOMMONDEFS_H_)








