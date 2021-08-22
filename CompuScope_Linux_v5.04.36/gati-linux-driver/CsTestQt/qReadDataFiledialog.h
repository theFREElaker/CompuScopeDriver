#ifndef READDATAFILEDIALOG_H
#define READDATAFILEDIALOG_H

#include <QDialog>
#include "ui_qReadDataFiledialog.h"
#include "qfiledialog.h"
#include "GageSystem.h"

namespace Ui {
class ReadDataFileDialog;
}

class ReadDataFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReadDataFileDialog(QWidget *parent = 0);
	~ReadDataFileDialog();

	int m_i32ChanCount;
	int m_i32SampleSize;
	QString m_DataFilePath;
    bool m_bOk;
    bool m_bCancelled;
    void showEvent(QShowEvent * event);

public slots:
	void s_Show();
	void s_SelFile();
	void s_Close();

private:
	Ui::ReadDataFileDialog *ui;
};

#endif // READDATAFILEDIALOG_H
