#ifndef __TRIGGER_H__
#define __TRIGGER_H__

#pragma once

#include "BaseSignal.h"
#include <CsTypes.h>

#ifdef __linux__
	#include <CWinEventHandle.h>
#endif

class CTrigger
{
public:
	CTrigger(unsigned long uSense = 10);
	~CTrigger(void);
	void SetDelay(int64 i64TriggerDelay) { m_i64TriggerDelay = i64TriggerDelay; };
	void SetHoldoff(int64 i64TriggerHoldoff) { m_i64TriggerHoldoff = i64TriggerHoldoff; };
	void SetCondition(unsigned long u32Slope, unsigned long u32Range, int i32Level)
	{
		m_u32Slope = u32Slope;
		m_u32Range = u32Range;
		m_i32Level = i32Level;
	};
	double FindTrigger(EVENT_HANDLE hAbort, EVENT_HANDLE hForce, double dTimeout);
	void SetChannelSource(CBaseSignal * pSignal)  {m_pSignal = pSignal; };
	void SetExternalSource(bool bExt){ m_bExternalTrigger = bExt;}

private:
	CBaseSignal* m_pSignal;
	int64 m_i64TriggerDelay;
	int64 m_i64TriggerHoldoff;
	unsigned long m_u32Slope;
	unsigned long m_u32Range;
	long m_i32Level;
	bool m_bExternalTrigger;
	unsigned long m_u32Sensitivity;

	static const unsigned c_uCheckInterval;
};

#endif // __TRIGGER_H__
