#include "GageSystem.h"
#include "qthread.h"
#include "CsTestFunc.h"

#include <QDebug>

////qDebug() << "sample comment";

//#include <tchar.h>
//#include <assert.h>

//Initialisation of the static attributes
//RG const CSSYSTEMINFO CGageSystem::EmptySystemInfo = {0};
//RG const CSACQUISITIONCONFIG CGageSystem::EmptyAcquisitionConfig = {0};

using namespace CsTestFunc;

CGageSystem::CGageSystem() : m_hSystem(0), 
							 m_bExternalClock(false), 
							 m_bMultipleRecord(false), 
							 m_bDcOffset(false), 
							 m_bCouplingDC(false),
							 m_bCouplingAC(false),
							 m_bImpedance50(false),
							 m_bImpedance1M(false),
							 m_bExternalTrigger(false),
							 m_bTriggerCouplingDC(false),
							 m_bTriggerCouplingAC(false),
							 m_bTriggerImpedance50(false),
							 m_bTriggerImpedanceHZ(false),
							 m_bTriggerRange1V(false),
                             m_bTriggerRange5V(false),
                             m_bCoerceFlag(TRUE)					
{
    m_i64Chan_mem_size = 0;
	m_bAbort = false;
	m_strError.clear();
    m_pMainSamplesBuffer = NULL;

	m_EventsTimer = new QElapsedTimer();
    #ifdef Q_OS_WIN
		m_hAcqEndEvent = m_hTrigEvent = 0;
        m_libCsRmDLL.setFileName("CsRmDll.dll");
    #else
        m_libCsRmDLL.setFileName("libCsRmDll.so");
    #endif
    m_libCsRmDLL.load();
    m_pCsRmGetSystemStateByIndex = (LPRMGETSYSTEMSTATEBYINDEX)m_libCsRmDLL.resolve("CsRmGetSystemStateByIndex");
}

CGageSystem::~CGageSystem()
{
    if (!m_pBufferArray.empty()) m_pBufferArray.clear();

    #ifdef Q_OS_WIN
        if(m_pMainSamplesBuffer!=NULL) VirtualFree(m_pMainSamplesBuffer, 0, MEM_RELEASE);
    #else
        munmap(m_pMainSamplesBuffer, m_i64MainSamplesBufferSize);
    #endif
    delete m_EventsTimer;
    m_libCsRmDLL.unload();
}

/*******************************************************************************************
										Getters and Setters
*******************************************************************************************/

CSHANDLE CGageSystem::getSystemHandle()
{
	return m_hSystem;
}

CSSYSTEMINFO CGageSystem::getSystemInfo()
{
	QMutexLocker locker(&m_MutexSystemInfo);
	return m_SystemInfo;
}

CSACQUISITIONCONFIG CGageSystem::getAcquisitionConfig()
{
	QMutexLocker locker(&m_MutexAcquisitionConfig);
	return m_AcquisitionConfig;
}

/*
CSSYSTEMINFOEX CGageSystem::getSystemInfoEx()
{
	QMutexLocker locker(&m_MutexSystemInfo);
	return m_SystemInfoEx;
}

uInt32 CGageSystem::getHardwareOptions()
{
	return m_SystemInfoEx.u32AddonHwOptions;
}
*/

int32 CGageSystem::getLastStatusCode()
{
	QMutexLocker locker(&m_MutexLastStatusCode);
	return m_i32LastStatusCode;
}

void CGageSystem::clearErrMess()
{
	m_strError.clear();
}

int32 CGageSystem::GetSystem(int BrdIdx, bool bConfigBrd)
{
	int32 i32Status = CompuscopeGetSystem(&m_hSystem, BrdIdx);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error connecting to the system.", i32Status);
		return i32Status;
	}

	m_SystemInfo.u32Size = sizeof(CSSYSTEMINFO);
	i32Status = CompuscopeGetSystemInfo(m_hSystem, &m_SystemInfo);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error getting system information.", i32Status);
		return i32Status;
	}


	if (!bConfigBrd)
	{
#ifdef Q_OS_WINn
		// Get the Event Handles
		i32Status = CsGetEventHandle(m_hSystem, ACQ_EVENT_TRIGGERED, &m_hTrigEvent);
		if (CS_FAILED(i32Status)) 
		{
			// It is not necessary to return an error and stop working
			// We can still use the polling mechanism
			m_ErrMng->UpdateErrMsg("Error gathering Trigger Event handle.", i32Status);
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		}

		i32Status = CsGetEventHandle(m_hSystem, ACQ_EVENT_END_BUSY, &m_hAcqEndEvent);
		if (CS_FAILED(i32Status)) 
		{
			// It is not necessary to return an error and stop working
			// We can still use the polling mechanism
			m_ErrMng->UpdateErrMsg("Error gathering EndOfAcquisition Event handle.", i32Status);
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		}
#endif
		// Initialize internal variables
		InitStructures();
		m_AcquisitionConfig.u32Size = sizeof(m_AcquisitionConfig);

		i32Status = GatherSystemInformation();
		if (CS_FAILED(i32Status))
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error gathering system information.", i32Status);
			return i32Status;
		}

     	i32Status = GatherSystemCaps();
		if (CS_FAILED(i32Status))
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error gathering system caps.", i32Status);
			return i32Status;
		}

        allocateMemory(m_AcquisitionConfig.u32Mode);
	}
	else
	{
		int CommitTime;
		i32Status = PrepareForCapture(&CommitTime);
		if (CS_FAILED(i32Status))
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error configuring board.", i32Status);
			return i32Status;
		}
	}

	return CS_SUCCESS;
}

void CGageSystem::setSystemHandle(CSHANDLE hSystem)
{
	if(m_hSystem != hSystem)
	{
		FreeSystem();
	}
	
	m_hSystem = hSystem;
}

void CGageSystem::ReleaseSystemHandle(CSHANDLE& hSystem)
{
	hSystem = 0; //Like this it's impossible for the user to use this pointer easily
	m_MutexSystemHandle.unlock();
}

int32 CGageSystem::UpdateSystemInformation()
{
	int i32Status;

	m_SystemInfo.u32Size = sizeof(CSSYSTEMINFO);
	i32Status = CompuscopeGetSystemInfo(m_hSystem, &m_SystemInfo);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error getting system information.", i32Status);
		return i32Status;
	}

	i32Status = GatherSystemInformation();
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error gathering system information.", i32Status);
		return i32Status;
	}

	i32Status = GatherSystemCaps();
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error gathering system caps.", i32Status);
		return i32Status;
	}

	return CS_SUCCESS;
}

