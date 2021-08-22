#include "cstestqt.h"
#include "GageDispSystem.h"
#include "GageSsmSystem.h"
#include "qdesktopwidget.h"
#include "qmath.h"
#include "qfile.h"
#include "qtextstream.h"
#include "qfiledialog.h"
#include "GageSystem.h"
#include "csSaveFile.h"
#include "DiskHead.h"
#include "CsTestFunc.h"
#include <QDebug>
#include "QSettings"
#include "QLibrary"

////qDebug() << "sample comment";

using namespace CsTestFunc;


CsTestQt::CsTestQt(bool bSsm, bool bDebug, bool bAdvance, QWidget *parent)
	: QMainWindow(parent), m_bCanAcquire(true)
{
    qDebug() << "cstestqt constructor start";
	m_bClosing = false;
	m_bAcqWait = false;
	m_bAcqRunning = false;
    m_bInterleavedData = false;
    m_bScopeInitDone = false;
    m_bScopeUpdateDone = false;
    m_bStatusBarUpdateDone = false;
    m_bUpdateScopeParmsDone = false;
    m_bUpdatingUi = false;
    m_bProcessingEvents = false;
    m_bParmsChanged = false;
    //bDebug = false;
    m_bReadingWindowSettings = false;
    m_bSettingCursor = false;

	//Initialize pointers.
    m_pCursorInfo = NULL;
	m_pReadWriteRegs = NULL;
	m_pNiosSpiCmd = NULL;
	m_pMulrec = NULL;
	m_pReadDataFile = NULL;
    m_pReadSigFile = NULL;
	m_ErrMng = NULL;
	m_MsgBox = NULL;
	m_AcqWaitLoop = NULL;
	m_PositionLabel = NULL;
	m_AcqCountLabel = NULL;
    m_CurrSegLabel = NULL;
	m_SrLabel = NULL;
	m_XferTimeLabel = NULL;
	m_ZoomLabel = NULL;
	m_BrdStatusLabel = NULL;
	m_AcqWorker = NULL;
	m_AsyncCsOps = NULL;
	m_config = NULL;
	m_WorkerThread = NULL;
	m_AsyncCsOpsThread = NULL;
	m_ConfigThread = NULL;
	m_pGageSystem = NULL;
    m_UiTimer = NULL;
    m_ProgBar = NULL;
    m_GoToSample = NULL;

    m_strCurrSerNum.clear();

    m_StatusBarInfo.m_BoardStatus = BoardReady;
    m_StatusBarInfo.m_dblStatBarCommitTimeMsec = 0;
    m_StatusBarInfo.m_dblStatBarXferTimeMsec = 0;
    m_StatusBarInfo.m_i64StatBarStartPos = 0;
    m_StatusBarInfo.m_u32StatBarHorizZoom = 128;
    m_StatusBarInfo.m_u64StatBarAcqCount = 0;
    m_StatusBarInfo.m_u64StatBarSampRate = 0;
    m_StatusBarInfo.m_u64StatBarSegCount = 0;
    m_StatusBarInfo.m_u64StatBarCurrSeg = 0;

    m_bDisplayOneSeg = false;
    m_i64CurrSeg = 1;

    //qDebug() << "setup ui";
	ui.setupUi(this);

    ui.menuBar->setNativeMenuBar(false);

    QDesktopWidget CurrWidget;
    int ScreenWidth = CurrWidget.screen()->width();
    int ScreenHeight = CurrWidget.screen()->height();
    int WidgetWidth = width();
    int WidgetHeight = height();
    setGeometry((ScreenWidth/2)-(WidgetWidth/2),(ScreenHeight/2)-(WidgetHeight/2),WidgetWidth,WidgetHeight);
    ReadWinPosSettings();

    if(!bDebug)
    {
        QAction* menuDebugAction = ui.menuDebug->menuAction();
        ui.menuBar->removeAction(menuDebugAction);
		ui.actionLoad_Data->setVisible(false);
    }

    if(!bAdvance)
    {
        ui.actionInterleavedData->setVisible(false);
        ui.actionTBForce_Calibration->setVisible(false);
        ui.actionForce_Calibration->setVisible(false);
    }

 	ui.actionLoad_Channel->setVisible(false);
   //ui.actionSave_File->setDisabled(true);
	ui.actionPerformance_Test->setDisabled(true);
	ui.actionAddon_Reset->setDisabled(true);
	ui.actionBusmaster_Asynchronous->setDisabled(true);
	ui.actionBusmaster_Synchronous->setDisabled(true);
	ui.actionSlave_Synchronous->setDisabled(true);
	ui.actionInterleavedData->blockSignals(false);
    ui.actionTBConnectPoints->setChecked(true);

	m_AcqWaitLoop = new QEventLoop(this);
	m_AcqWaitLoop->processEvents(QEventLoop::AllEvents);
	m_AcqWaitLoop->blockSignals(false);

    m_UiTimer = new QTimer(this);

    ui.mainScope->SetAddrDisplayOneSeg(&m_bDisplayOneSeg);
    ui.mainScope->SetAddrCurrSeg(&m_i64CurrSeg);

    ui.mainScope->GetGraph()->setAcquiring(false);
    ui.mainScope->GetGraph()->SetAddrScopeInitDone(&m_bScopeInitDone);
    ui.mainScope->GetGraph()->SetAddrScopeUpdateDone(&m_bScopeUpdateDone);
    ui.mainScope->GetGraph()->SetAddrAcqRunning(&m_bAcqRunning);
    ui.mainScope->GetGraph()->SetAddrMutexSharedVars(&m_MutexSharedVars);
    ui.mainScope->setBrdIdx(m_i32BrdIdx);

	m_ErrMng = new CsTestErrMng();
	m_ErrMng->ClrErr();

	m_MsgBox = new QMessageBox;

    #ifdef Q_OS_WIN
        if(!QLibrary::isLibrary("CsRmDll.dll"))
        {
            m_MsgBox->setText("Cannot find CsRmDll.dll");
            m_MsgBox->exec();
            throw(-123456789);
        }
    #else
        if(!QLibrary::isLibrary("libCsRmDll.so"))
        {
            m_MsgBox->setText("Cannot find CsRmDll.dll");
            m_MsgBox->exec();
            throw(-123456789);
        }
    #endif

    //qDebug() << "set status bar";
	statusBar()->setSizeGripEnabled(true);

	m_PositionLabel = new QLabel();
    QFont FontStatBar = m_PositionLabel->font();
    FontStatBar.setPointSize(8);
    m_PositionLabel->setFont(FontStatBar);
	m_PositionLabel->setMaximumWidth(140);
	statusBar()->addPermanentWidget(m_PositionLabel, 1);
	m_PositionLabel->setAlignment(Qt::AlignCenter);
	m_PositionLabel->setText("Start position: 0");

	m_AcqCountLabel = new QLabel();
    m_AcqCountLabel->setFont(FontStatBar);
	m_AcqCountLabel->setMaximumWidth(120);
	statusBar()->addPermanentWidget(m_AcqCountLabel, 1);
	m_AcqCountLabel->setAlignment(Qt::AlignCenter);
	m_AcqCountLabel->setText("Acq: 0");

    m_CurrSegLabel = new QLabel();
    m_CurrSegLabel->setFont(FontStatBar);
    m_CurrSegLabel->setMaximumWidth(100);
    statusBar()->addPermanentWidget(m_CurrSegLabel, 1);
    m_CurrSegLabel->setAlignment(Qt::AlignCenter);
    m_CurrSegLabel->setText("0 segment");

	m_SrLabel = new QLabel();
    m_SrLabel->setFont(FontStatBar);
	m_SrLabel->setMaximumWidth(80);
	statusBar()->addPermanentWidget(m_SrLabel, 1);
	m_SrLabel->setAlignment(Qt::AlignCenter);
	m_SrLabel->setText("100 MSPS");

	m_XferTimeLabel = new QLabel();
    m_XferTimeLabel->setFont(FontStatBar);
	m_XferTimeLabel->setMaximumWidth(200);
	statusBar()->addPermanentWidget(m_XferTimeLabel, 1);
	m_XferTimeLabel->setAlignment(Qt::AlignCenter);
	m_XferTimeLabel->setText("");

	m_ZoomLabel = new QLabel();
    m_ZoomLabel->setFont(FontStatBar);
	m_ZoomLabel->setMaximumWidth(175);
	statusBar()->addPermanentWidget(m_ZoomLabel, 1);
	m_ZoomLabel->setAlignment(Qt::AlignCenter);
	m_ZoomLabel->setText("Horizontal: 128 Samples/Div");

	m_BrdStatusLabel = new QLabel();
    m_BrdStatusLabel->setFont(FontStatBar);
	m_BrdStatusLabel->setMaximumWidth(80);
	statusBar()->addPermanentWidget(m_BrdStatusLabel, 1);
	m_BrdStatusLabel->setAlignment(Qt::AlignCenter);
	m_BrdStatusLabel->setText("Ready");

	m_i32BrdCount = -1;
	m_i32BrdIdx = -1;

    //qDebug() << "set Gage system";
    //bSsm = false;
	if (bSsm)
        m_pGageSystem = new CGageSsmSystem(m_ErrMng);
	else
        m_pGageSystem = new CGageDispSystem(m_ErrMng);


	//Try to get a system
	//qDebug() << "Select system";
	int32 i32Status = SelectSystem();
	if (i32Status<0)
	{
		//m_MsgBox->setText(m_ErrMng->GetErrMsg());
		//m_MsgBox->exec();
		throw(-123456789);
	}

	m_pGageSystem->m_bSsm = bSsm;
	m_pGageSystem->clearErrMess();
	m_pGageSystem->ClearAcqCount();

    m_pCursorInfo = new CursorInfodialog(this);
    m_ProgBar = new progressBar(this);
    m_GoToSample = new gotoSample(this);

	//Setup the MultiThreading
	//Manage the multithreading
    //qDebug() << "set threadsA";
    m_WorkerThread = new QThread(this);
	m_AcqWorker = new AcquisitionWorker(m_pGageSystem, m_ErrMng, &m_StatusBarInfo, &m_bUpdateScopeParmsDone, &m_bAcqRunning, &m_bAbortAcq);
	m_AcqWorker->SetAddrScopeInitDone(&m_bScopeInitDone);
    m_AcqWorker->SetAddrScopeUpdateDone(&m_bScopeUpdateDone);
    m_AcqWorker->SetAddrDisplayOneSeg(&m_bDisplayOneSeg);
    m_AcqWorker->SetAddrCurrSeg(&m_i64CurrSeg);
    m_AcqWorker->SetAddrParmsChanged(&m_bParmsChanged);
    m_AcqWorker->SetAddrMutexSharedVars(&m_MutexSharedVars);
    m_AcqWorker->moveToThread(m_WorkerThread);

	m_AsyncCsOpsThread = new QThread(this);
    AsyncCsOps* m_AsyncCsOps = new AsyncCsOps(m_pGageSystem, &m_bAcqRunning, m_ErrMng);
	m_AsyncCsOps->moveToThread(m_AsyncCsOpsThread);

	//Set interleaved data Tx mode.
	m_bInterleavedData = false;
	ui.mainScope->GetGraph()->m_bInterleavedData = m_bInterleavedData;
	m_AcqWorker->m_bInterleavedData = m_bInterleavedData;
	m_pGageSystem->m_bInterleavedData = m_bInterleavedData;

    qRegisterMetaType<GRAPH_UPDATE>("GRAPH_UPDATE");

    qDebug() << "set connectionsA";
    connect(this, SIGNAL(acquisitionThreadMustStart()), m_AcqWorker, SLOT(s_DoAcquisitionFromBoard()), Qt::QueuedConnection);
    connect(this, SIGNAL(acquisitionThreadMustStop()), m_AcqWorker, SLOT(s_DoStop()), Qt::DirectConnection);
    connect(this, SIGNAL(acquisitionThreadMustAbort()), m_AcqWorker, SLOT(s_Abort()), Qt::DirectConnection);
    connect(ui.mainScope->GetGraph(), SIGNAL(channelProtFault()), m_AcqWorker, SLOT(s_Abort()), Qt::DirectConnection);
    connect(ui.mainScope->GetGraph(), SIGNAL(channelProtFault()), this, SLOT(s_ChanProtFault()), Qt::DirectConnection);
    //connect(this, SIGNAL(acquisitionParmsChanged()), m_AcqWorker, SLOT(s_setParmsChanged()), Qt::QueuedConnection);
    //connect(ui.mainScope->GetGraph(), SIGNAL(AbortAcq()), m_AcqWorker, SLOT(s_DoAbort()), Qt::QueuedConnection);
    connect(this, SIGNAL(AbortCsOp()), m_AsyncCsOps, SLOT(s_AbortCsOp()), Qt::QueuedConnection);
    connect(this, SIGNAL(ForceTriggerCsOp()), m_AsyncCsOps, SLOT(s_ForceTriggerCsOp()), Qt::QueuedConnection);
    connect(this, SIGNAL(ResetCsOp()), m_AsyncCsOps, SLOT(s_ResetCsOp()), Qt::QueuedConnection);
    connect(this, SIGNAL(acquisitionThreadMustSetContinuous(bool)), m_AcqWorker, SLOT(s_SetContinuous(bool)), Qt::QueuedConnection);
    connect(this, SIGNAL(SetOverdraw(bool)), m_AcqWorker, SLOT(s_SetOverdraw(bool)), Qt::QueuedConnection);
    connect(m_AcqWorker, SIGNAL(finished()), this, SLOT(s_EnableAcquisition()), Qt::QueuedConnection);
	connect(m_AcqWorker, SIGNAL(Error()), this, SLOT(s_DisplayErrPopup()), Qt::QueuedConnection);
    connect(m_AcqWorker, SIGNAL(clearGraph()), ui.mainScope->GetGraph(), SLOT(s_ClearGraph()), Qt::QueuedConnection);
    connect(this, SIGNAL(clearGraphAcq(bool)), m_AcqWorker, SLOT(s_SetClearGraph(bool)), Qt::QueuedConnection);
	connect(this, SIGNAL(Error()), this, SLOT(s_DisplayErrPopup()), Qt::QueuedConnection);
    connect(this, SIGNAL(ShowError()), m_MsgBox, SLOT(exec()), Qt::QueuedConnection);

    connect(m_AcqWorker, SIGNAL(dataReady()), ui.mainScope, SLOT(s_UpdateRawData()), Qt::QueuedConnection);
    connect(m_AcqWorker, SIGNAL(AbortScopeUpdate()), ui.mainScope->GetGraph(), SLOT(s_AbortUpdate()), Qt::QueuedConnection);
    connect(ui.mainScope->GetGraph(), SIGNAL(updateTxTime(QString)), this, SLOT(s_UpdateTxTime(QString)), Qt::QueuedConnection);
	connect(this, SIGNAL(updateStatus(int)), this, SLOT(s_UpdateStatus(int)), Qt::QueuedConnection);
	connect(this, SIGNAL(updateSampRateStatus()), this, SLOT(s_UpdateSampRateStatus()), Qt::QueuedConnection);
	
    connect(ui.mainScope->GetGraph(), SIGNAL(updateCursorStatus(QString)), m_pCursorInfo, SLOT(s_Update(QString)), Qt::QueuedConnection);
	connect(ui.mainScope->GetGraph(), SIGNAL(updatePositionStatus(QString)), this, SLOT(s_UpdatePositionStatus(QString)), Qt::QueuedConnection);

	connect(ui.mainScope->GetGraph(), SIGNAL(horizChanged(int)), this, SLOT(s_UpdateHoriz(int)), Qt::QueuedConnection);
	connect(ui.mainScope->GetGraph(), SIGNAL(zoomXChanged()), ui.mainScope->GetGraph(), SLOT(s_updateCursorVals()), Qt::QueuedConnection);
	connect(this, SIGNAL(xferMem()), m_AcqWorker, SLOT(s_XferMem()), Qt::QueuedConnection);
	connect(this, SIGNAL(clearGraph()), ui.mainScope->GetGraph(), SLOT(s_ClearGraph()), Qt::QueuedConnection);

    connect(ui.mainScope, SIGNAL(updateGraph(GRAPH_UPDATE)), ui.mainScope->GetGraph(), SLOT(s_UpdateGraph(GRAPH_UPDATE)), Qt::QueuedConnection);
    connect(ui.mainScope->GetGraph(), SIGNAL(updateGraph(GRAPH_UPDATE)), ui.mainScope->GetGraph(), SLOT(s_UpdateGraph(GRAPH_UPDATE)), Qt::QueuedConnection);

    connect(this, SIGNAL(adjustHScrollBar()), ui.mainScope, SLOT(s_adjustHScrollBar()), Qt::QueuedConnection);
    connect(this, SIGNAL(ShowMessage(QString)), this, SLOT(s_DisplayMessage(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(SaveChannel()), this, SLOT(s_SaveChannels()), Qt::QueuedConnection);
    connect(this, SIGNAL(LoadData()), this, SLOT(s_LoadData()), Qt::QueuedConnection);

    //qDebug() << "start threads";
    //qDebug() << "start worker thread";
	m_WorkerThread->start();
    //qDebug() << "start AsynchCsOps thread";
	m_AsyncCsOpsThread->start();

    //qDebug() << "set dialogs";
	m_pReadWriteRegs = new ReadWriteRegsDialog(this, m_pGageSystem);
	m_pNiosSpiCmd = new NiosSpiCmdDialog(this, m_pGageSystem);
	m_pMulrec = new MulrecDialog(this, m_pGageSystem, m_AcqWorker);
    m_pReadDataFile = new ReadDataFileDialog(this);
    m_pReadSigFile = new ReadSigFileDialog(this);

    //m_ConfigThread = new QThread(this);
    m_config = new QConfigDialog(m_pGageSystem, m_ErrMng, this, &m_bUpdateScopeParmsDone);
    m_config->SetAddrDisplayOneSeg(&m_bDisplayOneSeg);
    m_config->SetAddrCurrSeg(&m_i64CurrSeg);
    m_config->SetAddrParmsChanged(&m_bParmsChanged);
    m_config->SetAddrMutexSharedVars(&m_MutexSharedVars);

    //qDebug() << "set connectionsB";
    connect(m_AcqWorker, SIGNAL(loadParms(CGageSystem*)), m_config, SLOT(s_loadParms(CGageSystem*)), Qt::QueuedConnection);
    connect(this, SIGNAL(loadParms(CGageSystem*)), m_config, SLOT(s_loadParms(CGageSystem*)), Qt::QueuedConnection);
    connect(m_config, SIGNAL(updateChannels(int)), GetScope(), SLOT(s_UpdateChannelList(int)), Qt::QueuedConnection);
//   connect(m_config, SIGNAL(ParmsChanged()), m_AcqWorker, SLOT(s_ParmsChanged()), Qt::QueuedConnection);
//    connect(m_config, SIGNAL(ParmsChanged()), ui.mainScope->GetGraph(), SLOT(s_DisableData()), Qt::QueuedConnection);
    connect(m_config, SIGNAL(disableData()), ui.mainScope->GetGraph(), SLOT(s_DisableData()), Qt::QueuedConnection);
//    connect(m_AcqWorker, SIGNAL(AcqParmsChanged()), m_config->m_AcqParmsChangedLoop, SLOT(quit()), Qt::QueuedConnection);
    connect(m_AcqWorker, SIGNAL(UpdateScopeParms()), this, SLOT(s_UpdateParms()), Qt::QueuedConnection);
    connect(m_AcqWorker, SIGNAL(AbortCsOp()), m_AsyncCsOps, SLOT(s_AbortCsOp()), Qt::QueuedConnection);
	connect(m_config, SIGNAL(updateCommitTime(int)), this, SLOT(s_UpdateCommitTime(int)), Qt::QueuedConnection);
    //connect(m_config, SIGNAL(StartAcq()), m_AcqWorker->m_AcqWaitingLoop, SLOT(quit()), Qt::DirectConnection);
    //connect(m_config, SIGNAL(StartAcq()), m_AcqWorker, SLOT(s_StartAcq()), Qt::DirectConnection);
	connect(this, SIGNAL(UpdateCommitDone()), m_config->m_UpdateCommitLoop, SLOT(quit()), Qt::QueuedConnection);
	connect(m_config, SIGNAL(ConfigError()), this, SLOT(s_DisplayErrPopup()), Qt::QueuedConnection);
    //connect(m_config, SIGNAL(ResetOnly()), m_AcqWorker, SLOT(s_ResetOnly()), Qt::QueuedConnection);
    //connect(m_AcqWorker, SIGNAL(ResetOnlyDone()), m_config->m_ResetWaitLoop, SLOT(quit()), Qt::QueuedConnection);
    connect(m_pCursorInfo, SIGNAL(disableCursor(bool)), this, SLOT(s_ShowCursor(bool)), Qt::QueuedConnection);
    connect(this, SIGNAL(SetProgBarMax(int)), m_ProgBar, SLOT(s_SetMaximum(int)), Qt::QueuedConnection);
    connect(this, SIGNAL(SetProgBarCurr(int)), m_ProgBar, SLOT(s_SetCurrent(int)), Qt::QueuedConnection);
    connect(ui.mainScope->GetGraph(), SIGNAL(updateScrollBar(int)), ui.mainScope, SLOT(s_UpdateScrollBar(int)), Qt::QueuedConnection);

	
    m_StatusBarInfo.m_BoardStatus = BoardReady;

    qDebug() << "set connectionsC";
    m_UiTimer->setSingleShot(false);
    m_UiTimer->setInterval(50);
    connect(m_UiTimer, SIGNAL(timeout()), this, SLOT(s_UpdateUi()), Qt::QueuedConnection);
    connect(m_UiTimer, SIGNAL(timeout()), m_AcqWorker, SLOT(s_ProcessEvents()), Qt::DirectConnection);
    m_UiTimer->start();

	// Update GUI control
	EnableMenuControls(true);
}

CsTestQt::~CsTestQt()
{
    qDebug() << "CsTest: Destructor";
    //TODO: Procedure before closing windows (ex: save?)
    if(m_pCursorInfo) delete m_pCursorInfo;
	if (m_pReadWriteRegs) delete m_pReadWriteRegs;
	if (m_pNiosSpiCmd) delete m_pNiosSpiCmd;
	if (m_pMulrec) delete m_pMulrec;
	if (m_pReadDataFile) delete m_pReadDataFile;
    if (m_pReadSigFile) delete m_pReadSigFile;
	//if (m_pDataFileBuffer!=NULL) VirtualFree(m_pDataFileBuffer, 0, MEM_RELEASE);

	//Wait until the acquisition is over
	QTime timeout;
	timeout.start();
	
    qDebug() << "CsTest: Destructor: Stop worker thread";
	if(m_WorkerThread)
    {
        int count = 10;
        while((count>0) && (!m_bCanAcquire))
		{
            qDebug() << "CsTest: Destructor: Stop worker thread: Stop current acquisition";
            emit acquisitionThreadMustAbort();
            QApplication::processEvents();
            quSleep(100000);
            if(m_bCanAcquire) break;
            count --;
		}
        if(m_WorkerThread->isRunning())
        {
            qDebug() << "CsTest: Destructor: Stop worker thread: Terminate thread";
            m_WorkerThread->exit();
            QApplication::processEvents();
        }
        while(m_WorkerThread->isRunning())
        {
            quSleep(100000);
        }
	}

    if(m_WorkerThread)
    {
        qDebug() << "CsTest: Destructor: Stop worker thread: Delete thread";
        delete m_WorkerThread;
        m_WorkerThread = NULL;
    }

	if(m_AcqWorker)
	{
        qDebug() << "CsTest: Destructor: Stop worker thread: Delete worker object";
		delete m_AcqWorker;
		m_AcqWorker = NULL;
	}

    qDebug() << "CsTest: Destructor: Stop worker thread: Done";

	if (m_config) delete m_config;

    // Close the current system
    //Commented out because CsFreeSystem is causing problems for now in Linux.
    if(m_pGageSystem)
    {
        qDebug() << "CsTest: Destructor: Free the system";
        m_pGageSystem->Abort();
        quSleep(100);
        int i32FreeStatus = m_pGageSystem->FreeSystem();
        qDebug() << "CsTest: Destructor: Free the system: Status: " << i32FreeStatus;
        quSleep(100);
        delete m_pGageSystem;
        m_pGageSystem = NULL;
    }
    //m_pGageSystem is not delete because it inherits from a QObject and Qt manage how it's deleted.

	if (m_AsyncCsOpsThread)
	{
        qDebug() << "CsTest: Destructor: Stop Async CsOp thread";
        if (m_AsyncCsOpsThread->isRunning())
        {
            m_AsyncCsOpsThread->exit();
            QApplication::processEvents();
        }
        while(m_AsyncCsOpsThread->isRunning())
        {
            quSleep(100000);
        }
		delete m_AsyncCsOpsThread;
        qDebug() << "CsTest: Destructor: Stop Async CsOp thread: Done";
	}
	if (m_AsyncCsOps) delete m_AsyncCsOps;

	if (m_ErrMng) delete m_ErrMng;
	if (m_MsgBox) delete m_MsgBox;
	if (m_AcqWaitLoop) delete m_AcqWaitLoop;
	if (m_PositionLabel) delete m_PositionLabel;
	if (m_AcqCountLabel) delete m_AcqCountLabel;
    if (m_CurrSegLabel) delete m_CurrSegLabel;
	if (m_SrLabel) delete m_SrLabel;
	if (m_XferTimeLabel) delete m_XferTimeLabel;
	if (m_ZoomLabel) delete m_ZoomLabel;
	if (m_BrdStatusLabel) delete m_BrdStatusLabel;
	if (m_UiTimer) delete m_UiTimer;
    if (m_ProgBar) delete m_ProgBar;
    if (m_GoToSample) delete m_GoToSample;

    qDebug() << "CsTest: Destructor: Done";
}


/*******************************************************************************************
										Public slots
*******************************************************************************************/

void CsTestQt::s_Close()
{
	emit close();
}

void CsTestQt::s_SaveFile()
{

	CSHANDLE myHandle = { 0 };
	char * pHeaderStart = 0;
	char * f = 0;
	int32 i32totalSize = 0;
	int32 length = 0;

	QString fileName = QFileDialog::getSaveFileName(this, "Save a file", QString(), "SIGX file (*.SIGX)");

	if (fileName.isEmpty())
	{
		return;
	}
	else {
		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly)) {
			QMessageBox::information(this, tr("Unable to open file"),
				file.errorString());


		}
		else
		{
			QDataStream in(&file);
			::csSaveFile(m_pGageSystem, myHandle, fileName, &pHeaderStart, &f, i32totalSize, length);
			file.write(pHeaderStart, i32totalSize);
            file.write(f, length);

            #ifdef Q_OS_WIN
                VirtualFree(pHeaderStart, i32totalSize, MEM_RELEASE);
                VirtualFree(f, length, MEM_RELEASE);
            #else
                free(pHeaderStart);
                free(f);
            #endif

			file.close();
		}
		return;

	}
}

