#include "qcsgraph.h"
#include "qthread.h"
#include "qmath.h"
#include "qapplication.h"
#include "CsTestFunc.h"
#include <QDebug>


using namespace CsTestFunc;


QCsGraph::QCsGraph(QWidget *parent)
    : QWidget(parent), m_ZoomIn(1), m_ZoomOut(1), m_u16ZoomY(1), m_u64SampleCount(0), m_i64ViewPosition(0)
{
	m_bPainting = false;
	m_bUpdating = false;
	m_bInitGraph = false;
	m_bUpdated = false;
	m_bCleared = false;
	m_bInterleavedData = false;
	m_bDataFromFile = false;
	m_bZoomXChanged = false;
    m_bUpdatePlot = false;
    m_bClearTrace = true;
    m_bOverdraw = false;
    m_GraphUpdateType = GraphInit;
    m_pDataFileBuffer = NULL;
    m_bAbort = false;
    m_bAcquiring = false;

	m_UpdateWaitLoop = new QEventLoop(this);
	m_UpdateWaitLoop->blockSignals(false);
	m_UpdateWaitLoop->processEvents(QEventLoop::AllEvents);

	connect(this, SIGNAL(ScopeUpdateDone()), m_UpdateWaitLoop, SLOT(quit()), Qt::QueuedConnection);

	m_penCenterLine.setColor(Qt::red);
	m_penCenterLine.setStyle(Qt::DashLine);
	m_penCenterLine.setWidth(1);

	m_penGridLine.setColor(Qt::green);
	m_penGridLine.setStyle(Qt::DashDotLine);

	m_penCursor.setWidth(1);
	m_penCursor.setColor(Qt::blue);
	m_penCursor.setStyle(Qt::DashLine);

	m_penTrigAxis.setWidth(1);
	m_penTrigAxis.setColor(Qt::red);
	m_penTrigAxis.setStyle(Qt::DashLine);

    m_bConnectPoints = true;
	m_bGageDataValid = false;
	m_bCursor = false;
	m_bUpdated = false;

	m_NewZoomIn = m_ZoomIn;
	m_NewZoomOut = m_ZoomOut;
	m_u16NewZoomY = m_u16ZoomY;
	m_i64NewViewPosition = m_i64ViewPosition;

	m_LastDigiZoomIn = m_ZoomIn;
	m_LastFileZoomIn = m_ZoomIn;
	m_LastDigiZoomOut = m_ZoomOut;
	m_LastFileZoomOut = m_ZoomOut;
	m_u16LastDigiZoomY = m_u16ZoomY;
	m_u16LastFileZoomY = m_u16ZoomY;
	m_i64LastDigiViewPosition = m_i64ViewPosition;
	m_i64LastFileViewPosition = m_i64ViewPosition;

	m_i32Height = geometry().height() - 1; //Reduce the height or we can't see the bottom line
	m_i32Width = geometry().width();
	m_i32CenterX = m_i32Width / 2;
	m_i32CenterY = m_i32Height / 2;

    m_pGraphImage_Backgnd = new QImage(m_i32Width, m_i32Height, QImage::Format_RGB32);
    m_pGraphImage_Trace = new QImage(m_i32Width, m_i32Height, QImage::Format_RGB32);

	m_bOverdraw = false;

	for ( uInt32 u32Line = 0; u32Line < getNbrChannels(); ++u32Line )
	{
		m_dAxisYPosition[u32Line] = m_i32Height / (getNbrChannels() * 2.0) * (u32Line * 2 + 1);
	}

	m_dDistanceBetweenHorizontalLines = m_i32Height / (NBR_HORIZONTAL_LINES - 1.0);

	for (uInt32 i = 0; i < getNbrChannels(); ++i)
	{
		m_pChannelsArray[i]->setCenterY(m_dAxisYPosition[i]);
		m_pChannelsArray[i]->setAmplitude(m_i32Height / (2.0 * getNbrChannels()));
		m_pChannelsArray[i]->setZoomX(m_ZoomIn, m_ZoomOut);
		m_pChannelsArray[i]->setChanSkip(getNbrChannels());
		m_pChannelsArray[i]->setChanIdx(i);
	}
}

QCsGraph::~QCsGraph()
{
	if(m_pGraphImage_Backgnd) delete m_pGraphImage_Backgnd;
	if(m_pGraphImage_Trace) delete m_pGraphImage_Trace;

    if (m_pDataFileBuffer!=NULL)
    {
        #ifdef Q_OS_WIN
            VirtualFree(m_pDataFileBuffer, 0, MEM_RELEASE);
        #else
            free(m_pDataFileBuffer);
        #endif
    }

	delete m_UpdateWaitLoop;
}

/*******************************************************************************************
										Getters and Setters
*******************************************************************************************/

double QCsGraph::getZoomXRatio()
{
    if (m_ZoomOut <= 1) return (((double)m_i32Width) / m_ZoomIn) / m_u64SampleCount;
    else return (((double)m_i32Width) * m_ZoomOut) / m_u64SampleCount;
}

unsigned int QCsGraph::getZoomOut()
{
	return m_ZoomOut;
}

unsigned int QCsGraph::getZoomIn()
{
	return m_ZoomIn;
}

unsigned short QCsGraph::getZoomY()
{
	return m_u16ZoomY;
}

unsigned long long QCsGraph::getViewPosition()
{
	return m_i64ViewPosition;
}

int QCsGraph::getHeight()
{
	return m_i32Height;
}

int QCsGraph::getWidth()
{
	return m_i32Width;
}

