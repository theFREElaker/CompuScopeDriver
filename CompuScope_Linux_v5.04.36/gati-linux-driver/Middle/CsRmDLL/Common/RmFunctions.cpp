#if defined (_WIN32)
#include <crtdbg.h>
#else
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>
#include <unistd.h>
#endif

#include "CsDefines.h"
#include "CsErrors.h"
#include "RawSocket.h"

#ifdef __linux__
#include <CsLinuxPort.h>
#include "misc.h"
#else
#include "misc.h"
#include "debug.h"
#endif

#include "CsStruct.h"
#include "RmFunctions.h"

#if defined (_WIN32)
static int g_WSAStarted = 0;
#endif

char strIPAddress[20] = "127.0.0.1";
#define SOCKET_WAIT_TIME_LIMIT 3000 // 3 Seconds

int32 DllExport CsRmInitialize(void)
{
	int32 i32RetVal = CS_FALSE;
	int nErrorCode;

#if defined (_WIN32)  // RICKY - CHECK TO SEE IF I CAN GET SOCKET VERSION IN LINUX

	WSADATA wsaData;

	TRACE(SOCKET_TRACE_LEVEL, "Rm:: g_WSAStarted = %d\n", g_WSAStarted);

	if (!g_WSAStarted)
	{
		g_WSAStarted = 1;
		WORD wVersionRequested = MAKEWORD( 2, 2 );

		nErrorCode = WSAStartup( wVersionRequested, &wsaData );
		if (nErrorCode != 0)
		{
			/* Tell the user that we could not find a usable */
			/* WinSock DLL.         */

			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode);
			return CS_SOCKET_NOT_FOUND;
		}
	}
#endif // _WIN32
	nErrorCode = 0;
	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		CWizReadWriteSocket socket;

		if(socket.Connect(strIPAddress, _CsRm_WinSock_Port))
		{
			char Com[FUNCTION_NAME_BUFFER_SIZE];
			_tcscpy_s(Com, FUNCTION_NAME_BUFFER_SIZE, "Init");
			if (socket.WriteString(Com, FUNCTION_NAME_BUFFER_SIZE))
			{
				char buf[sizeof(i32RetVal) + 1];
				socket.ReadString(buf, sizeof(i32RetVal));
				memcpy(&i32RetVal, buf, sizeof(i32RetVal));
				break;
			}
		}
		else
		{ // Need some error code for Write and Read as well ?

			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);
			// Stop if this has taken too long
			if(::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT)
				break;
		}
	}
	if (0 == nErrorCode)
		return i32RetVal;
	else
		return CS_UNABLE_CREATE_RM;
}

int32 DllExport CsRmReInitialize(int16 i16Action)
{
	int nErrorCode;
//RG - POSSIBLY MAKE A FUNCTION (StartSocket (?)) to encapsulate this
// or make dummy functions for the WSA stuff
#if defined (_WIN32)  // RICKY - CHECK TO SEE IF I CAN GET SOCKET VERSION IN LINUX

	WSADATA wsaData;

	TRACE(SOCKET_TRACE_LEVEL, "Rm:: g_WSAStarted = %d\n", g_WSAStarted);

	if (!g_WSAStarted)
	{
		g_WSAStarted = 1;
		WORD wVersionRequested = MAKEWORD( 2, 2 );

		nErrorCode = WSAStartup( wVersionRequested, &wsaData );
		if (nErrorCode != 0)
		{
			/* Tell the user that we could not find a usable */
			/* WinSock DLL.         */

			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode);
			return CS_SOCKET_NOT_FOUND;
		}
	}
