//! CsSplenda.h
//!
//!

#ifndef __CsSplendaDevice_h__
#define __CsSplendaDevice_h__

#include "CsDefines.h"
#include "CsTypes.h"
#include "CsPlxDefines.h"
#include "CsDeviceIDs.h"
#include "CsAdvStructs.h"
#include "CsiHeader.h"
#include "CsFirmwareDefs.h"
#include "CsSplendaCapsInfo.h"
#include "CsSplendaCal.h"
#include "CsSplendaFlash.h"

#include "CsPlxBase.h"
#include "CsNiosApi.h"

#include "CsDrvStructs.h"
#include "DrvCommonType.h"
#include "DllStruct.h"
#include "GageWrapper.h"
#include "CsSharedMemory.h"

#include "CsFirmwareDefs.h"
#include "CsSplendaState.h"
#include "CsNucleonBaseFlash.h"

#ifdef _WINDOWS_
#define USE_SHARED_CARDSTATE
#endif

#define SPLENDA_SHARENAME			_T("CsSplenda")

#define	SHARED_CAPS_NAME			_T("Cs16xyyWDFCaps")
#define MEMORY_MAPPED_SECTION		_T("Cs16xyyWDF-shared") // CR-2621

#define		PAGE_SIZE		4096

#define	MAX_SPDR_TRIGGER_ADDRESS_OFFSET		24		// Should be muliple of 8
#define	MAX_SPDR_TAIL_INVALID_SAMPLES		(7*8)	// Invalid sample at tail in Single mode
													// Half in Dual, 1/4 in Quad and 1/8 in Oct

#define CSPLNDA_AC_COUPLING_INDEX    0x01
#define CSPLNDA_FILTER_INDEX         0x02
#define CSPLNDA_IMPEDANCE_50_INDEX   0x04


#define CSPLNDA_DAC_SPAN			2048

// Trigger Engine Index in Table
#define	CSPLNDA_TRIGENGINE_EXT		0
#define	CSPLNDA_TRIGENGINE_1A		1
#define	CSPLNDA_TRIGENGINE_1B		2
#define	CSPLNDA_TRIGENGINE_2A		3
#define	CSPLNDA_TRIGENGINE_2B		4
#define	CSPLNDA_TRIGENGINE_3A		5
#define	CSPLNDA_TRIGENGINE_3B		6
#define	CSPLNDA_TRIGENGINE_4A		7
#define	CSPLNDA_TRIGENGINE_4B		8
#define	CSPLNDA_TOTAL_TRIGENGINES	9


#define	AV_ENSURE_POSTTRIG				96					// Max TrigAddrOffset in AVG mode

#define SPLNDA_PCI_MAX_MEM_HWAVG_DEPTH		(0xC000-64)		// Max trigger depth in HW Averaging mode (in samples)
#define SPLNDA_NUCLEON_MAX_MEM_HWAVG_DEPTH	(0x10000-64)	// Max trigger depth in HW Averaging mode (in samples)
#define SPLNDA_MAX_AVERAGING_PRETRIGDEPTH	0				// Max pre trigger depth in HW Averaging mode (in samples)
#define	SPLNDA_MAX_HW_AVERAGE_SEGMENTCOUNT	1024			// Max segment count in HW Averaging mode


// Limitation of sofware averaging
#define MAX_SW_AVERAGING_SEMGENTSIZE	512*1024		// Max segment size in Software averaging mode (in samples)
#define	SW_AVERAGING_BUFFER_PADDING		16				// Padding for Sw Avearge buffer
#define MAX_SW_AVERAGE_SEGMENTCOUNT		4096			// Max segment count in SW averaging mode

#define	CSPLNDA_MAX_FIR_PIPELINE				32				// Number of head samples lost due to FIR pipeline
														// Should be multiple of 8

#define	CSPLNDA_FIR_ACTUALSTART_ADJUST20	31				// Number of head samples lost due to FIR pipeline
														// Should be multiple of 8

#define	CSPLNDA_FIR_ACTUALSTART_ADJUST39	65				// Number of head samples lost due to FIR pipeline
														// Should be multiple of 8

#define	CSPLNDA_FIR_READINGOFFSET_ADJUST20	19			// Number of head samples lost due to FIR pipeline
														// Should be multiple of 8

#define	CSPLNDA_FIR_READINGOFFSET_ADJUST39	32				// Number of head samples lost due to FIR pipeline
														// Should be multiple of 8

#define CSPLNDA_DEFAULT_VALUE				0
#define CSPLNDA_DEFAULT_RELATION			CS_RELATION_OR

#define	CSPLNDA_FAST_MEMORY_CLOCK			166000000		// 166MHz clock
#define	CSPLNDA_MED_MEMORY_CLOCK			150000000		// 150MHz clock
#define	CSPLNDA_SLOW_MEMORY_CLOCK			133000000		// 133MHz clock
#define	CSPLNDA_PCIEX_MEMORY_CLOCK			200000000		// 250MHz clock

#define	CSPLNDA_CAL_RESTORE_COARSE			0x0001
#define	CSPLNDA_CAL_RESTORE_FINE			0x0002
#define	CSPLNDA_CAL_RESTORE_GAIN			0x0004
#define	CSPLNDA_CAL_RESTORE_POS  			0x0008
#define CSPLNDA_CAL_RESTORE_OFFSET          (CSPLNDA_CAL_RESTORE_COARSE|CSPLNDA_CAL_RESTORE_FINE)
#define	CSPLNDA_CAL_RESTORE_ALL  			(CSPLNDA_CAL_RESTORE_OFFSET|CSPLNDA_CAL_RESTORE_GAIN|CSPLNDA_CAL_RESTORE_POS)

//!----- Default settings
#define	CSPLNDA_DEFAULT_MSTRIGMODE			3
#define CSPLNDA_DEFAULT_TRIG_SENSITIVITY	0xC0
#define CSPLNDA_DEFAULT_SAMPLE_RATE			-1				// Force to the default sampling rate depending on card
#define CSPLNDA_DEFAULT_EXT_CLOCK_EN		0
#define CSPLNDA_DEFAULT_MODE				-1				// Default mode will be set depending on number of channels
#define CSPLNDA_DEFAULT_SAMPLE_BITS			CS_SAMPLE_BITS_16
#define CSPLNDA_DEFAULT_RES					-CS_SAMPLE_RES_16
#define CSPLNDA_DEFAULT_SAMPLE_SIZE			CS_SAMPLE_SIZE_2
#define CSPLNDA_DEFAULT_SAMPLE_OFFSET		CS_SAMPLE_OFFSET_16
#define CSPLNDA_DEFAULT_SEGMENT_COUNT		1
#define CSPLNDA_DEFAULT_DEPTH				8192
#define CSPLNDA_DEFAULT_SEGMENT_SIZE		CSPLNDA_DEFAULT_DEPTH
#define CSPLNDA_DEFAULT_TRIG_TIME_OUT		-1
#define CSPLNDA_DEFAULT_TRIG_ENGINES_EN		CS_TRIG_ENGINES_DIS
#define CSPLNDA_DEFAULT_TRIG_HOLD_OFF		0
#define CSPLNDA_DEFAULT_TRIG_DELAY			0

#define CSPLNDA_DEFAULT_TERM				CS_COUPLING_DC
#define CSPLNDA_DEFAULT_GAIN				CS_GAIN_2_V
#define CSPLNDA_DEFAULT_IMPEDANCE			CS_REAL_IMP_50_OHM
#define CSPLNDA_DEFAULT_FILTER				0
#define CSPLNDA_DEFAULT_POSITION			0

#define CSPLNDA_DEFAULT_CONDITION			CS_TRIG_COND_POS_SLOPE
#define CSPLNDA_DEFAULT_SOURCE				CS_CHAN_1
#define CSPLNDA_DEFAULT_EXT_COUPLING		CS_COUPLING_DC
#define CSPLNDA_DEFAULT_EXT_GAIN			CS_GAIN_2_V
#define CSPLNDA_DEFAULT_EXT_IMPEDANCE		CS_REAL_IMP_1M_OHM

// Spider v1.2 only
#define CSPLNDA_DEFAULT_ADCOFFSET_ADJ	17
#define CSPLNDA_DEFAULT_CLOCK_OUT 		eclkoNone

#define CSPLNDA_VARCAP_DATA             0x1
#define CSPLNDA_VARCAP_BASE             1
#define CSPLNDA_VARCAP_CHAH_STEP        2
#define CSPLNDA_VARCAP_DIV50            0
#define CSPLNDA_VARCAP_DIV10             1
#define CSPLNDA_VARCAP_STEPS           31
#define CSPLENDA_1MOHM_MUX_MASK	        0x00000180  //bits 7 and 8
#define CSPLENDA_1MOHM_DIV1             0x00000100
#define CSPLENDA_1MOHM_DIV5             0x00000080
#define CSPLENDA_1MOHM_DIV50            0x00000000

