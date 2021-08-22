#include "StdAfx.h"
#include <assert.h>
#include "GageSystemManager.h"
#include <winsock2.h>
#include "misc.h"
#include "CsErrors.h"
#include "GageSocket.h"
#include "RemoteSystemInfo.h"
#include "VxiDiscovery.h"
#include "DllStruct.h"
#include "CsEventLog.h"
#include "CsMsgLog.h"
#include "Debug.h"

const int AF_IPV4   = 0;
const int AF_IPV6   = 1;
const int SOCK_TCP  = SOCK_STREAM-1;
const int SOCK_UDP  = SOCK_DGRAM-1;

const int g_MaxRemoteTransferChunk = 8388608;

#define SHARED_MAP_SECTION _T(CS_FCI_SHARED_MEM)
REMOTESYSTEM fciSystems (SHARED_MAP_SECTION);

bool isSmaller ( systemIpInfo elem1, systemIpInfo elem2 )
{
	
	struct in_addr addr1;
	unsigned int i;
	
	struct hostent *phe1 = gethostbyname(elem1.strIpAddress.c_str());

	memcpy(&addr1, *phe1->h_addr_list, sizeof(struct in_addr));
	i =  (addr1.S_un.S_un_b.s_b1 << 24) | 
				(addr1.S_un.S_un_b.s_b2 << 16) | 
				(addr1.S_un.S_un_b.s_b3 << 8) | 
				 addr1.S_un.S_un_b.s_b4;

	unsigned int j;	
	struct in_addr addr2;	
	struct hostent *phe2 = gethostbyname(elem2.strIpAddress.c_str());

	memcpy(&addr2, *phe2->h_addr_list, sizeof(struct in_addr));
	j =  (addr2.S_un.S_un_b.s_b1 << 24) | 
				(addr2.S_un.S_un_b.s_b2 << 16) | 
				(addr2.S_un.S_un_b.s_b3 << 8) | 
				 addr2.S_un.S_un_b.s_b4;
	return i < j;
}


CGageSystemManager::CGageSystemManager(void)
{
#ifdef WIN32
	// Start up Winsock for the application process
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD( 2, 2 );
	int nErrorCode = ::WSAStartup(wVersionRequested, &wsaData);
	if ( 0 != nErrorCode )
	{
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.         */
		nErrorCode = ::GetLastSocketError();
		DisplayErrorMessage(nErrorCode);

		// also report it to the event log
		TCHAR szText[128];
		_stprintf_s( szText, sizeof(szText), TEXT("Could not start WinSock. Error code: %d"), nErrorCode);
		CsEventLog EventLog;
		EventLog.Report( CS_ERROR_TEXT, szText );
		// we don't need to stop. If there was a problem with Winsock we'll get
		// an error on our first Winsock call
	}
#endif
}

CGageSystemManager::~CGageSystemManager(void)
{
#ifdef WIN32
	// RG - Note: Have to watch this. We're calling WSACleanup from DllMain which
	//			  you're not supposed to do, so this could cause problems
	::WSACleanup();
#endif

	if ( !m_deviceMap.empty() )
	{
		SystemMap::iterator it;
		for ( it = m_deviceMap.begin(); it != m_deviceMap.end(); ++it )
		{
			if ( (*it).second.pSystem != NULL )
			{
				delete (*it).second.pSystem;
			}
		}
		m_deviceMap.clear();
	}
}
int32 CGageSystemManager::GetSystemIndex( CSHANDLE csHandle )
{
	int notFound = -1;
	for ( int i = 0; i < fciSystems.it()->count; i++ )
	{
		if ( csHandle == fciSystems.it()->System[i].csHandle )
		{
			return i;
		}
	}
	return notFound;		
}

