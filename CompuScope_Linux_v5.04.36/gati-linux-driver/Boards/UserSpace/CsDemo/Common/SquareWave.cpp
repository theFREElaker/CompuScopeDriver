
#include <CsTypes.h>

#include "SquareWave.h"
#include <math.h>
#include "xml.h"
#include "Gage_XML_Defines.h"

UINT GetGageDirectory(LPTSTR str, UINT uSize);

CSquareWave::CSquareWave(const string strSysInfo) : CBaseSignal(strSysInfo), m_dAmplitude(0.85), m_dFrequency(1000000.0),
	m_dDcOffset(0.), m_dDuty(.50), m_dPhaseShift(0.), m_dRiseTime(0.), m_dFallTime(0.),
	m_dHighRingTime(0.), m_dLowRingTime(0.)
{
	m_dPeriod = 1./m_dFrequency;
	m_dPosWidth = m_dPeriod * m_dDuty;

	m_dHigh = m_dDcOffset + m_dAmplitude;
    m_dLow =  m_dDcOffset - m_dAmplitude;
}

CSquareWave::~CSquareWave(void)
{
}

int CSquareWave::ReadParams(void)
{
	bool bError = true;
	string strFileName;

	DWORD dwPhase(0);

	TCHAR tcDirectory[MAX_PATH] = {'\n'};

	if (GetGageDirectory(tcDirectory, MAX_PATH))
	{
		strFileName = tcDirectory;
		strFileName.append(XML_FILE_NAME);
	}
	else
	{
		strFileName = XML_FILE_NAME;
	}

	XML* pSystem = new XML(strFileName.c_str());
	if (pSystem)
	{
		XMLElement* pRoot = pSystem->GetRootElement();

		if (pRoot)
		{
			int nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_AMPLITUDE, 2000, strFileName.c_str());
			m_dAmplitude = double(nData) / 1.e3;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_FREQUENCY, 1000000, strFileName.c_str());
			m_dFrequency = double(nData);

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_DUTY, 50, strFileName.c_str());
			if ( 0 == nData )
				nData = 1;
			if ( 99 < nData )
				nData = 99;

			m_dDuty = double(nData) / 1.e2;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_NOISE, 8, strFileName.c_str());
			m_dNoiseLevel = double(nData) / 1.e3;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_OFFSET, 0, strFileName.c_str());
			m_dDcOffset = double(nData) / 1.e3;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_PHASE, 0, strFileName.c_str());
			dwPhase = DWORD(nData);
			if ( dwPhase > 359 )
				dwPhase = 0;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_RISE_TIME, 30, strFileName.c_str());
			m_dRiseTime = double(nData) * 1.e9;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_FALL_TIME, 4, strFileName.c_str());
			m_dFallTime = double(nData) * 1.e9;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_HIGH_RING, 48, strFileName.c_str());
			m_dHighRingTime = double(nData) * 1.e9;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_LOW_RING, 14, strFileName.c_str());
			m_dLowRingTime = double(nData) * 1.e9;

			bError = false;
		}
		delete pSystem;
	}

	//Amplitude devided by 2 to convert from pk-pk => unipolar
	m_dAmplitude /= 2.;

	m_dPeriod = 1./m_dFrequency;
	m_dPosWidth = m_dPeriod * m_dDuty;

	m_dHigh = m_dDcOffset + m_dAmplitude;
    m_dLow =  m_dDcOffset - m_dAmplitude;

	m_dPhaseShift = m_dPeriod *  double(dwPhase)/360.;

	m_dRiseTime = max(m_dRiseTime, m_dPosWidth/1000.);
	m_dFallTime = max(m_dFallTime, (m_dPeriod - m_dPosWidth)/1000.);

	m_dRiseTime = min(m_dRiseTime, m_dPosWidth/2.);
	m_dFallTime = min(m_dFallTime, (m_dPeriod - m_dPosWidth)/2.);

	m_dHighRingTime = min(m_dHighRingTime, m_dPosWidth/3.);
	m_dLowRingTime = min(m_dLowRingTime, (m_dPeriod - m_dPosWidth)/3.);

	m_dHighAlpha = M_PI / m_dRiseTime;
	m_dLowAlpha = M_PI /m_dFallTime;

	m_dHighBeta = (m_dHigh - m_dLow)/ m_dHighAlpha;
	m_dLowBeta = (m_dHigh - m_dLow)/ m_dLowAlpha;

	if (bError)
		return 0;
	else
		return 1;
}


double CSquareWave::GetRawVoltage(double dTime)
{
	dTime += m_dPhaseShift;

	int n = int(dTime / m_dPeriod);

	if (n < 0)
		n--;

	double dTau = dTime - (n * m_dPeriod);

	if (dTau < 0.)
		dTau += m_dPeriod;

	if (0. == dTau)
		return m_dLow;

	else if (dTau < m_dRiseTime)
	{
		return m_dHigh - m_dHighBeta * sin(m_dHighAlpha * dTau) / dTau;
	}
	else if (dTau < m_dPosWidth)
	{
		return m_dHigh - m_dHighBeta * sin(m_dHighAlpha * dTau) / dTau * exp((m_dRiseTime - dTau) / m_dHighRingTime)*(dTau / (dTau + 4 * m_dRiseTime));
	}
	else
	{
		dTau -= m_dPosWidth;
		if (dTau < m_dFallTime)
		{
			if (0. == dTau)
				return m_dHigh;
			else
			{
				return m_dLow + m_dLowBeta * sin(m_dLowAlpha * dTau) / dTau;
			}
		}
		else
		{
			return m_dLow + m_dLowBeta * sin(m_dLowAlpha * dTau) / dTau * exp((m_dFallTime - dTau)/m_dLowRingTime) * (dTau / (dTau + 4 * m_dFallTime));
		}
	}
}
