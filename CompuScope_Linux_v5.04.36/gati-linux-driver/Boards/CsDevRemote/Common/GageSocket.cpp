#include "StdAfx.h"
#include "GageSocket.h"
#include "CsErrors.h"
#include "CsDefines.h"
#include "CsPrivateStruct.h"
#include "CsDevRemote_Defines.h"
#include "VxiDiscovery.h"


#define SOCKET_WAIT_TIME_LIMIT 3000 // 3 Seconds


void SysConfigToBuffer( PCSSYSTEMCONFIG pSystemConfig, char* pBuffer )
{
	// flattens out the ACQUISITION, ARRAY_CHANNEL and ARRAY_TRIGGER structures in pSystemConfig
	// and copies them into pBuffer.  Caller has to assure that the pointers are valid, and
	// the the buffers have been properly allocated
	DWORD dwOffset = 0;
	::memcpy(pBuffer + dwOffset, pSystemConfig->pAcqCfg, pSystemConfig->pAcqCfg->u32Size);
	dwOffset += pSystemConfig->pAcqCfg->u32Size;
	DWORD dwChanSize = ((pSystemConfig->pAChannels->u32ChannelCount - 1) * sizeof(CSCHANNELCONFIG)) + sizeof(ARRAY_CHANNELCONFIG);
	::memcpy(pBuffer + dwOffset, pSystemConfig->pAChannels, dwChanSize);
	dwOffset += dwChanSize;
	DWORD dwTrigSize = ((pSystemConfig->pATriggers->u32TriggerCount - 1) * sizeof(CSTRIGGERCONFIG)) + sizeof(ARRAY_TRIGGERCONFIG);
	::memcpy(pBuffer + dwOffset, pSystemConfig->pATriggers, dwTrigSize);
}

void BufferToSysConfig ( char* pBuffer, PCSSYSTEMCONFIG pSystemConfig )
{
	// copies the structures from the pBuffer back into pSystemConfig. Caller is responsible for
	// ensuring all pointers are valid and properly allocated

	// RG - CHECK SIZES, COPY SMALLER AMOUNT ALWAYS ???
	DWORD dwOffset = 0;
	::memcpy(pSystemConfig->pAcqCfg, pBuffer + dwOffset, pSystemConfig->pAcqCfg->u32Size);
	dwOffset += pSystemConfig->pAcqCfg->u32Size;
	DWORD dwChanSize = ((pSystemConfig->pAChannels->u32ChannelCount - 1) * sizeof(CSCHANNELCONFIG)) + sizeof(ARRAY_CHANNELCONFIG);
	::memcpy(pSystemConfig->pAChannels, pBuffer + dwOffset, dwChanSize);
	dwOffset += dwChanSize;
	DWORD dwTrigSize = ((pSystemConfig->pATriggers->u32TriggerCount - 1) * sizeof(CSTRIGGERCONFIG)) + sizeof(ARRAY_TRIGGERCONFIG);
	::memcpy(pSystemConfig->pATriggers, pBuffer + dwOffset, dwTrigSize);
}


size_t GetSysConfigSize(PCSSYSTEMCONFIG pSystemConfig)
{
	size_t tSize = 0;

	if ( !pSystemConfig )
	{
		return 0;
	}
	tSize += sizeof(CSACQUISITIONCONFIG);
	tSize += (sizeof(ARRAY_CHANNELCONFIG) + ((pSystemConfig->pAChannels->u32ChannelCount - 1) * sizeof(CSCHANNELCONFIG)));
	tSize += (sizeof(ARRAY_TRIGGERCONFIG) + ((pSystemConfig->pATriggers->u32TriggerCount - 1) * sizeof(CSTRIGGERCONFIG)));
	return tSize;
}


int32 IsSystemConfigValid(PCSSYSTEMCONFIG pSystemConfig)
{
	if ( NULL == pSystemConfig->pAcqCfg )
	{
		return CS_NULL_POINTER;
	}
	if ( sizeof(CSACQUISITIONCONFIG) != pSystemConfig->pAcqCfg->u32Size )
	{
		return CS_INVALID_STRUCT_SIZE;
	}
	if ( NULL == pSystemConfig->pAChannels )
	{
		return CS_NULL_POINTER;
	}
	for ( uInt32 i = 0; i < pSystemConfig->pAChannels->u32ChannelCount; i++ )
	{
		if ( sizeof(CSCHANNELCONFIG) != pSystemConfig->pAChannels->aChannel[i].u32Size )
		{
			return CS_INVALID_STRUCT_SIZE;
		}
	}
	if ( NULL == pSystemConfig->pATriggers )
	{
		return CS_NULL_POINTER;
	}
	for ( uInt32 i = 0; i < pSystemConfig->pATriggers->u32TriggerCount; i++ )
	{
		if ( sizeof(CSTRIGGERCONFIG) != pSystemConfig->pATriggers->aTrigger[i].u32Size )
		{
			return CS_INVALID_STRUCT_SIZE;
		}
	}
	return CS_SUCCESS;
}


