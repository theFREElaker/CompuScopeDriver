//! CsSpider.h
//!
//!

#ifndef __CsSpider_h__
#define __CsSpider_h__

#include "CsDefines.h"
#include "CsTypes.h"
#include "CsPlxDefines.h"
#include "CsDeviceIDs.h"
#include "CsAdvStructs.h"
#include "CsiHeader.h"
#include "CsFirmwareDefs.h"
#include "Cs8xxxCapsInfo.h"
#include "CsSpiderCal.h"
#include "CsSpiderFlash.h"

#include "CsPlxBase.h"
#include "CsNiosApi.h"

#include "CsDrvStructs.h"
#include "DrvCommonType.h"
#include "DllStruct.h"
#include "GageWrapper.h"
#include "CsSharedMemory.h"

#include "CsFirmwareDefs.h"
#include "PeevAdaptor.h"
#include "CsSpiderOptions.h"
#include "CsSpiderState.h"
#include "CsNucleonBaseFlash.h"

#ifdef _WINDOWS_
#define USE_SHARED_CARDSTATE
#endif

#define TRACE_CAL_HEADER       0x0001 //Trace general info
#define TRACE_CAL_COARSE_OFF   0x0002 //Coarse offset setting
#define TRACE_CAL_FINE_OFF     0x0004 //Fine offset setting
#define TRACE_CAL_GAIN         0x0008 //Gain offset settings
#define TRACE_CAL_POS          0x0010 //Position calibration settings
#define TRACE_CAL_SIGNAL       0x0020 //Trace final result of calibration signal DAC
#define TRACE_DAC			   0x0040 //Trace acces to All calibration DACs
#define TRACE_CAL_RESULT       0x0080 //Trace result of failed calibration if we go on
#define TRACE_CAL_SIGNAL_ITER  0x0100 //Trace itteration of the calibration signal DAC
#define TRACE_ZERO_OFFSET      0x0200 //Trace of the measured front end offset
#define TRACE_CAL_RESERVED     0x0400 //Reserved
#define TRACE_CAL_NOISE        0x0800 //Trace noise level of calibration line
#define TRACE_CAL_PROGRESS     0x1000 //Trace progress of the cal process




#define SPIDER_SHARENAME			_T("CsSpider")

#define	SHARED_CAPS_NAME			_T("Cs8xxxWDFCaps")
#define MEMORY_MAPPED_SECTION		_T("Cs8xxxWDF-shared") // CR-2621

#define		PAGE_SIZE		4096

#define	MAX_SPDR_TRIGGER_ADDRESS_OFFSET		24		// Should be muliple of 8
#define	MAX_SPDR_TAIL_INVALID_SAMPLES		(16*8)	// Invalid sample at tail in Single mode (should be multiple of 2*8)
													// Half in Dual, 1/4 in Quad and 1/8 in Oct

#define CSPDR_AC_COUPLING_INDEX    0x01
#define CSPDR_FILTER_INDEX         0x02
#define CSPDR_IMPEDANCE_50_INDEX   0x04


#define CSPDR_DAC_SPAN			2048

// Trigger Engine Index in Table
#define	CSPDR_TRIGENGINE_EXT	0
#define	CSPDR_TRIGENGINE_1A		1
#define	CSPDR_TRIGENGINE_1B		2
#define	CSPDR_TRIGENGINE_2A		3
#define	CSPDR_TRIGENGINE_2B		4
#define	CSPDR_TRIGENGINE_3A		5
#define	CSPDR_TRIGENGINE_3B		6
#define	CSPDR_TRIGENGINE_4A		7
#define	CSPDR_TRIGENGINE_4B		8
#define	CSPDR_TRIGENGINE_5A		9
#define	CSPDR_TRIGENGINE_5B		10
#define	CSPDR_TRIGENGINE_6A		11
#define	CSPDR_TRIGENGINE_6B		12
#define	CSPDR_TRIGENGINE_7A		13
#define	CSPDR_TRIGENGINE_7B		14
#define	CSPDR_TRIGENGINE_8A		15
#define	CSPDR_TRIGENGINE_8B		16
#define	CSPDR_TOTAL_TRIGENGINES	17


#define	MAX_FILENAME_LENGTH				128

// Limitation of hardware averaging
#define	AV_INVALIDSAMPLES_HEAD			28				// Limitation of FW
#define	AV_INVALIDSAMPLES_TAIL			8				// Limitation of FW


#define	AV_ENSURE_POSTTRIG				128				// (Max TrigAddrOffset + AV_INVALIDSAMPLES_HEAD
														// + AV_INVALIDSAMPLES_TAIL)
														// The value for normal normal spider cards is 64
														// for ST81AVG cards the value is 128 (bugid 2455)


#define MAX_AVERAGING_CUTOFF			128				// Limation of FW 64 +
														// 64 (Max TrigAddrOffset + AV_INVALIDSAMPLES_HEAD
														// + AV_INVALIDSAMPLES_TAIL)

// Max trigger depth in HW Averaging mode (in samples)
#define SPDR_NUCLEON_MAX_MEM_HWAVG_DEPTH	(0x10000-128)
#define SPDR_PCI_MAX_MEM_HWAVG_DEPTH		(0xC000-AV_ENSURE_POSTTRIG)	

#define SPDR_MAX_AVERAGING_PRETRIGDEPTH		0				// Max pre trigger depth in HW Averaging mode (in samples)
#define	SPDR_MAX_HW_AVERAGE_SEGMENTCOUNT	1024			// Max segment count in HW Averaging mode


// Limitation of sofware averaging
#define MAX_SW_AVERAGING_SEMGENTSIZE	512*1024		// Max segment size in Software averaging mode (in samples)
#define	SW_AVERAGING_BUFFER_PADDING		16				// Padding for Sw Avearge buffer
#define MAX_SW_AVERAGE_SEGMENTCOUNT		4096			// Max segment count in SW averaging mode

#define	CSPDR_MAX_FIR_PIPELINE				32				// Number of head samples lost due to FIR pipeline
														// Should be multiple of 8

#define	CSPDR_FIR_ACTUALSTART_ADJUST20	31				// Number of head samples lost due to FIR pipeline
														// Should be multiple of 8

#define	CSPDR_FIR_ACTUALSTART_ADJUST39	65				// Number of head samples lost due to FIR pipeline
														// Should be multiple of 8

#define	CSPDR_FIR_READINGOFFSET_ADJUST20	19			// Number of head samples lost due to FIR pipeline
														// Should be multiple of 8

#define	CSPDR_FIR_READINGOFFSET_ADJUST39	32				// Number of head samples lost due to FIR pipeline
														// Should be multiple of 8


#define CSPDR_DEFAULT_VALUE				0
#define CSPDR_DEFAULT_RELATION			CS_RELATION_OR

#define	CSPDR_FAST_MEMORY_CLOCK				166000000		// 166MHz clock
#define	CSPDR_MED_MEMORY_CLOCK				150000000		// 150MHz clock
#define	CSPDR_SLOW_MEMORY_CLOCK				133000000		// 133MHz clock
#define CSPDR_PCIEX_MEMORY_CLOCK			200000000		// 200MHz clock

