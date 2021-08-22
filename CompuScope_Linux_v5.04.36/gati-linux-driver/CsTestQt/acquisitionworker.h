#ifndef ACQUISITIONWORKER_H
#define ACQUISITIONWORKER_H

#include <QObject>
#include <QMutex>
#include <vector>

#include "GageSystem.h"
#include "CsTestFunc.h"
#include <qeventloop.h>
#include <qtimer.h>
#include "cstestqttypes.h"

class AcquisitionWorker : public QObject
{
	Q_OBJECT

public:
    AcquisitionWorker(CGageSystem* pGageSystem, CsTestErrMng* ErrMng, STATUS_BAR_INFO* pStatusBarInfo, bool* bUpdateScopeParmsDone, bool* bAcqRunning, bool* bAbortAcq);
	~AcquisitionWorker();

    typedef enum    _ACQ_STATUS
    {
        ACQ_IDLE, ACQ_INIT, ACQ_WAIT_TRIGGER, ACQ_WAIT_COMPLETE, WAIT_SCOPE_UPDATE
    } ACQ_STATUS;

	/********* Mutex for multi threading ****************/
    QMutex*			m_pMutexAcq;
    QMutex*			m_pMutexSamples;

public:
    bool*			m_bAcqRunning;
    bool*			m_bAbortAcq;
    bool*			m_bParmsChanged;
	bool			m_bInterleavedData;
	bool			m_bContinue;
    bool*           m_bScopeInitDone;
    bool*           m_bScopeUpdateDone;
    bool*           m_bUpdateScopeParmsDone;
    unsigned long long  m_u64XferTime;
    STATUS_BAR_INFO*    m_pStatusBarInfo;
    bool*           m_bDisplayOneSeg;
    int64*          m_i64CurrSeg;
    QMutex*         m_MutexSharedVars;

	QElapsedTimer		m_sleepTimer;


	bool	IsAcqRunning();
    bool	IsWaiting() { return m_bWaiting; }
    bool	ParmsChanged();

    void	setAllSegsMode(bool bAllSegs) { m_bDispAllSegs = bAllSegs; }
    bool	getAllSegsMode() { return m_bDispAllSegs; }
    void	setDispStartSeg(int32 StartSeg) { m_i32DispStartSeg = StartSeg; }
    int32	getDispStartSeg() { return m_i32DispStartSeg; }
    void	setDispStopSeg(int32 StopSeg) { m_i32DispStopSeg = StopSeg; }
    int32	getDispStopSeg() { return m_i32DispStopSeg; }

    void    SetAddrScopeInitDone(bool* bScopeInitDone);
    void    SetAddrScopeUpdateDone(bool* bScopeUpdateDone);
    void    SetAddrMutexAcqRunWait(QMutex* pMutexAcqRunWait);
    void    SetAddrDisplayOneSeg(bool* bDisplayOneSeg);
    void    SetAddrCurrSeg(int64* i64CurrSeg);
    void    SetAddrParmsChanged(bool* bParmsChanged);
    void    SetAddrMutexSharedVars(QMutex* MutexSharedVars);

public slots:
	void	s_DoAcquisitionFromBoard();
    void    s_DoStop();
	void	s_XferMem();
	void	s_SetContinuous(bool bContinuous);
	void	s_SetClearGraph(bool bClrGraph);
    void	s_Abort();
	void	s_ProcessEvents();
    void    s_SetOverdraw(bool bOverdraw);

signals:
    void	dataReady();
	void	finished();
	void	Error();
	void	clearGraph();
	void	UpdateScopeParms();
	void	AbortCsOp();
	void	loadParms(CGageSystem*);
    void    AbortScopeUpdate();

private:
    ACQ_STATUS          m_AcqStatus;
	int32               AcquireData();
	int32               AcquireSingleSeg();
    int32               AcquireMultiSeg();
    int32               AcquireInterSingleSeg();
    int32               AcquireInterMultiSeg();
    bool                m_bAbort;

	CGageSystem*		m_pGageSystem;
	CsTestErrMng*		m_pErrMng;
	bool				m_bContinuous;
	bool				m_bClrGraph;
	bool				m_bWaiting;
    bool				m_bProcessingEvents;
    bool				m_bUpdateGraph;
    bool                m_bOverdraw;
	void				AdjustStartStopSegs();
    void                WaitScopeInit();
    void                WaitScopeUpdate();
    void                WaitUpdateScopeParmsDone();

	/**** Attributes used to manage acquisition *********/
	CSACQUISITIONCONFIG		m_acqConfig;
	uInt32				m_u32StartChan;
	uInt32				m_u32ChanCount;
	uInt32				m_u32SegCount;
	uInt32				m_u32StartSeg;
	uInt32				m_u32StopSeg;
	uInt32				m_u32ChannelSkip;
	uInt32				m_u32AcqCount;
    LARGE_INTEGER       m_XferStart, m_XferEnd, m_XferDuration, m_TickFrequency;
	bool				m_bDispAllSegs;
    unsigned int		m_i32DispStartSeg;
    unsigned int		m_i32DispStopSeg;
};

#endif // ACQUISITIONWORKER_H
