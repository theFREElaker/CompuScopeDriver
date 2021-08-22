#include "qGoToSample.h"
#include "ui_qGotoSample.h"


gotoSample::gotoSample(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::gotoSample)
{
    ui->setupUi(this);

    m_bAddressValid = false;
}

gotoSample::~gotoSample()
{
    delete ui;
}

void gotoSample::paintEvent(QPaintEvent*)
{
}

void gotoSample::showEvent(QShowEvent*)
{
    m_bAddressValid = false;
}

bool gotoSample::IsAddressValid()
{
    return m_bAddressValid;
}

long long gotoSample::GetAddress()
{
    return ui->doubleSpinBox->value();
}

void gotoSample::s_Ok()
{
    m_bAddressValid = true;
    close();
}