unsigned long long QCsGraph::getSamplesPerScreen()
{
    double SampPerDiv;
    double DivPerScreen;
    unsigned long long SampsPerScreen;

    if(m_NewZoomOut>1) SampPerDiv = 128 * m_NewZoomOut;
    else SampPerDiv = 128.0 / m_NewZoomIn;

    DivPerScreen = m_i32Width / 128.0;
    SampsPerScreen = floor(DivPerScreen * SampPerDiv);

    return SampsPerScreen;
}

unsigned long long QCsGraph::CalcSamplesPerScreen(int ZoomIn, int ZoomOut)
{
    double SampPerDiv;
    double DivPerScreen;
    unsigned long long SampsPerScreen;

    if(ZoomOut>1) SampPerDiv = 128 * ZoomOut;
    else SampPerDiv = 128.0 / ZoomIn;

    DivPerScreen = m_i32Width / 128.0;

    SampsPerScreen = floor(DivPerScreen * SampPerDiv);

    return SampsPerScreen;
}

unsigned long long QCsGraph::getSampleCount()
{
	return m_u64SampleCount;
}

uInt32	QCsGraph::getNbrChannels()
{
    return static_cast<uInt32>(m_pChannelsArray.size());
}

void QCsGraph::switchGraphData()
{
	if (!m_bDataFromFile)
	{
		m_bDataFromFile = true;

		m_LastDigiZoomIn = m_ZoomIn;
		m_LastDigiZoomOut = m_ZoomOut;
		m_u16LastDigiZoomY = m_u16ZoomY;
		m_i64LastDigiViewPosition = m_i64ViewPosition;

		m_ZoomIn = m_LastFileZoomIn;
		m_ZoomOut = m_LastFileZoomOut;
		m_u16ZoomY = m_u16LastFileZoomY;
		m_i64ViewPosition = m_i64LastFileViewPosition;

		m_NewZoomIn = m_ZoomIn;
		m_NewZoomOut = m_ZoomOut;
		m_u16NewZoomY = m_u16ZoomY;
		m_i64NewViewPosition = m_i64ViewPosition;
	}
	else
	{
		m_bDataFromFile = false;

		m_LastFileZoomIn = m_ZoomIn;
		m_LastFileZoomOut = m_ZoomOut;
		m_u16LastFileZoomY = m_u16ZoomY;
		m_i64LastFileViewPosition = m_i64ViewPosition;

		m_ZoomIn = m_LastDigiZoomIn;
		m_ZoomOut = m_LastDigiZoomOut;
		m_u16ZoomY = m_u16LastDigiZoomY;
		m_i64ViewPosition = m_i64LastDigiViewPosition;

		m_NewZoomIn = m_ZoomIn;
		m_NewZoomOut = m_ZoomOut;
		m_u16NewZoomY = m_u16ZoomY;
		m_i64NewViewPosition = m_i64ViewPosition;
	}
}

void QCsGraph::setData(vector<DrawableChannel*> pChannelsArray, unsigned long long u64SampleCount)
{
	m_pChannelsArray = pChannelsArray;
	m_u64SampleCount = u64SampleCount;
}

void QCsGraph::addSigChannel(CSSIGSTRUCT *pTraceInfo, QString strSigFilePath)
{
    double dblAmplitude = m_pChannelsArray.at(0)->getAmplitude();
    double dblCenterY = m_pChannelsArray.at(0)->getCenterY();

    DrawableChannel* p = new DrawableChannel(NULL, NULL, XFER_BUFFER_SIZE, drawchanfilesig);
    p->setChanType(drawchanfilesig);
    p->setFilePath(strSigFilePath);
    p->setChanIdx(pTraceInfo->u32Channel);
    p->setSampleCount(pTraceInfo->i64RecordLength);
    p->setSampleSize(pTraceInfo->u32SampleSize);
    p->setDcOffset(pTraceInfo->i32DcOffset);
    p->setRange(pTraceInfo->u32InputRange);
    p->setSampleRes(pTraceInfo->i32SampleRes);
    p->setSampleOffset(pTraceInfo->i32SampleOffset);
    p->setCenterY(dblCenterY);
    p->setAmplitude(dblAmplitude);
    p->setZoomX(m_ZoomIn, m_ZoomOut);
    p->setWidth(m_i32Width);
    p->setHeight(m_i32Height);
    p->m_bDataValid = true;
    p->m_bInterleaved = false;
    m_pChannelsArray.push_back(p);

    if(pTraceInfo->i64RecordLength > static_cast<int64>(m_u64SampleCount))
        m_u64SampleCount = pTraceInfo->i64RecordLength;

    m_bGageDataValid = true;
    initGraph();
}

void QCsGraph::setChannels()
{
	for (uInt32 u32Line = 0; u32Line < getNbrChannels(); ++u32Line)
	{
		m_dAxisYPosition[u32Line] = m_i32Height / (getNbrChannels() * 2.0) * (u32Line * 2 + 1);
	}

	m_dDistanceBetweenHorizontalLines = m_i32Height / (NBR_HORIZONTAL_LINES - 1.0);

	for (uInt32 i = 0; i < getNbrChannels(); ++i)
	{
		m_pChannelsArray[i]->setCenterY(m_dAxisYPosition[i]);
		m_pChannelsArray[i]->setAmplitude(m_i32Height / (2.0 * getNbrChannels()));
		m_pChannelsArray[i]->setZoomX(m_ZoomIn, m_ZoomOut);
		m_pChannelsArray[i]->setWidth(m_i32Width);
        m_pChannelsArray[i]->m_bInterleaved = m_bInterleavedData;
	}

    m_i32BoardActiveChans = getNbrChannels();
}

