//! CsRabbit.h
//!
//!

#ifndef __CsRabbit_h__
#define __CsRabbit_h__

#include "CsDefines.h"
#include "CsTypes.h"
#include "CsPlxDefines.h"
#include "CsDeviceIDs.h"
#include "CsAdvStructs.h"
#include "CsiHeader.h"
#include "CsFirmwareDefs.h"
#include "CsBunnyCapsInfo.h"
#include "CsRabbitFlash.h"
#include "CsBunnyCal.h"

#include "CsPlxBase.h"
#include "CsNiosApi.h"

#include "CsDrvStructs.h"
#include "DrvCommonType.h"
#include "DllStruct.h"
#include "GageWrapper.h"
#include "CsSharedMemory.h"

#include "CsFirmwareDefs.h"
#include "CsRabbitOptions.h"
#include "CsRabbitState.h"
#include "CsNucleonBaseFlash.h"
#include "PeevAdaptor.h"

#ifdef _WINDOWS_
#define USE_SHARED_CARDSTATE
#endif

#define	USE_NEW_TRIGADDROFFSET_TABLE

#ifdef _RABBIT_DRV_
	#define RABBIT_SHARENAME			_T("CsRabbit")

	#define	SHARED_CAPS_NAME			_T("CsxyG8WDFCaps")
	#define MEMORY_MAPPED_SECTION		_T("CsxyG8WDF-shared") // CR-2621
#else
	#define RABBIT_SHARENAME			_T("CsEcdG8")

	#define	SHARED_CAPS_NAME			_T("CsEcdG8WDFCaps")
	#define MEMORY_MAPPED_SECTION		_T("CsEcdG8WDF-shared") // CR-2621
#endif

#define		PAGE_SIZE		4096


#define TRACE_ZERO_OFFSET      0x0001 //Trace of the Biased offset 
#define TRACE_CAL_FE_OFFSET    0x0002 //Fe offset setting
#define TRACE_CAL_GAIN         0x0004 //Gain offset settings
#define TRACE_CAL_POS          0x0008 //Position calibration settings
#define TRACE_CAL_SIGNAL       0x0010 //Trace final result of calibration signal DAC
#define TRACE_DAC              0x0020 //Trace acces to All calibration DACs
#define TRACE_CAL_RESULT       0x0040 //Trace result of failed calibration if we go on
#define TRACE_CAL_SIGNAL_ITER  0x0080 //Trace itteration of the calibration signal DAC
#define TRACE_CAL_RANGE        0x0100 //Trace calibration settings
#define TRACE_CAL_PINGPONG     0x0200 //Trace gain calibration in ping-pong mode
#define TRACE_ZERO_OFFSET_IT   0x0400 //Trace of the Biased offset iterations
#define TRACE_CAL_PINGPONG_IT  0x0800 //Trace gain calibration iterations in ping-pong mode
#define TRACE_CAL_PROGRESS     0x1000 //Trace calibration progress
#define TRACE_CAL_MS	       0x2000 //Trace Master/Slave TrigAddr Offset calibration
#define TRACE_CAL_EXTTRIG	   0x4000 //Trace Ext trigger Offset calibration


//	Leave debug code even in realse build just for now, while we are not sure 
//	100 % everything is working well
//#ifdef DBG
	#define		DEBUG_MASTERSLAVE
	#define		DEBUG_EXTTRIGCALIB
//#endif

#define	MAX_FILENAME_LENGTH				128


#define		CSRB_MAX_TRIGGER_ADDRESS_OFFSET			16	//	(Dual)	Double in Single
#define		CSRB_MAX_TAIL_INVALID_SAMPLES_DUAL		96	//	(Dual)	
#define		CSRB_MAX_TAIL_INVALID_SAMPLES_SINGLE	160	//	(Single)


// Calibration for External trigger
#define	EXTTRIGCALIB_LEVEL_EXT_50		0
#define	EXTTRIGCALIB_LEVEL_EXT_HiZ		0
#define EXTTRIGCALIB_SENSITIVE_1V		100
#define EXTTRIGCALIB_SENSITIVE_5V		60
#define	EXTTRIGCALIB_LEVEL_CHAN			0
#define	EXTTRIGCALIB_LEVELLOW_DAC_VAL	2400

#define	PULSE_VALIDATION_LENGTH			20
#define	MS_CALIB_LENGTH					128

#define	MSCALIB_LEVEL_CHAN				-20
#define	SINGLEPULSECALIB_SENSITIVE		80
#define SINGLEPULSECALIB_SLOPE			CS_TRIG_COND_NEG_SLOPE

#define	MAX_EXTTRIG_ADJUST					31			
#define	MIN_EXTTRIG_ADJUST					-32			
#define	MAX_DIGITALDELAY_ADJUST				255			// Only for Nucleon	
#define	MIN_DIGITALDELAY_ADJUST				(-256)		// Only for Nucleon	
#define	EXTTRIG_CALIB_LENGTH				(1+MAX_DIGITALDELAY_ADJUST-MIN_DIGITALDELAY_ADJUST)		// Only for Nucleon	

#define CSRB_CALIB_DEPTH		  (128*1024)
#define CSRB_CALIB_BUF_MARGIN	  512
#define CSRB_CALIB_BUFFER_SIZE	  Max( (2 * CSRB_CALIB_BUF_MARGIN + CSRB_CALIB_DEPTH), EXTTRIG_CALIB_LENGTH)
#define CSRB_CALIB_BLOCK_SIZE	  64

// Limitation of hardware averaging
#define	AV_INVALIDSAMPLES_HEAD			28				// Limitation of FW
#define	AV_INVALIDSAMPLES_TAIL			8				// Limitation of FW


#define	AV_ENSURE_POSTTRIG				64				// (Max TrigAddrOffset + AV_INVALIDSAMPLES_HEAD
														// + AV_INVALIDSAMPLES_TAIL)

#define MAX_AVERAGING_CUTOFF			128				// Limation of FW 64 + 
														// 64 (Max TrigAddrOffset + AV_INVALIDSAMPLES_HEAD
														// + AV_INVALIDSAMPLES_TAIL)


#define CSRB_NUCLEON_MAX_MEM_HWAVG_DEPTH		(0x10000-192)
#define CSRB_PCI_MAX_MEM_HWAVG_DEPTH			(0xC000-192)
//#define MAX_DRIVER_HWAVERAGING_DEPTH	(0xC000-192)	// Max trigger depth in HW Averaging mode (in samples) from application SW point of view
#define MAX_AVERAGING_PRETRIGDEPTH		16				// Max pre trigger depth in HW Averaging mode (in samples)


// Limitation of sofware averaging
#define MAX_SW_AVERAGING_SEMGENTSIZE	512*1024		// Max segment size in Software averaging mode (in samples)
#define	SW_AVERAGING_BUFFER_PADDING		16				// Padding for Sw Avearge buffer
#define MAX_SW_AVERAGE_SEGMENTCOUNT		4096			// Max segment count in SW averaging mode



#define CSRB_ADDONFLASH_MFG_ID			0x89

#define CSRB_DEFAULT_VALUE				0	
#define CSRB_DEFAULT_RELATION			CS_RELATION_OR
#define CSRB_DEFAULT_LEVEL				0

#define	CSRB_MEMORY_CLOCK				150000000		// 150MHz clock
#define CSRB_PCIEX_MEMORY_CLOCK			200000000		// 200MHz clock
#define CSRB_DAC_SETTLE_DELAY			5

//!----- Default settings
#define	CSRB_DEFAULT_MSTRIGMODE			3
#define CSRB_DEFAULT_TRIG_SENSITIVITY	10
#define CSRB_DEFAULT_EXT_TRIG_SENSE		60
#define CSRB_DEFAULT_SAMPLE_RATE		-1				// Force to the default sampling rate depending on card
#define CSRB_DEFAULT_EXT_CLOCK_EN		0
#define CSRB_DEFAULT_MODE				CS_MODE_DUAL
#define CSRB_DEFAULT_SAMPLE_BITS		CS_SAMPLE_BITS_8
#define CSRB_DEFAULT_RES				(-1*CS_SAMPLE_RES_8)
#define CSRB_DEFAULT_SAMPLE_SIZE		CS_SAMPLE_SIZE_1
#define CSRB_DEFAULT_SAMPLE_OFFSET		CS_SAMPLE_OFFSET_UNSIGN_8
#define CSRB_DEFAULT_SEGMENT_COUNT		1
#define CSRB_DEFAULT_DEPTH				8192
#define CSRB_DEFAULT_SEGMENT_SIZE		CSRB_DEFAULT_DEPTH
#define CSRB_DEFAULT_TRIG_TIME_OUT		-1
#define CSRB_DEFAULT_TRIG_ENGINES_EN	CS_TRIG_ENGINES_DIS
#define CSRB_DEFAULT_TRIG_HOLD_OFF		0
#define CSRB_DEFAULT_TRIG_DELAY			0

