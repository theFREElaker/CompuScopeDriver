#include "qdebugdialog.h"
#include "ui_qdebugdialog.h"
#include "CsPrivatePrototypes.h"
#include "CsPrivateStruct.h"

DebugDialog::DebugDialog(QWidget *parent, CGageSystem* pGageSystem) :
    QDialog(parent),
    ui(new Ui::DebugDialog)
{
    ui->setupUi(this);
	m_pGageSystem = pGageSystem;
	ui->lineEdit_ValRead->setVisible(false);
}

DebugDialog::~DebugDialog()
{
    delete ui;
}

void DebugDialog::s_Show()
{
	show();
}

void DebugDialog::s_WriteReg()
{
	int res = 0;
	bool bConvOk;
	QString strStatus;
	strStatus.clear();

	m_pGageSystem->getSystemHandle();

	if(ui->radioButton_NiosSpi->isChecked())
	{
		CS_SEND_NIOS_CMD_STRUCT NiosStruct;
		NiosStruct.bRead = true;
		NiosStruct.u16CardIndex = 1;
		NiosStruct.u32Timeout = 10;//10 * 100usec

		if(ui->lineEdit_Address->text().startsWith("0x", Qt::CaseInsensitive)) NiosStruct.u32Command = ui->lineEdit_Address->text().mid(2, -1).toUInt(&bConvOk, 16);
		else NiosStruct.u32Command = ui->lineEdit_Address->text().mid(0, -1).toUInt(&bConvOk, 10);

		if(ui->lineEdit_ValWrite->text().startsWith("0x", Qt::CaseInsensitive)) NiosStruct.u32DataIn = ui->lineEdit_ValWrite->text().mid(2, -1).toUInt(&bConvOk, 16);
		else NiosStruct.u32DataIn = ui->lineEdit_ValWrite->text().mid(0, -1).toUInt(&bConvOk, 10);

		res = m_pGageSystem->CsSetInfo(m_pGageSystem->getSystemHandle(), 123456, CS_SEND_NIOS_CMD, &NiosStruct);
		if(res<0)
		{
			ui->lineEdit_ValRead->setText("0");
			strStatus.append("Error sending NIOS command\r\n");
			strStatus += m_pGageSystem->GetErrorString(res);
			ui->textEdit_Status->setText(strStatus);
		}
		else
		{
			ui->lineEdit_ValRead->setText(QString::number(NiosStruct.u32DataOut, 10));
			ui->textEdit_Status->setText("Successfully sent NIOS command\r\n");
		}
	}
	else
	{
        CS_RW_GAGE_REGISTER_STRUCT GageRegStruct;
        //CS_RD_GAGE_REGISTER_STRUCT GageRegStruct;
		GageRegStruct.u16CardIndex = 1;

        //if(ui->lineEdit_Address->text().startsWith("0x", Qt::CaseInsensitive)) GageRegStruct.u16Offset = ui->lineEdit_Address->text().mid(2, -1).toUShort(&bConvOk, 16);
        //else GageRegStruct.u16Offset = ui->lineEdit_Address->text().mid(0, -1).toUShort(&bConvOk, 10);
        if(ui->lineEdit_Address->text().startsWith("0x", Qt::CaseInsensitive)) GageRegStruct.u32Offset = ui->lineEdit_Address->text().mid(2, -1).toUShort(&bConvOk, 16);
        else GageRegStruct.u32Offset = ui->lineEdit_Address->text().mid(0, -1).toUShort(&bConvOk, 10);

		if(ui->lineEdit_ValWrite->text().startsWith("0x", Qt::CaseInsensitive)) GageRegStruct.u32Data = ui->lineEdit_ValWrite->text().mid(2, -1).toUInt(&bConvOk, 16);
		else GageRegStruct.u32Data = ui->lineEdit_ValWrite->text().mid(0, -1).toUInt(&bConvOk, 10);

		res = m_pGageSystem->CsSetInfo(m_pGageSystem->getSystemHandle(), 123456, CS_WRITE_GAGE_REGISTER, &GageRegStruct);
		if(res<0)
		{
			strStatus.append("Error writing to Gage register\r\n");
			strStatus += m_pGageSystem->GetErrorString(res);
			ui->textEdit_Status->setText(strStatus);
		}
		else ui->textEdit_Status->setText("Successfully wrote to Gage register\r\n");
	}
}

void DebugDialog::s_ChangeMode(bool bState)
{
	if(bState)
	{
		//Gage register mode
		ui->label_Address->setText("Address");
		ui->pushButton_Read->setVisible(true);
		ui->pushButton_Write->setText("Write");
		ui->lineEdit_ValRead->setVisible(false);
	}
	else
	{
		//NIOS mode
		ui->label_Address->setText("Command");
		ui->pushButton_Read->setVisible(false);
		ui->pushButton_Write->setText("Execute");
		ui->lineEdit_ValRead->setVisible(true);
	}
}

void DebugDialog::s_ReadReg()
{
	int res = 0;
	bool bConvOk;
	QString strStatus;
	strStatus.clear();

	m_pGageSystem->getSystemHandle();

	if(ui->radioButton_NiosSpi->isChecked())
	{
		
	}
	else
	{
        CS_RW_GAGE_REGISTER_STRUCT GageRegStruct;
        //CS_RD_GAGE_REGISTER_STRUCT GageRegStruct;
		GageRegStruct.u16CardIndex = 1;
		GageRegStruct.u32Data = 0;

        //if(ui->lineEdit_Address->text().startsWith("0x", Qt::CaseInsensitive)) GageRegStruct.u16Offset = ui->lineEdit_Address->text().mid(2, -1).toUShort(&bConvOk, 16);
        //else GageRegStruct.u16Offset = ui->lineEdit_Address->text().mid(0, -1).toUShort(&bConvOk, 10);
        if(ui->lineEdit_Address->text().startsWith("0x", Qt::CaseInsensitive)) GageRegStruct.u32Offset = ui->lineEdit_Address->text().mid(2, -1).toUShort(&bConvOk, 16);
        else GageRegStruct.u32Offset = ui->lineEdit_Address->text().mid(0, -1).toUShort(&bConvOk, 10);

		res = m_pGageSystem->CsGetInfo(m_pGageSystem->getSystemHandle(), 123456, CS_READ_GAGE_REGISTER, &GageRegStruct);
		if(res<0)
		{
			strStatus.append("Error reading from Gage register\r\n");
			strStatus += m_pGageSystem->GetErrorString(res);
			ui->textEdit_Status->setText(strStatus);
		}
		else
		{
			ui->lineEdit_ValWrite->setText(QString::number((uint)GageRegStruct.u32Data, 10));
			ui->textEdit_Status->setText("Successfully read from Gage register\r\n");
		}
	}
}
