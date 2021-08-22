#include "DrawableChannel.h"
#include <QElapsedTimer>
#include "qapplication.h"
#include <QDebug>
#include "math.h"

DrawableChannel::DrawableChannel(CGageSystem* pGageSystem, void* pBuffer, unsigned long long u64SampleCount, DRAW_CHAN_TYPE ChanType) :
	m_pBuffer(pBuffer), m_u64SampleCount(u64SampleCount), m_ZoomIn(1), m_ZoomOut(1), m_u16VerFactor(1), m_dCenterY(0), m_dAmplitude(0), m_i32DcOffset(0)
{
    m_pGageSystem = pGageSystem;
    m_DrawChanType = ChanType;
	m_penLine.setWidth(1);
	m_penLine.setColor(Qt::black);
    m_penPoint.setWidth(1);
	m_penPoint.setColor(Qt::black);
	m_XferOffset = 0;
    m_bDataValid = false;
    m_bAbort = false;
}

DrawableChannel::~DrawableChannel()
{
}

/*******************************************************************************************
										Getters and Setters
*******************************************************************************************/
void DrawableChannel::setData(void* pBuffer, unsigned long long u64SampleCount, int XferOffset)
{
	m_pBuffer = pBuffer; 
	m_u64SampleCount = u64SampleCount;
	m_XferOffset = XferOffset;
}

void DrawableChannel::setXferOffset(int XferOffset)
{
	m_XferOffset = XferOffset;
}

void DrawableChannel::setCenterY(double dCenterY)
{
	m_dCenterY = dCenterY;
}

void DrawableChannel::setAmplitude(double dAmplitude)
{
	m_dAmplitude = dAmplitude;
}

void DrawableChannel::setZoomX(int ZoomIn, int ZoomOut)
{
	m_ZoomIn = ZoomIn;
	m_ZoomOut = ZoomOut;
}

void DrawableChannel::setZoomY(unsigned short zoomY)
{
	m_u16VerFactor = zoomY;
}

void DrawableChannel::setFilePath(QString strFilePath)
{
    m_strFilePath.clear();
    m_strFilePath.append(strFilePath);
}

int DrawableChannel::getSampVal(unsigned long long SamplePos)
{
    qDebug() << "getSampVal";
    qDebug() << "getSampVal: SamplePos: "  << SamplePos;
	int SampVal = 0;
    int i32Ret, XferOffset;
    bool bRes;
	uchar *pu8Buffer = (uchar *)m_pBuffer;
	short *ps16Buffer = (short *)m_pBuffer;
    int *pi32Buffer = (int *)m_pBuffer;
    QFile SigFile;
    QByteArray SigByteArr;
    QFile DataFile;
    QByteArray DataByteArr;
    long long i64XferTime;

    if(m_bInterleaved) SamplePos = (SamplePos * m_i32ActiveChanCount) + m_i32ChanIdx;

    if(SamplePos>m_u64SampleCount) return 0;

    switch(m_DrawChanType)
    {
        case drawchanreg:
            qDebug() << "getSampVal: drawchanreg";
            if(m_bDataValid)
            {
                switch(m_i32SampleSize)
                {
                    case 1:
                        i32Ret = m_pGageSystem->TransferDataBuffer((m_i32ChanIdx*m_i32ChanSkip) + 1, 1, pu8Buffer, SamplePos, MIN_XFER_SIZE, &XferOffset, &i64XferTime);
                        if(i32Ret<0)
                        {
                        }
                        SampVal = pu8Buffer[XferOffset];
                        break;

                    case 2:
                        i32Ret = m_pGageSystem->TransferDataBuffer((m_i32ChanIdx*m_i32ChanSkip) + 1, 1, ps16Buffer, SamplePos, MIN_XFER_SIZE, &XferOffset, &i64XferTime);
                        if(i32Ret<0)
                        {
                        }
                        SampVal = ps16Buffer[XferOffset];
                        break;

                    case 4:
                        i32Ret = m_pGageSystem->TransferDataBuffer((m_i32ChanIdx*m_i32ChanSkip) + 1, 1, pi32Buffer, SamplePos, MIN_XFER_SIZE, &XferOffset, &i64XferTime);
                        if(i32Ret<0)
                        {
                        }
                        SampVal = pi32Buffer[XferOffset];
                        break;
                }
            }
            else SampVal = 0;
            break;

        case drawchanfilesig:
            qDebug() << "getSampVal: drawchanfilesig";
            SigFile.setFileName(m_strFilePath);

            bRes = SigFile.open(QIODevice::ReadOnly);
            if (!bRes)
            {
                if(m_i32SampleSize==1) return 127;
                else return 0;
            }

            SigFile.seek(SIG_FILE_HEADER_SIZE + (SamplePos * m_i32SampleSize));
            SigByteArr = SigFile.read(m_i32SampleSize);
            SigFile.close();

            switch(m_i32SampleSize)
            {
                case 1:
                    pu8Buffer = (unsigned char*)SigByteArr.data();
                    SampVal = pu8Buffer[0];
                    break;

                case 2:
                    ps16Buffer = (short*)SigByteArr.data();
                    SampVal = ps16Buffer[0];
                    break;
            }
            break;

        case drawchanfiledata:
            qDebug() << "getSampVal: drawchanfiledata";
            DataFile.setFileName(m_strFilePath);

            bRes = DataFile.open(QIODevice::ReadOnly);
            if (!bRes)
            {
                if(m_i32SampleSize==1) return 127;
                else return 0;
            }

            DataFile.seek(SamplePos * m_i32SampleSize);
            DataByteArr = DataFile.read(m_i32SampleSize);
            DataFile.close();

            switch(m_i32SampleSize)
            {
                case 1:
                    pu8Buffer = (unsigned char*)DataByteArr.data();
                    SampVal = pu8Buffer[0];
                    break;

                case 2:
                    ps16Buffer = (short*)DataByteArr.data();
                    SampVal = ps16Buffer[0];
                    break;
            }
            break;
    }

    qDebug() << "getSampVal: Done";

	return SampVal;
}