int32 GetParamBufferSize( uInt32 u32ParamID, PVOID pParamBuffer )
{
	int i32Return;

	switch ( u32ParamID )
	{
		case CS_SYSTEM:
		{
			int32 i32BufferSize = 0;
			if ( sizeof(CSSYSTEMCONFIG) != ((PCSSYSTEMCONFIG)pParamBuffer)->u32Size )
			{
				return CS_INVALID_STRUCT_SIZE;
			}
			if ( sizeof(CSACQUISITIONCONFIG) != ((PCSSYSTEMCONFIG)pParamBuffer)->pAcqCfg->u32Size )
			{
				return CS_INVALID_STRUCT_SIZE;
			}
			i32BufferSize += ((PCSSYSTEMCONFIG)pParamBuffer)->pAcqCfg->u32Size;
			
			PARRAY_CHANNELCONFIG pChanCfg = ((PCSSYSTEMCONFIG)pParamBuffer)->pAChannels;
			for ( uInt32 i = 0; i < pChanCfg->u32ChannelCount; i++ )
			{
				if ( sizeof(CSCHANNELCONFIG) != pChanCfg->aChannel[i].u32Size )
				{
					return CS_INVALID_STRUCT_SIZE;
				}
			}
			i32BufferSize += ((pChanCfg->u32ChannelCount - 1) * sizeof(CSCHANNELCONFIG)) + sizeof(ARRAY_CHANNELCONFIG);

			PARRAY_TRIGGERCONFIG pTrigCfg = ((PCSSYSTEMCONFIG)pParamBuffer)->pATriggers;
			for ( uInt32 i = 0; i < pTrigCfg->u32TriggerCount; i++ )
			{
				if ( sizeof(CSTRIGGERCONFIG) != pTrigCfg->aTrigger[i].u32Size )
				{
					return CS_INVALID_STRUCT_SIZE;
				}
			}
			i32BufferSize += ((pTrigCfg->u32TriggerCount - 1) * sizeof(CSTRIGGERCONFIG)) + sizeof(ARRAY_TRIGGERCONFIG);

			i32Return = i32BufferSize;
		}
		break;

		case CS_ACQUISITION:
		{
			if ( sizeof(CSACQUISITIONCONFIG) != ((PCSACQUISITIONCONFIG)pParamBuffer)->u32Size )
			{
				return CS_INVALID_STRUCT_SIZE;
			}
			i32Return = sizeof(CSACQUISITIONCONFIG);
		}
		break;

		case CS_TRIGGER:
		{
			PARRAY_TRIGGERCONFIG pCfg = (PARRAY_TRIGGERCONFIG) pParamBuffer;
			for ( uInt32 i = 0; i < pCfg->u32TriggerCount; i++ )
			{
				if ( sizeof(CSTRIGGERCONFIG) != pCfg->aTrigger[i].u32Size )
				{
					return CS_INVALID_STRUCT_SIZE;
				}
			}
			i32Return = ((pCfg->u32TriggerCount - 1) * sizeof(CSTRIGGERCONFIG)) + sizeof(ARRAY_TRIGGERCONFIG);
		}
		break;

		case CS_CHANNEL:
		{
			PARRAY_CHANNELCONFIG pCfg = (PARRAY_CHANNELCONFIG) pParamBuffer;
			for ( uInt32 i = 0; i < pCfg->u32ChannelCount; i++ )
			{
				if ( sizeof(CSCHANNELCONFIG) != pCfg->aChannel[i].u32Size )
				{
					return CS_INVALID_STRUCT_SIZE;
				}
			}
			i32Return = ((pCfg->u32ChannelCount - 1) * sizeof(CSCHANNELCONFIG)) + sizeof(ARRAY_CHANNELCONFIG);
		}
		break;
/*
		case CS_READ_NVRAM: //---- Read NvRam (Boot Eeprom)
		{
			i32Return = sizeof(PPLX_NVRAM_IMAGE); //RG DIFFERENT TYPES FOR EACH BOARD
		}
		break;
*/
		case CS_READ_VER_INFO:
		{
			if ( sizeof(CS_FW_VER_INFO) != ((PCS_FW_VER_INFO)pParamBuffer)->u32Size )
			{
				return CS_INVALID_STRUCT_SIZE;
			}
			i32Return = sizeof(CS_FW_VER_INFO);
		}
		break;

		case CS_EXTENDED_BOARD_OPTIONS:
		{
			i32Return = sizeof(int64);
		}
		break;

		case CS_SYSTEMINFO_EX:
		{
			if ( sizeof(CSSYSTEMINFOEX) != ((PCSSYSTEMINFOEX)pParamBuffer)->u32Size )
			{
				return CS_INVALID_STRUCT_SIZE;
			}
			i32Return = sizeof(CSSYSTEMINFOEX);
		}
		break;

		case CS_TIMESTAMP_TICKFREQUENCY:
		{
			i32Return = sizeof(int64);
		}
		break;

/*
	case CS_DRV_BOARD_CAPS:
		{
			PBUNNYBOARDCAPS pCapsInfo = (PBUNNYBOARDCAPS) pParamBuffer;
			FillOutBoardCaps(pCapsInfo);
		}
		break;
*/
		case CS_MAX_SEGMENT_PADDING:
		{
			i32Return = sizeof(uInt32);
		}
		break;

		case CS_USE_DACCALIBTABLE:
		{
			i32Return = sizeof(uInt32);
		}
		break;

		case CS_DEFAULTUSE_DACCALIBTABLE:
		{
			i32Return = sizeof(uInt32);
		}
		break;

//		case CS_READ_FLASH:
//		case CS_READ_EEPROM:
//		{
//			if ( sizeof(CS_GENERIC_EEPROM_READWRITE) != ((PCS_GENERIC_EEPROM_READWRITE)pParamBuffer)->u32Size )
//			{
//				return CS_INVALID_STRUCT_SIZE;
//			}
//			i32Return = sizeof(CS_GENERIC_EEPROM_READWRITE);
//		}
//		break;
/*
		case CS_MEM_READ_THRU:
		case CS_TRIGADDR_OFFSET_ADJUST:
		case CS_TRIGLEVEL_ADJUST:
		case CS_TRIGGAIN_ADJUST:
		case CS_FIR_CONFIG:
		case CS_TRIG_OUT:
		case CS_CLOCK_OUT:
   		case CS_READ_NVRAM:
   		case CS_FLASHINFO:
			break;

		case CS_TRIG_OUT:
		{
			tSize = sizeof(CS_TRIG_OUT_STRUCT);
		}
		break;
	
	case CS_CLOCK_OUT:
		{
			if ( 0 != ( BUNNY_OPT_CLKOUT & m_pCardState->CsBoard.u32HardwareOptionsA ))
			{
				PCS_CLOCK_OUT_STRUCT pClockOut = (PCS_CLOCK_OUT_STRUCT) pUserBuffer;

				pClockOut->u16Value = (uInt16) GetClockOut();
				pClockOut->u16Valid = CS_OUT_NONE| CS_CLKOUT_REF|CS_CLKOUT_GBUS;
			}
			else
				i32Status = CS_FUNCTION_NOT_SUPPORTED;			
		}
		break;
*/
		case CS_MULREC_AVG_COUNT:
			i32Return = sizeof(uInt32);
			break;

		case CS_CLOCK_OUT:
			i32Return = sizeof(CS_CLOCK_OUT_STRUCT);
			break;
		case CS_TRIG_OUT:
			i32Return = sizeof(CS_TRIG_OUT_STRUCT);
			break;
		case CS_READ_PCIeLINK_INFO:
			i32Return = sizeof(PCIeLINK_INFO);

		default:
			i32Return = CS_INVALID_PARAMS_ID;
			break;
	}	
	return i32Return;
}




CGageSocket::CGageSocket(void)
{

}


CGageSocket::CGageSocket(string strIPAddress, DWORD dwPort) :  m_strIPAddress(strIPAddress),
																m_dwPort(dwPort)
{
	socket.Connect(m_strIPAddress.c_str(), m_dwPort);
	::InitializeCriticalSection(&m_CriticalSection); 
}


CGageSocket::~CGageSocket(void)
{
	::DeleteCriticalSection(&m_CriticalSection); 
}

int32 CGageSocket::CloseSystemForRm( CSHANDLE csHandle )
{
	int32 i32Result = 0;
	int nErrorCode = 0;

	WORD wHeaderSize = 0;

	DWORD dwTotalSize = sizeof(Preamble) + wHeaderSize + sizeof(csHandle);// don't have to send array, just have to receive it

	Preamble p;
	p.sCommand = CS_CLOSE_SYSTEM_FOR_RM;
	p.dwTotalSize = dwTotalSize;
	p.wHeaderSize = wHeaderSize;

	char* Com = new char[dwTotalSize];
	char* pCom = Com;

	::memcpy(pCom, &p, sizeof(Preamble));
	pCom += sizeof(Preamble);
	memcpy(pCom, &csHandle, sizeof(csHandle));

	::EnterCriticalSection(&m_CriticalSection);

	DWORD dwStart = ::GetTickCount();

	for(;;)
	{
		if ( socket.WriteString(Com, dwTotalSize) )
		{
			char buf[sizeof(i32Result)+1];
			socket.ReadString(buf,sizeof(i32Result));
			memcpy(&i32Result, buf, sizeof(i32Result));
			break;
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);

#ifdef WIN32
			if( WSANOTINITIALISED == nErrorCode )
				break;
#endif
           // Stop if this has taken too long
			if( ::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT )
				break;
		}
	}
	delete[] Com;
	::LeaveCriticalSection(&m_CriticalSection);
	return ( 0 == nErrorCode ) ? i32Result : CS_REMOTE_SOCKET_ERROR;
}

