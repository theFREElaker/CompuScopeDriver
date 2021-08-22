#include "qscope.h"
#include "math.h"
#include <QDebug>
#include <QLibrary>
#include "CsTestFunc.h"

using namespace CsTestFunc;

QScope::QScope(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	setFocusPolicy(Qt::StrongFocus);
	m_bMustAbort = false;
    m_bWheelEvent = false;

    connect(this, SIGNAL(displaySysInfo()), this, SLOT(s_DisplaySystemInfo()), Qt::QueuedConnection);
    connect(this, SIGNAL(displayFwVersions()), this, SLOT(s_DisplayFwVersions()), Qt::QueuedConnection);
}

QScope::~QScope()
{
	for( uInt32 i = 0; i < m_channelArray.size(); ++i )
	{
		delete m_channelArray[i];
	}
	m_channelArray.clear();
}

void QScope::setChannels()
{
	uInt32 u32ChannelCount = (m_pGageSystem->GetActiveChannelCount());
	int64 i64SegmentSize = m_pGageSystem->getSegmentSize();

	unsigned int ZoomIn = ui.graph->getZoomIn();
	unsigned int ZoomOut = ui.graph->getZoomOut();
	unsigned short ZoomY = ui.graph->getZoomY();
	int Width = ui.graph->getWidth();
    int Height = ui.graph->getHeight();
	ui.graph->m_bInterleavedData = m_pGageSystem->m_bInterleavedData;
    m_pGageSystem->GetMulRecAvgCount(&m_i32AvgCount);
    ui.graph->setDispChanCount(u32ChannelCount);

	for (size_t i = 0; i < m_channelArray.size(); i++)
	{
		delete m_channelArray.at(i);
	}

	m_channelArray.clear();

	for( uInt32 i = 0; i < u32ChannelCount; ++i )
	{
        DrawableChannel* p = new DrawableChannel(m_pGageSystem, m_pGageSystem->getBuffer(i), i64SegmentSize, drawchanreg);
        p->m_bDataValid = false;
		p->setZoomX(ZoomIn, ZoomOut);
		p->setZoomY(ZoomY);
		p->setWidth(Width);
        p->setHeight(Height);
        p->setChanSkip(m_pGageSystem->GetChannelSkip());
		p->setChanIdx(i);
		p->setSampleSize(m_pGageSystem->getSampleSize());
        p->setSampleRes(m_pGageSystem->getSampleRes());
        p->setSampleOffset(m_pGageSystem->getSampleOffset());
        p->setRange(m_pGageSystem->GetInputRange((i*m_pGageSystem->GetChannelSkip()) + 1));
        //p->setDcOffset(m_pGageSystem->GetDcOffset((i*m_pGageSystem->GetChannelSkip()) + 1));
        p->m_bInterleaved = m_pGageSystem->m_bInterleavedData;
        p->setSegCount(m_pGageSystem->getSegmentCount());
        p->setDisplayOneSeg(*m_bDisplayOneSeg);
        p->setCurrSeg(*m_i64CurrSeg);
        p->SetAddrXferTime(GetGraph()->GetAddrXferTime());
        p->SetAddrXferSamples(GetGraph()->GetAddrXferSamples());
        p->setActiveChanCount(m_pGageSystem->GetActiveChannelCount());
        p->setAvgCount(m_i32AvgCount);
		m_channelArray.push_back(p);
	}
	ui.graph->setData(m_channelArray, i64SegmentSize);

    ui.graph->CalculateChannelsCenter();

    //qDebug() << "clear graph from set channels";
	ui.graph->s_ClearGraph();
}