int32 CGageSystem::GatherSystemCaps()
{
#define USE_THIS 1

#ifdef nUSE_THIS

	m_pCaps = new CSystemCaps(m_hModule, m_hSystem);

	return CS_SUCCESS;

#else

	int32 i32Status;
	if (m_hSystem)
	{
		i32Status = GetAcquisitionModes();
		if (i32Status<0)
		{
			m_ErrMng->UpdateErrMsg("Error getting acquisition modes.", i32Status);
			return i32Status;
		}

		i32Status = GetSampleRates();
		if (i32Status<0)
		{
			m_ErrMng->UpdateErrMsg("Error getting sampling rates.", i32Status);
			return i32Status;
		}

		GetExtClock();
		GetMultipleRecord();

		i32Status = GetImpedances();
		if (i32Status<0)
		{
			m_ErrMng->UpdateErrMsg("Error getting impedances.", i32Status);
			return i32Status;
		}

		i32Status = GetInputRanges();
		if (i32Status<0)
		{
			m_ErrMng->UpdateErrMsg("Error getting input ranges.", i32Status);
			return i32Status;
		}

		i32Status = GetCouplings();
		if (i32Status<0)
		{
			m_ErrMng->UpdateErrMsg("Error getting couplings.", i32Status);
			return i32Status;
		}

		i32Status = GetFilters();
		if (i32Status<0)
		{
			m_ErrMng->UpdateErrMsg("Error getting filters.", i32Status);
			return i32Status;
		}

		GetDcOffsetCaps();

		//Trigger caps
		i32Status = GetTriggerSources();
		if (i32Status<0)
		{
			m_ErrMng->UpdateErrMsg("Error getting trigger sources.", i32Status);
			return i32Status;
		}

		i32Status = GetTriggerCouplingCaps();
		if (i32Status<0)
		{
			m_ErrMng->UpdateErrMsg("Error getting trigger couplings.", i32Status);
			return i32Status;
		}

		i32Status = GetTriggerImpedanceCaps();
		if (i32Status<0)
		{
			m_ErrMng->UpdateErrMsg("Error getting trigger impedances.", i32Status);
			return i32Status;
		}

		i32Status = GetTriggerRangeCaps();
		if (i32Status<0)
		{
			m_ErrMng->UpdateErrMsg("Error getting trigger ranges.", i32Status);
			return i32Status;
		}

		i32Status = GetExpertCaps();
		if (i32Status<0)
		{
			m_ErrMng->UpdateErrMsg("Error getting expert options.", i32Status);
			return i32Status;
		}
	}

	return CS_SUCCESS;

#endif
}

int32 CGageSystem::GetAcquisitionModes()
{
	// Get all supported modes from the driver 
    uInt32 dwRequiredSize = sizeof(m_u32ModesSupported);
	int32 i32Status = CompuscopeGetSystemCaps(m_hSystem, CAPS_ACQ_MODES, nullptr, &m_u32ModesSupported, &dwRequiredSize);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error getting system caps (acquisition modes).", i32Status);
		return i32Status;
	}

	m_bSingleChan2 = (m_u32ModesSupported & CS_MODE_SINGLE_CHANNEL2);

	// Build the list of modes based on the returned value from the driver
	m_vecModeNames.clear();
	uInt32 u32SuppModes = m_u32ModesSupported & CS_MASKED_MODE;
	uInt32 u32ModeCount = count_bits(u32SuppModes); // we only care about the actual acquisition modes here
	for (uInt32 i = 0, u32Mode = 1, u32ResMode = 0; i < u32ModeCount; i++, u32Mode <<= 1)
	{
		u32ResMode = u32SuppModes & u32Mode;
		if (u32ResMode != 0)
		{
			if (CS_MODE_SINGLE == u32ResMode)
			{
				if (m_bSingleChan2)
				{
					ModeNames mn;
					mn.strName = "Single Chan1";
					mn.u32Mode = CS_MODE_SINGLE;
					m_vecModeNames.push_back(mn);

					mn.strName = "Single Chan2";
					mn.u32Mode = CS_MODE_SINGLE | CS_MODE_SINGLE_CHANNEL2;
					m_vecModeNames.push_back(mn);
				}
				else
				{
					ModeNames mn;
					mn.strName = "Single";
					mn.u32Mode = CS_MODE_SINGLE;
					m_vecModeNames.push_back(mn);
				}
			}
			else if (CS_MODE_DUAL == u32ResMode)
			{
				ModeNames mn;
				mn.strName = "Dual";
				mn.u32Mode = CS_MODE_DUAL;
				m_vecModeNames.push_back(mn);
			}
			else if (CS_MODE_QUAD == u32ResMode)
			{
				ModeNames mn;
				mn.strName = "Quad";
				mn.u32Mode = CS_MODE_QUAD;
				m_vecModeNames.push_back(mn);
			}
			else if (CS_MODE_OCT == u32ResMode)
			{
				ModeNames mn;
				mn.strName = "Octal";
				mn.u32Mode = CS_MODE_OCT;
				m_vecModeNames.push_back(mn);
			}
		}
	}

	return i32Status;
}

int32 CGageSystem::GetSampleRates()
{
	uInt32 u32ModeCount = count_bits(m_u32ModesSupported & CS_MASKED_MODE); // we only care about the actual acquisition modes here
	uInt32 u32CurrentMode = m_AcquisitionConfig.u32Mode;
	int32 i32CurrentTrigSrc = GetSource(1);
    uInt32 dwRequiredSize = 0;
	int32 i32Status = CS_SUCCESS;
	int SrCount;

	if (m_bSingleChan2)
		u32ModeCount++;

	for (uInt32 i = 0, u32Mode = 1; i < u32ModeCount; i++)
	{
		u32Mode = m_vecModeNames[i].u32Mode;
		m_AcquisitionConfig.u32Mode = u32Mode;
		SetSource(1, 1);
		i32Status = CompuscopeSetAcq(m_hSystem, &m_AcquisitionConfig);
		if (CS_FAILED(i32Status))
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error setting acquisition configuration.", i32Status);
			return i32Status;
		}

		i32Status = CompuscopeGetSystemCaps(m_hSystem, CAPS_SAMPLE_RATES, nullptr, NULL, &dwRequiredSize);
		if (CS_FAILED(i32Status))
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error getting buffer size for sample rate caps.", i32Status);
			return i32Status;
		}

		m_mapSRTable[u32Mode].pSRTable = (PCSSAMPLERATETABLE)malloc(dwRequiredSize);
		if (!m_mapSRTable[u32Mode].pSRTable)
		{
			return -32767;
		}

		i32Status = CompuscopeGetSystemCaps(m_hSystem, CAPS_SAMPLE_RATES, nullptr, m_mapSRTable[u32Mode].pSRTable, &dwRequiredSize);
		if (CS_FAILED(i32Status))
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error getting sample rate caps.", i32Status);
			return i32Status;
		}
		m_mapSRTable[u32Mode].nItemCount = dwRequiredSize / sizeof(m_mapSRTable[u32Mode].pSRTable[0]);
		SrCount = m_mapSRTable[u32Mode].nItemCount;
		SrCount = SrCount;
	}
	// set back to what it was
	m_AcquisitionConfig.u32Mode = u32CurrentMode;
	SetSource(1, i32CurrentTrigSrc);
	i32Status = CompuscopeSetAcq(m_hSystem, &m_AcquisitionConfig);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error setting acquisition configuration.", i32Status);
		return i32Status;
	}

	return i32Status;
}

int32 CGageSystem::GetImpedances()
{
	// impedance table
	m_bImpedance1M = false;
	m_bImpedance50 = false;
	uInt32 u32RequiredSize;
	int32 i32Status = CompuscopeGetSystemCaps(m_hSystem, (CAPS_IMPEDANCES | 1), nullptr, NULL, &u32RequiredSize);
	if (CS_FAILED(i32Status)) return i32Status;

    unsigned long dwImpedanceCount = u32RequiredSize / sizeof(CSIMPEDANCETABLE);
	m_ImpedanceTable = new CSIMPEDANCETABLE[dwImpedanceCount];
	m_ImpedanceCount = dwImpedanceCount;
	i32Status = CompuscopeGetSystemCaps(m_hSystem, (CAPS_IMPEDANCES | 1), nullptr, m_ImpedanceTable, &u32RequiredSize);
	if (CS_SUCCEEDED(i32Status))
	{
        for (unsigned long i = 0; i < dwImpedanceCount; i++)
		{
			if (CS_REAL_IMP_1M_OHM == m_ImpedanceTable[i].u32Imdepdance)
			{
				m_bImpedance1M = true;
			}
			if (CS_REAL_IMP_50_OHM == m_ImpedanceTable[i].u32Imdepdance)
			{
				m_bImpedance50 = true;
			}
		}
	}

	return i32Status;
}