#define CSPLNDA_SPI				0x8000

//----- Trigger Setting
//----- 3 Slope
//----- 2 Control Enable
//----- 1 source 1
//----- 0 source 0
#define CSPLNDA_SET_GBLTRIG_ENABLED			0x0001
#define CSPLNDA_SET_EXT_TRIG_DC				0x0001
#define CSPLNDA_SET_EXT_TRIG_EXT1V			0x0002
#define CSPLNDA_SET_EXT_TRIG_POS_SLOPE		0x0004
#define CSPLNDA_SET_EXT_TRIG_ENABLED		0x0008

#define CSPLNDA_SET_DIG_TRIG_EXT_EN_OFF		0x0
#define CSPLNDA_SET_DIG_TRIG_EXT_EN_LIVE	0x2
#define CSPLNDA_SET_DIG_TRIG_EXT_EN_POS		0x4
#define CSPLNDA_SET_DIG_TRIG_EXT_EN_NEG		0x6
#define CSPLNDA_SET_DIG_TRIG_TRIGOUTENABLED	0x10
#define CSPLNDA_SET_TRIGOUT_CLKPHASE		0x20

//----- Register command: Add-On registers access
//----- Add-On SPI access : Nios routine: WriteToCpldFpga
#define CSPLNDA_CHAN1_SET_WR_CMD			0x00811102	// cmd = 0x008, scope = 0x1, add = 0x11  Model/Cs = 02
#define CSPLNDA_CHAN2_SET_WR_CMD			0x00811202	// cmd = 0x008, scope = 0x1, add = 0x12  Model/Cs = 02
#define CSPLNDA_CHAN3_SET_WR_CMD			0x00811302	// cmd = 0x008, scope = 0x1, add = 0x13  Model/Cs = 02
#define CSPLNDA_CHAN4_SET_WR_CMD			0x00811402	// cmd = 0x008, scope = 0x1, add = 0x14  Model/Cs = 02
#define CSPLNDA_VARCAP_SET_WR_CMD			0x00811502	// cmd = 0x008, scope = 0x1, add = 0x15  Model/Cs = 02
#define CSPLNDA_VSC_SET_WR_CMD				0x00811602	// cmd = 0x008, scope = 0x1, add = 0x16  Model/Cs = 02

#define CSPLNDA_PROTECTION_STATUS			0x00801D02	// cmd = 0x008, scope = 0x1, add = 0x1D  Model/Cs = 02

#define CSPLNDA_ACD1_RD_CMD					0x00801E02	// cmd = 0x008, scope = 0x0, add = 0x1E  Model/Cs = 02
#define CSPLNDA_ACV1_RD_CMD					0x00801F02	// cmd = 0x008, scope = 0x0, add = 0x1F  Model/Cs = 02

#define CSPLNDA_ACD2_RD_CMD					0x00801E03	// cmd = 0x008, scope = 0x0, add = 0x1E  Model/Cs = 03
#define CSPLNDA_ACV2_RD_CMD					0x00801F03	// cmd = 0x008, scope = 0x0, add = 0x1F  Model/Cs = 03

#define CSPLNDA_CLK_SET_RD_CMD				0x00802101	// cmd = 0x008, scope = 0x0, add = 0x21  Model/Cs = 01
#define CSPLNDA_CLK_SET_WR_CMD				0x00812101	// cmd = 0x008, scope = 0x1, add = 0x21  Model/Cs = 01
#define CSPLNDA_EXT_TRIG_SET_WR_CMD			0x00812501	// cmd = 0x008, scope = 0x1, add = 0x25  Model/Cs = 01
#define CSPLNDA_PULSE_SET_WR_CMD			0x00812601	// cmd = 0x008, scope = 0x1, add = 0x26  Model/Cs = 01
#define CSPLNDA_OPTION_DIR_SET_WR_CMD		0x00812701	// cmd = 0x008, scope = 0x1, add = 0x27  Model/Cs = 01
#define CSPLNDA_SYNTH_PLL_RESET_WR_CMD		0x00812901  // cmd = 0x008, scope = 0x1, add = 0x29  Model/Cs = 04

/*RG Check with Vlad */
#define CSPLNDA_CPLD_FD_RD_CMD				0x00803E04	// cmd = 0x008, scope = 0x0, add = 0x3E  Model/Cs = 04
#define CSPLNDA_CPLD_FV_RD_CMD				0x00803F04	// cmd = 0x008, scope = 0x0, add = 0x3F	 Model/Cs = 04

#define CSPLNDA_TEST_REG_WR_CMD				0x00814001	// cmd = 0x008, scope = 0x1, add = 0x40  Model/Cs = 01
#define CSPLNDA_SELFTEST_MODE_WR_CMD		0x00814101	// cmd = 0x008, scope = 0x1, add = 0x41  Model/Cs = 01
#define CSPLNDA_OPTION_SET_WR_CMD			0x00814201	// cmd = 0x008, scope = 0x1, add = 0x42  Model/Cs = 01
#define CSPLNDA_CAPTUREMODE_SET_WR_CMD		0x00814301	// cmd = 0x008, scope = 0x1, add = 0x43  Model/Cs = 01
#define CSPLNDA_FPGA_CLK_SET_WR_CMD			0x00814401	// cmd = 0x008, scope = 0x1, add = 0x44  Model/Cs = 01
#define CSPLNDA_DEC_CTL_WR_CMD				0x00814501	// cmd = 0x008, scope = 0x1, add = 0x45  Model/Cs = 01
#define CSPLNDA_DEC_FACTOR_LOW_WR_CMD		0x00814601	// cmd = 0x008, scope = 0x1, add = 0x46  Model/Cs = 01
#define CSPLNDA_DEC_FACTOR_HIGH_WR_CMD		0x00814701	// cmd = 0x008, scope = 0x1, add = 0x46  Model/Cs = 01
#define CSPLNDA_MS_REG_RD					0x00804801	// cmd = 0x008, scope = 0x0, add = 0x48  Model/Cs = 01
#define CSPLNDA_MS_REG_WR					0x00814801	// cmd = 0x008, scope = 0x1, add = 0x48  Model/Cs = 01
#define CSPLNDA_MS_BIST_ST1_RD				0x00804901	// cmd = 0x008, scope = 0x0, add = 0x48  Model/Cs = 01
#define CSPLNDA_MS_BIST_CTL1_WR				0x00814901	// cmd = 0x008, scope = 0x1, add = 0x48  Model/Cs = 01
#define CSPLNDA_MS_BIST_ST2_RD				0x00804A01	// cmd = 0x008, scope = 0x0, add = 0x48  Model/Cs = 01
#define CSPLNDA_MS_BIST_CTL2_WR				0x00814A01	// cmd = 0x008, scope = 0x1, add = 0x48  Model/Cs = 01


#define CSPLNDA_TRIGOUT_WIDTH_WR_CMD		0x00815101	// cmd = 0x008, scope = 0x1, add = 0x51  Model/Cs = 01
#define CSPLNDA_TRIGDELAY_SET_WR_CMD		0x00815201	// cmd = 0x008, scope = 0x1, add = 0x52  Model/Cs = 01
#define CSPLNDA_TRIG_SET_WR_CMD				0x00815401	// cmd = 0x008, scope = 0x1, add = 0x54  Model/Cs = 01
#define CSPLNDA_DTS_GBL_SET_WR_CMD			0x00815501	// cmd = 0x008, scope = 0x1, add = 0x55  Model/Cs = 01
#define CSPLNDA_DTS1_SET_WR_CMD				0x00815601	// cmd = 0x008, scope = 0x1, add = 0x56  Model/Cs = 01
#define CSPLNDA_DTS2_SET_WR_CMD				0x00815701	// cmd = 0x008, scope = 0x1, add = 0x57  Model/Cs = 01
#define CSPLNDA_DTS3_SET_WR_CMD				0x00815801	// cmd = 0x008, scope = 0x1, add = 0x58  Model/Cs = 01
#define CSPLNDA_DTS4_SET_WR_CMD				0x00815901	// cmd = 0x008, scope = 0x1, add = 0x59  Model/Cs = 01
#define CSPLNDA_DTS5_SET_WR_CMD				0x00815A01	// cmd = 0x008, scope = 0x1, add = 0x5A  Model/Cs = 01
#define CSPLNDA_DTS6_SET_WR_CMD				0x00815B01	// cmd = 0x008, scope = 0x1, add = 0x5B  Model/Cs = 01
#define CSPLNDA_DTS7_SET_WR_CMD				0x00815C01	// cmd = 0x008, scope = 0x1, add = 0x5C  Model/Cs = 01
#define CSPLNDA_DTS8_SET_WR_CMD				0x00815D01	// cmd = 0x008, scope = 0x1, add = 0x5D  Model/Cs = 01

