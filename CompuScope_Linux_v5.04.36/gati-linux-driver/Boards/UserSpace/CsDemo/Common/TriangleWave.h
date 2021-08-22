#ifndef __TRIANGLE_H__
#define __TRIANGLE_H__

#pragma once

#include "BaseSignal.h"

class CTriangleWave :
	public CBaseSignal
{
public:
	CTriangleWave(const string strSysInfo);
	~CTriangleWave(void);

	virtual int ReadParams(void);
	virtual double GetRawVoltage(double dTime);

private:
	double	m_dAmplitude;   // in V
	double	m_dFrequency;   // in Hz
	double	m_dDcOffset;    // in V
	double	m_dPhaseShift;  // in sec
	double  m_dDuty;        // in %

	double  m_dPosWidth;
	double  m_dPeriod;
	double m_dK_1;
	double m_dK_2;
	double m_dB_2;
};

#endif // __TRIANGLE_H__
