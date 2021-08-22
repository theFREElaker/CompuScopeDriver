/////////////////////////////////////////////////////////////////////
// Class Creator Version 2.0.000 Copyrigth (C) Poul A. Costinsky 1994
///////////////////////////////////////////////////////////////////
// Implementation File RawSocket.cpp
// class CWizRawSocket
//
// 23/07/1996 14:54                       Author: Poul, Hadas & Oren
///////////////////////////////////////////////////////////////////

#include "rm.h"

#include "RawSocket.h"

#if defined (_WIN32)
#include <crtdbg.h>

#else

#include <assert.h>
#include <unistd.h> // for close()
#include <wchar.h>
#include <stdio.h>  // for printf and sprintf
#include <fcntl.h>  // for fctl to set socket to O_NONBLOCK

#endif


//*****************************************************************
void CWizSyncSocket::Init(SOCKET h)
{
	m_hSocket = h;
}
//*****************************************************************
void CWizSyncSocket::Close()
{
	_ASSERT(m_hSocket != INVALID_SOCKET);

	::shutdown(m_hSocket, SD_SEND);
	::closesocket(m_hSocket);
	m_hSocket = INVALID_SOCKET;
}
//*****************************************************************
int CWizSyncSocket::SetIntOption(int level, int optname, int val)
{
#if !defined(_WIN32)

	if (SO_DONTLINGER == optname)
	{
		struct linger val;
		val.l_onoff = 0;
		val.l_linger = 0;
		optname = SO_LINGER;
	}
#endif
	return ::setsockopt (m_hSocket, level, optname, (char *)&val, sizeof(val));
}
//*****************************************************************
BOOL CWizSyncSocket::InitializeSocket(int nPort)
{
	// Socket must be created with socket()
	_ASSERT(m_hSocket != INVALID_SOCKET);
	// Make up address
	SOCKADDR_IN	SockAddr;
	memset(&SockAddr,0, sizeof(SockAddr));
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr.s_addr = INADDR_ANY;
	SockAddr.sin_port   = htons((u_short)nPort);
	// Bind to the address and port
	SetIntOption (SOL_SOCKET, SO_REUSEADDR,1); // RG - AUG 23, 2012 (ADDED)

	int r = ::bind(m_hSocket, (SOCKADDR*)&SockAddr, sizeof(SockAddr));
	if (r == 0)
	{
		SetIntOption (SOL_SOCKET, SO_DONTLINGER,1);
//		SetIntOption (SOL_SOCKET, SO_KEEPALIVE,1); // RG - AUG 23, 2012 (COMMENTED OUT)

		// establishes a socket to listen for incoming connection
		// so Accept can be called
		r = ::listen (m_hSocket, 5);
		if (r == 0)
			return TRUE;
	}
	return FALSE;
}
//*****************************************************************
BOOL CWizSyncSocket::Create(int nPort)
{
	// creates a socket
	m_hSocket = ::socket(PF_INET, SOCK_STREAM, 0);

	if (m_hSocket == INVALID_SOCKET)
		return FALSE;

	// Bind to the port
	if (!InitializeSocket(nPort))
	{
		Close();
		return FALSE;
	}

#ifdef __linux__
   // EG: NON-BLOCKING SOCKET ... !!!	
   int flags;
   flags = fcntl(m_hSocket,F_GETFL,0);
   assert(flags != -1);
   ::fcntl(m_hSocket, F_SETFL, flags | O_NONBLOCK);
   // EG: NON-BLOCKING SOCKET ... !!!
#endif

	return TRUE;
}
//*****************************************************************
// Create an invalid socket
CWizSyncSocket::CWizSyncSocket(SOCKET h /* = INVALID_SOCKET */)
{
	Init(h);
}
//*****************************************************************
// Create a listening socket
CWizSyncSocket::CWizSyncSocket(int nPort)
{
	Init(INVALID_SOCKET);
	if(!Create(nPort))
		throw XCannotCreate();
}
//*****************************************************************
CWizSyncSocket::~CWizSyncSocket()
{
	if (m_hSocket != INVALID_SOCKET)
		Close();
}
//*****************************************************************
// Waits for pending connections on the port,
// creates a new socket with the same properties as this
// and returns a handle to the new socket
SOCKET CWizSyncSocket::Accept()
{
	fd_set stReadFDS;  // file descriptor for socket
	fd_set stXcptFDS;
	struct timeval stTimeOut;

	// Clear all sockets from the FDS structure, then put our scoket
	// into the socket descriptor set
	FD_ZERO(&stReadFDS);
	FD_ZERO(&stXcptFDS);

#if defined (_WIN32)
	#pragma warning (disable : 4127)
#endif

	FD_SET(m_hSocket, &stReadFDS);
	FD_SET(m_hSocket, &stXcptFDS);

#if defined (_WIN32)
	#pragma warning (default : 4127)
#endif

	// Initialize the timeout value for ::select to 1 second
	stTimeOut.tv_sec = 1;
	stTimeOut.tv_usec = 0;

	// Check to see if the socket is in a readable state.  If so,
	// the ::accept call should not block. If it will block (i.e. the
	// return value from ::select is 0, then don't call accept

	int nRet = ::select(-1, &stReadFDS, NULL, &stXcptFDS, &stTimeOut);

	SOCKET h = INVALID_SOCKET;
	if (0 != nRet)
		h = ::accept(m_hSocket, NULL, NULL);

	return h;
}
//*****************************************************************
// Cerates a socket and establishes a connection to a peer
// on lpszHostAddress:nHostPort
BOOL CWizSyncSocket::Connect(LPCTSTR lpszHostAddress, UINT nHostPort )
{
	_ASSERT(lpszHostAddress != NULL);
	// Create ? socket
	if (m_hSocket == INVALID_SOCKET)
	{
		m_hSocket = ::socket(PF_INET, SOCK_STREAM, 0);
		if (m_hSocket == INVALID_SOCKET)
			return FALSE;
	}

	// Fill address machinery of sockets.
	SOCKADDR_IN sockAddr;
	memset(&sockAddr,0,sizeof(sockAddr));
	LPSTR lpszAscii = (LPSTR)lpszHostAddress;
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = inet_addr(lpszAscii);
	if (sockAddr.sin_addr.s_addr == INADDR_NONE)
	{
		LPHOSTENT lphost;
		lphost = gethostbyname(lpszAscii);
		if (lphost != NULL)
			sockAddr.sin_addr.s_addr = ((LPIN_ADDR)lphost->h_addr)->s_addr;
		else
		{
			::WSASetLastError(SOCKET_INVALID_VALUE);
			return FALSE;
		}
	}
	sockAddr.sin_port = htons((u_short)nHostPort);
	// connects to peer

	int r = ::connect(m_hSocket, (SOCKADDR*)&sockAddr, sizeof(sockAddr));
	if (r != SOCKET_ERROR)
	{
		_ASSERT(r == 0);
		return TRUE;
	}

	int e = ::WSAGetLastError();

	UNREFERENCED_PARAMETER(e);
    _ASSERT(e != SOCKET_WOULD_BLOCK);

	return FALSE;
}

