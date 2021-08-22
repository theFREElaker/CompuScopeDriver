
#if !defined( _CSSPLENDADRV_H_ )
#define _CSSPLENDADRV_H_

#include "GageDrv.h"
#include "CsPlxDefines.h"
#include "PlxDrvConsts.h"
#include "PlxSupport.h"
#include "AlteraSupport.h"


//----- ADD ON FLASH (EEPROM)
#define CSPDR_ADDON_FLASH_CMD		0x10		//---- (nios point of view address 0x10)
//---- details of the command register
#define CSPDR_ADDON_RESET		0x1			//----- bit0
#define	CSPDR_ADDON_FCE			0x2			//----- bit1
#define CSPDR_ADDON_FOE			0x4			//----- bit2
#define CSPDR_ADDON_FWE			0x8			//----- bit3
#define CSPDR_ADDON_CONFIG		0x10		//----- bit4

#define CSPDR_ADDON_FLASH_CMD		 0x10
#define CSPDR_ADDON_FLASH_ADD		 0x11
#define CSPDR_ADDON_FLASH_DATA		 0x12
#define CSPDR_ADDON_FLASH_ADDR_PTR	 0x13
#define CSPDR_ADDON_STATUS			 0x14
#define CSPDR_ADDON_FDV				 0x15
#define CSPDR_AD7450_WRITE_REG	     0X1E 
#define CSPDR_AD7450_READ_REG	     0X1F 
#define CSPDR_ADDON_HIO_TEST		 0x20	
#define CSPDR_ADDON_SPI_TEST_WR      0x21
#define CSPDR_ADDON_SPI_TEST_RD      0x22

#define	CSPDR_ADDON_STAT_FLASH_READY		0x1
#define	CSPDR_ADDON_STAT_CONFIG_DONE		0x2
#define CSPDR_TCPLD_INFO					0x4

//Spi clock control
#define CSPDR_START_SPI_CLK				0x3C

#define CSPLNDA_NCONFIG				0x10000 //(UINT)(1<<16)
#define CSPLNDA_CFG_CHIP_SELECT		0x20000 //(UINT)(1<<17)
#define CSPLNDA_CFG_WRITE_SELECT	0x40000 //(UINT)(1<<18)
#define CSPLNDA_CFG_ENABLE			0x80000 //(UINT)(1<<19)

#define CSPLNDA_CFG_DONE	0x2000000 //(UINT)(1<<25)
#define CSPLNDA_CFG_STATUS	0x4000000 //(UINT)(1<<26)
#define CSPLNDA_CFG_BUSY	0x8000000 //(UINT)(1<<27)


uInt32	AddonReadFlashByte(PFDO_DATA FdoData, uInt32 Addr);
void	AddonWriteFlashByte(PFDO_DATA FdoData, uInt32 Addr, uInt32 Data);
void	AddonResetFlash(PFDO_DATA FdoData);
int32	ProgramAddonFpga(PFDO_DATA FdoData, uInt8 *pBuffer, uInt32 u32ImageSize);
int32	WriteFpgaString(PFDO_DATA FdoData, unsigned char *buffer, int numbytes);
int32	SplendaAcquire( PFDO_DATA pMaster, Pin_STARTACQUISITION_PARAMS pAcqParams );


#endif