/*******************************************************************************************
										Drawing functions
*******************************************************************************************/
void DrawableChannel::draw(QPainter& painter, bool bConnectPoints)
{
    //qDebug() << "DrawableChannel: Draw";

    //qDebug() << "DrawableChannel: Draw: ZoomIn: " << m_ZoomIn;
    //qDebug() << "DrawableChannel: Draw: ZoomOut: " << m_ZoomOut;
    if (bConnectPoints)
    {
        painter.setPen(m_penLine);
        painter.drawPolyline(&m_Points[0], m_PointCount);
    }
    else
    {
        if(m_ZoomOut>1)
        {
            painter.setPen(m_penPoint);
            painter.drawLines(&m_Points[0], m_PointCount/2);
        }
        else
        {
            painter.setPen(m_penPoint);
            painter.drawPoints(&m_Points[0], m_PointCount);
        }
    }
}

int DrawableChannel::GenSamplePoints(unsigned long long u64StartSample, unsigned long long u64EndSample)
{
    int i32Ret = 0;

    switch(m_DrawChanType)
    {
        case drawchanfilesig:
            GenSamplePointsSigFile(u64StartSample, u64EndSample);
            break;

        case drawchanfiledata:
            GenSamplePointsDataFile(u64StartSample, u64EndSample);
            break;

        case drawchanreg:
            if(m_bInterleaved)
            {
                i32Ret = GenSamplePointsInter(u64StartSample, u64EndSample);
            }
            else
            {
                i32Ret = GenSamplePointsAcq(u64StartSample, u64EndSample);
            }
            break;
    }

    return i32Ret;
}