#define CSRB_DEFAULT_CHANNEL_INDEX		0
#define CSRB_DEFAULT_TERM				CS_COUPLING_DC
#define CSRB_DEFAULT_GAIN				CS_GAIN_2_V
#define CSRB_DEFAULT_GAIN_HIGHBW		CS_GAIN_400_MV
#define CSRB_DEFAULT_INPUTRANGE			-1				// Force to the default input range depending on card
#define CSRB_DEFAULT_IMPEDANCE			CS_REAL_IMP_50_OHM
#define CSRB_DEFAULT_FILTER				0
#define CSRB_DEFAULT_POSITION			0

#define CSRB_DEFAULT_CONDITION			CS_TRIG_COND_POS_SLOPE
#define CSRB_DEFAULT_SOURCE				CS_CHAN_1
#define CSRB_DEFAULT_EXT_COUPLING		CS_COUPLING_DC
#define CSRB_DEFAULT_EXT_GAIN			CS_GAIN_2_V
#define CSRB_DEFAULT_EXT_IMPEDANCE		CS_REAL_IMP_50_OHM

#define CSRB_DEFAULT_CLKOUT				eclkoNone	
#define CSRB_DEFAULT_EXTTRIGEN			eteAlways


// Trigger Engine Index in Table
#define	CSRB_TRIGENGINE_EXT		0
#define	CSRB_TRIGENGINE_A1		1
#define	CSRB_TRIGENGINE_B1		2
#define	CSRB_TRIGENGINE_A2		3
#define	CSRB_TRIGENGINE_B2		4

#define CSRB_SPI				0x8000

//----- Trigger Setting
//Reg CSRB_EXT_TRIGGER_SETTING
#define CSRB_SET_EXT_TRIGGER_POS_SLOPE	0x0004
#define CSRB_SET_EXT_TRIGGER_ENABLED	0x0008

//Reg CSRB_SET_EXT_TRIG
#define CSRB_SET_EXT_TRIG_TRIGOUTEN		0x0001
#define CSRB_SET_EXT_TRIG_BASE          0x0C00
#define CSRB_SET_EXT_TRIG_AC			0x0004
#define CSRB_SET_EXT_TRIG_HiZ			0x0008
#define CSRB_SET_EXT_TRIG_EXT5V			0x0010
#define CSRB_SET_EXT_TRIG_MUX_RST		0x0100
#define CSRB_SET_EXTTRIG_MUX_SYNC		0x0200

#define CSRB_SET_DIG_TRIG_A1_ENABLE		0x0001
#define CSRB_SET_DIG_TRIG_B1_ENABLE		0x0002
#define CSRB_SET_DIG_TRIG_A1_POS_SLOPE	0x0004
#define CSRB_SET_DIG_TRIG_B1_POS_SLOPE	0x0008
#define CSRB_SET_DIG_TRIG_A1_CHAN_SEL	0x0010
#define CSRB_SET_DIG_TRIG_B1_CHAN_SEL	0x0020
#define CSRB_SET_DIG_TRIG_A2_ENABLE		0x0040
#define CSRB_SET_DIG_TRIG_B2_ENABLE		0x0080
#define CSRB_SET_DIG_TRIG_A2_POS_SLOPE	0x0100
#define CSRB_SET_DIG_TRIG_B2_POS_SLOPE	0x0200
#define CSRB_SET_DIG_TRIG_A2_CHAN_SEL	0x0400
#define CSRB_SET_DIG_TRIG_B2_CHAN_SEL	0x0800

#define CSRB_GLB_TRIG_ENG_ENABLE        0x0001


// Rabit Registers Map
#define	CSRB_SET_CHAN1					0x00810001
#define	CSRB_SET_CHAN2					0x00810101
#define	CSRB_SET_CHAN_OFFSET			0x00810201
#define	CSRB_SET_ADC					0x00810301
#define	CSRB_SET_CALIB					0x00810401
#define	CSRB_SET_EXT_TRIG				0x00810501
#define	CSRB_CLOCK_REG1					0x00810601
#define	CSRB_CLOCK_REG2					0x00810701
#define	CSRB_MS_REG_RD					0x00800901
#define	CSRB_MS_REG_WR					0x00810901
#define CSRB_MS_BIST_CTL1_WR			0x00810A01
#define CSRB_MS_BIST_CTL2_WR			0x00810B01
#define CSRB_MS_BIST_ST1_RD				0x00801C01
#define CSRB_MS_BIST_ST2_RD				0x00801D01



#define CSRB_CLK_CTRL					0x00812101
#define CSRB_CHANNEL_CTRL				0x00814501
#define CSRB_DEC_FACTOR_LOW_WR_CMD		0x00814601
#define CSRB_DEC_FACTOR_HIGH_WR_CMD		0x00814701
#define CSRB_DATA_SEL_ADDR				0x00814101

#define CSRB_TEST_REG_WR_CMD			0x00814001
#define CSRB_SELFTEST_MODE_WR_CMD		0x00814101

#define CSRB_EXT_TRIGGER_SETTING		0x00812501

#define CSRB_TRIG_SETTING_ADDR			0x00815401
#define CSRB_EXTTRIG_ALIGNMENT			0x00815501
#define CSRB_DTE_SETTINGS_ADDR			0x00815601
#define CSRB_TGC_ADDR					0x00815701

//4G digital offset correction
#define CSFB_OFFSET_Q_CMD				0x00815701  // cmd = 0x008, scope = 0x1, add = 0x57  Model/Cs = 01
#define CSFB_OFFSET_I_CMD				0x00815801  // cmd = 0x008, scope = 0x1, add = 0x58  Model/Cs = 01
#define CSFB_TRIGGATECTRL_ADDR			0x00815901	// Trigger Gate Control (TriggerEnable) register (CobraMax PCIe)

#define CSRB_DLA1_SETTING_ADDR			0x00816001
#define CSRB_DSA1_SETTING_ADDR			0x00816101
#define CSRB_DLB1_SETTING_ADDR			0x00816201
#define CSRB_DSB1_SETTING_ADDR			0x00816301
#define CSRB_DLA2_SETTING_ADDR			0x00816401
#define CSRB_DSA2_SETTING_ADDR			0x00816501
#define CSRB_DLB2_SETTING_ADDR			0x00816601
#define CSRB_DSB2_SETTING_ADDR			0x00816701
#define CSRB_STATUS_REG					0x00806801
#define CSRB_TEST_REG_WR_CMD			0x00814001	// cmd = 0x008, scope = 0x1, add = 0x40  Model/Cs = 01
#define CSRB_SELFTEST_MODE_WR_CMD		0x00814101	// cmd = 0x008, scope = 0x1, add = 0x41  Model/Cs = 01



#define CSRB_FWD_RD_CMD					0x00805E01
#define CSRB_FWV_RD_CMD					0x00805F01
#define CSRB_HWR_RD_CMD					0x00807D01
#define CSRB_SWR_RD_CMD					0x00807E01
#define CSRB_OPT_RD_CMD					0x00807F01

//AD7788
#define CSRB_REF_ADC_WR_CMD				0x008100a0  // cmd = 0x008, scope = 0x1, add = 0x00  Model/Cs = A0
#define CSRB_REF_ADC_RD_CMD				0x008000a0  // cmd = 0x008, scope = 0x0, add = 0x00  Model/Cs = A0


// AD7788 commands
#define CSRB_REF_ADC_CMD_RD		        0x40011
#define CSRB_REF_ADC_CMD_RESET	        0x1F


// Bits setting for clock Register 1  (CSRB_CLOCK_REG1	0x00810601)
#define CSRB_CLKR1_CLK_OUT_EN_BT  0x800
#define CSRB_CLKR1_PLLCLKINPUT_BT 0x40
#define CSRB_CLKR1_STDCLKINPUT_BT 0x20
#define CSRB_CLKR1_EXTCLKINPUT_BT 0x8
#define CSRB_CLKR1_MSCLKINPUT_BT  0x10
#define CSRB_CLKR1_XOINPUT_BT	  0x18
#define CSRB_CLKR1_RESET_BT		  0x80
// Bits setting for clock out Register 1 bur u16ClkOut variable
#define CSRB_ADC_CLK_OUT_BT		  0x0
#define CSRB_FPGA_CLK_OUT_BT	  0x1
#define CSRB_REF_CLK_OUT_BT		  0x2
#define CSRB_NO_CLK_OUT_BT		  0x3