void CsTestQt::s_SaveChannels()
{
    bool bRes;
	int XferOffset;
    long long XferSize;
	int SaveLen;
    unsigned int CurrSeg;
	int StartChan;
	int ChanSkip;
    int Res;
    unsigned long long idxSamp;
    char *pFileBuffer;
    long long i64XferTime;

	//CSDISKFILEHEADER FileHeader;
	disk_file_header FileHeader;
	CSSIGSTRUCT TraceInfo;
	CSTIMESTAMP TimeStampInfo;
	TimeStampInfo.u16Hour = 0;
	TimeStampInfo.u16Minute = 0;
	TimeStampInfo.u16Point1Second = 0;
	TimeStampInfo.u16Second = 0;

	TraceInfo.u32Size = sizeof(TraceInfo);
	TraceInfo.i64RecordLength = m_pGageSystem->getAcquisitionConfig().i64SegmentSize;
	TraceInfo.i64RecordStart = m_pGageSystem->getAcquisitionConfig().i64TriggerHoldoff * -1;
	TraceInfo.i64SampleRate = m_pGageSystem->getAcquisitionConfig().i64SampleRate;
	TraceInfo.TimeStamp = TimeStampInfo;
	TraceInfo.u32RecordCount = m_pGageSystem->getAcquisitionConfig().u32SegmentCount;
	TraceInfo.u32SampleBits = m_pGageSystem->getSystemInfo().u32SampleBits;
	TraceInfo.u32SampleSize = m_pGageSystem->getSystemInfo().u32SampleSize;

    void *pXferBuffer = malloc((XFER_BUFFER_SIZE + 1000) * TraceInfo.u32SampleSize);

	StartChan = m_pGageSystem->GetStartChannel();
	ChanSkip = m_pGageSystem->GetChannelSkip();

	//Ask user for file name
	QFile ChanFile;
	QString strPath, strPathAndName, strFileName, strCurrFileName;
	QString strChan;

	strPath.clear();
	strFileName.clear();
    bool bOverwrite = false;
	int i32MsgRes;

    strPathAndName = QFileDialog::getSaveFileName(this, "Please select the file to save the signal files", QDir::currentPath(), "Signal Files (*.sig)");
    qDebug() << "s_SaveChannels: FilePath: " << strPathAndName;

    if(strPathAndName=="") return;

	//Extract path and file name from full path.
	int PathLength, NameLength;
	PathLength = strPathAndName.lastIndexOf("/");
	strPath = strPathAndName.left(PathLength);
	strFileName = strPathAndName.right(strPathAndName.length() - PathLength - 1);
	NameLength = strFileName.lastIndexOf(".");
	if (NameLength == -1) NameLength = strFileName.length();
	strFileName = strFileName.left(NameLength);

    unsigned int idxXfer = 0;
    unsigned int ProgessBarMax = (TraceInfo.i64RecordLength * TraceInfo.u32RecordCount * m_pGageSystem->GetActiveChannelCount()) / XFER_BUFFER_SIZE;
    m_ProgBar->setModal(true);
    m_ProgBar->show();
    m_ProgBar->SetMaximum(ProgessBarMax);

	for (int idxChan = 0; idxChan<m_pGageSystem->GetActiveChannelCount(); idxChan++)
	{
		TraceInfo.u32Channel = (idxChan * ChanSkip) + StartChan;

		//Generate full path and file name.
        strChan = QString("_CH%1").arg(TraceInfo.u32Channel);
        strCurrFileName = strPath + "/" + strFileName + strChan + ".sig";
		ChanFile.setFileName(strCurrFileName);
		
		//If exists, ask user if overwrite.
        if ((ChanFile.exists()) && !bOverwrite)
		{
            QMessageBox MsgOverwriteFile(QMessageBox::NoIcon, strCurrFileName, "File already exists. Overwrite?", QMessageBox::Yes | QMessageBox::No, this, Qt::Dialog);
			i32MsgRes = MsgOverwriteFile.exec();
            if (i32MsgRes == QMessageBox::Yes) bOverwrite = true;
			else bOverwrite = false;
		}
        else bOverwrite = true;

		if (bOverwrite)
		{
            bRes = ChanFile.open(QIODevice::WriteOnly);
			if (!bRes)
			{
                free(pXferBuffer);
				m_ErrMng->UpdateErrMsg("Error creating the signal file", -1);
				return;
			}

			TraceInfo.u32InputRange = ui.mainScope->GetGraph()->GetChannel(idxChan)->getRange();
			TraceInfo.i32DcOffset = ui.mainScope->GetGraph()->GetChannel(idxChan)->getDcOffset();
			TraceInfo.i32SampleOffset = ui.mainScope->GetGraph()->GetChannel(idxChan)->getSampleOffset();
			TraceInfo.i32SampleRes = ui.mainScope->GetGraph()->GetChannel(idxChan)->getSampleRes();

            qDebug() << "SaveChannels: Create sig header";
            Res = CsConvertToSigHeader((CSDISKFILEHEADER*)(&FileHeader), &TraceInfo, NULL, NULL);
            if(Res<0)
            {
                ChanFile.close();
                free(pXferBuffer);
                m_ErrMng->UpdateErrMsg("Error converting file to .sig format", -1);
                m_ProgBar->hide();
                return;
            }

            qDebug() << "SaveChannels: Write sig header";
			bRes = ChanFile.write((char*)(&FileHeader), 512);
			if (!bRes)
			{
				ChanFile.close();
                free(pXferBuffer);
				m_ErrMng->UpdateErrMsg("Error saving header in the signal file", -1);
                m_ProgBar->hide();
				return;
			}

            qDebug() << "SaveChannels: Save segments";
			for (CurrSeg = 1; CurrSeg <= TraceInfo.u32RecordCount; CurrSeg++)
			{
                qDebug() << "SaveChannels: Save segments: " << CurrSeg;
                for (idxSamp=0; idxSamp < (unsigned long long)TraceInfo.i64RecordLength; )
                {
                    XferSize = TraceInfo.i64RecordLength - idxSamp;
                    if((XferSize % MIN_XFER_SIZE)!=0) XferSize = ((XferSize / MIN_XFER_SIZE) + 1) * MIN_XFER_SIZE;
                    if(XferSize>XFER_BUFFER_SIZE) XferSize = XFER_BUFFER_SIZE;

                    //qDebug() << "SaveChannels: Save segments: ";
                    Res = m_pGageSystem->TransferDataBuffer(TraceInfo.u32Channel, CurrSeg, pXferBuffer, idxSamp, XferSize, &XferOffset, &i64XferTime);
                    if(Res<0)
                    {
                        ChanFile.close();
                        free(pXferBuffer);
                        m_ErrMng->UpdateErrMsg("Error transfering data from the board.", -1);
                        m_ProgBar->hide();
                        return;
                    }

                    pFileBuffer = (char *)pXferBuffer;
                    pFileBuffer+=(XferOffset * TraceInfo.u32SampleSize);
                    SaveLen = (XferSize - XferOffset) * TraceInfo.u32SampleSize;
                    if ((XferSize - XferOffset) > (TraceInfo.i64RecordLength - (int64)idxSamp))
                    {
                        SaveLen = (TraceInfo.i64RecordLength - idxSamp) * TraceInfo.u32SampleSize;
                    }
                    else
                    {
                        SaveLen = (XferSize - XferOffset) * TraceInfo.u32SampleSize;
                    }

                    bRes = ChanFile.write(pFileBuffer, SaveLen);
                    if (!bRes)
                    {
                        ChanFile.close();
                        free(pXferBuffer);
                        m_ErrMng->UpdateErrMsg("Error saving data in the signal file", -1);
                        m_ProgBar->hide();
                        return;
                    }

                    idxSamp+=SaveLen / TraceInfo.u32SampleSize;
                    idxXfer++;
                    //emit SetProgBarCurr(idxXfer);
                    m_ProgBar->SetValue(idxXfer);
                }
			}
			ChanFile.close();
		}
        else
        {
            m_ProgBar->hide();
            free(pXferBuffer);
            return;
        }
	}
    free(pXferBuffer);
    m_ProgBar->hide();
}

