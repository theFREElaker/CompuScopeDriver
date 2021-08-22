#ifndef __SINEWAVE_H__
#define __SINEWAVE_H__

#pragma once

#if defined WIN32 || defined CS_WIN64
	#include "stdafx.h"
#endif

#include "BaseSignal.h"

class CSineWave :
	public CBaseSignal
{
public:
	CSineWave(const string strSysInfo);
	~CSineWave(void);

	virtual int ReadParams(void);
	virtual double GetRawVoltage(double dTime);

private:
	double	m_dAmplitude;      // in V
	double	m_dFrequency;      // in Hz
	double	m_dDcOffset;       // in V
	double  m_dPhase;          // in rad
	double	m_dModAmplitude;   // in Hz
	double	m_dModFrequency;   // in Hz
};

#endif // __SINEWAVE_H__
