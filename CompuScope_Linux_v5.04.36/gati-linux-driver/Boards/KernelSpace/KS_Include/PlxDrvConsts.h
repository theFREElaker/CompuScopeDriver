#ifndef __PlxdrvConsts_h__
#define __PlxdrvConsts_h__


#define MIN_SIZE_MAPPING_BAR_0	0x100 
#define PCI_NUM_BARS	6
#define FALSE	0
#define TRUE	1

#define	ONE_DWORD			4

#ifdef USE_DMA_64
	#define PADDING_DMA_DESC	8*ONE_DWORD
#else
	#define PADDING_DMA_DESC	4*ONE_DWORD
#endif

//!----- DMA offset (SGL = ScatterGatherList)
#define SGL_OFFSET_PCI			0x0
#define SGL_OFFSET_LOCAL		0x4
#define SGL_OFFSET_COUNT		0x8
#define SGL_OFFSET_NEXT_DESC	0xC

#define DMA_CH0_DONE	0x10

//---- interrupt
#define LOCAL_INPUT_INT_ACTIVE	0x8000
#define DMA_CH0_INT_ACTIVE		0x200000

//----- VPD Constants
#define VPD_COMMAND_MAX_RETRIES      5         //----- Max number VPD command re-issues
#define VPD_STATUS_MAX_POLL          50        //----- Max number of times to read VPD status
#define VPD_STATUS_POLL_DELAY        100        //----- Delay between polling VPD status (Milliseconds)
#define CAP_ID	3
#define MAX_PLX_NVRAM_OFFSET		 0x200

//------ PLX default Value for "NvRam"
#define PLX_DEVICE_ID			0x9056		
#define PLX_VENDOR_ID			0x10b5		
#define PLX_CLASS_CODE			0x0680		
#define PLX_CLASS_REV			0x00ab		
#define PLX_MAX_LATENCY			0x0000			
#define INTPIN					0x0100		
#define MAILBOX0				0x0			
#define MAILBOX1				0x0			
#define SPACE0_RANGE			0xfffffc00	
#define SPACE0_REMAP			0x20000001	
#define MODE_DMA_ARB			0x07a30304	
#define VPD_BOUND				0x0130		
#define ENDIAN					0x4500		
#define EXP_ROM_RANGE			0x0			
#define EXP_ROM_REMAP			0x0			
#define SPACE0_DESCP			0x4b4700c7	
#define DM_2_PCI_RANGE			0x0			
#define DM_MEM_LOC_BA			0x50000000	
#define DM_IO_LOC_BA			0x40000000	
#define DM_PCI_MEM_REMAP		0x0			
#define DM_PCI_IO_PCI_CONF		0x0			
#define SUBSYSTEM_ID			0x9656		
#define SUBSYSTEM_VENDOR_ID		0x10b5		
#define SPACE1_RANGE			0xffffff00	
#define SPACE1_REMAP			0x10000001	
#define SPACE1_DESC				0x00000047	
#define HOT_SWAP				0x00004c06	
#define PCI_ARB_CTRL			0x0			
											
 
//!-------------------------------------------------
//! PLX Registers Address
//!-------------------------------------------------

//!----- GENERIC PCI Configuration Registers Address
#define CFG_VENDOR_ID           0x000
#define CFG_COMMAND             0x004
#define CFG_REV_ID              0x008
#define CFG_CACHE_SIZE          0x00C
#define CFG_BAR0                0x010
#define CFG_BAR1                0x014
#define CFG_BAR2                0x018
#define CFG_BAR3                0x01C
#define CFG_BAR4                0x020	//!----- Reserved
#define CFG_BAR5                0x024	//!----- Reserved
#define CFG_CIS_PTR             0x028	//!----- Not supported
#define CFG_SUB_VENDOR_ID       0x02C
#define CFG_EXP_ROM_BASE        0x030
#define CFG_CAP_PTR             0x034
#define CFG_RESERVED1           0x038	//!----- Reserved
#define CFG_INT_LINE            0x03C


//!#if defined(PCI_CODE)
    #define PCI9056_REG_BASE         0x000
    #define PCI9056_REG_BASE_1       0x000
    #define PCI9056_NEW_CAP_BASE     0x000
//!#elif defined(IOP_CODE)
	//!ICI
//!    #define PCI9056_REG_BASE         0x080
//!    #define PCI9056_REG_BASE_1       0x0a0
//!    #define PCI9056_NEW_CAP_BASE     0x140
//!#endif