#define CSRB_CLKR1_MASTERCLOCK_EN	0x1000

// Bits setting for clock out Register 1 for u16ClkOut variable
//with eco 10
#define CSRB_CLK_OUT_ADC	  0x0
#define CSRB_CLK_OUT_FPGA	  0x1
#define CSRB_CLK_OUT_REF	  0x2
#define CSRB_CLK_OUT_NONE	  0x3

#define CSRB_CLK_OUT_SA_ADC		  0xE00
#define CSRB_CLK_OUT_SA_FPGA	  0x401
#define CSRB_CLK_OUT_SA_REF		  0x402
#define CSRB_CLK_OUT_SA_NONE	  0x403

#define CSRB_CLK_OUT_MS_ADC		  0x800
#define CSRB_CLK_OUT_MS_FPGA	  0x801
#define CSRB_CLK_OUT_MS_REF		  0x802
#define CSRB_CLK_OUT_MS_NONE	  0x803
#define CSRB_CLKR1_CLK_OUT_CTL 	  0x400
#define CSRB_AD9514_OUT1_ONLY	  0xF

#define CSRB_AD9514_DIV_01		  0xF
#define CSRB_AD9514_DIV_02		  0xE
#define CSRB_AD9514_DIV_03		  0xD
#define CSRB_AD9514_DIV_04		  0xC
#define CSRB_AD9514_DIV_05		  0xB
#define CSRB_AD9514_DIV_06		  0xA
#define CSRB_AD9514_DIV_08		  0x9
#define CSRB_AD9514_DIV_09		  0x8
#define CSRB_AD9514_DIV_10		  0x7
#define CSRB_AD9514_DIV_12		  0x6
#define CSRB_AD9514_DIV_15		  0x5
#define CSRB_AD9514_DIV_16		  0x4
#define CSRB_AD9514_DIV_18		  0x3
#define CSRB_AD9514_DIV_24		  0x2
#define CSRB_AD9514_DIV_30		  0x1
#define CSRB_AD9514_DIV_32		  0x0

// CobraMax 24G8 PCIe ADC registers
#define CBMAX_ADC_REG_CFG        (0x1<<8)
#define CBMAX_ADC_REG_OFF1       (0x2<<8)
#define CBMAX_ADC_REG_GAIN1      (0x3<<8)
#define CBMAX_ADC_REG_ECM        (0x9<<8)
#define CBMAX_ADC_REG_OFF2       (0xA<<8)
#define CBMAX_ADC_REG_GAIN2      (0xB<<8)
#define CBMAX_ADC_REG_CLKP_FINE       (0xE<<8)
#define CBMAX_ADC_REG_CLKP_COARSE     (0xF<<8)

// CobraMax 14G8 PCIe 4G ADC registers
#define CBMAX_ADC_REG_CLKPHASE_FINE		(0xD<<8)
#define CBMAX_ADC_REG_CLKPHASE_COARSE	(0xE<<8)

#define CBMAX_ADC_CFG_MASK			  0xA0FF
#define CBMAX_ADC_REG_MASK			  0x00FF
#define CBMAX_ADC_OFF_MASK			  0x007F
#define CBMAX_ADC_GAIN_MASK			  0x007F
#define CBMAX_ADC_COARSE_MASK		  0x03FF


// Bits setting for clock Register 1  (CSFB_CLOCK_REG1	0x00810601)
#define CSFB_CLKR1_EXTCLKINPUT_BT		 0x8
#define CSFB_CLKR1_MSCLKINPUT_BT		0x10
#define CSFB_CLKR1_STDCLKINPUT_BT		0x20
#define CSFB_CLKR1_PLLCLKINPUT_BT		0x40
#define CSFB_CLKR1_RESET_BT				0x80
#define CSFB_CLKR1_MASTERCLOCK_EN		0x1000

// Bits setting for clock out Register 1 for u16ClkOut variable
// with eco 10
#define CSFB_CLK_OUT_FPGA	  0x01
#define CSFB_CLK_OUT_REF	  0x02
#define CSFB_CLK_OUT_NONE	  0x03

#define CSFB_CLKR1_CLKOUT_DISABLE	  0x400
#define CSFB_CLKR1_CLKOUT_ENABLE	  0xC00

// Nios command for Bunny cards
#define	CSFB_HWALIGN_EXTTRIG			0x03010000
#define	CSFB_HWALIGN_DPA				0x03310000


#define CSRB_NOT_INTERLEAVED_BT	  0x8

#define RB_DAC7552_OUTA			  0x0400
#define RB_DAC7552_OUTB			  0x0800

#define RB_ADC_DESENABLE_PINGPONG 0x8000
#define RB_ADC_DESENABLE_AUTO	  0x4000

#define RB_ADC_CFG_MASK			  0xA0FF
#define RB_ADC_REG_MASK			  0x00FF
#define RB_ADC_OFF_MASK			  0x007F
#define RB_ADC_DES_MASK			  0x3FFF
#define RB_ADC_COARSE_BITS_MASK	  0x4800
#define RB_ADC_COARSE_MASK		  0x07FF

#define RB_ADC_OFFSET_SIGN		  0x0080

#define RB_ADC_DESCOARSE_INPUTQ	  0x8000
#define RB_ADC_DESCOARSE_SIGN	  0x4000

#define RB_DEFAULT_ADC_CFG		  0xB7FF
#define COBRAMAX_DEFAULT_ADC_CFG  0xB2FF

// DAC 7552
#define RB_CAL_OFFSET_1			  0x20
#define RB_USER_OFFSET_CS		  0x21
#define RB_CAL_OFFSET_2			  0x22
#define RB_CAL_SRC				  0x23
#define	BR_CAL_DACEXTTRIG_NCS	  0x24
#define	RB_MS_DELAY				  0x25

#define RB_MS_DECSYNCH_RESET      0x1
#define RB_MS_DETECT              0x0080
#define RB_STATE_MACHINE_RESET    0x0200
#define RB_MS_RESET               0x4000

//CSRB_MS_BIST_CTL1_WR
#define CSRB_BIST_EN					0x0001
#define CSRB_BIST_DIR1					0x0002
#define CSRB_BIST_DIR2					0x0004
//CsRB_MS_BIST_CTL2_WR
//CSRB_MS_BIST_ST1_RD
#define CSRB_BIST_MS_SLAVE_ACK_MASK		0x007F
#define CSRB_BIST_MS_MULTIDROP_MASK		0x7F80
#define CSRB_BIST_MS_MULTIDROP_FIRST	0x0080
#define CSRB_BIST_MS_MULTIDROP_LAST		0x4000
//CSRB_MS_BIST_ST2_RD
#define CSRB_BIST_MS_FPGA_COUNT_MASK	0x003F
#define CSRB_BIST_MS_ADC_COUNT_MASK		0x3FC0
#define CSRB_BIST_MS_SYNC        		0x4000



#define CSRB_WRITE_TO_DAC7552_CMD 0x00810000	//----cmd = 0x008, scope = 0x1, add = 0xXX00


//ADC08D1000C1YB
#define CSRB_ADC_WR_CMD           0x008100e0  // cmd = 0x008, scope = 0x1, add = 0x00  Model/Cs = E0
#define CSRB_ADC_RD_CMD           0x008000e0  // cmd = 0x008, scope = 0x0, add = 0x00  Model/Cs = E0


//---- details of the command register
#define CSRB_ADDON_RESET		  0x1			//----- bit0
#define	CSRB_ADDON_FCE			  0x2			//----- bit1
#define CSRB_ADDON_FOE			  0x4			//----- bit2
#define CSRB_ADDON_FWE			  0x8			//----- bit3
#define CSRB_ADDON_CONFIG		  0x10		//----- bit4

#define CSRB_ADDON_FLASH_CMD	  0x10
#define CSRB_ADDON_FLASH_ADD	  0x11
#define CSRB_ADDON_FLASH_DATA	  0x12
#define CSRB_ADDON_FLASH_ADDR_PTR 0x13
#define CSRB_ADDON_STATUS		  0x14
#define CSRB_ADDON_FDV			  0x15
#define CSRB_ADDON_HIO_TEST		  0x20	
#define CSRB_ADDON_SPI_TEST_WR    0x21
#define CSRB_ADDON_SPI_TEST_RD    0x22
#define CSRB_AD7450_WRITE_REG	  0x88 
#define CSRB_AD7450_READ_REG	  0x89 

