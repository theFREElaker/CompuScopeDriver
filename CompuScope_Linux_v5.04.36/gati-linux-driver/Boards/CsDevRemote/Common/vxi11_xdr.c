#include <rpc/rpc.h>
#include "vxi11.h"

#pragma warning( disable : 4131 )

bool_t
xdr_Device_Link(xdrs, objp)
	XDR *xdrs;
	Device_Link *objp;
{
	if (!xdr_long(xdrs, objp)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Device_AddrFamily(xdrs, objp)
	XDR *xdrs;
	Device_AddrFamily *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Device_Flags(xdrs, objp)
	XDR *xdrs;
	Device_Flags *objp;
{
	if (!xdr_long(xdrs, objp)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Device_ErrorCode(xdrs, objp)
	XDR *xdrs;
	Device_ErrorCode *objp;
{
	if (!xdr_long(xdrs, objp)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Device_Error(xdrs, objp)
	XDR *xdrs;
	Device_Error *objp;
{
	if (!xdr_Device_ErrorCode(xdrs, &objp->error)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Create_LinkParms(xdrs, objp)
	XDR *xdrs;
	Create_LinkParms *objp;
{
	if (!xdr_long(xdrs, &objp->clientId)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->lockDevice)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->lock_timeout)) {
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->device, ~0)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Create_LinkResp(xdrs, objp)
	XDR *xdrs;
	Create_LinkResp *objp;
{
	if (!xdr_Device_ErrorCode(xdrs, &objp->error)) {
		return (FALSE);
	}
	if (!xdr_Device_Link(xdrs, &objp->lid)) {
		return (FALSE);
	}
	if (!xdr_u_short(xdrs, &objp->abortPort)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->maxRecvSize)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Device_WriteParms(xdrs, objp)
	XDR *xdrs;
	Device_WriteParms *objp;
{
	if (!xdr_Device_Link(xdrs, &objp->lid)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->io_timeout)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->lock_timeout)) {
		return (FALSE);
	}
	if (!xdr_Device_Flags(xdrs, &objp->flags)) {
		return (FALSE);
	}
	if (!xdr_bytes(xdrs, (char **)&objp->data.data_val, (u_int *)&objp->data.data_len, ~0)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Device_WriteResp(xdrs, objp)
	XDR *xdrs;
	Device_WriteResp *objp;
{
	if (!xdr_Device_ErrorCode(xdrs, &objp->error)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->size)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Device_ReadParms(xdrs, objp)
	XDR *xdrs;
	Device_ReadParms *objp;
{
	if (!xdr_Device_Link(xdrs, &objp->lid)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->requestSize)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->io_timeout)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->lock_timeout)) {
		return (FALSE);
	}
	if (!xdr_Device_Flags(xdrs, &objp->flags)) {
		return (FALSE);
	}
	if (!xdr_char(xdrs, &objp->termChar)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Device_ReadResp(xdrs, objp)
	XDR *xdrs;
	Device_ReadResp *objp;
{
	if (!xdr_Device_ErrorCode(xdrs, &objp->error)) {
		return (FALSE);
	}
	if (!xdr_long(xdrs, &objp->reason)) {
		return (FALSE);
	}
	if (!xdr_bytes(xdrs, (char **)&objp->data.data_val, (u_int *)&objp->data.data_len, ~0)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Device_ReadStbResp(xdrs, objp)
	XDR *xdrs;
	Device_ReadStbResp *objp;
{
	if (!xdr_Device_ErrorCode(xdrs, &objp->error)) {
		return (FALSE);
	}
	if (!xdr_u_char(xdrs, &objp->stb)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Device_GenericParms(xdrs, objp)
	XDR *xdrs;
	Device_GenericParms *objp;
{
	if (!xdr_Device_Link(xdrs, &objp->lid)) {
		return (FALSE);
	}
	if (!xdr_Device_Flags(xdrs, &objp->flags)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->lock_timeout)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->io_timeout)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Device_RemoteFunc(xdrs, objp)
	XDR *xdrs;
	Device_RemoteFunc *objp;
{
	if (!xdr_u_long(xdrs, &objp->hostAddr)) {
		return (FALSE);
	}
	if (!xdr_u_short(xdrs, &objp->hostPort)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->progNum)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->progVers)) {
		return (FALSE);
	}
	if (!xdr_Device_AddrFamily(xdrs, &objp->progFamily)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Device_EnableSrqParms(xdrs, objp)
	XDR *xdrs;
	Device_EnableSrqParms *objp;
{
	if (!xdr_Device_Link(xdrs, &objp->lid)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->enable)) {
		return (FALSE);
	}
	if (!xdr_bytes(xdrs, (char **)&objp->handle.handle_val, (u_int *)&objp->handle.handle_len, 40)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Device_LockParms(xdrs, objp)
	XDR *xdrs;
	Device_LockParms *objp;
{
	if (!xdr_Device_Link(xdrs, &objp->lid)) {
		return (FALSE);
	}
	if (!xdr_Device_Flags(xdrs, &objp->flags)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->lock_timeout)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Device_DocmdParms(xdrs, objp)
	XDR *xdrs;
	Device_DocmdParms *objp;
{
	if (!xdr_Device_Link(xdrs, &objp->lid)) {
		return (FALSE);
	}
	if (!xdr_Device_Flags(xdrs, &objp->flags)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->io_timeout)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->lock_timeout)) {
		return (FALSE);
	}
	if (!xdr_long(xdrs, &objp->cmd)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->network_order)) {
		return (FALSE);
	}
	if (!xdr_long(xdrs, &objp->datasize)) {
		return (FALSE);
	}
	if (!xdr_bytes(xdrs, (char **)&objp->data_in.data_in_val, (u_int *)&objp->data_in.data_in_len, ~0)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Device_DocmdResp(xdrs, objp)
	XDR *xdrs;
	Device_DocmdResp *objp;
{
	if (!xdr_Device_ErrorCode(xdrs, &objp->error)) {
		return (FALSE);
	}
	if (!xdr_bytes(xdrs, (char **)&objp->data_out.data_out_val, (u_int *)&objp->data_out.data_out_len, ~0)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_Device_SrqParms(xdrs, objp)
	XDR *xdrs;
	Device_SrqParms *objp;
{
	if (!xdr_bytes(xdrs, (char **)&objp->handle.handle_val, (u_int *)&objp->handle.handle_len, ~0)) {
		return (FALSE);
	}
	return (TRUE);
}

#pragma warning( default : 4131 )
