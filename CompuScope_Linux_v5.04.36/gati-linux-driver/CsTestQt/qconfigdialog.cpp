#include "qconfigdialog.h"
#include "CsExpert.h"
#include "GageSystem.h"
#include <qelapsedtimer.h>
#include "qdesktopwidget.h"
#include "qthread.h"
#include "qevent.h"
#include "math.h"
#include <QVariant>
#include "CsTestFunc.h"
//#include <functional> if I use not_equal_to
//#include <algorithm>  if I use adjacent_find
#include <QDebug>

////qDebug() << "sample comment";

using namespace CsTestFunc;

const int c_DefautTriggerTimeout	= 10;
const int c_TriggerSource			= 0;
const int c_TriggerCondition		= 1;
const int c_TriggerLevel			= 2;
const char* c_DisabledString		= "----";
const int c_MaxTriggerHoldoff = 200000;// 65536;

QConfigDialog::QConfigDialog(CGageSystem* pGageSystem, CsTestErrMng* ErrMng, QWidget *parent, bool* bUpdateScopeParmsDone)
	: QDialog(parent)
{
    //qDebug() << "Config dialog constructor";
    m_bParmsLoaded = false;

	ui.setupUi(this);
	setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

    //m_bParmsChanged = false;
    m_bUpdateScopeParmsDone = bUpdateScopeParmsDone;
    *m_bUpdateScopeParmsDone = false;
    m_bAdvancedTrigState = false;

	m_pErrMng = ErrMng;

	m_pGageSystem = pGageSystem;

	m_UpdateCommitLoop = new QEventLoop(parent);
	m_UpdateCommitLoop->processEvents(QEventLoop::AllEvents);
	m_UpdateCommitLoop->blockSignals(false);

    ui.sbxDCOffset->setMaximum(100);
    ui.sbxDCOffset->setMinimum(-100);

	ui.spbxTriggerLevel->setMaximum(100);
	ui.spbxTriggerLevel->setMinimum(-100);


	ui.spinBox_AvgCount->setMaximum(10000000);
	ui.spinBox_AvgCount->setMinimum(1);

    ui.txtExternalClock->setEnabled(true);
 
 	m_LastPos = QPoint(-1, -1);

    ui.dblSpinBox_CurrSeg->setSpecialValueText("All segments");

    ui.dblSpinBox_TriggerTimeout->setSpecialValueText("Infinite");

    //ui.txtNbrRecords->setText(QString::number(m_AcqParams.u32RecordCount));
    //bool bFlag = ( 1 == m_AcqParams.u32RecordCount ) ? false : true;
    //ui.ckbNbrRecords->setChecked(bFlag);
    //ui.txtNbrRecords->setEnabled(bFlag);
	ui.checkBox_DisplaySingleSeg->setVisible(false);
	ui.dblSpinBox_CurrSeg->setVisible(false);
    ui.checkBox_DisplaySingleSeg->setEnabled(false);
    ui.dblSpinBox_CurrSeg->setEnabled(false);
    ui.dblSpinBox_CurrSeg->setValue(0);

}

QConfigDialog::~QConfigDialog()
{
	delete	m_UpdateCommitLoop;
}

void QConfigDialog::s_ShowConfigWindow(CGageSystem* pGageSystem)
{
    qDebug() << "s_ShowConfigWindow";

    if (!isVisible())
    {
        ui.tabWidget->setCurrentIndex(0); // set to show acquisition page at startup
        if(m_LastPos!=QPoint(-1, -1)) move(m_LastPos);
        setVisible(true);
    }

  //  hide();
    show();
    setModal(true);

    s_loadParms(pGageSystem);
}

void QConfigDialog::s_loadParms(CGageSystem* pGageSystem)
{
    qDebug() << "Config: load parms";
    m_bParmsLoaded = false;
    m_MutexSharedVars->lock();
    *m_bParmsChanged = false;
    m_MutexSharedVars->unlock();

    m_pGageSystem = pGageSystem;

    LoadUi();

    m_bParmsLoaded = true;
}

void QConfigDialog::LoadUi()
{
    // figure out what controls to disable or hide
    //qDebug() << "Populate UI";
    CheckAvailabiity();

    //qDebug() << "Get parameters";
    GetAcquisitionParameters();
    GetChannelParameters();
    GetTriggerParameters();

    setWindowTitle(QString(m_pGageSystem->GetBoardName()) + " Configure");

    vector<ModeNames> names = m_pGageSystem->GetAcquisitionModeNames();

    // make sure combo box is empty first
    ui.cbxMode->clear();
    for ( size_t i = 0; i < names.size(); i++ )
    {
        ui.cbxMode->addItem(QString(names[i].strName.c_str()), QVariant::fromValue(names[i].u32Mode));
    }

    int AcqMode = m_pGageSystem->getAcquisitionMode();

    if (m_pGageSystem->GetExpertImg1() == EXPERT_NONE)
    {
        ui.ckbExpert1->setDisabled(true);
        ui.ckbExpert1->setText("Expert1 (None)");
    }
    else
    {
        ui.ckbExpert1->setDisabled(false);
        ui.ckbExpert1->setText(m_pGageSystem->GetExpertImg1Name());
    }

    if (m_pGageSystem->GetExpertImg2() == EXPERT_NONE)
    {
        ui.ckbExpert2->setDisabled(true);
        ui.ckbExpert2->setText("Expert2 (None)");
    }
    else
    {
        ui.ckbExpert2->setDisabled(false);
        ui.ckbExpert2->setText(m_pGageSystem->GetExpertImg2Name());
    }

    if (((m_pGageSystem->GetExpertImg1() == EXPERT_MULREC_AVG_TD) && (AcqMode & CS_MODE_USER1)) || ((m_pGageSystem->GetExpertImg2() == EXPERT_MULREC_AVG_TD) && (AcqMode & CS_MODE_USER2)))
    {
        ui.spinBox_AvgCount->setDisabled(false);
    }
    else
    {
        ui.spinBox_AvgCount->setDisabled(true);
    }
    ui.spinBox_AvgCount->setValue(m_pGageSystem->getMulRecAvgCount());

    SampleRateTable* pRateTable;
    pRateTable = m_pGageSystem->GetSampleRateTable(m_AcqParams.u32Mode & CS_MASKED_MODE);

    // make sure combo box is empty first
    ui.cbxSampleRate->clear();

    for ( int i = 0; i < pRateTable->nItemCount; i++ )
    {
        ui.cbxSampleRate->addItem(pRateTable->pSRTable[i].strText, (long long)pRateTable->pSRTable[i].i64SampleRate);
    }

    if (m_pGageSystem->hasExternalClock())
    {

        //double MinExtRate = m_pGageSystem->getMinExtRate();
        //double MaxExtRate = m_pGageSystem->getMaxExtRate();
        //QDoubleValidator* dblExtClkValidator = new QDoubleValidator(MinExtRate, MaxExtRate, 0, 0);
        //ui.txtExternalClock->setValidator(dblExtClkValidator);
    }
    else
    {
        ui.ckbExternalClock->setChecked(false);
        ui.ckbExternalClock->setDisabled(true);
    }

    //qDebug() << "Set acquisition parms";
	UpdateUiAcquisitionParameters();
    //qDebug() << "Set acquisition parms done";

    //qDebug() << "Set pre-trig";
// RG    int maxPreTrig = c_MaxTriggerHoldoff / (m_pGageSystem->getAcquisitionMode() & CS_MASKED_MODE);
// RG    QIntValidator* validHoldoff = new QIntValidator(0, maxPreTrig, ui.txtPreTrigger);
    //ui.txtPreTrigger->setValidator( validHoldoff );

    //qDebug() << "Set post-trig";
    int64 MaxMem = m_pGageSystem->GetMemorySize();
    double maxPostTrig = floor((((double)m_pGageSystem->GetMemorySize()) / (m_pGageSystem->getAcquisitionMode() & CS_MASKED_MODE)) + 1);
    maxPostTrig -= m_pGageSystem->getTriggerHoldoff();
//	maxPostTrig /= ui.txtNbrRecords->data().toUInt();
    maxPostTrig /= ui.txtNbrRecords->text().toUInt();
    // RG check for multiple of trigger res
    QDoubleValidator* dblvalidator = new QDoubleValidator(0, maxPostTrig, 0, ui.txtPostTrigger);
    ui.txtPostTrigger->setValidator( dblvalidator );

    //ui.txtPreTrigger->setValidator( new QIntValidator(0,c_MaxTriggerHoldoff / (m_AcqParams.u32Mode & CS_MASKED_MODE)) );

    //qDebug() << "Set Nbr rec";
    QIntValidator* valNbrRecords = new QIntValidator();
    int32 MaxSegs = (int32)floor((((double)MaxMem) / MIN_SEG_SIZE) + 0.5);
    valNbrRecords->setRange(0, MaxSegs);
    ui.txtNbrRecords->setValidator(valNbrRecords);
    //ui.spinBox_CurrSegNb->setMinimum(0);
    //ui.spinBox_CurrSegNb->setMaximum(MaxSegs);

    //qDebug() << "Set trig timeout";
    QIntValidator* valTriggerTimeOut = new QIntValidator();
    valTriggerTimeOut->setRange(0, INT_MAX); //TODO:: Set the good max value
    //ui.dblSpinBox_TriggerTimeout->setValidator(valTriggerTimeOut);

	SetChannelTabUI();
	SetTriggerTabUI();

    //QWidget* m = NULL;
    foreach(QWidget *widget, qApp->topLevelWidgets())
    {
        if(widget->inherits("QMainWindow"))
        {
            //m = widget;
            break;
        }
    }
}

// ***************************************************************************
// Channel Tab UI fucntions
// ***************************************************************************

void QConfigDialog::SetChannelTabUI()
{
	//qDebug() << "Start setting channels";
	/********** Channel *********************/
	uInt32 u32ChannelSkip = m_pGageSystem->GetChannelSkip();
	uInt32 u32ChannelCount = m_pGageSystem->getChannelCount();
	uInt32	u32ActiveChannelCount(0);

	ui.cbxChannelIndex->clear();
	for (uInt32 i = 1; i <= u32ChannelCount; i += u32ChannelSkip)
	{
		ui.cbxChannelIndex->addItem("Channel " + QString::number(i), QVariant::fromValue(i));
		u32ActiveChannelCount++;
	}

	// Check if channel parameters are the same for all channels. If they are
	// we can check the All Channels checkbox, otherwise make sure it's unchecked.
	bool bValuesAreEqual = true;

	//qDebug() << "Set channels vector";
	for (int i = 0; i < m_vecChanParams.size(); i += u32ChannelSkip)
	{
		if ((m_vecChanParams[0].bFilter != m_vecChanParams[i].bFilter) ||
			(m_vecChanParams[0].i32DcOffset != m_vecChanParams[i].i32DcOffset) ||
			(m_vecChanParams[0].u32Coupling != m_vecChanParams[i].u32Coupling) ||
			(m_vecChanParams[0].u32Impedance != m_vecChanParams[i].u32Impedance) ||
			(m_vecChanParams[0].u32InputRange != m_vecChanParams[i].u32InputRange))
		{
			bValuesAreEqual = false;
			break;
		}
	}
	//qDebug() << "Set channels vector done";

	// more c++ way of doing it, but I need to find out how to use a structure in not_equal_to
	//	bool bValuesAreEqual = adjacent_find( m_vecChanParams.begin(), m_vecChanParams.end(), not_equal_to<ChannelParameters>() ) == m_vecChanParams.end();

	ui.ckbAllChannels->blockSignals(true);
	ui.ckbAllChannels->setChecked(bValuesAreEqual);
	if (u32ActiveChannelCount > 1)
		ui.ckbAllChannels->show();
	else
		ui.ckbAllChannels->hide();
	ui.ckbAllChannels->blockSignals(false);

	ui.cbxChannelIndex->setEnabled(!bValuesAreEqual);
	ui.cbxChannelIndex->setCurrentIndex(0);
	//InputRangeTable* pIRTable = m_pGageSystem->GetInputRangeTable(m_vecChanParams[0].u32Impedance);

	//qDebug() << "Set input ranges";
	PopulateRangeComboBox();

	//int32 i32MaxOffset = ui.cbxInputRange->currentData().toInt() / 2;
	int32 i32MaxOffset = 100;
	ui.sbxDCOffset->setRange(-i32MaxOffset, i32MaxOffset);

	//qDebug() << "Set channel parms";
	SetChannelParameters();
	//qDebug() << "Set channel parms done";
}

// ***************************************************************************
// Trigger Tab UI fucntions
// ***************************************************************************

void QConfigDialog::SetTriggerTabUI()
{
	uInt32 u32ChannelSkip	= m_pGageSystem->GetChannelSkip();
	uInt32 u32ChannelCount	= m_pGageSystem->getChannelCount();
	int AcqMode				= m_pGageSystem->getAcquisitionMode();

	ui.cbxTriggerSource->clear();
	for (uInt32 i = 1; i <= u32ChannelCount; i += u32ChannelSkip)
	{
		ui.cbxTriggerSource->addItem("Channel " + QString::number(i), QVariant::fromValue(i));
	}

	if (m_pGageSystem->hasExternalTrigger())
	{
		ui.cbxTriggerSource->addItem("External", QVariant::fromValue(-1));
	}
	ui.cbxTriggerSource->addItem("Disable", QVariant::fromValue(0));

	ui.cbxTriggerCondition->clear();
	ui.cbxTriggerCondition->addItem("Rising", QVariant::fromValue(CS_TRIG_COND_POS_SLOPE));
	ui.cbxTriggerCondition->addItem("Falling", QVariant::fromValue(CS_TRIG_COND_NEG_SLOPE));


	//qDebug() << "Set trigger parms";
	SetTriggerParameters(AcqMode);
}


void QConfigDialog::CheckAvailabiity()
{
	QueryCaptureCaps();
	QueryChannelCaps();
    //QueryTriggerCaps();
}

