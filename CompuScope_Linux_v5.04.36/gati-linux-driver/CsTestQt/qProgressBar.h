#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include <QDialog>
#include "ui_qprogressbar.h"


namespace Ui {
class progressBar;
}

class progressBar : public QDialog
{
    Q_OBJECT

public:
    explicit progressBar(QWidget *parent = 0);
    ~progressBar();

    void paintEvent(QPaintEvent*);
    void SetMaximum(int Max);
    void SetValue(int Val);

public slots:
    void s_SetMaximum(int Max);
    void s_SetCurrent(int CurrVal);

signals:

private:
    Ui::Form *ui;
};

#endif // PROGRESSBAR_H
