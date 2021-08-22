#ifndef GAGESYSTEM_H
#define GAGESYSTEM_H

#include <QtGlobal>
#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include "cstestqttypes.h"
#include "QLibrary"
#include "CsPrivatePrototypes.h"

#ifdef Q_OS_WIN
    #define NOMINMAX	// to prevent compilation errors from QDateTime because of including windows.h
    #include <windows.h> // Used for the defines
#else
    #include <sys/mman.h>
    #ifndef UNREFERENCED_PARAMETER
        #define UNREFERENCED_PARAMETER(x) (void)(x)
    #endif
#endif

#define MIN_SEG_SIZE			32
#define CS_OPERATION_ABORTED	-555
#define CS_XFER_OFFSET			80
#define XFER_BUFFER_SIZE        1048576
#define MIN_XFER_SIZE           8192
#define SIG_FILE_HEADER_SIZE    512

#include "CsPrototypes.h"
#include "CsPrivatePrototypes.h"
#include "CsPrivateStruct.h"
#include "gage_drv.h"
#include <vector>
#include <map>
#include "CsTestErrMng.h"
#include <qelapsedtimer.h>
#include "CsExpert.h"
#include "qapplication.h"
#include "math.h"

#define MEM_PAGE_SIZE   4096

using namespace std;

typedef struct _System
{
	CSHANDLE	csHandle;
	QString		strBoardName;
    QString     strSerialNumber;
    int         i32BitCount;
    int         i32SampSize;
    int         i32ChanCount;
    int         i32BrdCount;
	int64		i64MemorySize;
	int			index;
	bool		bFree;
} System;

typedef struct _SampleRateTable
{
	int nItemCount;
	PCSSAMPLERATETABLE pSRTable;
} SampleRateTable, *PSampleRateTable;

// map for sample rates, the key is the acquisition mode
typedef map<uInt32, SampleRateTable> SRTableMap;

typedef struct _InputRangeTable
{
	int nItemCount;
	PCSRANGETABLE pRangeTable;
} InputRangeTable, *PInputRangeTable;

// map for input ranges, the key is impedance
typedef map<uInt32, InputRangeTable> IRTableMap;

typedef struct _ModeNames
{
	string strName;
	uInt32 u32Mode;
} ModeNames, *PModeNames;

typedef enum _ExpertOptions
{
	EXPERT_NONE,
	EXPERT_FIR,
	EXPERT_MINMAXDETECT,
	EXPERT_MULREC_AVG,
	EXPERT_FFT_512,
	EXPERT_FFT_1K,
	EXPERT_FFT_2K,
	EXPERT_FFT_4K,
	EXPERT_FFT_8K,
	EXPERT_MULREC_AVG_TD,
	EXPERT_COIN,
	EXPERT_GATE_ACQ,
	EXPERT_STREAM,
	EXPERT_HISTOGRAM,
	EXPERT_BIG_TRIGGER,
	EXPERT_UNKNOWN
} ExpertOptions, *PExpertOptions;

typedef int (*LPRMGETSYSTEMSTATEBYINDEX)(int, CSSYSTEMSTATE*);

class CGageSystem : public QObject
{
	Q_OBJECT

public:
	CGageSystem();
	~CGageSystem();

	// Temporary code as an example to use QWaitcondition
	QMutex              m_MutexBoardIdle;
	QWaitCondition		m_WaitBoardIdle;
	bool				WaitForBoardIdleState(int Timeout = (int)(-1));

	bool					m_bSsm;
	bool					m_bInterleavedData;

