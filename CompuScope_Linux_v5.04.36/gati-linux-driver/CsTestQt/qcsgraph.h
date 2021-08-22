#ifndef QCSGRAPH_H
#define QCSGRAPH_H

#include <QWheelEvent>
#include <QWidget>
#include <QPainter>
#include "qimage.h"
#include <qeventloop.h>
#include "qfile.h"
#include "DrawableChannel.h"
#include "CsTypes.h"
#include <vector>
#include "QMutex"
#include "cstestqttypes.h"
#include "GageSystem.h"

using namespace std;

#define NBR_MAX_CHANNELS 16
#define NBR_HORIZONTAL_LINES 9
#define DISTANCE_BETWEEN_VERTICAL_LINES 128
#define MAX_FILE_BYTES_READ 1000000
#define MIN_SAMPS_PER_SCREEN 32
#define MAX_SAMPS_PER_SCREEN 4194304

class QCsGraph : public QWidget
{
	Q_OBJECT

public:
    QCsGraph(QWidget *parent = 0);
	~QCsGraph();

	QSize sizeHint() const {
    return QSize(800, 200);
  }

    bool		m_bCursor;
	bool		m_bAcquiring;
	bool		m_bConnectPoints;
	bool		m_bOverdraw;
	bool		m_bGageDataValid;
	bool		m_bInterleavedData;
	bool		m_bDataFromFile;
    bool*       m_bScopeInitDone;
    bool*       m_bScopeUpdateDone;
    bool*		m_bAcqRunning;
	QFile		m_DataFile;
    char*		m_pDataFileBuffer;
	QByteArray	m_DataFileByteArray;
    unsigned long long		m_u64DataFileSampleCount;
	QImage*		m_pGraphImage_Backgnd;
	QImage*		m_pGraphImage_Trace;
	double		m_dblSampPerScroll;
	bool		m_bZoomXChanged;
    GRAPH_UPDATE    m_GraphUpdateType;
    QMutex*         m_MutexSharedVars;
    unsigned long long m_u64XferTime;
    unsigned long long *GetAddrXferTime() { return &m_u64XferTime; }
    unsigned long long m_u64XferSamples;
    unsigned long long *GetAddrXferSamples() { return &m_u64XferSamples; }

	/**************** Getters ***************************/

	// Ratio between the actual zoom and the minimal value
	double			getZoomXRatio();
	unsigned int	getZoomOut();
	unsigned int	getZoomIn();
	unsigned short	getZoomY();
    unsigned long long getViewPosition();
    void            setViewPosition(unsigned long long i64ViewPos) { m_i64NewViewPosition = i64ViewPos; }
	int				getHeight();
	int				getWidth();
    unsigned long long getSamplesPerScreen();
    unsigned long long CalcSamplesPerScreen(int ZoomIn, int ZoomOut);
    void setSampleCount(unsigned long long u64SampleCount) { m_u64SampleCount = u64SampleCount; }
    unsigned long long getSampleCount();
    uInt32 getNbrChannels();
    void GenCursorStatus();
    void AdjustZoom();

	/*************** Setters ****************************/
    void setData(vector<DrawableChannel*> pChannelsArray, unsigned long long u64SampleCount);
    void addSigChannel(CSSIGSTRUCT *pTraceInfo, QString strSigFilePath);
    void setBoardActiveChans(int ActiveChans) { m_i32BoardActiveChans = ActiveChans; }
    int  getBoardActiveChans() { return m_i32BoardActiveChans; }
    void setChannels();
	void setSampleSize(uInt32 u32SampleSize);
    void setPreTrig(unsigned long long u64PreTrig);
	void editZoomX(double dDelta);
	void editZoomY(double dDelta);
	void ChangePositionX(int i32value);
    void GoToSample(long long SampAddr, bool bCenterScreen);
    void SetAddrScopeInitDone(bool* bScopeInitDone);
    void SetAddrScopeUpdateDone(bool* bScopeUpdateDone);
    void SetAddrAcqRunning(bool* bAcqRunning);
    void SetAddrStatusBarUpdateDone(bool* bStatusUpdateDone);
    void SetAddrMutexSharedVars(QMutex* MutexSharedVars);
    void SetChanAddrXferTime();
    void setCursor(int Xpos);
    void unsetCursor();
	void updateCursorVals();
	void enableCursor();
	void setCursorSampPos(unsigned int CursorSampPos);
	void setCursorScreenPos(unsigned int CursorScreenPos);
    unsigned int getCursorScreenPos() { return m_CursorScreenPos; }
	void setAcquiring(bool bAcqState);
    void setTrigPos(unsigned long long TrigPos);
    unsigned long long getTrigPos() { return m_u64TrigPosition; }
    void setDispChanCount(int DispChanCount) { m_i32DispChanCount = DispChanCount; }
    int getDispChanCount() { return m_i32DispChanCount; }
    void CalculateChannelsCenter();
    void ClearAbort();
    unsigned long long CalculateMaxViewPos();
    void AdjustViewPos();

