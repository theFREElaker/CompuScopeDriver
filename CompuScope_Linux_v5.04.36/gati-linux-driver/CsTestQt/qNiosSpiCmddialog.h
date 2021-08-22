#ifndef NIOSSPICMDDIALOG_H
#define NIOSSPICMDDIALOG_H

#include <QDialog>
#include "ui_qNiosSpiCmddialog.h"
#include "GageSystem.h"

namespace Ui {
class NiosSpiCmdDialog;
}

class NiosSpiCmdDialog : public QDialog
{
    Q_OBJECT

public:
	explicit NiosSpiCmdDialog(QWidget *parent = 0, CGageSystem* pGageSystem = NULL);
	~NiosSpiCmdDialog();

public slots:
	void s_Show();
	void s_SendCmd();

private:
	Ui::NiosSpiCmdDialog *ui;
	CGageSystem* m_pGageSystem;
};

#endif // NIOSSPICMDDIALOG_H
