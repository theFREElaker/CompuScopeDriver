#include "rm.h"

#if defined (_WIN32)

#include <windows.h>

#else

#include <stdio.h>
#include <string.h>

// using namespace::std

#endif

#include "Worker.h"
#include "RawSocketServerWorker.h"
#include "CsResourceManager.h"
#include "misc.h"
#include "CsErrors.h"


/////////////////////////////////////////////////////////////////////////////
CWorker::CWorker(): CWizRawSocketListener (_CsRm_WinSock_Port), m_nCurrThreads (0), m_nMaxThreads (0), m_nRequests(0)
{
	m_pRM = new CsResourceManager();
}
CWorker::~CWorker()
{
	delete m_pRM;
}
#if defined (_WIN32)
BOOL CWorker::TreatData   (HANDLE hShutDownEvent, HANDLE hDataTakenEvent)
#else
BOOL CWorker::TreatData   (pthread_cond_t* hShutDownEvent, pthread_cond_t& hDataTakenEvent)
#endif
{
	{
		CWizCritSect wcs (cs);
		if(++m_nCurrThreads > m_nMaxThreads)
			m_nMaxThreads = m_nCurrThreads;
	}

	if (!CWizRawSocketListener::TreatData (hShutDownEvent, hDataTakenEvent))
		return FALSE;

	{
		CWizCritSect wcs (cs);
		--m_nCurrThreads;
	}
	return TRUE;
}


