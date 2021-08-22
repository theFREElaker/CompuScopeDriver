#ifndef QCONFIGDIALOG_H
#define QCONFIGDIALOG_H

#include <QDialog>
#include "ui_qconfigdialog.h"
#include <QVector> 
#include "acquisitionworker.h"
#include <qeventloop.h>
#include "QMessageBox"

typedef struct ACQUISITIONPARAMETERS
{
	uInt32	u32Mode;
	bool	bExternalClock;
	bool	bReferenceClock;
	bool	bExpert1;
	bool	bExpert2;
	int64	i64SampleRate;
	uInt32	u32RecordCount;
	int64	i64Pretrigger;
    int64	i64TriggerDelay;
	int64	i64PostTrigger;
	int64	i64TriggerTimeout;
} AcquisitionParameters;

typedef struct CHANNELPARAMETERS
{
	uInt32	u32Coupling;
	uInt32	u32Impedance;
	bool	bFilter;
	uInt32	u32InputRange;
	int32	i32DcOffset;
} ChannelParameters;

typedef struct CHANNEL_DESC
{
	char				szChannelLabel[32];
	CSCHANNELCONFIG		CsChannelCfg;
}CHANNEL_DESC;


typedef struct SIGX_HEADER
{
	// header of New SIGX file format 
	uInt32					u32Version;				//version of the header
	uInt32					u32Size;				//Size of the header / Offset of the data from the beginning of the file
	char					szDescription[256];		//User comments
	CSBOARDINFO				BoardInfo;				//boardinfo structure
	CSSYSTEMINFO			CsSystemInfo;			//System Info structure
	uInt32					CardIndex;				//Card index in a M/S systeme 1,2,3..
	uInt32					NbOfChannels;			//Number of channel structures
	uInt32					NbOfTriggers;			//Number of trigger structures
	uInt32					TailSize;				//Inter Segment/Tail size in bytes 
    unsigned long long		u64StartSegment;		//The first segment in the file
    unsigned long long		u64EndSegment;			//The last segment in the file
	int64					i64StartSample;			//The first sample of the first segment
	int64					i64EndSample;			//The last sample of the last segment
	uInt32					Continuous;				//Data is continuous from one segment to anther
	CSACQUISITIONCONFIG		CsAcqCfg;				//The Acquisition Config structure.

	// variables
	//CHANNEL_DESC			CsChannelDesc;
	//CSTRIGGERCONFIG		CsTriggerCfg;



} SIGX_HEADER;

typedef struct TRIGGERPARAMETERS
{
	int32	i32Source;
	uInt32	u32Condition;
	int32	i32Level;
	uInt32	u32ExtCoupling;
	uInt32	u32ExtImpedance;
	uInt32	u32Range;
} TriggerParameters;

class QConfigDialog : public QDialog
{
	Q_OBJECT

public:
    QConfigDialog(CGageSystem* pGageSystem, CsTestErrMng* ErrMng, QWidget *parent, bool*  bUpdateScopeParmsDone);
	~QConfigDialog();

    bool*           m_bUpdateScopeParmsDone;
    bool*			m_bParmsChanged;
    bool*           m_bDisplayOneSeg;
    int64*          m_i64CurrSeg;
    QMutex*         m_MutexSharedVars;

	QEventLoop*		m_UpdateCommitLoop;

	void paintEvent(QPaintEvent*);

	QPoint m_LastPos;

    void LoadUi();
	void SetAcquisitionTab();
	void SetChannelTabUI();
	void SetTriggerTabUI();
	
    void WaitUpdateScopeParmsDone();
    void SetAddrMutexAcqRunWait(QMutex* pMutexAcqRunWait);
    void SetAddrDisplayOneSeg(bool* bDisplayOneSeg);
    void SetAddrCurrSeg(int64* i64CurrSeg);
    void SetAddrParmsChanged(bool* bParmsChanged);
    void SetAddrMutexSharedVars(QMutex* MutexSharedVars);

signals:
	void updateChannels(int);			// Not used  ????
	void updateCommitTime(int);
	void ConfigError(void);
	void LoadParms(void);
	void StopAcq(void);
	void StartAcq(void);
    void disableData();

public slots:
	void s_ShowConfigWindow(CGageSystem* pGageSystem);
    void s_Cancel();
    void s_Ok();
	void s_loadParms(CGageSystem*);
	void s_UpdateTab(int i32Index);

