//! CsHexagon.h
//!
//!

#ifndef __CsHexagon_h__
#define __CsHexagon_h__

#include "CsDefines.h"
#include "CsTypes.h"
#include "CsPlxDefines.h"
#include "CsDeviceIDs.h"
#include "CsAdvStructs.h"
#include "CsiHeader.h"
#include "CsFirmwareDefs.h"
#include "CsHexagonFlash.h"
#include "CsHexagonCal.h"
#include "CsHexagonCapsInfo.h"
#include "CsHexagonOptions.h"
#include "CsEventLog.h"

#include "CsPlxBase.h"
#include "CsNiosApi.h"

#include "CsDrvStructs.h"
#include "DrvCommonType.h"
#include "DllStruct.h"
#include "GageWrapper.h"
#include "CsSharedMemory.h"

#include "CsFirmwareDefs.h"
#include "CsHexagonState.h"

//#define DBG_TRIGGER_SETTING

#define HEXAGON_DBG_INFOS_CMD				0xFF00				// Calib Info Dump Cmd
#define HEXAGON_DBG_ADV_INFOS_CMD			0xFE00				// Calib Advance Dump Cmd
#define HEXAGON_DBG_INFOS_CMD_MASK			HEXAGON_DBG_INFOS_CMD

#define	SHARED_CAPS_NAME			_T("CsE16bcdCaps")
#define MEMORY_MAPPED_SECTION		_T("CsE16bcdWDF-shared")	// IMPORTANT: Must be the DLL name plus "-shared" (Compatible with RM)
#define	HEXAGON_LOWRANGE_EXT_NAME	_T("-LR")

#define	COMMIT_TIME_DELAY_MS	50
#define HEXAGON_DB_MAX_TABLE_SIZE_DMP 256

#define REGULAR_INDEX_OFFSET	1		// 0: Boot, 1: Reg, 2: Expert1, 3: Expert2
#define EXPERT_INDEX_OFFSET		2		// 0: Boot, 1: Reg, 2: Expert1, 3: Expert2

// Max depth counter registers (29 bits)
#define HEXAGON_MAX_HW_DEPTH_COUNTER			(0x20000000-1)

// Max depth counter registers (32 bits)
#define HEXAGON_MAX_HW_TRIGGER_DELAY			(0x100000000-1)

// Max segment size for eXpert averaging (in samples)
#define	HEXAGON_MAX_HWAVG_DEPTH				(4092*16*2)
// Max Numver of Average for eXpert averaging (in samples)
#define	HEXAGON_MAX_NUMBER_OF_AVERAGE		(64*1024)
#define	HEXAGON_FFTSIZE_MAX_SUPPORTED		8192

// Amount of memory in bytes at the end of each record that HW reserved to store ETBs and Timestamps
#define	HEXAGON_HWMULREC_TAIL_BYTES			128


#define	SPI_READ_CMD				0x00800000
#define	SPI_WRTIE_CMD				0x00810000
#define	NIOS_WRTIE_BIT				0x00010000

#define SAFEBOOT_FW			0x1
#define DEFAULT_FW			0x2
#define EXPERT1_FW			0x4
#define EXPERT2_FW			0x8	
#define EXPERT_FW			(EXPERT1_FW | EXPERT2_FW)


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
	FFT				= CS_BBOPTIONS_FFT_8K
} eXpertId;

typedef enum
{
	SampleClk		= 0,
	ExternalClk		= 1,
	ReferenceClk	= 2
} CLOCK_MODE;

typedef struct _HEXAGON_CAL_LUT
{                           
	uInt32	u32FeSwing_mV;
	float	Vref_mV;								// same value for Vref+ and Vref-
	uInt16	u16GainCtrl;
} HEXAGON_CAL_LUT, *PHEXAGON_CAL_LUT;


// Limitation of sofware averaging
#define MAX_SW_AVERAGING_SEMGENTSIZE		512*1024		// Max segment size in Software averaging mode (in samples)
#define	SW_AVERAGING_BUFFER_PADDING			16				// Padding for Sw Avearge buffer
#define MAX_SW_AVERAGE_SEGMENTCOUNT			4096			// Max segment count in SW averaging mode
#define	MAX_PRETRIGGERDEPTH_SINGLE			130880			// Max pretrigger depth in single channel mode 128K 
															// round down to trigger resolution

#define HEXAGON_DEFAULT_VALUE				0	
#define HEXAGON_DEFAULT_RELATION			CS_RELATION_OR
#define HEXAGON_DEFAULT_LEVEL				0