#endif // _WIN32
	int32 i32RetVal = 0;
	nErrorCode = 0;
	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		CWizReadWriteSocket socket;

		if(socket.Connect(strIPAddress, _CsRm_WinSock_Port))
		{
			const uInt32 cSize = FUNCTION_NAME_BUFFER_SIZE + sizeof(i16Action);
			char Com[cSize], *pCom;
			pCom = Com;
			_tcscpy_s(Com, FUNCTION_NAME_BUFFER_SIZE, "ReSet");
			pCom += FUNCTION_NAME_BUFFER_SIZE;
			memcpy(pCom, &i16Action, sizeof(i16Action));
			if (socket.WriteString(Com, cSize))
			{
				char buf[sizeof(i32RetVal)+1];
				socket.ReadString(buf,sizeof(i32RetVal));
				memcpy(&i32RetVal, buf, sizeof(i32RetVal));
				break;
			}
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode,1);
			// Stop if this has taken too long
			if(::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT)
				break;
		}
	}
	if (0 == nErrorCode)
		return i32RetVal;
	else
		return CS_SOCKET_ERROR;
}

int32 DllExport CsRmGetAvailableSystems(void)
{
	int32 i32NumberOfSystems = 0;

	int nErrorCode = 0;
	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		CWizReadWriteSocket socket;

		if(socket.Connect(strIPAddress, _CsRm_WinSock_Port))
		{
			char Com[FUNCTION_NAME_BUFFER_SIZE];
			_tcscpy_s(Com, FUNCTION_NAME_BUFFER_SIZE, "GetAvail");
			if (socket.WriteString(Com, FUNCTION_NAME_BUFFER_SIZE))
			{
				char buf[sizeof(i32NumberOfSystems)+1];
				socket.ReadString(buf,sizeof(i32NumberOfSystems));
				memcpy(&i32NumberOfSystems, buf, sizeof(i32NumberOfSystems));
				break;
			}
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode,1);
			// Stop if this has taken too long
			if(::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT)
				break;
		}
	}
	if (0 == nErrorCode)
		return i32NumberOfSystems;
	else
		return CS_SOCKET_ERROR;
}


int32 DllExport CsRmGetNumberOfSystems(void)
{
	int32 i16NumberOfSystems = 0;

	int nErrorCode = 0;
	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		CWizReadWriteSocket socket;

		if(socket.Connect(strIPAddress, _CsRm_WinSock_Port))
		{
			char Com[FUNCTION_NAME_BUFFER_SIZE];
			_tcscpy_s(Com, FUNCTION_NAME_BUFFER_SIZE, "GetCount");
			if (socket.WriteString(Com, FUNCTION_NAME_BUFFER_SIZE))
			{
				char buf[sizeof(i16NumberOfSystems)+1];
				socket.ReadString(buf,sizeof(i16NumberOfSystems));
				memcpy(&i16NumberOfSystems, buf, sizeof(i16NumberOfSystems));
				break;
			}
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);
			// Stop if this has taken too long
			if(::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT)
				break;
		}
	}
	if (0 == nErrorCode)
		return i16NumberOfSystems;
	else
		return CS_SOCKET_ERROR;
}

int32 DllExport CsRmGetSystemStateByHandle(RMHANDLE hSystemHandle, CSSYSTEMSTATE* pSystemState)
{
	int32 i32Result = 0;
	int nErrorCode = 0;
	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		CWizReadWriteSocket socket;

		if(socket.Connect(strIPAddress, _CsRm_WinSock_Port))
		{
			const uInt32 cSize = FUNCTION_NAME_BUFFER_SIZE + sizeof(hSystemHandle);
			char Com[cSize], *pCom;
			pCom = Com;
			_tcscpy_s(Com, FUNCTION_NAME_BUFFER_SIZE, "GetHState");
			pCom += FUNCTION_NAME_BUFFER_SIZE;
			memcpy(pCom, &hSystemHandle, sizeof(hSystemHandle));
			if (socket.WriteString(Com, cSize))
			{
				char buf[sizeof(CSSYSTEMSTATE)+1 + sizeof(i32Result) + 1];
				socket.ReadString(buf,sizeof(CSSYSTEMSTATE) + sizeof(i32Result));
				memcpy(&i32Result, buf, sizeof(i32Result));
				if (i32Result)
					memcpy(pSystemState, buf + sizeof(i32Result), sizeof(CSSYSTEMSTATE));
				else
					::ZeroMemory(pSystemState, sizeof(CSSYSTEMSTATE));
				break;
			}
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);
			// Stop if this has taken too long
			if(::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT)
				break;
		}
	}
	if (0 == nErrorCode)
		return i32Result;
	else
		return CS_SOCKET_ERROR;
}

