#include <rpc/rpc.h>
#include "vxi11.h"

#pragma warning( disable : 4131 )

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

Device_Error *
device_abort_1(argp, clnt)
	Device_Link *argp;
	CLIENT *clnt;
{
	static Device_Error res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, device_abort, xdr_Device_Link, argp, xdr_Device_Error, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


Create_LinkResp *
create_link_1(argp, clnt)
	Create_LinkParms *argp;
	CLIENT *clnt;
{
	static Create_LinkResp res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, create_link, xdr_Create_LinkParms, argp, xdr_Create_LinkResp, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


Device_WriteResp *
device_write_1(argp, clnt)
	Device_WriteParms *argp;
	CLIENT *clnt;
{
	static Device_WriteResp res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, device_write, xdr_Device_WriteParms, argp, xdr_Device_WriteResp, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


Device_ReadResp *
device_read_1(argp, clnt)
	Device_ReadParms *argp;
	CLIENT *clnt;
{
	static Device_ReadResp res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, device_read, xdr_Device_ReadParms, argp, xdr_Device_ReadResp, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


Device_ReadStbResp *
device_readstb_1(argp, clnt)
	Device_GenericParms *argp;
	CLIENT *clnt;
{
	static Device_ReadStbResp res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, device_readstb, xdr_Device_GenericParms, argp, xdr_Device_ReadStbResp, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


Device_Error *
device_trigger_1(argp, clnt)
	Device_GenericParms *argp;
	CLIENT *clnt;
{
	static Device_Error res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, device_trigger, xdr_Device_GenericParms, argp, xdr_Device_Error, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


Device_Error *
device_clear_1(argp, clnt)
	Device_GenericParms *argp;
	CLIENT *clnt;
{
	static Device_Error res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, device_clear, xdr_Device_GenericParms, argp, xdr_Device_Error, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


Device_Error *
device_remote_1(argp, clnt)
	Device_GenericParms *argp;
	CLIENT *clnt;
{
	static Device_Error res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, device_remote, xdr_Device_GenericParms, argp, xdr_Device_Error, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


Device_Error *
device_local_1(argp, clnt)
	Device_GenericParms *argp;
	CLIENT *clnt;
{
	static Device_Error res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, device_local, xdr_Device_GenericParms, argp, xdr_Device_Error, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


Device_Error *
device_lock_1(argp, clnt)
	Device_LockParms *argp;
	CLIENT *clnt;
{
	static Device_Error res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, device_lock, xdr_Device_LockParms, argp, xdr_Device_Error, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


Device_Error *
device_unlock_1(argp, clnt)
	Device_Link *argp;
	CLIENT *clnt;
{
	static Device_Error res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, device_unlock, xdr_Device_Link, argp, xdr_Device_Error, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


Device_Error *
device_enable_srq_1(argp, clnt)
	Device_EnableSrqParms *argp;
	CLIENT *clnt;
{
	static Device_Error res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, device_enable_srq, xdr_Device_EnableSrqParms, argp, xdr_Device_Error, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


Device_DocmdResp *
device_docmd_1(argp, clnt)
	Device_DocmdParms *argp;
	CLIENT *clnt;
{
	static Device_DocmdResp res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, device_docmd, xdr_Device_DocmdParms, argp, xdr_Device_DocmdResp, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


Device_Error *
destroy_link_1(argp, clnt)
	Device_Link *argp;
	CLIENT *clnt;
{
	static Device_Error res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, destroy_link, xdr_Device_Link, argp, xdr_Device_Error, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


Device_Error *
create_intr_chan_1(argp, clnt)
	Device_RemoteFunc *argp;
	CLIENT *clnt;
{
	static Device_Error res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, create_intr_chan, xdr_Device_RemoteFunc, argp, xdr_Device_Error, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


Device_Error *
destroy_intr_chan_1(argp, clnt)
	void *argp;
	CLIENT *clnt;
{
	static Device_Error res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, destroy_intr_chan, xdr_void, argp, xdr_Device_Error, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


void *
device_intr_srq_1(argp, clnt)
	Device_SrqParms *argp;
	CLIENT *clnt;
{
	static char res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, device_intr_srq, xdr_Device_SrqParms, argp, xdr_void, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return ((void *)&res);
}

#pragma warning( default : 4131 )