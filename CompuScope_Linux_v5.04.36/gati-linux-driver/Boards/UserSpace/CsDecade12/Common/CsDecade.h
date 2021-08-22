//! CsDecade.h
//!
//!

#ifndef __CsDecade12_h__
#define __CsDecade12_h__

#include "CsDefines.h"
#include "CsTypes.h"
#include "CsPlxDefines.h"
#include "CsDeviceIDs.h"
#include "CsAdvStructs.h"
#include "CsiHeader.h"
#include "CsFirmwareDefs.h"
#include "CsDecadeFlash.h"
#include "CsDecadeCal.h"
#include "CsDecadeCapsInfo.h"
#include "CsDecadeOptions.h"

#include "CsPlxBase.h"
#include "CsNiosApi.h"

#include "CsDrvStructs.h"
#include "DrvCommonType.h"
#include "DllStruct.h"
#include "GageWrapper.h"
#include "CsSharedMemory.h"

#include "CsFirmwareDefs.h"
#include "CsDecadeState.h"

#ifndef _WINDOWS_
#include "CsDrvShare.h"
#endif


#ifdef _WINDOWS_
#define USE_SHARED_CARDSTATE
#else
//#define USE_SHARED_CARDSTATE              // Enable this flag for shared cardstate
                                          // kernel code can support both modes without flag change
#endif

//#define DECADE_LINUX_DBG

#define ADDON_HW_VERSION_1_07		0x107	// for new add-on hardware
#define	SHARED_CAPS_NAME			_T("CsE12xGyCaps")
#define MEMORY_MAPPED_SECTION		_T("CsE12xGyWdf-shared") // CR-2621


#define REGULAR_INDEX_OFFSET	1		// 0: Boot, 1: Reg, 2: Expert1, 3: Expert2
#define EXPERT_INDEX_OFFSET		2		// 0: Boot, 1: Reg, 2: Expert1, 3: Expert2

#define DECADE_WD_TIMER_FREQ_100us		12500		// Watch dog unit count equal to 100 microsecond, one count is 8ns

// Max depth counter registers (29 bits)
#define DECADE_MAX_HW_DEPTH_COUNTER			(0x20000000-1)

// Max segment size for eXpert averaging (in samples)
#define	DECADE_MAX_HWAVG_DEPTH				(4092*16*2)
// Max Numver of Average for eXpert averaging (in samples)
#define	DECADE_MAX_NUMBER_OF_AVERAGE		1000000
#define	DECADE_FFTSIZE_MAX_SUPPORTED		8192

// Amount of memory in bytes at the end of each record that HW reserved to store ETBs and Timestamps
#define	DECADE_HWMULREC_TAIL_BYTES			160


#define	SPI_READ_CMD				0x00800000
#define	SPI_WRTIE_CMD				0x00810000
#define	NIOS_WRTIE_BIT				0x00010000

#define SAFEBOOT_FW			0x1
#define DEFAULT_FW			0x2
#define EXPERT1_FW			0x4
#define EXPERT2_FW			0x8	
#define EXPERT_FW			(EXPERT1_FW | EXPERT2_FW)

#define DDC_FIRMWARE_AVAILABLE			//VC Add
//#define DDC_FIRMWARE_DEBUG	1
#define OCT_FIRMWARE_AVAILABLE			//VC Add

#ifdef _DEBUG
/// DEBUG: Compile-time assertion
//# define GAGE_CT_ASSERT(e)		do{int _[(e)?1:-1];_[0]=0;}while(0)
# define GAGE_CT_ASSERT(e)		{int _[(e)?1:-1];_[0]=0;}
#else
# define GAGE_CT_ASSERT(e)
#endif

typedef enum
{
	DataStreaming	= CS_BBOPTIONS_STREAM,
	Averaging		= CS_BBOPTIONS_MULREC_AVG_TD,
	FFT				= CS_BBOPTIONS_FFT_8K,
	DDC				= CS_BBOPTIONS_DDC,
	OCT				= CS_BBOPTIONS_OCT
} eXpertId;


typedef struct _DECADE_CAL_LUT
{                           
	uInt32	u32FeSwing_mV;
	int32   i32PosLevel_uV;
	int32   i32NegLevel_uV;
	uInt16  u16PosRegMask;
	uInt16  u16NegRegMask;
	float	Vref_mV;

} DECADE_CAL_LUT, *PDECADE_CAL_LUT;

typedef struct _DECADE_VREF
{                           
	int32	u32Range_mV;
	float	Vref_mV;

} DECADE_Vref, *PDECADE_Vref;

typedef enum _eCalInput
{
	eciDirectRef = 0,
	eciCalSrc = eciDirectRef + 1,
	eciCalChan = eciCalSrc + 1,
} eCalInput;

// Limitation of sofware averaging
#define MAX_SW_AVERAGING_SEMGENTSIZE		512*1024		// Max segment size in Software averaging mode (in samples)
#define	SW_AVERAGING_BUFFER_PADDING			16				// Padding for Sw Avearge buffer
#define MAX_SW_AVERAGE_SEGMENTCOUNT			4096			// Max segment count in SW averaging mode
#define	MAX_PRETRIGGERDEPTH_SINGLE			130880			// Max pretrigger depth in single channel mode 128K 
															// round down to trigger resolution

#define DECADE_DEFAULT_VALUE			0	
#define DECADE_DEFAULT_RELATION			CS_RELATION_OR
#define DECADE_DEFAULT_LEVEL			0

//!----- Default settings
#define DECADE_DEFAULT_PINGPONG_ADJ			0x2D0
#define DECADE_DEFAULT_TRIG_SENSITIVITY		128
#define DECADE_DEFAULT_EXT_TRIG_SENSE		100
#define DECADE_DEFAULT_SAMPLE_RATE			-1				// Force to the default sampling rate depending on card
#define DECADE_DEFAULT_EXT_CLOCK_EN			0
#define DECADE_DEFAULT_MODE					CS_MODE_DUAL
#define DECADE_DEFAULT_SAMPLE_BITS			CS_SAMPLE_BITS_12
#define DECADE_DEFAULT_RES					(-1*CS_SAMPLE_RES_LJ)
#define DECADE_DEFAULT_SAMPLE_SIZE			CS_SAMPLE_SIZE_2
#define DECADE_DEFAULT_SAMPLE_OFFSET		CS_SAMPLE_OFFSET_12_LJ
#define DECADE_DEFAULT_SEGMENT_COUNT		1
#define DECADE_DEFAULT_DEPTH				8160
#define DECADE_DEFAULT_SEGMENT_SIZE			DECADE_DEFAULT_DEPTH
#define DECADE_DEFAULT_TRIG_TIME_OUT		-1
#define DECADE_DEFAULT_TRIG_ENGINES_EN		CS_TRIG_ENGINES_DIS
#define DECADE_DEFAULT_TRIG_HOLD_OFF		0
#define DECADE_DEFAULT_TRIG_DELAY			0

