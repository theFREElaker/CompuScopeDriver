#ifndef __CsPlxDefines_h__
#define __CsPlxDefines_h__

#include "CsTypes.h"
#include "CsDrvConst.h"



#define NIOS_TIMEOUT	0x200000

#define DATA_WRITE_TO_NIOS	0X0	//----- WRITE, (nios point of view address 0x0)
#define DATA_READ_FROM_NIOS	0X0	//----- READ   (nios point of view address 0x0)
#define COMMAND_TO_NIOS		0X4	//----- WRITE  (nios point of view address 0x1)
//General status register (busy, trig..)
#define STATUS_OF_ACQ		0X4	//----- READ	(nios point of view address 0x1)

#define STATUS_IDLE			0x0
#define STATUS_ACQ_DONE		0x1		//---- acq done ready to be read
#define STATUS_ACQ_WRITE	0x2	    //---- write to the memory
#define STATUS_ACQ_READ		0x4		//---- read the memory
#define STATUS_REC_DONE		0x8		//---- previous segment done
#define STATUS_ABORT_WRITE	0x10	//---- acq aborted during the write
#define STATUS_ABORT_READ	0x20	//---- acq aborted during the read
#define STATUS_ACQ_TIMEOUT	0x40	//----
#define STATUS_BUSY			0x80	//----
#define WD_TIMER_EXPIRED    0x2000  //---- Watch dog timer has expired

#define STATUS_OF_NIOS		0x4	//				(nios point of view address 0x1)
#define MISC_REG			0x8	//				(nios point of view address 0x2)

//----- WRITE
#define	EXE_TO_NIOS			0x1	//bit
#define ACK_TO_NIOS			0x2 //bit
//----- READ
#define	EXE_DONE_FROM_NIOS	0x1 //bit
#define INT_FROM_NIOS_BIS	0x2 //bit
#define PASS_FAIL_NIOS_BIS	0x4 //bit

#define FAILED_NIOS_DATA		0xA5A5A5A5

#define TIMESTAMP_STATUS	0x10
//---- details of the timestamp register
#define TS_MCLK_SRC	        0x01
#define TS_FREE_RUN	        0x02
#define TS_RESET			0x80


#define MODESEL				0xC		//	(nios point of view address 0x3)
#define INTR_CLEAR			0xC		//	(nios point of view address 0x3)
//----- WRITE
#define	MODESEL0			0X0	//bit 1,0
#define MODESEL1			0X1 //bit 1,0
#define MODESEL2			0X2 //bit 1,0
#define MODESEL3			0X3 //bit 1,0
#define	MSEL_CLR			0X4 //bit
#define INT_CLR				0X8 //bit
#define TEST_REG			0x10 //				(nios point of view address 0x4)R/W Reagister for testing

#define GEN_COMMAND_R_REG	0X10 //----- WRITE	(nios point of view address 0x4)
#define GIO_WRITE_DATA_REG	0X14 //----- WRITE	(nios point of view address 0x5)
#define GIO_READ_DATA_REG	0X14 //----- READ	(nios point of view address 0x5)
#define GIO_CMD_ADDR_REG	0X18 //----- WRITE	(nios point of view address 0x6)
//----- Access to the IO bus
#define GIO_HOST_LOC_REQ_MASK	0x00000002
#define GIO_HOST_LOC_RW_MASK	0x00000001
#define GIO_HOST_LOC_ADD_MASK	0x0000FF00
#define GIO_HOST_LOC_ACK_MASK	0x80000000

#define GEN_COMMAND					0x1C	//----- WRITE	(nios point of view address 0x7)
#define	ADDON_EXCLUSIVE_RESET		0x02	// Bit 1
#define	SW_FORCED_TRIGGER			0x80	// Bit 7
#define WATCH_DOG_RUN_MASK			0x100	// bit 8
#define WATCH_DOG_STOP_MASK			0x200	// bit 9
#define WD_TIMER_FREQ_100us         6600    // 66 MHz

