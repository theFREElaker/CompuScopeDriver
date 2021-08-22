#include "qReadSigFiledialog.h"
#include "ui_qReadSigFiledialog.h"
#include "CsPrivatePrototypes.h"
#include "CsPrivateStruct.h"
#include <QDebug>

ReadSigFileDialog::ReadSigFileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReadSigFileDialog)
{
    ui->setupUi(this);

    ui->lineEdit_FilePath->setText("Please select the .sig file...");

    m_bCancelled = false;
    m_bOk = false;
}

ReadSigFileDialog::~ReadSigFileDialog()
{
    delete ui;
}

void ReadSigFileDialog::showEvent(QShowEvent * event)
{
    UNREFERENCED_PARAMETER(event);
    qDebug() << "ReadSigFileDialog: Show event";

    m_bCancelled = true;
    m_bOk = false;
    //show();
}

void ReadSigFileDialog::s_Show()
{
    qDebug() << "ReadSigFileDialog: OK";

    m_bCancelled = true;
    m_bOk = false;
	show();
}

void ReadSigFileDialog::s_SelFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Select a .sig file", QString(), "Gage Signal files (*.sig);;All files (*.*)");

	ui->lineEdit_FilePath->setText(fileName);
}

void ReadSigFileDialog::s_Close()
{
    qDebug() << "ReadSigFileDialog: OK";

    m_bOk = true;
    m_bCancelled = false;
	m_DataFilePath = ui->lineEdit_FilePath->text();

    hide();
}


