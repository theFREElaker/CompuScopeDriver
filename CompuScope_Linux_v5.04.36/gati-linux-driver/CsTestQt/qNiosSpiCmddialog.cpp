#include "qNiosSpiCmddialog.h"
#include "ui_qNiosSpiCmddialog.h"
#include "CsPrivatePrototypes.h"
#include "CsPrivateStruct.h"

NiosSpiCmdDialog::NiosSpiCmdDialog(QWidget *parent, CGageSystem* pGageSystem) :
    QDialog(parent),
	ui(new Ui::NiosSpiCmdDialog)
{
    ui->setupUi(this);
	m_pGageSystem = pGageSystem;
	ui->lineEdit_DataRead->setVisible(true);
	ui->comboBox_Cmd->clear();
	ui->comboBox_Cmd->setMaxCount(5);
	ui->comboBox_Cmd->addItem(QIcon(), QString("0x800000"));
	ui->lineEdit_Data->setText(QString("0x0000"));
}

NiosSpiCmdDialog::~NiosSpiCmdDialog()
{
    delete ui;
}

void NiosSpiCmdDialog::s_Show()
{
	show();
}

void NiosSpiCmdDialog::s_SendCmd()
{
	int res = 0;
	bool bConvOk;
	QString strCommand;
	QString strStatus;
	strStatus.clear();

	m_pGageSystem->getSystemHandle();

	CS_SEND_NIOS_CMD_STRUCT NiosStruct;
	NiosStruct.bRead = ui->checkBox_ReadData->isChecked();
	NiosStruct.u16CardIndex = 1;
	NiosStruct.u32Timeout = ui->spinBox_Timeout->value() * 10;//value * 100usec

	strCommand = ui->comboBox_Cmd->currentText();

	if (strCommand.startsWith("0x", Qt::CaseInsensitive)) NiosStruct.u32Command = strCommand.mid(2, -1).toUInt(&bConvOk, 16);
	else NiosStruct.u32Command = strCommand.mid(0, -1).toUInt(&bConvOk, 10);

	if(ui->lineEdit_Data->text().startsWith("0x", Qt::CaseInsensitive)) NiosStruct.u32DataIn = ui->lineEdit_Data->text().mid(2, -1).toUInt(&bConvOk, 16);
	else NiosStruct.u32DataIn = ui->lineEdit_Data->text().mid(0, -1).toUInt(&bConvOk, 10);

	res = m_pGageSystem->CsSetInfo(m_pGageSystem->getSystemHandle(), 0, CS_SEND_NIOS_CMD, &NiosStruct);
	if(res<0)
	{
		ui->lineEdit_DataRead->setText("");
		strStatus.append("Error sending NIOS command\r\n");
		strStatus += m_pGageSystem->GetErrorString(res);
		ui->textEdit_Status->setText(strStatus);
	}
	else
	{
		if (NiosStruct.bRead) ui->lineEdit_DataRead->setText(QString::number(NiosStruct.u32DataOut, 16));
		else ui->lineEdit_DataRead->setText("");
		bool bCmdListed = false;
		for (int idxCmd = 0; idxCmd < ui->comboBox_Cmd->maxCount(); idxCmd++)
		{
			if (strCommand == ui->comboBox_Cmd->itemText(idxCmd))
			{
				bCmdListed = true;
				break;
			}
		}
		if (!bCmdListed) ui->comboBox_Cmd->insertItem(0, QIcon(), strCommand);
		ui->textEdit_Status->setText("Successfully sent NIOS command\r\n");
	}
}

