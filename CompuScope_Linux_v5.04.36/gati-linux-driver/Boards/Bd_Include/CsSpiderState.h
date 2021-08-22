#ifndef _SPIDER_CARDSTATE_h_
#define	_SPIDER_CARDSTATE_h_

#include "CsDrvStructs.h"
#include "DrvCommonType.h"
#include "Cs8xxxCapsInfo.h"
#include "CsSpiderCal.h"

#pragma pack (8)

typedef struct _SPIDER_SAMPLE_RATE_INFO
{
	LONGLONG llSampleRate;
	uInt32   u32ClockFrequencyMhz;
	uInt32   u32Decimation;
	uInt32   u32PingPong;
}SPIDER_SAMPLE_RATE_INFO;


//----- The following structure groups all the registers of the Add-On
typedef struct _CSSPIDER_ADDON_REGS
{
	uInt32	u32FeIndex[CSPDR_CHANNELS];
	uInt32	u32FeSetup[CSPDR_CHANNELS];
	uInt32	u32Impendance[CSPDR_CHANNELS];
	int32	i32Position_mV[CSPDR_CHANNELS];
	uInt16  u16ChanSelect;
	uInt16  u16ClkSelect;
	uInt16  u16ClkOut;
	uInt16	u16ExtTrig;
	uInt16	u16TrigEnable;
	uInt32	u32ClkFreq;
	uInt32  u32Decimation;
	uInt16	u16Option;
	uInt16	u16Capture;
	uInt16	u16Calib;
	uInt16	u16SelfTest;

	// Nucleon only 
	uInt16	u16MsSetup;
	uInt16	u16MasterSlave;
	uInt16  u16MsBist1;
	uInt16  u16MsBist2;
} CSSPIDER_ADDON_REGS, *PCSSPIDER_ADDON_REGS;


typedef struct	_SPIDER_CARDSTATE
{
	uInt32				u32Size;			// size of this structure
	uInt16				u16DeviceId;
	uInt8				u8ImageId;			// Current active FPGA image;
	uInt16				u16CardNumber;		// Card number in M/S system

	uInt8				u8MsCardId;			// M/S card Id, 0 is Master, 1 to 7 are slabes
	uInt8				u8MsTriggMode;		// Master/Slave trigger mode
	uInt8				u8SlaveConnected;	// Position of slaves cards
	eRoleMode			ermRoleMode;

	ePulseMode			epmPulseOut;
	eTrigEnableMode		eteTrigEnable;
	eClkOutMode			eclkoClockOut;
	uInt16				u16TrigOutMode;
	BOOLEAN				bDisableTrigOut;			// Disable trigger out
	BOOLEAN				bForceIndMode;				// Force Independent Mode
	BOOLEAN				bPciExpress;
	BOOLEAN				bVirginBoard;				// PciExpress virgin card

	uInt32				u32ProcId;					// owner process Id
	uInt32				u32ActualSegmentCount;
	BOOLEAN				bCardStateValid;
	BOOLEAN				bAddOnRegistersInitialized;
	BOOLEAN				bAddonInit;
	BOOLEAN				bCalNeeded[CSPDR_CHANNELS];
	BOOLEAN				bInCal[CSPDR_CHANNELS];
	BOOLEAN				bAuxConnector;
	BOOLEAN				bSafeBootImageActive;			// card is operated with the Safe Boot image

	uInt32				u32InstalledMisc;
	uInt32				u32InstalledTerminations;

	BOOLEAN				bSpiderV12;			// Spider Hw v1.2
	BOOLEAN				bSpiderLP;			// Spider Hw v1.2 Low Power consumsion
	BOOLEAN             bZap;
	BOOLEAN				bFastCalibSettingsDone[CSPDR_CHANNELS];
	uInt16				u16ChanProtectStatus;	// Spider 1.2 only. Channels protection fault status
	uInt16				u16AdcOffsetAdjust;

	uInt32				u32DefaultMode;
	uInt32				u32SampleClock;				// The base sample clock used for decimation
	uInt32				u32LowCutoffFreq[CSPDR_FILTERS];
	uInt32				u32HighCutoffFreq[CSPDR_FILTERS];


	CSSPIDER_ADDON_REGS		AddOnReg;
	FRONTEND_SPIDER_DAC_INFO  DacInfo[CSPDR_CHANNELS][CSPDR_MAX_FE_SET_INDEX][CSPDR_MAX_RANGES];

	CS_FW_VER_INFO		VerInfo;

	SPIDER_SAMPLE_RATE_INFO SrInfo[CSPDR_MAX_SR_COUNT];
	uInt32					u32SrInfoSize;
	uInt32					u32SwingTable[CSPDR_IMPED][CSPDR_MAX_RANGES];
	uInt32					u32SwingTableSize[CSPDR_IMPED];
	char					szFpgaFileName[MAX_FILENAME_LENGTH];		// FPGA file name

	CSACQUISITIONCONFIG AcqConfig;
	CS_CHANNEL_PARAMS	ChannelParams[CSPDR_CHANNELS];
	CS_TRIGGER_PARAMS	TriggerParams[1+2*CSPDR_CHANNELS];

	PLXBASE_DATA		PlxData;
	uInt32				u32DataMask;

}SPIDER_CARDSTATE, *PSPIDER_CARDSTATE;

#pragma pack ()

typedef SPIDER_CARDSTATE  CSCARD_STATE;
typedef PSPIDER_CARDSTATE PCSCARD_STATE;

#endif	//_SPIDER_CARDSTATE_h_