//!----- Default settings
#define HEXAGON_DEFAULT_TRIG_SENSITIVITY	2048			// half of 12 bits
#define HEXAGON_DEFAULT_EXT_TRIG_SENSE		60				// VC TBC. This param not used ?.
#define HEXAGON_DEFAULT_SAMPLE_RATE			-1				// Force to the default sampling rate depending on card
#define HEXAGON_DEFAULT_EXT_CLOCK_EN		0
#define HEXAGON_DEFAULT_SAMPLE_BITS			CS_SAMPLE_BITS_12
#define HEXAGON_DEFAULT_RES					(-1*CS_SAMPLE_RES_LJ)
#define HEXAGON_DEFAULT_SAMPLE_SIZE			CS_SAMPLE_SIZE_2
#define HEXAGON_DEFAULT_SAMPLE_OFFSET		CS_SAMPLE_OFFSET_12_LJ
#define HEXAGON_DEFAULT_SEGMENT_COUNT		1
#define HEXAGON_DEFAULT_DEPTH				8192
#define HEXAGON_DEFAULT_SEGMENT_SIZE		HEXAGON_DEFAULT_DEPTH
#define HEXAGON_DEFAULT_TRIG_TIME_OUT		-1
#define HEXAGON_DEFAULT_TRIG_ENGINES_EN		CS_TRIG_ENGINES_DIS
#define HEXAGON_DEFAULT_TRIG_HOLD_OFF		0
#define HEXAGON_DEFAULT_TRIG_DELAY			0

#define HEXAGON_DEFAULT_CHANNEL_INDEX		0
#define HEXAGON_DEFAULT_TERM				CS_COUPLING_DC
#define HEXAGON_DEFAULT_GAIN				CS_GAIN_2_V
#define HEXAGON_DEFAULT_INPUTRANGE			-1				// Force to the default input range depending on card
#define HEXAGON_DEFAULT_IMPEDANCE			CS_REAL_IMP_50_OHM
#define HEXAGON_DEFAULT_FILTER				0
#define HEXAGON_DEFAULT_POSITION			0

#define HEXAGON_DEFAULT_CONDITION			CS_TRIG_COND_POS_SLOPE
#define HEXAGON_DEFAULT_SOURCE				CS_CHAN_1
#define HEXAGON_DEFAULT_EXT_COUPLING		CS_COUPLING_DC				// Hexagon AC
#define HEXAGON_DEFAULT_EXT_GAIN			3300		                // 3.3 V
#define HEXAGON_DEFAULT_EXT_IMPEDANCE		CS_REAL_IMP_1K_OHM          // 1K

#define HEXAGON_DEFAULT_CLKOUT				eclkoNone	

#define HEXAGON_DEFAULT_CLOCK_REG			0x41		// Default: Internal clock, Sample clock out
#define HEXAGON_DEFAULT_TRIGIO_REG			0xF			// Default: Trig out disabled
														// TRIGGER_IN_CAL_SEL = always(1)
														// EXT_TRIG_OUT_XP_EN = always(1)  (Firmware 49 and up)
														// EXT_TRIG_OUT_XP_SEL = always(1)
														// LDVS_TRIG_RX_EN = always(1)
														// LDVS_TRIG_RX_EN = always(0)

#define NCO_CONSTANT						0x10000000		// DDC internal NCO value				

// Trigger Engine Index in Table
#define	CSRB_TRIGENGINE_EXT		0
#define	CSRB_TRIGENGINE_A1		1
#define	CSRB_TRIGENGINE_B1		2
#define	CSRB_TRIGENGINE_A2		3
#define	CSRB_TRIGENGINE_B2		4
#define	CSRB_TRIGENGINE_A3		5
#define	CSRB_TRIGENGINE_B3		6
#define	CSRB_TRIGENGINE_A4		7
#define	CSRB_TRIGENGINE_B4		8

#define	HEXAGON_TOTAL_TRIGENGINES	9

#define CSRB_GLB_TRIG_ENG_ENABLE	0x0001

//----- Self Test _mode
#define HEXAGON_CAPTURE_DATA	0x0000
#define HEXAGON_SELF_COUNTER	0x0004

// Valid Expert Firmware
#define HEXAGON_VALID_EXPERT_FW		(CS_BBOPTIONS_STREAM | CS_BBOPTIONS_MULREC_AVG_TD | CS_BBOPTIONS_FFT_8K)

typedef enum 
{
	 formatDefault = 0,
	 formatHwAverage,
	 formatSoftwareAverage,
	 formatFIR
} 
_CSDATAFORMAT;


