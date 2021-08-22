#ifndef __CsDeere_h__
#define __CsDeere_h__


#include "CsDefines.h"
#include "CsTypes.h"
#include "CsDeviceIDs.h"
#include "CsAdvStructs.h"
#include "CsiHeader.h"
#include "CsFirmwareDefs.h"
#include "CsDeereCapsInfo.h"
#include "CsDeereCal.h"
#include "CsPlxDefines.h"
#include "CsPlxBase.h"
#include "CsDeereOptions.h"
#include "CsDeereSineFit.h"

#include "CsNiosApi.h"

#include "CsDrvStructs.h"
#include "DrvCommonType.h"
#include "DllStruct.h"
#include "GageWrapper.h"
#include "CsSharedMemory.h"
#include "CsEventLog.h"
#include "DeereCardState.h"

#ifdef _WINDOWS_
#define USE_SHARED_CARDSTATE
#endif

#define	EVENT_LOG_SOURCENAME		_T("CscdG12")
#define DEERE_SHARENAME				_T("CsJohnDeere")

#define	SHARED_CAPS_NAME			_T("CscdG12WDFCaps")
#define MEMORY_MAPPED_SECTION		_T("CscdG12WDF-shared") // CR-2621

#define		PAGE_SIZE		4096

//	Leave debug code even in realse build just for now, while we are not sure 
//	100 % everything is working well
//#ifdef DBG
	#define		DEBUG_MASTERSLAVE
	#define		DEBUG_EXTTRIGCALIB
//#endif

#define		DBG_NORMAL_CALIB			0
#define		DBG_NEVER_CALIB				1
#define		DBG_ALWAYS_CALIB			2

// One memory location has 16 Samples
#define	SHIFT_DUAL		3
#define	SHIFT_SINGLE	4

#define	MAX_FILENAME_LENGTH				128

// JohnDeere diagnositc Nios Commands
#define DEERE_CHECK_ADC_ALIGN_CMD				0x2C00000
#define DEERE_ADC_ALIGN_CHECK_BUSY				(~0x20000)


#define	DEERE_DEFAULT_XTRAPRETRIGGER			0	//	(Dual)	Double in Single
#define	DEERE_MAX_TRIGGER_ADDRESS_OFFSET		16	//	(Dual)	Double in Single
#define	DEERE_MAX_TAIL_INVALID_SAMPLES_DUAL		96	//	(Dual)	
#define	DEERE_MAX_TAIL_INVALID_SAMPLES_SINGLE	128	//	(Single)
#define	DEERE_MAX_MS_OFFSET						128	//	(Single)
#define DEERE_BYTESRESERVED_HWMULREC_SQR_TAIL	16  //  (2x256 bits) in samples

// Limitation of hardware averaging
#define	AV_INVALIDSAMPLES_HEAD			28				// Limitation of FW
#define	AV_INVALIDSAMPLES_TAIL			8				// Limitation of FW


#define	DEERE_AVG_FIXED_PRETRIG			32
#define	DEERE_AVG_ENSURE_POSTTRIG		32				// Max variable trigAddrOffset 

#define MAX_DRIVER_HWAVERAGING_DEPTH	0x20000	    	// Max trigger depth in HW Averaging mode (in samples)
#define MAX_AVERAGING_PRETRIGDEPTH		16				// Max pre trigger depth in HW Averaging mode (in samples)

// Limitation of sofware averaging
#define MAX_SW_AVERAGING_SEMGENTSIZE	512*1024		// Max segment size in Software averaging mode (in samples)
#define	SW_AVERAGING_BUFFER_PADDING		16				// Padding for Sw Avearge buffer
#define MAX_SW_AVERAGE_SEGMENTCOUNT		4096			// Max segment count in SW averaging mode

#define DEERE_MAX_SQUARE_PRETRIGDEPTH			(4092*16)

#define	MAX_EXTTRIG_ADJUST_SPAN			64


// Default adjustment for trigger address offset when Hw average mode is enabled
#define DEERE_DEFAULT_HWAVERAGE_ADJOFFSET	4
#define DEERE_DEFAULT_ADC_CORRECTION        7

//external trigger calibration
#define	EXTTRIGCALIB_LEVEL_CHAN  		-10
#define EXTTRIGCALIB_SLOPE				CS_TRIG_COND_NEG_SLOPE
#define	EXTTRIG_CALIB_LENGTH			256
#define	MAX_EXTTRIG_ADJUST				63
#define	MIN_EXTTRIG_ADJUST				0
#define	DEFAULT_EXTTRIG_ADJUST		    32
#define	MSCALIB_TRIGLEVEL				-25



// Trigger Engine Index in Table
#define	INDEX_TRIGENGINE_EXT		0
#define	INDEX_TRIGENGINE_A1			1
#define	INDEX_TRIGENGINE_B1			2
#define	INDEX_TRIGENGINE_A2			3
#define	INDEX_TRIGENGINE_B2			4

#define	DEERE_SPI					0x8000

//----- Reset PLL 
#define DEERE_PLL_RESET 			0x3100000 //----cmd = 0x031, scope = 0x1, add = 0x0000
//----- Reset and reinit ADC
#define DEERE_ADC_RESET				0x3200000 //----cmd = 0x032, scope = 0x1, add = 0x0000
#define DEERE_ADC_RESET_TIMEOUT     20000     // 2 sec

//----- Trigger Setting
//Bit settings register DEERE_EXT_TRIGGER_SETTING
#define DEERE_SET_EXT_TRIGGER_POS_SLOPE	0x0004
#define DEERE_SET_EXT_TRIGGER_ENABLED	0x0008

// Bit settings for register DEERE_EXTTRIG_INPUT
#define DEERE_ETI_NOT_PULSE         0x0001
#define DEERE_ETI_AC				0x0002
#define DEERE_ETI_50_OHM			0x0004
#define DEERE_ETI_NORMAL			0x0018 //DAC is on and not reset
#define DEERE_ETI_HIGH_GAIN		    0x0020

#define DEERE_SET_DIG_TRIG_A1_ENABLE	0x0001
#define DEERE_SET_DIG_TRIG_B1_ENABLE	0x0002
#define DEERE_SET_DIG_TRIG_A1_POS_SLOPE	0x0004
#define DEERE_SET_DIG_TRIG_B1_POS_SLOPE	0x0008
#define DEERE_SET_DIG_TRIG_A1_CHAN_SEL	0x0010
#define DEERE_SET_DIG_TRIG_B1_CHAN_SEL	0x0020
#define DEERE_SET_DIG_TRIG_A2_ENABLE	0x0040
#define DEERE_SET_DIG_TRIG_B2_ENABLE	0x0080
#define DEERE_SET_DIG_TRIG_A2_POS_SLOPE	0x0100
#define DEERE_SET_DIG_TRIG_B2_POS_SLOPE	0x0200
#define DEERE_SET_DIG_TRIG_A2_CHAN_SEL	0x0400
#define DEERE_SET_DIG_TRIG_B2_CHAN_SEL	0x0800
#define DEERE_SET_DIG_TRIG_TRIGEN3_EN	0x1000

#define DEERE_GLB_TRIG_ENG_ENABLE        0x0001

// JOHN DEERE Registers Map
#define	DEERE_MS_CTRL					0x00810201  // addr = 0x02  Model/Cs = 01
#define	DEERE_SET_ADC					0x00810301  // addr = 0x03  Model/Cs = 01

#define	DEERE_EXTTRIG_REG				0x00810501  // addr = 0x06  Model/Cs = 01
#define	DEERE_CLOCK_CTRL				0x00810601  // addr = 0x06  Model/Cs = 01