//!----- Default settings
#define	CSPDR_DEFAULT_MSTRIGMODE		3
#define CSPDR_DEFAULT_TRIG_SENSITIVITY	0x20
#define CSPDR_DEFAULT_ZAP_TRIG_SENSE	0xc0
#define CSPDR_DEFAULT_SAMPLE_RATE		-1				// Force to the default sampling rate depending on card
#define CSPDR_DEFAULT_EXT_CLOCK_EN		0
#define CSPDR_DEFAULT_MODE				-1				// Default mode will be set depending on number of channels
#define CSPDR_DEFAULT_SAMPLE_BITS		CS_SAMPLE_BITS_14
#define CSPDR_DEFAULT_RES				(-CS_SAMPLE_RES_LJ)
#define CSPDR_DEFAULT_SAMPLE_SIZE		CS_SAMPLE_SIZE_2
#define CSPDR_DEFAULT_SAMPLE_OFFSET		CS_SAMPLE_OFFSET_14_LJ
#define CSPDR_DEFAULT_SEGMENT_COUNT		1
#define CSPDR_DEFAULT_DEPTH				8192
#define CSPDR_DEFAULT_SEGMENT_SIZE		CSPDR_DEFAULT_DEPTH
#define CSPDR_DEFAULT_TRIG_TIME_OUT		-1
#define CSPDR_DEFAULT_TRIG_ENGINES_EN	CS_TRIG_ENGINES_DIS
#define CSPDR_DEFAULT_TRIG_HOLD_OFF		0
#define CSPDR_DEFAULT_TRIG_DELAY		0

#define CSPDR_DEFAULT_TERM				CS_COUPLING_DC
#define CSPDR_DEFAULT_GAIN				CS_GAIN_2_V
#define CSPDR_DEFAULT_IMPEDANCE			CS_REAL_IMP_50_OHM
#define CSPDR_DEFAULT_FILTER			0
#define CSPDR_DEFAULT_POSITION			0

#define CSPDR_DEFAULT_CONDITION			CS_TRIG_COND_POS_SLOPE
#define CSPDR_DEFAULT_SOURCE			CS_CHAN_1
#define CSPDR_DEFAULT_EXT_COUPLING		CS_COUPLING_DC
#define CSPDR_DEFAULT_EXT_GAIN			CS_GAIN_2_V
#define CSPDR_DEFAULT_EXT_IMPEDANCE		CS_REAL_IMP_1M_OHM

// Spider v1.2 only
#define CSPDR_DEFAULT_ADCOFFSET_ADJ		17
#define CSPDR_DEFAULT_CLOCK_OUT 		eclkoNone

#define CSPDR_SPI				0x8000


//----- Trigger Setting
//----- 3 Slope
//----- 2 Control Enable
//----- 1 source 1
//----- 0 source 0
#define CSPDR_SET_GBLTRIG_ENABLED		0x0001
#define CSPDR_SET_EXT_TRIG_DC			0x0001
#define CSPDR_SET_EXT_TRIG_EXT1V		0x0002
#define CSPDR_SET_EXT_TRIG_POS_SLOPE	0x0004
#define CSPDR_SET_EXT_TRIG_ENABLED		0x0008

#define CSPDR_SET_DIG_TRIG_EXT_EN_OFF    0x0
#define CSPDR_SET_DIG_TRIG_EXT_EN_LIVE   0x2
#define CSPDR_SET_DIG_TRIG_EXT_EN_POS    0x4
#define CSPDR_SET_DIG_TRIG_EXT_EN_NEG    0x6
#define CSPDR_SET_DIG_TRIG_TRIGOUTENABLED    0x10
#define CSPDR_SET_TRIGOUT_CLKPHASE		 0x20


//----- Register command: Add-On registers access
//----- Add-On SPI access : Nios routine: WriteToCpldFpga
#define CSPDR_CHAN1_SET_WR_CMD			0x00811102	// cmd = 0x008, scope = 0x1, add = 0x11  Model/Cs = 02
#define CSPDR_CHAN2_SET_WR_CMD			0x00811202	// cmd = 0x008, scope = 0x1, add = 0x12  Model/Cs = 02
#define CSPDR_CHAN3_SET_WR_CMD			0x00811302	// cmd = 0x008, scope = 0x1, add = 0x13  Model/Cs = 02
#define CSPDR_CHAN4_SET_WR_CMD			0x00811402	// cmd = 0x008, scope = 0x1, add = 0x14  Model/Cs = 02
#define CSPDR_CHAN5_SET_WR_CMD			0x00811103	// cmd = 0x008, scope = 0x1, add = 0x11  Model/Cs = 03
#define CSPDR_CHAN6_SET_WR_CMD			0x00811203	// cmd = 0x008, scope = 0x1, add = 0x12  Model/Cs = 03
#define CSPDR_CHAN7_SET_WR_CMD			0x00811303	// cmd = 0x008, scope = 0x1, add = 0x13  Model/Cs = 03
#define CSPDR_CHAN8_SET_WR_CMD			0x00811403	// cmd = 0x008, scope = 0x1, add = 0x14  Model/Cs = 03

#define CSPDR_PROTECTION_STATUS1		0x00801D02	// cmd = 0x008, scope = 0x1, add = 0x1D  Model/Cs = 02
#define CSPDR_PROTECTION_STATUS2		0x00801D03	// cmd = 0x008, scope = 0x1, add = 0x1D  Model/Cs = 03

#define CSPDR_ACD1_RD_CMD				0x00801E02	// cmd = 0x008, scope = 0x0, add = 0x1E  Model/Cs = 02
#define CSPDR_ACV1_RD_CMD				0x00801F02	// cmd = 0x008, scope = 0x0, add = 0x1F  Model/Cs = 02

#define CSPDR_ACD2_RD_CMD				0x00801E03	// cmd = 0x008, scope = 0x0, add = 0x1E  Model/Cs = 03
#define CSPDR_ACV2_RD_CMD				0x00801F03	// cmd = 0x008, scope = 0x0, add = 0x1F  Model/Cs = 03

#define CSPDR_CLK_SET_RD_CMD			0x00802104	// cmd = 0x008, scope = 0x0, add = 0x21  Model/Cs = 04
#define CSPDR_CLK_SET_WR_CMD			0x00812104	// cmd = 0x008, scope = 0x1, add = 0x21  Model/Cs = 04
#define CSPDR_CLK_DLLE_WR_CMD			0x00812204	// cmd = 0x008, scope = 0x1, add = 0x22  Model/Cs = 04
#define CSPDR_CLK_DLD_WR_CMD			0x00812304	// cmd = 0x008, scope = 0x1, add = 0x23  Model/Cs = 04
#define CSPDR_CALIBMODE_SET_WR_CMD		0x00812404	// cmd = 0x008, scope = 0x1, add = 0x24  Model/Cs = 04
#define CSPDR_EXT_TRIG_SET_WR_CMD		0x00812504	// cmd = 0x008, scope = 0x1, add = 0x25  Model/Cs = 04
#define CSPDR_PULSE_SET_WR_CMD			0x00812604	// cmd = 0x008, scope = 0x1, add = 0x26  Model/Cs = 04
#define CSPDR_OPTION_DIR_SET_WR_CMD		0x00812704	// cmd = 0x008, scope = 0x1, add = 0x27  Model/Cs = 04

#define CSPDR_CPLD_FD_RD_CMD			0x00803E04	// cmd = 0x008, scope = 0x0, add = 0x3E  Model/Cs = 04
#define CSPDR_CPLD_FV_RD_CMD			0x00803F04	// cmd = 0x008, scope = 0x0, add = 0x3F	 Model/Cs = 04

#define CSPDR_TEST_REG_WR_CMD			0x00814001	// cmd = 0x008, scope = 0x1, add = 0x40  Model/Cs = 01
#define CSPDR_SELFTEST_MODE_WR_CMD		0x00814101	// cmd = 0x008, scope = 0x1, add = 0x41  Model/Cs = 01
#define CSPDR_OPTION_SET_WR_CMD			0x00814201	// cmd = 0x008, scope = 0x1, add = 0x42  Model/Cs = 01
#define CSPDR_CAPTUREMODE_SET_WR_CMD	0x00814301	// cmd = 0x008, scope = 0x1, add = 0x43  Model/Cs = 01
#define CSPDR_FPGA_CLK_SET_WR_CMD		0x00814401	// cmd = 0x008, scope = 0x1, add = 0x44  Model/Cs = 01
#define CSPDR_DEC_CTL_WR_CMD			0x00814501	// cmd = 0x008, scope = 0x1, add = 0x45  Model/Cs = 01
#define CSPDR_DEC_FACTOR_LOW_WR_CMD		0x00814601	// cmd = 0x008, scope = 0x1, add = 0x46  Model/Cs = 01
#define CSPDR_DEC_FACTOR_HIGH_WR_CMD	0x00814701	// cmd = 0x008, scope = 0x1, add = 0x46  Model/Cs = 01