class CsHexagon : public CsPlxBase,  public CsNiosApi
{
private:
	PGLOBAL_PERPROCESS		m_ProcessGlblVars;

#if !defined(USE_SHARED_CARDSTATE)
	CSHEXAGON_DEVEXT		m_CardState;
#endif
	PCSHEXAGON_DEVEXT		m_pCardState;
	BOARD_VOLATILE_STATE	m_VolatileState;
	void					GetCardState();
	void					UpdateCardState( PCSHEXAGON_DEVEXT pCardSt=0 );
	uInt32					m_u32SystemInitFlag;	// Compuscope System initialization flag
	uInt32					m_u32SrInfoSize;
	KERNEL_MISC_STATE		m_KernelMiscState;

	static const uInt32 c_u32CalDacCount;		
	static const uInt32 c_u32CalAdcCount;

	static HEXAGON_CAL_LUT	s_CalCfg50[];

	const static FRONTEND_HEXAGON_INFO		c_FrontEndGain50[];
	const static FRONTEND_HEXAGON_INFO	c_FrontEndGainLowRange50[];

	const static HEXAGON_SAMPLE_RATE_INFO	Hexagon_SR[];

	CLOCK_MODE				m_ClockMode;
	uInt16					m_u16TrigSensitivity;			// Digital Trigger sensitivity;
	int32					m_i32ExtTrigSensitivity;		// ExtTrigger Trigger sensitivity;
	CsEventLog				m_EventLog;

public:
	CsHexagon*			m_MasterIndependent;	// Master /Independant card
	CsHexagon*			m_Next;					// Next Slave
	CsHexagon*			m_HWONext;				// HW OBJECT next
	DRVHANDLE			m_PseudoHandle;			// User level card handle
	DRVHANDLE			m_CardHandle;			// Handle from Kernel Driver
	uInt8				m_u8CardIndex;			// Card index in native card detection order
	uInt16				m_DeviceId;

	PDRVSYSTEM			m_pSystem;				// Pointer to DRVSYSTEM
	CsHexagon*			m_pTriggerMaster;		// Point to card that has trigger engines enabled

	// Contructor
	CsHexagon( uInt8 u8Index, PCSDEVICE_INFO pDevInfo, PGLOBAL_PERPROCESS ProcessGlblVars);


protected:
	eCalMode				m_CalMode;
	int16*					m_pi16CalibBuffer;
	int32*					m_pi32CalibA2DBuffer;
	
	CSACQUISITIONCONFIG		m_OldAcqConfig;
	CS_TRIGGER_PARAMS		m_CfgTrigParams[HEXAGON_TOTAL_TRIGENGINES];
	CS_TRIGGER_PARAMS		m_OldTrigParams[HEXAGON_TOTAL_TRIGENGINES];
	CS_CHANNEL_PARAMS		m_OldChanParams[HEXAGON_CHANNELS];

	//----- Unit number for this device
	bool					m_bForceCal[HEXAGON_CHANNELS];		
	bool					m_bXS_Depth;					// Extra Small Segment enabled

	char					m_szCsiFileName[MAX_FILENAME_LENGTH];	// Csi file name
	char					m_szFwlFileName[MAX_FILENAME_LENGTH];	// fwl file name
	uInt32                  m_u32IgnoreCalibError;

	CSI_ENTRY				m_BaseBoardCsiEntry;
	FPGA_HEADER_INFO		m_BaseBoardExpertEntry[2];		// Active Xpert on base board
	uInt32					m_u32DefaultExtClkSkip;
			
	uInt32					m_u32HwSegmentCount;
	int64					m_i64HwSegmentSize;
	int64					m_i64HwPostDepth;
	int64					m_i64HwTriggerDelay;
	int64					m_i64HwTriggerHoldoff;
	uInt32					m_32SegmentTailBytes;
	
	// HEXAGON registers
	CsCaptureMode			m_CaptureMode;
	STREAMSTRUCT			m_Stream;						// Stream state
	CS_TRIG_OUT_CONFIG_PARAMS	m_TrigOutCfg;

	//----- Variable related to Data Transfer
	CHANNEL_DATA_TRANSFER	m_DataTransfer;

	BOOLEAN				m_UseIntrTrigger;		
	BOOLEAN				m_UseIntrBusy;			

	BOOLEAN				m_bSoftwareAveraging;		// Mode Software Averaging enabled
	bool				m_bMulrecAvgTD;				// Mode Expert Multiple Record AVG TD
	uInt32				m_u32MulrecAvgCount;		// Mulrec AVG count (should be <= 1024)
	BOOLEAN				m_bExpertAvgFullRes;		// Mode Harware Averaging in full resolution (32bits)
	int16				*m_pAverageBuffer;			// Temporarary buffer to keep the result of avearging
	BOOLEAN					m_bNeverCalibExtTrig;		// No Calibration for External required

	// HEXAGON CALIB
	bool			m_bLogFailureOnly;
	bool			m_bUseEepromCal;

