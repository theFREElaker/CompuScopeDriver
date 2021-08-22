
#include "GageDrv.h"
#include "NiosApi.h"
#include "CsErrors.h"
#include "CsPlxDefines.h"


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32	ReadRegisterFromNios(PFDO_DATA FdoData, uInt32 Data, uInt32 Command)
{
	uInt32 status1, timeout = NIOS_TIMEOUT;
	uInt32 DataRead;

#ifdef _WIN32
//	KIRQL			kIrql;

	//KeAcquireSpinLock( &m_kNiosCmdSpinLock, &kIrql );
#endif

	//----		6- Write ack_to_nios = 1 to say I'm done with the previous one, ready for the next
	WriteGageRegister(FdoData, MISC_REG, ACK_TO_NIOS);

	//----		2- send the read command in "command_to_nios" to the nios (read from spi)
	WriteGageRegister(FdoData, COMMAND_TO_NIOS, Command);
	//----		3- send the data to the nios ("data_write_to_nios")
	WriteGageRegister(FdoData, DATA_WRITE_TO_NIOS, Data);
	//----		3- write the exe_to_nios bit
	WriteGageRegister(FdoData, MISC_REG, EXE_TO_NIOS);
	//----		4- read the exe_done from_nios bit
	//while (!( (ReadGageRegister(MISC_REG)) & EXE_DONE_FROM_NIOS));
	timeout = NIOS_TIMEOUT;
	status1 = ReadGageRegister(FdoData, MISC_REG);
	while ((status1 & EXE_DONE_FROM_NIOS) == 0)
	{
		status1 = ReadGageRegister(FdoData, MISC_REG);
		timeout--;
		if (timeout == 0)
			break;
	}

	if ( (status1 & PASS_FAIL_NIOS_BIS) == PASS_FAIL_NIOS_BIS )
	{
		DataRead = FAILED_NIOS_DATA;
	}
	else
	{
		//----		5- read the data from "data_read_from_nios"
		DataRead = ReadGageRegister(FdoData, DATA_READ_FROM_NIOS);
	}

	//----		6- Write ack_to_nios = 1 to say I'm done with the previous one, ready for the next
	WriteGageRegister(FdoData, MISC_REG, ACK_TO_NIOS);

#ifdef _WIN32
//	KeReleaseSpinLock( &m_kNiosCmdSpinLock, kIrql );
#endif

	return (DataRead);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	WriteRegisterThruNios(PFDO_DATA FdoData, uInt32 Data, uInt32 Command )
{

	return WriteRegisterThruNiosEx( FdoData, Data, Command, NULL );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	WriteRegisterThruNiosEx(PFDO_DATA FdoData, uInt32 Data, uInt32 Command, uInt32 *pu32NiosReturn )
{
	uInt32 status1, timeout = NIOS_TIMEOUT;
	int32  i32Ret = 1;//CS_SUCCESS;

#ifdef _WIN32
//	KIRQL			kIrql;

//	KeAcquireSpinLock( &m_kNiosCmdSpinLock, &kIrql );
#endif

	//----		2- send the write command in "command_to_nios" to the nios (write to spi)
	WriteGageRegister(FdoData, COMMAND_TO_NIOS, Command);
	//----		3- send the data to the nios ("data_write_to_nios")
	WriteGageRegister(FdoData, DATA_WRITE_TO_NIOS, Data);
	//----		4- write the "exe_to_nios"
	WriteGageRegister(FdoData, MISC_REG, EXE_TO_NIOS);

	//----		1- read the "exe_done from_nios" bit
	//while (!( (ReadGageRegister(MISC_REG)) & EXE_DONE_FROM_NIOS));
	status1 = ReadGageRegister(FdoData, MISC_REG);
	while ((status1 & EXE_DONE_FROM_NIOS)==0)
	{
		status1 = ReadGageRegister(FdoData, MISC_REG);
		timeout--;
		if (timeout == 0)
		{
			i32Ret = CS_FRM_NO_RESPONSE;
			break;
		}
	}

	if ( (status1 & PASS_FAIL_NIOS_BIS) == PASS_FAIL_NIOS_BIS )
	{
		i32Ret = CS_INVALID_FRM_CMD;
	}

	if ( pu32NiosReturn )
		*pu32NiosReturn = ReadGageRegister(FdoData, DATA_READ_FROM_NIOS);

	//----		5- Write ack_to_nios = 1 to say I'm done with the previous one, ready for the next
	WriteGageRegister(FdoData, MISC_REG, ACK_TO_NIOS);

#ifdef _WIN32
//	KeReleaseSpinLock( &m_kNiosCmdSpinLock, kIrql );
#endif

	return i32Ret;
}



#ifdef PLXBASE
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOLEAN    IsNiosInit( PFDO_DATA FdoData )
{
	int			i;
	uInt32		u32Status;
	uInt32		u32NiosStatus = 0;
	int32		i32Retry = 1000;

	// Wait for initialization of Nios
	for (  i = 0; i < 8; i ++ )
	{
//		KeDelayExecutionThread();

		u32Status = ReadFlashRegister(FdoData, STATUS_REG_PLX);
		if(ERROR_FLAG != (u32Status & ERROR_FLAG) )
			break;
	}

	u32Status = ReadFlashRegister(FdoData, STATUS_REG_PLX);
	if(ERROR_FLAG == (u32Status & ERROR_FLAG) )
	{
		//m_EventLog.ReportEventString( CS_ERROR_TEXT, "NiosInit error: STATUS_REG_PLX error" );
	}
	else
	{
		uInt32 u32DimmSize = ReadRegisterFromNios(FdoData, 0, CSPLXBASE_READ_MCFG_CMD);
		while ( FAILED_NIOS_DATA == u32DimmSize )
		{
			i32Retry --;
			u32DimmSize = ReadRegisterFromNios(FdoData, 0, CSPLXBASE_READ_MCFG_CMD);
			if ( i32Retry <= 0 )
			{
//					m_EventLog.ReportEventString( CS_ERROR_TEXT, "NiosInit error: timeout" );
				return FALSE;	// timeout !!!
			}
		}
		u32NiosStatus = 1;
	}

	return (BOOLEAN) (u32NiosStatus);

}


#else
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOLEAN    IsNiosInit( PFDO_DATA FdoData )
{
	int32   i32Retry = 1000;
	uInt32 u32DimmSize = ReadRegisterFromNios( FdoData, 0, CSPLXBASE_READ_MCFG_CMD);

	while ( FAILED_NIOS_DATA == u32DimmSize )
	{
		u32DimmSize = ReadRegisterFromNios( FdoData, 0, CSPLXBASE_READ_MCFG_CMD);
		if ( i32Retry-- <= 0 )
		{
			return FALSE;	// timeout !!!
		}
	}
	return TRUE;
}

#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32  StartAcquisition( PFDO_DATA FdoData, BOOLEAN	bRtMode )
{
	int32	i32Status = CS_SUCCESS;

	if ( bRtMode )
	{
		uInt32 u32Data = ReadGageRegister( FdoData, GEN_COMMAND_R_REG );
//		u32Data |= (0x6 << 8);
		u32Data = (0x6 << 8);

		WriteGageRegister(FdoData, GEN_COMMAND_R_REG, u32Data);
		i32Status = WriteRegisterThruNios(FdoData, 0, CSPLXBASE_START_RTACQ);
	}
	else
		i32Status = WriteRegisterThruNios(FdoData, 0, CSPLXBASE_START_ACQ_CMD);

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	AbortAcquisition ( PFDO_DATA FdoData )
{
	return WriteRegisterThruNios(FdoData, 0, CSPLXBASE_ABORT_ACQ_CMD);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	AbortTransfer ( PFDO_DATA FdoData )
{
	return WriteRegisterThruNios(FdoData, 0, CSPLXBASE_ABORT_READ_CMD);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32	StartTransferAtAddress ( PFDO_DATA FdoData, uInt32 StartAddressOrSegmentNumber)
{
	return WriteRegisterThruNios(FdoData, StartAddressOrSegmentNumber - 1, CSPLXBASE_SET_SEG_NUM_CMD);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	StartTransferDDRtoFIFO (  PFDO_DATA FdoData, uInt32 Offset )
{
	WriteRegisterThruNios(FdoData, Offset,CSPLXBASE_READ_SEG_OFFSET_CMD);//---- set the DDR burst to FIFO, i.e. start reading from Offset
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void	SetFifoMode ( PFDO_DATA FdoData, int32 Mode)
{
	WriteGageRegister( FdoData, MODESEL, MSEL_CLR | Mode);
	WriteGageRegister( FdoData, MODESEL, (~MSEL_CLR) & Mode);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void BBtiming(PFDO_DATA FdoData, uInt32 u32Delay_100us)
{
	uInt32 u32Delay = u32Delay_100us * WD_TIMER_FREQ_100us;

	if( 0 == u32Delay_100us )
		return;
	WriteGageRegister(FdoData, WD_TIMER, u32Delay);
	WriteGageRegister(FdoData, GEN_COMMAND, WATCH_DOG_STOP_MASK);
	WriteGageRegister(FdoData, GEN_COMMAND, WATCH_DOG_RUN_MASK);
	WriteGageRegister(FdoData, GEN_COMMAND, 0);

	while( 0 == (ReadGageRegister( FdoData, GEN_COMMAND ) & WD_TIMER_EXPIRED) );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void	ForceTrigger( PFDO_DATA FdoData )
{
	WriteGageRegister(FdoData, GEN_COMMAND, SW_FORCED_TRIGGER);
	WriteGageRegister(FdoData, GEN_COMMAND, 0);
}