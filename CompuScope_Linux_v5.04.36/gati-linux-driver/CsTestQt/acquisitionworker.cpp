#include "acquisitionworker.h"
#include <QtDebug>
#include <QMessageBox>
#include "qapplication.h"
#include <QTime>
#include "CsTestFunc.h"
#include <QDebug>

////qDebug() << "sample comment";

using namespace CsTestFunc;


AcquisitionWorker::AcquisitionWorker(CGageSystem* pGageSystem, CsTestErrMng* ErrMng, STATUS_BAR_INFO* pStatusBarInfo, bool* bUpdateScopeParmsDone, bool* bAcqRunning, bool* bAbortAcq):
	m_bContinuous(false), m_acqConfig()
{
	m_pGageSystem = pGageSystem;
	m_pErrMng = ErrMng;
    m_pStatusBarInfo = pStatusBarInfo;
	m_bClrGraph = false;
	m_bContinue = false;
    m_bUpdateScopeParmsDone = bUpdateScopeParmsDone;
    *m_bUpdateScopeParmsDone = false;
    m_bAcqRunning = bAcqRunning;
    *m_bAcqRunning = false;
    m_bAbortAcq = bAbortAcq;
    *m_bAbortAcq = false;
    m_bUpdateGraph = true;
    m_bProcessingEvents = false;
    m_bOverdraw = false;
    m_bAbort = false;
	//QueryPerformanceFrequency(&m_TickFrequency);
    m_AcqStatus = ACQ_IDLE;

    m_pMutexAcq = new QMutex(QMutex::NonRecursive);
    m_pMutexSamples = new QMutex(QMutex::NonRecursive);

	m_bDispAllSegs = true;
}

AcquisitionWorker::~AcquisitionWorker()
{
    delete m_pMutexAcq;
    delete m_pMutexSamples;
}

void AcquisitionWorker::s_SetClearGraph(bool bClrGraph)
{
    m_pMutexAcq->lock();
	m_bClrGraph = bClrGraph;
    m_pMutexAcq->unlock();
}