#define NIOS_GEN_STATUS			0x1C	//----- READ	(nios point of view address 0x7)
#define NIOS_BUSY			0x1		// Bit	0

#define FDATA_OUT			0x200	//----- READ	(nios point of view address 0x80)
#define INT_STATUS			0x20	//----- READ	(nios point of view address 0x8)
#define INT_COUNT			0x24	//----- READ	(nios point of view address 0x9)
#define INT_CFG				0x2C	//----- WRITE/READ	(nios point of view address 0xB)
#define WD_TIMER			0x34	//----- WRITE	(nios point of view address 0xd)
#define INT_FIFO			0x100	//----- READ	(nios point of view address 0x40)
#define FIFO_DATA			0x200	//----- READ	(nios point of view address 0x80)

#define USER_REG			0x80	// READ/WRITE	(nios point of view address 0x20)
#define USER_REG_INDEX		0x84	// WRITE		(nios point of view address 0x21)


#define SRAM_REG			0x8000	//----- READ/WRITE	(nios point of view address 0x2000)
#define SRAM_SIZE			0x4000	// Size in bytes

#define PLX_REC_COUNT_MASK  0x1ffffff


// Bit defined for Interrupt config register (INT_CFG (0x2C) )
//---- INT_CFG is the INT Mask Register
#define MAOINT				0x0001	//Bit0
#define INT_FROM_NIOS		0x0002	//Bit1
#define MBUSY_Q				0x0004	//Bit2
#define RECORD_DONE			0x0008	//Bit3
#define GGATE				0x0010	//Bit4
#define MTRIGTO_Q			0x0020	//Bit5
#define MCAPT				0x0040	//Bit6
#define GDVALID				0x0080	//Bit7
#define MPULSEEN_Q			0x0100	//Bit8
#define N_MBUSY_REC_DONE	0x0200	//Bit9
#define MBUSY_REC_DONE		0x0400	//Bit10
#define ALT_DMA_DONE		0x0800	//Bit11
#define EXPIRE				0x1000	//Bit12
#define EXPIRE_Q			0x2000	//Bit13
#define ISRUNNING			0x4000	//Bit14
#define MTRIG				0x8000	//Bit15
#define ACQ_FIFO_FULL		0x10000	//Bit16
#define HOST_FIFO_EMPTY		0x20000	//Bit17
#define ADDONFIFO_HALF		0x40000	//Bit18
#define ADDONFIFO_ALMOSTFULL	0x80000	//Bit19
#define MULREC_AVG_TRIGGERED	0x100000	//Bit20
#define STREAM_READY		0x100000	//Bit20
#define DMA_TIMEOUT			0x200000	//Bit21
#define STREAM_OVERFLOW		0x400000	//Bit22
#define MULREC_AVG_DONE		0x800000	//Bit23


#define FIRMWARE_VERSION	0x30	//----- READ  (nios point of view address 0xC)
#define TRIG_TIMEOUT_REG	0x30	//----- WRITE (nios point of view address 0xC)
#define FIRMWARE_OPTIONS	0x34	//----- READ  (nios point of view address 0xd)	(Rabbit only)
#define WATCH_DOG_REG		0x34	//----- WRITE (nios point of view address 0xd)
#define	RECONFIG_REG		0x60	//----- WRITE (nios point of view address 0x18) (Bunny only)

//#define WATCH_DOG_EXPIRE_MASK		0x00004000
#define WATCH_DOG_EXPIRE_Q_MASK		0x00002000
#define WATCH_DOG_IS_RUNNING_MASK	0x00008000

#define MASK_REG			0x40	//----- READ/WRITE (nios point of view address 0x10)


//----- FIFO REAL READ ADDRESS	Real LOCAL physical address
#define FIFO_READ_ADD		FDATA_OUT | 0x20000000