int32 CGageSocket::OpenSystemForRm( CSHANDLE csHandle )
{
	int32 i32Result = 0;
	int nErrorCode = 0;

	WORD wHeaderSize = 0;

	DWORD dwTotalSize = sizeof(Preamble) + wHeaderSize + sizeof(csHandle);// don't have to send array, just have to receive it

	Preamble p;
	p.sCommand = CS_OPEN_SYSTEM_FOR_RM;
	p.dwTotalSize = dwTotalSize;
	p.wHeaderSize = wHeaderSize;

	char* Com = new char[dwTotalSize];
	char* pCom = Com;

	::memcpy(pCom, &p, sizeof(Preamble));
	pCom += sizeof(Preamble);
	memcpy(pCom, &csHandle, sizeof(csHandle));

	::EnterCriticalSection(&m_CriticalSection);	
	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		if ( socket.WriteString(Com, dwTotalSize) )
		{
			char buf[sizeof(i32Result)+1];
			socket.ReadString(buf,sizeof(i32Result));
			memcpy(&i32Result, buf, sizeof(i32Result));
			break;
		}
		else
		{
			nErrorCode = ::WSAGetLastError();
			DisplayErrorMessage(nErrorCode, 1);

#ifdef WIN32
			if( WSANOTINITIALISED == nErrorCode )
				break;
#endif
           // Stop if this has taken too long
			if( ::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT )
				break;
		}
	}
	delete[] Com;
	::LeaveCriticalSection(&m_CriticalSection);
	return ( 0 == nErrorCode ) ? i32Result : CS_REMOTE_SOCKET_ERROR;
}


int32 CGageSocket::GetAcquisitionSystemCount( uInt16 bForceIndependantMode, uInt16* pu16SystemFound )
{
	int32 i32RetVal = CS_FALSE;
	int nErrorCode = 0;

	WORD wHeaderSize = 0;

	DWORD dwTotalSize = sizeof(Preamble) + wHeaderSize + sizeof(bForceIndependantMode);// don't have to send pointer + sizeof(*pu16SystemFound);

	Preamble p;
	p.sCommand = CS_GET_ACQUISITION_SYSTEM_COUNT;
	p.dwTotalSize = dwTotalSize;
	p.wHeaderSize = wHeaderSize;

	char* Com = new char[dwTotalSize];
	char* pCom = Com;
	::memcpy(pCom, &p, sizeof(Preamble));
	pCom += sizeof(Preamble);
	memcpy(pCom, &bForceIndependantMode, sizeof(bForceIndependantMode));

	::EnterCriticalSection(&m_CriticalSection);
	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		if ( socket.WriteString(Com, dwTotalSize) )
		{
			char buf[sizeof(uInt16) + sizeof(i32RetVal) + 1];
			socket.ReadString(buf, sizeof(uInt16) + sizeof(i32RetVal));
			memcpy(&i32RetVal, buf, sizeof(i32RetVal));
			if ( CS_SUCCEEDED(i32RetVal) )
			{
				memcpy(pu16SystemFound, buf + sizeof(i32RetVal), sizeof(uInt16));
			}
			else
			{
				*pu16SystemFound = 0;
			}
			break;
		}
		else
		{ // Need some error code for Write and Read as well ?
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);
			if ( WSAECONNREFUSED == nErrorCode ) // other socket is not there, so return success but no systems found
			{
				nErrorCode = 0;
				i32RetVal = CS_SUCCESS;
			}
			// Stop if this has taken too long
			if(::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT)
				break;
		}
	}
	delete[] Com;
	::LeaveCriticalSection(&m_CriticalSection);
	return ( 0 == nErrorCode ) ? i32RetVal : CS_REMOTE_SOCKET_ERROR;
}

uInt32 CGageSocket::GetAcquisitionSystemHandles( PDRVHANDLE pDrvHdl, uInt32 u32Size )
{
	int32 i32RetVal = CS_FALSE;
	int nErrorCode = 0;

	WORD wHeaderSize = 0;

	DWORD dwTotalSize = sizeof(Preamble) + wHeaderSize + sizeof(u32Size);// don't have to send array, just have to receive it

	Preamble p;
	p.sCommand = CS_GET_ACQUISITION_SYSTEM_HANDLES;
	p.dwTotalSize = dwTotalSize;
	p.wHeaderSize = wHeaderSize;

	char* Com = new char[dwTotalSize];

	char* pCom = Com;
	::memcpy(pCom, &p, sizeof(Preamble));
	pCom += sizeof(Preamble);
	memcpy(pCom, &u32Size, sizeof(u32Size));

	::EnterCriticalSection(&m_CriticalSection);
	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		if ( socket.WriteString(Com, dwTotalSize) )
		{
			const uInt32 cSize = u32Size + sizeof(i32RetVal) + 1;
			char* buf = new char[cSize];
			socket.ReadString(buf, u32Size + sizeof(i32RetVal));
				
			::memcpy(&i32RetVal, buf, sizeof(i32RetVal));
			if ( CS_SUCCEEDED(i32RetVal) )
			{
				::memcpy(pDrvHdl, buf + sizeof(i32RetVal), sizeof(DWORD)); // RG size is for 1 system ONLY
			}
			delete[] buf;
			break;
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);

#ifdef WIN32
			if( WSANOTINITIALISED == nErrorCode )
				break;
#endif
           // Stop if this has taken too long
			if( ::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT )
				break;
		}
	}
	delete[] Com;
	::LeaveCriticalSection(&m_CriticalSection);
	return ( 0 == nErrorCode ) ? i32RetVal : CS_REMOTE_SOCKET_ERROR;
}

int32 CGageSocket::AcquisitionSystemInit( CSHANDLE csHandle, BOOL bResetDefaultSetting )
{
	int32 i32RetVal = CS_FALSE;
	int nErrorCode = 0;

	WORD wHeaderSize = 0;

	DWORD dwTotalSize = sizeof(Preamble) + wHeaderSize + sizeof(csHandle) + sizeof(bResetDefaultSetting);
	Preamble p;
	p.sCommand = CS_ACQUISITION_SYSTEM_INIT;
	p.dwTotalSize = dwTotalSize;
	p.wHeaderSize = wHeaderSize;

	char* Com = new char[dwTotalSize];
	char* pCom = Com;

	::memcpy(pCom, &p, sizeof(Preamble));
	pCom += sizeof(Preamble);

	memcpy(pCom, &csHandle, sizeof(csHandle));
	pCom += sizeof(csHandle);
	memcpy(pCom, &bResetDefaultSetting, sizeof(bResetDefaultSetting));

	::EnterCriticalSection(&m_CriticalSection);	
	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		if ( socket.WriteString(Com, dwTotalSize) )
		{
			const uInt32 cSize = sizeof(i32RetVal) + 1;
			char buf[cSize];
			socket.ReadString(buf, sizeof(i32RetVal));
			memcpy(&i32RetVal, buf, sizeof(i32RetVal));
			break;
		}
		else
		{
			nErrorCode = ::WSAGetLastError();
			DisplayErrorMessage(nErrorCode, 1);

#ifdef WIN32
			if( WSANOTINITIALISED == nErrorCode )
				break;
#endif
           // Stop if this has taken too long
			if( ::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT )
				break;
		}
	}
	delete[] Com;
	::LeaveCriticalSection(&m_CriticalSection);
	return ( 0 == nErrorCode ) ? i32RetVal : CS_REMOTE_SOCKET_ERROR;	
}