void QConfigDialog::QueryCaptureCaps()
{
	uInt32 u32ModesSupported = m_pGageSystem->getModesSupported(); 

	bool bFlag = ( u32ModesSupported & CS_MODE_REFERENCE_CLK ) ? true : false;
	ui.ckbReferenceClock->setEnabled(bFlag);

	bFlag = ( u32ModesSupported & CS_MODE_USER1 ) ? true : false;
	ui.ckbExpert1->setEnabled(bFlag);

	bFlag = ( u32ModesSupported & CS_MODE_USER2 ) ? true : false;
	ui.ckbExpert2->setEnabled(bFlag);

	bFlag = m_pGageSystem->hasMultipleRecord() ? true : false;
	ui.ckbNbrRecords->setEnabled(bFlag);

	// Find out if external clock is available
	if ( !m_pGageSystem->hasExternalClock() )
	{
        //ui.stackedWidget->setCurrentIndex(0);
		ui.ckbExternalClock->setEnabled(false);
	}
	else 
	{
		// set stacked widget to either sample rate combobox or ext clock edit field
        //ui.stackedWidget->setCurrentIndex( m_pGageSystem->getExtClockStatus() ? 1 : 0 );
	}
}

void QConfigDialog::QueryChannelCaps()
{
	bool bFlag = m_pGageSystem->hasDcOffset() ? true : false;
	ui.sbxDCOffset->setEnabled(bFlag);

	bFlag = m_pGageSystem->hasCouplingAC() ? true : false;
	ui.rbtnChannelAC->setEnabled(bFlag);

	bFlag = m_pGageSystem->hasCouplingDC() ? true : false;
	ui.rbtnChannelDC->setEnabled(bFlag);

	bFlag = m_pGageSystem->hasImpedance1M() ? true : false;
	ui.rbtnChannelOneMegaOhms->setEnabled(bFlag);

	bFlag = m_pGageSystem->hasImpedance50() ? true : false;
	ui.rbtnChannelFiftyOhms->setEnabled(bFlag);

	bFlag = m_pGageSystem->hasFilter() ? true : false;
    ui.radioButton_Filter->setEnabled(bFlag);

}

void QConfigDialog::QueryTriggerCaps()
{
	bool bFlag = m_pGageSystem->hasTriggerCouplingAC();
    ui.rbtnTriggerAC->setEnabled(bFlag);
    ui.rbtnTriggerAC->setAutoExclusive(false);

	bFlag = m_pGageSystem->hasTriggerCouplingDC();
    ui.rbtnTriggerDC->setEnabled(bFlag);
    ui.rbtnTriggerDC->setAutoExclusive(false);

	bFlag = m_pGageSystem->hasTriggerImpedance50();
    ui.rbtnTriggerFiftyOhm->setEnabled(bFlag);
    ui.rbtnTriggerFiftyOhm->setAutoExclusive(false);

	bFlag = m_pGageSystem->hasTriggerImpedanceHZ();
    ui.rbtnTriggerHighZ->setEnabled(bFlag);
    ui.rbtnTriggerHighZ->setAutoExclusive(false);

	bFlag = m_pGageSystem->hasTriggerRange1V();
    ui.rbtnTriggerOneVolt->setEnabled(bFlag);
    ui.rbtnTriggerOneVolt->setAutoExclusive(false);

	bFlag = m_pGageSystem->hasTriggerRange5V();
    ui.rbtnTriggerFiveVolt->setEnabled(bFlag);
    ui.rbtnTriggerFiveVolt->setAutoExclusive(false);

    bFlag = m_pGageSystem->hasTriggerRange3V3();
    ui.rbtnTrigger3V3->setEnabled(bFlag);
    ui.rbtnTrigger3V3->setAutoExclusive(false);

    bFlag = m_pGageSystem->hasTriggerRangePlus5V();
    ui.rbtnTriggerPlus5V->setEnabled(bFlag);
    ui.rbtnTriggerPlus5V->setAutoExclusive(false);
}

void QConfigDialog::EnableExternalTriggerParms(bool bEnable)
{
    if(bEnable)
    {
        ui.boxExternalTriggerParameters->setEnabled(true);
        bool bFlag = m_pGageSystem->hasTriggerCouplingAC();
        ui.rbtnTriggerAC->setEnabled(bFlag);
        ui.rbtnTriggerAC->setAutoExclusive(false);

        bFlag = m_pGageSystem->hasTriggerCouplingDC();
        ui.rbtnTriggerDC->setEnabled(bFlag);
        ui.rbtnTriggerDC->setAutoExclusive(false);

        bFlag = m_pGageSystem->hasTriggerImpedance50();
        ui.rbtnTriggerFiftyOhm->setEnabled(bFlag);
        ui.rbtnTriggerFiftyOhm->setAutoExclusive(false);

        bFlag = m_pGageSystem->hasTriggerImpedanceHZ();
        ui.rbtnTriggerHighZ->setEnabled(bFlag);
        ui.rbtnTriggerHighZ->setAutoExclusive(false);

        bFlag = m_pGageSystem->hasTriggerRange1V();
        ui.rbtnTriggerOneVolt->setEnabled(bFlag);
        ui.rbtnTriggerOneVolt->setAutoExclusive(false);

        bFlag = m_pGageSystem->hasTriggerRange5V();
        ui.rbtnTriggerFiveVolt->setEnabled(bFlag);
        ui.rbtnTriggerFiveVolt->setAutoExclusive(false);

        bFlag = m_pGageSystem->hasTriggerRange3V3();
        ui.rbtnTrigger3V3->setEnabled(bFlag);
        ui.rbtnTrigger3V3->setAutoExclusive(false);

        bFlag = m_pGageSystem->hasTriggerRangePlus5V();
        ui.rbtnTriggerPlus5V->setEnabled(bFlag);
        ui.rbtnTriggerPlus5V->setAutoExclusive(false);
    }
    else
    {
        ui.boxExternalTriggerParameters->setEnabled(false);
        ui.rbtnTriggerAC->setEnabled(false);
        ui.rbtnTriggerDC->setEnabled(false);
        ui.rbtnTriggerFiftyOhm->setEnabled(false);
        ui.rbtnTriggerHighZ->setEnabled(false);
        ui.rbtnTriggerOneVolt->setEnabled(false);
        ui.rbtnTriggerFiveVolt->setEnabled(false);
        ui.rbtnTrigger3V3->setEnabled(false);
        ui.rbtnTriggerPlus5V->setEnabled(false);
    }
}

void QConfigDialog::GetAcquisitionParameters()
{
    //qDebug() << "Get acquisition parameters";
	m_AcqParams.u32Mode = m_pGageSystem->getAcquisitionMode();
    m_u32AcqMode = m_AcqParams.u32Mode;
    m_u32ChanSkip = m_pGageSystem->GetChannelSkip();
	m_AcqParams.bExternalClock = ( 0 == m_pGageSystem->getExtClockStatus() ) ? false : true;
	m_AcqParams.bReferenceClock = ( m_AcqParams.u32Mode & CS_MODE_REFERENCE_CLK ) ? true : false;
	m_AcqParams.bExpert1 = ( m_AcqParams.u32Mode & CS_MODE_USER1 ) ? true : false;
	m_AcqParams.bExpert2 = ( m_AcqParams.u32Mode & CS_MODE_USER2 ) ? true : false;
	m_i32AvgCount = m_pGageSystem->getMulRecAvgCount();
	m_AcqParams.i64SampleRate = m_pGageSystem->getSampleRate();
    m_AcqParams.u32RecordCount = m_pGageSystem->getSegmentCount();
	m_AcqParams.i64PostTrigger = m_pGageSystem->getDepth();
	m_AcqParams.i64Pretrigger = m_pGageSystem->getSegmentSize() - m_pGageSystem->getDepth();//m_pGageSystem->getTriggerHoldoff();
    m_AcqParams.i64TriggerDelay = m_pGageSystem->getTriggerDelay();
    m_AcqParams.i64TriggerTimeout = m_pGageSystem->getTriggerTimeout(); // RG convert to us
}

void QConfigDialog::UpdateUiAcquisitionParameters()
{
    //qDebug() << "Set acquisition parameters";

	int currentSelection = ui.cbxMode->findData(QVariant::fromValue(m_AcqParams.u32Mode & (CS_MASKED_MODE | CS_MODE_SINGLE_CHANNEL2)));
	if ( -1 == currentSelection )
	{
		currentSelection = 0;
	}
	ui.cbxMode->setCurrentIndex(currentSelection);

	int AcqMode = m_pGageSystem->getAcquisitionMode();

	if (((m_pGageSystem->GetExpertImg1() == EXPERT_MULREC_AVG_TD) && (AcqMode & CS_MODE_USER1)) || ((m_pGageSystem->GetExpertImg2() == EXPERT_MULREC_AVG_TD) && (AcqMode & CS_MODE_USER2)))
	{
		ui.spinBox_AvgCount->setDisabled(false);
	}
	else
	{
		ui.spinBox_AvgCount->setDisabled(true);
	}
	ui.spinBox_AvgCount->setValue(m_pGageSystem->getMulRecAvgCount());

    ui.ckbReferenceClock->setCheckState(m_AcqParams.bReferenceClock ? Qt::Checked : Qt::Unchecked);
    ui.ckbExpert1->setCheckState(m_AcqParams.bExpert1 ? Qt::Checked : Qt::Unchecked);
    ui.ckbExpert2->setCheckState(m_AcqParams.bExpert2 ? Qt::Checked : Qt::Unchecked);
	ui.spinBox_AvgCount->setValue(m_i32AvgCount);

    ui.ckbExternalClock->setCheckState(m_AcqParams.bExternalClock ? Qt::Checked : Qt::Unchecked);
	ui.ckbReferenceClock->setCheckState(m_AcqParams.bReferenceClock ? Qt::Checked : Qt::Unchecked);

	SampleRateTable* pRateTable;
	pRateTable = m_pGageSystem->GetSampleRateTable(m_AcqParams.u32Mode & CS_MASKED_MODE);

	// make sure combo box is empty first
	ui.cbxSampleRate->clear();

	for (int i = 0; i < pRateTable->nItemCount; i++)
	{
        ui.cbxSampleRate->addItem(pRateTable->pSRTable[i].strText, (long long)pRateTable->pSRTable[i].i64SampleRate);
	}

	if ( m_AcqParams.bExternalClock )
	{
        ui.txtExternalClock->setVisible(true);
        ui.cbxSampleRate->setVisible(false);
		QString strExtClk;
		strExtClk.setNum(m_AcqParams.i64SampleRate);
		ui.txtExternalClock->setText(strExtClk);
		ui.label_SampleRate->setText("Sample Rate (Hz)");
	}
	else
	{
        ui.txtExternalClock->setVisible(false);
        ui.cbxSampleRate->setVisible(true);
		currentSelection = ui.cbxSampleRate->findData(QVariant::fromValue(m_AcqParams.i64SampleRate));
		if ( -1 == currentSelection )
		{
			currentSelection = 0;
		}
		ui.cbxSampleRate->setCurrentIndex(currentSelection);
		ui.label_SampleRate->setText("Sample Rate");
	}

	ui.txtNbrRecords->setText(QString::number(m_AcqParams.u32RecordCount));
	bool bFlag = ( 1 == m_AcqParams.u32RecordCount ) ? false : true;
    ui.ckbNbrRecords->setCheckState(bFlag ? Qt::Checked : Qt::Unchecked);
    ui.txtNbrRecords->setEnabled(bFlag);

    if(*m_bDisplayOneSeg)
    {
        ui.checkBox_DisplaySingleSeg->setChecked(true);
        ui.dblSpinBox_CurrSeg->setEnabled(true);
        ui.dblSpinBox_CurrSeg->setValue(*m_i64CurrSeg);
    }
    else
    {
        ui.checkBox_DisplaySingleSeg->setChecked(false);
        ui.dblSpinBox_CurrSeg->setEnabled(false);
        ui.dblSpinBox_CurrSeg->setValue(0);
    }

    ui.txtPreTrigger->setText(QString::number(m_AcqParams.i64Pretrigger));
    if(m_AcqParams.i64TriggerDelay>0) ui.txtPreTrigger->setText(QString::number(-1 * m_AcqParams.i64TriggerDelay));
	ui.txtPostTrigger->setText(QString::number(m_AcqParams.i64PostTrigger));

	if ( m_AcqParams.i64TriggerTimeout < 0 )
	{
        //qDebug() << "Trigger timeout negative" << m_AcqParams.i64TriggerTimeout;
        ui.dblSpinBox_TriggerTimeout->setValue(-1);
        ui.dblSpinBox_TriggerTimeout->setEnabled(false);
        ui.ckbTriggerTimeOut->setCheckState(Qt::Unchecked);
	}
	else 
	{
        //qDebug() << "Trigger timeout" << m_AcqParams.i64TriggerTimeout;
        ui.dblSpinBox_TriggerTimeout->setValue(m_AcqParams.i64TriggerTimeout / 10.0);
        ui.dblSpinBox_TriggerTimeout->setEnabled(true);
        ui.ckbTriggerTimeOut->setCheckState(Qt::Checked);
	}
}

void QConfigDialog::GetChannelParameters()
{
    //qDebug() << "Get channel parameters";
    uInt32 u32ChannelCount = m_pGageSystem->getChannelCount();

	m_vecChanParams.clear();
	for ( uInt32 u32Chan = 1;u32Chan <= u32ChannelCount; u32Chan++ )
	{
		ChannelParameters cp;
        qDebug() << "Get chan parms: CH" << u32Chan;
		cp.u32InputRange = m_pGageSystem->GetInputRange(u32Chan);
        qDebug() << "Get chan parms: Range" << cp.u32InputRange;
		cp.u32Impedance = m_pGageSystem->GetImpedance(u32Chan);
        qDebug() << "Get chan parms: Impedance" << cp.u32Impedance;
		cp.u32Coupling = m_pGageSystem->GetCoupling(u32Chan);
        cp.i32DcOffset = m_pGageSystem->GetDcOffset(u32Chan);
        qDebug() << "Get chan parms: DC Offset: " << cp.i32DcOffset;
		cp.bFilter = ( 0 != m_pGageSystem->GetFilter(u32Chan) );
        qDebug() << "Filter: " << cp.bFilter;
		m_vecChanParams.push_back(cp);
	}

    //qDebug() << "Get channel parameters: Done";
}

