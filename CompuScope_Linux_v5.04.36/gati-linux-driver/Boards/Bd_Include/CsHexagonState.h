#ifndef _HEXAGON_CARDSTATE_h_
#define	_HEXAGON_CARDSTATE_h_


#include "CsDrvStructs.h"
#include "DrvCommonType.h"
#include "CsHexagonCapsInfo.h"
#include "CsHexagonCal.h"
#include "CsHexagonRegs.h"

#define	INFINITE_U32				((uInt32)(-1))
#define	INFINITE_U16				((uInt16)(-1))

// Hexagon Only interrupts
#define ACQUISITION_DONE_INTR 	0x0200		//Bit9
#define DMA_DONE_INTR			0x0800		//Bit11
#define TRIGGER_INTR			0x8000		//Bit15
#define FIRST_TRIGGER_INTR		0x80000		//Bit19

#define DMA_TIMEOUT_INTR		0x200000	//Bit21
#define STREAM_OVERFLOW_INTR	0x400000	//Bit22
#define OVER_TEMPERATURE_INTR	0x800000	//Bit23
#define OVER_VOLTAGE_INTR		0x1000000	//Bit24
#define MULREC_AVG_DONE_INTR	0x2000000	//Bit25
#define FIRST_TRIGGER_AVG_INTR	0x4000000	//Bit26


#pragma pack (8)


typedef struct _HEXAGON_ADDON_REGS
{
	uInt32	u32FeIndex[HEXAGON_CHANNELS];
	uInt32	u32Impendance[HEXAGON_CHANNELS];
	int32	i32Position_mV[HEXAGON_CHANNELS];

	uInt16	u16ExtTrigLevel;
	uInt16	u16ExtTrigEnable;
	uInt32	u32ClkFreqMHz;
	uInt32  u32Decimation;
	uInt16	u16Calib;
	uInt16	u16SelfTest;

} HEXAGON_ADDON_REGS, *PHEXAGON_ADDON_REGS;


typedef struct _HEXAGON_BASEBOARD_REGS
{
	uInt32		u32RecordCount;
	uInt32		u32AverageCount;
	uInt32		u32SegmentLength;
	uInt32		u32PostTrigDepth;
	uInt32		u32PreTrigDepth;
	uInt32		u32TrigDelay;
	uInt32		u32TrigHoldoff;

	DIGI_TRIG_CTRL_REG	TrigCtrl_Reg;
	DIGI_TRIG_CTRL_REG	TrigCtrl_Reg_2;
	int16		i16TrigLevelA1;
	uInt16		u16TrigSensitiveA1;
	int16		i16TrigLevelB1;
	uInt16		u16TrigSensitiveB1;
	int16		i16TrigLevelA2;
	uInt16		u16TrigSensitiveA2;
	int16		i16TrigLevelB2;
	uInt16		u16TrigSensitiveB2;

	int16		i16TrigLevelA3;
	uInt16		u16TrigSensitiveA3;
	int16		i16TrigLevelB3;
	uInt16		u16TrigSensitiveB3;
	int16		i16TrigLevelA4;
	uInt16		u16TrigSensitiveA4;
	int16		i16TrigLevelB4;
	uInt16		u16TrigSensitiveB4;
	
	uInt32		u32TrigTimeOut;
	uInt32		u32ExtTrigSetting;

} HEXAGON_BASEBOARD_REGS, *PHEXAGON_BASEBOARD_REGS;

typedef struct _DecadeClockSet
{
	LONGLONG llSampleRate;
	uInt32   u32ClockFrequencyMhz;
	uInt32   u32Decimation;

} HEXAGON_SAMPLE_RATE_INFO, *PHEXAGON_SAMPLE_RATE_INFO;


typedef struct _FRONTEND_HEXAGON_INFO
{  
	uInt32	u32Swing_mV;
	uInt32	u32RegSetup;
} FRONTEND_HEXAGON_INFO, *PFRONTEND_HEXAGON_INFO;


// Hexxagon Device Extension version. 
// When ever the structure is changed, the version should be increased.
#define HEXAGON_DEVEXT_VERSION				2

