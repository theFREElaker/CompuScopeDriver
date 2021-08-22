#ifndef _FASTBALL_CARDSTATE_h_
#define	_FASTBALL_CARDSTATE_h_

#include "CsDrvStructs.h"
#include "DrvCommonType.h"
#include "CsBunnyCapsInfo.h"
#include "CsBunnyCal.h"

#pragma pack (8)

typedef	struct _CS_FASTBL_NODE
{
	int64	i64MaxMemory;				// Memory size of the system. (Physical board limitation)
										// Native parameters: express in samples
	uInt32	MemorySize;

	BOOLEAN	bBusy;
	BOOLEAN	bTriggered;
	uInt32	DpcIntrStatus;			// Keep track of which interrupt received for DPC

	uInt16	u32BoardType;				// ID that identifies the card type (ex :CS14100_BOARDTYPE )

	uInt32	u32MemSizeKb;					// Memory size in KB (value read from eeprom)
										// Native parameters: express in samples

	uInt32	u32SerNum;
	uInt32	u32HardwareOptionsB;		// "BaseBoard" Hw options
	uInt32	u32HardwareOptionsA;		// "Addon" Hw options
	uInt32	u32HardwareVersion;
	uInt32	u32FirmwareVersion[3];		// Raw firmware version
	uInt32	u32FirmwareOptions[3];
	CSFWVERSION	u32UserFwVersionB[3];	// Base board user firmware version converted from raw one
	int64	i64ExpertPermissions;

	uInt8	NbTriggerMachine;
	uInt8	NbAnalogChannel;
	uInt8	NbDigitalChannel;

} CS_FASTBL_NODE, *PCS_FASTBL_NODE;

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
} BUNNY_ADC_REGS, *PBUNNY_ADC_REGS;

typedef struct _CSRABIT_ADDON_REGS
{
	uInt32	u32FeIndex[BUNNY_CHANNELS];
	uInt32	u32FeSetup[BUNNY_CHANNELS];
	uInt32	u32Impendance[BUNNY_CHANNELS];
	int32	i32Position_mV[BUNNY_CHANNELS];

	uInt16  u16ClkSelect1;
	uInt16  u16ClkSelect2;
	uInt16  u16ClkOut;
	uInt16	u16ExtTrig;
	uInt16	u16ExtTrigEnable;
	uInt32	u32ClkFreq;
	uInt32  u32Decimation;
	uInt16  u16ClockDiv;
	uInt16	u16Option;
	uInt16	u16Calib;
	uInt16	u16SelfTest;
	uInt16	u16PingPong;
	BUNNY_ADC_REGS AdcReg;
	uInt16	u16AdcCtrl;
	uInt16	u16MasterSlave;
	uInt16  u16MsBist1;
	uInt16  u16MsBist2;

} CSRABIT_ADDON_REGS, *PCSRABIT_ADDON_REGS;

typedef struct _FasBallClockSet
{
	LONGLONG llSampleRate;
	uInt32   u32ClockFrequencyMhz;
	uInt32   u32Decimation;
	uInt16   u16ClockDivision;
	uInt16   u16PingPong;

	// Trigger Address Offset
	int8		i8TAO_Dual;
	int8		i8TAO_Single;

} FASTBL_SAMPLERATE_INFO, *PFASTBL_SAMPLERATE_INFO;


typedef struct	_FASTBALL_CARDSTATE
{
	uInt32				u32Size;			// size of this structure
	uInt16				u16DeviceId;
	PLDACORE_DATA		CoreData;

	uInt16				u16CardNumber;		// Card number in M/S system 1, 2, 3 ...
	uInt8				u8MsCardId;			// M/S card Id, 0 is Master, 1 to 7 are slaves
	uInt8				u8MsTriggMode;		// Master/Slave trigger mode
	uInt8				u8SlaveConnected;	// Position of slaves cards
	uInt8				u8BadFirmware;		// The card has wrong or outdate FPGA firmware.
	eRoleMode			ermRoleMode;
	eClkOutMode			eclkoClockOut;
	uInt16				u16TrigOutMode;
	BOOLEAN				bDisableTrigOut;			// Disable trigger out
	BOOLEAN				bForceIndMode;				// Force Independent Mode
	BOOLEAN				bPciExpress;

	uInt32				u32ActualSegmentCount;
	uInt32				u32CsiTarget;
	uInt32				u32DefaultRange;				// Default input range
	uInt32				u32DefaultExtTrigRange;			// Default Ext Trigger range
	uInt32				u32DefaultExtTrigCoupling;		// Default Ext Trigger coupling
	uInt32				u32DefaultExtTrigImpedance;		// Default Ext Trigger impedance

	eTrigEnableMode		ExtTrigEn;

	CS_FASTBL_NODE		CsBoard;
	INITSTATUS			InitStatus;
	uInt32				u32ProcId;					// owner process Id
	BOOLEAN				bCardStateValid;
	BOOLEAN				bActive;						// card is in active state (in use)
	BOOLEAN				bNeedFullReset;					// Full reset is needed
	BOOLEAN				bAddOnRegistersInitialized;

	BOOLEAN				bHighBandwidth;					// Bunny High Banwidth cards
	BOOLEAN				bCalNeeded[BUNNY_CHANNELS];
	BOOLEAN				bForceCal[BUNNY_CHANNELS];
	BOOLEAN				bAllowExtClk;
	BOOLEAN				b4GAdc;
	BOOLEAN				b3GAdc;
	BOOLEAN				bxx20Adc;
	BOOLEAN				bExtClkOverClocking;
	BOOLEAN				bFwExtTrigCalibDone;
	BOOLEAN             bLowExtTrigCalib;
	BOOLEAN				bMasterSlaveCalib;		// Calibration for Master/Slave
	BOOLEAN				bMsDecimationReset;		// Decimation reset for Master/Slave is required
	BOOLEAN				bExtTrigCalib;			// Calibration for External required
	BOOLEAN				bEco10;					// Eco 10 for M/S
	BOOLEAN				bMemoryLess;			// Memory less card
	BOOLEAN				bUseDacCalibFromEEprom;
	
	uInt32				u32VerifyTrigOffsetRateIx;
	uInt8				u8VerifyTrigOffsetModeIx;
	BOOLEAN				bVerifyTrigOffsetSquare;


	// Calibration ofr  M/S and Ext trigger
	int16				i16ExtTrigSkew;
	int16				i16MasterSlaveSkew;
	uInt32				u32AdcClock;				// The clock of the Adc used for decimation
	uInt32				u32SampleClock;				// The base sample clock used for decimation

	CS_FW_VER_INFO		VerInfo;

	//----- Add-On regsiters
	CSRABIT_ADDON_REGS	AddOnReg;
	BUNNYCHANCALIBINFO  DacInfo[BUNNY_CHANNELS][BUNNY_MAX_RANGES];
	BUNNY_CAL_INFO		CalibInfoTable;			// Active Calibration Info Table
	char				szFpgaFileName[MAX_FILENAME_LENGTH];		// FPGA file name

	CSACQUISITIONCONFIG AcqConfig;
	CS_CHANNEL_PARAMS	ChannelParams[BUNNY_CHANNELS];
	CS_TRIGGER_PARAMS	TriggerParams[1+2*BUNNY_CHANNELS];

}FASTBALL_CARDSTATE, *PFASTBALL_CARDSTATE;

#pragma pack ()

typedef FASTBALL_CARDSTATE CSCARD_STATE;
typedef PFASTBALL_CARDSTATE PCSCARD_STATE;

#endif	//_DEERE_CARDSTATE_h_
