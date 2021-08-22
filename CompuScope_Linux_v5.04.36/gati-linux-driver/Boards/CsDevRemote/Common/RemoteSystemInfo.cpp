#include <winsock2.h>
#include "RemoteSystemInfo.h"


int FindRemoteSystems(vecIpInfo& vSystem, int* err)
{
	int xid = 1000;
	int sockfd;
	struct sockaddr_in dest_addr;	// connector's address information
	struct sockaddr_in sLocalAddr;	

	int numbytes;
	int broadcast = 1;

	if ((sockfd = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		*err = WSAGetLastError();
		return -1;
	}

	// this call is what allows broadcast packets to be sent:
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof broadcast) == -1) 
	{
		*err = WSAGetLastError();
		return -1;
	}
	DWORD dwTimeout = 1000;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwTimeout, sizeof(dwTimeout)) == -1) 
	{
		*err = WSAGetLastError();
		return -1;
	}

	dest_addr.sin_family = AF_INET;     // host byte order
	dest_addr.sin_port = htons(SERVERPORT); // short, network byte order
	dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);//*((struct in_addr *)he->h_addr);
	memset(dest_addr.sin_zero, '\0', sizeof dest_addr.sin_zero);

	RpcCallPacket rSend;

	rSend.xid = htonl(xid);
	rSend.type = htonl(0);
	rSend.cbody.rpcvers = htonl(2);
	rSend.cbody.prog = htonl(100000);
	rSend.cbody.vers = htonl(2);
	rSend.cbody.proc = htonl(3);
	rSend.cbody.cred.flavor = AUTH_NULL;
	rSend.cbody.cred.length = htonl(0);
	rSend.cbody.verf.flavor = AUTH_NULL;
	rSend.cbody.verf.length = htonl(0);
	rSend.prgm = htonl(395183);
	rSend.vers = htonl(1);
	rSend.proto = htonl(6);
	rSend.port = htonl(0);

	if ((numbytes=sendto(sockfd, (const char*)&rSend, sizeof(rSend), 0, (struct sockaddr *)&dest_addr, sizeof dest_addr)) == -1) 
	{
		*err = WSAGetLastError();
		return -1;
	}

	int recStringLen;
	struct sockaddr_in from;
	
	RpcReplyPacket reply;
	reply.xid = 0;
	reply.type = 0;
	reply.rbody.replystate = 0;
	reply.rbody.verf.flavor = AUTH_NULL;
	reply.rbody.verf.length = 0;
	reply.rbody.acceptstate = 0;
	reply.port = 0;
	xid = (xid + 1) % 1000;

	bool bFlag = true; // just to get rid of a compiler warning
	while (bFlag)
	{
		u_short recPort = 5555;
		sLocalAddr.sin_family = AF_INET;
		sLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		sLocalAddr.sin_port = htons(recPort);
		
		int fromSize = sizeof(from);
		recStringLen = recvfrom(sockfd, (char*)&reply, sizeof(reply), 0, (sockaddr*)&from, &fromSize);
		int er = WSAGetLastError();
		if ( WSAETIMEDOUT == er )
		{
			break;
		}
		else if ( 0 != er ) // some other error occurred
		{
			*err = er;
			break;
		}

		int port_no = ntohl(reply.port);
		if ( 0 != port_no )
		{
			systemIpInfo sysInfo;
			sysInfo.dwPort = (DWORD)ntohl(reply.port);
			sysInfo.strIpAddress = inet_ntoa(from.sin_addr);
			vSystem.push_back(sysInfo);
		}
	}

//	xid = (xid + 1) % 1000;
	closesocket(sockfd);
	return (int)vSystem.size();
}