//----- Real LOCAL physical address
#define SRAM_ADDR			SRAM_REG  | 0x20000000


//----- NIOS COMMAND TABLE
#define NOP						0X0
#define READ_SPD				0X80000000
#define SET_RECORD_LENGTH		0X00000001
#define SET_NUMBER_OF_RECORD	0X00000002
#define SET_PRE_TRIGGER_LENGTH	0X00000003
#define SET_POST_TRIGGER_LENGTH	0X00000004
#define START_ACQUISITION		0X00001000
#define SET_START_ADDRESS		0X00000005
#define CLR_STATUS_REGISTER		0X00000006
#define WRITE_TO_SPI			0X9000
#define MODE_MASK_SPI			0X9000
#define READ_FROM_SPI			0X8000

//----- Acquisition State
#define STILL_BUSY			0x1	//---- still busy for the current acquisition
#define WAIT_TRIGGER		0x2	//---- wait for the trigger so acquiring pre trigger
#define TRIGGERED			0x4	//---- get the trigger so acquiring post trigger

#define SPI_WRITE_CMD_MASK		0x10000

//----- Register command: Base Board registers access
//----- read SPD module
#define CSPLXBASE_READ_SPD_CMD			0x00100000	//----cmd = 0x001, scope = 0x0, add = 0x0000

//----- Set Segment
#define CSPLXBASE_SET_SEG_LENGTH_CMD	0x00210009	//----cmd = 0x002, scope = 0x1, add = 0x0009
#define CSPLXBASE_SET_SEG_PRE_TRIG_CMD	0x0021000A	//----cmd = 0x002, scope = 0x1, add = 0x000A
#define CSPLXBASE_SET_SEG_POST_TRIG_CMD	0x0021000B	//----cmd = 0x002, scope = 0x1, add = 0x000B
#define CSPLXBASE_SET_SEG_NUMBER_CMD	0x0021000C	//----cmd = 0x002, scope = 0x1, add = 0x000C
#define CSPLXBASE_SET_CIRCULAR_REC_CMD	0x0021000E	//----cmd = 0x002, scope = 0x1, add = 0x000E		// Cobra Max only
//----- set Data Pack mode
#define CSNIOS_SET_PACK_MODE_CMD		0x00210018	//----cmd = 0x001, scope = 0x0, add = 0x0013

#define CSPLXBASE_SET_MULREC_AVG_COUNT	0x00210025	//----cmd = 0x002, scope = 0x1, add = 0x0025
#define CSPLXBASE_SET_MULREC_AVG_DELAY	0x00210026	//----cmd = 0x002, scope = 0x1, add = 0x0026

// These 2 commands are for Cobra Max cards only
#define CSPLXBASE_SET_MULREC_AVG_COUNT_FB	0x00210035	//----cmd = 0x002, scope = 0x1, add = 0x0035
#define CSPLXBASE_SET_MULREC_AVG_DELAY_FB	0x00210036	//----cmd = 0x002, scope = 0x1, add = 0x0036

// ---- Mem Write thru NIOS
#define CSPLXBASE_SET_MEM_DATAREAD_00	0x00210020	//----cmd = 0x002, scope = 0x1, add = 0x0020
#define CSPLXBASE_SET_MEM_DATAREAD_01	0x00210021	//----cmd = 0x002, scope = 0x1, add = 0x0021
#define CSPLXBASE_SET_MEM_DATAREAD_10	0x00210022	//----cmd = 0x002, scope = 0x1, add = 0x0022
#define CSPLXBASE_SET_MEM_DATAREAD_11	0x00210023	//----cmd = 0x002, scope = 0x1, add = 0x0023


#define CSPLXBASE_SET_SQUARE_TDELAY_CMD		0x0021004A	//----cmd = 0x002, scope = 0x1, add = 0x004B
#define CSPLXBASE_SET_SQUARE_THOLDOFF_CMD	0x0021004B	//----cmd = 0x002, scope = 0x1, add = 0x004A