#define	DEERE_MS_REG_WR					0x00810901	// addr = 0x09  Model/Cs = 01
#define	DEERE_MS_REG_RD					0x00800901  // addr = 0x09  Model/Cs = 01
#define	DEERE_MS_BISTCTRL_REG1			0x00810A01	// addr = 0x0A  Model/Cs = 01
#define	DEERE_MS_BISTCTRL_REG2			0x00810B01  // addr = 0x0B  Model/Cs = 01
#define	DEERE_FPGA_STATUS				0x00800C01  // addr = 0x0C  Model/Cs = 01

#define	DEERE_MS_BITSTSTAT_REG1			0x00801C01  // addr = 0x1C  Model/Cs = 01
#define	DEERE_MS_BITSTSTAT_REG2			0x00801D01	// addr = 0x1D  Model/Cs = 01

#define DEERE_ECXTCLK_OUTPUT            0x00812101	// addr = 0x21  Model/Cs = 01
													
#define DEERE_EXT_TRIGGER_SETTING		0x00812501	// addr = 0x25  Model/Cs = 01
#define DEERE_PULSE_CALIB				0x00813901	// addr = 0x39  Model/Cs = 01

#define DEERE_TEST_REG_WR_CMD			0x00814001  // addr = 0x40  Model/Cs = 01
#define	DEERE_DATA_SEL_WR_CMD			0x00814101	// addr = 0x41  Model/Cs = 01
#define	DEERE_ADC_ALIGN_WR_CMD			0x00814201	// addr = 0x42  Model/Cs = 01

#define	DEERE_DECIMATION_CTRL			0x00814501	// addr = 0x45  Model/Cs = 01
#define DEERE_DEC_FACTOR_LOW_WR_CMD		0x00814601  // addr = 0x46  Model/Cs = 01
#define DEERE_DEC_FACTOR_HIGH_WR_CMD	0x00814701	// addr = 0x47  Model/Cs = 01

#define DEERE_TRIG_SETTING				0x00815401  // addr = 0x54  Model/Cs = 01
#define DEERE_EXTTRIG_ALIGNMENT			0x00815501	// addr = 0x55  Model/Cs = 01
#define DEERE_DTE_SETTINGS				0x00815601	// addr = 0x56  Model/Cs = 01
#define DEERE_ADC_0_2_COR				0x00815701	// addr = 0x57  Model/Cs = 01
#define DEERE_ADC_1_3_COR				0x00815801	// addr = 0x58  Model/Cs = 01
#define DEERE_EXT_TRIG_EN_CFG			0x00815901	// addr = 0x59  Model/Cs = 01

#define DEERE_DLA1_SETTING_ADDR			0x00816001	// addr = 0x60  Model/Cs = 01
#define DEERE_DSA1_SETTING_ADDR			0x00816101	// addr = 0x61  Model/Cs = 01
#define DEERE_DLB1_SETTING_ADDR			0x00816201	// addr = 0x62  Model/Cs = 01
#define DEERE_DSB1_SETTING_ADDR			0x00816301	// addr = 0x63  Model/Cs = 01
#define DEERE_DLA2_SETTING_ADDR			0x00816401	// addr = 0x64  Model/Cs = 01
#define DEERE_DSA2_SETTING_ADDR			0x00816501	// addr = 0x65  Model/Cs = 01
#define DEERE_DLB2_SETTING_ADDR			0x00816601	// addr = 0x66  Model/Cs = 01
#define DEERE_DSB2_SETTING_ADDR			0x00816701	// addr = 0x67  Model/Cs = 01

#define DEERE_FWV_RD_CMD				0x00805F01  // addr = 0x5F  Model/Cs = 01
#define DEERE_HWR_RD_CMD				0x00807D01	// addr = 0x7D  Model/Cs = 01
#define DEERE_SWR_RD_CMD				0x00807E01	// addr = 0x7E  Model/Cs = 01

#define	DEERE_SET_CHAN1					0x00810003  // addr = 0x00  Model/Cs = 03
#define	DEERE_SET_CHAN2					0x00810103	// addr = 0x01  Model/Cs = 03
#define	DEERE_SET_ADCDRV				0x00810203	// addr = 0x02  Model/Cs = 03
#define	DEERE_SET_CALDC   				0x00810403	// addr = 0x04  Model/Cs = 03
#define	DEERE_SET_CALAC   				0x00810503	// addr = 0x05  Model/Cs = 03
#define DEERE_EXTTRIG_INPUT				0x00810603	// addr = 0x06  Model/Cs = 03
#define DEERE_ALARM_CMD                 0x00800703	// addr = 0x07  Model/Cs = 03
#define DEERE_AN_FWR_CMD                0x00800F03	// addr = 0x0F  Model/Cs = 03

#define DEERE_7552_WR_CMD               0x00810010	// addr = 0xXX  Model/Cs = 1x

#define DEERE_4360_WR_CMD               0x00810020  // addr = 0x00  Model/Cs = 20
													
#define	DEERE_CALADC_CTL   				0x00810040	// addr = 0x00  Model/Cs = 40
#define	DEERE_CALADC_DATA  				0x00800040  // addr = 0x00  Model/Cs = 40
#define	DEERE_CALADC_RNG   				0x00810140  // addr = 0x01  Model/Cs = 40

#define	DEERE_4302_WR_CMD 				0x00810050  // addr = 0x01  Model/Cs = 50

#define DEERE_9511_WR_CMD               0x00810070  // addr = 0x00  Model/Cs = 70

#define DEERE_ADC_WR_CMD                0x008100e0  // addr = 0x00  Model/Cs = E0
#define DEERE_ADC_RD_CMD                0x008000e0  // addr = 0x00  Model/Cs = E0


// Bits setting for M/S Control register
#define DEERE_MSTRANSCEIVEROUT_DISABLE		0x8
#define DEERE_MSDAC_RESET					0x4
#define DEERE_MSDAC_PWRDOWN					0x2
#define DEERE_MSCLKBUFFER_ENABLE			0x1


//Bits setting for Reg DEERE_EXTTRIG_REG
#define	DEERE_TRIGOUTENABLED		0x1
#define	DEERE_BUSYOUTENABLED		0x2


//Bits setting for Reg DEERE_EXTTRIG_INPUT
#define DEERE_EXTTRIG_PULSEIN		0x0010

// Bits setting for clock Register  
#define DEERE_CLKR1_REFECI		  0x1
#define DEERE_CLKR1_LOCALCLK	  0x2
#define DEERE_CLKR1_SYNTH_FUNC	  0x4
#define DEERE_CLKR1_PLL_SRC  	  0x8

//Bits for status register
#define DEERE_STATUS_ACPLD_ALARM  0x1
#define DEERE_STATUS_LOSS_OF_LOCK 0x2

// Bits setting for clock out 21  u16ClkOut variable
#define DEERE_ADC_CLK_OUT	      0x1
#define DEERE_CLK_OUT_EN	      0x2
#define DEERE_PING_PONG  	      0x8

// Bits setting for M/S ststus register DEERE_MS_REG_WR
#define DEERE_MS_CLK_LOCKED	      0x80

#define DEERE_4_ADC_BY_1_CH	      0x0
#define DEERE_1_ADC_BY_1_CH	      0x1
#define DEERE_2_ADC_BY_1_CH	      0x2
#define DEERE_2_ADC_BY_2_CH	      0x3
#define DEERE_1_ADC_BY_2_CH	      0x4

#define DEERE_DUAL_CHAN  	      0x0
#define DEERE_CHAN_A_ONLY  	      0x1
#define DEERE_CHAN_B_ONLY  	      0x2

#define	DEERE_DUAL_CHAN_ON_BIT	  0x10000

#define DEERE_DAC7552_OUTA		  0x0400
#define DEERE_DAC7552_OUTB		  0x0800

#define DEERE_MS_DETECT           0x0080

