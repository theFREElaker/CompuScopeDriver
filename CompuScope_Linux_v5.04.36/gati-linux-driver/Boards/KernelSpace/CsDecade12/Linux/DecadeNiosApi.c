
#include "CsErrors.h"
#include "DecadeGageDrv.h"
#include "DecadeNiosApi.h"
#include "CsPlxDefines.h"

#define DECADE_WD_TIMER_FREQ_100us		12500		// Watch dog unit count equal to 100 microsecond, one count is 8ns 
#define DECADE_WD_DEFAULT_TIMMEOUT		100000		// 10 sec in 100 micro second uinit
#define WATCH_DOG_EXPIRED_MASK 0x00002000

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32   ReadRegisterFromNios (PFDO_DATA FdoData, uInt32 Data, uInt32 Command)
{
   uInt32 status1, timeout = NIOS_TIMEOUT;
   uInt32 DataRead;

   //----      6- Write ack_to_nios = 1 to say I'm done with the previous one, ready for the next
   WriteGageRegister (FdoData, MISC_REG, ACK_TO_NIOS);
   //----      2- send the read command in "command_to_nios" to the nios (read from spi)
   WriteGageRegister (FdoData, COMMAND_TO_NIOS, Command);
   //----      3- send the data to the nios ("data_write_to_nios")
   WriteGageRegister (FdoData, DATA_WRITE_TO_NIOS, Data);
   //----      3- write the exe_to_nios bit
   WriteGageRegister (FdoData, MISC_REG, 0);
   //----      3- write the exe_to_nios bit
   WriteGageRegister (FdoData, MISC_REG, EXE_TO_NIOS);
   //----      4- read the exe_done from_nios bit
   //while (! ((ReadGageRegister (MISC_REG)) & EXE_DONE_FROM_NIOS));
   timeout = NIOS_TIMEOUT;
   status1 = ReadGageRegister (FdoData, MISC_REG);
   while ((status1 & EXE_DONE_FROM_NIOS) == 0)
   {
      status1 = ReadGageRegister (FdoData, MISC_REG);
      timeout--;
      if (timeout == 0)
         break;
   }

   if ((status1 & PASS_FAIL_NIOS_BIS) == PASS_FAIL_NIOS_BIS)
   {
      DataRead = FAILED_NIOS_DATA;
   }
   else
   {
      //----      5- read the data from "data_read_from_nios"
      DataRead = ReadGageRegister (FdoData, DATA_FROM_NIOS_OFFSET);
   }

   //----      6- Write ack_to_nios = 1 to say I'm done with the previous one, ready for the next
   WriteGageRegister (FdoData, MISC_REG, ACK_TO_NIOS);

   return (DataRead);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   WriteRegisterThruNios (PFDO_DATA FdoData, uInt32 Data, uInt32 Command, int32 i32TimeOut_100us)
{

   return WriteRegisterThruNiosEx (FdoData, Data, Command,  i32TimeOut_100us, NULL);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   WriteRegisterThruNiosEx (PFDO_DATA FdoData, uInt32 Data, uInt32 Command, int32 i32TimeOut_100us, uInt32 *pu32NiosReturn)
{
   uInt32   u32Status;
   uInt32   u32TimeoutStatus;
   int32   i32Ret = CS_SUCCESS;
   uInt32   u32TimeOut_10ns = DECADE_WD_DEFAULT_TIMMEOUT;

   // Calculate Timeout
   if (i32TimeOut_100us > 0)
      u32TimeOut_10ns = (uInt32) 1000*i32TimeOut_100us;

	u32Status = ReadGageRegister(FdoData, MISC_REG);
	if ( (u32Status & EXE_DONE_FROM_NIOS) !=0 )
	{
		WriteGageRegister(FdoData, MISC_REG, ACK_TO_NIOS);
		WriteGageRegister(FdoData, MISC_REG, 0);
	}
	SetWatchDogTimer( FdoData, u32TimeOut_10ns, FALSE );

	//----		send the write command in "command_to_nios" to the nios (write to spi)
	WriteGageRegister(FdoData, COMMAND_TO_NIOS_OFFSET, Command);
	//----		send the data to the nios ("data_write_to_nios")
	WriteGageRegister(FdoData, DATA_WRITE_TO_NIOS_OFFSET, Data);
	//----		write the "exe_to_nios"
	WriteGageRegister(FdoData, MISC_REG, 0);	// Should be a pulse
	WriteGageRegister(FdoData, MISC_REG, EXE_TO_NIOS);


	do 
	{
		u32Status = ReadGageRegister(FdoData, MISC_REG_OFFSET);
		u32TimeoutStatus = ReadGageRegister(FdoData, GEN_CMD_OFFSET);
		if ( WATCH_DOG_EXPIRED_MASK & u32TimeoutStatus )
		{
			i32Ret = CS_FRM_NO_RESPONSE;
			break;
		}
	}
	while ((u32Status & EXE_DONE_FROM_NIOS)==0);

	if ( (u32Status & PASS_FAIL_NIOS_BIS) == PASS_FAIL_NIOS_BIS )
	{
		i32Ret = CS_INVALID_FRM_CMD;
	}

	if ( CS_SUCCESS == i32Ret && 0 != pu32NiosReturn  )
		*pu32NiosReturn = ReadGageRegister(FdoData, DATA_FROM_NIOS_OFFSET);

	//----		Write ack_to_nios = 1 to say I'm done with the previous one, ready for the next
	WriteGageRegister(FdoData, MISC_REG_OFFSET, ACK_TO_NIOS);
	WriteGageRegister(FdoData, MISC_REG, 0);
   

   return i32Ret;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOLEAN IsNiosInit (PFDO_DATA FdoData)
{
   int32  i32Retry = 1000;
   uInt32 u32DimmSize = ReadRegisterFromNios (FdoData, 0, CSPLXBASE_READ_MCFG_CMD);

   while (FAILED_NIOS_DATA == u32DimmSize)
   {
      u32DimmSize = ReadRegisterFromNios (FdoData, 0, CSPLXBASE_READ_MCFG_CMD);
      if (i32Retry-- <= 0)
      {
         return FALSE;   // timeout !!!
      }
   }
   return TRUE;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  StartAcquisition (PFDO_DATA FdoData, BOOLEAN   bRtMode)
{
   int32   i32Status = CS_SUCCESS;

   if (bRtMode)
   {
      uInt32 u32Data = ReadGageRegister (FdoData, GEN_COMMAND_R_REG);
//      u32Data |= (0x6 << 8);
      u32Data = (0x6 << 8);

      WriteGageRegister (FdoData, GEN_COMMAND_R_REG, u32Data);
      i32Status = SEND_NIOS_COMMAND (FdoData, 0, CSPLXBASE_START_RTACQ);
   }
   else
      i32Status = SEND_NIOS_COMMAND (FdoData, 0, CSPLXBASE_START_ACQ_CMD);

   return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   AbortAcquisition (PFDO_DATA FdoData)
{
   return SEND_NIOS_COMMAND (FdoData, 0, CSPLXBASE_ABORT_ACQ_CMD);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   AbortTransfer (PFDO_DATA FdoData)
{
   return SEND_NIOS_COMMAND (FdoData, 0, CSPLXBASE_ABORT_READ_CMD);
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   StartTransferAtAddress (PFDO_DATA FdoData, uInt32 StartAddressOrSegmentNumber)
{
   return SEND_NIOS_COMMAND (FdoData, StartAddressOrSegmentNumber - 1, CSPLXBASE_SET_SEG_NUM_CMD);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void   StartTransferDDRtoFIFO ( PFDO_DATA FdoData, uInt32 Offset)
{
   SEND_NIOS_COMMAND (FdoData, Offset,CSPLXBASE_READ_SEG_OFFSET_CMD);//---- set the DDR burst to FIFO, i.e. start reading from Offset
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void   SetFifoMode (PFDO_DATA FdoData, int32 Mode)
{
   WriteGageRegister (FdoData, MODESEL, MSEL_CLR | Mode);
   WriteGageRegister (FdoData, MODESEL, (~MSEL_CLR) & Mode);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void SetWatchDogTimer (PFDO_DATA FdoData, uInt32 u32Delay_100us, BOOLEAN bWait)
{
   uInt32 u32Delay = u32Delay_100us * DECADE_WD_TIMER_FREQ_100us;

   if (0 == u32Delay_100us)
      return;

   WriteGageRegister (FdoData, WDTIMER_OFFSET, u32Delay);
   WriteGageRegister (FdoData, GEN_CMD_OFFSET, WATCH_DOG_STOP_MASK);
   WriteGageRegister (FdoData, GEN_CMD_OFFSET, WATCH_DOG_RUN_MASK);
   WriteGageRegister (FdoData, GEN_CMD_OFFSET, 0);

   if (bWait)
      while (0 == (ReadGageRegister (FdoData, GEN_CMD_OFFSET) & WATCH_DOG_EXPIRED_MASK));
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void StopWatchDogTimer (PFDO_DATA FdoData)
{
   WriteGageRegister (FdoData, GEN_CMD_OFFSET, WATCH_DOG_STOP_MASK);
   WriteGageRegister (FdoData, GEN_CMD_OFFSET, 0);
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void   ForceTrigger (PFDO_DATA FdoData)
{
   WriteGageRegister (FdoData, GEN_CMD_OFFSET, SW_FORCED_TRIGGER);
   WriteGageRegister (FdoData, GEN_CMD_OFFSET, 0);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void   WriteGIO (PFDO_DATA   FdoData, uInt8 u8Addr, uInt32 u32Data)
{
   uInt32 Status = 1;

   WriteGageRegister (FdoData, GIO_DATA_OFFSET, u32Data);
   WriteGageRegister (FdoData, GIO_CMD_OFFSET, ((u8Addr << 8)& GIO_HOST_LOC_ADD_MASK) | GIO_HOST_LOC_REQ_MASK | GIO_HOST_LOC_RW_MASK);

   SetWatchDogTimer (FdoData, 0x80000, FALSE);

   while (! ((ReadGageRegister (FdoData, GEN_CMD_OFFSET)) & GIO_HOST_LOC_ACK_MASK))
   {
      if ((ReadGageRegister (FdoData, GEN_CMD_OFFSET)) & WATCH_DOG_EXPIRED_MASK)
      {
         Status = 0;      // timeout
         break;
      }
   }

   WriteGageRegister (FdoData, GIO_CMD_OFFSET, 0);
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
uInt32  ReadGIO (PFDO_DATA   FdoData, uInt8 Addr)
{
   uInt32 Data = 0;

   WriteGageRegister (FdoData, GIO_CMD_OFFSET, ((Addr << 8)& GIO_HOST_LOC_ADD_MASK) | GIO_HOST_LOC_REQ_MASK);
   SetWatchDogTimer (FdoData, 0x80000, FALSE);

   while (! ((ReadGageRegister (FdoData, GEN_CMD_OFFSET)) & GIO_HOST_LOC_ACK_MASK))
   {
      if ((ReadGageRegister (FdoData, GEN_CMD_OFFSET)) & WATCH_DOG_EXPIRED_MASK)
      {
         // u32Status = TIME OUT
         break;
      }
   }

   WriteGageRegister (FdoData, GIO_CMD_OFFSET, 0);
   Data = ReadGageRegister (FdoData, GIO_DATA_OFFSET);

   return Data;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void ClearInterrupts (PFDO_DATA   FdoData)
{
   /* GEN_DEBUG_PRINT (("ClearInterrupts\n")); */
   //----- Clear INT_CFG => disable all the interrupts
   WriteGageRegister (FdoData, INT_CFG, 0);
   FdoData->u32IntBaseMask = 0;

   //----- Clear INT_FIFO and INT_COUNT
   WriteGageRegister (FdoData, INTR_CLEAR, INT_CLR);
   WriteGageRegister (FdoData, INTR_CLEAR, 0);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void EnableInterrupts (PFDO_DATA FdoData, uInt32 IntType)
{
   FdoData->u32IntBaseMask |= IntType;
   WriteGageRegister (FdoData, INT_CFG, FdoData->u32IntBaseMask);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void DisableInterrupts (PFDO_DATA FdoData, uInt32 IntType)
{
   /* GEN_DEBUG_PRINT (("DisableInterrupts 0x%x\n", IntType)); */
   FdoData->u32IntBaseMask &= ~IntType;
   WriteGageRegister (FdoData, INT_CFG, FdoData->u32IntBaseMask);
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void ConfigureInterrupts (PFDO_DATA FdoData, uInt32 u32IntrMask)
{
   /* GEN_DEBUG_PRINT (("ConfigureInterrupts 0x%x\n", u32IntrMask)); */
   FdoData->u32IntBaseMask = 0;
   EnableInterrupts (FdoData, u32IntrMask);
}

