#ifndef __SQUAREWAVE_H__
#define __SQUAREWAVE_H__

#pragma once

#include "BaseSignal.h"

class CSquareWave :
	public CBaseSignal
{
public:
	CSquareWave(const string strSysInfo);
	~CSquareWave(void);

	virtual int ReadParams(void);
	virtual double GetRawVoltage(double dTime);

private:
	double	m_dAmplitude;   // in V
	double	m_dFrequency;   // in Hz
	double	m_dDcOffset;    // in V
	double	m_dDuty;        // in %
	double	m_dPhaseShift;  // in sec

	double  m_dPeriod;
	double  m_dPosWidth;

	double  m_dRiseTime;
	double  m_dFallTime;

	double m_dHighRingTime;
	double m_dLowRingTime;

	double m_dHighAlpha;
	double m_dLowAlpha;

	double m_dHighBeta;
	double m_dLowBeta;

	double  m_dHigh;
	double  m_dLow;
};

#endif // __SQUAREWAVE_H__