#define DEERE_BIST_EN					0x0001
#define DEERE_BIST_DIR1					0x0002
#define DEERE_BIST_DIR2					0x0004
#define DEERE_BIST_MS_SLAVE_ACK_MASK	0x007F
#define DEERE_BIST_MS_MULTIDROP_MASK	0x7F80
#define DEERE_BIST_MS_MULTIDROP_FIRST	0x0080
#define DEERE_BIST_MS_MULTIDROP_LAST	0x4000
#define DEERE_BIST_MS_FPGA_COUNT_MASK	0x003F
#define DEERE_BIST_MS_ADC_COUNT_MASK	0x3FC0
#define DEERE_BIST_MS_SYNC        		0x4000

#define DEERE_MS_DECSYNCH_RESET			 0x1
#define	DEERE_DEFAULT_MSTRIGMODE		3
#define DEERE_MS_DETECT              0x0080
#define DEERE_MS_RESET               0x4000


#define DEERE_4360_CCLR            0x0000
#define DEERE_4360_RCLR            0x0100
#define DEERE_4360_NCLR            0x0200

#define DEERE_9511_OUT1            0x3E00
#define DEERE_9511_OUT2            0x3F00

#define DEERE_4360_CLK_1V1        10       //MHz
#define DEERE_4360_CLK_1V0        250      //MHz
#define DEERE_HI_CAL_FREQ         197      //MHz
#define DEERE_LOW_CAL_FREQ         97      //MHz

#define DEERE_CALIB_BUF_MARGIN	  256
#define DEERE_CALIB_DEPTH		  16384
#define DEERE_CALIB_BUFFER_SIZE	  (DEERE_CALIB_BUF_MARGIN + DEERE_CALIB_DEPTH)
#define DEERE_CALIB_BLOCK_SIZE	  64

//---- details of the command register
#define DEERE_ADDON_RESET		  0x1			//----- bit0
#define	DEERE_ADDON_FCE			  0x2			//----- bit1
#define DEERE_ADDON_FOE			  0x4			//----- bit2
#define DEERE_ADDON_FWE			  0x8			//----- bit3
#define DEERE_ADDON_CONFIG		  0x10		    //----- bit4

#define DEERE_ADDON_FLASH_CMD	   0x10
#define DEERE_ADDON_FLASH_ADD	   0x11
#define DEERE_ADDON_FLASH_DATA	   0x12
#define DEERE_ADDON_FLASH_ADDR_PTR 0x13
#define DEERE_ADDON_STATUS		   0x14
#define DEERE_ADDON_FDV			   0x15
#define DEERE_ADDON_HIO_TEST	   0x20	
#define DEERE_ADDON_SPI_TEST_WR    0x21
#define DEERE_ADDON_SPI_TEST_RD    0x22
#define DEERE_AD7450_WRITE_REG	   0x88 
#define DEERE_AD7450_READ_REG	   0x89 

#define	DEERE_ADDON_STAT_FLASH_READY		0x1
#define	DEERE_ADDON_STAT_CONFIG_DONE		0x2
#define DEERE_TCPLD_INFO					0x4

#define DEERE_AD7450_RESET         0x1
#define DEERE_AD7450_READ          0x2
#define DEERE_AD7450_DATA_READ     0x10000
#define DEERE_AD7450_FIFO_FULL     0x20000

// Register configs
#define DEERE_CAL_EXT_TRIG_SRC   	0x40	//	(Calibbration register)
#define	DEERE_MSPULSE_DISABLE		0x20	//	(Calibbration register)
#define	DEERE_LOCAL_PULSE_ENBLE		0x10	//	(Calibbration register)
#define	DEERE_PULSE_SIGNAL			0x08	//	(Calibbration register)
#define	DEERE_CAL_AC_GAIN			0x04	//	(Calibbration register)
#define	DEERE_CAL_POWER  			0x02	//	(Calibbration register)
#define	DEERE_CAL_RESET      		0x01	//	(Calibbration register)

#define DEERE_SET_DC_COUPLING		0x8
#define DEERE_SET_CHAN_FILTER		0x4
#define DEERE_SET_USER_INPUT		0x1

#define DEERE_ADC_RESET_BIT			0x0001

#define DEERE_DIG_TRIGLEVEL_SPAN	0x4000		// 14 bits

#define	DEERE_ADCREG_PORT_CFG		0x0000
#define	DEERE_ADCREG_INDEX			0x1000
#define	DEERE_ADCREG_OFF_COARSE		0x2000
#define	DEERE_ADCREG_OFF_FINE		0x2100
#define	DEERE_ADCREG_GAIN_COARSE	0x2200
#define	DEERE_ADCREG_GAIN_MEDIUM	0x2300
#define	DEERE_ADCREG_GAIN_FINE		0x2400
#define	DEERE_ADCREG_OP_MODE		0x2500
#define	DEERE_ADCREG_SKEW			0x7000
#define	DEERE_ADCREG_CLOCK_DIV		0x7200
#define	DEERE_ADCREG_OUTPUTMODEA	0x7300
#define	DEERE_ADCREG_OUTPUTMODEB	0x7400
#define	DEERE_ADCREG_CFG_STATUS 	0x7500

#define DEERE_EXTTRIG_EN_ENABLE     0x0003
#define DEERE_EXTTRIG_EN_GATE_MODE  0x0004
#define DEERE_EXTTRIG_EN_POS_SLOPE  0x0008
#define DEERE_EXTTRIG_EN_RST_START  0x0010
#define DEERE_EXTTRIG_EN_FORCE_RST  0x0020

#define	DEERE_ADC_DLL_SLOW      	0x40

#define DEERE_CALAC_LPLL_LOCK           0x01
#define DEERE_CALAC_POWER               0x02

#define DEERE_CALDC_REF_MASK            0x03
#define DEERE_CALDC_SRC_GRD             0x40

//Ad7321 parameters
#define DEERE_AD7321_RESTART			 0x1
#define DEERE_AD7321_READ				 0x2
#define DEERE_AD7321_DATA_READ			 0x00010000
#define DEERE_AD7321_RD_FIFO_FULL		 0x00020000
#define DEERE_AD7321_WR_FIFO_FULL		 0x00040000
#define DEERE_AD7321_STATE				 0x00380000
#define DEERE_AD7321_STATE_SHIFT		 19
#define DEERE_AD7321_COUNT				 0x1fc00000
#define DEERE_AD7321_COUNT_SHIFT		 22
#define DEERE_AD7321_FIXED_ERR   		 25000       //25 mV
#define DEERE_AD7321_GAIN_ERR            5           // 5 %

#define DEERE_ADC7321_CTRL				 0x8010     //SE mode, no power managment, 2's compliment, int ref, no sequence
#define DEERE_ADC7321_CHAN				 0x0400     //input selection bit, should be the defult position
#define DEERE_ADC7321_RANGE				 0xA000
#define DEERE_ADC7321_CH0_10_V			 0x0000
#define DEERE_ADC7321_CH0_5_V			 0x0800
#define DEERE_ADC7321_CH0_2p5_V			 0x1000
#define DEERE_ADC7321_CH1_10_V			 0x0000
#define DEERE_ADC7321_CH1_5_V			 0x0080
#define DEERE_ADC7321_CH1_2p5_V			 0x0100
#define DEERE_AD7321_CODE_SPAN             4095

#define DEERE_CALA2D_2p5_V_RANGE          2500
#define DEERE_CALA2D_5_V_RANGE            5000
#define DEERE_CALA2D_10_V_RANGE          10000

#define DEERE_ALARM_INVALID              0x8000
#define DEERE_ALARM_CLEAR                0x0001
#define DEERE_ALARM_PROTECT_CH1          0x0002
#define DEERE_ALARM_PROTECT_CH2          0x0004
#define DEERE_ALARM_CAL_PLL_UNLOCK       0x0008


//----- Self Test _mode
#define DEERE_SELF_BIST			0x0001
#define DEERE_SELF_COUNTER		0x0002

