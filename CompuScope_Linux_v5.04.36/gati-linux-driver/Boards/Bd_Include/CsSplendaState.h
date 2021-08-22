#ifndef _SPLENDA_CARDSTATE_h_
#define	_SPLENDA_CARDSTATE_h_

#include "CsDrvStructs.h"
#include "DrvCommonType.h"
#include "Cs8xxxCapsInfo.h"
#include "CsSplendaCal.h"

#pragma pack (8)

typedef struct _SPLENDA_SAMPLE_RATE_INFO
{
	LONGLONG llSampleRate;
	uInt32   u32ClockFrequencyMhz;
	uInt32   u32Decimation;
} SPLENDA_SAMPLE_RATE_INFO;

typedef struct _FRONTEND_SPLENDA_INFO
{                           // 0     1
	uInt32	u32Swing_mV;
	uInt32	u32RegSetup;
} FRONTEND_SPLENDA_INFO, *PFRONTEND_SPLENDA_INFO;


//----- The following structure groups all the registers of the Add-On
typedef struct _CSSPLENDA_ADDON_REGS
{
	uInt32	u32FeSetup[CSPLNDA_CHANNELS];
	int32	i32Position_mV[CSPLNDA_CHANNELS];
	uInt16  u16ChanSelect;
	uInt16  u16ClkSelect;
	uInt16	u16TrigEnable;
	uInt32	u32ClkFreq;
	uInt32  u32Decimation;
	uInt16	u16SelfTest;
	uInt16	u16MasterSlave;
	uInt16  u16VSC;
	uInt16  u16MsBist1;
	uInt16  u16MsBist2;
} CSSPLENDA_ADDON_REGS, *PCSSPLENDA_ADDON_REGS;

typedef struct	_SPLENDA_CARDSTATE
{
	uInt32				u32Size;			// size of this structure
	uInt16				u16DeviceId;
	uInt8				u8ImageId;			// Current active FPGA image;
	uInt16				u16CardNumber;		// Card number in M/S system

	uInt8				u8MsCardId;			// M/S card Id, 0 is Master, 1 to 7 are slabes
	uInt8				u8SlaveConnected;	// Position of slaves cards

	eRoleMode			ermRoleMode;
	eTrigEnableMode		eteTrigEnable;
	eClkOutMode			eclkoClockOut;
	uInt16				u16TrigOutMode;
	BOOLEAN				bDisableTrigOut;			// Disable trigger out
	BOOLEAN				bForceIndMode;				// Force Independent Mode
	BOOLEAN				bPciExpress;
	BOOLEAN				bVirginBoard;				// PciExpress virgin card
	BOOLEAN				bImage0BootSuccess;			// For Nucleon PCI-E only. Patch for problem of FW on PCI_E Gen 2 on some of computers.

	eTrigEnableMode		ExtTrigEn;

	uInt32				u32ProcId;					// owner process Id
	BOOLEAN				bCardStateValid;
	BOOLEAN				bActive;						// card is in active state (in use)
	BOOLEAN				bSafeBootImageActive;			// card is operated with the Safe Boot image
	BOOLEAN				bNeedFullReset;					// Full reset is needed
	BOOLEAN				bAddOnRegistersInitialized;
	BOOLEAN				bAddonInit;
	BOOLEAN				bLowCalFreq;
	BOOLEAN				bCalNeeded[CSPLNDA_CHANNELS];
	BOOLEAN				bInCal[CSPLNDA_CHANNELS];
	BOOLEAN				bAuxConnector;
	uInt32				u32InstalledMisc;
	uInt32				u32InstalledTerminations;
	uInt32				u32InstalledResolution;

	BOOLEAN				bFastCalibSettingsDone[CSPLNDA_CHANNELS];
	BOOLEAN				bCapacitorCharged[CSPLNDA_CHANNELS];
	BOOLEAN				bVarCap;
	BOOLEAN				bFlashReset;
	BOOLEAN				bAlarm;						// Channel protection fault

	uInt32				u32ActualSegmentCount;
	uInt32				u32DefaultMode;
	uInt32				u32SampleClock;				// The base sample clock used for decimation
	uInt32				u32LowCutoffFreq[CSPLNDA_FILTERS];
	uInt32				u32HighCutoffFreq[CSPLNDA_FILTERS];

	CS_FW_VER_INFO		VerInfo;

	CSSPLENDA_ADDON_REGS		AddOnReg;
	FRONTEND_SPLENDA_DAC_INFO  DacInfo[CSPDR_CHANNELS][CSPLNDA_MAX_FE_SET_INDEX][CSPLNDA_MAX_RANGES];

	SPLENDA_SAMPLE_RATE_INFO SrInfo[CSPLNDA_MAX_SR_COUNT];
	uInt32					u32SrInfoSize;
	uInt32					u32SwingTable[CSPLNDA_IMPED][CSPLNDA_MAX_RANGES];
	uInt32					u32FeSetupTable[CSPLNDA_IMPED][CSPLNDA_MAX_RANGES];
	uInt32					u32SwingTableSize[CSPLNDA_IMPED];
	CSPLNDA_CAL_INFO		CalibInfoTable;

	CSACQUISITIONCONFIG AcqConfig;
	CS_CHANNEL_PARAMS	ChannelParams[CSPLNDA_CHANNELS];
	CS_TRIGGER_PARAMS	TriggerParams[1+2*CSPLNDA_CHANNELS];

	PLXBASE_DATA		PlxData;

}SPLENDA_CARDSTATE, *PSPLENDA_CARDSTATE;

#pragma pack ()

typedef SPLENDA_CARDSTATE  CSCARD_STATE;
typedef PSPLENDA_CARDSTATE PCSCARD_STATE;

#endif	//_SPLENDA_CARDSTATE_h_