// Nucleon only
#define	CSPDR_MS_REG_RD					0x00800901
#define	CSPDR_MS_REG_WR					0x00810901
#define CSPDR_MS_BIST_CTL1_WR			0x00810A01
#define CSPDR_MS_BIST_CTL2_WR			0x00810B01
#define CSPDR_MS_BIST_ST1_RD			0x00801C01
#define CSPDR_MS_BIST_ST2_RD			0x00801D01
#define CSPDR_MSCTRL_REG_WR				0x00814801	
#define CSPDR_MS_STATUS_RD				0x00802804
#define CSPDR_MS_SETUP_WD				0x00812804

#define CSPDR_CLK_STOP_CFG_WR_CMD		0x00815001	// cmd = 0x008, scope = 0x1, add = 0x50  Model/Cs = 01
#define CSPDR_TRIGDELAY_SET_WR_CMD		0x00815201	// cmd = 0x008, scope = 0x1, add = 0x52  Model/Cs = 01
#define CSPDR_TRIG_SET_WR_CMD			0x00815401	// cmd = 0x008, scope = 0x1, add = 0x54  Model/Cs = 01
#define CSPDR_DTS_GBL_SET_WR_CMD		0x00815501	// cmd = 0x008, scope = 0x1, add = 0x55  Model/Cs = 01
#define CSPDR_DTS1_SET_WR_CMD			0x00815601	// cmd = 0x008, scope = 0x1, add = 0x56  Model/Cs = 01
#define CSPDR_DTS2_SET_WR_CMD			0x00815701	// cmd = 0x008, scope = 0x1, add = 0x57  Model/Cs = 01
#define CSPDR_DTS3_SET_WR_CMD			0x00815801	// cmd = 0x008, scope = 0x1, add = 0x58  Model/Cs = 01
#define CSPDR_DTS4_SET_WR_CMD			0x00815901	// cmd = 0x008, scope = 0x1, add = 0x59  Model/Cs = 01
#define CSPDR_DTS5_SET_WR_CMD			0x00815A01	// cmd = 0x008, scope = 0x1, add = 0x5A  Model/Cs = 01
#define CSPDR_DTS6_SET_WR_CMD			0x00815B01	// cmd = 0x008, scope = 0x1, add = 0x5B  Model/Cs = 01
#define CSPDR_DTS7_SET_WR_CMD			0x00815C01	// cmd = 0x008, scope = 0x1, add = 0x5C  Model/Cs = 01
#define CSPDR_DTS8_SET_WR_CMD			0x00815D01	// cmd = 0x008, scope = 0x1, add = 0x5D  Model/Cs = 01

#define CSPDR_FWD_RD_CMD				0x00805E01	// cmd = 0x008, scope = 0x0, add = 0x5E  Model/Cs = 01
#define CSPDR_FWV_RD_CMD				0x00805F01	// cmd = 0x008, scope = 0x0, add = 0x5F  Model/Cs = 01

#define CSPDR_DL_1A_SET_WR_CMD			0x00816001	// cmd = 0x008, scope = 0x1, add = 0x60  Model/Cs = 01
#define CSPDR_DS_1A_SET_WR_CMD			0x00816101	// cmd = 0x008, scope = 0x1, add = 0x61  Model/Cs = 01

#define CSPDR_DL_1B_SET_WR_CMD			0x00816201	// cmd = 0x008, scope = 0x1, add = 0x62  Model/Cs = 01
#define CSPDR_DS_1B_SET_WR_CMD			0x00816301	// cmd = 0x008, scope = 0x1, add = 0x63  Model/Cs = 01

#define CSPDR_DL_2A_SET_WR_CMD			0x00816401	// cmd = 0x008, scope = 0x1, add = 0x64  Model/Cs = 01
#define CSPDR_DS_2A_SET_WR_CMD			0x00816501	// cmd = 0x008, scope = 0x1, add = 0x65  Model/Cs = 01

#define CSPDR_DL_2B_SET_WR_CMD			0x00816601	// cmd = 0x008, scope = 0x1, add = 0x66  Model/Cs = 01
#define CSPDR_DS_2B_SET_WR_CMD			0x00816701	// cmd = 0x008, scope = 0x1, add = 0x67  Model/Cs = 01

#define CSPDR_DL_3A_SET_WR_CMD			0x00816801	// cmd = 0x008, scope = 0x1, add = 0x68  Model/Cs = 01
#define CSPDR_DS_3A_SET_WR_CMD			0x00816921	// cmd = 0x008, scope = 0x1, add = 0x69  Model/Cs = 01

#define CSPDR_DL_3B_SET_WR_CMD			0x00816A01	// cmd = 0x008, scope = 0x1, add = 0x6A  Model/Cs = 01
#define CSPDR_DS_3B_SET_WR_CMD			0x00816B01	// cmd = 0x008, scope = 0x1, add = 0x6B  Model/Cs = 01

#define CSPDR_DL_4A_SET_WR_CMD			0x00816C01	// cmd = 0x008, scope = 0x1, add = 0x6C  Model/Cs = 01
#define CSPDR_DS_4A_SET_WR_CMD			0x00816D01	// cmd = 0x008, scope = 0x1, add = 0x6D  Model/Cs = 01

#define CSPDR_DL_4B_SET_WR_CMD			0x00816E01	// cmd = 0x008, scope = 0x1, add = 0x6E  Model/Cs = 01
#define CSPDR_DS_4B_SET_WR_CMD			0x00816F01	// cmd = 0x008, scope = 0x1, add = 0x6F  Model/Cs = 01

#define CSPDR_DL_5A_SET_WR_CMD			0x00817001	// cmd = 0x008, scope = 0x1, add = 0x70  Model/Cs = 01
#define CSPDR_DS_5A_SET_WR_CMD			0x00817101	// cmd = 0x008, scope = 0x1, add = 0x71  Model/Cs = 01

#define CSPDR_DL_5B_SET_WR_CMD			0x00817201	// cmd = 0x008, scope = 0x1, add = 0x72  Model/Cs = 01
#define CSPDR_DS_5B_SET_WR_CMD			0x00817301	// cmd = 0x008, scope = 0x1, add = 0x73  Model/Cs = 01

#define CSPDR_DL_6A_SET_WR_CMD			0x00817401	// cmd = 0x008, scope = 0x1, add = 0x74  Model/Cs = 01
#define CSPDR_DS_6A_SET_WR_CMD			0x00817501	// cmd = 0x008, scope = 0x1, add = 0x75  Model/Cs = 01

#define CSPDR_DL_6B_SET_WR_CMD			0x00817601	// cmd = 0x008, scope = 0x1, add = 0x76  Model/Cs = 01
#define CSPDR_DS_6B_SET_WR_CMD			0x00817701	// cmd = 0x008, scope = 0x1, add = 0x77  Model/Cs = 01

#define CSPDR_DL_7A_SET_WR_CMD			0x00817801	// cmd = 0x008, scope = 0x1, add = 0x78  Model/Cs = 01
#define CSPDR_DS_7A_SET_WR_CMD			0x00817901	// cmd = 0x008, scope = 0x1, add = 0x79  Model/Cs = 01

#define CSPDR_DL_7B_SET_WR_CMD			0x00817A01	// cmd = 0x008, scope = 0x1, add = 0x7A  Model/Cs = 01
#define CSPDR_DS_7B_SET_WR_CMD			0x00817B01	// cmd = 0x008, scope = 0x1, add = 0x7B  Model/Cs = 01