#define DECADE_DEFAULT_CHANNEL_INDEX		0
#define DECADE_DEFAULT_TERM					CS_COUPLING_DC
#define DECADE_DEFAULT_GAIN					CS_GAIN_2_V
#define DECADE_DEFAULT_INPUTRANGE			-1				// Force to the default input range depending on card
#define DECADE_DEFAULT_IMPEDANCE			CS_REAL_IMP_50_OHM
#define DECADE_DEFAULT_FILTER				0
#define DECADE_DEFAULT_POSITION				0

#define DECADE_DEFAULT_CONDITION			CS_TRIG_COND_POS_SLOPE
#define DECADE_DEFAULT_SOURCE				CS_CHAN_1
#define DECADE_DEFAULT_EXT_COUPLING			CS_COUPLING_DC
#define DECADE_DEFAULT_EXT_GAIN				CS_GAIN_5_V
#define DECADE_DEFAULT_EXT_IMPEDANCE		CS_REAL_IMP_1K_OHM

#define DECADE_DEFAULT_CLKOUT				eclkoNone	
#define DECADE_DEFAULT_EXTTRIGEN			eteAlways

#define DECADE_DEFAULT_CLOCK_REG			0x1			// Default: Internal clock, clock out diabled
#define DECADE_DEFAULT_TRIGIO_REG			0x8			// version 1.06 Default: Trig out disabled
#define DECADE_DEFAULT_TRIGIN_REG			0x0			// for CPLD misc: external trig disable
#define DECADE_DEFAULT_TRIGOUT_PINGPONG_REG	0x1			// For A1h: Trig out disabled


#define	DECADE_MEMORY_CLOCK					150000000		// 150MHz clock
#define DECADE_TS_MEMORY_CLOCK				200000000		// 200MHz clock
#define DECADE_TS_SAMPLE_CLOCK				187500000		// 3GHz/16  = 187.5MHz clock

#define NCO_CONSTANT						0x10000000		// DDC internal NCO value				

// Trigger Engine Index in Table
#define	CSRB_TRIGENGINE_EXT		0
#define	CSRB_TRIGENGINE_A1		1
#define	CSRB_TRIGENGINE_B1		2
#define	CSRB_TRIGENGINE_A2		3
#define	CSRB_TRIGENGINE_B2		4

#define CSRB_GLB_TRIG_ENG_ENABLE        0x0001

//----- Self Test _mode
#define DECADE_CAPTURE_DATA	0x0000
#define DECADE_SELF_COUNTER	0x0004

// Valid Expert Firmware
#define DECADE_VALID_EXPERT_FW	(CS_BBOPTIONS_STREAM | CS_BBOPTIONS_MULREC_AVG_TD | CS_BBOPTIONS_FFT_8K | CS_BBOPTIONS_DDC)

typedef enum 
{
	 formatDefault = 0,
	 formatHwAverage,
	 formatSoftwareAverage,
	 formatPacked_8,
	 formatPacked_12
} 
_CSDATAFORMAT;


const uInt32	g_u32ValidExpertFw =  CS_BBOPTIONS_STREAM | CS_BBOPTIONS_MULREC_AVG_TD | CS_BBOPTIONS_FFT_8K | CS_BBOPTIONS_DDC | CS_BBOPTIONS_OCT;

class CsDecade : public CsPlxBase,  public CsNiosApi
{
private:
	PGLOBAL_PERPROCESS		 m_ProcessGlblVars;

#if !defined(USE_SHARED_CARDSTATE)
	CSDECADE_DEVEXT				m_CardState;
#endif
	PCSDECADE_DEVEXT			m_pCardState;
	void						UpdateCardState(PCSDECADE_DEVEXT pCardSt = NULL,  BOOLEAN bReset = FALSE);
	uInt32						m_u32SystemInitFlag;	// Compuscope System initialization flag
	uInt32						m_u32SrInfoSize;

	static const uInt16 c_u16CalDacCount;
	static const uInt16 c_u16CalAdcCount;

	static const uInt32 c_u32CalDacCount;		
	static const uInt32 c_u32CalAdcCount;

	static DECADE_CAL_LUT	s_CalCfg50[];

	const static FRONTEND_DECADE_INFO		c_FrontEndGain50[];
	const static DECADE_SAMPLE_RATE_INFO	Decade12_3_2[];

	uInt8					m_u8TrigSensitivity;			// Digital Trigger sensitivity;
	int32					m_i32ExtTrigSensitivity;		// ExtTrigger Trigger sensitivity;

public:
	CsDecade*			m_MasterIndependent;	// Master /Independant card
	CsDecade*			m_Next;					// Next Slave
	CsDecade*			m_HWONext;				// HW OBJECT next
	DRVHANDLE			m_PseudoHandle;			// User level card handle
	DRVHANDLE			m_CardHandle;			// Handle from Kernel Driver
	uInt8				m_u8CardIndex;			// Card index in native card detection order
	uInt16				m_DeviceId;

	PDRVSYSTEM			m_pSystem;				// Pointer to DRVSYSTEM
	CsDecade*			m_pTriggerMaster;		// Point to card that has trigger engines enabled
	BOOLEAN				m_BusMasterSupport;		// HW support BusMaster transfer

	// Contructor
	CsDecade( uInt8 u8Index, PCSDEVICE_INFO pDevInfo, PGLOBAL_PERPROCESS ProcessGlblVars);


protected:
	uInt8					m_CalibTableId;				// Current active calibration table;
	uInt32					m_u32CalFreq;
	eCalMode				m_CalMode;
	uInt8*					m_pu8CalibBuffer;
	int16*					m_pi16CalibBuffer;
	int32*					m_pi32CalibA2DBuffer;
	
	uInt32					m_u32CalibRange_uV;


