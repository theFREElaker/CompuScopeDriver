
#ifndef decadedrv_h
#define decadedrv_h

#include "DecadeGageDrv.h"
#include "CsPlxDefines.h"
#include "DecadeAlteraSupport.h"

#define CSRB_ADDON_RESET  0x1    //----- bit0
#define CSRB_ADDON_FCE    0x2    //----- bit1
#define CSRB_ADDON_FOE    0x4    //----- bit2
#define CSRB_ADDON_FWE    0x8    //----- bit3
#define CSRB_ADDON_CONFIG 0x10   //----- bit4

#define CSRB_ADDON_FLASH_CMD      0x10
#define CSRB_ADDON_FLASH_ADD      0x11
#define CSRB_ADDON_FLASH_DATA     0x12
#define CSRB_ADDON_FLASH_ADDR_PTR 0x13
#define CSRB_ADDON_STATUS         0x14
#define CSRB_ADDON_FDV            0x15
#define CSRB_ADDON_HIO_TEST       0x20
#define CSRB_ADDON_SPI_TEST_WR    0x21
#define CSRB_ADDON_SPI_TEST_RD    0x22
#define CSRB_AD7450_WRITE_REG     0x88
#define CSRB_AD7450_READ_REG      0x89

#define CSRB_ADDON_STAT_FLASH_READY 0x1
#define CSRB_ADDON_STAT_CONFIG_DONE 0x2
#define CSRB_TCPLD_INFO             0x4


uInt32 AddonReadFlashByte(PFDO_DATA FdoData, uInt32 Addr);
void   AddonWriteFlashByte(PFDO_DATA FdoData, uInt32 Addr, uInt32 Data);
void   AddonResetFlash(PFDO_DATA FdoData);
int32  DecadeAcquire( PFDO_DATA pMaster, Pin_STARTACQUISITION_PARAMS pAcqParams );

#endif