#define CSPDR_DL_8A_SET_WR_CMD			0x00817C01	// cmd = 0x008, scope = 0x1, add = 0x7C  Model/Cs = 01
#define CSPDR_DS_8A_SET_WR_CMD			0x00817D01	// cmd = 0x008, scope = 0x1, add = 0x7D  Model/Cs = 01

#define CSPDR_DL_8B_SET_WR_CMD			0x00817E01	// cmd = 0x008, scope = 0x1, add = 0x7E  Model/Cs = 01
#define CSPDR_DS_8B_SET_WR_CMD			0x00817F01	// cmd = 0x008, scope = 0x1, add = 0x7F  Model/Cs = 01

#define CSPDR_DDS_CFR1_WR_CMD           0x00810060  // cmd = 0x008, scope = 0x1, add = 0x00  Model/Cs = 60
#define CSPDR_DDS_CFR2_WR_CMD           0x00810160  // cmd = 0x008, scope = 0x1, add = 0x01  Model/Cs = 60
#define CSPDR_DDS_ASF_WR_CMD            0x00810260  // cmd = 0x008, scope = 0x1, add = 0x02  Model/Cs = 60
#define CSPDR_DDS_ARR_WR_CMD            0x00810360  // cmd = 0x008, scope = 0x1, add = 0x03  Model/Cs = 60
#define CSPDR_DDS_FTW0_WR_CMD           0x00810460  // cmd = 0x008, scope = 0x1, add = 0x04  Model/Cs = 60
#define CSPDR_DDS_POW0_WR_CMD           0x00810560  // cmd = 0x008, scope = 0x1, add = 0x05  Model/Cs = 60

// Spider 1.2 only
// Channels ADC Data Offset Correction
#define CSPDR_ADC_DOC1			        0x00812101  // cmd = 0x008, scope = 0x1, add = 0x21  Model/Cs = 01
#define CSPDR_ADC_DOC2					0x00812201  // cmd = 0x008, scope = 0x1, add = 0x22  Model/Cs = 01
#define CSPDR_ADC_DOC3			        0x00812301  // cmd = 0x008, scope = 0x1, add = 0x23  Model/Cs = 01
#define CSPDR_ADC_DOC4					0x00812401  // cmd = 0x008, scope = 0x1, add = 0x24  Model/Cs = 01
#define CSPDR_ADC_DOC5			        0x00812501  // cmd = 0x008, scope = 0x1, add = 0x25  Model/Cs = 01
#define CSPDR_ADC_DOC6					0x00812601  // cmd = 0x008, scope = 0x1, add = 0x26  Model/Cs = 01
#define CSPDR_ADC_DOC7			        0x00812701  // cmd = 0x008, scope = 0x1, add = 0x27  Model/Cs = 01
#define CSPDR_ADC_DOC8					0x00812801  // cmd = 0x008, scope = 0x1, add = 0x28  Model/Cs = 01


//AD7788
#define CSPDR_REF_ADC_WR_CMD            0x008100a0  // cmd = 0x008, scope = 0x1, add = 0x00  Model/Cs = A0
#define CSPDR_REF_ADC_RD_CMD            0x008000a0  // cmd = 0x008, scope = 0x0, add = 0x00  Model/Cs = A0


// AD7788 commands
#define SPDR_REF_ADC_CMD_RD		        0x40011
#define SPDR_REF_ADC_CMD_RESET	        0x1F

//----- SPIDER Trigger Setting
//----- 3 Slope
//----- 2 Control Enable
//----- 1 source 1
//----- 0 source 0
#define CSPDR_SET_DIG_TRIG_A_ENABLE		0x0001
#define CSPDR_SET_DIG_TRIG_B_ENABLE		0x0002
#define CSPDR_SET_DIG_TRIG_A_NEG_SLOPE	0x0004
#define CSPDR_SET_DIG_TRIG_B_NEG_SLOPE	0x0008

// Relate to Decimation Control
#define CSPDR_OCT_CHAN				0x0
#define CSPDR_QUAD_CHAN				0x1
#define CSPDR_DUAL_CHAN				0x2
#define CSPDR_SINGLE_CHAN			0x3

//Clock control
#define CSPRD_SET_PLL_CLK           0x1
#define CSPRD_SET_EXT_CLK           0x2
#define CSPRD_SET_MS_CLK            0x4
#define CSPRD_SET_FIXED_CLK         0x6
#define CSPRD_SET_STRAIGHT_CLK      0x8
#define CSPDR_SET_REFCLK_OUT		0x10
#define CSPDR_CLK_OUT_MUX_MASK		0x110

#define CSPDR_PLL_NOT_LOCKED        0x4000

#define CSPDR_EXT_CLK_BYPASS        0x80
#define CSPRD_CLK_OUT_ENABLE		0x100


// DAC 7552
#define FENDA_OFFSET1_CS	0x20
#define FENDA_OFFSET2_CS	0x21
#define FENDA_OFFSET3_CS	0x22
#define FENDA_OFFSET4_CS	0x23

#define FENDA_GAIN12_CS		0x24
#define FENDA_GAIN34_CS		0x25

#define FENDB_OFFSET1_CS	0x26
#define FENDB_OFFSET2_CS	0x27
#define FENDB_OFFSET3_CS	0x28
#define FENDB_OFFSET4_CS	0x29

#define FENDB_GAIN56_CS		0x2a
#define FENDB_GAIN78_CS		0x2b

#define	CAL_DACVARREF_CS	0x2c
#define	CAL_DACDC_CS		0x2d
#define CAL_DACEXTRIG_CS	0x2e
#define	SYNTH_DAC24_CS		0x2f
#define SYNTH_DAC68_CS		0x3a
#define SYNTH_DACMS_CS		0x3b

#define DAC7552_OUTA		0x0400
#define DAC7552_OUTB		0x0800

#define CSPDR_DDS_DEFAULT_CFR1   0x02001202
#define CSPDR_DDS_DEFAULT_CFR2   0x00000000
#define CSPDR_DDS_DEFAULT_ASF    0x0000ffff
#define CSPDR_DDS_DEFAULT_ARR    0x00000000
#define CSPDR_DDS_DEFAULT_POW0   0x00000000


#define CSPSR_WRITE_TO_DAC7552_CMD	0x00810000	//----cmd = 0x008, scope = 0x1, add = 0xXX00


//---- details of the command register
#define CSPDR_ADDON_RESET		0x1			//----- bit0
#define	CSPDR_ADDON_FCE			0x2			//----- bit1
#define CSPDR_ADDON_FOE			0x4			//----- bit2
#define CSPDR_ADDON_FWE			0x8			//----- bit3
#define CSPDR_ADDON_CONFIG		0x10		//----- bit4

#define CSPDR_ADDON_FLASH_CMD		 0x10
#define CSPDR_ADDON_FLASH_ADD		 0x11
#define CSPDR_ADDON_FLASH_DATA		 0x12
#define CSPDR_ADDON_FLASH_ADDR_PTR	 0x13
#define CSPDR_ADDON_STATUS			 0x14
#define CSPDR_ADDON_FDV				 0x15
#define CSPDR_AD7450_WRITE_REG	     0X1E
#define CSPDR_AD7450_READ_REG	     0X1F
#define CSPDR_ADDON_HIO_TEST		 0x20
#define CSPDR_ADDON_SPI_TEST_WR      0x21
#define CSPDR_ADDON_SPI_TEST_RD      0x22

#define	CSPDR_ADDON_STAT_FLASH_READY		0x1
#define	CSPDR_ADDON_STAT_CONFIG_DONE		0x2
#define CSPDR_TCPLD_INFO					0x4


#define CSPDR_DIG_TRIGLEVEL_SPAN			0x800		// 12 bits always


//----Calib source
#define CSPDR_CALIB_SRC_DC		    0x0
#define CSPDR_CALIB_SRC_HI_FREQ		0x2
#define CSPDR_CALIB_SRC_LO_FREQ		0x4
#define CSPDR_CALIB_SRC_UPDATE		0x8