	uInt32					m_u32HwTrigTimeout;
	CSACQUISITIONCONFIG		m_OldAcqConfig;
	CS_TRIGGER_PARAMS		m_CfgTrigParams[2*DECADE_CHANNELS+1];
	CS_TRIGGER_PARAMS		m_OldTrigParams[2*DECADE_CHANNELS+1];
	CS_CHANNEL_PARAMS		m_OldChanParams[DECADE_CHANNELS];

	//----- Unit number for this device
	BOOLEAN					m_bUseDefaultTrigAddrOffset;	// Skip TrigAddr Calib table
	bool					m_bForceCal[DECADE_CHANNELS];		
	bool					m_bSafeImageActive;
	bool					m_bClearConfigBootLocation;

	uInt8					m_u8BootLocation;				// BootLocation: 0 = BootImage, 1 = Image0; 2= Image1 ...
	FILE_HANDLE				m_hCsiFile;						// Handle of the Csi file
	FILE_HANDLE				m_hFwlFile;						// Handle of the Fwl file
	char					m_szCsiFileName[MAX_FILENAME_LENGTH];	// Csi file name
	char					m_szFwlFileName[MAX_FILENAME_LENGTH];	// fwl file name
	char					m_szCalibFileName[MAX_FILENAME_LENGTH];	// Calibration params file name
	uInt32                  m_u32IgnoreCalibErrorMask;
	uInt32                  m_u32IgnoreCalibErrorMaskLocal;

	CS_STRUCT_DATAFORMAT_INFO		m_DataFormatInfo;
	CSI_ENTRY				m_BaseBoardCsiEntry;
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
	
	// DECADE registers
	ADC12D1600_REGS			m_Adc12_Reg;
	DIGI_TRIG_CTRL_REG		m_TrigCtrl_Reg;

	CsCaptureMode			m_CaptureMode;					// Capture Mode: Memory or Streaming
	CsDataPackMode			m_DataPackMode;					// Actual Data Pack Mode: Unpack, Packed_8 or Packed_12
	CsDataPackMode			m_ConfigDataPackMode;			// Data Pack Mode from user setting: Unpack, Packed_8 or Packed_12

	STREAMSTRUCT			m_Stream;						// Stream state
	CS_TRIG_OUT_CONFIG_PARAMS	m_TrigOutCfg;

	//----- Variable related to Data Transfer
	CHANNEL_DATA_TRANSFER	m_DataTransfer;

	BOOLEAN				m_bFirmwareChannelRemap;	// Remap the channels from Firmware v55 or newer
	BOOLEAN				m_UseIntrTrigger;		
	BOOLEAN				m_UseIntrBusy;			
	BOOLEAN				m_bCommitIsRequired;		// Some changes on configuration of the card are made. Commit is required.
													// Usually this action is handle by SSM. Stream Data packing is recenly added and it is outside
													// things that SSM can handle. We have to handle in the driver.

	BOOLEAN				m_bSoftwareAveraging;		// Mode Software Averaging enabled
	bool				m_bMulrecAvgTD;				// Mode Expert Multiple Record AVG TD
	uInt32				m_u32MulrecAvgCount;		// Mulrec AVG count (should be <= 1024)
	BOOLEAN				m_bExpertAvgFullRes;		// Mode Harware Averaging in full resolution (32bits)
	int16				*m_pAverageBuffer;			// Temporarary buffer to keep the result of avearging

	// Variables for Master / Slave calibration
	uInt16					m_u16DebugExtTrigCalib;
	BOOLEAN					m_bNeverCalibExtTrig;		// No Calibration for External required
	BOOLEAN					m_bForceCalibExtTrig;		// Always CalibExtTrig (to test).

	eCalMode				m_CalibMode;

	// DECADE CALIB
	bool			m_bSkipLVDS_Cal;					// Skip LVDS calibration
	bool			m_bBetterEnob;						// Calibration for better ENOB
	bool			m_bBetterCalib_IQ;					// Calib ADC for I/Q match
	bool			m_bLogFaluireOnly;
	bool			m_bUseEepromCal;
	bool			m_bPingpongCapture;					// Request acquisition with ping-pong sampling rate
	bool			m_bCalibPingpong;				// Pingpong calibration (Matching Gain) is required
	bool			m_bSingleChannel2;					// Capture in SINGLE channel mode using the channel 2

	uInt16			m_u16SkipCalibDelay; //in minutes;
	bool			m_bExtTrigLevelFix;					// Extternal Trigger Level cannot be changed
	bool			m_bNeverCalibrateAc;
	bool			m_bNeverCalibrateDc;
	bool			m_bNeverCalibrateGain;
	bool			m_bSkipCalib;
	bool			m_bFastCalibSettingsDone[DECADE_CHANNELS];
	bool			m_Adc_GainIQ_CalNeeded[DECADE_CHANNELS];	// Adc Gain I/Q Cal needed
	bool			m_Adc_OffsetIQ_CalNeeded[DECADE_CHANNELS];	// Adc Offset I/Q Cal needed
	bool			m_bOneSampleResolution;				// Enabled One sample resolution for Depth, pretrigger and Trigger delay
	bool			m_bExtClkPingPong;					// Use PingPong in ExtClock mode

	bool			m_bCalibActive;					// In calibration mode
	bool			m_DDC_enabled;					// Expert DDC is enabled
	uInt32			m_iDDC_ModeSelect;				// The current DDC mode select value( from user settings )
	uInt32			m_iOld_DDC_ModeSelect;			// DDC mode select value before calibration
	uInt32			m_iDDC_Enable_ModeSelect;		// DDC mode select value when enable;

	int32	        m_i32DcOfffsetFudgeFactor_mV;	// Fudge factor for HW problem of DC offset calibration
	uInt32			m_u32PingPongShift;				// External clock pingpong shifter value to set
	uInt32			m_u32CurrentPingPongShift;		// External clock pingpong shifter value current
	uInt8           m_u8CalDacSettleDelay;
	uInt8           m_u8DepthIncrement;
	uInt32			m_u32CalSrcSettleDelay;
	uInt32			m_u32DebugCalibTrace;
	uInt32			m_u32DDCTraceDebug;
	uInt16			m_ChannelStartIndex;				// The default channel index that the driver uses in the loop of calibrations