void QConfigDialog::SetChannelParameters()
{
    //qDebug() << "Set channel parameters";

	uInt32 u32Chan;
    int CurrIdx;
	if ( ui.ckbAllChannels->isChecked() )
	{
		u32Chan = 1;
	}
	else
	{
        CurrIdx = ui.cbxChannelIndex->currentIndex();
        u32Chan = ui.cbxChannelIndex->itemData(CurrIdx, Qt::UserRole).toInt();
	}
    //qDebug() << "Set channel parameters: Channel" << u32Chan;

	int index = u32Chan - 1;

	if ( m_vecChanParams.at(index).u32Coupling & CS_COUPLING_AC )
	{
        ui.rbtnChannelAC->setChecked(true);
        ui.rbtnChannelDC->setChecked(false);
	}
	else
	{
        ui.rbtnChannelAC->setChecked(false);
        ui.rbtnChannelDC->setChecked(true);
	}

	if ( m_vecChanParams.at(index).u32Impedance == CS_REAL_IMP_1M_OHM )
	{
        ui.rbtnChannelOneMegaOhms->setChecked(true);
        ui.rbtnChannelFiftyOhms->setChecked(false);
	}
	else
	{
        ui.rbtnChannelOneMegaOhms->setChecked(false);
        ui.rbtnChannelFiftyOhms->setChecked(true);
	}

	int currentSelection = ui.cbxInputRange->findData(QVariant::fromValue(m_vecChanParams.at(index).u32InputRange));
	if ( -1 == currentSelection )
	{
		currentSelection = 0;
	}
	ui.cbxInputRange->setCurrentIndex(currentSelection);

    bool bFilter = m_vecChanParams.at(index).bFilter;
    ui.radioButton_Filter->setChecked(bFilter);

    int InputRange = m_vecChanParams.at(index).u32InputRange;
    //qDebug() << "Set chan parms: Input range: " << InputRange;
    int DcOffsetVolt = m_vecChanParams.at(index).i32DcOffset;
    //qDebug() << "Set chan parms: DC offset volt: " << DcOffsetVolt;
    int DcOffsetPerc = floor((((DcOffsetVolt / (InputRange / 2.0)) * 100)) + 0.5);
    //qDebug() << "Set chan parms: DC Offset(%): " << DcOffsetPerc;
    ui.sbxDCOffset->setValue(DcOffsetPerc);
}

void QConfigDialog::GetTriggerParameters()
{
    //qDebug() << "Get trigger parameters";

	uInt32	nTriggerEnableCount = 0;
	uInt32	u32TriggerCount = m_pGageSystem->getTriggerCount();

	m_vecTrigParams.clear();
	m_vecSuccessTrigParams.clear(); 
	for ( uInt32 i = 0, u32Trig = 1; i < u32TriggerCount; i++, u32Trig++ )
	{
		TriggerParameters tp;
		tp.i32Source	= m_pGageSystem->GetSource(u32Trig);
		tp.i32Level		= m_pGageSystem->GetLevel(u32Trig);
		tp.u32Condition = m_pGageSystem->GetCondition(u32Trig);
		tp.u32Range		= m_pGageSystem->GetExtTriggerRange(u32Trig);
		tp.u32ExtCoupling	= m_pGageSystem->GetExtCoupling(u32Trig);
        tp.u32ExtImpedance	= m_pGageSystem->GetExtImpedance(u32Trig);
		m_vecTrigParams.push_back(tp);
		m_vecSuccessTrigParams.push_back(tp);

		if (0 != tp.i32Source)
			nTriggerEnableCount++;

        qDebug() << "Trigger engine #" << u32Trig << ", " << tp.u32Range;
	}

	ui.tableAdvancedTriggering->setVisible(nTriggerEnableCount > 1);
	ui.ckbAdvanceTriggering->setChecked(nTriggerEnableCount > 1);
	s_EnableAdvanceTriggering(0);

    //qDebug() << "Get trigger parameters: Done";
}

void QConfigDialog::SetTriggerParameters(uInt32 u32Mode)
{
	qDebug() << "Set trigger parameters";

	if (!ui.ckbAdvanceTriggering->isChecked())
	{
        SetTriggerControls(0);
        ChangeTriggerSource(1);
	}
	else
	{
		// make sure the table is empty
        ui.tableAdvancedTriggering->clearContents();

		if (!ui.tableAdvancedTriggering->isVisible())
		{
			ui.tableAdvancedTriggering->setVisible(true);
		}

		for (int i = 0; i < ui.tableAdvancedTriggering->rowCount(); i++)
		{
            qDebug() << "Set trigger parameters: Source: " << m_vecTrigParams[i].i32Source;

			if (CS_TRIG_SOURCE_DISABLE == m_vecTrigParams[i].i32Source)
			{
				QTableWidgetItem* item1 = new QTableWidgetItem("Disable");
                item1->setData(Qt::UserRole, (int)m_vecTrigParams[i].i32Source);
				ui.tableAdvancedTriggering->setItem(i, c_TriggerSource, item1);
				QTableWidgetItem* item2 = new QTableWidgetItem(c_DisabledString);
				item2->setData(Qt::UserRole, QVariant::fromValue(m_vecTrigParams[i].u32Condition));
				ui.tableAdvancedTriggering->setItem(i, c_TriggerCondition, item2);
				QTableWidgetItem* item3 = new QTableWidgetItem(c_DisabledString);
                item3->setData(Qt::UserRole, (int)m_vecTrigParams[i].i32Level); // RG not sure that I need this
				ui.tableAdvancedTriggering->setItem(i, c_TriggerLevel, item3);
			}
			else if (CS_TRIG_SOURCE_EXT == m_vecTrigParams[i].i32Source)
			{
				QTableWidgetItem* item = new QTableWidgetItem("External");
				item->setData(Qt::UserRole, (int)m_vecTrigParams[i].i32Source);
				ui.tableAdvancedTriggering->setItem(i, c_TriggerSource, item);
				ui.tableAdvancedTriggering->setItem(i, c_TriggerCondition, new QTableWidgetItem((CS_TRIG_COND_NEG_SLOPE == m_vecTrigParams[i].u32Condition) ? "Falling" : "Rising"));
				ui.tableAdvancedTriggering->setItem(i, c_TriggerLevel, new QTableWidgetItem(QString::number(m_vecTrigParams[i].i32Level)));
			}
			else // trigger source is a channel
			{
                if(m_pGageSystem->IsValidTriggerSource(m_vecTrigParams[i].i32Source, u32Mode))
                {
                    QTableWidgetItem* item1 = new QTableWidgetItem("Channel " + QString::number(m_vecTrigParams[i].i32Source));
                    item1->setData(Qt::UserRole, (int)m_vecTrigParams[i].i32Source);
                    ui.tableAdvancedTriggering->setItem(i, c_TriggerSource, item1);
                    QTableWidgetItem* item2 = new QTableWidgetItem((CS_TRIG_COND_NEG_SLOPE == m_vecTrigParams[i].u32Condition) ? "Falling" : "Rising");
                    item2->setData(Qt::UserRole, QVariant::fromValue(m_vecTrigParams[i].u32Condition));
                    ui.tableAdvancedTriggering->setItem(i, c_TriggerCondition, item2);
                    QTableWidgetItem* item3 = new QTableWidgetItem(QString::number(m_vecTrigParams[i].i32Level));
                    item3->setData(Qt::UserRole, (int)m_vecTrigParams[i].i32Level);  // RG Do I need this ?? RG RG
                    ui.tableAdvancedTriggering->setItem(i, c_TriggerLevel, item3);
                }
                else
                {
                    QTableWidgetItem* item1 = new QTableWidgetItem("Disable");
                    item1->setData(Qt::UserRole, (int)m_vecTrigParams[i].i32Source);
                    ui.tableAdvancedTriggering->setItem(i, c_TriggerSource, item1);
                    QTableWidgetItem* item2 = new QTableWidgetItem(c_DisabledString);
                    item2->setData(Qt::UserRole, QVariant::fromValue(m_vecTrigParams[i].u32Condition));
                    ui.tableAdvancedTriggering->setItem(i, c_TriggerCondition, item2);
                    QTableWidgetItem* item3 = new QTableWidgetItem(c_DisabledString);
                    item3->setData(Qt::UserRole, (int)m_vecTrigParams[i].i32Level);
                    ui.tableAdvancedTriggering->setItem(i, c_TriggerLevel, item3);
                }
			}
		}

        ui.tableAdvancedTriggering->blockSignals(true);
        ui.tableAdvancedTriggering->selectRow(0);
        ui.tableAdvancedTriggering->blockSignals(false);
        SetTriggerControls(0);
	}

    qDebug() << "Set trigger parameters: Done";
}

void QConfigDialog::SetTriggerControls(uInt32 idxTrig)
{
    int CurrSource = m_vecTrigParams[idxTrig].i32Source;
    //qDebug() << "Set trigger parameters: Current source: " << CurrSource;
    int currentSelection = ui.cbxTriggerSource->findData(QVariant::fromValue(m_vecTrigParams[0].i32Source));
    //qDebug() << "Set trigger parameters: Current selection: " << currentSelection;
    if (-1 == currentSelection)
    {
        currentSelection = 0;
    }
    ui.cbxTriggerSource->setCurrentIndex(currentSelection);

    ui.spbxTriggerLevel->setValue(m_vecTrigParams[idxTrig].i32Level);

    currentSelection = ui.cbxTriggerCondition->findData(QVariant::fromValue(m_vecTrigParams[idxTrig].u32Condition));
    if (-1 == currentSelection)
    {
        currentSelection = 0;
    }
    ui.cbxTriggerCondition->setCurrentIndex(currentSelection);

    if(CurrSource==CS_TRIG_SOURCE_EXT) EnableExternalTriggerParms(true);
    else EnableExternalTriggerParms(false);
}

void QConfigDialog::ChangeTriggerSource(uInt32 u32Engine /* = 1*/)
{
    qDebug() << "ChangeTriggerSource";
	//Source: Disable
    int CurrIdx, SelTrigSrc;
    CurrIdx = ui.cbxTriggerSource->currentIndex();
    m_vecTrigParams[u32Engine-1].i32Source = ui.cbxTriggerSource->itemData(CurrIdx, Qt::UserRole).toInt();

    CurrIdx = ui.cbxTriggerSource->currentIndex();
    SelTrigSrc = ui.cbxTriggerSource->itemData(CurrIdx, Qt::UserRole).toInt();

    qDebug() << "ChangeTriggerSource: Trig source: " << SelTrigSrc;
    if ( CS_TRIG_SOURCE_DISABLE == SelTrigSrc )
	{
        ui.boxExternalTriggerParameters->setEnabled(false);
		ui.cbxTriggerCondition->setCurrentIndex(ui.cbxTriggerCondition->findData(QVariant::fromValue(CS_TRIG_COND_POS_SLOPE)));
		ui.cbxTriggerCondition->setEnabled(false);
		ui.spbxTriggerLevel->setValue(0);
		ui.spbxTriggerLevel->setEnabled(false);
	}
	//Source: External 1
    if ( CS_TRIG_SOURCE_EXT == SelTrigSrc )
	{
        qDebug() << "ChangeTriggerSource: External";
        ui.boxExternalTriggerParameters->setEnabled(true);
		ui.cbxTriggerCondition->setEnabled(true);
		ui.spbxTriggerLevel->setEnabled(true);

		if (!m_pGageSystem->hasTriggerRange1V()) ui.rbtnTriggerOneVolt->setEnabled(false);
        else ui.rbtnTriggerOneVolt->setEnabled(true);
		if (!m_pGageSystem->hasTriggerRange5V()) ui.rbtnTriggerFiveVolt->setEnabled(false);
        else ui.rbtnTriggerFiveVolt->setEnabled(true);
        if (!m_pGageSystem->hasTriggerRange3V3()) ui.rbtnTrigger3V3->setEnabled(false);
        else ui.rbtnTrigger3V3->setEnabled(true);
        if (!m_pGageSystem->hasTriggerRangePlus5V()) ui.rbtnTriggerPlus5V->setEnabled(false);
        else ui.rbtnTriggerPlus5V->setEnabled(true);
		if (!m_pGageSystem->hasTriggerCouplingAC()) ui.rbtnTriggerAC->setEnabled(false);
        else ui.rbtnTriggerAC->setEnabled(true);
		if (!m_pGageSystem->hasTriggerCouplingDC()) ui.rbtnTriggerDC->setEnabled(false);
        else ui.rbtnTriggerDC->setEnabled(true);
		if (!m_pGageSystem->hasTriggerImpedance50()) ui.rbtnTriggerFiftyOhm->setEnabled(false);
        else ui.rbtnTriggerFiftyOhm->setEnabled(true);
		if (!m_pGageSystem->hasTriggerImpedanceHZ()) ui.rbtnTriggerHighZ->setEnabled(false);
        else ui.rbtnTriggerHighZ->setEnabled(true);

		if ( CS_COUPLING_AC == m_vecTrigParams[u32Engine-1].u32ExtCoupling )
		{
            ui.rbtnTriggerAC->setChecked(true);
		}
		else
		{
            ui.rbtnTriggerDC->setChecked(true);
		}

		if ( CS_REAL_IMP_50_OHM == m_vecTrigParams[u32Engine-1].u32ExtImpedance )
		{
            ui.rbtnTriggerFiftyOhm->setChecked(true);
		}
		else
		{
            ui.rbtnTriggerHighZ->setChecked(true);
		}

        qDebug() << "ChangeTriggerSource: External: Range: " << m_vecTrigParams[u32Engine-1].u32Range;

        if ( CS_GAIN_2_V == m_vecTrigParams[u32Engine-1].u32Range )
        {
            if (m_pGageSystem->hasTriggerRange1V()) ui.rbtnTriggerOneVolt->setChecked(true);
            else ui.rbtnTriggerOneVolt->setChecked(false);
        }
        else if( CS_GAIN_10_V == m_vecTrigParams[u32Engine-1].u32Range )
        {
            if (m_pGageSystem->hasTriggerRange5V()) ui.rbtnTriggerFiveVolt->setChecked(true);
            else ui.rbtnTriggerFiveVolt->setChecked(false);
        }
        else if( 3300 == m_vecTrigParams[u32Engine-1].u32Range )
        {
            if (m_pGageSystem->hasTriggerRange3V3()) ui.rbtnTrigger3V3->setChecked(true);
            else ui.rbtnTrigger3V3->setChecked(false);
        }
        else if( CS_GAIN_5_V == m_vecTrigParams[u32Engine-1].u32Range )
        {
            if (m_pGageSystem->hasTriggerRangePlus5V()) ui.rbtnTriggerPlus5V->setChecked(true);
            else ui.rbtnTriggerPlus5V->setChecked(false);
        }
	}
	//Source: Channel Y
	else // trigger source is a channel
	{
        ui.boxExternalTriggerParameters->setEnabled(false);
		ui.cbxTriggerCondition->setEnabled(true);
		ui.spbxTriggerLevel->setEnabled(true);
	}
    qDebug() << "ChangeTriggerSource: Done";
}


