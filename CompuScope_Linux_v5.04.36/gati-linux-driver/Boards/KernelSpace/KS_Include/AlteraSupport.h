// AltCommonDefs.h : Common Definitions header file 
//

#if !defined(_ALTCOMMONDEFS_H_)
#define _ALTCOMMONDEFS_H_

#define ALT_INT_FIFO			0x48	//----- READ
//#define ALT_FIFO_DATA			0x200	//----- READ
#define ALT_FDATA_OUT			0x4C
#define STREAM_STATUS0			0x60
#define STREAM_STATUS1			0x64



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

enum {
	ALTERA_REG_DMACSR  = 0x0, /* 0 Control Status register */
	ALTERA_REG_DMAWRA  = 0x4, /* 4 DMA DT WR ADDRESS */
	ALTERA_REG_DMASIZE = 0x8, /* 8 DMA DT WR SIZE + CONTROL */

	ALTERA_REG_DMASTAT	 = 0x0, /* 0xbc DMA Status */
	ALTERA_REG_PCCONTROL = 0xC, /* Performance counter control */
	ALTERA_REG_PCTOTAL   = 0x10, /* Performance counter total bytes */


	ALTERA_REG_DMAACR = 0x4, /* 4 PCI Address - Host memory address */
	ALTERA_REG_DMABCR = 0x8, /* 8 Byte Count Register - In SG mode */
	ALTERA_REG_DMAISR = 0xc, /* c Interrupt Status register */
	ALTERA_REG_DMALAR = 0x10,
	ALTERA_REG_TARG_PERF = 0x80,
	ALTERA_REG_MSTR_PERF = 0x84,
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
uInt32	AltBuildDmaDescContiguous( PFDO_DATA FdoData, uInt32 PhysAddr, uInt32 u32Size );
BOOLEAN	AltCheckBlockTransferDone( PFDO_DATA FdoData, BOOLEAN bWaitComplete );
void	AltAbortDmaTransfer( PFDO_DATA	FdoData );
void	AltEnableInterruptDMA( PFDO_DATA FdoData );
void	AltDisableInterruptDMA( PFDO_DATA FdoData );

#endif // !defined(_ALTCOMMONDEFS_H_)