#define	DEERE_PULSE_DURATION	0x0008
#define	DEERE_OB_PULSE_DISABLED	0x0004
#define	DEERE_MS_PULSE_DISABLED	0x0002
#define	DEERE_PULSE_DISABLED	(DEERE_OB_PULSE_DISABLED|DEERE_MS_PULSE_DISABLED)

#define	DEERE_PULSE_GENERATE	0x0001


typedef struct _DEERE_COMMIT
{
	void	*ActionBuffer;
	uInt32	u32CommitFlag;			//input
	uInt32 	u32ChanNeedCalib;		// outpu

} DEERE_COMMIT, *PDEERE_COMMIT;



typedef enum _CSDATAFORMAT
{
	 formatDefault = 0,
	 formatHwAverage,
	 formatSoftwareAverage,
	 formatFIR
} _CSDATAFORMAT;


#ifdef	USE_NEW_TRIGADDROFFSET_TABLE

typedef struct _DeereTrigAddrOffset
{
	uInt32		u32Decimation;
	int8		i8OffsetDual;
	int8		i8OffsetSingle;
} DEERE_TRIGADDR_OFFSET, *PDEERE_TRIGADDR_OFFSET;

#else

typedef struct _RabitTrigAddrOffset
{
	LONGLONG	llSampleRate;
	uInt8		u8OffsetDual;
	uInt8		u8OffsetSingle;
} RABIT_TRIGADDR_OFFSET, *PRABIT_TRIGADDR_OFFSET;

#endif


class CsDeereDevice : public CsPlxBase,  public CsNiosApi
{
private:
	PGLOBAL_PERPROCESS		m_ProcessGlblVars;

#if !defined( USE_SHARED_CARDSTATE )
	DEERE_CARDSTATE			m_CardState;
#endif

	PDEERE_CARDSTATE		m_pCardState;
	void UpdateCardState(BOOLEAN bSuccess = TRUE);

	uInt32					m_u32SystemInitFlag;	// Compuscope System initialization flag
	//! Constructors
public:
	CsDeereDevice( uInt8 u8Index, PCSDEVICE_INFO pDevInfo, PGLOBAL_PERPROCESS ProcessGlblVars);
	CsDeereDevice*		m_MasterIndependent;	// Master / Independent card
	CsDeereDevice*		m_Next;					// Next Slave
	CsDeereDevice*		m_HWONext;				// Native HW OBJECT next (order detected by PCI bus driver)
	CsDeereDevice*		m_pNextInTheChain;		// use for M/S detection only
	DRVHANDLE			m_PseudoHandle;
	DRVHANDLE			m_CardHandle;			// Handle from Kernel Driver
	uInt8				m_u8CardIndex;			// Card index in native card detection order

	PDRVSYSTEM			m_pSystem;					// Pointer to DRVSYSTEM
	CsDeereDevice*		m_pTriggerMaster;			// Point to card that has trigger engines enabled
	CsDeereDevice*		m_pVirtualMaster;			// Point to card that has trigger engines enabled
													// Should be the same as m_pTriggerMaster
													// To be removed one of these two

protected:
	CS_BOARD_NODE *			m_pCsBoard;
	uInt8					m_u8TrigSensitivity;			// Default Digital Trigger sensitivity;
	int32					m_u16ExtTrigSensitivity;		// Default External Trigger sensitivity;
	BOOLEAN					m_bZeroTrigAddress;				// Reading from beginning of memory
	bool					m_bZeroTrigAddrOffset;			// No Adjustment for Trigger Address Offset
	BOOLEAN					m_bZeroDepthAdjust;				// No Adjustment for Depth and Segment Size
	BOOLEAN					m_bZeroExtTrigDataPathAdjust;	// No Adjustment for Ext Trigger Calib Data Path
	bool					m_bSquareRec;					// Square Rec mode (vs Circular Rec mode )
	bool					m_bCalibSqr;					// Calibration in Square Mode
	uInt32					m_SqrEtb;						// Etb in square  mode

	uInt32					m_32BbFpgaInfoReqrd;			// Base board FPGA version required.
	uInt32					m_32AddonFpgaInfoReqrd;			// Add-on board FPGA version required.
	bool					m_bCsiFileValid;					
	bool					m_bFwlFileValid;						
	CSI_ENTRY				m_AddonCsiEntry;
	FPGA_HEADER_INFO		m_AddonBoardExpertEntry[2];

	int8					m_i8OffsetHwAverage;		// Hw Averaging trig address offset adjust

	// Variables for Master / Slave calibration
	bool					m_bCalibActive;				// In calibration mode
	bool					m_bMsDecimationReset;		// Decimation reset for Master/Slave is required
	int16					m_i16ZeroCrossLevel;
	bool					m_bMsOffsetAdjust;
	uInt16					m_u16DebugMsPulse;
	uInt16					m_u16DebugExtTrigCalib;
	uInt8					m_u8NeverCalibrateMs;
	bool					m_bTestBridgeEnabled;		// Decimation reset for Master/Slave is required
	uInt8					m_u8DebugCalibExtTrig;		// No Calibration for External required
	bool					m_bNoAdjExtTrigFail;		// No Adjustment for Ext Trig if Calib Fails
	bool					m_bNeverCalibAdcAlign;		// No atempt of adc alignment
	bool					m_bSkipCalibAdcAlign;		// Do not write ADC aligment
	bool					m_bUseUserCalSignal;
	bool                    m_bGrayCode;
	uInt8                   m_u8DataCorrection;         //To combat digital feedback
	uInt16                  m_u16DigitalDelay;
	bool				    m_bLowExtTrigGain;;
	bool					m_bDynamicSquareRec;		// If it is false then always use circular rec on single rec mode
	bool					m_bCh2Single;				// Capture single channel mode with channel 2 input
	uInt32					m_u32MatchAdcGainTolerance;
	uInt32					m_u32MatchAdcOffsetTolerance;

	uInt32					m_u32DefaultRange;					// Default input range
	uInt32					m_u32DefaultExtTrigRange;			// Default Ext Trigger range
	uInt32					m_u32DefaultExtTrigCoupling;		// Default Ext Trigger coupling
	uInt32					m_u32DefaultExtTrigImpedance;		// Default Ext Trigger impedance

	int16					m_i16ZeroCrossOffset;

	uInt32					m_u32MulrecAvgCount;
	uInt32					m_u32CalFreq;   //Hz



	//
	// Calibration /Adjustment variables
	//
	DEERE_CAL_INFO	m_CalibInfoTable;		// Active Calibration Info Table
	DEERE_CAL_INFO	m_CalFileTable;			// Calibration Info Table from Cal file

	uInt16			m_u16SkipCalibDelay; //in minutes;
	uInt16          m_u16AdcDpaMask;
	bool            m_bSkipAdcSelfCal;
	bool			m_bNeverCalibrateAc;
	bool			m_bNeverCalibrateDc;
	bool            m_bNeverMatchAdcOffset;
	bool            m_bNeverMatchAdcGain;
	bool            m_bNeverMatchAdcPhase;
	bool            m_bSkipUserCal;
	bool			m_bSkipCalib;
	bool			m_bDebugCalibSrc;
	bool            m_bDebugCalSettlingTime;
	eCalMode        m_ecmDebugCalSrcMode;
	bool            m_bReportOffsetSaturation[DEERE_CHANNELS];
	bool			m_bFastCalibSettingsDone[DEERE_CHANNELS];
	bool			m_bUseDacCalibFromEEprom;
	bool			m_bMulrecAvgTD;
	bool			m_bDeviceInitialized;

	bool			m_bHwAvg32;
	bool			m_bSkipFirmwareCheck;		// Skip validation of firmware version
	uInt32					m_u32CsiTarget;
	CSI_ENTRY				m_BaseCsiEntry;
	FPGA_HEADER_INFO		m_BaseExpertEntry[2];

	uInt16          m_u16Atten4302;

