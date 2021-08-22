#include "qReadWriteRegsdialog.h"
#include "ui_qReadWriteRegsdialog.h"
#include "CsPrivatePrototypes.h"
#include "CsPrivateStruct.h"

ReadWriteRegsDialog::ReadWriteRegsDialog(QWidget *parent, CGageSystem* pGageSystem) :
    QDialog(parent),
	ui(new Ui::ReadWriteRegsDialog)
{
    ui->setupUi(this);
	m_pGageSystem = pGageSystem;
	ui->lineEdit_Address->setText("0x34");
}

ReadWriteRegsDialog::~ReadWriteRegsDialog()
{
    delete ui;
}

void ReadWriteRegsDialog::s_Show()
{
	show();
}

void ReadWriteRegsDialog::s_WriteReg()
{
	int res = 0;
	bool bConvOk;
	QString strStatus;
	strStatus.clear();

	m_pGageSystem->getSystemHandle();

    CS_RW_GAGE_REGISTER_STRUCT GageRegStruct;
	GageRegStruct.u16CardIndex = 1;

    if(ui->lineEdit_Address->text().startsWith("0x", Qt::CaseInsensitive)) GageRegStruct.u32Offset = ui->lineEdit_Address->text().mid(2, -1).toUShort(&bConvOk, 16);
    else GageRegStruct.u32Offset = ui->lineEdit_Address->text().mid(0, -1).toUShort(&bConvOk, 10);

	if(ui->lineEdit_ValWrite->text().startsWith("0x", Qt::CaseInsensitive)) GageRegStruct.u32Data = ui->lineEdit_ValWrite->text().mid(2, -1).toUInt(&bConvOk, 16);
	else GageRegStruct.u32Data = ui->lineEdit_ValWrite->text().mid(0, -1).toUInt(&bConvOk, 10);

	res = m_pGageSystem->CsSetInfo(m_pGageSystem->getSystemHandle(), 0, CS_WRITE_GAGE_REGISTER, &GageRegStruct);
	if(res<0)
	{
		strStatus.append("Error writing to Gage register\r\n");
		strStatus += m_pGageSystem->GetErrorString(res);
		ui->textEdit_Status->setText(strStatus);
	}
	else ui->textEdit_Status->setText("Successfully wrote to Gage register\r\n");
}

void ReadWriteRegsDialog::s_ReadReg()
{
	int res = 0;
	bool bConvOk;
	QString strStatus;
	strStatus.clear();

	m_pGageSystem->getSystemHandle();

    CS_RW_GAGE_REGISTER_STRUCT GageRegStruct;
	GageRegStruct.u16CardIndex = 1;
	//GageRegStruct.u32Data = 0;

    if(ui->lineEdit_Address->text().startsWith("0x", Qt::CaseInsensitive)) GageRegStruct.u32Offset = ui->lineEdit_Address->text().mid(2, -1).toUShort(&bConvOk, 16);
    else GageRegStruct.u32Offset = ui->lineEdit_Address->text().mid(0, -1).toUShort(&bConvOk, 10);

	res = m_pGageSystem->CsGetInfo(m_pGageSystem->getSystemHandle(), 0, CS_READ_GAGE_REGISTER, &GageRegStruct);
	if(res<0)
	{
		strStatus.append("Error reading from Gage register\r\n");
		strStatus += m_pGageSystem->GetErrorString(res);
		ui->textEdit_Status->setText(strStatus);
	}
	else
	{
		ui->lineEdit_ValRead->setText(QString::number((uint)GageRegStruct.u32Data, 16));
		ui->textEdit_Status->setText("Successfully read from Gage register\r\n");
	}
	
}