	//Acquisition tab
	void s_UpdateExtClock();
	void s_ChangeExpert1(int i32State);
	void s_ChangeExpert2(int i32State);
	void s_ChangeAvgCount();
    void s_EnableNbrRecords(bool bState);
	void s_EnableTriggerTimeOut(int i32State);
	void s_ChangeTriggerTimeOut();
	void s_EnableExternalClock(int i32State);
	void s_EnableReferenceClock(int i32State);
	void s_ChangeAcquisitionMode(int i32State);
    void s_DisplaySingleSeg(int i32State);
    void s_DisplaySingleSeg(bool b32State);
    void s_ChangeCurrSeg();
    void s_ChangeCurrSeg(double);

	//Channel tab
	void s_EnableAllChannels(int i32State);
	void s_ChangeChannelIndex(int i32ChannelIndex);
    void s_EnableFilter(bool bState);
	void s_ChangeInputRange(int i32RangeIndex);
	void s_ChangeDcOffset();
	void s_EnableAC(bool bState);
	void s_EnableDC(bool bState);
	void s_Enable50Ohm(bool bState);
	void s_Enable1MOhm(bool bState);

	//Trigger tab
	void s_EnableAdvanceTriggering(int i32State);
	void s_ChangeTriggerSource(int i32Index);
	void s_ChangeTriggerCondition(int i32Index);
	void s_ChangeTriggerLevel(int);
	void s_AdvancedTriggerItemClicked(QTableWidgetItem* widget);
	void s_EnableTriggerAC(bool bState);
	void s_EnableTriggerDC(bool bState);
	void s_EnableTriggerHZ(bool bState);
	void s_EnableTrigger50Ohm(bool bState);
	void s_EnableTrigger5V(bool bState);
	void s_EnableTrigger1V(bool bState);
    void s_EnableTrigger3V3(bool bState);
    void s_EnableTriggerPlus5V(bool bState);

private:
    void ChangeTriggerSource(uInt32 u32Engine  = 1);

	Ui::QConfigDialog ui;

	void CheckAvailabiity();
	void QueryCaptureCaps();
	void QueryChannelCaps();
	void QueryTriggerCaps();
    void EnableExternalTriggerParms(bool bEnable);

	void GetAcquisitionParameters();
	void UpdateUiAcquisitionParameters();

	void GetChannelParameters();
	void SetChannelParameters();
	
	void GetTriggerParameters();
    void SetTriggerParameters(uInt32 u32Mode);
    void SetTriggerControls(uInt32 idxTrig);

	void SetExpertFunction(int idxImage, ExpertOptions Option);
	void UnsetExpertFunctions();

    void PopulateRangeComboBox();
	void SaveLastGoodConfigurations();
	void RestoreLastGoodConfigurations();


	AcquisitionParameters m_AcqParams;
	QVector<ChannelParameters> m_vecChanParams;
	QVector<TriggerParameters> m_vecTrigParams;
	QVector<ChannelParameters> m_vecSuccessChanParams;
	QVector<TriggerParameters> m_vecSuccessTrigParams;
	CGageSystem*		m_pGageSystem;
	AcquisitionWorker*	m_AcqWorker;
	QWidget *			m_Parent;
	CsTestErrMng*		m_pErrMng;
	int32				m_i32AvgCount;
    bool m_bParmsLoaded;
    int64               m_PrevIntClockRate;
    uInt32              m_u32AcqMode;
    uInt32              m_u32ChanSkip;
    bool                m_bAdvancedTrigState;
};

#endif // QCONFIGDIALOG_H