int32 CGageSocket::GetAcquisitionSystemInfo( CSHANDLE csHandle, PCSSYSTEMINFO pSysInfo )
{
	int32 i32RetVal = CS_FALSE;
	int nErrorCode = 0;

	if ( sizeof(CSSYSTEMINFO) != pSysInfo->u32Size ) // RG DO WE WANT TO SEND THE SIZE ACROSS
	{
		return CS_INVALID_STRUCT_SIZE;
	}

	DWORD dwHeader[1];
	dwHeader[0] = (DWORD)sizeof(CSSYSTEMINFO);
	WORD wHeaderSize = sizeof(dwHeader);

	DWORD dwTotalSize = sizeof(Preamble) + wHeaderSize + sizeof(csHandle); // pSysInfo only comes back
	Preamble p;
	p.sCommand = CS_GET_ACQUISITION_SYSTEM_INFO;
	p.dwTotalSize = dwTotalSize;
	p.wHeaderSize = wHeaderSize;

	char* Com = new char[dwTotalSize];
	char* pCom = Com;
	::memcpy(pCom, &p, sizeof(Preamble));
	pCom += sizeof(Preamble);
	::memcpy(pCom, dwHeader, wHeaderSize); // RG  - NEW
	pCom += wHeaderSize;
	memcpy(pCom, &csHandle, sizeof(csHandle));

	::EnterCriticalSection(&m_CriticalSection);	
	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		if ( socket.WriteString(Com, dwTotalSize) )
		{
			const uInt32 cSize = sizeof(i32RetVal) + sizeof(CSSYSTEMINFO) + 1;
			char* buf = new char[cSize];
			socket.ReadString(buf, sizeof(i32RetVal) + sizeof(CSSYSTEMINFO));
				
			memcpy(&i32RetVal, buf, sizeof(i32RetVal));
			if ( CS_SUCCEEDED(i32RetVal) )
			{
				memcpy(pSysInfo, buf + sizeof(i32RetVal), sizeof(CSSYSTEMINFO));
			}
			delete[] buf;
			break;
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);

#ifdef WIN32
			if( WSANOTINITIALISED == nErrorCode )
				break;
#endif
           // Stop if this has taken too long
			if( ::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT )
				break;
		}
	}
	delete[] Com;
	::LeaveCriticalSection(&m_CriticalSection);
	return ( 0 == nErrorCode ) ? i32RetVal : CS_REMOTE_SOCKET_ERROR;
}

int32 CGageSocket::GetBoardsInfo( CSHANDLE csHandle, PARRAY_BOARDINFO pBoardInfo )
{
	if ( NULL == pBoardInfo )
	{
		return CS_NULL_POINTER;
	}
	if ( pBoardInfo->u32BoardCount < 1 )
	{
		return CS_INVALID_CARD_COUNT;
	}
	// check that board indices and structure sizes are valid
	for ( uInt32 i = 0; i < pBoardInfo->u32BoardCount; i++ )
	{
		if ( pBoardInfo->aBoardInfo[i].u32Size != sizeof(CSBOARDINFO) )
		{
			return CS_INVALID_STRUCT_SIZE;
		}	
		if ( pBoardInfo->aBoardInfo[i].u32BoardIndex < 0 || pBoardInfo->aBoardInfo[i].u32BoardIndex > pBoardInfo->u32BoardCount )
		{
			return CS_INVALID_CARD;
		}
	}
	UCHAR ucBoardIndex = 0;
	UCHAR ucMask = 0x1;
	for ( uInt32 i = 0; i < pBoardInfo->u32BoardCount; i++ )
	{
		// Set bits for each board, so, for example, board 1 will set bit 1, board 2 will set bit 2, etc.
		ucBoardIndex |= (ucMask << (pBoardInfo->aBoardInfo[i].u32BoardIndex - 1));
	}
	int32 i32Result = 0;
	int nErrorCode = 0;

	DWORD dwHeader[1];
	dwHeader[0] = (DWORD)sizeof(CSBOARDINFO); // so the instrument knows to return this amount of data
	WORD wHeaderSize = sizeof(dwHeader);

	UCHAR ucBoardCount = static_cast<UCHAR>(pBoardInfo->u32BoardCount);

	DWORD dwTotalSize = sizeof(Preamble) + wHeaderSize + sizeof(csHandle) + sizeof(ucBoardCount) + sizeof(ucBoardIndex);

	Preamble p;
	p.sCommand = CS_GET_BOARDS_INFO;
	p.dwTotalSize = dwTotalSize;
	p.wHeaderSize = wHeaderSize;

	char *Com;
	try
	{
		Com = new char[dwTotalSize];
	}
	catch (bad_alloc&)
	{
		return CS_MEMORY_ERROR;
	}
	
	char*  pCom = Com;
	::memcpy(pCom, &p, sizeof(Preamble));
	pCom += sizeof(Preamble);
	::memcpy(pCom, dwHeader, wHeaderSize);
	pCom += wHeaderSize;
	memcpy(pCom, &csHandle, sizeof(csHandle));
	pCom += sizeof(csHandle);
	::memcpy(pCom, &ucBoardCount, sizeof(ucBoardCount));
	pCom += sizeof(ucBoardCount);
	::memcpy(pCom, &ucBoardIndex, sizeof(ucBoardIndex));

	::EnterCriticalSection(&m_CriticalSection);	
	DWORD dwStart = ::GetTickCount();

	for(;;)
	{
		if ( socket.WriteString(Com, dwTotalSize) )
		{
			size_t tBoardInfoSize = ((pBoardInfo->u32BoardCount - 1) * sizeof(CSBOARDINFO)) + sizeof(ARRAY_BOARDINFO);
			size_t tSize = sizeof(i32Result) + tBoardInfoSize;
			char* buf;
				
			try 
			{
				buf = new char[tSize+1]; //RG - ALL ROUTINES MUST HANDLE IF WE CAN'T ALLOCATE MEMORY
			}
			catch ( bad_alloc& )
			{
				delete[] Com;
				::LeaveCriticalSection(&m_CriticalSection);
				return CS_MEMORY_ERROR;
			}

			socket.ReadString(buf, (int)tSize);
			::memcpy(&i32Result, buf, sizeof(i32Result));

//				RG - MAYBE TRY IT THIS WAY i32Result = (int32)buf[0];
//				pBoardInfo->u32BoardCount = (uInt32)buf[1]; // or advance pointer

			DWORD dwOffset = sizeof(i32Result);
			::memcpy(&pBoardInfo->u32BoardCount,  buf + dwOffset, sizeof(pBoardInfo->u32BoardCount));
			dwOffset += sizeof(pBoardInfo->u32BoardCount);
			tBoardInfoSize = pBoardInfo->u32BoardCount * sizeof(CSBOARDINFO); 
				// if the result was not 1 (meaning the structure was padded or truncated) the instrument
				// has adjusted the u32Size field so it's right.
			::memcpy(&pBoardInfo->aBoardInfo, buf + dwOffset, tBoardInfoSize);
			delete[] buf;
			break;
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);

#ifdef WIN32
			if( WSANOTINITIALISED == nErrorCode )
				break;
#endif
           // Stop if this has taken too long
			if( ::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT )
				break;
		}
	}
	delete[] Com;
	::LeaveCriticalSection(&m_CriticalSection);
	return ( 0 == nErrorCode ) ? i32Result : CS_REMOTE_SOCKET_ERROR;
}