	// Variable for FFT support
	CS_FFT_CONFIG_PARAMS	m_FFTConfig;
	int16					m_i16CoefficientFFT[DECADE_FFTSIZE_MAX_SUPPORTED];
	FFTCONFIG_REG			m_u32FftReg;				// Last value written to FFT config register
	bool					m_bFFTsupported;
	bool					m_bFFT_enabled;
	bool					m_OCT_enabled;				// Expert OCT is enabled
	bool					m_OCT_Config_set;			// Core Config of OCT has been set
	uInt32					m_iOCT_ModeSelect;			// The current OCT mode select value( from user settings )
	uInt32					m_u32OCTTraceDebug;
	uInt32					m_OverVoltageDetection;		// Enable or disable Overvoltage detection

	//----- CsDecade.cpp
	int32  MsCsResetDefaultParams( BOOLEAN bReset = FALSE );
	int32  CsSetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg = NULL );
	int32  CsSetTriggerConfig( PARRAY_TRIGGERCONFIG pATrigCfg = NULL );
	int32  CsSetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg = NULL );

	int32  RemoveDeviceFromUnCfgList();
	void   AddDeviceToInvalidCfgList();
	
	uInt16 BuildIndependantSystem( CsDecade** FirstCard, BOOLEAN bForceInd );
	uInt16 DetectAcquisitionSystems(BOOL bForceInd); 

	void	TraceCalib(uInt16 u16Channel, uInt32 u32TraceFlag, uInt32 u32V1 = 0, uInt32 u32V2=0, int32 i32V3=0, int32 i32V4=0);
	void	TraceDebugCalib(uInt16 u16Channel, uInt32 u32TraceFlag, uInt32 u32V1 = 0, uInt32 u32V2=0, int32 i32V3=0, int32 i32V4=0);
	
	int32	CsGetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg );
	int32   CsGetTriggerConfig( PARRAY_TRIGGERCONFIG pATriggerCfg );
	int32	CsGetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg );
	
	int32  GetFreeTriggerEngine( int32	i32TrigSource, uInt16 *u16TriggerEngine );
	uInt16 NormalizedChannelIndex( uInt16 u16ChannelIndex );
	int32  NormalizedTriggerSource( int32 i32TriggerSource );

	bool  IsClockChange(LONGLONG llSampleRate, uInt32 u32ExtClkSampleSkip, BOOLEAN bRefClk );
	int32 SendClockSetting(LONGLONG llSampleRate = DECADE_DEFAULT_SAMPLE_RATE, uInt32 u32ExtClkSampleSkip = 0, BOOLEAN bRefClk = FALSE);
	int32 SendCaptureModeSetting(uInt32 u32Mode = CS_MODE_DUAL, uInt16 u16HwChannelSingle = CS_CHAN_1);
	int32 SendCalibModeSetting( eCalMode ecMode = ecmCalModeNormal );
	int32 RestoreCalibModeSetting();
	int32 SendChannelSetting(uInt16 Channel, uInt32 u32InputRange = DECADE_DEFAULT_INPUTRANGE, uInt32 u32Term = DECADE_DEFAULT_TERM, uInt32 u32Impendance = DECADE_DEFAULT_IMPEDANCE, int32 i32Position = 0, uInt32 u32Filter = 0, int32 i32ForceCalib = 0);
	int32 SendTriggerEngineSetting(int32	i32SourceA1,
											uInt32  u32ConditionA1 = DECADE_DEFAULT_CONDITION,
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
										   uInt32	u32ExtCondition = DECADE_DEFAULT_CONDITION,
										   uInt32	u32ExtTriggerRange =  DECADE_DEFAULT_EXT_GAIN, 
										   uInt32	u32ExtTriggerImped =  DECADE_DEFAULT_EXT_IMPEDANCE, 
										   uInt32	u32ExtCoupling = DECADE_DEFAULT_EXT_COUPLING,
										   int32	i32Sensitivity = DECADE_DEFAULT_EXT_TRIG_SENSE );
	
	int32 SendDigitalTriggerSensitivity( uInt16  u16TrigEngine, uInt8 u8Sensitive = DECADE_DEFAULT_TRIG_SENSITIVITY );
	int32 SetTriggerSensitivity();
	int32 SendExtTriggerLevel( int32 i32Level );

	int32 WriteToSelfTestRegister( uInt16 SelfTestMode = CSST_DISABLE );
	int32 InitAddOnRegisters(bool bForce = false);

	void	AssignBoardType();
	void	AssignAcqSystemBoardName( PCSSYSTEMINFO pSysInfo );
	int32	AcqSystemInitialize();
	void	SetDataMaskModeTransfer ( uInt32 u32TxMode );
	int32	CalibrateAllChannels(void);

	int32	GetFEIndex(uInt16 u16Channel);
	void	FillOutBoardCaps(PCSDECADE_BOARDCAPS pCapsInfo);

	int32	WriteCalibTableToEeprom(uInt8	u8CalibTableId, uInt32 u32Valid );

	eClkOutMode GetClockOut(void){ return m_pCardState->eclkoClockOut; }
	eTrigEnableMode GetExtTrigEnable(void){return m_pCardState->eteTrigEnable;}
	
	int32	SendSegmentSetting(uInt32 u32SegmentCount, int64 i64PostDepth, int64 i64SegmentSize, int64 i64HoldOff, int64 i64TrigDelay = 0);
	bool ForceFastCalibrationSettingsAllChannels();

	void ReadVersionInfo();
	int32	ReadTriggerTimeStampEtb( uInt32 StartSegment, int64 *pBuffer, uInt32 u32NumOfSeg );

	int32	CsGetAverage(uInt16 u16Channel, int16* pi16Buffer, uInt32 u32BufferSizeSamples, int32 i32StartPosistion, int32* pi32OddAvg_x10, int32* pi32EvenAvg_x10 = 0);
	int32	ProcessSoftwareAveraging( int64 i64StartAddress, uInt16 u16Channel, int32 *pi32ResultBuffer, uInt32 u32SamplesInBuffer );
	int32	ReadCommonRegistryOnBoardInit();
#ifdef _WINDOWS_ 
	int32	ReadCommonRegistryOnAcqSystemInit();
	void	ReadCalibrationSettings(void *Params);
#else
	int32	ReadCommonCfgOnAcqSystemInit();
	void	ReadCalibrationSettings(char *szIniFile, void *Params);
   int32 ShowCfgKeyInfo();
