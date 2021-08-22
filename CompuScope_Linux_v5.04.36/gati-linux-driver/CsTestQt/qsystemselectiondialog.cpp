#include "qsystemselectiondialog.h"
#include "QMessageBox"

const int c_Gigabyte = 1024 * 1024 * 1024;
const int c_Megabyte = 1024 * 1024;
const int c_Kilobyte = 1024;

QSystemSelectionDialog::QSystemSelectionDialog(QWidget *parent, QVector<System> hSystemArray, QString strCurrSerNum)
	: QDialog(parent), m_i32SelectedSystemIndex(-1), m_i32Error(1)
{
	ui.setupUi(this);
	setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

    m_hSystemArray = hSystemArray;
    m_strCurrSerNum = strCurrSerNum;

    ui.listCsSystems->clear();

    bool bSelected = false;
    int idxSystem = 0;
    int idxSelSystem = 0;

	for(int i = 0; i < hSystemArray.size(); ++i)
	{
		QString name = hSystemArray[i].strBoardName;
        int64 i64MemorySize = hSystemArray[i].i64MemorySize;
		QString strIndex = QString::number(hSystemArray[i].index);
        QString CurrString, description;

        if (!name.isEmpty() && !name.isNull() && hSystemArray[i].bFree)
		{
			// Add the system to the list
            CurrString = name;
            description = AdjustFieldLen(name, 10);

            description += AdjustFieldLen(hSystemArray[i].strSerialNumber, 20);

            CurrString = QString::number(hSystemArray[i].i32BrdCount);
            if(hSystemArray[i].i32BrdCount>1) CurrString += "Boards";
            else CurrString += "Board";
            description += AdjustFieldLen(CurrString, 10);

            CurrString = QString::number(hSystemArray[i].i32ChanCount);
            if(hSystemArray[i].i32ChanCount>1) CurrString += "Channels";
            else CurrString += "Channel";
            description += AdjustFieldLen(CurrString, 12);

            CurrString = QString::number(hSystemArray[i].i32BitCount);
            CurrString += "Bits";
            description += AdjustFieldLen(CurrString, 10);

            if (i64MemorySize >= c_Gigabyte)
			{
                CurrString = QString::number(i64MemorySize / c_Gigabyte);
                CurrString += "GS";
			}
            else if (i64MemorySize >= c_Megabyte)
			{
                CurrString = QString::number(i64MemorySize / c_Megabyte);
                CurrString += "MS";
			}
            else if (i64MemorySize >= c_Kilobyte)
			{
                CurrString = QString::number(i64MemorySize / c_Kilobyte);
                CurrString += "KS";
			}
            else
            {
                CurrString = QString::number(i64MemorySize);
                CurrString += "S";
            }

            description += AdjustFieldLen(CurrString, 10);

			ui.listCsSystems->addItem(description);
            ui.listCsSystems->item(i)->setWhatsThis(strIndex);

			m_i32ListedSystemsIndexes[i] = hSystemArray[i].index;

            if(!bSelected)
            {
                if(hSystemArray[i].strSerialNumber!=m_strCurrSerNum)
                {
                    bSelected = true;
                    idxSelSystem = idxSystem;
                }
            }

            idxSystem++;
		}
	}

    ui.listCsSystems->setCurrentRow(idxSelSystem);

    connect(ui.listCsSystems, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(s_ItemDoubleClicked(QListWidgetItem *)));


}

QSystemSelectionDialog::~QSystemSelectionDialog()
{

}

/*******************************************************************************************
										Getters and Setters
*******************************************************************************************/

int32 QSystemSelectionDialog::getIndexSelectedSystem() const
{
	return m_i32SelectedSystemIndex;
}

/*******************************************************************************************
                                        Overwritten events
*******************************************************************************************/

void QSystemSelectionDialog::keyPressEvent(QKeyEvent *e) 
{
    if(e->key() != Qt::Key_Escape)
	{
        QDialog::keyPressEvent(e);
	}
    else 
	{
		emit s_Close();
	}
}


/*******************************************************************************************
										Public slots
*******************************************************************************************/

void QSystemSelectionDialog::s_Close()
{
	m_i32SelectedSystemIndex = -1;
	emit close();
}

void QSystemSelectionDialog::s_Save()
{
	// Save the chosen system
    if(m_hSystemArray[ui.listCsSystems->currentRow()].i64MemorySize==0)
    {
        QMessageBox MsgBox;
        MsgBox.setText("The select system shows a memory size of 0.\r\nPlease select another one.");
        MsgBox.exec();
        return;
    }
	m_i32SelectedSystemIndex = m_i32ListedSystemsIndexes[ui.listCsSystems->currentRow()];
	close();
}

void QSystemSelectionDialog::s_ItemDoubleClicked(QListWidgetItem *)
{
    m_i32SelectedSystemIndex = m_i32ListedSystemsIndexes[ui.listCsSystems->currentRow()];
    close();
}

QString QSystemSelectionDialog::AdjustFieldLen(QString strIn, int32 i32FieldLen)
{
    QString strOut = strIn;
    int32 PadLen = i32FieldLen - strIn.length();



    for(int32 i=0;i<PadLen;i++)
    {
        strOut += " ";
    }

    return strOut;
}
