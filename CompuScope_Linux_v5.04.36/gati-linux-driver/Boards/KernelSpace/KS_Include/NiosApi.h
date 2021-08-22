#ifndef	NIOSAPI_H_
#define NIOSAPI_H_

#include "CsTypes.h"
#include "GageDrv.h"

BOOLEAN	IsNiosInit( PFDO_DATA FdoData );
uInt32	ReadRegisterFromNios(PFDO_DATA FdoData, uInt32 Data, uInt32 Command);
int32	WriteRegisterThruNios(PFDO_DATA FdoData, uInt32 Data, uInt32 Command );
int32	WriteRegisterThruNiosEx(PFDO_DATA FdoData, uInt32 Data, uInt32 Command, uInt32 *pu32NiosReturn );

int32	StartAcquisition( PFDO_DATA FdoData, BOOLEAN	bRtMode );
int32	AbortAcquisition ( PFDO_DATA FdoData );
int32	AbortTransfer ( PFDO_DATA FdoData );
int32	StartTransferAtAddress ( PFDO_DATA FdoData, uInt32 StartAddressOrSegmentNumber);
void	StartTransferDDRtoFIFO (  PFDO_DATA FdoData, uInt32 Offset );
void	SetFifoMode ( PFDO_DATA FdoData, int32 Mode);
void	ForceTrigger( PFDO_DATA FdoData );

			
void	SetFdoLookupTable( PFDO_DATA FdoData );
void	ClearFdoLookupTable( PFDO_DATA FdoData );
uInt32	GetFreeCardIndex(void);
void	SetFreeIndex( uInt32 Index );
void	ClearFreeIndex( uInt32 Index );
void	BBtiming(PFDO_DATA FdoData, uInt32 u32Delay_100us);


#endif	// NIOS_API_H_
