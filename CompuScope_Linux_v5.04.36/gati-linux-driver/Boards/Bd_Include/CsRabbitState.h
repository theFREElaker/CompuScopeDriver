#ifndef _RABBIT_CARDSTATE_h_
#define	_RABBIT_CARDSTATE_h_


#include "CsDrvStructs.h"
#include "DrvCommonType.h"
#include "CsBunnyCapsInfo.h"
#include "CsBunnyCal.h"

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
	CSRABBIT_ADC_REGS AdcReg;
	uInt16	u16AdcCtrl;
	uInt16	u16MasterSlave;
	uInt16  u16MsBist1;
	uInt16  u16MsBist2;

} CSRABIT_ADDON_REGS, *PCSRABIT_ADDON_REGS;

typedef struct _RabitClockSet
{
	LONGLONG llSampleRate;
	uInt32   u32ClockFrequencyMhz;
	uInt32   u32Decimation;
	uInt16   u16ClockDivision;
	uInt16   u16PingPong;

} RABBIT_SAMPLE_RATE_INFO, *PRABBIT_SAMPLE_RATE_INFO;


typedef struct _FRONTEND_RABBIT_INFO
{                           // 0     1
	uInt32	u32Swing_mV;
	uInt32	u32RegSetup;
} FRONTEND_RABBIT_INFO, *PFRONTEND_RABBIT_INFO;

typedef struct	_RABBIT_CARDSTATE
{
	uInt32				u32Size;			// size of this structure
	uInt16				u16DeviceId;
	uInt16				u16CardNumber;		// Card number in M/S system 1, 2, 3 ...
	uInt8				u8ImageId;			
	uInt8				u8MsCardId;			// M/S card Id, 0 is Master, 1 to 7 are slaves
	uInt8				u8MsTriggMode;		// Master/Slave trigger mode
	uInt8				u8SlaveConnected;	// Position of slaves cards
	eRoleMode			ermRoleMode;
	eClkOutMode			eclkoClockOut;
	uInt16				u16TrigOutMode;
	BOOLEAN				bDisableTrigOut;			// Disable trigger out
	BOOLEAN				bForceIndMode;				// Force Independent Mode
	BOOLEAN				bPciExpress;
	BOOLEAN				bVirginBoard;				// PciExpress virgin card

	uInt32				u32ActualSegmentCount;
	uInt32				u32DefaultRange;				// Default input range
	uInt32				u32DefaultExtTrigRange;			// Default Ext Trigger range
	uInt32				u32DefaultExtTrigCoupling;		// Default Ext Trigger coupling
	uInt32				u32DefaultExtTrigImpedance;		// Default Ext Trigger impedance
	uInt32				u32VerifyTrigOffsetRateIx;
	uInt8				u8VerifyTrigOffsetModeIx;
	int16				i16MasterSlaveSkew;
	int16				i16ExtTrigSkew;

	eTrigEnableMode		eteTrigEnable;

	uInt32				u32ProcId;					// owner process Id
	BOOLEAN				bCardStateValid;
	BOOLEAN				bActive;						// card is in active state (in use)
	BOOLEAN				bSafeBootImageActive;			// card is operated with the Safe Boot image
	BOOLEAN				bNeedFullReset;					// Full reset is needed
	BOOLEAN				bAddOnRegistersInitialized;
	BOOLEAN				bImage0BootSuccess;
	BOOLEAN				bLAB11G;						// Labcyte card. The channel 2 become the external trigger input
	BOOLEAN				bHighBandwidth;					// Bunny High Banwidth cards
	BOOLEAN				bCalNeeded[BUNNY_CHANNELS];
	BOOLEAN				bSonoscanCalibOnStartUpDone;
	BOOLEAN				bFwExtTrigCalibDone;
	BOOLEAN				bMasterSlaveCalib;		// Calibration for Master/Slave
	BOOLEAN				bMsDecimationReset;		// Decimation reset for Master/Slave is required
	BOOLEAN				bExtTrigCalib;			// Calibration for External required
	BOOLEAN				bEco10;					// Eco 10 for M/S
	BOOLEAN				bMemoryLess;			// Memory less card
	BOOLEAN		        bAddonInit;             // Add-on board ititialized
	uInt32				u32AdcClock;			// The clock of the Adc used for decimation
	uInt32				u32SampleClock;			// The base sample clock used for decimation
	BOOLEAN				bUseDacCalibFromEEprom;
	BOOLEAN				b14G8;					// Nucleon CobraMax PCIE 14G8


	CS_FW_VER_INFO		VerInfo;
	BUNNY_CAL_INFO		CalibInfoTable;		// Active Calibration Info Table

	RABBIT_SAMPLE_RATE_INFO		SrInfoTable[BUNNY_MAX_SR_COUNT];
	uInt32						u32SrInfoSize;

	FRONTEND_RABBIT_INFO		FeInfoTable[BUNNY_MAX_RANGES];
	uInt32						u32FeInfoSize;

	//----- Add-On regsiters
	CSRABIT_ADDON_REGS	AddOnReg;
	BUNNYCHANCALIBINFO  DacInfo[BUNNY_CHANNELS][BUNNY_MAX_RANGES];
	
	CSACQUISITIONCONFIG AcqConfig;
	CS_CHANNEL_PARAMS	ChannelParams[BUNNY_CHANNELS];
	CS_TRIGGER_PARAMS	TriggerParams[1+2*BUNNY_CHANNELS];

	PLXBASE_DATA		PlxData;

}CSRABBIT_CARDSTATE, *PCSRABBIT_CARDSTATE;

#pragma pack ()

typedef CSRABBIT_CARDSTATE CSCARD_STATE;
typedef PCSRABBIT_CARDSTATE PCSCARD_STATE;

#endif	//_RABBIT_CARDSTATE_h_
