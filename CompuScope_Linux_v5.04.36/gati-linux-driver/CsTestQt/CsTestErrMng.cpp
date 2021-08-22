#include "CsTestErrMng.h"

CsTestErrMng::CsTestErrMng()
{
	bQuit = false;
}

CsTestErrMng::~CsTestErrMng()
{
}

void CsTestErrMng::ClrErr()
{
	bQuit = false;
	m_strErrMsg.clear();
	m_i32LastErrCode = 0;
}

void CsTestErrMng::UpdateErrMsg(QString ErrMsg, int ErrCode, bool bAddCRLF)
{
	m_i32LastErrCode = ErrCode;
    if(bAddCRLF)
    {
        m_strErrMsg.insert(0, "\r\n");
    }
	m_strErrMsg.insert(0, ErrMsg);
}

QString CsTestErrMng::GetErrMsg()
{
	return m_strErrMsg;
}

int CsTestErrMng::GetLastErrCode()
{
	return m_i32LastErrCode;
}
