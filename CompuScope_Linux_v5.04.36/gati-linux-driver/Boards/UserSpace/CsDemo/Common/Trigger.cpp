#include "Trigger.h"

#include <CsDefines.h>
#include <misc.h>

#ifdef __linux__
	#include <stdlib.h>
#endif

const unsigned CTrigger::c_uCheckInterval = 10000;

CTrigger::CTrigger(unsigned long uSense) : m_i64TriggerDelay(0), m_i64TriggerHoldoff(0),
						m_u32Slope(0), m_u32Range(2000), m_i32Level(0),
						m_bExternalTrigger(false), m_u32Sensitivity(uSense)
{
}

CTrigger::~CTrigger(void)
{
}

double CTrigger::FindTrigger(EVENT_HANDLE hAbort, EVENT_HANDLE hForce, double dTimeout)
{
	if(m_bExternalTrigger)
		return (0.0);

	double dTime = double(GetTickCount()%100000) / 1000.; // Convert to seconds
	dTime += ((rand() - static_cast<double>(RAND_MAX) / 2.0) / static_cast<double>(RAND_MAX) / 2.0);

	double dSampleRate = NULL != m_pSignal ? m_pSignal->GetSampleRate(): 1.e6;

	double dTbs = 1.0 / dSampleRate;

	// Convert trigger level into a voltage
	double dHoldoffTime = double(m_i64TriggerHoldoff)* dTbs;
	dTime += dHoldoffTime;

	double dTriggerLevel = (double(m_i32Level) / 100.0) * double(m_u32Range) / 2000.;
	double dQualifier =  dTriggerLevel - double(m_u32Range) / 2000. * double(m_u32Sensitivity)/100.;

	double dMult = 1.0;

	if (CS_TRIG_COND_NEG_SLOPE == m_u32Slope)
	{
		dMult = -1.0;
	}

	int nCheckCounter(0);

	EVENT_HANDLE aHandles[2] = { hAbort, hForce };

	double dVoltage;

	double dStartTime(dTime);

	// look for a value less then the trigger level;
	do
	{
		if(NULL != m_pSignal  )
			dVoltage = dMult * m_pSignal->GetVoltage(dTime);
		else
			dVoltage = 0.0;

		dTime += dTbs;
		nCheckCounter++;

		if ( nCheckCounter > int(c_uCheckInterval) )
			nCheckCounter = 0;

		if( 0 == nCheckCounter )
		{
			if( WAIT_TIMEOUT != ::WaitForMultipleObjects(2, aHandles, FALSE, 0 ) )
				return dTime;
		}

		if ( dTime - dStartTime > dTimeout )
			return dTime;

		// Just read and throw away until we've cycled thru the data
		// and we're below the trigger level
		// We need a way to break out of this !!!!
	}
	while (dVoltage > dQualifier);

	// If we're here then the values read are less than the trigger level
	// and we can check to see we meet our trigger condition
	for(;;)
	{
		if(NULL != m_pSignal  )
			dVoltage = dMult * m_pSignal->GetVoltage(dTime);
		else
			dVoltage = 0.0;

		nCheckCounter++;

		if ( nCheckCounter > int(c_uCheckInterval) )
			nCheckCounter = 0;

		if( 0 == nCheckCounter )
		{
			if( WAIT_TIMEOUT != ::WaitForMultipleObjects(2, aHandles, FALSE, 0 ) )
				return dTime;
		}

		if (dVoltage > dTriggerLevel)
			break;

		dTime += dTbs;
		if ( dTime - dStartTime > dTimeout )
			return dTime;
	}

//	Take out line below for now, it causes the delay to be wrong.
//	dTime += double(m_i64TriggerDelay) * dTbs;
	return dTime;
}

