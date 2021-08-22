
#ifndef	_CSDRIVERTYPES_H_

#define _CSDRIVERTYPES_H_

#include "CsTypes.h"
#include "CsStruct.h"
#include "CsPrivateStruct.h"

#ifdef __linux__
#ifndef __KERNEL__
#include "CWinEventHandle.h"
#else
typedef	 HANDLE		EVENT_HANDLE;
#endif
#endif

#ifdef _WINDOWS_

typedef  HANDLE		FILE_HANDLE;
typedef	 HANDLE		EVENT_HANDLE;
typedef	 HANDLE		SEM_HANDLE;

#endif

typedef enum
{
	PCI_BUS	= 1,		// PCI
	PCIE_BUS	= 2		// PCI Express
	
} BUSTYPE, *PBUSTYPE;


#define		MAX_STR_LENGTH	50


#pragma pack (8)

typedef struct _CSDEVICE_INFO
{
	uInt16		DeviceId;
	BUSTYPE		BusType;
	DRVHANDLE	DrvHdl;
	char		SymbolicLink[MAX_STR_LENGTH];
} CSDEVICE_INFO, *PCSDEVICE_INFO;


typedef struct _out_GET_CS_CARD_COUNT
{
	uInt32	u32NumOfCards;		// Number of cards detected by Kernel driver
	BOOLEAN	bChanged;			// Number of cards has chnaged since the last request.

} CS_CARD_COUNT;



typedef struct _in_DATA_TRANSFER
{
	IN_PARAMS_TRANSFERDATA	InParams;
	OUT_PARAMS_TRANSFERDATA	*OutParams;

} in_DATA_TRANSFER;



typedef struct _io_DATA_TRANSFER_EX
{
	IN_PARAMS_TRANSFERDATA_EX		InParams;
	OUT_PARAMS_TRANSFERDATA_EX		*OutParams;

} io_DATA_TRANSFER_EX;



#pragma pack ()
#endif 		// _CSTOCTLTYPES_H_