void CsTestQt::s_LoadChannels()
{
    //qDebug() << "CsTestQt: s_LoadChannels";

    bool bRes;
    int Res;
    QFile SigFile;
    QByteArray SigByteArr;

    //CSDISKFILEHEADER FileHeader;
    disk_file_header *pFileHeader;
    CSSIGSTRUCT TraceInfo;
    TraceInfo.u32Size = sizeof(CSSIGSTRUCT);

    m_pReadSigFile->exec();

    if(m_pReadSigFile->m_bCancelled)
    {
        qDebug() << "CsTestQt: s_LoadChannels: Cancelled";

        return;
    }

    //qDebug() << "CsTestQt: s_LoadChannels: Set variables";

    SigFile.setFileName(m_pReadSigFile->m_DataFilePath);

    bRes = SigFile.open(QIODevice::ReadOnly);
    if (!bRes)
    {
        m_ErrMng->UpdateErrMsg("Error creating the signal file", -1);
        return;
    }

    //qDebug() << "CsTestQt: s_LoadChannels: Read sig header";
    SigByteArr = SigFile.read(SIG_FILE_HEADER_SIZE);
    pFileHeader = (disk_file_header *)SigByteArr.data();

    //qDebug() << "CsTestQt: s_LoadChannels: Convert sig header";
    Res = CsConvertFromSigHeader((CSDISKFILEHEADER*)(pFileHeader), &TraceInfo, NULL, NULL);
    if(Res<0)
    {
        //qDebug() << "CsTestQt: s_LoadChannels: Error in sig header: " << Res;
        SigFile.close();
        m_ErrMng->UpdateErrMsg("Error converting from .sig format", -1);
        return;
    }

    //qDebug() << "CsTestQt: s_LoadChannels: Sample size: " << TraceInfo.u32SampleSize;
    //qDebug() << "CsTestQt: s_LoadChannels: Sample res: " << TraceInfo.i32SampleRes;
    //qDebug() << "CsTestQt: s_LoadChannels: Sample offset: " << TraceInfo.i32SampleOffset;
    //qDebug() << "CsTestQt: s_LoadChannels: DC offset: " << TraceInfo.i32DcOffset;
    //qDebug() << "CsTestQt: s_LoadChannels: Range: " << TraceInfo.u32InputRange;
    //qDebug() << "CsTestQt: s_LoadChannels: Record length: " << TraceInfo.i64RecordLength;
    //qDebug() << "CsTestQt: s_LoadChannels: Record count: " << TraceInfo.u32RecordCount;

    //qDebug() << "CsTestQt: s_LoadChannels: Add sig channel";
    ui.mainScope->GetGraph()->addSigChannel(&TraceInfo, m_pReadSigFile->m_DataFilePath);

    ui.mainScope->AdjustHScrollBar();
}