void QScope::setFileChannels(int ChanCount, int SampSize, int Range, QString strDataFilePath)
{
    qDebug() << "QScope: Set file channels";

    unsigned long long DataFileSize;
	int SampOffset, SampRes;
	uInt32 u32ChannelCount = ChanCount;
	int SampleCount;
    QFile DataFile;

	unsigned int ZoomIn = ui.graph->getZoomIn();
	unsigned int ZoomOut = ui.graph->getZoomOut();
	unsigned short ZoomY = ui.graph->getZoomY();
	int Width = ui.graph->getWidth();
    int Height = ui.graph->getHeight();
	ui.graph->m_bInterleavedData = true;
    ui.graph->setDispChanCount(ChanCount);

	switch (SampSize)
	{
		case 1:
			SampOffset = 127;
			SampRes = -128;
			break;

		case 2:
			SampOffset = -4;
			SampRes = -32768;
			break;

        default: // assume sample size is 2
            SampOffset = -4;
            SampRes = -32768;
            break;
	}

	ui.graph->m_bGageDataValid = false;

    qDebug() << "QScope: Set file channels: Get file size";
    DataFile.setFileName(strDataFilePath);
    DataFile.open(QIODevice::ReadOnly);
    DataFile.seek(0);
    DataFileSize = DataFile.size();
    qDebug() << "QScope: Set file channels: File size: " << DataFileSize;
    DataFile.close();
    SampleCount = DataFileSize / (SampSize * ChanCount);

	setSampleSize(SampSize);
	setAvgCount(1);
	GetGraph()->setPreTrig(0);
	GetGraph()->setTrigPos(0);
    AdjustHScrollBar();
    ui.graph->s_ClearGraph();

    qDebug() << "QScope: Set file channels: Clear channels array";
	for (size_t i = 0; i < m_channelArray.size(); i++)
	{
		delete m_channelArray.at(i);
	}
	m_channelArray.clear();

    qDebug() << "QScope: Set file channels: Set new channels array";
	for (uInt32 i = 0; i < u32ChannelCount; ++i)
	{
        DrawableChannel* p = new DrawableChannel(m_pGageSystem, NULL, SampleCount, drawchanfiledata);
        p->setFilePath(strDataFilePath);
		p->setZoomX(ZoomIn, ZoomOut);
		p->setZoomY(ZoomY);
		p->setWidth(Width);
        p->setHeight(Height);
		p->setChanSkip(ChanCount);
		p->setChanIdx(i);
		p->setSampleSize(SampSize);
		p->setSampleOffset(SampOffset);
		p->setSampleRes(SampRes);
		p->setRange(Range);
        p->setDcOffset(0);
        p->setActiveChanCount(ChanCount);
        p->m_bDataValid = true;
        p->SetAddrXferTime(GetGraph()->GetAddrXferTime());
        p->SetAddrXferSamples(GetGraph()->GetAddrXferSamples());
		m_channelArray.push_back(p);
	}
    qDebug() << "QScope: Set file channels: Set data";
	ui.graph->setData(m_channelArray, SampleCount);
    qDebug() << "QScope: Set file channels: Set new channels array: Done";
    ui.graph->CalculateChannelsCenter();
    qDebug() << "QScope: Set file channels: Set channels";
	ui.graph->setChannels();
	ui.graph->m_bGageDataValid = true;
    qDebug() << "QScope: Set file channels: Init graph";
	ui.graph->initGraph();

    qDebug() << "QScope: Set file channels: Done";
}

void QScope::setSampleSize(uInt32 u32SampleSize)
{
	ui.graph->setSampleSize(u32SampleSize);
}

void QScope::setAvgCount(int32 AvgCount)
{
	m_i32AvgCount = AvgCount;
}


/*******************************************************************************************
										Overwritten functions
*******************************************************************************************/

void QScope::wheelEvent(QWheelEvent* wheelEvent)
{
    if(m_bWheelEvent) return;
    m_bWheelEvent = true;
    qDebug() << "WheelEvent";

	if ( wheelEvent->modifiers() & Qt::ControlModifier )
	{
		if ( wheelEvent->delta() < 0 )
		{
			ui.graph->editZoomY(-1);
		}
		else
		{
			ui.graph->editZoomY(1);
		}
	}
	else
	{
		if(wheelEvent->delta() < 0)
		{
			ui.graph->editZoomX(-0.02);
		}
		else
		{
			ui.graph->editZoomX(0.02);
		}
        AdjustHScrollBar();

        if (!GetGraph()->m_bAcquiring)
        {
            GetGraph()->updateGraph(GraphInit);
        }
	}

    qDebug() << "WheelEvent: Done";
    m_bWheelEvent = false;
}

