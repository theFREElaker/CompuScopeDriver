
int32 FlashWriteValue(PFDO_DATA FdoData, uInt32 u32Addr, uInt32 u32Val );
int32 FlashReadValue(PFDO_DATA FdoData, uInt32 u32Addr, uInt16 *pu16Data );
int32 FlashProramWordP(PFDO_DATA FdoData, uInt32 u32Addr, uInt32 u32Val );
int32 FlashProgramWord32(PFDO_DATA FdoData, uInt32 u32Addr, uInt32 u32SectorAddr, void *pBuffer, uInt32 u32BufferSize );
void  HardResetFlash(PFDO_DATA FdoData);
uInt16	ReadFlashData(PFDO_DATA FdoData);
int32	FlashSectorErase(PFDO_DATA FdoData, uInt32 u32Sector);

int NucleonFlashRead( PFDO_DATA FdoData,  uInt32 u32Addr, void *pBuffer,  uInt32 u32BufferSize);
int NucleonFlashWrite( PFDO_DATA FdoData,  uInt32 u32Sector, uInt32 u32Offset, void *pBuffer,  uInt32 u32BufferSize);