void CsTestQt::s_LoadData()
{
    qDebug() << "CsTestQt: Load data";

	m_pReadDataFile->exec();

    if(m_pReadDataFile->m_bCancelled)
    {
        //qDebug() << "CsTestQt: Load data: Cancelled";

        return;
    }

    qDebug() << "CsTestQt: Load data: Set variables";

	int ChanCount = m_pReadDataFile->m_i32ChanCount;
	int SampleSize = m_pReadDataFile->m_i32SampleSize;

	ui.mainScope->GetGraph()->m_bGageDataValid = false;

    qDebug() << "CsTestQt: Load data: Set channels";
    ui.mainScope->setFileChannels(ChanCount, SampleSize, 2000, m_pReadDataFile->m_DataFilePath);
    qDebug() << "CsTestQt: Load data: Set channels: Done";

    ui.mainScope->AdjustHScrollBar();

    qDebug() << "CsTestQt: Load data: Done";
}

void CsTestQt::ErrorClear()
{
	m_strErrMsg.clear();
}

void CsTestQt::ErrorAppend(QString ErrString)
{
	m_strErrMsg.append(ErrString);
}

QString CsTestQt::ErrorGet()
{
	return m_strErrMsg;
}

void CsTestQt::s_SelectSystem()
{
    int32 i32Status = SelectSystem();
	if(i32Status<0)
	{
		m_ErrMng->UpdateErrMsg("Error selecting the system", i32Status);
		m_ErrMng->bQuit = true;
		emit Error();
	}
}

void CsTestQt::s_SetInterleavedData(bool bState)
{
	Q_UNUSED(bState);
}