/*******************************************************************************************
										Public slots
*******************************************************************************************/

void QConfigDialog::RestoreLastGoodConfigurations()
{
//	for (uInt32 i = 0; i < m_SystemInfo.u32ChannelCount; i++)
//	{
//		m_vecChannelConfig[i] = m_vecSucceedChannelConfig[i];
//	}

	uInt32 TriggerEngines = m_pGageSystem->getTriggerCount();
	for (uInt32 i = 0; i < TriggerEngines; i++)
	{
		m_vecTrigParams[i] = m_vecSuccessTrigParams[i];
		
		m_pGageSystem->SetSource(i+1, m_vecTrigParams[i].i32Source);
		m_pGageSystem->SetCondition(i+1, m_vecTrigParams[i].u32Condition);
		m_pGageSystem->SetLevel(i+1, m_vecTrigParams[i].i32Level);
		m_pGageSystem->SetExtCoupling(i+1, m_vecTrigParams[i].u32ExtCoupling);
		m_pGageSystem->SetExtImpedance(i+1, m_vecTrigParams[i].u32ExtImpedance);
		m_pGageSystem->SetExtTriggerRange(i+1, m_vecTrigParams[i].u32Range);
	}

	m_pGageSystem->PrepareForCapture(NULL);
}

void QConfigDialog::SaveLastGoodConfigurations()
{
	//for (uInt32 i = 0; i < m_pGageSystem->getChannelCount(); i++)
	//{
//		m_vecSuccessChanParams[i] = m_vecChanParams[i];
//	}

	uInt32 TriggerEngines = m_pGageSystem->getTriggerCount();
	for (uInt32 i = 0; i < TriggerEngines; i++)
	{
		m_vecSuccessTrigParams[i] = m_vecTrigParams[i];
	}
}


void QConfigDialog::s_Cancel()
{
    reject();
}

void QConfigDialog::s_Ok()
{
	qDebug() << "s_Ok";

	// The user could change DC offset then click the OK button. Get the DC offset value fom GUI
	s_ChangeDcOffset();

	emit disableData();
	qDebug() << "s_Ok: Set acquisition parameters";

	m_bAdvancedTrigState = ui.ckbAdvanceTriggering->isChecked();

	// Update Acquisition SW structure
	int CurrIdx;
	CurrIdx = ui.cbxMode->currentIndex();
	uInt32 u32Mode = ui.cbxMode->itemData(CurrIdx, Qt::UserRole).toUInt();

	if (ui.ckbReferenceClock->isChecked())
	{
		u32Mode |= CS_MODE_REFERENCE_CLK;
	}
	else
	{
		u32Mode &= ~CS_MODE_REFERENCE_CLK;
	}
	if (ui.ckbExpert1->isChecked())
	{
		u32Mode |= CS_MODE_USER1;
	}
	else
	{
		u32Mode &= ~CS_MODE_USER1;
	}
	if (ui.ckbExpert2->isChecked())
	{
		u32Mode |= CS_MODE_USER2;
	}
	else
	{
		u32Mode &= ~CS_MODE_USER2;
	}
	m_pGageSystem->setAcquisitionMode(u32Mode);
	m_pGageSystem->SetMulRecAvgCount(ui.spinBox_AvgCount->value());

	m_pGageSystem->setExtClock(ui.ckbExternalClock->isChecked() ? 1 : 0);

	int64 SampRate;
	CurrIdx = ui.cbxSampleRate->currentIndex();
	SampRate = ui.cbxSampleRate->itemData(CurrIdx, Qt::UserRole).toLongLong();
	int64 i64Val = ui.ckbExternalClock->isChecked() ? ui.txtExternalClock->text().toLongLong() : SampRate;

	m_pGageSystem->setSampleRate(i64Val);

	uInt32 u32SegCount = ui.txtNbrRecords->text().toUInt();
	m_pGageSystem->setSegmentCount(u32SegCount);

	int64 i64PreTrig = ui.txtPreTrigger->text().toLongLong();
	if (i64PreTrig >= 0)
	{
		m_pGageSystem->setTriggerHoldoff(i64PreTrig);
		m_pGageSystem->setTriggerDelay(0);
	}
	else
	{
		m_pGageSystem->setTriggerHoldoff(0);
		m_pGageSystem->setTriggerDelay(-1 * i64PreTrig);
	}

	i64Val = ui.txtPostTrigger->text().toULongLong();
	m_pGageSystem->setDepth(i64Val);

	i64Val = ui.ckbTriggerTimeOut->isChecked() ? (ui.dblSpinBox_TriggerTimeout->value() * 10) : -1; // -1 means infinite timeout

	m_pGageSystem->setTriggerTimeout(i64Val);

	// Update Channel SW structures
	uInt32 u32ChannelCount = m_vecChanParams.size();

	if (ui.ckbAllChannels->isChecked())
	{
		qDebug() << "s_EnableAllChannels: Checked";
		ui.cbxChannelIndex->setEnabled(false);

		CurrIdx = ui.cbxInputRange->currentIndex();
		uInt32 u32Range = ui.cbxInputRange->itemData(CurrIdx, Qt::UserRole).toUInt();
		uInt32 u32Coupling = ui.rbtnChannelAC->isChecked() ? CS_COUPLING_AC : CS_COUPLING_DC;
		uInt32 u32Impedance = ui.rbtnChannelFiftyOhms->isChecked() ? CS_REAL_IMP_50_OHM : CS_REAL_IMP_1M_OHM;
		int i32DcOffset = floor((ui.sbxDCOffset->value() / 100.0) * (m_pGageSystem->GetInputRange(1) / 2.0));
		uInt32 u32Filter = ui.radioButton_Filter->isChecked() ? CS_FILTER_ON : CS_FILTER_OFF;

		CurrIdx = ui.cbxMode->currentIndex();
		uInt32 u32Mode = ui.cbxMode->itemData(CurrIdx, Qt::UserRole).toUInt();
		uInt32 u32ChannelSkip = m_pGageSystem->getChannelSkip(u32Mode, m_pGageSystem->GetChannelsPerBoard());
		for (uInt32 i = 0; i < m_pGageSystem->getChannelCount(); i += u32ChannelSkip)
		{
			m_vecChanParams[i].u32InputRange	= u32Range;
			m_vecChanParams[i].u32Coupling		= u32Coupling;
			m_vecChanParams[i].u32Impedance		= u32Impedance;
			m_vecChanParams[i].i32DcOffset		= i32DcOffset;
			m_vecChanParams[i].bFilter			= u32Filter;
		}
	}

    for (uInt32 i=0; i < u32ChannelCount; i++)
    {
        m_pGageSystem->SetInputRange(i+1, m_vecChanParams[i].u32InputRange);
        m_pGageSystem->SetCoupling(i+1, m_vecChanParams[i].u32Coupling);
        m_pGageSystem->SetImpedance(i+1, m_vecChanParams[i].u32Impedance);
        m_pGageSystem->SetFilter(i+1, m_vecChanParams[i].bFilter);
        m_pGageSystem->SetDcOffset(i+1, m_vecChanParams[i].i32DcOffset);
    }

	// Update Trigger SW structures
    qDebug() << "s_Ok: Set trigger parameters";
	for ( uInt32 i = 0, u32Trigger = 1; i < (uInt32)m_vecTrigParams.size(); i++, u32Trigger++ )
	{
        if(m_pGageSystem->IsValidTriggerSource(m_vecTrigParams[i].i32Source))
        {
            m_pGageSystem->SetSource(u32Trigger, m_vecTrigParams[i].i32Source);
            if ( CS_TRIG_SOURCE_DISABLE != m_vecTrigParams[i].i32Source )
            {
                m_pGageSystem->SetCondition(u32Trigger, m_vecTrigParams[i].u32Condition);
                m_pGageSystem->SetLevel(u32Trigger, m_vecTrigParams[i].i32Level);
                if ( CS_TRIG_SOURCE_EXT == m_vecTrigParams[i].i32Source )
                {
                    m_pGageSystem->SetExtCoupling(u32Trigger, m_vecTrigParams[i].u32ExtCoupling);
                    m_pGageSystem->SetExtImpedance(u32Trigger, m_vecTrigParams[i].u32ExtImpedance);
                    m_pGageSystem->SetExtTriggerRange(u32Trigger, m_vecTrigParams[i].u32Range);
                }
            }
        }
        else
        {
            m_pGageSystem->SetSource(u32Trigger, CS_TRIG_SOURCE_DISABLE);
        }
	}

    qDebug() << "s_Ok: Commit";
	int CommitTime;

	int32 i32Status = m_pGageSystem->PrepareForCapture(&CommitTime);
    if ( CS_FAILED(i32Status) )
    {
        m_pErrMng->UpdateErrMsg("Error saving the new parameters.", i32Status);
		*m_bParmsChanged = true;
		m_bParmsLoaded = true;
		RestoreLastGoodConfigurations();
		reject();
        emit ConfigError();
        return;
    }

	SaveLastGoodConfigurations();

    //qDebug() << "updateCommitTime";
    emit updateCommitTime(CommitTime);
    m_UpdateCommitLoop->exec();
    //qDebug() << "m_UpdateCommitLoop done";

    if(ui.checkBox_DisplaySingleSeg->isChecked())
    {
        *m_bDisplayOneSeg = true;
        int64 i64Value;
        i64Value = floor(ui.dblSpinBox_CurrSeg->value());
        if(i64Value>m_pGageSystem->getSegmentCount()) i64Value = m_pGageSystem->getSegmentCount();
        *m_i64CurrSeg = i64Value;
    }
    else
    {
        *m_bDisplayOneSeg = false;
        *m_i64CurrSeg = 1;
    }

	// After Commit, some of parameter could bechnaged by the driver.
	// Get the latest configuration from the driver
	GetAcquisitionParameters();
	UpdateUiAcquisitionParameters();
    GetChannelParameters();
	GetTriggerParameters();

    long long i64Value = floor(ui.dblSpinBox_CurrSeg->value());
    if(i64Value<1)
    {
        i64Value = 1;
        ui.dblSpinBox_CurrSeg->setValue(1);
    }
    if(i64Value>m_AcqParams.u32RecordCount)
    {
        i64Value = m_AcqParams.u32RecordCount;
        ui.dblSpinBox_CurrSeg->setValue(i64Value);
    }

    *m_i64CurrSeg = i64Value;

    *m_bParmsChanged = true;

	accept();
    qDebug() << "s_Ok: Done";
}

void QConfigDialog::s_ChangeAcquisitionMode(int i32State)
{
    qDebug() << "s_ChangeAcquisitionMode";
    Q_UNUSED (i32State);
 
    int CurrIdx = ui.cbxMode->currentIndex();
    uInt32 u32Mode = ui.cbxMode->itemData(CurrIdx, Qt::UserRole).toUInt();
    m_u32AcqMode = u32Mode;

	// Update the sample rate table
    SampleRateTable* pRateTable;
    pRateTable = m_pGageSystem->GetSampleRateTable(u32Mode & CS_MASKED_MODE);

    // make sure combo box is empty first
    ui.cbxSampleRate->clear();

    for ( int i = 0; i < pRateTable->nItemCount; i++ )
    {
        ui.cbxSampleRate->addItem(pRateTable->pSRTable[i].strText, (long long)pRateTable->pSRTable[i].i64SampleRate);
    }

	// Update the channel list table 
    uInt32 u32ChansPerBoard = m_pGageSystem->getChannelCountPerBoard();
    uInt32 u32ChannelSkip = m_pGageSystem->getChannelSkip(u32Mode, u32ChansPerBoard);
    uInt32 u32ChannelCount = m_pGageSystem->getChannelCount();
    m_u32ChanSkip = u32ChannelSkip;

    ui.cbxChannelIndex->clear();
    for ( uInt32 i = 1; i <= u32ChannelCount; i += u32ChannelSkip )
    {
        ui.cbxChannelIndex->addItem("Channel " +  QString::number(i), QVariant::fromValue(i));
    }

    bool bValuesAreEqual = true;
    if( CS_MODE_SINGLE == u32Mode )
    {
        ui.ckbAllChannels->hide();
    }
    else
    {
		ui.ckbAllChannels->show();
    }

    for ( uInt32 i = 0; i < u32ChannelCount; i+=u32ChannelSkip )
    {
        if  ( (m_vecChanParams[0].bFilter != m_vecChanParams[i].bFilter) ||
              (m_vecChanParams[0].i32DcOffset != m_vecChanParams[i].i32DcOffset) ||
              (m_vecChanParams[0].u32Coupling != m_vecChanParams[i].u32Coupling) ||
              (m_vecChanParams[0].u32Impedance != m_vecChanParams[i].u32Impedance) ||
              (m_vecChanParams[0].u32InputRange != m_vecChanParams[i].u32InputRange) )
        {
            bValuesAreEqual = false;
            break;
        }
    }

	ui.cbxChannelIndex->setEnabled(!bValuesAreEqual);
	ui.ckbAllChannels->blockSignals(true);
    ui.ckbAllChannels->setChecked(bValuesAreEqual);
	ui.ckbAllChannels->blockSignals(false);

    ui.cbxChannelIndex->setCurrentIndex(0);

    PopulateRangeComboBox();

    SetChannelParameters();

    ui.cbxTriggerSource->clear();
    for ( uInt32 i = 1; i <= u32ChannelCount; i+= u32ChannelSkip )
    {
        ui.cbxTriggerSource->addItem("Channel " +  QString::number(i), QVariant::fromValue(i));
    }

    if ( m_pGageSystem->hasExternalTrigger() )
    {
        ui.cbxTriggerSource->addItem("External", QVariant::fromValue(-1));
    }
    ui.cbxTriggerSource->addItem("Disable", QVariant::fromValue(0));

    ui.cbxTriggerCondition->clear();
    ui.cbxTriggerCondition->addItem("Rising", QVariant::fromValue(CS_TRIG_COND_POS_SLOPE));
    ui.cbxTriggerCondition->addItem("Falling", QVariant::fromValue(CS_TRIG_COND_NEG_SLOPE));

    ui.tableAdvancedTriggering->setVisible(false);

    qDebug() << "s_ChangeAcquisitionMode: AcqMode: " << u32Mode;
    SetTriggerParameters(u32Mode);

     qDebug() << "s_ChangeAcquisitionMode: Done";
}