void QCsGraph::setSampleSize(uInt32 u32SampleSize)
{
	m_u32SampleSize = u32SampleSize;
}

void QCsGraph::setPreTrig(unsigned long long u64PreTrig)
{
	m_u64PreTrig = u64PreTrig;
}

void QCsGraph::setCursor(int Xpos)
{
    qDebug() << "SetCursor";

    if(m_ZoomOut>1)
	{
        qDebug() << "SetCursor: ZoomOut: " << m_ZoomOut;
        if(m_bCursor)
        {
            m_CursorSampPos = (Xpos * m_ZoomOut) + m_i64ViewPosition;
            m_CursorScreenPos = Xpos;
        }
        else
        {
            m_CursorSampPos = (m_i32CenterX * m_ZoomOut) + m_i64ViewPosition;
            m_CursorScreenPos = m_i32CenterX;
        }


        if ((unsigned long long)m_CursorSampPos > m_u64SampleCount - 1)
        {
            m_CursorSampPos = (m_u64SampleCount - 1);
            m_CursorScreenPos = (unsigned int)floor(((double)m_CursorSampPos) - m_i64ViewPosition) / m_ZoomOut;
        }
	}
	else
	{
        qDebug() << "SetCursor: ZoomIn: " << m_ZoomIn;
        if(m_bCursor)
        {
            m_CursorSampPos = ((unsigned long long)floor((((double)Xpos) / ((double)m_ZoomIn)) + 0.5)) + m_i64ViewPosition;
            m_CursorScreenPos = (m_CursorSampPos - m_i64ViewPosition) * m_ZoomIn;
        }
        else
        {
            m_CursorSampPos = ((unsigned long long)floor((((double)m_i32CenterX) / ((double)m_ZoomIn)) + 0.5)) + m_i64ViewPosition;
            m_CursorScreenPos = (m_CursorSampPos - m_i64ViewPosition) * m_ZoomIn;
        }
		
        if ((unsigned long long)m_CursorSampPos > m_u64SampleCount - 1)
        {
            m_CursorSampPos = (m_u64SampleCount - 1);
            m_CursorScreenPos = (m_CursorSampPos - m_i64ViewPosition) * m_ZoomIn;
        }
	}

	m_NewCursorScreenPos = m_CursorScreenPos;

	m_bCursor = true;

	if (!m_bAcquiring)
	{
        qDebug() << "SetCursor: Emit updateGraph";
        updateCursorVals();
        emit updateGraph(GraphInit);
	}
    qDebug() << "SetCursor";
}

void QCsGraph::unsetCursor()
{
    //qDebug() << "UnsetCursor";
    m_bCursor = false;

    if (!m_bAcquiring)
    {
        //qDebug() << "UnsetCursor: Emit updateGraph";
        emit updateGraph(GraphInit);
    }
}

void QCsGraph::updateCursorVals()
{
    qDebug() << "updateCursorVals";
    size_t ChanCount = m_pChannelsArray.size();
    qDebug() << "updateCursorVals: ChanCount: " << ChanCount;
    for (size_t i = 0; i < ChanCount; i++)
    {
        m_i32CursorVals[i] = m_pChannelsArray[i]->getSampVal(m_CursorSampPos);
        qDebug() << "updateCursorVals: Chan" << i+1 << ", " << m_i32CursorVals[i];
    }

    GenCursorStatus();

    qDebug() << "updateCursorVals: Done";
}

void QCsGraph::GenCursorStatus()
{
    qDebug() << "GenCursorStatus";
	if(m_bCursor)
	{
		uInt32 Range;
		int SampRawVal;
        double SampleRes;
        int AvgCount;
        double SampleOffset;
        int DcOffset;
        int idxSigFile = 0;
		double SampVoltVal;
        bool bDataValid;
		QString strCursorStatus;
		strCursorStatus.clear();
		char szText[128];
		sprintf(szText, "Sample: %lld\r\n", (long long)(m_CursorSampPos - m_u64TrigPosition));
		strCursorStatus.append(QString(szText));
        //strCursorStatus.append(QString("Sample: %1\r\n").arg(m_CursorSampPos- m_u64TrigPosition));
        size_t ChanCount = m_pChannelsArray.size();

        for(size_t i=1; i <= ChanCount; i++)
		{
            bDataValid = m_pChannelsArray[i-1]->m_bDataValid;

            qDebug() << "GenCursorStatus: Chan" << i+1 << ", Valid: " << bDataValid;

            if( bDataValid && ((unsigned long)m_CursorSampPos < m_pChannelsArray[i-1]->getSampleCount()))
            {
                Range = m_pChannelsArray[i - 1]->getRange();
                SampRawVal = m_i32CursorVals[i - 1];
                SampleOffset = m_pChannelsArray[i-1]->getSampleOffset();
                SampleRes = m_pChannelsArray[i-1]->getSampleRes();
                DcOffset = m_pChannelsArray[i-1]->getDcOffset();
                AvgCount = m_pChannelsArray[i-1]->getAvgCount();
                SampVoltVal = (((double)(SampleOffset - SampRawVal)) / ((double)SampleRes * AvgCount) * (Range / 2000.0)) + (DcOffset / 1000.0);

                switch(m_pChannelsArray[i-1]->getChanType())
                {

                    case drawchanreg:
                        strCursorStatus.append(QString("CH%1: %2, %3V\r\n").arg(i).arg(SampRawVal).arg(QString().setNum(SampVoltVal, 'g', 3)));
                        break;

                    case drawchanfilesig:
                        strCursorStatus.append(QString("Sig file #%1: %2, %3V\r\n").arg(idxSigFile + 1).arg(SampRawVal).arg(QString().setNum(SampVoltVal, 'g', 3)));
                        idxSigFile++;
                        break;

                    case drawchanfiledata:
                        strCursorStatus.append(QString("CH%1: %2\r\n").arg(i).arg(SampRawVal));
                        break;
                }
            }
		}

		emit updateCursorStatus(strCursorStatus);
	}

	else emit updateCursorStatus("");

    qDebug() << "GenCursorStatus: Done";
}

