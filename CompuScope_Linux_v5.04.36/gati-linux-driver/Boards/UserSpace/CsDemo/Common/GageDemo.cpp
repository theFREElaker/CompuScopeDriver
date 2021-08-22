/*
 * GageDemo.cpp
 *
 *  Created on: 24-Jul-2008
 *      Author: egiard
 */

#if defined WIN32 || defined CS_WIN64

	#include "stdafx.h"
	#include "CsEventLog.h"
	#include "CsMsgLog.h"
	#ifndef CS_WIN64
		#include "debug.h"
	#endif

#elif defined __linux__

	#include <cstdio>
	#include <stdint.h>

#endif

#include <math.h>
#include <stdlib.h>

#include "GageDemo.h"
#include "GageDemoSystem.h"
#include "Gage_XML_Defines.h"


CGageDemo* CGageDemo::m_pInstance = NULL;

Critical_Section CGageDemo::__key;
extern SHAREDDLLMEM Globals;


#if defined (_WIN32) || defined (CS_WIN64)

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

//-----------------------------------------------------------------------------
// Check if the process is a 32 bits process under 64bit windwos
//-----------------------------------------------------------------------------

BOOL IsWow64()
{
    BOOL bIsWow64 = FALSE;
	LPFN_ISWOW64PROCESS g_fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle("kernel32"),"IsWow64Process");

    if (NULL != g_fnIsWow64Process)
    {
        if (!g_fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
        {
			// Assume 32 bit OS
            bIsWow64 = FALSE;
        }
    }
    return bIsWow64;
}

#endif

UINT GetGageDirectory(LPTSTR str, UINT uSize)
{
	UINT uRet = 0;

#if defined (_WIN32) || defined (CS_WIN64)

	TCHAR* ptCh;

	errno_t err = ::_tdupenv_s(&ptCh, NULL, _T("GageDir"));
	if (err)
	{
		return 0;
	}
	else
	{
		_tcsncpy_s(str, MAX_PATH, ptCh, uSize);
		::free(ptCh);

		if( str[_tcslen(str)-1] != _T('\\') )
		{
			_tcscat_s(str, MAX_PATH,  _T("\\") );
		}
		uRet = UINT(_tcslen(str));
	}

#elif defined __linux__

	char* homePath = (char *)XML_FILE_PATH;

	if (homePath)
	{
		strncpy(str, homePath, uSize);
		strcat(str, "/");
	}

	uRet = UINT(_tcslen(str));

#endif

	return uRet;
}

unsigned int CalculateMonth(char *strMonth)
{
	TCHAR lpMonth[12][4] = {_T("Jan"),
							_T("Feb"),
							_T("Mar"),
							_T("Apr"),
							_T("May"),
							_T("Jun"),
							_T("Jul"),
							_T("Aug"),
							_T("Sep"),
							_T("Oct"),
							_T("Nov"),
							_T("Dec")};

	for (int i = 0; i < 12; i++)
	{
		if (0 == _tcsncmp(strMonth, lpMonth[i], 3))
			return i + 1;
	}
	// If not found pretend it's January.
	return 1;
}

