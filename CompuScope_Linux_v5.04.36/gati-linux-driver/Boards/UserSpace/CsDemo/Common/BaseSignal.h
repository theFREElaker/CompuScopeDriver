#ifndef __BASESIGNAL_H__
#define __BASESIGNAL_H__

#pragma once

#if defined(WIN32) || defined(CS_WIN64)
	#include "stdafx.h"
#endif

#include <string>
#include <vector>

#ifdef __linux__
	#include "CsLinuxPort.h"
	#include <math.h>
#endif


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std ;

class CBaseSignal
{
public:
	CBaseSignal(const string strSysInfo);
	virtual ~CBaseSignal(void);
	virtual int ReadParams(void) = 0;
	virtual double GetRawVoltage(double dTime) = 0;
	double  GetSampleRate(void) { return m_dSampleRate; }
	double* GetNoisePointer(void) { return m_pdNoiseArray; }
	void    CreateNoiseArray(void);
	double GetVoltage(double dTime)
	{
		return (GetRawVoltage(dTime) + m_pdNoiseArray[(m_uNoiseIndex++)%c_uMagicPrime]);
	}
	void  SeedNoiseIndex(unsigned uSeg)
	{
		m_uNoiseIndex = uSeg*c_uMagicPrimeShift;
	}

	void SetSampleRate(double dSampleRate) 	
	{
		m_dSampleRate = fabs(dSampleRate); 
	}

protected:
	double	 m_dSampleRate;
	double*	 m_pdNoiseArray;
	double   m_dNoiseLevel; //in V
	unsigned m_uNoiseIndex;
	double   m_dImpedanceFactor;

	static const unsigned c_uMagicPrime;
	static const unsigned c_uMagicPrimeShift;

	string m_strSysInfo;
};

#endif // __BASESIGNAL_H__
