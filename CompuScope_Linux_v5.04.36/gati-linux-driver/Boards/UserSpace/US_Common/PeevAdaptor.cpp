#include "StdAfx.h"
#ifdef _WINDOWS_
#include <conio.h>
#endif
#include "PeevAdaptor.h"
#include "CsDrvDefines.h"
#include "CsIoctl.h"
#ifdef _WINDOWS_
#include "CsDevIoctl.h"
#endif
#include "CsIoctlTypes.h"

// Pararelle Port data
#define	REG_CSN			0
#define	REG_SI			1
#define	REG_RWN			2
#define	REG_STORE_CLK	3
#define	REG_SHIFT_CLK	4
#define	RLY_PWR_ON		5


PeevAdaptor::PeevAdaptor(): m_ParalellePort(0x378),
							m_u32CurrentConfig(0),
							m_bInitialized(FALSE)
{

}


PeevAdaptor::~PeevAdaptor()
{

}

int32	PeevAdaptor::Initialize( FILE_HANDLE hDevIoctl, char *szDriver )
{
	uInt32		u32DefaultConfig = 0xF00;
	ULONG		u32IoPort = 0x378;
	char		szRegistryKey[256];

#ifdef _WINDOWS_
	strcpy_s( szRegistryKey, sizeof(szRegistryKey), "SYSTEM\\CurrentControlSet\\Services\\" );
	strcat_s( szRegistryKey, sizeof(szRegistryKey), szDriver );
	strcat_s( szRegistryKey, sizeof(szRegistryKey), "\\Parameters" );

	HKEY hKey;
	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, szRegistryKey, 0, KEY_QUERY_VALUE, &hKey) )
	{
		DWORD	DataSize = sizeof(u32IoPort);

		::RegQueryValueEx( hKey, _T("PeevIoPort"), NULL, NULL, (LPBYTE)&u32IoPort, &DataSize );
		m_ParalellePort = (uInt16) u32IoPort;

		::RegCloseKey( hKey );
	}
#endif	

	m_hIoctlHandle = hDevIoctl;

	SetConfig( u32DefaultConfig, TRUE );
	m_bInitialized = TRUE;

	return CS_SUCCESS;
}

void	PeevAdaptor::Invalidate()
{
	if ( m_bInitialized )
	{
		// Power off
		PeevOutp( 0 );
		m_bInitialized = FALSE;
	}
}

#define	BUNNY_PEEV_CTRLBITS	16			// Number of controls bits in PVADAPTORCONFIG struct

int32	PeevAdaptor::SetConfig( uInt32 u32NewConfig, BOOLEAN bInit )
{
	if ( ! bInit && ! m_bInitialized )
		return CS_PEEVADAPTOR_NOT_INIT;

	// Validation of new config
	if (u32NewConfig == 0 )
		return CS_PEEVADAPTOR_INVALID_CONFIG;

	uInt32	u32BitMask = 0;
	uInt8	u8Data;
	int16	i = 0;
	for ( i = BUNNY_PEEV_CTRLBITS-1; i >= 0  ; i -- )
	{
		if ( m_bInitialized )
		{
			// Power ON
			u8Data = 0;
		}
		else
		{
			// Power off
			u8Data = 1 << RLY_PWR_ON;
		}

		u32BitMask = (1 << i ) & u32NewConfig;
		u8Data |= 1 << REG_SHIFT_CLK;
		if ( 0 != u32BitMask )
			u8Data |= (1 << REG_SI);

		PeevOutp( u8Data );

		// Falling edge: Load data into shift register
		u8Data &= ~(1 << REG_SHIFT_CLK);
		PeevOutp( u8Data );
	}

	// Raising edge: Load shift register to output
	u8Data |= (1 << REG_STORE_CLK);
	PeevOutp( u8Data );

	// Hold Data
	u8Data |= (1 << REG_CSN);
	PeevOutp( u8Data );

	// Power ON
	if ( ! m_bInitialized )
	{
		// Turn on and off Power a coule of time to make clicking sound
		// Clicking sound let user known that PeeAdaptor is working
		// We do not this code once the problem reading from parallele port is resolved
		for (i = 0; i< 4; i ++)
		{
			// Power ON
			u8Data &= ~(1 << RLY_PWR_ON);
			PeevOutp( u8Data );
			timing(250);

			// Power off
			u8Data |= (1 << RLY_PWR_ON);
			PeevOutp( u8Data );
			timing(250);
		}

		u8Data &= ~(1 << RLY_PWR_ON);
		PeevOutp( u8Data );
	}

	m_u32CurrentConfig = u32NewConfig;

	return CS_SUCCESS;
}

uInt32	PeevAdaptor::GetSwConfig()
{
	return m_u32CurrentConfig;
}


void	PeevAdaptor::PeevOutp( uInt8 u8Val )
{
#ifdef _WINDOWS_
	CS_IOCTL_STATUS			CsIoctlStatus;
	uInt32					u32ByteReturned;

#ifdef _WINDOWS_
	_in_WRITE_REGISTER		RegWriteStr;
#else
	io_READWRITE_PCI_REG	RegWriteStr;
#endif

	RegWriteStr.u32Offset	= m_ParalellePort;
	RegWriteStr.u32RegVal	= u8Val;

	uInt32 u32RetCode = DeviceIoControl( m_hIoctlHandle, CS_IOCTL_WRITE_PORT_IO,
										 &RegWriteStr, sizeof(RegWriteStr), &CsIoctlStatus, sizeof(CsIoctlStatus), &u32ByteReturned, NULL );

	_ASSERT( u32RetCode != DEV_IOCTL_ERROR );
	if ( u32RetCode == DEV_IOCTL_ERROR )
	{
		u32RetCode = GetLastError();

		TRACE( DRV_TRACE_LEVEL, "IOCTL_WRITE_PORT_IO fails. (WinError = %x )\n", u32RetCode );
	}
#endif
}