CGageDemo::CGageDemo(void) : m_u32SystemCount(0),
							 m_pDrvHandles(NULL),
							 m_strXmlFileName("")
{
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CGageDemo::~CGageDemo(void)
{
	DEMOSYSTEMMAP::iterator theMapIterator;
	if (!m_DemoSystemMap.empty())
	{
		for (theMapIterator = m_DemoSystemMap.begin(); theMapIterator  != m_DemoSystemMap.end(); theMapIterator++)
		{
			CGageDemoSystem* ptr = static_cast<CGageDemoSystem*>((*theMapIterator).second);
			delete (ptr);
		}
		m_DemoSystemMap.clear();
	}
	m_DemoSystemInfoMap.clear();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
DWORD CGageDemo::EnumerateSystemChannelInfo(XMLElement* pSystem, ChannelInfo& csChInfo)
{
	DWORD dwChannelCount(0);

	csChInfo.clear();

	XMLVariable* pChannelCount = pSystem->FindVariableZ((char *)XML_VAR_CHANNELS);

	if (NULL == pChannelCount)
	{
		return 0;
	}

	dwChannelCount = pChannelCount->GetValueInt();

	if (0 == dwChannelCount)
	{
		return 0;
	}

	ChannelType echType;
	TCHAR strChanName[MAX_PATH];
	TCHAR str[30];

	DWORD		 dwValidChannelCount = dwChannelCount;
	XMLElement*	 pXmlElement = NULL;
	XMLVariable* pXmlVariable = NULL;

	for( unsigned u = 0; u < dwChannelCount; u++ )
	{
		_stprintf_s(strChanName, _countof(strChanName), _T("Chan%02d"), u+1 );

		pXmlElement = pSystem->FindElementZ(strChanName);

		if (NULL == pXmlElement)
		{
			dwValidChannelCount--;
			continue;
		}

		pXmlVariable = pXmlElement->FindVariableZ((char *)XML_VAR_SIGNAL_TYPE);

		if (NULL == pXmlVariable)
		{
			echType = echSine;
		}
		else
		{
			pXmlVariable->GetValue(str);

			if( 0 == _tcsncmp ( _T("sine"), str, _tcslen( _T("sine") ) ) )
			{
				echType = echSine;
			}
			else if( 0 == _tcsnccmp ( _T("triangle"), str, _tcslen( _T("triangle") )) )
			{
				echType = echTriangle;
			}
			else if( 0 == _tcsnccmp ( _T("square"), str, _tcslen( _T("square") )) )
			{
				echType = echSquare;
			}
			else if( 0 == _tcsnccmp ( _T("twotone"), str, _tcslen( _T("twotone") )) )
			{
				echType = echTwoTone;
			}
			else
			{
				echType = echSine;
			}
		}

		csChInfo.push_back( echType );
	}

	return dwValidChannelCount;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CGageDemo::EnumerateSystemCapsInfo(XMLElement* pCaps, SystemCapsInfo& csCapsInfo)
{
	csCapsInfo.veSampleRate.clear();
	csCapsInfo.veInputRange.clear();

	// Enumerate sample rates
	TCHAR tsValueName[100];
	XMLElement* pSampleRates = pCaps->FindElementZ((char *)XML_ELEMENT_RATES);
	if (!pSampleRates)
		return FALSE;

	DWORD dwCount = pSampleRates->GetVariableNum();

	for (DWORD i = 0; i < dwCount; i++)
	{
		CSSAMPLERATETABLE_EX entry;

		XMLVariable *pVar = pSampleRates->GetVariables()[i];
		if (!pVar || (-1 == pVar->GetName(tsValueName))) //RICKY ?? CHECK THE -1
			continue;

		//remove leading space
		memmove(tsValueName, &tsValueName[1], strlen(tsValueName));

		entry.PublicPart.i64SampleRate = _tstoi64( tsValueName );
		if ( 0 == entry.PublicPart.i64SampleRate )
			continue;

		if ( entry.PublicPart.i64SampleRate >= 1000000000 )
		{
			sprintf_s( entry.PublicPart.strText, sizeof(entry.PublicPart.strText),
					   "%u GS/s", uInt32(entry.PublicPart.i64SampleRate/1000000000) );
		}
		else if ( entry.PublicPart.i64SampleRate >= 1000000 )
		{
			sprintf_s( entry.PublicPart.strText, sizeof(entry.PublicPart.strText),
					   "%u MS/s", uInt32(entry.PublicPart.i64SampleRate/1000000) );
		}
		else if ( entry.PublicPart.i64SampleRate >= 1000 )
		{
			sprintf_s( entry.PublicPart.strText, sizeof(entry.PublicPart.strText),
				       "%u kS/s", uInt32(entry.PublicPart.i64SampleRate/1000) );
		}
		else
		{
			sprintf_s( entry.PublicPart.strText, sizeof(entry.PublicPart.strText),
				       "%u S/s", uInt32(entry.PublicPart.i64SampleRate) );
		}

		DWORD dwData = pVar->GetValueInt();

		entry.u32CardSupport = 0;
		entry.u32Mode = dwData;

		csCapsInfo.veSampleRate.push_back(entry);
	}

	if( csCapsInfo.veSampleRate.empty() )
		return false;

	//Enumerate Input ranges

	XMLElement* pRanges = pCaps->FindElementZ((char *)XML_ELEMENT_RANGES);
	if (!pRanges)
		return FALSE;

	dwCount = pRanges->GetVariableNum();

	for (DWORD i = 0; i < dwCount; i++)
	{
		XMLVariable *pVar = pRanges->GetVariables()[i];
		if (!pVar || (-1 == pVar->GetName(tsValueName))) //RICKY ?? CHECK THE -1
			continue;

		//remove leading space
		memmove(tsValueName, &tsValueName[1], strlen(tsValueName));

		CSRANGETABLE entry;

		entry.u32InputRange = _tstoi( tsValueName );

		if ( 0 >= entry.u32InputRange )
			continue;

		if ( entry.u32InputRange >= 2000 )
		{
			sprintf_s( entry.strText, sizeof(entry.strText), "±%u V", entry.u32InputRange/2000 );
		}
		else
		{
			sprintf_s( entry.strText, sizeof(entry.strText), "±%u mV", entry.u32InputRange/2 );
		}
		entry.u32Reserved = pVar->GetValueInt();
		csCapsInfo.veInputRange.push_back(entry);
	}
	if( csCapsInfo.veInputRange.empty() )
		return false;

	//Add external trigger
	CSRANGETABLE entry = {CS_GAIN_10_V,	"±5.0 V", CS_IMP_50_OHM | CS_IMP_EXT_TRIG};
	csCapsInfo.veInputRange.push_back(entry);

	//Read trigger caps
	csCapsInfo.uTriggerSense = 10;
	XMLElement* pTrigger = pCaps->FindElementZ((char *)XML_ELEMENT_TRIGGER);
	if (pTrigger)
	{
		DWORD dwData = 20;
		XMLVariable* pSensitivity = pTrigger->FindVariableZ((char *)XML_VAR_SENSITIVITY);

		if (NULL != pSensitivity)
			dwData = pSensitivity->GetValueInt();

		csCapsInfo.uTriggerSense = min(20, (int)dwData);
	}

	return true;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CGageDemo::GetSystemsFromFile(XMLElement *pElement, uInt32 u32SampleBits, uInt32 u32Offset)
{
	uInt32 u32SystemsFound = 0;
	uInt32 u32HandleOffset = u32Offset + 1;

	uInt32 u32SystemCount = pElement->GetChildrenNum();
	for (uInt32 i = 0; i < u32SystemCount; i++)
	{
		ChannelInfo csChInfo;
		SystemCapsInfo csCapsInfo;

		XMLElement* pSystem = pElement->GetChildren()[i];
		if (!pSystem)
			return 0;

		DWORD dwChannelCount = EnumerateSystemChannelInfo(pSystem, csChInfo);
		XMLElement* pCaps = pSystem->FindElementZ((char *)XML_ELEMENT_CAPS);
		if (!pCaps)
			return 0;

		bool bRet = EnumerateSystemCapsInfo(pCaps, csCapsInfo);

		if( 0 != dwChannelCount && bRet)
		{
			u32SystemsFound++;

			DEMOSYSTEMINFO stDemoInfo;

#ifdef __linux__
			stDemoInfo.drvHandle = (DRVHANDLE)((uInt32)(DEMO_BASE_HANDLE + u32HandleOffset++));
#else
			stDemoInfo.drvHandle = (DRVHANDLE)((ULONG_PTR)(DEMO_BASE_HANDLE + u32HandleOffset++));
#endif
			stDemoInfo.csSystemInfo.u32SampleBits = u32SampleBits;
			pSystem->GetElementName(stDemoInfo.tcSystemName);

			cschar strBoardName[MAX_PATH];

			// For now, if we can't read the BoardName field to get the name,
			// we'll use the KeyName (i.e. CsDemo16-1, etc) as the system name.
			XMLVariable* pBoardName = pSystem->FindVariableZ((char *)XML_VAR_BOARDNAME);

			if ((NULL == pBoardName) || (-1 == pBoardName->GetValue(strBoardName)))
			{
				pSystem->GetElementName(strBoardName);
			}

			lstrcpyn(stDemoInfo.csSystemInfo.strBoardName, strBoardName, 31);

			stDemoInfo.csSystemInfo.u32ChannelCount = dwChannelCount;
			stDemoInfo.csChannelInfo = csChInfo;
			stDemoInfo.csSysCaps = csCapsInfo;

			SetSystemDefaults(&stDemoInfo.csSystemInfo);

			if (CS_SAMPLE_BITS_8 == u32SampleBits)
				_stprintf_s(stDemoInfo.csBoardInfo.strSerialNumber, sizeof(stDemoInfo.csBoardInfo.strSerialNumber),
							_T("v%u000%03u"), u32SampleBits, i + 1);
			else
				_stprintf_s(stDemoInfo.csBoardInfo.strSerialNumber, sizeof(stDemoInfo.csBoardInfo.strSerialNumber),
								_T("v%u00%03u"), u32SampleBits, i + 1);
			SetBoardDefaults(&stDemoInfo.csBoardInfo);
			SetVersionInfoDefaults(&stDemoInfo.csVerInfo);
			SetExtendedSystemInfoDefaults(&stDemoInfo.csSystemExInfo, &stDemoInfo.csVerInfo);
			m_DemoSystemInfoMap.insert(DEMOSYSTEMINFOMAP::value_type(stDemoInfo.drvHandle, stDemoInfo));
		}
	}

	return u32SystemsFound;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CGageDemo::GetSystemInfoFromFile()
{
#ifdef _WIN32
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	DWORD dwError;
#endif

	string strFileName;

	TCHAR tcDirectory[MAX_PATH] = {'\n'};

	if (GetGageDirectory(tcDirectory, MAX_PATH))
	{
#ifdef _WINDOWS		
		// if last character is not \, then add it
		if( tcDirectory[_tcslen(tcDirectory)-1] != _T('\\') )
		{
			_tcscat_s(tcDirectory, MAX_PATH,  _T("\\") );
		}
#else
		if( tcDirectory[_tcslen(tcDirectory)-1] != _T('/') )
		{
			_tcscat_s(tcDirectory, MAX_PATH,  _T("/") );
		}
#endif		
		strFileName = tcDirectory;
		strFileName.append(XML_FILE_NAME);
	}
	else
	{
#ifdef _WINDOWS
		// Report to event log 
		CsEventLog EventLog;
		EventLog.Report(CS_WARNING_TEXT, _T("GageDir environment variable or directory not found"));
		return 0;
#else
		strFileName = XML_FILE_NAME;
#endif
	}

	m_strXmlFileName = strFileName;

	if (!m_DemoSystemInfoMap.empty())
	{
		m_DemoSystemInfoMap.clear();
	}

#ifdef _WIN32

	hFind = ::FindFirstFile(strFileName.c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		dwError = ::GetLastError();
		// if the file is not found, there's no virtual systems for sure
		if (ERROR_FILE_NOT_FOUND == dwError)
			return 0;
	}

	::FindClose(hFind);

#endif

	XML *pSystem = new XML(strFileName.c_str());

	if (!pSystem)
		return 0;

	XMLElement* pRoot = pSystem->GetRootElement();
	if (!pRoot)
	{
		delete pSystem;
		return 0;
	}

	int nChildren = pRoot->GetChildrenNum();
	if (0 == nChildren)
	{
		// if there's a file but no children under the root, there's no virtual systems.
		delete pSystem;
		return 0;
	}

	uInt32 u32SystemCount = 0;

	XMLElement* pElement = pRoot->FindElementZ((char *)XML_8_BIT_NODE);
	if (pElement)
	{
		u32SystemCount += GetSystemsFromFile(pElement, CS_SAMPLE_BITS_8, u32SystemCount);
	}
	pElement = pRoot->FindElementZ((char *)XML_12_BIT_NODE);
	if (pElement)
	{
		u32SystemCount += GetSystemsFromFile(pElement, CS_SAMPLE_BITS_12, u32SystemCount);
	}
	pElement = pRoot->FindElementZ((char *)XML_14_BIT_NODE);
	if (pElement)
	{
		u32SystemCount += GetSystemsFromFile(pElement, CS_SAMPLE_BITS_14, u32SystemCount);
	}
	pElement = pRoot->FindElementZ((char *)XML_16_BIT_NODE);
	if (pElement)
	{
		u32SystemCount += GetSystemsFromFile(pElement, CS_SAMPLE_BITS_16, u32SystemCount);
	}

	delete pSystem;

	m_u32SystemCount = u32SystemCount;
	return u32SystemCount;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CGageDemo::SetSystemDefaults(PCSSYSTEMINFO pSystemInfo)
{
	pSystemInfo->u32Size = sizeof(CSSYSTEMINFO);
	pSystemInfo->i64MaxMemory = CGageDemoSystem::c_i64MemSize;

	if (CS_SAMPLE_BITS_12 == pSystemInfo->u32SampleBits)
	{
		// 12 Bit demo emulates a Combine board which has
		// left-shifted data, therefor sample offset and res are adjusted
		pSystemInfo->i32SampleResolution = -CS_SAMPLE_RES_LJ;
		pSystemInfo->u32SampleSize = CS_SAMPLE_SIZE_2;
		pSystemInfo->i32SampleOffset = CS_SAMPLE_OFFSET_12_LJ;
		pSystemInfo->u32BoardType = CSDEMO12_BOARDTYPE ;
	}
	else if (CS_SAMPLE_BITS_16 == pSystemInfo->u32SampleBits)
	{
		pSystemInfo->i32SampleResolution = CS_SAMPLE_RES_16;
		pSystemInfo->u32SampleSize = CS_SAMPLE_SIZE_2;
		pSystemInfo->i32SampleOffset = CS_SAMPLE_OFFSET_16;
		pSystemInfo->u32BoardType = CSDEMO16_BOARDTYPE ;
	}
	else if (CS_SAMPLE_BITS_8 == pSystemInfo->u32SampleBits)
	{
		pSystemInfo->i32SampleResolution = CS_SAMPLE_RES_8;
		pSystemInfo->u32SampleSize = CS_SAMPLE_SIZE_1;
		pSystemInfo->i32SampleOffset = CS_SAMPLE_OFFSET_UNSIGN_8;
		pSystemInfo->u32BoardType = CSDEMO8_BOARDTYPE ;
	}
	else // 14 bits  14 Bit demo emulates an Octupus board which has
	{	 // left-shifted data, therefor sample offset and res are adjusted
		pSystemInfo->i32SampleResolution = -CS_SAMPLE_RES_LJ;
		pSystemInfo->u32SampleSize = CS_SAMPLE_SIZE_2;
		pSystemInfo->i32SampleOffset = CS_SAMPLE_OFFSET_14_LJ;
		pSystemInfo->u32BoardType = CSDEMO14_BOARDTYPE ;
	}
	pSystemInfo->u32AddonOptions = 0x0000;		// EEPROM options encoding features like gate, trigger enable, digital in, pulse out
	pSystemInfo->u32BaseBoardOptions = 0x0000;	// EEPROM options -------------------------||---------------------------------------
	pSystemInfo->u32BoardCount = 1; // Board count is always 1 (unless we change it in the registry)
	pSystemInfo->u32TriggerMachineCount = 1; // ??? RICKY

#ifdef _WINDOWS	
	// Note: This should be moved to the XML file for compatiblity with Linux
	TCHAR szRegistryKey[_MAX_PATH];
	::strcpy_s(szRegistryKey, sizeof(szRegistryKey), "SYSTEM\\CurrentControlSet\\Services\\CsDemo");
	HKEY hKey;
	uInt32 u32Value;
	if ( ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegistryKey, 0, KEY_QUERY_VALUE, &hKey) )
	{
		DWORD dwDataSize = sizeof(u32Value);
		long lError = ::RegQueryValueEx(hKey, _T("BoardCount"), NULL, NULL, (LPBYTE)&u32Value, &dwDataSize);
		if ( ERROR_SUCCESS == lError && 0 != u32Value)
		{
			pSystemInfo->u32BoardCount = u32Value;
		}
		lError = ::RegQueryValueEx(hKey, _T("TriggerCount"), NULL, NULL, (LPBYTE)&u32Value, &dwDataSize);
		if ( ERROR_SUCCESS == lError && 0 != u32Value )
		{
			pSystemInfo->u32TriggerMachineCount = u32Value;
		}
	}
#endif	// _WINDOWS

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CGageDemo::SetBoardDefaults(PCSBOARDINFO pBoardInfo)
{
	// The board count will always be 1
	// The board type and serial number fields will have been
	// set by the calling routine

	pBoardInfo->u32Size = sizeof(CSBOARDINFO);
	pBoardInfo->u32BoardIndex = 1; // Always only 1 demo board in a system
	pBoardInfo->u32BaseBoardVersion = 0x00000101;
	pBoardInfo->u32BaseBoardFirmwareVersion = 0;
	pBoardInfo->u32AddonBoardVersion = 0x00000101;
	pBoardInfo->u32AddonBoardFirmwareVersion = 0;
	pBoardInfo->u32AddonFwOptions = 0;
	pBoardInfo->u32BaseBoardFwOptions = 0;
	pBoardInfo->u32AddonHwOptions = 0;
	pBoardInfo->u32BaseBoardHwOptions = 0;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CGageDemo::SetExtendedSystemInfoDefaults(PCSSYSTEMINFOEX pSysInfoEx, PCS_FW_VER_INFO pVerInfo)
{
	pSysInfoEx->u32Size = sizeof(CSSYSTEMINFOEX);
	pSysInfoEx->u32AddonHwOptions = 0;
	pSysInfoEx->u32BaseBoardHwOptions = 0;
	pSysInfoEx->u32BaseBoardFwVersions[0].u32Reg = pVerInfo->BbFpgaInfo.u32Reg;
	pSysInfoEx->u32BaseBoardFwVersions[1].u32Reg = 0;
	pSysInfoEx->u32BaseBoardFwVersions[2].u32Reg = 0;
	pSysInfoEx->u32AddonFwVersions[0].u32Reg = pVerInfo->AddonFpgaFwInfo.u32Reg;
	pSysInfoEx->u32AddonFwVersions[1].u32Reg = 0;
	pSysInfoEx->u32AddonFwVersions[2].u32Reg = 0;
	pSysInfoEx->u32BaseBoardFwOptions[0] = 0;
	pSysInfoEx->u32BaseBoardFwOptions[1] = 0;
	pSysInfoEx->u32BaseBoardFwOptions[2] = 0;
	pSysInfoEx->u32AddonFwOptions[0] = 0;
	pSysInfoEx->u32AddonFwOptions[1] = 0;
	pSysInfoEx->u32AddonFwOptions[2] = 0;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CGageDemo::SetVersionInfoDefaults(PCS_FW_VER_INFO pVerInfo)
{
	pVerInfo->u32Size = sizeof(CS_FW_VER_INFO);
#ifdef _WINDOWS
	SYSTEMTIME st;
	::GetLocalTime(&st);
	TCHAR strDate[] = _T(__DATE__);
	char lpSeparators[] = _T(" ,");
	char *NextToken;

	unsigned int uMonth = CalculateMonth(::_tcstok_s(strDate, lpSeparators, &NextToken));
	unsigned int uDay = ::_ttoi(::_tcstok_s(NULL, lpSeparators, &NextToken));
	unsigned int uYear = ::_ttoi(::_tcstok_s(NULL, lpSeparators, &NextToken)) - 2000;
#else
	unsigned int uMonth = 1;
	unsigned int uDay = 1;
	unsigned int uYear = 1;
#endif


	pVerInfo->AddonFpgaFwInfo.Version.uMajor = 0;
	pVerInfo->AddonFpgaFwInfo.Version.uMinor = 0;
	pVerInfo->AddonFpgaFwInfo.Version.uRev = 1;
	pVerInfo->AddonFpgaFwInfo.Version.uIncRev = 0;

	pVerInfo->AnCpldFwInfo.Info.uMajor = 0;
	pVerInfo->AnCpldFwInfo.Info.uMinor = 0;
	pVerInfo->AnCpldFwInfo.Info.uRev = 1;
	pVerInfo->AnCpldFwInfo.Info.uReserved = 0;
	pVerInfo->AnCpldFwInfo.Info.uDay = uDay;
	pVerInfo->AnCpldFwInfo.Info.uMonth = uMonth;
	pVerInfo->AnCpldFwInfo.Info.uYear = uYear;

	pVerInfo->BbCpldInfo.Info.uMajor = 1;
	pVerInfo->BbCpldInfo.Info.uMinor = 0;

	pVerInfo->BbFpgaInfo.Version.uMajor = 0;
	pVerInfo->BbFpgaInfo.Version.uMinor = 0;
	pVerInfo->BbFpgaInfo.Version.uRev = 1;
	pVerInfo->BbFpgaInfo.Version.uIncRev = 0;

	pVerInfo->ConfCpldInfo.Info.uMajor = 0;
	pVerInfo->ConfCpldInfo.Info.uMinor = 0;
	pVerInfo->ConfCpldInfo.Info.uRev = 1;
	pVerInfo->ConfCpldInfo.Info.uReserved = 0;
	pVerInfo->ConfCpldInfo.Info.uDay = uDay;
	pVerInfo->ConfCpldInfo.Info.uMonth = uMonth;
	pVerInfo->ConfCpldInfo.Info.uYear = uYear;

	pVerInfo->TrigCpldFwInfo.Info.uMajor = 0;
	pVerInfo->TrigCpldFwInfo.Info.uMinor = 0;
	pVerInfo->TrigCpldFwInfo.Info.uRev = 1;
	pVerInfo->TrigCpldFwInfo.Info.uReserved = 0;
	pVerInfo->TrigCpldFwInfo.Info.uDay = uDay;
	pVerInfo->TrigCpldFwInfo.Info.uMonth = uMonth;
	pVerInfo->TrigCpldFwInfo.Info.uYear = uYear;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CGageDemoSystem* CGageDemo::GetDemoSystem(DRVHANDLE DrvHdl)
{
	DEMOSYSTEMMAP::iterator theMapIterator;
	if ((theMapIterator = m_DemoSystemMap.find(DrvHdl)) != m_DemoSystemMap.end())
		return ((*theMapIterator).second);
	else
		return NULL;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemo::GetAcquisitionSystemCount(uInt16 *u16SystemFound)
{
	int32 i32Status = CS_SUCCESS;

	if (NULL != u16SystemFound)
	{
		DWORD dwSystemsFound = GetSystemInfoFromFile();
		*u16SystemFound = static_cast<uInt16>(dwSystemsFound);
	}
#ifdef _WINDOWS
	if (*u16SystemFound )
	{
		char	szText[128];
		sprintf_s( szText, sizeof(szText), TEXT("Demo System(s) initialized\n"));
		CsEventLog EventLog;
		EventLog.Report( CS_INFO_TEXT, szText );	
	}
#endif // _WINDOWS

	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CGageDemo::GetAcquisitionSystemHandles(PDRVHANDLE pDrvHdl, uInt32)
{
	uInt32 u32SystemCount = static_cast<uInt32>(m_DemoSystemInfoMap.size());

	assert(m_u32SystemCount == u32SystemCount);

	if (m_u32SystemCount != u32SystemCount)
		m_u32SystemCount = u32SystemCount;

	DEMOSYSTEMINFOMAP::iterator theMapIterator;
	if (!m_DemoSystemInfoMap.empty())
	{
		uInt32 u32Index = 0;
		for (theMapIterator = m_DemoSystemInfoMap.begin(); theMapIterator  != m_DemoSystemInfoMap.end(); theMapIterator++)
		{
			pDrvHdl[u32Index] = Globals.it()->g_StaticLookupTable.DrvSystem[u32Index].DrvHandle = (*theMapIterator).first;
			u32Index++;
		}
	}

	return u32SystemCount;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemo::GetAcquisitionSystemInfo(DRVHANDLE DrvHdl, PCSSYSTEMINFO pSysInfo)
{
	if (m_DemoSystemInfoMap.empty())
	{
		GetSystemInfoFromFile();
		// if no systems are found it will be caught in the next section
		// because there will be no entry for that handle int he map
	}
	DEMOSYSTEMINFOMAP::iterator theMapIterator;
	if ((theMapIterator = m_DemoSystemInfoMap.find(DrvHdl)) != m_DemoSystemInfoMap.end())
	{
		// We can catch structure size errors here.
		if (pSysInfo->u32Size > sizeof(CSSYSTEMINFO))
		{
			ZeroMemory(pSysInfo, pSysInfo->u32Size);
			return CS_INVALID_STRUCT_SIZE;
		}
		if (0 == pSysInfo->u32Size)
		{
			ZeroMemory(pSysInfo, pSysInfo->u32Size);
			return CS_INVALID_STRUCT_SIZE;
		}
		// The structure size is equal or less than our structure so we can
		// copy the requested size requested size into the buffer.
		CopyMemory(pSysInfo, &(*theMapIterator).second.csSystemInfo, pSysInfo->u32Size);

		return CS_SUCCESS;
	}
	else
		return CS_INVALID_HANDLE;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemo::GetBoardsInfo(DRVHANDLE DrvHdl, PARRAY_BOARDINFO pBoardInfo)
{
	if (pBoardInfo->u32BoardCount > 1)
	{
		return CS_INVALID_CARD;
	}
	DEMOSYSTEMINFOMAP::iterator theMapIterator;
	if ((theMapIterator = m_DemoSystemInfoMap.find(DrvHdl)) != m_DemoSystemInfoMap.end())
	{
		// The structure size is equal or less than our structure so we can
		// copy the requested size requested size into the buffer.
		for (uInt32 i = 0; i < pBoardInfo->u32BoardCount; i++)
		{
			uInt32 u32Size = pBoardInfo->aBoardInfo[i].u32Size;
			// We can catch structure size errors here.
			if (u32Size > sizeof(CSBOARDINFO))
			{
				ZeroMemory(&pBoardInfo->aBoardInfo[i], u32Size);
				return CS_INVALID_STRUCT_SIZE;
			}
			if (0 == u32Size)
			{
				ZeroMemory(&pBoardInfo->aBoardInfo[i], u32Size);
				return CS_INVALID_STRUCT_SIZE;
			}
			if (1 != pBoardInfo->aBoardInfo[i].u32BoardIndex)
			{
				ZeroMemory(&pBoardInfo->aBoardInfo[i], u32Size);
				return CS_INVALID_CARD;
			}

			CopyMemory(&pBoardInfo->aBoardInfo[i], &(*theMapIterator).second.csBoardInfo, u32Size);
		}
		return CS_SUCCESS;
	}
	else
		return CS_INVALID_HANDLE;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemo::GetParams(DRVHANDLE DrvHdl, uInt32 u32ParamID, void *ParamBuffer)
{
	// CsDrvGetParams is called from CsRm with CS_SYSTEM_INFO_EX and
	// CS_READ_VER_INFO before AcquisitionSystemInit is ever called, so CGageDemoSystem
	// is not initialized yet. For these 2 cases we'll handle the call in the
	// CGageDemo class. All other parameters should go to the CGageDemoSystem class
	DEMOSYSTEMINFOMAP::iterator theMapIterator;
	if (CS_SYSTEMINFO_EX == u32ParamID)
	{
		PCSSYSTEMINFOEX pSysInfoEx = static_cast<PCSSYSTEMINFOEX>(ParamBuffer);
		if (pSysInfoEx->u32Size != sizeof(CSSYSTEMINFOEX))
			return CS_INVALID_STRUCT_SIZE;

		if ((theMapIterator = m_DemoSystemInfoMap.find(DrvHdl)) != m_DemoSystemInfoMap.end())
			CopyMemory(pSysInfoEx, &(*theMapIterator).second.csSystemExInfo, pSysInfoEx->u32Size);
		else
			return CS_INVALID_HANDLE;

		return CS_SUCCESS;
	}
	else if (CS_READ_VER_INFO == u32ParamID)
	{
		PCS_FW_VER_INFO pVerInfo = static_cast<PCS_FW_VER_INFO>(ParamBuffer);
		if (pVerInfo->u32Size != sizeof(CS_FW_VER_INFO))
			return CS_INVALID_STRUCT_SIZE;

		if ((theMapIterator = m_DemoSystemInfoMap.find(DrvHdl)) != m_DemoSystemInfoMap.end())
			CopyMemory(pVerInfo, &(*theMapIterator).second.csVerInfo, pVerInfo->u32Size);
		else
			return CS_INVALID_HANDLE;

		return CS_SUCCESS;
	}
	else
		return (GetDemoSystem(DrvHdl)->GetParams(u32ParamID, ParamBuffer));
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemo::AcquisitionSystemInit(DRVHANDLE DrvHdl)
{
	// check if sysinfo and boardinfo and handle are valid

	// If the call to AcquisitionSystemInit is coming from an application,
	// the DemoSystemInfoMap may be empty because it's filled up on the call
	// to GetAcquisitionSystemCount which is called by CsRm. In that case this
	// instance of the CsDemo.dll will not have the map filled in so we'll do
	// it now.

	
	// Nov 30, 2012 - Always re-read the system information from the file. This fixes a
	// bug where a second system that was initialized afterwards ( but without restarting RM )
	// would sometimes not be recognized by RM.

	GetSystemInfoFromFile();

	DEMOSYSTEMINFOMAP::iterator theMapIterator;
	if ((theMapIterator = m_DemoSystemInfoMap.find(DrvHdl)) != m_DemoSystemInfoMap.end())
	{
		PCSSYSTEMINFO pSystemInfo = &(*theMapIterator).second.csSystemInfo;
		PCSBOARDINFO pBoardInfo = &(*theMapIterator).second.csBoardInfo;
		PCSSYSTEMINFOEX pSysInfoEx = &(*theMapIterator).second.csSystemExInfo;
		PCS_FW_VER_INFO pVerInfo = &(*theMapIterator).second.csVerInfo;
		ChannelInfo ChanInfo = (*theMapIterator).second.csChannelInfo;
		SystemCapsInfo sysCaps = (*theMapIterator).second.csSysCaps;
		string strSysName = (*theMapIterator).second.tcSystemName;
		CGageDemoSystem* pDemoSystem = new CGageDemoSystem(DrvHdl, this, pSystemInfo, pBoardInfo, pSysInfoEx,
			pVerInfo, ChanInfo, sysCaps, strSysName);

		m_DemoSystemMap.insert(DEMOSYSTEMMAP::value_type(DrvHdl, pDemoSystem));
		return (pDemoSystem->AcquisitionSystemInit());
	}
	else
		return (CS_INVALID_HANDLE);
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemo::AcquisitionSystemCleanup(DRVHANDLE DrvHdl)
{
	DEMOSYSTEMMAP::iterator theMapIterator;

	int32 i32Status = GetDemoSystem(DrvHdl)->AcquisitionSystemCleanup();

	if (CS_SUCCESS != i32Status)
		return i32Status;
	else
	{
		if ((theMapIterator = m_DemoSystemMap.find(DrvHdl)) != m_DemoSystemMap.end())
			m_DemoSystemMap.erase(theMapIterator);
		else
			i32Status = CS_INVALID_HANDLE;
	}
	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CGageDemo::RefreshSystemSignals(DRVHANDLE DrvHdl, ChannelInfo& ChanInfo)
{
	DEMOSYSTEMINFOMAP::iterator theMapIterator;

	if ((theMapIterator = m_DemoSystemInfoMap.find(DrvHdl)) != m_DemoSystemInfoMap.end())
	{
		ChannelInfo csChInfo = (*theMapIterator).second.csChannelInfo;

		XML* pSystem = new XML(m_strXmlFileName.c_str());
		if (!pSystem)
			return CS_NO_SYSTEMS_FOUND;

		XMLElement* pRoot = pSystem->GetRootElement();
		if (!pRoot)
			return CS_NO_SYSTEMS_FOUND;

		int nChildren = pRoot->GetChildrenNum();
		if (0 == nChildren)
			return CS_INVALID_CHANNEL_COUNT;

		uInt32 u32SampleBits = (*theMapIterator).second.csSystemInfo.u32SampleBits;

		XMLElement* pBits;
		if (CS_SAMPLE_BITS_8 == u32SampleBits)
			pBits = pRoot->FindElementZ((char *)XML_8_BIT_NODE);
		else if (CS_SAMPLE_BITS_12 == u32SampleBits)
			pBits = pRoot->FindElementZ((char *)XML_12_BIT_NODE);
		else if (CS_SAMPLE_BITS_14 == u32SampleBits)
			pBits = pRoot->FindElementZ((char *)XML_14_BIT_NODE);
		else if (CS_SAMPLE_BITS_16 == u32SampleBits)
			pBits = pRoot->FindElementZ((char *)XML_16_BIT_NODE);
		else
		{
			delete pSystem;
			return CS_NO_SYSTEMS_FOUND;
		}

		if (!pBits)
		{
			delete pSystem;
			return CS_NO_SYSTEMS_FOUND;
		}

		XMLElement* pElement = pBits->FindElementZ((*theMapIterator).second.tcSystemName);

		if (!pElement)
		{
			delete pSystem;
			return CS_NO_SYSTEMS_FOUND;
		}
		DWORD dwChannelCount = EnumerateSystemChannelInfo(pElement, csChInfo);
		(*theMapIterator).second.csChannelInfo = csChInfo;

		uInt32 u32Last = static_cast<uInt32>(csChInfo.size());

		for (uInt32 i = 0; i < u32Last; i++)
			ChanInfo.push_back(csChInfo.at(i));

		delete pSystem;
		return dwChannelCount;
	}
	else
		return CS_INVALID_HANDLE;
}