void AcquisitionWorker::s_DoAcquisitionFromBoard()
{
	//qDebug() << "s_DoAcquisitionFromBoard";
    m_MutexSharedVars->lock();
    *m_bAcqRunning = true;
    *m_bAbortAcq = false;
    m_MutexSharedVars->unlock();
	int32 i32Ret = 0;
	m_u32AcqCount = 0;
	m_pGageSystem->ClearAcqCount();
	m_pGageSystem->ClearAbort();

	//Everything is supposed to be already set and committed (Go to ConfigDialog)
	m_acqConfig = m_pGageSystem->getAcquisitionConfig();
    m_u32StartChan = m_pGageSystem->GetStartChannel();
	m_u32ChanCount = m_pGageSystem->GetActiveChannelCount();
	m_u32ChannelSkip = m_pGageSystem->GetChannelSkip();
	if (m_u32SegCount != m_pGageSystem->getSegmentCount()) m_bDispAllSegs = true;
	m_u32SegCount = m_pGageSystem->getSegmentCount();
	m_bInterleavedData = m_pGageSystem->m_bInterleavedData;
	AdjustStartStopSegs();

    if(*m_bParmsChanged)
    {
        qDebug() << "emit update scope parms";
        *m_bUpdateScopeParmsDone = false;
        emit UpdateScopeParms();
        qDebug() << "WaitUpdateScopePArms";
        WaitUpdateScopeParmsDone();
        qDebug() << "update scope parms done";
        *m_bParmsChanged = false;
        if(!m_bOverdraw)
        {
            qDebug() << "emit clear graph";
            *m_bScopeInitDone = false;
            emit clearGraph();
            WaitScopeInit();
            qDebug() << "clear graph done";
            m_pStatusBarInfo->m_u64StatBarCurrSeg = 0;
        }
    }

    if(!m_bOverdraw)
    {
        qDebug() << "emit clear graph";
        *m_bScopeInitDone = false;
        emit clearGraph();
        WaitScopeInit();
        qDebug() << "clear graph done";
    }

    do
	{
        //qDebug() << "Start";
        m_AcqStatus = ACQ_INIT;
        m_pStatusBarInfo->m_BoardStatus = BoardStart;
#if 0
		// Check if there a  new configutations request pending
		if ( m_pGageSystem->NewConfigPending() )
		{
			WaitScopeUpdate();
			m_pGageSystem->SetBoardBusyAcquisition(false);
			// Signal the board is now in the idle state
			m_pGageSystem->m_WaitBoardIdle.wakeOne();

			// Waiting for configutations changed from Config Dialog
			m_pGageSystem->WaitForConfigChanged();
			if (m_pErrMng->GetLastErrCode())
			{
				emit finished();
				m_pStatusBarInfo->m_BoardStatus = BoardReady;
				*m_bAcqRunning = false;
				emit Error();
				return;
			}
		}
#endif
	
        //qDebug() << "Check parms changed";
        if (*m_bParmsChanged)
		{
            qDebug() << "parms changed";
            m_MutexSharedVars->lock();
            *m_bParmsChanged = false;
            m_MutexSharedVars->unlock();
			m_bClrGraph = true;
            *m_bUpdateScopeParmsDone = false;
            emit UpdateScopeParms();
            WaitUpdateScopeParmsDone();
			m_u32StartChan = m_pGageSystem->GetStartChannel();
			m_u32ChanCount = m_pGageSystem->GetActiveChannelCount();
			if (m_u32SegCount != m_pGageSystem->getSegmentCount()) m_bDispAllSegs = true;
			m_u32SegCount = m_pGageSystem->getSegmentCount();
			m_u32ChannelSkip = m_pGageSystem->GetChannelSkip();
			AdjustStartStopSegs();
            qDebug() << "parms changed done";
		}

        //qDebug() << "Check clear graph";
		if(m_bClrGraph)
		{
            qDebug() << "clear graph";
			//m_pGageSystem->ClearAcqCount();
            m_pMutexAcq->lock();
			m_bClrGraph = false;
            m_pMutexAcq->unlock();
            *m_bScopeInitDone = false;
            emit clearGraph();
            WaitScopeInit();
            qDebug() << "clear graph done";
            m_pStatusBarInfo->m_u64StatBarAcqCount = 0;
            m_pStatusBarInfo->m_u64StatBarCurrSeg = 0;
		}

        //qDebug() << "Acquire";
		if(m_u32SegCount==1) i32Ret = AcquireSingleSeg();
		else i32Ret = AcquireMultiSeg();

        //qDebug() << "Acquire :Done";
		if (CS_FAILED(i32Ret))
		{
            qDebug() << "Acquire returned error";

			emit finished();
            m_pStatusBarInfo->m_BoardStatus = BoardReady;
            *m_bAcqRunning = false;
			if (i32Ret==CS_OPERATION_ABORTED)
			{
                qDebug() << "Acquire returned operation aborted";
                m_pErrMng->ClrErr();
                return;
			}
			else
			{
				m_pErrMng->UpdateErrMsg("Error occured during acquisition", i32Ret);
				emit Error();
				return;
			}
		}

		//Check if we must acquire again
		if (!m_bContinuous)
			break;
	}while(1);

    qDebug() << "Acquisition continuous loop done";

	emit finished();
    m_pStatusBarInfo->m_BoardStatus = BoardReady;
    m_AcqStatus = ACQ_IDLE;
    m_pStatusBarInfo->m_u64StatBarSegCount = m_pGageSystem->getSegmentCount();
    m_pStatusBarInfo->m_dblStatBarXferTimeMsec = (double)m_u64XferTime;
    *m_bAcqRunning = false;
	m_bAbort = false;

    qDebug() << "DoAcquisition: Done";
}