	uInt16			m_u16SkipCalibDelay; //in minutes;
	bool			m_bNeverCalibrateDc;
	bool			m_bNeverCalibrateGain;
	bool			m_bSkipCalib;

	bool			m_bCalibActive;					// In calibration mode
	uInt32			m_u32CalSrcSettleDelay;
	uInt32			m_u32DebugCalibTrace;
	uInt16			m_u16GainCode_2vpp;
	bool			m_bNoDCLevelFreeze;
	bool			m_bNoDCLevelFreezePreset;
	bool			m_bLowRange_mV;					// Board with low input range 240mV
	uInt32			m_u32CommitTimeDelay;			// CommitDelayTime in ms second. wait for hardware stable.
	uInt32			m_u32GainTolerance_lr;			// low range Gain tolerance for calibration.


	// Variable for FFT support
	CS_FFT_CONFIG_PARAMS	m_FFTConfig;
	int16					m_i16CoefficientFFT[HEXAGON_FFTSIZE_MAX_SUPPORTED];
	FFTCONFIG_REG			m_u32FftReg;				// Last value written to FFT config register
	bool					m_bFFTsupported;
	bool					m_bFFT_enabled;
	char					szDbgText[256];				// Debug text
	char					szErrorStr[128];			// Error log

	//----- CsHexagon.cpp
	int32  MsCsResetDefaultParams( BOOLEAN bReset = FALSE );
	int32  CsSetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg = NULL );
	int32  CsSetTriggerConfig( PARRAY_TRIGGERCONFIG pATrigCfg = NULL );
	int32  CsSetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg = NULL );

	int32  RemoveDeviceFromUnCfgList();
	void   AddDeviceToInvalidCfgList();
	
	uInt16 BuildIndependantSystem( CsHexagon** FirstCard, BOOLEAN bForceInd );
	uInt16 DetectAcquisitionSystems(BOOL bForceInd); 

	void	TraceCalib(uInt16 u16Channel, uInt32 u32TraceFlag, uInt32 u32V1 = 0, uInt32 u32V2=0, int32 i32V3=0, int32 i32V4=0);
	void	TraceDebugCalib(uInt16 u16Channel, uInt32 u32TraceFlag, uInt32 u32V1 = 0, uInt32 u32V2=0, int32 i32V3=0, int32 i32V4=0);
	
	int32	CsGetAcquisitionConfig( PCSACQUISITIONCONFIG pAcqCfg );
	int32   CsGetTriggerConfig( PARRAY_TRIGGERCONFIG pATriggerCfg );
	int32	CsGetChannelConfig( PARRAY_CHANNELCONFIG pAChanCfg );
	
	int32  GetFreeTriggerEngine( int32	i32TrigSource, uInt16 *u16TriggerEngine );
	uInt16 NormalizedChannelIndex( uInt16 u16ChannelIndex );
	int32  NormalizedTriggerSource( int32 i32TriggerSource );

	int32 SendClockSetting(LONGLONG llSampleRate = HEXAGON_DEFAULT_SAMPLE_RATE, BOOLEAN bExtClk = FALSE, BOOLEAN bRefClk = FALSE);
	int32 SendCaptureModeSetting(uInt32 u32Mode = CS_MODE_DUAL);
	int32 SendCalibModeSetting( eCalMode ecMode = ecmCalModeNormal );
	int32 RestoreCalibModeSetting();
	int32 SendChannelSetting(uInt16 Channel, uInt32 u32InputRange = HEXAGON_DEFAULT_INPUTRANGE, uInt32 u32Term = HEXAGON_DEFAULT_TERM, uInt32 u32Impendance = HEXAGON_DEFAULT_IMPEDANCE, int32 i32Position = 0, uInt32 u32Filter = 0, int32 i32ForceCalib = 0);
	// Hexagon board has slope inverse (0: neg, 1: pos)
	int32 SendTriggerEngineSetting(  uInt16	u16EngineTblIdx,
											int32	i32SourceA1 = 0 ,
											uInt32  u32ConditionA1 = CS_TRIG_COND_POS_SLOPE,
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
										   uInt32	u32ExtCondition = HEXAGON_DEFAULT_CONDITION,
										   uInt32	u32ExtTriggerRange =  HEXAGON_DEFAULT_EXT_GAIN, 
										   uInt32	u32ExtTriggerImped =  HEXAGON_DEFAULT_EXT_IMPEDANCE, 
										   uInt32	u32ExtCoupling = HEXAGON_DEFAULT_EXT_COUPLING,
										   int32	i32Sensitivity = HEXAGON_DEFAULT_EXT_TRIG_SENSE );
	
	int32 SendDigitalTriggerSensitivity( uInt16  u16TrigEngine, uInt16 u16Sensitive = HEXAGON_DEFAULT_TRIG_SENSITIVITY );
	int32 SendExtTriggerLevel( int32 i32Level );

	int32 WriteToSelfTestRegister( uInt16 SelfTestMode = CSST_DISABLE );
	int32 InitAddOnRegisters(bool bForce = false);

	void	AssignBoardType();
	void	AssignAcqSystemBoardName( PCSSYSTEMINFO pSysInfo );
	int32	AcqSystemInitialize();
	int32	CalibrateAllChannels(bool bForce = false);

	int32	GetFEIndex(uInt16 u16Channel);
	void	FillOutBoardCaps(PCSHEXAGON_BOARDCAPS pCapsInfo);
	int32	WriteCalibTableToEeprom(uInt8	u8CalibTableId, uInt32 u32Valid );

	eClkOutMode GetClockOut(void){ return m_pCardState->eclkoClockOut; }
	eTrigEnableMode GetExtTrigEnable(void){return m_pCardState->eteTrigEnable;}
	
	int32	SendSegmentSetting(uInt32 u32SegmentCount, int64 i64PostDepth, int64 i64SegmentSize, int64 i64HoldOff, int64 i64TrigDelay = 0);
	bool	ForceFastCalibrationSettingsAllChannels();

	void	ReadVersionInfo();
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

	int32	CommitNewConfiguration( PCSACQUISITIONCONFIG pAcqCfg,
									PARRAY_CHANNELCONFIG pAChanCfg,
									PARRAY_TRIGGERCONFIG pATrigCfg );
	void	ResetCachedState(void);

	void	RestoreCalibDacInfo();
	void	SetDefaultAuxIoCfg();