int CsTestQt::SelectSystem()
{
    qDebug() << "CsTestQt: Select system";

/*	bool bDataValid;

	if (!m_bCanAcquire) s_DoAbort();
	ui.actionTBContinuous->setChecked(false);
	s_SetContinuousMode(false);
	bDataValid = ui.mainScope->GetGraph()->m_bGageDataValid;
	ui.mainScope->GetGraph()->m_bGageDataValid = false;
    while(m_bAcqRunning)
    {
        quSleep(10000);
        QCoreApplication::processEvents();
    }
*/
	//If a system is already present, we must close it first
    m_pGageSystem->setSystemHandle(0);
	setWindowTitle("CsTest Qt");

	// Initialize and find the number of systems
	int32 i32SystemCount;
    if (m_i32BrdCount == -1)
	{
        qDebug() << "CsTestQt: Select system: Initialize";
		i32SystemCount = m_pGageSystem->Initialize();
        qDebug() << "CsTestQt: Select system: System count: " << i32SystemCount;
		m_i32BrdCount = i32SystemCount;
	}
	else i32SystemCount = m_i32BrdCount;

    qDebug() << "CsTestQt: Select system: Board count: " << m_i32BrdCount;

	// No system found = Error
    if (i32SystemCount < 0)
	{
        qDebug() << "CsTestQt: Select system: No system found";
        m_MsgBox->setText(m_pGageSystem->GetErrorString(i32SystemCount));
        m_MsgBox->exec();
        return -1;
	}
	// Only one system found, it's ok
	else if(i32SystemCount == 1)
	{
		// Get the system and use it
		m_i32BrdIdx = 1;
        ui.mainScope->setBrdIdx(m_i32BrdIdx);
		int32 i32Status = m_pGageSystem->GetSystem(1, false);
		if ( CS_FAILED(i32Status) )
		{
            QMessageBox MsgBox;
            MsgBox.setText("The only board present in the system is not available.\r\nCsTestQt will exit.");
            MsgBox.exec();
			return i32Status;
		}

        CSSYSTEMINFO SystemInfo = m_pGageSystem->getSystemInfo();
        if(SystemInfo.i64MaxMemory==0)
        {
            QMessageBox MsgBox;
            MsgBox.setText("The system shows a memory size of 0.\r\nCsTestQt will exit.");
            MsgBox.exec();
            return -1;
        }

		ui.actionSelect_System->setDisabled(true);

		m_pGageSystem->setSystemHandle(m_pGageSystem->getSystemHandle());
	}
    else 	// If more than one system is found, put up a dialog box to choose one of them
	{
        // List all the free system board names and memory sizes
        qDebug() << "CsTestQt: Select system: System count: " << i32SystemCount;
		CSSYSTEMINFO SystemInfo;
		SystemInfo.u32Size = sizeof(CSSYSTEMINFO);
        QVector<System> hSystemVector;

        for(int i = 0, j = 0; i < i32SystemCount; i++, j++)
		{
            int idx = i + 1;
            CSHANDLE csHandle;
            int32 i32Status = m_pGageSystem->CompuscopeGetSystem(&csHandle, idx);
            qDebug() << "CsTestQt: Select system: Connect status: " << i32Status;
	
            if (CS_SUCCEEDED(i32Status))
			{
                System system;
                char strSerialNum[100];
                m_pGageSystem->CompuscopeGetSystemInfo(csHandle, &SystemInfo);
                m_pGageSystem->GetSerialNumber(csHandle, &strSerialNum[0]);

                system.csHandle = csHandle;
                system.strBoardName.clear();
                system.strBoardName.append(SystemInfo.strBoardName);
                system.strSerialNumber.clear();
                system.strSerialNumber.append(QString(&strSerialNum[0]));
                system.i64MemorySize = SystemInfo.i64MaxMemory;
                system.i32BitCount = SystemInfo.u32SampleBits;
                system.i32SampSize = SystemInfo.u32SampleSize;
                system.i32ChanCount = SystemInfo.u32ChannelCount;
                system.i32BrdCount = SystemInfo.u32BoardCount;
                system.index = idx;
                system.bFree = true;
                hSystemVector.push_back(system);
                i32Status = m_pGageSystem->FreeSystem(csHandle);
                qDebug() << "CsTestQt: Select system: Free status: " << i32Status;
			}
            if(hSystemVector.size()==i32SystemCount) break;
		}

        if (hSystemVector.empty())
		{
            QMessageBox MsgBox;
            MsgBox.setText("No board currently available in the system.\r\nCsTestQt will exit.");
            MsgBox.exec();
			return -1;
		}

		// Create the dialog to let the user select the system
        //qDebug() << "CsTestQt: Select system: Show dialog";
        qDebug() << "CsTestQt: Select system: Systems array size: " << hSystemVector.size();
        QSystemSelectionDialog* sysDialog = new QSystemSelectionDialog(0 , hSystemVector, m_strCurrSerNum);
		sysDialog->exec();
		
		// Get the user response
        int	i32SelectedSystemIndex = sysDialog->getIndexSelectedSystem();
		delete sysDialog;

		// Free all the other (unselected) systems
		bool bSuccess = true; //Indicate if the free operation has been a success for all the systems

		//Force close if there is no selection and no board previously selected (launch)
		if ((-1 == i32SelectedSystemIndex) && (m_i32BrdIdx==-1))
		{
            qDebug() << "CsTestQt: Select system: No board selected or previously selected";
			m_ErrMng->ClrErr();
            m_ErrMng->UpdateErrMsg("No system selected", -1);
			return -1;
		}
		//Use previous board if there is no selection
		else if ((-1 == i32SelectedSystemIndex) && (m_i32BrdIdx >= 1))
		{
            qDebug() << "CsTestQt: Select system: No board selected. Use previous";
			int32 i32Status;
            i32Status = m_pGageSystem->GetSystem(m_i32BrdIdx, true);
			if (CS_FAILED(i32Status))
			{
				m_ErrMng->ClrErr();
				m_ErrMng->UpdateErrMsg("Error getting the selected system.", i32Status);
				return i32Status;
			}

			m_pGageSystem->setSystemHandle(m_pGageSystem->getSystemHandle());
			//ui.mainScope->GetGraph()->m_bGageDataValid = bDataValid;
            setWindowTitle(QString(m_pGageSystem->getSystemInfo().strBoardName) + " --- CsTest Qt");
			return 0;
		}
		//Force close if error.
		else if(!bSuccess)
		{
            qDebug() << "CsTestQt: Select system: Error: Force close";
			m_ErrMng->ClrErr();
			m_ErrMng->UpdateErrMsg("Error getting the list of systems.", -1);
			return -1;
		}
		// Save the selected system
		else
		{
            qDebug() << "CsTestQt: Select system: Use selected system: Board idx: " << i32SelectedSystemIndex;
			int32 i32Status;
            m_i32BrdIdx = i32SelectedSystemIndex;
            ui.mainScope->setBrdIdx(m_i32BrdIdx);
            i32Status = m_pGageSystem->GetSystem(m_i32BrdIdx, false);
            qDebug() << "CsTestQt: Select system: Use selected system: Connect status: " << i32Status;
			if (CS_FAILED(i32Status))
			{
				m_ErrMng->ClrErr();
				m_ErrMng->UpdateErrMsg("Error getting the selected system.", i32Status);
				return i32Status;
			}

			m_pGageSystem->setSystemHandle(m_pGageSystem->getSystemHandle());
		}
	}

    qDebug() << "CsTestQt: Select system: Update graph variables";
	ui.mainScope->GetGraph()->m_bGageDataValid = false;
	ui.mainScope->GetGraph()->m_bInterleavedData = m_bInterleavedData;
	m_pGageSystem->m_bInterleavedData = m_bInterleavedData;
    unsigned long long PreTrig = m_pGageSystem->getPreTrigger();
	ui.mainScope->setGageSystem(m_pGageSystem);
	ui.mainScope->setSampleSize(m_pGageSystem->getSampleSize());
	ui.mainScope->setAvgCount(1);
    ui.mainScope->GetGraph()->setBoardActiveChans(m_pGageSystem->GetActiveChannelCount());
    ui.mainScope->setChannels();
	ui.mainScope->GetGraph()->setPreTrig(PreTrig);
	ui.mainScope->GetGraph()->setTrigPos(PreTrig);
    ui.mainScope->AdjustHScrollBar();
    ui.mainScope->GetGraph()->s_movePositionX(0);
	m_u64XferSize = m_pGageSystem->GetActiveChannelCount() * (m_pGageSystem->getSegmentSize() * m_pGageSystem->getSampleSize());
    GetScope()->GetGraph()->AdjustViewPos();
	s_UpdateSampRateStatus();

    char CurrSerNum[200];
    m_pGageSystem->GetSerialNumber(m_pGageSystem->getSystemHandle(), &CurrSerNum[0]);
    m_strCurrSerNum = QString("%1").arg(&CurrSerNum[0]);
	setWindowTitle(QString(m_pGageSystem->getSystemInfo().strBoardName) + " --- CsTest Qt");

	return 0;
}

void CsTestQt::s_OpenAboutDialog()
{
	QAboutDialog* aboutDialog = new QAboutDialog();
	aboutDialog->exec();
}

void CsTestQt::s_OpenControlsDialog()
{
	QDialog* controlsDialog = new QDialog();
	::Ui::QControlsDialog design;
	
	design.setupUi(controlsDialog);
	controlsDialog->exec();
}

void CsTestQt::s_DoPerformanceTest()
{
	QPerformanceTestDialog* perfTest = new QPerformanceTestDialog();
	perfTest->exec();
}

void CsTestQt::s_ConfigureSystem()
{
    //qDebug() << "*****************s_ConfigureSystem*************************";
	m_config->s_ShowConfigWindow(m_pGageSystem);
}