int32 CGageSystem::GetInputRanges()
{
	int32 i32Status = CS_SUCCESS;
    unsigned long dwImpedanceCount = m_ImpedanceCount;

    uInt32 dwRequiredSize = 0;

	for (uInt32 i = 0; i < dwImpedanceCount; i++)
	{
		// Get the maximum size
		i32Status = CompuscopeGetSystemCaps(m_hSystem, CAPS_INPUT_RANGES | 1, nullptr, NULL, &dwRequiredSize);
		if (CS_FAILED(i32Status))
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error getting input ranges caps.", i32Status);
			return i32Status;
		}

		// Allocate memory for the Input Ranges table
		m_mapIRTable[m_ImpedanceTable[i].u32Imdepdance].pRangeTable = (PCSRANGETABLE)malloc(dwRequiredSize);
		if (!m_mapIRTable[m_ImpedanceTable[i].u32Imdepdance].pRangeTable)
		{
			m_ErrMng->UpdateErrMsg("Error mapping input range table.", i32Status);
			return -1;
		}

		// Query for the Input Ranges table
		i32Status = CompuscopeGetSystemCaps(m_hSystem, CAPS_INPUT_RANGES | 1, nullptr, m_mapIRTable[m_ImpedanceTable[i].u32Imdepdance].pRangeTable, &dwRequiredSize);
		m_mapIRTable[m_ImpedanceTable[i].u32Imdepdance].nItemCount = dwRequiredSize / sizeof(m_mapIRTable[m_ImpedanceTable[i].u32Imdepdance].pRangeTable[0]);
	}

	return i32Status;
}

int32 CGageSystem::GetCouplings()
{
	// coupling table
	m_bCouplingDC = false;
	m_bCouplingAC = false;
	uInt32 u32RequiredSize = 0;
	int32 i32Status = CompuscopeGetSystemCaps(m_hSystem, (CAPS_COUPLINGS | 1), nullptr, NULL, &u32RequiredSize);
	if (CS_FAILED(i32Status)) return i32Status;

    unsigned long dwSize = u32RequiredSize / sizeof(CSCOUPLINGTABLE);
	m_CouplingTable = new CSCOUPLINGTABLE[dwSize];
	CompuscopeGetSystemCaps(m_hSystem, (CAPS_COUPLINGS | 1), nullptr, m_CouplingTable, &u32RequiredSize);
	if (CS_SUCCEEDED(i32Status))
	{
        for (unsigned long i = 0; i < dwSize; i++)
		{
			if (CS_COUPLING_DC == m_CouplingTable[i].u32Coupling)
			{
				m_bCouplingDC = true;
			}
			if (CS_COUPLING_AC == m_CouplingTable[i].u32Coupling)
			{
				m_bCouplingAC = true;
			}
		}
	}

	return i32Status;
}

int32 CGageSystem::GetFilters()
{
	// filter table
	uInt32 u32RequiredSize = 0;
	int32 i32Status = CompuscopeGetSystemCaps(m_hSystem, (CAPS_FILTERS | 1), nullptr, NULL, &u32RequiredSize);
	if ((i32Status == CS_FUNCTION_NOT_SUPPORTED) || (i32Status == CS_INVALID_REQUEST))
	{
		m_bFilter = false;
		m_FilterTable = new CSFILTERTABLE[16];
		m_FilterTable[0].u32LowCutoff = 0;
		m_FilterTable[0].u32HighCutoff = 0;
		return 0;
	}
    unsigned long dwSize = u32RequiredSize / sizeof(CSFILTERTABLE);
	m_FilterTable = new CSFILTERTABLE[dwSize];
	i32Status = CompuscopeGetSystemCaps(m_hSystem, (CAPS_FILTERS | 1), nullptr, m_FilterTable, &u32RequiredSize);
	if (CS_FAILED(i32Status)) m_bFilter = false;
	else m_bFilter = true;
	return i32Status;
}

int32 CGageSystem::GetTriggerSources()
{
	uInt32	u32Size;
	int32	i32Status = CS_SUCCESS;

	i32Status = CompuscopeGetSystemCaps(m_hSystem, CAPS_TRIGGER_SOURCES, nullptr, NULL, &u32Size);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error getting buffer size for trigger source caps.", i32Status);
		return i32Status;
	}

	i32Status = CompuscopeGetSystemCaps(m_hSystem, CAPS_TRIGGER_SOURCES, nullptr, m_TriggerSource, &u32Size);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error getting trigger source caps.", i32Status);
		return i32Status;
	}

	for (uInt32 i = 0; i < u32Size; i++)
	{
		if (m_TriggerSource[i] < 0)
		{
			m_bExternalTrigger = true;
			break;
		}
	}

	return i32Status;
}

int32 CGageSystem::GetTriggerCouplingCaps()
{
	CSCOUPLINGTABLE ExtTrigCouplingTable[2];

	m_bTriggerCouplingDC = false;
	m_bTriggerCouplingAC = false;

	uInt32 u32Size = sizeof(ExtTrigCouplingTable);

	int32 i32Status = CompuscopeGetSystemCaps(m_hSystem, CAPS_COUPLINGS, nullptr, &ExtTrigCouplingTable, &u32Size);
	if (CS_SUCCEEDED(i32Status))
	{
		uInt32 u32Count = u32Size / sizeof(ExtTrigCouplingTable[0]);
		for (uInt32 i = 0; i < u32Count; i++)
		{
			if (CS_COUPLING_DC == ExtTrigCouplingTable[i].u32Coupling)
			{
				m_bTriggerCouplingDC = true;
			}
			if (CS_COUPLING_AC == ExtTrigCouplingTable[i].u32Coupling)
			{
				m_bTriggerCouplingAC = true;
			}
		}
	}

	return i32Status;
}

int32 CGageSystem::GetTriggerImpedanceCaps()
{
	CSIMPEDANCETABLE ExtTrigImpedanceTable[2];
	uInt32 u32Size = sizeof(ExtTrigImpedanceTable);

	m_bTriggerImpedance50 = false;
	m_bTriggerImpedanceHZ = false;

	int32 i32Status = CompuscopeGetSystemCaps(m_hSystem, CAPS_IMPEDANCES, nullptr, &ExtTrigImpedanceTable, &u32Size);
	if (CS_SUCCEEDED(i32Status))
	{
		uInt32 u32Count = u32Size / sizeof(ExtTrigImpedanceTable[0]);
		for (uInt32 i = 0; i < u32Count; i++)
		{
            qDebug() << "Ext trig impedance" << i << ": " << ExtTrigImpedanceTable[i].u32Imdepdance;
			if (CS_REAL_IMP_50_OHM == ExtTrigImpedanceTable[i].u32Imdepdance)
			{
				m_bTriggerImpedance50 = true;
			}
            else if (CS_REAL_IMP_1M_OHM == ExtTrigImpedanceTable[i].u32Imdepdance)
			{
				m_bTriggerImpedanceHZ = true;
			}
            else if (1000 == ExtTrigImpedanceTable[i].u32Imdepdance)
            {
                m_bTriggerImpedanceHZ = true;
            }
		}
	}

	return i32Status;
}

int32 CGageSystem::GetTriggerRangeCaps()
{
    CSRANGETABLE ExtTrigRangeTable[3];
	uInt32 u32Size = sizeof(ExtTrigRangeTable);

	m_bTriggerRange1V = false;
	m_bTriggerRange5V = false;
    m_bTriggerRange3V3 = false;
    m_bTriggerRangePlus5V = false;

	int32 i32Status = CompuscopeGetSystemCaps(m_hSystem, CAPS_INPUT_RANGES, nullptr, &ExtTrigRangeTable, &u32Size);
	if (CS_SUCCEEDED(i32Status))
	{
		uInt32 u32Count = u32Size / sizeof(ExtTrigRangeTable[0]);
		for (uInt32 i = 0; i < u32Count; i++)
		{
            qDebug() << "Ext trig range" << i << ": " << ExtTrigRangeTable[i].u32InputRange;
			if (CS_GAIN_2_V == ExtTrigRangeTable[i].u32InputRange)
			{
				m_bTriggerRange1V = true;
			}
            else if (CS_GAIN_10_V == ExtTrigRangeTable[i].u32InputRange)
			{
				m_bTriggerRange5V = true;
			}
            else if (3300 == ExtTrigRangeTable[i].u32InputRange)
            {
                m_bTriggerRange3V3 = true;
            }
            else if (CS_GAIN_5_V == ExtTrigRangeTable[i].u32InputRange)
            {
                m_bTriggerRangePlus5V = true;
            }
		}
	}

	return i32Status;
}