#define CSPDR_DDS_CLK_FREQ         10000000

// Spider v.12 only
#define CSPDR_PULSE_SET_ENABLED		0x01

//----- Option settings
//----- 1 0: PulseOut 1:Digital in
//----- 0 Option board
#define CSPDR_PULSE_GD_VAL		0x0000
#define CSPDR_PULSE_GD_INV		0x0001
#define CSPDR_PULSE_GD_RISE		0x0002
#define CSPDR_PULSE_GD_FALL		0x0003
#define CSPDR_PULSE_SYNC  		0x0004

#define CSPDR_CLK_STOP_CFG      0x0003  //Counter enable and Rising edge

//Spi clock control
#define CSPDR_START_SPI_CLK     0x3C
#define CSPDR_AD7450_DATA_READ  0x10000
#define CSPDR_AD7450_FIFO_FULL  0x20000

//----- Self Test _mode
#define CSPDR_SELF_CAPTURE	0x0001
#define CSPDR_SELF_BIST		0x0003
#define CSPDR_SELF_COUNTER	0x0005
#define CSPDR_SELF_MISC		0x0007

#define CSPDR_CALIB_DEPTH		 4096
#define CSPDR_CALIB_BUF_MARGIN	 512
#define CSPDR_CALIB_BUFFER_SIZE	 (2 * CSPDR_CALIB_BUF_MARGIN + CSPDR_CALIB_DEPTH)
#define CSPDR_CALIB_BLOCK_SIZE   64

//FTR-B3-GA4.5Z-B102A settling time is 3 ms
//9802-05-20 settling time is 0.4 ms
//thus a 3 ms delay

#define CSPDR_RELAY_SETTLING_TIME	30

// Default adjustment for trigger address offset when FIR filter is enabled
#define CSPDR_DEFAULT_FIR20_ADJOFFSET		0
#define CSPDR_DEFAULT_FIR39_ADJOFFSET		0

// Default adjustment for trigger address offset when Hw average mode is enabled
#define CSPDR_DEFAULT_HWAVERAGE_ADJOFFSET	4

#define CSPDR_12BIT_DATA_MASK   0xC000C
#define CSPDR_14BIT_DATA_MASK   0x00000

//CSPDR_MS_REG_WR
#define CSPDR_MS_DECSYNCH_RESET			0x0001
#define CSPDR_MS_RESET					0x4000

//CSPDR_MS_BIST_CTL1_WR
#define CSPDR_BIST_EN					0x0001
#define CSPDR_BIST_DIR1					0x0002
#define CSPDR_BIST_DIR2					0x0004
//CSPDR_MS_BIST_CTL2_WR
//CSPDR_MS_BIST_ST1_RD
#define CSPDR_BIST_MS_SLAVE_ACK_MASK	0x007F
#define CSPDR_BIST_MS_MULTIDROP_MASK	0x7F80
#define CSPDR_BIST_MS_MULTIDROP_FIRST   0x0080
#define CSPDR_BIST_MS_MULTIDROP_LAST    0x4000
//CSPDR_MS_BIST_ST2_RD
#define CSPDR_BIST_MS_FPGA_COUNT_MASK	0x003F
#define CSPDR_BIST_MS_ADC_COUNT_MASK	0x3FC0
#define CSPDR_BIST_MS_SYNC        		0x4000





// Channel setting
#define CSPDR_SET_20V				0
#define CSPDR_SET_10V				1
#define CSPDR_SET_4V				2
#define CSPDR_SET_2V				3
#define CSPDR_SET_1V				4
#define CSPDR_SET_400MV				5
#define CSPDR_SET_200MV				6
#define CSPDR_SET_100MV				7

#define CSPDR_MAX_DAC			 4095


#define	CSPDR_PCIE_OPT_RD_CMD			0x30
#define CSPDR_PCIE_SWR_RD_CMD			0x34
#define CSPDR_PCIE_HWR_RD_CMD			0x38


typedef enum
{
	 formatDefault = 0,
	 formatHwAverage,
	 formatSoftwareAverage,
	 formatFIR
} _CSDATAFORMAT;


typedef enum _CalibFlag
{
	 CalibCLEAR = 0,
	 CalibSET,
	 CalibNoChange
} CALIBFLAG;


class CsSpiderDev : public CsPlxBase,  public CsNiosApi // CsDevIoCtl
{
private:
	PGLOBAL_PERPROCESS		m_ProcessGlblVars;

#if !defined( USE_SHARED_CARDSTATE )
	SPIDER_CARDSTATE		m_CardState;
#endif

	PSPIDER_CARDSTATE		m_pCardState;

	uInt32					m_u32SystemInitFlag;	// Compuscope System initialization flag
	void UpdateCardState(BOOLEAN bReset = FALSE);

public:
	//! Constructors
	CsSpiderDev( uInt8 u8Index, PCSDEVICE_INFO pDevInfo, PGLOBAL_PERPROCESS ProcessGlblVars);
	CsSpiderDev*		m_MasterIndependent;	// Master /Independant card
	CsSpiderDev*		m_Next;					// Next Slave
	CsSpiderDev*		m_HWONext;				// HW OBJECT next
	CsSpiderDev*		m_pNextInTheChain;		// use for M/S detection
	uInt16				m_DeviceId;
	DRVHANDLE			m_PseudoHandle;
	DRVHANDLE			m_CardHandle;			// Handle From Driver
	uInt8				m_u8CardIndex;			// Card index in native card detection order
	bool				m_bNucleon;				// Nucleon Base board

	uInt16				m_u16NextSlave;			// Next Slave card index

	PDRVSYSTEM			m_pSystem;		// Pointer to DRVSYSTEM
	CsSpiderDev*		m_pTriggerMaster;			// Point to card that has trigger engines enabled
	BOOLEAN				m_BusMasterSupport;			// HW support BusMaster transfer

protected:
	uInt8				m_DelayLoadImageId;			// Image Id to be load later
	uInt8				m_CalibTableId;				// Current active calibration table;

	int16				*m_i16LpXferBuffer;		// Temporarary buffer for Spider Lp

	int64				m_i64PreDepthLimit;
	eRoleMode			m_ermRoleMode;
	uInt32				m_u32Ad7450CtlReg;
	int16				m_TrigAddrOffsetAdjust;			// Trigger address offset adjustment

	uInt32              m_u32CalFreq;
	eCalMode            m_CalMode;
	int16*              m_pi16CalibBuffer;
	int32*              m_pi32CalibA2DBuffer;
	bool				m_bSkipCalVoltageAdjust;
	bool				m_bForceCal[CSPDR_CHANNELS];

	uInt32				m_u32DebugCalibSrc;
	uInt32				m_u32DebugCalibTrace;
	uInt8               m_u8CalDacSettleDelay;

	CSPDR_CAL_INFO		m_CalibInfoTable;

	uInt16				m_u16CalCodeA;
	uInt16				m_u16CalCodeB;
	int16				m_i16CalCodeZ;

	uInt16              m_u16VarRefCode;
	uInt32              m_u32VarRefLevel_uV;
	uInt32              m_u32CalibRange_uV;

	CSACQUISITIONCONFIG		m_OldAcqCfg;
	uInt32					m_u32HwTrigTimeout;

	uInt32					m_u32TailExtraData;		// Amount of extra samples to be added in configuration
													// in order to ensure post trigger depth
	uInt32					m_u32HwSegmentCount;
	int64					m_i64HwSegmentSize;
	int64					m_i64HwPostDepth;
	int64					m_i64HwTriggerHoldoff;
	uInt32					m_32SegmentTailBytes;
	uInt32                  m_u32MaxHwAvgDepth;

