#ifndef QABOUTDIALOG_H
#define QABOUTDIALOG_H

#include <QtGlobal>
#include <QDialog>
#include "ui_qaboutdialog.h"

#include "CsTypes.h"

#ifdef Q_OS_WIN
    #define NOMINMAX	// to prevent compilation errors from QDateTime because of including windows.h
    #include "windows.h"
#endif

class QAboutDialog : public QDialog
{
	Q_OBJECT

public:
	QAboutDialog(QWidget *parent = 0);
	~QAboutDialog();

private:
#ifdef Q_OS_WIN
	QString getDllPath(const QString& strDllName);
	QString getDllVersion(const QString& strDllFileName);
#endif

private:
	Ui::QAboutDialog ui;
};

#endif // QABOUTDIALOG_H