void CsTestQt::s_SaveTimeStamp()
{
	IN_PARAMS_TRANSFERDATA		InParams;
    OUT_PARAMS_TRANSFERDATA		OutParams;// = { 0 };
	uInt32			u32TimeStampCount = m_pGageSystem->getSegmentCount();
	int64			TsFrequency = 0;
	int64			*TsBuffer;
	int32			i32Status = CS_SUCCESS;
	uInt32			u32Delta = 0;
	int64			i64LastTs = 0;
	uInt32			i = 0;

	i32Status = m_pGageSystem->CsGetInfo(m_pGageSystem->getSystemHandle(), CS_PARAMS, CS_TIMESTAMP_TICKFREQUENCY, &TsFrequency); 
	if (i32Status<0)
	{
		// Virtual system does not support timeStamp.
		m_ErrMng->UpdateErrMsg("Error getting time stamps frequency", -1);
		emit Error();
		return;
	}

    #ifdef Q_OS_WIN
        TsBuffer = (int64*)VirtualAlloc(NULL, (unsigned long long)u32TimeStampCount*sizeof(int64), MEM_COMMIT, PAGE_READWRITE);
    #else
        TsBuffer = (int64*)malloc((unsigned long long)u32TimeStampCount*sizeof(int64));
    #endif

	InParams.u32Segment = 1;
	InParams.u16Channel = 1;
	InParams.i64StartAddress = 1;
	InParams.i64Length = u32TimeStampCount;
	InParams.pDataBuffer = TsBuffer;
	InParams.hNotifyEvent = NULL;
	InParams.u32Mode = TxMODE_TIMESTAMP;
	


	i32Status = CsTransfer(m_pGageSystem->getSystemHandle(), &InParams, &OutParams);
	if (i32Status<0)
	{
		m_ErrMng->UpdateErrMsg("Error getting time stamps", -1);
		s_DisplayErrPopup();
		emit Error();
		return;
	}

	QFile TimeStampFile;
	QString strPath;
	QString strFileName;

	strPath.clear();
	strFileName.clear();
	strPath = QFileDialog::getExistingDirectory(this, "Please select the directory to save the time stamps files", "/home", QFileDialog::ShowDirsOnly);
	strFileName = QString("%1/TimeStamp.txt").arg(strPath);
	TimeStampFile.setFileName(strFileName);
	TimeStampFile.open(QIODevice::WriteOnly);
	QTextStream out(&TimeStampFile);
	out << "TimeStamp Frequency = " << TsFrequency << "\r\n"
		<< "Segment Start Ts (H/L)	" <<"	Delta(Samples)"	<< "\r\n";

	i64LastTs = (uInt32)TsBuffer[0];
	for (i = 0; i < u32TimeStampCount; i++)
	{
		QString str32TsLow = QString("%1").arg((int32)TsBuffer[i], 8, 16, QChar('0'));
		QString str32TsHigh = QString("%1").arg((int32)(TsBuffer[i] >> 32), 8, 16, QChar('0'));
		u32Delta = (uInt32)(TsBuffer[i] - i64LastTs);
		u32Delta = (uInt32)(m_pGageSystem->getSampleRate()*u32Delta / TsFrequency);

		out << i + 1 << "	"
			<< "0x" << str32TsHigh << " "
			<< "0x" << str32TsLow << "	";
		out << u32Delta << "\r\n";

		i64LastTs = TsBuffer[i];
	}
	TimeStampFile.close();
}

void CsTestQt::s_UpdateParms()
{
    qDebug() << "CsTestQt: Update parms";
	int i32Status = 0;

	ui.mainScope->GetGraph()->m_bGageDataValid = false;
	ui.mainScope->GetGraph()->m_bInterleavedData = m_bInterleavedData;
	m_pGageSystem->m_bInterleavedData = m_bInterleavedData;
    unsigned long long PreTrig = m_pGageSystem->getPreTrigger();
//RG    i32Status = m_pGageSystem->allocateMemory(m_pGageSystem->getAcquisitionMode(), m_pGageSystem->getSegmentSize());
    i32Status = m_pGageSystem->allocateMemory(m_pGageSystem->getAcquisitionMode());
    //qDebug() << "CsTestQt: Update parms: Reallocate done";
	if (i32Status == CS_CONFIG_CHANGED)
	{
        //qDebug() << "not enough memory";
        //Disabled the code displaying the warning pop-up to the user, because this is blocking the slot and then getting CsTestQt to crash.
        emit ShowMessage("Not enough memory available for requested segment size.\r\nPre-trigger set to 0.\r\nPost-trigger set to 160.");
        while(!m_MsgBox->isVisible())
        {
            quSleep(1000);
            QCoreApplication::processEvents();
        }
        while(m_MsgBox->isVisible())
        {
            quSleep(1000);
            QCoreApplication::processEvents();
        }
	}
	if (i32Status<0)
	{
		// Virtual system does not support timeStamp.
		m_ErrMng->UpdateErrMsg("Error reallocating memory", -1);
		emit Error();
		return;
	}
    qDebug() << "Set scope: Set gage system";
	ui.mainScope->setGageSystem(m_pGageSystem);
    qDebug() << "Set scope: Update channels list";
	ui.mainScope->s_UpdateChannelList(m_pGageSystem->GetActiveChannelCount());
    qDebug() << "Set scope: Set sample size";
	ui.mainScope->setSampleSize(m_pGageSystem->getSampleSize());
    qDebug() << "Set scope: Set AVG count";
	ui.mainScope->setAvgCount(m_pGageSystem->getMulRecAvgCount());
    ui.mainScope->GetGraph()->setSampleCount(m_pGageSystem->getSegmentSize());
    qDebug() << "Set scope: Set pre-trig";
	ui.mainScope->GetGraph()->setPreTrig(PreTrig);
    qDebug() << "Set scope: Set trigger pos";
	ui.mainScope->GetGraph()->setTrigPos(PreTrig);
	m_u64XferSize = m_pGageSystem->GetActiveChannelCount() * (m_pGageSystem->getSegmentSize() * m_pGageSystem->getSampleSize());
    qDebug() << "emit updateSampRateStatus";
	emit updateSampRateStatus();

    if(GetScope()->GetGraph()->m_bCursor)
    {
        //qDebug() << "CsTestQt: Update parms: Update cursor vals";
        GetScope()->GetGraph()->setCursor(GetScope()->GetGraph()->getCursorScreenPos());
    }

    GetScope()->GetGraph()->AdjustZoom();
    GetScope()->GetGraph()->AdjustViewPos();
    ui.mainScope->AdjustHScrollBar();

    m_bUpdateScopeParmsDone = true;

    qDebug() << "CsTestQt: Update parms: Done";
}

void CsTestQt::s_SetBusMasterSynchronous()
{
	ui.actionBusmaster_Asynchronous->setChecked(false);
	ui.actionSlave_Synchronous->setChecked(false);

	//TODO:: Something with the system?...
}

void CsTestQt::s_SetBusMasterASynchronous()
{
	ui.actionBusmaster_Synchronous->setChecked(false);
	ui.actionSlave_Synchronous->setChecked(false);

	//TODO:: Something with the system?...
}

void CsTestQt::s_SetSlaveSynchronous()
{
	ui.actionBusmaster_Asynchronous->setChecked(false);
	ui.actionBusmaster_Synchronous->setChecked(false);

	//TODO:: Something with the system?...
}

void CsTestQt::s_DoAcquisition()
{
    //qDebug() << "s_DoAcquisition";

	if(m_bCanAcquire)
	{
        //qDebug() << "s_DoAcquisition: Trigger acquisition";

		m_bCanAcquire = false;

		// Update GUI control
		EnableMenuControls(false);

        ui.mainScope->GetGraph()->ClearAbort();
		ui.mainScope->GetGraph()->setAcquiring(!m_bCanAcquire);
        DrawableChannel *pDrawChan = ui.mainScope->GetGraph()->GetChannel(0);
        if(pDrawChan->getChanType()!=drawchanreg) ui.mainScope->setChannels();

		ui.mainScope->setAbort(false);
		emit acquisitionThreadMustStart();
	}
}

void CsTestQt::s_SetContinuousMode(bool bMode)
{
     qDebug() << "s_SetContinuousMode";

	//Synchronise with the model
	emit acquisitionThreadMustSetContinuous(bMode);

	// Update GUI
	ui.actionContinuous->blockSignals(true);
	ui.actionContinuous->setChecked(bMode);
	ui.actionTBContinuous->setChecked(bMode);
	ui.actionContinuous->blockSignals(false);

	// Do not allow users to change configurations
	EnableMenuControls(!bMode);

	if(m_bCanAcquire && bMode)
	{
        qDebug() << "s_SetContinuousMode1";
        DrawableChannel *pDrawChan = ui.mainScope->GetGraph()->GetChannel(0);
        if(pDrawChan->getChanType()!=drawchanreg) ui.mainScope->setChannels();

        ui.mainScope->GetGraph()->ClearAbort();
		ui.mainScope->GetGraph()->m_bInterleavedData = m_bInterleavedData;
		m_pGageSystem->m_bInterleavedData = m_bInterleavedData;

		m_bCanAcquire = false;
		ui.mainScope->GetGraph()->setAcquiring(!m_bCanAcquire);
		ui.mainScope->setAbort(false);
 
		emit acquisitionThreadMustStart();
	}

	if ((!m_bCanAcquire) && (!bMode))
	{
        qDebug() << "**********s_SetContinuousMode2**************";
        emit acquisitionThreadMustStop();
        int Count = 10;
        while(!m_bCanAcquire && (Count>0))
        {
            QCoreApplication::processEvents();
            quSleep(100000);
            Count--;
        }
        if(!m_bCanAcquire)
        {
  /*          QMessageBox MsgBox;
            MsgBox.setButtonText(QMessageBox::Ok, "Abort");
            MsgBox.setText("Please wait for the acquisition to be done.");
            MsgBox.setModal(true);
            MsgBox.show();
            while(!m_bCanAcquire && MsgBox.isVisible())
            {
                QCoreApplication::processEvents();
                quSleep(100000);
            }
            if(MsgBox.isVisible())
            {
                MsgBox.hide();
            }
            else*/
            {
                emit acquisitionThreadMustAbort();
                ui.mainScope->setAbort(true);
            }
        }

        ui.mainScope->GetGraph()->setAcquiring(!m_bCanAcquire);
        //qDebug() << "**********s_SetContinuousMode2 done**************";
	}
    qDebug() << "s_SetContinuousMode: Done";
}