	uInt8				m_u8BootLocation;				// BootLocation: 0 = BootImage, 1 = Image0; 2= Image1 ...
	uInt16				m_u16TrigSensitivity;			// Default Digital Trigger sensitivity;
	BOOL				m_bUseDefaultTrigAddrOffset;	// Skip TrigAddr Calib table
	bool				m_bSkipMasterSlaveDetection;	// Skip Master/Slave detection
	BOOL				m_bZeroTrigAddrOffset;			// No Adjustment for Trigger Address Offset
	BOOL				m_bZeroDepthAdjust;				// No adjustment for Postriggger (ensure posttrigger depth)
	bool				m_bZeroTriggerAddress;          // Hw Trigger address is always zero
	bool				m_bSkipFirmwareCheck;		// Skip validation of firmware version
	uInt8				m_AddonImageId;					// Current active Addon FPGA image;
	uInt32				m_32BbFpgaInfoReqrd;			// Base board FPGA version required.
	uInt32				m_32AddonFpgaInfoReqrd;			// Add-on board FPGA version required.
	bool				m_bCsiFileValid;			
	bool				m_bFwlFileValid;
	bool				m_bFwlOld;						// Old version of Fwl file
	bool				m_bCsiOld;						// Old version of Csi file
	bool				m_bNoConfigReset;
	bool				m_bSafeImageActive;
	bool				m_bClearConfigBootLocation;
	uInt32				m_u32IgnoreCalibErrorMask;
	uInt32				m_u32IgnoreCalibErrorMaskLocal;
	CSI_ENTRY			m_BaseBoardCsiEntry;
	CSI_ENTRY			m_AddonCsiEntry;
	FPGA_HEADER_INFO	m_BaseBoardExpertEntry[2];		// Active Xpert on base board
	FPGA_HEADER_INFO	m_AddonBoardExpertEntry[2];		// Active Xpert on addon board board
	BOOLEAN				m_bInvalidHwConfiguration;		// The card has wrong limitation table
	uInt32				m_u32ExtClkStop;                //Enable and configure clock stop detection
	bool				m_bDisableUserOffet;			//Disable user offest
	bool				m_bCalibActive;					// In calibration mode
	bool				m_bMsResetNeeded;

	STREAMSTRUCT		m_Stream;						// Stream state
	CS_TRIG_OUT_CONFIG_PARAMS	m_TrigOutCfg;
	CS_TRIGGER_PARAMS	m_CfgTrigParams[17];

	//----- Variable related to Data Transfer
	CHANNEL_DATA_TRANSFER	m_DataTransfer;

	BOOLEAN				m_IntPresent;
	BOOLEAN				m_UseIntrTrigger;
	BOOLEAN				m_UseIntrBusy;
	BOOLEAN				m_UseIntrTransfer;


	PeevAdaptor			m_PeevAdaptor;				// Peev Adaptor
	BOOLEAN				m_bSoftwareAveraging;		// Mode Software Averaging enabled
	BOOLEAN				m_bHardwareAveraging;		// Mode Harware Averaging enabled
	bool				m_bMulrecAvgTD;				// Mode Expert Multiple Record AVG
	uInt32				m_u32MulrecAvgCount;		// Mulrec AVG count (should be <= 1024)
	int16				*m_pAverageBuffer;			// Temporarary buffer to keep the result of avearging
	BOOLEAN				m_bFlashReset;				// Reset flash is needed before any operation on Flash

	eCalMode				m_CalibMode;

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
	static const uInt32 c_u32RefAdcCount;
	static const uInt16 c_u16CalAdcCount;

	static const uInt16 c_u32MarginNom;
	             uInt16 m_u32MarginDenom;
	static const uInt16 c_u32MarginDenomHiRange;
	static const uInt16 c_u32MarginDenomLoRange;
	static const uInt16 c_u32MarginDenomHiRange_v2;
	static const uInt16 c_u32MarginDenomLoRange_v2;
	static const uInt16 c_u32MarginLoRangeBoundary;


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
	void  ControlSpiClock(bool bStart);
	int32 SendClockSetting(LONGLONG llSampleRate = CSPDR_DEFAULT_SAMPLE_RATE, uInt32 u32ExtClk = 0, uInt32 u32ExtClkSampleSkip = 0, BOOLEAN bRefClk = FALSE);
	int32 RelockReference();
	int32 CalibrateFrontEnd(uInt16 u16Channel);

	void  SetDefaultTrigAddrOffset(void);
	int32 SendCaptureModeSetting(uInt32 u32Mode = CSPDR_DEFAULT_MODE );
	int32 SendCalibModeSetting(const uInt16 u16Channel, const BOOL bSet = false);
	int32 SetClockForCalibration(const bool bSet);
	int32 CalibrateAllChannels(void);
	int32 MsCalibrateAllChannels(void);
	int32 SendCalibSource(const eCalMode ecMode = ecmCalModeDc, const uInt32 u32CalFequency = 0);
	int32 SendChannelSetting(uInt16 Channel, uInt32 u32InputRange = CSPDR_DEFAULT_GAIN, uInt32 u32Term = CSPDR_DEFAULT_TERM, uInt32 u32Impendance = CSPDR_DEFAULT_IMPEDANCE, int32 i32Position = 0, uInt32 u32Filter = 0, int32 i32ForceCalib = 0);
	int32	SendAdcOffsetAjust(uInt16 Channel, uInt16 u16Offet );

	int32 SendDigitalTriggerSensitivity( uInt16 u16TrigEngine, BOOLEAN bA, uInt16 u16Sensitivity = CSPDR_DEFAULT_TRIG_SENSITIVITY );
	int32 SendDigitalTriggerLevel( uInt16  u16TrigEngine, BOOLEAN bA, int32 i32Level);
	int32 SendExtTriggerLevel( int32 int32Level, uInt32 u32ExtCondition,  uInt32 u32ExtTriggerRange );

	int32 SendTriggerEngineSetting(uInt16 u16TrigEngine, int32 i32SourceA, uInt32  u32ConditionA  = CS_TRIG_COND_POS_SLOPE,  int32   i32LevelA = 0,
										  int32	  i32SourceB = 0, uInt32  u32ConditionB = CS_TRIG_COND_POS_SLOPE, int32   i32LevelB = 0);

	int32 SendExtTriggerSetting( BOOLEAN  bExtTrigEnabled = FALSE,
										   eTrigEnableMode eteSet = eteAlways,
										   int32	i32ExtLevel = 0,
										   uInt32	u32ExtCondition = CSPDR_DEFAULT_CONDITION,
										   uInt32	u32ExtTriggerRange =  CSPDR_DEFAULT_EXT_GAIN,
										   uInt32	u32ExtCoupling = CSPDR_DEFAULT_EXT_COUPLING);

	int32 SendTriggerRelation( uInt32 u32Relation	);
	int32 WriteToSelfTestRegister( uInt16 SelfTestMode = CSST_DISABLE );
	int32 InitAddOnRegisters(bool bForce = false);

	int32 WriteToCalibDac(uInt16 Addr, uInt16 u32Data, uInt8 u8Delay = 0);


	void	AssignBoardType();
	void	AssignAcqSystemBoardName( PCSSYSTEMINFO pSysInfo );
	int32	AcqSystemInitialize();
	int32	MsResetDefaultParams( BOOLEAN bReset = FALSE );
	uInt8	GetExtClkTriggerAddressOffsetAdjust();
	void	SetExtClkTriggerAddressOffsetAdjust( uInt8 u8TrigAddressOffset );
	int8	GetTriggerAddressOffsetAdjust( uInt32 u32Mode, uInt32 u32SampleRate );
	void	SetTriggerAddressOffsetAdjust( int8 u8TrigAddressOffset );
	BOOLEAN	CheckRequiredFirmwareVersion();
	void	SetDataMaskModeTransfer ( uInt32 u32TxMode );
	uInt32	GetDataMaskModeTransfer ( uInt32 u32TxMode );
	uInt32	GetFifoModeTransfer( uInt16 u16UserChannel, bool bInterLeaved = false );
	int32	FillOutBoardCaps(PCS8XXXBOARDCAPS pCapsInfo);