void QCsGraph::editZoomX(double dDelta)
{
    qDebug() << "EditZoomX: " << dDelta;

    unsigned long long SampsPerScreen;
    if (dDelta < 0)
    {
        if (m_NewZoomOut > 1)
        {
            m_NewZoomOut >>= 1;
            m_bZoomXChanged = true;
        }
        else
        {            
            int ZoomIn = m_NewZoomIn << 1;
            SampsPerScreen = CalcSamplesPerScreen(ZoomIn, m_NewZoomOut);

            if(SampsPerScreen < MIN_SAMPS_PER_SCREEN) return;

            m_NewZoomIn <<= 1;
            m_bZoomXChanged = true;
        }
    }
    else
    {
        if (m_NewZoomIn > 1)
        {
            m_NewZoomIn >>= 1;
            m_bZoomXChanged = true;
        }
        else
        {
            SampsPerScreen = getSamplesPerScreen();
            if(m_u64SampleCount<SampsPerScreen) return;

            int ZoomOut = m_NewZoomOut << 1;
            SampsPerScreen = CalcSamplesPerScreen(m_NewZoomIn, ZoomOut);

            if(SampsPerScreen > MAX_SAMPS_PER_SCREEN) return;
            if(m_u64SampleCount<(SampsPerScreen/2.0)) return;

            m_NewZoomOut <<= 1;
            m_bZoomXChanged = true;
        }
    }
	
	int zoom;
    if(m_NewZoomIn > 1)
	{
		zoom = 128 / m_NewZoomIn;
	}
	else
	{
		zoom = 128 * m_NewZoomOut;
	}

	emit horizChanged(zoom);

	s_updateCursorVals();

	if (!m_bAcquiring && m_bZoomXChanged)
	{
        emit zoomXChanged();
        //updateGraph(GraphInit);
        //qDebug() << "EditZoomX: Emit updateGraph";
        emit updateGraph(GraphInit);
	}

    qDebug() << "EditZoomX: Done";
}

void QCsGraph::editZoomY(double dDelta)
{
    //qDebug() << "EditZoomY";
	if ( dDelta < 0 )
	{
        if ( m_u16NewZoomY < 256 )
		{
			m_u16NewZoomY <<= 1;
		}
	}
	else
	{
		if ( m_u16NewZoomY > 1 )
		{
			m_u16NewZoomY >>= 1;
		}
	}

	if (!m_bAcquiring)
	{
		emit zoomXChanged();
        //updateGraph(GraphInit);
        //qDebug() << "EditZoomY: Emit updateGraph";
        emit updateGraph(GraphInit);
	}
}

void QCsGraph::AdjustZoom()
{
    qDebug() << "AdjustZoom";
    unsigned long long u64SampleCount = getSampleCount();
    int ZoomIn = m_NewZoomIn;
    int ZoomOut = m_NewZoomOut;
    int SampPerScreen = CalcSamplesPerScreen(ZoomIn, ZoomOut);
    int SampLimit = SampPerScreen / 2;
    qDebug() << "AdjustZoom: " << SampPerScreen << ", " << u64SampleCount;
    if (u64SampleCount >= (unsigned long long)SampLimit)
        return;
    while (u64SampleCount < (unsigned long long)SampLimit)
    {
        if(ZoomOut>1) ZoomOut>>=1;
        else ZoomIn<<=1;
        SampPerScreen = CalcSamplesPerScreen(ZoomIn, ZoomOut);
        SampLimit = SampPerScreen / 2;
        qDebug() << "AdjustZoom: " << SampPerScreen << ", " << ZoomIn << ", " << ZoomOut;
    }

    m_i64NewViewPosition = 0;
    m_NewZoomIn = ZoomIn;
    m_NewZoomOut = ZoomOut;
    m_bZoomXChanged = true;

    int zoom;
    if(m_NewZoomIn > 1)
    {
        zoom = 128 / m_NewZoomIn;
    }
    else
    {
        zoom = 128 * m_NewZoomOut;
    }

    emit horizChanged(zoom);
    qDebug() << "AdjustZoom: Done";
}

void QCsGraph::enableCursor()
{
	m_bCursor = true;
}

void QCsGraph::setCursorSampPos(unsigned int CursorSampPos)
{
	m_CursorSampPos = CursorSampPos;
}

void QCsGraph::setCursorScreenPos(unsigned int CursorScreenPos)
{
	m_CursorScreenPos = CursorScreenPos;
}

void QCsGraph::setAcquiring(bool bAcqState)
{
	m_bAcquiring = bAcqState;
}

void QCsGraph::setTrigPos(unsigned long long TrigPos)
{
	m_u64TrigPosition = TrigPos;
}

/*******************************************************************************************
										Overwritted events
*******************************************************************************************/