int DrawableChannel::GenSamplePointsAcq(unsigned long long u64StartSample, unsigned long long u64EndSample)
{
    unsigned char *pBuffer8 = (unsigned char *)m_pBuffer;
    short *pBuffer16 = (short *)m_pBuffer;
    int *pBuffer32 = (int *)m_pBuffer;
    long long i64XferTime;

    float fltVertMaxAmpl = m_dAmplitude * m_u16VerFactor;
    float fltVerFactor = m_u16VerFactor;
    float fltVertAdjAmpl = fltVertMaxAmpl * fltVerFactor;
    float fltSampleOffset = m_i32SampleOffset;
    float fltSampleRes = m_i32SampleRes;
    float fltDcVertOffset = floor(((fltVerFactor * fltVertMaxAmpl) * (m_i32DcOffset / (m_i32Range / 2.0))) + 0.5);
    float fltCenterVertOffset = m_dCenterY;
    float fltPointY, fltPointYmin, fltPointYmax;
    float fltAvgCount = 0;

    unsigned long long SampleCount;

    int PointY, PointYmin, PointYmax;
    int CurrVal = 0, SegMin = 0, SegMax = 0;
    int PointCount = 0;

    int idxPoint = 0;
    unsigned short PointX = 0;

    int i32Ret;
    int XferSize, XferOffset;

    unsigned char SampSize = m_i32SampleSize;

    switch(SampSize)
    {
        case 1:
        case 2:
            fltAvgCount = 1;
            break;

        case 4:
            fltAvgCount = m_i32AvgCount;
            break;
    }

    if (m_ZoomOut > 1)
    {
        int SubCount = 0;
        unsigned long long idxSamp;
        int idxSubSamp, SubSampCount;
        idxSamp = 0;

        SampleCount = m_i32Width * m_ZoomOut;
        if (SampleCount > (u64EndSample - u64StartSample))
            SampleCount = u64EndSample - u64StartSample;

        for (idxSamp = 0; idxSamp < SampleCount; )
        {
            XferSize = SampleCount - idxSamp;
            if ((XferSize % MIN_XFER_SIZE) != 0)
                XferSize = ((XferSize / MIN_XFER_SIZE) + 1) * MIN_XFER_SIZE;

            if (XferSize > XFER_BUFFER_SIZE)
                XferSize = XFER_BUFFER_SIZE;

            SubSampCount = XferSize;

            m_XferTimer.restart();
            i32Ret = m_pGageSystem->TransferDataBuffer((m_i32ChanIdx*m_i32ChanSkip) + 1, m_i64CurrSeg, m_pBuffer, u64StartSample + idxSamp, XferSize, &XferOffset, &i64XferTime);
            *m_pu64XferTime+=i64XferTime;
            *m_pu64XferSamples+=XferSize;
            if(m_bAbort) return 0;
            if(i32Ret<0)
            {
                if(i32Ret==CS_CHANNEL_PROTECT_FAULT)
                {
                    return i32Ret;
                }
                XferOffset = 0;
				ZeroBuffer(m_pBuffer, SampSize, XferSize);
            }

            switch(SampSize)
            {
                case 1:
                    pBuffer8 += XferOffset;
                    CurrVal = *pBuffer8;
                    break;

                case 2:
                    pBuffer16 += XferOffset;
                    CurrVal = *pBuffer16;
                    break;

                case 4:
                    pBuffer32 += XferOffset;
                    CurrVal = *pBuffer32;
                    break;
            }

            SegMin = CurrVal;
            SegMax = CurrVal;

            if ((SampleCount-idxSamp) < (unsigned long long)SubSampCount)
                SubSampCount = SampleCount - idxSamp;

            for (idxSubSamp = 0; idxSubSamp < SubSampCount; idxSubSamp++)
            {
                switch(SampSize)
                {
                    case 1:
                        CurrVal = *pBuffer8++;
                        break;

                    case 2:
                        CurrVal = *pBuffer16++;
                        break;

                    case 4:
                        CurrVal = *pBuffer32++;
                        break;
                }

                if(CurrVal<SegMin) SegMin = CurrVal;
                if(CurrVal>SegMax) SegMax = CurrVal;

                SubCount++;
                idxSamp++;

                if(SubCount>=m_ZoomOut)
                {
                    fltPointYmax = (((fltSampleOffset - SegMax) * fltVertAdjAmpl) / (fltSampleRes * fltAvgCount)) + fltDcVertOffset;
                    PointYmax = fltCenterVertOffset - fltPointYmax;
                    if(PointYmax<0) PointYmax = 0;
                    else if(PointYmax>m_i32Height) PointYmax = m_i32Height;

                    fltPointYmin = (((fltSampleOffset - SegMin) * fltVertAdjAmpl) / (fltSampleRes * fltAvgCount)) + fltDcVertOffset;
                    PointYmin = fltCenterVertOffset - fltPointYmin;
                    if(PointYmin<0) PointYmin = 0;
                    else if(PointYmin>m_i32Height) PointYmin = m_i32Height;

                    if(idxPoint==0)
                    {
                        m_Points[idxPoint].setX(PointX);
                        m_Points[idxPoint++].setY(PointYmin);
                        m_Points[idxPoint].setX(PointX);
                        m_Points[idxPoint++].setY(PointYmax);
                    }
                    else
                    {
                        if(abs(m_Points[idxPoint-1].y()-PointYmin) < abs(m_Points[idxPoint-1].y()-PointYmax))
                        {
                            m_Points[idxPoint].setX(PointX);
                            m_Points[idxPoint++].setY(PointYmin);
                            m_Points[idxPoint].setX(PointX);
                            m_Points[idxPoint++].setY(PointYmax);
                        }
                        else
                        {
                            m_Points[idxPoint].setX(PointX);
                            m_Points[idxPoint++].setY(PointYmax);
                            m_Points[idxPoint].setX(PointX);
                            m_Points[idxPoint++].setY(PointYmin);
                        }
                    }

                    SubCount = 0;
                    PointX++;

                    switch(SampSize)
                    {
                        case 1:
                            CurrVal = *pBuffer8;
                            break;

                        case 2:
                            CurrVal = *pBuffer16;
                            break;

                        case 4:
                            CurrVal = *pBuffer32;
                            break;
                    }
                    SegMin = CurrVal;
                    SegMax = CurrVal;
                }
            }
        }
    }
    else
    {
        //PointCount = m_i32Width / m_ZoomIn;
		PointCount = floor((1.0*m_i32Width) / m_ZoomIn + 0.5);
        if ((unsigned long long)PointCount > (u64EndSample - u64StartSample+1))
            PointCount = u64EndSample - u64StartSample+1;
	
		XferSize = RoundUpTransferSize(PointCount);
    
        m_XferTimer.restart();
        i32Ret = m_pGageSystem->TransferDataBuffer((m_i32ChanIdx*m_i32ChanSkip) + 1, m_i64CurrSeg, m_pBuffer, u64StartSample, XferSize, &XferOffset, &i64XferTime);
        if(m_bAbort) return 0;
        if(i32Ret<0)
        {
            if(i32Ret==CS_CHANNEL_PROTECT_FAULT)
            {
                return i32Ret;
            }
            XferOffset = 0;
			ZeroBuffer(m_pBuffer, SampSize, XferSize);
        }
        *m_pu64XferTime+=i64XferTime;
        *m_pu64XferSamples+=XferSize;

        switch(SampSize)
        {
            case 1:
                pBuffer8 = (unsigned char *)m_pBuffer;
                pBuffer8 += XferOffset;
                break;

            case 2:
                pBuffer16 = (short *)m_pBuffer;
                pBuffer16 += XferOffset;
                break;

            case 4:
                pBuffer32 = (int *)m_pBuffer;
                pBuffer32 += XferOffset;
                break;
        }

        for (idxPoint = 0; idxPoint < PointCount; idxPoint++)
        {
            switch (SampSize)
            {
                case 1:
                    CurrVal = *pBuffer8++;
                    break;

                case 2:
                    CurrVal = *pBuffer16++;
                    break;

                case 4:
                    CurrVal = *pBuffer32++;
                    break;

                default:  // assume default is 2 bytes
                    CurrVal = *pBuffer16++;
                    break;
            }

            //fltPointY = (((fltSampleOffset - CurrVal) * fltVertAdjAmpl) / (fltSampleRes * fltAvgCount)) + fltDcVertOffset;
			fltPointY = (((fltSampleOffset - CurrVal) * fltVertAdjAmpl) / (fltSampleRes * fltAvgCount));
			PointY = fltPointY;

            m_Points[idxPoint].setX(PointX);
            PointY = fltCenterVertOffset - PointY;
            if(PointY<0) PointY = 0;
            else if(PointY>m_i32Height) PointY = m_i32Height;
            m_Points[idxPoint].setY(PointY);

            PointX += m_ZoomIn;
        }
    }

    m_PointCount = idxPoint;

    return 0;
}