	bool            m_bSkipSpiTest;
	bool			m_bSkipCalib;
	bool			m_bSkipTrim;
	uInt16			m_u16SkipCalibDelay; //in minutes;
	bool			m_bNeverCalibrateDc;

	int32 SetTrigOut( uInt16 u16Value );
	int32 SetPulseOut( ePulseMode epmSet = epmDataValid );
	int32 SetClockOut( eClkOutMode ecomSet= eclkoSampleClk );

	ePulseMode  GetPulseOut(void){return m_pCardState->epmPulseOut;}
	eClkOutMode GetClockOut(void){return m_pCardState->eclkoClockOut;}

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

	int32	CsGetAverage(uInt16 u16Channel, int16* pi16Buffer, uInt32 u32BufferSize, int32* pi32Average);
	int32	CsLoadFpgaImage( uInt8 u8ImageId, bool bForce = false );
	void	SetupHwAverageMode();
	int32	ProcessSoftwareAveraging( int64 i64StartAddress, uInt16 u16Channel, int32 *pi32ResultBuffer, uInt32 u32SamplesInBuffer );
#ifdef _WINDOWS_   
	int32	ReadCommonRegistryOnBoardInit();
	int32	ReadCommonRegistryOnAcqSystemInit();
#else
   int32 ReadCommonCfgOnBoardInit();   
	int32	ReadCommonCfgOnAcqSystemInit();
   int32 ShowCfgKeyInfo();
#endif      
	int32	ReadAndValidateCsiFile( uInt32 u32BbHwOpt, uInt32 u32AoHwOpt );
	int32	ReadAndValidateFwlFile();
	void	CsSetDataFormat( _CSDATAFORMAT csFormat = formatDefault );
	BOOLEAN	IsAddonBoardPresent();
	int32	ReadCalibInfoFromCalFile();

	void	AbortSystemDmaTransferDescriptor();
	void	SignalEventsExpert();
	int32	CsDelayLoadBaseBoardFpgaImage( uInt8 u8ImageId );

	void	AdjustedHwSegmentParameters( uInt32 u32Mode, uInt32 *u32SegmentCount, int64 *i64SegmentSize, int64 *i64Depth );
	int32	CommitNewConfiguration( PCSACQUISITIONCONFIG pAcqCfg,
									PARRAY_CHANNELCONFIG pAChanCfg,
									PARRAY_TRIGGERCONFIG pATrigCfg );

	uInt32	CsGetPostTriggerExtraSamples( uInt32 u32Mode );
	uInt32	GetSegmentPadding(uInt32 u32Mode);


public:
	int32	DeviceInitialize(void);
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
	int32	ProcessDataTransferModeAveraging(in_DATA_TRANSFER *InDataXfer);

	CsSpiderDev *GetCardPointerFromChannelIndex( uInt16 ChannelIndex );
	CsSpiderDev *GetCardPointerFromBoardIndex( uInt16 BoardIndex );
	CsSpiderDev *GetCardPointerFromTriggerIndex( uInt16 TriggerIndex );
	CsSpiderDev *GetCardPointerFromTriggerSource( int32 i32TriggerSoure );

	int32   CsGetBoardsInfo( PARRAY_BOARDINFO pABoardInfo );

	int32	CsValidateAcquisitionConfig( PCSACQUISITIONCONFIG, uInt32 u32Coerce );

	void	UpdateSystemInfo( CSSYSTEMINFO *SysInfo );

	uInt16 BuildIndependantSystem( CsSpiderDev** FirstCard, BOOLEAN bForceInd );
	uInt16 BuildMasterSlaveSystem( CsSpiderDev** FirstCard );
	uInt16	DetectAcquisitionSystems( BOOL bForceInd );

	uInt16	GetHardwareChannelSkip();
	uInt16	ConvertToHwChannel( uInt16 u16UserChannel );
	uInt16	ConvertToUserChannel( uInt16 u16HwChannel );
	BOOLEAN	IsCalibrationRequired();
	void	SetFlagCalibRequired( CALIBFLAG MsCalib,CALIBFLAG ExtTrigCalib );

	int32	SetPosition(uInt16 u16Channel, int32 i32Position_mV);
	int32	SendPosition(uInt16 u16Channel, int32 i32Position_mV);

	int32 FindFeIndex( const uInt32 u32Swing, const uInt32 u32Imed, uInt32& u32Index );

	inline	void   ClearFlag_bCalNeeded();

	int32	AcquireAndAverage(uInt16	u16Channel, int32* pi32Avrage);
	int32	CalibrateOffset(uInt16 u16Channel);
	int32	CalibrateGain(uInt16 u16Channel);
	int32	CalibratePosition(uInt16 u16Channel);
	int32   SetCalibrationRange(uInt32 u32FrontEndSwing_mV, uInt32 u32Imped);
	int32   SetCalibrationRange_v2(uInt32 u32FrontEndSwing_mV, uInt32 u32Imped);

	int32		SendDigitalTriggerLevel( uInt16  u16TrigEngine, int32 i32Level );
	LONGLONG	GetMaxSampleRate( BOOLEAN bPingpong );
	LONGLONG	GetMaxExtClockRate( uInt32 u32Mode, BOOLEAN *bPingpong );

	int32		InitHwTables();
	BOOLEAN		FindSrInfo( const LONGLONG llSr, SPIDER_SAMPLE_RATE_INFO *pSrInfo );

	int32		GetFEIndex(uInt16 u16Channel);
	void		SetFastCalibration(uInt16 u16Channel);
	int32		FindSRIndex( LONGLONG llSampleRate, uInt32 *u32IndexSR );
	void		InitBoardCaps();
	int32		CsAutoUpdateFirmware();
	int32		ReadCalibTableFromEeprom(uInt8 u8CalibTableId = 1);
	int32		WriteCalibTableToEeprom(uInt8	u8CalibTableId, uInt32 u32Valid );
	void		UpdateMaxMemorySize( uInt32	u32BaseBoardHardwareOptions, uInt32 u32SampleSize );

	int32		CsDetectZeroCrossAddress();
	int32		CalibrateExtTrigger();
	int32		CsDetectZeroCrossAddress( void	*pBuffer, uInt32 u32BufferSize,
											    uInt32 *u32PosCross, uInt32 *u32NegCross );
	int32		AcquireCalibSignal( uInt16 u16UserChannel, int32 i32StartPos, int32 *i32OffsetAdjust );
	int32		AdjustOffsetChannelAndExtTrigger();


	int32	ReadCalibA2D(int32* pi32V_uv);
	int32   ReadCalibA2D_MeanSquare(int32* pi32Mean, int32* pi32MeanSquare );
	int32	SendCalibDC_128(int32 i32Target_uV, uInt32 u32Imped, int32* pVoltage_uV = NULL, int16* pi16InitialCode = NULL);
	int32	SendCalibDC_128_v2(int32 i32Target_uV, uInt32 u32Imped, int32* pVoltage_uV = NULL, int16* pi16InitialCode = NULL);
	bool    FastCalibrationSettings(uInt16 u16Channel, BOOLEAN bForce = FALSE);
	BOOLEAN	ForceFastCalibrationSettingsAllChannels();
	int32	SendAdcSelfCal(void);

	int32	GeneratePulseCalib();
	int32	InitializeExtTrigEngine( BOOLEAN bRising );
	int32	SetupForExtTrigCalib_SourceExtTrig();

	int32	SetupForExtTrigCalib_SourceChan1();
	int32	SetupForExtTrigCalib( bool bSetup );

	uInt32	BbFwVersionConvert( uInt32 u32OldVersion );
	uInt32	AddonFwVersionConvert( uInt32 u32OldVersion );

	void	TraceCalib(uInt16 u16Channel, uInt32 u32TraceFlag, uInt16 u16Code, int32 i32Target, int32 i32Real);

