#ifndef __TWOTONE_H__
#define __TWOTONE_H__

#pragma once

#if defined WIN32 || defined CS_WIN64
	#include "stdafx.h"
#endif

#include "BaseSignal.h"

class CTwoTone :
	public CBaseSignal
{
public:
	CTwoTone(const string strSysInfo);
	~CTwoTone(void);

	virtual int ReadParams(void);
	virtual double GetRawVoltage(double dTime);

private:
	double	m_dAmplitude;   // in V
	double	m_dFrequency1;  // in Hz
	double	m_dFrequency2;  // in Hz
	double	m_dDcOffset;    // in V
	double  m_dPhase;       // in rad
};

#endif // __TWOTONE_H__