	/**************** Getters ***************************/
	CSHANDLE			getSystemHandle(); //WARNING: If you get a system handle, you must release it with ReleaseSystemHandle()
	CSSYSTEMINFO		getSystemInfo();
//RG    int32           allocateMemory(uInt32 u32Mode, int64 i64SegmentSize); // move to private ??
    int32               allocateMemory(uInt32 u32Mode); // move to private ??
	int32				GetSystem(int BrdIdx, bool bConfigBrd);
	int32				UpdateSystemInformation();
	int32				GatherSystemCaps();
	int32				WaitForTrigger();
	int32				DataCaptureComplete();
	int32				WaitForReady();
    int32               GetSysInfoString(int BrdIndex, QString *pQStrSystemInfo);
    int32               GetFwVersion(int BrdIndex, QString *pQStrVersionInfo);
	int32				TransferData(uInt32 u32Channel, uInt32 u32Segment, void* pBuffer, int *pXferOffset);
    int32               TransferDataBuffer(uInt32 u32Channel, uInt32 u32Segment, void* pBuffer, unsigned long long StartSample, unsigned long long SampleCount, int *pXferOffset, long long *pXferTimeNsec);
    int32				TransferDataInter(uInt32 u32Segment, void* pBuffer, int *pXferOffset);
    int32				TransferDataInterBuffer(uInt32 u32Segment, void* pBuffer, unsigned long long StartSample, unsigned long long SampleCount, int *pXferOffset,long long *pXferTimeNsec);
	int32				ResetSystem();
	int32				ResetSystemUpdateInfo();
	int32				ForceTrigger();
    int32				ForceCalib();
	int32				Abort();
	uInt32				getChannelCount() { return m_SystemInfo.u32ChannelCount; }
	uInt32				getTriggerCount() { return m_SystemInfo.u32TriggerMachineCount; }
	uInt32				getBoardCount() { return m_SystemInfo.u32BoardCount; }
	uInt32				getImpedanceCount() { return m_ImpedanceCount; }
    uInt32              getChannelCountPerBoard() { return (m_SystemInfo.u32ChannelCount / m_SystemInfo.u32BoardCount); }
//	CSSYSTEMINFOEX		getSystemInfoEx();
//	uInt32				getHardwareOptions();
	CSACQUISITIONCONFIG	getAcquisitionConfig();
	bool				hasExternalClock() { return m_bExternalClock; }
	int64				getMinExtRate() { return m_i64MinExtRate; }
	int64				getMaxExtRate() { return m_i64MaxExtRate; }
//	bool				hasReferenceClock() { return m_bReferenceClock; }
	bool				hasMultipleRecord() { return m_bMultipleRecord; }
	bool				hasDcOffset() { return m_bDcOffset; }
	bool				hasCouplingDC() { return m_bCouplingDC; }
	bool				hasCouplingAC() { return m_bCouplingAC; }
	bool				hasFilter() { return m_bFilter; }
	bool				hasImpedance1M() { return m_bImpedance1M; }
	bool				hasImpedance50() { return m_bImpedance50; }
	uInt32				getModesSupported() { return m_u32ModesSupported; }
	bool				hasExternalTrigger() { return m_bExternalTrigger; }

	bool				hasTriggerCouplingAC() { return m_bTriggerCouplingAC; }
	bool				hasTriggerCouplingDC() { return m_bTriggerCouplingDC; }
	bool				hasTriggerImpedance50() { return m_bTriggerImpedance50; }
	bool				hasTriggerImpedanceHZ() { return m_bTriggerImpedanceHZ; }
	bool				hasTriggerRange1V() { return m_bTriggerRange1V; }
	bool				hasTriggerRange5V() { return m_bTriggerRange5V; }
    bool				hasTriggerRange3V3() { return m_bTriggerRange3V3; }
    bool				hasTriggerRangePlus5V() { return m_bTriggerRangePlus5V; }
	bool				hasSingleChan2Mode() { return m_bSingleChan2; }

	int32				getLastStatusCode();
	void				updateErrMess(QString strErrMess);
	void				clearErrMess();
	QString				getLastError() { return m_strError; }
    void*               m_pMainSamplesBuffer;
    unsigned long long  m_i64MainSamplesBufferSize;
    unsigned long long  m_i64Chan_mem_size;
	void*				getBuffer(int i);// { return m_pBufferArray[i]; } //RG DO CHECK IF i is out of bounds
	int					getXferOffset(int i);// { return m_pOffsetsArray[i]; } //RG DO CHECK IF i is out of bounds
	int64				getPreTrigger() { return m_AcquisitionConfig.i64SegmentSize - m_AcquisitionConfig.i64Depth; }
	int64				getSegmentSize() { return m_AcquisitionConfig.i64SegmentSize; }
	uInt32				getSegmentCount() { return m_AcquisitionConfig.u32SegmentCount; }
	uInt32				getSampleSize() { return m_AcquisitionConfig.u32SampleSize; }
	uInt32				getAcquisitionMode() { return m_AcquisitionConfig.u32Mode; }
	int64				getSampleRate() { return m_AcquisitionConfig.i64SampleRate; }
	int64				getDepth() { return m_AcquisitionConfig.i64Depth; }
	int64				getTriggerHoldoff() { return m_AcquisitionConfig.i64TriggerHoldoff; }
    int64				getTriggerDelay() { return m_AcquisitionConfig.i64TriggerDelay; }
	int64				getTriggerTimeout() { return m_AcquisitionConfig.i64TriggerTimeout; }
	int32				getSampleRes() { return m_AcquisitionConfig.i32SampleRes; }
	int32				getSampleOffset() { return m_AcquisitionConfig.i32SampleOffset; }
	uInt32				getExtClockStatus() { return m_AcquisitionConfig.u32ExtClk; }

