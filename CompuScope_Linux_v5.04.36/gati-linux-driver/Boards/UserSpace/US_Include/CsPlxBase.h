#ifndef __CSPLXBASE_H__

#define __CSPLXBASE_H__

#include "CsTypes.h"
#include "CsDevIoCtl.h"
#include "DrvCommonType.h"
#include "CsSharedMemory.h"
#include "CsDrvStructs.h"
#include "PlxDrvConsts.h"
#include "CsPrivateStruct.h"

#define PLXBASE_SHARENAME		_T("CsPlxBase")

#define	BBMAPREG_OFFSET			0x40000
#define MIN_SIZE_MAPPING_BAR_0	0x100
#define PCI_NUM_BARS	6

#define	MAX_DMA_DESCRITPOR		128

//----- CONSTANTS
//---- maximum number of sectors supported
#define MAXSECTORS  64
#define SECTOR_SIZE	0x10000

//---- Command codes for the flash_command routine
#define FLASH_SELECT			0       //---- no command; just perform the mapping
#define FLASH_RESET				1       //---- reset to read mode
#define FLASH_READ				1       //---- reset to read mode, by any other name
#define FLASH_AUTOSEL			2       //---- autoselect (fake Vid on pin 9)
#define FLASH_PROG				3       //---- program a word
#define FLASH_CHIP_ERASE		4       //---- chip erase
#define FLASH_SECTOR_ERASE		5       //---- sector erase
#define FLASH_ERASE_SUSPEND		6       //---- suspend sector erase
#define FLASH_ERASE_RESUME		7       //---- resume sector erase
#define FLASH_UNLOCK_BYP		8       //---- go into unlock bypass mode
#define FLASH_UNLOCK_BYP_PROG	9       //---- program a word using unlock bypass
#define FLASH_UNLOCK_BYP_RESET	10      //---- reset to read mode from unlock bypass mode
#define FLASH_LASTCMD			10      //---- used for parameter checking

//---- Return codes from flash_status
#define STATUS_READY			0       //---- ready for action
#define STATUS_FLASH_BUSY		1       //---- operation in progress
#define STATUS_ERASE_SUSP		2		//---- erase suspended
#define STATUS_FLASH_TIMEOUT	3       //---- operation timed out
#define STATUS_ERROR			4       //---- unclassified but unhappy status

//---- AMD's manufacturer ID
#define AMDPART   	0x01

//---- AMD device ID's
#define ID_AM29LV033C   0xA3

//---- Index for a given type in the table FlashInfo
#define AM29LV033C	0

//---- Retry count for command, could be specific for each command
#define COMMON_RETRY	15//----- CONSTANTS
//---- maximum number of sectors supported
#define PLXBASEFLASH_MAXSECTORS		64
#define PLXBASEFLASH_SECTORSIZE		0x10000
#define	PLXBASEFLASHSIZE			0x800000


typedef struct flashinfo
{
	 char *name;         /* "Am29DL800T", etc. */
	 unsigned long addr; /* physical address, once translated */
	 int nsect;          /* # of sectors -- 19 in LV, 22 in DL */
	 struct
	 {
		long size;           /* # of bytes in this sector */
		long base;           /* offset from beginning of device */
	 } sec[PLXBASEFLASH_MAXSECTORS];  /* per-sector info */

} FastBallFlashInfo;

//----- STRUCTURE
struct FlashInfo {
	int8 *name;				// "Am29LV033C" for the 1st version of Combine on the base and add-on boards
	uInt32	addr;			// physical address, once translated
	int32 areg;				// Can be set to zero for all parts
	int32 nsect;			// # of sectors
	int32 bank1start;		// first sector # in bank 1
	int32 bank2start;		// first sector # in bank 2
	uInt32	u32FlashSize;
	uInt32	u32SectorSize;

	struct	{
		uInt32 size;				// # of bytes in this sector
		uInt32 base;				// offset from beginning of device
		int32 bank;				// 1 or 2 for DL; 1 for LV
			} sec[PLXBASEFLASH_MAXSECTORS];  // per-sector info
};

class CsPlxBase : virtual public CsDevIoCtl
{
public:
	PLXBASE_DATA			*m_PlxData;

	CsPlxBase *GetPlxBaseObj() { return this; };
	CS_BOARD_NODE *GetBoardNode() {	return &m_PlxData->CsBoard; }
	INITSTATUS	*GetInitStatus() { return &m_PlxData->InitStatus; }

	int32	Initialized();
	uInt16	GetDeviceId();
	int32	CsReadPlxNvRAM( PPLX_NVRAM_IMAGE nvRAM_image );
	int32	CsWritePlxNvRam( PPLX_NVRAM_IMAGE nvRAM_image );

	void	WriteDeviceSpecificConfig( void *pBuffer, uInt32 u32Offset, uInt32 Count );
	void	ReadDeviceSpecificConfig( void *pBuffer, uInt32 u32Offset, uInt32 Count );

	unsigned int PlxVpdRead(unsigned short Offset);
	unsigned char PlxVpdWrite( unsigned short Offset, unsigned int VpdData);
	void	ClearInterrupts(void);
	void	EnableInterrupts(uInt32 IntType);
	void	DisableInterrupts(uInt32 IntType);
	void	EnableInterruptDMA0( BOOL bDmaDemandMode = FALSE );
	void	DisableInterruptDMA0( BOOL bDmaDemandMode = FALSE );
	void	ClearInterruptDMA0();
	bool	IsDmaInterrupt();
	void	AbortDmaTransfer();
	int32	BaseBoardConfigReset(uInt32 Addr);

	BOOL    IsNiosInit();
	uInt32	ReadRawMemorySize_Kb( uInt32	u32BaseBoardHwOptions );
	void	UpdateMaxMemorySize( uInt32	u32BaseBoardHardwareOptions, uInt32 u32SampleSize );

	int32	ReadBaseBoardHardwareInfoFromFlash( BOOLEAN bChecksum = TRUE );

	unsigned char  PlxInterruptEnable( BOOL bDmaIntr );
	unsigned char  PlxInterruptDisable( BOOL bDmaIntr );
	void  PlxPciBoardReset();

	//----- Read/Write the Base Board Flash
	void	WriteSectorsOfFlash(BaseBoardFlash32Mbit *Flash, uInt32 Addr, uInt32 Size);
	int32	WriteFlashEx(void* Flash, uInt32 Addr, uInt32 Size, FlashAccess flSequence = AccessAuto);
	int32	ReadFlash(void *Flash, uInt32 Addr, uInt32 Size);
	void	FlashReset();

private:
	//----- Basic Functions for the AMD flash
	FlashInfo	*FlashPtr;
	void	InitFlash(uInt32 FlashIndex);
	void	FlashCommand(uInt32 Command, uInt32 Sector, uInt32 Offset, uInt32 Data);
	uInt32	FlashStatus(uInt32 Addr);
	void	FlashWriteByte( uInt32 Addr, uInt32 Data );
	int32	WriteEntireFlash(BaseBoardFlash32Mbit *Flash);
	void	FlashChipErase();
	void	FlashSectorErase(uInt32 Sector);
	uInt32	ReadFlashByte(uInt32 Addr);
	void	SendFlashCmd(uInt32 Addr, uInt32 Data);
	uInt32	ResetFlash();
};




#endif // __CsPlxBase_H__
