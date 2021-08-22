#include "qProgressBar.h"
#include "ui_qprogressbar.h"


progressBar::progressBar(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Form)
{
    ui->setupUi(this);
}

progressBar::~progressBar()
{
    delete ui;
}

void progressBar::paintEvent(QPaintEvent*)
{
}

void progressBar::s_SetMaximum(int Max)
{
    ui->progressBar->setMaximum(Max);
    ui->progressBar->setValue(0);
    repaint();
}

void progressBar::s_SetCurrent(int CurrVal)
{
    ui->progressBar->setValue(CurrVal);
}

void progressBar::SetMaximum(int Max)
{
    ui->progressBar->setMaximum(Max);
    update();
    QCoreApplication::processEvents();
}

void progressBar::SetValue(int Val)
{
    ui->progressBar->setValue(Val);
    update();
    QCoreApplication::processEvents();
}