int32 CGageSocket::GetAcquisitionSystemCaps( CSHANDLE csHandle, uInt32 u32CapsId, PCSSYSTEMCONFIG pSysCfg, void* pBuffer, uInt32* pu32BufferSize )
{
	int32 i32Result = 0;
	int nErrorCode = 0;

	if ( NULL != pBuffer && NULL == pu32BufferSize )
	{
		return CS_NULL_POINTER;
	}

	if ( NULL != pSysCfg )
	{
		i32Result = IsSystemConfigValid(pSysCfg);
		if ( CS_FAILED(i32Result) )
		{
			return i32Result;
		}
	}

	size_t tSystemConfigSize = GetSysConfigSize(pSysCfg);

	
	// we have several possibilities.  
	// 1) pSysCfg can be NULL, in which case we want all possible caps (i.e caps for both single and dual mode)
	// 2) pBuffer can be NULL in which case the call wants to know the size of buffer to allocate
	// 3) a combination of 1 and 2
	// 4) pSysCfg, pBuffer and pu32BufferSize are NULL in which case we only receive our return value
	// 5) neither are null

	// if a pointer is null we'll store a 0 in it's place.  If it's not null, we'll store a value that will
	// be the offset (from that position) of where the structure is.


	DWORD dwHeader[3];
	dwHeader[0] = (DWORD)tSystemConfigSize;
	dwHeader[1] = ( NULL == pBuffer ) ? 0 : ( NULL == pu32BufferSize ) ? 0 : *pu32BufferSize;
	dwHeader[2] = ( NULL == pu32BufferSize ) ? 0 : sizeof(uInt32);
		
	WORD wHeaderSize = sizeof(dwHeader);


	DWORD dwTotalSize = sizeof(Preamble) + wHeaderSize + sizeof(csHandle) + sizeof(u32CapsId) + dwHeader[0] + dwHeader[1] + dwHeader[2];

	Preamble p;
	p.sCommand = CS_GET_ACQUISITION_SYSTEM_CAPS;
	p.dwTotalSize = dwTotalSize;
	p.wHeaderSize = wHeaderSize;


	// RG - USE AND OFFSET AFTER THE CAPS ID.  IF VALUE IS 0, then pSysCfg is NULL.  If not zero it's an offset from that point
	// to where the structure will be.  If pBuffer is 0 then it's NULL and we just want the size. CHECK THE SIZE BEFORE DOING ANYTHING.
	// ANYWAYS, WE DON'T NEED TO SEND DOWN pBuffer, but we do need to receive it from the instrument, if it's not null.

	// we have several possibilities.  
	// 1) pSysCfg can be NULL, in which case we want all possible caps (i.e caps for both single and dual mode)
	// 2) pBuffer can be NULL in which case the call wants to know the size of buffer to allocate
	// 3) a combination of 1 and 2
	// 4) pSysCfg, pBuffer and pu32BufferSize are NULL in which case we only receive our return value
	// 4) neither are null

	// if a pointer is null we'll store a 0 in it's place.  If it's not null, we'll store a value that will
	// be the offset (from that position) of where the structure is.

	char* Com = new char[dwTotalSize];
	char* pCom = Com;
	::memcpy(pCom, &p, sizeof(p));
	pCom += sizeof(p);
	::memcpy(pCom, dwHeader, wHeaderSize);
	pCom += wHeaderSize;
	memcpy(pCom, &csHandle, sizeof(csHandle));
	pCom += sizeof(csHandle);
	::memcpy(pCom, &u32CapsId, sizeof(u32CapsId));
	pCom += sizeof(u32CapsId);

	if ( dwHeader[0] )
	{
		::memcpy(pCom, pSysCfg->pAcqCfg, sizeof(CSACQUISITIONCONFIG));
		pCom += sizeof(CSACQUISITIONCONFIG);
		size_t tChanSize = ((pSysCfg->pAChannels->u32ChannelCount - 1) * sizeof(CSCHANNELCONFIG)) + sizeof(ARRAY_CHANNELCONFIG);
		::memcpy(pCom, pSysCfg->pAChannels, tChanSize);
		pCom += tChanSize;
		size_t tTrigSize = ((pSysCfg->pATriggers->u32TriggerCount - 1) * sizeof(CSTRIGGERCONFIG)) + sizeof(ARRAY_TRIGGERCONFIG);
		::memcpy(pCom, pSysCfg->pATriggers, tTrigSize);
		pCom += tTrigSize;
	}
	// pBuffer doesn't need to go down but we want to send down it's size
	if ( dwHeader[2] )
	{
		::memcpy(pCom, pu32BufferSize, dwHeader[2]);
	}


	::EnterCriticalSection(&m_CriticalSection);
	DWORD dwStart = ::GetTickCount();

	for(;;)
	{
		if ( socket.WriteString(Com, dwTotalSize) )
		{
			// now ...  we don't need to get bac pSysCfg because the callee won't change it. we just need to get back
			// the return value.  If pu32BufferSize was 0 on the way there (because pBuffer was null), then only that 
			// value will be returned.  If it had a value then only the buffer (which will be pu32BufferSize) is returned.
			const uInt32 u32Size = ( NULL == pBuffer ) ? sizeof(i32Result) + sizeof(uInt32) : sizeof(i32Result) + *pu32BufferSize;
			char* buf = new char[u32Size];

			socket.ReadString(buf, u32Size);
			memcpy(&i32Result, buf, sizeof(i32Result));
			DWORD dwOffset = sizeof(i32Result);
			if ( CS_SUCCEEDED(i32Result) )
			{
				if ( pBuffer )
				{
					::memcpy(pBuffer, buf + dwOffset, *pu32BufferSize);
					dwOffset += *pu32BufferSize;
				}
				else
				{
					if ( pu32BufferSize )
					{
						::memcpy(pu32BufferSize, buf + dwOffset, sizeof(*pu32BufferSize));
					}
				}
			}
			delete[] buf;
			break;
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);

#ifdef WIN32
			if( WSANOTINITIALISED == nErrorCode )
				break;
#endif
           // Stop if this has taken too long
			if( ::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT )
				break;
		}
	}
	delete[] Com;
	::LeaveCriticalSection(&m_CriticalSection);
	return ( 0 == nErrorCode ) ? i32Result : CS_REMOTE_SOCKET_ERROR;
}


int32 CGageSocket::AcquisitionSystemCleanup( CSHANDLE csHandle )
{
	int32 i32Result = 0;
	int nErrorCode = 0;

	WORD wHeaderSize = 0;

	DWORD dwTotalSize = sizeof(Preamble) + wHeaderSize + sizeof(csHandle);// don't have to send array, just have to receive it

	Preamble p;
	p.sCommand = CS_ACQUISITION_SYSTEM_CLEANUP;
	p.dwTotalSize = dwTotalSize;
	p.wHeaderSize = wHeaderSize;

	char* Com = new char[dwTotalSize];
	char* pCom = Com;

	::memcpy(pCom, &p, sizeof(Preamble));
	pCom += sizeof(Preamble);
	memcpy(pCom, &csHandle, sizeof(csHandle));

	::EnterCriticalSection(&m_CriticalSection);
	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		if ( socket.WriteString(Com, dwTotalSize) )
		{
			char buf[sizeof(i32Result)+1];
			socket.ReadString(buf,sizeof(i32Result));
			memcpy(&i32Result, buf, sizeof(i32Result));
			break;
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);

#ifdef WIN32
			if( WSANOTINITIALISED == nErrorCode )
				break;
#endif
           // Stop if this has taken too long
			if( ::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT )
				break;
		}
	}
	delete[] Com;
	::EnterCriticalSection(&m_CriticalSection);
	return ( 0 == nErrorCode ) ? i32Result : CS_REMOTE_SOCKET_ERROR;
}