	uInt32			m_u32DebugCalibSrc;
	uInt32			m_u32DebugCalibTrace;
	uInt32			m_u32IgnoreCalibError;
	uInt32			m_u32IgnoreTargetMismatch;
	bool            m_bVerifyDcCalSrc;


	BOOLEAN			m_bExtClkNeverPingpong;
	bool			m_bAdcMatchNeeded[DEERE_CHANNELS];
	bool			m_bCoreMatchNeeded[DEERE_CHANNELS];
	bool            m_bForceCal[DEERE_CHANNELS];

	BOOLEAN			m_bExpertAvgFullRes;		// Mode Harware Averaging in full resolution (32bits)

	uInt8			m_u8AdcIndex[DEERE_ADC_COUNT][DEERE_ADC_CORES];   // The order cores' data appear in the data stream and used in calibration. 
																		   // Since it is determined for each channel prior to its use there is no need to make it channel spesific 
	uInt8					m_au8AdcOrder[DEERE_ADC_COUNT];
	int32					m_ai32AdcAlignment[DEERE_ADC_COUNT];
	int16					m_i16DelayAdc[DEERE_ADC_COUNT];	// 0: 0, 1:90, 2:180, 3:270
	int16*					m_pi16DataAdc[DEERE_ADC_COUNT];
	int16*					m_pi16CalibBuffer;
	int32*					m_pi32CalibA2DBuffer;
	bool                    m_bAutoClockAdjust;

	uInt32						m_u32CalibRange_mV;
	uInt32						m_u32CalibSwing_uV;
	uInt32						m_u32VarRefLevel_uV;
	int32                       m_i32CalAdcOffset_uV;
	uInt16                      m_u16CalCodeB;
	uInt8						m_u8CalDacSettleDelay;
	uInt32			            m_u32CalSrcRelayDelay;
	uInt16						m_u16AdcRegDelay;

	unsigned long		m_BytesTransferred;
	BOOLEAN				m_BusMasterSupport;			// HW support BusMaster transfer

	uInt32				m_u32HwSegmentCount;
	int64				m_i64HwSegmentSize;
	int64				m_i64HwPostDepth;
	int64				m_i64HwTriggerHoldoff;


#ifdef DEBUG_MASTERSLAVE
	uInt8				m_PulseData[PAGE_SIZE];
#endif

#ifdef DEBUG_EXTTRIGCALIB
	uInt8*				m_pu8DebugBuffer1[PAGE_SIZE];
	uInt8*				m_pu8DebugBufferEx[PAGE_SIZE];
#endif
	
	CSACQUISITIONCONFIG	m_OldAcqConfig;
	CS_TRIGGER_PARAMS	m_CfgTrigParams[2*DEERE_CHANNELS+1];
	CS_TRIGGER_PARAMS	m_OldTrigParams[2*DEERE_CHANNELS+1];
	CS_CHANNEL_PARAMS	m_OldChanParams[DEERE_CHANNELS];
	char				m_szCsiFileName[MAX_FILENAME_LENGTH];	// Csi file name
	char				m_szFwlFileName[MAX_FILENAME_LENGTH];	// fwl file name

	FRONTEND_DEERE_INFO		m_FeInfoTable[DEERE_MAX_RANGES];
	uInt32					m_u32FeInfoSize;

	DEERE_SAMPLERATE_INFO	m_SrInfoTable[DEERE_MAX_SR_COUNT];
	uInt32					m_u32SrInfoSize;


	//----- Variable related to Data Transfer
	CHANNEL_DATA_TRANSFER	m_DataTransfer;

	//----- Trigger Address
	uInt32	m_Etb;		// Enhanced trigger bit
	uInt32	m_Stb;		// Sub sample trigger bits

	uInt32	m_MemEtb;		// Memory address trigger bit
	uInt32	m_SkipEtb;		// Skiping enhanced trigger bit
	uInt32	m_AddonEtb;		// Add-on enhanced trigger bit
	uInt32	m_ChannelSkipEtb;		// Channel skipping enhanced trigger bit
	uInt32  m_DecEtb;				// Decimation etb
	uInt16  m_u16AlarmSource;
	uInt16  m_u16AlarmStatus;
	uInt16	m_u16AlarmMask;
	uInt32	m_u32TailExtraData;
	BOOLEAN				m_bUpdateCoreOrder[DEERE_CHANNELS];

	BOOLEAN				m_UseIntrTrigger;
	BOOLEAN				m_UseIntrBusy;

	CS_FIR_CONFIG_PARAMS	m_FirConfig;
	CS_FFT_CONFIG_PARAMS	m_FFTConfig;
	BOOLEAN				m_bSoftwareAveraging;		// Mode Software Averaging enabled
	BOOLEAN				m_bHardwareAveraging;		// Mode Harware Averaging enabled
	int16				*m_pAverageBuffer;			// Temporarary buffer to keep the result of avearging
	BOOLEAN				m_bFlashReset;				// Reset flash is needed before any operation on Flash

	eCalMode			m_CalibMode;

	bool				m_bAvgFullRes;