void QConfigDialog::s_UpdateTab(int i32Index)
{
	Q_UNUSED (i32Index);
	ui.btnOk->setFocus();
}

void QConfigDialog::s_EnableExternalClock(int i32State)
{
    Q_UNUSED (i32State);

	//qDebug() << "s_EnableExternalClock";
	if (ui.ckbExternalClock->isChecked())
    {
		// Uncheck Reference clock
		ui.ckbReferenceClock->setChecked(false);

        m_PrevIntClockRate = m_pGageSystem->getSampleRate();
        ui.txtExternalClock->setVisible(true);
        ui.txtExternalClock->setEnabled(true);
        ui.txtExternalClock->setDisabled(false);
        ui.cbxSampleRate->setVisible(false);
        ui.label_SampleRate->setText("Sample Rate (Hz)");
        int64 MinExtRate = m_pGageSystem->getMinExtRate();
        int64 MaxExtRate = m_pGageSystem->getMaxExtRate();
        int64 CurrExtRate = ui.txtExternalClock->text().toLongLong();
        qDebug() << "Curr external rate: " << CurrExtRate;
        if(CurrExtRate < MinExtRate) CurrExtRate = MinExtRate;
        else if(CurrExtRate > MaxExtRate) CurrExtRate = MaxExtRate;

        ui.txtExternalClock->setText(QString("%1").arg(CurrExtRate));
    }
    else
    {
        ui.txtExternalClock->setVisible(false);
        ui.cbxSampleRate->setVisible(true);
        ui.label_SampleRate->setText("Sample Rate");
    }
}

void QConfigDialog::s_EnableReferenceClock(int i32State)
{
    Q_UNUSED (i32State);
    //qDebug() << "s_EnableReferenceclock";

    if (ui.ckbReferenceClock->isChecked())
    {
		// Uncheck External clock
        ui.ckbExternalClock->setChecked(false);
    }
}

void QConfigDialog::s_UpdateExtClock()
{
    qDebug() << "s_UpdateExtClock";

    if(m_PrevIntClockRate==ui.txtExternalClock->text().toLongLong())
        return;

    int64 MinExtRate = m_pGageSystem->getMinExtRate();
    int64 MaxExtRate = m_pGageSystem->getMaxExtRate();
    int64 CurrExtRate = ui.txtExternalClock->text().toLongLong();

    if(CurrExtRate<MinExtRate) CurrExtRate = MinExtRate;
    else if(CurrExtRate>MaxExtRate) CurrExtRate = MaxExtRate;

    m_pGageSystem->getSampleRate();

    ui.txtExternalClock->setText(QString("%1").arg(CurrExtRate));
    m_PrevIntClockRate = CurrExtRate;
}


void QConfigDialog::s_ChangeExpert1(int i32State)
{
    Q_UNUSED (i32State);
    if (!m_bParmsLoaded) return;
    qDebug() << "s_ChangeExpert1";

    m_bParmsLoaded = false;

	if (ui.ckbExpert1->isChecked())
	{
        qDebug() << "Expert1 is checked";
        if(m_pGageSystem->GetExpertImg1() == EXPERT_STREAM)
        {
            QMessageBox ThisMsgBox;
            ThisMsgBox.setText("Streaming is not supported");
            ThisMsgBox.exec();
            ui.ckbExpert1->setCheckState(Qt::Unchecked);
            m_bParmsLoaded = true;
            return;
        }
        else
        {
            m_AcqParams.bExpert1 = true;
        }

        if(m_pGageSystem->GetExpertImg1() == EXPERT_MULREC_AVG_TD) ui.spinBox_AvgCount->setEnabled(true);
	}
	else
	{
		m_AcqParams.bExpert1 = false;
        ui.spinBox_AvgCount->setEnabled(false);
	}

    m_bParmsLoaded = true;

    ui.btnOk->setFocus();

    qDebug() << "s_ChangeExpert1: Done";
}

void QConfigDialog::s_ChangeExpert2(int i32State)
{
    Q_UNUSED (i32State);
    if (!m_bParmsLoaded) return;
    //qDebug() << "s_ChangeExpert2";

    m_bParmsLoaded = false;

	if (ui.ckbExpert2->isChecked())
	{
        qDebug() << "Expert2 is checked";
        if(m_pGageSystem->GetExpertImg2() == EXPERT_STREAM)
        {
            QMessageBox ThisMsgBox;
            ThisMsgBox.setText("Streaming is not supported");
            ThisMsgBox.exec();
            ui.ckbExpert2->setCheckState(Qt::Unchecked);
            m_bParmsLoaded = true;
            return;
        }
        else
        {
            m_AcqParams.bExpert2 = true;
        }

        if(m_pGageSystem->GetExpertImg2() == EXPERT_MULREC_AVG_TD) ui.spinBox_AvgCount->setEnabled(true);
	}
	else
	{
		m_AcqParams.bExpert2 = false;
        ui.spinBox_AvgCount->setEnabled(false);
	}

    m_bParmsLoaded = true;

    ui.btnOk->setFocus();
}

void QConfigDialog::s_ChangeAvgCount()
{
    qDebug() << "s_ChangeAvgCount";

    m_i32AvgCount = ui.spinBox_AvgCount->value();
}

void QConfigDialog::s_EnableNbrRecords(bool bState)
{
    Q_UNUSED (bState);
    qDebug() << "s_EnableNbrRecords";

    if (!ui.ckbNbrRecords->isChecked())
    {
        qDebug() << "s_EnableNbrRecords: Unchecked";
        m_AcqParams.u32RecordCount = 1;
        ui.txtNbrRecords->setText("1");
        ui.txtNbrRecords->setEnabled(false);
        ui.checkBox_DisplaySingleSeg->setEnabled(false);
        ui.checkBox_DisplaySingleSeg->setChecked(false);
        ui.dblSpinBox_CurrSeg->setEnabled(false);
        ui.dblSpinBox_CurrSeg->setValue(0);
    }
    else
    {
        qDebug() << "s_EnableNbrRecords: Checked";
        ui.txtNbrRecords->setEnabled(true);
        ui.checkBox_DisplaySingleSeg->setEnabled(true);
        ui.dblSpinBox_CurrSeg->setValue(0);
        if (1 == m_AcqParams.u32RecordCount)
        {
            ui.txtNbrRecords->setText("2");
            m_AcqParams.u32RecordCount = 2;
        }
    }
    //qDebug() << "s_EnableNbrRecords done: ParmsLoaded: " << m_bParmsLoaded;
}

void QConfigDialog::s_DisplaySingleSeg(int i32State)
{
    Q_UNUSED(i32State);
    qDebug() << "s_DisplaySingleSeg";

    if(ui.checkBox_DisplaySingleSeg->isChecked())
    {
        qDebug() << "s_DisplaySingleSeg: Enable";
        ui.dblSpinBox_CurrSeg->setEnabled(true);
        ui.dblSpinBox_CurrSeg->setValue(1);
    }
    else
    {
        qDebug() << "s_DisplaySingleSeg: Disable";
        ui.dblSpinBox_CurrSeg->setEnabled(false);
        ui.dblSpinBox_CurrSeg->setValue(0);
    }

    qDebug() << "s_DisplaySingleSeg: Done";
}

void QConfigDialog::s_DisplaySingleSeg(bool bState)
{
    Q_UNUSED(bState);
    qDebug() << "s_DisplaySingleSeg: Start";

    if(ui.checkBox_DisplaySingleSeg->isChecked())
    {
        qDebug() << "s_DisplaySingleSeg: Enable";
        ui.dblSpinBox_CurrSeg->setEnabled(true);
        ui.dblSpinBox_CurrSeg->setValue(1);
    }
    else
    {
        qDebug() << "s_DisplaySingleSeg: Disable";
        ui.dblSpinBox_CurrSeg->setEnabled(false);
        ui.dblSpinBox_CurrSeg->setValue(0);
    }

    qDebug() << "s_DisplaySingleSeg: Done";
}

void QConfigDialog::s_ChangeCurrSeg()
{
    qDebug() << "s_ChangeCurrSegVoid: Start";

    int64 i64Value;
    i64Value = floor(ui.dblSpinBox_CurrSeg->value());
    qDebug() << "s_ChangeCurrSeg: New seg: " << i64Value;
    if(i64Value<1)
    {
        i64Value = 1;
        ui.dblSpinBox_CurrSeg->setValue(1);
    }

    qDebug() << "s_ChangeCurrSeg: Segment: " << i64Value;
}

void QConfigDialog::s_ChangeCurrSeg(double)
{
    qDebug() << "s_ChangeCurrSegDouble: Start";

    int64 i64Value;
    i64Value = floor(ui.dblSpinBox_CurrSeg->value());
    qDebug() << "s_ChangeCurrSeg: New seg: " << i64Value;
    if(i64Value<1)
    {
        i64Value = 1;
        ui.dblSpinBox_CurrSeg->setValue(1);
    }

    qDebug() << "s_ChangeCurrSegDouble: Done";
}

void QConfigDialog::s_EnableTriggerTimeOut(int i32State)
{
	if (i32State == Qt::Unchecked)
	{
		m_AcqParams.i64TriggerTimeout = -1;
        //ui.txtTriggerTimeOut->setText("Infinite");
        ui.dblSpinBox_TriggerTimeout->setValue(-1);
        //ui.dblSpinBox_TriggerTimeout->setSpecialValueText("Infinite");
        ui.dblSpinBox_TriggerTimeout->setEnabled(false);
	}
	else
	{
        //ui.txtTriggerTimeOut->setText(QString::number(c_DefautTriggerTimeout));
        ui.dblSpinBox_TriggerTimeout->setValue(c_DefautTriggerTimeout);
        m_AcqParams.i64TriggerTimeout = c_DefautTriggerTimeout * 10;
        ui.dblSpinBox_TriggerTimeout->setEnabled(true);
	}
}

void QConfigDialog::s_ChangeTriggerTimeOut()
{

}

void QConfigDialog::s_EnableAllChannels(int i32State)
{
    Q_UNUSED (i32State);

    qDebug() << "s_EnableAllChannels";

	if(!ui.ckbAllChannels->isChecked())
	{
        qDebug() << "s_EnableAllChannels: was Unchecked";
		ui.cbxChannelIndex->setEnabled(true);

	}
	else
	{
        qDebug() << "s_EnableAllChannels: was Checked";
		ui.cbxChannelIndex->setEnabled(false);
	}

    qDebug() << "s_EnableAllChannels: Done";
}

void QConfigDialog::s_ChangeChannelIndex(int i32ChannelIndex)
{
    Q_UNUSED (i32ChannelIndex);
    qDebug() << "s_ChangeChannelIndex";

    int CurrIdx = ui.cbxChannelIndex->currentIndex();
    uInt32 u32Channel = ui.cbxChannelIndex->itemData(CurrIdx, Qt::UserRole).toUInt();

    qDebug() << "s_ChangeChannelIndex: CH" << u32Channel;
    if ( (u32Channel < 1) || (u32Channel > m_pGageSystem->getChannelCount()))
	{
        qDebug() << "s_ChangeChannelIndex: Channel out of range";
        m_bParmsLoaded = true;
		return;
	}

	uInt32 u32Range = m_vecChanParams.at(u32Channel-1).u32InputRange;
	int currentSelection = ui.cbxInputRange->findData(QVariant::fromValue(u32Range));
	ui.cbxInputRange->setCurrentIndex(currentSelection);

	uInt32 u32Coupling = m_vecChanParams.at(u32Channel-1).u32Coupling;
	if ( u32Coupling & CS_COUPLING_AC )
	{
		ui.rbtnChannelAC->setChecked(true);
        ui.rbtnChannelDC->setChecked(false);
	}
	else
	{
        ui.rbtnChannelAC->setChecked(false);
		ui.rbtnChannelDC->setChecked(true);
	}
	uInt32 u32Impedance = m_vecChanParams.at(u32Channel-1).u32Impedance;
	if ( CS_REAL_IMP_1M_OHM == u32Impedance )
	{
		ui.rbtnChannelOneMegaOhms->setChecked(true);
        ui.rbtnChannelFiftyOhms->setChecked(false);
	}
	else
	{
        ui.rbtnChannelOneMegaOhms->setChecked(false);
		ui.rbtnChannelFiftyOhms->setChecked(true);
	}

    ui.radioButton_Filter->setChecked(m_vecChanParams.at(u32Channel-1).bFilter);

    int DcOffsetVolt = m_vecChanParams.at(u32Channel-1).i32DcOffset;
    int DcOffsetPerc = floor((((DcOffsetVolt / (u32Range / 2.0)) * 100)) + 0.5);
    ui.sbxDCOffset->setValue(DcOffsetPerc);

     qDebug() << "s_ChangeChannelIndex: Done";
}

void QConfigDialog::s_EnableFilter(bool bState)
{
    Q_UNUSED (bState);
    qDebug() << "s_EnableFilter";

    m_bParmsLoaded = false;
	
    bool bFlag = ui.radioButton_Filter->isChecked();

/*	if ( ui.ckbAllChannels->isChecked() )
	{
		uInt32 u32ChannelCount = m_vecChanParams.size();
        for ( uInt32 i = 0; i < u32ChannelCount; i+=m_u32ChanSkip )
		{
			m_vecChanParams[i].bFilter = bFlag ? CS_FILTER_ON : CS_FILTER_OFF;
		}
	}
	else
*/
	{
        int CurrIdx = ui.cbxChannelIndex->currentIndex();
        uInt32 u32Chan = ui.cbxChannelIndex->itemData(CurrIdx, Qt::UserRole).toUInt();
		m_vecChanParams[u32Chan-1].bFilter = bFlag;
	}

    qDebug() << "s_EnableFilter: Done";
}