int DrawableChannel::GenSamplePointsInter(unsigned long long u64StartSample, unsigned long long u64EndSample)
{
    unsigned char *pBuffer8 = (unsigned char *)m_pBuffer;
    short *pBuffer16 = (short *)m_pBuffer;
    long long i64XferTime;

    float fltVertMaxAmpl = m_dAmplitude * m_u16VerFactor;;
    float fltVerFactor = m_u16VerFactor;
    float fltVertAdjAmpl = fltVertMaxAmpl * fltVerFactor;
    float fltSampleOffset = m_i32SampleOffset;
    float fltSampleRes = m_i32SampleRes;
    float fltDcVertOffset = floor(((fltVerFactor * fltVertMaxAmpl) * (m_i32DcOffset / (m_i32Range / 2.0))) + 0.5);
    float fltCenterVertOffset = m_dCenterY;

    unsigned long long SampleCount;

    int PointY, PointYmin, PointYmax;;
    float fltPointY, fltPointYmin, fltPointYmax;;
    int PointCount;
    int CurrVal = 0, SegMin = 0, SegMax = 0;

    int idxPoint = 0;
    unsigned short PointX = 0;

    int i32Ret = CS_SUCCESS;
    int XferSize, XferOffset;

    unsigned char SampSize = m_i32SampleSize;

    if (m_ZoomOut > 1)
    {
        int SubCount = 0;
        unsigned long long idxSamp;
        int idxSubSamp, SubSampCount;
        idxSamp = 0;

        SampleCount = m_i32Width * m_ZoomOut;
        if(SampleCount>(u64EndSample - u64StartSample)) SampleCount = u64EndSample - u64StartSample;

        for(idxSamp=0;idxSamp<SampleCount; )
        {
            XferSize = (SampleCount - idxSamp) * m_i32ActiveChanCount;
            if((XferSize % MIN_XFER_SIZE)!=0) XferSize = ((XferSize / MIN_XFER_SIZE) + 1) * MIN_XFER_SIZE;
            if(XferSize>XFER_BUFFER_SIZE) XferSize = XFER_BUFFER_SIZE;

            SubSampCount = XferSize / m_i32ActiveChanCount;

            m_XferTimer.restart();
            i32Ret = m_pGageSystem->TransferDataInterBuffer(m_i64CurrSeg, m_pBuffer, u64StartSample + idxSamp, XferSize, &XferOffset, &i64XferTime);
            if(m_bAbort) return 0;
            if(i32Ret<0)
            {
                if(i32Ret==CS_CHANNEL_PROTECT_FAULT)
                {
                    return i32Ret;
                }
                XferOffset = 0;
				ZeroBuffer(m_pBuffer, SampSize, XferSize);
            }
            *m_pu64XferTime+=i64XferTime;
            *m_pu64XferSamples+=XferSize;

            switch(SampSize)
            {
                case 1:
                     pBuffer8 += XferOffset + m_i32ChanIdx;
                    CurrVal = *pBuffer8;
                    break;

                case 2:
                     pBuffer16 += XferOffset + m_i32ChanIdx;
                    CurrVal = *pBuffer16;
                    break;
            }

            SegMin = CurrVal;
            SegMax = CurrVal;

            for(idxSubSamp=0;idxSubSamp<SubSampCount;idxSubSamp++)
            {
                switch(SampSize)
                {
                    case 1:
                        CurrVal = *pBuffer8;
                        pBuffer8+=m_i32ActiveChanCount;
                        break;

                    case 2:
                        CurrVal = *pBuffer16;
                        pBuffer16+=m_i32ActiveChanCount;
                        break;
                }

                if(CurrVal<SegMin) SegMin = CurrVal;
                if(CurrVal>SegMax) SegMax = CurrVal;

                SubCount++;

                if(SubCount>=m_ZoomOut)
                {
                    fltPointYmax = (((fltSampleOffset - SegMax) * fltVertAdjAmpl) / fltSampleRes) + fltDcVertOffset;
                    PointYmax = fltCenterVertOffset - fltPointYmax;
                    if(PointYmax<0) PointYmax = 0;
                    else if(PointYmax>m_i32Height) PointYmax = m_i32Height;

                    fltPointYmin = (((fltSampleOffset - SegMin) * fltVertAdjAmpl) / fltSampleRes) + fltDcVertOffset;
                    PointYmin = fltCenterVertOffset - fltPointYmin;
                    if(PointYmin<0) PointYmin = 0;
                    else if(PointYmin>m_i32Height) PointYmin = m_i32Height;

                    if(idxPoint==0)
                    {
                        m_Points[idxPoint].setX(PointX);
                        m_Points[idxPoint++].setY(PointYmin);
                        m_Points[idxPoint].setX(PointX);
                        m_Points[idxPoint++].setY(PointYmax);
                    }
                    else
                    {
                        if(abs(m_Points[idxPoint-1].y()-PointYmin) < abs(m_Points[idxPoint-1].y()-PointYmax))
                        {
                            m_Points[idxPoint].setX(PointX);
                            m_Points[idxPoint++].setY(PointYmin);
                            m_Points[idxPoint].setX(PointX);
                            m_Points[idxPoint++].setY(PointYmax);
                        }
                        else
                        {
                            m_Points[idxPoint].setX(PointX);
                            m_Points[idxPoint++].setY(PointYmax);
                            m_Points[idxPoint].setX(PointX);
                            m_Points[idxPoint++].setY(PointYmin);
                        }
                    }

                    SubCount = 0;
                    PointX++;

                    switch(SampSize)
                    {
                        case 1:
                            CurrVal = *pBuffer8;
                            break;

                        case 2:
                            CurrVal = *pBuffer16;
                            break;
                    }
                    SegMin = CurrVal;
                    SegMax = CurrVal;
                }

                idxSamp++;

                if (idxSamp >= SampleCount) break;
            }
        }
    }
    else
    {
        PointCount = m_i32Width / m_ZoomIn;
        if ((unsigned long long)PointCount > (u64EndSample - u64StartSample))
            PointCount = u64EndSample - u64StartSample;

		XferSize = RoundUpTransferSize(PointCount * m_i32ActiveChanCount);

        m_XferTimer.restart();
        i32Ret = m_pGageSystem->TransferDataInterBuffer( m_i64CurrSeg, m_pBuffer, u64StartSample, XferSize, &XferOffset, &i64XferTime);
        if(m_bAbort) return 0;
        if(i32Ret<0)
        {
            if(i32Ret==CS_CHANNEL_PROTECT_FAULT)
            {
                return i32Ret;
            }
            XferOffset = 0;
			ZeroBuffer(m_pBuffer, SampSize, XferSize);
        }
        *m_pu64XferTime+=i64XferTime;
        *m_pu64XferSamples+=XferSize;

        switch(SampSize)
        {
            case 1:
                pBuffer8 = (unsigned char *)m_pBuffer;
                pBuffer8 += XferOffset + m_i32ChanIdx;
                break;

            case 2:
                pBuffer16 = (short *)m_pBuffer;
                pBuffer16 += XferOffset + m_i32ChanIdx;
                break;
        }

        for(idxPoint=0;idxPoint<PointCount;idxPoint++)
        {
            switch(SampSize)
            {
                case 1:
                    CurrVal = *pBuffer8;
                    pBuffer8+=m_i32ActiveChanCount;
                    break;

                case 2:
                    CurrVal = *pBuffer16;
                    pBuffer16+=m_i32ActiveChanCount;
                    break;

            }

            fltPointY = (((fltSampleOffset - CurrVal) * fltVertAdjAmpl) / fltSampleRes) + fltDcVertOffset;

            PointY = fltPointY;

            m_Points[idxPoint].setX(PointX);
            PointY = fltCenterVertOffset - PointY;
            if(PointY<0) PointY = 0;
            else if(PointY>m_i32Height) PointY = m_i32Height;
            m_Points[idxPoint].setY(PointY);

            PointX += m_ZoomIn;
        }
    }

    m_PointCount = idxPoint;
    return i32Ret;
}