int32 CGageSystem::GetExtClock()
{
	if (CsIsLegacyBoard(m_SystemInfo.u32BoardType))
	{
		m_bExternalClock = (m_SystemInfoEx.u32AddonHwOptions & EEPROM_OPTIONS_EXTERNAL_CLOCK) ? true : false;
	}
	else
	{
		int32 i32Status = CompuscopeGetSystemCaps(m_hSystem, CAPS_CLK_IN, NULL, NULL, NULL);
		m_bExternalClock = CS_SUCCEEDED(i32Status) ? true : false;
	}

	if (m_bExternalClock)
	{
		int64 ExtRate;
		uInt32 ReqSize = sizeof(ExtRate);
		int32 i32Status = CompuscopeGetSystemCaps(m_hSystem, CAPS_MIN_EXT_RATE, nullptr, &ExtRate, &ReqSize);
        if (CS_FAILED(i32Status))
        {
            m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
            m_ErrMng->UpdateErrMsg("Error getting external clock min rate options.", i32Status);
            return i32Status;
        }
		m_i64MinExtRate = ExtRate;
		i32Status = CompuscopeGetSystemCaps(m_hSystem, CAPS_MAX_EXT_RATE, nullptr, &ExtRate, &ReqSize);
        if (CS_FAILED(i32Status))
        {
            m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
            m_ErrMng->UpdateErrMsg("Error getting external clock max rate options.", i32Status);
            return i32Status;
        }
		m_i64MaxExtRate = ExtRate;
	}

    return 0;
}

void CGageSystem::GetMultipleRecord()
{
	if (CsIsLegacyBoard(m_SystemInfo.u32BoardType))
	{
		m_bMultipleRecord = (m_SystemInfoEx.u32AddonHwOptions & EEPROM_OPTIONS_MULTIPLE_REC) ? true : false;
	}
	else
	{
		int32 i32Status = CompuscopeGetSystemCaps(m_hSystem, CAPS_MULREC, NULL, NULL, NULL);
		m_bMultipleRecord = CS_SUCCEEDED(i32Status) ? true : false;
	}
}

void CGageSystem::GetDcOffsetCaps()
{
	int32 i32Status = CompuscopeGetSystemCaps(m_hSystem, CAPS_DC_OFFSET_ADJUST, NULL, NULL, NULL);
	m_bDcOffset = CS_SUCCEEDED(i32Status) ? true : false;
}

int32 CGageSystem::GetExpertCaps()
{
	int64 i64ExtendedOptions;
	int32 i32Status;

	i32Status = CompuscopeGet(m_hSystem, CS_PARAMS, CS_EXTENDED_BOARD_OPTIONS, &i64ExtendedOptions);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error getting extended board options.", i32Status);
		return i32Status;
	}

	ExtractExpertOptions(i64ExtendedOptions);

	return i32Status;
}


int32 CGageSystem::GetSysInfoString(int BrdIndex, QString *pQStrSystemInfo)
{
    CSSYSTEMINFO SysInfo;
    SysInfo.u32Size = sizeof(CSSYSTEMINFO);

    int i32Status = CompuscopeGetSystemInfo(m_hSystem, &SysInfo);
    if (CS_FAILED(i32Status))
    {
        m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
        m_ErrMng->UpdateErrMsg("Error getting system information.", i32Status);
        return i32Status;
    }

#ifdef Q_OS_WIN
    ARRAY_BOARDINFO *pBoardsInfo = static_cast<ARRAY_BOARDINFO*>(::VirtualAlloc (NULL, ((1 - 1) * sizeof(CSBOARDINFO)) + sizeof(ARRAY_BOARDINFO), MEM_COMMIT, PAGE_READWRITE));
#else
    ARRAY_BOARDINFO *pBoardsInfo = (ARRAY_BOARDINFO *)mmap(0, sizeof(ARRAY_BOARDINFO), PROT_WRITE | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif


    pBoardsInfo->u32BoardCount = 1;
    pBoardsInfo->aBoardInfo[0].u32Size = sizeof(CSBOARDINFO);
    pBoardsInfo->aBoardInfo[0].u32BoardIndex = BrdIndex;

    i32Status = CompuscopeGet(m_hSystem, CS_BOARD_INFO, 0, pBoardsInfo);
    if (CS_FAILED(i32Status))
    {
        m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
        m_ErrMng->UpdateErrMsg("Error getting board information.", i32Status);
        #ifdef Q_OS_WIN
            VirtualFree(pBoardsInfo, 0, MEM_RELEASE);
        #else
            munmap(pBoardsInfo, sizeof(ARRAY_BOARDINFO));
        #endif
        return i32Status;
    }

    QString TempQString;

    TempQString = QString("System information:\r\nSerial number: %1\r\n").arg(pBoardsInfo->aBoardInfo[0].strSerialNumber);
    TempQString += QString("Board name: %1\r\n").arg(SysInfo.strBoardName);
    TempQString += QString("Board type: %1\r\n").arg(SysInfo.u32BoardType);
    TempQString += QString("Max memory: %1\r\n").arg(SysInfo.i64MaxMemory);
    TempQString += QString("Channel count: %1\r\n").arg(SysInfo.u32ChannelCount);
    TempQString += QString("Trigger count: %1\r\n").arg(SysInfo.u32TriggerMachineCount);
    TempQString += QString("Sample offset: %1\r\n").arg(SysInfo.i32SampleOffset);
    TempQString += QString("Sample resolution: %1\r\n").arg(SysInfo.i32SampleResolution);
    TempQString += QString("Sample size: %1\r\n").arg(SysInfo.u32SampleSize);
    TempQString += QString("Sample bits: %1\r\n").arg(SysInfo.u32SampleBits);

    pQStrSystemInfo->append(TempQString);

#ifdef Q_OS_WIN
    VirtualFree(pBoardsInfo, 0, MEM_RELEASE);
#else
    munmap(pBoardsInfo, sizeof(ARRAY_BOARDINFO));
#endif
    return 0;
}

int32 CGageSystem::GetFwVersion(int BrdIndex, QString *pQStrVersionInfo)
{
    CSSYSTEMSTATE CsSystemState;
    CsSystemState.u32Size = sizeof(CSSYSTEMSTATE);

    int i32Ret, idxFw;
    i32Ret = m_pCsRmGetSystemStateByIndex(BrdIndex, &CsSystemState);
    if (i32Ret<0)
    {
        return i32Ret;
    }

    if(CsSystemState.bCanChangeBootFw)
    {
        idxFw = CsSystemState.nBootLocation;
    }
    else
    {
        idxFw = 0;
    }

    qDebug() << "Get FW version: Boot Image: " << idxFw;

    CSSYSTEMINFOEX MySystemInfoEx;
    MySystemInfoEx.u32Size = sizeof(CSSYSTEMINFOEX);

    i32Ret = CompuscopeGet(m_hSystem, CS_PARAMS, CS_SYSTEMINFO_EX, &MySystemInfoEx);
    if (i32Ret<0)
    {
        return i32Ret;
    }

    QString TempQString;
    pQStrVersionInfo->clear();

    //Baseboard FPGA
    TempQString = QString("Baseboard FW version:  %1.%2.%3.%4\r\n").arg(MySystemInfoEx.u32BaseBoardFwVersions[idxFw].Version.uMajor)
                                                                   .arg(MySystemInfoEx.u32BaseBoardFwVersions[idxFw].Version.uMinor)
                                                                   .arg(MySystemInfoEx.u32BaseBoardFwVersions[idxFw].Version.uRev)
                                                                   .arg(MySystemInfoEx.u32BaseBoardFwVersions[idxFw].Version.uIncRev);

    pQStrVersionInfo->append(TempQString);

    TempQString = QString("Addonboard FW version: %1.%2.%3.%4\r\n").arg(MySystemInfoEx.u32AddonFwVersions[idxFw].Version.uMajor)
                                                                   .arg(MySystemInfoEx.u32AddonFwVersions[idxFw].Version.uMinor)
                                                                   .arg(MySystemInfoEx.u32AddonFwVersions[idxFw].Version.uRev)
                                                                   .arg(MySystemInfoEx.u32AddonFwVersions[idxFw].Version.uIncRev);

    pQStrVersionInfo->append(TempQString);

    return 0;
}

int32 CGageSystem::TransferData(uInt32 u32Channel, uInt32 u32Segment, void* pBuffer, int *pXferOffset)
{
    IN_PARAMS_TRANSFERDATA inParams;// = { 0 };
    OUT_PARAMS_TRANSFERDATA outParams;// = { 0 };

	int ExpectedStartAddr;

	inParams.u32Segment = u32Segment;
	inParams.u32Mode = (sizeof(uInt32) == m_AcquisitionConfig.u32SampleSize) ? TxMODE_DATA_32 : TxMODE_DEFAULT;
	ExpectedStartAddr = m_AcquisitionConfig.i64Depth - m_AcquisitionConfig.i64SegmentSize + m_AcquisitionConfig.i64TriggerDelay;
	inParams.i64StartAddress = ExpectedStartAddr - CS_XFER_OFFSET;
    //inParams.i64Length = m_AcquisitionConfig.i64Depth + m_AcquisitionConfig.i64TriggerHoldoff + (2 * CS_XFER_OFFSET);
    inParams.i64Length = getTriggerHoldoff() + getDepth() + (2 * CS_XFER_OFFSET);

	inParams.hNotifyEvent = NULL;
	inParams.u16Channel = static_cast<uInt16>(u32Channel); //RG for now
	inParams.pDataBuffer = pBuffer;

	int32 i32Status = CompuscopeTransferData(m_hSystem, &inParams, &outParams);
	if (i32Status<0)
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error transfering data.", i32Status);
		return i32Status;
	}

	*pXferOffset = ExpectedStartAddr - outParams.i64ActualStart;

	return i32Status;
}

