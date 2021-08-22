#ifndef CSTESTERRMNG_H
#define CSTESTERRMNG_H

#include <qstring.h>

class CsTestErrMng
{
public:
    explicit CsTestErrMng();
    ~CsTestErrMng();

	void		ClrErr();
	void		UpdateErrMsg(QString ErrMsg, int ErrCode, bool bAddCRLF = true);
	QString		GetErrMsg();
	int			GetLastErrCode();
	bool		bQuit;

//public slots:

private:
    QString				m_strErrMsg;
	int					m_i32LastErrCode;
};

#endif // CSTESTERRMNG_H