#define CSPLNDA_FWD_RD_CMD					0x00805E01	// cmd = 0x008, scope = 0x0, add = 0x5E  Model/Cs = 01
#define CSPLNDA_FWV_RD_CMD					0x00805F01	// cmd = 0x008, scope = 0x0, add = 0x5F  Model/Cs = 01

#define CSPLNDA_DL_1A_SET_WR_CMD			0x00816001	// cmd = 0x008, scope = 0x1, add = 0x60  Model/Cs = 01
#define CSPLNDA_DS_1A_SET_WR_CMD			0x00816101	// cmd = 0x008, scope = 0x1, add = 0x61  Model/Cs = 01

#define CSPLNDA_DL_1B_SET_WR_CMD			0x00816201	// cmd = 0x008, scope = 0x1, add = 0x62  Model/Cs = 01
#define CSPLNDA_DS_1B_SET_WR_CMD			0x00816301	// cmd = 0x008, scope = 0x1, add = 0x63  Model/Cs = 01

#define CSPLNDA_DL_2A_SET_WR_CMD			0x00816401	// cmd = 0x008, scope = 0x1, add = 0x64  Model/Cs = 01
#define CSPLNDA_DS_2A_SET_WR_CMD			0x00816501	// cmd = 0x008, scope = 0x1, add = 0x65  Model/Cs = 01

#define CSPLNDA_DL_2B_SET_WR_CMD			0x00816601	// cmd = 0x008, scope = 0x1, add = 0x66  Model/Cs = 01
#define CSPLNDA_DS_2B_SET_WR_CMD			0x00816701	// cmd = 0x008, scope = 0x1, add = 0x67  Model/Cs = 01

#define CSPLNDA_DL_3A_SET_WR_CMD			0x00816801	// cmd = 0x008, scope = 0x1, add = 0x68  Model/Cs = 01
#define CSPLNDA_DS_3A_SET_WR_CMD			0x00816921	// cmd = 0x008, scope = 0x1, add = 0x69  Model/Cs = 01

#define CSPLNDA_DL_3B_SET_WR_CMD			0x00816A01	// cmd = 0x008, scope = 0x1, add = 0x6A  Model/Cs = 01
#define CSPLNDA_DS_3B_SET_WR_CMD			0x00816B01	// cmd = 0x008, scope = 0x1, add = 0x6B  Model/Cs = 01

#define CSPLNDA_DL_4A_SET_WR_CMD			0x00816C01	// cmd = 0x008, scope = 0x1, add = 0x6C  Model/Cs = 01
#define CSPLNDA_DS_4A_SET_WR_CMD			0x00816D01	// cmd = 0x008, scope = 0x1, add = 0x6D  Model/Cs = 01

#define CSPLNDA_DL_4B_SET_WR_CMD			0x00816E01	// cmd = 0x008, scope = 0x1, add = 0x6E  Model/Cs = 01
#define CSPLNDA_DS_4B_SET_WR_CMD			0x00816F01	// cmd = 0x008, scope = 0x1, add = 0x6F  Model/Cs = 01

// Channels ADC Data Offset Correction
#define CSPLNDA_ADC_DOC1					0x00812A01  // cmd = 0x008, scope = 0x1, add = 0x2A  Model/Cs = 01
#define CSPLNDA_ADC_DOC2					0x00812B01  // cmd = 0x008, scope = 0x1, add = 0x2B  Model/Cs = 01
#define CSPLNDA_ADC_DOC3					0x00812C01  // cmd = 0x008, scope = 0x1, add = 0x2C  Model/Cs = 01
#define CSPLNDA_ADC_DOC4					0x00812D01  // cmd = 0x008, scope = 0x1, add = 0x2D  Model/Cs = 01



// AD7788 commands
#define SPLNDA_REF_ADC_CMD_RD		        0x40011
#define SPLNDA_REF_ADC_CMD_RESET	        0x1F

//----- SPLENDA Trigger Setting
//----- 3 Slope
//----- 2 Control Enable
//----- 1 source 1
//----- 0 source 0
#define CSPLNDA_DIG_TRIG_FROM_SAME			1			// Trigger From Same channel
#define CSPLNDA_DIG_TRIG_FROM_OTHER			2			// Trigger from

#define CSPLNDA_SET_DIG_TRIG_A_ENABLE		0x0001
#define CSPLNDA_SET_DIG_TRIG_B_ENABLE		0x0002
#define CSPLNDA_SET_DIG_TRIG_A_NEG_SLOPE	0x0004
#define CSPLNDA_SET_DIG_TRIG_B_NEG_SLOPE	0x0008
#define CSPLNDA_SET_DIG_TRIG_A_ADJ_INPUT	0x0010
#define CSPLNDA_SET_DIG_TRIG_B_ADJ_INPUT	0x0020


// Relate to Decimation Control
#define CSPLNDA_QUAD_CHAN			0x1
#define CSPLNDA_DUAL_CHAN			0x2
#define CSPLNDA_SINGLE_CHAN			0x3

//Clock control
#define CSPLNDA_SET_EXT_CLK         0x0002
#define CSPLNDA_SET_TCXO_REF_CLK	0x0004
#define CSPLNDA_SET_REFCLK_OUT		0x0010
#define CSPLNDA_CLK_OUT_MUX_MASK	0x0110
#define CSPLNDA_CLK_SRC_SEL         0x0200
#define CSPLNDA_SET_CLK_FILTER      0x0400
#define CSPLNDA_PLL_NOT_LOCKED      0x4000

#define CSPLNDA_CLK_FREQ_MASK       0x0ff
#define CSPLNDA_EXT_CLK_BYPASS      0x100
#define CSPLNDA_MASTER_CLK          0x200

#define CSPLNDA_CLK_OUT_ENABLE		0x100

//CSPLNDA_MS_REG_WR
#define CSPLNDA_MS_DECSYNCH_RESET	0x0001
#define CSPLNDA_MS_RESET			0x4000

//CSPLNDA_MS_BIST_CTL1_WR
#define CSPLNDA_BIST_EN						0x0001
#define CSPLNDA_BIST_DIR1					0x0002
#define CSPLNDA_BIST_DIR2					0x0004
//CSPLNDA_MS_BIST_CTL2_WR
//CSPLNDA_MS_BIST_ST1_RD
#define CSPLNDA_BIST_MS_SLAVE_ACK_MASK		0x007F
#define CSPLNDA_BIST_MS_MULTIDROP_MASK		0x7F80
#define CSPLNDA_BIST_MS_MULTIDROP_FIRST     0x0080
#define CSPLNDA_BIST_MS_MULTIDROP_LAST      0x4000
//CSPLNDA_MS_BIST_ST2_RD
#define CSPLNDA_BIST_MS_FPGA_COUNT_MASK		0x003F
#define CSPLNDA_BIST_MS_ADC_COUNT_MASK		0x3FC0
#define CSPLNDA_BIST_MS_SYNC        		0x4000



// DAC 7552
#define FENDA_OFFSET1_CS	0x20
#define FENDA_OFFSET2_CS	0x21
#define FENDA_OFFSET3_CS	0x22
#define FENDA_OFFSET4_CS	0x23
#define FENDA_GAIN12_CS		0x24
#define FENDA_GAIN34_CS		0x25
#define CAL_DACEXTRIG_CS	0x2e
#define DAC7552_OUTA		0x0400
#define DAC7552_OUTB		0x0800
#define CSPLNDA_WRITE_TO_DAC7552_CMD	0x00810000	//----cmd = 0x008, scope = 0x1, add = 0x00XX

#define CSPLNDA_WRITE_SPI_FLASH_CMD		0x008100e0  //----cmd = 0x008, scope = 0x1, add = 0x00E0
////
//----- Bits to use with CSPLNDA_WRITE_SPI_FLASH_CMD (with AT25250A_070713)
#define CSPLNDA_ADDON_WRITE_STATUS			0x01		//WRITE STATUS
#define CSPLNDA_ADDON_WRITE					0x02		//WRITE DATA
#define CSPLNDA_ADDON_READ					0x03		//READ DATA
#define CSPLNDA_ADDON_RESET_WRITE_ENABLE	0x04		//RESET WRITE ENABLE LATCH
#define CSPLNDA_ADDON_READ_STATUS			0x05		//READ STATUS
#define CSPLNDA_ADDON_WRITE_ENABLE			0x06		//SET WRITE ENABLE LATCH


