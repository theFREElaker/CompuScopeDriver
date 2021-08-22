#include "StdAfx.h"
#include "CsTypes.h"
#include "CsPlxDefines.h"
#include "CsNiosApi.h"


CsNiosApi::~CsNiosApi()
{

}

//------------------------
//----- SetTriggerTimeout (uInt32 TimeOut)
//----- BASE-BRD REGISTER
//----- CSPLXBASE_ABORT_ACQ_CMD
//------------------------
void	CsNiosApi:: SetTriggerTimeout (uInt32 u32TimeOut)
{
	WriteGageRegister(TRIG_TIMEOUT_REG, u32TimeOut);
}

//------------------------
//----- ForceTrigger
//----- BASE-BRD REGISTER
//----- Address ICI NAT: To be reviewed, the register will be on the base board to avoid sending a pulse
//-----
//------------------------
void	CsNiosApi:: ForceTrigger ()
{
	WriteGageRegister(GEN_COMMAND, SW_FORCED_TRIGGER);
	WriteGageRegister(GEN_COMMAND, 0);
}


//------------------------
//
//------------------------
void	CsNiosApi:: AddonReset()
{
	WriteGageRegister(GEN_COMMAND, ADDON_EXCLUSIVE_RESET);
	WriteGageRegister(GEN_COMMAND, 0);
}

//------------------------
//----- AbortAcquisition ()
//----- BASE-BRD REGISTER
//----- CSPLXBASE_ABORT_ACQ_CMD
//------------------------
int32	CsNiosApi:: AbortAcquisition ()
{
	return WriteRegisterThruNios(0, CSPLXBASE_ABORT_ACQ_CMD);
}

//------------------------
//----- AbortTransfer ()
//----- BASE-BRD REGISTER
//----- CsNiosApi_ABORT_READ_CMD
//------------------------
int32	CsNiosApi:: AbortTransfer ()
{
	return WriteRegisterThruNios(0, CSPLXBASE_ABORT_READ_CMD);
}

//------------------------
//----- StartTransferDDRtoFIFO
//----- BASE-BRD REGISTER
//----- CSPLXBASE_READ_SEG_OFFSET_CMD
//------------------------
void	CsNiosApi:: StartTransferDDRtoFIFO(uInt32 Offset)
{
	WriteRegisterThruNios(Offset,CSPLXBASE_READ_SEG_OFFSET_CMD);//---- set the DDR burst to FIFO, i.e. start reading from Offset
}

//------------------------
//----- SetFifoMode ()
//----- BASE-BRD REGISTER
//-----
//------------------------
void	CsNiosApi:: SetFifoMode (int32 Mode)
{
	WriteGageRegister(MODESEL, MSEL_CLR | Mode);
	WriteGageRegister(MODESEL, (~MSEL_CLR) & Mode);
}


//------------------------
//-----
//----- BASE-BRD REGISTER
//----- Address
//----- 15
//----- 14
//----- 13
//----- 12
//----- 11
//----- 10
//----- 9
//----- 8
//----- 7
//----- 6
//----- 5
//----- 4
//----- 3
//----- 2
//----- 1
//----- 0
//------------------------
uInt16 CsNiosApi::ReadTriggerStatus()
{
	uInt32	RegStatus;

	RegStatus = ReadGageRegister( NIOS_GEN_STATUS );

	// Check the value of the Acquisitions status byte
	if ( ((RegStatus >> 17 ) & 0xFF) > 4 )
		return TRIGGERED;
	else
		return WAIT_TRIGGER;
}