int32 CGageSystem::TransferDataBuffer(uInt32 u32Channel, uInt32 u32Segment, void* pBuffer, unsigned long long StartSample, unsigned long long SampleCount, int *pXferOffset, long long *pXferTimeNsec)
{
    IN_PARAMS_TRANSFERDATA inParams;// = { 0 };
    OUT_PARAMS_TRANSFERDATA outParams;// = { 0 };

    long long ExpectedStartAddr;

    inParams.u32Segment = u32Segment;
    inParams.u32Mode = (sizeof(uInt32) == m_AcquisitionConfig.u32SampleSize) ? TxMODE_DATA_32 : TxMODE_DEFAULT;
    ExpectedStartAddr = m_AcquisitionConfig.i64Depth - m_AcquisitionConfig.i64SegmentSize + m_AcquisitionConfig.i64TriggerDelay;
    ExpectedStartAddr += StartSample;
    inParams.i64StartAddress = ExpectedStartAddr - CS_XFER_OFFSET;
    //inParams.i64Length = m_AcquisitionConfig.i64Depth + m_AcquisitionConfig.i64TriggerHoldoff + (2 * CS_XFER_OFFSET);
    inParams.i64Length = SampleCount + (2 * CS_XFER_OFFSET);

    inParams.hNotifyEvent = NULL;
    inParams.u16Channel = static_cast<uInt16>(u32Channel); //RG for now
    inParams.pDataBuffer = pBuffer;

    m_XferTimer.restart();
    int32 i32Status = CompuscopeTransferData(m_hSystem, &inParams, &outParams);
    *pXferTimeNsec = m_XferTimer.nsecsElapsed();
    if (i32Status<0)
    {
        m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
        m_ErrMng->UpdateErrMsg("Error transfering data.", i32Status);
        return i32Status;
    }

    *pXferOffset = ExpectedStartAddr - outParams.i64ActualStart;

    return i32Status;
}

int32 CGageSystem::TransferDataInter(uInt32 u32Segment, void* pBuffer, int *pXferOffset)
{
    IN_PARAMS_TRANSFERDATA inParams;// = { 0 };
    OUT_PARAMS_TRANSFERDATA outParams;// = { 0 };

	int DataTxMode;
	switch (getAcquisitionMode())
	{
		case CS_MODE_SINGLE:
			DataTxMode = TxFORMAT_SINGLE;
			break;

		case CS_MODE_DUAL:
			DataTxMode = TxFORMAT_DUAL_INTERLEAVED;
			break;

		case CS_MODE_QUAD:
			DataTxMode = TxFORMAT_QUAD_INTERLEAVED;
			break;

		case CS_MODE_OCT:
			DataTxMode = TxFORMAT_OCTAL_INTERLEAVED;
			break;
	}

	int ExpectedStartAddr;

	DataTxMode = TxMODE_DATA_INTERLEAVED;

	inParams.u32Segment = u32Segment;
	inParams.u32Mode = (sizeof(uInt32) == m_AcquisitionConfig.u32SampleSize) ? TxMODE_DATA_32 : DataTxMode;
	ExpectedStartAddr = m_AcquisitionConfig.i64Depth - m_AcquisitionConfig.i64SegmentSize + m_AcquisitionConfig.i64TriggerDelay;
	inParams.i64StartAddress = ExpectedStartAddr - CS_XFER_OFFSET;
	inParams.i64Length = (GetActiveChannelCount() * (m_AcquisitionConfig.i64Depth + m_AcquisitionConfig.i64TriggerHoldoff)) + (2 * CS_XFER_OFFSET);

	inParams.hNotifyEvent = NULL;
	//inParams.u16Channel = static_cast<uInt16>(u32Channel); //RG for now
	inParams.u16Channel = 1; 
	inParams.pDataBuffer = pBuffer;

	int32 i32Status = CompuscopeTransferData(m_hSystem, &inParams, &outParams);
	if (i32Status<0)
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error transfering data.", i32Status);
		return i32Status;
	}

	*pXferOffset = ExpectedStartAddr - outParams.i64ActualStart;

	return i32Status;
}

