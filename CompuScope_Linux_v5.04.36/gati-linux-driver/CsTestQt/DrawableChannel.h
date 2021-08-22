#ifndef DRAWABLECHANNEL_H
#define DRAWABLECHANNEL_H

#include <QPainter>
#include <qpen.h>
#include "CsTypes.h"
#include "cstestqttypes.h"
#include "GageSystem.h"
#include <QTime>
#include "qfile.h"
#include <QMessageBox>

#define LIMIT_ZOOM_POINTS   40

//typedef enum { drawchanreg, drawchansig } DrawChanType;

class DrawableChannel
{
public:
	DrawableChannel();
    DrawableChannel(CGageSystem* pGageSystem, void* pBuffer, unsigned long long u64SampleCount, DRAW_CHAN_TYPE ChanType);
	~DrawableChannel();

	/**************** Setters ***************************/
    void    setChanType(DRAW_CHAN_TYPE ChanType) { m_DrawChanType = ChanType; }
    DRAW_CHAN_TYPE getChanType() { return m_DrawChanType; }
    void	setData(void* pBuffer, unsigned long long u64SampleCount, int XferOffset);
	void	setXferOffset(int XferOffset);
    bool    m_bDataValid;
    bool    m_bInterleaved;
    unsigned long long *m_pu64XferTime;
    unsigned long long *m_pu64XferSamples;
    void    SetAddrXferTime(unsigned long long *pu64XferTime) { m_pu64XferTime = pu64XferTime; }
    void    SetAddrXferSamples(unsigned long long *pu64XferSamples) { m_pu64XferSamples = pu64XferSamples; }
    void    SetAbort() { m_bAbort = true; }
    void    ClearAbort() { m_bAbort = false; }

	void	setCenterY(double dCenterY);
    double	getCenterY() { return m_dCenterY; }
	void	setAmplitude(double dAmplitude);
    double	getAmplitude() { return m_dAmplitude; }
	void	setZoomX(int dZoomIn, int dZoomOut);
	void	setZoomY(unsigned short ZoomY);
    void	setWidth(int Width) { m_i32Width = Width; }
    void    setHeight(int Height) { m_i32Height = Height; }

	void	setSampleSize(int32 i32Res) { m_i32SampleSize = i32Res; }
    int32	getSampleSize() { return m_i32SampleSize ; }
	void	setSampleRes(int32 i32Res) { m_i32SampleRes = i32Res; }
	int32	getSampleRes() { return m_i32SampleRes; }
	void	setSampleOffset(int32 i32Offset) { m_i32SampleOffset = i32Offset; }
	int32	getSampleOffset() { return m_i32SampleOffset; }
	void	setDcOffset(int32 i32DcOffset) { m_i32DcOffset = i32DcOffset; }
	int32	getDcOffset() { return m_i32DcOffset; }
	void	setRange(int32 i32Range) { m_i32Range = i32Range; }
	int32	getRange() { return m_i32Range; }
	void	setAvgCount(int32 AvgCount) { m_i32AvgCount = AvgCount; }
    int32	getAvgCount() { return m_i32AvgCount; }
	void	setChanSkip(int32 ChanSkip) { m_i32ChanSkip = ChanSkip; }
	void	setChanIdx(int32 ChanIdx) { m_i32ChanIdx = ChanIdx; }
    void	setDisplayOneSeg(bool bDisplayOneSeg) { m_bDisplayOneSeg = bDisplayOneSeg; }
    void	setCurrSeg(int64 i64CurrSeg) { m_i64CurrSeg = i64CurrSeg; }
    void	setSegCount(int64 i64SegCount) { m_i64SegCount = i64SegCount; }
    void	setActiveChanCount(int32 i32ChanCount) { m_i32ActiveChanCount = i32ChanCount; }
    void    setFilePath(QString strFilePath);
    void	setSampleCount(unsigned long long u64SampleCount) { m_u64SampleCount = u64SampleCount; }
    unsigned long long	getSampleCount() { return m_u64SampleCount; }

    int		getSampVal(unsigned long long SamplePos);

    int     GenSamplePoints(unsigned long long u64StartSample, unsigned long long u64EndSample);
    int     GenSamplePointsAcq(unsigned long long u64StartSample, unsigned long long u64EndSample);
    int     GenSamplePointsInter(unsigned long long u64StartSample, unsigned long long u64EndSample);
    void    GenSamplePointsSigFile(unsigned long long u64StartSample, unsigned long long u64EndSample);
    void    GenSamplePointsDataFile(unsigned long long u64StartSample, unsigned long long u64EndSample);
    void    SetFlatLine();

    void    draw(QPainter& painter, bool bConnectPoints);

private:
	/********** Attributes for the raw data *************/
	void*	m_pBuffer;
	int		m_XferOffset;
    unsigned long long	m_u64SampleCount;

	/********** Attributes for drawing ******************/
    DRAW_CHAN_TYPE m_DrawChanType;
	int		m_ZoomIn;
	int		m_ZoomOut;
	uInt16	m_u16VerFactor;
	double	m_dCenterY;
	double	m_dAmplitude;
	int		m_i32Width;
    int     m_i32Height;

	QPen	m_penLine;
	QPen	m_penPoint;

	int32	m_i32SampleSize;
	int32	m_i32SampleRes;
	int32	m_i32SampleOffset;
	int32	m_i32DcOffset;
	int32	m_i32Range;
    int32   m_PointCount;
    QPoint	m_Points[2000000];
	int32	m_i32AvgCount;
	int32	m_i32ChanSkip;
	int32	m_i32ChanIdx;
    bool    m_bDisplayOneSeg;
    int32   m_i64CurrSeg;
    int32   m_i64SegCount;
    int32   m_i32ActiveChanCount;
    CGageSystem* m_pGageSystem;
    QElapsedTimer m_XferTimer;
    QString m_strFilePath;
    bool    m_bAbort;
    void    ZeroBuffer(void *pBuffer, int SampleSize, int SampleCount);
	uInt32	RoundUpTransferSize(uInt32 SamplesToDisplay);
};

#endif