#define	CSRB_ADDON_STAT_FLASH_READY		0x1
#define	CSRB_ADDON_STAT_CONFIG_DONE		0x2
#define CSRB_TCPLD_INFO					0x4

#define CSRB_AD7450_RESTART       0x1
#define CSRB_AD7450_READ          0x2
#define CSRB_AD7450_DATA_READ     0x00010000
#define CSRB_AD7450_RD_FIFO_FULL  0x00020000
#define CSRB_AD7450_WR_FIFO_FULL  0x00040000
#define CSRB_AD7450_STATE         0x00380000
#define CSRB_AD7450_STATE_SHIFT   19
#define CSRB_AD7450_COUNT         0x1fc00000
#define CSRB_AD7450_COUNT_SHIFT   22

// Register configs
#define CSRB_LOCAL_SINGLEPULSE_ENABLE  	0x80	//	(Calibbration register)
#define CSRB_CAL_EXT_TRIG_SRC   	0x40	//	(Calibbration register)
#define	CSRB_MSPULSE_DISABLE		0x20	//	(Calibbration register)
#define	CSRB_LOCAL_PULSE_ENBLE		0x10	//	(Calibbration register)
#define	CSRB_PULSE_SIGNAL			0x08	//	(Calibbration register)
#define	CSRB_CAL_AC_GAIN			0x04	//	(Calibbration register)
#define	CSRB_CAL_POWER  			0x02	//	(Calibbration register)
#define	CSRB_CAL_RESET      		0x01	//	(Calibbration register)

#define CSRB_SET_AC_COUPLING	0x2
#define CSRB_SET_CHAN_CAL		0x1
#define CSRB_SET_CHAN_FILTER	0x20
#define CSRB_SET_CHAN_AMP   	0x40

#define CSRB_ADC_CAL_ACTIVATE	0x0002

#define CSRB_DIG_TRIGLEVEL_SPAN	0x100		// 8 bits

#define CSRB_ADC_REG_CFG        (0x1<<8)
#define CSRB_ADC_REG_OFF1       (0x2<<8)
#define CSRB_ADC_REG_GAIN1      (0x3<<8)
#define CSRB_ADC_REG_OFF2       (0xA<<8)
#define CSRB_ADC_REG_GAIN2      (0xB<<8)
#define CSRB_ADC_REG_DES        (0xD<<8)
#define CSRB_ADC_REG_COARSE     (0xE<<8)
#define CSRB_ADC_REG_FINE       (0xF<<8)

#define CSRB_ADC_DEFAULT_CFG     0xb7ff

//----- Self Test _mode
#define CSPDR_SELF_CAPTURE	0x0001
#define CSPDR_SELF_BIST		0x0003
#define CSPDR_SELF_COUNTER	0x0005
#define CSPDR_SELF_MISC		0x0007


// Addon Expert Histogram (Cobra PCI only)
#define	HISTO_READ_CTRL_REG		0x40
#define	HISTO_HIST_CTRL_REG		0x41
#define	HISTO_HIST_READ_REG		0x40
#define	HISTO_HIST_STAT_REG		0x41

#define	HIST_READ_ENABLE_BIT	(1<<12)
#define	HIST_ACLR_BIT			0x1
#define	HIST_RUN2OVER_BIT		0x4

typedef enum _PeakDetectState
{
	 PEAK_WAITFORDATAFROMFIFO = 0,
 	 PEAK_DATAFROMFIFO_AVAILABLE,
	 PEAK_DATATRANSFER_ARMED,
	 PEAK_RECEIVEDDATAFROMFIFO

} PEAKSTATE;


typedef enum _CalibFlag
{
	 CalibCLEAR = 0,
	 CalibSET,
	 CalibNoChange
} CALIBFLAG;

typedef enum 
{
	 formatDefault = 0,
	 formatHwAverage,
	 formatSoftwareAverage,
	 formatFIR
} _CSDATAFORMAT;
	
typedef	struct _RABBIT_FLASH_FOOTER
{
	CS_FLASHFOOTER		AddonFooter;

	uInt16				u16TrigOutCfg;		// Trig enable mode or trigout mode
	uInt16				u16TrigOutDisabled;	// Trigger out disabled
	eTrigEnableMode		TrigEnableCfg;		// Trigger enable config
	eClkOutMode			ClockOutCfg;		// Clock out config
} RABBIT_FLASH_FOOTER, *PRABBIT_FLASH_FOOTER;

typedef struct _NewRabitTrigAddrOffset
{
	uInt32		u32Decimation;
	int8		i8OffsetDual;
	int8		i8OffsetSingle;
} NEWRABBIT_TRIGADDR_OFFSET, *PNEWRABBIT_TRIGADDR_OFFSET;



class CsRabbitDev : public CsPlxBase,  public CsNiosApi
{
private:
	PGLOBAL_PERPROCESS		 m_ProcessGlblVars;

#if !defined(USE_SHARED_CARDSTATE)
	CSRABBIT_CARDSTATE		m_CardState;
#endif
	PCSRABBIT_CARDSTATE		m_pCardState;
	void UpdateCardState(BOOLEAN bReset = FALSE);
	uInt32					m_u32SystemInitFlag;	// Compuscope System initialization flag
	bool					m_bNucleon;				// Nucleon Base board


	uInt16  ClkDivToAD9514(uInt16 u16ClockDivision)
	{
		switch(u16ClockDivision)
		{
			default:
			case 1: return	CSRB_AD9514_DIV_01;
			case 2: return	CSRB_AD9514_DIV_02;
			case 3: return	CSRB_AD9514_DIV_03;
			case 4: return	CSRB_AD9514_DIV_04;
			case 5: return	CSRB_AD9514_DIV_05;
			case 6: return	CSRB_AD9514_DIV_06;
			case 8: return	CSRB_AD9514_DIV_08;
			case 9: return	CSRB_AD9514_DIV_09;
			case 10: return	CSRB_AD9514_DIV_10;
			case 12: return	CSRB_AD9514_DIV_12;
			case 15: return	CSRB_AD9514_DIV_15;
			case 16: return	CSRB_AD9514_DIV_16;
			case 18: return	CSRB_AD9514_DIV_18;
			case 24: return	CSRB_AD9514_DIV_24;
			case 30: return	CSRB_AD9514_DIV_30;
			case 32: return	CSRB_AD9514_DIV_32;
		}
	};

	static const uInt16 c_u16CalGainWnd;
	static const uInt32 c_u32MaxPositionOffset_mV;

	static const uInt32 c_u32OffsetCalibrationRange_mV;

	static const uInt32 c_u32MaxCalVoltage_mV;
	static const uInt32 c_u32MaxCalRange_mV;
	static const uInt32 c_u32MinCalRef_uV;
	static const uInt16 c_u16CalLevelSteps;
	static const uInt16 c_u16Vref_mV;
	static const uInt16 c_u16CalDacCount;
	static const uInt32 c_u32RefAdcCount;
	static const uInt16 c_u16CalAdcCount;

	static const uInt16 c_u16MaxIterations;
	
	static const uInt16 c_u16R8;
	static const uInt16 c_u16R10;
	static const uInt16 c_u16R19;
	static const uInt16 c_u16R26;
	static const uInt16 c_u16R28;
	static const uInt16 c_u16R165;
	static const uInt16 c_u16R171;
	static const uInt16 c_u16R212;
	static const uInt16 c_u16R220;

	static const uInt16 c_u16R20;
	static const uInt16 c_u16R22;
	static const uInt32 c_u32A_scaled;
	static const uInt32 c_u32B_scaled;

	static const uInt32 c_u32ScaleFactor;
	static const uInt32 c_u32K_scaled;
	static const uInt32 c_u32D_scaled;

	static const uInt32 c_u32R322;
	static const uInt32 c_u32R320;
	static const uInt32 c_u32MaxExtTrigDacLevel;
	static const uInt32 c_u32ExtTrigRange;



