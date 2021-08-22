#ifndef GOTOSAMPLE_H
#define GOTOSAMPLE_H

#include <QDialog>
#include "ui_qGotoSample.h"


namespace Ui {
class gotoSample;
}

class gotoSample : public QDialog
{
    Q_OBJECT

public:
    explicit gotoSample(QWidget *parent = 0);
    ~gotoSample();

    void paintEvent(QPaintEvent*);
    void showEvent(QShowEvent*);
    bool IsAddressValid();
    long long GetAddress();

public slots:
    void s_Ok();

signals:

private:
    bool m_bAddressValid;
    Ui::gotoSample *ui;
};

#endif // GOTOSAMPLE_H