void QConfigDialog::s_ChangeInputRange(int i32RangeIndex)
{
    Q_UNUSED (i32RangeIndex);
    qDebug() << "s_ChangeInputRange";

    m_bParmsLoaded = false;
	
    int CurrIdx = ui.cbxInputRange->currentIndex();
    uInt32 u32Range = ui.cbxInputRange->itemData(CurrIdx, Qt::UserRole).toUInt();
		
/*	if ( ui.ckbAllChannels->isChecked() )
	{
		uInt32 u32ChannelCount = m_vecChanParams.size();
        for ( uInt32 i = 0; i < u32ChannelCount; i+=m_u32ChanSkip )
		{
			m_vecChanParams[i].u32InputRange = u32Range;
		}
	}
	else
*/
	{
        CurrIdx = ui.cbxChannelIndex->currentIndex();
        uInt32 u32Chan = ui.cbxChannelIndex->itemData(CurrIdx, Qt::UserRole).toUInt();
		m_vecChanParams[u32Chan-1].u32InputRange = u32Range;
	}
}

void QConfigDialog::s_ChangeDcOffset()
{
    if (!m_bParmsLoaded) return;
    qDebug() << "s_ChangeDcOffset(int)";

    m_bParmsLoaded = false;

    int DcOffsetVal = ui.sbxDCOffset->value();

    int32 i32DcOffset;

    qDebug() << "DC offset percentage: " << DcOffsetVal;

    if (ui.ckbAllChannels->isChecked())
    {
        uInt32 u32ChannelCount = m_vecChanParams.size();
        i32DcOffset = floor(((DcOffsetVal / 100.0) * (m_vecChanParams[0].u32InputRange / 2.0)) + 0.5);

        qDebug() << "s_ChangeDcOffset: " << i32DcOffset;

        for (uInt32 i = 0; i < u32ChannelCount; i+=m_u32ChanSkip)
        {
            m_vecChanParams[i].i32DcOffset = i32DcOffset;
        }
    }
    else
    {
        int CurrIdx = ui.cbxChannelIndex->currentIndex();
        uInt32 u32Chan = ui.cbxChannelIndex->itemData(CurrIdx, Qt::UserRole).toUInt();

        qDebug() << "s_ChangeDcOffset: CH" << u32Chan;

        i32DcOffset = floor(((DcOffsetVal / 100.0) * (m_vecChanParams[u32Chan-1].u32InputRange / 2.0)) + 0.5);

        qDebug() << "s_ChangeDcOffset: " << i32DcOffset;

        m_vecChanParams[u32Chan - 1].i32DcOffset = i32DcOffset;
    }

    m_bParmsLoaded = true;

    qDebug() << "s_ChangeDcOffset(int): Done";
}

void QConfigDialog::s_EnableAC(bool bStateIn)
{
    Q_UNUSED (bStateIn);
    if (!m_bParmsLoaded) return;
    //qDebug() << "s_EnableAC";

    m_bParmsLoaded = false;
	
	bool bState = ui.rbtnChannelAC->isChecked();

    if(bState)
    {
        ui.rbtnChannelDC->blockSignals(true);
        ui.rbtnChannelDC->setChecked(false);
        ui.rbtnChannelDC->blockSignals(false);
    }
    else
    {
        ui.rbtnChannelAC->blockSignals(true);
        ui.rbtnChannelAC->setChecked(true);
        ui.rbtnChannelAC->blockSignals(false);
        m_bParmsLoaded = true;
        return;
    }

	if ( ui.ckbAllChannels->isChecked() )
	{
		uInt32 u32ChannelCount = m_vecChanParams.size();
        for ( uInt32 i = 0; i < u32ChannelCount;i+=m_u32ChanSkip )
		{
			m_vecChanParams[i].u32Coupling = bState ? CS_COUPLING_AC : CS_COUPLING_DC;
		}
	}
	else
	{
        int CurrIdx = ui.cbxChannelIndex->currentIndex();
        uInt32 u32Chan = ui.cbxChannelIndex->itemData(CurrIdx, Qt::UserRole).toUInt();
		m_vecChanParams[u32Chan-1].u32Coupling = bState ? CS_COUPLING_AC : CS_COUPLING_DC;
	}

    m_bParmsLoaded = true;
}

void QConfigDialog::s_EnableDC(bool bStateIn)
{
    Q_UNUSED (bStateIn);
    if (!m_bParmsLoaded) return;
    //qDebug() << "s_EnableDC";

    m_bParmsLoaded = false;
	
	bool bState = ui.rbtnChannelDC->isChecked();
    if(bState)
    {
        ui.rbtnChannelAC->blockSignals(true);
        ui.rbtnChannelAC->setChecked(false);
        ui.rbtnChannelAC->blockSignals(false);
    }
    else
    {
        ui.rbtnChannelDC->blockSignals(true);
        ui.rbtnChannelDC->setChecked(true);
        ui.rbtnChannelDC->blockSignals(false);
        m_bParmsLoaded = true;
        return;
    }

	if ( ui.ckbAllChannels->isChecked() )
	{
		uInt32 u32ChannelCount = m_vecChanParams.size();
        for ( uInt32 i = 0; i < u32ChannelCount; i+=m_u32ChanSkip )
		{
			m_vecChanParams[i].u32Coupling = bState ? CS_COUPLING_DC : CS_COUPLING_AC;
		}
	}
	else
	{
        int CurrIdx = ui.cbxChannelIndex->currentIndex();
        uInt32 u32Chan = ui.cbxChannelIndex->itemData(CurrIdx, Qt::UserRole).toUInt();
		m_vecChanParams[u32Chan-1].u32Coupling = bState ? CS_COUPLING_DC : CS_COUPLING_AC;
	}

    m_bParmsLoaded = true;
}

void QConfigDialog::s_Enable50Ohm(bool bStateIn)
{
    Q_UNUSED (bStateIn);
    if (!m_bParmsLoaded) return;
    //qDebug() << "s_Enable50Ohm";

    m_bParmsLoaded = false;
	
	bool bState = ui.rbtnChannelFiftyOhms->isChecked();
    if(bState)
    {
        ui.rbtnChannelOneMegaOhms->blockSignals(true);
        ui.rbtnChannelOneMegaOhms->setChecked(false);
        ui.rbtnChannelOneMegaOhms->blockSignals(false);
    }
    else
    {
        ui.rbtnChannelFiftyOhms->blockSignals(true);
        ui.rbtnChannelFiftyOhms->setChecked(true);
        ui.rbtnChannelFiftyOhms->blockSignals(false);
        m_bParmsLoaded = true;
        return;
    }

	if ( ui.ckbAllChannels->isChecked() )
	{
		uInt32 u32ChannelCount = m_vecChanParams.size();
        for ( uInt32 i = 0; i < u32ChannelCount; i+=m_u32ChanSkip )
		{
			m_vecChanParams[i].u32Impedance = bState ? CS_REAL_IMP_50_OHM: CS_REAL_IMP_1M_OHM;
		}
	}
	else
	{
        int CurrIdx = ui.cbxChannelIndex->currentIndex();
        uInt32 u32Chan = ui.cbxChannelIndex->itemData(CurrIdx, Qt::UserRole).toUInt();
		m_vecChanParams[u32Chan-1].u32Impedance = bState ? CS_REAL_IMP_50_OHM: CS_REAL_IMP_1M_OHM;
	}

    m_bParmsLoaded = true;
}

void QConfigDialog::s_Enable1MOhm(bool bStateIn)
{
    Q_UNUSED (bStateIn);
    if (!m_bParmsLoaded) return;
    qDebug() << "s_Enable1MOhm";

    m_bParmsLoaded = false;
	
	bool bState = ui.rbtnChannelOneMegaOhms->isChecked();
    if(bState)
    {
        ui.rbtnChannelFiftyOhms->blockSignals(true);
        ui.rbtnChannelFiftyOhms->setChecked(false);
        ui.rbtnChannelFiftyOhms->blockSignals(false);
    }
    else
    {
        ui.rbtnChannelOneMegaOhms->blockSignals(true);
        ui.rbtnChannelOneMegaOhms->setChecked(true);
        ui.rbtnChannelOneMegaOhms->blockSignals(false);
        m_bParmsLoaded = true;
        return;
    }

	if ( ui.ckbAllChannels->isChecked() )
	{
        uInt32 u32ChannelCount = m_vecChanParams.size();
        for ( uInt32 i = 0; i < u32ChannelCount; i+=m_u32ChanSkip )
		{
			m_vecChanParams[i].u32Impedance = bState ? CS_REAL_IMP_1M_OHM: CS_REAL_IMP_50_OHM;
		}
	}
	else
	{
        int CurrIdx = ui.cbxChannelIndex->currentIndex();
        uInt32 u32Chan = ui.cbxChannelIndex->itemData(CurrIdx, Qt::UserRole).toUInt();
		m_vecChanParams[u32Chan-1].u32Impedance = bState ? CS_REAL_IMP_1M_OHM: CS_REAL_IMP_50_OHM;
	}

    m_bParmsLoaded = true;
}

void QConfigDialog::s_EnableAdvanceTriggering(int i32StateIn)
{
    Q_UNUSED (i32StateIn);

    if (!m_bParmsLoaded) return;

    m_bParmsLoaded = false;
    //qDebug() << "s_EnableAdvanceTriggering";

	int i32State = ui.ckbAdvanceTriggering->isChecked() ? Qt::Checked : Qt::Unchecked;

	if(i32State == Qt::Unchecked)
	{
		ui.tableAdvancedTriggering->setVisible(false);

		// unchecking the box cancels advanced triggering, so we'll reset
		// all trigger engines (except for the first one) back to their defaults
		// and we'll set the trigger source, condition and level boxes to whatever
		// trigger engine 1 is set to
		for ( uInt32 i = 1; i < m_pGageSystem->getTriggerCount(); i++ )
		{
            //Change trigger parameters of the vector of the config dialog.
            m_vecTrigParams[i].i32Source = CS_TRIG_SOURCE_DISABLE;
            m_vecTrigParams[i].i32Level = 0;
            m_vecTrigParams[i].u32Condition = CS_TRIG_COND_POS_SLOPE;
		}
		int currentSelection = ui.cbxTriggerSource->findData(QVariant::fromValue(m_vecTrigParams[0].i32Source));
		ui.cbxTriggerSource->setCurrentIndex(currentSelection);
		currentSelection = ui.cbxTriggerCondition->findData(QVariant::fromValue(m_vecTrigParams[0].u32Condition));
		ui.cbxTriggerCondition->setCurrentIndex(currentSelection);
		ui.spbxTriggerLevel->setValue(m_vecTrigParams[0].i32Level);
		if ( CS_TRIG_SOURCE_EXT == m_vecTrigParams[0].i32Source  )
		{
            ui.boxExternalTriggerParameters->setEnabled(true);

			bool bFlag = CS_COUPLING_AC == m_vecTrigParams[0].u32ExtCoupling;
			ui.rbtnTriggerAC->setChecked(bFlag);
			ui.rbtnTriggerDC->setChecked(!bFlag);

			bFlag = CS_REAL_IMP_50_OHM == m_vecTrigParams[0].u32ExtImpedance;
			ui.rbtnTriggerFiftyOhm->setChecked(bFlag);
			ui.rbtnTriggerHighZ->setChecked(!bFlag);

			bFlag = CS_GAIN_2_V == m_vecTrigParams[0].u32Range;
			ui.rbtnTriggerOneVolt->setChecked(bFlag);

            bFlag = CS_GAIN_10_V == m_vecTrigParams[0].u32Range;
            ui.rbtnTriggerFiveVolt->setChecked(bFlag);

            bFlag = 3300 == m_vecTrigParams[0].u32Range;
            ui.rbtnTrigger3V3->setChecked(bFlag);

            bFlag = CS_GAIN_5_V == m_vecTrigParams[0].u32Range;
            ui.rbtnTriggerPlus5V->setChecked(bFlag);
		}
	}
	else
	{
		// make sure the table is empty
		ui.tableAdvancedTriggering->clearContents();

		ui.tableAdvancedTriggering->setVisible(true);

		ui.tableAdvancedTriggering->setRowCount(m_vecTrigParams.size()); // RG ??

		QStringList headerNames;
		headerNames << "Source" << "Condition" << "Level";
		ui.tableAdvancedTriggering->setHorizontalHeaderLabels(headerNames);
		ui.tableAdvancedTriggering->setShowGrid(false);

		// next 3 lines are to set the height of each row in the table so I 
		// don't have to set them individually
		QHeaderView *verticalHeader = ui.tableAdvancedTriggering->verticalHeader();
		verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
		verticalHeader->setDefaultSectionSize(24);

		for ( int i = 0; i < ui.tableAdvancedTriggering->rowCount(); i++ )
		{
			if ( CS_TRIG_SOURCE_DISABLE == m_vecTrigParams[i].i32Source )
			{
				QTableWidgetItem* item1 = new QTableWidgetItem("Disable");
                item1->setData(Qt::UserRole, (int)m_vecTrigParams[i].i32Source);
				ui.tableAdvancedTriggering->setItem(i, c_TriggerSource, item1);
				QTableWidgetItem* item2 = new QTableWidgetItem(c_DisabledString);
				item2->setData(Qt::UserRole, QVariant::fromValue(m_vecTrigParams[i].u32Condition));
				ui.tableAdvancedTriggering->setItem(i, c_TriggerCondition, item2);
				QTableWidgetItem* item3 = new QTableWidgetItem(c_DisabledString);
                item3->setData(Qt::UserRole, (int)m_vecTrigParams[i].i32Level); // RG not sure that I need this
				ui.tableAdvancedTriggering->setItem(i, c_TriggerLevel, item3);
			}
			else if ( CS_TRIG_SOURCE_EXT == m_vecTrigParams[i].i32Source )
			{
				QTableWidgetItem* item = new QTableWidgetItem("External");
				item->setData(Qt::UserRole, (int)m_vecTrigParams[i].i32Source);
				ui.tableAdvancedTriggering->setItem(i, c_TriggerSource, item);
				ui.tableAdvancedTriggering->setItem(i, c_TriggerCondition, new QTableWidgetItem(( CS_TRIG_COND_NEG_SLOPE == m_vecTrigParams[i].u32Condition ) ? "Falling" : "Rising"));
				ui.tableAdvancedTriggering->setItem(i, c_TriggerLevel, new QTableWidgetItem(QString::number(m_vecTrigParams[i].i32Level)));
			}
			else // trigger source is a channel
			{
				QTableWidgetItem* item1 = new QTableWidgetItem("Channel " + QString::number(m_vecTrigParams[i].i32Source));
                item1->setData(Qt::UserRole, (int)m_vecTrigParams[i].i32Source);
				ui.tableAdvancedTriggering->setItem(i, c_TriggerSource, item1);
				QTableWidgetItem* item2 = new QTableWidgetItem(( CS_TRIG_COND_NEG_SLOPE == m_vecTrigParams[i].u32Condition ) ? "Falling" : "Rising");
				item2->setData(Qt::UserRole, QVariant::fromValue(m_vecTrigParams[i].u32Condition));
				ui.tableAdvancedTriggering->setItem(i, c_TriggerCondition, item2);
				QTableWidgetItem* item3 = new QTableWidgetItem(QString::number(m_vecTrigParams[i].i32Level));
                item3->setData(Qt::UserRole, (int)m_vecTrigParams[i].i32Level);  // RG Do I need this ?? RG RG
				ui.tableAdvancedTriggering->setItem(i, c_TriggerLevel, item3);
			}
		}

        ui.tableAdvancedTriggering->blockSignals(true);
		ui.tableAdvancedTriggering->selectRow(0);
        ui.tableAdvancedTriggering->blockSignals(false);
	}

    if ( CS_TRIG_SOURCE_EXT == m_vecTrigParams[0].i32Source ) ui.boxExternalTriggerParameters->setEnabled(true);
    else ui.boxExternalTriggerParameters->setEnabled(false);

    m_bParmsLoaded = true;
}