	const static FRONTEND_RABBIT_INFO c_FrontEndRabitGainDirectADC[];
	const static FRONTEND_RABBIT_INFO c_FrontEndRabitGain50[];
	const static RABBIT_SAMPLE_RATE_INFO	Rabit_2_2[];
	const static RABBIT_SAMPLE_RATE_INFO	Rabit_2_1[];
	const static RABBIT_SAMPLE_RATE_INFO	Rabit_1_1[];
	const static RABBIT_SAMPLE_RATE_INFO	CobraMax_2_4[];
	const static RABBIT_SAMPLE_RATE_INFO	CobraMax_1_4[];
	const static NEWRABBIT_TRIGADDR_OFFSET m_MsTrigAddrOffsetAdjust[];
	const static NEWRABBIT_TRIGADDR_OFFSET	m_NewTrigAddrOffsetTable[];

	const static uInt8 m_ExtClkTrigAddrOffsetTable[2][2];

	uInt8					m_u8TrigSensitivity;			// Digital Trigger sensitivity;
	int32					m_i32ExtTrigSensitivity;		// ExtTrigger Trigger sensitivity;
	bool					m_bSkipFirmwareCheck;			// Skip validation of firmware version

public:
	CsRabbitDev*		m_MasterIndependent;	// Master /Independant card
	CsRabbitDev*		m_Next;					// Next Slave
	CsRabbitDev*		m_HWONext;				// HW OBJECT next
	CsRabbitDev*		m_pNextInTheChain;		// use for M/S detection
	DRVHANDLE			m_PseudoHandle;			// User level card handle
	DRVHANDLE			m_CardHandle;			// Handle from Kernel Driver
	uInt8				m_u8CardIndex;			// Card index in native card detection order
	uInt16				m_u16NextSlave;			// Next Slave card index
	uInt16				m_DeviceId;
	bool				m_bCobraMaxPCIe;		

	PDRVSYSTEM			m_pSystem;		// Pointer to DRVSYSTEM
	CsRabbitDev*		m_pTriggerMaster;			// Point to card that has trigger engines enabled
	BOOLEAN				m_BusMasterSupport;			// HW support BusMaster transfer

	// Contructor
	CsRabbitDev( uInt8 u8Index, PCSDEVICE_INFO pDevInfo, PGLOBAL_PERPROCESS ProcessGlblVars);


protected:
	uInt8				m_DelayLoadImageId;			// Image Id to be load later
	uInt8				m_CalibTableId;				// Current active calibration table;
	int16				m_TrigAddrOffsetAdjust;			// Trigger address offset adjustment

	bool				m_bAutoClockAdjust;

	uInt32              m_u32CalFreq;
	eCalMode            m_CalMode;
	uInt8*	            m_pu8CalibBuffer;
	int32*              m_pi32CalibA2DBuffer;
	
	BUNNY_CAL_INFO		m_CalFileTable;			// Calibration Info Table from Cal file
	uInt16              m_u16VarRefCode;

	uInt32              m_u32CalibRange_uV;


	CSACQUISITIONCONFIG		m_OldAcqConfig;
	CSACQUISITIONCONFIG		m_ValidConfig;
	CS_TRIGGER_PARAMS		m_CfgTrigParams[2*CSRB_CHANNELS+1];
	CS_TRIGGER_PARAMS		m_OldTrigParams[2*CSRB_CHANNELS+1];
	CS_CHANNEL_PARAMS		m_OldChanParams[CSRB_CHANNELS];

	//----- Unit number for this device
	ULONG					m_Unit;
	uInt32					m_u32TailExtraData;			// Amount of extra samples to be added in configuration
														// in order to ensure post trigger depth
	uInt32                  m_u32MaxHwAvgDepth;
	BOOLEAN					m_bUseDefaultTrigAddrOffset;	// Skip TrigAddr Calib table
	bool					m_bOneSampleResolution;			// Enabled One sample resolution for Depth, pretrigger and Trigger delay
	bool					m_bAllowExtClkMs_pp;
	bool					m_bSkipMasterSlaveDetection;	// Skip Master/Slave detection
	bool					m_bZeroTrigAddress;				// Reading from beginning of memory
	bool					m_bZeroTrigAddrOffset;			// No Adjustment for Trigger Address Offset
	bool					m_bZeroDepthAdjust;				// No adjustment for Postriggger (ensure posttrigger depth)
	bool                    m_bZeroTriggerAddress;          // Hw Trigger address is always zero
	bool					m_bZeroExtTrigDataPathAdjust;	// No Adjustment for Ext Trigger Calib Data Path
	bool					m_bNeverValidateExtClk;
	bool					m_bSingleChannel2;				// Capture Single channel at channel 2
	bool					m_bUseI;						// Select ADC input I or Q	
	bool					m_bForceCal[CSRB_CHANNELS];		
	bool					m_bNoConfigReset;
   bool					m_bClearConfigBootLocation;

	uInt8					m_AddonImageId;					// Current active Addon FPGA image;
	uInt32					m_32BbFpgaInfoReqrd;			// Base board FPGA version required.
	uInt32					m_32AddonFpgaInfoReqrd;			// Add-on board FPGA version required.
	FILE_HANDLE				m_hCsiFile;						// Handle of the Csi file
	FILE_HANDLE				m_hFwlFile;						// Handle of the Fwl file
	bool					m_bFwlOld;						// Old version of Fwl file
	bool					m_bCsiOld;						// Old version of Csi file
	char					m_szCsiFileName[MAX_FILENAME_LENGTH];	// Csi file name
	char					m_szFwlFileName[MAX_FILENAME_LENGTH];	// fwl file name
	char					m_szCalibFileName[MAX_FILENAME_LENGTH];	// Calibration params file name
	uInt32                  m_u32IgnoreCalibErrorMask;
	uInt32                  m_u32IgnoreCalibErrorMaskLocal;
	uInt32					m_u32SegmentResolution;
	int32					m_i32OneSampleResAdjust;

	CSI_ENTRY				m_BaseBoardCsiEntry;
	CSI_ENTRY				m_AddonCsiEntry;
	FPGA_HEADER_INFO		m_BaseBoardExpertEntry[2];		// Active Xpert on base board
	FPGA_HEADER_INFO		m_AddonBoardExpertEntry[2];		// Active Xpert on addon board board
	BOOLEAN					m_bInvalidHwConfiguration;			// The card has wrong limitation table
	uInt32					m_u32DefaultExtClkSkip;
			
	uInt32					m_u32HwSegmentCount;
	int64					m_i64HwSegmentSize;
	int64					m_i64HwPostDepth;
	int64					m_i64HwTriggerDelay;
	int64					m_i64HwTriggerHoldoff;
	uInt32					m_32SegmentTailBytes;
	uInt32					m_u32CsiTarget;
	
	BOOLEAN					m_bPulseCalib;				// Single Pulse for cabration present
	uInt8                   m_u8CalDacSettleDelay;
	uInt32					m_u32ChanPosCross;		// Ext Trig Calib, Channel trigger
	uInt32					m_u32PosCross;			// Ext Trig Calib	xt Trigger

	STREAMSTRUCT			m_Stream;						// Stream state
	CS_TRIG_OUT_CONFIG_PARAMS	m_TrigOutCfg;

	//----- Variable related to Data Transfer
	CHANNEL_DATA_TRANSFER	m_DataTransfer;

	BOOLEAN				m_IntPresent;

	BOOLEAN				m_UseIntrTrigger;		
	BOOLEAN				m_UseIntrBusy;			

	PeevAdaptor			m_PeevAdaptor;				// Peev Adaptor
	BOOLEAN				m_bSoftwareAveraging;		// Mode Software Averaging enabled
	BOOLEAN				m_bHardwareAveraging;		// Mode Harware Averaging enabled
	bool				m_bMulrecAvgTD;				// Mode Expert Multiple Record AVG TD
	uInt32				m_u32MulrecAvgCount;		// Mulrec AVG count (should be <= 1024)
	BOOLEAN				m_bExpertAvgFullRes;		// Mode Harware Averaging in full resolution (32bits)
	bool				m_bHwAvg32;					// Force AVG running in 32 bit full resolution (For Debug purpose)
	int16				*m_pAverageBuffer;			// Temporarary buffer to keep the result of avearging
	BOOLEAN				m_bFlashReset;				// Reset flash is needed before any operation on Flash
	bool				m_bHistogram;				// Mode Expert Histogram enabled
	uInt32				m_u32HistogramMemDepth;		// Histogram segment size in memory locations


