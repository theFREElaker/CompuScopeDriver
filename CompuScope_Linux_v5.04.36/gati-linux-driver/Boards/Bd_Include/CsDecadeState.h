#ifndef _DECADE_CARDSTATE_h_
#define	_DECADE_CARDSTATE_h_


#include "CsDrvStructs.h"
#include "DrvCommonType.h"
#include "CsDecadeCapsInfo.h"
#include "CsDecadeCal.h"
#include "CsDecadeRegs.h"

// Decade Only interrupts
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

//----- The following structure groups all the registers of the Add-On
typedef struct _BUNNY_ADC_REGS
{
	uInt16 Cfg;
	uInt16 Off1;
	uInt16 Gain1;
	uInt16 Off2;
	uInt16 Gain2;
	uInt16 DES;
	uInt16 FineDly;
	uInt16 CoarseDly;
} CSRABBIT_ADC_REGS, *PCSRABBIT_ADC_REGS;


//Decade Frontend setup
typedef union _DECADE_FRONTEND_SETUP
{
    uInt32	u32Val;
    struct
	{
//      --Channel setup
        uInt32 Att_6Db	          :1;
        uInt32 Att_20Db           :1;
        uInt32 PreBypass          :1;
//      --Channel selection
        uInt32 Channel         	  :2;
    }bits;
} DECADE_FRONTEND_SETUP, *PDECADE_FRONTEND_SETUP;


typedef struct _DECADE_ADDON_REGS
{
	uInt32	u32FeIndex[DECADE_CHANNELS];
	DECADE_FRONTEND_SETUP	u32FeSetup[DECADE_CHANNELS];
	uInt32	u32Impendance[DECADE_CHANNELS];
	int32	i32Position_mV[DECADE_CHANNELS];
	uInt16  u16VSC;

//	uInt16  u16ClkSelect1;
//	uInt16  u16ClkSelect2;
//	uInt16  u16ClkOut;
	uInt16	u16ExtTrig;
	uInt16	u16ExtTrigEnable;
	uInt32	u32ClkFreq;
	uInt32  u32Decimation;
	uInt16  u16ClockDiv;
	uInt16	u16AdcRegister0;
	uInt16	u16Calib;
	uInt16	u16SelfTest;
	uInt16	u16PingPong;
	CSRABBIT_ADC_REGS AdcReg;
	uInt16	u16AdcCtrl;

} DECADE_ADDON_REGS, *PDECADE_ADDON_REGS;


typedef struct _DECADE_BASEBOARD_REGS
{
	uInt32		u32TrigTimeOut;
	uInt32		u32ExtTrigSetting;
	uInt16		u32TrigLevelA1;
	uInt16		u32TrigSlopeA1;
	uInt16		u32TrigLevelB1;
	uInt16		u32TrigSlopeB1;
	uInt16		u32TrigLevelA2;
	uInt16		u32TrigSlopeA2;
	uInt16		u32TrigLevelB2;
	uInt16		u32TrigSlopeB2;

} DECADE_BASEBOARD_REGS, *PDECADE_BASEBOARD_REGS;

typedef struct _DecadeClockSet
{
	LONGLONG llSampleRate;
	uInt32   u32ClockFrequencyMhz;
	uInt32   u32Decimation;
	uInt16   u16ClockDivision;
	uInt16   u16PingPong;

} DECADE_SAMPLE_RATE_INFO, *PDECADE_SAMPLE_RATE_INFO;


typedef struct _FRONTEND_DECADE_INFO
{  
	uInt32	u32Swing_mV;
	uInt32	u32RegSetup;
} FRONTEND_DECADE_INFO, *PFRONTEND_DECADE_INFO;


// Decade Device Extension version. 
// When ever the structure is changed, the version should be increased.
#define DECADE_DEVEXT_VERSION				7

