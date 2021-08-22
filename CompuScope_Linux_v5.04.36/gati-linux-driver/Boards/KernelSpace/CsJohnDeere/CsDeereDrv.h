
#if !defined( _CSDEEREDRV_H_ )
#define _CSDEEREDRV_H_

#include "GageDrv.h"
#include "CsPlxDefines.h"
#include "PlxDrvConsts.h"
#include "PlxSupport.h"

#define DEERE_ALARM_CMD                 0x00800703	// addr = 0x07  Model/Cs = 03
#define DEERE_AN_FWR_CMD                0x00800F03	// addr = 0x0F  Model/Cs = 03


#define DEERE_ALARM_INVALID              0x8000
#define DEERE_ALARM_CLEAR                0x0001
#define DEERE_ALARM_PROTECT_CH1          0x0002
#define DEERE_ALARM_PROTECT_CH2          0x0004
#define DEERE_ALARM_CAL_PLL_UNLOCK       0x0008


int32	GetMsCalibOffset( PFDO_DATA FdoData );
uInt32	AddonReadFlashByte(PFDO_DATA FdoData, uInt32 Addr);
void	AddonWriteFlashByte(PFDO_DATA FdoData, uInt32 Addr, uInt32 Data);
void	AddonResetFlash(PFDO_DATA FdoData);


int32	DeereAcquire( PFDO_DATA	FdoData, Pin_STARTACQUISITION_PARAMS AcqParams );
int32	SetupPreDataTransferEx( PFDO_DATA	FdoData, uInt32 u32Segment, uInt32 u32StartAddr, uInt32 u32ReadCount, uInt32 u32SkipCount, Pin_PREPARE_DATATRANSFER pXferConfig );

void	GetAlarmStatus( PFDO_DATA	FdoData );
int32   ClearAlarmStatus( PFDO_DATA	FdoData, uInt16 u16AlarmSource );
int32   ReportAlarm( PFDO_DATA	FdoData );

#endif