	// Variables for Master / Slave calibration
	BOOLEAN					m_bMsDecimationReset;		// Decimation reset for Master/Slave is required
	uInt8					m_u8TrigCardIndex;			// Index of the M/S card that has the trigger event
	int16					m_i16MsAlignOffset;
	int16					m_i16ZeroCrossOffset;
	BOOLEAN					m_bMsOffsetAdjust;
	BOOLEAN					m_bPulseTransfer;
	uInt16					m_u16DebugMsPulse;
	uInt16					m_u16DebugExtTrigCalib;
	BOOLEAN					m_bNeverCalibrateMs;
	BOOLEAN					m_bNeverCalibExtTrig;		// No Calibration for External required
	uInt8					m_u8DebugCalibExtTrig;		// Debug the Calibration for External required
	BOOLEAN					m_bNoAdjExtTrigFail;		// No Adjustment for Ext Trig if Calib Fails
#ifdef DEBUG_MASTERSLAVE
	uInt8					m_PulseData[PAGE_SIZE];
#endif

#ifdef DEBUG_EXTTRIGCALIB
	uInt8*					m_pu8DebugBuffer1[PAGE_SIZE];
	uInt8*					m_pu8DebugBufferEx[PAGE_SIZE];
#endif

	eCalMode				m_CalibMode;

	uInt16			m_u16SkipCalibDelay; //in minutes;
	bool			m_bNeverCalibrateAc;
	bool			m_bNeverCalibrateDc;
	bool			m_bSkipCalib;
	bool			m_bDebugCalibSrc;
	bool            m_bErrorOnOffsetSaturation[CSRB_CHANNELS];
	bool			m_bFastCalibSettingsDone[CSRB_CHANNELS];
	bool			m_bPreCalib;
	bool            m_bOnlyOnePulseLoad;
	bool			m_bCalibActive;					// In calibration mode
	bool			m_bLowExtTrigCalib;
	bool			m_bSonoscan_LimitedRanges;		// Limited Input ranges for sonoscan
	
	int32			m_i32MsCalibTrigLevel;
	uInt32			m_u32DebugCalibSrc;
	uInt32			m_u32DebugCalibTrace;
	uInt32			m_u32IgnoreCalibError;

	uInt32			m_u32CalibRange_mV;
	uInt32			m_u32CalibSwing_uV;
	uInt32			m_u32VarRefLevel_uV;
	int32           m_i32CalAdcOffset_uV;
	uInt16          m_u16CalCodeB;


	//----- CsRabbitDev.cpp
	int32  MsCsResetDefaultParams( BOOLEAN bReset = FALSE );
	int32  CsSetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg = NULL );
	int32  CsSetTriggerConfig( PARRAY_TRIGGERCONFIG pATrigCfg = NULL );
	int32  CsSetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg = NULL );

	int32  RemoveDeviceFromUnCfgList();
	void   AddDeviceToInvalidCfgList();
	
	bool   SetTriggerMaster(const int nIndex);
	uInt16 BuildIndependantSystem( CsRabbitDev** FirstCard, BOOLEAN bForceInd );
	uInt16 BuildMasterSlaveSystem(CsRabbitDev** FirstCard );
	uInt16 DetectAcquisitionSystems(BOOL bForceInd); 

	void	TraceCalib(uInt32 u32Id , uInt32 u32V1, uInt32 u32V2, int32 i32V3 , int32 i32V4);
	
	int32	CsGetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg );
	int32   CsGetTriggerConfig( PARRAY_TRIGGERCONFIG pATriggerCfg );
	int32	CsGetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg );
	
	int32  GetFreeTriggerEngine( int32	i32TrigSource, uInt16 *u16TriggerEngine );
	uInt16 NormalizedChannelIndex( uInt16 u16ChannelIndex );
	int32  NormalizedTriggerSource( int32 i32TriggerSource );

	int32 SendClockSetting(uInt32 u32Mode, LONGLONG llSampleRate = CSRB_DEFAULT_SAMPLE_RATE, uInt32 u32ExtClk = 0, BOOLEAN bRefClk = FALSE, uInt32 u32ExtClkSkip = CS_SAMPLESKIP_FACTOR);
	int32 CobraMaxSendClockSetting(uInt32 u32Mode, LONGLONG llSampleRate = CSRB_DEFAULT_SAMPLE_RATE, BOOLEAN bRefClk = FALSE);
	void  SetDefaultTrigAddrOffset(void);
	int32 SendCaptureModeSetting(uInt32 u32Mode = CS_MODE_DUAL, uInt16 u16HwChannelSingle = CS_CHAN_2);
	int32 SendCalibModeSetting(uInt16 Channel, eCalMode ecMode = ecmCalModeNormal, uInt32 u32WaitingTime = 100 );
	int32 SendChannelSetting(uInt16 Channel, uInt32 u32InputRange = CSRB_DEFAULT_INPUTRANGE, uInt32 u32Term = CSRB_DEFAULT_TERM, uInt32 u32Impendance = CSRB_DEFAULT_IMPEDANCE, int32 i32Position = 0, uInt32 u32Filter = 0, int32 i32ForceCalib = 0);
	int32 SendTriggerEngineSetting(int32	i32SourceA1,
											uInt32  u32ConditionA1 = CSRB_DEFAULT_CONDITION,
											int32   i32LevelA1 = 0,
                                            int32	i32SourceB1 = 0,
											uInt32  u32ConditionB1 = 0,
											int32   i32LevelB1 = 0, 
											int32	i32SourceA2 = 0,
											uInt32  u32ConditionA2 = 0,
											int32   i32LevelA2 = 0,
                                            int32	i32SourceB2 = 0,
											uInt32  u32ConditionB2 = 0,
											int32   i32LevelB2 = 0 );
											
	int32 SendExtTriggerSetting( BOOLEAN  bExtTrigEnabled = FALSE,
										   int32	i32ExtLevel = 0,
										   uInt32	u32ExtCondition = CSRB_DEFAULT_CONDITION,
										   uInt32	u32ExtTriggerRange =  CSRB_DEFAULT_EXT_GAIN, 
										   uInt32	u32ExtTriggerImped =  CSRB_DEFAULT_EXT_IMPEDANCE, 
										   uInt32	u32ExtCoupling = CSRB_DEFAULT_EXT_COUPLING,
										   int32	i32Sensitivity = CSRB_DEFAULT_EXT_TRIG_SENSE );
	
	int32 SendDigitalTriggerSensitivity( uInt16  u16TrigEngine, uInt8 u8Sensitive = CSRB_DEFAULT_TRIG_SENSITIVITY );
	int32 SendExtTriggerLevel( int32 i32Level, bool bRising, int32 i32Sensitivity = 0 );

	int32 SendTriggerRelation( uInt32 u32Relation	);
	int32 WriteToSelfTestRegister( uInt16 SelfTestMode = CSST_DISABLE );
	int32 InitAddOnRegisters(bool bForce = false);


	int32	WriteToDac(uInt16 u16Addr, uInt16 u16Data, uInt8 u8Delay = 0, BOOLEAN bUpdateDacInfo = FALSE );

	void	AssignBoardType();
	void	AssignAcqSystemBoardName( PCSSYSTEMINFO pSysInfo );
	int32	AcqSystemInitialize();
	int8	GetExtClkTriggerAddressOffsetAdjust();
	void	SetExtClkTriggerAddressOffsetAdjust( int8 u8TrigAddressOffset );
	int8	GetTriggerAddressOffsetAdjust( uInt32 u32SampleRate );
	void	SetTriggerAddressOffsetAdjust( int8 u8TrigAddressOffset );
	bool	CheckRequiredFirmwareVersion();
	int32	CheckMasterSlaveFirmwareVersion();
	void	SetDataMaskModeTransfer ( uInt32 u32TxMode );
	int32	CalibrateAllChannels(void);

	int32	GetFEIndex(uInt16 u16Channel);
	void	FillOutBoardCaps(PCSXYG8BOARDCAPS pCapsInfo);

	int32	WriteCalibTableToEeprom(uInt8	u8CalibTableId, uInt32 u32Valid );
	int32	ReadCalibTableFromEeprom(uInt8 u8CalibTableId = 1);

	int32	CsDetectZeroCrossAddress_PCI();

	int32	SetTrigOut( PCS_TRIG_OUT_STRUCT	pTrigOut = NULL );
	int32	SetClockOut( eRoleMode eMode, eClkOutMode ecomSet = eclkoNone );
	int32	SetRoleMode( eRoleMode ermSet );

	uInt16	GetValidTrigOutMode(void);
	uInt16	GetValidTrigOutValue(uInt16 u16Mode);
	uInt16	GetTrigOutMode(void) {return m_pCardState->u16TrigOutMode; };
	uInt16	GetTrigOut(void);
	eClkOutMode GetClockOut(void){return m_pCardState->eclkoClockOut;}
	eTrigEnableMode GetExtTrigEnable(void){return m_pCardState->eteTrigEnable;}
	
	int32	SendSegmentSetting(uInt32 u32SegmentCount, int64 i64PostDepth, int64 i64SegmentSize, int64 i64HoldOff, int64 i64TrigDelay = 0);

	int32 WriteToAdcReg(uInt16 u16Addr, uInt16 u16Data);
	int32 SetAdcOffset(uInt16 u16Chan, int16 i16Offset = 0);
	int32 SetAdcGain(uInt16 u16Chan, uInt16 u16Gain = BUNNY_ADC_REG_SPAN/2);
	int32 SetAdcDelay(int16 i16Coarse = 0, int16 i16Fine = 0);
	int32 SetAdcMode(bool bUseI = true, bool bPingPong = false, bool bAuto = true);
	int32 SetDataPath( uInt32 u16DataSelect = 0 );
	int32 CheckSinglePulseForCalib();
	bool ForceFastCalibrationSettingsAllChannels();

	bool IsAddonInit();
	void ReadVersionInfo();
	void ReadVersionInfoEx();

	uInt32	AddonReadFlashByte(uInt32 Addr);
	void	AddonWriteFlashByte(uInt32 Addr, uInt32 Data);
	int32	DoAddonWriteFlash( uInt32 u32Sector, uInt32 u32Offset, PVOID pBuffer, uInt32 u32WriteSize );

	uInt32	AddonResetFlash(void);
	int32	AddonConfigReset(uInt32 Addr = 0);
	uInt16	GetTriggerLevelOffset( uInt16	u16TrigEngine );
	void	SetTriggerLevelOffset( uInt16 u16TriggerEngine, uInt16 u16TrigLevelOffset );
	void	SetTriggerGainAdjust( uInt16 u16TrigEngine, uInt16 u16TrigGain );
	uInt16	GetTriggerGainAdjust( uInt16	u16TrigEngine );

	int32	WriteEntireEeprom(AddOnFlash32Mbit *Eeprom);
	int32   WriteSectorsOfEeprom(void *pData, uInt32 u32StartAddr, uInt32 u32Size);
	int32	WriteEepromEx(void *pData, uInt32 u32StartAddr, uInt32 u32Size, FlashAccess flSequence = AccessAuto);
	int32	ReadEeprom(void *Eeprom, uInt32 Addr, uInt32 Size);
	int32	ReadAddOnHardwareInfoFromEeprom(BOOLEAN bChecksum = TRUE);
	int32	ReadTriggerTimeStampEtb( uInt32 StartSegment, int64 *pBuffer, uInt32 u32NumOfSeg );

	int32	SetupBufferForCalibration( bool bSetup, eCalMode ecMode );
	int32	SetupForCalibration( bool bSetup, eCalMode ecMode = ecmCalModeDc );

	int32	CsGetAverage( uInt16 u16Channel, uInt8* pu8Buffer, uInt32 u32BufferSize, int16* pi16OddAvg_x10, int16* pi16EvenAvg_x10 );
	void	SetupHwAverageMode( BOOLEAN bFullRes = TRUE, uInt32 u32ScaleFactor = 1 );
	int32	ProcessSoftwareAveraging( int64 i64StartAddress, uInt16 u16Channel, int32 *pi32ResultBuffer, uInt32 u32SamplesInBuffer );