#define CSPLNDA_ADDON_ACCESS_DISABLE		0x00
#define CSPLNDA_ADDON_ACCESS_THRU_CPLD		0x03
#define CSPLNDA_ADDON_ACCESS_THRU_FPGA		0x07

#define CSPLNDA_ADDON_LED_DEFAULT_CFG		0x09
////
//---- details of the command register
#define CSPLNDA_ADDON_RESET		0x1			//----- bit0
#define	CSPLNDA_ADDON_FCE		0x2			//----- bit1
#define CSPLNDA_ADDON_FOE		0x4			//----- bit2
#define CSPLNDA_ADDON_FWE		0x8			//----- bit3
#define CSPLNDA_ADDON_CONFIG	0x10		//----- bit4

#define CSPLNDA_ADDON_FLASH_CMD			0x10
#define CSPLNDA_ADDON_FLASH_ADD			0x11
#define CSPLNDA_ADDON_FLASH_DATA		0x12
#define CSPLNDA_ADDON_FLASH_ADDR_PTR	0x13
#define CSPLNDA_ADDON_STATUS			0x14
#define CSPLNDA_ADDON_CPLD_FMW_REV		0x15
#define CSPLNDA_ADDON_CPLD_OPT          0x16
#define CSPLNDA_ADDON_CPLD_TEST_REG		0x17
#define CSPLNDA_AD7321_CFG_REG  		0x1A

#define CSPLNDA_ADDON_FPGA_FMW_REV      0x1C
#define CSPLNDA_ADDON_FPGA_OPT   		0x1D
#define CSPLNDA_AD7321_WRITE_REG		0x1E
#define CSPLNDA_AD7321_READ_REG			0x1F
#define CSPLNDA_ADDON_FPGA_TEST_REG		0x20
#define CSPLNDA_ADDON_SPI_TEST_WR		0x21
#define CSPLNDA_ADDON_SPI_TEST_RD		0x22

#define CSPLNDA_ADDON_LED_CFG_REG  		0x80


//ADC 7321
#define CSPLNDA_AD7321_RESTART       0x1
#define CSPLNDA_AD7321_READ          0x2
#define CSPLNDA_AD7321_CLK_DIS       0x4
#define CSPLNDA_AD7321_DATA_READ     0x00010000
#define CSPLNDA_AD7321_RD_FIFO_FULL  0x00020000
#define CSPLNDA_AD7321_WR_FIFO_FULL  0x00040000
#define CSPLNDA_AD7321_STATE         0x00380000
#define CSPLNDA_AD7321_STATE_SHIFT   19
#define CSPLNDA_AD7321_COUNT         0x1fc00000
#define CSPLNDA_AD7321_COUNT_SHIFT   22

#define VSC_CHAN_SRC				 0x0080     // Bit in VSC register to select channel load as input to AD7321

#define ADC7321_CTRL				 0x8010     //SE mode, no power managment, 2's compliment, int ref, no sequence
#define ADC7321_CHAN				 0x0400     //input selection bit, should be the defult position
#define ADC7321_RANGE				 0xA000
#define ADC7321_CH0_10_V			 0x0000
#define ADC7321_CH0_5_V				 0x0800
#define ADC7321_CH0_2p5_V			 0x1000
#define ADC7321_CH1_10_V			 0x0000
#define ADC7321_CH1_5_V				 0x0080
#define ADC7321_CH1_2p5_V			 0x0100
#define CSPLNDA_AD7321_CODE_SPAN     4095


#define CSPLNDA_PUSLE_WIDTH          0x0020      //0- short; 1-long
#define CSPLNDA_PUSLE                0x0010      //toggle 1-0
#define CSPLNDA_PUSLE_EN             0x0008      //stan alone
#define CSPLNDA_PUSLE_DIS_LAST       0x0004      //pulse enable to the last 4 boards
#define CSPLNDA_PUSLE_DIS_FIRST      0x0002      //pulse enable to the first 4 boards
#define CSPLNDA_PUSLE_TO_CAL         0x0001      //connect pulse to cal input

#define	CSPLNDA_ADDON_STAT_FLASH_READY	0x1
#define	CSPLNDA_ADDON_STAT_CONFIG_DONE	0x2
#define CSPLNDA_TCPLD_INFO				0x4

#define CSPLNDA_ADDON_FLASH_WRITE_BUSY		0x1
#define CSPLNDA_ADDON_FLASH_WR_ENABLED		0x2
#define CSPLNDA_ADDON_FLASH_WR_STATUS_MASK	0x3 // write is allowed when bits 0 and 1 are equal to 2
												// (bit 0 = 0 and bit 1 = 2)

#define CSPLNDA_DIG_TRIGLEVEL_SPAN		0x2000		// 14 bits


//----Calib source
#define CSPLNDA_CALIB_SRC_DC		    0x0
#define CSPLNDA_CALIB_SRC_HI_FREQ		0x2
#define CSPLNDA_CALIB_SRC_LO_FREQ		0x4
#define CSPLNDA_CALIB_SRC_UPDATE		0x8

#define CSPLNDA_DDS_CLK_FREQ         10000000

// Spider v.12 only
#define CSPLNDA_PULSE_SET_ENABLED		0x01

//----- Option settings
//----- 1 0: PulseOut 1:Digital in
//----- 0 Option board
#define CSPLNDA_PULSE_GD_VAL		0x0000
#define CSPLNDA_PULSE_GD_INV		0x0001
#define CSPLNDA_PULSE_GD_RISE		0x0002
#define CSPLNDA_PULSE_GD_FALL		0x0003
#define CSPLNDA_PULSE_SYNC  		0x0004

//----- Self Test _mode
#define CSPLNDA_SELF_CAPTURE	0x0000
#define CSPLNDA_SELF_TEST		0x0001
#define CSPLNDA_SELF_BIST		0x0002
#define CSPLNDA_SELF_COUNTER	0x0004
#define CSPLNDA_SELF_MISC		0x0006

#define CSPLNDA_CALIB_DEPTH		 4096
#define CSPLNDA_CALIB_BUF_MARGIN	 512
#define CSPLNDA_CALIB_BUFFER_SIZE	 (2*CSPLNDA_CALIB_BUF_MARGIN + CSPLNDA_CALIB_DEPTH)
#define CSPLNDA_CALIB_BLOCK_SIZE   64

// Default adjustment for trigger address offset when FIR filter is enabled
#define CSPLNDA_DEFAULT_FIR20_ADJOFFSET		0
#define CSPLNDA_DEFAULT_FIR39_ADJOFFSET		0

// Default adjustment for trigger address offset when Hw average mode is enabled
#define CSPLNDA_DEFAULT_HWAVERAGE_ADJOFFSET	4

#define CSPLNDA_12BIT_DATA_MASK   0xC000C
#define CSPLNDA_14BIT_DATA_MASK   0x00000

#define CSPLNDA_PCIE_OPT_RD_CMD			0x30
#define CSPLNDA_PCIE_SWR_RD_CMD			0x34
#define CSPLNDA_PCIE_HWR_RD_CMD			0x38


#define CSPLNDA_CHANPROTECT_INVALID   0x8000

typedef struct _FRONTEND_SPLENDA_POS_COEF
{
	uInt16 u16Mult;
	uInt16 u16Denom;
}FRONTEND_SPLENDA_POS_COEF, *PFRONTEND_SPLENDA_POS_COEF;

typedef struct _SPLENDA_CAL_LUT
{                           
	uInt32	u32FeSwing_mV;
	int32   i32PosLevel_uV;
	int32   i32NegLevel_uV;
	uInt16  u16PosRegMask;
	uInt16  u16NegRegMask;
} SPLENDA_CAL_LUT, *PSPLENDA_CAL_LUT;

typedef enum _eCalInput
{
	eciDirectRef = 0,
	eciCalSrc = eciDirectRef + 1,
	eciCalChan = eciCalSrc + 1,
} eCalInput;


typedef enum CsDataFormat
{
	 formatDefault = 0,
	 formatHwAverage,
	 formatSoftwareAverage,
	 formatFIR
} _CSDATAFORMAT;


class CsSplendaDev : public CsPlxBase,  public CsNiosApi
{
private:
	PGLOBAL_PERPROCESS		m_ProcessGlblVars;
	SPLENDA_CARDSTATE		m_CardState;
	PSPLENDA_CARDSTATE		m_pCardState;
	void UpdateCardState(BOOLEAN bReset = FALSE);