typedef struct	_CSDECADE_DEVEXT
{
	uInt32				u32Version;			// Version of this structure. In the version
	uInt32				u32Size;			// size of this structure
	uInt16				u16DeviceId;
	uInt16				u16CardNumber;		// Card number in M/S system 1, 2, 3 ...
	uInt8				u8ImageId;			
	uInt8				u8MsCardId;			// M/S card Id, 0 is Master, 1 to 7 are slaves
	uInt8				u8MsTriggMode;		// Master/Slave trigger mode
	eRoleMode			ermRoleMode;
	eClkOutMode			eclkoClockOut;
	uInt16				u16TrigOutMode;
	BOOLEAN				bDisableTrigOut;			// Disable trigger out
	BOOLEAN				bForceIndMode;				// Force Independent Mode
	BOOLEAN				bPciExpress;
	BOOLEAN				bVirginBoard;				// PciExpress virgin card
	BOOLEAN				bNeedFullShutdown;			// Full Shutdown then Reboot sequence is needed
	BOOLEAN				bEnobCalibRequired;			// Calibrate for improving ENOB
	BOOLEAN				bClockPingpPong_Calibrated;	// LVDS and I/Q Calibrated for pingong

	uInt32				u32DmaDescriptorSize;
	uInt32				u32ActualSegmentCount;
	uInt32				u32DefaultRange;				// Default input range
	uInt32				u32DefaultExtTrigRange;			// Default Ext Trigger range
	uInt32				u32DefaultExtTrigCoupling;		// Default Ext Trigger coupling
	uInt32				u32DefaultExtTrigImpedance;		// Default Ext Trigger impedance
	uInt32				u32IntConfig;					// Interrupt to be used for acquisition
	uInt32				u32AcqMode;					// Acquisition mode.( acquisition with or w/o eXpert )


	// Driver current boot and config location. Boot=0, Reg=1, Expert1=2, Expert2=3
	// From the user point of view Boot=-1, Reg=0, Expert1=1, Expert2=2
	int32				i32CurrentBootLocation;		
	int32				i32ConfigBootLocation;	

	eTrigEnableMode		eteTrigEnable;

	uInt32				u32ProcId;					// owner process Id
	BOOLEAN				bCardStateValid;
	BOOLEAN				bSafeBootImageActive;		// card is operated with the Safe Boot image
	BOOLEAN				bNeedFullReset;				// Full reset is needed
	BOOLEAN				bAddOnRegistersInitialized;
	BOOLEAN				bCalNeeded[DECADE_CHANNELS];	// Front end Cal needed
	BOOLEAN				bExtTrigCalib;				// Calibration for External required
	BOOLEAN		        bAddonInit;					 // Add-on board ititialized
	BOOLEAN				bUseDacCalibFromEEprom;
	BOOLEAN				bFastCalibSettingsDone[DECADE_CHANNELS];
	BOOLEAN				bVarCap;
	BOOLEAN				bAdcLvdsAligned;
	BOOLEAN				bExtTriggerAligned;
	BOOLEAN				bOverVoltage;				
	BOOLEAN				bMultiSegmentTransfer;		// Multiple records MultiSegments transfer mode

	CS_FW_VER_INFO		VerInfo;
	CSDECADE_CAL_INFO	CalibInfoTable;		// Active Calibration Info Table

	DECADE_SAMPLE_RATE_INFO	SrInfoTable[DECADE_MAX_SR_COUNT];
	uInt32					u32SrInfoSize;
	uInt32					u32SwingTable[DECADE_IMPED][DECADE_MAX_RANGES];
	uInt32					u32FeSetupTable[DECADE_IMPED][DECADE_MAX_RANGES];
	uInt32					u32SwingTableSize[DECADE_IMPED];


	FRONTEND_DECADE_INFO		FeInfoTable[DECADE_MAX_RANGES];
	uInt32						u32FeInfoSize;

	//----- Baseboard and Add-On regsiters
	DECADE_TRIG_IO_REG			TrigIoReg;						// Add-on version 1.06, 1.07
	DECADE_CLOCK_REG			ClockReg;

	DECADE_ADDON_REGS			AddOnReg;
	DECADE_BASEBOARD_REGS		BaseBoardRegs;		
	FRONTEND_DECADE_DAC_INFO	DacInfo[DECADE_CHANNELS][DECADE_MAX_FE_SET_INDEX][DECADE_MAX_RANGES];
	
	CSACQUISITIONCONFIG			AcqConfig;
	CS_CHANNEL_PARAMS			ChannelParams[DECADE_CHANNELS];
	CS_TRIGGER_PARAMS			TriggerParams[1+2*DECADE_CHANNELS];

	PLXBASE_DATA				PlxData;

}CSDECADE_DEVEXT, *PCSDECADE_DEVEXT;

#pragma pack ()

typedef CSDECADE_DEVEXT CSCARD_STATE;
typedef PCSDECADE_DEVEXT PCSCARD_STATE;

#endif	//_DECADE_CARDSTATE_h_