void AcquisitionWorker::s_XferMem()
{
    m_AcqStatus = WAIT_SCOPE_UPDATE;
    m_MutexSharedVars->lock();
    *m_bAbortAcq = false;
	QElapsedTimer XferTimer;
	m_u64XferTime = 0;
	int SegClear = 0;
	uInt32 seg;
    m_MutexSharedVars->unlock();

	//Everything is supposed to be already set and committed (Go to ConfigDialog)
	m_acqConfig = m_pGageSystem->getAcquisitionConfig();
    m_u32StartChan = m_pGageSystem->GetStartChannel();
	m_u32ChanCount = m_pGageSystem->GetActiveChannelCount();
	m_u32SegCount = m_pGageSystem->getSegmentCount();
	m_u32ChannelSkip = m_pGageSystem->GetChannelSkip();

    AdjustStartStopSegs();

    m_pStatusBarInfo->m_BoardStatus = BoardTransfer;

    *m_bScopeInitDone = false;
    emit clearGraph();
    WaitScopeInit();

	XferTimer.restart();
	for ( seg = m_i32DispStartSeg; seg <= m_i32DispStopSeg; seg++)
	{
		m_u64XferTime = XferTimer.nsecsElapsed();
        m_pStatusBarInfo->m_dblStatBarXferTimeMsec = (double)m_u64XferTime;

		if (m_bClrGraph)
		{
			m_bClrGraph = false;
			SegClear = seg;
            *m_bScopeInitDone = false;
            emit clearGraph();
            WaitScopeInit();
            m_pStatusBarInfo->m_u64StatBarAcqCount = 0;
            m_pStatusBarInfo->m_u64StatBarCurrSeg = 0;
		}

        m_MutexSharedVars->lock();
        *m_bScopeUpdateDone = false;
        m_MutexSharedVars->unlock();
        *m_i64CurrSeg = seg;
        emit dataReady();
        WaitScopeUpdate();

        m_pStatusBarInfo->m_u64StatBarCurrSeg = seg;
        m_pStatusBarInfo->m_u64StatBarSegCount = seg - SegClear;
	}

    //emit updateTxTime((double)m_u64XferTime);
	emit finished();
    m_pStatusBarInfo->m_BoardStatus = BoardReady;
    m_AcqStatus = ACQ_IDLE;
}

int32 AcquisitionWorker::AcquireData()
{
	int32 i32Ret;

	m_pStatusBarInfo->m_BoardStatus = BoardAcquire;
	i32Ret = m_pGageSystem->DoAcquisition();
	if (CS_FAILED(i32Ret))
	{
		m_pErrMng->UpdateErrMsg("Error occured during acquisition", i32Ret);
		emit finished();
		emit Error();
		m_pStatusBarInfo->m_BoardStatus = BoardReady;
		return i32Ret;
	}

	m_AcqStatus = ACQ_WAIT_TRIGGER;
	m_pStatusBarInfo->m_BoardStatus = BoardWaitForTrigger;
	i32Ret = m_pGageSystem->WaitForTrigger();
	
	if (CS_FAILED(i32Ret))
	{
		m_pErrMng->UpdateErrMsg("Error occured waiting for trigger", i32Ret);
		return i32Ret;
	}
	if (m_bAbort)
	{
		m_bAbort = false;
		return CS_OPERATION_ABORTED;
	}

	m_AcqStatus = ACQ_WAIT_COMPLETE;
	m_pStatusBarInfo->m_BoardStatus = BoardCapture;
	i32Ret = m_pGageSystem->DataCaptureComplete();
	
	if (CS_FAILED(i32Ret))
	{
		m_pErrMng->UpdateErrMsg("Error occured during capture", i32Ret);
		return i32Ret;
	}

	m_pGageSystem->IncAcqCount();
	m_u32AcqCount += 1;

	return i32Ret;
}

int32 AcquisitionWorker::AcquireSingleSeg()
{
    int32 i32Ret;
    QElapsedTimer XferTimer;
    m_u64XferTime = 0;

	i32Ret = AcquireData();
    if (CS_FAILED(i32Ret))
		return i32Ret;

    m_pStatusBarInfo->m_BoardStatus = BoardTransfer;
    XferTimer.restart();

    m_u64XferTime = XferTimer.nsecsElapsed();

    if (m_bAbort)
    {
        m_bAbort = false;
        return CS_OPERATION_ABORTED;
    }

	m_AcqStatus = WAIT_SCOPE_UPDATE;
    m_MutexSharedVars->lock();
    *m_bScopeUpdateDone = false;
    m_MutexSharedVars->unlock();
    *m_i64CurrSeg = 1;
    emit dataReady();
    WaitScopeUpdate();
    m_pStatusBarInfo->m_u64StatBarCurrSeg = 1;
    m_pStatusBarInfo->m_u64StatBarSegCount = 1;

    if(m_pGageSystem->GetAcqCount()==0)
    {
        m_pStatusBarInfo->m_dblStatBarXferTimeMsec = (double)m_u64XferTime;
    }

    return i32Ret;
}