void CsTestQt::s_ConnectPoints(bool bMode)
{
    /*
    char strSerNum[100];
    m_pGageSystem->GetSerialNumber(&strSerNum[0]);
    QString qstrSerNum = QString(&strSerNum[0]);
    QMessageBox MsgSerialNum(QMessageBox::NoIcon, "", qstrSerNum);
    MsgSerialNum.exec();
    */


	ui.mainScope->GetGraph()->m_bConnectPoints = bMode;
    if(m_bCanAcquire) ui.mainScope->GetGraph()->updateGraph(GraphInit);
	return;
}

void CsTestQt::s_Overdraw(bool bMode)
{
	ui.mainScope->GetGraph()->m_bOverdraw = bMode;
    emit SetOverdraw(bMode);
	return;
}

void CsTestQt::s_ClearGraph()
{
	if(m_bCanAcquire)
	{
		m_pGageSystem->ClearAcqCount();
        //s_UpdateStatus(0);
	}
    emit clearGraph();
    m_StatusBarInfo.m_u64StatBarCurrSeg = 0;

    m_pCursorInfo->hide();
    ui.actionTBShowCursor->blockSignals(true);
    ui.actionTBShowCursor->setChecked(false);
    ui.actionTBShowCursor->blockSignals(false);
}

void CsTestQt::s_ShowCursor(bool bShow)
{
    if(m_bSettingCursor) return;

    m_bSettingCursor = true;
    qDebug() << "s_ShowCursor";
    ui.actionTBShowCursor->blockSignals(true);
    if(!m_bCanAcquire) ui.mainScope->GetGraph()->blockSignals(true);

    if(bShow)
    {
        if(!GetScope()->GetGraph()->m_bGageDataValid)
        {
            ui.actionTBShowCursor->setChecked(false);
            ui.actionTBShowCursor->blockSignals(false);
            m_bSettingCursor = false;
            return;
        }
        else
        {
            ui.mainScope->GetGraph()->setCursor(width()/2);
            m_pCursorInfo->s_Show();
        }
    }
    else
    {
        ui.actionTBShowCursor->setChecked(false);
        ui.mainScope->GetGraph()->unsetCursor();
        m_pCursorInfo->s_Hide();
    }

    quSleep(100000);
    ui.mainScope->GetGraph()->blockSignals(false);
    ui.actionTBShowCursor->blockSignals(false);
    m_bSettingCursor = false;
    qDebug() << "s_ShowCursor: Done";
}


void CsTestQt::s_DoAbort()
{
    //qDebug() << "s_DoAbort";
	if ( ui.actionTBContinuous->isChecked() )
		s_SetContinuousMode(false);
	else
		emit acquisitionThreadMustAbort();

    //ui.mainScope->setAbort(true);
	//ui.actionTBContinuous->setChecked(false);
}

void CsTestQt::s_DoForceTrigger()
{
	emit ForceTriggerCsOp();
}

void CsTestQt::s_DoForceCalibration()
{
    if(m_bCanAcquire)
    {
        QMessageBox MsgBox;
        MsgBox.setText("Calibration in progress....");
        MsgBox.setModal(true);
        MsgBox.setStandardButtons(QMessageBox::NoButton);
        MsgBox.show();
        QCoreApplication::processEvents();
        m_pGageSystem->ForceCalib();
        quSleep(500000);
        MsgBox.hide();
    }
    else
    {
        QMessageBox MsgBox;
        MsgBox.setText("Cannot force a calibration while an acquisition is running.");
        MsgBox.exec();
    }
}

void CsTestQt::s_XferMem()
{
	if(!ui.actionContinuous->isChecked() && m_bCanAcquire)
	{
        if(m_bParmsChanged)
        {
            m_MsgBox->setText("The board parameters have changed since the last acquisition.\r\nThe memory content is not valid.");
            m_MsgBox->exec();
            return;
        }

		m_bCanAcquire = false;
		ui.mainScope->GetGraph()->setAcquiring(!m_bCanAcquire);
		ui.mainScope->setAbort(false);
        s_UpdateParms(); // CR # 5873 (in Linux View)
		emit xferMem();
	}
}

void CsTestQt::s_DoAddOnReset()
{
	//TODO:: A lot of things...
}

void CsTestQt::s_DoSystemReset()
{
	if(!ui.actionContinuous->isChecked() && m_bCanAcquire)
	{
		m_bCanAcquire = false;
		ui.mainScope->GetGraph()->setAcquiring(!m_bCanAcquire);
		ui.mainScope->setAbort(false);
		m_pGageSystem->ResetSystemUpdateInfo();
		m_pGageSystem->UpdateSystemInformation();
		emit loadParms(m_pGageSystem);
		m_bCanAcquire = true;
		ui.mainScope->GetGraph()->setAcquiring(!m_bCanAcquire);
		m_bParmsChanged = true;

	}
}

void CsTestQt::s_GoToSample()
{
    qDebug() << "s_GoToSample";


    if(!GetScope()->GetGraph()->m_bGageDataValid)
    {
        m_MsgBox->setText("No acquisition done since last change done to board parameters.");
        m_MsgBox->exec();
        return;
    }
    else
    {
        m_GoToSample->exec();
        if(m_GoToSample->IsAddressValid())
        {
            long long SampAddr = m_GoToSample->GetAddress();

            qDebug() << "s_GoToSample: SampAddr: " << SampAddr;

            ui.actionTBShowCursor->blockSignals(true);
            ui.actionTBShowCursor->setChecked(true);
            ui.actionTBShowCursor->blockSignals(false);
            ui.mainScope->GetGraph()->GoToSample(SampAddr, true);
            m_pCursorInfo->s_Show();
        }
    }

    quSleep(100000);

    qDebug() << "s_GoToSample: Done";
}

void CsTestQt::s_EnableAcquisition(bool bCanAcquire)
{
    Q_UNUSED (bCanAcquire);
    m_MutexCanAcquire.lock();
    m_bCanAcquire = true;
    
	ui.mainScope->GetGraph()->setAcquiring(!m_bCanAcquire);
	// Allow user to change configuration
	EnableMenuControls(true);

    m_MutexCanAcquire.unlock();
}

void CsTestQt::keyPressEvent(QKeyEvent* event)
{
	switch(event->key())
	{
		case Qt::Key_F6:
			emit xferMem();
			break;

        case Qt::Key_S:
            if (QApplication::keyboardModifiers() & Qt::ControlModifier)
            {
                emit SaveChannel();
            }
            break;

        case Qt::Key_L:
            if (QApplication::keyboardModifiers() & Qt::ControlModifier)
            {
                emit LoadData();
            }
            break;

		default:
			break;
	}
}

void CsTestQt::mousePressEvent(QMouseEvent *eventPress)
{
	QPoint p;

	switch(eventPress->buttons())
	{
		case Qt::LeftButton:
            //qDebug() << "CsTestQt: Left button clicked";
            if(ui.actionTBShowCursor->isChecked())
            {
                p = ui.mainScope->mapFromGlobal(QCursor::pos());
                if (ui.mainScope->GetGraph()->m_bGageDataValid)
                {
                    ui.mainScope->GetGraph()->setCursor(p.x());
                }
            }
			break;

		default:
			break;
	}
}

void CsTestQt::s_UpdateStatus(int seg)
{
	// showMessage is for temporary messages. You must create a widget and add it to the status bar
	//i.e. statusBar()->addWidget(new someWidget) for a normal or permanent message
	int Acq = GetGageSystem()->GetAcqCount();
	m_AcqCountLabel->setText(QString("%1 acquisitions").arg(Acq));

    if(seg > 1) m_CurrSegLabel->setText(QString("%1 segments").arg(seg));
    else m_CurrSegLabel->setText(QString("%1 segment").arg(seg));
}

void CsTestQt::s_UpdateSampRateStatus()
{
	// showMessage is for temporary messages. You must create a widget and add it to the status bar
	//i.e. statusBar()->addWidget(new someWidget) for a normal or permanent message
	int64 SampRate = m_pGageSystem->getSampleRate();
	if (SampRate >= 1000000000) m_SrLabel->setText(QString("%1 GS/s").arg(QString().setNum(m_pGageSystem->getSampleRate() / 1000000000.0, 'g', 3)));
	else if (SampRate >= 1000000) m_SrLabel->setText(QString("%1 MS/s").arg(QString().setNum(m_pGageSystem->getSampleRate() / 1000000.0, 'g', 3)));
	else if (SampRate >= 1000) m_SrLabel->setText(QString("%1 kS/s").arg(QString().setNum(m_pGageSystem->getSampleRate() / 1000.0, 'g', 3)));
	else m_SrLabel->setText(QString("%1 S/s").arg(QString().setNum((double)m_pGageSystem->getSampleRate(), 'g', 3)));
}

void CsTestQt::s_UpdateTxTime(QString strXferTime)
{
	// showMessage is for temporary messages. You must create a widget and add it to the status bar
	//i.e. statusBar()->addWidget(new someWidget) for a normal or permanent message
    m_XferTimeLabel->setText(strXferTime);
}

void CsTestQt::s_UpdateCommitTime(int CommitTime)
{
	// showMessage is for temporary messages. You must create a widget and add it to the status bar
	//i.e. statusBar()->addWidget(new someWidget) for a normal or permanent message
	double dblCommitTime = CommitTime / 1000.0;

	m_XferTimeLabel->setText(QString("CommitTime: %1secs").arg(QString().setNum(dblCommitTime, 'g', 3)));

	emit UpdateCommitDone();
}

