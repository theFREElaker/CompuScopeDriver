#include "qReadDataFiledialog.h"
#include "ui_qReadDataFiledialog.h"
#include "CsPrivatePrototypes.h"
#include "CsPrivateStruct.h"
#include <QDebug>

ReadDataFileDialog::ReadDataFileDialog(QWidget *parent) :
    QDialog(parent),
	ui(new Ui::ReadDataFileDialog)
{
    ui->setupUi(this);
	ui->comboBox_NbChans->addItem("1");
	ui->comboBox_NbChans->addItem("2");
	ui->comboBox_NbChans->addItem("4");
	ui->comboBox_NbChans->addItem("8");

	ui->comboBox_SampSize->addItem("1");
	ui->comboBox_SampSize->addItem("2");

	ui->lineEdit_FilePath->setText("Please select the Data file...");

    m_bCancelled = false;
    m_bOk = false;
}

ReadDataFileDialog::~ReadDataFileDialog()
{
    delete ui;
}

void ReadDataFileDialog::showEvent(QShowEvent * event)
{
    UNREFERENCED_PARAMETER(event);
    qDebug() << "ReadDataFileDialog: Show event";

    m_bCancelled = true;
    m_bOk = false;
    //show();
}

void ReadDataFileDialog::s_Show()
{
    qDebug() << "ReadDataFileDialog: OK";

    m_bCancelled = true;
    m_bOk = false;
	show();
}

void ReadDataFileDialog::s_SelFile()
{
	QString fileName = QFileDialog::getOpenFileName(this, "Select a data file", QString(), "Data files (*.dat);;All files (*.*)");

	ui->lineEdit_FilePath->setText(fileName);
}

void ReadDataFileDialog::s_Close()
{
    qDebug() << "ReadDataFileDialog: OK";

    m_bOk = true;
    m_bCancelled = false;

	m_i32ChanCount = ui->comboBox_NbChans->currentText().toInt();
	m_i32SampleSize = ui->comboBox_SampSize->currentText().toInt();
	m_DataFilePath = ui->lineEdit_FilePath->text();

    hide();
}