int32 AcquisitionWorker::AcquireMultiSeg()
{
    int32 i32Ret;
    QElapsedTimer XferTimer;
    m_u64XferTime = 0;
    int SegClear = 0;

	m_pStatusBarInfo->m_BoardStatus = BoardAcquire;
    i32Ret = AcquireData();
    if (CS_FAILED(i32Ret))
		return i32Ret;

    m_AcqStatus = WAIT_SCOPE_UPDATE;
    m_pStatusBarInfo->m_BoardStatus = BoardTransfer;
    XferTimer.restart();
 
	if(*m_bDisplayOneSeg)
    {
        m_u64XferTime = XferTimer.nsecsElapsed();
        m_MutexSharedVars->lock();
        *m_bScopeUpdateDone = false;
        m_MutexSharedVars->unlock();
        emit dataReady();
        WaitScopeUpdate();
        m_pStatusBarInfo->m_u64StatBarCurrSeg = *m_i64CurrSeg;
        m_pStatusBarInfo->m_u64StatBarSegCount = 1;
        if (m_bAbort)
        {
            m_bAbort = false;
            return CS_OPERATION_ABORTED;
        }
    }
    else
    {
        for (uInt32 seg = m_i32DispStartSeg; seg <= m_i32DispStopSeg; seg++)
        {
            m_u64XferTime = XferTimer.nsecsElapsed();
   
            if (m_i32DispStopSeg == 1)
            {

                if (m_bUpdateGraph || (m_u32AcqCount == 1))
                {
                    m_pStatusBarInfo->m_BoardStatus = GraphUpdate;
                    m_pStatusBarInfo->m_u64StatBarSegCount = seg - SegClear;
                    m_pStatusBarInfo->m_dblStatBarXferTimeMsec = (double)m_u64XferTime;

                    m_MutexSharedVars->lock();
                    *m_bScopeUpdateDone = false;
                    m_MutexSharedVars->unlock();
                    *m_i64CurrSeg = seg;
                    emit dataReady();
                    WaitScopeUpdate();

                    m_pMutexAcq->lock();
                    m_bUpdateGraph = false;
                    m_pMutexAcq->unlock();
                }
            }
            else
            {
                if (m_bClrGraph)
                {
                    m_pMutexAcq->lock();
                    m_bClrGraph = false;
                    SegClear = seg;
                    m_pMutexAcq->unlock();

                    *m_bScopeInitDone = false;
                    emit clearGraph();
                    WaitScopeInit();

                    m_pStatusBarInfo->m_u64StatBarCurrSeg = 0;
                    m_pStatusBarInfo->m_u64StatBarSegCount = 0;                
                }
     
                m_MutexSharedVars->lock();
                *m_bScopeUpdateDone = false;
                m_MutexSharedVars->unlock();
                *m_i64CurrSeg = seg;
                emit dataReady();
                WaitScopeUpdate();
                m_pStatusBarInfo->m_u64StatBarSegCount = seg - SegClear;
            }
            if (m_bAbort)
            {
                m_bAbort = false;
                return CS_OPERATION_ABORTED;
            }
            m_pStatusBarInfo->m_u64StatBarCurrSeg = seg;
        }
    }

    if(m_pGageSystem->GetAcqCount()==0)
    {
        m_pStatusBarInfo->m_dblStatBarXferTimeMsec = (double)m_u64XferTime;
    }

    return i32Ret;
}

int32 AcquisitionWorker::AcquireInterSingleSeg()
{
    int32 i32Ret;
    int XferOffset = 0;
    QElapsedTimer XferTimer;
    m_u64XferTime = 0;

    i32Ret = AcquireData();
    if (CS_FAILED(i32Ret))
		return i32Ret;
 
	m_AcqStatus = WAIT_SCOPE_UPDATE;
    m_pStatusBarInfo->m_BoardStatus = BoardTransfer;
    XferTimer.restart();
 
    i32Ret = m_pGageSystem->TransferDataInter(*m_i64CurrSeg, m_pGageSystem->getBuffer(0), &XferOffset);
    if (CS_FAILED(i32Ret))
    {
        m_pErrMng->UpdateErrMsg("Error occured during data transfer.", i32Ret);
        return i32Ret;
    }
    for (uInt32 i = 0, j = 0; i < m_u32ChanCount; i++, j++)
    {
        m_pGageSystem->setXferOffset(j, XferOffset);
    }

    m_u64XferTime = XferTimer.nsecsElapsed();

    if (m_bClrGraph)
    {
        m_pMutexAcq->lock();
        m_bClrGraph = false;
        m_pMutexAcq->unlock();

        *m_bScopeInitDone = false;
        emit clearGraph();
        WaitScopeInit();

        m_pStatusBarInfo->m_u64StatBarCurrSeg = 1;
        m_pStatusBarInfo->m_u64StatBarSegCount = 0;
    }

    m_MutexSharedVars->lock();
    *m_bScopeUpdateDone = false;
    m_MutexSharedVars->unlock();
    emit dataReady();
    WaitScopeUpdate();
    m_pStatusBarInfo->m_u64StatBarSegCount = 1;

    if(m_pGageSystem->GetAcqCount()==0)
    {
        m_pStatusBarInfo->m_dblStatBarXferTimeMsec = (double)m_u64XferTime;
    }

    return i32Ret;
}