void QConfigDialog::s_ChangeTriggerSource(int i32IndexIn)
{
    Q_UNUSED (i32IndexIn);
    if (!m_bParmsLoaded) return;
    //qDebug() << "s_ChangeTriggerSource";

    m_bParmsLoaded = false;

	int32 i32Index = ui.cbxTriggerSource->currentIndex();

	uInt32 u32Engine = ( ui.ckbAdvanceTriggering->isChecked() ) ? ui.tableAdvancedTriggering->currentRow() : 0;

	//Source: Disable
    int CurrIdx = ui.cbxTriggerSource->currentIndex();
    int32 i32Source = ui.cbxTriggerSource->itemData(CurrIdx, Qt::UserRole).toInt();
	m_vecTrigParams[u32Engine].i32Source = i32Source;

    if ( CS_TRIG_SOURCE_DISABLE == i32Source )
	{
        ui.boxExternalTriggerParameters->setEnabled(false);
		ui.cbxTriggerCondition->setCurrentIndex(ui.cbxTriggerCondition->findData(QVariant::fromValue(CS_TRIG_COND_POS_SLOPE)));
		ui.cbxTriggerCondition->setEnabled(false);
		ui.spbxTriggerLevel->setValue(0);
		ui.spbxTriggerLevel->setEnabled(false);
	}
	//Source: External 1
    else if ( CS_TRIG_SOURCE_EXT == i32Source )
	{
        ui.boxExternalTriggerParameters->setEnabled(true);
		ui.cbxTriggerCondition->setEnabled(true);
		ui.spbxTriggerLevel->setEnabled(true);
        QueryTriggerCaps();

		if ( CS_COUPLING_AC == m_vecTrigParams[u32Engine].u32ExtCoupling )
		{
			ui.rbtnTriggerAC->setChecked(true);
		}
		else
		{
			ui.rbtnTriggerDC->setChecked(true);
		}

		if ( CS_REAL_IMP_50_OHM == m_vecTrigParams[u32Engine].u32ExtImpedance )
		{
			ui.rbtnTriggerFiftyOhm->setChecked(true);

            //m_pGageSystem->SetExtImpedance(u32Engine + 1, CS_REAL_IMP_50_OHM);
		}
		else
		{
			ui.rbtnTriggerHighZ->setChecked(true);
		}

		if ( CS_GAIN_2_V == m_vecTrigParams[u32Engine].u32Range )
		{
			ui.rbtnTriggerOneVolt->setChecked(true);

            //m_pGageSystem->SetExtTriggerRange(u32Engine + 1, CS_GAIN_2_V);
		}
        else if ( CS_GAIN_10_V == m_vecTrigParams[u32Engine].u32Range )
		{
			ui.rbtnTriggerFiveVolt->setChecked(true);
		}
        else if ( 3300 == m_vecTrigParams[u32Engine].u32Range )
        {
            ui.rbtnTrigger3V3->setChecked(true);
        }
        else if ( CS_GAIN_5_V == m_vecTrigParams[u32Engine].u32Range )
        {
            ui.rbtnTriggerPlus5V->setChecked(true);
        }
	}
	//Source: Channel Y
	else // trigger source is a channel
	{
        ui.boxExternalTriggerParameters->setEnabled(false);
		ui.cbxTriggerCondition->setEnabled(true);
		ui.spbxTriggerLevel->setEnabled(true);
	}

	// if the advanced trigger box is checked, change the contents of it as well
	if ( ui.ckbAdvanceTriggering->isChecked() )
	{
		int currentRow = ui.tableAdvancedTriggering->currentRow();
		QTableWidgetItem* q = ui.tableAdvancedTriggering->item(currentRow, c_TriggerSource);
		q->setText(ui.cbxTriggerSource->itemText(i32Index));
		q->setData(Qt::UserRole, QVariant::fromValue(ui.cbxTriggerSource->itemData(i32Index)));

		q = ui.tableAdvancedTriggering->item(currentRow, c_TriggerCondition);
		q->setText(ui.cbxTriggerCondition->currentText());
        CurrIdx = ui.cbxTriggerCondition->currentIndex();
        q->setData(Qt::UserRole, ui.cbxTriggerCondition->itemData(CurrIdx, Qt::UserRole));

		q = ui.tableAdvancedTriggering->item(currentRow, c_TriggerLevel);
		q->setText(ui.spbxTriggerLevel->text());
		q->setData(Qt::UserRole, QVariant::fromValue(ui.spbxTriggerLevel->value()));
	}

    m_bParmsLoaded = true;
}

void QConfigDialog::s_ChangeTriggerCondition(int i32Index)
{
    Q_UNUSED (i32Index);
    if (!m_bParmsLoaded) return;
    //qDebug() << "s_ChangeTriggerCondition";

    m_bParmsLoaded = false;

    int CurrIdx = ui.cbxTriggerSource->currentIndex();
    int i32TrigSource = ui.cbxTriggerSource->itemData(CurrIdx, Qt::UserRole).toInt();
    CurrIdx = ui.cbxTriggerCondition->currentIndex();
    uInt32 u32Trigcond = ui.cbxTriggerCondition->itemData(CurrIdx, Qt::UserRole).toUInt();

	uInt32 u32Engine = ( ui.ckbAdvanceTriggering->isChecked() ) ? ui.tableAdvancedTriggering->currentRow() : 0;

	if ( ui.ckbAdvanceTriggering->isChecked() )
	{
		int row = ui.tableAdvancedTriggering->currentRow();
		QTableWidgetItem* q = ui.tableAdvancedTriggering->item(row, c_TriggerCondition);

        QString strText = ( CS_TRIG_SOURCE_DISABLE == i32TrigSource ) ? c_DisabledString : ui.cbxTriggerCondition->currentText();
        uInt32 u32Condition = ( CS_TRIG_SOURCE_DISABLE == i32TrigSource ) ? CS_TRIG_COND_POS_SLOPE : u32Trigcond;

		q->setText(strText);
		q->setData(Qt::UserRole, QVariant::fromValue(u32Condition));
        m_vecTrigParams[u32Engine].u32Condition = u32Condition;
	}
	else
	{
        CurrIdx = ui.cbxTriggerCondition->currentIndex();
        m_vecTrigParams[u32Engine].u32Condition = ui.cbxTriggerCondition->itemData(CurrIdx, Qt::UserRole).toUInt();
	}

    m_bParmsLoaded = true;
}

void QConfigDialog::s_ChangeTriggerLevel(int)
{
    if (!m_bParmsLoaded) return;
    //qDebug() << "s_ChangeTriggerLevel";

    m_bParmsLoaded = false;

    int CurrIdx;

	int32 i32Level = ui.spbxTriggerLevel->text().toInt();

	uInt32 u32Engine = ( ui.ckbAdvanceTriggering->isChecked() ) ? ui.tableAdvancedTriggering->currentRow() : 0;

	if ( ui.ckbAdvanceTriggering->isChecked() )
	{
		int row = ui.tableAdvancedTriggering->currentRow();
		QTableWidgetItem* q = ui.tableAdvancedTriggering->item(row, c_TriggerLevel);

        CurrIdx = ui.cbxTriggerSource->currentIndex();
        int i32TrigSource = ui.cbxTriggerSource->itemData(CurrIdx, Qt::UserRole).toInt();

        QString strText = ( CS_TRIG_SOURCE_DISABLE == i32TrigSource ) ? c_DisabledString : ui.spbxTriggerLevel->text();
        int32 i32TriggerLevel = ( CS_TRIG_SOURCE_DISABLE == i32TrigSource ) ? 0 : i32Level;

		q->setText(strText);
        q->setData(Qt::UserRole, QVariant::fromValue(i32TriggerLevel));
        m_vecTrigParams[u32Engine].i32Level = i32TriggerLevel;
	}
	else
	{
		m_vecTrigParams[u32Engine].i32Level = i32Level;
	}

    m_bParmsLoaded = true;
}

void QConfigDialog::s_EnableTriggerAC(bool bState)
{
    Q_UNUSED (bState);
    if (!m_bParmsLoaded) return;
    //qDebug() << "s_EnableTriggerAC";

    m_bParmsLoaded = false;

    if(bState)
    {
        ui.rbtnTriggerDC->blockSignals(true);
        ui.rbtnTriggerDC->setChecked(false);
        ui.rbtnTriggerDC->blockSignals(false);
    }
    else
    {
        ui.rbtnTriggerAC->blockSignals(true);
        ui.rbtnTriggerAC->setChecked(true);
        ui.rbtnTriggerAC->blockSignals(false);
        m_bParmsLoaded = true;
        return;
    }

	uInt32 u32Engine = ( ui.ckbAdvanceTriggering->isChecked() ) ? ui.tableAdvancedTriggering->currentRow() : 0;
	m_vecTrigParams[u32Engine].u32ExtCoupling = CS_COUPLING_AC;

    m_bParmsLoaded = true;
}

void QConfigDialog::s_EnableTriggerDC(bool bState)
{
        Q_UNUSED (bState);
    if (!m_bParmsLoaded) return;
    //qDebug() << "s_EnableTriggerDC";

    m_bParmsLoaded = false;

    if(bState)
    {
        ui.rbtnTriggerAC->blockSignals(true);
        ui.rbtnTriggerAC->setChecked(false);
        ui.rbtnTriggerAC->blockSignals(false);
    }
    else
    {
        ui.rbtnTriggerDC->blockSignals(true);
        ui.rbtnTriggerDC->setChecked(true);
        ui.rbtnTriggerDC->blockSignals(false);
        m_bParmsLoaded = true;
        return;
    }

	uInt32 u32Engine = ( ui.ckbAdvanceTriggering->isChecked() ) ? ui.tableAdvancedTriggering->currentRow() : 0;
	m_vecTrigParams[u32Engine].u32ExtCoupling = CS_COUPLING_DC;

    m_bParmsLoaded = true;
}

void QConfigDialog::s_EnableTriggerHZ(bool bState)
{
    Q_UNUSED (bState);
    if (!m_bParmsLoaded) return;
    //qDebug() << "s_EnableTriggerHZ";

    m_bParmsLoaded = false;

    if(bState)
    {
        ui.rbtnTriggerFiftyOhm->blockSignals(true);
        ui.rbtnTriggerFiftyOhm->setChecked(false);
        ui.rbtnTriggerFiftyOhm->blockSignals(false);
    }
    else
    {
        ui.rbtnTriggerHighZ->blockSignals(true);
        ui.rbtnTriggerHighZ->setChecked(true);
        ui.rbtnTriggerHighZ->blockSignals(false);
        m_bParmsLoaded = true;
        return;
    }

	uInt32 u32Engine = ( ui.ckbAdvanceTriggering->isChecked() ) ? ui.tableAdvancedTriggering->currentRow() : 0;
	m_vecTrigParams[u32Engine].u32ExtImpedance = CS_REAL_IMP_1M_OHM;

    m_bParmsLoaded = true;
}

void QConfigDialog::s_EnableTrigger50Ohm(bool bState)
{
    Q_UNUSED (bState);
    if (!m_bParmsLoaded) return;
    //qDebug() << "s_EnableTrigger50Ohm";

    m_bParmsLoaded = false;

    if(bState)
    {
        ui.rbtnTriggerHighZ->blockSignals(true);
        ui.rbtnTriggerHighZ->setChecked(false);
        ui.rbtnTriggerHighZ->blockSignals(false);
    }
    else
    {
        ui.rbtnTriggerFiftyOhm->blockSignals(true);
        ui.rbtnTriggerFiftyOhm->setChecked(true);
        ui.rbtnTriggerFiftyOhm->blockSignals(false);
        m_bParmsLoaded = true;
        return;
    }

	uInt32 u32Engine = ( ui.ckbAdvanceTriggering->isChecked() ) ? ui.tableAdvancedTriggering->currentRow() : 0;
	m_vecTrigParams[u32Engine].u32ExtImpedance = CS_REAL_IMP_50_OHM;

    m_bParmsLoaded = true;
}

