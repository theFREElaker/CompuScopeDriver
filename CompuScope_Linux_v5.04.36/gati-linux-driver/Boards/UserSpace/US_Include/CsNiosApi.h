#ifndef __CsNiosApi_h__
#define __CsNiosApi_h__

#include "CsTypes.h"
#include "CsDrvConst.h"
#include "CsDrvStructs.h"
#include "CsDevIoCtl.h"

#define RANK_WIDTH					4
#define PLX_2_SODIMMS				0x1
#define PLX_2_RANKS					0x2


class	CsNiosApi : virtual public CsDevIoCtl
{
public:
	virtual	~CsNiosApi();

protected:
	//!----- "Base Board routines" :
	uInt32  ReadRawMemorySize_Kb( uInt32 u32BaseBoardHwOptions );

	void	ForceTrigger ();
	int32	AbortAcquisition ();
	int32	AbortTransfer ();
	void	StartTransferDDRtoFIFO(uInt32 Offset);
	void	SetFifoMode (int32 Mode);
	uInt16	ReadTriggerStatus();
	uInt16	ReadBusyStatus();
	void	AddonReset();

	int32	ReadTriggerTimeStamp( uInt32 StartSegment, int64 *pBuffer, uInt32 BufferSize );
	int32   SendTimeStampMode(bool bMclk = false, bool bFreeRun = true);
	int32   ResetTimeStamp();
	int32	SetMulrecAvgCount( uInt32 u32Count );

public:
	void	SetTriggerTimeout (uInt32 TimeOut = CS_TIMEOUT_DISABLE);

	void	BBtiming(uInt32 u32Delay_100us);

};


#endif
