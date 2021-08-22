#ifndef __DEERE_FLASHOBJ_H__
#define __DEERE_FLASHOBJ_H__

#include "CsTypes.h"
#include "CsDefines.h"
#include "CsDeereFlash.h"

//----- STRUCTURE
typedef struct _FlashInfo {
	int8 *name;				// "Am29LV033C" for the 1st version of Combine on the base and add-on boards
	uInt32	addr;			// physical address, once translated 
	int32 areg;				// Can be set to zero for all parts 
	int32 nsect;			// # of sectors  
	int32 bank1start;		// first sector # in bank 1
	int32 bank2start;		// first sector # in bank 2
	uInt32	u32FlashSize;
	uInt32	u32SectorSize;

} DeereFlashInfo;

class CsPlxBase;

class CsDeereFlash
{
public:

	CsDeereFlash( CsPlxBase* CsCardObj );
	void	Initialize( CsPlxBase *pPlxBase );

	uInt8	GetFlashManufactoryCode();
	void	InitFlash(uInt32 FlashIndex);
	void	FlashCommand(uInt32 Command, uInt32 Sector, uInt32 Offset, uInt32 Data);
	uInt32  FlashStatus(uInt32 Addr);
	void	FlashSectorErase( uInt32 Sector);
	uInt32	ResetFlash(void);

	void	WriteFlashByte( uInt32 Addr, uInt32 Data );
	uInt8	FlashReadByte(uInt32 Addr);
	int32   FlashWriteEx(uInt32 u32StartAddr, void *pData, uInt32 u32Size, bool bBackupContent = true );

#ifdef _DEBUG
	void	Debug();
#endif

private:
	CsPlxBase	*m_PlxBase;
	DeereFlashInfo	*FlashPtr;


	void FlashClearBlockLockBits();
	void FlashSetBlockLockBit( uInt32 u32Sector );
	static DeereFlashInfo IntelFlashDesc[];

};

#endif
