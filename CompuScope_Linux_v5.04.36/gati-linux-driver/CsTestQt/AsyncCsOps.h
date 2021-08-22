#ifndef ASYNCCSOPS_H
#define ASYNCCSOPS_H

#include <QObject>
#include <QMutex>
#include <vector> //Could not use QVector

#include "GageSystem.h"
#include "CsTestFunc.h"
#include <qeventloop.h>
#include <qelapsedtimer.h>

class AsyncCsOps : public QObject
{
	Q_OBJECT

public:
    AsyncCsOps(CGageSystem* pGageSystem, bool* bAcqRunning, CsTestErrMng* ErrMng);
	~AsyncCsOps();

	/********* Mutex for multi threading ****************/
    QMutex*		m_pMutexAbort;
    bool*       m_bAcqRunning;

public slots:
	void	s_AbortCsOp();
	void	s_ForceTriggerCsOp();
	void	s_ResetCsOp();

signals:
	void	AbortDone();

private:
	CGageSystem*		m_pGageSystem;
	CsTestErrMng*		m_pErrMng;

};

#endif // ASYNCCSOPS_H