void DrawableChannel::GenSamplePointsSigFile(unsigned long long u64StartSample, unsigned long long u64EndSample)
{
    unsigned char *pBuffer8 = NULL;
    short *pBuffer16 = NULL;

    float fltVertMaxAmpl = m_dAmplitude * m_u16VerFactor;
    float fltVerFactor = m_u16VerFactor;
    float fltVertAdjAmpl = fltVertMaxAmpl * fltVerFactor;
    float fltSampleOffset = m_i32SampleOffset;
    float fltSampleRes = m_i32SampleRes;
    float fltDcVertOffset = floor(((fltVerFactor * fltVertMaxAmpl) * (m_i32DcOffset / (m_i32Range / 2.0))) + 0.5);
    float fltCenterVertOffset = m_dCenterY;

    unsigned long long SampleCount;
    unsigned long long u64SampPos;

    int PointY, PointYmin, PointYmax;;
    float fltPointY, fltPointYmin, fltPointYmax;;
    int PointCount;
    int CurrVal = 0, SegMin, SegMax;
    int SampSize = m_i32SampleSize;

    int idxPoint = 0;
    unsigned short PointX = 0;

    int XferSize;

    QFile SigFile;
    QByteArray SigByteArr;
    bool bRes;

    SigFile.setFileName(m_strFilePath);

    bRes = SigFile.open(QIODevice::ReadOnly);
    if (!bRes)
    {
        //m_ErrMng->UpdateErrMsg("Error creating the signal file", -1);
        //return;
    }

    SigFile.seek(SIG_FILE_HEADER_SIZE + (u64StartSample * m_i32SampleSize));

    u64SampPos = u64StartSample;

    if (m_ZoomOut > 1)
    {
        int SubCount = 0;
        unsigned long long idxSamp;
        int idxSubSamp, SubSampCount;
        idxSamp = 0;

        SampleCount = m_i32Width * m_ZoomOut;
        if(SampleCount>(u64EndSample - u64StartSample)) SampleCount = u64EndSample - u64StartSample;

        for(idxSamp=0;idxSamp<SampleCount; )
        {
            XferSize = SampleCount - idxSamp;
            if((XferSize % MIN_XFER_SIZE)!=0) XferSize = ((XferSize / MIN_XFER_SIZE) + 1) * MIN_XFER_SIZE;
            if(XferSize>XFER_BUFFER_SIZE) XferSize = XFER_BUFFER_SIZE;

            SubSampCount = XferSize;

            SigByteArr = SigFile.read(XFER_BUFFER_SIZE*m_i32SampleSize);

            if(m_bAbort)
            {
                SigFile.close();
                return;
            }

            switch(SampSize)
            {
                case 1:
                    pBuffer8 = (unsigned char *)SigByteArr.data();
                    CurrVal = *pBuffer8;
                    break;

                case 2:
                    pBuffer16 = (short *)SigByteArr.data();
                    CurrVal = *pBuffer16;
                    break;
            }

            SegMin = CurrVal;
            SegMax = CurrVal;

            for(idxSubSamp=0;idxSubSamp<SubSampCount;idxSubSamp++)
            {
                switch(SampSize)
                {
                    case 1:
                        CurrVal = *pBuffer8++;
                        break;

                    case 2:
                        CurrVal = *pBuffer16++;
                        break;
                }

                if(CurrVal<SegMin) SegMin = CurrVal;
                if(CurrVal>SegMax) SegMax = CurrVal;

                SubCount++;

                if(SubCount>=m_ZoomOut)
                {
                    fltPointYmax = (((fltSampleOffset - SegMax) * fltVertAdjAmpl) / fltSampleRes) + fltDcVertOffset;
                    PointYmax = fltCenterVertOffset - fltPointYmax;
                    if(PointYmax<0) PointYmax = 0;
                    else if(PointYmax>m_i32Height) PointYmax = m_i32Height;

                    fltPointYmin = (((fltSampleOffset - SegMin) * fltVertAdjAmpl) / fltSampleRes) + fltDcVertOffset;
                    PointYmin = fltCenterVertOffset - fltPointYmin;
                    if(PointYmin<0) PointYmin = 0;
                    else if(PointYmin>m_i32Height) PointYmin = m_i32Height;

                    if(idxPoint==0)
                    {
                        m_Points[idxPoint].setX(PointX);
                        m_Points[idxPoint++].setY(PointYmin);
                        m_Points[idxPoint].setX(PointX);
                        m_Points[idxPoint++].setY(PointYmax);
                    }
                    else
                    {
                        if(abs(m_Points[idxPoint-1].y()-PointYmin) < abs(m_Points[idxPoint-1].y()-PointYmax))
                        {
                            m_Points[idxPoint].setX(PointX);
                            m_Points[idxPoint++].setY(PointYmin);
                            m_Points[idxPoint].setX(PointX);
                            m_Points[idxPoint++].setY(PointYmax);
                        }
                        else
                        {
                            m_Points[idxPoint].setX(PointX);
                            m_Points[idxPoint++].setY(PointYmax);
                            m_Points[idxPoint].setX(PointX);
                            m_Points[idxPoint++].setY(PointYmin);
                        }
                    }

                    SubCount = 0;
                    PointX++;

                    switch(SampSize)
                    {
                        case 1:
                            CurrVal = *pBuffer8;
                            break;

                        case 2:
                            CurrVal = *pBuffer16;
                            break;
                    }
                    SegMin = CurrVal;
                    SegMax = CurrVal;
                }

                idxSamp++;
                u64SampPos++;

                if (idxSamp >= SampleCount) break;
                if(u64SampPos>=m_u64SampleCount) break;
            }
            if(u64SampPos>=m_u64SampleCount) break;
        }
    }
    else
    {
        PointCount = m_i32Width / m_ZoomIn;
        if ((unsigned long long)PointCount > (u64EndSample - u64StartSample))
            PointCount = u64EndSample - u64StartSample;

        if ((PointCount % MIN_XFER_SIZE) == 0)
            XferSize = PointCount;

        else XferSize = ((PointCount / MIN_XFER_SIZE) + 1) * MIN_XFER_SIZE;

        SigByteArr = SigFile.read(XFER_BUFFER_SIZE*m_i32SampleSize);

        switch(SampSize)
        {
            case 1:
                pBuffer8 = (unsigned char *)SigByteArr.data();
                break;

            case 2:
                pBuffer16 = (short *)SigByteArr.data();
                break;
        }
        if(m_bAbort)
        {
            SigFile.close();
            return;
        }

        for(idxPoint=0;idxPoint<PointCount;idxPoint++)
        {
            switch(SampSize)
            {
                case 1:
                    CurrVal = *pBuffer8++;
                    break;

                case 2:
                    CurrVal = *pBuffer16++;
                    break;
            }

            fltPointY = (((fltSampleOffset - CurrVal) * fltVertAdjAmpl) / fltSampleRes) + fltDcVertOffset;

            PointY = fltPointY;

            m_Points[idxPoint].setX(PointX);
            PointY = fltCenterVertOffset - PointY;
            if(PointY<0) PointY = 0;
            else if(PointY>m_i32Height) PointY = m_i32Height;
            m_Points[idxPoint].setY(PointY);

            PointX += m_ZoomIn;
            u64SampPos++;

            if(u64SampPos>=m_u64SampleCount) break;
        }
    }

    m_PointCount = idxPoint;

    SigFile.close();
}