	int32  MsCsResetDefaultParams(bool bReset = false);
	int32  CsSetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg = NULL );
	int32  CsSetTriggerConfig( PARRAY_TRIGGERCONFIG pATrigCfg = NULL );
	int32  CsSetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg = NULL, bool bResetDefaultParams = true );

	int32	ProcessDataTransfer(in_DATA_TRANSFER *InDataXfer);
	int32	ProcessDataTransferEx(io_DATA_TRANSFER_EX *DataXfer);
	int32	ProcessDataTransferModeAveraging(in_DATA_TRANSFER *InDataXfer);

	void	UpdateSystemInfo( CSSYSTEMINFO *SysInfo );
	uInt16	BuildIndependantSystem( CsDeereDevice** FirstCard, BOOLEAN bForceInd );
	uInt16	 BuildMasterSlaveSystem( CsDeereDevice** FirstCard );
	uInt16	DetectAcquisitionSystems( BOOL bForceInd );
	BOOLEAN	IsCalibrationRequired();
	BOOLEAN	IsExtTriggerEnable( PARRAY_TRIGGERCONFIG pATrigCfg );
	BOOLEAN	IsFeCalibrationRequired(void);
	int32	SendPosition(uInt16 u16Channel);
	int32	FindFeIndex( const uInt32 u32Swing, uInt32 *u32Index );

	inline	void   ClearFlag_bCalNeeded();

	int32		SendDigitalTriggerLevel( uInt16  u16TrigEngine, int32 i32Level );
	LONGLONG	GetMaxSampleRate( BOOLEAN bPingpong );
	LONGLONG	GetMaxSampleRateForCalib( uInt32 u32ClockFreq, BOOLEAN bPingpong );
	LONGLONG	GetMaxExtClockRate( uInt32 u32Mode, BOOLEAN *bPingpong );
	int32		SetDataPath( uInt32 u16DataSelect = 0 );
	bool		FindSrInfo( const LONGLONG llSr, PDEERE_SAMPLERATE_INFO *pSrInfo );
	int32		GetFEIndex(uInt16 u16Channel);
	void		SetFastCalibration(uInt16 u16Channel);
	bool		FastCalibrationSettings(uInt16 u16HwChannel, bool bForceSet = false);
	bool		ForceFastCalibrationSettingsAllChannels();
	int32		SendAdcSelfCal(bool bRestore = false);
	int32		FindSRIndex( LONGLONG llSampleRate, uInt32 *u32IndexSR );
	void		InitBoardCaps();
	int32		ReadCalibTableFromEeprom(uInt8 u8CalibTableId = 1);
	int32		WriteCalibTableToEeprom(uInt8	u8CalibTableId, uInt32 u32Valid );
	void		UpdateMaxMemorySize( uInt32	u32BaseBoardHardwareOptions, uInt32 u32SampleSize );

	int32		CalibrateExtTrigger();
	int32		CsDetectZeroCrossAddress( void* pBuffer, uInt32 u32BufferSize, uInt8 u8CrossLevel, uInt32* pu32PosCross );
	int32		CsDetectMsZeroCrossAddress();
	int32		CalculateExtTriggerSkew();
	bool		CsDetectLevelCrossPosition( int16* pBuffer, uInt32 u32BufferSize, uInt32* pu32PosCross, int16 i16DetectLevel = 0, int16* pi16CrossLevel = NULL );
	int32		AcquireCalibSignal( uInt16 u16UserChannel, int32 i32StartPos, int32 *i32OffsetAdjust );
	int32		AdjustOffsetChannelAndExtTrigger();
	int32		VerifyTriggerOffset();
	bool        VerifyTriggerOffsetNeeded();
	int32		CalibTriggerOffset();
	int32		AdjustDigitalDelay();

	const static DEERE_CAL_LUT s_CalCfg50[];
	const static DEERE_CAL_LUT s_CalCfg1M[];
	
	const static FRONTEND_DEERE_INFO c_FrontEnd[];
	const static DEERE_SAMPLERATE_INFO	DeereSRI[];
	const static int32 m_i32ExtClkTrigAddrOffsetTable[2];
	const static uInt16 c_u16CalAttenLowFreq[];

	
	// Variables and functions foir calibration
	static const uInt16 c_u16CalAdcCount;
	static const uInt32 c_u32MaxPositionOffset_mV;
	static const uInt8  c_u8AdcCoarseGainSpan;
	static const uInt8  c_u8AdcCalRegSpan;
	static const uInt16 c_u16CalDacCount;
	static const uInt16 c_u16DacPhaseLimit;

	static const LONGLONG c_llR100;
	static const LONGLONG c_llR116;
	static const LONGLONG c_llR118;
	static const LONGLONG c_llR120;
	static const LONGLONG c_llR122;
	static const LONGLONG c_llR124;
	static const LONGLONG c_llR134;
	static const LONGLONG c_llR136;
	static const LONGLONG c_llR150;
	static const LONGLONG c_llR158;
	static const LONGLONG c_llR160;
	static const LONGLONG c_llExtTrigRangeScaled;
	static const LONGLONG c_llVref;

	int32   ReadCalibA2D(int32* pi32Mean, int32* pi32MeanSquare = NULL );
	int32   SendCalibDC(int32 i32Target_mV, bool bTerm, int32* pVoltage_uV = NULL, uInt16* pu16InitialCode = NULL );

	int32	GeneratePulseCalib();
	int32	DetectAdcCrossings(uInt16 u16Channel = CS_CHAN_1);
	void    FindAdcOrder( uInt16 u16Channel, int16* pArrayLevel );
	bool    IsOnBoardPllLocked(void);
	bool	IsFpgaPllLocked();
	bool	IsCalPllLocked();

	void	DefaultAdcCalibInfoStructure(void);
	int32	AdjustAdcAlign(void);
	int32	MsCalibrateAdcAlign(void);
	int32	CalibrateAdcAlignSingleChan( uInt16 u16Channel = CS_CHAN_1 );
	int32	MsCalibrateAdcAlignForChannels();
	int32	MsCalibrate4AdcAlignSingleChan(uInt16 u16Channel = CS_CHAN_1);

	int32	SetupForExtTrigCalib( bool bSetup );
	void	ResetCachedState(void);

	void	TraceCalib(uInt32 u32Id, uInt32 u32V1 = 0, uInt32 u32V2 = 0, int32 i32V3 = 0, int32 i32V4 = 0);
	void	TraceCalibEx( DWORD dwTraceId, int nTraceIdMod, uInt16 u16Chan = 0, uInt16 u16AdcPerChan = 0, const void* const pBuf1 = NULL, const void* const pBuf2 = NULL, const void* const pBuf3 = NULL, const void* const pBuf4 = NULL );

	int32	SetupBufferForCalibration( bool bSetup, eCalMode ecMode = ecmCalModeDc );
	int32	SetupForCalibration( bool bSetup, eCalMode ecMode = ecmCalModeDc );
	int32	PrepareForCalibration( bool bSetup );

	int32 SendClockSetting(uInt32 u32Mode, LONGLONG llSampleRate = DEERE_DEFAULT_SAMPLE_RATE, uInt32 u32ExtClk = 0, BOOLEAN bRefClk = FALSE);
	int32 SendCaptureModeSetting(uInt32 u32Mode = CS_MODE_DUAL, uInt16 u16ChannelSingle = CS_CHAN_1 );
	int32 SendCalibModeSetting(uInt16 Channel, eCalMode ecMode = ecmCalModeNormal, uInt32 u32WaitingTime = 100 );
	int32 SendChannelSetting(uInt16 Channel, uInt32 u32InputRange, uInt32 u32Term = DEERE_DEFAULT_TERM, uInt32 u32Impendance = DEERE_DEFAULT_IMPEDANCE, int32 i32Position = 0, uInt32 u32Filter = 0, int32 i32ForceCalib = 0);
	int32 SendTriggerEngineSetting(int32	i32SourceA1,
								    uInt32  u32ConditionA1 = DEERE_DEFAULT_CONDITION,
									int32   i32LevelA1 = 0,
                                    int32	i32SourceB1 = 0,
									uInt32  u32ConditionB1 = 0,
									int32   i32LevelB1 = 0, 
									int32	i32SourceA2 = 0,
									uInt32  u32ConditionA2 = 0,
									int32   i32LevelA2 = 0,
                                    int32	i32SourceB2 = 0,
									uInt32  u32ConditionB2 = 0,
									int32   i32LevelB2 = 0,
									int16	i16AdcSelect = -1);
										
	int32 SendExtTriggerSetting( bool  bExtTrigEnabled = FALSE,
								 int32	i32ExtLevel = 0,
								 uInt32	u32ExtCondition = DEERE_DEFAULT_CONDITION,
								 uInt32	u32ExtTriggerRange =  uInt32(-1), 
								 uInt32	u32ExtTriggerImped =  uInt32(-1), 
								 uInt32	u32ExtCoupling = uInt32(-1));
	
	int32 SendDigitalTriggerSensitivity( uInt16  u16TrigEngine, uInt8 u8Sensitive = DEERE_DEFAULT_TRIG_SENSITIVITY );
	int32 SendExtTriggerLevel( int32 i32Level, bool bRising );
	bool	FastFeCalibrationSettings(uInt16 u16HwChannel, bool bForceSet = false);

	int32	AcquireAndXfer(uInt16 u16HwChannel, int16** ppi16Buffer, uInt32* pu32Size);
	int32	AcquireAndAverage(uInt16 u16HwChannel, int32& ri32Avg_x10);
	int32	AcquireAndAverageCore(uInt16 u16HwChannel, int32* ai32_x10);
	int32   DetermineAdcDataOrder(uInt16 u16HwChannel);
	int32	CalibrateOffset(uInt16 u16HwChannel);
	int32	NullOffset(uInt16 u16HwChannel);
	int32	CompensateOffset(uInt16 u16HwChannel, bool bCoarse);
	int32   DetermineIrFactor(uInt16 u16HwChannel);
	int32	CalibrateGain(uInt16 u16HwChannel);
	int32	CalibratePosition(uInt16 u16HwChannel);
	int32   MeasureAdcGain(uInt16 u16HwChannel, int32* ai32Gain, int32* pi32Target = NULL);
	int32	MatchAdcOffset(uInt16 u16HwChannel, bool bFine);
	int32	MatchAdcGain(uInt16 u16HwChannel, bool bFine);
	int32   TraceCalibrationMatch(uInt16 u16HwChannel );
	int32	CalibrateFrontEnd(uInt16 u16HwChannel);
	bool    isAdcPingPong(void);
	int32   ConfigureDebugCalSrc(void);
	int32   ConfigureAuxIn(bool bExtTrigEnabled);
	int32   ExtTrigEnableReset(void);


	int32	SetDefaultDacCalib( uInt16 u16HwChannel );
	int32	CalculateDualChannelSkewAdc();
	int32	MsCalibrateAdcAlign2ChannelsDual();
	int32	AlignDualChannelAdc(uInt32 u32Pos1, uInt32 u32Pos2);
	int32	SendAdcAlign( eCalMode eMode );
	int16	CalculateLevelForPulseDetection( int16 *pi16CalibPulseBuffer, uInt32 u32FlatLevelSize );
	int32	InCalibrationCalibrate( uInt16 Channel );
	uInt32	GetAdcCoreOrder( uInt16 Channel );
	void	UpdateCoreOrder( uInt16 u16HwChannel, int16 i16DigitalDelay );

	void	GetAlarmStatus(void);
	int32	ClearAlarmStatus( uInt16 u16AlarmSource );
	int32   ReportAlarm(void);
	int32   ConfigureAlarmSource(bool bIncludeAcSrc = false);
	int32	SelectADC(uInt16 u16Core, uInt16 u16Adc, uInt16 u16Addr, uInt16* pNiosAddr = NULL );
	int32   WriteToAdcRegDirect( uInt16 u16Adc, uInt16 u16NiosAddr, uInt8 u8Data );
	int32	WriteToAdcReg(uInt16 u16Core, uInt16 u16Adc, uInt16 u16Addr, uInt8 u8Data, uInt16 u16Delay = 0xFFFF);
	int32   WriteToAdcRegAllChannelCores(uInt16 u16HwChannel, uInt16 u16Addr, uInt8 u8Data, uInt16 u16Delay = 0xFFFF);
	int32   WriteDeltaToAdcRegAllChannelCores(uInt16 u16HwChannel, uInt16 u16Addr, int16 i16Delta, uInt16 u16Delay = 0xFFFF);
	uInt8   ReadAdcRegDirect( uInt16 u16Adc, uInt16 u16NiosAddr);
	int32	ReadAdcReg(uInt16 u16Core, uInt16 u16Adc, uInt16 u16Addr, uInt8* pu8Data, uInt32 u32InputRange = 0 );
	int32   SetAdcDllRange( void );

	int32	WriteToDac( uInt16 u16Addr, uInt16 u16Data, uInt8 u8Delay = 0xff );
	int32   Configure4360(void);
	int32   Configure4302(void);
	int32   Configure4302(uInt16 u16HwChannel);
	int32   SendAcdCorrection(uInt16 u16HwChannel, uInt8 u8Cor);

		
	int32	AcquireData(uInt32 u32IntrEnabled = 0);
	void    MsAbort();

	uInt32	GetDataMaskModeTransfer ( uInt32 u32TxMode );
	uInt32	GetFifoModeTransfer( uInt16 u16UserChannel, uInt32 u32TxMode = 0 );

