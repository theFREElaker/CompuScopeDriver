#ifndef QPERFORMANCETESTDIALOG_H
#define QPERFORMANCETESTFIALOG_H

#include <QDialog>
#include "CsTestFunc.h"
#include "ui_qperformancetestdialog.h"

class QPerformanceTestDialog : public QDialog
{
	Q_OBJECT

public:
	QPerformanceTestDialog(QWidget *parent = 0);
	~QPerformanceTestDialog();

public slots:
	void s_DoTest();
	void s_Close();

private:
	Ui::QPerformanceTestDialog ui;
};

#endif // QPERFORMANCETEST_H