//*****************************************************************
// read raw data
int	CWizReadWriteSocket::Read(void *pData, int nLen)
{
	char* pcData = (char* )pData;
	int	n = nLen;
	// if data size is bigger then network buffer
	// handle it nice
	do
	{
		int r1 = ::recv (m_hSocket, pcData, n, 0);
		if (r1 == SOCKET_ERROR)
		{
			int e = ::WSAGetLastError();

			UNREFERENCED_PARAMETER(e);
			_ASSERT(e != SOCKET_WOULD_BLOCK);
			return 0;
		}
		else if (r1 == 0)
			return 0;
		else if (r1 < 0)
		{
			_ASSERT(0);
			return 0;
		}
		pcData += r1;
		n -= r1;
	}while (n > 0);

	_ASSERT(n == 0);
	return nLen;
}
//*****************************************************************
// write raw data
int	CWizReadWriteSocket::Write(const void *pData, int nLen)
{
	const char* pcData = (const char* )pData;
	int	n = nLen;
	// if data size is bigger then network buffer
	// handle it nice

	do
	{
		int r1 = ::send (m_hSocket, pcData, n, 0);
		if (r1 == SOCKET_ERROR)
		{
			int e = ::WSAGetLastError();

			UNREFERENCED_PARAMETER(e);
			_ASSERT(e != SOCKET_WOULD_BLOCK);
			return 0;
		}
		else if (r1 == 0)
			return 0;
		else if (r1 < 0)
		{
			_ASSERT(0);
			return 0;
		}
		pcData += r1;
		n -= r1;
	}while (n > 0);

	_ASSERT(n == 0);
	return nLen;
}
//*****************************************************************
// Reads UNICODE string from socket.
// Converts string from ANSI to UNICODE.
int CWizReadWriteSocket::ReadString  (WCHAR* lpszString, int nMaxLen)
{
	// read string length
	u_long nt_Len;
	if (Read(&nt_Len, sizeof(nt_Len)) < (int)sizeof(nt_Len))
		return 0;
	int Len = ntohl(nt_Len);
	if (Len == 0 || Len >= nMaxLen)
		return 0;

	static const int BUFF_SIZE = SOCKET_BUFFER_LENGTH * 2;
	if (Len >= BUFF_SIZE)
		return 0;
	char buff[BUFF_SIZE];
	// Read ANSI string to the buffer
	if (Read(buff, Len) < Len)
		return 0;
	buff[Len] = 0;

#if defined (_WIN32) // RICKY - CHECK ABOUT THIS FOR LINUX
	// Convert ANSI string to the UNICODE
	::MultiByteToWideChar(CP_ACP, 0, buff, Len, lpszString, nMaxLen*sizeof(lpszString[0]));
#endif
	return Len;
}
//*****************************************************************
// Reads ANSI string from socket.
int CWizReadWriteSocket::ReadString  (char* lpszString, int nMaxLen)
{
	// read string length
	u_long nt_Len;
	if (Read(&nt_Len, sizeof(nt_Len)) < (int)sizeof(nt_Len))
		return 0;
	int Len = ntohl(nt_Len);
	if (Len == 0 || Len > nMaxLen)
		return 0;

	// Read ANSI string
	if (Read(lpszString, Len) < Len)
		return 0;
	if(Len < nMaxLen)
		lpszString[Len] = 0;
	return Len;
}
//*****************************************************************
inline int Length(const char* p)
{
	return (int)strlen(p);
}
//*****************************************************************
inline int Length(const WCHAR* p)
{
	return (int)wcslen(p);
}
//*****************************************************************
// Writes UNICODE string to socket,
// converts UNICODE string to ANSI.
BOOL CWizReadWriteSocket::WriteString (const WCHAR*  lpszString, int nLen /* = -1*/)
{
	if (nLen < 0)
		nLen = Length(lpszString);
	static const int BUFF_SIZE = SOCKET_BUFFER_LENGTH * 2;
	if (nLen >= (int)(BUFF_SIZE*sizeof(lpszString) + sizeof(u_long)))
		return FALSE;
	char buff[BUFF_SIZE];
	u_long nt_Len = htonl(nLen);
	int nSize = sizeof(nt_Len);
	memcpy(buff, &nt_Len, nSize);
	// To make one call less, the length of the string
	// copied to the buffer before the string itself
	// and the buffer sent once.

	if (nLen > 0)
	{
#if defined (_WIN32) // RICKY - HAVE TO FIX THIS FOR LINUX
		// Convert ANSI to UNICODE
		char* ptr = buff + nSize;
		int s = WideCharToMultiByte(CP_ACP, 0, lpszString, nLen, ptr, BUFF_SIZE - sizeof(u_long), NULL, NULL);
		_ASSERT(s > 0);
		nSize += s;
#endif
	}
	return Write(buff, nSize);
}
//*****************************************************************
// Writes ANSI string to socket.
BOOL CWizReadWriteSocket::WriteString (const char* lpszString, int nLen )
{
	char* buff = new char[nLen+100];

	u_long nt_Len = htonl(nLen);

	int nSize = sizeof(nt_Len);

	memcpy(buff, &nt_Len, nSize);

	// To make one call less, the length of the string
	// copied to the buffer before the string itself
	// and the buffer sent once.

	char* ptr = buff + nSize;

	if (nLen > 0)
	{
		memcpy(ptr, lpszString, nLen);
		nSize += nLen;
	}
	int ret = Write(buff, nSize);
	delete[] buff;
	return ret;
}
//*****************************************************************
BOOL CWizSyncSocket::GetHostName(LPTSTR lpszAddress, size_t nAddrSize, UINT& rSocketPort)
{
	if (nAddrSize < 1)
		return FALSE;

	_ASSERT(lpszAddress != NULL);
	if (NULL == lpszAddress)
		return FALSE;

	*lpszAddress = TCHAR(0);

	SOCKADDR_IN sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));

	SOCKET_LENGTH nSockAddrLen = sizeof(sockAddr);
	int r;

	while ((r = ::getsockname(m_hSocket, (SOCKADDR*)&sockAddr, &nSockAddrLen)) == SOCKET_ERROR)
	{
		int e = ::WSAGetLastError();

		if (e != SOCKET_IN_PROGRESS)
			return FALSE;
	}

	_ASSERT(r == 0);
	rSocketPort = ntohs(sockAddr.sin_port);

	char    szName[64];
	struct  hostent *h;
	DWORD	dwMyAddress;

	int r1;
	while((r1 = ::gethostname(szName,sizeof(szName))) == SOCKET_ERROR)
	{
		int e = ::WSAGetLastError();

		if (e != SOCKET_IN_PROGRESS)
			return FALSE;
	}
	_ASSERT(r1 == 0);

	h = (struct hostent *) ::gethostbyname(szName);
        if (h == NULL)
        {
#if defined(_DEBUG) && !defined(_WIN32)
			DisplayErrorMessage(h_errno, 1);
#endif
			return FALSE;
        }

	memcpy(&dwMyAddress,h->h_addr_list[0],sizeof(DWORD));
	if (dwMyAddress == INADDR_NONE)
		return FALSE;

	struct   in_addr     tAddr;
	memcpy(&tAddr,&dwMyAddress,sizeof(tAddr));
	char    *ptr = ::inet_ntoa(tAddr);

	_ASSERT(ptr != NULL);

	if (NULL == ptr)
		return FALSE;
	if (size_t(lstrlen(ptr)) >= nAddrSize)
		return FALSE;
	lstrcpy(lpszAddress, ptr);
	return TRUE;
}