// FFT Mulrec mode
#define CSPLXBASE_SET_FFT_MULREC		0x00210028	//----cmd = 0x002, scope = 0x1, add = 0x0028

//----- Read Trigger Address in Realtime mode
#define CSPLXBASE_READ_RTTRIGADDR_CMD	0x00200012	//----cmd = 0x001, scope = 0x0, add = 0x0012
//----- Read Trigger Address in Realtime mode
#define CSPLXBASE_READ_REC_COUNT_CMD	0x00200013	//----cmd = 0x001, scope = 0x0, add = 0x0013
//----- Read trigger address in AVG mode
#define CSPLXBASE_READ_AVG_TRIGADDR_CMD	0x00200018	//----cmd = 0x001, scope = 0x0, add = 0x0013
// ---- Mem READ thru NIOS
#define CSPLXBASE_GET_MEM_DATAREAD_00	0x00200020	//----cmd = 0x002, scope = 0x0, add = 0x0020
#define CSPLXBASE_GET_MEM_DATAREAD_01	0x00200021	//----cmd = 0x002, scope = 0x0, add = 0x0021
#define CSPLXBASE_GET_MEM_DATAREAD_10	0x00200022	//----cmd = 0x002, scope = 0x0, add = 0x0022
#define CSPLXBASE_GET_MEM_DATAREAD_11	0x00200023	//----cmd = 0x002, scope = 0x0, add = 0x0023

#define CSPLXBASE_SET_SQUARE_TDELAY_CMD		0x0021004A	//----cmd = 0x002, scope = 0x1, add = 0x004A		// Cobra Max only
#define CSPLXBASE_SET_SQUARE_THOLDOFF_CMD	0x0021004B	//----cmd = 0x002, scope = 0x1, add = 0x004B		// Cobra Max only
#define CSPLXBASE_SQR_READVALIDCOUNT		0x0021004D	//----cmd = 0x002, scope = 0x1, add = 0x004D		// Cobra Max only
#define CSPLXBASE_SQR_READSKIPCOUNT			0x0021004E	//----cmd = 0x002, scope = 0x1, add = 0x004E		// Cobra Max only
#define CSPLXBASE_ONE_SAMPLE_RESOLUITON_ADJ	0x002100E4	//----cmd = 0x002, scope = 0x1, add = 0x00E4		// Cobra & Cobra MAx PCI-E only
#define CSPLXBASE_ONE_SAMPLE_TRIGDELAY		0x002100EE	//----cmd = 0x002, scope = 0x1, add = 0x00EE		// EON Express only

#define CSPLXBASE_SQR_AQC_SKIP_READ			0x01610000	//----cmd = 0x016, scope = 0x1, add = 0x0000		// Cobra Max only
#define CSPLXBASE_SQR_AQC_SKIP_READ			0x01610000	//----cmd = 0x016, scope = 0x1, add = 0x0000		// Cobra Max only

//----- Start acquisition
#define CSPLXBASE_START_ACQ_CMD			0x00310000	//----cmd = 0x003, scope = 0x1, add = 0x0000
//----- Abort acquisition
#define CSPLXBASE_ABORT_ACQ_CMD			0x00510000	//----cmd = 0x005, scope = 0x1, add = 0x0000

//----- Reading tail command
#define CSPLXBASE_READ_TAIL  			0x01100000	//----cmd = 0x011, scope = 0x0, add = 0x0000
#define CSPLXBASE_READ_TAIL_1			0x01100001	//----cmd = 0x011, scope = 0x0, add = 0x0001

// ---- Start acquisition in Streaming mode
#define CSPLXBASE_START_RTACQ			0x01200000	//----cmd = 0x012, scope = 0x0, add = 0x0000

//----- Acq Reading (transfer)
#define CSPLXBASE_READ_ACQ_CMD			0x00610000	//----cmd = 0x006, scope = 0x1, add = 0x0000
//----- Abort reading (transfer)
#define CSPLXBASE_ABORT_READ_CMD		0x00710000	//----cmd = 0x007, scope = 0x1, add = 0x0000