	uInt32					m_u32SystemInitFlag;	// Compuscope System initialization flag
	//! Constructors
public:
	CsSplendaDev( uInt8 u8Index, PCSDEVICE_INFO pDevInfo, PGLOBAL_PERPROCESS ProcessGlblVars);
	CsSplendaDev*		m_MasterIndependent;	// Master /Independant card
	CsSplendaDev*		m_Next;					// Next Slave
	CsSplendaDev*		m_HWONext;				// HW OBJECT next
	CsSplendaDev*		m_pNextInTheChain;		// use for M/S detection
	DRVHANDLE			m_PseudoHandle;
	DRVHANDLE			m_CardHandle;			// Handle From Driver
	uInt8				m_u8CardIndex;			// Card index in native card detection order
	bool				m_bNucleon;				// Nucleon Base board
	bool				m_bOscar;				// Oscar Compuscope
	uInt16				m_DeviceId;

	uInt16				m_u16NextSlave;			// Next Slave card index

	PDRVSYSTEM			m_pSystem;				// Pointer to DRVSYSTEM
	CsSplendaDev*		m_pTriggerMaster;		// Point to card that has trigger engines enabled
	BOOLEAN				m_BusMasterSupport;		// HW support BusMaster transfer
	STREAMSTRUCT		m_Stream;				// Stream state


protected:
	uInt8				m_DelayLoadImageId;			// Image Id to be load later
	uInt8				m_CalibTableId;				// Current active calibration table;


	int64				m_i64PreDepthLimit;
	bool				m_bOptionBoard;

	uInt32				m_u32Ad7450CtlReg;

	BOOLEAN				m_bMemRollOver;			// Data rolls over in memory
	uInt32				m_MemEtb;				// Memory address trigger bit
	uInt32				m_Stb;					// Sub sample trigger bits


	int16	m_TrigAddrOffsetAdjust;			// Trigger address offset adjustment



	uInt32              m_u32CalFreq;
	eCalMode            m_CalMode;
	int16*              m_pi16CalibBuffer;
	int32*              m_pi32CalibA2DBuffer;
	bool				m_bSkipCalVoltageAdjust;
	bool				m_bForceCal[CSPLNDA_CHANNELS];

	bool				m_bInvalidateCalib;
	bool				m_bLogFaluireOnly;
	bool				m_bDebugCalSettlingTime;
	bool				m_bExtraDelayNeeded;
	uInt32				m_u32DebugCalibSrc;
	uInt32				m_u32DebugCalibTrace;
	uInt8               m_u8CalDacSettleDelay;
	uInt32				m_u32CalSrcRelayDelay;
	uInt32				m_u32CalSrcSettleDelay;
	uInt32				m_u32CalSrcSettleDelay_1MOhm;

	uInt16				m_u16CalCodeA;
	uInt16				m_u16CalCodeB;
	int16				m_i16CalCodeZ;

	uInt16              m_u16VarRefCode;
	uInt32              m_u32VarRefLevel_uV;
	uInt32              m_u32CalibRange_uV;
	uInt16				m_u16TrigOutWidth;

	uInt32					m_u32TailExtraData;		// Amount of extra samples to be added in configuration
													// in order to ensure post trigger depth
	uInt32                  m_u32MaxHwAvgDepth;

	uInt8					m_u8TrigSensitivity;			// Default Digital Trigger sensitivity;
	BOOLEAN					m_bUseDefaultTrigAddrOffset;	// Skip TrigAddr Calib table
	bool					m_bSkipMasterSlaveDetection;	// Skip Master/Slave detection
	BOOL					m_bZeroTrigAddrOffset;			// No Adjustment for Trigger Address Offset
	BOOL					m_bZeroDepthAdjust;				// No adjustment for Postriggger (ensure posttrigger depth)
	bool                    m_bZeroTriggerAddress;          // Hw Trigger address is always zero
	uInt8					m_AddonImageId;					// Current active Addon FPGA image;
	uInt8					m_u8BadAddonFirmware;			// The card has wrong or outdate Add-on FPGA firmware.
	uInt32					m_32BbFpgaInfoReqrd;			// Base board FPGA version required.
	uInt32					m_32AddonFpgaInfoReqrd;			// Add-on board FPGA version required.
	BOOL					m_bCsiFileValid;				// Handle of the Csi file
	BOOL					m_bFwlFileValid;				// Handle of the Fwl file
	bool					m_bFwlOld;						// Old version of Fwl file
	bool					m_bCsiOld;						// Old version of Csi file
	char					m_szCsiFileName[MAX_FILENAME_LENGTH];	// Csi file name
	char					m_szFwlFileName[MAX_FILENAME_LENGTH];	// fwl file name
	uInt32                  m_u32IgnoreCalibErrorMask;
	uInt32                  m_u32IgnoreCalibErrorMaskLocal;
	CSI_ENTRY				m_AddonCsiEntry;
	CSI_ENTRY				m_BaseBoardCsiEntry;
	FPGA_HEADER_INFO		m_BaseBoardExpertEntry[2];		// Active Xpert on base board
	BOOLEAN					m_bInvalidHwConfiguration;		// The card has wrong limitation table
	bool					m_bSkipFirmwareCheck;			// Skip validation of firmware version

	uInt32					m_u32HwSegmentCount;
	int64					m_i64HwSegmentSize;
	int64					m_i64HwPostDepth;
	int64					m_i64HwTriggerHoldoff;
	uInt32					m_32SegmentTailBytes;

	CS_TRIG_OUT_CONFIG_PARAMS	m_TrigOutCfg;
	CSACQUISITIONCONFIG			m_OldAcqCfg;
	uInt32					m_u32HwTrigTimeout;
	bool					m_bCalibActive;
//	PeevAdaptor				m_PeevAdaptor;		// Peev Adaptor

	//----- Variable related to Data Transfer
	CHANNEL_DATA_TRANSFER	m_DataTransfer;
	BOOLEAN				m_IntPresent;
	BOOLEAN				m_UseIntrTrigger;
	BOOLEAN				m_UseIntrBusy;
	BOOLEAN				m_UseIntrTransfer;


	CS_FIR_CONFIG_PARAMS	m_FirConfig;
	CS_FFT_CONFIG_PARAMS	m_FFTConfig;
	BOOLEAN				m_bSoftwareAveraging;		// Mode Software Averaging enabled
	bool				m_bMulrecAvgTD;				// Mode Expert Multiple Record AVG
	uInt32				m_u32MulrecAvgCount;		// Mulrec AVG count (should be <= 1024)
	int16				*m_pAverageBuffer;			// Temporarary buffer to keep the result of avearging
	BOOLEAN				m_bFlashReset;				// Reset flash is needed before any operation on Flash

	eCalMode				m_CalibMode;
	CS_TRIGGER_PARAMS	m_CfgTrigParams[CSPLNDA_TOTAL_TRIGENGINES];

	uInt32                     m_u32FrontPortResistance_mOhm[CSPLNDA_CHANNELS];
	uInt16                     m_u16GainTarFactor[CSPLNDA_CHANNELS][CSPLNDA_MAX_RANGES][CSPLNDA_IMPED][CSPLNDA_FILTERS];
	uInt32					   m_u32SrInfoSize;
	uInt16                     m_u16TrimCap[CSPLNDA_CHANNELS][2];

	static const uInt16 c_u16R918;
	static const uInt16 c_u16R921;
	static const uInt16 c_u16R922;
	static const uInt16 c_u16R922_v2;
	static const uInt16 c_u16R923;
	static const uInt16 c_u16R977;
	static const uInt16 c_u16R978;
	static const uInt16 c_u16R980;
	static const uInt16 c_u16R1637;
	static const uInt16 c_u16R1638;
	static const uInt16 c_u16R1639;
	static const uInt16 c_u16L6;
	static const uInt32 c_u32MaxCalVoltage_uV;
	static const uInt32 c_u32MaxCalVoltageBoosted_uV;
	static const uInt16 c_u16CalLevelSteps;

	static const uInt32 c_u32ScaleFactor;
	static const uInt32 c_u32K_scaled;
	static const uInt32 c_u32K_scaled_v2;
	static const uInt32 c_u32D_scaled;
	static const uInt32 c_u32A_scaled;
	static const uInt32 c_u32B_scaled;
	static const uInt32 c_u32C_scaled;

	static const uInt16 c_u16Vref_mV;
	static const uInt16 c_u16CalDacCount;
	static const uInt8  c_u8DacSettleDelay;
	static const uInt32 c_u32R77_R67;
	static const uInt32 c_u32CalSrcDelay;
	static const uInt32 c_u32CalSrcSettleDelay;
	static const uInt32 c_u32CalSrcSettleDelay_1MOhm;