	void				setDepth(int64 i64Depth);
	void				setTriggerHoldoff(int64 i64Holdoff);
    void				setTriggerDelay(int64 i64Delay);
	void				setAcquisitionMode(uInt32 u32Mode) { m_AcquisitionConfig.u32Mode = u32Mode; }
	void				setTriggerTimeout(int64 i64Timeout) { m_AcquisitionConfig.i64TriggerTimeout = i64Timeout; }
	void				setSampleRate(int64 i64Rate) { m_AcquisitionConfig.i64SampleRate = i64Rate; }
	void				setSegmentCount(uInt32 u32RecordCount) { m_AcquisitionConfig.u32SegmentCount = u32RecordCount; }
	void				setExtClock(uInt32 u32ExtClock) { m_AcquisitionConfig.u32ExtClk = u32ExtClock; }
	void				setExtRef();

	ExpertOptions		GetExpertImg1() { return m_ExpertImg1; }
	QString				GetExpertImg1Name() { return m_strExpertImg1Name; }
	ExpertOptions		GetExpertImg2() { return m_ExpertImg2; }
	QString				GetExpertImg2Name() { return m_strExpertImg2Name; }

	int32				getMulRecAvgCount() { return m_MulRecAvgCount; }	

	/**************** Caps ***************************/

	int32				GetAcquisitionModes();
	int32				GetSampleRates();
	int32				GetInputRanges();
	int32				GetImpedances();
	int32				GetCouplings();
	int32				GetFilters();
	int32				GetTriggerSources();
	int32				GetTriggerCouplingCaps();
	int32				GetTriggerImpedanceCaps();
	int32				GetTriggerRangeCaps();
    int32				GetExtClock();
	void				GetMultipleRecord();
	void				GetDcOffsetCaps(); // change name
	int32				GetExpertCaps();

	//		TODO
	//		Expert, Ext Clock, reference Clock

	/**************** Setters ***************************/
	void				setSystemHandle(CSHANDLE hSystem);
	void				setBuffers(vector<void*>BufferArray);
	void				setXferOffsets(vector<int>OffsetsArray);
	void				setXferOffset(int idx, int Offset);
    void				SetAbort();
	void				ClearAbort();
	bool				IsValidTriggerSource(int TrigSource);
    bool				IsValidTriggerSource(int TrigSource, uInt32 u32Mode);
	
	/************* Mutex Management *********************/
	void				ReleaseSystemHandle(CSHANDLE& hSystem);