#ifdef __linux__
	void 	CreateInterruptEventThread();
	void	InitEventHandlers();
	void	ThreadFuncIntr();
#endif

	int32	CreateThreadAsynTransfer();

	int32	SendDpaAlignment( bool bPingpong );
	int32	SendInternalHwExtTrigAlign();
	int16	GetDataPathOffsetAdjustment();

	int32	ResetPLL(void);
	uInt16* GetDacInfoPointer( uInt16 u16DacAddr );
	int32	ReadDac(uInt16 u16Addr, uInt16* pu16Data);
	uInt32	ConvertToHwTimeout( uInt32 u32SwTimeout, uInt32 u32SampleRate );
	uInt8	KenetCoarseGainConvert( uInt8 u8Val, bool bWrite );
	void	ValidateDacCalibTable(void);
	void	DefaultAdcCalibInfoRange( uInt16 u16ZeroBasedChan, uInt32 u32InputRange = 0 );
	bool	IsSquareRec( uInt32 u32Mode, uInt32	u32SegmentCount = 1, LONGLONG i64PreTrigDepth = 0);

	int32   FindCalLutItem( const uInt32 u32Swing, const uInt32 u32Imped, DEERE_CAL_LUT& Item );
	int32   SendCalibDC(const uInt32 u32Imped, const uInt32 u32InputRange, bool bPolarity, int32*  pi32SetLevel_uV = NULL);
	int32   ReadCalibA2D(int32& i32Expected_uV, eCalInputJD eInput, int32* pi32V_uV=NULL, int32* pi32Noise_01LSB = NULL);
	int32   CheckCalibrator(void);

	bool    isAdcForChannel( uInt8 u8AdcNum, uInt16 u16ZeroBasedChannel );
	uInt8   AdcForChannel( uInt16 u16ZeroBasedChannel, uInt8 u8Parity );
	uInt16  ChannelForAdc( uInt16 u16Adc );
	void	FillOutBoardCaps(PCSDEEREBOARDCAPS pCapsInfo);
	void	UpdateAdcCalibInfoStructure(uInt32 u32InputRange, uInt16 u16Core, uInt16 u16Adc, uInt16 u16Addr, uInt8 u8Data);

	int32	SetupForExtTrigCalib_SourceExtTrig();
	int32	SetupForExtTrigCalib_SourceChan1();	
	int32	MsCalibrateSkew();
	int32	ResetMSDecimationSynch();
	int32	ResetMasterSlave();
	int32	SetDigitalDelay( int16 i16Offset );
	int32	MsAdjustDigitalDelay();
	int32	MsCalculateMasterSlaveSkew();
	int32	CsDetectMsZeroCrossAddress(int16 *i16CrossPos, int16 i16BestMatchLevel = 0);
	void    CheckCoerceResult(int32& rResult );
	uInt32	GetSquareEtb();

	void    OutputCalibTraceUsage(void);
	void    OutputDacInfo(void);

	int32	CalibrateAllChannels(void);

	CsDeereDevice *GetCardPointerFromChannelIndex( uInt16 ChannelIndex );
	CsDeereDevice *GetCardPointerFromBoardIndex( uInt16 BoardIndex );
	CsDeereDevice *GetCardPointerFromTriggerIndex( uInt16 TriggerIndex );
	CsDeereDevice *GetCardPointerFromTriggerSource( int32 i32TriggerSoure );
	int32  GetFreeTriggerEngine( int32	i32TrigSource, uInt16 *u16TriggerEngine );

	uInt16 NormalizedChannelIndex( uInt16 u16ChannelIndex );
	int32  NormalizedTriggerSource( int32 i32TriggerSource );

	int32	SendSegmentSetting(uInt32 u32SegmentCount, int64 i64PostDepth, int64 i64SegmentSize, int64 i64HoldOff );

	void ReadVersionInfo(uInt8	u8ImageId = 0);

	//----- Read/Write the Add-On Board Flash 
	int32	ReadHardwareInfoFromEeprom(BOOLEAN bChecksum = TRUE);
	int32	ReadTriggerTimeStampEtb( uInt32 StartSegment, int64 *pBuffer, uInt32 u32NumOfSeg );

	int32	CsLoadFpgaImage( uInt8 u8ImageId );
	void	SetupHwAverageMode( bool bFullRes = true, uInt32 u32ScaleFactor = 1 );
	int32	ProcessSoftwareAveraging( int64 i64StartAddress, uInt16 u16Channel, int32 *pi32ResultBuffer, uInt32 u32SamplesInBuffer );
	int32	ReadCommonRegistryOnBoardInit();
	int32	ReadCommonRegistryOnAcqSystemInit(void);
	void	ReadCalibrationSettings(void* pParams);
	void	ReadConfigurationRegistryInformation(void);

	int32	ReadAndValidateCsiFile( uInt32 u32BbHwOpt, uInt32 u32AoHwOpt );
	int32	ReadAndValidateFwlFile(void);
	void	CsSetDataFormat( _CSDATAFORMAT csFormat = formatDefault );
	int32	ReadCalibInfoFromCalFile(void);
	void	AdjustedHwSegmentParameters( uInt32 u32Mode, uInt32* pu32SegmentCount, int64* pi64SegmentSize, int64* pi64Depth );
	int32	CommitNewConfiguration( PCSACQUISITIONCONFIG pAcqCfg,
									PARRAY_CHANNELCONFIG pAChanCfg,
									PARRAY_TRIGGERCONFIG pATrigCfg );
	void	DetectMasterSlave(int16 *i16NumMasterSlaveSystems, int16 *i16NumStandAloneSystems);
	int32	SendMasterSlaveSetting(void);

	BOOLEAN	ReadMasterSlaveResponse(void);
	int32	DetectSlaves(void);
	uInt32	CsGetPostTriggerExtraSamples( uInt32 u32Mode );
	uInt32	GetSegmentPadding(uInt32 u32Mode);
	uInt32	GetHwTriggerAddress( uInt32 u32Segment, bool* bRollOver = NULL );
	
	int32 SetClockOut( eClkOutMode ecomSet= eclkoNone, bool bForce = false );

	ePulseMode  GetPulseOut(void){return epmNone;}
	eClkOutMode GetClockOut(void);

	int32 WriteToSelfTestRegister( uInt16 SelfTestMode = CSST_DISABLE );
	int32 InitRegisters(bool bForce = false);
	int32 InitRegisters0(bool bForce = false);
	void AddonReset();


	int32	EnableMsBist(bool bEnable);
	int32	TestMsBridge();
	int32	SetRoleMode( eRoleMode ermSet );
	int32	ConfiureTriggerBusyOut(bool bTrigOutEn = false, bool bBusyOutEn = false);

	int32	BaseBoardConfigReset(uInt8 u8ImageId);
	uInt16	ReadTriggerStatus(void);
	uInt16	ReadBusyStatus(void);


	void	AssignBoardType();
	void	AssignAcqSystemBoardName( PCSSYSTEMINFO pSysInfo );
	int32	AcqSystemInitialize();

	int32	CheckRequiredFirmwareVersion(void);
	int32	CheckMasterSlaveFirmwareVersion(void);

	bool	CompareFwVersion(FPGA_FWVERSION Ver1, FPGA_FWVERSION Ver2);
	int32	RemoveDeviceFromUnCfgList();

	int32	SendCoreSkewAdjust( const uInt8 u8Adc, const uInt8 u8Skew );
	int32	SendAdcPhaseAdjust( const uInt8 u8Adc, const uInt16 u16Val );
	int32	MatchAllAdcCores( const uInt16 u16SysChanIndex, const double dSr,  const double dFreq  );
	int32	MeasureCalResult( const uInt16 u16SysChanIndex, const double dSr, const double dFreq, void* pBuffer, SINEFIT3* pSol, double* pdOff, double* pdAmp, double* pdPh, double* pdSnr = NULL);
	int32	AcquireAndRead( const uInt16 u16ChanIndex, void* const pBuff, const uInt32 u32ReadLen );
	int32	MsMatchAdcPhaseAllChannels();
	int32	RematchGainAndOffset( const uInt16 u16ChanIndex, const double dSr,  const double dFreq );
	int32   ReadCoreGainOffsetAdjust(const uInt16 u16ChanIndex, const uInt8 u8Adc, const uInt8 u8Core, bool bGain, bool bFine, uInt16 *u16Value );
	int32	VerifyCalibration( const uInt16 u16ChanIndex, const double dSr,  const double dFreq );
	int32	SendCoreGainOffsetAdjust( const uInt16 u16ChanIndex, const uInt8 u8Adc, const uInt8 u8Core, bool bGain, bool bFine, const uInt16 u16Value );
	int32	DetermineCoreOrder( const uInt16 u16ChanIndex, const double dSr,  const double dFreq, void* pBuffer, SINEFIT3* pSol, double* pdOff, double* pdAmp, double* pdPh, uInt8 u8AdcIx[DEERE_ADC_COUNT][DEERE_ADC_CORES]);

	uInt32 AddonReadFlashByte(uInt32 Addr)
	{
		UNREFERENCED_PARAMETER(Addr);
		ASSERT(1);
		return (uInt32) CS_FUNCTION_NOT_SUPPORTED;
	};

	void AddonWriteFlashByte(uInt32 Addr, uInt32 Data)
	{
		UNREFERENCED_PARAMETER(Addr);
		UNREFERENCED_PARAMETER(Data);
		ASSERT(1);
	};

	uInt32 AddonResetFlash(void)
	{
		ASSERT(1);
		return (uInt32) CS_FUNCTION_NOT_SUPPORTED;
	};