//!-----  Additional definitions
#define PCI9056_MAX_REG_OFFSET       (PCI9056_REG_BASE_1 + 0x108)
#define PCI9056_EEPROM_SIZE          0x05c          //!----- EEPROM size (bytes) used by PLX Chip
#define PCI9056_DMA_CHANNELS         2              //!-----  Number of DMA channels supported by PLX Chip


//!----- PCI Configuration Registers
#define PCI9056_VENDOR_ID            CFG_VENDOR_ID
#define PCI9056_COMMAND              CFG_COMMAND
#define PCI9056_REV_ID               CFG_REV_ID
#define PCI9056_CACHE_SIZE           CFG_CACHE_SIZE
#define PCI9056_RTR_MEM_BASE         CFG_BAR0
#define PCI9056_RTR_IO_BASE          CFG_BAR1
#define PCI9056_LOCAL_BASE0          CFG_BAR2
#define PCI9056_LOCAL_BASE1          CFG_BAR3
#define PCI9056_UNUSED_BASE1         CFG_BAR4
#define PCI9056_UNUSED_BASE2         CFG_BAR5
#define PCI9056_CIS_PTR              CFG_CIS_PTR
#define PCI9056_SUB_ID               CFG_SUB_VENDOR_ID
#define PCI9056_EXP_ROM_BASE         CFG_EXP_ROM_BASE
#define PCI9056_CAP_PTR              CFG_CAP_PTR
#define PCI9056_RESERVED2            CFG_RESERVED1
#define PCI9056_INT_LINE             CFG_INT_LINE
//!----- For the device specific area of the configuration space OFFSET are relative to 0x40
//!----- because of KPciConfiguration::ReadDeviceSpecificConfig 
//!----- and KPciConfiguration::WriteDeviceSpecificConfig
#define PCI9056_PM_CAP_ID            (PCI9056_NEW_CAP_BASE + 0x00)	//(PCI9056_NEW_CAP_BASE + 0x040)
#define PCI9056_PM_CSR				 (PCI9056_NEW_CAP_BASE + 0x04)	//(PCI9056_NEW_CAP_BASE + 0x044)
#define PCI9056_HS_CAP_ID            (PCI9056_NEW_CAP_BASE + 0x08)	//(PCI9056_NEW_CAP_BASE + 0x048)
#define PCI9056_VPD_CAP_ID           (PCI9056_NEW_CAP_BASE + 0x0c)	//(PCI9056_NEW_CAP_BASE + 0x04c)
#define PCI9056_VPD_DATA             (PCI9056_NEW_CAP_BASE + 0x10)	//(PCI9056_NEW_CAP_BASE + 0x050)


//!----- Local Configuration Registers to be removed if not useful ICI
#define PCI9056_SPACE0_RANGE         (PCI9056_REG_BASE   + 0x000)
#define PCI9056_SPACE0_REMAP         (PCI9056_REG_BASE   + 0x004)
#define PCI9056_LOCAL_DMA_ARBIT      (PCI9056_REG_BASE   + 0x008)
#define PCI9056_ENDIAN_DESC          (PCI9056_REG_BASE   + 0x00c)
#define PCI9056_EXP_ROM_RANGE        (PCI9056_REG_BASE   + 0x010)
#define PCI9056_EXP_ROM_REMAP        (PCI9056_REG_BASE   + 0x014)
#define PCI9056_SPACE0_ROM_DESC      (PCI9056_REG_BASE   + 0x018)
#define PCI9056_DM_RANGE             (PCI9056_REG_BASE   + 0x01c)
#define PCI9056_DM_MEM_BASE          (PCI9056_REG_BASE   + 0x020)
#define PCI9056_DM_IO_BASE           (PCI9056_REG_BASE   + 0x024)
#define PCI9056_DM_PCI_MEM_REMAP     (PCI9056_REG_BASE   + 0x028)
#define PCI9056_DM_PCI_IO_CONFIG     (PCI9056_REG_BASE   + 0x02c)
#define PCI9056_SPACE1_RANGE         (PCI9056_REG_BASE   + 0x0f0)
#define PCI9056_SPACE1_REMAP         (PCI9056_REG_BASE   + 0x0f4)
#define PCI9056_SPACE1_DESC          (PCI9056_REG_BASE   + 0x0f8)
#define PCI9056_DM_DAC               (PCI9056_REG_BASE   + 0x0fc)
#define PCI9056_ARBITER_CTRL         (PCI9056_REG_BASE_1 + 0x100)
#define PCI9056_ABORT_ADDRESS        (PCI9056_REG_BASE_1 + 0x104)


//!----- Runtime Registers
//!#if defined(PCI_CODE)
    #define PCI9056_MAILBOX0         0x078
    #define PCI9056_MAILBOX1         0x07c
