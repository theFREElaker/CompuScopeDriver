#ifndef WAITPOPUP_H
#define WAITPOPUP_H

#include <QObject>
#include <QDebug>
#include "qtimer.h"
#include "QMessageBox"

class WaitPopUp : public QObject
{
    Q_OBJECT

    public:
    explicit WaitPopUp();
    ~WaitPopUp();

    QTimer*         m_PleaseWaitTimer;
    QTimer*         m_ProcessEventsTimer;
    QMessageBox*    m_MsgBoxPleaseWait;
    bool            m_bProcessEvents;
    int             i32ProcessEventsTimer;

    public slots:
    void        s_ProcessEvents();
    void        s_StartPleaseWaitTimer();
    void        s_ShowPleaseWait();
    void        s_HidePleaseWait();

    private:
};

#endif // WAITPOPUP_H