void QCsGraph::s_AbortUpdate()
{
    qDebug() << "s_AbortUpdate";
    uInt32 u32ChannelCount = getNbrChannels();

    m_bAbort = true;
    for (uInt32 i = 0; i < u32ChannelCount; i++)
    {
        m_pChannelsArray[i]->SetAbort();
    }
}

void QCsGraph::ClearAbort()
{
    uInt32 u32ChannelCount = getNbrChannels();

    for (uInt32 i = 0; i < u32ChannelCount; i++)
    {
        m_pChannelsArray[i]->ClearAbort();
    }
    m_bAbort = false;
}

void QCsGraph::paintEvent(QPaintEvent* pEvent)
{
    Q_UNUSED(pEvent);
    if(!m_bPainting)
	{
        m_bPainting = true;
        //qDebug() << "Paint";

        QPainter painterA;

        if(m_bInitGraph)
        {
            //qDebug() << "Paint: InitGraph";
            drawBackground();
            painterA.begin(m_pGraphImage_Trace);
            painterA.setRenderHint(QPainter::SmoothPixmapTransform, true);
            painterA.drawImage(0, 0, *m_pGraphImage_Backgnd);
            painterA.end();
        }

        if (m_bGageDataValid)
        {
            //qDebug() << "Paint: Data valid";
            UpdateSamplePoints();
            if(m_bAbort)
            {
                ClearAbort();

            }
            else
            {
                //qDebug() << "Paint: DrawPlot";
                drawPlot();
            }
        }

        if(m_bCursor) updateCursorVals();

        if(isVisible())
        {
            //qDebug() << "Paint: Paint scope";
            painterA.begin(this);
            painterA.setRenderHint(QPainter::SmoothPixmapTransform, true);
            painterA.drawImage(0, 0, *m_pGraphImage_Trace);
            painterA.end();

            drawTrigAxis();
            if(m_bCursor) drawCursor();
        }

        m_bInitGraph = false;
    }

    //qDebug() << "Paint: Done";
    //qDebug() << "";

    m_bPainting = false;

    m_bInitGraph = false;
}

void QCsGraph::initGraph()
{
    //qDebug() << "InitGraph: Emit updateGraph";
    emit updateGraph(GraphInit);
}

void QCsGraph::s_UpdateGraph(GRAPH_UPDATE UpdateAction)
{
    //qDebug() << "UpdateGraph";
    while(m_bPainting || m_bUpdating)
    {
        QCoreApplication::processEvents();
        quSleep(10);
    }

    m_bUpdating = true;

    m_bInitGraph = false;

    if ((m_ZoomIn != m_NewZoomIn) || (m_ZoomOut != m_NewZoomOut) || (m_u16ZoomY != m_u16NewZoomY) || (m_i64ViewPosition != m_i64NewViewPosition) || (m_CursorScreenPos != m_NewCursorScreenPos))
    {
        m_ZoomIn = m_NewZoomIn;
        m_ZoomOut = m_NewZoomOut;
        m_u16ZoomY = m_u16NewZoomY;
        m_i64ViewPosition = m_i64NewViewPosition;
        m_CursorScreenPos = m_NewCursorScreenPos;

        for (uInt32 i = 0; i < getNbrChannels(); ++i)
        {
            m_pChannelsArray[i]->setZoomX(m_ZoomIn, m_ZoomOut);
            m_pChannelsArray[i]->setZoomY(m_u16ZoomY);
        }
    }

    if (isVisible())
    {
        switch(UpdateAction)
        {
            case GraphInit:
                m_bInitGraph = true;
                break;

            case GraphUpdatePlot:
                m_bInitGraph = false;
                break;

            default:
                break;
        }

        while (m_bPainting)
        {
            QCoreApplication::processEvents();
            quSleep(10);
        }

        m_GraphUpdateType = UpdateAction;

        switch(UpdateAction)
        {
            case GraphInit:
               repaint();
               break;

            case GraphUpdatePlot:
                repaint();
                break;

            default:
                break;
        }
    }

    m_MutexSharedVars->lock();
    switch(UpdateAction)
    {
        case GraphInit:
            *m_bScopeInitDone = true;
            break;

        case GraphUpdatePlot:
            *m_bScopeUpdateDone = true;
            break;

        default:
            break;
    }
    m_MutexSharedVars->unlock();

    m_bUpdating = false;

    //qDebug() << "UpdateGraph: Done";
}

void QCsGraph::resizeEvent(QResizeEvent*)
{
    //qDebug() << "resizeEvent";
    while(m_bPainting)
    {
        QCoreApplication::processEvents();
        quSleep(10);
    }

    int NewHeight = geometry().height() - 1;
    int NewWidth = geometry().width();

    if((NewHeight==m_i32Height) && (NewWidth = m_i32Width))
    {
        return;
    }
	m_i32Height = geometry().height() - 1; //Reduce the height or we can't see the bottom line
	m_i32Width = geometry().width();
	m_i32CenterX = m_i32Width/2;
	m_i32CenterY = m_i32Height/2;

    delete m_pGraphImage_Backgnd;
    delete m_pGraphImage_Trace;

    m_pGraphImage_Backgnd = new QImage(m_i32Width, m_i32Height, QImage::Format_RGB32);
    m_pGraphImage_Backgnd->fill(Qt::lightGray);
    m_pGraphImage_Trace = new QImage(m_i32Width, m_i32Height, QImage::Format_RGB32);
    m_pGraphImage_Trace->fill(Qt::lightGray);

    CalculateChannelsCenter();

    //qDebug() << "resizeEvent: Emit updateGraph";
    emit updateGraph(GraphInit);	
}

/*******************************************************************************************
										Private functions
*******************************************************************************************/