int32 CGageSocket::GetParams( CSHANDLE csHandle, uInt32 u32ParamID, void* pParamBuffer )
{
	int32 i32Status = CS_SUCCESS;

	if ( NULL == pParamBuffer )
	{
		return CS_NULL_POINTER;
	}

	i32Status = GetParamBufferSize(u32ParamID, pParamBuffer);
	if ( CS_FAILED(i32Status) )
	{
		return i32Status;
	}
	//RG - FOR NOW WE'RE NOT SENDING ANY BUFFER TO THE INSTRUMENT ??
	size_t tBufferSize = size_t(i32Status); //RG - CHECK THAT THIS IS THE SIZE RETURNED OVER THE SOCKET

	DWORD dwHeader = (DWORD)tBufferSize;
	WORD wHeaderSize = sizeof(dwHeader);
	DWORD dwTotalSize = sizeof(Preamble) + wHeaderSize + sizeof(csHandle) + sizeof(u32ParamID);// + tBufferSize; //RG - tBufferSize for now

	dwTotalSize += (DWORD)tBufferSize;

	Preamble p;
	p.sCommand = CS_GET_PARAMS;
	p.dwTotalSize = dwTotalSize;
	p.wHeaderSize = wHeaderSize;


	int32 i32Result = 0;
	int nErrorCode = 0;

	char* Com = new char[dwTotalSize];
	char* pCom = Com;
	::memcpy(pCom, &p, sizeof(p));
	pCom += sizeof(p);
	if ( wHeaderSize )
	{
		::memcpy(pCom, &dwHeader, wHeaderSize);
		pCom += wHeaderSize;
	}
	memcpy(pCom, &csHandle, sizeof(csHandle));
	pCom += sizeof(csHandle);
	::memcpy(pCom, &u32ParamID, sizeof(u32ParamID));
	pCom += sizeof(u32ParamID);

	::memcpy(pCom, pParamBuffer, tBufferSize);

	::EnterCriticalSection(&m_CriticalSection);	
	DWORD dwStart = ::GetTickCount();

	for(;;)
	{
		if ( socket.WriteString(Com, dwTotalSize) )
		{
			// buffer will always be returned			
			size_t tSize = sizeof(i32Result) + tBufferSize;
			char* buf = new char[tSize+1]; //RG - ALL ROUTINES MUST HANDLE IF WE CAN'T ALLOCATE MEMORY
			socket.ReadString(buf, (int)tSize);
			memcpy(&i32Result, buf, sizeof(i32Result));
			//RG - DO A COMPARE ON MEMORY SIZES HERE
			memcpy(pParamBuffer, buf + sizeof(i32Result), tBufferSize);
			delete[] buf;
			break;
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);

#ifdef WIN32
			if( WSANOTINITIALISED == nErrorCode )
				break;
#endif
           // Stop if this has taken too long
			if( ::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT )
				break;
		}
	}
	delete[] Com;
	::LeaveCriticalSection(&m_CriticalSection);
	return ( 0 == nErrorCode ) ? i32Result : CS_REMOTE_SOCKET_ERROR;
}


int32 CGageSocket::SetParams( CSHANDLE csHandle, uInt32 u32ParamID, void* pParamBuffer )
{
	int32 i32Status = CS_SUCCESS;

	if ( NULL == pParamBuffer )
	{
		return CS_NULL_POINTER;
	}

	i32Status = GetParamBufferSize(u32ParamID, pParamBuffer);
	if ( CS_FAILED(i32Status) )
	{
		return i32Status;
	}
	//RG - ON THE SET WE SEND DOWN THE BUFFER, BUT WE DON'T NEED TO RETURN IT FROM THE INSTRUMENT
	size_t tBufferSize = size_t(i32Status); //RG - CHECK THAT THIS IS THE SIZE RETURNED OVER THE SOCKET

	DWORD dwHeader = (DWORD)tBufferSize;
	WORD wHeaderSize = sizeof(dwHeader);
	DWORD dwTotalSize = sizeof(Preamble) + wHeaderSize + sizeof(csHandle) + sizeof(u32ParamID) + (DWORD)tBufferSize; 

	Preamble p;
	p.sCommand = CS_SET_PARAMS;
	p.dwTotalSize = dwTotalSize;
	p.wHeaderSize = wHeaderSize;


	int32 i32Result = 0;
	int nErrorCode = 0;

	char* Com = new char[dwTotalSize];
	char* pCom = Com;
	::memcpy(pCom, &p, sizeof(p));
	pCom += sizeof(p);
	if ( wHeaderSize )
	{
		::memcpy(pCom, &dwHeader, wHeaderSize);
		pCom += wHeaderSize;
	}
	memcpy(pCom, &csHandle, sizeof(csHandle));
	pCom += sizeof(csHandle);
	::memcpy(pCom, &u32ParamID, sizeof(u32ParamID));
	pCom += sizeof(u32ParamID);
	::memcpy(pCom, pParamBuffer, tBufferSize);

	::EnterCriticalSection(&m_CriticalSection);
	DWORD dwStart = ::GetTickCount();

	for(;;)
	{
		if ( socket.WriteString(Com, dwTotalSize) )
		{
			// buffer will never be returned			
			char buf[sizeof(i32Result)+1]; //RG - ALL ROUTINES MUST HANDLE IF WE CAN'T ALLOCATE MEMORY
			socket.ReadString(buf, sizeof(i32Result));
			memcpy(&i32Result, buf, sizeof(i32Result));
			//RG - DO A COMPARE ON MEMORY SIZES HERE
			break;
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);

#ifdef WIN32
			if( WSANOTINITIALISED == nErrorCode )
				break;
#endif
           // Stop if this has taken too long
			if( ::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT )
				break;
		}
	}
	delete[] Com;
	::LeaveCriticalSection(&m_CriticalSection);
	return ( 0 == nErrorCode ) ? i32Result : CS_REMOTE_SOCKET_ERROR;
}