void CsTestQt::s_UpdatePositionStatus(QString strPositionStatus)
{
	// showMessage is for temporary messages. You must create a widget and add it to the status bar
	//i.e. statusBar()->addWidget(new someWidget) for a normal or permanent message
    //qDebug() << "Update position status: " << strPositionStatus;
	m_PositionLabel->setText(strPositionStatus);
}

void CsTestQt::s_UpdateHoriz(int zoom)
{
	m_ZoomLabel->setText(QString("Horizontal: %1 Samples/Div").arg(zoom));
}

void CsTestQt::s_DisplayErrPopup()
{
	emit acquisitionThreadMustAbort();
	m_MsgBox->setText(m_ErrMng->GetErrMsg());
	m_MsgBox->exec();
	m_ErrMng->ClrErr();
	m_bCanAcquire = true;

//	emit loadParms(m_pGageSystem);
	ui.actionTBContinuous->setChecked(false);
	ui.actionContinuous->setChecked(false);
	if (m_ErrMng->bQuit) emit close();
	m_ErrMng->bQuit = false;
	m_pGageSystem->UpdateSystemInformation();
//	emit loadParms(m_pGageSystem);
}

void CsTestQt::s_DisplayMessage(QString Msg)
{
    emit acquisitionThreadMustAbort();
    m_MsgBox->setText(Msg);
    m_MsgBox->exec();
}

void CsTestQt::s_ActionAfterError(int Result)
{
    Q_UNUSED (Result);
    if (m_ErrMng->bQuit)
    {
        emit close();
    }
    m_ErrMng->bQuit = false;
    m_pGageSystem->UpdateSystemInformation();
 //   emit loadParms(m_pGageSystem);
}

void CsTestQt::s_UpdateUi()
{
    m_bProcessingEvents = true;
    //m_UiTimer->stop();

    //GetScope()->GetGraph()->AdjustZoom();
    //GetScope()->AdjustHScrollBar();

    m_AcqCountLabel->setText(QString("Acq: %1").arg(m_pGageSystem->GetAcqCount()));

    if(m_StatusBarInfo.m_u64StatBarCurrSeg==0) m_CurrSegLabel->setText(QString("Segment:"));
    else m_CurrSegLabel->setText(QString("Segment: %1").arg(m_StatusBarInfo.m_u64StatBarCurrSeg));

    QString strBrdStatus;
    switch(m_StatusBarInfo.m_BoardStatus)
    {
        case BoardReady:
            strBrdStatus = "Ready";
            break;

        case BoardStart:
            strBrdStatus = "Start";
            break;

        case BoardAcquire:
            strBrdStatus = "Acquire";
            break;

        case BoardWaitForTrigger:
            strBrdStatus = "Wait for trigger";
            break;

        case BoardCapture:
            strBrdStatus = "Capture";
            break;

        case BoardTransfer:
            strBrdStatus = "Transfer";
            break;

        case GraphUpdate:
            strBrdStatus = "Update";
            break;
    }

    m_BrdStatusLabel->setText(strBrdStatus);

    //qDebug() << "CsTestQt: Process events";
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    //m_UiTimer->start(50);
    m_bProcessingEvents = false;
}

void CsTestQt::s_ReadWriteRegs()
{
	m_pReadWriteRegs->show();
}

void CsTestQt::s_NiosSpiCmd()
{
	m_pNiosSpiCmd->show();
}

void CsTestQt::s_Mulrec()
{
	m_pMulrec->s_Show();
}

void CsTestQt::WriteWinPosSettings()
{
    #ifdef Q_OS_WIN
        QSettings qsettings( QSettings::UserScope, "Gage", "CsTestQt" );

        qsettings.beginGroup( "mainwindow" );

        qsettings.setValue( "XPos", geometry().x() );
        qsettings.setValue( "YPos", geometry().y() );
        qsettings.setValue( "Width", geometry().width() );
        qsettings.setValue( "Height", geometry().height() );

        qsettings.endGroup();
    #else
        QFile IniFile;
        bool bRes;

        IniFile.setFileName("CsTestQt.ini");

        bRes = IniFile.open(QIODevice::WriteOnly);
        if (!bRes)
        {
            return;
        }

        QString TempQString;
        TempQString.clear();

        TempQString = QString("Xpos:%1\r\n").arg(geometry().x());
        TempQString += QString("Ypos:%1\r\n").arg(geometry().y());
        TempQString += QString("Width:%1\r\n").arg(geometry().width());
        TempQString += QString("Height:%1\r\n").arg(geometry().height());

        IniFile.write(TempQString.toLocal8Bit());
        if (!bRes)
        {
            return;
        }
        IniFile.close();
    #endif
}

void CsTestQt::ReadWinPosSettings()
{
    m_bReadingWindowSettings = true;

    int XPos, YPos, ThisWidth, ThisHeight;

    #ifdef Q_OS_WIN
        QSettings qsettings( QSettings::UserScope, "Gage", "CsTestQt" );

        qsettings.beginGroup( "mainwindow" );

        if(qsettings.contains("XPos"))
        {
            XPos = qsettings.value("XPos").toInt();
            YPos = qsettings.value("YPos").toInt();
            ThisWidth = qsettings.value("Width").toInt();
            ThisHeight = qsettings.value("Height").toInt();

            setGeometry(XPos, YPos, ThisWidth, ThisHeight);
        }

        qsettings.endGroup();
    #else
        qDebug() << "CsTestQt: ReadPosSettings";

        m_bReadingWindowSettings = true;

        QFile IniFile;
        bool bRes;
        QString QStrTemp1, QStrTemp2;
        QByteArray QByteArrTemp;
        int idx1, idx2, SubLen;

        IniFile.setFileName("CsTestQt.ini");

        if(!IniFile.exists())
        {
            return;
        }

        bRes = IniFile.open(QIODevice::ReadOnly);
        if (!bRes)
        {
            return;
        }

        QByteArrTemp = IniFile.readAll();
        QStrTemp1.clear();
        QStrTemp1.append(QByteArrTemp);
        idx1 = QStrTemp1.indexOf("Xpos:");
        idx2 = QStrTemp1.indexOf("\r\n");
        SubLen = idx2 - idx1 - 5;
        QStrTemp2 = QStrTemp1.mid(idx1 + 5, SubLen);
        XPos = QStrTemp2.toInt();
        QStrTemp1 = QStrTemp1.mid(idx2+2, -1);

        idx1 = QStrTemp1.indexOf("Ypos:");
        idx2 = QStrTemp1.indexOf("\r\n");
        SubLen = idx2 - idx1 - 5;
        QStrTemp2 = QStrTemp1.mid(idx1 + 5, SubLen);
        YPos = QStrTemp2.toInt();
        QStrTemp1 = QStrTemp1.mid(idx2+2, -1);

        idx1 = QStrTemp1.indexOf("Width:");
        idx2 = QStrTemp1.indexOf("\r\n");
        SubLen = idx2 - idx1 - 6;
        QStrTemp2 = QStrTemp1.mid(idx1 + 6, SubLen);
        ThisWidth = QStrTemp2.toInt();
        QStrTemp1 = QStrTemp1.mid(idx2+2, -1);

        idx1 = QStrTemp1.indexOf("Height:");
        idx2 = QStrTemp1.indexOf("\r\n");
        SubLen = idx2 - idx1 - 7;
        QStrTemp2 = QStrTemp1.mid(idx1 + 7, SubLen);
        ThisHeight = QStrTemp2.toInt();

        setGeometry(XPos, YPos, ThisWidth, ThisHeight);

        IniFile.close();

        m_bReadingWindowSettings = false;
    #endif

    m_bReadingWindowSettings = false;
}

void CsTestQt::moveEvent(QMoveEvent * eventMove)
{
    UNREFERENCED_PARAMETER(eventMove);
    //qDebug() << "CsTestQt: Move event";

    if(m_bReadingWindowSettings) return;
    WriteWinPosSettings();
}

void CsTestQt::resizeEvent(QResizeEvent *eventResize)
{
    UNREFERENCED_PARAMETER(eventResize);
    //qDebug() << "CsTestQt: Resize event";

    if(m_bReadingWindowSettings) return;
    WriteWinPosSettings();
}

void CsTestQt::closeEvent(QCloseEvent *eventClose)
{
    //qDebug() << "CsTestQt: Close event";
    UNREFERENCED_PARAMETER(eventClose);
    WriteWinPosSettings();
}

void CsTestQt::s_ChanProtFault()
{
    //QMessageBox MsgBox;
    m_MsgBox->setText("Channel protection fault. Acquisition stopped.");
    m_MsgBox->show();
    m_MsgBox->setModal(true);

    ui.actionContinuous->blockSignals(true);
    ui.actionTBContinuous->blockSignals(true);

    ui.actionContinuous->setChecked(false);
    ui.actionTBContinuous->setChecked(false);

    ui.actionContinuous->blockSignals(false);
    ui.actionTBContinuous->blockSignals(false);
}


void CsTestQt::EnableMenuControls(bool bIdleState)
{
	// Menu items
	ui.actionSelect_System->setEnabled((m_i32BrdCount > 1) && bIdleState);
	ui.actionSystem_Reset->setEnabled(bIdleState);
	ui.actionAddon_Reset->setEnabled(bIdleState);
	ui.actionConfiguration->setEnabled(bIdleState);
	ui.actionXfer_Mem->setEnabled(bIdleState);
	ui.actionAcquire->setEnabled(bIdleState);
//	ui.actionContinuous->setEnabled(bIdleState);
	ui.actionForce_Trigger->setEnabled(!bIdleState);
	ui.actionAbort->setEnabled(!bIdleState);

	// Shortcuts
	ui.actionTBSystem_Reset->setEnabled(bIdleState);
	ui.actionTBAddOn_Reset->setEnabled(bIdleState);
	ui.actionTBXfer_Mem->setEnabled(bIdleState);
	ui.actionTBAcquire->setEnabled(bIdleState);
//	ui.actionTBContinuous->setEnabled(bIdleState);
	ui.actionTBForce_Trigger->setEnabled(!bIdleState);
	ui.actionTBAbort->setEnabled(!bIdleState);
}