void QCsGraph::drawBackground()
{
    //qDebug() << "drawBackground";
    m_i32Height = geometry().height(); //Reduce the height or we can't see the bottom line
    m_i32Width = geometry().width();

    //qDebug() << "drawBackground: draw";
    QPainter painter(m_pGraphImage_Backgnd);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    painter.fillRect(0, 0, m_i32Width, m_i32Height, Qt::lightGray);

    //Draw the grid (HORIZONTAL)
    painter.setPen(m_penGridLine);

    float fltDistHorizLines = ((float)m_i32Height) / ((m_i32DispChanCount * 8));
    uInt32 u32HorizLinesCount = (m_i32DispChanCount * 8) + 1;
    uInt32 HorizPos;
    for ( uInt32 i = 0; i < u32HorizLinesCount; i++ )
    {
        HorizPos = floor((i * fltDistHorizLines) + 0.5);
        painter.drawLine(0, HorizPos, m_i32Width, HorizPos);
    }

    //Draw the grid (VERTICAL)
    for ( int i = 0; i < m_i32Width; i += DISTANCE_BETWEEN_VERTICAL_LINES )
    {
        painter.drawLine(i, 0, i, m_i32Height);
    }

    //Draw the red line (one per channel)
    painter.setPen(m_penCenterLine);

    for( int32 i32Line = 0; i32Line < m_i32DispChanCount; ++i32Line )
    {
        painter.drawLine(0, m_dAxisYPosition[i32Line], m_i32Width, m_dAxisYPosition[i32Line]);
    }

     painter.end();
}

void QCsGraph::drawTrigAxis()
{
    QPainter painter(this);

    unsigned int AxisScreenPos;
	if(m_ZoomOut>1)
	{
		AxisScreenPos = (m_u64TrigPosition - m_i64ViewPosition) / m_ZoomOut;
	}
	else
	{
		AxisScreenPos = (m_u64TrigPosition - m_i64ViewPosition) * m_ZoomIn;
	}

    painter.setPen(m_penTrigAxis);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawLine(AxisScreenPos, 0, AxisScreenPos, m_i32Height);

    QFont AxisFont = painter.font();
    AxisFont.setBold(false);
    AxisFont.setPointSize(16);
    painter.setFont(AxisFont);
    painter.drawText(AxisScreenPos + 5, m_i32Height - 25, AxisScreenPos + 30, m_i32Height - 5, 0, "T");

    painter.end();
}

void QCsGraph::drawPlot()
{
    QPainter painter(m_pGraphImage_Trace);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    if(!m_bOverdraw)
    {
        //qDebug() << "drawPlot: No overdraw";
        painter.drawImage(0, 0, *m_pGraphImage_Backgnd);
    }
    else
    {
        //qDebug() << "drawPlot: Overdraw";
    }
    uInt32 u32ChannelCount = getNbrChannels();

    for (uInt32 i = 0; i < u32ChannelCount; ++i)
    {
        if(m_pChannelsArray[i]->m_bDataValid) m_pChannelsArray[i]->draw(painter, m_bConnectPoints);
    }

    painter.end();
}

void QCsGraph::drawCursor()
{
    if (m_CursorSampPos < m_i64ViewPosition) return;
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setPen(m_penCursor);
    painter.drawLine(m_CursorScreenPos, 0, m_CursorScreenPos, m_i32Height);

    painter.end();
}

/*******************************************************************************************
										Public slots
*******************************************************************************************/

unsigned long long QCsGraph::CalculateMaxViewPos()
{
    unsigned long long MaxViewPos;
    int SampPerScreen = getSamplesPerScreen();
    unsigned long long u64SampleCount = getSampleCount();

    MaxViewPos = u64SampleCount - (0.75 * SampPerScreen);

    return MaxViewPos;
}

void QCsGraph::AdjustViewPos()
{
    unsigned long long MaxViewPos = CalculateMaxViewPos();
    if ((unsigned long long)m_i64ViewPosition > MaxViewPos)
        m_i64NewViewPosition = MaxViewPos;
}

void QCsGraph::s_movePositionX(int i32value)
{
    qDebug() << "s_movePositionX";
    qDebug() << "s_movePositionX: Val: " << i32value;
    m_i64NewViewPosition = floor(i32value * m_dblSampPerScroll);

    qDebug() << "s_movePositionX: NewViewPos: " << m_i64NewViewPosition;

	if(m_NewZoomOut>1)
	{
		m_NewCursorScreenPos = (unsigned int)floor((((double)(m_CursorSampPos - m_i64NewViewPosition)) / m_NewZoomOut) + 0.5);
	}
	else
	{
		m_NewCursorScreenPos = floor(((m_CursorSampPos - m_i64NewViewPosition) * m_NewZoomIn)  + 0.5);
	}
	
    if (!m_bAcquiring)
	{
        //updateGraph(GraphInit);
        //qDebug() << "s_movePositionX: Emit update graph";
        emit updateGraph(GraphInit);
	}

	long long StartPos = (long long)m_i64NewViewPosition - m_u64PreTrig;
	QString strPositionStatus;
	strPositionStatus.clear();
	strPositionStatus.append(QString("Start position: %1").arg(StartPos));
	emit updatePositionStatus(strPositionStatus);
    qDebug() << "s_movePositionX: Done";
}