void QScope::keyPressEvent(QKeyEvent* event)
{
	switch(event->key())
	{
		case Qt::Key_Plus:
			ui.graph->editZoomX(-0.02);
            if (ui.graph->m_bZoomXChanged) AdjustHScrollBar();
			break;

		case Qt::Key_Minus:
			ui.graph->editZoomX(0.02);
            if (ui.graph->m_bZoomXChanged) AdjustHScrollBar();
			break;

		case Qt::Key_Asterisk:
			ui.graph->editZoomY(-1);
			break;

		case Qt::Key_Slash:
			ui.graph->editZoomY(1);
			break;

		case Qt::Key_Left:
			if (QApplication::keyboardModifiers() & Qt::AltModifier)
			{
				ui.graph->ChangePositionX(-256);
			}
			else
			{
				ui.graph->ChangePositionX(-1);
			}
			break;

		case Qt::Key_Right:
			if (QApplication::keyboardModifiers() & Qt::AltModifier)
			{
				ui.graph->ChangePositionX(256);
			}
			else
			{
				ui.graph->ChangePositionX(1);
			}
			break;

		case Qt::Key_Home:
			if (QApplication::keyboardModifiers() & Qt::ControlModifier)
			{
				ui.sbarRange->setValue(ui.graph->getTrigPos());
			}
			else
			{
                ui.sbarRange->setValue(0);
			}
			break;

		case Qt::Key_End:
            if(ui.graph->getZoomOut()>1) ui.sbarRange->setValue(ui.graph->getSampleCount() - ((ui.graph->getZoomOut() * (ui.graph->getWidth() / 2))));
            else ui.sbarRange->setValue((ui.graph->getSampleCount() - (ui.graph->getWidth() / (2 * ui.graph->getZoomIn()))));
			break;

        case Qt::Key_I:
            if(QApplication::keyboardModifiers() & (Qt::AltModifier)) emit displayFwVersions();
            break;

        case Qt::Key_S:
            if(QApplication::keyboardModifiers() & (Qt::AltModifier)) emit displaySysInfo();
            break;

		default:
			break;
	}
}


/*******************************************************************************************
										Public slots
*******************************************************************************************/

void QScope::AdjustHScrollBar()
{
    // Ratio between the actual zoom and the minimal value
    qDebug() << "Adjust H Scroll bar";
    ui.graph->m_bZoomXChanged = false;
    unsigned long long MaxViewPos = ui.graph->CalculateMaxViewPos();
    int SampPerScreen = ui.graph->getSamplesPerScreen();
    double SampPerScroll;
    //qDebug() << "Adjust H Scroll bar: Sample count: " << u64SampleCount;

    ui.sbarRange->setVisible(true);
    ui.sbarRange->setEnabled(true);

    SampPerScroll = SampPerScreen / SAMP_PER_SCROLL_RATIO;
    if(SampPerScroll<64) SampPerScroll = 1;

    ui.sbarRange->blockSignals(true);
    qDebug() << "Adjust H Scroll bar: SampPerScreen: " << SampPerScreen;
    ui.graph->m_dblSampPerScroll = SampPerScroll;
    int ScrollRangeMax = MaxViewPos / ui.graph->m_dblSampPerScroll;
    ui.sbarRange->setRange(0, ScrollRangeMax);
    ui.sbarRange->setSingleStep(1);
    ui.sbarRange->setPageStep(SampPerScreen / ui.graph->m_dblSampPerScroll);
    unsigned long long CurrViewPos = ui.graph->getViewPosition();
    if(CurrViewPos>MaxViewPos) CurrViewPos = MaxViewPos;
    ui.graph->setViewPosition(CurrViewPos);
    int ScrollVal = floor(CurrViewPos / ui.graph->m_dblSampPerScroll);
    ui.sbarRange->setValue(ScrollVal);
    qDebug() << "Adjust H Scroll bar: ScrollMin: " << ui.sbarRange->minimum();
    qDebug() << "Adjust H Scroll bar: ScrollMax: " << ui.sbarRange->maximum();
    qDebug() << "Adjust H Scroll bar: ScrollVal: " << ScrollVal;
    qDebug() << "Adjust H Scroll bar: SampPerScroll: " << ui.graph->m_dblSampPerScroll;
    ui.sbarRange->blockSignals(false);

    qDebug() << "Adjust H Scroll bar: Done";
}

void QScope::s_adjustHScrollBar()
{
    AdjustHScrollBar();
}

void QScope::s_UpdateScrollBar(int Val)
{
    ui.sbarRange->blockSignals(true);
    ui.sbarRange->setValue(Val);
    ui.sbarRange->blockSignals(false);
}

void QScope::s_UpdateRawData()
{
    for (size_t i = 0; i < m_channelArray.size(); ++i)
    {
        switch(m_channelArray[i]->getChanType())
        {
            case drawchanreg:
                m_channelArray[i]->setCurrSeg(*m_i64CurrSeg);
                m_channelArray[i]->m_bDataValid = true;
                break;

            case drawchanfilesig:
                break;

            default:
                break;

        }
    }

    ui.graph->setData(m_channelArray, m_pGageSystem->getSegmentSize());

	ui.graph->m_bGageDataValid = true;

    emit updateGraph(GraphUpdatePlot);
}