	EVENT_HANDLE	GetEventTxDone() { return m_DataTransfer.hAsynEvent; }
	EVENT_HANDLE	GetEventSystemCleanup() { return m_pSystem->hAcqSystemCleanup; }
	EVENT_HANDLE	GetEventTriggered() { return m_pSystem->hEventTriggered; }
	EVENT_HANDLE	GetEventEndOfBusy() { return m_pSystem->hEventEndOfBusy; }
	EVENT_HANDLE	GetEventAlarm() { return 0; }
	void			OnAlarmEvent() {};


	void	SetStateTriggered() { m_PlxData->CsBoard.bTriggered = true; }
	void	SetStateAcqDone() { m_PlxData->CsBoard.bBusy = false; }
	void	SignalEvents( BOOL bAquisitionEvent = TRUE );


	BOOLEAN	AssignBoardProperty();
	int32	AddonConfigReset();
	int32	ValidateCsiTarget(uInt32 u32Target);
	void	ResetCachedState(void);
	int32	CsLoadBaseBoardFpgaImage( uInt8 u8ImageId );
	int32	SetClockThruNios( uInt32 u32ClockMhz );
	uInt32	GetDefaultRange();
	int32	ConfigureFe( const uInt16 u16Channel, const uInt32 u32Index, const uInt32 u32Imed, const uInt32 u32Unrestricted);
	uInt16	GetControlDacId( eCalDacId ecdId, uInt16 u16Channel );
	int32	SetupAcquireAverage(bool bSetup);
	int32	MsSetupAcquireAverage( bool bSetup);
	int32	UpdateCalibDac(uInt16 u16Channel, eCalDacId ecdId, uInt16 u16Data);
	int32	GetEepromCalibInfoValues(uInt16 u16Channel, eCalDacId ecdId, uInt16 &u16Data);
	int32	UpdateCalibInfoStrucure();
	int32	SendTriggerDelay();

    void *operator new(size_t  size);//implicitly declared as a static member function
//	void operator delete (void *p );						 //implicitly declared as a static member function

#ifdef __linux__
	void 	CreateInterruptEventThread();
	void	InitEventHandlers();
	void	ThreadFuncIntr();
#endif

	int32	CreateThreadAsynTransfer();
	void	AsynDataTransferDone();

	BOOL	bDebugPrf_DoNotReadData;
	int32	DebugPrf( uInt32 u32DebugOption );

	FlashInfo	*FlashPtr;
	void  SpiderInitFlash(uInt32 FlashIndex);
	void  SpiderFlashCommand(uInt32 Command, uInt32 Sector, uInt32 Offset, uInt32 Data);
	int32 SpiderFlashWrite( uInt32 Sector, uInt32 Offset, uInt8 *Buf, uInt32 nBytes );
    uInt32 SpiderFlashStatus(uInt32 Addr);
	void  SpiderFlashReset();
	void  SpiderFlashWriteByte(  uInt32 Addr, uInt32 Data );
	void  SpiderFlashSectorErase( uInt32 Sector);
	void  SpiderFlashChipErase();
	uInt32	AddonReadFlashByte(uInt32 Addr);
	void	AddonWriteFlashByte(uInt32 Addr, uInt32 Data);
	uInt32	AddonResetFlash(void);
	int32	DoAddonWriteFlash( uInt32 u32Sector, uInt32 u32Offset, PVOID pBuffer, uInt32 u32WriteSize );

	int32	WriteEntireEeprom(AddOnFlash32Mbit *Eeprom);
	int32   WriteSectorsOfEeprom(void *pData, uInt32 u32StartAddr, uInt32 u32Size);
	int32	WriteEepromEx(void *pData, uInt32 u32StartAddr, uInt32 u32Size, FlashAccess flSequence = AccessAuto);
	int32	ReadEeprom(void *Eeprom, uInt32 Addr, uInt32 Size);
	int32	WriteEeprom(void *Eeprom, uInt32 Addr, uInt32 Size);
	int32	ReadAddOnHardwareInfoFromEeprom(BOOLEAN bChecksum = TRUE);

	void    DetectAuxConnector() {m_pCardState->bAuxConnector = (0 != (m_PlxData->CsBoard.u32AddonHardwareOptions & CSPDR_AOHW_OPT_AUX_CONNECTOR));}
	void    DetectExtTrigConnector(){if( 0 != (m_PlxData->CsBoard.u32AddonHardwareOptions&CSPDR_AOHW_OPT_EMONA_FF) ) m_pCardState->u32InstalledMisc = 0;}
	int32	CombineCardUpdateFirmware();

	// Overload functions to support Nulceon
	BOOLEAN ConfigurationReset( uInt8	u8UserImageId );
	int32	BaseBoardConfigurationReset(uInt8	u8UserImageId);
	int32	CsReadPlxNvRAM( PPLX_NVRAM_IMAGE nvRAM_image );
	int32	CsWritePlxNvRam( PPLX_NVRAM_IMAGE nvRAM_image );
	// DEBUG 
	// bPciExpress
	BOOL    IsNiosInit();
	bool	IsImageValid( uInt8 u8Index );
	int32	CsNucleonCardUpdateFirmware();
	int32	ReadBaseBoardHardwareInfoFromFlash( BOOLEAN bChecksum = TRUE);
	int32	NucleonWriteFlash(uInt32 Addr, void *pBuffer, uInt32 WriteSize, bool bBackup = FALSE );
	int32	IoctlGetPciExLnkInfo( PPCIeLINK_INFO pPciExInfo );

	int32	UpdateAddonFirmware( FILE_HANDLE hCsiFile, uInt8 *pu8Buffer );
	int64	GetTimeStampFrequency();
	int32	Trim1M_TCAP( uInt16 u16Channel, BOOL bTCap2Sel, uInt8 u8TrimCount );

	int32 	CsStmTransferToBuffer( PVOID pVa, uInt32 u32TransferSize );
	int32	CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *u32ErrorFlag, uInt32 *u32ActualSize, uInt8 *u8EndOfData );
	int32	CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *u32ErrorFlag );
	void 	MsStmResetParams();
	bool	IsConfigResetRequired( uInt8 u8ExpertImageId );
	bool	IsExpertAVG(uInt8 u8ImageId);
	bool	IsExpertSTREAM(uInt8 u8ImageId = 0);
	void	PreCalculateStreamDataSize( PCSACQUISITIONCONFIG pCsAcqCfg );
	uInt8	ReadBaseBoardCpldVersion();
	uInt32	ReadConfigBootLocation();
	int32	WriteConfigBootLocation( uInt32	u32Location );

	int32	MsInitializeDevIoCtl();
#ifdef USE_SHARED_CARDSTATE
	int32	InitializeDevIoCtl();
#endif


void	DetectMasterSlave(int16 *i16NumMasterSlaveSystems, int16 *i16NumStandAloneSystems);
int32	EnableMsBist(bool bEnable);
int32	DetectSlaves();
int32	SendMasterSlaveSetting();
int32	TestMsBridge();
int32	ResetMasterSlave(bool bForce = false);
bool	BoardInitOK() {	return ( m_PlxData->InitStatus.Info.u32Nios != 0 &&
								 m_PlxData->InitStatus.Info.u32AddOnPresent != 0 &&
								 m_PlxData->InitStatus.Info.u32AddOnFpga != 0 ); }

int32	SetMsTriggerEnable(bool bEnable);
int32	UpdateBaseBoardMsStatus();
int32	GetTriggerInfo( uInt32	u32Segment, uInt32 u32NumOfSegment, int16 *pi16Channel );
CsSpiderDev *GetCardPointerFromMsCardId( uInt8 MsCardId );
uInt16	ReadBusyStatus();
int32	TestFpgaCounter();


};


// Prototype for functions
unsigned int WINAPI ThreadFuncIntr( LPVOID lpParam );
unsigned int WINAPI ThreadFuncTxfer( LPVOID lpParam );

#endif