int32 CGageSystem::TransferDataInterBuffer(uInt32 u32Segment, void* pBuffer, unsigned long long StartSample, unsigned long long SampleCount, int *pXferOffset, long long *pXferTimeNsec)
{
    IN_PARAMS_TRANSFERDATA inParams;// = { 0 };
    OUT_PARAMS_TRANSFERDATA outParams;// = { 0 };

    int DataTxMode;
    switch (getAcquisitionMode())
    {
        case CS_MODE_SINGLE:
            DataTxMode = TxFORMAT_SINGLE;
            break;

        case CS_MODE_DUAL:
            DataTxMode = TxFORMAT_DUAL_INTERLEAVED;
            break;

        case CS_MODE_QUAD:
            DataTxMode = TxFORMAT_QUAD_INTERLEAVED;
            break;

        case CS_MODE_OCT:
            DataTxMode = TxFORMAT_OCTAL_INTERLEAVED;
            break;
    }

    int ExpectedStartAddr;

    DataTxMode = TxMODE_DATA_INTERLEAVED;

    inParams.u32Segment = u32Segment;
    inParams.u32Mode = (sizeof(uInt32) == m_AcquisitionConfig.u32SampleSize) ? TxMODE_DATA_32 : DataTxMode;
	ExpectedStartAddr = m_AcquisitionConfig.i64Depth - m_AcquisitionConfig.i64SegmentSize + m_AcquisitionConfig.i64TriggerDelay;
	ExpectedStartAddr+=StartSample;
    inParams.i64StartAddress = ExpectedStartAddr - CS_XFER_OFFSET;
    inParams.i64Length = SampleCount + (2 * CS_XFER_OFFSET);

    inParams.hNotifyEvent = NULL;
    //inParams.u16Channel = static_cast<uInt16>(u32Channel); //RG for now
    inParams.u16Channel = 1;
    inParams.pDataBuffer = pBuffer;

    m_XferTimer.restart();
    int32 i32Status = CompuscopeTransferData(m_hSystem, &inParams, &outParams);
    *pXferTimeNsec = m_XferTimer.nsecsElapsed();
    if (i32Status<0)
    {
        m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
        m_ErrMng->UpdateErrMsg("Error transfering data.", i32Status);
        return i32Status;
    }

    *pXferOffset = ExpectedStartAddr - outParams.i64ActualStart;

    return i32Status;
}

int32 CGageSystem::ResetSystem()
{
	int i32Status;

	i32Status = CompuscopeDo(m_hSystem, ACTION_RESET);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error resetting the system.", i32Status);
		return i32Status;
	}

    quSleep(10000);

	return i32Status;
}

int32 CGageSystem::ResetSystemUpdateInfo()
{
	int i32Status;

	i32Status = CompuscopeDo(m_hSystem, ACTION_RESET);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error resetting the system.", i32Status);
		return i32Status;
	}

    quSleep(10000);

	i32Status = UpdateSystemInformation();
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg("Error gathering system information.", i32Status);
		return i32Status;
	}

	return i32Status;
}

int32 CGageSystem::ForceTrigger()
{
	int i32Status;

	i32Status = CompuscopeDo(m_hSystem, ACTION_FORCE);
	if (CS_FAILED(i32Status))
	{
		m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		m_ErrMng->UpdateErrMsg("Error forcing trigger.", i32Status);
		return i32Status;
	}

    quSleep(10000);

	return i32Status;
}

int32 CGageSystem::ForceCalib()
{
    int i32Status;

    i32Status = CompuscopeDo(m_hSystem, ACTION_CALIB);
    if (CS_FAILED(i32Status))
    {
        m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
        m_ErrMng->UpdateErrMsg("Error forcing calibration.", i32Status);
        return i32Status;
    }

    quSleep(10000);

    return i32Status;
}

int32 CGageSystem::Abort()
{
    //qDebug() << "Abort";
	int i32Status = 0;

    quSleep(1000);
    i32Status = CompuscopeDo(m_hSystem, ACTION_ABORT);

	return i32Status;
}

void CGageSystem::setBuffers(vector<void*>BufferArray)
{
	m_pBufferArray = BufferArray;
}

void* CGageSystem::getBuffer(int i)
{ 
	return m_pBufferArray[i]; 
} //RG DO CHECK IF i is out of bounds

void CGageSystem::setXferOffsets(vector<int>OffsetsArray)
{
	m_pXferOffsetsArray = OffsetsArray;
}

int CGageSystem::getXferOffset(int i)
{ 
	return m_pXferOffsetsArray[i]; 
} //RG DO CHECK IF i is out of bounds

void CGageSystem::setXferOffset(int idx, int Offset)
{ 
	m_pXferOffsetsArray[idx] = Offset; 
} //RG DO CHECK IF i is out of bounds


int32 CGageSystem::GetStartChannel()
{
	int32 StartChan;

	if (getAcquisitionMode() & CS_MODE_SINGLE_CHANNEL2) StartChan = 2;
	else StartChan = 1;

	return StartChan;
}

int32 CGageSystem::GetActiveChannelCount()
{
	return GetActiveChannelCount( getAcquisitionMode() );
}

int32 CGageSystem::GetActiveChannelCount(uInt32 u32Mode)
{
    int32 ActChanCount;

    if (u32Mode & CS_MODE_SINGLE_CHANNEL2) ActChanCount = getBoardCount();
    else ActChanCount = (u32Mode & CS_MASKED_MODE) * getBoardCount();

    return ActChanCount;
}

uInt32 CGageSystem::getChannelSkip(uInt32 u32Mode, uInt32 u32ChannelsPerBoard)
{
	if (getAcquisitionMode() & CS_MODE_SINGLE_CHANNEL2) return u32ChannelsPerBoard;
	else
	{
		Q_ASSERT(u32Mode != 0);

		u32Mode = (0 != u32Mode) ? u32Mode : 1;

		Q_ASSERT(u32ChannelsPerBoard != 0);
		u32ChannelsPerBoard = (0 != u32ChannelsPerBoard) ? u32ChannelsPerBoard : 1;

		uInt32 u32MaskedMode = u32Mode & CS_MASKED_MODE;
		return u32ChannelsPerBoard / u32MaskedMode;
	}
}

// counts the number of bits that are set in an integer value.  We're
// using it to determine the number of acquisition modes there are.
long CGageSystem::count_bits(long n) 
{     
	unsigned int c; // c accumulates the total bits set in v
	for (c = 0; n; c++) 
	{
		n &= n - 1; // clear the least significant bit set
	}
	return c;
}

void CGageSystem::setDepth(int64 i64Depth)
{
	m_AcquisitionConfig.i64Depth = i64Depth;
	m_AcquisitionConfig.i64SegmentSize = i64Depth + m_AcquisitionConfig.i64TriggerHoldoff;
}

void CGageSystem::setTriggerHoldoff(int64 i64Holdoff)
{
	m_AcquisitionConfig.i64TriggerHoldoff = i64Holdoff;
	m_AcquisitionConfig.i64SegmentSize = i64Holdoff + m_AcquisitionConfig.i64Depth;
}

void CGageSystem::setTriggerDelay(int64 i64Delay)
{
    m_AcquisitionConfig.i64TriggerDelay = i64Delay;
    m_AcquisitionConfig.i64SegmentSize = m_AcquisitionConfig.i64Depth;
}

void CGageSystem::setExtRef()
{
	m_AcquisitionConfig.u32ExtClk = 0;
	m_AcquisitionConfig.u32Mode = m_AcquisitionConfig.u32Mode | CS_MODE_REFERENCE_CLK;
}

