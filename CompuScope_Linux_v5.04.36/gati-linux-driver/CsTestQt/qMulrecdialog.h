#ifndef MULRECDIALOG_H
#define MULRECDIALOG_H

#include <QDialog>
#include "ui_qMulrecdialog.h"
#include "GageSystem.h"
#include "acquisitionworker.h"

namespace Ui {
class MulrecDialog;
}

class MulrecDialog : public QDialog
{
    Q_OBJECT

public:
	explicit MulrecDialog(QWidget *parent = 0, CGageSystem* pGageSystem = NULL, AcquisitionWorker* pAcqWorker = NULL);
	~MulrecDialog();

public slots:
	void s_Ok();
	void s_Cancel();
	void s_Show();
	void s_ModeChanged(bool bAllSegs);

private:
	Ui::MulrecDialog *ui;
	CGageSystem* m_pGageSystem;
	AcquisitionWorker* m_pAcqWorker;
	bool m_bInit;
	bool m_bAllSegsLastSt;
	bool m_bOnlySegsLastSt;
	bool m_bOneSegLastSt;
	int32 m_i32FromSegLastSt;
	int32 m_i32ToSegLastSt;
};

#endif // MULRECDIALOG_H