	DrawableChannel* GetChannel(int ChanIdx) { return m_pChannelsArray[ChanIdx]; }

	/********** Overwritted functions *******************/
	void paintEvent(QPaintEvent*);
	void resizeEvent(QResizeEvent*);
    void initGraph();
	void switchGraphData();
    int UpdateSamplePoints();

	QEventLoop*	m_UpdateWaitLoop;

public slots:
	void s_ClearGraph();
    void s_UpdateGraph(GRAPH_UPDATE UpdateAction);
    void s_DisableData();
    void s_AbortUpdate();

signals:
    void updateGraph(GRAPH_UPDATE UpdateAction);
	void ScopeUpdateDone();
	void GraphCleared();
    void updateTxTime(QString strXferTime);
    void updateScrollBar(int Val);
    void channelProtFault();
	
private:
	/********** Painting functions **********************/
	void drawBackground();
	void drawTrigAxis();
    void drawPlot();
	void drawCursor();

signals:
	void zoomXChanged();
	void horizChanged(int);
	void updateCursorStatus(QString);
	void updatePositionStatus(QString);

public slots:
	void s_movePositionX(int i32value);
	void s_updateCursorVals();

private:
	/********** Attributes for drawing ******************/
	QPen	m_penCenterLine;
	QPen	m_penGridLine;
	QPen	m_penCursor;
	QPen	m_penTrigAxis;

	/********** Attributes for the display **************/
	double	m_dDistanceBetweenHorizontalLines;
	double  m_dAxisYPosition[NBR_MAX_CHANNELS];

	unsigned int		m_ZoomIn;
	unsigned int		m_NewZoomIn;
	unsigned int		m_ZoomOut;
	unsigned int		m_NewZoomOut;
	unsigned short		m_u16ZoomY;
	unsigned short		m_u16NewZoomY;

	unsigned int		m_LastDigiZoomIn;
	unsigned int		m_LastFileZoomIn;
	unsigned int		m_LastDigiZoomOut;
	unsigned int		m_LastFileZoomOut;
	unsigned short		m_u16LastDigiZoomY;
	unsigned short		m_u16LastFileZoomY;
	int64				m_i64LastDigiViewPosition;
	int64				m_i64LastFileViewPosition;

    long long           m_CursorSampPos;
	unsigned int		m_CursorScreenPos;
	unsigned int		m_NewCursorScreenPos;
    int					m_i32CursorVals[64];

	int		m_i32Height;
	int		m_i32Width;
	int		m_i32CenterX;
	int		m_i32CenterY;

	/********** Attributes for the raw data *************/
	vector<DrawableChannel*> m_pChannelsArray;
	vector<int> m_XferOffsets;
    unsigned long long	m_u64SampleCount;
	int64	m_i64ViewPosition;
	int64	m_i64NewViewPosition;
    unsigned long long	m_u64TrigPosition;
	uInt32	m_u32SampleSize;
    unsigned long long	m_u64PreTrig;
	bool	m_bUpdated;
	bool	m_bUpdating;
	bool	m_bPainting;
	bool	m_bInitGraph;
	bool	m_bCleared;
    bool    m_bUpdatePlot;
    bool    m_bClearTrace;
    int     m_i32BoardActiveChans;
    int     m_i32DispChanCount;
    bool    m_bAbort;
};

#endif // QCSGRAPH_H