#endif   
	void	CsSetDataFormat( _CSDATAFORMAT csFormat = formatDefault );
	void	CsResetDataFormat();

	int32	CommitNewConfiguration( PCSACQUISITIONCONFIG pAcqCfg,
									PARRAY_CHANNELCONFIG pAChanCfg,
									PARRAY_TRIGGERCONFIG pATrigCfg );
	void	ResetCachedState(void);

	void	RestoreCalibDacInfo();
	void	InitializeDefaultAuxIoCfg();
	void	SetDefaultAuxIoCfg();

public:
	CsDecade *GetCardPointerFromChannelIndex( uInt16 ChannelIndex );
	CsDecade *GetCardPointerFromBoardIndex( uInt16 BoardIndex );
	CsDecade *GetCardPointerFromTriggerIndex( uInt16 TriggerIndex );
	CsDecade *GetCardPointerFromTriggerSource( int32 i32TriggerSoure );

	int32   CsGetBoardsInfo( PARRAY_BOARDINFO pABoardInfo );
	int32	DeviceInitialize(void);
	int32	CsAutoUpdateFirmware();

	int32  AcquireData(uInt32 u32IntrEnabled = OVER_VOLTAGE_INTR);
	void   MsAbort();

	uInt32	GetDataMaskModeTransfer ( uInt32 u32TxMode );
	uInt32	GetModeSelect( uInt16 u16UserChannel );

	//----- Read/Write the Add-On Board Flash 
	int32	ReadCalibInfoFromCalFile();
	int32	CalibrateFrontEnd(uInt16 u16Channel, uInt32* pu32SuccessFlag = NULL);
	int32	CalibrateOffset(uInt16	u16HwChannel);

	LONGLONG GetMaxSampleRate( BOOLEAN bPingpong );
	uInt16	ConvertToHwChannel( uInt16 u16UserChannel );
	uInt16	RemapChannel( uInt16 u16UserChannel );
	uInt16	ConvertToUserChannel( uInt16 u16HwChannel );
	uInt16	ConvertToMsUserChannel( uInt16 u16UserChannel );
	BOOLEAN	IsCalibrationRequired();
	int32	SendPosition(uInt16 u16Channel, int32 i32Position_mV);

	int32	CsAcquisitionSystemInitialize( uInt32 u32InitFlag );
	int32	CsGetParams( uInt32 u32ParamId, PVOID pUserBuffer, uInt32 u32OutSize = 0);
	int32	CsSetParams( uInt32 u32ParamId, PVOID pInBuffer, uInt32 u32InSize = 0 );
	int32	CsStartAcquisition();
	int32	CsAbort();
	void	CsForceTrigger();
	int32	CsDataTransfer( in_DATA_TRANSFER *InDataXfer );
	int32	CsMemoryTransfer(in_DATA_TRANSFER *InDataXfer);
	int32	CsTransferTimeStamp(in_DATA_TRANSFER *InDataXfer);
	uInt32	CsGetAcquisitionStatus();
	int32	CsRegisterEventHandle(in_REGISTER_EVENT_HANDLE &InRegEvnt );
	void	CsAcquisitionSystemCleanup();
	void	CsSystemReset(BOOL bHard_reset);
	int32	CsForceCalibration();
	int32	CsGetAsyncDataTransferResult( uInt16 u16Channel,CSTRANSFERDATARESULT *pTxDataResult, BOOL bWait );
	int32	CsGetAcquisitionSystemCount( BOOL bForceInd, uInt16 *pu16SystemFound );

	int32	ProcessDataTransfer(in_DATA_TRANSFER *InDataXfer);
	int32	ProcessDataTransferDebug(in_DATA_TRANSFER *InDataXfer);
	int32	ProcessDataTransferModeAveraging(in_DATA_TRANSFER *InDataXfer);

	int32	CsValidateAcquisitionConfig( PCSACQUISITIONCONFIG, uInt32 u32Coerce );
	void	UpdateSystemInfo( CSSYSTEMINFO *SysInfo );
	int32	SetPosition(uInt16 u16Channel, int32 i32Position_mV);
	int32	FindFeIndex( const uInt32 u32Swing, uInt32& u32Index );

	int32	AcquireAndAverage(uInt16	u16Channel, int32* pi32AvrageOdd_x10 = 0, int32* pi32AvrageEven_x10 = 0);