//RG int32 CGageSystem::allocateMemory(uInt32 u32Mode, int64 i64SegmentSize)
int32 CGageSystem::allocateMemory(uInt32 u32Mode)
{
    //qDebug() << "AllocateMemory";

    uInt32 NbChans = GetActiveChannelCount();
    uInt32 u32Size = (u32Mode & CS_MASKED_MODE) * m_SystemInfo.u32BoardCount;
    int LoopCount = 2;
    int i32SuccErrorCode = CS_SUCCESS;

    do
    {
        //m_i64Chan_mem_size = (i64SegmentSize + (4 * CS_XFER_OFFSET)) * m_AcquisitionConfig.u32SampleSize;
        m_i64Chan_mem_size = (XFER_BUFFER_SIZE + (4 * CS_XFER_OFFSET)) * m_AcquisitionConfig.u32SampleSize;
        m_i64Chan_mem_size = ((unsigned long long)ceil(((double)m_i64Chan_mem_size) / MEM_PAGE_SIZE) + 1) * MEM_PAGE_SIZE;
        m_i64MainSamplesBufferSize = NbChans * m_i64Chan_mem_size;
//RG        unsigned int ChanMemSize = m_i64Chan_mem_size;

        //qDebug() << "Chan mem size: " << m_i64Chan_mem_size;
        //qDebug() << "Samples buffer size: " << m_i64MainSamplesBufferSize;

        if (m_bInterleavedData)
        {
            // AcquisitionConfig should now have the new mode and / or segment size
            if(m_pMainSamplesBuffer!=NULL)
            {
                #ifdef Q_OS_WIN
                    VirtualFree(m_pMainSamplesBuffer, 0, MEM_RELEASE);
                #else
                    munmap(m_pMainSamplesBuffer, m_i64MainSamplesBufferSize);
                #endif

                m_pMainSamplesBuffer = NULL;
                m_pBufferArray.clear();
            }

            if (!m_pXferOffsetsArray.empty())
            {
                m_pXferOffsetsArray.clear();
            }

            #ifdef Q_OS_WIN
                m_pMainSamplesBuffer = VirtualAlloc(NULL, m_i64MainSamplesBufferSize, MEM_COMMIT, PAGE_READWRITE);
            #else
                m_pMainSamplesBuffer = mmap(0, m_i64MainSamplesBufferSize, PROT_WRITE | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            #endif

            // RG USE A TRY CATCH HERE INCASE MALLOC FAILS, ALSO MAYBE USE NEW ??
            // AND / OR RETURN CS_MEMORY_ERROR. free whatever was allocated

            for (uInt32 i = 0; i < u32Size; i++)
            {
                m_pBufferArray.push_back(m_pMainSamplesBuffer);
                m_pXferOffsetsArray.push_back(0);
            }

            m_i64LastValidHoldoff = getTriggerHoldoff();
            m_i64LastValidDepth = getDepth();

            return CS_SUCCESS;
        }
        else
        {
            // AcquisitionConfig should now have the new mode and / or segment size
            if(m_pMainSamplesBuffer!=NULL)
            {
                //qDebug() << "Allocate: Free previous buffer";

                #ifdef Q_OS_WIN
                    VirtualFree(m_pMainSamplesBuffer, 0, MEM_RELEASE);
                #else
                    munmap(m_pMainSamplesBuffer, m_i64MainSamplesBufferSize);
                #endif

                m_pMainSamplesBuffer = NULL;
            }

            if(!m_pBufferArray.empty())
            {
                //qDebug() << "Allocate: Clear buffer array";
                m_pBufferArray.clear();
            }

            if (!m_pXferOffsetsArray.empty())
            {
                //qDebug() << "Allocate: Clear offset array";
                m_pXferOffsetsArray.clear();
            }

            #ifdef Q_OS_WIN
                //qDebug() << "Allocate: Virtual alloc";
                m_pMainSamplesBuffer = VirtualAlloc(NULL, m_i64MainSamplesBufferSize, MEM_COMMIT, PAGE_READWRITE);
            #else
                m_pMainSamplesBuffer = mmap(0, m_i64MainSamplesBufferSize, PROT_WRITE | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            #endif

            //qDebug() << "Allocate: Samples main buffer: " << m_pMainSamplesBuffer;

            if(m_pMainSamplesBuffer!=NULL)
            {
                void *CurrChanBuffer = m_pMainSamplesBuffer;
                for (uInt32 i = 0; i < u32Size; i++)
                {
                    //qDebug() << "Allocate: Assign buffer CH" << (i+1);
                    CurrChanBuffer = ((char *)m_pMainSamplesBuffer) + (i * m_i64Chan_mem_size);
                    //qDebug() << "Allocate: Buffer: " << CurrChanBuffer;
                    m_pBufferArray.push_back(CurrChanBuffer);
                    m_pXferOffsetsArray.push_back(0);
                }

                m_i64LastValidHoldoff = getTriggerHoldoff();
                m_i64LastValidDepth = getDepth();

                return i32SuccErrorCode;
            }

            else
            {
                setTriggerHoldoff(0);
                setDepth(160);

                int CommitTime;
                CommitAcq(&CommitTime);
//RG                i64SegmentSize = m_AcquisitionConfig.i64SegmentSize;
                i32SuccErrorCode = CS_CONFIG_CHANGED;
            }
        }

        LoopCount--;
    }while(LoopCount>0);

    return -1;
}

#ifdef UNICODE
QString CGageSystem::GetErrorString(int32 i32ErrorCode)
{
    //WCHAR temp[255];
    //CsGetErrorString(i32ErrorCode, &temp[0], 255);
    wchar_t temp[255];
    CsGetErrorString(i32ErrorCode, &temp[0], 255);
    return QString::fromWCharArray(temp);
}
#else
QString CGageSystem::GetErrorString(int32 i32ErrorCode)
{
    char temp[255];
    CsGetErrorString(i32ErrorCode, &temp[0], 255);
    return QString::fromLocal8Bit(temp);
}
#endif

void CGageSystem::ClearAcqCount()
{
	m_u32AcqCount = 0;
}

void CGageSystem::IncAcqCount()
{
	m_u32AcqCount += 1;
}

uInt32 CGageSystem::GetAcqCount()
{
	return m_u32AcqCount;
}

void CGageSystem::updateErrMess(QString strErrMess)
{
	m_strError.insert(0, strErrMess);
	return;
}

void CGageSystem::AcqSleep(int msDelay)
{
	m_SleepTimer.restart();
	while (!m_SleepTimer.hasExpired(msDelay))
	{
		emit updateUi();
	}
}

void CGageSystem::ExtractExpertOptions(int64 i64ExtendedOptions)
{
	int idxExp;
	int32 i32Expert1Options = i64ExtendedOptions & 0xFFFFFFFF;
	int32 i32Expert2Options = (i64ExtendedOptions >> 32) & 0xFFFFFFFF;
	int32 i32CurrFlag = 0;
	QString CurrOptName;

	bool bExp1Found = false;
	bool bExp2Found = false;
	for (idxExp = EXPERT_NONE; idxExp <= (int)EXPERT_UNKNOWN; idxExp++)
	{
		switch (idxExp)
		{
			case EXPERT_NONE:
				i32CurrFlag = 0;
				if (i32Expert1Options == 0)
				{
					m_ExpertImg1 = EXPERT_NONE;
					m_strExpertImg1Name = "None";
					bExp1Found = true;
				}
				if (i32Expert2Options == 0)
				{
					m_ExpertImg2 = EXPERT_NONE;
					m_strExpertImg2Name = "None";
					bExp2Found = true;
				}
				break;

			case EXPERT_FIR:
				i32CurrFlag = CS_BBOPTIONS_FIR;
				CurrOptName = "FIR";
				break;

			case EXPERT_MINMAXDETECT:
				i32CurrFlag = CS_BBOPTIONS_MINMAXDETECT;
				CurrOptName = "MinMaxDetect";
				break;

			case EXPERT_MULREC_AVG:
				i32CurrFlag = CS_BBOPTIONS_MULREC_AVG;
				CurrOptName = "MulRecAvg";
				break;

			case EXPERT_FFT_512:
				//i32CurrFlag = CS_BBOPTIONS_FFT_512;
				CurrOptName = "FFT512";
				break;

			case EXPERT_FFT_1K:
				//i32CurrFlag = CS_BBOPTIONS_FFT_1K;
				CurrOptName = "FFT1K";
				break;

			case EXPERT_FFT_2K:
				//i32CurrFlag = CS_BBOPTIONS_FFT_2K;
				CurrOptName = "FFT2K";
				break;

			case EXPERT_FFT_4K:
				i32CurrFlag = CS_BBOPTIONS_FFT_4K;
				CurrOptName = "FFT4K";
				break;

			case EXPERT_FFT_8K:
				i32CurrFlag = CS_BBOPTIONS_FFT_4K;//CS_BBOPTIONS_FFT_8K
				CurrOptName = "FFT4K";
				break;

			case EXPERT_MULREC_AVG_TD:
				i32CurrFlag = CS_BBOPTIONS_MULREC_AVG_TD;
                CurrOptName = "Signal Averaging";
				break;

			case EXPERT_COIN:
				//i32CurrFlag = CS_BBOPTIONS_COIN;
				CurrOptName = "Coincidence Filter";
				break;

			case EXPERT_GATE_ACQ:
				//i32CurrFlag = CS_BBOPTIONS_GATE_ACQ;
				CurrOptName = "GateAcq";
				break;

			case EXPERT_STREAM:
				i32CurrFlag = CS_BBOPTIONS_STREAM;
				CurrOptName = "Stream";
				break;

			case EXPERT_HISTOGRAM:
				i32CurrFlag = CS_AOPTIONS_HISTOGRAM;
				CurrOptName = "Histogram";
				break;

			case EXPERT_UNKNOWN:
				if (!bExp1Found)
				{
					m_ExpertImg1 = EXPERT_UNKNOWN;
					m_strExpertImg1Name = "Unknown";
					bExp1Found = true;
				}
				if (!bExp2Found)
				{
					m_ExpertImg2 = EXPERT_UNKNOWN;
					m_strExpertImg2Name = "Unknown";
					bExp2Found = true;
				}
				break;
		}

		if (!bExp1Found)
		{
			if ((i32Expert1Options & i32CurrFlag) != 0)
			{
				m_ExpertImg1 = (ExpertOptions)idxExp;
				m_strExpertImg1Name = CurrOptName;
				bExp1Found = true;
			}
		}
		if (!bExp2Found)
		{
			if ((i32Expert2Options & i32CurrFlag) != 0)
			{
				m_ExpertImg2 = (ExpertOptions)idxExp;
				m_strExpertImg2Name = CurrOptName;
				bExp2Found = true;
			}
		}

		if (bExp1Found && bExp2Found)
		{
			break;
		}
	}

	return;
}

void CGageSystem::SetAbort()
{
    m_bAbort = true;
}

void CGageSystem::ClearAbort()
{
	m_bAbort = false;
}

bool CGageSystem::IsValidTriggerSource(int TrigSource)
{
	int MaxChan;
    uInt32 u32ChannelSkip;
    MaxChan = getChannelCount();
    u32ChannelSkip = GetChannelSkip();

    switch(m_AcquisitionConfig.u32Mode)
    {
        case CS_MODE_SINGLE:
            if(TrigSource==1) return true;
            else return false;
            break;

        case CS_MODE_SINGLE_CHANNEL2:
            if(TrigSource==2) return true;
            else return false;
            break;
    }

    if (TrigSource == CS_TRIG_SOURCE_EXT) return true;
    else if (TrigSource == CS_TRIG_SOURCE_DISABLE) return true;
	else
	{
        if (((((TrigSource - 1) % u32ChannelSkip)) == 0) && (TrigSource <= MaxChan)) return true;
	}

	return false;
}

bool CGageSystem::IsValidTriggerSource(int TrigSource, uInt32 u32Mode)
{
    int MaxChan;

    MaxChan = getChannelCount();
    uInt32 ChanCountPerBoard = getChannelCountPerBoard();
    uInt32 u32ChannelSkip = getChannelSkip(u32Mode, ChanCountPerBoard);

    switch(u32Mode)
    {
        case CS_MODE_SINGLE:
            if(TrigSource==1) return true;
            else return false;
            break;

        case CS_MODE_SINGLE_CHANNEL2:
            if(TrigSource==2) return true;
            else return false;
            break;
    }

    if (TrigSource == CS_TRIG_SOURCE_EXT) return true;
    else if (TrigSource == CS_TRIG_SOURCE_DISABLE) return true;
    else
    {
        if (((((TrigSource - 1) % u32ChannelSkip)) == 0) && (TrigSource <= MaxChan)) return true;
    }

    return false;
}

int32 CGageSystem::WaitForTrigger()
{
	//qDebug() << "Wait for trigger";
#ifdef Q_OS_WIN
	bool bTriggered = false;
	// Get the Event Handles
	if (m_hTrigEvent)
	{
		while (!bTriggered)
		{
			DWORD dwRet = WaitForSingleObject(m_hTrigEvent, 100);
			if ((dwRet != 0) && (dwRet != WAIT_TIMEOUT))
			{
				m_ErrMng->UpdateErrMsg("Error waiting for trigger.", 0);
				return 0;
			}
			if (dwRet == 0) bTriggered = true;

			if (m_bAbort)
			{
				Abort();
				m_bAbort = false;
			}
		}
	}
	else
#endif
	{
		int32 i32Status;
		do
		{
			quSleep(10);
			i32Status = CompuscopeGetStatus(m_hSystem);
			if (i32Status < 0)
			{
				//qDebug() << "Wait for trigger loop error" << " Status: " << i32AcqStatus << " Abort: " << m_bAbort;
				m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
				m_ErrMng->UpdateErrMsg("Error waiting for trigger.", i32Status);
				return i32Status;
			}

			if (m_bAbort)
			{
				m_bAbort = false;
				Abort();
			}
		} while ((i32Status != ACQ_STATUS_TRIGGERED) && (i32Status != ACQ_STATUS_READY));
	}

	return 1;
}

int32 CGageSystem::DataCaptureComplete()
{
	//qDebug() << "Wait data capture complete";

#ifdef Q_OS_WIN

	bool bAcqComplete = false;
	if (m_hAcqEndEvent)
	{
		while (!bAcqComplete)
		{
			DWORD dwRet = WaitForSingleObject(m_hAcqEndEvent, 100);
			if ((dwRet != 0) && (dwRet != WAIT_TIMEOUT))
			{
				m_ErrMng->UpdateErrMsg("Error waiting for end of acquisition.", 0);
				return 0;
			}
			if (dwRet == 0) bAcqComplete = true;

			if (m_bAbort)
			{
				m_bAbort = false;
				Abort();
			}
		}

		//   if ((CS_DRIVER_ASYNC_NOT_SUPPORTED!=i32Status) && (i32Status<0))
		//    {
		//        m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
		//        m_ErrMng->UpdateErrMsg("Error waiting for capture complete.", i32Status);
		//        return i32Status;
		//	  }
	}
	else

#endif
	{
		int32 i32Status = WaitForReady();
		if (i32Status < 0)
		{
			m_ErrMng->UpdateErrMsg(GetErrorString(i32Status), i32Status);
			m_ErrMng->UpdateErrMsg("Error waiting for capture complete.", i32Status);
			return i32Status;
		}
	}

	return 1;
}

// Wait for end of acqusition
int32 CGageSystem::WaitForReady()
{
	int32 i32Status;
    //qDebug() << "Wait ready";
	do
	{
        quSleep(10);
		i32Status = CompuscopeGetStatus(m_hSystem);
        if(i32Status<0) return i32Status;
        if(m_bAbort)
        {
            m_bAbort = false;
            Abort();
        }
	}
	while (!(ACQ_STATUS_READY == i32Status));

    //qDebug() << "Done wait ready";

	return i32Status;
}


#define WAIT_TIME_INTERVAL	10
bool	CGageSystem::WaitForBoardIdleState(int Timeout)
{
	int TimeIncrement = 0;
	bool	bSuccess;

	// Waiting for board Idle state
	m_MutexBoardIdle.lock();
	while ( !(bSuccess = m_WaitBoardIdle.wait(&m_MutexBoardIdle, WAIT_TIME_INTERVAL)) )
	{
		TimeIncrement += WAIT_TIME_INTERVAL;
		if (TimeIncrement >= Timeout)
			break;
		QCoreApplication::processEvents();
	}

	m_MutexBoardIdle.unlock();
	return bSuccess;
}

