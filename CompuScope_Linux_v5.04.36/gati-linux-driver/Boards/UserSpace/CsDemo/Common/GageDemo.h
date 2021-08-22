/*
 * GageDemo.h
 *
 *  Created on: 24-Jul-2008
 *      Author: egiard
 */

#ifndef GAGEDEMO_H_
#define GAGEDEMO_H_

#include <CsTypes.h>
#include <CsThreadLock.h>
#include <CsPrivateStruct.h>
#include <CsBaseHwDLL.h>
#include "xml.h"

// temp Added temporarily before all headers files are converted

#include <string>
#include <misc.h>

// temp

#include <map>
#include <vector>

using namespace std;

#define DEMO_BASE_HANDLE 0x123400

UINT GetGageDirectory(LPTSTR str, UINT uSize);

typedef enum _ChannelType
{
	echSine = 0,
	echSquare = echSine +1,
	echTriangle = echSquare +1,
	echTwoTone = echTriangle +1,

} ChannelType;

typedef vector <ChannelType> ChannelInfo;


typedef struct _SystemCapsInfo
{
	vector <CSSAMPLERATETABLE_EX> veSampleRate;
	vector <CSRANGETABLE>         veInputRange;
	unsigned long				  uTriggerSense;

} SystemCapsInfo;


typedef struct _DEMOSYTEMINFO
{
	DRVHANDLE		drvHandle;
	CSSYSTEMINFO	csSystemInfo;
	CSBOARDINFO		csBoardInfo;
	CSSYSTEMINFOEX	csSystemExInfo;
	CS_FW_VER_INFO	csVerInfo;
	ChannelInfo		csChannelInfo;
	SystemCapsInfo	csSysCaps;
	TCHAR			tcSystemName[24];

} DEMOSYSTEMINFO, *PDEMOSYSTEMINFO;


class CGageDemoSystem;

typedef map<CSHANDLE, CGageDemoSystem *> DEMOSYSTEMMAP;
typedef map<CSHANDLE, DEMOSYSTEMINFO> DEMOSYSTEMINFOMAP;


class CGageDemo
{
private:
	CGageDemo(void);
	~CGageDemo(void);

	static CGageDemo* m_pInstance;
	static Critical_Section __key;

public:
	static CGageDemo& GetDemoInstance()
	{
		if( NULL == m_pInstance )
		{
			Lock_Guard<Critical_Section> __lock(__key);
			if(NULL == m_pInstance )
			{
				m_pInstance = new CGageDemo();
			}
		}
		return *m_pInstance;
	}

	static void RemoveDemoInstance()
	{
		Lock_Guard<Critical_Section> __lock(__key);
		delete m_pInstance;
		m_pInstance = NULL;
	}

	CGageDemoSystem* GetDemoSystem(DRVHANDLE DrvHdl);

	int32 GetAcquisitionSystemCount(uInt16 *u16SystemFound);
	uInt32 GetAcquisitionSystemHandles(PDRVHANDLE, uInt32);
	int32 AcquisitionSystemInit(DRVHANDLE DrvHdl);
	int32 GetAcquisitionSystemInfo(DRVHANDLE DrvHdl, PCSSYSTEMINFO pSysInfo);
	int32 AcquisitionSystemCleanup(DRVHANDLE DrvHdl);
	int32 GetBoardsInfo(DRVHANDLE DrvHdl, PARRAY_BOARDINFO pABoardInfo);
	int32 GetParams(DRVHANDLE DrvHdl, uInt32 u32ParamID, void *ParamBuffer);
	int32 RefreshSystemSignals(DRVHANDLE DrvHdl, ChannelInfo& ChanInfo);

private:

	uInt32 GetSystemsFromFile(XMLElement *pElement, uInt32 u32SampleBits, uInt32 u32Offset);
	uInt32 GetSystemInfoFromFile(void);
	void SetSystemDefaults(PCSSYSTEMINFO pSystemInfo);
	void SetBoardDefaults(PCSBOARDINFO pBoardInfo);
	void SetVersionInfoDefaults(PCS_FW_VER_INFO pVerInfo);
	void SetExtendedSystemInfoDefaults(PCSSYSTEMINFOEX pSysInfoEx, PCS_FW_VER_INFO pVerInfo);

	static DWORD EnumerateSystemChannelInfo(XMLElement* pSystem, ChannelInfo& csChInfo);
	static bool EnumerateSystemCapsInfo(XMLElement* pCaps, SystemCapsInfo& csCapsInfo);

	uInt32 m_u32SystemCount;
	DEMOSYSTEMMAP m_DemoSystemMap;
	DEMOSYSTEMINFOMAP m_DemoSystemInfoMap;
	DRVHANDLE* m_pDrvHandles;
	string m_strXmlFileName;
};

#endif /* GAGEDEMO_H_ */
