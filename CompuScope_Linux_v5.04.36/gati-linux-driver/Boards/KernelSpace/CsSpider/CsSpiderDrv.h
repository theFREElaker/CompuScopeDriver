
#if !defined( _CSSPIDERDRV_H_ )
#define _CSSPIDERDRV_H_

#include "GageDrv.h"
#include "CsPlxDefines.h"
#include "PlxDrvConsts.h"
#include "PlxSupport.h"
#include "AlteraSupport.h"


//----- ADD ON FLASH (EEPROM)
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


void  ControlSpiClock(PFDO_DATA	 FdoData, BOOLEAN bStart);
int32  SpiderAcquire( PFDO_DATA pMaster, Pin_STARTACQUISITION_PARAMS pAcqParams   );

#endif