void QScope::s_UpdateChannelList(int i32NbrChannels)
{
    i32NbrChannels = (m_pGageSystem->getAcquisitionMode() & CS_MASKED_MODE) * m_pGageSystem->getBoardCount();
    GetGraph()->setBoardActiveChans(i32NbrChannels);

	unsigned int ZoomIn = ui.graph->getZoomIn();
	unsigned int ZoomOut = ui.graph->getZoomOut();
	unsigned short ZoomY = ui.graph->getZoomY();
	int Width = ui.graph->getWidth();
    int Height = ui.graph->getHeight();
	ui.graph->m_bInterleavedData = m_pGageSystem->m_bInterleavedData;
    m_pGageSystem->GetMulRecAvgCount(&m_i32AvgCount);
    ui.graph->setDispChanCount(i32NbrChannels);
    ui.graph->m_bGageDataValid = false;

	if (1)//( i32NbrChannels != m_channelArray.size() )
	{
		for ( size_t i = 0; i < m_channelArray.size(); i++ )
		{
			delete m_channelArray.at(i);
		}
		m_channelArray.clear();
		for ( int i = 0; i < i32NbrChannels; i++ )
		{
            DrawableChannel* p = new DrawableChannel(m_pGageSystem, m_pGageSystem->getBuffer(i), m_pGageSystem->getSegmentSize(), drawchanreg);
			p->setZoomX(ZoomIn, ZoomOut);
			p->setZoomY(ZoomY);
			p->setWidth(Width);
            p->setHeight(Height);
            p->setChanSkip(m_pGageSystem->GetChannelSkip());
			p->setChanIdx(i);
			p->setSampleSize(m_pGageSystem->getSampleSize());
            p->setSampleRes(m_pGageSystem->getSampleRes());
            p->setSampleOffset(m_pGageSystem->getSampleOffset());
            p->setRange(m_pGageSystem->GetInputRange((i*m_pGageSystem->GetChannelSkip()) + 1));
            //p->setDcOffset(m_pGageSystem->GetDcOffset((i*m_pGageSystem->GetChannelSkip()) + 1));
			p->m_bInterleaved = ui.graph->m_bInterleavedData;
            p->setSegCount(m_pGageSystem->getSegmentCount());
            p->setDisplayOneSeg(*m_bDisplayOneSeg);
            p->setCurrSeg(*m_i64CurrSeg);
            p->SetAddrXferTime(GetGraph()->GetAddrXferTime());
            p->SetAddrXferSamples(GetGraph()->GetAddrXferSamples());
            p->setActiveChanCount(m_pGageSystem->GetActiveChannelCount());
            p->setAvgCount(m_i32AvgCount);
            p->m_bDataValid =  false;
			m_channelArray.push_back(p);
		}
		ui.graph->setData(m_channelArray, m_pGageSystem->getSegmentSize());
        ui.graph->CalculateChannelsCenter();
	}
}

void QScope::s_DisplaySystemInfo()
{
    QString QStrSysInfo;

    int i32Ret = m_pGageSystem->GetSysInfoString(m_i32BrdIdx, &QStrSysInfo);
    if(i32Ret<0)
    {
        QStrSysInfo = "Error getting the FW versions from the board.";
    }

    //qDebug() << "Info pop-up";
    QMessageBox ThisOtherMsgBox;
    QFont ThisFont;
    ThisFont.setFamily("Lucida Console");
    ThisOtherMsgBox.setFont(ThisFont);
    qDebug() << "Set text info pop-up";
    ThisOtherMsgBox.setText(QStrSysInfo);
    qDebug() << "Display info pop-up";
    ThisOtherMsgBox.exec();

    //qDebug() << "Exit";
}

void QScope::s_DisplayFwVersions()
{
    QString QStrVersionInfo;

    int i32Ret = m_pGageSystem->GetFwVersion(m_i32BrdIdx, &QStrVersionInfo);
    if(i32Ret<0)
    {
        QStrVersionInfo = "Error getting the FW versions from the board.";
    }

    //qDebug() << "Info pop-up";
    QMessageBox ThisOtherMsgBox;
    QFont ThisFont;
    ThisFont.setFamily("Lucida Console");
    ThisOtherMsgBox.setFont(ThisFont);
    qDebug() << "Set text info pop-up";
    ThisOtherMsgBox.setText(QStrVersionInfo);
    qDebug() << "Display info pop-up";
    ThisOtherMsgBox.exec();

    //qDebug() << "Exit";
}

void QScope::SetAddrDisplayOneSeg(bool* bDisplayOneSeg)
{
    m_bDisplayOneSeg = bDisplayOneSeg;
}

void QScope::SetAddrCurrSeg(int64* i64CurrSeg)
{
    m_i64CurrSeg = i64CurrSeg;
}