//*****************************************************************
BOOL CWizSyncSocket::GetPeerName(LPTSTR lpszAddress, size_t nAddrSize, UINT& rPeerPort)
{
	if (nAddrSize < 1)
		return FALSE;
	*lpszAddress = TCHAR(0);

	SOCKADDR_IN sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));

	SOCKET_LENGTH nSockAddrLen = sizeof(sockAddr);
	int r;
	while ((r = ::getpeername(m_hSocket, (SOCKADDR*)&sockAddr, &nSockAddrLen)) == SOCKET_ERROR)
	{
		int e = ::WSAGetLastError();

		if (e != SOCKET_IN_PROGRESS)
			return FALSE;
	}

	_ASSERT(r == 0);
	rPeerPort = ntohs(sockAddr.sin_port);
	char * pAddr = inet_ntoa(sockAddr.sin_addr);
	int len = (int)strlen(pAddr);
	if (size_t(len) >= nAddrSize)
		return FALSE;
	memcpy(lpszAddress, pAddr,len + 1);
	return TRUE;
}
//*****************************************************************

#ifdef _DEBUG
void Foo()
{

	CWizReadWriteSocket s;
	int i=1;
	short j= 2;
	char k=3;
	double d=4.0;

	s << i << j << k << d;
	s >> i >> j >> k >> d;
}

#endif