int32 DllExport CsRmGetSystemStateByIndex(int16 i16Index, CSSYSTEMSTATE* pSystemState)
{
	int32 i32Result = 0;
	int nErrorCode = 0;
	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		CWizReadWriteSocket socket;

		if(socket.Connect(strIPAddress, _CsRm_WinSock_Port))
		{
			const uInt32 cSize = FUNCTION_NAME_BUFFER_SIZE + sizeof(i16Index);
			char Com[cSize], *pCom;
			pCom = Com;
			_tcscpy_s(Com, FUNCTION_NAME_BUFFER_SIZE, "GetState");
			pCom += FUNCTION_NAME_BUFFER_SIZE;
			memcpy(pCom, &i16Index, sizeof(i16Index));
			if (socket.WriteString(Com, cSize))
			{
				char buf[sizeof(CSSYSTEMSTATE)+1 + sizeof(i32Result) + 1];
				socket.ReadString(buf,sizeof(CSSYSTEMSTATE) + sizeof(i32Result));
				memcpy(&i32Result, buf, sizeof(i32Result));
				if (i32Result)
					memcpy(pSystemState, buf + sizeof(i32Result), sizeof(CSSYSTEMSTATE));
				else
					::ZeroMemory(pSystemState, sizeof(CSSYSTEMSTATE));
				break;
			}
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);
			// Stop if this has taken too long
			if(::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT)
				break;
		}
	}
	if (0 == nErrorCode)
		return i32Result;
	else
		return CS_SOCKET_ERROR;
}

int32 DllExport CsRmFreeSystem(RMHANDLE hSystemHandle)
{
	int32 i32Result = 0;
	int nErrorCode = 0;
	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		CWizReadWriteSocket socket;

		if(socket.Connect(strIPAddress, _CsRm_WinSock_Port))
		{
			const uInt32 cSize = FUNCTION_NAME_BUFFER_SIZE + sizeof(hSystemHandle);
			char Com[cSize], *pCom;
			pCom = Com;
			_tcscpy_s(Com, FUNCTION_NAME_BUFFER_SIZE, "Free");
			pCom += FUNCTION_NAME_BUFFER_SIZE;
			memcpy(pCom, &hSystemHandle, sizeof(hSystemHandle));

			if (socket.WriteString(Com, cSize))
			{
				char buf[sizeof(i32Result)+1];
				socket.ReadString(buf,sizeof(i32Result));
				memcpy(&i32Result, buf, sizeof(i32Result));
				break;
			}
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);
#ifdef _WINDOWS

			if( WSANOTINITIALISED == nErrorCode )
				break;

#endif
           // Stop if this has taken too long
			if(::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT)
				break;
		}
	}
	if (0 == nErrorCode)
        return i32Result;
	else
		return CS_SOCKET_ERROR;
}

