#ifndef _DEERE_CARDSTATE_h_
#define	_DEERE_CARDSTATE_h_

#include "CsDrvStructs.h"
#include "DrvCommonType.h"
#include "CsDeereCapsInfo.h"
#include "CsDeereCal.h"

#pragma pack (8)

//----- The following structure groups all the registers of the Add-On
typedef struct _DEERE_ADDON_REGS
{
	uInt32	u32FeIndex[DEERE_CHANNELS];
	uInt32	u32FeSetup[DEERE_CHANNELS];
	int32	i32Position_mV[DEERE_CHANNELS];

	uInt16  u16ClkSelect;
	uInt16  u16ClkOut;
	uInt16  u16TbOutCfg;
	uInt16	u16ExtTrigIn;
	uInt16	u16ExtTrigSet;
	uInt16	u16TrigEnable;
	uInt32	u32ClkFreq;
	uInt32  u32Decimation;
	uInt16	u16Option;
	uInt16	u16Calib;
	uInt16	u16DataSelect;
	uInt16	u16MasterSlave;
	uInt16	u16MsCtrl;
	uInt16	u16CalibPulse;
	uInt16  u16CalDc;
	uInt16  u16CalAc;
	uInt16  u16AdcDrv;
	uInt16  u16AuxCfg;
	uInt16  u16AdcCtrl;

	uInt16  u16MsBist1;
	uInt16  u16MsBist2;

} DEERE_ADDON_REGS, *PDEERE_ADDON_REGS;


typedef struct _DeereClockSet
{
	LONGLONG llSampleRate;
	uInt32   u32ClockFrequencyMhz;
	uInt32   u32Decimation;
	int32    i32TaoDual;
	int32    i32TaoSingle;
} DEERE_SAMPLERATE_INFO, *PDEERE_SAMPLERATE_INFO;


typedef struct _FRONTEND_DEERE_INFO
{    
	uInt32	u32Swing_mV;
	uInt32	u32RegSetup;
	uInt16	u16AdcDrv;
	uInt16  u16CalAtten;
} FRONTEND_DEERE_INFO, *PFRONTEND_DEERE_INFO;

typedef struct _DEERE_CAL_LUT
{
	uInt32 u32FeSwing_mV;
	int32  i32PosLevel_uV;  
	int32  i32NegLevel_uV;
	uInt16 u16PosRegMask;
	uInt16 u16NegRegMask;
}DEERE_CAL_LUT;

typedef enum 
{
	eciDirectRef = 0,
	eciCalChan = eciDirectRef + 1,
	eciCalGrd = eciCalChan + 1,
}eCalInputJD;

typedef struct _DEERE_EXT_TRIG_EN
{
	BOOLEAN  bPolarity;
	BOOLEAN  bEdge;  
	BOOLEAN  bResetOnStart;
}DEERE_EXT_TRIG_EN;

typedef struct	_DEERE_CARDSTATE
{
	uInt32				u32Size;			// size of this structure
	uInt16				u16DeviceId;
	uInt8				u8ImageId;			// Current active FPGA image;
	uInt16				u16CardNumber;		// Card number in M/S system

	uInt8				u8MsCardId;			// M/S card Id, 0 is Master, 1 to 7 are slabes
	uInt8				u8MsTriggMode;		// Master/Slave trigger mode
	uInt8				u8SlaveConnected;	// Position of slaves cards
	uInt8				u8BadFirmware;		// The card has wrong or outdate Base board FPGA firmware.	eRoleMode			ermRoleMode;

	eRoleMode			ermRoleMode;
	BOOLEAN				bForceIndMode;				// Force Independent Mode
	BOOLEAN				bPciExpress;


	eTrigEnableMode		ExtTrigEn;

	uInt32				u32ProcId;					// owner process Id
	uInt32				u32ActualSegmentCount;		// Because of channel protection, 
													// this variable represent the number of Segment valid in memory
	BOOLEAN				bCardStateValid;
	BOOLEAN				bActive;						// card is in active state (in use)

	BOOLEAN				bNeedFullReset;					// Full reset is needed
	BOOLEAN				bAddOnRegistersInitialized;
	BOOLEAN				bLowCalFreq;
	BOOLEAN				bCalNeeded[DEERE_CHANNELS];
	BOOLEAN				bDpaNeeded;         // DPA and core matching
	BOOLEAN				bV10;
	BOOLEAN             b14Bits;
	BOOLEAN				bMemoryLess;		// Memory less card
	BOOLEAN				bDualChanAligned;
	BOOLEAN				bAdcAlignCalibReq;		// Calibration for Adc alignment required
	BOOLEAN             bLowExtTrigCalib;
	BOOLEAN				bMasterSlaveCalib;		// Calibration for Master/Slave
	BOOLEAN				bMsDecimationReset;		// Decimation reset for Master/Slave is required
	BOOLEAN				bExtTrigCalib;			// Calibration for External required

	// Calibration ofr  M/S and Ext trigger
	int16				i16ExtTrigSkew;
	int16				i16MasterSlaveSkew;
	uInt8				u8NumOfAdc;					// Number of ADC per channel
	uInt32				u32Rcount;
	uInt32				u32Ncount;


	uInt32				u32AdcClock;				// The clock of the Adc used for decimation
	uInt32				u32SampleClock;				// The base sample clock used for decimation
	
	//	Out Config
	eAuxIn					AuxIn;
	eAuxOut					AuxOut;
	eClkOutMode				eclkoClockOut;
	uInt16					u16TrigOutMode;
	BOOLEAN					bDisableTrigOut;
	uInt16					u16ExtTrigEnCfg;


	CS_FW_VER_INFO			VerInfo;
	DEERE_ADDON_REGS		AddOnReg;
	DEERE_CHANCALIBINFO     DacInfo[DEERE_CHANNELS][DEERE_MAX_RANGES];
	DEERE_ADC_CALIBINFO  	AdcReg[DEERE_ADC_COUNT][DEERE_MAX_RANGES];
	uInt16					u16NulledUserOffset[DEERE_CHANNELS];

	FRONTEND_DEERE_INFO		FeInfoTable[DEERE_MAX_RANGES];

	CSACQUISITIONCONFIG AcqConfig;
	CS_CHANNEL_PARAMS	ChannelParams[DEERE_CHANNELS];
	CS_TRIGGER_PARAMS	TriggerParams[1+2*DEERE_CHANNELS];

	PLXBASE_DATA		PlxData;

}DEERE_CARDSTATE, *PDEERE_CARDSTATE;

#pragma pack ()


typedef DEERE_CARDSTATE CSCARD_STATE;
typedef PDEERE_CARDSTATE PCSCARD_STATE;

#endif	//_DEERE_CARDSTATE_h_