void QCsGraph::ChangePositionX(int i32value)
{
    qDebug() << "Change PositionX";
    if(i32value<0)
    {
        m_i64NewViewPosition += i32value;
        if(m_i64NewViewPosition<0) m_i64NewViewPosition = 0;
    }
    else
    {
//       int64 i64SampPerScreen = getSamplesPerScreen();

        int64 i64HiLimit = m_u64SampleCount;
        if(i64HiLimit<0) i64HiLimit = 0;

        m_i64NewViewPosition += i32value;
        if(m_i64NewViewPosition>=i64HiLimit) m_i64NewViewPosition = i64HiLimit;
    }

    qDebug() << "Change PositionX: New pos: " << m_i64NewViewPosition;

	if (m_NewZoomOut>1)
	{
		m_NewCursorScreenPos = (unsigned int)floor((double)(m_CursorSampPos - m_i64NewViewPosition)) / m_NewZoomOut;
	}
	else
	{
		m_NewCursorScreenPos = (m_CursorSampPos - m_i64NewViewPosition) * m_NewZoomIn;
	}

	if (!m_bAcquiring)
	{
        //updateGraph(GraphInit);
        qDebug() << "Change PositionX: Emit updateGraph" << m_i64NewViewPosition;
        emit updateGraph(GraphInit);
	}

    int ScrollPos = floor((m_i64NewViewPosition / m_dblSampPerScroll) + 0.5);
    emit updateScrollBar(ScrollPos);

	long long StartPos = (long long)m_i64NewViewPosition - m_u64PreTrig;
	QString strPositionStatus;
	strPositionStatus.clear();
    strPositionStatus.append(QString("Start position: %1").arg(StartPos));
    emit updatePositionStatus(strPositionStatus);
}

void QCsGraph::GoToSample(long long SampAddr, bool bCenterScreen)
{
    qDebug() << "Change PositionX";

    m_i64NewViewPosition = SampAddr + m_u64PreTrig;

    if (m_i64NewViewPosition<0)
        m_i64NewViewPosition = 0;
    if ((unsigned long long)m_i64NewViewPosition >= m_u64SampleCount)
        m_i64NewViewPosition = m_u64SampleCount - 1;

    m_CursorSampPos = m_i64NewViewPosition;

    int64 i64SampPerScreen;

    if(m_NewZoomOut>1) i64SampPerScreen = m_i32Width * m_NewZoomOut;
    else i64SampPerScreen = (int64)ceil(((double)m_i32Width) / m_NewZoomIn);

    if(bCenterScreen) m_i64NewViewPosition = m_i64NewViewPosition - (i64SampPerScreen / 2);

    //int64 i64HiLimit = m_u64SampleCount - i64SampPerScreen + 8;
    int64 i64HiLimit = m_u64SampleCount - 1;
    if(i64HiLimit<0) i64HiLimit = 0;

    if(m_i64NewViewPosition<0) m_i64NewViewPosition = 0;
    if(m_i64NewViewPosition>=i64HiLimit) m_i64NewViewPosition = i64HiLimit;

    qDebug() << "Change PositionX_SampAddr: New pos: " << m_i64NewViewPosition;

    if (m_NewZoomOut>1)
    {
        m_NewCursorScreenPos = (unsigned int)floor((double)(m_CursorSampPos - m_i64NewViewPosition)) / m_NewZoomOut;
    }
    else
    {
        m_NewCursorScreenPos = (m_CursorSampPos - m_i64NewViewPosition) * m_NewZoomIn;
    }

    m_bCursor = true;

    if (!m_bAcquiring)
    {
        //updateGraph(GraphInit);
        qDebug() << "Change PositionX: Emit updateGraph" << m_i64NewViewPosition;
        emit updateGraph(GraphInit);
    }

    int ScrollPos = floor((m_i64NewViewPosition / m_dblSampPerScroll) + 0.5);
    emit updateScrollBar(ScrollPos);

    long long StartPos = (long long)m_i64NewViewPosition - m_u64PreTrig;
    QString strPositionStatus;
    strPositionStatus.clear();
    strPositionStatus.append(QString("Start position: %1").arg(StartPos));
    emit updatePositionStatus(strPositionStatus);
}

void QCsGraph::s_updateCursorVals()
{
	if(m_bCursor)
	{
		if(m_ZoomOut>1)
		{
			m_NewCursorScreenPos = (unsigned int)floor(((double)m_CursorSampPos) / m_NewZoomOut);
		}
		else
		{
			m_NewCursorScreenPos = m_CursorSampPos * m_NewZoomIn;
		}
	}
}

void QCsGraph::s_ClearGraph()
{
    //qDebug() << "s_ClearGraph";
    m_bGageDataValid = false;

    uInt32 u32ChannelCount = getNbrChannels();

    for(uInt32 i=0;i<u32ChannelCount;i++)
    {
        m_pChannelsArray[i]->m_bDataValid = false;
    }

    if(!(*m_bAcqRunning))
		m_bCursor = false;
	m_bCleared = true;

    //qDebug() << "s_ClearGraph: Emit updateGraph";
    emit updateGraph(GraphInit);
}

void QCsGraph::SetAddrScopeInitDone(bool* bScopeInitDone)
{
    m_bScopeInitDone = bScopeInitDone;
}

void QCsGraph::SetAddrScopeUpdateDone(bool* bScopeUpdateDone)
{
    m_bScopeUpdateDone = bScopeUpdateDone;
}
void QCsGraph::SetAddrAcqRunning(bool* bAcqRunning)
{
    m_bAcqRunning = bAcqRunning;
}

void QCsGraph::SetAddrMutexSharedVars(QMutex* MutexSharedVars)
{
    m_MutexSharedVars = MutexSharedVars;
}