void QConfigDialog::s_EnableTrigger5V(bool bState)
{
    Q_UNUSED (bState);
    if (!m_bParmsLoaded) return;
    //qDebug() << "s_EnableTrigger5V";

    m_bParmsLoaded = false;

    if(bState)
    {
        ui.rbtnTrigger3V3->blockSignals(true);
        ui.rbtnTrigger3V3->setChecked(false);
        ui.rbtnTrigger3V3->blockSignals(false);

        ui.rbtnTriggerOneVolt->blockSignals(true);
        ui.rbtnTriggerOneVolt->setChecked(false);
        ui.rbtnTriggerOneVolt->blockSignals(false);

        ui.rbtnTriggerPlus5V->blockSignals(true);
        ui.rbtnTriggerPlus5V->setChecked(false);
        ui.rbtnTriggerPlus5V->blockSignals(false);
    }
    else
    {
        ui.rbtnTriggerFiveVolt->blockSignals(true);
        ui.rbtnTriggerFiveVolt->setChecked(true);
        ui.rbtnTriggerFiveVolt->blockSignals(false);
        m_bParmsLoaded = true;
        return;
    }

	uInt32 u32Engine = ( ui.ckbAdvanceTriggering->isChecked() ) ? ui.tableAdvancedTriggering->currentRow() : 0;
	m_vecTrigParams[u32Engine].u32Range = CS_GAIN_10_V;

    m_bParmsLoaded = true;
}

void QConfigDialog::s_EnableTrigger1V(bool bState)
{
    Q_UNUSED (bState);
    if (!m_bParmsLoaded) return;
    //qDebug() << "s_EnableTrigger1V";

    m_bParmsLoaded = false;

    if(bState)
    {
        ui.rbtnTrigger3V3->blockSignals(true);
        ui.rbtnTrigger3V3->setChecked(false);
        ui.rbtnTrigger3V3->blockSignals(false);

        ui.rbtnTriggerFiveVolt->blockSignals(true);
        ui.rbtnTriggerFiveVolt->setChecked(false);
        ui.rbtnTriggerFiveVolt->blockSignals(false);

        ui.rbtnTriggerPlus5V->blockSignals(true);
        ui.rbtnTriggerPlus5V->setChecked(false);
        ui.rbtnTriggerPlus5V->blockSignals(false);
    }
    else
    {
        ui.rbtnTriggerOneVolt->blockSignals(true);
        ui.rbtnTriggerOneVolt->setChecked(true);
        ui.rbtnTriggerOneVolt->blockSignals(false);
        m_bParmsLoaded = true;
        return;
    }

	uInt32 u32Engine = ( ui.ckbAdvanceTriggering->isChecked() ) ? ui.tableAdvancedTriggering->currentRow() : 0;
	m_vecTrigParams[u32Engine].u32Range = CS_GAIN_2_V;

    m_bParmsLoaded = true;
}

void QConfigDialog::s_EnableTrigger3V3(bool bState)
{
    Q_UNUSED (bState);
    if (!m_bParmsLoaded) return;
    //qDebug() << "s_EnableTrigger3V3";

    m_bParmsLoaded = false;

    if(bState)
    {
        ui.rbtnTriggerOneVolt->blockSignals(true);
        ui.rbtnTriggerOneVolt->setChecked(false);
        ui.rbtnTriggerOneVolt->blockSignals(false);

        ui.rbtnTriggerFiveVolt->blockSignals(true);
        ui.rbtnTriggerFiveVolt->setChecked(false);
        ui.rbtnTriggerFiveVolt->blockSignals(false);

        ui.rbtnTriggerPlus5V->blockSignals(true);
        ui.rbtnTriggerPlus5V->setChecked(false);
        ui.rbtnTriggerPlus5V->blockSignals(false);
    }
    else
    {
        ui.rbtnTrigger3V3->blockSignals(true);
        ui.rbtnTrigger3V3->setChecked(true);
        ui.rbtnTrigger3V3->blockSignals(false);
        m_bParmsLoaded = true;
        return;
    }

    uInt32 u32Engine = ( ui.ckbAdvanceTriggering->isChecked() ) ? ui.tableAdvancedTriggering->currentRow() : 0;
    m_vecTrigParams[u32Engine].u32Range = 3300;

    m_bParmsLoaded = true;
}

void QConfigDialog::s_EnableTriggerPlus5V(bool bState)
{
    Q_UNUSED (bState);
    if (!m_bParmsLoaded) return;
    //qDebug() << "s_EnableTriggerPlus5V";

    m_bParmsLoaded = false;

    if(bState)
    {
        ui.rbtnTrigger3V3->blockSignals(true);
        ui.rbtnTrigger3V3->setChecked(false);
        ui.rbtnTrigger3V3->blockSignals(false);

        ui.rbtnTriggerOneVolt->blockSignals(true);
        ui.rbtnTriggerOneVolt->setChecked(false);
        ui.rbtnTriggerOneVolt->blockSignals(false);

        ui.rbtnTriggerFiveVolt->blockSignals(true);
        ui.rbtnTriggerFiveVolt->setChecked(false);
        ui.rbtnTriggerFiveVolt->blockSignals(false);
    }
    else
    {
        ui.rbtnTriggerPlus5V->blockSignals(true);
        ui.rbtnTriggerPlus5V->setChecked(true);
        ui.rbtnTriggerPlus5V->blockSignals(false);
        m_bParmsLoaded = true;
        return;
    }

    uInt32 u32Engine = ( ui.ckbAdvanceTriggering->isChecked() ) ? ui.tableAdvancedTriggering->currentRow() : 0;
    m_vecTrigParams[u32Engine].u32Range = CS_GAIN_5_V;

    m_bParmsLoaded = true;
}

void QConfigDialog::s_AdvancedTriggerItemClicked(QTableWidgetItem* widget)
{
    if (!m_bParmsLoaded) return;
    qDebug() << "s_AdvancedTriggerItemClicked";

    m_bParmsLoaded = false;

	int row = ui.tableAdvancedTriggering->row(widget);
	QTableWidgetItem* q = ui.tableAdvancedTriggering->item(row, c_TriggerSource);
	QString str = q->text();
	int i32Source = q->data(Qt::UserRole).toInt();

    qDebug() << "s_AdvancedTriggerItemClicked: TrigSrc: " << i32Source;

    blockSignals(true);

	int selection = ui.cbxTriggerSource->findData(QVariant::fromValue(i32Source));
	int32 currentSelection = ui.cbxTriggerSource->findData(QVariant::fromValue(i32Source));
	ui.cbxTriggerSource->setCurrentIndex(currentSelection);

	uInt32 u32Condition = m_vecTrigParams[row].u32Condition;
	selection = ui.cbxTriggerCondition->findData(QVariant::fromValue(u32Condition));
	ui.cbxTriggerCondition->setCurrentIndex(selection);

	int i32Level = m_vecTrigParams[row].i32Level;
	selection = ui.cbxTriggerCondition->findData(QVariant::fromValue(i32Level));
	ui.spbxTriggerLevel->setValue(i32Level);

	// set the rest of the items
	if ( CS_TRIG_SOURCE_EXT == i32Source )
	{
        ui.boxExternalTriggerParameters->setEnabled(true);

        qDebug() << "s_AdvancedTriggerItemClicked: TrigSrc Ext";
        bool bFlag = (CS_COUPLING_DC == m_vecTrigParams[row].u32ExtCoupling);
        qDebug() << "s_AdvancedTriggerItemClicked: TrigSrc Ext: bFlag: " << bFlag;
        ui.rbtnTriggerAC->setChecked(!bFlag);
        ui.rbtnTriggerAC->setEnabled(!bFlag);
        ui.rbtnTriggerDC->setChecked(bFlag);
        ui.rbtnTriggerDC->setEnabled(bFlag);

		bFlag = CS_GAIN_2_V == m_vecTrigParams[row].u32Range;
        qDebug() << "s_AdvancedTriggerItemClicked: TrigSrc Ext: bFlag: " << bFlag;
		ui.rbtnTriggerOneVolt->setChecked(bFlag);
        ui.rbtnTriggerOneVolt->setEnabled(bFlag);

        bFlag = CS_GAIN_10_V == m_vecTrigParams[row].u32Range;
        qDebug() << "s_AdvancedTriggerItemClicked: TrigSrc Ext: bFlag: " << bFlag;
        ui.rbtnTriggerFiveVolt->setChecked(bFlag);
        ui.rbtnTriggerFiveVolt->setEnabled(bFlag);

        bFlag = 3300 == m_vecTrigParams[row].u32Range;
        qDebug() << "s_AdvancedTriggerItemClicked: TrigSrc Ext: bFlag: " << bFlag;
        ui.rbtnTrigger3V3->setChecked(bFlag);
        ui.rbtnTrigger3V3->setEnabled(bFlag);

        bFlag = (CS_GAIN_5_V == m_vecTrigParams[row].u32Range);
        qDebug() << "s_AdvancedTriggerItemClicked: TrigSrc Ext: bFlag: " << bFlag;
        ui.rbtnTriggerPlus5V->setChecked(bFlag);
        ui.rbtnTriggerPlus5V->setEnabled(bFlag);

        bFlag = (CS_REAL_IMP_50_OHM == m_vecTrigParams[row].u32ExtImpedance);
        qDebug() << "s_AdvancedTriggerItemClicked: TrigSrc Ext: bFlag: " << bFlag;
		ui.rbtnTriggerFiftyOhm->setChecked(bFlag);
        ui.rbtnTriggerFiftyOhm->setEnabled(bFlag);
		ui.rbtnTriggerHighZ->setChecked(!bFlag);
        ui.rbtnTriggerHighZ->setEnabled(!bFlag);
	}
	else
	{
        ui.boxExternalTriggerParameters->setEnabled(false);
	}

    blockSignals(false);

    m_bParmsLoaded = true;

    qDebug() << "s_AdvancedTriggerItemClicked: Done";
}

void QConfigDialog::SetExpertFunction(int idxImage, ExpertOptions Option)
{
	int CommitTime = 0;

	uInt32 u32Mode = m_pGageSystem->getAcquisitionMode();
	uInt32 ModeMask = ~CS_MODE_USER1;
	ModeMask &= ~CS_MODE_USER2;
	u32Mode &= ModeMask;

	//Set mode.
	if (idxImage==1)
	{
		u32Mode |= CS_MODE_USER1;
		ui.ckbExpert1->setChecked(true);
		ui.ckbExpert2->setChecked(false);
	}
	else if(idxImage==2)
	{
		u32Mode |= CS_MODE_USER2;
		ui.ckbExpert1->setChecked(false);
		ui.ckbExpert2->setChecked(true);
	}
	else
	{
		ui.ckbExpert1->setChecked(false);
		ui.ckbExpert2->setChecked(false);
	}
	m_pGageSystem->setAcquisitionMode(u32Mode);

	switch (Option)
	{
		case EXPERT_MULREC_AVG_TD:
			m_pGageSystem->SetMulRecAvgCount(ui.spinBox_AvgCount->value());
			break;

		default:
			m_pGageSystem->SetMulRecAvgCount(1);
			break;
	}

	int32 i32Status = m_pGageSystem->PrepareForCapture(&CommitTime);
	if (CS_FAILED(i32Status))
	{
        m_pErrMng->UpdateErrMsg("Error saving the new parameters.", i32Status);
		emit ConfigError();
		return;
	}

	GetAcquisitionParameters();
	UpdateUiAcquisitionParameters();

    m_MutexSharedVars->lock();
    *m_bParmsChanged = true;
    m_MutexSharedVars->unlock();

}

void QConfigDialog::UnsetExpertFunctions()
{
	int CommitTime;


	uInt32 u32Mode = m_pGageSystem->getAcquisitionMode();
	uInt32 ModeMask = ~CS_MODE_USER1;
	ModeMask &= ~CS_MODE_USER2;
	u32Mode &= ModeMask;

	ui.ckbExpert1->setChecked(false);
	ui.ckbExpert2->setChecked(false);
	ui.ckbExpert1->update();
	ui.ckbExpert2->update();

	u32Mode &= ModeMask;
	m_pGageSystem->setAcquisitionMode(u32Mode);

	int32 i32Status = m_pGageSystem->PrepareForCapture(&CommitTime);
	if (CS_FAILED(i32Status))
	{
        m_pErrMng->UpdateErrMsg("Error saving the new parameters.", i32Status);
		emit ConfigError();
		return;
	}

	GetAcquisitionParameters();
	UpdateUiAcquisitionParameters();

    m_MutexSharedVars->lock();
    *m_bParmsChanged = true;
    m_MutexSharedVars->unlock();

}

void QConfigDialog::paintEvent(QPaintEvent* pEvent)
{
    Q_UNUSED (pEvent);
}


void QConfigDialog::WaitUpdateScopeParmsDone()
{
    //unsigned long long count = 0;

    while(*m_bUpdateScopeParmsDone==false)
    {
        //if((count % 3000)==0) QCoreApplication::processEvents();
        quSleep(10);
    }
}

void QConfigDialog::SetAddrDisplayOneSeg(bool* bDisplayOneSeg)
{
    m_bDisplayOneSeg = bDisplayOneSeg;
}

void QConfigDialog::SetAddrCurrSeg(int64* i64CurrSeg)
{
    m_i64CurrSeg = i64CurrSeg;
}

void QConfigDialog::SetAddrParmsChanged(bool* bParmsChanged)
{
    m_bParmsChanged = bParmsChanged;
}

void QConfigDialog::SetAddrMutexSharedVars(QMutex* MutexSharedVars)
{
    m_MutexSharedVars = MutexSharedVars;
}

void QConfigDialog::PopulateRangeComboBox()
{
    //qDebug() << "Populate range combo box";

    //qDebug() << "Populate range combo box: Impedance: " << m_vecChanParams[0].u32Impedance;
    InputRangeTable* pIRTable = m_pGageSystem->GetInputRangeTable(m_vecChanParams[0].u32Impedance);

    // make sure combo box is empty first
    ui.cbxInputRange->clear();

    QString strCurrRange;
    strCurrRange.clear();

    for ( int i = 0; i < pIRTable->nItemCount; i++ )
    {
        // the fromLatin1 conversion needs to be done or the +/- doesn't come out right
        strCurrRange.clear();
        strCurrRange.append(QString::fromLatin1(pIRTable->pRangeTable[i].strText));
        //If "±" is not the first character, truncate the string starting at the second position.
        //This occurs with virtual systems
        if(strCurrRange.at(0)!=QChar((uchar)241))
        {
            //qDebug() << "Truncate range string";
            strCurrRange = strCurrRange.mid(1);
        }
        ui.cbxInputRange->addItem(strCurrRange, QVariant::fromValue(pIRTable->pRangeTable[i].u32InputRange));
    }
}



