#include <winsock2.h>
#include "vxi11.h"
#include <rpc/rpc.h>
#include <rpc/pmap_cln.h>
#include <stdio.h>
#include <tchar.h>

#define VXI_PORT 111
char szIpAddress[25] = "";
int  tcp_port = 0;

static bool_t who_responded(char *out, struct sockaddr_in *addr) 
{
	int my_port_T;
//	int my_port_U;
	UNREFERENCED_PARAMETER(out);
	my_port_T = pmap_getport(addr, DEVICE_CORE, DEVICE_CORE_VERSION, IPPROTO_TCP);
	if ( my_port_T )
	{
		lstrcpy(szIpAddress, inet_ntoa(addr->sin_addr));
		tcp_port = my_port_T;
	}
	if ( 0 == tcp_port || !lstrcmp(szIpAddress, "") )
		return 0;
	else
		return 1;
}

int vxi_discovery(char* Address, DWORD* dwPort)
{
	CLIENT* VXI11Client;
	Create_LinkParms MyCreate_LinkParms;
	Create_LinkResp* MyCreate_LinkResp;
	Device_Link MyLink;
	Device_WriteParms MyDevice_WriteParms;
	Device_ReadParms MyDevice_ReadParms;
	Device_WriteResp* MyDevice_WriteResp;
	Device_ReadResp* MyDevice_ReadResp;	
	char DataRead[200];
	int  IsGageBoard;

	enum clnt_stat rpc_stat;


	rpc_stat = clnt_broadcast(DEVICE_CORE, DEVICE_CORE_VERSION, NULLPROC,
							 (xdrproc_t)xdr_void, (char *)NULL,
							 (xdrproc_t)xdr_void, (char *)NULL,
							 who_responded);

	if (RPC_SUCCESS != rpc_stat)
	{
		// RG - LOG AN ERROR
		//Use clnt_perrno for error message.
		return -1;
	}
	VXI11Client = clnt_create(szIpAddress, DEVICE_CORE, DEVICE_CORE_VERSION, "tcp");
	if (VXI11Client == NULL)
	{
		fprintf(stderr, "Couldn't connect");
		//RG - LOG ERROR USE clnt_perrno  for error message
		return -1;
	}

	MyCreate_LinkParms.clientId = 0; // not used
	MyCreate_LinkParms.lockDevice = 0; // no exclusive access
	MyCreate_LinkParms.lock_timeout = 0;
	MyCreate_LinkParms.device = "inst0";

	if ((MyCreate_LinkResp = create_link_1(&MyCreate_LinkParms, VXI11Client)) == NULL)
	{
		fprintf(stderr, "Couldn't connect");
		//RG - LOG ERROR USE clnt_perrno  for error message
		return -1;
	}

	MyLink = MyCreate_LinkResp->lid; // save link for future use

	MyDevice_WriteParms.lid = MyLink;
	MyDevice_WriteParms.io_timeout = 10000; // in ms
	MyDevice_WriteParms.lock_timeout = 10000; // in ms
	MyDevice_WriteParms.flags = 0;
	MyDevice_WriteParms.data.data_val = "*IDN?\n";
	MyDevice_WriteParms.data.data_len = 6;
	if ((MyDevice_WriteResp = device_write_1(&MyDevice_WriteParms, VXI11Client)) == NULL)
	{
		fprintf(stderr, "Couldn't write to device");
		//RG - LOG ERROR USE clnt_perrno  for error message
		return -1;
	}


	MyDevice_ReadParms.lid = MyLink;
	MyDevice_ReadParms.requestSize = 200;
	MyDevice_ReadParms.io_timeout = 10000;
	MyDevice_ReadParms.lock_timeout = 10000;
	MyDevice_ReadParms.flags = 0;
	MyDevice_ReadParms.termChar = '\n';

	if ((MyDevice_ReadResp = device_read_1(&MyDevice_ReadParms, VXI11Client)) == NULL)
	{
//		fprintf(stderr, "Couldn't read from device");
		//RG - LOG ERROR USE clnt_perrno  for error message
		return -1;
	}		


	strncpy_s(DataRead, _countof(DataRead), MyDevice_ReadResp->data.data_val, MyDevice_ReadResp->data.data_len);
	DataRead[MyDevice_ReadResp->data.data_len] = 0;
//	printf("Instrument ID string: %s\n", DataRead); //RG LOG INSTEAD

	if (destroy_link_1(&MyLink, VXI11Client) == NULL)
	{
		fprintf(stderr, "Couldn't destroy link");
		//RG - LOG ERROR USE clnt_perrno  for error message 
		return -1;
	}
	clnt_destroy(VXI11Client);

	
	IsGageBoard = ( (NULL == _tcsstr(DataRead, "Gage Applied")) && (NULL == _tcsstr(DataRead, "GAGE")) ) ? 0 : 1;
	if ( IsGageBoard )
	{
		*dwPort = (DWORD)tcp_port;
		lstrcpy(Address, szIpAddress);
	}
	return IsGageBoard;
}

