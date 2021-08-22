#ifndef QSYSTEMSELECTIONDIALOG_H
#define QSYSTEMSELECTIONDIALOG_H

#include <QDialog>
#include <QKeyEvent>
#include "ui_qsystemselectiondialog.h"
#include "GageSystem.h"

class QSystemSelectionDialog : public QDialog
{
	Q_OBJECT

public:
    QSystemSelectionDialog(QWidget *parent, QVector<System> hSystemArray, QString strCurrSerNum = "");
	~QSystemSelectionDialog();

	/**************** Getters ***************************/
	int32	getIndexSelectedSystem() const;

protected:
	/********** Overwritted functions *******************/

    //This function is overwritten because we want to call s_Close() with the 'ESC' key
	//and not reject() that will do a default close() 
	void keyPressEvent(QKeyEvent *e);

public slots:
	void s_Close();
	void s_Save();
    void s_ItemDoubleClicked(QListWidgetItem *);

private:
	Ui::QSystemSelectionDialog ui;

	int32				m_i32SelectedSystemIndex;
	int32				m_i32Error;
	int32				m_i32ListedSystemsIndexes[20];
    QString             AdjustFieldLen(QString strIn, int32 i32FieldLen);
    QVector<System>     m_hSystemArray;
    QString             m_strCurrSerNum;
};

#endif // QSYSTEMSELECTIONDIALOG_H