BOOL CWorker::ReadWrite (CWizReadWriteSocket& socket)
{
	TCHAR buff[SOCKET_BUFFER_LENGTH];
	TCHAR Comm[FUNCTION_NAME_BUFFER_SIZE];
	int32 i32RetVal = 0;

	int  Len = socket.ReadString(buff, SOCKET_BUFFER_LENGTH);
	if(Len == 0)
		return FALSE;

	memcpy(Comm, buff, FUNCTION_NAME_BUFFER_SIZE);

	if(Len < FUNCTION_NAME_BUFFER_SIZE)
	{
		::ZeroMemory(buff, SOCKET_BUFFER_LENGTH);
		int32 i32RetVal = CS_BUFFER_TOO_SMALL;
		memcpy(buff, &i32RetVal, sizeof(i32RetVal));
		return socket.WriteString(buff, sizeof(i32RetVal));
	}
	if (strcmp(Comm, "Init") == 0)
	{
		if (Len >= FUNCTION_NAME_BUFFER_SIZE)
		{
			i32RetVal = m_pRM->CsRmInitialize();

			size_t tSize = sizeof(i32RetVal);
			if (SOCKET_BUFFER_LENGTH < tSize)
				i32RetVal = CS_SOCKET_ERROR;

			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
		else
		{
			::ZeroMemory(buff, SOCKET_BUFFER_LENGTH);
			i32RetVal = CS_BUFFER_TOO_SMALL;
			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
	}
	else if (strcmp(Comm, "ReSet") == 0)
	{
		if (Len >= 12)
		{
			int16 i16Action;
			memcpy(&i16Action, buff + FUNCTION_NAME_BUFFER_SIZE, sizeof(i16Action));
			i32RetVal = m_pRM->CsRmInitialize(i16Action);

			size_t tSize = sizeof(i32RetVal);
			if (SOCKET_BUFFER_LENGTH < tSize)
				i32RetVal = CS_SOCKET_ERROR;

			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
		else
		{
			::ZeroMemory(buff, SOCKET_BUFFER_LENGTH);
			i32RetVal = CS_BUFFER_TOO_SMALL;
			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
	}
	else if(strcmp(Comm, "GetAvail") == 0)
	{
		if (Len >= FUNCTION_NAME_BUFFER_SIZE)
		{
			i32RetVal = m_pRM->CsRmGetAvailableSystems();

			size_t tSize = sizeof(i32RetVal);
			if (SOCKET_BUFFER_LENGTH < tSize)
				i32RetVal = CS_SOCKET_ERROR;

			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
		else
		{
			::ZeroMemory(buff, SOCKET_BUFFER_LENGTH);
			i32RetVal = CS_BUFFER_TOO_SMALL;
			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
	}
	else if(strcmp(Comm, "GetCount") == 0)
	{
		if (Len >= FUNCTION_NAME_BUFFER_SIZE)
		{
			i32RetVal = m_pRM->CsRmGetNumberOfSystems();

			size_t tSize = sizeof(i32RetVal);
			if (SOCKET_BUFFER_LENGTH < tSize)
				i32RetVal = CS_SOCKET_ERROR;

			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
		else
		{
			::ZeroMemory(buff, SOCKET_BUFFER_LENGTH);
			i32RetVal = CS_BUFFER_TOO_SMALL;
			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
	}
	else if(strcmp(Comm, "GetState") == 0)
	{
		if (Len >= 12)
		{
			int16	i16Index;
			CSSYSTEMSTATE csSystemState;
			csSystemState.u32Size = sizeof(CSSYSTEMSTATE);
			memcpy(&i16Index, buff + FUNCTION_NAME_BUFFER_SIZE, sizeof(i16Index));

			i32RetVal = m_pRM->CsRmGetSystemStateByIndex(i16Index, &csSystemState);
			size_t tSize = sizeof(i32RetVal) + sizeof(CSSYSTEMSTATE);
			if (SOCKET_BUFFER_LENGTH < tSize)
			{
				i32RetVal = CS_SOCKET_ERROR;
				memcpy(buff, &i32RetVal, sizeof(i32RetVal));
				return socket.WriteString(buff, sizeof(i32RetVal));
			}
			else
			{
				memcpy(buff, &i32RetVal, sizeof(i32RetVal));
				memcpy(buff+sizeof(i32RetVal), &csSystemState, sizeof(CSSYSTEMSTATE));
				return socket.WriteString(buff, sizeof(i32RetVal) + sizeof(CSSYSTEMSTATE));
			}
		}
		else
		{
			::ZeroMemory(buff, SOCKET_BUFFER_LENGTH);
			i32RetVal = CS_BUFFER_TOO_SMALL;
			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
	}
	else if(strcmp(Comm, "GetHState") == 0)
	{
		if (Len >= 14)
		{
			RMHANDLE hSystemHandle;
			CSSYSTEMSTATE csSystemState;

			memcpy(&hSystemHandle, buff + FUNCTION_NAME_BUFFER_SIZE, sizeof(hSystemHandle));

			csSystemState.u32Size = sizeof(CSSYSTEMSTATE);
			i32RetVal = m_pRM->CsRmGetSystemStateByHandle(hSystemHandle, &csSystemState);
			size_t tSize = sizeof(i32RetVal) + sizeof(CSSYSTEMSTATE);
			if (SOCKET_BUFFER_LENGTH < tSize)
			{
				i32RetVal = CS_SOCKET_ERROR;
				memcpy(buff, &i32RetVal, sizeof(i32RetVal));
				return socket.WriteString(buff, sizeof(i32RetVal));
			}
			else
			{
				memcpy(buff, &i32RetVal, sizeof(i32RetVal));
				memcpy(buff+sizeof(i32RetVal), &csSystemState, sizeof(CSSYSTEMSTATE));
				return socket.WriteString(buff, sizeof(i32RetVal) + sizeof(CSSYSTEMSTATE));
			}
		}
		else
		{
			::ZeroMemory(buff, SOCKET_BUFFER_LENGTH);
			i32RetVal = CS_BUFFER_TOO_SMALL;
			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
	}
	else if (strcmp(Comm, "GetSystem") == 0)
	{
		if (Len >= 26)
		{
			uInt32 i32BoardType, u32ChannelCount, u32SampleBits, u32ProcessID;
			int16  i16Index;
			RMHANDLE rmHandle;
			int nBufferPosIncrement = FUNCTION_NAME_BUFFER_SIZE;
			memcpy(&i32BoardType, buff + nBufferPosIncrement, sizeof(i32BoardType));
			nBufferPosIncrement += sizeof(i32BoardType);
			memcpy(&u32ChannelCount, buff + nBufferPosIncrement, sizeof(u32ChannelCount));
			nBufferPosIncrement += sizeof(u32ChannelCount);
			memcpy(&u32SampleBits, buff + nBufferPosIncrement, sizeof(u32SampleBits));
			nBufferPosIncrement += sizeof(u32SampleBits);
			memcpy(&u32ProcessID, buff + nBufferPosIncrement, sizeof(u32ProcessID));
			nBufferPosIncrement += sizeof(u32ProcessID);
			memcpy(&i16Index, buff + nBufferPosIncrement, sizeof(i16Index));
			i32RetVal = m_pRM->CsRmGetSystem(i32BoardType, u32ChannelCount, u32SampleBits, &rmHandle, u32ProcessID, i16Index);

			size_t tSize = sizeof(i32RetVal) + sizeof(RMHANDLE);
			if (SOCKET_BUFFER_LENGTH < tSize)
			{
				i32RetVal = CS_SOCKET_ERROR;
				memcpy(buff, &i32RetVal, sizeof(i32RetVal));
				return socket.WriteString(buff, sizeof(i32RetVal));
			}
			else
			{
				memcpy(buff, &i32RetVal, sizeof(i32RetVal));
				memcpy(buff+sizeof(i32RetVal), &rmHandle, sizeof(RMHANDLE));
                return socket.WriteString(buff, sizeof(i32RetVal) + sizeof(RMHANDLE));
			}
		}
		else
		{
			::ZeroMemory(buff, SOCKET_BUFFER_LENGTH);
			i32RetVal = CS_BUFFER_TOO_SMALL;
			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
	}
	else if (strcmp(Comm, "Lock") == 0)
	{
		if (Len >= 18)
		{
			RMHANDLE hSystemHandle;
			uInt32 u32ProcessID;

			memcpy(&hSystemHandle, buff + FUNCTION_NAME_BUFFER_SIZE, sizeof(hSystemHandle));
			memcpy(&u32ProcessID, buff + FUNCTION_NAME_BUFFER_SIZE + sizeof(hSystemHandle), sizeof(u32ProcessID));
			i32RetVal = m_pRM->CsRmLockSystem(hSystemHandle, u32ProcessID);

			size_t tSize = sizeof(i32RetVal);
			if (SOCKET_BUFFER_LENGTH < tSize)
				i32RetVal = CS_SOCKET_ERROR;

			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
		else
		{
			::ZeroMemory(buff, SOCKET_BUFFER_LENGTH);
			i32RetVal = CS_BUFFER_TOO_SMALL;
			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
	}
	else if (strcmp(Comm, "Free") == 0)
	{
		if (Len >= 14)
		{
			RMHANDLE hSystemHandle;

			memcpy(&hSystemHandle, buff + FUNCTION_NAME_BUFFER_SIZE, sizeof(hSystemHandle));
			i32RetVal = m_pRM->CsRmFreeSystem(hSystemHandle);

			size_t tSize = sizeof(i32RetVal);
			if (SOCKET_BUFFER_LENGTH < tSize)
				i32RetVal = CS_SOCKET_ERROR;

			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
		else
		{
			::ZeroMemory(buff, SOCKET_BUFFER_LENGTH);
			i32RetVal = CS_BUFFER_TOO_SMALL;
			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
	}
	else if (strcmp(Comm, "Valid") == 0)
	{
		if (Len >= 18)
		{
			RMHANDLE hSystemHandle;
			uInt32 u32ProcessID;

			memcpy(&hSystemHandle, buff + FUNCTION_NAME_BUFFER_SIZE, sizeof(hSystemHandle));
			memcpy(&u32ProcessID, buff + FUNCTION_NAME_BUFFER_SIZE + sizeof(hSystemHandle), sizeof(u32ProcessID));

			i32RetVal = m_pRM->CsRmIsSystemHandleValid(hSystemHandle, u32ProcessID);

			size_t tSize = sizeof(i32RetVal);
			if (SOCKET_BUFFER_LENGTH < tSize)
				i32RetVal = CS_SOCKET_ERROR;

			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
		else
		{
			::ZeroMemory(buff, SOCKET_BUFFER_LENGTH);
			i32RetVal = CS_BUFFER_TOO_SMALL;
			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
	}
	else if (strcmp(Comm, "GetInfo") == 0)
	{
		if (Len >= 14)
		{
			RMHANDLE hSystemHandle;
			CSSYSTEMINFO csSystemInfo;

			memcpy(&hSystemHandle, buff + FUNCTION_NAME_BUFFER_SIZE, sizeof(hSystemHandle));

			i32RetVal = m_pRM->CsRmGetSystemInfo(hSystemHandle, &csSystemInfo);
			size_t tSize = sizeof(i32RetVal) + sizeof(CSSYSTEMINFO);
			if (SOCKET_BUFFER_LENGTH < tSize)
			{
				i32RetVal = CS_SOCKET_ERROR;
				memcpy(buff, &i32RetVal, sizeof(i32RetVal));
				return socket.WriteString(buff, sizeof(i32RetVal));
			}
			else
			{
				memcpy(buff, &i32RetVal, sizeof(i32RetVal));
				memcpy(buff+sizeof(i32RetVal), &csSystemInfo, sizeof(CSSYSTEMINFO));
				return socket.WriteString(buff, sizeof(i32RetVal) + sizeof(CSSYSTEMINFO));
			}
		}
		else
		{
			::ZeroMemory(buff, SOCKET_BUFFER_LENGTH);
			i32RetVal = CS_BUFFER_TOO_SMALL;
			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
	}
	else if (strcmp(Comm, "Refresh") == 0)
	{
		if (Len >= FUNCTION_NAME_BUFFER_SIZE)
		{
			i32RetVal = m_pRM->CsRmRefreshAllSystemHandles();

			size_t tSize = sizeof(i32RetVal);
			if (SOCKET_BUFFER_LENGTH < tSize)
				i32RetVal = CS_SOCKET_ERROR;

			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
		else
		{
			::ZeroMemory(buff, SOCKET_BUFFER_LENGTH);
			i32RetVal = CS_BUFFER_TOO_SMALL;
			memcpy(buff, &i32RetVal, sizeof(i32RetVal));
			return socket.WriteString(buff, sizeof(i32RetVal));
		}
	}
	else
		return FALSE;
}