int32 CGageSystemManager::GetAcquisitionSystemCount( uInt16 bForceIndependantMode, uInt16* pu16SystemFound )
{
	vecIpInfo vSystem, vGageSystem;
	int err;

	int num = FindRemoteSystems(vSystem, &err);
	if ( 0 > num ) // if num is negative there was an error
	{
		LPVOID lpMsgBuf;

		if (FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER | 
							FORMAT_MESSAGE_FROM_SYSTEM | 
							FORMAT_MESSAGE_IGNORE_INSERTS,
							NULL,
							err,
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
							(LPTSTR) &lpMsgBuf,
							0,
							NULL ))
			{
				CsEventLog EventLog;
				EventLog.Report(CS_ERROR_TEXT, (LPTSTR)lpMsgBuf);

				// Display the string.
				TRACE(MISC_TRACE_LEVEL, (LPCTSTR)lpMsgBuf);
				// Free the buffer.
				LocalFree( lpMsgBuf );
			}

		*pu16SystemFound = 0;
		return CS_SOCKET_ERROR;
	}

	for ( int i = 0; i < g_MaxSystemCount; i++ )
	{
		fciSystems.it()->System[i].dwPort = 0;
		_tcscpy_s(fciSystems.it()->System[i].strIPAddress, _countof(fciSystems.it()->System[i].strIPAddress), _T(""));
	}
	fciSystems.it()->count = 0;

	for ( int i = 0; i < num; i++ )
	{
		const char* szIpAddress = vSystem[i].strIpAddress.c_str();
		if ( 1 == IsGageSystem((char*)szIpAddress) )
		{
			{
				vSystem[i].dwPort += 1;
				vGageSystem.push_back(vSystem[i]);
			}
		}
	}
//	Sort the Gage systems by IP Address so they always come up in the same order

	::sort(vGageSystem.begin(), vGageSystem.end(), isSmaller);

	uInt16 u16SystemCount = 0;
	int systemCount = ( vGageSystem.size() < g_MaxSystemCount ) ? (int)vGageSystem.size() : g_MaxSystemCount;
	int systemFound = 0;
	for ( int i = 0; i < systemCount; i++ )
	{
		CGageSocket cSystem(vGageSystem[i].strIpAddress, vGageSystem[i].dwPort);
		int32 i32Result = cSystem.GetAcquisitionSystemCount( bForceIndependantMode, pu16SystemFound );

		if ( CS_SUCCEEDED(i32Result) && *pu16SystemFound > 0 )
		{
			fciSystems.it()->System[systemFound].dwPort = vGageSystem[i].dwPort;
			_tcscpy_s(fciSystems.it()->System[systemFound].strIPAddress, _countof(fciSystems.it()->System[i].strIPAddress), vGageSystem[i].strIpAddress.c_str());
			fciSystems.it()->System[systemFound].csHandle = 0;
			u16SystemCount += *pu16SystemFound;
			systemFound++;
		}
		else
		{
			CsEventLog EventLog;
			TCHAR	szText[128];
			sprintf_s( szText, _countof(szText), TEXT("Device at %s reports error %d"), vGageSystem[i].strIpAddress.c_str(), i32Result);
			EventLog.Report(CS_ERROR_TEXT, (LPTSTR)szText);

			// Display the string.
			TRACE(MISC_TRACE_LEVEL, szText);
			fciSystems.it()->System[i].dwPort = 0;
			*pu16SystemFound = 0;
		}
	}
	fciSystems.it()->count = u16SystemCount;

	*pu16SystemFound = u16SystemCount;

	return CS_SUCCESS;	 // RG WHAT ABOUT AN ERROR
}

uInt32 CGageSystemManager::GetAcquisitionSystemHandles( PDRVHANDLE pDrvHdl, uInt32 u32Size )
{
	uInt32 u32Ret;
	UNREFERENCED_PARAMETER(u32Size); // RG CHECK WHY
	// u32Size should be number of systems ( in this case vecGageSystem.size() * sizeof(DWORD) )
//	_ASSERT( u32Size == m_vSystem.size() * sizeof(DWORD) );
//	if ( u32Size != m_vSystem.size() * sizeof(DWORD) )
//	{
		// error
//	}

	int nIndex = 0;
	uInt32 u32SystemCount = 0;
	DRVHANDLE drvHandle;

	for ( int i = 0; i < fciSystems.it()->count; i++ )
	{
		CGageSocket* p = new CGageSocket(fciSystems.it()->System[i].strIPAddress, fciSystems.it()->System[i].dwPort);
		u32Ret = p->GetAcquisitionSystemHandles( &drvHandle, sizeof(DRVHANDLE));
		if ( 0 != u32Ret )
		{
			pDrvHdl[nIndex++] = drvHandle;
			fciSystems.it()->System[i].csHandle = drvHandle;
//
			systemInfo sysInfo;
			sysInfo.strIPAddress = fciSystems.it()->System[i].strIPAddress;
			sysInfo.dwPort = fciSystems.it()->System[i].dwPort;
			sysInfo.pSystem = p;
			m_deviceMap[fciSystems.it()->System[i].csHandle] = sysInfo;

//
			u32SystemCount++;
		}
		else
		{
			delete p;
		}
	}
	return u32SystemCount;
}