//------------------------
//-----
//----- BASE-BRD REGISTER
//----- Address
//----- 15
//----- 14
//----- 13
//----- 12
//----- 11
//----- 10
//----- 9
//----- 8
//----- 7
//----- 6
//----- 5
//----- 4
//----- 3
//----- 2
//----- 1
//----- 0
//------------------------
uInt16 CsNiosApi::ReadBusyStatus()
{
	uInt32	RegStatus;
	BOOLEAN	bStatusAcq, bStatusRecDone, bStatusAbortWR;

	// Check the status from Nios
	RegStatus = (ReadGageRegister(STATUS_OF_ACQ));

	bStatusAcq		= (BOOLEAN) (RegStatus & STATUS_ACQ_DONE);
	bStatusRecDone	= (BOOLEAN) (RegStatus & STATUS_REC_DONE);
	bStatusAbortWR	= (BOOLEAN) (RegStatus & STATUS_ABORT_WRITE);

	if ( bStatusAbortWR  || ( bStatusAcq && bStatusRecDone ) )
		return 0;
	else
		return STILL_BUSY;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsNiosApi:: ReadTriggerTimeStamp( uInt32 StartSegment, int64 *pBuffer, uInt32 BufferSize )
{
	int64	i64TimeStamp;

	for (uInt32 i = 0; i < BufferSize; i ++)
	{
		i64TimeStamp	= ReadRegisterFromNios( StartSegment, CSPLXBASE_READ_TIMESTAMP1_CMD );
		i64TimeStamp	<<= 32;
		i64TimeStamp	|= ReadRegisterFromNios( StartSegment++, CSPLXBASE_READ_TIMESTAMP0_CMD );
		i64TimeStamp	= i64TimeStamp >> 4;

		pBuffer[i] = i64TimeStamp;
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32  CsNiosApi::SendTimeStampMode(bool bMclk, bool bFreeRun)
{
	uInt32 u32Data = ReadGageRegister( TIMESTAMP_STATUS );


	if(bMclk)
		u32Data |= TS_MCLK_SRC;
	else
		u32Data &= ~TS_MCLK_SRC;


	if(bFreeRun)
		u32Data |= TS_FREE_RUN;
	else
		u32Data &= ~TS_FREE_RUN;


	WriteGageRegister(TIMESTAMP_STATUS, u32Data);

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32  CsNiosApi::ResetTimeStamp()
{
	uInt32 u32Data = ReadGageRegister( TIMESTAMP_STATUS );

	u32Data &= ~TS_RESET;
	WriteGageRegister(TIMESTAMP_STATUS, u32Data);

	u32Data |= TS_RESET;
	WriteGageRegister(TIMESTAMP_STATUS, u32Data);

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32  CsNiosApi::ReadRawMemorySize_Kb( uInt32	u32BaseBoardHwOptions )
{
	uInt32 u32DimmSize;		// in KBytes unit
	uInt32 u32Ret = ReadRegisterFromNios( 0, CSPLXBASE_READ_MCFG_CMD);

	u32BaseBoardHwOptions;

	// The value return from CSPLXBASE_READ_MCFG_CMD is in MBytes Total
	u32DimmSize  = ((u32Ret >> 24) & 0xff) * RANK_WIDTH;
	u32DimmSize  = u32DimmSize << ( ((u32Ret >> 24) & 0x3) ? 8 : 0 );

	u32DimmSize *= (u32Ret & PLX_2_SODIMMS) == PLX_2_SODIMMS ? 2:1;
	u32DimmSize *= (u32Ret & PLX_2_RANKS) == PLX_2_RANKS ? 2:1;

	// Convert to KBytes unit
	u32DimmSize *= 1024;

	return u32DimmSize;		// in KBytes unit
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	CsNiosApi:: SetMulrecAvgCount( uInt32 u32Count )
{
	return WriteRegisterThruNios( u32Count, CSPLXBASE_SET_MULREC_AVG_COUNT );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsNiosApi::BBtiming(uInt32 u32Delay_100us)
{
	if( 0 == u32Delay_100us )
		return;
	uInt32 u32Delay = u32Delay_100us * WD_TIMER_FREQ_100us;

	WriteGageRegister(WD_TIMER, u32Delay);

	WriteGageRegister(GEN_COMMAND, WATCH_DOG_STOP_MASK);
	WriteGageRegister(GEN_COMMAND, WATCH_DOG_RUN_MASK);
	WriteGageRegister(GEN_COMMAND, 0);

	do
	{
	}while( 0 == (ReadGageRegister( GEN_COMMAND ) & WD_TIMER_EXPIRED) );
}