typedef struct	_CSHEXAGON_DEVEXT
{
	uInt32				u32Version;			// Version of this structure. In the version
	uInt32				u32Size;			// size of this structure
	uInt16				u16DeviceId;
	uInt16				u16CardNumber;		// Card number in M/S system 1, 2, 3 ...
	uInt8				u8MsCardId;			// M/S card Id, 0 is Master, 1 to 7 are slaves
	eRoleMode			ermRoleMode;		
	eClkOutMode			eclkoClockOut;
	BOOLEAN				bDisableTrigOut;			// Disable trigger out
	BOOLEAN				bForceIndMode;				// Force Independent Mode
	BOOLEAN				bPciExpress;
	BOOLEAN				bVirginBoard;				// PciExpress virgin card
	BOOLEAN				bNeedFullShutdown;			// Full Shutdown then Reboot sequence is needed
	BOOLEAN				bEnobCalibRequired;				// PciExpress virgin card
	uInt32				u32ActualSegmentCount;
	uInt32				u32IntConfig;					// Interrupt to be used for acquisition
	uInt32				u32AcqMode;						// Acquisition mode.( acquisition with or w/o eXpert )


	// Driver current boot and config location. Boot=0, Reg=1, Expert1=2, Expert2=3
	// From the user point of view Boot=-1, Reg=0, Expert1=1, Expert2=2
	int32				i32CurrentBootLocation;		
	int32				i32ConfigBootLocation;	

	eTrigEnableMode		eteTrigEnable;

	uInt32				u32ProcId;					// owner process Id
	BOOLEAN				bCardStateValid;
	BOOLEAN				bSafeBootImageActive;		// card is operated with the Safe Boot image
	BOOLEAN				bAddOnRegistersInitialized;
	BOOLEAN				bCalNeeded[HEXAGON_CHANNELS];	// Front end Cal needed
	BOOLEAN				bExtTrigCalib;				// Calibration for External required
	BOOLEAN		        bAddonInit;					 // Add-on board ititialized
	BOOLEAN				bUseDacCalibFromEEprom;
	BOOLEAN				bFastCalibSettingsDone[HEXAGON_CHANNELS];		// Not used. To be removed ????
	BOOLEAN				bExtTriggerAligned;
	BOOLEAN				bAdcDcLevelFreezed;
	BOOLEAN				bOverVoltage;				
	BOOLEAN				bMultiSegmentTransfer;		// Multiple records MultiSegments transfer mode
	BOOLEAN				bPresetDCLevelFreeze;		// Preset ADC Level Freeze before Calibrate.


	CS_FW_VER_INFO			VerInfo;
	CSHEXAGON_CAL_INFO		CalibInfoTable;		// Active Calibration Info Table

	HEXAGON_SAMPLE_RATE_INFO	SrInfoTable[HEXAGON_MAX_SR_COUNT];
	uInt32					u32SrInfoSize;
	uInt32					u32SwingTable[HEXAGON_IMPED][HEXAGON_MAX_RANGES];		// Not used. To be removed
	uInt32					u32SwingTableSize[HEXAGON_IMPED];


	FRONTEND_HEXAGON_INFO		FeInfoTable[HEXAGON_MAX_RANGES];					// Only use u32Swing_mV. Can be cleanup with u32SwingTable ???
	uInt32						u32FeInfoSize;

	//----- Baseboard and Add-On regsiters
	HEXAGON_TRIG_IO_REG			TrigIoReg;
	HEXAGON_CLOCK_REG			ClockReg;
	HEXAGON_CLK_CTRL_REG		ClkCtrlReg;	

	HEXAGON_ADDON_REGS			AddOnReg;
	HEXAGON_BASEBOARD_REGS		BaseBoardRegs;		
	FRONTEND_HEXAGON_DAC_INFO	DacInfo[HEXAGON_CHANNELS][HEXAGON_MAX_FE_SET_INDEX][HEXAGON_MAX_RANGES];
	
	CSACQUISITIONCONFIG			AcqConfig;
	CS_CHANNEL_PARAMS			ChannelParams[HEXAGON_CHANNELS];
	CS_TRIGGER_PARAMS			TriggerParams[1+2*HEXAGON_CHANNELS];

	PLXBASE_DATA				PlxData;

}CSHEXAGON_DEVEXT, *PCSHEXAGON_DEVEXT;

#pragma pack ()

typedef CSHEXAGON_DEVEXT CSCARD_STATE;
typedef PCSHEXAGON_DEVEXT PCSCARD_STATE;

#endif	//_HEXAGON_CARDSTATE_h_