//----- SPI Read /Write
#define CSPLXBASE_SPI_READ_CMD			0x00800000	//----cmd = 0x008, scope = 0x0, add = 0x0000

//----- read memory configuration
#define CSPLXBASE_READ_MCFG_CMD			0x00900000	//----cmd = 0x009, scope = 0x0, add = 0x0000

//----- set start address
#define CSPLXBASE_SET_START_ADD_CMD		0x00A00000	//----cmd = 0x00A, scope = 0x0, add = 0x0000

//----- set start segment number (PREFERRED)
#define CSPLXBASE_SET_SEG_NUM_CMD		0x00B00000	//----cmd = 0x00B, scope = 0x0, add = 0x0000

//----- start read at address (set and start Transfer)
#define CSPLXBASE_START_READ_ADD_CMD	0x00C10000	//----cmd = 0x00C, scope = 0x1, add = 0x0000

//----- start read at segment number (set and start Transfer)
#define CSPLXBASE_START_READ_SEG_CMD	0x00D10000	//----cmd = 0x00D, scope = 0x1, add = 0x0000

//----- read Trigger Address of Segment number (Segement number in the data register)
#define CSPLXBASE_READ_TRIG_ADD_CMD		0x00E00000	//----cmd = 0x00E, scope = 0x0, add = 0x0000

//----- read Segment info of Segment number (Segment number in the data register)
#define CSPLXBASE_READ_SEG_INFO_CMD		0x00F00000	//----cmd = 0x00F, scope = 0x0, add = 0x0000

//----- read at offset (PREFERRED)(start Transfer at Offset)
#define CSPLXBASE_READ_SEG_OFFSET_CMD	0x01000000	//----cmd = 0x010, scope = 0x0, add = 0x0000

//----- read at first acq
#define CSPLXBASE_READ_SEG_CMD	0x02000000	//----cmd = 0x020, scope = 0x0, add = 0x0000

//----- read Time Stamp Trigger Data 1
#define CSPLXBASE_READ_TIMESTAMP0_CMD	0x01100002	//----cmd = 0x011, scope = 0x0, add = 0x0002

//----- read Time Stamp Trigger Data 1
#define CSPLXBASE_READ_TIMESTAMP1_CMD	0x01100003	//----cmd = 0x011, scope = 0x0, add = 0x0003

//----- read Memory pass trough
#define CSPLXBASE_READ_MEM_THRU	        0x01300000	//----cmd = 0x013, scope = 0x0, add = 0x0000

//----- write Memory pass trough
#define CSPLXBASE_WRITE_MEM_THRU 	    0x01410000	//----cmd = 0x014, scope = 0x1, add = 0x0000

//----- Get Memory Size
#define CSPLXBASE_GET_MEMORYSIZE_CMD	0x02100000	//----cmd = 0x021, scope = 0x1, add = 0x0000

//----- Get Memory Size
#define CSPLXBASE_READ_FLASHMEMORY_CMD	0x02200000	//----cmd = 0x022, scope = 0x1, add = 0x0000

// Streaming mode
#define CSPLXBASE_SET_STREAM_RECORDCOUNT		0x00210050	//----cmd = 0x002, scope = 0x1, add = 0x0028

//----- PLL Frequency Set (SPIDER only)
#define CSPLXBASE_PLL_FREQUENCY_SET_CMD	0x02410000	//----cmd = 0x024, scope = 0x1, add = 0x0000

//----- Front end setup (SPIDER and SPLENDA only)
#define CSPLXBASE_SETFRONTEND_CMD	 	0x02510000	//----cmd = 0x025, scope = 0x1, add = 0x0000

//----- Read Allow Capabilities (SPIDER only)
#define CSPLXBASE_READ_HW_ALLOWS_CAPS	0x02600000	//----cmd = 0x026, scope = 0x0, add = 0x0000

