
#include "StdAfx.h"
#include "CsDevIoCtl.h"
#include "CsIoctl.h"
#include "CsPrivateStruct.h"
#include "CsDrvDefines.h"
#include "GageWrapper.h"
#include "CsPlxDefines.h"

#ifndef _WINDOWS_
#include <sys/mman.h>
#endif

#define STM_USE_ASYNC_REQUEST
#define ALT_DMA_MAX_SIZE         4096*254

#ifdef _WINDOWS_
void  ThreadAsyncSerialDataTransfer( LPVOID  lpParams );
void  ThreadWaitForAsyncTransferComplete( LPVOID  lpParams );
#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CsDevIoCtl::CsDevIoCtl() :m_hDriver(0), m_BoardHandle(0), m_bAsyncReadMemory(false), m_bAbortReadMemory(false)
#ifdef _WINDOWS_
                          ,m_hAsyncTxEvent(0)
#endif
{
   GageZeroMemoryX(m_Overlaped, sizeof(m_Overlaped));
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CsDevIoCtl::~CsDevIoCtl()
{
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void   CsDevIoCtl::InitializeDevIoCtl( PCSDEVICE_INFO   pDevInfo )
{
   GageCopyMemoryX( &m_DevInfo, pDevInfo, sizeof(m_DevInfo) );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::InitializeDevIoCtl()
{
   char szDriverName[MAX_STR_LENGTH];

#ifdef _WINDOWS_
   // Build the driver name for CreateFile
   strcpy_s(szDriverName , sizeof(szDriverName), "\\\\.\\");
   strcat_s(szDriverName, sizeof(szDriverName), m_DevInfo.SymbolicLink);

   // Temporary Notification event for each block
   m_hAsyncTxEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
   if ( NULL == m_hAsyncTxEvent )
   {
      return CS_CREATEEVENT_ERROR;
   }

   // Open kernel driver by using its device symbolic link
   m_hDriver = CreateFile ((LPCSTR)szDriverName,
                           GENERIC_READ|GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           0,
                           (void *)NULL);

   if ( m_hDriver == (HANDLE) -1  )
   {
      m_hDriver = NULL;
      CloseHandle( m_hAsyncTxEvent );
      return CS_NO_SYSTEMS_FOUND;
   }


   // Open kernel driver by using its device symbolic link
   m_hAsyncDriver = CreateFile ((LPCSTR)szDriverName,
                                GENERIC_READ|GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_OVERLAPPED,
                                (void *)NULL);


   if ( m_hAsyncDriver == (HANDLE) -1  )
   {
      m_hDriver = m_hAsyncDriver = NULL;
      CloseHandle( m_hDriver );
      CloseHandle( m_hAsyncTxEvent );
      return CS_NO_SYSTEMS_FOUND;
   }
#else   // __linux__

   // Build the driver name for CreateFile
   strcpy_s(szDriverName, sizeof(szDriverName), _T("/proc/driver/"));
   strcat(szDriverName, m_DevInfo.SymbolicLink);
   m_hDriver = open (szDriverName, O_RDWR);

   if ( m_hDriver == (FILE_HANDLE) -1  )
   {
      m_hDriver = 0;
      return CS_NO_SYSTEMS_FOUND;
   }

   //   DEBUG !!!!!!!!!!
   // Just for now while do not know how to handle async Io in linux
   //
   m_hAsyncDriver = 0;

#endif

   m_BoardHandle = m_DevInfo.DrvHdl;

   return CS_SUCCESS;
};


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void   CsDevIoCtl::UnInitializeDevIoCtl()
{
#ifdef _WINDOWS_
   if ( m_hAsyncTxEvent )
   {
      CloseHandle(m_hAsyncTxEvent);
      m_hAsyncTxEvent = 0;
   }

   if ( m_hDriver )
   {
      CloseHandle(m_hDriver);
      m_hDriver = NULL;
   }

   if ( m_hAsyncDriver )
   {
      CloseHandle(m_hAsyncDriver);
      m_hAsyncDriver = NULL;
   }
#else

   if (m_MapAddr && m_MapLength)
   {
      if( munmap (m_MapAddr, m_MapLength)==-1)
      {
         fprintf( stdout, "munmap (m_MapAddr %p m_MapLength %ld ) failed! \n", 
            m_MapAddr, m_MapLength );
      }
      m_MapAddr = NULL;
      m_MapLength = 0;
   }
   
   if ( m_hDriver )
   {
      close(m_hDriver);
      m_hDriver = 0;
   }

   if ( m_hAsyncDriver )
   {
      close(m_hAsyncDriver);
      m_hAsyncDriver = 0;
   }

#endif

   m_BoardHandle = 0;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

PCS_OVERLAPPED CsDevIoCtl::GetFreeOverlapped()
{
   for(int i = 0; i < MAX_OVERLAPPED; i++)
   {
      if ( FALSE == m_Overlaped[i].bBusy )
      {
         memset(&m_Overlaped[i], 0, sizeof(CS_OVERLAPPED));
         m_Overlaped[i].bBusy = TRUE;

         return &m_Overlaped[i];
      }
   }

   return NULL;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void CsDevIoCtl::SetFreeOverlapped( PCS_OVERLAPPED pCsOverlap )
{
   if ( NULL == pCsOverlap )
      pCsOverlap = m_pCurrentOverlaped;

   for(int i = 0; i < MAX_OVERLAPPED; i++)
   {
      if ( pCsOverlap == &m_Overlaped[i] )
      {
         m_Overlaped[i].bBusy = FALSE;
         return;
      }
   }
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDevIoCtl::WritePciRegister(uInt32 u32Offset, uInt32 u32RegVal)
{
   return GenericWriteRegister( CS_IOCTL_WRITE_PCI_REGISTER, u32Offset, u32RegVal);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CsDevIoCtl::ReadPciRegister(uInt32 u32Offset)
{
   return GenericReadRegister( CS_IOCTL_READ_PCI_REGISTER, u32Offset );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDevIoCtl::WriteGageRegister(uInt32 u32Offset, uInt32 u32RegVal)
{
   return GenericWriteRegister( CS_IOCTL_WRITE_GAGE_REGISTER, u32Offset, u32RegVal);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CsDevIoCtl::ReadGageRegister(uInt32 u32Offset)
{
   return GenericReadRegister( CS_IOCTL_READ_GAGE_REGISTER, u32Offset );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDevIoCtl::WriteFlashRegister(uInt32 u32Offset, uInt32 u32RegVal)
{
   return GenericWriteRegister( CS_IOCTL_WRITE_FLASH_REGISTER, u32Offset, u32RegVal);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CsDevIoCtl::ReadFlashRegister(uInt32 u32Offset)
{
   return GenericReadRegister( CS_IOCTL_READ_FLASH_REGISTER, u32Offset );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDevIoCtl::WriteGIO(uInt32 u32Offset, uInt32 u32RegVal)
{
   return GenericWriteRegister( CS_IOCTL_WRITE_GIO, u32Offset, u32RegVal);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CsDevIoCtl::ReadGIO(uInt32 u32Offset)
{
   return GenericReadRegister( CS_IOCTL_READ_GIO, u32Offset );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDevIoCtl::WriteGioCpld(uInt32 u32Offset, uInt32 u32RegVal)
{
   return GenericWriteRegister( CS_IOCTL_WRITE_GIO_CPLD, u32Offset, u32RegVal);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CsDevIoCtl::ReadGioCpld(uInt32 u32Offset)
{
   return GenericReadRegister( CS_IOCTL_READ_GIO_CPLD, u32Offset );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDevIoCtl::WriteCpldRegister(uInt32 u32Offset, uInt32 u32RegVal)
{
	return GenericWriteRegister( CS_IOCTL_WRITE_CPLD_REG, u32Offset, u32RegVal);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CsDevIoCtl::ReadCpldRegister(uInt32 u32Offset)
{
	return GenericReadRegister( CS_IOCTL_READ_CPLD_REG, u32Offset );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDevIoCtl::WriteByteToFlash(uInt32 u32Offset, uInt32 u32RegVal)
{
   return GenericWriteRegister( CS_IOCTL_WRITEBYTE_TOFLASH, u32Offset, u32RegVal);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CsDevIoCtl::ReadByteFromFlash(uInt32 u32Offset)
{
   return GenericReadRegister( CS_IOCTL_READBYTE_FROMFLASH, u32Offset );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDevIoCtl::AddonWriteByteToFlash(uInt32 u32Offset, uInt32 u32RegVal)
{
   return GenericWriteRegister( CS_IOCTL_ADDON_WRITEBYTE_TOFLASH, u32Offset, u32RegVal);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CsDevIoCtl::AddonReadByteFromFlash(uInt32 u32Offset)
{
   return GenericReadRegister( CS_IOCTL_ADDON_READBYTE_FROMFLASH, u32Offset );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDevIoCtl::GenericWriteRegister( uInt32 u32IoCtlCode, uInt32 u32Offset, uInt32 u32RegVal)
{
   uInt32   u32RetCode;
   CS_IOCTL_STATUS         CsIoctlStatus;
   uInt32               u32ByteRead = 0;

#ifdef _WINDOWS_
   _in_WRITE_REGISTER      RegWriteStr;
#else
   io_READWRITE_PCI_REG   RegWriteStr;
#endif

   RegWriteStr.u32Offset   = u32Offset;
   RegWriteStr.u32RegVal   = u32RegVal;

#ifdef _WINDOWS_

   u32RetCode = DeviceIoControl( m_hDriver,
                                 u32IoCtlCode,
                                 &RegWriteStr,
                                 sizeof(RegWriteStr),
                                 &CsIoctlStatus,
                                 sizeof(CsIoctlStatus),
                                 &u32ByteRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "GENRIC WRITE REGISTER fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
   _ASSERT( u32ByteRead == sizeof(CsIoctlStatus) );

   return CsIoctlStatus.i32Status;

#else

   u32RetCode = ioctl( m_hDriver, u32IoCtlCode, &RegWriteStr);

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "GENRIC WRITE REGISTER fails.\n");
      return CS_DEVIOCTL_ERROR;
   }

   return CS_SUCCESS;

#endif


}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CsDevIoCtl::GenericReadRegister(uInt32 u32IoCtlCode, uInt32 u32Offset)
{
   uInt32   u32RetCode;

#ifdef _WINDOWS_
   in_READ_REGISTER      inReadStr;
   out_READ_REGISTER      outReadStr;

   inReadStr.u32Offset      = u32Offset;

   u32RetCode = DeviceIoControl( m_hDriver,
                                 u32IoCtlCode,
                                 &inReadStr,
                                 sizeof(inReadStr),
                                 &outReadStr,
                                 sizeof(outReadStr),
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "GENERIC READ REGISTER fails. (WinError = %x )\n", u32RetCode );
      return (uInt32)(-1);
   }

   _ASSERT( m_BytesRead == sizeof(outReadStr) );

   if ( CS_SUCCEEDED( outReadStr.i32Status ) )
      return outReadStr.u32RegVal;
   else
      return (uInt32)(-1);
#else

   io_READWRITE_PCI_REG   RegWriteStr;
   CS_IOCTL_STATUS         CsIoctlStatus;
   uInt32               u32ByteRead = 0;

   RegWriteStr.u32Offset   = u32Offset;

   u32RetCode = ioctl( m_hDriver, u32IoCtlCode, &RegWriteStr);

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "GENRIC WRITE REGISTER fails.\n" );
      return (uInt32)(-1);
   }

   return RegWriteStr.u32RegVal;

#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDevIoCtl::WriteRegisterThruNios(uInt32 u32Data, uInt32 u32Command, int32 i32Timeout_100us, uInt32 *pu32NiosReturn )
{
   uInt32   u32RetCode;

#ifdef _WINDOWS_
   in_READWRITE_NIOS_REGISTER  RegWriteStr;
   out_READ_REGISTER           outReadStr;

   RegWriteStr.u32Data      = u32Data;
   RegWriteStr.u32Command   = u32Command;
   RegWriteStr.i32Timeout   = i32Timeout_100us;

   u32RetCode = DeviceIoControl (m_hDriver,
                                 pu32NiosReturn ? CS_IOCTL_WRITE_NIOS_REGISTER_EX : CS_IOCTL_WRITE_NIOS_REGISTER,
                                 &RegWriteStr,
                                 sizeof(RegWriteStr),
                                 &outReadStr,
                                 sizeof(outReadStr),
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_WRITE_NIOS_REGISTER fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
   _ASSERT( m_BytesRead == sizeof(outReadStr) );

   if (pu32NiosReturn)
      *pu32NiosReturn = outReadStr.u32RegVal;

   return outReadStr.i32Status;
#else
   io_READWRITE_NIOS_REGISTER RegWriteStr;

   RegWriteStr.u32Data      = u32Data;
   RegWriteStr.u32Command   = u32Command;

   u32RetCode = ioctl (m_hDriver,
                       pu32NiosReturn ? CS_IOCTL_WRITE_NIOS_REGISTER_EX : CS_IOCTL_WRITE_NIOS_REGISTER,
                       &RegWriteStr);

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "IOCTL_WRITE_NIOS_REGISTER fails.\n" );
      return CS_DEVIOCTL_ERROR;
   }

   if (pu32NiosReturn)
      *pu32NiosReturn = RegWriteStr.u32RetVal;

   return CS_SUCCESS;
#endif

}

//-----------------------------------------------------------------------------
// Fw Bugs !!!!!
// Patch for the problem of firmware
// This function perform read Nios register in a slow way, ie a lots of transition
// User Space - Kernel. 
// In some cases if we read by using the fast way ( all code in kernel ) it does not work properly.
//-----------------------------------------------------------------------------
uInt32 CsDevIoCtl::ReadRegisterFromNios_Slow(uInt32 u32Data, uInt32 u32Command, uInt32 u32TimeoutMs /* = 10000*/  )
{
	uInt32 status1;
	uInt32 DataRead;
#ifdef _WINDOWS_
	LARGE_INTEGER	StartTime;
	LARGE_INTEGER	CurrentTime;
	LARGE_INTEGER	EslapseTime;
	LARGE_INTEGER	TimeFrequency;
	double			EslapseMs;

	QueryPerformanceCounter( &StartTime );
	QueryPerformanceFrequency( &TimeFrequency );
#endif


	//----		6- Write ack_to_nios = 1 to say I'm done with the previous one, ready for the next
	WriteGageRegister(MISC_REG, ACK_TO_NIOS);

	//----		2- send the read command in "command_to_nios" to the nios (read from spi)
	WriteGageRegister(COMMAND_TO_NIOS, u32Command);
	//----		3- send the data to the nios ("data_write_to_nios")
	WriteGageRegister(DATA_WRITE_TO_NIOS, u32Data);
	//----		3- write the exe_to_nios bit
	WriteGageRegister(MISC_REG, EXE_TO_NIOS);
	//----		4- read the exe_done from_nios bit
	//while (!( (ReadGageRegister(MISC_REG)) & EXE_DONE_FROM_NIOS));
	status1 = ReadGageRegister(MISC_REG);
	while ((status1 & EXE_DONE_FROM_NIOS) == 0)
	{
		status1 = ReadGageRegister(MISC_REG);

#ifdef _WINDOWS_
		QueryPerformanceCounter( &CurrentTime );
		EslapseTime.QuadPart = CurrentTime.QuadPart - StartTime.QuadPart;
		EslapseMs = 1000.0*EslapseTime.QuadPart/TimeFrequency.QuadPart;

		if ( EslapseMs > 1.0*u32TimeoutMs )
			break;
#endif
	}

	if ( (status1 & PASS_FAIL_NIOS_BIS) == PASS_FAIL_NIOS_BIS )
	{
		DataRead = FAILED_NIOS_DATA;
	}
	else
	{
		//----		5- read the data from "data_read_from_nios"
		DataRead = ReadGageRegister(DATA_READ_FROM_NIOS);
	}

	//----		6- Write ack_to_nios = 1 to say I'm done with the previous one, ready for the next
	WriteGageRegister(MISC_REG, ACK_TO_NIOS);

	return (DataRead);

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
uInt32 CsDevIoCtl::ReadRegisterFromNios(uInt32 u32Data, uInt32 u32Command, int32 i32Timeout_100us )
{
   uInt32   u32RetCode;

#ifdef _WINDOWS_
   in_READWRITE_NIOS_REGISTER inReadStr;
   out_READ_REGISTER          outReadStr;

   inReadStr.u32Data      = u32Data;
   inReadStr.u32Command   = u32Command;
   inReadStr.i32Timeout   = i32Timeout_100us;


   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_READ_NIOS_REGISTER,
                                 &inReadStr,
                                 sizeof(inReadStr),
                                 &outReadStr,
                                 sizeof(outReadStr),
                                 &m_BytesRead,
                                 NULL );


   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_READ_NIOS_REGISTER fails. (WinError = %x )\n", u32RetCode );
      return 0;
   }
   _ASSERT( m_BytesRead == sizeof(out_READ_REGISTER) );

   if ( CS_SUCCEEDED( outReadStr.i32Status ) )
      return outReadStr.u32RegVal;
   else
      return 0;
#else

   io_READWRITE_NIOS_REGISTER      inReadStr;

   //inReadStr.CardHandle   = m_BoardHandle;
   inReadStr.u32Data      = u32Data;
   inReadStr.u32Command   = u32Command;

   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_READ_NIOS_REGISTER,
                       &inReadStr);

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "IOCTL_READ_NIOS_REGISTER fails.\n" );
      return 0;
   }

   if ( CS_SUCCEEDED( inReadStr.i32Status ) )
      return inReadStr.u32RetVal;
   else
      return 0;
#endif
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::ReadPciConfigSpace(PVOID pBuffer, uInt32 u32ByteCount, uInt32 u32Offset )
{
   uInt32   u32RetCode;

#ifdef _WINDOWS_
   in_READ_PCI_CONFIG      InParams = {0};

   InParams.u32Offset      = u32Offset;
   InParams.u32ByteCount   = u32ByteCount;

   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_READ_PCI_CONFIG_SPACE,
                                 &InParams,
                                 sizeof(InParams),
                                 pBuffer,
                                 u32ByteCount,
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR || (m_BytesRead != u32ByteCount)  )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );
      TRACE( DRV_TRACE_LEVEL, "IOCTL_READ_PCI_CONFIG_SPACE fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   _ASSERT( m_BytesRead == u32ByteCount );
#else
   io_READWRITE_PCI_CONFIG      InParams = {0};
   InParams.u32Offset      = u32Offset;
   InParams.u32ByteCount   = u32ByteCount;
   InParams.ConfigBuffer    = pBuffer;


   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_READ_PCI_CONFIG_SPACE,
                       &InParams);

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "IOCTL_READ_PCI_CONFIG_SPACE fails.\n" );
      return CS_DEVIOCTL_ERROR;
   }
#endif

   return CS_SUCCESS;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::WritePciConfigSpace( PVOID pBuffer, uInt32 u32ByteCount, uInt32 u32Offset )
{
   uInt32   u32RetCode;

#ifdef _WINDOWS_
   in_WRITE_PCI_CONFIG      InParams = {0};

   InParams.u32Offset      = u32Offset;
   InParams.ConfigBuffer   = pBuffer;
   InParams.u32ByteCount   = u32ByteCount;

   u32RetCode = DeviceIoControl (m_hDriver,
                                 CS_IOCTL_WRITE_PCI_CONFIG_SPACE,
                                 &InParams,
                                 sizeof(InParams),
                                 NULL,
                                 0,
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_WRITE_PCI_CONFIG_SPACE fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
#else
   io_READWRITE_PCI_CONFIG   InParams = {0};

   InParams.u32Offset      = u32Offset;
   InParams.ConfigBuffer   = pBuffer;
   InParams.u32ByteCount   = u32ByteCount;

   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_WRITE_PCI_CONFIG_SPACE,
                       &InParams);

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "IOCTL_WRITE_PCI_CONFIG_SPACE fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
#endif

   return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::RegisterEventHandle( EVENT_HANDLE hHamdle, uInt32 u32EventType )
{
   uInt32   u32RetCode;
   in_REGISTER_EVENT_HANDLE   InStruct = {0};

#ifdef _WINDOWS_
   CS_IOCTL_STATUS            OutStruct;

   InStruct.u32EventType      = u32EventType;
   InStruct.bExternalEvent      = FALSE;         // Event created by driver
   InStruct.hUserEventHandle   = hHamdle;

   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_REGISTER_EVENT_HANDLE,
                                 &InStruct,
                                 sizeof(InStruct),
                                 &OutStruct,
                                 sizeof(OutStruct),
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_REGISTER_EVENT_HANDLE fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   return OutStruct.i32Status;
#else
   InStruct.u32EventType      = u32EventType;
   InStruct.bExternalEvent      = (BOOLEAN) FALSE;         // Event created by driver
   InStruct.hUserEventHandle   = hHamdle;

   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_REGISTER_EVENT_HANDLE,
                       &InStruct);

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "IOCTL_REGISTER_EVENT_HANDLE fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   return CS_SUCCESS;
#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDevIoCtl::TransferDataFromOnBoardMemory( PVOID pBuffer, int64 i64ByteCount, EVENT_HANDLE hAsyncEvent, uInt32 *pu32TransferCount )
{
   uInt32   u32RetCode = 0;
   uInt32   u32TempTransferCount = 0;

   uInt32   u32TempBufferSize;
   
#ifndef _WINDOWS_
      // Linux has issue when DMA block followed not page align. 
      // ALT_DMA_MAX_SIZE is max amount of memory that hardware can transfer as a single block.
      // workaround by align the end of first block. 
      uInt32 offset = (size_t)pBuffer & 0xFFF;
      if ((offset) && ( (i64ByteCount + offset) > ALT_DMA_MAX_SIZE) )
      {
         u32TempBufferSize = ALT_DMA_MAX_SIZE - offset;
         u32RetCode = CardReadMemory(pBuffer, u32TempBufferSize, hAsyncEvent, &u32TempTransferCount);
         
         pBuffer = (char *)pBuffer + u32TempBufferSize;

         if(CS_FAILED(u32RetCode))
            return u32RetCode;
         i64ByteCount -= u32TempBufferSize;
      }
#endif      
   
   while(i64ByteCount > 0x3FFFF000)
   {
      u32TempBufferSize = 0x3FFFF000;
      u32RetCode = CardReadMemory(pBuffer, u32TempBufferSize, hAsyncEvent, &u32TempTransferCount);

      if ( pu32TransferCount )
         *pu32TransferCount += u32TempTransferCount;
      pBuffer = (char *)pBuffer + u32TempBufferSize;

      if(CS_FAILED(u32RetCode))
         return u32RetCode;
      i64ByteCount -= u32TempBufferSize;
   }
   return CardReadMemory(pBuffer, (uInt32)i64ByteCount, hAsyncEvent, pu32TransferCount);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::CardReadMemory( PVOID pBuffer, uInt32 u32ByteCount, EVENT_HANDLE hAsyncEvent, uInt32 *pu32TransferCount )
{
   uInt32   u32RetCode = 0;

	PCS_OVERLAPPED 	pCsOverlapped = 0;

   m_bAsyncReadMemory = (hAsyncEvent != 0);
   m_u32TransferCount = 0;

#ifdef _WINDOWS_
	if ( hAsyncEvent )
	{
		uInt32	u32ThreadId;

		pCsOverlapped = GetFreeOverlapped();

		if ( ! pCsOverlapped )
			return CS_OVERLAPPED_ERROR;

		// Reset the abort flag
		m_bAbortReadMemory = false;

		pCsOverlapped->hUserNotifyevent		= hAsyncEvent;
		pCsOverlapped->OverlapStruct.hEvent = m_hAsyncTxEvent;
		pCsOverlapped->hUserNotifyevent		= hAsyncEvent;
		pCsOverlapped->pUserBuffer			= pBuffer;
		pCsOverlapped->u32UserBufferSize	= u32ByteCount;

		// Create the thread waiting for asynchronous data transfer completed
		pCsOverlapped->hThrWaitAsyncTxComplete = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CsDevIoCtl::ThreadWaitForAsyncTransferComplete,
														(LPVOID)(pCsOverlapped), 0, &u32ThreadId);

		if ( ! pCsOverlapped->hThrWaitAsyncTxComplete )
		{
			SetFreeOverlapped( pCsOverlapped );
			return CS_MISC_ERROR;
		}
	}

	int32 i32Status = TransferDataKernel( pBuffer, u32ByteCount, pCsOverlapped );
	if ( pu32TransferCount )
		*pu32TransferCount = m_u32TransferCount;
	return i32Status;

#else

   /* For the moment transfer in chunks of no more than 4K * (256 - 2). Altera needs that. */
   int res, len = u32ByteCount;
   uInt8 *pbuf = (uInt8*)pBuffer;

   m_BytesRead = 0;
   while ( len > 0 )
   {
      int tlen = min(len, ALT_DMA_MAX_SIZE);  
      if ( (res = read( m_hDriver, pbuf, tlen )) <= 0 )
         break;

      pbuf += res;
      m_BytesRead += res;
      len -= res;
   }

   if (m_BytesRead == u32ByteCount)
      u32RetCode = 1;
   else
      u32RetCode = DEV_IOCTL_ERROR;

   if ( ! hAsyncEvent )
      _ASSERT( u32RetCode != DEV_IOCTL_ERROR );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
		TRACE( DRV_TRACE_LEVEL, "Read Data fails. (WinError = %lx )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   return CS_SUCCESS;

#endif

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlDrvStartAcquisition( Pin_STARTACQUISITION_PARAMS pAcqParams )
{
   int32   i32Status = CS_SUCCESS;
   uInt32   u32RetCode;

#ifdef _WINDOWS_
   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_START_ACQUISITION,
                                 pAcqParams,
                                 sizeof(in_STARTACQUISITION_PARAMS),
                                 &i32Status,
                                 sizeof(i32Status),
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_START_ACQUISITION fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   return i32Status;
#else
   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_START_ACQUISITION,
                       pAcqParams );

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "IOCTL_START_ACQUISITION fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
   return CS_SUCCESS;
#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlSetMasterSlaveLink( MASTER_SLAVE_LINK_INFO *SlaveInfoArray, uInt32 u32NumOfSlaves )
{
   uInt32         u32RetCode;
#ifdef _WINDOWS_
   CS_IOCTL_STATUS   IoCtlStatus;

   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_MASTERSLAVE_LINK,
                                 SlaveInfoArray,
                                 u32NumOfSlaves * sizeof(MASTER_SLAVE_LINK_INFO),
                                 &IoCtlStatus,
                                 sizeof(IoCtlStatus),
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_MASTERSLAVE_LINK fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   return IoCtlStatus.i32Status;
#else   //   __linux__
   MS_LINK_INFO   MsLinkInfo;
   void         *ioStruct = &MsLinkInfo;

   GageZeroMemoryX(&MsLinkInfo, sizeof(MsLinkInfo));
   if( u32NumOfSlaves > 0 )
   {
      MsLinkInfo.u32NumOfSlave = u32NumOfSlaves;
      GageCopyMemoryX( MsLinkInfo.MsLinkInfo, SlaveInfoArray, u32NumOfSlaves * sizeof(MASTER_SLAVE_LINK_INFO));
   }

   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_MASTERSLAVE_LINK,
                       ioStruct );

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "IOCTL_MASTERSLAVE_LINK fails. (error = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   return CS_SUCCESS;
#endif
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlSetMasterSlaveCalibInfo( int32   *i32AlignOffset, uInt32 u32NumOfSlaves )
{
   uInt32         u32RetCode;
#ifdef _WINDOWS_
   CS_IOCTL_STATUS   IoCtlStatus;

   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_MASTERSLAVE_CALIBINFO,
                                 i32AlignOffset,
                                 u32NumOfSlaves * sizeof(int32),
                                 &IoCtlStatus,
                                 sizeof(IoCtlStatus),
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_MASTERSLAVE_CALIBINFO fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   return IoCtlStatus.i32Status;
#else   // __linux__
   MS_CALIB_INFO   MsCalibInfo;
   void         *ioStruct = &MsCalibInfo;

   GageZeroMemoryX(&MsCalibInfo, sizeof(MsCalibInfo));
   if( u32NumOfSlaves > 0 )
   {
      MsCalibInfo.u32NumOfSlave = u32NumOfSlaves;
      GageCopyMemoryX( MsCalibInfo.MsAlignOffset, i32AlignOffset, u32NumOfSlaves * sizeof(int32));
   }

   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_MASTERSLAVE_CALIBINFO,
                       ioStruct );

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "IOCTL_MASTERSLAVE_CALIBINFO fails. (error = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   return CS_SUCCESS;
#endif
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlSetAcquisitionConfigEx( PCSACQUISITIONCONFIG_EX pAcqCfgEx )
{
   uInt32         u32RetCode;

#ifdef _WINDOWS_
   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_SET_ACQCONFIG,
                                 pAcqCfgEx,
                                 sizeof(CSACQUISITIONCONFIG_EX),
                                 NULL,
                                 0,
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_SET_ACQCONFIG fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
#else
   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_SET_ACQCONFIG,
                       pAcqCfgEx );

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "IOCTL_SET_ACQCONFIG fails. (error = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
#endif
   return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlPreDataTransfer( Pin_PREPARE_DATATRANSFER pInParams, Pout_PREPARE_DATATRANSFER pOutParams )
{
   uInt32         u32RetCode;

#ifdef   _WINDOWS_
   uInt32         u32OutputSize = 0;

   if ( pOutParams )
      u32OutputSize = sizeof(out_PREPARE_DATATRANSFER);

   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_PRE_DATATRANSFER,
                                 pInParams,
                                 sizeof(in_PREPARE_DATATRANSFER),
                                 pOutParams,
                                 u32OutputSize,
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );
		_ASSERT( m_BytesRead == u32OutputSize );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_DATA_TRANSFER fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
#else // __linux__

   io_PREPARE_DATATRANSFER      Temp;


   //   memset(&Temp, 0, sizeof(io_DATATRANSFER_PARAMS));
   CopyMemory( &Temp.InParams, pInParams, sizeof(in_PREPARE_DATATRANSFER) );

   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_PRE_DATATRANSFER,
                       &Temp );

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "IOCTL_PRE_DATATRANSFER fails. (error = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   CopyMemory( pOutParams, &Temp.OutParams, sizeof(out_PREPARE_DATATRANSFER) );


#endif

   return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlSetCardState( PCSCARD_STATE pCardState )
{
   uInt32         u32RetCode;

#ifdef _WINDOWS_
   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_SET_CARDSTATE,
                                 pCardState,
                                 sizeof(CSCARD_STATE),
                                 NULL,
                                 0,
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_SET_CARDSTATE fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
#else
   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_SET_CARDSTATE,
                       pCardState );

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "IOCTL_SET_CARDSTATE fails. (error = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
#endif
   return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlGetCardState( PCSCARD_STATE pCardState )
{
   uInt32         u32RetCode;

#ifdef _WINDOWS_
   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_GET_CARDSTATE,
                                 NULL,
                                 0,
                                 pCardState,
                                 sizeof(CSCARD_STATE),
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_GET_CARDSTATE fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
#else
 #if !defined( USE_SHARED_CARDSTATE )
   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_GET_CARDSTATE,
                       pCardState );

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "IOCTL_GET_CARDSTATE fails. (error = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
 #else
   return CS_DEVIOCTL_ERROR;
 #endif
   
#endif

   return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlGetCardStatePointer( PCSCARD_STATE *pCardState )
{
   uInt32         u32RetCode;

#ifdef _WINDOWS_
   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_GET_CARDSTATE_PTR,
                                 NULL,
                                 0,
                                 pCardState,
                                 sizeof(pCardState),
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_GET_CARDSTATE_PTR fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
#else
   uInt32 pageSize = (uInt32)sysconf(_SC_PAGESIZE);
   m_MapLength = (sizeof(CSCARD_STATE) + pageSize -1) & ~(pageSize -1); 
   
   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_GET_CARDSTATE_PTR,
                       0 );

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "CS_IOCTL_GET_CARDSTATE_PTR fails. (error = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
 
   m_MapAddr = mmap (NULL, m_MapLength, PROT_READ | PROT_WRITE , MAP_SHARED, m_hDriver, (off_t)(0));
//   fprintf( stdout, "mmap (m_hDriver %d m_MapLength %d  mode = 0x%x )return . (m_MapAddr = %lx )\n", 
//               m_hDriver, m_MapLength, PROT_READ | PROT_WRITE, m_MapAddr );
      
   if ((0 == m_MapAddr) || (MAP_FAILED == m_MapAddr)) 
   {
      uInt32 Error = GetLastError();
      TRACE( DRV_TRACE_LEVEL, "CS_IOCTL_GET_CARDSTATE_PTR mmap fails. (error = 0x%x )\n", Error );
      return CS_DEVIOCTL_ERROR;
   }
   *pCardState = (PCSCARD_STATE)m_MapAddr;
 
#endif

   return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlMapCardStatePointer( uInt32 CardIndex, uInt32 u32SizeInBytes, PVOID *pMapAddr )
{
   uInt32         u32RetCode;

#ifdef _WINDOWS_
   return CS_FUNCTION_NOT_SUPPORTED;
#else
   MAP_CARDSTATE_BUFFER CardStateParam = {0};

   CardStateParam.in.pVa = pMapAddr;
   CardStateParam.in.CardIndex = CardIndex;
   CardStateParam.in.u32BufferSize = u32SizeInBytes;   
   CardStateParam.in.u32ProcId = GetCurrentProcessId();
   
   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_MAP_CARDSTATE_BUFFER,
                       &CardStateParam );

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "CS_IOCTL_MAP_CARDSTATE_BUFFER fails. (error = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   return CS_SUCCESS;

#endif

}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlGetAsynDataTransferResult( CSTRANSFERDATARESULT *pTxResult )
{
   uInt32         u32RetCode;

#ifdef _WINDOWS_
   uInt32         u32OutputSize = sizeof(CSTRANSFERDATARESULT);

   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_GET_ASYNC_DATA_TRANSFER_RESULT,
                                 NULL,
                                 0,
                                 pTxResult,
                                 u32OutputSize,
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_GET_ASYNC_DATA_TRANSFER_RESULT fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
	_ASSERT( m_BytesRead == u32OutputSize );

   return CS_SUCCESS;
#else   // __linux__

   return CS_FUNCTION_NOT_SUPPORTED;

#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlReadFlash( uInt32 u32Addr, PVOID pBuffer, uInt32 u32ReadSize )
{
   uInt32            u32RetCode;
   READ_FLASH_STRUCT   ReadFlashStruct;

   ReadFlashStruct.u32Addr      = u32Addr;
   ReadFlashStruct.u32ReadSize = u32ReadSize;
   ReadFlashStruct.pBuffer      = pBuffer;

#ifdef _WINDOWS_
   CS_IOCTL_STATUS      CsIoctlStatus;
   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_FLASH_READ,
                                 &ReadFlashStruct,
                                 sizeof(ReadFlashStruct),
                                 &CsIoctlStatus,
                                 sizeof(CsIoctlStatus),
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_FLASH_READ fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
   return CsIoctlStatus.i32Status;
#else
   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_FLASH_READ,
                       &ReadFlashStruct );

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "IOCTL_FLASH_READ fails. (error = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
   return CS_SUCCESS;
#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlAddonReadFlash( uInt32 u32Addr, PVOID pBuffer, uInt32 u32ReadSize )
{
   uInt32            u32RetCode;
   READ_FLASH_STRUCT   ReadFlashStruct;

   ReadFlashStruct.u32Addr      = u32Addr;
   ReadFlashStruct.u32ReadSize = u32ReadSize;
   ReadFlashStruct.pBuffer      = pBuffer;

#ifdef _WINDOWS_
   CS_IOCTL_STATUS      CsIoctlStatus;
   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_ADDON_FLASH_READ,
                                 &ReadFlashStruct,
                                 sizeof(ReadFlashStruct),
                                 &CsIoctlStatus,
                                 sizeof(CsIoctlStatus),
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_ADDON_FLASH_READ fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
   return CsIoctlStatus.i32Status;
#else
   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_ADDON_FLASH_READ,
                       &ReadFlashStruct );

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "IOCTL_ADDON_FLASH_READ fails. (error = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
   return CS_SUCCESS;
#endif
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlWriteFlash( uInt32 u32Sector, uInt32 u32Offset, PVOID pBuffer, uInt32 u32WriteSize )
{
   uInt32            u32RetCode;
   WRITE_FLASH_STRUCT   WriteFlashStruct;

   WriteFlashStruct.u32Sector      = u32Sector;
   WriteFlashStruct.u32Offset      = u32Offset;
   WriteFlashStruct.u32WriteSize   = u32WriteSize;
   WriteFlashStruct.pBuffer      = pBuffer;

#ifdef _WINDOWS_
   CS_IOCTL_STATUS      CsIoctlStatus;
   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_FLASH_WRITE,
                                 &WriteFlashStruct,
                                 sizeof(WriteFlashStruct),
                                 &CsIoctlStatus,
                                 sizeof(CsIoctlStatus),
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_FLASH_WRITE fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
   return CsIoctlStatus.i32Status;
#else
   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_FLASH_WRITE,
                       &WriteFlashStruct );

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "IOCTL_FLASH_WRITE fails. (error = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
   return CS_SUCCESS;
#endif
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlSendFlashCommand( uInt32 u32Addr, uInt8 u8Value )
{
   uInt32               u32RetCode;
   RW_FLASH_BYTE_STRUCT   SendFlashCmd;

   SendFlashCmd.u32Addr      = u32Addr;
   SendFlashCmd.u8Value      = u8Value;

#ifdef _WINDOWS_
   CS_IOCTL_STATUS         CsIoctlStatus;
   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_SEND_FLASH_CMD,
                                 &SendFlashCmd,
                                 sizeof(SendFlashCmd),
                                 &CsIoctlStatus,
                                 sizeof(CsIoctlStatus),
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );
      u32RetCode = GetLastError();

      TRACE( DRV_TRACE_LEVEL, "IOCTL_FLASH_WRITE_BYTE fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
   return CsIoctlStatus.i32Status;
#else
#ifdef _BROKEN_LINUX_
   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_SEND_FLASH_CMD,
                       &SendFlashCmd );

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "IOCTL_FLASH_WRITE_BYTE fails. (error = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   return CS_SUCCESS;
#else
   return CS_FUNCTION_NOT_SUPPORTED;
#endif
#endif
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlAddonWriteFlash( uInt32 u32Sector, uInt32 u32Offset, PVOID pBuffer, uInt32 u32WriteSize )
{
   uInt32            u32RetCode;
   WRITE_FLASH_STRUCT   WriteFlashStruct;

   WriteFlashStruct.u32Sector      = u32Sector;
   WriteFlashStruct.u32Offset      = u32Offset;
   WriteFlashStruct.u32WriteSize   = u32WriteSize;
   WriteFlashStruct.pBuffer      = pBuffer;

#ifdef _WINDOWS_
   CS_IOCTL_STATUS      CsIoctlStatus;
   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_ADDON_FLASH_WRITE,
                                 &WriteFlashStruct,
                                 sizeof(WriteFlashStruct),
                                 &CsIoctlStatus,
                                 sizeof(CsIoctlStatus),
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_FLASH_WRITE fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
   return CsIoctlStatus.i32Status;
#else
   u32RetCode = ioctl( m_hDriver,
                       CS_IOCTL_ADDON_FLASH_WRITE,
                       &WriteFlashStruct );

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "IOCTL_FLASH_WRITE fails. (error = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   return CS_SUCCESS;
#endif
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlAbortAsyncTransfer()
{
   uInt32               u32RetCode;

#ifdef _WINDOWS_
   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_ABORT_TRANSFER,
                                 NULL,
                                 0,
                                 NULL,
                                 0,
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_ABORT_TRANSFER fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
#else
   u32RetCode = ioctl (m_hDriver,
                       CS_IOCTL_ABORT_TRANSFER,
                       0 );

   _ASSERT( u32RetCode >= 0 );
   if ( u32RetCode < 0 )
   {
      TRACE( DRV_TRACE_LEVEL, "CS_IOCTL_ABORT_TRANSFER fails. (error= %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }
#endif
   return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void   CsDevIoCtl::AbortCardReadMemory()
{
   if ( m_bAsyncReadMemory )
   {
      m_bAbortReadMemory = true;
      IoctlAbortAsyncTransfer();
#ifdef _WINDOWS_
      ::WaitForSingleObject( m_hSerialAsyncTransfer, 500 );
#endif
   }

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDevIoCtl::IoctlGetMiscState( PKERNEL_MISC_STATE pMiscState )
{
	uInt32			u32RetCode;

#ifdef _WINDOWS_
	u32RetCode = DeviceIoControl( m_hDriver,
					 CS_IOCTL_GET_KERNEL_MISC_STATE,
					 NULL,
					 0,
					 pMiscState,
					 sizeof(KERNEL_MISC_STATE),
					 &m_BytesRead,
					 NULL );

	if ( u32RetCode == DEV_IOCTL_ERROR )
	{
		u32RetCode = GetLastError();

		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );
		TRACE( DRV_TRACE_LEVEL, "IOCTL_KERNEL_MISC_STATE fails. (WinError = %x )\n", u32RetCode );
		return CS_DEVIOCTL_ERROR;
	}
#else
	u32RetCode = ioctl( m_hDriver,
					 CS_IOCTL_GET_KERNEL_MISC_STATE,
					 pMiscState );

	if ( u32RetCode < 0 )
	{
		u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

		TRACE( DRV_TRACE_LEVEL, "IOCTL_KERNEL_MISC_STATE fails. (LastError = %x )\n", u32RetCode );
		return CS_DEVIOCTL_ERROR;
	}

#endif

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDevIoCtl::IoctlSetMiscState( PKERNEL_MISC_STATE pMiscState )
{
	uInt32			u32RetCode;

#ifdef _WINDOWS_
	u32RetCode = DeviceIoControl( m_hDriver,
					 CS_IOCTL_SET_KERNEL_MISC_STATE,
					 pMiscState,
					 sizeof(KERNEL_MISC_STATE),
					 NULL,
					 0,
					 &m_BytesRead,
					 NULL );

	if ( u32RetCode == DEV_IOCTL_ERROR )
	{
		u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

		TRACE( DRV_TRACE_LEVEL, "IOCTL_KERNEL_MISC_STATE fails. (WinError = %x )\n", u32RetCode );
		return CS_DEVIOCTL_ERROR;
	}
#else
	u32RetCode = ioctl( m_hDriver,
					 CS_IOCTL_SET_KERNEL_MISC_STATE,
					 pMiscState );

	if ( u32RetCode < 0 )
	{
		u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

		TRACE( DRV_TRACE_LEVEL, "IOCTL_KERNEL_MISC_STATE fails. (LastError = %x )\n", u32RetCode );
		return CS_DEVIOCTL_ERROR;
	}

#endif

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlStmAllocateDmaBuffer( uInt32 u32SizeInBytes, PVOID *pVA )
{

   uInt32                 u32RetCode;
   ALLOCATE_DMA_BUFFER    DmaBuffer = {0};

   if (!pVA || !u32SizeInBytes)
      return CS_INVALID_PARAMETER;

   DmaBuffer.in.u32BufferSize = u32SizeInBytes;

#ifdef _WINDOWS_
   u32RetCode = DeviceIoControl (m_hDriver,
                                 CS_IOCTL_ALLOCATE_STM_DMA_BUFFER,
                                 &DmaBuffer.in,
                                 sizeof(DmaBuffer.in),
                                 &DmaBuffer.out,
                                 sizeof(DmaBuffer.out),
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_ALLOCATE_DMA_BUFFER fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   if ( CS_SUCCEEDED(DmaBuffer.out.i32Status) )
      *pVA = DmaBuffer.out.pVa;

   return DmaBuffer.out.i32Status;
#else
   u32RetCode = ioctl (m_hDriver,
                       CS_IOCTL_ALLOCATE_STM_DMA_BUFFER,
                       &DmaBuffer);
   if (u32RetCode != 0) {
      TRACE (DRV_TRACE_LEVEL, "CS_IOCTL_ALLOCATE_STM_DMA_BUFFER failed: 0x%x\n", errno );
      return CS_DEVIOCTL_ERROR;
   }
   else {
      void* voidp;
      voidp = mmap (NULL, u32SizeInBytes, PROT_READ, MAP_SHARED, m_hDriver, (off_t)(DmaBuffer.out.pVa));
      if ((0 == voidp) || (MAP_FAILED == voidp)) {
         FREE_DMA_BUFFER dmafree = {0};
         dmafree.in.pVa = DmaBuffer.out.pVa;
         dmafree.in.kaddr = true;
         ioctl (m_hDriver, CS_IOCTL_FREE_STM_DMA_BUFFER, &dmafree);
         return CS_DEVIOCTL_ERROR;
      }
      *pVA = voidp;
   }
   return CS_SUCCESS;
#endif
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDevIoCtl::IoctlStmFreeDmaBuffer (PVOID pVA)
{
   uInt32           u32RetCode;
   FREE_DMA_BUFFER  DmaBuffer = {0};

   DmaBuffer.in.pVa = pVA;

#ifdef _WINDOWS_
   u32RetCode = DeviceIoControl (m_hDriver,
                                 CS_IOCTL_FREE_STM_DMA_BUFFER,
                                 &DmaBuffer,
                                 sizeof(DmaBuffer),
                                 &DmaBuffer,
                                 sizeof(DmaBuffer),
                                 &m_BytesRead,
                                 NULL);

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

		TRACE( DRV_TRACE_LEVEL, "IOCTL_FREE_DMA_BUFFER fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   return DmaBuffer.out.i32Status;
#else
   u32RetCode = ioctl (m_hDriver,
                       CS_IOCTL_STM_DMA_BUFFER_SIZE,
                       &DmaBuffer);
   if (u32RetCode != 0) {
      TRACE( DRV_TRACE_LEVEL, "CS_IOCTL_STM_DMA_BUFFER_SIZE failed: 0x%x\n", errno );
      return CS_DEVIOCTL_ERROR;
   }
   if (munmap (pVA, DmaBuffer.out.i32Status) == -1) {
      TRACE( DRV_TRACE_LEVEL, "munmap failed");
   }
   DmaBuffer.in.pVa = pVA;
   u32RetCode = ioctl (m_hDriver,
                       CS_IOCTL_FREE_STM_DMA_BUFFER,
                       &DmaBuffer);
   if (u32RetCode != 0) {
      TRACE (DRV_TRACE_LEVEL, "CS_IOCTL_FREE_STM_DMA_BUFFER failed: 0x%x\n", errno );
      return CS_DEVIOCTL_ERROR;
   }
   return CS_SUCCESS;
#endif
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

#ifdef STM_USE_ASYNC_REQUEST

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlStmTransferDmaBuffer( uInt32 u32TransferSizeBytes, PVOID pBuffer )
{
   int32            i32Status = CS_SUCCESS;
   TRANSFER_DMA_BUFFER   Params = {0};
   BOOL            bRetVal = FALSE;
   BOOL            bErrorFound = FALSE;
   uInt32            u32ErrorCode = 0;

#ifdef _WINDOWS_
   __try
   {
      m_pCurrentOverlaped = GetFreeOverlapped();

      if ( ! m_pCurrentOverlaped )
      {
         i32Status =  CS_OVERLAPPED_ERROR;
         __leave;
      }

      ResetEvent( m_hAsyncTxEvent );
      m_pCurrentOverlaped->OverlapStruct.hEvent   = m_hAsyncTxEvent;

      Params.in.pBuffer         = pBuffer;
      Params.in.u32TransferSize   = u32TransferSizeBytes;

      bRetVal = DeviceIoControl( m_hAsyncDriver,
                                 CS_IOCTL_START_STM_DMA_BUFFER,
                                 &Params,
                                 sizeof(Params),
                                 &m_StmTransferOut,
                                 sizeof(m_StmTransferOut),
                                 &m_BytesRead,
                                 &m_pCurrentOverlaped->OverlapStruct );

      if ( ! bRetVal )
      {
         // This is an asynchronous request so we expected to receive the status ERROR_IO_PENDING
         u32ErrorCode = GetLastError();
         if ( ERROR_IO_PENDING == u32ErrorCode )
         {
            bRetVal = GetOverlappedResult( m_hAsyncDriver, &m_pCurrentOverlaped->OverlapStruct, &m_BytesRead, FALSE );
            if ( ! bRetVal )
            {
               // The request has not yet completed. Expect to recieve ERROR_IO_INCOMPLETE as error code
               u32ErrorCode = GetLastError();
               if ( ERROR_IO_INCOMPLETE != u32ErrorCode )
                  bErrorFound = TRUE;
            }
            else
            {
               // The request has been completed
               i32Status = m_StmTransferOut.i32Status;
               __leave;
            }
         }
         else
            bErrorFound = TRUE;
      }
   }
   __finally
   {
      if ( bErrorFound )
      {
         TRACE( DRV_TRACE_LEVEL, "IOCTL_TEST_DMA_BUFFER fails. (WinError = %x )\n", u32ErrorCode );
         i32Status = CS_DEVIOCTL_ERROR;
      }
   }

   return i32Status;
#else
   Params.in.pBuffer         = pBuffer;
   Params.in.u32TransferSize   = u32TransferSizeBytes;
   
   i32Status = ioctl (m_hDriver,
                       CS_IOCTL_START_STM_DMA_BUFFER,
                       &Params);
   if (i32Status != 0) {
      TRACE (DRV_TRACE_LEVEL, "CS_IOCTL_START_STM_DMA_BUFFER failed: 0x%x\n", errno );
      return CS_DEVIOCTL_ERROR;
   }
   return CS_SUCCESS;
   
#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32   CsDevIoCtl::IoctlStmWaitDmaComplete( uInt32 u32TimeoutMs, uInt32 *pu32ErrorFlag, bool *bDmaComplete )
{
   int32   i32Status = CS_SUCCESS;
   uInt32  u32RetCode;

   *bDmaComplete = false;

#ifdef _WINDOWS_
   u32RetCode = WaitForSingleObject( m_hAsyncTxEvent, u32TimeoutMs );

   if ( WAIT_TIMEOUT == u32RetCode )
      i32Status = CS_STM_TRANSFER_TIMEOUT;
   else if ( WAIT_OBJECT_0 == u32RetCode )
   {
      // Stream transfer has been completed. Get the status
      SetFreeOverlapped( m_pCurrentOverlaped );

      if ( pu32ErrorFlag )
         *pu32ErrorFlag   = m_StmTransferOut.u32ErrorFlag;

      // Mark DMA has complete
      *bDmaComplete = true;
      i32Status = m_StmTransferOut.i32Status;
   }
   else
   {
      u32RetCode = GetLastError();
      TRACE( DRV_TRACE_LEVEL, "WaitForSingleObject(WAIT_STREAM_DMA_DONE) fails. (WinError = %x )\n", u32RetCode );
      i32Status = CS_DEVIOCTL_ERROR;
   }

   return i32Status;
#else
   WAIT_STREAM_DMA_DONE dma_wait = {0};
   // Patch for the problem of timeout  = 0
   if ( 0 == u32TimeoutMs )
      u32TimeoutMs = 1;

   dma_wait.in.u32TimeoutMs = u32TimeoutMs;
   int ErrFlag = 0;

   u32RetCode = ioctl (m_hDriver,
                       CS_IOCTL_WAIT_STREAM_DMA_DONE,
                       &dma_wait);
   if (u32RetCode != 0) {
      switch (errno) {
         case ENOMEM: ErrFlag = STM_TRANSFER_ERROR_FIFOFULL;  break; 
         case ETIME : i32Status = CS_STM_TRANSFER_TIMEOUT; break;
         case EACCES: i32Status = CS_STM_TRANSFER_ABORTED; break;
         default    : i32Status = CS_DEVIOCTL_ERROR; break;
      }
      if (pu32ErrorFlag)
         *pu32ErrorFlag = ErrFlag;
   }
   if (i32Status == CS_SUCCESS)   
   {
      *bDmaComplete = true;
   }
   
   return i32Status;
#endif
}

#else  /* STM_USE_ASYNC_REQUEST */

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32   CsDevIoCtl::IoctlStmTransferDmaBuffer( uInt32 u32TransferSizeBytes, PVOID pBuffer )
{
   uInt32              u32RetCode;
   TRANSFER_DMA_BUFFER Params = {0};

#ifdef _WINDOWS_
   Params.in.pBuffer         = pBuffer;
   Params.in.u32TransferSize = u32TransferSizeBytes;

   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_TEST_DMA_BUFFER,
                                 &Params,
                                 sizeof(Params),
                                 &Params.out,
                                 sizeof(Params.out),
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_FREE_DMA_BUFFER fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   return Params.out.i32Status;
#else
   return CS_FUNCTION_NOT_SUPPORTED;
#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32   CsDevIoCtl::IoctlStmWaitDmaComplete( uInt32 u32TimeoutMs, uInt32 *pu32ErrorFlag, uInt32 *pu32ActualSize, uInt8 *pu8EndOfData )
{
   uInt32                u32RetCode;
   WAIT_STREAM_DMA_DONE  WaitDma = {0};

#ifdef _WINDOWS_
   WaitDma.in.u32TimeoutMs = u32TimeoutMs;

   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_WAIT_STREAM_DMA_DONE,
                                 &WaitDma,
                                 sizeof(WaitDma),
                                 &WaitDma,
                                 sizeof(WaitDma),
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_WAIT_STREAM_DMA_DONE fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   *pu32ErrorFlag   = WaitDma.out.u32ErrorFlag;
   *pu32ActualSize = WaitDma.out.u32ActualSize;
   *pu8EndOfData   = WaitDma.out.u8EndOfData;

   return WaitDma.out.i32Status;
#else
   return CS_FUNCTION_NOT_SUPPORTED;
#endif
}

#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32   CsDevIoCtl::IoctlStmSetStreamMode( int32   i32Mode )
{
   uInt32           u32RetCode;
   SET_STREAM_MODE  StmMode;

   StmMode.in.i32Mode = i32Mode;

#ifdef _WINDOWS_
   u32RetCode = DeviceIoControl( m_hDriver,
                                 CS_IOCTL_SET_STREAM_MODE,
                                 &StmMode,
                                 sizeof(StmMode),
                                 &StmMode,
                                 sizeof(StmMode),
                                 &m_BytesRead,
                                 NULL );

   if ( u32RetCode == DEV_IOCTL_ERROR )
   {
      u32RetCode = GetLastError();
		_ASSERT( u32RetCode != DEV_IOCTL_ERROR );

      TRACE( DRV_TRACE_LEVEL, "IOCTL_SET_STREAM_MODE fails. (WinError = %x )\n", u32RetCode );
      return CS_DEVIOCTL_ERROR;
   }

   return StmMode.out.i32Status;
#else
   u32RetCode = ioctl (m_hDriver,
                       CS_IOCTL_SET_STREAM_MODE,
                       &StmMode);
   if (u32RetCode != 0) {
      TRACE (DRV_TRACE_LEVEL, "CS_IOCTL_WAIT_STREAM_DMA_DONE failed: 0x%x\n", errno);
      return CS_DEVIOCTL_ERROR;
   }
   return CS_SUCCESS;
#endif
}


#ifdef _WINDOWS_
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDevIoCtl::TransferDataKernel( PVOID pBuffer, uInt32 u32ByteCount, PCS_OVERLAPPED pCsOverlapped )
{
	int32	i32Status = CS_SUCCESS;
	uInt32	u32RetCode = 0;
	uInt32	u32BytesRemain = u32ByteCount;
	uInt8	*pCuurentPointer = (uInt8 *) pBuffer;
	uInt32	u32CurrBuffSize = u32ByteCount;
	OVERLAPPED	*pOverlapped = NULL;
	HANDLE	hDriverHandle = m_hDriver;


	if ( pCsOverlapped )
	{
		pOverlapped = &pCsOverlapped->OverlapStruct;
		pCsOverlapped->hDriver = m_hAsyncDriver;
		hDriverHandle = m_hAsyncDriver;
	}

	while ( !m_bAbortReadMemory && 0 != u32BytesRemain )
	{
		if ( pCsOverlapped )
			pCsOverlapped->u32CurrentTransferSize	= u32CurrBuffSize;

		u32RetCode = ReadFile(hDriverHandle, pCuurentPointer, u32CurrBuffSize, &m_BytesRead, pOverlapped );

		if ( u32RetCode == DEV_IOCTL_ERROR )
		{
			uInt32 u32ErrorCode = GetLastError();

			if ( ERROR_IO_PENDING == u32ErrorCode )
				return CS_ASYNC_SUCCESS;
			else if ( u32ErrorCode >= ERROR_NO_SYSTEM_RESOURCES &&
				u32ErrorCode  <= ERROR_PAGEFILE_QUOTA )
			{
				// Not enough ressource for locking down the buffer. Usually because the buffer is too big
				// Retry with smaller buffer
				u32CurrBuffSize = u32CurrBuffSize/2;		// try with half of data
				continue;
			}
			else
			{
				// TRACE( DRV_TRACE_LEVEL, "ReadFile() fails. (WinError = %x )\n",  u32RetCode );
				i32Status = CS_DEVIOCTL_ERROR;
				break;
			}
		}
		else
		{
			if ( m_BytesRead != u32CurrBuffSize )
			{
				// Only happens if there is something wrong of data transfer and we did not complete DMA.
				i32Status = CS_TRANSFER_DATA_TIMEOUT;
				break;
			}
			u32BytesRemain = u32BytesRemain - u32CurrBuffSize;
			pCuurentPointer += u32CurrBuffSize;

			// Count the number of ReadFile request sent to kernel driver
			m_u32TransferCount ++;
			if ( u32CurrBuffSize > u32BytesRemain )
				u32CurrBuffSize = u32BytesRemain;
		}
	}

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void  CsDevIoCtl::ThreadWaitForAsyncTransferComplete( LPVOID  lpParams )
{
	CsDevIoCtl		*pDevIoCtl = (CsDevIoCtl *) lpParams;
	PCS_OVERLAPPED  pCsOverlap = pDevIoCtl->m_pCurrentOverlaped;
	uInt32			u32RetCode = WAIT_TIMEOUT;
	uInt32			u32BytesCount = 0;
	uInt8			*pBuffer = (uInt8 *)pCsOverlap->pUserBuffer;
	int64			i64RemainBufferSize = pCsOverlap->i64OriginalBufferSize;

	while (! pDevIoCtl->m_bAbortReadMemory && 0 != i64RemainBufferSize)
	{
		while(WAIT_OBJECT_0 != u32RetCode)
		{
			u32RetCode = ::WaitForSingleObject( pCsOverlap->OverlapStruct.hEvent, 1000 );
		}

		::GetOverlappedResult(pCsOverlap->hDriver, &pCsOverlap->OverlapStruct, &u32BytesCount, TRUE );

		if ( u32BytesCount != pCsOverlap->u32CurrentTransferSize )
		{
			// This happens only when there is error or DMA has been aborted
			// How to return error to application ?????????
			break;
		}

		pCsOverlap->i64CompletedTransferSize += u32BytesCount;
		i64RemainBufferSize = pCsOverlap->i64OriginalBufferSize - pCsOverlap->i64CompletedTransferSize;
		if ( i64RemainBufferSize > 0)
		{
			pBuffer += u32BytesCount;
			if ( pCsOverlap->u32CurrentTransferSize >= i64RemainBufferSize )
				pCsOverlap->u32CurrentTransferSize = (uInt32) i64RemainBufferSize;

			u32RetCode = ReadFile( pCsOverlap->hDriver, pBuffer, pCsOverlap->u32CurrentTransferSize, &u32BytesCount, &pCsOverlap->OverlapStruct );
			if ( u32RetCode == DEV_IOCTL_ERROR )
			{
				uInt32 u32ErrorCode = GetLastError();

				if ( ERROR_IO_PENDING == u32ErrorCode )
				{
					u32RetCode = WAIT_TIMEOUT;
					continue;
				}
	/*			else if ( u32ErrorCode >= ERROR_NO_SYSTEM_RESOURCES &&
					u32ErrorCode  <= ERROR_PAGEFILE_QUOTA )
				{
					// Not enough ressource for locking down the buffer. Usually because the buffer is too big
					// Retry with smaller buffer
					u32CurrBuffSize = u32CurrBuffSize/2;		// try with half of data
					continue;
				}
				else
				{
					// TRACE( DRV_TRACE_LEVEL, "ReadFile() fails. (WinError = %x )\n",  u32RetCode );
					i32Status = CS_DEVIOCTL_ERROR;
					break;
				}
			}
*/
			}
		}
	}

	::SetEvent( pCsOverlap->hUserNotifyevent );
	pCsOverlap->bBusy = FALSE;
}


#endif