void DrawableChannel::GenSamplePointsDataFile(unsigned long long u64StartSample, unsigned long long u64EndSample)
{
    unsigned char *pBuffer8 = NULL;
    short *pBuffer16 = NULL;

    float fltVertMaxAmpl = m_dAmplitude * m_u16VerFactor;;
    float fltVerFactor = m_u16VerFactor;
    float fltVertAdjAmpl = fltVertMaxAmpl * fltVerFactor;
    float fltSampleOffset = m_i32SampleOffset;
    float fltSampleRes = m_i32SampleRes;
    float fltDcVertOffset = floor(((fltVerFactor * fltVertMaxAmpl) * (m_i32DcOffset / (m_i32Range / 2.0))) + 0.5);
    float fltCenterVertOffset = m_dCenterY;

    unsigned long long SampleCount;

    int PointY, PointYmin, PointYmax;;
    float fltPointY, fltPointYmin, fltPointYmax;;
    int PointCount;
    int CurrVal = 0 , SegMin = 0, SegMax = 0;
    int SampSize = m_i32SampleSize;

    int idxPoint = 0;
    unsigned short PointX = 0;
    int ReadSize;

    int XferSize;

    QFile DataFile;
    QByteArray DataByteArr;
    bool bRes;

    DataFile.setFileName(m_strFilePath);

    bRes = DataFile.open(QIODevice::ReadOnly);
    if (!bRes)
    {
        //m_ErrMng->UpdateErrMsg("Error creating the signal file", -1);
        //return;
    }

    DataFile.seek((u64StartSample * m_i32ActiveChanCount * m_i32SampleSize) + (m_i32ChanIdx * m_i32SampleSize));

    if (m_ZoomOut > 1)
    {
        int SubCount = 0;
        unsigned long long idxSamp;
        int idxSubSamp, SubSampCount;
        idxSamp = 0;

        SampleCount = m_i32Width * m_ZoomOut;
        if(SampleCount>(u64EndSample - u64StartSample)) SampleCount = u64EndSample - u64StartSample;

        for(idxSamp=0;idxSamp<SampleCount; )
        {
            DataFile.seek(((u64StartSample + idxSamp) * m_i32ActiveChanCount * m_i32SampleSize) + (m_i32ChanIdx * m_i32SampleSize));

            XferSize = (SampleCount - idxSamp) * m_i32ActiveChanCount;
            if(XferSize>XFER_BUFFER_SIZE) XferSize = XFER_BUFFER_SIZE;

            SubSampCount = XferSize / m_i32ActiveChanCount;

            ReadSize = XferSize * m_i32ActiveChanCount * m_i32SampleSize;
            m_XferTimer.restart();
            DataByteArr = DataFile.read(ReadSize);
            *m_pu64XferTime+=m_XferTimer.nsecsElapsed();
            *m_pu64XferSamples+=ReadSize/2;

            switch(SampSize)
            {
                case 1:
                    pBuffer8 = (unsigned char *)DataByteArr.data();
                    CurrVal = *pBuffer8;
                    break;

                case 2:
                    pBuffer16 = (short *)DataByteArr.data();
                    CurrVal = *pBuffer16;
                    break;
            }
            if(m_bAbort)
            {
                DataFile.close();
                return;
            }

            SegMin = CurrVal;
            SegMax = CurrVal;

            for(idxSubSamp=0;idxSubSamp<SubSampCount;idxSubSamp++)
            {
                switch(SampSize)
                {
                    case 1:
                        CurrVal = *pBuffer8;
                        pBuffer8+=m_i32ActiveChanCount;
                        break;

                    case 2:
                        CurrVal = *pBuffer16;
                        pBuffer16+=m_i32ActiveChanCount;
                        break;
                }

                if(CurrVal<SegMin) SegMin = CurrVal;
                if(CurrVal>SegMax) SegMax = CurrVal;

                SubCount++;

                if(SubCount>=m_ZoomOut)
                {
                    fltPointYmax = (((fltSampleOffset - SegMax) * fltVertAdjAmpl) / fltSampleRes) + fltDcVertOffset;
                    PointYmax = fltCenterVertOffset - fltPointYmax;
                    if(PointYmax<0) PointYmax = 0;
                    else if(PointYmax>m_i32Height) PointYmax = m_i32Height;

                    fltPointYmin = (((fltSampleOffset - SegMin) * fltVertAdjAmpl) / fltSampleRes) + fltDcVertOffset;
                    PointYmin = fltCenterVertOffset - fltPointYmin;
                    if(PointYmin<0) PointYmin = 0;
                    else if(PointYmin>m_i32Height) PointYmin = m_i32Height;

                    if(idxPoint==0)
                    {
                        m_Points[idxPoint].setX(PointX);
                        m_Points[idxPoint++].setY(PointYmin);
                        m_Points[idxPoint].setX(PointX);
                        m_Points[idxPoint++].setY(PointYmax);
                    }
                    else
                    {
                        if(abs(m_Points[idxPoint-1].y()-PointYmin) < abs(m_Points[idxPoint-1].y()-PointYmax))
                        {
                            m_Points[idxPoint].setX(PointX);
                            m_Points[idxPoint++].setY(PointYmin);
                            m_Points[idxPoint].setX(PointX);
                            m_Points[idxPoint++].setY(PointYmax);
                        }
                        else
                        {
                            m_Points[idxPoint].setX(PointX);
                            m_Points[idxPoint++].setY(PointYmax);
                            m_Points[idxPoint].setX(PointX);
                            m_Points[idxPoint++].setY(PointYmin);
                        }
                    }

                    SubCount = 0;
                    PointX++;

                    switch(SampSize)
                    {
                        case 1:
                            CurrVal = *pBuffer8;
                            break;

                        case 2:
                            CurrVal = *pBuffer16;
                            break;
                    }
                    SegMin = CurrVal;
                    SegMax = CurrVal;
                }

                idxSamp++;

                if (idxSamp >= SampleCount) break;
            }
        }
    }
    else
    {
        PointCount = m_i32Width / m_ZoomIn;
        if ((unsigned long long)PointCount > (u64EndSample - u64StartSample))
            PointCount = u64EndSample - u64StartSample;

        XferSize = RoundUpTransferSize(PointCount * m_i32ActiveChanCount);

        ReadSize = XferSize * m_i32ActiveChanCount * m_i32SampleSize;
        m_XferTimer.restart();
        DataByteArr = DataFile.read(ReadSize);
        *m_pu64XferTime+=m_XferTimer.nsecsElapsed();
        *m_pu64XferSamples+=ReadSize/2;

        switch(SampSize)
        {
            case 1:
                pBuffer8 = (unsigned char *)DataByteArr.data();
                break;

            case 2:
                pBuffer16 = (short *)DataByteArr.data();
                break;
        }
        if(m_bAbort)
        {
            DataFile.close();
            return;
        }

        for(idxPoint=0;idxPoint<PointCount;idxPoint++)
        {
            switch(SampSize)
            {
                case 1:
                    CurrVal = *pBuffer8;
                    pBuffer8+=m_i32ActiveChanCount;
                    break;

                case 2:
                    CurrVal = *pBuffer16;
                    pBuffer16+=m_i32ActiveChanCount;
                    break;
            }

            fltPointY = (((fltSampleOffset - CurrVal) * fltVertAdjAmpl) / fltSampleRes) + fltDcVertOffset;

            PointY = fltPointY;

            m_Points[idxPoint].setX(PointX);
            PointY = fltCenterVertOffset - PointY;
            if(PointY<0) PointY = 0;
            else if(PointY>m_i32Height) PointY = m_i32Height;
            m_Points[idxPoint].setY(PointY);

            PointX += m_ZoomIn;
        }
    }

    m_PointCount = idxPoint;

    DataFile.close();
}

void DrawableChannel::ZeroBuffer(void *pBuffer, int SampleSize, int SampleCount)
{
	memset(pBuffer, 0, SampleSize*SampleCount);   
}

void DrawableChannel::SetFlatLine()
{
    int i;

    for(i=0;i<m_i32Width;i++)
    {
        m_Points[i].setX(i);
        m_Points[i].setY(m_dCenterY);
    }

    m_PointCount = i;
}

uInt32	DrawableChannel::RoundUpTransferSize(uInt32 SamplesToDisplay)
{
	if ((SamplesToDisplay % MIN_XFER_SIZE) == 0)
		return SamplesToDisplay;
	else
		return  (SamplesToDisplay / MIN_XFER_SIZE + 1) * MIN_XFER_SIZE;

}
