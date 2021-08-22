#ifndef CURSORINFODIALOG_H
#define CURSORINFODIALOG_H

#include <QDialog>
#include "ui_qCursorInfodialog.h"
#include "GageSystem.h"
#include "QKeyEvent"

namespace Ui {
class CursorInfodialog;
}

class CursorInfodialog : public QDialog
{
    Q_OBJECT

public:
    explicit CursorInfodialog(QWidget *parent = 0);
    ~CursorInfodialog();

    void    moveEvent(QMoveEvent *eventMove);
    void	closeEvent(QCloseEvent *eventClose);
    void    keyPressEvent(QKeyEvent* event);

public slots:
    void s_Show();
    void s_Hide();
    void s_Update(QString strCursorInfo);

signals:
    void disableCursor(bool);

private:
    Ui::CursorInfodialog *ui;
    bool    m_bHide;
    QWidget *m_pParent;
};

#endif // CURSORINFODIALOG_H
