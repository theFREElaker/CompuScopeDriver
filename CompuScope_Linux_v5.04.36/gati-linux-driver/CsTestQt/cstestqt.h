#ifndef CSTESTQT_H
#define CSTESTQT_H

#include <QtGlobal>
#include "cstestqttypes.h"

#ifdef Q_OS_WIN
    #include <windows.h>
#endif

//#include <QtWidgets/QMainWindow>
#include <QWidget>
#include <QMessageBox>
#include <QTimer>
#include <QTime>
#include <QThread>
#include <QWaitCondition>

#include "CsTestErrMng.h"
#include "qsystemselectiondialog.h"
#include "qperformancetestdialog.h"
#include "qconfigdialog.h"
#include "qaboutdialog.h"
#include "qCursorInfodialog.h"
#include "qReadWriteRegsdialog.h"
#include "qNiosSpiCmddialog.h"
#include "qMulrecdialog.h"
#include "qReadDataFiledialog.h"
#include "qReadSigFiledialog.h"

#include "ui_cstestqt.h"
#include "ui_qcontrolsdialog.h"

#include "acquisitionworker.h"
#include "AsyncCsOps.h"
#include "qconfigdialog.h"
#include "qProgressBar.h"
#include "qGoToSample.h"

#define TIMEOUT_CLOSETHREAD 1000

class CsTestQt : public QMainWindow
{
	Q_OBJECT

public:
    CsTestQt(bool bSsm = true, bool bDebug = false, bool bAdvance = false, QWidget *parent = 0);
	~CsTestQt();

	CGageSystem*			GetGageSystem() { return m_pGageSystem; }
	QScope*					GetScope() { return ui.mainScope; }
	void					keyPressEvent(QKeyEvent* event);
	void					mousePressEvent(QMouseEvent *eventPress);
    void					moveEvent(QMoveEvent *eventMove);
    void					resizeEvent(QResizeEvent *eventResize);
    void					closeEvent(QCloseEvent *eventClose);
    CursorInfodialog*       m_pCursorInfo;
//	DebugDialog*			m_pdebug;
	ReadWriteRegsDialog*	m_pReadWriteRegs;
	NiosSpiCmdDialog*		m_pNiosSpiCmd;
    MulrecDialog*           m_pMulrec;
	ReadDataFileDialog*		m_pReadDataFile;
    ReadSigFileDialog*		m_pReadSigFile;
	void					ErrorClear();
	void					ErrorAppend(QString ErrString);
	QString					ErrorGet();
	int						SelectSystem();
	CsTestErrMng*			m_ErrMng;
	QMessageBox*			m_MsgBox;
    QMutex                  m_MutexSharedVars;

	bool			m_bAcqWait;
	bool			m_bAcqRunning;			// UI processing of the request od StartAcq is in progress: Enter of AcquisitionWorker::s_DoAcquisitionFromBoard()
    bool			m_bAbortAcq;
	bool			m_bInterleavedData;
    bool            m_bScopeInitDone;
    bool            m_bScopeUpdateDone;
    bool            m_bUpdateScopeParmsDone;
    bool            m_bStatusBarUpdateDone;
    bool            m_bUpdatingUi;
    bool            m_bDisplayOneSeg;
    bool			m_bParmsChanged;
    int64           m_i64CurrSeg;
    QString             m_strCurrSerNum;

	QEventLoop*		m_AcqWaitLoop;

	/********* Mutex for multi threading ****************/
	//QMutex			m_MutexAbort;
	//QMutex			m_MutexContinuous;
	//QMutex			m_MutexClrGraph;
    QMutex              m_MutexSamples;
    QMutex              m_MutexAcqRunWait;