//	int32	CalibrateOffset(uInt16 u16Channel,  uInt32* pu32SuccessFlag);
	int32	CalibrateGain(uInt16 u16Channel);
	int32	CalibratePosition(uInt16 u16Channel);
	int32	SetCalibrationRange(uInt32 u32Range_mV);
	int32	ReadnCheckCalibA2D(int32 i32Expected_mV, int32* pi32V_uv);
	int32	SendDigitalTriggerLevel( uInt16  u16TrigEngine, int32 i32Level );
	
	int32	InitHwTables();
	BOOLEAN	FindSrInfo( const LONGLONG llSr, PDECADE_SAMPLE_RATE_INFO *pSrInfo );

	void	SetFastCalibration(uInt16 u16Channel);
	int32	FindSRIndex( LONGLONG llSampleRate, uInt32 *u32IndexSR );
	void	InitBoardCaps();

	int32	FastCalibrationSettings(uInt16 u16Channel, bool bForce = false);

	EVENT_HANDLE	GetEventTxDone() { return m_DataTransfer.hAsynEvent; }
	EVENT_HANDLE	GetEventSystemCleanup() { return m_pSystem->hAcqSystemCleanup; }
	EVENT_HANDLE	GetEventTriggered() { return m_pSystem->hEventTriggered; }
	EVENT_HANDLE	GetEventEndOfBusy() { return m_pSystem->hEventEndOfBusy; }
	EVENT_HANDLE	GetEventAlarm() { return 0; }
	void			OnAlarmEvent() {};

	void	SetStateTriggered() { m_PlxData->CsBoard.bTriggered = true; }
	void	SetStateAcqDone() { m_PlxData->CsBoard.bBusy = false; }
	void	SignalEvents( BOOL bAquisitionEvent = TRUE );
	
	int32	SetClockThruNios( uInt32 u32ClockMhz );
	int32	ConfigureFe( const uInt16 u16Channel, const uInt32 u32Index, const uInt32 u32Imed, const uInt32 u32Unrestricted);
	int32	SetupForCalibration(bool bSetup = false);
	int32	SetupForReadFullMemory(bool bSetup = false);
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
	BOOLEAN	IsExpertAVG_enabled() {return ( m_bMulrecAvgTD );}

	FlashInfo	*FlashPtr;

	bool	IsExtTriggerEnable( PARRAY_TRIGGERCONFIG pATrigCfg );

	bool	IsImageValid( uInt8 u8Index );
	int32	WriteBaseBoardFlash(uInt32 Addr, void *pBuffer, uInt32 WriteSize, bool bBackup = FALSE );
	int32	IoctlGetPciExLnkInfo( PPCIeLINK_INFO pPciExInfo );

	// Overload functions to support Nulceon
	int32	CsReadPlxNvRAM( PPLX_NVRAM_IMAGE nvRAM_image );
	int32	CsWritePlxNvRam( PPLX_NVRAM_IMAGE nvRAM_image );

	int32	ReadHardwareInfoFromFlash();
	BOOL	IsNiosInit(void);
	int64	GetTimeStampFrequency();

	int32 	CsStmTransferToBuffer( PVOID pVa, uInt32 u32TransferSize );
	int32	CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *u32ErrorFlag, uInt32 *u32ActualSize, uInt8 *u8EndOfData );
	int32	CsStmGetTransferStatus( uInt32 u32TimeoutMs, uInt32 *u32ErrorFlag );
	void 	MsStmResetParams();

	int32	MsInitializeDevIoCtl();
	bool	IsExpertSupported(uInt8 u8ImageId, eXpertId xpId );
	void	PreCalculateStreamDataSize( PCSACQUISITIONCONFIG pCsAcqCfg );
	void	GetMemorySize();
	uInt32	ReadConfigBootLocation();
	int32	WriteConfigBootLocation( uInt32	u32Location );
	int32	GetConfigBootLocation();
	int32	SetConfigBootLocation( int32 i32BootLocation );

	// DDC
	int32	DDCModeSelect(uInt32 mode_select, bool bForce = false);
	int32	DDCModeStop();
	int32	SetDDCConfig(PDDC_CONFIG_PARAMS pConfig);
	int32	GetDDCConfig(PDDC_CONFIG_PARAMS pConfig);
	int32	SendDDCControl(uInt32 Cmd);
	int32	GetDDCScaleStatus(PDDC_SCALE_OVERFLOW_STATUS pConfig);

	void	TraceDDC(uInt32 u32TraceFlag, void *param1, void *param2, 
					 void *param3 = NULL, void *param4 = NULL, void *param5 = NULL, void *param6 = NULL);

        // OCT
	int32	SetOCTConfig(PCSOCT_CONFIG_PARAMS pConfig);
	int32	GetOCTConfig(PCSOCT_CONFIG_PARAMS pConfig);
	int32	SetOCTRecordLength(uInt32 size);
	int32	GetOCTRecordLength(uInt32 *size);
	int32	OCTModeSelect(uInt32 mode_select, bool bForce = false );

	void	TraceOCT(uInt32 u32TraceFlag, char *szDisplay, uInt8 u8Slope, uInt16 u16Level, uInt16 u16Hysteresis, uInt32 u32OutDepth);
	char	szDbgText[256];

#ifdef USE_SHARED_CARDSTATE
	int32	InitializeDevIoCtl();
	int32	UnInitializeDevIoCtl();
#endif

	uInt16	ReadBusyStatus();
	int32	CalibrateCoreOffset();

	int32	CsCheckPowerState()
	{	if ( 0 == m_pTriggerMaster->m_pCardState->bCardStateValid )
			return CS_POWERSTATE_ERROR;
		else
			return CS_SUCCESS;
	}

	int32	SendADC_LVDS_Align( bool bKeep_ADC_Settings, bool bForce = false );
	int32	SendExtTriggerAlign();
	int32	LockPLL();
	bool	isPLLLocked();		
	int32	UpdateCalibDac(uInt16 u16Channel, eCalDacId ecdId, uInt16 u16Data, bool bVal=false);
	int32   DumpAverageTable(uInt16 u16Channel, eCalDacId ecdId, uInt16 u16Vref=0, bool bInitDefault=false, uInt16 NbSteps=16);		
	
	BOOLEAN ConfigurationReset( uInt32	u32BootLocation );
	int32	SetClockControlRegister( uInt32 u32Val, bool bForce = false );
	int32	SetTriggerIoRegister( uInt32 u32Val, bool bForce = false );			// for old Add-on V1.06
	int32	SetTrigOut( uInt16 u16Value );
	int32	SetClockOut( eClkOutMode ClkOutValue );
	int32	FlashIdentifyLED( bool bOn );
	uInt32	ConvertTriggerTimeout (int64 i64Timeout_in_100ns);
	void	SetTriggerTimeout (uInt32 TimeOut = CS_TIMEOUT_DISABLE);
	int32	WriteToAdcRegister(uInt16 u16Chan, uInt16 u16Reg, uInt16 u16Data);
	void	CsResetFFT ();
	void	CsConfigFFTWindow ();
	void	CsCopyFFTWindowParams ( uInt32 u32WinSize, int16 *pi16Coef );
	void	CsCopyFFTparams ( CS_FFT_CONFIG_PARAMS *pFFT );
	void	CsStartFFT ();
	void	CsStopFFT ();
	void	CsWaitForFFT_DataReady();


	void	DetectBootLocation();
	int32	FindCalLutItem( const uInt32 u32Swing, const uInt32 u32Imped, DECADE_CAL_LUT& Item );
	int32	FindFeCalIndices( uInt16 u16Channel, uInt32& u32RangeIndex,  uInt32& u32FeModIndex );
	uInt16	GetControlDacId( eCalDacId ecdId, uInt16 u16Channel );
	uInt16	GetUserChannelSkip();
	int32	DefaultCalibrationSettings(uInt16 u16Channel, uInt32 u32RestoreFlag);
	int32	TestGainCalib( uInt16 u16Channel, uInt32 u32InpuRange );
	int32	AdjustAdcGain(uInt16 u16Channel, uInt32 u32Val, bool bI = true, bool bQ = true);
	int32	AdjustAdcOffset(uInt16 u16Channel, uInt32 u32Val, bool bI = true, bool bQ = true);
	int32	SendAdcReggister0( uInt16 u16Val, bool bForce = false );
	void	ClockPingpongAdjust(uInt32	u32Adjust);
	void	DetectPingpongAcquisitions(PCSACQUISITIONCONFIG pAcqCfg);

	int32	CaptureCalibSignal(bool bWaitTrigger = false);
	int32	WriteRegisterThruNiosEx( uInt32 Data, uInt32 Command, int32 i32Timeout, uInt32 *pu32NiosReturn );
    int		DumpBuffer(char *Label, uInt8 *pBuf, int int32);
	int32	AdcOffsetIQCalib();
	int32	SendADC_IQ_Calib( uInt16 u16Channel, bool bGain_Offset );
	int32	CalibrateADC_IQ();
	int32	TryCalibrateOff(uInt16 u16HwChannel, bool bIsGoUp, uInt16 *bBestOffset, int32 *DeltaErr);
	int32	SetDataPackMode(CsDataPackMode DataPackMode, bool bForce = false);
	int32	UpdateDcCalibReg(uInt16 u16Data);
	int32	WriteRegisterThruNios(uInt32 u32Data, uInt32 u32Command, int32 i32Timeout_100us = -1, uInt32 *pu32NiosReturn = 0, bool bTracing = true);
	void 	BBtiming(uInt32 u32Delay_100us);	
	int32	DumpGainTable(uInt16 u16Channel, uInt16 u16Vref, bool bInitDefault = false, uInt16 Pos_0V = 0x8000, uInt16 NbSteps = 32);
	void	DumpTriggerParams(char *Str);
	void	DumpCfgTrigParams(char *Str);
	void	DumpTriggerConfig( PARRAY_TRIGGERCONFIG pATrigCfg );
};