//----- Read Allow High Impedance Ranges (SPIDER only)
#define CSPLXBASE_READ_RANGES_HIGH_IMPED	0x02700000	//----cmd = 0x027, scope = 0x0, add = 0x0000

//----- Read Low Impedance Ranges (SPIDER only)
#define CSPLXBASE_READ_RANGES_LOW_IMPED		0x02800000	//----cmd = 0x028, scope = 0x0, add = 0x0000

//----- Read Low Impedance Ranges (SPIDER v1.2 only)
#define CSPLXBASE_TRIM_FE_CAPS			0x02910000	//----cmd = 0x029, scope = 0x1, add = 0x0000

//----- Initialize Addon (Splenda only)
#define CSPLXBASE_ADDON_INIT			0x02A00000	//----cmd = 0x02A, scope = 0x0, add = 0x0000

//----- GEMS monitor Code--- JUST TO DEBUG
#define CSPLXBASE_JUMP_TO_GEMS	0x03000000

//----- Reset PLL (Brainiac only)
#define CSPLXBASE_PLL_RESET				0x3110000

//----- Validation of Clock input frequency
#define CSPLXBASE_CHECK_CLK_FREQUENCY		0x3500000

// ----- Start acquisition in Streaming mode
#define CSPLXBASE_START_STREAM_CMD		0x04000000


#define FLASH_DATA_REGISTER		0x0	//	base board point of view 0x0
#define FLASH_ADD_REG_1			0x4	//	base board point of view 0x1
#define FLASH_ADD_REG_2			0x8	//	base board point of view 0x2
#define FLASH_ADD_REG_3			0xc	//	base board point of view 0x3
#define COMMAND_REG_PLD			0x10//	base board point of view 0x4
//---- details of the command register
#define HOST_CTRL	0x1			//----- bit0
#define	DEV_OE		0x2			//----- bit1
#define DEV_CLR		0x4			//----- bit2
#define FWE_REG		0x8			//----- bit3
#define FOE_HOST	0x10		//----- bit4
#define FCE_HOST	0x20		//----- bit5
#define RESET_INT	0x40		//----- bit6
#define CONFIG		0x80		//----- bit7

#define SEND_HOST_CMD		HOST_CTRL|CONFIG					// 10000001
#define SEND_WRITE_CMD		HOST_CTRL|FCE_HOST|FWE_REG|CONFIG	// 10101001
#define SEND_READ_CMD		HOST_CTRL|FCE_HOST|FOE_HOST|CONFIG	// 10110001

#define STATUS_REG_PLX				0x10 //base board point of view 0x4
#define VERSION_REG_PLX				0x14 //base board point of view 0x5
//---- details of the status register
#define NSTATUS		0x1			//----- bit0
#define NCEO		0x2			//----- bit1
#define CONF_DONE	0x4			//----- bit2
#define READY		0x8			//----- bit3
#define INIT_DONE	0x10		//----- bit4
#define FRBY		0x20		//----- bit5
#define NO_VER		0x40		//----- bit6
#define ERROR_FLAG	0x80		//----- bit7









//----- CONSTANTS
//---- maximum number of sectors supported
#define MAXSECTORS  64
#define SECTOR_SIZE	0x10000

//---- Command codes for the flash_command routine
#define FLASH_SELECT			0       //---- no command; just perform the mapping
#define FLASH_RESET				1       //---- reset to read mode
#define FLASH_READ				1       //---- reset to read mode, by any other name
#define FLASH_AUTOSEL			2       //---- autoselect (fake Vid on pin 9)
#define FLASH_PROG				3       //---- program a word
#define FLASH_CHIP_ERASE		4       //---- chip erase
#define FLASH_SECTOR_ERASE		5       //---- sector erase
#define FLASH_ERASE_SUSPEND		6       //---- suspend sector erase
#define FLASH_ERASE_RESUME		7       //---- resume sector erase
#define FLASH_UNLOCK_BYP		8       //---- go into unlock bypass mode
#define FLASH_UNLOCK_BYP_PROG	9       //---- program a word using unlock bypass
#define FLASH_UNLOCK_BYP_RESET	10      //---- reset to read mode from unlock bypass mode
#define FLASH_LASTCMD			10      //---- used for parameter checking