int32 AcquisitionWorker::AcquireInterMultiSeg()
{
    int32 i32Ret;
    int XferOffset = 0;
    QElapsedTimer XferTimer;
    m_u64XferTime = 0;
    int SegClear = 0;

	i32Ret = AcquireData();
	if (CS_FAILED(i32Ret))
		return i32Ret;

    m_AcqStatus = WAIT_SCOPE_UPDATE;
    m_pStatusBarInfo->m_BoardStatus = BoardTransfer;
    XferTimer.restart();

    for (uInt32 seg = m_i32DispStartSeg; seg <= m_i32DispStopSeg; seg++)
    {
        for (uInt32 i = 0, j = 0; i < m_u32ChanCount; i++, j++)
        {
            m_pMutexSamples->lock();
            i32Ret = m_pGageSystem->TransferData((i*m_u32ChannelSkip) + m_u32StartChan, seg, m_pGageSystem->getBuffer(j), &XferOffset);
            m_pMutexSamples->unlock();
 
            if (CS_FAILED(i32Ret))
            {
                m_pErrMng->UpdateErrMsg("Error occured during data transfer.", i32Ret);
                return i32Ret;
            }
            m_pGageSystem->setXferOffset(j, XferOffset);
        }

        m_u64XferTime = XferTimer.nsecsElapsed();
        if (m_i32DispStopSeg == 1)
        {
            if (m_bUpdateGraph || (m_u32AcqCount == 1))
            {
                m_pStatusBarInfo->m_BoardStatus = GraphUpdate;
                m_pStatusBarInfo->m_u64StatBarSegCount = seg - SegClear;
                m_pStatusBarInfo->m_dblStatBarXferTimeMsec = (double)m_u64XferTime;

                m_MutexSharedVars->lock();
                *m_bScopeUpdateDone = false;
                m_MutexSharedVars->unlock();
                emit dataReady();
                WaitScopeUpdate();

                m_pMutexAcq->lock();
                m_bUpdateGraph = false;
                m_pMutexAcq->unlock();
            }
        }
        else
        {
            if (m_bClrGraph)
            {
                m_pMutexAcq->lock();
                m_bClrGraph = false;
                SegClear = seg;
                m_pMutexAcq->unlock();

                *m_bScopeInitDone = false;
                emit clearGraph();
                WaitScopeInit();

                m_pStatusBarInfo->m_u64StatBarSegCount = 0;
                m_MutexSharedVars->lock();
                *m_bScopeUpdateDone = false;
                m_MutexSharedVars->unlock();
                emit dataReady();
                WaitScopeUpdate();
            }

            if (seg == 1)
            {
                m_MutexSharedVars->lock();
                *m_bScopeUpdateDone = false;
                m_MutexSharedVars->unlock();
                emit dataReady();
                WaitScopeUpdate();
                m_pStatusBarInfo->m_u64StatBarSegCount = seg - SegClear;
            }
            else if (seg == m_i32DispStopSeg)
            {
                m_MutexSharedVars->lock();
                *m_bScopeUpdateDone = false;
                m_MutexSharedVars->unlock();
                emit dataReady();
                WaitScopeUpdate();
                m_pStatusBarInfo->m_u64StatBarSegCount = seg - SegClear;
            }
            else
            {
                m_MutexSharedVars->lock();
                *m_bScopeUpdateDone = false;
                m_MutexSharedVars->unlock();
                emit dataReady();
                WaitScopeUpdate();
                m_pStatusBarInfo->m_u64StatBarSegCount = seg - SegClear;
            }
        }
        m_pStatusBarInfo->m_u64StatBarCurrSeg = seg;
    }

    if(m_pGageSystem->GetAcqCount()==0)
    {
        m_pStatusBarInfo->m_dblStatBarXferTimeMsec = (double)m_u64XferTime;
    }


    return i32Ret;
}