// Prototype for functions
unsigned int WINAPI ThreadFuncIntr( LPVOID lpParam );
unsigned int WINAPI ThreadFuncTxfer( LPVOID lpParam );

#endif


// HOST REGISTERMAP (Offset 32bits convert to byte address)
// Decade registers for Firmware versions and options
#define	DECADE_OPT_RD_CMD			0x30
#define DECADE_SWR_RD_CMD			0x34
#define DECADE_HWR_RD_CMD			0x38
#define DECADE_FFT_USER_DATA		0x1CC
#define DECADE_FFT_USER_INDEX		0x1D0
#define DECADE_FFT_CONTROL			0x1D4

//DDC support - Byte Address
#define DECADE_DDC_NCO_CTRL			0x190
#define DECADE_DDC_DDS_COMMAND		0x194
#define DECADE_DDC_SCALE_FACTOR		0x0020003C

#define	DECADE_SET_MULREC_AVG_COUNT			0x00210035

//OCT support
#define	DECADE_OCT_TRIGGER_REG				0x00200037
#define	DECADE_OCT_COMMAND_REG				0x00200038
#define	DECADE_OCT_RECORD_REG				0x00200039
#define	DECADE_OCT_MODE_REG					0x0020003A


#define	DECADE_BB_TRIGTIMEOUT_REG			0x00210049
#define DECADE_DATA_SELECT					0x002100C1
#define DECADE_CHANNEL_CTRL					0x002100C5
#define DECADE_DECIMATION_SETUP				0x002100C6
#define DECADE_TRIGENGINES_EN				0x002100D4
#define DECADE_DT_CONTROL					0x002100D6
#define DECADE_TRIGOUT_N_PINGPONG_SETUP		0x002100A1

#define DECADE_CALIB_ADC112S_CTRL			0x00810040			// Baseboard Calib ADC 

#define TRACE_COARSE_OFFSET    0x0001 //Trace of the coarse offset
#define TRACE_FINE_OFFSET      0x0002 //Fine offset setting
#define TRACE_CAL_GAIN         0x0004 //Gain offset settings
#define TRACE_CAL_POS          0x0008 //Position calibration settings
#define TRACE_CAL_SIGNAL       0x0010 //Trace final result of calibration signal DAC
#define TRACE_CAL_DAC          0x0020 //Trace acces to All calibration DACs
#define TRACE_CAL_RESULT       0x0040 //Trace result of failed calibration if we go on
#define TRACE_CAL_RANGE        0x0080 //Trace calibration settings
//verbose
#define TRACE_CAL_SIGNAL_ITER  0x0100 //Trace  of the calibration ADC reading
#define TRACE_CAL_ITER         0x0200 //Trace of the iterations of the current stage
#define TRACE_ALL_DAC          0x0400 //Trace acces to All DACs
#define TRACE_CAL_SIGNAL_VERB  0x0800 //Trace final result of calibration signal DAC verbose

#define TRACE_CAL_PROGRESS     0x1000 //Trace calibration progress
#define TRACE_GAIN_ADJ         0x2000 //Trace gain adjust


#define TRACE_DDC_INFO      	   0x0001 //Trace of DDC INFO
#define TRACE_DDC_SEND_CONTROL     0x0010 //Trace of DDC CONTROL
#define TRACE_DDC_SET_CONFIG	   0x0020 //Trace of DDC SET CONFIG
#define TRACE_DDC_GET_CONFIG	   0x0030 //Trace of DDC GET CONFIG 
#define TRACE_DDC_GET_SCALE_STATUS 0x0040 //Trace of DDC GET SCALE STATUS

//Verbose

// OCT TRACE
#define TRACE_OCT_INFO			0x0001 //Trace Info
#define TRACE_OCT_SET_CONFIG	0x0002 //Trace of Set Config
#define TRACE_OCT_GET_CONFIG	0x0004 //Trace of Set Config
#define TRACE_OCT_SET_RECORD_LENGTH	0x0008 //Trace of Set Record Length
#define TRACE_OCT_GET_RECORD_LENGTH	0x0010 //Trace of Get Record Length
#define TRACE_OCT_SEND_COMMAND	0x0020 //Trace of Send command

#define DECADE_DIG_TRIGLEVEL_SPAN	0x1000		// 12 bits


#define DECADE_CALREF0		    2048
#define DECADE_CALREF1		    4096
#define DECADE_CALREF2		   10000
#define DECADE_CALREF3		       0
#define DECADE_MAX_CALREF_ERR_C  50
#define DECADE_MAX_CALREF_ERR_P   5
#define DECADE_POS_CAL_LEVEL      1
#define DECADE_GND_CAL_LEVEL      0
#define DECADE_NEG_CAL_LEVEL     -1