//---- Return codes from flash_status
#define STATUS_READY			0       //---- ready for action
#define STATUS_FLASH_BUSY		1       //---- operation in progress
#define STATUS_ERASE_SUSP		2		//---- erase suspended
#define STATUS_FLASH_TIMEOUT	3       //---- operation timed out
#define STATUS_ERROR			4       //---- unclassified but unhappy status





// Index used in Command Read Allow Capabilities (SPIDER only)
#define	SPDRCAPS_CHANNELNUM				0
#define	SPDRCAPS_TERMINATION			1
#define	SPDRCAPS_HIGHIMPED				2
#define	SPDRCAPS_LOWIMPED				3
#define	SPDRCAPS_FLTR1_LOWCUTOFF		4
#define	SPDRCAPS_FLTR1_HIGHCUTOFF		5
#define	SPDRCAPS_FLTR2_LOWCUTOFF		6
#define	SPDRCAPS_FLTR2_HIGHCUTOFF		7
#define	SPDRCAPS_ANALOG_DATAMASK		8
#define	SPDRCAPS_MAX_CLOCK				9

// Index used in Command Read Allow Capabilities (SPLENDA only)
#define	SPLENDACAPS_RESOLUTION			0
#define	SPLENDACAPS_CHANNEL				1
#define	SPLENDACAPS_MEMORY				2



//    DECADE

#define DATA_WRITE_TO_NIOS_OFFSET	0x00000000
#define DATA_FROM_NIOS_OFFSET		0x00000000
#define MODESELECT_OFFSET			0x00000003
#define STATUS_FROM_NIOS_OFFSET		0x00000004
#define COMMAND_TO_NIOS_OFFSET		0x00000004
#define ACK_EXE_TO_NIOS_OFFSET		0x00000008
#define MISC_REG_OFFSET			    0x00000008
#define MODESEL_OFFSET				0x0000000C
#define	TEST_REG_OFFSET				0x00000010
#define GIO_DATA_OFFSET				0x00000014
#define GIO_CMD_OFFSET				0x00000018
#define GEN_CMD_OFFSET				0x0000001C
#define GEN_STATUS_OFFSET			0x0000001C
#define INT_STATUS_OFFSET			0x00000020
#define INT_COUNT_OFFSET			0x00000024

#define INT_CFG_OFFSET				0x0000002C
#define TRIG_TOUT_OFFSET			0x00000030
#define FW_OPTION_OFFSET			0x00000030
#define FW_VERSION_OFFSET			0x00000034
#define WDTIMER_OFFSET				0x00000034
#define MASK_OFFSET					0x00000040
#define USER_REGS_OFFSET			0x00000080
#define UREGS_INDEX_OFFSET			0x00000084
#define CPLD_REG_OFFSET				0x000000C4
#define FLASH_REG_OFFSET			0x000000C4

#define INT_FIFO_OFFSET				0x00000100
#define FIFO_OFFSET					0x0000004C
#define SRAM_OFFSET					0x00008000
#define DPRAM_OFFSET				0x00008000
#define BUNNY_RECONFIG_OFFSET		0x00000060			//
#define DPM_RECS_COUNT_OFFSET		0x00000064

#define	UIREGS_REG_OFFSET			0x00000084

#define	FLASH_OFFSET				0x000000C4

#define io_read_enable_bit	0x0200000
#define io_write_strobe_bit	0x0100000
#define io_address_mask		0x00F0000
#define io_ready_bit		0x0010000


#endif