	virtual int32		InitStructures() = 0;
	virtual int32		Initialize() = 0;
	virtual int32		FreeSystem(CSHANDLE csHandle) = 0;
	virtual int32		FreeSystem() = 0;
    virtual int32       GetSerialNumber(CSHANDLE hSystem, char *pSerialNumber) = 0;
	virtual int32		DoAcquisition() = 0;
	virtual int32		CompuscopeGetStatus(CSHANDLE hSystem) = 0;
	virtual int32		CompuscopeTransferData(CSHANDLE hSystem, IN_PARAMS_TRANSFERDATA *inParams, OUT_PARAMS_TRANSFERDATA *outParams) = 0;
	virtual int32		CompuscopeTransferDataEx(CSHANDLE hSystem, IN_PARAMS_TRANSFERDATA_EX *inParams, OUT_PARAMS_TRANSFERDATA_EX *outParams) = 0;
	virtual int32		CompuscopeGetSystem(CSHANDLE *hSystem, int BrdIdx) = 0;
	virtual int32		CompuscopeGetSystemInfo(CSHANDLE hSystem, PCSSYSTEMINFO pSystemInfo) = 0;
	virtual int32		GatherSystemInformation() = 0;
	virtual int32		CompuscopeDo(CSHANDLE hSystem, int16 i16Operation) = 0;
	virtual int32		CompuscopeCommit(CSHANDLE hSystem, bool bCoerce) = 0;
	virtual int32		CompuscopeSet(CSHANDLE CsHandle, uInt32 ParamID, void *ParamBuffer) = 0;
	virtual int32		CompuscopeGet(CSHANDLE hSystem, int32 nIndex, int32 nConfig, void* pData) = 0;
	virtual int32		CompuscopeSetAcq(CSHANDLE CsHandle, CSACQUISITIONCONFIG *pAcquisitionParms) = 0;
	virtual int32		CompuscopeSetChan(CSHANDLE CsHandle, int Chan, CSSYSTEMCONFIG *pSystemParms) = 0;
	virtual int32		CompuscopeGetSystemCaps(CSHANDLE CsHandle, uInt32 CapsId, PCSSYSTEMCONFIG pSysCfg, void *pBuffer, uInt32 *BufferSize) = 0;
	virtual int32		CsGetInfo(CSHANDLE hSystem, int32 nIndex, int32 nConfig, void* pData) = 0;
	virtual int32		CsSetInfo(CSHANDLE hSystem, int32 nIndex, int32 nConfig, void* pData) = 0;
	virtual int32		SetMulRecAvgCount(uInt32 AvgCount) = 0;
	virtual int32		GetMulRecAvgCount(uInt32 *AvgCount) = 0;
//	virtual QString		GetBoardName() = 0;
//	virtual int64		GetMemorySize() = 0;
//	virtual uInt32		GetChannelsPerBoard() = 0;
	virtual int32		CommitAcq(int *i32CommitTimeMsec) = 0;
	virtual int32		PrepareForCapture(int *i32CommitTimeMsec) = 0;


	// Channel parameters get and set
	virtual uInt32		GetImpedance(uInt32 u32Channel) = 0;
	virtual void		SetImpedance(uInt32 u32Channel, uInt32 u32Impedance) = 0;

	virtual uInt32		GetCoupling(uInt32 u32Channel) = 0;
	virtual void		SetCoupling(uInt32 u32Channel, uInt32 u32Coupling) = 0;

	virtual uInt32		GetFilter(uInt32 u32Channel) = 0;
	virtual void		SetFilter(uInt32 u32Channel, uInt32 u32Filter) = 0;

	virtual int32		GetDcOffset(uInt32 u32Channel) = 0;
	virtual void		SetDcOffset(uInt32 u32Channel, int32 i32DcOffset) = 0;

	virtual uInt32		GetInputRange(uInt32 u32Channel) = 0;
	virtual void		SetInputRange(uInt32 u32Channel, uInt32 u32InputRange) = 0;

	// Trigger parameters get and set
	virtual int32		GetSource(uInt32 u32Trigger) = 0;
	virtual void		SetSource(uInt32 u32Trigger, int32 i32Source)  = 0;

	virtual int32		GetLevel(uInt32 u32Trigger)  = 0;
	virtual void		SetLevel(uInt32 u32Trigger, int32 i32Level)  = 0;

	virtual uInt32		GetCondition(uInt32 u32Trigger)  = 0;
	virtual void		SetCondition(uInt32 u32Trigger, uInt32 u32Condition)  = 0;

	virtual uInt32		GetExtCoupling(uInt32 u32Trigger)  = 0;
	virtual void		SetExtCoupling(uInt32 u32Trigger, uInt32 u32ExtCoupling)  = 0;

	virtual uInt32		GetExtImpedance(uInt32 u32Trigger)  = 0;
	virtual void		SetExtImpedance(uInt32 u32Trigger, uInt32 u32ExtImpedance)  = 0;

	virtual uInt32		GetExtTriggerRange(uInt32 u32Trigger)  = 0;
	virtual void		SetExtTriggerRange(uInt32 u32Trigger, uInt32 u32ExtTriggerRange)  = 0;

	QString				GetBoardName(){  return QString(m_SystemInfo.strBoardName); }
	int64				GetMemorySize() { return m_SystemInfo.i64MaxMemory; }
	uInt32				GetChannelsPerBoard() { return m_SystemInfo.u32ChannelCount / m_SystemInfo.u32BoardCount; }
	
