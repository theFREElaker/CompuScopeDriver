#if defined WIN32 || defined CS_WIN64

	#include "stdafx.h"
	#include <process.h>
	#ifndef CS_WIN64
		#include "debug.h"
	#endif

#elif defined _linux

	#include <cstdio>
	#include <stdint.h>

#endif

#include "GageDemoSystem.h"
#include "SineWave.h"
#include "SquareWave.h"
#include "TriangleWave.h"
#include "TwoTone.h"
#include "Trigger.h"
#include <algorithm>
#include "Gage_XML_Defines.h"

//RG #include "HiddenWindow.h" for use with signal generator

#if defined WIN32 || defined CS_WIN64
	#define MaxInt64 9223372036854775807i64 //RG MOVE INTO CsLinuxPorts or CsTypes
#elif defined __linux__
	#define MaxInt64 9223372036854775807LLU
#endif

/*
#ifdef _WINDOWS
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_COPYDATA:
			
			break;

		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}
#endif
*/

bool SRgreater ( CSSAMPLERATETABLE_EX elem1, CSSAMPLERATETABLE_EX elem2 )
{
	return elem1.PublicPart.i64SampleRate > elem2.PublicPart.i64SampleRate;
}

bool IRgreater ( CSRANGETABLE elem1, CSRANGETABLE elem2 )
{
	return elem1.u32InputRange > elem2.u32InputRange;
}

const int64 CGageDemoSystem::c_i64MemSize = 33554432; // 32 Megasamples
const uInt32 CGageDemoSystem::c_u32MaxSegmentCount = 256;


#if defined (_WIN32) || defined (CS_WIN64)
	unsigned __stdcall CGageDemoSystem::CaptureThread(void *arg)
#elif defined __linux__
	void* CGageDemoSystem::CaptureThread(void *arg)
#endif
{
	CGageDemoSystem* pSystem = (CGageDemoSystem*)arg;

	EVENT_HANDLE ahHandles[3] = {pSystem->GetTerminateThreadEvent(),
		                   pSystem->GetPerformAcquisitionEvent(),
	                       pSystem->GetAbortAcquisitionEvent()};

	for(;;)
	{
		DWORD dwRet = ::WaitForMultipleObjects(3, ahHandles, FALSE, INFINITE);
		if( WAIT_OBJECT_0 == dwRet )
			break;

		if( WAIT_OBJECT_0 + 2 == dwRet )
		{
			if ( NULL != pSystem->GetAcqTriggeredEvent())
			{
				SetEvent(pSystem->GetAcqTriggeredEvent());
			}
			SetEvent(pSystem->GetInternalAcqEndBusyEvent());

			if ( NULL != pSystem->GetAcqEndBusyEvent() )
			{
				SetEvent(pSystem->GetAcqEndBusyEvent());
			}

			ResetEvent( pSystem->GetAbortAcquisitionEvent() );
			continue;
		}
		ResetEvent( pSystem->GetInternalAcqEndBusyEvent());
		ResetEvent( pSystem->GetAbortAcquisitionEvent() );

		pSystem->CreateNoise();
		// Simulate waiting for the capture, i.e number of points divided by sample rate
		double dTimeToWait = static_cast<double>(pSystem->GetTriggerDepth()) / static_cast<double>(pSystem->GetSampleRate());
		dTimeToWait *= 1000.0; // Convert to milliseconds
		DWORD dwTimeForSegment = static_cast<DWORD>(dTimeToWait);

        unsigned uSegNum = pSystem->GetSegmentCount();

		::ResetEvent( pSystem->GetForceTriggerEvent() );

		for( unsigned u = 0; u < uSegNum; u++ )
		{
			double dTriggerTime = 0.0;
			CTrigger* pTrigger = pSystem->GetTrigger();
			if(NULL != pTrigger )
			{
				dTriggerTime = pTrigger->FindTrigger(pSystem->GetAbortAcquisitionEvent(), pSystem->GetForceTriggerEvent(), double(pSystem->GetTriggerTimeout())/1e7);
			}
			if ( WAIT_OBJECT_0 == ::WaitForSingleObject( pSystem->m_hTerminateThreadEvent, 0 ) )
			{
				SetEvent(pSystem->GetAcqEndBusyEvent());
				break;
			}

			pSystem->SetTriggerTime(dTriggerTime, u);
			if ( NULL != pSystem->GetAcqTriggeredEvent() && u == 0 )
			{
				SetEvent(pSystem->GetAcqTriggeredEvent());
			}
			::Sleep(dwTimeForSegment);//RG
		}
		if ( NULL != pSystem->GetAcqEndBusyEvent() )
		{
			SetEvent(pSystem->GetAcqEndBusyEvent());
		}
		SetEvent(pSystem->GetInternalAcqEndBusyEvent());
		ResetEvent(pSystem->GetPerformAcquisitionEvent());//RG - FIND OUT WHY

	}
	// It doesn't matter if the call fails, once we exit the thread
	// routine we'll try it again so we'll catch the failure there.

#if defined (_WIN32) || defined (CS_WIN64)
	::_endthreadex(0);
	return 0;
#elif defined __linux__
	pthread_exit((void*)NULL);
#endif

}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#ifdef _WINDOWS // for now

