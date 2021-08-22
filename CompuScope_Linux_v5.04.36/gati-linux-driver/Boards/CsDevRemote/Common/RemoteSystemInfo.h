
#pragma once

#include <vector>

using namespace std;

#define SERVERPORT 111    // the port users will be connecting to, the RPC port

typedef unsigned int uint;

enum auth_flavor
{
	AUTH_NULL   = 0,
	AUTH_UNIX   = 1,
	AUTH_SHORT  = 2,
	AUTH_DES    = 3
};
struct opaque_auth
{
	auth_flavor flavor;
	uint length;
};
struct call_body
{
	uint rpcvers;
	uint prog;
	uint vers;
	uint proc;
	opaque_auth cred;
	opaque_auth verf;
};

    //RPC call packet structure
struct RpcCallPacket
{
	uint xid;
	uint type;
	call_body cbody;
	uint prgm;
	uint vers;
	uint proto;
	uint port;
};


struct reply_body
{
	uint replystate;
	opaque_auth verf;
	uint acceptstate;
};

//RPC Reply packet structure
struct RpcReplyPacket
{
	uint xid;
	uint type;
	reply_body rbody;
	uint port;
};

typedef struct _systemIpInfo
{
	string	strIpAddress;
	DWORD	dwPort;
} systemIpInfo;

typedef vector<systemIpInfo> vecIpInfo;

int FindRemoteSystems(vecIpInfo& vSystem, int* err);
