
#include "CsTypes.h"

#include "SineWave.h"
#include <math.h>
#include "xml.h"
#include "Gage_XML_Defines.h"

UINT GetGageDirectory(LPTSTR str, UINT uSize);


CSineWave::CSineWave(const string strSysInfo) : CBaseSignal(strSysInfo), m_dAmplitude(0.87), m_dFrequency(1000000.0),
		m_dDcOffset(0.), m_dPhase(0.), m_dModAmplitude(0.), m_dModFrequency(0.)
{
}

CSineWave::~CSineWave(void)
{
}

int CSineWave::ReadParams(void)
{
	bool bError = true;
	string strFileName;

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
			int nData = 0;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_AMPLITUDE, 2000, strFileName.c_str());
			m_dAmplitude = double(nData) / 1.e3;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_FREQUENCY, 1000000, strFileName.c_str());
			m_dFrequency = double(nData);

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_PHASE, 0, strFileName.c_str());
			if (nData > 359)
				nData = 0;
			m_dPhase = M_PI * double(nData) / 180;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_NOISE, 6, strFileName.c_str());
			m_dNoiseLevel = double(nData) / 1.e3;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_OFFSET, 0, strFileName.c_str());
			m_dDcOffset = double(nData) / 1.e3;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_MOD_FREQUENCY, 0, strFileName.c_str());
			m_dModFrequency = double(nData);

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_MOD_AMPLITUDE, 0, strFileName.c_str());
			m_dModAmplitude = double(nData);

			bError = false;
		}
		delete pSystem;
	}

	//Amplitude devided by 2 to convert from pk-pk => unipolar
	m_dAmplitude /= 2.;

	m_dFrequency *= 2.0 * M_PI;
	m_dModFrequency *= 2.0 * M_PI;

	if (bError)
		return 0;
	else
		return 1;
}


double CSineWave::GetRawVoltage(double dTime)
{
	return m_dAmplitude * sin((m_dFrequency + m_dModAmplitude * sin(m_dModFrequency * dTime)) * dTime + m_dPhase) + m_dDcOffset;
}