#ifdef _WINDOWS_
	int32	ReadCommonRegistryOnBoardInit();
	int32	ReadCommonRegistryOnAcqSystemInit();
	void	ReadCalibrationSettings(void *Params);
#else
   int32 ReadCommonCfgOnBoardInit();   
	int32	ReadCommonCfgOnAcqSystemInit();
	void	ReadCalibrationSettings(char *szIniFile, void *Params);
   int32 ShowCfgKeyInfo();
#endif   
	int32	ReadAndValidateCsiFile( uInt32 u32BbHwOpt, uInt32 u32AoHwOpt );
	int32	ReadAndValidateFwlFile();
	void	CsSetDataFormat( _CSDATAFORMAT csFormat = formatDefault );
	void	AdjustedHwSegmentParameters( uInt32 u32Mode, uInt32 *u32SegmentCount, int64 *i64SegmentSize, int64 *i64Depth );
	void	AdjustForOneSampleResolution( uInt32 u32Mode, int64 *i64SegmentSize, int64 *i64Depth, int64 *i64TrigDelay );

	uInt8 GetFlashManufactoryCode();
	void  RabbitInitFlash(uInt32 FlashIndex);
	void  RabbitFlashCommand(uInt32 Command, uInt32 Sector, uInt32 Offset, uInt32 Data);
	int32 RabbitFlashWrite( uInt32 Sector, uInt32 Offset, uInt8 *Buf, uInt32 nBytes );
    uInt32 RabbitFlashStatus();
	void  RabbitFlashReset();
	void  RabbitFlashWriteByte(  uInt32 Addr, uInt32 Data );
	void  RabbitFlashSectorErase( uInt32 Sector);
	void  RabbitFlashChipErase();

	int32	CommitNewConfiguration( PCSACQUISITIONCONFIG pAcqCfg,
									PARRAY_CHANNELCONFIG pAChanCfg,
									PARRAY_TRIGGERCONFIG pATrigCfg );
	void	DetectMasterSlave(int16 *i16NumMasterSlaveSystems, int16 *i16NumStandAloneSystems);
	int32	SendMasterSlaveSetting();
	int32	ResetMasterSlave();
	int32	ResetMSDecimationSynch();
	int32	EnableMsBist(bool bEnable);
	int32	TestMsBridge();

	int32	DetectSlaves();
	uInt32	CsGetPostTriggerExtraSamples( uInt32 u32Mode );
	int32	CalibrateMasterSlave();
	uInt32	GetSegmentPadding(uInt32 u32Mode);
	void	ResetCachedState(void);

	int32	AcquireCalibSignal( uInt16 u16UserChannel, int32 i32StartPos, int32 *i32OffsetAdjust );
	int32	GeneratePulseCalib();
	int32	CalibrateExtTrigger();
	void	RestoreCalibDacInfo();
	int32	SetupForExtTrigCalib_SourceExtTrig();
	int32	SetupForExtTrigCalib_SourceChan1();
	int32	SetupForMsCalib_SourceChan1();
	int32	SetupForSinglePulseCalib( bool bSetup );
	void	SetDefaultAuxIoCfg();

public:
	CsRabbitDev *GetCardPointerFromChannelIndex( uInt16 ChannelIndex );
	CsRabbitDev *GetCardPointerFromBoardIndex( uInt16 BoardIndex );
	CsRabbitDev *GetCardPointerFromTriggerIndex( uInt16 TriggerIndex );
	CsRabbitDev *GetCardPointerFromTriggerSource( int32 i32TriggerSoure );

	int32   CsGetBoardsInfo( PARRAY_BOARDINFO pABoardInfo );
	int32	DeviceInitialize(void);
	int32	CsAutoUpdateFirmware();
	int32	CombineCardUpdateFirmware();

	int32  AcquireData(uInt32 u32IntrEnabled = 0);
	void   MsAbort();
	int32  WriteToCalibDac(uInt16 Addr, uInt16 u32Data, uInt8 u8Delay = 0);

	uInt32	GetDataMaskModeTransfer ( uInt32 u32TxMode );
	uInt32	GetFifoModeTransfer( uInt16 u16UserChannel );

	//----- Read/Write the Add-On Board Flash 
	BOOLEAN	IsAddonBoardPresent();
	int32	ReadCalibInfoFromCalFile();
	int32	CalibrateFrontEnd(uInt16 u16Channel);

	LONGLONG GetMaxExtClockRate( uInt32 u32Mode = CS_MODE_SINGLE );
	LONGLONG GetMaxSampleRate( BOOLEAN bPingpong );
	uInt16	ConvertToHwChannel( uInt16 u16UserChannel );
	uInt16	ConvertToUserChannel( uInt16 u16HwChannel );
	uInt16	ConvertToMsUserChannel( uInt16 u16UserChannel );
	BOOLEAN	IsCalibrationRequired();
	int32	SendPosition(uInt16 u16Channel, int32 i32Position_mV);

