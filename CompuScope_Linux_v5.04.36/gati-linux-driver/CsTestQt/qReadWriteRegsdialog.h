#ifndef READWRITEREGSDIALOG_H
#define READWRITEREGSDIALOG_H

#include <QDialog>
#include "ui_qReadWriteRegsdialog.h"
#include "GageSystem.h"

namespace Ui {
class ReadWriteRegsDialog;
}

class ReadWriteRegsDialog : public QDialog
{
    Q_OBJECT

public:
	explicit ReadWriteRegsDialog(QWidget *parent = 0, CGageSystem* pGageSystem = NULL);
	~ReadWriteRegsDialog();

public slots:
	void s_Show();
	void s_WriteReg();
	void s_ReadReg();

private:
	Ui::ReadWriteRegsDialog *ui;
	CGageSystem* m_pGageSystem;
};

#endif // READWRITEREGSDIALOG_H
