#include <CsTypes.h>

#include "TriangleWave.h"
#include <math.h>
#include "xml.h"
#include "Gage_XML_Defines.h"

UINT GetGageDirectory(LPTSTR str, UINT uSize);

CTriangleWave::CTriangleWave(const string strSysInfo) : CBaseSignal(strSysInfo), m_dAmplitude(1.0), m_dFrequency(10000000.0),
	m_dDcOffset(0.), m_dPhaseShift(0.), m_dDuty(0.5)
{
	m_dPeriod = 1.0/ m_dFrequency;
	m_dPosWidth = m_dPeriod * m_dDuty;
}

CTriangleWave::~CTriangleWave(void)
{
}

int CTriangleWave::ReadParams(void)
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
			if (0 == nData)
				nData = 1000000;
			m_dFrequency = double(nData);

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_PHASE, 0, strFileName.c_str());
			dwPhase = DWORD(nData);
			if ( dwPhase > 359 )
				dwPhase = 0;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_NOISE, 8, strFileName.c_str());
			m_dNoiseLevel = double(nData) / 1.e3;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_OFFSET, 0, strFileName.c_str());
			m_dDcOffset = double(nData) / 1.e3;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_DUTY, 50, strFileName.c_str());
			if ( 0 == nData)
				nData = 1;
			if ( 99 < nData)
				nData = 99;

			m_dDuty = double(nData) / 1.e2;

			bError = false;
		}
		delete pSystem;
	}

	//Amplitude devided by 2 to convert from pk-pk => unipolar
	m_dAmplitude /= 2.;

	m_dPeriod = 1.0/m_dFrequency;
	m_dPhaseShift = m_dPeriod *  double(dwPhase)/360.;
	m_dPosWidth = m_dPeriod * m_dDuty;

	m_dK_1 = 2.0 * m_dAmplitude / m_dPosWidth;
	m_dK_2 = 2.0 * m_dAmplitude / (m_dPosWidth - m_dPeriod);

	m_dB_2 = m_dAmplitude - m_dPosWidth * m_dK_2;

	if (bError)
		return 0;
	else
		return 1;
}


double CTriangleWave::GetRawVoltage(double dTime)
{
	dTime += m_dPhaseShift;

	int n = int(dTime / m_dPeriod);
	if(n < 0)
		n--;

	double dTau = dTime - (n * m_dPeriod);

	if (dTau < 0.)
		dTau += m_dPeriod;

	if (dTau < m_dPosWidth )
		return (m_dK_1 * dTau - m_dAmplitude + m_dDcOffset);
	else
		return (m_dB_2 + m_dK_2 * dTau + m_dDcOffset);
}
