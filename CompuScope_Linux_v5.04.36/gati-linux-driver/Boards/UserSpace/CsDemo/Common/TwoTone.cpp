
#include <CsTypes.h>

#include "TwoTone.h"
#include <math.h>
#include "xml.h"
#include "Gage_XML_Defines.h"

UINT GetGageDirectory(LPTSTR str, UINT uSize);

CTwoTone::CTwoTone(const string strSysInfo) : CBaseSignal(strSysInfo), m_dAmplitude(0.87), m_dFrequency1(1000000.0),
							m_dFrequency2(1000000.0),m_dDcOffset(0.), m_dPhase(0.)
{
}

CTwoTone::~CTwoTone(void)
{
}

int CTwoTone::ReadParams(void)
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
			m_dFrequency1 = double(nData);

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_FREQUENCY_2, int(m_dFrequency1), strFileName.c_str());
			m_dFrequency2 = double(nData);

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_PHASE, 0, strFileName.c_str());
			if (nData > 359)
				nData = 0;
			m_dPhase = M_PI * double(nData) / 180;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_NOISE, 12, strFileName.c_str());
			m_dNoiseLevel = double(nData) / 1.e3;

			nData = XMLGetInt(m_strSysInfo.c_str(), XML_VAR_OFFSET, 0, strFileName.c_str());
			m_dDcOffset = double(nData) / 1.e3;

			bError = false;
		}
		delete pSystem;
	}

	//Amplitude devided by 4
	// 2 to convert from pk-pk => unipolar
	// and 2 due to two sine waves
	m_dAmplitude /= 4.;

	if (bError)
		return 0;
	else
		return 1;
}


double CTwoTone::GetRawVoltage(double dTime)
{
	return m_dAmplitude * ( sin(2.0 * M_PI * m_dFrequency1 * dTime + m_dPhase) + sin(2.0 * M_PI * m_dFrequency2 * dTime)) + m_dDcOffset;
}
