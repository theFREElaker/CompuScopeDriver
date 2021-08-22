#include "qMulrecdialog.h"
#include "ui_qMulrecdialog.h"
#include "CsPrivatePrototypes.h"
#include "CsPrivateStruct.h"

MulrecDialog::MulrecDialog(QWidget *parent, CGageSystem* pGageSystem, AcquisitionWorker* pAcqWorker) :
    QDialog(parent),
	ui(new Ui::MulrecDialog)
{
    ui->setupUi(this);
	m_pGageSystem = pGageSystem;
	m_pAcqWorker = pAcqWorker;
	m_bInit = false;
	ui->radioButton_AllSegs->setChecked(true);
	ui->radioButton_OnlySegs->setChecked(false);
	ui->checkBox_OneSeg->setChecked(false);

	ui->spinBox_FromSeg->setEnabled(false);
	ui->spinBox_ToSeg->setEnabled(false);
	ui->spinBox_SegCount->setEnabled(false);
}

MulrecDialog::~MulrecDialog()
{
    delete ui;
}

void MulrecDialog::s_Show()
{
	int SegCount = m_pGageSystem->getSegmentCount();

	ui->spinBox_FromSeg->setMinimum(1);
	ui->spinBox_FromSeg->setMaximum(SegCount);
	ui->spinBox_ToSeg->setMinimum(1);
	ui->spinBox_ToSeg->setMaximum(SegCount);
	ui->spinBox_SegCount->setMinimum(1);
	ui->spinBox_SegCount->setMaximum(SegCount);

	if (m_pAcqWorker->getAllSegsMode())
	{
		ui->spinBox_FromSeg->setValue(1);
		ui->spinBox_ToSeg->setValue(SegCount);
	}
	else
	{
		ui->spinBox_FromSeg->setValue(m_pAcqWorker->getDispStartSeg());
		ui->spinBox_ToSeg->setValue(m_pAcqWorker->getDispStopSeg());
	}

	ui->spinBox_SegCount->setValue(SegCount);

	m_bAllSegsLastSt = ui->radioButton_AllSegs->isChecked();
	m_bOnlySegsLastSt = ui->radioButton_OnlySegs->isChecked();
	m_bOneSegLastSt = ui->checkBox_OneSeg->isChecked();
	m_i32FromSegLastSt = ui->spinBox_FromSeg->value();
	m_i32ToSegLastSt = ui->spinBox_ToSeg->value();

	if (ui->spinBox_ToSeg->value() > ui->spinBox_SegCount->value())
	{
		ui->spinBox_ToSeg->setValue(SegCount);
	}

	if (ui->spinBox_FromSeg->value() > ui->spinBox_ToSeg->value())
	{
		ui->spinBox_FromSeg->setValue(ui->spinBox_ToSeg->value());
	}

	show();
}

void MulrecDialog::s_Cancel()
{
	ui->radioButton_AllSegs->setChecked(m_bAllSegsLastSt);
	ui->radioButton_OnlySegs->setChecked(m_bOnlySegsLastSt);
	ui->checkBox_OneSeg->setChecked(m_bOneSegLastSt);
	ui->spinBox_FromSeg->setValue(m_i32FromSegLastSt);
	ui->spinBox_ToSeg->setValue(m_i32ToSegLastSt);

	close();
}

void MulrecDialog::s_Ok()
{
	if (ui->checkBox_OneSeg->isChecked())
	{
		m_pAcqWorker->setAllSegsMode(false);

		if (ui->radioButton_AllSegs->isChecked())
		{
			m_pAcqWorker->setDispStartSeg(1);
			m_pAcqWorker->setDispStopSeg(1);
		}
		else
		{
			m_pAcqWorker->setDispStartSeg(ui->spinBox_FromSeg->value());
			m_pAcqWorker->setDispStopSeg(ui->spinBox_FromSeg->value());
		}
	}
	else
	{
		if (ui->radioButton_AllSegs->isChecked())
		{
			m_pAcqWorker->setAllSegsMode(true);
			m_pAcqWorker->setDispStartSeg(1);
			m_pAcqWorker->setDispStopSeg(m_pGageSystem->getSegmentCount());
		}
		else
		{
			m_pAcqWorker->setAllSegsMode(false);
			m_pAcqWorker->setDispStartSeg(ui->spinBox_FromSeg->value());
			m_pAcqWorker->setDispStopSeg(ui->spinBox_ToSeg->value());
		}
	}

	close();
}

void MulrecDialog::s_ModeChanged(bool bAllSegs)
{
	if (bAllSegs)
	{
		ui->spinBox_FromSeg->setEnabled(false);
		ui->spinBox_ToSeg->setEnabled(false);
		ui->spinBox_SegCount->setEnabled(false);
	}
	else
	{
		ui->spinBox_FromSeg->setEnabled(true);
		ui->spinBox_ToSeg->setEnabled(true);
		ui->spinBox_SegCount->setEnabled(true);
	}
}