void AcquisitionWorker::s_DoStop()
{
    //qDebug() << "AcqWorker: s_DoAbort";
    m_bContinuous = false;
    //qDebug() << "AcqWorker: ************Emit abort Cs op************";
    //emit AbortCsOp();
    //qDebug() << "AcqWorker: s_DoAbort done";
}

void AcquisitionWorker::s_SetContinuous(bool bContinuous)
{
    m_pMutexAcq->lock();
	m_bContinuous = bContinuous;
    m_pMutexAcq->unlock();
}

void AcquisitionWorker::s_Abort()
{
    if (*m_bAcqRunning == true)
    {
        m_bAbort = true;

        switch(m_AcqStatus)
        {
            case ACQ_INIT:
                break;

            case ACQ_WAIT_TRIGGER:
            case ACQ_WAIT_COMPLETE:
                m_pGageSystem->SetAbort();
                break;

            case WAIT_SCOPE_UPDATE:
                emit AbortScopeUpdate();
                break;

            default:
                break;
        }
    }
}


void AcquisitionWorker::s_ProcessEvents()
{
    if(m_bProcessingEvents) return;
    m_bProcessingEvents = true;
    //qDebug() << "AcqWorker: Process events";
    QCoreApplication::processEvents();
    m_pMutexAcq->lock();
    m_bUpdateGraph = true;
    m_pMutexAcq->unlock();
    m_bProcessingEvents = false;
}


void AcquisitionWorker::AdjustStartStopSegs()
{
	if (m_bDispAllSegs)
	{
		m_i32DispStartSeg = 1;
		m_i32DispStopSeg = m_pGageSystem->getSegmentCount();
	}
	else
	{
		if (m_i32DispStopSeg > m_pGageSystem->getSegmentCount()) m_i32DispStopSeg = m_pGageSystem->getSegmentCount();
		if (m_i32DispStartSeg > m_i32DispStopSeg) m_i32DispStartSeg = m_i32DispStopSeg;
	}
}

void AcquisitionWorker::WaitScopeInit()
{
    //unsigned long long count = 0;

    while(!(*m_bScopeInitDone))
    {
        //if((count % 3000)==0) QCoreApplication::processEvents();
        quSleep(10);
    }
}

void AcquisitionWorker::WaitScopeUpdate()
{
    //unsigned long long count = 0;

    while(!(*m_bScopeUpdateDone))
    {
        //if((count % 3000)==0) QCoreApplication::processEvents();
        quSleep(10);
    }
}

void AcquisitionWorker::WaitUpdateScopeParmsDone()
{
    //unsigned long long count = 0;

    while(*m_bUpdateScopeParmsDone==false)
    {
        qDebug() << "Wait update scope parms";
        quSleep(10000);
        QCoreApplication::processEvents();
    }
}

void AcquisitionWorker::SetAddrScopeInitDone(bool* bScopeInitDone)
{
    m_bScopeInitDone = bScopeInitDone;
}

void AcquisitionWorker::SetAddrScopeUpdateDone(bool* bScopeUpdateDone)
{
    m_bScopeUpdateDone = bScopeUpdateDone;
}

void AcquisitionWorker::SetAddrDisplayOneSeg(bool* bDisplayOneSeg)
{
    m_bDisplayOneSeg = bDisplayOneSeg;
}

void AcquisitionWorker::SetAddrCurrSeg(int64* i64CurrSeg)
{
    m_i64CurrSeg = i64CurrSeg;
}

void AcquisitionWorker::SetAddrParmsChanged(bool* bParmsChanged)
{
    m_bParmsChanged = bParmsChanged;
}

void AcquisitionWorker::SetAddrMutexSharedVars(QMutex* MutexSharedVars)
{
    m_MutexSharedVars = MutexSharedVars;
}

void AcquisitionWorker::s_SetOverdraw(bool bOverdraw)
{
    m_bOverdraw = bOverdraw;
}

bool AcquisitionWorker::ParmsChanged()
{
    bool bParmsChanged = *m_bParmsChanged;
    return bParmsChanged;
}