public:
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
	int32	ProcessDataTransferModeAveraging(in_DATA_TRANSFER *InDataXfer);
	int32	DataTransferModeHistogram(in_DATA_TRANSFER *InDataXfer);

	int32	CsValidateAcquisitionConfig( PCSACQUISITIONCONFIG, uInt32 u32Coerce );
	void	UpdateSystemInfo( CSSYSTEMINFO *SysInfo );
	int32	SetPosition(uInt16 u16Channel, int32 i32Position_mV);
	int32	FindFeIndex( const uInt32 u32Swing, uInt32& u32Index );

	int32	AcquireAndAverage(uInt16 u16Channel, int16* pi16Odd, int16* pi16Even );
	int32	CalibrateOffset(uInt16 u16Channel);
	int32	CalibrateGain(uInt16 u16Channel);
	int32	CalibratePosition(uInt16 u16Channel);
	int32	SetCalibrationRange(uInt32 u32Range_mV);
	int32   ReadCalibA2D(int32* pi32Mean, int32* pi32MeanSquare = NULL );
	int32   SendCalibDC(int32 i32Target_mV, bool bTerm, int32* pVoltage_uV = NULL, uInt16* pu16InitialCode = NULL );
	int32	SendDigitalTriggerLevel( uInt16  u16TrigEngine, int32 i32Level );
	
	int32	InitHwTables();
	BOOLEAN	FindSrInfo( const LONGLONG llSr, PRABBIT_SAMPLE_RATE_INFO *pSrInfo );

	void	SetFastCalibration(uInt16 u16Channel);
	int32	FindSRIndex( LONGLONG llSampleRate, uInt32 *u32IndexSR );
	void	InitBoardCaps();

	int32	CsDetectLevelCrossAddress( void	*pBuffer, uInt32 u32BufferSize, uInt32 *u32PosCrosss, uInt8 u8CrossSlope = SINGLEPULSECALIB_SLOPE);
	int32	NucleonDetectZeroCrossAddress( void* pBuffer, uInt32 u32BufferSize, uInt8	u8CrossLevel, uInt32* pu32PosCross );
	int32	AdjustOffsetChannelAndExtTrigger();
	int32	CobraMaxSonoscanCalibOnStartup();


	bool    FastCalibrationSettings(uInt16 u16Channel, bool bForce = false);
	int32	SendAdcSelfCal(void);

	int32	InitializeExtTrigEngine( BOOLEAN bRising );

	uInt32	BbFwVersionConvert( uInt32 u32OldVersion );
	uInt32	AddonFwVersionConvert( uInt32 u32OldVersion );


	EVENT_HANDLE	GetEventTxDone() { return m_DataTransfer.hAsynEvent; }
	EVENT_HANDLE	GetEventSystemCleanup() { return m_pSystem->hAcqSystemCleanup; }
	EVENT_HANDLE	GetEventTriggered() { return m_pSystem->hEventTriggered; }
	EVENT_HANDLE	GetEventEndOfBusy() { return m_pSystem->hEventEndOfBusy; }
	EVENT_HANDLE	GetEventAlarm() { return 0; }
	void			OnAlarmEvent() {};

	void	SetStateTriggered() { m_PlxData->CsBoard.bTriggered = true; }
	void	SetStateAcqDone() { m_PlxData->CsBoard.bBusy = false; }
	void	SignalEvents( BOOL bAquisitionEvent = TRUE );
	

	int32	ValidateCsiTarget(uInt32 u32Target);
	int32	CsLoadBaseBoardFpgaImage( uInt8 u8ImageId );
	int32	SetClockThruNios( uInt32 u32ClockMhz );
	int32	ConfigureFe( const uInt16 u16Channel, const uInt32 u32Index, const uInt32 u32Imed, const uInt32 u32Unrestricted);
	int32	SetupAcquireAverage(bool bSetup = false);
	int32	UpdateCalibInfoStrucure();	

    void *operator new(size_t  size);//implicitly declared as a static member function

	int32	CreateThreadAsynTransfer();
	void	AsynDataTransferDone();

#ifdef __linux__
	void 	CreateInterruptEventThread();
	void	InitEventHandlers();
	void	ThreadFuncIntr();

	// DEBUG   DEBUG  DEBUG   DEBUG
	int32	DebugPrf( uInt32 DebugOption ){};
#endif


	// CsExpert
	BOOLEAN	IsExpertAVG() {return ( m_bHardwareAveraging || m_bMulrecAvgTD );}

	FlashInfo	*FlashPtr;
	int32	WriteToCalibCtrl(eBunnyCalCtrl eccId, uInt16 u16Channel, int16 i16Data, uInt8 u8Delay = 0);

	void	UpdateCalibInfoStructure(uInt16 u16Addr, uInt16 u16Data);
	int16	GetDataPathOffsetAdjustment();
	int32	SendTriggerEnableSetting( eTrigEnableMode ExtTrigEn = eteAlways );
	int32	ValidateExtClockFrequency( uInt32 u32Mode, uInt32 u32ClkFreq_Hz, uInt32 u32ExtClkSkip );
	int32   VerifyTriggerOffset();
	int32   CalibTriggerOffset();
	bool	VerifyTriggerOffsetNeeded();
	bool	IsExtTriggerEnable( PARRAY_TRIGGERCONFIG pATrigCfg );
	int32	CalculateExtTriggerSkew();

	bool	IsImageValid( uInt8 u8Index );
	int32	NucleonWriteFlash(uInt32 Addr, void *pBuffer, uInt32 WriteSize, bool bBackup = FALSE );
	int32	IoctlGetPciExLnkInfo( PPCIeLINK_INFO pPciExInfo );

	// Overload functions to support Nulceon
	int32	CsReadPlxNvRAM( PPLX_NVRAM_IMAGE nvRAM_image );
	int32	CsWritePlxNvRam( PPLX_NVRAM_IMAGE nvRAM_image );
	BOOLEAN ConfigurationReset( uInt8	u8UserImageId );
	int32	BaseBoardConfigurationReset(uInt8	u8UserImageId);

	int32	CsNucleonCardUpdateFirmware();
	int32	UpdateAddonFirmware( FILE_HANDLE hCsiFile, uInt8 *pBuffer );
	int32	ReadBaseBoardHardwareInfoFromFlash( BOOLEAN bChecksum = TRUE );
	BOOL	IsNiosInit(void);
	int64	GetTimeStampFrequency();

	int32 	CsStmTransferToBuffer( PVOID pVa, uInt32 u32TransferSize );
	int32	CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *u32ErrorFlag, uInt32 *u32ActualSize, uInt8 *u8EndOfData );
	int32	CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *u32ErrorFlag );
	void 	MsStmResetParams();
	int32	MsCalibrateSkew(bool bExtTrigEn);
	int32	CalculateMasterSlaveSkew();
	int32	DetectMsZeroCrossAddress();
	int32	SetDigitalDelay( int16 i16Offset );
	int32	MsAdjustDigitalDelay();
	bool	IsConfigResetRequired( uInt8 u8ImageId );

	int32	MsInitializeDevIoCtl();
	bool	IsExpertAVG(uInt8 u8ImageId);
	bool	IsExpertSTREAM(uInt8 u8ImageId = 0);
	void	PreCalculateStreamDataSize( PCSACQUISITIONCONFIG pCsAcqCfg );
	void	NucleonGetMemorySize();
	uInt8	ReadBaseBoardCpldVersion();
	uInt32	ReadConfigBootLocation();
	int32	WriteConfigBootLocation( uInt32	u32Location );

#ifdef USE_SHARED_CARDSTATE
	int32	InitializeDevIoCtl();
#endif

	int32	CobraMaxSendDpaAlignment( bool bPingpong );
	int32	CobraMaxSetClockOut( eRoleMode eMode, eClkOutMode ecomSet = eclkoNone );
	void	HistogramReset();
	bool	IsHistogramEnabled();
	void	MsHistogramActivate( bool bEnable );
	uInt16	ReadBusyStatus();
	int32	CalibrateCoreOffset();
	void	SetCalibBit( bool EnableCalMode, uInt32 *pu32Val );

	int32	CsCheckPowerState()
	{	if ( 0 == m_pTriggerMaster->m_pCardState->bCardStateValid )
			return CS_POWERSTATE_ERROR;
		else
			return CS_SUCCESS;
	}

	int32	SendInternalHwExtTrigAlign();

};


// Prototype for functions
unsigned int WINAPI ThreadFuncIntr( LPVOID lpParam );
unsigned int WINAPI ThreadFuncTxfer( LPVOID lpParam );

#endif