unsigned __stdcall CGageDemoSystem::AsynchronousTransferThread(void *arg)
{
	// Copy the variables before they go out of scope
	ThreadTransferStruct* ts = static_cast<ThreadTransferStruct*>(arg);
	CGageDemoSystem* pSystem = static_cast<CGageDemoSystem*>(ts->pSystem);
	CBaseSignal* pSignal = pSystem->m_vecSignals.at(ts->InParams.u16Channel - 1);

	::WaitForSingleObject(pSystem->GetTransferSemaphore(), INFINITE);

	pSignal->SeedNoiseIndex(ts->InParams.u32Segment-1);
	double dTime = pSystem->GetTriggerTime(ts->InParams.u32Segment-1);
	double dTbs = 1.0 / double(pSystem->GetSampleRate());

	dTime += double(ts->InParams.i64StartAddress) * dTbs;

	double dSampleRes = double(pSystem->m_csSystemConfig.pAcqCfg->i32SampleRes);
	double dOff = double(pSystem->m_csSystemConfig.pAChannels->aChannel[ts->InParams.u16Channel - 1].i32DcOffset) / 1.e3;
	double dInputRange = double(pSystem->m_csSystemConfig.pAChannels->aChannel[ts->InParams.u16Channel - 1].u32InputRange) / 2.e3;

	double dImpedanceFactor = 2. * double(pSystem->m_csSystemConfig.pAChannels->aChannel[ts->InParams.u16Channel - 1].u32Impedance) / (50. + pSystem->m_csSystemConfig.pAChannels->aChannel[ts->InParams.u16Channel - 1].u32Impedance);
	double dLow = dOff - dInputRange;
	double dHigh = dOff + dInputRange;

	double dScaleAdjust = (dSampleRes > 0) ? 1 : -1;
	double dScale = (dSampleRes-dScaleAdjust)/dInputRange;

	pSystem->m_csAsyncTransferResult.i32Status = CS_SUCCESS;
	pSystem->m_csAsyncTransferResult.i64BytesTransfered = 0;

	if (CS_SAMPLE_BITS_8 == pSystem->m_csSystemInfo.u32SampleBits)
	{
		int8 i8SampleOff = int8(pSystem->m_csSystemConfig.pAcqCfg->i32SampleOffset);

		uInt8* pBuffer = static_cast<uInt8*>(ts->InParams.pDataBuffer);
		for (int64 i = 0; i < ts->InParams.i64Length; i++)
		{
			if(WAIT_OBJECT_0 == ::WaitForSingleObject(pSystem->GetAbortTransferEvent(), 0 ) )
				break;

			double dVoltage = dImpedanceFactor * pSignal->GetVoltage(dTime);
			dTime += dTbs;

			dVoltage = min ( dHigh, max(dLow, dVoltage));
			dVoltage -= dOff;

			int32 i32Value = int32(i8SampleOff - dVoltage * dScale);

			if ( 0 > i32Value)
				i32Value = 0;
			if (UCHAR_MAX < i32Value)
				i32Value = UCHAR_MAX;
			pBuffer[i] = uInt8(i32Value);
			pSystem->m_csAsyncTransferResult.i64BytesTransfered++;
		}
	}
	else
	{
		int16 i16SampleOff = int16(pSystem->m_csSystemConfig.pAcqCfg->i32SampleOffset);

		int16* pBuffer = static_cast<int16*>(ts->InParams.pDataBuffer);
		uInt32 u32Mask;
		if (CS_SAMPLE_BITS_12 == pSystem->m_csSystemInfo.u32SampleBits)
			u32Mask = 0xFFFFFFF0;
		else if (CS_SAMPLE_BITS_14 == pSystem->m_csSystemInfo.u32SampleBits)
			u32Mask = 0xFFFFFFFC;
		else // 16 bit
			u32Mask = 0xFFFFFFFF;

		for (int64 i = 0; i < ts->InParams.i64Length; i++)
		{
			if(WAIT_OBJECT_0 == ::WaitForSingleObject(pSystem->GetAbortTransferEvent(), 0 ) )
				break;

			double dVoltage = dImpedanceFactor * pSignal->GetVoltage(dTime);
			dTime += dTbs;
			dVoltage = min ( dHigh, max(dLow, dVoltage));
			dVoltage -= dOff;
			int32 i32Value = int32(i16SampleOff) - int16(dVoltage * dScale);
			if (SHRT_MIN > i32Value)
				i32Value = SHRT_MIN;
			if (SHRT_MAX < i32Value)
				i32Value = SHRT_MAX;

			pBuffer[i] = int16(i32Value);
			pBuffer[i] &= u32Mask;
			pSystem->m_csAsyncTransferResult.i64BytesTransfered += pSystem->m_csSystemInfo.u32SampleSize;
		}
	}
	::SetEvent(*ts->InParams.hNotifyEvent);
	::ReleaseSemaphore(pSystem->GetTransferSemaphore(), 1, NULL);
	::GlobalFree(ts);
	::_endthreadex(0);
	return 0;
}
#endif // _WINDOWS
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CGageDemoSystem::CGageDemoSystem(DRVHANDLE csHandle, CGageDemo* pGageDemo, PCSSYSTEMINFO pSystemInfo, PCSBOARDINFO pBoardInfo,
								 PCSSYSTEMINFOEX pSysInfoEx, PCS_FW_VER_INFO pVerInfo, const ChannelInfo& ChanInfo,
								 const SystemCapsInfo& sysCaps, const string& strSysName)
	: m_csHandle(csHandle),
	  m_hAcqTriggeredEvent(NULL), m_hAcqEndBusyEvent(NULL),
	  m_hIntAcqEndBusyEvent(NULL),
	  m_pGageDemo(pGageDemo),
	  m_CapsInfo(sysCaps),
	  m_bManualResetTimeStamp(false)
{
	m_csAsyncTransferResult.i32Status = 0;
	m_csAsyncTransferResult.i64BytesTransfered = 0;
	m_u32SampleBits = pSystemInfo->u32SampleBits;
	CopyMemory(&m_csSystemInfo, pSystemInfo, pSystemInfo->u32Size);
	CopyMemory(&m_csBoardInfo, pBoardInfo, pBoardInfo->u32Size);
	CopyMemory(&m_csSysInfoEx, pSysInfoEx, pSysInfoEx->u32Size);
	CopyMemory(&m_csVerInfo, pVerInfo, pVerInfo->u32Size);
	m_strSysName.assign(strSysName);

	assert(m_csSystemInfo.u32ChannelCount == ChanInfo.size() );
	m_csSystemInfo.u32ChannelCount = uInt32(ChanInfo.size());
	CBaseSignal* pBs(NULL);
	string strChanInfo;

	switch ( m_csSystemInfo.u32SampleBits )
	{
		case CS_SAMPLE_BITS_8 : strChanInfo = XML_8_BIT_NODE;  break;
		case CS_SAMPLE_BITS_12: strChanInfo = XML_12_BIT_NODE; break;
		default:
		case CS_SAMPLE_BITS_14: strChanInfo = XML_14_BIT_NODE; break;
		case CS_SAMPLE_BITS_16: strChanInfo = XML_16_BIT_NODE; break;
	}
	strChanInfo.append(_T("\\"));
	strChanInfo.append(strSysName);
	strChanInfo.append(_T("\\"));

	for (uInt32 i = 0; i < m_csSystemInfo.u32ChannelCount; i++)
	{
		TCHAR tcChanName[25];
		string str = strChanInfo;
		_stprintf_s(tcChanName, _countof(tcChanName), _T("Chan%02lu"), i+1 );
		str.append(tcChanName);

		switch( ChanInfo[i] )
		{
			default:
			case echSine:     pBs = new CSineWave(str); break;
			case echSquare:   pBs = new CSquareWave(str); break;
			case echTriangle: pBs = new CTriangleWave(str); break;
			case echTwoTone:  pBs = new CTwoTone(str); break;
		}
		pBs->ReadParams();
		m_vecSignals.push_back(pBs);
	}

	sort(m_CapsInfo.veSampleRate.begin(), m_CapsInfo.veSampleRate.end(), SRgreater );
	sort(m_CapsInfo.veInputRange.begin(), m_CapsInfo.veInputRange.end(), IRgreater );

	m_pTrigger = new CTrigger(m_CapsInfo.uTriggerSense);
	InitHandles();

/*
	HANDLE hThread = CreateWindowThread(this); for use with signal generator

	WNDCLASS wc;
	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = 0;
	wc.hIcon = 0;
	wc.hCursor = 0;
	wc.hbrBackground = 0;
	wc.lpszMenuName =0;
	wc.lpszClassName = "MessageOnly";
	BOOL b = ::RegisterClass(&wc);
	hWnd = ::CreateWindowEx(WS_EX_LEFT, "MessageOnly", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0);
*/	
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CGageDemoSystem::InitHandles(void)
{
	srand( GetTickCount() );

	m_hAbortAcquisitionEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hAbortTransferEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hTerminateThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hTransferSemaphore = ::CreateSemaphore(NULL, 1, 1, NULL);

	// Set trigger event to automatically be reset
	m_hForceTriggerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hPerformAcquisitionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hIntAcqEndBusyEvent = CreateEvent( NULL, TRUE, TRUE, NULL);
	#if defined (_WIN32) || defined (CS_WIN64)
		m_hCaptureThreadHandle = reinterpret_cast<HANDLE>(::_beginthreadex(NULL, 0, CaptureThread, (void *)this, 0, NULL));
	#elif defined __linux__
		int res;
		res = pthread_create(&m_hCaptureThreadHandle, NULL, CaptureThread, (void*)this);

		if (0 != res)
			perror("Thread (CaptureThread) Creation Failed");
	#endif
}
//-----------------------------------------------------------------------------
CGageDemoSystem::~CGageDemoSystem(void)
{
	SetEvent(m_hTerminateThreadEvent);

	// Wait for the thread to end. Because there's an issue with LabVIEW and
	// Matlab and waiting for the thread to end, we're only waiting for 100 ms.
	// The issue is that sometimes the CGageDemoSystem destructor finishes before
	// the thread ends. Since the thread is using some of it's members it hangs and
	// never ends. So, if the thread doesn't end in time we'll continue on. Windows
	// will clean up after us when the application (i.e. LabVIEW of MatLab) ends.

#if defined (_WIN32) || defined (CS_WIN64)
	WaitForSingleObject(m_hCaptureThreadHandle, 100);
#elif defined __linux__
/*
	void*	thread_result;
	pthread_join(m_hCaptureThreadHandle, &thread_result);//TODO: NEED TO IMPLEMENT A TIMED pthread_join
*/
#endif

	if (NULL != m_csSystemConfig.pAcqCfg)
	{
		VirtualFree(m_csSystemConfig.pAcqCfg, 0, MEM_RELEASE);
		m_csSystemConfig.pAcqCfg = NULL;
	}
	if (NULL != m_csSystemConfig.pAChannels)
	{
		VirtualFree(m_csSystemConfig.pAChannels, 0, MEM_RELEASE);
		m_csSystemConfig.pAChannels = NULL;
	}
	if (NULL != m_csSystemConfig.pATriggers)
	{
		VirtualFree(m_csSystemConfig.pATriggers, 0, MEM_RELEASE);
		m_csSystemConfig.pATriggers = NULL;
	}

	DEMOSIGNALVECTOR::iterator theIterator;
	for (theIterator = m_vecSignals.begin(); theIterator  != m_vecSignals.end(); theIterator++)
	{
		CBaseSignal* pBs = (*theIterator);
		delete pBs;
	}

	m_vecSignals.clear();

	m_veTriggerTime.clear();

	delete m_pTrigger;
	m_pTrigger = NULL;

	CloseHandle(m_hAbortAcquisitionEvent);
	CloseHandle(m_hAbortTransferEvent);

#ifdef __linux__
	DestroySemaphore(m_hTransferSemaphore);//RG
#else
	CloseHandle(m_hTransferSemaphore);
#endif

	CloseHandle(m_hForceTriggerEvent);
	CloseHandle(m_hTerminateThreadEvent);
	CloseHandle(m_hPerformAcquisitionEvent);

#if defined (_WIN32) || defined (CS_WIN64)
	CloseHandle(m_hCaptureThreadHandle);
#endif
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CGageDemoSystem::SetAcquisitionDefaults(void)
{
	m_csSystemConfig.pAcqCfg->u32Size = sizeof(CSACQUISITIONCONFIG);

	m_csSystemConfig.pAcqCfg->u32SampleBits = m_csSystemInfo.u32SampleBits;
	m_csSystemConfig.pAcqCfg->u32SampleSize = m_csSystemInfo.u32SampleSize;
	// Since we're emulating Combine boards by default, the raw data uses
	// the leftmost bits. 8 bit samples (as well as 16 bits) aren't affected
	// by this.
	if (CS_SAMPLE_BITS_8 == m_csSystemInfo.u32SampleBits)
	{
        m_csSystemConfig.pAcqCfg->i32SampleRes = m_csSystemInfo.i32SampleResolution;
		m_csSystemConfig.pAcqCfg->i32SampleOffset = m_csSystemInfo.i32SampleOffset;
	}
	else
	{
		m_csSystemConfig.pAcqCfg->i32SampleRes = -CS_SAMPLE_RES_16;
		m_csSystemConfig.pAcqCfg->i32SampleOffset = -(1 << (16 - m_csSystemInfo.u32SampleBits));
	}
	m_csSystemConfig.pAcqCfg->i64TriggerDelay = 0;
	m_csSystemConfig.pAcqCfg->i64TriggerHoldoff = 0;
	m_csSystemConfig.pAcqCfg->i64TriggerTimeout = static_cast<int>(CS_TIMEOUT_DISABLE);
	m_csSystemConfig.pAcqCfg->u32ExtClk = 0;
	m_csSystemConfig.pAcqCfg->u32ExtClkSampleSkip = 0;

	m_csSystemConfig.pAcqCfg->u32SegmentCount = 1;
	m_csSystemConfig.pAcqCfg->u32TimeStampConfig = 0;
	m_csSystemConfig.pAcqCfg->u32TrigEnginesEn = 1;

#ifdef _WINDOWS	
	m_csSystemConfig.pAcqCfg->i64Depth = min (m_csSystemInfo.i64MaxMemory / m_csSystemInfo.u32ChannelCount, 8192);
#else
	//TODO: We have to cast the division or MIN doesn't work properly
	m_csSystemConfig.pAcqCfg->i64Depth = MIN((long)(m_csSystemInfo.i64MaxMemory / m_csSystemInfo.u32ChannelCount), 8192);
#endif

	m_csSystemConfig.pAcqCfg->i64SegmentSize = m_csSystemConfig.pAcqCfg->i64Depth;

	// This should be the highest mode the system supports
	uInt32 u32Mode = m_csSystemInfo.u32ChannelCount / m_csSystemInfo.u32BoardCount;
	// add channels to the demo but forget to increase the number of boards accordingly
	if ( u32Mode > CS_MODE_OCT )
	{
		u32Mode = CS_MODE_OCT;
	}

	AdjustAcqModesToAvailable(&u32Mode);
	uInt32 u32Mask = CS_MODE_OCT;

	while( 0 != u32Mask )
	{
		if(0 != (u32Mode & u32Mask ) )
			break;

		u32Mask >>= 1;
	}

	if( 0 == u32Mask )
	{
		assert( !_T("No mode available") );
		u32Mask = CS_MODE_SINGLE;
	}


	m_csSystemConfig.pAcqCfg->u32Mode = u32Mask;

	int nSampleRateCount = static_cast<int>(m_CapsInfo.veSampleRate.size());
	// Find the highest sample rate that supports the highest mode the system has
	// The sample rate table goes from highest to lowest
	for (int i = 0; i < nSampleRateCount; i++)
	{
		if (u32Mask & m_CapsInfo.veSampleRate[i].u32Mode)
		{
			m_csSystemConfig.pAcqCfg->i64SampleRate = m_CapsInfo.veSampleRate[i].PublicPart.i64SampleRate;
			break;
		}
	}
	m_veTriggerTime.resize(m_csSystemConfig.pAcqCfg->u32SegmentCount, 0.);
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CGageDemoSystem::SetChannelDefaults(void)
{
	for (uInt32 i = 0; i < m_csSystemConfig.pAChannels->u32ChannelCount; i++)
	{
		m_csSystemConfig.pAChannels->aChannel[i].u32ChannelIndex = i + 1;
		m_csSystemConfig.pAChannels->aChannel[i].u32Size = sizeof(CSCHANNELCONFIG);
		m_csSystemConfig.pAChannels->aChannel[i].i32Calib = 0;
		m_csSystemConfig.pAChannels->aChannel[i].i32DcOffset = 0;
		m_csSystemConfig.pAChannels->aChannel[i].u32Filter = 0;
		m_csSystemConfig.pAChannels->aChannel[i].u32Impedance = CS_REAL_IMP_50_OHM;
		m_csSystemConfig.pAChannels->aChannel[i].u32InputRange = CS_GAIN_2_V;
		m_csSystemConfig.pAChannels->aChannel[i].u32Term = CS_COUPLING_DC;
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CGageDemoSystem::SetTriggerEngineDefaults(void)
{
	for (uInt32 i = 0; i < m_csSystemConfig.pATriggers->u32TriggerCount; i++)
	{
		m_csSystemConfig.pATriggers->aTrigger[i].u32TriggerIndex = i + 1;
		m_csSystemConfig.pATriggers->aTrigger[i].u32Size = sizeof(CSTRIGGERCONFIG);
		m_csSystemConfig.pATriggers->aTrigger[i].u32Condition = CS_TRIG_COND_POS_SLOPE;
		m_csSystemConfig.pATriggers->aTrigger[i].i32Level = 0;
		m_csSystemConfig.pATriggers->aTrigger[i].i32Source = CS_TRIG_SOURCE_CHAN_1;
		m_csSystemConfig.pATriggers->aTrigger[i].u32ExtCoupling = CS_COUPLING_DC;
		m_csSystemConfig.pATriggers->aTrigger[i].u32ExtTriggerRange = CS_GAIN_2_V;
		m_csSystemConfig.pATriggers->aTrigger[i].u32ExtImpedance = CS_REAL_IMP_1M_OHM;
		m_csSystemConfig.pATriggers->aTrigger[i].i32Value1 = 0;
		m_csSystemConfig.pATriggers->aTrigger[i].i32Value2 = 0;
		m_csSystemConfig.pATriggers->aTrigger[i].u32Filter = 0;
		m_csSystemConfig.pATriggers->aTrigger[i].u32Relation = 0;
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int64 CGageDemoSystem::GetMaxLength(void)
{
	return m_csSystemInfo.i64MaxMemory / m_csSystemConfig.pAcqCfg->u32Mode;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int64 CGageDemoSystem::GetTriggerHoldoff(void)
{
	return NULL == m_csSystemConfig.pAcqCfg ? 0 : m_csSystemConfig.pAcqCfg->i64TriggerHoldoff;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int64 CGageDemoSystem::GetTriggerTimeout(void)
{
	return NULL == m_csSystemConfig.pAcqCfg ? 0 : m_csSystemConfig.pAcqCfg->i64TriggerTimeout<0 ? MaxInt64 : m_csSystemConfig.pAcqCfg->i64TriggerTimeout;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int64 CGageDemoSystem::GetTriggerDelay(void)
{
	return NULL == m_csSystemConfig.pAcqCfg ? 0 : m_csSystemConfig.pAcqCfg->i64TriggerDelay;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int CGageDemoSystem::GetTriggerLevel(void)
{
	return m_csSystemConfig.pATriggers->aTrigger->i32Level;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CGageDemoSystem::GetTriggerSlope(void)
{
	return m_csSystemConfig.pATriggers->aTrigger->u32Condition;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CGageDemoSystem::GetTriggerRange(void)
{
	int32 i32Source = m_csSystemConfig.pATriggers->aTrigger->i32Source;
	if (CS_TRIG_SOURCE_CHAN_1 == i32Source)
		return m_csSystemConfig.pAChannels->aChannel[0].u32InputRange;
	else if (CS_TRIG_SOURCE_CHAN_2 == i32Source)
		return m_csSystemConfig.pAChannels->aChannel[1].u32InputRange;
	else if (CS_TRIG_SOURCE_EXT == i32Source)
		return m_csSystemConfig.pATriggers->aTrigger->u32ExtTriggerRange;
	else // if source disabled
		return m_csSystemConfig.pAChannels->aChannel[0].u32InputRange;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CGageDemoSystem::GetExternalSource(void)
{
	if (CS_TRIG_SOURCE_EXT == m_csSystemConfig.pATriggers->aTrigger->i32Source)
		return true;

	return false;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CBaseSignal* CGageDemoSystem::GetChannelSource(void)
{
	int32 i32TriggerSource = m_csSystemConfig.pATriggers->aTrigger->i32Source;

	if (CS_TRIG_SOURCE_EXT == i32TriggerSource || CS_TRIG_SOURCE_DISABLE == i32TriggerSource )
		return NULL;

	i32TriggerSource--;

	if (m_vecSignals.size() >  size_t(i32TriggerSource))
	{
		return m_vecSignals.at( i32TriggerSource );
	}
	else
	{
		return NULL;
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CGageDemoSystem::CreateNoise(void)
{
	DEMOSIGNALVECTOR::iterator theIterator;
	for (theIterator = m_vecSignals.begin(); theIterator  != m_vecSignals.end(); theIterator++)
	{
		CBaseSignal* pBs = (*theIterator);
		pBs->CreateNoiseArray();
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::StartAcquisition(void)
{
	SetEvent(m_hPerformAcquisitionEvent);
	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::ForceTrigger(void)
{
	SetEvent(m_hForceTriggerEvent);
	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::Abort(void)
{
	SetEvent(m_hAbortAcquisitionEvent);
	SetEvent(m_hAbortTransferEvent);

#ifdef _WINDOWS
	HANDLE hd[2] = {m_hTransferSemaphore, m_hIntAcqEndBusyEvent};
	::WaitForMultipleObjects(2, hd, TRUE, INFINITE);
#else
	sem_wait(m_hTransferSemaphore); //TODO: RG: or try sem_trywait
	WaitForSingleObject(m_hIntAcqEndBusyEvent, INFINITE );
#endif
	::ReleaseSemaphore(m_hTransferSemaphore, 1, NULL);

	ResetEvent(m_hAbortTransferEvent);
	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::GetAcquisitionParams(PCSACQUISITIONCONFIG pAcqConfig)
{
	if( ::IsBadWritePtr( pAcqConfig,sizeof(CSACQUISITIONCONFIG) ) )
		return CS_BUFFER_TOO_SMALL;

	if (pAcqConfig->u32Size > sizeof(CSACQUISITIONCONFIG))
	{
		ZeroMemory(pAcqConfig, pAcqConfig->u32Size);
		return CS_INVALID_STRUCT_SIZE;
	}
	if (0 == pAcqConfig->u32Size)
	{
		ZeroMemory(pAcqConfig, pAcqConfig->u32Size);
		return CS_INVALID_STRUCT_SIZE;
	}
	CopyMemory(pAcqConfig, m_csSystemConfig.pAcqCfg, pAcqConfig->u32Size);

	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::SetAcquisitionParams(PCSACQUISITIONCONFIG pAcqConfig)
{
	if( ::IsBadWritePtr( pAcqConfig,sizeof(CSACQUISITIONCONFIG) ) )
		return CS_BUFFER_TOO_SMALL;

	if (pAcqConfig->u32Size > sizeof(CSACQUISITIONCONFIG))
		return CS_INVALID_STRUCT_SIZE;

	if (0 == pAcqConfig->u32Size)
		return CS_INVALID_STRUCT_SIZE;

	CopyMemory(m_csSystemConfig.pAcqCfg, pAcqConfig, pAcqConfig->u32Size);

	m_veTriggerTime.resize(m_csSystemConfig.pAcqCfg->u32SegmentCount, 0.);
	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::GetChannelParams(PCSCHANNELCONFIG pChanConfig)
{
	if( ::IsBadWritePtr( pChanConfig, sizeof(CSCHANNELCONFIG) ) )
		return CS_BUFFER_TOO_SMALL;

	if (pChanConfig->u32Size > sizeof(CSCHANNELCONFIG))
	{
		ZeroMemory(pChanConfig, pChanConfig->u32Size);
		return CS_INVALID_STRUCT_SIZE;
	}
	if (0 == pChanConfig->u32Size)
	{
		ZeroMemory(pChanConfig, pChanConfig->u32Size);
		return CS_INVALID_STRUCT_SIZE;
	}
	if ((1 > pChanConfig->u32ChannelIndex) || (pChanConfig->u32ChannelIndex > m_csSystemConfig.pAChannels->u32ChannelCount))
	{
		ZeroMemory(pChanConfig, pChanConfig->u32Size);
		return CS_INVALID_CHANNEL;
	}
	uInt32 u32Index = pChanConfig->u32ChannelIndex - 1;
	CopyMemory(pChanConfig, &m_csSystemConfig.pAChannels->aChannel[u32Index], pChanConfig->u32Size);
	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::SetChannelParams(PCSCHANNELCONFIG pChanConfig)
{
	if( ::IsBadWritePtr( pChanConfig, sizeof(CSCHANNELCONFIG) ) )
		return CS_BUFFER_TOO_SMALL;

	if (pChanConfig->u32Size > sizeof(CSCHANNELCONFIG))
		return CS_INVALID_STRUCT_SIZE;

	if (0 == pChanConfig->u32Size)
		return CS_INVALID_STRUCT_SIZE;

	if ((1 > pChanConfig->u32ChannelIndex) || (pChanConfig->u32ChannelIndex > m_csSystemConfig.pAChannels->u32ChannelCount))
		return CS_INVALID_CHANNEL;

	uInt32 u32Index = pChanConfig->u32ChannelIndex - 1;
	CopyMemory(&m_csSystemConfig.pAChannels->aChannel[u32Index], pChanConfig, sizeof(CSCHANNELCONFIG));
	m_csSystemConfig.pAChannels->aChannel[u32Index].u32Size = sizeof(CSCHANNELCONFIG);

	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::GetTriggerParams(PCSTRIGGERCONFIG pTrigConfig)
{
	if( ::IsBadWritePtr( pTrigConfig, sizeof(CSTRIGGERCONFIG) ) )
		return CS_BUFFER_TOO_SMALL;


	if (pTrigConfig->u32Size > sizeof(CSTRIGGERCONFIG))
	{
		ZeroMemory(pTrigConfig, pTrigConfig->u32Size);
		return CS_INVALID_STRUCT_SIZE;
	}
	if (0 == pTrigConfig->u32Size)
	{
		ZeroMemory(pTrigConfig, pTrigConfig->u32Size);
		return CS_INVALID_STRUCT_SIZE;
	}
	if ((1 > pTrigConfig->u32TriggerIndex) || (pTrigConfig->u32TriggerIndex > m_csSystemConfig.pATriggers->u32TriggerCount))
	{
		ZeroMemory(pTrigConfig, pTrigConfig->u32Size);
		return CS_INVALID_TRIGGER;
	}
	uInt32 u32Index = pTrigConfig->u32TriggerIndex - 1;
	CopyMemory(pTrigConfig, &m_csSystemConfig.pATriggers->aTrigger[u32Index], sizeof(CSTRIGGERCONFIG));
	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::SetTriggerParams(PCSTRIGGERCONFIG pTrigConfig)
{
	if( ::IsBadWritePtr( pTrigConfig, sizeof(CSTRIGGERCONFIG) ) )
		return CS_BUFFER_TOO_SMALL;

	if (pTrigConfig->u32Size > sizeof(CSTRIGGERCONFIG))
		return CS_INVALID_STRUCT_SIZE;

	if (0 == pTrigConfig->u32Size)
		return CS_INVALID_STRUCT_SIZE;

	if ((1 > pTrigConfig->u32TriggerIndex) || (pTrigConfig->u32TriggerIndex > m_csSystemConfig.pATriggers->u32TriggerCount))
		return CS_INVALID_TRIGGER;

	uInt32 u32Index = pTrigConfig->u32TriggerIndex - 1;
	CopyMemory(&m_csSystemConfig.pATriggers->aTrigger[u32Index], pTrigConfig, sizeof(CSTRIGGERCONFIG));
	m_csSystemConfig.pATriggers->aTrigger[u32Index].u32Size = sizeof(CSTRIGGERCONFIG);

	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::GetAvailableSampleRates(uInt32, PCSSYSTEMCONFIG pSystemConfig,
												PCSSAMPLERATETABLE pSRTable, uInt32 *u32BufferSize)
{
	if (NULL == u32BufferSize)
		return (CS_INVALID_PARAMETER);

	if (NULL == pSystemConfig)
	{
		if (NULL == pSRTable)
		{
			*u32BufferSize = uInt32(m_CapsInfo.veSampleRate.size() * sizeof(CSSAMPLERATETABLE));
			return CS_SUCCESS;
		}
		else
		{
			if ( *u32BufferSize < m_CapsInfo.veSampleRate.size() * sizeof(CSSAMPLERATETABLE) )
				return CS_BUFFER_TOO_SMALL;
			else
			{
				for (size_t i = 0; i < m_CapsInfo.veSampleRate.size(); i ++ )
				{
					pSRTable[i] = m_CapsInfo.veSampleRate[i].PublicPart;
				}
				return CS_SUCCESS;
			}
		}
	}
	else
	{
		// Validate pSystemConfig for write operation
		int32	Status = ValidateBufferForReadWrite(CS_SYSTEM, pSystemConfig, TRUE);
		if ( Status != CS_SUCCESS )
			return Status;

		PCSACQUISITIONCONFIG pAcqCfg = pSystemConfig->pAcqCfg;

		uInt32 u32NumOfElement = 0;
		// Calculate number of element in SR table depending on card
		for (size_t i = 0; i < m_CapsInfo.veSampleRate.size(); i ++ )
		{
			if (0 != (m_CapsInfo.veSampleRate[i].u32Mode & pAcqCfg->u32Mode) )
				u32NumOfElement++;
		}

		if (pSRTable == NULL)
		{
			*u32BufferSize = u32NumOfElement * sizeof(CSSAMPLERATETABLE);
			return u32NumOfElement;
		}
		else
		{
			if (*u32BufferSize < u32NumOfElement * sizeof(CSSAMPLERATETABLE))
				return CS_BUFFER_TOO_SMALL;

			for (size_t i = 0, j = 0; i < m_CapsInfo.veSampleRate.size() && j < u32NumOfElement; i ++ )
			{
				if (0 != (m_CapsInfo.veSampleRate[i].u32Mode & pAcqCfg->u32Mode) )
				{
					pSRTable[j] = m_CapsInfo.veSampleRate[i].PublicPart;
					j++;
				}
			}
			return u32NumOfElement;
		}
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::GetAvailableInputRanges(uInt32 u32SubCapsId, PCSSYSTEMCONFIG pSystemConfig,
											   PCSRANGETABLE pRangesTbl, uInt32 *u32BufferSize)
{
	uInt32 u32NumOfElement = uInt32(m_CapsInfo.veInputRange.size());

	if (NULL == u32BufferSize)
		return (CS_INVALID_PARAMETER);

	if (NULL == pSystemConfig)
	{
		// If pSystemConfig == NULL,
		// Application request for all sample rates available, regardless the current configuration
		//
		if (NULL == pRangesTbl)
		{
			// If pSRTable == NULL
			// Application request for size(in byte) of IR Table and number of elements inside
			//
			*u32BufferSize = u32NumOfElement*sizeof(CSRANGETABLE);
			return CS_SUCCESS;
		}
		else
		{
			if (*u32BufferSize < u32NumOfElement*sizeof(CSRANGETABLE))
				return CS_BUFFER_TOO_SMALL;
			else
			{
				for ( uInt32 i = 0; i < u32NumOfElement; i++ )
				{
					pRangesTbl[i] = m_CapsInfo.veInputRange[i];
				}
				return CS_SUCCESS;
			}
		}
	}
	else
	{
		int32 i32Status = ValidateBufferForReadWrite(CS_SYSTEM, pSystemConfig, TRUE);
		if (i32Status != CS_SUCCESS)
			return i32Status;

		uInt16 u16Input = static_cast<uInt16>(u32SubCapsId);
		PCSCHANNELCONFIG pChanCfg = pSystemConfig->pAChannels->aChannel;

		uInt16 u16ImpedanceMask = CS_IMP_1M_OHM; // external trigger has only 1 Mohm

		if (0 != u16Input)
		{
			// Internal Trigger
			if (u16Input > pSystemConfig->pAChannels->u32ChannelCount)
				return CS_INVALID_CHANNEL;

			for (uInt32 i = 0; i < pSystemConfig->pAChannels->u32ChannelCount; i++)
			{
				if (u16Input == pChanCfg[i].u32ChannelIndex)
				{
					if (pChanCfg[i].u32Impedance == CS_REAL_IMP_50_OHM)
						u16ImpedanceMask = CS_IMP_50_OHM;
					else
						u16ImpedanceMask = CS_IMP_1M_OHM;

					if (pChanCfg[i].u32Term & CS_DIRECT_ADC_INPUT)
					{
						u16ImpedanceMask = CS_IMP_50_OHM | CS_IMP_DIRECT_ADC;
					}
					break;
				}
			}
		}

		// Calculate number of element in Input Range table depending on configuration
		u32NumOfElement = 0;
		if (0 == u16Input)
		{
			// External Trigger
			for (uInt32 i = 0; i < m_CapsInfo.veInputRange.size(); i++)
			{
				if ( ( 0 != ( m_CapsInfo.veInputRange[i].u32Reserved & CS_IMP_EXT_TRIG ) ) &&
                     ( 0 != ( m_CapsInfo.veInputRange[i].u32Reserved & u16ImpedanceMask ) ) )
				{
					u32NumOfElement++;
				}
			}
		}
		else
		{
			// Trigger from channel
			for (uInt32 i = 0; i < m_CapsInfo.veInputRange.size(); i++)
			{
				if ( m_CapsInfo.veInputRange[i].u32Reserved & CS_IMP_EXT_TRIG)
					continue;
				if ( ( m_CapsInfo.veInputRange[i].u32Reserved & u16ImpedanceMask ) == u16ImpedanceMask)
					u32NumOfElement++;
			}
		}

		if (NULL == pRangesTbl)
		{
			// If pSRTable == NULL
			// Application request for size(in byte) of IR Table and number of elements inside
			//
			*u32BufferSize = u32NumOfElement * sizeof(CSRANGETABLE);
			return CS_SUCCESS;
		}
		else
		{
			// If pIRTable != NULL
			// pIRTable is the pointer to application allocated memory
			//
			if (*u32BufferSize < u32NumOfElement * sizeof(CSRANGETABLE))
				return CS_BUFFER_TOO_SMALL;

			if (0 == u16Input)
			{
				// External Trigger
				for (uInt32 i = 0, j = 0; i < m_CapsInfo.veInputRange.size(); i++)
				{
					if (m_CapsInfo.veInputRange[i].u32Reserved & CS_IMP_EXT_TRIG)
					{
						if (m_CapsInfo.veInputRange[i].u32Reserved & u16ImpedanceMask)
						{
							pRangesTbl[j++] = m_CapsInfo.veInputRange[i];
						}
					}
				}
			}
			else
			{
				for (uInt32 i = 0, j = 0; i < m_CapsInfo.veInputRange.size(); i++)
				{
					if (m_CapsInfo.veInputRange[i].u32Reserved & CS_IMP_EXT_TRIG)
						continue;
					if ((m_CapsInfo.veInputRange[i].u32Reserved & u16ImpedanceMask ) == u16ImpedanceMask)
					{
						pRangesTbl[j++] = m_CapsInfo.veInputRange[i];
					}
				}
			}
			return CS_SUCCESS;
		}
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::GetAvailableImpedances(uInt32 u32SubCapsId, PCSSYSTEMCONFIG pSystemConfig,
											   PCSIMPEDANCETABLE pImpedancesTbl, uInt32 *u32BufferSize)
{
	PCSCHANNELCONFIG pChanCfg = NULL;

	if (NULL == u32BufferSize)
		return (CS_INVALID_PARAMETER);

	bool b50Ohm = false;
	bool b1MOhm = false;
	bool b50Ohm_DADC = false;
	bool b1MOhm_DADC = false;


	for( size_t i = 0; i < m_CapsInfo.veInputRange.size(); i++ )
	{
		if ( 0 != (m_CapsInfo.veInputRange[i].u32Reserved & CS_IMP_EXT_TRIG) )
			continue;

		if ( 0 != (m_CapsInfo.veInputRange[i].u32Reserved & CS_IMP_1M_OHM) )
		{
			b1MOhm = true;
			if ( 0 != (m_CapsInfo.veInputRange[i].u32Reserved & CS_IMP_DIRECT_ADC) )
				b1MOhm_DADC = true;
		}

		if ( 0 != (m_CapsInfo.veInputRange[i].u32Reserved & CS_IMP_50_OHM) )
		{
			b50Ohm = true;
			if ( 0 != (m_CapsInfo.veInputRange[i].u32Reserved & CS_IMP_DIRECT_ADC) )
				b50Ohm_DADC = true;
		}
	}

	size_t stTableSize = 0;

	if ( b1MOhm )
		stTableSize ++;

	if ( b50Ohm )
		stTableSize ++;

	if( 0 == stTableSize  )
	{
		return CS_MISC_ERROR;
	}

	CSIMPEDANCETABLE* pCsDemoImpedanceTable = new CSIMPEDANCETABLE[stTableSize];

	size_t stIndex = 0;
	if( b1MOhm )
	{
		pCsDemoImpedanceTable[stIndex].u32Reserved = 0;
		pCsDemoImpedanceTable[stIndex].u32Imdepdance = CS_REAL_IMP_1M_OHM;
		sprintf_s(pCsDemoImpedanceTable[stIndex].strText, sizeof(pCsDemoImpedanceTable[stIndex].strText), "1 MOhm");

		if ( b1MOhm_DADC )
			pCsDemoImpedanceTable[stIndex].u32Reserved |= CS_DIRECT_ADC_INPUT;

		stIndex++;
	}

	if( b50Ohm )
	{
		pCsDemoImpedanceTable[stIndex].u32Reserved = 0;
		pCsDemoImpedanceTable[stIndex].u32Imdepdance = CS_REAL_IMP_50_OHM;
		sprintf_s(pCsDemoImpedanceTable[stIndex].strText, sizeof(pCsDemoImpedanceTable[stIndex].strText), "50 Ohm");

		if ( b50Ohm_DADC )
			pCsDemoImpedanceTable[stIndex].u32Reserved |= CS_DIRECT_ADC_INPUT;

		stIndex++;
	}


	if (u32SubCapsId)
	{
		if (u32SubCapsId > pSystemConfig->pAChannels->u32ChannelCount)
		{
			return (CS_INVALID_PARAMETER);
		}

		for (uInt32 i = 0; i < pSystemConfig->pAChannels->u32ChannelCount; i ++)
		{
			if(pSystemConfig->pAChannels->aChannel[i].u32ChannelIndex == u32SubCapsId)
			{
				pChanCfg = &pSystemConfig->pAChannels->aChannel[i];
				break;
			}
		}
		if (NULL == pChanCfg)
			return CS_INVALID_CHANNEL;


		if (pChanCfg->u32Term & CS_DIRECT_ADC_INPUT)
		{
			if (NULL == pImpedancesTbl)//----- first call return the size
			{
				*u32BufferSize = uInt32(stTableSize * sizeof(CSIMPEDANCETABLE));
				return CS_SUCCESS;
			}
			else //----- 2nd call return the table
			{
				if (*u32BufferSize < stTableSize * sizeof(CSIMPEDANCETABLE) )
					return CS_BUFFER_TOO_SMALL;
				else
				{
					::memset(pImpedancesTbl, 0,  stTableSize * sizeof(CSIMPEDANCETABLE));
					for (size_t i = 0, j = 0; i < stTableSize; i++)
					{
						if (pCsDemoImpedanceTable[i].u32Reserved & CS_DIRECT_ADC_INPUT)
							pImpedancesTbl[j] =  pCsDemoImpedanceTable[i];
					}
					return CS_SUCCESS;
				}
			}
		}
	}

	if (NULL == pImpedancesTbl)//----- first call return the size
	{
		*u32BufferSize = uInt32(stTableSize * sizeof(CSIMPEDANCETABLE));
		return CS_SUCCESS;
	}
	else //----- 2nd call return the table
	{
		if (*u32BufferSize < stTableSize * sizeof(CSIMPEDANCETABLE))
			return CS_BUFFER_TOO_SMALL;
		else
		{
			CopyMemory(pImpedancesTbl, pCsDemoImpedanceTable,  stTableSize * sizeof(CSIMPEDANCETABLE));
			return CS_SUCCESS;
		}
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::GetAvailableCouplings(uInt32 u32SubCapsId, PCSSYSTEMCONFIG pSystemConfig,
											PCSCOUPLINGTABLE pCouplingTbl, uInt32 *u32BufferSize)
{
	CSCOUPLINGTABLE CsDemoCouplingTable[] =
	{
		{CS_COUPLING_AC, "AC", 0},
		{CS_COUPLING_DC, "DC", 0}
	};

	if (NULL == u32BufferSize)
		return (CS_INVALID_PARAMETER);

	if (u32SubCapsId > pSystemConfig->pAChannels->u32ChannelCount)
	{
		return (CS_INVALID_PARAMETER);
	}

	if (NULL == pCouplingTbl) //----- first call return the size
	{
		*u32BufferSize = sizeof(CsDemoCouplingTable);
		return CS_SUCCESS;
	}
	else //----- 2nd call return the table
	{
		if ( *u32BufferSize < sizeof(CsDemoCouplingTable) )
			return CS_BUFFER_TOO_SMALL;
		else
		{
			CopyMemory(pCouplingTbl, CsDemoCouplingTable,  sizeof(CsDemoCouplingTable));
			return CS_SUCCESS;
		}
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CGageDemoSystem::AdjustAcqModesToAvailable(uInt32 *u32AcqModes)
{
	// This is a sanity check, which should not be needed once
	// CompuScope Manager is used to set the registry entries. For now,
	// though, we can have different available modes in the sample rate
	// table and the number of channels for the demo set to 1. This will
	// give us an invalid channel index error for all modes but single
	// (at least in CsTest). So we'll compare our available modes to the
	// number of channels and adjust it if it doesn't make sense.

	if (CS_MODE_OCT <= m_csSystemInfo.u32ChannelCount)
		*u32AcqModes &= (CS_MODE_OCT | CS_MODE_QUAD | CS_MODE_DUAL | CS_MODE_SINGLE);
	else if (CS_MODE_QUAD <= m_csSystemInfo.u32ChannelCount)
		*u32AcqModes &= (CS_MODE_QUAD | CS_MODE_DUAL | CS_MODE_SINGLE);
	else if (CS_MODE_DUAL <= m_csSystemInfo.u32ChannelCount)
		*u32AcqModes &= (CS_MODE_DUAL | CS_MODE_SINGLE);
	else
		*u32AcqModes &= CS_MODE_SINGLE;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::GetAvailableAcqModes(uInt32 *u32AcqModes, uInt32 *u32BufferSize)
{
	if (NULL == u32BufferSize)
		return (CS_INVALID_PARAMETER);

	if (NULL == u32AcqModes)	//----- first call return the size
	{
		*u32BufferSize = sizeof(uInt32);
		return CS_SUCCESS;
	}
	else						//----- 2nd call return the table
	{
		if (*u32BufferSize < sizeof(uInt32) )
			return CS_BUFFER_TOO_SMALL;
		else
		{
			uInt32 u32ValidModes = 0;
			for(size_t i = 0; i < m_CapsInfo.veSampleRate.size(); i++ )
			{
				u32ValidModes |= m_CapsInfo.veSampleRate[i].u32Mode;
			}
			*u32AcqModes = u32ValidModes;
			AdjustAcqModesToAvailable(u32AcqModes);

			return CS_SUCCESS;
		}
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::GetAvailableTerminations(uInt32 *u32Terminations, uInt32 *u32BufferSize)
{
	if (NULL == u32BufferSize)
		return (CS_INVALID_PARAMETER);

	if (NULL == u32Terminations)	//----- first call return the size
	{
		*u32BufferSize = sizeof(uInt32);
		return CS_SUCCESS;
	}
	else						//----- 2nd call return the table
	{
		if (*u32BufferSize < sizeof(uInt32) )
			return CS_BUFFER_TOO_SMALL;
		else
		{
			*u32Terminations = GetValidTerminations();
			return CS_SUCCESS;
		}
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::GetAvailableFlexibleTrigger(uInt32 *u32FlexibleTrigger, uInt32 *u32BufferSize)
{
	if (NULL == u32BufferSize)
		return (CS_INVALID_PARAMETER);

	if (NULL == u32FlexibleTrigger)	//----- first call return the size
	{
		*u32BufferSize = sizeof(uInt32);
		return CS_SUCCESS;
	}
	else						//----- 2nd call return the table
	{
		if (*u32BufferSize < sizeof(uInt32) )
			return CS_BUFFER_TOO_SMALL;
		else
		{
			*u32FlexibleTrigger = 0; // Demo does not have flexible trigger
			return CS_SUCCESS;
		}
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::GetBoardTriggerEngines(uInt32 *u32BoardEngineCount, uInt32 *u32BufferSize)
{
	if (NULL == u32BufferSize)
		return (CS_INVALID_PARAMETER);

	if (NULL == u32BoardEngineCount)	//----- first call return the size
	{
		*u32BufferSize = sizeof(uInt32);
		return CS_SUCCESS;
	}
	else						//----- 2nd call return the table
	{
		if (*u32BufferSize < sizeof(uInt32) )
			return CS_BUFFER_TOO_SMALL;
		else
		{
			 // Demo has 1 trigger engine per board
			*u32BoardEngineCount = m_csSystemInfo.u32TriggerMachineCount / m_csSystemInfo.u32BoardCount;
			return CS_SUCCESS;
		}
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::GetAvailableTriggerSources(uInt32 SubCapsId, int32* pi32ArrayTriggerSources, uInt32 *u32BufferSize)
{
	uInt32	i;
	uInt32	k;
	uInt32	u32ChannelSkip = 0;

	if (NULL == u32BufferSize)
		return (CS_INVALID_PARAMETER);

	// Calculate number of trigger source available
	uInt32	u32NumberTriggerSources = 0;

	// External triggers (one per board) and Disable
	u32NumberTriggerSources = m_csSystemInfo.u32BoardCount + 1;

	uInt32	u32ChannelsPerCard = m_csSystemInfo.u32ChannelCount / m_csSystemInfo.u32BoardCount;

	if ( 0 == SubCapsId )
	{
		u32NumberTriggerSources += m_csSystemInfo.u32ChannelCount;
		u32ChannelSkip = 1;
	}
	else
	{
		uInt32 u32MaskedMode = SubCapsId & CS_MASKED_MODE;
		u32ChannelSkip = u32ChannelsPerCard / u32MaskedMode;
		u32NumberTriggerSources += u32MaskedMode;
	}
	if ( NULL == pi32ArrayTriggerSources )
	{
		*u32BufferSize = u32NumberTriggerSources * sizeof(uInt32);
		return CS_SUCCESS;
	}
	else
	{
		if ( *u32BufferSize < u32NumberTriggerSources * sizeof(uInt32) )
			return CS_BUFFER_TOO_SMALL;
		else
		{
			k = 0;
			// Add external triggers, one per board
			for( i = 0; i < m_csSystemInfo.u32BoardCount; i ++ )
			{
				pi32ArrayTriggerSources[k++] =  (int32) (i-m_csSystemInfo.u32BoardCount);
			}
			// add disable trigger
			pi32ArrayTriggerSources[k++] = 0;

			// add channel trigger sources
			for( i = 0; i < m_csSystemInfo.u32ChannelCount; i += u32ChannelSkip )
			{
				pi32ArrayTriggerSources[k++] =  (int32) (i+1);
				i += u32ChannelSkip;
			}

			*u32BufferSize = u32NumberTriggerSources * sizeof(uInt32);
			return CS_SUCCESS;
		}
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::GetMaxSegmentPadding(uInt32 *pBuffer,  uInt32 *BufferSize)
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	if (IsBadWritePtr(BufferSize, sizeof(uInt32)))
		return CS_INVALID_POINTER_BUFFER;

	if (IsBadWritePtr( pBuffer, sizeof(uInt32)))
		return CS_INVALID_POINTER_BUFFER;

	if (NULL != pBuffer)
	{
		// Determine max padding for the board type
		if (CS_SAMPLE_BITS_8 == m_u32SampleBits) // 8 bit demo emulates CS82G
			*pBuffer = 128;
		else if (CS_SAMPLE_BITS_16 == m_u32SampleBits) // 16 bit demo emulates CS1610
			*pBuffer = 128;
		else if (CS_SAMPLE_BITS_12 == m_u32SampleBits) // 12 bit demo emulates CS12400
			*pBuffer = 112;
		else // 14 bit demo emulates an 8 channel spider board
			*pBuffer = 112;
	}

	*BufferSize = sizeof(uInt32);

	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::CsDrvGetTriggerResolution(uInt32 *pBuffer,  uInt32 *u32BufferSize)
{
	if (NULL == u32BufferSize)
		return (CS_INVALID_PARAMETER);

	if ( NULL == pBuffer )
	{
		*u32BufferSize = sizeof(uInt32);
		return CS_SUCCESS;
	}

	if ( IsBadWritePtr( u32BufferSize, sizeof( uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( IsBadWritePtr( pBuffer, sizeof( uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( NULL != pBuffer )
	{
		*pBuffer = DEMO_TRIGGER_RES;
	}

	*u32BufferSize = sizeof(uInt32);

	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::AcquisitionSystemInit(void)
{
	int nAcquisitionStructSize = sizeof(CSACQUISITIONCONFIG);
	m_csSystemConfig.pAcqCfg = static_cast<PCSACQUISITIONCONFIG>(VirtualAlloc(NULL, nAcquisitionStructSize, MEM_COMMIT, PAGE_READWRITE));
	if (NULL == m_csSystemConfig.pAcqCfg)
		return CS_MEMORY_ERROR;
	ZeroMemory(m_csSystemConfig.pAcqCfg, nAcquisitionStructSize);

	// Allocate memory for channel configurations
	int nChannelStructSize = sizeof(ARRAY_CHANNELCONFIG) + ((m_csSystemInfo.u32ChannelCount - 1) * sizeof(CSCHANNELCONFIG));
	m_csSystemConfig.pAChannels = static_cast<PARRAY_CHANNELCONFIG>(VirtualAlloc(NULL, nChannelStructSize, MEM_COMMIT, PAGE_READWRITE));
	if (NULL == m_csSystemConfig.pAChannels)
	{
		VirtualFree(m_csSystemConfig.pAcqCfg, 0, MEM_RELEASE);
        return CS_MEMORY_ERROR;
	}
	ZeroMemory(m_csSystemConfig.pAChannels, nChannelStructSize);
	m_csSystemConfig.pAChannels->u32ChannelCount = m_csSystemInfo.u32ChannelCount;

	// Allocate memory for trigger configurations
	int nTriggerStructSize = sizeof(ARRAY_TRIGGERCONFIG) + ((m_csSystemInfo.u32TriggerMachineCount - 1) * sizeof(CSTRIGGERCONFIG));
	m_csSystemConfig.pATriggers = static_cast<PARRAY_TRIGGERCONFIG>(VirtualAlloc(NULL, nTriggerStructSize, MEM_COMMIT, PAGE_READWRITE));
	if (NULL == m_csSystemConfig.pATriggers)
	{
		VirtualFree(m_csSystemConfig.pAcqCfg, 0, MEM_RELEASE);
		VirtualFree(m_csSystemConfig.pAChannels, 0, MEM_RELEASE);
        return CS_MEMORY_ERROR;
	}
	ZeroMemory(m_csSystemConfig.pATriggers, nTriggerStructSize);
	m_csSystemConfig.pATriggers->u32TriggerCount = m_csSystemInfo.u32TriggerMachineCount;
	m_csSystemConfig.u32Size = sizeof(CSSYSTEMCONFIG);

	SetAcquisitionDefaults();
	SetChannelDefaults();
	SetTriggerEngineDefaults();

	// Maybe do in StartCapture - RICKY
	// so I can handle single capture mode

	m_pTrigger->SetChannelSource(GetChannelSource());
	m_pTrigger->SetExternalSource(GetExternalSource());
	m_pTrigger->SetHoldoff(m_csSystemConfig.pAcqCfg->i64TriggerHoldoff);
	m_pTrigger->SetDelay(m_csSystemConfig.pAcqCfg->i64TriggerDelay);
	m_pTrigger->SetCondition(GetTriggerSlope(), GetTriggerRange(), GetTriggerLevel());

	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::AcquisitionSystemCleanup(void)
{
	// Do any necessary cleanup
	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::GetAcquisitionSystemCaps(uInt32 u32CapsId, PCSSYSTEMCONFIG pSysCfg,
												void *pBuffer, uInt32 *u32BufferSize)
{
    uInt32  u32MainCapId = u32CapsId & CAPS_ID_MASK;
    uInt16  u16SubCapId = (uInt16) (u32CapsId & 0xFFFF);
	int32 i32Status = 0;

	switch(u32MainCapId)
	{
		case CAPS_SAMPLE_RATES:
			i32Status = GetAvailableSampleRates(u16SubCapId, pSysCfg, (PCSSAMPLERATETABLE)pBuffer, u32BufferSize);
			break;

		case CAPS_INPUT_RANGES:
			i32Status = GetAvailableInputRanges(u16SubCapId, pSysCfg, (PCSRANGETABLE)pBuffer, u32BufferSize);
			break;

		case CAPS_IMPEDANCES:
			i32Status = GetAvailableImpedances(u16SubCapId, pSysCfg, (PCSIMPEDANCETABLE) pBuffer, u32BufferSize);
			break;

		case CAPS_COUPLINGS:
			i32Status = GetAvailableCouplings(u16SubCapId, pSysCfg, (PCSCOUPLINGTABLE) pBuffer, u32BufferSize);
			break;

		case CAPS_ACQ_MODES:
			i32Status = GetAvailableAcqModes((uInt32 *) pBuffer, u32BufferSize);
			break;

		case CAPS_TERMINATIONS:
			i32Status = GetAvailableTerminations((uInt32 *)pBuffer, u32BufferSize);
        	break;

		case CAPS_FLEXIBLE_TRIGGER:
			i32Status = GetAvailableFlexibleTrigger((uInt32 *)pBuffer, u32BufferSize);
			break;

		case CAPS_BOARD_TRIGGER_ENGINES:
			i32Status = GetBoardTriggerEngines((uInt32 *)pBuffer, u32BufferSize);
			break;

		case CAPS_TRIGGER_SOURCES:
			i32Status = GetAvailableTriggerSources(u16SubCapId, (int32 *)pBuffer, u32BufferSize);
			break;

		case CAPS_MAX_SEGMENT_PADDING:
			i32Status = GetMaxSegmentPadding((uInt32 *)pBuffer, u32BufferSize);
			break;

		case CAPS_MULREC:
			i32Status = CS_SUCCESS;
			break;

		case CAPS_TRIGGER_RES:
			return CsDrvGetTriggerResolution((uInt32 *) pBuffer, u32BufferSize );
			break;

		default:
			i32Status = CS_INVALID_REQUEST;
	}
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::GetAcquisitionStatus(uInt32 *u32Status)
{
	*u32Status = 0;
	return *u32Status;
//	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::GetParams(uInt32 u32ParamID, void *ParamBuffer)
{
	switch (u32ParamID)
	{
		case CS_SYSTEM:
		{
			// Validate Buffer for Write Access
			int32 i32Status = ValidateBufferForReadWrite(u32ParamID, ParamBuffer, FALSE);
			if (i32Status != CS_SUCCESS)
				return i32Status;

			PCSSYSTEMCONFIG pSysConfig = static_cast<PCSSYSTEMCONFIG>(ParamBuffer);
			if (0 == pSysConfig->u32Size)
				return CS_INVALID_STRUCT_SIZE;

			i32Status = GetAcquisitionParams(pSysConfig->pAcqCfg);
			if (CS_FAILED(i32Status))
				return i32Status;

			for (uInt32 i = 0; i < pSysConfig->pAChannels->u32ChannelCount; i++)
			{
				i32Status = GetChannelParams(&pSysConfig->pAChannels->aChannel[i]);
				if (CS_FAILED(i32Status))
					return i32Status;
			}
			for (uInt32 i = 0; i < pSysConfig->pATriggers->u32TriggerCount; i++)
			{
				i32Status = GetTriggerParams(&pSysConfig->pATriggers->aTrigger[i]);
				if (CS_FAILED(i32Status))
					return i32Status;
			}
			return CS_SUCCESS;
		}

		case CS_ACQUISITION:
		{
			// Validate Buffer for Write Access
			int32 i32Status = ValidateBufferForReadWrite(u32ParamID, ParamBuffer, FALSE);
			if (i32Status != CS_SUCCESS)
				return i32Status;

			PCSACQUISITIONCONFIG pAcqConfig = static_cast<PCSACQUISITIONCONFIG>(ParamBuffer);
			return (GetAcquisitionParams(pAcqConfig));
		}

		case CS_TRIGGER:
		{
			// Validate Buffer for Write Access
			int32 i32Status = ValidateBufferForReadWrite(u32ParamID, ParamBuffer, FALSE);
			if (i32Status != CS_SUCCESS)
				return i32Status;

			PARRAY_TRIGGERCONFIG pTrigArrayConfig = static_cast<PARRAY_TRIGGERCONFIG>(ParamBuffer);
			for (uInt32 i = 0; i < pTrigArrayConfig->u32TriggerCount; i++)
			{
				i32Status = GetTriggerParams(&pTrigArrayConfig->aTrigger[i]);
				if (CS_FAILED(i32Status))
					return i32Status;
			}
			return i32Status;
		}

		case CS_CHANNEL:
		{
			// Validate Buffer for Write Access
			int32 i32Status = ValidateBufferForReadWrite(u32ParamID, ParamBuffer, FALSE);
			if (i32Status != CS_SUCCESS)
				return i32Status;

			PARRAY_CHANNELCONFIG pChanArrayConfig = static_cast<PARRAY_CHANNELCONFIG>(ParamBuffer);
			for (uInt32 i = 0; i < pChanArrayConfig->u32ChannelCount; i++)
			{
				i32Status = GetChannelParams(&pChanArrayConfig->aChannel[i]);
				if (CS_FAILED(i32Status))
					return i32Status;
			}
			return i32Status;
		}

		case CS_READ_FLASH:
		{
			// check buffer for Read access

			// For now, do nothing
			return CS_SUCCESS;
		}

		case CS_MEM_READ_THRU:
		case CS_TRIGADDR_OFFSET_ADJUST:
		case CS_TRIGLEVEL_ADJUST:
		case CS_TRIGGAIN_ADJUST:
		{
			// Validate Buffer for Write Access
			int32 i32Status = ValidateBufferForReadWrite(u32ParamID, ParamBuffer, FALSE);
			if (i32Status != CS_SUCCESS)
				return i32Status;
			return CS_SUCCESS;
		}
		case CS_FIR_CONFIG:
			return CS_FUNCTION_NOT_SUPPORTED;

		case CS_EXTENDED_BOARD_OPTIONS:
		{
			// Validate Buffer for Write Access
			int32 i32Status = ValidateBufferForReadWrite(u32ParamID, ParamBuffer, FALSE);
			if (i32Status != CS_SUCCESS)
				return i32Status;

			PULONG pExOptions = static_cast<PULONG>(ParamBuffer);
			pExOptions[0] = 0;
			pExOptions[1] = 0;
			return CS_SUCCESS;
		}

		case CS_SYSTEMINFO_EX:
		{
			// Should be handled in CGageDemo class but just in case...
			// Validate Buffer for Write Access
			int32 i32Status = ValidateBufferForReadWrite(u32ParamID, ParamBuffer, FALSE);
			if (i32Status != CS_SUCCESS)
				return i32Status;

			PCSSYSTEMINFOEX pSysInfoEx = static_cast<PCSSYSTEMINFOEX>(ParamBuffer);

			CopyMemory(pSysInfoEx, &m_csSysInfoEx, pSysInfoEx->u32Size);
			return CS_SUCCESS;
		}

		case CS_READ_NVRAM:
		{
			// check buffer for Read access
			PCS_PLX_NVRAM_RW_STRUCT pNVRAM_Struct = static_cast<PCS_PLX_NVRAM_RW_STRUCT>(ParamBuffer);

			if (IsBadWritePtr(ParamBuffer, sizeof(CS_PLX_NVRAM_RW_STRUCT)))
				return CS_INVALID_POINTER_BUFFER;

			if (pNVRAM_Struct->u32Size != sizeof(CS_PLX_NVRAM_RW_STRUCT))
				return CS_INVALID_STRUCT_SIZE;

			// For now, do nothing
			return CS_SUCCESS;
		}

		case CS_READ_VER_INFO:
		{
			// Should be handled in CGageDemo class but just in case...
			// check buffer for Read access
			PCS_FW_VER_INFO pInfoStruct = static_cast<PCS_FW_VER_INFO>(ParamBuffer);

			/*if (IsBadWritePtr(ParamBuffer, sizeof(CS_FW_VER_INFO)))
				return CS_INVALID_POINTER_BUFFER;*/

			if (pInfoStruct->u32Size != sizeof(CS_FW_VER_INFO))
				return CS_INVALID_STRUCT_SIZE;

			CopyMemory(pInfoStruct, &m_csVerInfo, pInfoStruct->u32Size);
			return CS_SUCCESS;
		}

		case CS_READ_EEPROM:
		{
			// check buffer for Write access
			PCS_GENERIC_FLASH_READWRITE pEepromStruct = static_cast<PCS_GENERIC_FLASH_READWRITE>(ParamBuffer);

			if (IsBadWritePtr( pEepromStruct, sizeof(CS_GENERIC_FLASH_READWRITE)))
				return CS_INVALID_POINTER_BUFFER;

			if (pEepromStruct->u32Size != sizeof(CS_GENERIC_FLASH_READWRITE))
				return CS_INVALID_STRUCT_SIZE;

			// For now, do nothing
			return CS_SUCCESS;
		}

		default:
			return CS_INVALID_PARAMS_ID;
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::SetParams(uInt32 u32ParamID, void *ParamBuffer)
{
	// Validate configuration Params
	switch (u32ParamID)
	{
		case CS_ACQUISITION:
		{
			PCSACQUISITIONCONFIG pAcqConfig = static_cast<PCSACQUISITIONCONFIG>(ParamBuffer);
			return (SetAcquisitionParams(pAcqConfig));
		}

		case CS_TRIGGER:
		{
			PCSTRIGGERCONFIG pTrigConfig = static_cast<PCSTRIGGERCONFIG>(ParamBuffer);
			return (SetTriggerParams(pTrigConfig));
		}

		case CS_CHANNEL:
		{
			PCSCHANNELCONFIG pChanConfig = static_cast<PCSCHANNELCONFIG>(ParamBuffer);
			return (SetChannelParams(pChanConfig));
		}
		break;

		case CS_SYSTEM:
		{
			PCSSYSTEMCONFIG pSysConfig = static_cast<PCSSYSTEMCONFIG>(ParamBuffer);
			if (0 == pSysConfig->u32Size)
				return CS_INVALID_STRUCT_SIZE;

			int32 i32Status = SetAcquisitionParams(pSysConfig->pAcqCfg);
			if (CS_FAILED(i32Status))
				return i32Status;

			for (uInt32 i = 0; i < pSysConfig->pAChannels->u32ChannelCount; i++)
			{
				i32Status = SetChannelParams(&pSysConfig->pAChannels->aChannel[i]);
				if (CS_FAILED(i32Status))
					return i32Status;
			}
			for (uInt32 i = 0; i < pSysConfig->pATriggers->u32TriggerCount; i++)
			{
				i32Status = SetTriggerParams(&pSysConfig->pATriggers->aTrigger[i]);
				if (CS_FAILED(i32Status))
					return i32Status;
			}
			return CS_SUCCESS;
		}

		case CS_WRITE_FLASH:
		{
			// check buffer for Read access
			PCS_GENERIC_FLASH_READWRITE pFlashStruct = static_cast<PCS_GENERIC_FLASH_READWRITE>(ParamBuffer);

			/*if (IsBadReadPtr(ParamBuffer, sizeof(CS_GENERIC_FLASH_READWRITE)))
				return CS_INVALID_POINTER_BUFFER;*/

			if (pFlashStruct->u32Size != sizeof(CS_GENERIC_FLASH_READWRITE))
				return CS_INVALID_STRUCT_SIZE;

			// For now, do nothing
			return CS_SUCCESS;
		}

		case CS_FIR_CONFIG:
			return CS_FUNCTION_NOT_SUPPORTED;

		case CS_SEND_DAC:
		case CS_SELF_TEST_MODE:
		case CS_TRIG_OUT:
		case CS_ADDON_RESET:
		case CS_TRIG_ENABLE:
		case CS_NIOS_DEBUG:
		case CS_MEM_WRITE_THRU:
		case CS_CALIBMODE_CONFIG:
		case CS_TRIGADDR_OFFSET_ADJUST:
		case CS_TRIGLEVEL_ADJUST:
		case CS_TRIGGAIN_ADJUST:
		case CS_DELAY_LINE_MODE:
		{
			// Validate Buffer for Read/Write
			int32 i32Status = ValidateBufferForReadWrite(u32ParamID, ParamBuffer, TRUE);
			if (i32Status != CS_SUCCESS)
				return i32Status;
			else
				// for now, do nothing
				return CS_SUCCESS;
		}

		case CS_WRITE_NVRAM:
		{
			// check buffer for Read access
			PCS_PLX_NVRAM_RW_STRUCT pNVRAM_Struct = static_cast<PCS_PLX_NVRAM_RW_STRUCT>(ParamBuffer);

			/*if (IsBadReadPtr(ParamBuffer, sizeof(CS_PLX_NVRAM_RW_STRUCT)))
				return CS_INVALID_POINTER_BUFFER;*/

			if (pNVRAM_Struct->u32Size != sizeof(CS_PLX_NVRAM_RW_STRUCT))
				return CS_INVALID_STRUCT_SIZE;

			// Do nothing
			return CS_SUCCESS;
		}

		case CS_WRITE_EEPROM:
		{
			// check buffer for Read access
			PCS_GENERIC_FLASH_READWRITE pEepromStruct = static_cast<PCS_GENERIC_FLASH_READWRITE>(ParamBuffer);

			/*if ( IsBadReadPtr(pEepromStruct, sizeof(CS_GENERIC_FLASH_READWRITE)))
				return CS_INVALID_POINTER_BUFFER;*/

			if (pEepromStruct->u32Size != sizeof(CS_GENERIC_FLASH_READWRITE))
					return CS_INVALID_STRUCT_SIZE;

			// Do nothing
			return CS_SUCCESS;
		}

		case CS_TRIM_CALIBRATOR:
		case CS_SAVE_CALIBTABLE_TO_EEPROM:
			return CS_SUCCESS;

		default:
			return CS_INVALID_PARAMS_ID;

	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::CsDrvValidateAcquisitionConfig(PCSSYSTEMCONFIG pSysCfg, PCSSYSTEMINFO pSysInfo, uInt32 u32Coerce)
{
	PCSACQUISITIONCONFIG pAcqCfg = static_cast<PCSACQUISITIONCONFIG>(pSysCfg->pAcqCfg);
	int32	i32Status = CS_SUCCESS;
	// MaxMemPerChan is the total board memory (32 MegaSamples) divided by number channels
	// in the current mode. The mode constants are the same as the number of channels used,
	// i.e. single = 1, dual = 2, quad = 4 and octal = 8
	int64	MaxMemPerChan = pSysInfo->i64MaxMemory / (pAcqCfg->u32Mode & 0x0000ffff);
	uInt32	u32AcqModeValid(0), u32Size(sizeof(uInt32));

	GetAvailableAcqModes(&u32AcqModeValid, &u32Size);

	uInt32 u32ModeIn(pAcqCfg->u32Mode), u32Mask(CS_MODE_OCT);
	u32ModeIn &= u32AcqModeValid;

	int nBits(0);
	while( 0 != u32Mask )
	{
		nBits += ( 0 != (u32Mask & u32ModeIn ) ) ? 1 : 0;
		u32Mask >>= 1;
	}

	u32Mask = CS_MODE_OCT;
	while( 0 == ( u32Mask & u32AcqModeValid ) )
	{
		u32Mask >>= 1;
	}

	if( ( 1 != nBits ) ||
		( u32ModeIn != pAcqCfg->u32Mode ) )

	{
		if (u32Coerce)
		{
			pAcqCfg->u32Mode =  1 == nBits ? u32ModeIn : u32Mask;
			i32Status = CS_CONFIG_CHANGED ;
		}
		else
			return CS_INVALID_ACQ_MODE;
	}

	//----- Sample Rate
	int64 i64CoerceSampleRate = ValidateSampleRate(pSysCfg);
	if (i64CoerceSampleRate != pSysCfg->pAcqCfg->i64SampleRate)
	{
		if (u32Coerce)
		{
			pAcqCfg->i64SampleRate = i64CoerceSampleRate;
			i32Status = CS_CONFIG_CHANGED ;
		}
		else
			return CS_INVALID_SAMPLE_RATE;
	}

	if (0 == pAcqCfg->u32SegmentCount)
	{
		if (u32Coerce)
		{
			pAcqCfg->u32SegmentCount = 1;
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_SEGMENT_COUNT;
	}

	// Validation of trigger holdoff
	if  (pAcqCfg->i64TriggerHoldoff < 0)
	{
		if (u32Coerce)
		{
			pAcqCfg->i64TriggerHoldoff = 0;
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_TRIGHOLDOFF;
	}
	uInt32	u32DepthRes = (pAcqCfg->u32Mode & CS_MODE_DUAL) ? 2: 4;

	// Validation of Segment Size
	if (pAcqCfg->i64SegmentSize % u32DepthRes)
	{
		if (u32Coerce)
		{
			pAcqCfg->i64SegmentSize -= (pAcqCfg->i64SegmentSize % u32DepthRes);
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_SEGMENT_SIZE;
	}

	int64 i64MaxSegmentSize = MaxMemPerChan;

	if ( (pAcqCfg->i64SegmentSize < CSMV_MIN_SEG_SIZE)  ||
		 (pAcqCfg->i64SegmentSize > i64MaxSegmentSize ) )
	{
		if (u32Coerce)
		{
			if ( pAcqCfg->i64SegmentSize < CSMV_MIN_SEG_SIZE)
				pAcqCfg->i64SegmentSize = CSMV_MIN_SEG_SIZE;
			else
				pAcqCfg->i64SegmentSize = i64MaxSegmentSize;

			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_SEGMENT_SIZE;
	}

	// Validate Segment count again to check if the count is too large
	// based on the segment size
	uInt32 u32MaxSegmentCount = static_cast<uInt32>(i64MaxSegmentSize / pAcqCfg->i64SegmentSize);
	// Demo dll has a hardcoded restriction of 256 segments
	u32MaxSegmentCount = (u32MaxSegmentCount > c_u32MaxSegmentCount) ? c_u32MaxSegmentCount : u32MaxSegmentCount;

	if (pAcqCfg->u32SegmentCount > u32MaxSegmentCount)
	{
		if (u32Coerce)
		{
			pAcqCfg->u32SegmentCount = u32MaxSegmentCount;
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_SEGMENT_COUNT;
	}

	// Validate triggger depth
	if (pAcqCfg->i64Depth % u32DepthRes)
	{
		if (u32Coerce)
		{
			pAcqCfg->i64Depth -= (pAcqCfg->i64Depth % u32DepthRes);
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_TRIG_DEPTH;
	}

	if ( (pAcqCfg->i64Depth < CSMV_MIN_SEG_SIZE)  ||
			(pAcqCfg->i64Depth > pAcqCfg->i64SegmentSize) )
	{
		if (u32Coerce)
		{
			if ( pAcqCfg->i64Depth < CSMV_MIN_SEG_SIZE)
				pAcqCfg->i64Depth = CSMV_MIN_SEG_SIZE;
			else
				pAcqCfg->i64Depth = pAcqCfg->i64SegmentSize;

			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_DEPTH_SIZE_TOO_BIG;
	}

	// Validation of Trigger Delay
	if  (pAcqCfg->i64TriggerDelay < 0)
	{
		if (u32Coerce)
		{
			pAcqCfg->i64TriggerDelay = 0;
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_TRIGDELAY;
	}
	int64 i64MaxPreTrigDepth = i64MaxSegmentSize;
	if ( pAcqCfg->i64TriggerDelay != 0 )
	{
		i64MaxPreTrigDepth = 0;		// No pretrigger if TriggerDelay is enable
	}

	if ( pAcqCfg->i64SegmentSize - pAcqCfg->i64Depth > i64MaxPreTrigDepth )
	{
		if ( u32Coerce )
		{
			pAcqCfg->i64SegmentSize = pAcqCfg->i64Depth + i64MaxPreTrigDepth;
			i32Status = CS_CONFIG_CHANGED;
		}
		else
		{
			return CS_INVALID_PRETRIGGER_DEPTH;
		}
	}

	//------ Trigger
	if ((pAcqCfg->u32TrigEnginesEn != CS_TRIG_ENGINES_DIS) && (pAcqCfg->u32TrigEnginesEn != CS_TRIG_ENGINES_EN))
		return CS_MISC_ERROR;

	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CGageDemoSystem::GetValidTerminations()
{
	uInt32 u32Term = 0;
	for( size_t i = 0; i < m_CapsInfo.veInputRange.size(); i++ )
	{
		if ( 0 != (m_CapsInfo.veInputRange[i].u32Reserved & CS_IMP_EXT_TRIG) )
			continue;

		u32Term |= m_CapsInfo.veInputRange[i].u32Reserved;
	}

	return u32Term;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::CsDrvValidateChannelConfig(PCSSYSTEMCONFIG pSysCfg, PCSSYSTEMINFO pSysInfo, uInt32 u32Coerce)
{
	PCSCHANNELCONFIG pChanCfg = static_cast<PCSCHANNELCONFIG>(pSysCfg->pAChannels->aChannel);
	int32 i32Status = CS_SUCCESS;

	if (pSysCfg->pAChannels->u32ChannelCount > pSysInfo->u32ChannelCount)
	{
		if (u32Coerce)
		{
			pSysCfg->pAChannels->u32ChannelCount = pSysInfo->u32ChannelCount;
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_CHANNEL_COUNT;
	}

	uInt32 u32ValidTerminations = GetValidTerminations();

	for (uInt32 k = 0; k < pSysCfg->pAChannels->u32ChannelCount; k ++, pChanCfg++)
	{
		//----- Channel Index
		if ((pChanCfg->u32ChannelIndex == 0) || (pChanCfg->u32ChannelIndex > pSysInfo->u32ChannelCount))
		{
			if (u32Coerce)
			{
				pChanCfg->u32ChannelIndex = 1;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_CHANNEL;
		}
		//----- Term
		if (pChanCfg->u32Term & ~u32ValidTerminations)
		{
			if (u32Coerce)
			{
				pChanCfg->u32Term = CS_COUPLING_DC;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_COUPLING;
		}
		//----- Impedance
		uInt32 ImpedanceMask = CS_IMP_1M_OHM;
		if (CS_REAL_IMP_1M_OHM == pChanCfg->u32Impedance)
		{
			ImpedanceMask = CS_IMP_1M_OHM;
		}
		else if (CS_REAL_IMP_50_OHM == pChanCfg->u32Impedance)
		{
			ImpedanceMask = CS_IMP_50_OHM;
		}
		else
		{
			if (u32Coerce)
			{
				pChanCfg->u32Impedance = CS_REAL_IMP_1M_OHM;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_IMPEDANCE;
		}
		//----- Gain
		uInt32	i;
		for (i = 0; i < m_CapsInfo.veInputRange.size(); i++)
		{
			if (m_CapsInfo.veInputRange[i].u32Reserved & CS_IMP_EXT_TRIG)
				continue;

			if (m_CapsInfo.veInputRange[i].u32Reserved & ImpedanceMask)
			{
				if (m_CapsInfo.veInputRange[i].u32InputRange == pChanCfg->u32InputRange )
					break;
			}
		}

		if (i >= m_CapsInfo.veInputRange.size())
		{
			if (u32Coerce)
			{
				pChanCfg->u32InputRange = m_CapsInfo.veInputRange[0].u32InputRange;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_GAIN;
		}
		// If the DC offset is out of range, coerce it back even if the
		// coerce flag is not set
		// Range limit is +1000 or -1000 in the 2000 mV range.
		int32 i32RangeLimit = pChanCfg->u32InputRange / 2;
		if (pChanCfg->i32DcOffset < -i32RangeLimit)
			pChanCfg->i32DcOffset = -i32RangeLimit;
		if (pChanCfg->i32DcOffset > i32RangeLimit)
			pChanCfg->i32DcOffset = i32RangeLimit;
	}
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CGageDemoSystem::ValidateTriggerChannelSourceAvailable(int32 i32TriggerSource, uInt32 u32Mode, uInt32 u32ChannelCount)
{
	if ((i32TriggerSource < CS_TRIG_SOURCE_CHAN_1) || (i32TriggerSource > (int32)u32ChannelCount))
		return false;

	bool bRet = false;

	uInt32 u32MaskedMode = u32Mode & CS_MASKED_MODE;
	uInt32 u32ChannelSkip = u32ChannelCount / u32MaskedMode;
	if (u32ChannelSkip < 1)
		return false;

	for (uInt32 i = 1; i <= u32ChannelCount; i += u32ChannelSkip)
	{
		if (i32TriggerSource == (int32)i)
		{
			bRet = true;
			break;
		}
	}
	return bRet;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::CsDrvValidateTriggerConfig(PCSSYSTEMCONFIG pSysCfg, PCSSYSTEMINFO pSysInfo, uInt32 u32Coerce)
{
	PCSTRIGGERCONFIG pTrigCfg = static_cast<PCSTRIGGERCONFIG>(pSysCfg->pATriggers->aTrigger);
	int32 i32Status = CS_SUCCESS;

	if (pSysCfg->pATriggers->u32TriggerCount > pSysInfo->u32TriggerMachineCount)
	{
		if (u32Coerce)
		{
			pSysCfg->pATriggers->u32TriggerCount = pSysInfo->u32TriggerMachineCount;
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_TRIGGER_COUNT;
	}
	for (uInt32	i = 0; i < pSysCfg->pATriggers->u32TriggerCount; i++, pTrigCfg++)
	{
		//----- trigger count: must be 0 < count < max_count
		if (pTrigCfg->u32TriggerIndex <= 0 || pTrigCfg->u32TriggerIndex > pSysInfo->u32TriggerMachineCount)
		{
			if (u32Coerce)
			{
				pTrigCfg->u32TriggerIndex = 1;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_TRIGGER;
		}

		//----- condition: might be pos, neg or stg else TBD
		if (pTrigCfg->u32Condition != CS_TRIG_COND_POS_SLOPE &&
			pTrigCfg->u32Condition != CS_TRIG_COND_NEG_SLOPE &&
			pTrigCfg->u32Condition != CS_TRIG_COND_PULSE_WIDTH)
		{
			if (u32Coerce)
			{
				pTrigCfg->u32Condition = CS_TRIG_COND_POS_SLOPE;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_TRIG_COND;
		}

		//----- level: must be -100<=level<=100
		if (pTrigCfg->i32Level > 100 || pTrigCfg->i32Level < -100)
		{
			if (u32Coerce)
			{
				pTrigCfg->i32Level = 0;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_TRIG_LEVEL;
		}

		//----- source: must be either ext_1V, ext 5_5, disable or
		//----- an available channel source depending on the mode
		if ( !ValidateTriggerChannelSourceAvailable(pTrigCfg->i32Source,
					pSysCfg->pAcqCfg->u32Mode, pSysInfo->u32ChannelCount) &&
			 pTrigCfg->i32Source != CS_TRIG_SOURCE_DISABLE &&
			 pTrigCfg->i32Source != CS_TRIG_SOURCE_EXT)
		{
			if (u32Coerce)
			{
				pTrigCfg->i32Source = CS_TRIG_SOURCE_CHAN_1;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_TRIG_SOURCE;
		}

		//----- coupling for external trigger: must be either AC or DC
		if (CS_TRIG_SOURCE_EXT == pTrigCfg->i32Source)
		{
			if ((pTrigCfg->u32ExtCoupling != CS_COUPLING_AC) && (pTrigCfg->u32ExtCoupling != CS_COUPLING_DC))
			{
				if (u32Coerce)
				{
					pTrigCfg->u32ExtCoupling = CS_COUPLING_DC;
					i32Status = CS_CONFIG_CHANGED;
				}
				else
					return CS_INVALID_COUPLING;
			}

			//----- gain for External Trig,
			if ((pTrigCfg->u32ExtTriggerRange != CS_GAIN_2_V) && (pTrigCfg->u32ExtTriggerRange != CS_GAIN_10_V))
			{
				if (u32Coerce)
				{
					pTrigCfg->u32ExtTriggerRange = CS_GAIN_2_V;
					i32Status = CS_CONFIG_CHANGED;
				}
				else
					return CS_INVALID_GAIN;
			}
		}
	}
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int64 CGageDemoSystem::CoerceSampleRate(int64 i64AnySampleRate, uInt32 u32ExtClk, uInt32 u32Mode)
{
	int64 i64UpperSampleRate = m_CapsInfo.veSampleRate[0].PublicPart.i64SampleRate;
	int64 i64LowerSampleRate = i64UpperSampleRate;

	uInt32 u32SrSize = uInt32(m_CapsInfo.veSampleRate.size());

	uInt32 j;
	for (uInt32 i = 0; i < u32SrSize - 1; i++)
	{
		bool bFoundUpper = false;
		// find the uper and lower sample rate
		for (j = i; j < u32SrSize; j++)
		{
			if (0 == (u32Mode & m_CapsInfo.veSampleRate[j].u32Mode))
				continue;

			if (!bFoundUpper)
			{
				i64UpperSampleRate = m_CapsInfo.veSampleRate[j].PublicPart.i64SampleRate;
				bFoundUpper = true;
			}
			else
			{
				i64LowerSampleRate = m_CapsInfo.veSampleRate[j].PublicPart.i64SampleRate;
				break;
			}
		}
		// if external clock rate is set,

		if (u32ExtClk)
		{
			if (CS_SAMPLE_BITS_8 == m_csSystemInfo.u32SampleBits)
			{ // treat like CS82G
				if (i64AnySampleRate > CS_SR_1GHZ)
					i64AnySampleRate = CS_SR_1GHZ;
				if (i64AnySampleRate < CS_SR_10MHZ)
					i64AnySampleRate = CS_SR_10MHZ;
				return i64AnySampleRate;
			}
			else if (CS_SAMPLE_BITS_12 == m_csSystemInfo.u32SampleBits)
			{ // treat like CS12400
				if (CS_MODE_SINGLE == u32Mode)
				{
					if (i64AnySampleRate > CS_SR_420MHZ)
						i64AnySampleRate = CS_SR_420MHZ;
				}
				else
				{
					if (i64AnySampleRate > CS_SR_210MHZ)
						i64AnySampleRate = CS_SR_210MHZ;
				}
				if (i64AnySampleRate < CS_SR_40MHZ)
					i64AnySampleRate = CS_SR_40MHZ;
				return i64AnySampleRate;
			}
			else if (CS_SAMPLE_BITS_16 == m_csSystemInfo.u32SampleBits)
			{ // treat like CS1610
				if (i64AnySampleRate < CS_SR_2KHZ)
					i64AnySampleRate = CS_SR_2KHZ;
				if (i64AnySampleRate > CS_SR_20MHZ)
					i64AnySampleRate = CS_SR_20MHZ;
				return i64AnySampleRate;
			}
			else // must be 14 bits
			{
				//	just check to see that the sample rate falls within the
				//	upper and lower sample rates. If not fall thru to coerce
				//	the rate.
				if ((i64AnySampleRate <= i64UpperSampleRate) && (i64AnySampleRate >= i64LowerSampleRate))
					return i64AnySampleRate;
			}
		}

		// find the closest match
		if (i64AnySampleRate >= i64UpperSampleRate)
		{
			return	i64UpperSampleRate;
		}
		else if (i64AnySampleRate > i64LowerSampleRate)
		{
			int64 i64ToUpper = i64UpperSampleRate - i64AnySampleRate;
			int64 i64ToLower =  i64AnySampleRate - i64LowerSampleRate;

			if (i64ToUpper <= i64ToLower)
				return i64UpperSampleRate;
			else
				return i64LowerSampleRate;
		}

		i = j - 1;
	}

	return i64LowerSampleRate;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int64 CGageDemoSystem::ValidateSampleRate (PCSSYSTEMCONFIG pSysCfg)
{
	PCSACQUISITIONCONFIG pAcqCfg = static_cast<PCSACQUISITIONCONFIG>(pSysCfg->pAcqCfg);
	PCSCHANNELCONFIG pChanCfg = static_cast<PCSCHANNELCONFIG>(pSysCfg->pAChannels->aChannel);
	int64	i64CoerceSampleRate = pAcqCfg->i64SampleRate;

	i64CoerceSampleRate = CoerceSampleRate(pAcqCfg->i64SampleRate, pAcqCfg->u32ExtClk, pAcqCfg->u32Mode);

	if ( i64CoerceSampleRate == CS_SR_200MHZ ||
		 i64CoerceSampleRate == CS_SR_160MHZ ||
		 (pAcqCfg->u32ExtClk > 0 && i64CoerceSampleRate > CS_SR_100MHZ))
	{
		for (uInt32 k = 0; k < pSysCfg->pAChannels->u32ChannelCount; k ++, pChanCfg++)
		{
			if (pChanCfg->u32Term & CS_DIRECT_ADC_INPUT)
			{
				 i64CoerceSampleRate = CS_SR_100MHZ;
				 break;
			}
		}
	}
	return  i64CoerceSampleRate;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::ValidateNumberOfTriggerEnabled(PARRAY_TRIGGERCONFIG pATriggers,
														PCSSYSTEMINFO pSysInfo,
														uInt32 u32Coerce)
{
	PCSTRIGGERCONFIG pTrigCfg = pATriggers->aTrigger;
	uInt32	u32NumOfTriggerEnabled = 0;

	for (uInt32 i = 0; i < pATriggers->u32TriggerCount; i++)	//----- True if u32TriggerCount is the counter of the enabled triggers
																//----- meaning for CS14160 we can have up to 2 engines so 0, 1
																//----- triggerIndex will tell us which brd should triggered
	{
		if (CS_TRIG_SOURCE_DISABLE != pTrigCfg[i].i32Source)
			u32NumOfTriggerEnabled ++;
	}

	if (u32NumOfTriggerEnabled > (pSysInfo->u32TriggerMachineCount / pSysInfo->u32BoardCount))
	{
		if (u32Coerce)
		{
			for (uInt32 i = 0; i < pATriggers->u32TriggerCount; i++)
			{
				if ( CS_TRIG_SOURCE_DISABLE != pTrigCfg[i].i32Source )
				{
					if ( u32NumOfTriggerEnabled < 2 )
						u32NumOfTriggerEnabled ++;
					else
					{
						// Diable all the remaining trigge engines
						pTrigCfg[i].i32Source = CS_TRIG_SOURCE_DISABLE;
					}
				}
			}
			return CS_CONFIG_CHANGED;
		}
		else
		{
			return CS_INVALID_TRIGGER_ENABLED;
		}
	}
	else
		return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::ValidateTriggerSources(PARRAY_TRIGGERCONFIG pATriggers,
											  PCSSYSTEMINFO pSysInfo,
											  uInt32 u32Coerce)
{
	PCSTRIGGERCONFIG pTrigCfg = pATriggers->aTrigger;
	uInt32	CardIndex1 = 0;
	uInt32	CardIndex2 = 0;
	uInt32	TrigEnabledIndex1 = 0;
	uInt32	TrigEnabledIndex2 = 0;

	// Number of trigger machine per card
	uInt32 NbTrigEngines = pSysInfo->u32TriggerMachineCount / pSysInfo->u32BoardCount;

	// Looking for 2 trigger engines enabled
	uInt32 i;
	for (i = 0; i < pATriggers->u32TriggerCount; i++)	//----- True if u32TriggerCount is the counter of the enabled triggers
														//----- meaning for CS14160 we can have up to 2 engines so 0, 1
														//----- triggerIndex will tell us which brd should triggered
	{
		if (CS_TRIG_SOURCE_DISABLE != pTrigCfg[i].i32Source)
		{
			if (0 == CardIndex1)
			{
				TrigEnabledIndex1 = i;
				CardIndex1 = (i + NbTrigEngines - 1) / NbTrigEngines;
			}
			else if (0 == CardIndex2)
			{
				TrigEnabledIndex2 = i;
				CardIndex2 = (i + NbTrigEngines - 1) / NbTrigEngines;
			}
		}
	}
	// Make sure 2 enabled trigger engines come from the same card
	if (0 != CardIndex2 && 0 != CardIndex2)
	{
		if (CardIndex1 != CardIndex2)
		{
			if (u32Coerce)
			{
				for (i = 0; i < pATriggers->u32TriggerCount; i++)
				{
					// Disable the second trigger engine
					if (i == TrigEnabledIndex2)
						pTrigCfg[i].i32Source = CS_TRIG_SOURCE_DISABLE;
				}
				return CS_CONFIG_CHANGED;
			}
			else
			{
				return CS_NOT_TRIGGER_FROM_SAME_CARD;
			}
		}
	}
	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::ValidateParams(uInt32 u32ParamID, uInt32 u32Coerce, void *ParamBuffer)
{
	int32		i32Status = CS_SUCCESS;
	int32		i32CoerceStatus = CS_SUCCESS;

	PCSSYSTEMINFO pSysInfo = &m_csSystemInfo;

	// Probably not needed, but here as an extra check
	if (!pSysInfo )
		return CS_INVALID_HANDLE;

	// Validate Buffer for Read/Write
	i32Status = ValidateBufferForReadWrite(u32ParamID, ParamBuffer, TRUE);
	if (CS_SUCCESS != i32Status)
		return i32Status;

	//----- Validate configuration Params
	switch (u32ParamID)
	{
		case CS_SYSTEM:
		{
			PCSSYSTEMCONFIG pSysCfg = static_cast<PCSSYSTEMCONFIG>(ParamBuffer);

			//----- ACQUISITION
			i32Status = CsDrvValidateAcquisitionConfig (pSysCfg, pSysInfo, u32Coerce);
			if (CS_FAILED(i32Status))
				return i32Status;

			if ((0 != u32Coerce) && (CS_CONFIG_CHANGED != i32CoerceStatus))
				i32CoerceStatus = i32Status;

			i32Status = ValidateNumberOfTriggerEnabled(pSysCfg->pATriggers, pSysInfo, u32Coerce);
			if (CS_FAILED(i32Status))
				return i32Status;

			if ((0 != u32Coerce) && (CS_CONFIG_CHANGED != i32CoerceStatus))
				i32CoerceStatus = i32Status;

			i32Status = ValidateTriggerSources(pSysCfg->pATriggers, pSysInfo, u32Coerce);
			if (CS_FAILED(i32Status))
				return i32Status;

			if ((0 != u32Coerce) && (CS_CONFIG_CHANGED != i32CoerceStatus))
				i32CoerceStatus = i32Status;

			//----- TRIGGER
			i32Status = CsDrvValidateTriggerConfig(pSysCfg, pSysInfo, u32Coerce);
			if (CS_FAILED(i32Status))
				return i32Status;

			if ((0 != u32Coerce) && (CS_CONFIG_CHANGED != i32CoerceStatus))
				i32CoerceStatus = i32Status;

			//----- CHANNEL
			i32Status = CsDrvValidateChannelConfig(pSysCfg, pSysInfo, u32Coerce);
			if (CS_FAILED(i32Status))
				return i32Status;

			if ((0 != u32Coerce) && (CS_CONFIG_CHANGED != i32CoerceStatus))
				i32CoerceStatus = i32Status;

			if ((0 != u32Coerce) && (CS_CONFIG_CHANGED != i32CoerceStatus))
				i32CoerceStatus = i32Status;
		}
		break;

	default:
		return CS_INVALID_PARAMS_ID;

	}		// end of switch

	if (u32Coerce)
		return i32CoerceStatus;
	else
		return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::ValidateBufferForReadWrite(uInt32 u32ParamID, void *ParamBuffer, BOOL bRead)
{
	BOOL	bTestStructSize = TRUE;
	uInt32	ParamBuffSize;
	BOOL	bBadPtr = FALSE;

	switch (u32ParamID)
	{
		case CS_SYSTEM:
		{
			PCSSYSTEMCONFIG pSysCfg = static_cast<PCSSYSTEMCONFIG>(ParamBuffer);

			if (!(pSysCfg->pAcqCfg && pSysCfg->pAChannels && pSysCfg->pATriggers))
				return CS_NULL_POINTER;

			if (bRead)
			{
				// Validate all pointers to application buffer for Read operation
				bBadPtr |= IsBadReadPtr(pSysCfg->pAcqCfg, sizeof(CSACQUISITIONCONFIG));
				bBadPtr |= IsBadReadPtr(pSysCfg->pAChannels, sizeof(ARRAY_CHANNELCONFIG) +
								(pSysCfg->pAChannels->u32ChannelCount-1)* sizeof(CSCHANNELCONFIG));
				bBadPtr |= IsBadReadPtr(pSysCfg->pATriggers, sizeof(ARRAY_TRIGGERCONFIG) +
								(pSysCfg->pATriggers->u32TriggerCount-1)* sizeof(CSTRIGGERCONFIG));
			}
			else
			{
				// Validate all pointers to application buffer for Read operation
				bBadPtr |= IsBadWritePtr(pSysCfg->pAcqCfg, sizeof(CSACQUISITIONCONFIG));
				bBadPtr |= IsBadWritePtr(pSysCfg->pAChannels, sizeof(ARRAY_CHANNELCONFIG) +
								(pSysCfg->pAChannels->u32ChannelCount-1)* sizeof(CSCHANNELCONFIG));
				bBadPtr |= IsBadWritePtr(pSysCfg->pATriggers, sizeof(ARRAY_TRIGGERCONFIG) +
								(pSysCfg->pATriggers->u32TriggerCount-1)* sizeof(CSTRIGGERCONFIG));
			}
			if (! bBadPtr) // always false for linux
			{
				if (pSysCfg->u32Size != sizeof(CSSYSTEMCONFIG))
					return CS_INVALID_STRUCT_SIZE;

				for (uInt32 i = 0; i< pSysCfg->pATriggers->u32TriggerCount; i++)
				{
					if (pSysCfg->pATriggers->aTrigger[i].u32Size != sizeof(CSTRIGGERCONFIG))
						return CS_INVALID_STRUCT_SIZE;
				}
				for (uInt32 i = 0; i< pSysCfg->pAChannels->u32ChannelCount; i++)
				{
					if (pSysCfg->pAChannels->aChannel[i].u32Size != sizeof(CSCHANNELCONFIG))
					{
						return CS_INVALID_STRUCT_SIZE;
					}
				}
				return CS_SUCCESS;
			}
			else
				return CS_INVALID_POINTER_BUFFER;

		}

		case CS_ACQUISITION:
		{
			PCSACQUISITIONCONFIG pAcqCfg = static_cast<PCSACQUISITIONCONFIG>(ParamBuffer);
			ParamBuffSize = sizeof(CSACQUISITIONCONFIG);

			// Validate pointers to the application buffer for Read/Write operation
			if (bRead)
				bBadPtr = IsBadReadPtr(ParamBuffer, ParamBuffSize);
			else
				bBadPtr = IsBadWritePtr(ParamBuffer, ParamBuffSize);

			if (bBadPtr)
				return CS_INVALID_POINTER_BUFFER;
			else
			{
				if (pAcqCfg->u32Size != sizeof(CSACQUISITIONCONFIG))
					return CS_INVALID_STRUCT_SIZE;
				else
					return CS_SUCCESS;
			}
		}

		case CS_TRIGGER:
		{
			PARRAY_TRIGGERCONFIG pATrigCfg = static_cast<PARRAY_TRIGGERCONFIG>(ParamBuffer);
			ParamBuffSize = (pATrigCfg->u32TriggerCount-1) * sizeof(CSTRIGGERCONFIG) + sizeof(ARRAY_TRIGGERCONFIG);

			// Validate pointers to the application buffer for Read/Write operation
			if (bRead)
				bBadPtr = IsBadReadPtr(pATrigCfg->aTrigger, ParamBuffSize);
			else
				bBadPtr = IsBadWritePtr(pATrigCfg->aTrigger, ParamBuffSize);

			if (bBadPtr)
				return CS_INVALID_POINTER_BUFFER;
			else
			{
				for (uInt32 i = 0; i< pATrigCfg->u32TriggerCount; i++)
				{
					if (pATrigCfg->aTrigger[i].u32Size != sizeof(CSTRIGGERCONFIG))
						return CS_INVALID_STRUCT_SIZE;
				}
				return CS_SUCCESS;
			}
		}

		case CS_CHANNEL:
		{
			PARRAY_CHANNELCONFIG pAChanCfg = static_cast<PARRAY_CHANNELCONFIG>(ParamBuffer);
			ParamBuffSize = (pAChanCfg->u32ChannelCount-1) * sizeof(CSCHANNELCONFIG) + sizeof(ARRAY_CHANNELCONFIG);

			// Validate pointers to the application buffer for Read/Write operation
			if ( bRead )
				bBadPtr = IsBadReadPtr(pAChanCfg->aChannel, ParamBuffSize);
			else
				bBadPtr = IsBadWritePtr(pAChanCfg->aChannel, ParamBuffSize);

			if (bBadPtr)
				return CS_INVALID_POINTER_BUFFER;
			else
			{
				for (uInt32 i = 0; i< pAChanCfg->u32ChannelCount; i++)
				{
					if (pAChanCfg->aChannel[i].u32Size != sizeof(CSCHANNELCONFIG))
					{
						return CS_INVALID_STRUCT_SIZE;
					}
				}
				return CS_SUCCESS;
			}
		}

		case CS_BOARD_INFO:
		{
			PARRAY_BOARDINFO pABoardInfo = static_cast<PARRAY_BOARDINFO>(ParamBuffer);
			ParamBuffSize = (pABoardInfo->u32BoardCount-1) * sizeof(CSBOARDINFO) + sizeof(ARRAY_BOARDINFO);

			// Validate pointers to the application buffer for Read/Write operation
			if (bRead)
				bBadPtr = IsBadReadPtr(pABoardInfo->aBoardInfo, ParamBuffSize);
			else
				bBadPtr = IsBadWritePtr(pABoardInfo->aBoardInfo, ParamBuffSize);

			if (bBadPtr)
				return CS_INVALID_POINTER_BUFFER;
			else
			{
				for (uInt32 i = 0; i< pABoardInfo->u32BoardCount; i++)
				{
					if (pABoardInfo->aBoardInfo[i].u32Size != sizeof(CSBOARDINFO))
						return CS_INVALID_STRUCT_SIZE;
				}
				return CS_SUCCESS;
			}
		}

		case CS_READ_NVRAM:
		case CS_WRITE_NVRAM:
		{
			ParamBuffSize = sizeof(NVRAM_IMAGE);
			bTestStructSize = FALSE;
		}
		break;

		case CS_DELAY_LINE_MODE:
		{
			ParamBuffSize = sizeof(CS_PLX_DELAYLINE_STRUCT);
			bTestStructSize = FALSE;
		}
		break;

		case CS_MEM_READ_THRU:
		case CS_MEM_WRITE_THRU:
		{
			ParamBuffSize = sizeof(CS_MEM_TEST_STRUCT);
			bTestStructSize = FALSE;
		}
		break;

		case CS_EXTENDED_BOARD_OPTIONS:
		{
			ParamBuffSize = sizeof(int64);
			bTestStructSize = FALSE;
		}
		break;


		case CS_READ_EEPROM:
		case CS_WRITE_EEPROM:
		// The memory has been checked before getting here. When getting here
		// the pointer and size should be correct
			return CS_SUCCESS;
			break;

		case CS_READ_FLASH:
		case CS_WRITE_FLASH:
		{
			ParamBuffSize = sizeof(CS_GENERIC_FLASH_READWRITE);
		}
		break;

		case CS_SEND_DAC:
		{
			ParamBuffSize = sizeof(CS_SENDDAC_STRUCT);
		}
		break;

		case CS_UPDATE_CALIB_TABLE:
		{
			ParamBuffSize = sizeof(CS_UPDATE_CALIBTABLE_STRUCT);
		}
		break;

		case CS_WRITE_DELAY_LINE:
		{
			ParamBuffSize = sizeof(CS_DELAYLINE_STRUCT);
		}
		break;

		case CS_CALIBMODE_CONFIG:
		{
			ParamBuffSize = sizeof(CS_CALIBMODE_PARAMS);
		}
		break;

		case CS_TRIGADDR_OFFSET_ADJUST:
		{
			ParamBuffSize = sizeof(CS_TRIGADDRESS_OFFSET_STRUCT);
		}
		break;

		case CS_TRIGLEVEL_ADJUST:
		{
			ParamBuffSize = sizeof(CS_TRIGLEVEL_OFFSET_STRUCT);
		}
		break;

		case CS_TRIGGAIN_ADJUST:
		{
			ParamBuffSize = sizeof(CS_TRIGGAIN_STRUCT);
		}
		break;

		case CS_FIR_CONFIG:
		{
			ParamBuffSize = sizeof(CS_FIR_CONFIG_PARAMS);
		}
		break;

		case CS_SYSTEMINFO_EX:
		{
			ParamBuffSize = sizeof(CSSYSTEMINFOEX);
		}
		break;

		case CS_FLASHING_LED:
		case CS_SELF_TEST_MODE:
		case CS_TRIG_OUT:
		case CS_ADDON_RESET:
		case CS_TRIG_ENABLE:

			//Nothing to validate. Return success
			return CS_SUCCESS;

		default:
			return CS_INVALID_REQUEST;
	}

	if (bRead)
		bBadPtr = IsBadReadPtr(ParamBuffer, ParamBuffSize);
	else
		bBadPtr = IsBadWritePtr(ParamBuffer, ParamBuffSize);

	if (bBadPtr)
		return CS_INVALID_POINTER_BUFFER;

	else if (bTestStructSize)
	{
		// For structtures which have u32Size member variable
		// This variable must be the first member (offset = 0 )
		// It is safe to access and read from this address using *uInt32 pointer
		if (*(static_cast<uInt32 *>(ParamBuffer)) != ParamBuffSize)
			return CS_INVALID_STRUCT_SIZE;
	}
    return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::Do(uInt32 u32ActionID, void *pActionBuffer)
{

	switch (u32ActionID)
	{
		case ACTION_START:
			return StartAcquisition();

		case ACTION_FORCE:
			return ForceTrigger();

		case ACTION_ABORT:
			return Abort();

		case ACTION_RESET:
		{
			// Delete everything and recreate the signals
			DEMOSIGNALVECTOR::iterator theIterator;
			for (theIterator = m_vecSignals.begin(); theIterator  != m_vecSignals.end(); theIterator++)
			{
				CBaseSignal* pBs = (*theIterator);
				delete pBs;
			}

			m_vecSignals.clear();
			delete m_pTrigger;
			m_pTrigger = NULL;

			// Call RefreshSystemSignals to re-read the signal part of
			// the file and then recreate the signals
			ChannelInfo ChanInfo;
			int32 i32Status = m_pGageDemo->RefreshSystemSignals(m_csHandle, ChanInfo);
			if (CS_FAILED(i32Status))
				return i32Status;
			if (i32Status != static_cast<int32>(m_csSystemInfo.u32ChannelCount))
				return CS_INVALID_CHANNEL;

			CBaseSignal* pBs(NULL);

			string strSysInfo = _T("");

			string strNode = _T("\\");
			strNode.append(m_strSysName);
			strNode.append(_T("\\"));

			if (CS_SAMPLE_BITS_8 == m_csSystemInfo.u32SampleBits)
			{
				strSysInfo = XML_8_BIT_NODE;
			}
			else if (CS_SAMPLE_BITS_12 == m_csSystemInfo.u32SampleBits)
			{
				strSysInfo = XML_12_BIT_NODE;
			}
			else if (CS_SAMPLE_BITS_14 == m_csSystemInfo.u32SampleBits)
			{
				strSysInfo = XML_14_BIT_NODE;
			}
			else if (CS_SAMPLE_BITS_16 == m_csSystemInfo.u32SampleBits)
			{
				strSysInfo = XML_16_BIT_NODE;
			}
			strSysInfo.append(strNode);

			for (uInt32 i = 0; i < m_csSystemInfo.u32ChannelCount; i++)
			{
				TCHAR tcChanName[25];
				string str = strSysInfo;
				_stprintf_s(tcChanName, _countof(tcChanName), _T("Chan%02lu"), i+1 );
				str.append(tcChanName);

				switch( ChanInfo[i] )
				{
					default:

					case echSine:     pBs = new CSineWave(str); break;
					case echSquare:   pBs = new CSquareWave(str); break;
					case echTriangle: pBs = new CTriangleWave(str); break;
					case echTwoTone:  pBs = new CTwoTone(str); break;
				}
				pBs->ReadParams();
				m_vecSignals.push_back(pBs);
			}
			SetAcquisitionDefaults();
			SetChannelDefaults();
			SetTriggerEngineDefaults();
			m_pTrigger = new CTrigger(m_CapsInfo.uTriggerSense);
			m_pTrigger->SetChannelSource(GetChannelSource());
			m_pTrigger->SetExternalSource(GetExternalSource());
			m_pTrigger->SetHoldoff(m_csSystemConfig.pAcqCfg->i64TriggerHoldoff);
			m_pTrigger->SetDelay(m_csSystemConfig.pAcqCfg->i64TriggerDelay);
			m_pTrigger->SetCondition(GetTriggerSlope(), GetTriggerRange(), GetTriggerLevel());

			return CS_SUCCESS;
		}

		case ACTION_COMMIT:
		{
			int32 i32Status;
			i32Status = SetParams(CS_SYSTEM, pActionBuffer);
			if (i32Status == CS_SUCCESS)
			{
				// Keep a latest copy of Valid AcquisitionConfig. The parameters
				// may have changed in ValidateParameters

				i32Status = GetParams(CS_ACQUISITION, m_csSystemConfig.pAcqCfg);
			}
			if (i32Status == CS_SUCCESS)
			{
				m_pTrigger->SetChannelSource(GetChannelSource());
				m_pTrigger->SetExternalSource(GetExternalSource());
				m_pTrigger->SetHoldoff(m_csSystemConfig.pAcqCfg->i64TriggerHoldoff);
				m_pTrigger->SetDelay(m_csSystemConfig.pAcqCfg->i64TriggerDelay);
				m_pTrigger->SetCondition(GetTriggerSlope(), GetTriggerRange(), GetTriggerLevel());
				for (DEMOSIGNALVECTOR::iterator theIterator = m_vecSignals.begin(); theIterator  != m_vecSignals.end(); theIterator++)
				{
					(*theIterator)->SetSampleRate( double(m_csSystemConfig.pAcqCfg->i64SampleRate) );
				}
			}
			return i32Status;
		}
		break;

	case ACTION_TIMESTAMP_RESET:
		m_bManualResetTimeStamp = true;
		return CS_SUCCESS;

	case ACTION_CALIB:
		return CS_SUCCESS;

	default:
		return CS_INVALID_REQUEST;
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::TransferData(IN_PARAMS_TRANSFERDATA InParams, POUT_PARAMS_TRANSFERDATA OutParams)
{
#ifdef __linux__
	EVENT_HANDLE h = (EVENT_HANDLE)InParams.hNotifyEvent;
#else
	HANDLE h = InParams.hNotifyEvent;
#endif
	//Validate InParams
	if( InParams.u32Segment > m_veTriggerTime.size() )
	{
		if (NULL != h)
		{
			SetEvent( h );
		}		
		return CS_INVALID_SEGMENT;
	}

	if ( InParams.u16Channel >  m_vecSignals.size() )
	{
		if (NULL != h)
		{
			SetEvent( h );
		}		
		return CS_INVALID_CHANNEL;
	}

    if ( InParams.u16Channel < 1 )
	{
		if (NULL != h)
		{
			SetEvent( h );
		}		
		return CS_INVALID_CHANNEL;
	}

	if (TxMODE_TIMESTAMP != InParams.u32Mode)
	{
		if( InParams.i64StartAddress <  -m_csSystemConfig.pAcqCfg->i64TriggerHoldoff )
			InParams.i64StartAddress =  -m_csSystemConfig.pAcqCfg->i64TriggerHoldoff;

		if( InParams.i64StartAddress < (m_csSystemConfig.pAcqCfg->i64Depth + m_csSystemConfig.pAcqCfg->i64TriggerDelay) - m_csSystemConfig.pAcqCfg->i64SegmentSize )
			InParams.i64StartAddress =  (m_csSystemConfig.pAcqCfg->i64Depth + m_csSystemConfig.pAcqCfg->i64TriggerDelay) - m_csSystemConfig.pAcqCfg->i64SegmentSize;

		if( InParams.i64StartAddress > (m_csSystemConfig.pAcqCfg->i64Depth + m_csSystemConfig.pAcqCfg->i64TriggerDelay))
			InParams.i64StartAddress = m_csSystemConfig.pAcqCfg->i64Depth + m_csSystemConfig.pAcqCfg->i64TriggerDelay;

		if( InParams.i64StartAddress + InParams.i64Length > (m_csSystemConfig.pAcqCfg->i64Depth + m_csSystemConfig.pAcqCfg->i64TriggerDelay))
			InParams.i64Length = m_csSystemConfig.pAcqCfg->i64Depth + m_csSystemConfig.pAcqCfg->i64TriggerDelay - InParams.i64StartAddress;

		if( InParams.i64Length > m_csSystemConfig.pAcqCfg->i64SegmentSize)
			InParams.i64Length = m_csSystemConfig.pAcqCfg->i64SegmentSize;

		if( m_csSystemConfig.pAcqCfg->i64SegmentSize < (m_csSystemConfig.pAcqCfg->i64Depth + m_csSystemConfig.pAcqCfg->i64TriggerHoldoff))
		{
			if (InParams.i64StartAddress <= (-m_csSystemConfig.pAcqCfg->i64TriggerHoldoff))
				InParams.i64StartAddress = -(m_csSystemConfig.pAcqCfg->i64SegmentSize - m_csSystemConfig.pAcqCfg->i64Depth);
		}
	}
	if( InParams.i64Length < 1 )
		InParams.i64Length = 1;

	if (NULL == InParams.pDataBuffer)
	{
		OutParams->i64ActualLength = InParams.i64Length;
		OutParams->i64ActualStart = InParams.i64StartAddress;
		return CS_SUCCESS;
	}

	size_t stBufferSize;
	if ( TxMODE_TIMESTAMP == InParams.u32Mode )
		stBufferSize = size_t(InParams.i64Length) * sizeof(int64);
	else
		stBufferSize = size_t(InParams.i64Length) * m_csSystemInfo.u32SampleSize;

	if( ::IsBadWritePtr( InParams.pDataBuffer, stBufferSize ) )
	{
		if (NULL != InParams.hNotifyEvent)
		{
			SetEvent( h );
		}
		return CS_BUFFER_TOO_SMALL;
	}


    if( WAIT_OBJECT_0 == WaitForSingleObject ( m_hAbortTransferEvent, 0 ) )
	{
		ZeroMemory( InParams.pDataBuffer, stBufferSize );
		if (NULL != InParams.hNotifyEvent)
		{
			SetEvent( h );			
		}

		ResetEvent(m_hAbortTransferEvent);
		return CS_SUCCESS;
	}

	if (TxMODE_TIMESTAMP == InParams.u32Mode)
	{
		int64 i64TickFrequency;
		if (m_csSystemConfig.pAcqCfg->u32TimeStampConfig & TIMESTAMP_MCLK)
			i64TickFrequency = 133000000;
		else
			i64TickFrequency = m_csSystemConfig.pAcqCfg->i64SampleRate;

		int64* pBuffer = static_cast<int64*>(InParams.pDataBuffer);

		double dTimeStampStart = 0.;
		// If we're set to TIMESTAMP_SEG_RESET or the ManualResetTimeStamp flag is
		// set, we'll set dTimeStampStart to the first trigger time. This way when
		// we subtract it fromt the trigger times the first one we'll be 0.
		if (m_bManualResetTimeStamp || (0 == (m_csSystemConfig.pAcqCfg->u32TimeStampConfig & TIMESTAMP_FREERUN)))
			dTimeStampStart = m_veTriggerTime.at(0);

		for (uInt32 i = InParams.u32Segment; i < InParams.i64Length; i++)
		{
			// The trigger time stamps are differences. That is, timestamp 1 =
			// timestamp(0) + m_veTriggerTime.at(1)
			double dTickCount = 0;
			for (uInt32 j = 0; j < i; j++)
				dTickCount += m_veTriggerTime.at(j);

			pBuffer[i] = static_cast<int64>(static_cast<double>(i64TickFrequency) * dTickCount);
		}
		OutParams->i32LowPart = static_cast<int32>(i64TickFrequency);
		// Reset the ManualResetTimeStamp flag back to false for the next transfer
		// Check... this might be a problem if someone reads the tmb data for the
		// same capture twice
		m_bManualResetTimeStamp = false;
		return CS_SUCCESS;
	}
	else if ((TxMODE_DEFAULT == InParams.u32Mode) || (TxMODE_SLAVE == InParams.u32Mode))
	{
		/*if (NULL != InParams.hNotifyEvent)
		{
			// Fill these in at the start because they'll always be these values
			// RICKY - CHECK IF WE NEED THESE SET TO INTERMEDIATE VALUES
			OutParams->i64ActualLength = InParams.i64Length;
			OutParams->i64ActualStart = InParams.i64StartAddress;

			// Use GlobalAlloc to allocate the structure and free it at the end of
			// the thread with GlobalFree.
			ThreadTransferStruct* pTransferStruct = static_cast<ThreadTransferStruct*>(::GlobalAlloc(GPTR, sizeof(ThreadTransferStruct)));
			if (NULL == pTransferStruct)
				return CS_MEMORY_ERROR;

			pTransferStruct->InParams = InParams;
			pTransferStruct->pSystem = this;

			::_beginthreadex(NULL, 0, AsynchronousTransferThread, (void *)pTransferStruct, 0, NULL);

			return CS_SUCCESS;
		}*/
		CBaseSignal* pSignal = m_vecSignals.at(InParams.u16Channel - 1);

		pSignal->SeedNoiseIndex(InParams.u32Segment-1);

		double dTime = GetTriggerTime(InParams.u32Segment-1);

		double dTbs = 1.0 / double(GetSampleRate());

		dTime += double(InParams.i64StartAddress) * dTbs;

		double dSampleRes = double(m_csSystemConfig.pAcqCfg->i32SampleRes);
		double dOff = double(m_csSystemConfig.pAChannels->aChannel[InParams.u16Channel - 1].i32DcOffset) / 1.e3;
		double dInputRange = double(m_csSystemConfig.pAChannels->aChannel[InParams.u16Channel - 1].u32InputRange) / 2.e3;

		double dImpedanceFactor = 2. * double(m_csSystemConfig.pAChannels->aChannel[InParams.u16Channel - 1].u32Impedance) / (50. + m_csSystemConfig.pAChannels->aChannel[InParams.u16Channel - 1].u32Impedance);
		double dLow = dOff - dInputRange;
		double dHigh = dOff + dInputRange;

		double dScaleAdjust = (dSampleRes > 0) ? 1 : -1;
		double dScale = (dSampleRes-dScaleAdjust)/dInputRange;

		if (CS_SAMPLE_BITS_8 == m_csSystemInfo.u32SampleBits)
		{
			int8 i8SampleOff = int8(m_csSystemConfig.pAcqCfg->i32SampleOffset);

			uInt8* pBuffer = static_cast<uInt8*>(InParams.pDataBuffer);
			for (int64 i = 0; i < InParams.i64Length; i++)
			{
				double dVoltage = dImpedanceFactor * pSignal->GetVoltage(dTime);
				dTime += dTbs;

				dVoltage = min ( dHigh, max(dLow, dVoltage));
				dVoltage -= dOff;
				int32 i32Value = int32(i8SampleOff - dVoltage * dScale);
				if ( 0 > i32Value)
					i32Value = 0;
				if (UCHAR_MAX < i32Value)
					i32Value = UCHAR_MAX;
				pBuffer[i] = uInt8(i32Value);
			}
		}
		else
		{
			int16 i16SampleOff = int16(m_csSystemConfig.pAcqCfg->i32SampleOffset);

			int16* pBuffer = static_cast<int16*>(InParams.pDataBuffer);
			uInt32 u32Mask;
			if (CS_SAMPLE_BITS_12 == m_csSystemInfo.u32SampleBits)
				u32Mask = 0xFFFFFFF0;
			else if (CS_SAMPLE_BITS_14 == m_csSystemInfo.u32SampleBits)
				u32Mask = 0xFFFFFFFC;
			else // 16 bit
				u32Mask = 0xFFFFFFFF;


			for (int64 i = 0; i < InParams.i64Length; i++)
			{
				double dVoltage = dImpedanceFactor * pSignal->GetVoltage(dTime);
				dTime += dTbs;
				dVoltage = min ( dHigh, max(dLow, dVoltage));
				dVoltage -= dOff;
				int32 i32Value = int32(i16SampleOff) - int16(dVoltage * dScale);
				if (SHRT_MIN > i32Value)
					i32Value = SHRT_MIN;
				if (SHRT_MAX < i32Value)
					i32Value = SHRT_MAX;

				pBuffer[i] = int16(i32Value);
				pBuffer[i] &= u32Mask;
			}
		}

		OutParams->i64ActualLength = InParams.i64Length;
		OutParams->i64ActualStart = InParams.i64StartAddress;

		return CS_SUCCESS;
	}
	else
	{
		if (NULL != InParams.hNotifyEvent)
		{
//			SetEvent( *InParams.hNotifyEvent );
			SetEvent( h );
		}
		return CS_INVALID_TRANSFER_MODE;
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::GetAsyncTransferDataResult(uInt8 Channel, CSTRANSFERDATARESULT* pTxDataResult, BOOL bWait)
{
	Channel = Channel;
	pTxDataResult->i32Status = m_csAsyncTransferResult.i32Status;
	pTxDataResult->i64BytesTransfered = m_csAsyncTransferResult.i64BytesTransfered;
	bWait = bWait;
	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemoSystem::RegisterEventHandle(uInt32 u32EventType, EVENT_HANDLE *EventHandle)
{
	// RICKY - For now asynchronous transfer is not supported, but we still
	// need the events to be registered.

	if (ACQ_EVENT_TRIGGERED == u32EventType)
	{
		m_hAcqTriggeredEvent = *EventHandle;
		return CS_SUCCESS;
	}
	else if (ACQ_EVENT_END_BUSY == u32EventType)
	{
		m_hAcqEndBusyEvent = *EventHandle;
		return CS_SUCCESS;
	}
	return CS_DRIVER_ASYNC_NOT_SUPPORTED;
}