	static const uInt16 c_u32MarginNom;
	             uInt16 m_u32MarginDenom;
	static const uInt16 c_u32MarginDenomHiRange;
	static const uInt16 c_u32MarginDenomLoRange;
	static const uInt16 c_u32MarginDenomHiRange_v2;
	static const uInt16 c_u32MarginDenomLoRange_v2;
	static const uInt16 c_u32MarginLoRangeBoundary;

	void   AcquireData(uInt32 u32IntrEnabled = 0);
	void   Abort();
	int32  CsSetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg = NULL );
	int32  CsSetTriggerConfig( PARRAY_TRIGGERCONFIG pATrigCfg = NULL );
	int32  CsSetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg = NULL );


	int32	CsGetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg );
	int32   CsGetTriggerConfig( PARRAY_TRIGGERCONFIG pATriggerCfg );
	int32	CsGetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg );

	BOOLEAN	IsChannelIndexValid( uInt16 u16ChannelIndex );
	int32  GetFreeTriggerEngine( int32	i32TrigSource, uInt16 *u16TriggerEngine );

	uInt16 NormalizedChannelIndex( uInt16 u16ChannelIndex );
	int32  NormalizedTriggerSource( int32 i32TriggerSource );
	uInt16 GetUserChannelSkip();


	//----- "Hardware routines" (CsXYG8Hw.cpp):
	int32 SendClockSetting(LONGLONG llSampleRate = CSPLNDA_DEFAULT_SAMPLE_RATE, uInt32 u32ExtClk = 0, uInt32 u32ExtClkSampleSkip = 0, BOOLEAN bRefClk = FALSE);
	int32 RelockReference();
	int32 CalibrateFrontEnd(uInt16 u16Channel, uInt32* pu32SuccessFlag);
	void  MsInvalidateCalib();

	void  SetDefaultTrigAddrOffset(void);
	int32 SendCaptureModeSetting(uInt32 u32Mode = CSPLNDA_DEFAULT_MODE );
	int32 SendCalibModeSetting(const uInt16 u16Channel, const eCalMode ecMode = ecmCalModeNormal);
	int32 SetClockForCalibration(const bool bSet);
	int32 SetSegmentForCalibration(const bool bSet);
	int32 CalibrateAllChannels(void);
	int32 MsCalibrateAllChannels(void);
	int32 DoFastCalibrationAllChannels();
	int32 SendCalibSource(const eCalMode ecMode = ecmCalModeDc, const uInt32 u32CalFequency = 0);
	int32 SendChannelSetting(uInt16 Channel, uInt32 u32InputRange = CSPLNDA_DEFAULT_GAIN, uInt32 u32Term = CSPLNDA_DEFAULT_TERM, uInt32 u32Impendance = CSPLNDA_DEFAULT_IMPEDANCE, int32 i32Position = 0, uInt32 u32Filter = 0, int32 i32ForceCalib = 0);
	int32	SendAdcOffsetAjust(uInt16 Channel, uInt16 u16Offet );

	int32 SendDigitalTriggerSensitivity( uInt16 u16TrigEngine, BOOLEAN bA, uInt16 u16Sensitivity = CSPLNDA_DEFAULT_TRIG_SENSITIVITY );
	int32 SendDigitalTriggerLevel( uInt16  u16TrigEngine, BOOLEAN bA, int32 i32Level);
	int32 SendExtTriggerLevel( int32 int32Level, uInt32 u32ExtCondition,  uInt32 u32ExtTriggerRange );

	int32 SendTriggerEngineSetting(uInt16 u16TrigEngine, int32 i32SourceA, uInt32  u32ConditionA  = CS_TRIG_COND_POS_SLOPE,  int32   i32LevelA = 0,
										  int32	  i32SourceB = 0, uInt32  u32ConditionB = CS_TRIG_COND_POS_SLOPE, int32   i32LevelB = 0);

	int32 SendExtTriggerSetting( BOOLEAN  bExtTrigEnabled = FALSE,
										   eTrigEnableMode eteSet = eteAlways,
										   int32	i32ExtLevel = 0,
										   uInt32	u32ExtCondition = CSPLNDA_DEFAULT_CONDITION,
										   uInt32	u32ExtTriggerRange =  CSPLNDA_DEFAULT_EXT_GAIN,
										   uInt32	u32ExtCoupling = CSPLNDA_DEFAULT_EXT_COUPLING);

	int32 SendTriggerRelation( uInt32 u32Relation	);
	int32 WriteToSelfTestRegister( uInt16 SelfTestMode = CSST_DISABLE );
	int32 InitAddOnRegisters(bool bForce = false);

	int32 WriteToCalibDac(uInt16 Addr, uInt16 u32Data, uInt8 u8Delay = 0);


	void	AssignBoardType();
	void	AssignAcqSystemBoardName( PCSSYSTEMINFO pSysInfo );
	int32	AcqSystemInitialize();
	int32	MsCsResetDefaultParams( bool bReset = FALSE );
	uInt8	GetExtClkTriggerAddressOffsetAdjust();
	void	SetExtClkTriggerAddressOffsetAdjust( uInt8 u8TrigAddressOffset );
	int16	GetTriggerAddressOffsetAdjust( uInt32 u32Mode, uInt32 u32SampleRate );
	void	SetTriggerAddressOffsetAdjust( int8 u8TrigAddressOffset );
	BOOL	CheckRequiredFirmwareVersion();
	uInt32	GetDataMaskModeTransfer ( uInt32 u32TxMode );
	uInt32	GetFifoModeTransfer( uInt16 u16UserChannel );
	int32	FillOutBoardCaps(PCSSPLENDABOARDCAPS pCapsInfo);

	eTrigEnableMode m_eteTrigEnable;

	bool			m_bUseEepromCal;
	bool			m_bUseEepromCalBackup;
	bool            m_bSkipSpiTest;
	bool			m_bSkipCalib;
	bool			m_bSkipTrim;
	bool			m_bNoConfigReset;
	bool			m_bSafeImageActive;
	bool			m_bClearConfigBootLocation;
	uInt16			m_u16SkipCalibDelay; //in minutes;
	bool			m_bNeverCalibrateDc;
	uInt16			m_u16AdcOffsetAdjust;
	bool			m_bClockFilter;
	bool			m_bGainTargetAdjust;
	bool            m_b5V1MDiv50;
	bool			m_bForce100MSps;
	bool			m_bNoFilter;

	uInt16			m_u16ChanProtectStatus;
	uInt8			m_u8BootLocation;					// BootLocation: 0 = BootImage, 1 = Image0; 2= Image1 ...

	int32	SetTrigOut( uInt16 u16Value );
	int32	SetClockOut( eClkOutMode ecomSet= eclkoSampleClk );

	int32	SendSegmentSetting(uInt32 u32SegmentCount, int64 i64PostDepth, int64 i64SegmentSize, int64 i64HoldOff, int64 i64TrigDelay = 0);

	bool	IsAddonInit();
	void	ReadVersionInfo();
	void	ReadVersionInfoEx();

	//----- Read/Write the Add-On Board Flash
	uInt16	GetTriggerLevelOffset( uInt16	u16TrigEngine );
	void	SetTriggerLevelOffset( uInt16 u16TriggerEngine, uInt16 u16TrigLevelOffset );
	void	SetTriggerGainAdjust( uInt16 u16TrigEngine, uInt16 u16TrigGain );
	uInt16	GetTriggerGainAdjust( uInt16	u16TrigEngine );

	int32	ReadTriggerTimeStampEtb( uInt32 StartSegment, int64 *pBuffer, uInt32 u32NumOfSeg );

	int32	SetupBufferForCalibration( bool bSetup, eCalMode ecMode = ecmCalModeDc );
	int32	SetupForCalibration( bool bSetup, eCalMode ecMode = ecmCalModeDc );

	int32	CsGetAverage(uInt16 u16Channel, int16* pi16Buffer, uInt32 u32BufferSize, int32* pi32Avrage_x10);
	void	SetupHwAverageMode();
	int32	ProcessSoftwareAveraging( int64 i64StartAddress, uInt16 u16Channel, int32 *pi32ResultBuffer, uInt32 u32SamplesInBuffer );
#ifdef _WINDOWS_    
	void	ReadCommonRegistryOnBoardInit();
	void	ReadCommonRegistryOnAcqSystemInit();
	void	ReadCalibrationSettings(void *Params);