int IsGageSystem(char* szIpAddress)
{
	CLIENT* VXI11Client;
	Create_LinkParms MyCreate_LinkParms;
	Create_LinkResp* MyCreate_LinkResp;
	Device_Link MyLink;
	Device_WriteParms MyDevice_WriteParms;
	Device_ReadParms MyDevice_ReadParms;
	Device_WriteResp* MyDevice_WriteResp;
	Device_ReadResp* MyDevice_ReadResp;	
	char DataRead[200];
	int  IsGageBoard;


	VXI11Client = clnt_create(szIpAddress, DEVICE_CORE, DEVICE_CORE_VERSION, "tcp");
	if (VXI11Client == NULL)
	{
//		fprintf(stderr, "Couldn't connect");
		//RG - LOG ERROR USE clnt_perrno  for error message
		return -1;
	}

	MyCreate_LinkParms.clientId = 0; // not used
	MyCreate_LinkParms.lockDevice = 0; // no exclusive access
	MyCreate_LinkParms.lock_timeout = 0;
	MyCreate_LinkParms.device = "inst0";

	if ((MyCreate_LinkResp = create_link_1(&MyCreate_LinkParms, VXI11Client)) == NULL)
	{
//		fprintf(stderr, "Couldn't connect");
		//RG - LOG ERROR USE clnt_perrno  for error message
		return -1;
	}

	MyLink = MyCreate_LinkResp->lid; // save link for future use

	MyDevice_WriteParms.lid = MyLink;
	MyDevice_WriteParms.io_timeout = 10000; // in ms
	MyDevice_WriteParms.lock_timeout = 10000; // in ms
	MyDevice_WriteParms.flags = 0;
	MyDevice_WriteParms.data.data_val = "*IDN?\n";
	MyDevice_WriteParms.data.data_len = 6;
	if ((MyDevice_WriteResp = device_write_1(&MyDevice_WriteParms, VXI11Client)) == NULL)
	{
		fprintf(stderr, "Couldn't write to device");
		//RG - LOG ERROR USE clnt_perrno  for error message
		return -1;
	}


	MyDevice_ReadParms.lid = MyLink;
	MyDevice_ReadParms.requestSize = 200;
	MyDevice_ReadParms.io_timeout = 10000;
	MyDevice_ReadParms.lock_timeout = 10000;
	MyDevice_ReadParms.flags = 0;
	MyDevice_ReadParms.termChar = '\n';

	if ((MyDevice_ReadResp = device_read_1(&MyDevice_ReadParms, VXI11Client)) == NULL)
	{
		fprintf(stderr, "Couldn't read from device");
		//RG - LOG ERROR USE clnt_perrno  for error message
		return -1;
	}		


	strncpy_s(DataRead, _countof(DataRead), MyDevice_ReadResp->data.data_val, MyDevice_ReadResp->data.data_len);
	DataRead[MyDevice_ReadResp->data.data_len] = 0;

	if (destroy_link_1(&MyLink, VXI11Client) == NULL)
	{
		fprintf(stderr, "Couldn't destroy link");
		//RG - LOG ERROR USE clnt_perrno  for error message 
		return -1;
	}
	clnt_destroy(VXI11Client);


	IsGageBoard = ( (NULL == _tcsstr(DataRead, "Gage Applied")) && (NULL == _tcsstr(DataRead, "GAGE")) ) ? 0 : 1;
	return IsGageBoard;
}