int32 CGageSocket::ValidateParams( CSHANDLE csHandle, uInt32 u32ParamID, uInt32 u32Coerce, void* pParamBuffer )
{
	// We'll try to handle all cases, but in our dlls and CsTest (with the /disp option) CsDrvValidateParams
	// is ONLY sent with PCSSYSYTEMCONFIG

	int32 i32Status = CS_SUCCESS;

	if ( NULL == pParamBuffer )
	{
		return CS_NULL_POINTER;
	}

	if ( CS_SYSTEM == u32ParamID )
	{
		i32Status = IsSystemConfigValid((PCSSYSTEMCONFIG)pParamBuffer);
		if ( CS_FAILED(i32Status) )
		{
			return i32Status;
		}
	}
	i32Status = GetParamBufferSize(u32ParamID, pParamBuffer);
	if ( CS_FAILED(i32Status) )
	{
		return i32Status;
	}

	//RG - FOR NOW WE'RE NOT SENDING ANY BUFFER TO THE INSTRUMENT ??
	DWORD dwBufferSize = size_t(i32Status); //RG - CHECK THAT THIS IS THE SIZE RETURNED OVER THE SOCKET

	DWORD dwHeader = dwBufferSize;
	WORD wHeaderSize = sizeof(dwHeader);
	DWORD dwTotalSize = sizeof(Preamble) + wHeaderSize + sizeof(csHandle) + sizeof(u32ParamID) + sizeof(u32Coerce);// + tBufferSize; //RG - tBufferSize for now

	dwTotalSize += dwBufferSize;

	Preamble p;
	p.sCommand = CS_VALIDATE_PARAMS;
	p.dwTotalSize = dwTotalSize;
	p.wHeaderSize = wHeaderSize;


	int32 i32Result = 0;
	int nErrorCode = 0;

	char* Com = new char[dwTotalSize];
	char* pCom = Com;
	::memcpy(pCom, &p, sizeof(p));
	pCom += sizeof(p);
	if ( wHeaderSize )
	{
		::memcpy(pCom, &dwHeader, wHeaderSize);
		pCom += wHeaderSize;
	}
	memcpy(pCom, &csHandle, sizeof(csHandle));
	pCom += sizeof(csHandle);
	::memcpy(pCom, &u32ParamID, sizeof(u32ParamID));
	pCom += sizeof(u32ParamID);
	::memcpy(pCom, &u32Coerce, sizeof(u32Coerce));
	pCom += sizeof(u32Coerce);

	if ( CS_SYSTEM == u32ParamID )
	{
		SysConfigToBuffer((PCSSYSTEMCONFIG)pParamBuffer, pCom);
	}
	else
	{
		::memcpy(pCom, pParamBuffer, dwBufferSize);
	}

	::EnterCriticalSection(&m_CriticalSection);	
	DWORD dwStart = ::GetTickCount();

	for(;;)
	{
		//RG - FOR NOW, WE'RE NOT SENDING ANYTHING OVER THE SOCKET TO THE INSTRUMENT ???

		if ( socket.WriteString(Com, dwTotalSize) )
		{
			// buffer will always be returned			
			size_t tSize = sizeof(i32Result) + dwBufferSize;
			char* buf = new char[tSize+1]; //RG - ALL ROUTINES MUST HANDLE IF WE CAN'T ALLOCATE MEMORY
			socket.ReadString(buf, (int)tSize);
			memcpy(&i32Result, buf, sizeof(i32Result));
				
			if ( CS_SUCCEEDED(i32Result) ) // only do the copy if the call succeeded
			{
				//RG - DO A COMPARE ON MEMORY SIZES HERE
				if ( CS_SYSTEM == u32ParamID )
				{
					BufferToSysConfig(buf + sizeof(i32Result), (PCSSYSTEMCONFIG)pParamBuffer);
				}
				else
				{		
					memcpy(pParamBuffer, buf + sizeof(i32Result), dwBufferSize);
				}
			}
			delete[] buf;
			break;
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);

#ifdef WIN32
			if( WSANOTINITIALISED == nErrorCode )
				break;
#endif
           // Stop if this has taken too long
			if( ::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT )
				break;
		}
	}
	delete[] Com;
	::LeaveCriticalSection(&m_CriticalSection);
	return ( 0 == nErrorCode ) ? i32Result : CS_REMOTE_SOCKET_ERROR;
}

int32 CGageSocket::Do( CSHANDLE csHandle, uInt32 u32ActionID, void* pActionBuffer )
{
	size_t tBufferSize = 0;

	if ( NULL != pActionBuffer )
	{
		if ( !IsSystemConfigValid((PCSSYSTEMCONFIG)pActionBuffer) )
		{
			return CS_INVALID_STRUCT_SIZE; // RG - WRONG ERROR
		}
		tBufferSize = GetSysConfigSize((PCSSYSTEMCONFIG) pActionBuffer);
	}
	
	//RG - WE SEND DOWN THE BUFFER, BUT WE DON'T NEED TO RETURN IT FROM THE INSTRUMENT


	DWORD dwHeader = (DWORD)tBufferSize;
	WORD wHeaderSize = sizeof(dwHeader);
	DWORD dwTotalSize = sizeof(Preamble) + wHeaderSize + sizeof(csHandle) + sizeof(u32ActionID) + (DWORD)tBufferSize; 

	Preamble p;
	p.sCommand = CS_ACTION_DO;
	p.dwTotalSize = dwTotalSize;
	p.wHeaderSize = wHeaderSize;


	int32 i32Result = 0;
	int nErrorCode = 0;

	char* Com = new char[dwTotalSize];
	char* pCom = Com;
	::memcpy(pCom, &p, sizeof(p));
	pCom += sizeof(p);
	if ( wHeaderSize )
	{
		::memcpy(pCom, &dwHeader, wHeaderSize);
		pCom += wHeaderSize;
	}
	memcpy(pCom, &csHandle, sizeof(csHandle));
	pCom += sizeof(csHandle);
	::memcpy(pCom, &u32ActionID, sizeof(u32ActionID));
	pCom += sizeof(u32ActionID);
	if ( tBufferSize )
	{
		PCSSYSTEMCONFIG pSysCfg = (PCSSYSTEMCONFIG)pActionBuffer;
		::memcpy(pCom, pSysCfg->pAcqCfg, sizeof(CSACQUISITIONCONFIG));
		pCom += sizeof(CSACQUISITIONCONFIG);
		size_t tChanSize = ((pSysCfg->pAChannels->u32ChannelCount - 1) * sizeof(CSCHANNELCONFIG)) + sizeof(ARRAY_CHANNELCONFIG);
		::memcpy(pCom, pSysCfg->pAChannels, tChanSize);
		pCom += tChanSize;
		size_t tTrigSize = ((pSysCfg->pATriggers->u32TriggerCount - 1) * sizeof(CSTRIGGERCONFIG)) + sizeof(ARRAY_TRIGGERCONFIG);
		::memcpy(pCom, pSysCfg->pATriggers, tTrigSize);
	}

	::EnterCriticalSection(&m_CriticalSection);
	DWORD dwStart = ::GetTickCount();

	for(;;)
	{
		if ( socket.WriteString(Com, dwTotalSize) )
		{
			// buffer will never be returned			
			char buf[sizeof(i32Result)+1]; //RG - ALL ROUTINES MUST HANDLE IF WE CAN'T ALLOCATE MEMORY
			socket.ReadString(buf, sizeof(i32Result));
			memcpy(&i32Result, buf, sizeof(i32Result));
			break;
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);

#ifdef WIN32
			if( WSANOTINITIALISED == nErrorCode )
				break;
#endif
           // Stop if this has taken too long
			if( ::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT )
				break;
		}
	}
	delete[] Com;
	::LeaveCriticalSection(&m_CriticalSection);
	return ( 0 == nErrorCode ) ? i32Result : CS_REMOTE_SOCKET_ERROR;
}


int32 CGageSocket::GetAcquisitionStatus( CSHANDLE csHandle, uInt32* pu32Status )
{
	int32 i32Result = 0;
	int nErrorCode = 0;

	if ( NULL == pu32Status )
	{
		return CS_NULL_POINTER;
	}
	WORD wHeaderSize = 0;

	DWORD dwTotalSize = sizeof(Preamble) + wHeaderSize + sizeof(csHandle);

	Preamble p;
	p.sCommand = CS_GET_ACQUISITION_STATUS;
	p.dwTotalSize = dwTotalSize;
	p.wHeaderSize = wHeaderSize;

	char* Com = new char[dwTotalSize];
	char* pCom = Com;

	::memcpy(pCom, &p, sizeof(Preamble));
	pCom += sizeof(Preamble);
	::memcpy(pCom, &csHandle, sizeof(csHandle));

	::EnterCriticalSection(&m_CriticalSection);	
	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		if ( socket.WriteString(Com, dwTotalSize) )
		{
			const size_t tSize = sizeof(i32Result) + sizeof(uInt32);
			char buf[tSize+1];
			socket.ReadString(buf, tSize);
			::memcpy(&i32Result, buf, sizeof(i32Result));
			::memcpy(pu32Status, buf + sizeof(i32Result), sizeof(*pu32Status));
			break;
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);

#ifdef WIN32
			if( WSANOTINITIALISED == nErrorCode )
				break;
#endif
           // Stop if this has taken too long
			if( ::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT )
				break;
		}
	}
	delete[] Com;
	::LeaveCriticalSection(&m_CriticalSection);
	return ( 0 == nErrorCode ) ? i32Result : CS_REMOTE_SOCKET_ERROR;
}