#else
   int32 ReadCommonCfgOnBoardInit();   
	int32	ReadCommonCfgOnAcqSystemInit();
   void  ReadCalibrationSettings(char *szIniFile, void *Params);   
   int32 ShowCfgKeyInfo();
#endif   
	int32	ReadAndValidateCsiFile( uInt32 u32BbHwOpt );
	int32	ReadAndValidateFwlFile();
	void	CsSetDataFormat( _CSDATAFORMAT csFormat = formatDefault );
	void	IsAddonBoardPresent();
	int32	ReadCalibInfoFromCalFile();

	void	AbortSystemDmaTransferDescriptor();
	void	SignalEventsExpert();
	int32	CsDelayLoadBaseBoardFpgaImage( uInt8 u8ImageId );

	void	AdjustedHwSegmentParameters( uInt32 u32Mode, uInt32* pu32RealSegmentCount, int64* pi64RealSegmentSize, int64* pi64RealDepth );
	int32	CommitNewConfiguration( PCSACQUISITIONCONFIG pAcqCfg,
									PARRAY_CHANNELCONFIG pAChanCfg,
									PARRAY_TRIGGERCONFIG pATrigCfg );

	uInt32	CsGetPostTriggerExtraSamples( uInt32 u32Mode );
	uInt32	GetHwTriggerAddress( uInt32 u32Segment, BOOLEAN *bRollOver = NULL );

public:
	CsSplendaDev *GetCardPointerFromChannelIndex( uInt16 ChannelIndex );
	CsSplendaDev *GetCardPointerFromBoardIndex( uInt16 BoardIndex );
	CsSplendaDev *GetCardPointerFromTriggerIndex( uInt16 TriggerIndex );
	CsSplendaDev *GetCardPointerFromTriggerSource( int32 i32TriggerSoure );

	int32 	DeviceInitialize(void);
	int32	RemoveDeviceFromUnCfgList();
	int32	CsAcquisitionSystemInitialize( uInt32 u32InitFlag );
	int32	CsGetParams( uInt32 u32ParamId, PVOID pUserBuffer, uInt32 u32OutSize = 0);
	int32	CsSetParams( uInt32 u32ParamId, PVOID pInBuffer, uInt32 u32InSize = 0 );
	int32	CsStartAcquisition();
	int32	CsAbort();
	void	CsForceTrigger();
	int32	CsDataTransfer( in_DATA_TRANSFER *InDataXfer );
	uInt32	CsGetAcquisitionStatus();
	int32	CsRegisterEventHandle(in_REGISTER_EVENT_HANDLE &InRegEvnt );
	void	CsAcquisitionSystemCleanup();
	void	CsSystemReset(BOOL bHard_reset);
	int32	CsForceCalibration();
	int32	CsGetAsyncDataTransferResult( uInt16 u16Channel,CSTRANSFERDATARESULT *pTxDataResult, BOOL bWait );
	int32	CsGetAcquisitionSystemCount( BOOL bForceInd, uInt16 *pu16SystemFound );

	int32	ProcessDataTransfer(in_DATA_TRANSFER *InDataXfer);
	int32	ProcessDataTransferDebug(in_DATA_TRANSFER *InDataXfer);
	int32	ProcessDataTransferModeAveraging(in_DATA_TRANSFER *InDataXfer);
	int32   CsGetBoardsInfo( PARRAY_BOARDINFO pABoardInfo );

	int32	CsValidateAcquisitionConfig( PCSACQUISITIONCONFIG, uInt32 u32Coerce );

	void	UpdateSystemInfo( CSSYSTEMINFO *SysInfo );

	uInt16 BuildIndependantSystem( CsSplendaDev** FirstCard, BOOLEAN bForceInd );
	uInt16 BuildMasterSlaveSystem( CsSplendaDev** FirstCard );
	uInt16	DetectAcquisitionSystems( BOOL bForceInd );

	uInt16	ConvertToHwChannel( uInt16 u16UserChannel );
	uInt16	ConvertToUserChannel( uInt16 u16HwChannel );
	bool    IsCalibrationRequired(bool bForHw = false);
	int32	FindGainFactorIndex( uInt16 u16Channel, uInt32 u32InpuRange, uInt32 u32Impedance, uInt32 u32Filter,
											 uInt32 *u32RangeIndex, uInt32 *u32ImpedIndex, uInt32 *u32FilterIndex);

	int32	SetPosition(uInt16 u16Channel, int32 i32Position_mV);
	int32	SendPosition(uInt16 u16Channel, int32 i32Position_mV, uInt8 u8Delay = 0);

	int32 FindFeIndex( const uInt32 u32Swing, const uInt32 u32Imed, uInt32& u32Index );

	void   ClearFlag_bCalNeeded();

	int32	AcquireAndAverage(uInt16	u16Channel, int32* pi32Avrage_x10);
	int32	CalibrateOffset(uInt16 u16Channel, uInt32* pu32SuccessFlag);
	int32	CalibrateGain(uInt16 u16Channel);
	int32	CalibratePosition(uInt16 u16Channel);

	int32		SendDigitalTriggerLevel( uInt16  u16TrigEngine, int32 i32Level );
	bool		FindSrInfo( const LONGLONG llSr, SPLENDA_SAMPLE_RATE_INFO *pSrInfo );

	void		SetFastCalibration(uInt16 u16Channel);
	void		InitBoardCaps();
	int32		CsAutoUpdateFirmware();
	int32		ReadCalibTableFromEeprom();
	int32		WriteCalibTableToEeprom( uInt32 u32Valid );
	void		UpdateCalibVariables();
	void		UpdateMaxMemorySize( uInt32	u32BaseBoardHardwareOptions, uInt32 u32SampleSize );

	int32		AcquireCalibSignal( uInt16 u16UserChannel, int32 i32StartPos, int32 *i32OffsetAdjust );

	int32	ReadCalibA2D(int32 i32ExpectedVoltage, eCalInput eInput, int32* pi32V_uv=NULL, int32* pi32Noise_01LSB = NULL);
	int32	FastCalibrationSettings(uInt16 u16Channel, bool bForce = false);
	bool	ForceFastCalibrationSettingsAllChannels();

	void TraceCalib(uInt16 u16Channel, uInt32 u32TraceFlag, uInt32 u32V1 = 0, uInt32 u32V2=0, int32 i32V3=0, int32 i32V4=0);

	EVENT_HANDLE	GetEventTxDone() { return m_DataTransfer.hAsynEvent; }
	EVENT_HANDLE	GetEventSystemCleanup() { return m_pSystem->hAcqSystemCleanup; }
	EVENT_HANDLE	GetEventTriggered() { return m_pSystem->hEventTriggered; }
	EVENT_HANDLE	GetEventEndOfBusy() { return m_pSystem->hEventEndOfBusy; }
	EVENT_HANDLE	GetEventAlarm() { return m_pSystem->hEventAlarm; }

	void	SetStateTriggered() { m_PlxData->CsBoard.bTriggered = true; }
	void	SetStateAcqDone() { m_PlxData->CsBoard.bBusy = false; }
	void	SignalEvents( BOOL bAquisitionEvent = TRUE );


	int32	AddonConfigReset(uInt32 Addr);
	void	ResetCachedState(void);
	int32	CsLoadBaseBoardFpgaImage( uInt8 u8ImageId );
	int32	SetClockThruNios( uInt32 u32ClockMhz );
	uInt32	GetDefaultRange();
	uInt16	GetControlDacId( eCalDacId ecdId, uInt16 u16Channel );
	int32	SetupAcquireAverage(bool bSetup = false);
	int32	MsSetupAcquireAverage(bool bSetup = false);
	int32	UpdateCalibDac(uInt16 u16Channel, eCalDacId ecdId, uInt16 u16Data);
	int32	GetEepromCalibInfoValues(uInt16 u16Channel, eCalDacId ecdId, uInt16 &u16Data);
	int32	UpdateCalibInfoStrucure();
	void    UpdateAuxCalibInfoStrucure();

    void *operator new(size_t  size);//implicitly declared as a static member function
//	void operator delete (void *p );						 //implicitly declared as a static member function

	int32	CreateThreadAsynTransfer();
	void	AsynDataTransferDone();

#ifdef __linux__
	void 	CreateInterruptEventThread();
	void	InitEventHandlers();
	void	ThreadFuncIntr();

	// DEBUG   DEBUG  DEBUG   DEBUG
	int32	DebugPrf( uInt32 DebugOption ){};