//!#elif defined(IOP_CODE)
//!    #define PCI9056_MAILBOX0         0x0c0	//!ICI
//!    #define PCI9056_MAILBOX1         0x0c4
//!#endif

#define PCI9056_MAILBOX2             (PCI9056_REG_BASE + 0x048)
#define PCI9056_MAILBOX3             (PCI9056_REG_BASE + 0x04c)
#define PCI9056_MAILBOX4             (PCI9056_REG_BASE + 0x050)
#define PCI9056_MAILBOX5             (PCI9056_REG_BASE + 0x054)
#define PCI9056_MAILBOX6             (PCI9056_REG_BASE + 0x058)
#define PCI9056_MAILBOX7             (PCI9056_REG_BASE + 0x05c)
#define PCI9056_LOCAL_DOORBELL       (PCI9056_REG_BASE + 0x060)
#define PCI9056_PCI_DOORBELL         (PCI9056_REG_BASE + 0x064)
#define PCI9056_INT_CTRL_STAT        (PCI9056_REG_BASE + 0x068)
#define PCI9056_EEPROM_CTRL_STAT     (PCI9056_REG_BASE + 0x06c)
#define PCI9056_PERM_VENDOR_ID       (PCI9056_REG_BASE + 0x070)
#define PCI9056_REVISION_ID          (PCI9056_REG_BASE + 0x074)


//!----- DMA Registers
#define PCI9056_DMA0_MODE            (PCI9056_REG_BASE + 0x080)
#define PCI9056_DMA0_PCI_ADDR        (PCI9056_REG_BASE + 0x084)
#define PCI9056_DMA0_LOCAL_ADDR      (PCI9056_REG_BASE + 0x088)
#define PCI9056_DMA0_COUNT           (PCI9056_REG_BASE + 0x08c)
#define PCI9056_DMA0_DESC_PTR        (PCI9056_REG_BASE + 0x090)
#define PCI9056_DMA1_MODE            (PCI9056_REG_BASE + 0x094)
#define PCI9056_DMA1_PCI_ADDR        (PCI9056_REG_BASE + 0x098)
#define PCI9056_DMA1_LOCAL_ADDR      (PCI9056_REG_BASE + 0x09c)
#define PCI9056_DMA1_COUNT           (PCI9056_REG_BASE + 0x0a0)
#define PCI9056_DMA1_DESC_PTR        (PCI9056_REG_BASE + 0x0a4)
#define PCI9056_DMA_COMMAND_STAT     (PCI9056_REG_BASE + 0x0a8)
#define PCI9056_DMA_ARBIT            (PCI9056_REG_BASE + 0x0ac)
#define PCI9056_DMA_THRESHOLD        (PCI9056_REG_BASE + 0x0b0)
#define PCI9056_DMA0_PCI_DAC         (PCI9056_REG_BASE + 0x0b4)
#define PCI9056_DMA1_PCI_DAC         (PCI9056_REG_BASE + 0x0b8)


//!----- Messaging Unit Registers
#define PCI9056_OUTPOST_INT_STAT     (PCI9056_REG_BASE + 0x030)
#define PCI9056_OUTPOST_INT_MASK     (PCI9056_REG_BASE + 0x034)
#define PCI9056_MU_CONFIG            (PCI9056_REG_BASE + 0x0c0)
#define PCI9056_FIFO_BASE_ADDR       (PCI9056_REG_BASE + 0x0c4)
#define PCI9056_INFREE_HEAD_PTR      (PCI9056_REG_BASE + 0x0c8)
#define PCI9056_INFREE_TAIL_PTR      (PCI9056_REG_BASE + 0x0cc)
#define PCI9056_INPOST_HEAD_PTR      (PCI9056_REG_BASE + 0x0d0)
#define PCI9056_INPOST_TAIL_PTR      (PCI9056_REG_BASE + 0x0d4)
#define PCI9056_OUTFREE_HEAD_PTR     (PCI9056_REG_BASE + 0x0d8)
#define PCI9056_OUTFREE_TAIL_PTR     (PCI9056_REG_BASE + 0x0dc)
#define PCI9056_OUTPOST_HEAD_PTR     (PCI9056_REG_BASE + 0x0e0)
#define PCI9056_OUTPOST_TAIL_PTR     (PCI9056_REG_BASE + 0x0e4)
#define PCI9056_FIFO_CTRL_STAT       (PCI9056_REG_BASE + 0x0e8)

#define RANK_WIDTH					4
#define PLX_2_SODIMMS				0x1
#define PLX_2_RANKS					0x2


#endif			//! __PlxDrvConsts_h__