public:
	CsHexagon *GetCardPointerFromChannelIndex( uInt16 ChannelIndex );
	CsHexagon *GetCardPointerFromBoardIndex( uInt16 BoardIndex );
	CsHexagon *GetCardPointerFromTriggerIndex( uInt16 TriggerIndex );
	CsHexagon *GetCardPointerFromTriggerSource( int32 i32TriggerSoure );

	int32   CsGetBoardsInfo( PARRAY_BOARDINFO pABoardInfo );
	int32	DeviceInitialize(bool bForce = false);
	int32	CsAutoUpdateFirmware();

	int32  AcquireData(uInt32 u32IntrEnabled = OVER_VOLTAGE_INTR);
	void   MsAbort();

	uInt32	GetDataMaskModeTransfer ( uInt32 u32TxMode );
	uInt32	GetModeSelect( uInt16 u16UserChannel );

	//----- Read/Write the Add-On Board Flash 
	int32	ReadCalibInfoFromCalFile();
	int32	CalibrateFrontEnd(uInt16 u16Channel);
	int32	CalibrateOffset(uInt16	u16HwChannel);

	uInt16	ConvertToHwChannel( uInt16 u16UserChannel );
	uInt16	ConvertToUserChannel( uInt16 u16HwChannel );
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
	int32	CalibrateGain(uInt16 u16Channel);
	int32	CalibratePosition(uInt16 u16Channel);
	int32	SetCalibrationRange(uInt32 u32Range_mV);
	int32	SendDigitalTriggerLevel( uInt16  u16TrigEngine, int32 i32Level );

	BOOLEAN	FindSrInfo( const LONGLONG llSr, PHEXAGON_SAMPLE_RATE_INFO *pSrInfo );

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
	
	int32	SetupForCalibration(bool bSetup = false);
	int32	SetupForReadFullMemory(bool bSetup);

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

	bool	IsBootLocationValid( uInt8 u8Index );
	int32	WriteBaseBoardFlash(uInt32 Addr, void *pBuffer, uInt32 WriteSize, bool bBackup = FALSE );
	int32	IoctlGetPciExLnkInfo( PPCIeLINK_INFO pPciExInfo );

	// Overload functions to support Nulceon
	int32	CsReadPlxNvRAM( PPLX_NVRAM_IMAGE nvRAM_image );
	int32	CsWritePlxNvRam( PPLX_NVRAM_IMAGE nvRAM_image );

	int32	ReadHardwareInfoFromFlash();
	int32	ReadIoConfigFromFlash();
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

#ifdef USE_SHARED_CARDSTATE
	int32	InitializeDevIoCtl();
