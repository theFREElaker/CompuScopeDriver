#ifndef DEBUGDIALOG_H
#define DEBUGDIALOG_H

#include <QDialog>
#include "ui_qdebugdialog.h"
#include "GageSystem.h"

namespace Ui {
class DebugDialog;
}

class DebugDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DebugDialog(QWidget *parent = 0, CGageSystem* pGageSystem = NULL);
    ~DebugDialog();

public slots:
	void s_Show();
	void s_ChangeMode(bool bState);
	void s_WriteReg();
	void s_ReadReg();

private:
    Ui::DebugDialog *ui;
	CGageSystem* m_pGageSystem;
};

#endif // DEBUGDIALOG_H