public:
	int32	CsAcquisitionSystemInitialize( uInt32 u32InitFlag );
	int32	CsGetParams( uInt32 u32ParamId, PVOID pUserBuffer, uInt32 u32OutSize = 0);
	int32	CsSetParams( uInt32 u32ParamId, PVOID pInBuffer, uInt32 u32InSize = 0, uInt32 u32CommitFlag = 0 );
	int32	CsStartAcquisition();
	int32	CsAbort();
	void	CsForceTrigger();
	int32	CsDataTransfer( in_DATA_TRANSFER *InDataXfer );
	int32	CsDataTransferEx( io_DATA_TRANSFER_EX *DataXferEx );
	uInt32	CsGetAcquisitionStatus();
	int32	CsRegisterEventHandle(in_REGISTER_EVENT_HANDLE &InRegEvnt );
	void	CsAcquisitionSystemCleanup();
	void	CsSystemReset(BOOL bHard_reset);
	int32	CsForceCalibration();
	int32	CsGetAsyncDataTransferResult( uInt16 u16Channel,CSTRANSFERDATARESULT *pTxDataResult, BOOL bWait );
	int32	CsGetAcquisitionSystemCount( BOOL bForceInd, uInt16 *pu16SystemFound );
	int32   CsGetBoardsInfo( PARRAY_BOARDINFO pABoardInfo );
	int32	CsGetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg );
	int32   CsGetTriggerConfig( PARRAY_TRIGGERCONFIG pATriggerCfg );
	int32	CsGetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg );
	int32	CsValidateAcquisitionConfig( PCSACQUISITIONCONFIG, uInt32 u32Coerce );
	int32	DeviceInitialize();
	int32	CsAutoUpdateFirmware();
	void	SetDefaultAuxIoCfg();
	void	RetryChangeSkew();

	EVENT_HANDLE	GetEventTxDone() { return m_DataTransfer.hAsynEvent; }
	EVENT_HANDLE	GetEventSystemCleanup() { return m_pSystem->hAcqSystemCleanup; }
	EVENT_HANDLE	GetEventTriggered() { return m_pSystem->hEventTriggered; }
	EVENT_HANDLE	GetEventEndOfBusy() { return m_pSystem->hEventEndOfBusy; }
	EVENT_HANDLE	GetEventAlarm() { return m_pSystem->hEventAlarm; }
	void	SetStateTriggered() { GetBoardNode()->bTriggered = true; }
	void	SetStateAcqDone() { GetBoardNode()->bBusy = false; }
	void	OnAlarmEvent();
	void	SignalEvents( BOOL bAquisitionEvent = TRUE );
	void	SignalEventsExpert(void);

	void	AsynDataTransferDone();

    void *operator new(size_t  size);//implicitly declared as a static member function

	int32 MsInitializeDevIoCtl();

#ifdef USE_SHARED_CARDSTATE
	int32	InitializeDevIoCtl();
#endif

	//----- Member Functions
};


// Prototype for functions
unsigned int WINAPI ThreadFuncIntr( LPVOID lpParam );
unsigned int WINAPI ThreadFuncTxfer( LPVOID lpParam );

#endif