int QCsGraph::UpdateSamplePoints()
{
    //qDebug() << "UpdateSamplePoints";
    unsigned long long ViewPos = getViewPosition();
    unsigned long long SampleCount = getSampleCount();
    unsigned long long u64NbDisplaySamples;
    unsigned long long u64EndSample;
    int i32Ret = 0;

    long long StartPos = (long long)m_i64NewViewPosition - m_u64PreTrig;
    QString strPositionStatus;
    strPositionStatus.clear();
    strPositionStatus.append(QString("Start position: %1").arg(StartPos));
    emit updatePositionStatus(strPositionStatus);

    //Get first sample
    ViewPos = getViewPosition();

    int ZoomOut = getZoomOut();
    int ZoomIn = getZoomIn();

    //Calculate number of samples to display.
    if (ZoomOut <= 1) u64NbDisplaySamples = floor((((double)getWidth()) / ZoomIn) + 0.5);
    else u64NbDisplaySamples = floor(((((double)getWidth()) * ZoomOut)) + 0.5);

    u64EndSample = ViewPos + u64NbDisplaySamples;

    if (u64EndSample >= (SampleCount - 1)) u64EndSample = SampleCount - 1;

    m_u64XferTime = 0;
    m_u64XferSamples = 0;

    for (size_t i = 0; i < m_pChannelsArray.size(); ++i)
    {
        i32Ret = m_pChannelsArray[i]->GenSamplePoints(ViewPos, u64EndSample);
        if(i32Ret<0) break;
        if(m_bAbort) break;
    }

    if(i32Ret==CS_CHANNEL_PROTECT_FAULT)
    {
        for (size_t i = 0; i < m_pChannelsArray.size(); ++i)
        {
            m_pChannelsArray[i]->SetFlatLine();
        }

        m_bGageDataValid = false;
        emit channelProtFault();
    }

    QString strXferTime;
    double dblXferRate;
    double dblXferTimeMsec = m_u64XferTime / 1000000.0;
    double XferSamples = m_u64XferSamples;
    double XferBytes = XferSamples * m_u32SampleSize;
    dblXferRate = XferBytes / (dblXferTimeMsec / 1000.0);
    if(dblXferRate>=1000000000)
    {
        dblXferRate/=1000000000;
        strXferTime = QString("TxTime: %1msecs").arg(QString().setNum(dblXferTimeMsec, 'f', 3)) + QString(" (%1GB/s)").arg(QString().setNum(dblXferRate, 'f', 3));
    }
    else if(dblXferRate>=1000000)
    {
        dblXferRate/=1000000;
        strXferTime = QString("TxTime: %1msecs").arg(QString().setNum(dblXferTimeMsec, 'f', 3)) + QString(" (%1MB/s)").arg(QString().setNum(dblXferRate, 'f', 3));
    }
    else if(dblXferRate>=1000)
    {
        dblXferRate/=1000;
        strXferTime = QString("TxTime: %1msecs").arg(QString().setNum(dblXferTimeMsec, 'f', 3)) + QString(" (%1KB/s)").arg(QString().setNum(dblXferRate, 'f', 3));
    }
    else
    {
        strXferTime = QString("TxTime: %1msecs").arg(QString().setNum(dblXferTimeMsec, 'f', 3)) + QString(" (%1B/s)").arg(QString().setNum(dblXferRate, 'f', 3));
    }

    emit updateTxTime(strXferTime);

    return i32Ret;
}

void QCsGraph::CalculateChannelsCenter()
{
    qDebug() << "CalculateChannelsCenter";
    uInt32 u32ChannelCount = getNbrChannels();
    qDebug() << "CalculateChannelsCenter: Chan count: " << u32ChannelCount;
    m_i32Height = geometry().height(); //Reduce the height or we can't see the bottom line
    m_i32Width = geometry().width();

    uInt32 u32DispChanChanCount = getDispChanCount();
    qDebug() << "CalculateChannelsCenter: Disp chan count: " << u32DispChanChanCount;
    float fltDistYPos = ((float)m_i32Height) / u32DispChanChanCount;
    float fltYStartPos = fltDistYPos / 2;
    for ( uInt32 u32Line = 0; u32Line < u32DispChanChanCount; ++u32Line )
    {
        m_dAxisYPosition[u32Line] = floor(((u32Line * fltDistYPos) + fltYStartPos) + 0.5);
    }

    for ( uInt32 i = 0; i < u32ChannelCount; ++i )
    {
        switch(m_pChannelsArray[i]->getChanType())
        {
            case drawchanreg:
                m_pChannelsArray[i]->setCenterY(m_dAxisYPosition[i]);
                break;

            case drawchanfilesig:
                m_pChannelsArray[i]->setCenterY(m_dAxisYPosition[0]);
                break;

            case drawchanfiledata:
                m_pChannelsArray[i]->setCenterY(m_dAxisYPosition[i]);
                break;
        }

        m_pChannelsArray[i]->setAmplitude(m_i32Height / (2.0 * m_i32DispChanCount));
        m_pChannelsArray[i]->setZoomX(m_ZoomIn, m_ZoomOut);
        m_pChannelsArray[i]->setWidth(m_i32Width);
        m_pChannelsArray[i]->setHeight(m_i32Height - 2);
    }
    qDebug() << "CalculateChannelsCenter: Done";
}

void QCsGraph::s_DisableData()
{
    m_bGageDataValid = false;

    uInt32 u32ChannelCount = getNbrChannels();

    for(uInt32 i=0;i<u32ChannelCount;i++)
    {
        m_pChannelsArray[i]->m_bDataValid = false;
    }
}
