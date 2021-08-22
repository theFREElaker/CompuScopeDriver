#include "AsyncCsOps.h"
#include <QtDebug>
#include <QMessageBox>
#include "qapplication.h"
#include <QTime>


AsyncCsOps::AsyncCsOps(CGageSystem* pGageSystem, bool* bAcqRunning, CsTestErrMng* ErrMng)
{
	m_pGageSystem = pGageSystem;
	m_pErrMng = ErrMng;
    m_bAcqRunning = bAcqRunning;

	m_pMutexAbort = new QMutex(QMutex::NonRecursive);
}

AsyncCsOps::~AsyncCsOps()
{
	delete m_pMutexAbort;
}

void AsyncCsOps::s_AbortCsOp()
{
    //qDebug() << "***************Asynch: s_AbortAcq****************";
    //m_pGageSystem->ForceTrigger();
    m_pGageSystem->Abort();
}

void AsyncCsOps::s_ForceTriggerCsOp()
{
	m_pGageSystem->ForceTrigger();
}

void AsyncCsOps::s_ResetCsOp()
{
	m_pGageSystem->ResetSystem();
}