#endif

	uInt16	ReadBusyStatus();

	int32	CsCheckPowerState()
	{	if ( 0 == m_pTriggerMaster->m_pCardState->bCardStateValid )
			return CS_POWERSTATE_ERROR;
		else
			return CS_SUCCESS;
	}

	int32	SendExtTriggerAlign();
	int32	SendADC_DC_Level_Freeze( uInt32	u32Option, bool bForce = false );
	int32	LockPLL();
	bool	isPLLLocked();		
	int32 	ResetPLL();	
	int32	UpdateCalibDac(uInt16 u16Channel, eCalDacId ecdId, uInt16 u16Data, bool bVal=false);
	int32   DumpAverageTable(uInt16 u16Channel, eCalDacId ecdId, uInt16 u16Vref=0, uInt16 u16Gain=0, bool bForceMode=false, uInt16 NbSteps=16, bool bZoomMode=false, uInt16 u16ZommAddr=0, uInt16 u16ZoomFactor=0 );		
	int32   DumpCalibAvgTable(uInt16 u16Channel, uInt32 u32Params);		
	
	BOOLEAN ConfigurationReset( uInt32	u32BootLocation );
	int32	SetClockControlRegister( uInt32 u32Val, bool bForce = false );
	int32	SetTriggerIoRegister( uInt32 u32Val, bool bForce = false );
	int32	SetTrigOut( uInt16 u16Value );
	int32	SetClockOut( eClkOutMode ClkOutValue = eclkoNone, bool bForce = false );
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
	int32	CheckOverVoltage();


	void	DetectBootLocation();
	int32	FindCalLutItem( const uInt32 u32Swing, const uInt32 u32Imped, HEXAGON_CAL_LUT& Item );
	int32	FindFeCalIndices( uInt16 u16Channel, uInt32& u32RangeIndex,  uInt32& u32FeModIndex );
	uInt16	GetControlDacId( eCalDacId ecdId, uInt16 u16Channel );
	uInt16	GetUserChannelSkip();
	void	SetDefaultInterrutps();

	int32	ForceGainControl(const uInt32 u32Swing, uInt16 u16GainCode);
	int32	CaptureCalibSignal(bool bWaitTrigger = false);
	int32	WriteRegisterThruNiosEx( uInt32 Data, uInt32 Command, int32 i32Timeout, uInt32 *pu32NiosReturn );
    int		DumpBuffer(char *Label, uInt8 *pBuf, int int32);
	int32	FindCalibOffset(uInt16 u16HwChannel, eCalDacId ecdId, bool bIsGoUp, uInt16 *bBestOffset, int32 *pi32Avg_Meas);
	int32	WriteToCalibDac(uInt16 Addr, uInt16 u32Data, uInt8 u8Delay = 0);
	int32	ReadCalibADCRef(int32 *pAvg_x10);
	int32	ValidateAllChannelGain();
	int32	PresetDCLevelFreeze();
	int32	DbgAcquireAndAverage(uInt16 u16Channel, uInt32 Params );
	int32	DbgMain( uInt32 Options, uInt32 Params );
	void	DbgShowHelp();
	int32	DbgShowCalibInfo();
	int32	DbgAvgTable(uInt16 u16Channel, uInt32 Params);
	int32	DbgAvgTableZoom(uInt16 u16Channel, uInt32 Params);
	void	DbgCalibRegShow();
	int32	DbgTriggerCfgShow();
	int32	DbgCalibPosition(uInt16 u16Channel);
	int32	DbgDumpAddonReg();
	int32	DbgDumpNiosReg();
	bool	SameSign(int32 x, int32 y) {return (x >= 0) ^ (y < 0);}

	void	DumpTriggerParams(char *Str = NULL);
	void	DumpTriggerConfig(PARRAY_TRIGGERCONFIG pATrigCfg);
	void	DumpCfgTrigParams(char *Str);
	int32	DbgAdvMain( uInt32 Options, uInt32 Params );
	void	DbgAdvShowHelp();
	int32	DbgAdvAvgTable(uInt16 u16Channel, uInt32 Params);
	int32	DbgAdvDCOffset(uInt16 u16Channel, uInt32 Params);
	int32	DbgRestoreCalibSetting(uInt16 u16Channel);
	int32	DumpOutputMsg(char *szMsg);
	int32	DbgGainBoundary();
	int32	DbgBoardSummary(uInt32 Params);
	int32	DbgGetBoardsInfo(PCSBOARDINFO	pCsBoardInfo);
	int32	DbgRecalibrate(uInt32 Params);
	int32	ZoomInMaxFromOffset(uInt16 u16Channel, uInt32 Params);
	bool	IsBoardLowRange();
	int32	DbgAdvZeroOffset(uInt16 u16Channel, uInt32 Params);
};


// Prototype for functions
unsigned int WINAPI ThreadFuncIntr( LPVOID lpParam );
unsigned int WINAPI ThreadFuncTxfer( LPVOID lpParam );

#endif

// HOST REGISTERMAP
// Hexagon registers for Firmware versions and options
#define	HEXAGON_OPT_RD_CMD			0x30
#define HEXAGON_SWR_RD_CMD			0x34
#define HEXAGON_HWR_RD_CMD			0x38
#define HEXAGON_FFT_USER_DATA		0x1CC
#define HEXAGON_FFT_USER_INDEX		0x1D0
#define HEXAGON_FFT_CONTROL			0x1D4

