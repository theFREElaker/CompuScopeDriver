
#if !defined( _CSFASTBALLDMA_H_ )
#define _CSFASTBALLDMA_H_

#define PLDA_DMA_WR_LSB_ADDR			0x00
#define PLDA_DMA_WR_MSB_ADDR			0x04
#define PLDA_DMA_WR_SIZE				0x08
#define PLDA_DMA_WR_INFO				0x0C
#define	PLDA_DMA_RD_INFO				0x0C
#define	PLDA_DMA_INTR_RD_INFO			0x34


#define PLDA_DMA_START_CMD				0x4
#define PLDA_DMA_ABORT_CMD				0x8
#define PLDA_DMASG_CMD					0x10

#define PLDA_GIO_BASE_REG				0x8000
#define PLDA_GIO_TEST_REG				0x87


#define	MAX_DMA_DESC_BLOCKS				512


typedef struct _PLDA_DMA_DESCRIPTOR
{
	PHYSICAL_ADDRESS	PhysAddr;
	ULONG				u32SizeBytes;
	ULONG				NextDescPtr;

} PLDA_DMA_DESCRIPTOR, *PPLDA_DMA_DESCRIPTOR;


void	EnableInterrupts(PFDO_DATA FdoData, uInt32 IntType);
void	DisableInterrupts(PFDO_DATA FdoData, uInt32 IntType);
void	ConfigureInterrupts( PFDO_DATA FdoData, uInt32 u32IntrMask );
void	ClearInterrupts(PFDO_DATA	FdoData);

#ifdef __linux__
void 	BuildDmaTransferDescriptor( PFDO_DATA	FdoData, struct scatterlist *ScatterGather, uInt32 NumberOfElements );
#else
void	BuildDmaTransferDescriptor( PFDO_DATA	FdoData, PSCATTER_GATHER_LIST  ScatterGather );
#endif
void	StartDmaTransferDescriptor( PFDO_DATA	FdoData );
BOOL	CheckBlockTransferDone( PFDO_DATA FdoData, BOOLEAN bWaitComplete );
void	AbortDmaTransfer( PFDO_DATA	FdoData );



#endif