	PSampleRateTable	GetSampleRateTable(uInt32 u32Mode) { return &m_mapSRTable[u32Mode]; }
	vector<ModeNames>&	GetAcquisitionModeNames() { return m_vecModeNames; }
	PInputRangeTable	GetInputRangeTable(uInt32 u32Impedance) { return &m_mapIRTable[u32Impedance]; }
	uInt32				GetChannelSkip() { return getChannelSkip(m_AcquisitionConfig.u32Mode, m_SystemInfo.u32ChannelCount / m_SystemInfo.u32BoardCount); }

	int32				GetStartChannel();
	int32				GetActiveChannelCount();
    int32				GetActiveChannelCount(uInt32 u32Mode);
	uInt32				getChannelSkip(uInt32 u32Mode, uInt32 u32ChannelsPerBoard);
	QString				GetErrorString(int32 i32Status);

	void				ClearAcqCount();
	void				IncAcqCount();
	uInt32				GetAcqCount();

signals:
	void				updateUi();


protected:
//	uInt32 getChannelSkip(uInt32 u32Mode, uInt32 u32ChannelsPerBoard);
	long count_bits(long n);

	CsTestErrMng*			m_ErrMng;
	SRTableMap				m_mapSRTable;
	IRTableMap				m_mapIRTable;
	PCSIMPEDANCETABLE		m_ImpedanceTable;
	uInt32					m_ImpedanceCount;
	PCSCOUPLINGTABLE		m_CouplingTable;
	PCSFILTERTABLE			m_FilterTable;
	int32					m_TriggerSource[50]; //RG TRY AND DO WITH A VECTOR
	bool					m_bExternalClock;
	int64					m_i64MinExtRate;
	int64					m_i64MaxExtRate;
	bool					m_bMultipleRecord;
	bool					m_bDcOffset;
	bool					m_bCouplingDC;
	bool					m_bCouplingAC;
	bool					m_bFilter;
	bool					m_bImpedance50;
	bool					m_bImpedance1M;
	bool					m_bExternalTrigger;

	bool					m_bTriggerCouplingDC;
	bool					m_bTriggerCouplingAC;

	bool					m_bTriggerImpedance50;
	bool					m_bTriggerImpedanceHZ;

	bool					m_bTriggerRange1V;
	bool					m_bTriggerRange5V;
    bool					m_bTriggerRange3V3;
    bool					m_bTriggerRangePlus5V;

	uInt32					m_u32ModesSupported;
	vector<ModeNames>		m_vecModeNames;

	bool					m_bSingleChan2;

	ExpertOptions			m_ExpertImg1;
	QString					m_strExpertImg1Name;
	ExpertOptions			m_ExpertImg2;
	QString					m_strExpertImg2Name;

	int32					m_MulRecAvgCount;

	uInt32					m_u32AcqCount;
	uInt32					m_u32ChannelSkip;
	BOOL					m_bCoerceFlag;
	int32					m_i32LastStatusCode;
	QString					m_strError;

	void					AcqSleep(int msDelay);
	QElapsedTimer			m_SleepTimer;

	int64					m_i64LastValidHoldoff;
	int64					m_i64LastValidDepth;

	/********* Default structures ***********************/
//	static const CSSYSTEMINFO			EmptySystemInfo;
//	static const CSACQUISITIONCONFIG	EmptyAcquisitionConfig;

	vector<void*>			m_pBufferArray;
	vector<int>				m_pXferOffsetsArray;

protected:

	/************* Data about the board *****************/
	CSHANDLE				m_hSystem;
	CSSYSTEMINFO			m_SystemInfo;
	CSACQUISITIONCONFIG		m_AcquisitionConfig;
	CSSYSTEMINFOEX			m_SystemInfoEx;
	
	/********* Mutex for multi threading ****************/
	QMutex					m_MutexSystemHandle;
	QMutex					m_MutexSystemInfo;
	QMutex					m_MutexAcquisitionConfig;
	QMutex					m_MutexLastStatusCode;

	void					ExtractExpertOptions(int64 i64ExtendedOptions);

	bool					m_bAbort;

	QElapsedTimer*			m_EventsTimer;
    QLibrary                m_libCsRmDLL;
    LPRMGETSYSTEMSTATEBYINDEX   m_pCsRmGetSystemStateByIndex;
    QElapsedTimer           m_XferTimer;
    #ifdef Q_OS_WIN
        HANDLE                  m_hAcqEndEvent;
        HANDLE                  m_hTrigEvent;
    #endif
};

#endif // GAGESYSTEM_H