int32 CGageSystemManager::CloseSystemForRm( CSHANDLE csHandle )
{
	int32 i32Index = GetSystemIndex(csHandle);
	if ( i32Index < 0 )
	{
		return CS_INVALID_HANDLE;
	}

	return m_deviceMap[csHandle].pSystem->CloseSystemForRm(csHandle);
}


int32 CGageSystemManager::OpenSystemForRm( CSHANDLE csHandle )
{
	int32 i32Index = GetSystemIndex(csHandle);
	if ( i32Index < 0 )
	{
		return CS_INVALID_HANDLE;
	}

	return m_deviceMap[csHandle].pSystem->OpenSystemForRm(csHandle);
}



int32 CGageSystemManager::AcquisitionSystemInit( CSHANDLE csHandle, BOOL bResetDefaultSetting )
{
	int32 i32Index = GetSystemIndex(csHandle);
	if ( i32Index < 0 )
	{
		return CS_INVALID_HANDLE;
	}

	if ( m_deviceMap.empty() || m_deviceMap.find(csHandle) == m_deviceMap.end() )
	{
		// either create the map or add this new handle to it
		systemInfo sysInfo;
		sysInfo.dwPort = fciSystems.it()->System->dwPort;
		sysInfo.strIPAddress = fciSystems.it()->System->strIPAddress;
		sysInfo.pSystem = new CGageSocket(sysInfo.strIPAddress.c_str(), sysInfo.dwPort);
		m_deviceMap[csHandle] = sysInfo;
	}
	return m_deviceMap[csHandle].pSystem->AcquisitionSystemInit(csHandle, bResetDefaultSetting);
}

int32 CGageSystemManager::GetAcquisitionSystemInfo( CSHANDLE csHandle, PCSSYSTEMINFO pSysInfo )
{
	int32 i32Index = GetSystemIndex(csHandle);
	if ( i32Index < 0 )
	{
		return CS_INVALID_HANDLE;
	}

	return m_deviceMap[csHandle].pSystem->GetAcquisitionSystemInfo(csHandle, pSysInfo);
}

int32 CGageSystemManager::GetBoardsInfo( CSHANDLE csHandle, PARRAY_BOARDINFO pBoardInfo )
{
	int32 i32Index = GetSystemIndex(csHandle);
	if ( i32Index < 0 )
	{
		return CS_INVALID_HANDLE;
	}

	return m_deviceMap[csHandle].pSystem->GetBoardsInfo(csHandle, pBoardInfo);	
}

int32 CGageSystemManager::GetAcquisitionSystemCaps( CSHANDLE csHandle, uInt32 u32CapsId, PCSSYSTEMCONFIG pSysCfg, void* pBuffer, uInt32* pu32BufferSize )
{
	int32 i32Index = GetSystemIndex(csHandle);
	if ( i32Index < 0 )
	{
		return CS_INVALID_HANDLE;
	}

	return m_deviceMap[csHandle].pSystem->GetAcquisitionSystemCaps(csHandle, u32CapsId, pSysCfg, pBuffer, pu32BufferSize);
}

int32 CGageSystemManager::AcquisitionSystemCleanup( CSHANDLE csHandle )
{
	// RG - REMOVE FROM SHARED MEMORY (AND MAP)
	int32 i32Index = GetSystemIndex(csHandle);
	if ( i32Index < 0 )
	{
		return CS_INVALID_HANDLE;
	}
	return m_deviceMap[csHandle].pSystem->AcquisitionSystemCleanup(csHandle);
}

int32 CGageSystemManager::GetParams( CSHANDLE csHandle, uInt32 u32ParamID, void* pParamBuffer )
{
	int32 i32Index = GetSystemIndex(csHandle);
	if ( i32Index < 0 )
	{
		return CS_INVALID_HANDLE;
	}

	return m_deviceMap[csHandle].pSystem->GetParams(csHandle, u32ParamID, pParamBuffer);
}

int32 CGageSystemManager::SetParams( CSHANDLE csHandle, uInt32 u32ParamID, void* pParamBuffer )
{
	int32 i32Index = GetSystemIndex(csHandle);
	if ( i32Index < 0 )
	{
		return CS_INVALID_HANDLE;
	}

	return m_deviceMap[csHandle].pSystem->SetParams(csHandle, u32ParamID, pParamBuffer);
}