#define	DECADE_CAL_RESTORE_COARSE			0x0001
#define	DECADE_CAL_RESTORE_FINE				0x0002
#define	DECADE_CAL_RESTORE_GAIN				0x0004
#define	DECADE_CAL_RESTORE_POS  			0x0008
#define DECADE_CAL_RESTORE_OFFSET			(DECADE_CAL_RESTORE_COARSE|DECADE_CAL_RESTORE_FINE)
#define	DECADE_CAL_RESTORE_ALL  		 	(DECADE_CAL_RESTORE_OFFSET|DECADE_CAL_RESTORE_GAIN|DECADE_CAL_RESTORE_POS)

#define DECADE_CALIB_DEPTH			4000
#define DECADE_CALIB_BUF_MARGIN		160
#define DECADE_CALIB_BUFFER_SIZE	(DECADE_CALIB_BUF_MARGIN + DECADE_CALIB_DEPTH)
#define DECADE_CALIB_BLOCK_SIZE   64


#define DECADE_RD_BB_REGISTER			0x00200000		// Read Base board registers command
#define DECADE_WR_BB_REGISTER			0x00210000		// Write Base board registers command
#define DECADE_DIGITAL_ENGINE			(DECADE_WR_BB_REGISTER | 0xA4)
#define DECADE_EXT_TRIGGER_SETTING		(DECADE_WR_BB_REGISTER | 0xA5)
#define DECADE_DLA1						(DECADE_WR_BB_REGISTER | 0xE0)
#define DECADE_DSA1						(DECADE_WR_BB_REGISTER | 0xE1)
#define DECADE_DLB1						(DECADE_WR_BB_REGISTER | 0xE2)
#define DECADE_DSB1						(DECADE_WR_BB_REGISTER | 0xE3)
#define DECADE_DLA2						(DECADE_WR_BB_REGISTER | 0xE4)
#define DECADE_DSA2						(DECADE_WR_BB_REGISTER | 0xE5)
#define DECADE_DLB2						(DECADE_WR_BB_REGISTER | 0xE6)
#define DECADE_DSB2						(DECADE_WR_BB_REGISTER | 0xE7)

#define	DECADE_USER_LED_CRTL			(DECADE_WR_BB_REGISTER | 0x50)
#define	DECADE_BB_TRIGGER_AC_CALIB_REG	(DECADE_WR_BB_REGISTER | 0x51)
#define	DECADE_BB_DC_CALIB_REG_OLD		(DECADE_WR_BB_REGISTER | 0x52)
#define	DECADE_BB_DC_CALIB_REG			(DECADE_ADDON_CPLD | 0x5200 | DECADE_SPI_WRITE)


#define DECADE_CHAN1_SET_WR_CMD			0x00811102	// cmd = 0x008, scope = 0x1, add = 0x11  Model/Cs = 02
#define DECADE_CHAN2_SET_WR_CMD			0x00811202	// cmd = 0x008, scope = 0x1, add = 0x12  Model/Cs = 02
#define DECADE_CHAN3_SET_WR_CMD			0x00811302	// cmd = 0x008, scope = 0x1, add = 0x13  Model/Cs = 02
#define DECADE_CHAN4_SET_WR_CMD			0x00811402	// cmd = 0x008, scope = 0x1, add = 0x14  Model/Cs = 02
#define DECADE_VARCAP_SET_WR_CMD		0x00811502	// cmd = 0x008, scope = 0x1, add = 0x15  Model/Cs = 02


#define DECADE_ADDON_CPLD				0x00800001	
#define	DECADE_ADC1						0x00810002					// ADC TI ADC12D1600
#define	DECADE_ADC2						0x00810003					// ADC TI ADC12D1600
#define	DECADE_BOTH_ADCs				0x00810007					// Both ADC1 and ADC2
#define DECADE_DAC_MAX5135				0x00810005					// DAC MAXIM MAX5135 for DC offset
#define	DECADE_EXTTRIG_DAC				0x00810006					
#define DECADE_SPI_WRITE				0x10000	

// Addon CPLD registers
#define DECADE_ADDON_CPLD_FRONTEND_CH1	( DECADE_ADDON_CPLD | 0x0000 )	
#define DECADE_ADDON_CPLD_FRONTEND_CH2	( DECADE_ADDON_CPLD | 0x0100 )	
#define	DEACDE_CLOCK_CONTROL			( DECADE_ADDON_CPLD | 0x0200 )
#define DECADE_ADDON_CPLD_MISC_CONTROL	( DECADE_ADDON_CPLD | 0x0300 )	
#define	DECADE_TRIGGER_IO_CONTROL		( DECADE_ADDON_CPLD | 0x1000 )
#define DECADE_ADDON_CPLD_FW_DATE		( DECADE_ADDON_CPLD | 0x1E00 )	
#define DECADE_ADDON_CPLD_FW_VERSION	( DECADE_ADDON_CPLD | 0x1F00 )	
#define DECADE_ADDON_CPLD_MISC_STATUS	( DECADE_ADDON_CPLD | 0x4000 )	
#define DECADE_ADDON_CPLD_GEN_COMMAND	( DECADE_ADDON_CPLD | 0x2F00 )	 


#define DECADE_COARSE_OFFSET_0			(DECADE_DAC_MAX5135 | 0x3100)
#define DECADE_FINE__OFFSET_0			(DECADE_DAC_MAX5135 | 0x3200)
#define DECADE_COARSE_OFFSET_1			(DECADE_DAC_MAX5135 | 0x3400)
#define DECADE_FINE__OFFSET_1			(DECADE_DAC_MAX5135 | 0x3800)
#define	DECADE_DAC_OFFSET				(DECADE_DAC_MAX5135 | 0x3F00)

#define	DECADE_FE_ATT20DB_RLY_BIT		0x2			// Decade FrontEnd Att20Db_rly bit
#define	DECADE_CH01_OVER_DETECTED		0x8			// Over Protection Detected bit

#define DECADE_SETFRONTEND_CMD			0x2300000
#define	DECADE_EXT_TRIG_ALIGN			0x3000000
#define	DEACDE_RESET_PLL				0x3100000
#define	DEACDE_ADC_LVDS_ALIGN			0x3300000
#define	DEACDE_ADC_LVDS_ALIGN_SHORT		0x3600000
#define	DEACDE_ADC_INTERNAL_CALIB		0x3800000
#define	DEACDE_ADC_IQ_OFFSET_CALIB		0x4100000
#define	DEACDE_ADC_IQ_GAIN_CALIB		0x4200000