#endif

	virtual  uInt32	AddonReadFlashByte(uInt32 Addr);
	virtual  void	AddonWriteFlashByte(uInt32 Addr, uInt32 Data);
	virtual uInt32	AddonResetFlash(void);
	int32	WriteEntireEeprom(AddOnFlash32Mbit *Eeprom);
	int32   WriteSectorsOfEeprom(void *pData, uInt32 u32StartAddr, uInt32 u32Size);
	int32	WriteEepromEx(void *pData, uInt32 u32StartAddr, uInt32 u32Size, bool bErrorLog = true );
	int32	ReadEeprom(void *Eeprom, uInt32 Addr, uInt32 Size);
	int32	WriteEeprom(void *Eeprom, uInt32 Addr, uInt32 Size);
	int32	ReadAddOnHardwareInfoFromEeprom();



	const static FRONTEND_SPLENDA_INFO	c_FrontEndSplendaGain50[];
	const static FRONTEND_SPLENDA_INFO	s_FrontEndSplendaGain1M[];
	const static FRONTEND_SPLENDA_INFO	s_FrontEndSplendaGain1M_div10[];
	const static SPLENDA_SAMPLE_RATE_INFO c_SPLNDA_SR_INFO_200[];
	const static SPLENDA_SAMPLE_RATE_INFO c_SPLNDA_SR_INFO_100[];
	static SPLENDA_CAL_LUT	s_CalCfg50[];
	static SPLENDA_CAL_LUT	s_CalCfg1M[];


	int32 ResyncClock(void);
	int32 CheckMasterSlaveFirmwareVersion();

	void	DetectMasterSlave(int16 *i16NumMasterSlaveSystems, int16 *i16NumStandAloneSystems);
	int32	SendMasterSlaveSetting();
	int32	EnableMsBist(bool bEnable);
	int32	ResetMasterSlave();
	int32	DetectSlaves();
	int32	TestMsBridge();
	int32	CsDisableAllTriggers();

	int32  CheckCalibrationReferences();
	void   ReportProtectionFault();
	int32  SendCalibDC(uInt32 u32Imped, uInt32 u32Range, int32 i32Level, int32* pi32SetLevel_uV=NULL, bool bCheckRef=false );
	int32  FindCalLutItem( const uInt32 u32Swing, const uInt32 u32Imped, SPLENDA_CAL_LUT& Item );
	int32	EepromCalibrationSettings(uInt16 u16Channel, bool bForceSet = false );
	int32	DefaultCalibrationSettings(uInt16 u16Channel, uInt32 u32RestoreFlag = CSPLNDA_CAL_RESTORE_ALL);
	bool	GetChannelProtectionStatus();
	int32	ClearChannelProtectionStatus();
	int32	MsClearChannelProtectionStatus( bool bForce = false );
	void	OnAlarmEvent();
	int32	FindFeCalIndices( uInt16 u16Channel, uInt32& u32RangeIndex,  uInt32& u32FeModIndex );
	int32	SendAddonInit();

	int32	AddonFlashWriteEnable(void);
	int32	AddonFlashResetWriteEnable(void);
	uInt32	AddonFlashReadStatus(void);
	bool	VerifySwappedFpgaBus();
	int32	ReadCsiFileAndProgramFpga();
	int32	IoctlProgramAddonFpga(uInt8 *pBuffer, uInt32 u32ImageSize);
	int32	ProgramAddonFpga(uInt8 *pBuffer, uInt32 u32ImageSize);
	int32	WriteFpgaString(unsigned char *buffer, int numbytes);
	int32	TrimCapacitor( const uInt16 cu16ZeroBased, uInt16 u16Value, bool bDiv10 );

	uInt16	ReadTriggerStatus();
	uInt16	ReadBusyStatus();
	int32	CombineCardUpdateFirmware();

	// Overload functions to support Nulceon
	BOOLEAN ConfigurationReset( uInt8	u8UserImageId );
	int32	BaseBoardConfigurationReset(uInt8	u8UserImageId);
	int32	CsReadPlxNvRAM( PPLX_NVRAM_IMAGE nvRAM_image );
	int32	CsWritePlxNvRam( PPLX_NVRAM_IMAGE nvRAM_image );

	// Nucleon PciExpress
	bool	IsImageValid( uInt8 u8Index );
	BOOL    IsNiosInit();
	int32	CsNucleonCardUpdateFirmware();
	int32	ReadBaseBoardHardwareInfoFromFlash( BOOLEAN bChecksum = TRUE);
	int32	NucleonWriteFlash(uInt32 Addr, void *pBuffer, uInt32 WriteSize, bool bBackup = FALSE );
	int32	IoctlGetPciExLnkInfo( PPCIeLINK_INFO pPciExInfo );

	int64	GetTimeStampFrequency();
	
	int32 	CsStmTransferToBuffer( PVOID pVa, uInt32 u32TransferSize );
	int32	CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *u32ErrorFlag, uInt32 *u32ActualSize, uInt8 *u8EndOfData );
	int32	CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *u32ErrorFlag );
	void 	MsStmResetParams();
	bool	IsConfigResetRequired( uInt8 u8ExpertImageId );
	bool	IsExpertAVG(uInt8 u8ImageId);
	bool	IsExpertSTREAM(uInt8 u8ImageId = 0);
	void	PreCalculateStreamDataSize(PCSACQUISITIONCONFIG pCsAcqCfg);
	uInt8	ReadBaseBoardCpldVersion();
	uInt32	ReadConfigBootLocation();
	int32	WriteConfigBootLocation( uInt32	u32Location );

	int32	MsInitializeDevIoCtl();
#ifdef USE_SHARED_CARDSTATE
	int32	InitializeDevIoCtl();
#endif

	int32	CsCheckPowerState()
	{	if ( 0 == m_pTriggerMaster->m_pCardState->bCardStateValid )
			return CS_POWERSTATE_ERROR;
		else
			return CS_SUCCESS;
	}

//----- Member Functions
};


// Splenda defines
#define CSPLNDA_MS_TrigCpld_REQUIRED 0x0105

#define CSPLNDA_NCONFIG				0x10000 //(UINT)(1<<16)
#define CSPLNDA_CFG_CHIP_SELECT		0x20000 //(UINT)(1<<17)
#define CSPLNDA_CFG_WRITE_SELECT	0x40000 //(UINT)(1<<18)
#define CSPLNDA_CFG_ENABLE			0x80000 //(UINT)(1<<19)

#define CSPLNDA_CFG_DONE	0x2000000 //(UINT)(1<<25)
#define CSPLNDA_CFG_STATUS	0x4000000 //(UINT)(1<<26)
#define CSPLNDA_CFG_BUSY	0x8000000 //(UINT)(1<<27)

#define CSPLNDA_CALA2D_2p5_V    2500
#define CSPLNDA_CALA2D_5_V      5000
#define CSPLNDA_CALA2D_10_V    10000

#define CSPLNDA_CALREF0		    2048
#define CSPLNDA_CALREF1		    4096
#define CSPLNDA_CALREF2		   10000
#define CSPLNDA_CALREF3		       0
#define CSPLNDA_MAX_CALREF_ERR_C  50
#define CSPLNDA_MAX_CALREF_ERR_P   5
#define CSPLNDA_POS_CAL_LEVEL      1
#define CSPLNDA_GND_CAL_LEVEL      0
#define CSPLNDA_NEG_CAL_LEVEL     -1


#define TRACE_COARSE_OFFSET    0x0001 //Trace of the coarse offset
#define TRACE_FINE_OFFSET      0x0002 //Fine offset setting
#define TRACE_CAL_GAIN         0x0004 //Gain offset settings
#define TRACE_CAL_POS          0x0008 //Position calibration settings
#define TRACE_CAL_SIGNAL       0x0010 //Trace final result of calibration signal DAC
#define TRACE_CAL_DAC          0x0020 //Trace acces to All calibration DACs
#define TRACE_CAL_RESULT       0x0040 //Trace result of failed calibration if we go on
#define TRACE_CAL_RANGE        0x0080 //Trace calibration settings
//verbose
#define TRACE_CAL_SIGNAL_ITER  0x0100 //Trace  of the calibration ADC reading
#define TRACE_CAL_ITER         0x0200 //Trace of the iterations of the current stage
#define TRACE_ALL_DAC          0x0400 //Trace acces to All DACs
#define TRACE_CAL_SIGNAL_VERB  0x0800 //Trace final result of calibration signal DAC verbose

#define TRACE_CAL_PROGRESS     0x1000 //Trace calibration progress
#define TRACE_GAIN_ADJ         0x2000 //Trace gain adjust




// Prototype for functions
unsigned int WINAPI ThreadFuncIntr( LPVOID lpParam );
unsigned int WINAPI ThreadFuncTxfer( LPVOID lpParam );

#endif