int32 CGageSystemManager::ValidateParams( CSHANDLE csHandle, uInt32 u32ParamID, uInt32 u32Coerce, void* pParamBuffer )
{
	int32 i32Index = GetSystemIndex(csHandle);
	if ( i32Index < 0 )
	{
		return CS_INVALID_HANDLE;
	}

	return m_deviceMap[csHandle].pSystem->ValidateParams(csHandle, u32ParamID, u32Coerce, pParamBuffer);
}

int32 CGageSystemManager::Do( CSHANDLE csHandle, uInt32 u32ActionID, void* pActionBuffer )
{
	int32 i32Index = GetSystemIndex(csHandle);
	if ( i32Index < 0 )
	{
		return CS_INVALID_HANDLE;
	}

	return m_deviceMap[csHandle].pSystem->Do(csHandle, u32ActionID, pActionBuffer);
}

int32 CGageSystemManager::GetAcquisitionStatus( CSHANDLE csHandle, uInt32* pu32Status )
{
	int32 i32Index = GetSystemIndex(csHandle);
	if ( i32Index < 0 )
	{
		return CS_INVALID_HANDLE;
	}

	return m_deviceMap[csHandle].pSystem->GetAcquisitionStatus(csHandle, pu32Status);
}

int32 CGageSystemManager::TransferData( CSHANDLE csHandle, IN_PARAMS_TRANSFERDATA inParams, POUT_PARAMS_TRANSFERDATA pOutParams )
{
	int32 i32Index = GetSystemIndex(csHandle);
	if ( i32Index < 0 )
	{
		return CS_INVALID_HANDLE;
	}


	// if inParams.pDataBuffer is NULL the caller just wants the actual size and length, not the data
	if ( NULL == inParams.pDataBuffer )
	{
		return m_deviceMap[csHandle].pSystem->TransferData(csHandle, inParams, pOutParams);
	}

	if ( g_MaxRemoteTransferChunk > inParams.i64Length )
	{
		return m_deviceMap[csHandle].pSystem->TransferData(csHandle, inParams, pOutParams);
	}
	else
	{
		int64 i64TotalTransferSize = inParams.i64Length;
		int64 i64TransferChunk = g_MaxRemoteTransferChunk; // samples
		int	nTransferNumber = 0;

		IN_PARAMS_TRANSFERDATA inParamsTemp;
		OUT_PARAMS_TRANSFERDATA outParamsTemp;
		::memcpy(&inParamsTemp, &inParams, sizeof(IN_PARAMS_TRANSFERDATA));
		
		uInt32 u32SampleSize = 0;
		while ( i64TotalTransferSize > 0 )
		{
			// adjust what needs to be adjusted
			inParamsTemp.i64Length =  i64TransferChunk; //  in samples

			int32 i32Status = m_deviceMap[csHandle].pSystem->TransferData(csHandle, inParamsTemp, &outParamsTemp, &u32SampleSize);

			if ( CS_FAILED(i32Status) )
			{
				return i32Status;
			}
			if ( 0 == nTransferNumber )
			{
				pOutParams->i64ActualStart = outParamsTemp.i64ActualStart;
				pOutParams->i32LowPart = outParamsTemp.i32LowPart;
				pOutParams->i32HighPart = outParamsTemp.i32HighPart;
			}
			pOutParams->i64ActualLength += outParamsTemp.i64ActualLength;
			i64TotalTransferSize -= i64TransferChunk;
			
			// increment the start address and the data buffer with the previously used transfer chunk
			inParamsTemp.i64StartAddress += i64TransferChunk;
			inParamsTemp.pDataBuffer = (char*)inParamsTemp.pDataBuffer + (i64TransferChunk * u32SampleSize);
			i64TransferChunk = ( g_MaxRemoteTransferChunk < i64TotalTransferSize ) ? g_MaxRemoteTransferChunk : i64TotalTransferSize;
			nTransferNumber++;
		}

		return CS_SUCCESS;
	}
}

int32 CGageSystemManager::SetSystemInUse( CSHANDLE csHandle, BOOL bSet )
{
	int32 i32Index = GetSystemIndex(csHandle);
	if ( i32Index < 0 )
	{
		return CS_INVALID_HANDLE;
	}

	return m_deviceMap[csHandle].pSystem->SetSystemInUse(csHandle, bSet);
}

int32 CGageSystemManager::IsSystemInUse( CSHANDLE csHandle, BOOL* bSet )
{
	int32 i32Index = GetSystemIndex(csHandle);
	if ( i32Index < 0 )
	{
		return CS_INVALID_HANDLE;
	}

	return m_deviceMap[csHandle].pSystem->IsSystemInUse(csHandle, bSet);
}
