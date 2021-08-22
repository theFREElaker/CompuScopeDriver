#include "qperformancetestdialog.h"

QPerformanceTestDialog::QPerformanceTestDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
}

QPerformanceTestDialog::~QPerformanceTestDialog()
{

}

/*******************************************************************************************
										Public slots
*******************************************************************************************/

void QPerformanceTestDialog::s_DoTest()
{
	//TODO: Implements test
}

void QPerformanceTestDialog::s_Close()
{
	ui.txtResult->setText("Cancelled");
	ui.txtResult->repaint();
    CsTestFunc::quSleep(500);
	emit close();
}