// Hexagon BaseBoard Register
#define	HEXAGON_SET_MULREC_AVG_COUNT		0x00210035
#define	HEXAGON_BB_TRIGTIMEOUT_REG			0x00210049
#define	HEXAGON_BB_FPGA_TEMPERATURE			0x00200063
#define HEXAGON_DATA_SELECT					0x002100C1
#define HEXAGON_CHANNEL_CTRL				0x002100C5
#define HEXAGON_DECIMATION_SETUP			0x002100C6
#define HEXAGON_TRIGENGINES_EN				0x002100D4			// Trigger Engines Enable
#define HEXAGON_DT_CONTROL					0x002100D6
#define HEXAGON_TRIGOUT_DIS					0x002100A1


#define HEXAGON_RD_BB_REGISTER				0x00200000			// Read Base board registers command
#define HEXAGON_DIGITAL_ENGINE				0x002100A4
#define HEXAGON_EXT_TRIGGER_SETTING			0x002100A5
#define HEXAGON_DLA1						0x002100E0
#define HEXAGON_DSA1						0x002100E1
#define HEXAGON_DLB1						0x002100E2
#define HEXAGON_DSB1						0x002100E3
#define HEXAGON_DLA2						0x002100E4
#define HEXAGON_DSA2						0x002100E5
#define HEXAGON_DLB2						0x002100E6
#define HEXAGON_DSB2						0x002100E7

#define	HEXAGON_USER_LED_CRTL				0x00210050			// BB register0
#define	HEXAGON_BB_TRIGGER_AC_CALIB_REG		0x00210051			// BB register1
#define	HEXAGON_BB_DC_CALIB_REG				0x00210052			// BB register2

#define HEXAGON_CALIB_ADC112S_CTRL			0x00810040			// Baseboard Calib ADC 

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





//Verbose

#define HEXAGON_DIG_TRIGLEVEL_SPAN	0x10000		// 16 bits

#define HEXAGON_CALIB_DEPTH			4000
#define HEXAGON_CALIB_BUF_MARGIN	160
#define HEXAGON_CALIB_BUFFER_SIZE	(HEXAGON_CALIB_BUF_MARGIN + HEXAGON_CALIB_DEPTH)
#define HEXAGON_CALIB_BLOCK_SIZE	64
#define HEXAGON_CALIB_BLOCK_MARGIN  10


#define HEXAGON_CHAN1_SET_WR_CMD		0x00811102	// cmd = 0x008, scope = 0x1, add = 0x11  Model/Cs = 02
#define HEXAGON_CHAN2_SET_WR_CMD		0x00811202	// cmd = 0x008, scope = 0x1, add = 0x12  Model/Cs = 02
#define HEXAGON_CHAN3_SET_WR_CMD		0x00811302	// cmd = 0x008, scope = 0x1, add = 0x13  Model/Cs = 02
#define HEXAGON_CHAN4_SET_WR_CMD		0x00811402	// cmd = 0x008, scope = 0x1, add = 0x14  Model/Cs = 02
#define HEXAGON_VARCAP_SET_WR_CMD		0x00811502	// cmd = 0x008, scope = 0x1, add = 0x15  Model/Cs = 02


#define HEXAGON_ADDON_CPLD				0x00800001	
#define HEXAGON_VGA_CH1					0x00800202              // LMH6401 VGA 0	
#define HEXAGON_VGA_CH2					0x00800212              // LMH6401 VGA 1	
#define HEXAGON_VGA_CH3					0x00800222              // LMH6401 VGA 2	
#define HEXAGON_VGA_CH4					0x00800232              // LMH6401 VGA 3	

#define HEXAGON_ADC1					0x00800003				// ADC TI ADS54J60 Ch1 + Ch2
#define HEXAGON_ADC2					0x00800004				// ADC TI ADS54J60 Ch3 + Ch4
#define HEXAGON_EXTTRIG_DAC				0x00800005				// LTC2641 Ext Trig DAC	
#define HEXAGON_DAC_SPI_SYNC			0x00800006				// DAC TI 128S085 for DC offset
#define HEXAGON_LMK_SPI_CS				0x00800007				// LMK04828 PLL
#define HEXAGON_CAL_ADC_SPI				0x00800008				// ADS8681 ADC					

#define HEXAGON_SPI_WRITE				0x10000	
#define HEXAGON_SPI_WRITE				0x10000	
#define HEXAGON_BOTH_ADCs				0x00810007				// Both ADC1 and ADC2 VC obsolete