int32 CGageSocket::TransferData( CSHANDLE csHandle, IN_PARAMS_TRANSFERDATA inParams, POUT_PARAMS_TRANSFERDATA pOutParams, uInt32* u32SampleLength /* == NULL */ )
{
	int32 i32Result = 0;
	int nErrorCode = 0;

	//RG - DO VARIOUS CHECKS ON inParams, especially pDataBuffer, although I think it can be NULL
	//RG - BE SURE TO CHECK IF IT IS NULL

	WORD wHeaderSize = 0;

	DWORD dwTotalSize = sizeof(Preamble) + wHeaderSize + sizeof(csHandle) + sizeof(IN_PARAMS_TRANSFERDATA);

	Preamble p;
	p.sCommand = CS_TRANSFER_DATA;
	p.dwTotalSize = dwTotalSize;
	p.wHeaderSize = wHeaderSize;

	char* Com = new char[dwTotalSize];
	char* pCom = Com;
			
	::memcpy(pCom, &p, sizeof(Preamble));
	pCom += sizeof(Preamble);
	::memcpy(pCom, &csHandle, sizeof(csHandle));
	pCom += sizeof(csHandle);
	::memcpy(pCom, &inParams, sizeof(IN_PARAMS_TRANSFERDATA));
	
	::EnterCriticalSection(&m_CriticalSection);
	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		if ( socket.WriteString(Com, dwTotalSize) )
		{
			const uInt32 cSize = sizeof(i32Result) + sizeof(OUT_PARAMS_TRANSFERDATA);
			char buf[cSize+1];
			uInt32 u32Offset = 0;
			uInt32 u32SampleSize;
			socket.ReadString(buf, cSize); 
				
			// send acknowledgement. For now we need to send and receive acknowledgment or the 2 writesockets in a 
			// row cause small transfers to be very slow. A possible alternative solution would be to combine
			// OutParams and the data, but we would need to double buffer the data

			socket.WriteString(Com, 4);

			::memcpy(&i32Result, buf, sizeof(i32Result));
			u32Offset += sizeof(i32Result);
			::memcpy(&u32SampleSize, buf + u32Offset, sizeof(u32SampleSize));
			u32Offset += sizeof(u32SampleSize);
			//RG - VALIDATE SIZE OF pOutParams and whether it's NULL or not
			::memcpy(pOutParams, buf + u32Offset, sizeof(OUT_PARAMS_TRANSFERDATA));
			//RG - DO WE HAVE TO DOUBLE BUFFER LIKE THIS
			//RG HAVE TO BE ABLE TO HANDLE: IF > INT HAVE TO SEND IN CHUNKS

			if ( NULL != u32SampleLength )
			{
				*u32SampleLength = u32SampleSize; // pass the sample size back to caller
			}
			int len = 0;
			if ( CS_SUCCEEDED(i32Result) && inParams.pDataBuffer )
			{
				len = socket.ReadString((char*)inParams.pDataBuffer, (int)(pOutParams->i64ActualLength) * u32SampleSize);// RG !!!!!need the size of a sample

				if ( len != (pOutParams->i64ActualLength * u32SampleSize) )
				{
					// error - handle
				}
			}
			break;
		}
		else
		{
			nErrorCode = ::WSAGetLastError();
			DisplayErrorMessage(nErrorCode, 1);

#ifdef WIN32
			if( WSANOTINITIALISED == nErrorCode )
				break;
#endif
           // Stop if this has taken too long
			if( ::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT )
				break;
		}
	}
	delete[] Com;
	::LeaveCriticalSection(&m_CriticalSection);
	return ( 0 == nErrorCode ) ? i32Result : CS_REMOTE_SOCKET_ERROR;
}

int32 CGageSocket::SetSystemInUse( CSHANDLE csHandle, BOOL bSet )
{
	int32 i32RetVal = CS_FALSE;
	int nErrorCode = 0;

	WORD wHeaderSize = 0;

	DWORD dwTotalSize = sizeof(Preamble) + wHeaderSize + sizeof(csHandle) + sizeof(bSet);
	Preamble p;
	p.sCommand = CS_SET_SYSTEM_IN_USE;
	p.dwTotalSize = dwTotalSize;
	p.wHeaderSize = wHeaderSize;

	char* Com = new char[dwTotalSize];
	char* pCom = Com;

	::memcpy(pCom, &p, sizeof(Preamble));
	pCom += sizeof(Preamble);

	memcpy(pCom, &csHandle, sizeof(csHandle));
	pCom += sizeof(csHandle);
	memcpy(pCom, &bSet, sizeof(bSet));

	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		if ( socket.WriteString(Com, dwTotalSize) )
		{
			const uInt32 cSize = sizeof(i32RetVal);
			char buf[cSize+1];
			socket.ReadString(buf, sizeof(i32RetVal));
			memcpy(&i32RetVal, buf, sizeof(i32RetVal));
			break;
		}
		else
		{
			nErrorCode = ::WSAGetLastError();
			DisplayErrorMessage(nErrorCode, 1);

#ifdef WIN32
			if( WSANOTINITIALISED == nErrorCode )
				break;
#endif
           // Stop if this has taken too long
			if( ::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT )
				break;
		}
	}
	delete[] Com;
	return ( 0 == nErrorCode ) ? i32RetVal : CS_REMOTE_SOCKET_ERROR;	
}

int32 CGageSocket::IsSystemInUse( CSHANDLE csHandle, BOOL* bSet )
{
	int32 i32RetVal = CS_FALSE;
	int nErrorCode = 0;

	WORD wHeaderSize = 0;

	DWORD dwTotalSize = sizeof(Preamble) + wHeaderSize + sizeof(csHandle);// + sizeof(bSet);
	Preamble p;
	p.sCommand = CS_IS_SYSTEM_IN_USE;
	p.dwTotalSize = dwTotalSize;
	p.wHeaderSize = wHeaderSize;

	char* Com = new char[dwTotalSize];
	char* pCom = Com;

	::memcpy(pCom, &p, sizeof(Preamble));
	pCom += sizeof(Preamble);

	memcpy(pCom, &csHandle, sizeof(csHandle));

	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		if ( socket.WriteString(Com, dwTotalSize) )
		{
			const uInt32 cSize = sizeof(i32RetVal) + sizeof(BOOL);
			char buf[cSize+1];
			socket.ReadString(buf, cSize);
			memcpy(&i32RetVal, buf, sizeof(i32RetVal));

			if ( CS_FAILED(i32RetVal) )
			{
				*bSet = FALSE;
			}
			else
			{
				DWORD dwOffset = sizeof(i32RetVal);
				memcpy(bSet, buf + dwOffset, sizeof(BOOL));
			}
			break;
		}
		else
		{
			nErrorCode = ::WSAGetLastError();
			DisplayErrorMessage(nErrorCode, 1);

#ifdef WIN32
			if( WSANOTINITIALISED == nErrorCode )
				break;
#endif
           // Stop if this has taken too long
			if( ::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT )
				break;
		}
	}
	delete[] Com;
	return ( 0 == nErrorCode ) ? i32RetVal : CS_REMOTE_SOCKET_ERROR;	
}