int32 DllExport CsRmLockSystem(RMHANDLE hSystemHandle)
{
	int32 i32RetVal = 0;

	uInt32 u32ProcessID = ::GetCurrentProcessId();

	int nErrorCode = 0;
	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		CWizReadWriteSocket socket;

		if(socket.Connect(strIPAddress, _CsRm_WinSock_Port))
		{
			const uInt32 cSize = FUNCTION_NAME_BUFFER_SIZE + sizeof(hSystemHandle) + sizeof(u32ProcessID);
			char Com[cSize], *pCom;
			pCom = Com;
			_tcscpy_s(Com, FUNCTION_NAME_BUFFER_SIZE, "Lock");
			pCom += FUNCTION_NAME_BUFFER_SIZE;
			memcpy(pCom, &hSystemHandle, sizeof(hSystemHandle));
			pCom += sizeof(hSystemHandle);
			memcpy(pCom, &u32ProcessID, sizeof(u32ProcessID));
			if (socket.WriteString(Com, cSize))
			{
				char buf[sizeof(i32RetVal)+1];
				socket.ReadString(buf,sizeof(i32RetVal));
				memcpy(&i32RetVal, buf, sizeof(i32RetVal));
				break;
			}
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);
			// Stop if this has taken too long
			if(::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT)
				break;
		}
	}
	//	The interface was found.  There are now 3 possibilities.
	//	1:	hSystemHandle was not found in the table. In this case
	//		RM will return i32RetVal as CS_INVALID_HANDLE.
	//	2:	hSystemHandle was found but is locked by another process. In this
	//		case, RM will return i32RetVal as CS_HANDLE_IN_USE.
	//	3:	hSystemHandle is found in the table and locked. RM will return
	//		i32RetVal == CS_SUCCESS.
	if (0 == nErrorCode)
		return i32RetVal;
	else
		return CS_SOCKET_ERROR;
}


int32 DllExport CsRmIsSystemHandleValid(RMHANDLE hSystemHandle)
{
	int32 i32RetVal = 0;

	uInt32 u32ProcessID = ::GetCurrentProcessId();

	int nErrorCode = 0;
	DWORD dwStart = ::GetTickCount();
	for(;;)
	{
		CWizReadWriteSocket socket;

		if(socket.Connect(strIPAddress, _CsRm_WinSock_Port))
		{
			const uInt32 cSize = FUNCTION_NAME_BUFFER_SIZE + sizeof(hSystemHandle) + sizeof(u32ProcessID);
			char Com[cSize], *pCom;
			pCom = Com;
			_tcscpy_s(Com, FUNCTION_NAME_BUFFER_SIZE, "Valid");
			pCom += FUNCTION_NAME_BUFFER_SIZE;
			memcpy(pCom, &hSystemHandle, sizeof(hSystemHandle));
			pCom += sizeof(hSystemHandle);
			memcpy(pCom, &u32ProcessID, sizeof(u32ProcessID));

			if (socket.WriteString(Com, cSize))
			{
				char buf[sizeof(i32RetVal)+1];
				socket.ReadString(buf,sizeof(i32RetVal));
				memcpy(&i32RetVal, buf, sizeof(i32RetVal));
				break;
			}
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);
			// Stop if this has taken too long
			if(::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT)
				break;
		}
	}
	// RICKY - MIGHT ALSO HAVE A HANDLE_IN_USE ERROR
	if (0 == nErrorCode)
		return i32RetVal;
	else
		return CS_SOCKET_ERROR;
}



int32 DllExport CsRmRefreshAllSystemHandles(void)
{
	int32 i32Result = CS_FALSE;
	int nErrorCode = 0;
	DWORD dwStart = ::GetTickCount();

	for(;;)
	{
		CWizReadWriteSocket socket;

		if(socket.Connect(strIPAddress, _CsRm_WinSock_Port))
		{
			char Com[FUNCTION_NAME_BUFFER_SIZE];
			_tcscpy_s(Com, FUNCTION_NAME_BUFFER_SIZE, "Refresh");

			if (socket.WriteString(Com, FUNCTION_NAME_BUFFER_SIZE))
			{
				char buf[sizeof(i32Result)+1];
				socket.ReadString(buf,sizeof(i32Result));
				memcpy(&i32Result, buf, sizeof(i32Result));
				break;
			}
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);
			// Stop if this has taken too long
			if(::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT)
				break;
		}
	}
	if (0 == nErrorCode)
		return i32Result;
	else
		return CS_SOCKET_ERROR;
}