// Addon CPLD registers
#define HEXAGON_ADDON_CPLD_MISC_STATUS	( HEXAGON_ADDON_CPLD | 0x0000 )	
#define HEXAGON_ADDON_CPLD_EXTCLOCK_REF	( HEXAGON_ADDON_CPLD | 0x0100 )	
#define HEXAGON_ADDON_CPLD_CALSRC_REF	( HEXAGON_ADDON_CPLD | 0x0200 )	
#define HEXAGON_ADDON_CPLD_CALSEL_CHAN	( HEXAGON_ADDON_CPLD | 0x0300 )	
#define HEXAGON_TRIGGER_IO_CONTROL		( HEXAGON_ADDON_CPLD | 0x1000 )
#define HEXAGON_ADDON_CPLD_FW_DATE		( HEXAGON_ADDON_CPLD | 0x1E00 )	
#define HEXAGON_ADDON_CPLD_FW_VERSION	( HEXAGON_ADDON_CPLD | 0x1F00 )	
#define HEXAGON_ADDON_CPLD_GEN_COMMAND	( HEXAGON_ADDON_CPLD | 0x2F00 )	 


#define HEXAGON_ADDON_CPLD_MISC_CONTROL	( HEXAGON_ADDON_CPLD | 0x0300 )	//VC to be changed

// Source select for ref calibration register.
#define HEXAGON_CAL_SRC_0V				1	
#define HEXAGON_CAL_SRC_TRIG			2	
#define HEXAGON_CAL_SRC_POS				4	
#define HEXAGON_CAL_SRC_NEG				8	

#define HEXAGON_COARSE_OFFSET1_CS		0x0000
#define HEXAGON_FINE_OFFSET1_CS			0x0100
#define HEXAGON_COARSE_OFFSET2_CS		0x0200
#define HEXAGON_FINE_OFFSET2_CS			0x0300
#define HEXAGON_COARSE_OFFSET3_CS		0x0400
#define HEXAGON_FINE_OFFSET3_CS			0x0500
#define HEXAGON_COARSE_OFFSET4_CS		0x0600
#define HEXAGON_FINE_OFFSET4_CS			0x0700

#define HEXAGON_GAIN1_CS				0x02
#define HEXAGON_GAIN2_CS				0x12
#define HEXAGON_GAIN3_CS				0x22
#define HEXAGON_GAIN4_CS				0x32

#define HEXAGON_WRITE_TO_DAC128S085_CMD 0x00810006	//----cmd = 0x008, scope = 0x1, add = 0x00XX
#define HEXAGON_WRITE_TO_LMH6401_CMD	0x00810200	//----cmd = 0x008, scope = 0x1, register = 0x2 Gain Control

#define HEXAGON_DAC_WTM_MODE			0x9000		// WTM mode for DAC128S085
#define HEXAGON_DAC_WRM_MODE			0x8000		// WRM mode for DAC128S085

// MISC NIOS COMMANDS
#define	HEXAGON_EXT_TRIG_ALIGN			0x3000000
#define	HEXAGON_RESET_PLL				0x3100000
#define	HEXAGON_ADC_DC_LEVEL_FREEZE		0x3300000
#define	HEXAGON_ADC_LVDS_ALIGN_SHORT	0x3600000
#define HEXAGON_CLOCKOUT_CTRL			0x3800000	
#define	HEXAGON_ADC_IQ_OFFSET_CALIB		0x4100000
#define	HEXAGON_ADC_IQ_GAIN_CALIB		0x4200000


#define HEXAGON_DC_OFFSET_DEFAULT_VALUE		0x800		// no offset
#define HEXAGON_DC_GAIN_CODE_1V				0x08		// 1V
#define HEXAGON_DC_GAIN_CODE_LR				2			// 480mV
#define HEXAGON_DC_NB_GAIN_CONTROL			31			// 0-20h
#define HEXAGON_GAIN_CODE_MIN				0			// 0-20h
#define HEXAGON_GAIN_CODE_MIN_ALLOWED		4			// Gain code hirange (2000mv) calib End.
														// Do not cross this boundary, or damage ADC
#define HEXAGON_GAIN_CODE_MAX				32			// 0-20h
#define HEXAGON_GAIN_CODE_HR_START			12			// Gain code hirange (2000mv) calib start.
#define HEXAGON_GAIN_CODE_LR_START			6			// Gain code lowrange (480mv) calib start. 
#define HEXAGON_GAIN_LR_MV					480			// Low input range 480 mV pk-pk
#define	HEXAGON_GAIN_CODE_INVALID			0xFF
#define HEXAGON_NO_GAIN_SETTING				HEXAGON_GAIN_CODE_INVALID

#define DEFAULT_GAIN_TOLERANCE_LR			10			// 10% of Vref

