#include "qCursorInfodialog.h"
#include "ui_qCursorInfodialog.h"
#include "qdebug.h"


CursorInfodialog::CursorInfodialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CursorInfodialog)
{
    ui->setupUi(this);

    m_pParent = parent;
    m_bHide = false;

    move(parent->x() + parent->width() - width() - 20, parent->y() + parent->height() - height() - 80);
}

CursorInfodialog::~CursorInfodialog()
{
    delete ui;
}

void CursorInfodialog::moveEvent(QMoveEvent *eventMove)
{
    UNREFERENCED_PARAMETER(eventMove);
}

void CursorInfodialog::closeEvent(QCloseEvent *eventClose)
{
    UNREFERENCED_PARAMETER(eventClose);
    emit disableCursor(false);
}

void CursorInfodialog::keyPressEvent(QKeyEvent* event)
{
    switch(event->key())
    {
        case Qt::Key_Escape:
            qDebug() << "Escape key";
            emit disableCursor(false);
            break;

        default:
            break;
    }
}

void CursorInfodialog::s_Show()
{
    //qDebug() << "CursorInfodialog: Show";
	show();
    m_bHide = false;
}

void CursorInfodialog::s_Hide()
{
    //qDebug() << "CursorInfodialog: Hide";
    hide();
    m_bHide = true;
}

void CursorInfodialog::s_Update(QString strCursorInfo)
{
    //qDebug() << "CursorInfodialog: s_Update: " << strCursorInfo;
    ui->textEdit_Status->clear();
    ui->textEdit_Status->setText(strCursorInfo);
    if(!m_bHide) show();
}