    STATUS_BAR_INFO     m_StatusBarInfo;

public slots:
	void s_Close();
	void s_SaveChannels();
    void s_LoadChannels();
	void s_LoadData();
	void s_SaveFile();
	void s_SelectSystem();
	void s_SetInterleavedData(bool bState);
	void s_OpenAboutDialog();
	void s_OpenControlsDialog();
	void s_DoPerformanceTest();
	void s_ConfigureSystem();
	void s_UpdateParms();
	void s_SetBusMasterSynchronous();
	void s_SetBusMasterASynchronous();
	void s_SetSlaveSynchronous();
	void s_SaveTimeStamp();
	void s_DoAcquisition();
	void s_SetContinuousMode(bool bMode);
	void s_DoAbort();
	void s_DoForceTrigger();
    void s_DoForceCalibration();
	void s_XferMem();
	void s_DoAddOnReset();
	void s_DoSystemReset();
    void s_GoToSample();
	void s_ConnectPoints(bool bMode);
	void s_Overdraw(bool bMode);
	void s_ClearGraph();
    void s_ShowCursor(bool);
	void s_EnableAcquisition(bool bCanAcquire = true);
	void s_UpdateStatus(int);
	void s_UpdateSampRateStatus();
    void s_UpdateTxTime(QString strXferTime);
	void s_UpdateCommitTime(int);
	void s_UpdatePositionStatus(QString);
	void s_UpdateHoriz(int);
	void s_DisplayErrPopup();
    void s_DisplayMessage(QString Msg);
	void s_ActionAfterError(int Result);
	void s_UpdateUi();
	void s_ReadWriteRegs();
	void s_NiosSpiCmd();
	void s_Mulrec();
    void s_ChanProtFault();

signals:
    void adjustHScrollBar();
	void acquisitionThreadMustStart();
    void acquisitionThreadMustStop();
	void acquisitionThreadMustAbort();
	void acquisitionThreadMustSetContinuous(bool);
	void xferMem();
	void clearGraph();
	void clearGraphAcq(bool);
	void debugShow();
	void updateStatus(int);
	void updateSampRateStatus();
    void updateCursorStatus(QString);
	void ShowError();
    void ShowMessage(QString Msg);
	void Error();
	void loadParms(CGageSystem*);
	void UpdateCommitDone(void);
	void StopAcq();
//	void StartAcq();
//	void AbortAcq();
	void ResetSystem();
	void AbortCsOp();
	void ForceTriggerCsOp();
	void ResetCsOp();
    void StatusUpdated();
    void SaveChannel();
    void LoadData();
    void SetOverdraw(bool);
    void SetProgBarMax(int);
    void SetProgBarCurr(int);

private:
    void    WriteWinPosSettings();
    void    ReadWinPosSettings();
	void    EnableMenuControls(bool bEnable);
	
	Ui::CsTestQtClass	ui;
	QLabel*				m_PositionLabel;
	QLabel*				m_AcqCountLabel;
    QLabel*				m_CurrSegLabel;
	QLabel*				m_SrLabel;
	QLabel*				m_XferTimeLabel;
	QLabel*				m_ZoomLabel;
	QLabel*				m_BrdStatusLabel;
	QMutex				m_MutexCanAcquire;
	AcquisitionWorker*	m_AcqWorker;
	AsyncCsOps*			m_AsyncCsOps;
	QConfigDialog*		m_config;
	QThread*			m_GraphThread;
	QThread*			m_WorkerThread;
	QThread*			m_AsyncCsOpsThread;
	QThread*			m_ConfigThread;
	CGageSystem*		m_pGageSystem;
	bool				m_bCanAcquire;
	int					m_i32ErrCode;
	QString				m_strErrMsg;
	bool				m_bClosing;
    bool				m_bProcessingEvents;
	QTimer*				m_UiTimer;
    unsigned long long	m_u64XferSize;
	int					m_i32BrdCount;
	int					m_i32BrdIdx;
    bool                m_bReadingWindowSettings;
    bool                m_bSettingCursor;
    progressBar*        m_ProgBar;
    gotoSample*         m_GoToSample;

};

#endif // CSTESTQT_H
