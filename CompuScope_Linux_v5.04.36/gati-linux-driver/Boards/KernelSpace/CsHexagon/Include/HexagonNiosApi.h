#ifndef hexagonniosapi_h
#define hexagonniosapi_h

#include "CsTypes.h"

#define   SEND_NIOS_COMMAND(FdoDat, Data, Command)   WriteRegisterThruNios(FdoDat, Data, Command, -1)

BOOLEAN IsNiosInit              (PFDO_DATA FdoData);
uInt32  ReadRegisterFromNios    (PFDO_DATA FdoData, uInt32 Data, uInt32 Command);
int32   WriteRegisterThruNios   (PFDO_DATA FdoData, uInt32 Data, uInt32 Command, int32 i32Timeout_100us);
int32   WriteRegisterThruNiosEx (PFDO_DATA FdoData, uInt32 Data, uInt32 Command, int32 i32Timeout_100us, uInt32 *pu32NiosReturn);

int32  StartAcquisition       (PFDO_DATA FdoData, BOOLEAN   bRtMode);
int32  AbortAcquisition       (PFDO_DATA FdoData);
int32  AbortTransfer          (PFDO_DATA FdoData);
int32  StartTransferAtAddress (PFDO_DATA FdoData, uInt32 StartAddressOrSegmentNumber);
void   StartTransferDDRtoFIFO (PFDO_DATA FdoData, uInt32 Offset);
void   SetFifoMode            (PFDO_DATA FdoData, int32 Mode);
void   SetWatchDogTimer       (PFDO_DATA FdoData, uInt32 u32Delay_100us, BOOLEAN bWait);
void   StopWatchDogTimer      (PFDO_DATA FdoData);
void   ForceTrigger           (PFDO_DATA FdoData);
void   WriteGIO               (PFDO_DATA FdoData, uInt8 u8Addr, uInt32 u32Data);
uInt32 ReadGIO                (PFDO_DATA FdoData, uInt8 Addr);

void ClearInterrupts      (PFDO_DATA FdoData);
void EnableInterrupts     (PFDO_DATA FdoData, uInt32 IntType);
void DisableInterrupts    (PFDO_DATA FdoData, uInt32 IntType);
void ConfigureInterrupts  (PFDO_DATA FdoData, uInt32 u32IntrMask);

#endif   // NIOS_API_H_