int32 DllExport CsRmGetSystem(uInt32 u32BoardType, uInt32 u32ChannelCount, uInt32 u32SampleBits, RMHANDLE *rmHandle, int16 i16Index /* = 0*/)
{
	int32 i32RetVal = CS_FALSE;
	int nErrorCode = 0;
	DWORD dwStart = ::GetTickCount();

	uInt32 u32ProcessID = ::GetCurrentProcessId();

	for(;;)
	{
		CWizReadWriteSocket socket;

		if(socket.Connect(strIPAddress, _CsRm_WinSock_Port))
		{
			const uInt32 cSize = FUNCTION_NAME_BUFFER_SIZE + sizeof(u32BoardType) +
				     sizeof(u32ChannelCount) + sizeof(u32SampleBits) +
					 sizeof(u32ProcessID) + sizeof(i16Index);
			char Com[cSize], *pCom;
			pCom = Com;
			_tcscpy_s(Com, FUNCTION_NAME_BUFFER_SIZE, "GetSystem");
			pCom += FUNCTION_NAME_BUFFER_SIZE;
			memcpy(pCom, &u32BoardType, sizeof(u32BoardType));
			pCom += sizeof(u32BoardType);
			memcpy(pCom, &u32ChannelCount, sizeof(u32ChannelCount));
			pCom += sizeof(u32ChannelCount);
			memcpy(pCom, &u32SampleBits, sizeof(u32SampleBits));
			pCom += sizeof(u32SampleBits);
			memcpy(pCom, &u32ProcessID, sizeof(u32ProcessID));
			pCom += sizeof(u32ProcessID);
			memcpy(pCom, &i16Index, sizeof(i16Index));

			if (socket.WriteString(Com, cSize))
			{
				char buf[sizeof(i32RetVal)+1 + sizeof(RMHANDLE) + 1];
				socket.ReadString(buf, sizeof(i32RetVal) + sizeof(RMHANDLE));
				memcpy(&i32RetVal, buf, sizeof(i32RetVal));
				if (CS_SUCCEEDED(i32RetVal))
					memcpy(rmHandle, buf + sizeof(i32RetVal), sizeof(RMHANDLE));
				else
					rmHandle = 0;
				break;
			}
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);
			// Stop if this has taken too long
			if(::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT)
				break;
		}
	}
	if (0 == nErrorCode)
		return i32RetVal;
	else
		return CS_SOCKET_ERROR;
}

int32 DllExport CsRmGetSystemInfo(RMHANDLE hSystemHandle, CSSYSTEMINFO* pSystemInfo)
{
	_ASSERT(NULL != pSystemInfo);
	if (NULL == pSystemInfo)
		return CS_NULL_POINTER;

	int32 i32Result = 0;
	int nErrorCode = 0;
	DWORD dwStart = ::GetTickCount();

	for(;;)
	{
		CWizReadWriteSocket socket;

		if(socket.Connect(strIPAddress, _CsRm_WinSock_Port))
		{
			const uInt32 cSize = FUNCTION_NAME_BUFFER_SIZE + sizeof(hSystemHandle);
			char Com[cSize], *pCom;
			pCom = Com;
			_tcscpy_s(Com, FUNCTION_NAME_BUFFER_SIZE, "GetInfo");
			pCom += FUNCTION_NAME_BUFFER_SIZE;
			memcpy(pCom, &hSystemHandle, sizeof(hSystemHandle));

			if (socket.WriteString(Com, cSize))
			{
				char buf[sizeof(CSSYSTEMINFO)+1 + sizeof(i32Result) + 1];
				socket.ReadString(buf,sizeof(CSSYSTEMINFO) + sizeof(i32Result));
				memcpy(&i32Result, buf, sizeof(i32Result));
				if (i32Result)
					memcpy(pSystemInfo, buf + sizeof(i32Result), sizeof(CSSYSTEMINFO));
				else
					::ZeroMemory(pSystemInfo, sizeof(CSSYSTEMINFO));
				break;
			}
		}
		else
		{
			nErrorCode = ::WSAGetLastError();

			DisplayErrorMessage(nErrorCode, 1);
			// Stop if this has taken too long
			if(::GetTickCount() - dwStart >= SOCKET_WAIT_TIME_LIMIT)
				break;
		}
	}
	if (0 == nErrorCode)
		return i32Result;
	else
		return CS_SOCKET_ERROR;
}
