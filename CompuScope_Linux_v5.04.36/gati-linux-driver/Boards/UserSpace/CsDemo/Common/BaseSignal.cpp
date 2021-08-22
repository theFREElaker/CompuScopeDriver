#include "BaseSignal.h"
#include "GageDemoSystem.h"

const unsigned CBaseSignal::c_uMagicPrime = 10687;
const unsigned CBaseSignal::c_uMagicPrimeShift = 43;

CBaseSignal::CBaseSignal(const string strSysInfo ):m_dSampleRate(100000000.0), m_dNoiseLevel(0.0),
 m_strSysInfo(strSysInfo)
{
	m_pdNoiseArray = new double[c_uMagicPrime];
}


CBaseSignal::~CBaseSignal(void)
{
	delete[] m_pdNoiseArray;
}

void CBaseSignal::CreateNoiseArray(void)
{
	double dHalfRandMax = static_cast<double>(RAND_MAX) / 2.0;

	for (int i = 0; i < int(c_uMagicPrime); i++)
		m_pdNoiseArray[i] = ((rand() - dHalfRandMax) / dHalfRandMax) * m_dNoiseLevel;
}

