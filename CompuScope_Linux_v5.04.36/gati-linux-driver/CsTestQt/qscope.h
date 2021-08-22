#ifndef QSCOPE_H
#define QSCOPE_H

#include <QWidget>
#include "DrawableChannel.h"
#include "ui_qscope.h"
#include "GageSystem.h" // RG see if I can get rid of this
#include <vector>
#include <QMessageBox>
#include "RmFunctions.h"
#include "CsPrivateStruct.h"
#include "cstestqttypes.h"


#define SAMP_PER_SCROLL_RATIO 128.0

using namespace std;

class QScope : public QWidget
{
	Q_OBJECT

public:
	QScope(QWidget *parent = 0);
	~QScope();

	/********** Overwritted functions *******************/
	void wheelEvent(QWheelEvent* wheelEvent);
	void keyPressEvent(QKeyEvent* event);

	void setGageSystem(CGageSystem* pGageSystem) { m_pGageSystem = pGageSystem; }
	void setChannels();
    void setFileChannels(int ChanCount, int SampSize, int Range, QString strDataFilePath);
	void setSampleSize(uInt32 u32SampleSize);
	void setAvgCount(int32 AvgCount);
    void setBrdIdx(int32 BrdIdx) { m_i32BrdIdx = BrdIdx; }
	void UpdateChannelList(int i32NbrChannels);
	void setAbort(bool bSet) { m_bMustAbort = bSet; }
	QCsGraph* GetGraph() { return ui.graph; }
    //void adjustHScrollBar();
    void SetAddrDisplayOneSeg(bool* bDisplayOneSeg);
    void SetAddrCurrSeg(int64* i64CurrSeg);
    bool* m_bDisplayOneSeg;
    int64* m_i64CurrSeg;
    void AdjustHScrollBar();

public slots:
	void s_adjustHScrollBar();
    void s_UpdateRawData();
	void s_UpdateChannelList(int);
    void s_DisplaySystemInfo();
    void s_DisplayFwVersions();
    void s_UpdateScrollBar(int Val);

signals:
    void updateGraph(GRAPH_UPDATE UpdateAction);
    void displaySysInfo();
    void displayFwVersions();

private:
    Ui::QScope ui;
	vector<DrawableChannel*> m_channelArray;

	//DEBUG
	void* m_pBuffer;

	CGageSystem* m_pGageSystem;
	bool m_bMustAbort;
    uInt32 m_i32AvgCount;
    int     m_i32BrdIdx;
    bool    m_bWheelEvent;
};

#endif // QSCOPE_H
