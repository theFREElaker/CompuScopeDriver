#ifndef decadeflash_h
#define decadeflash_h

#include "CsTypes.h"

int32 FlashWriteValue    (PFDO_DATA FdoData, uInt32 u32Addr, uInt32 u32Val);
int32 FlashReadValue     (PFDO_DATA FdoData, uInt32 u32Addr, uInt16 *pu16Data);
int32 FlashProgramBuffer (PFDO_DATA FdoData, uInt32 u32Addr, void *pBuffer, uInt32 u32BufferSize);
void  HardResetFlash     (PFDO_DATA FdoData);

uInt32 ReadFlashData    (PFDO_DATA FdoData);
uInt32 ReadFlashData    (PFDO_DATA FdoData);
int32  FlashSectorErase (PFDO_DATA FdoData, uInt32 u32Sector);

int32 DecadeFlashRead        (PFDO_DATA FdoData,  uInt32 u32Addr, void *pBuffer,  uInt32 u32BufferSize);
int32 DecadeFlashWriteSector (PFDO_DATA FdoData,  uInt32 u32Addr, uInt32 u32Offset, void *pBuffer,  uInt32 u32BufferSize);
void  TestFlash              (PFDO_DATA FdoData);

#endif
