#ifndef READSIGFILEDIALOG_H
#define READSIGFILEDIALOG_H

#include <QDialog>
#include "ui_qReadSigFiledialog.h"
#include "qfiledialog.h"
#include "GageSystem.h"

namespace Ui {
class ReadSigFileDialog;
}

class ReadSigFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReadSigFileDialog(QWidget *parent = 0);
    ~ReadSigFileDialog();

	QString m_DataFilePath;
    bool m_bOk;
    bool m_bCancelled;
    void showEvent(QShowEvent * event);

public slots:
	void s_Show();
	void s_SelFile();
	void s_Close();

private:
    Ui::ReadSigFileDialog *ui;
};

#endif // READSIGFILEDIALOG_H
