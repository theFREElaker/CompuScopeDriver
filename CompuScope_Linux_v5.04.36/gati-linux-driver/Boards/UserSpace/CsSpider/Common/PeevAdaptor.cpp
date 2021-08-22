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


// Shift register
#define	CTRL_CH1				0x1
#define	CTRL_CH2				0x2
#define	CTRL_CH3				0x4
#define	CTRL_CH4				0x8
#define	CTRL_CH5				0x10
#define	CTRL_CH6				0x20
#define	CTRL_CH7				0x40
#define	CTRL_CH8				0x80
#define	CTRL_CLKIN				0x100
#define	CTRL_TRIGIN				0x200
#define	CTRL_CLKOUT				0x400
#define	CTRL_TRIGOUT			0x800
#define	CTRL_AUX_SRC			0x1000
#define	CTRL_CLKOUT_TRIGOUT		0x2000
#define	CTRL_CLKOUT_TRIGOUT_ON	0x400
#define	CTRL_GND_ON				0x8000
#define	CTRL_LOOP_DC			0x10000
#define	CTRL_LOOP_AUX			0x20000
#define	CTRL_AUX_FILTER			0x40000


PeevAdaptor::PeevAdaptor(): m_ParalellePort(0x378),
							m_u32CurrentConfig(0),
							m_bInitialized(FALSE)
{

}


PeevAdaptor::~PeevAdaptor()
{

}

int32	PeevAdaptor::Initialize( FILE_HANDLE hDevIoctl )
{
	uInt32		u32DefaultConfig = 0xF00;
	ULONG		u32IoPort = 0x378;

#ifdef _WINDOWS_
	HKEY hKey;
	if( ERROR_SUCCESS == ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Cs8xxxWDF\\Parameters", 0, KEY_QUERY_VALUE, &hKey) )
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
		uInt8	u8Data = 0;

		// Power off
		u8Data = 0;
		PeevOutp( u8Data );
		m_bInitialized = FALSE;
	}
}

int32	PeevAdaptor::SetConfig( uInt32 u32NewConfig, BOOLEAN bInit )
{

	if ( ! bInit && ! m_bInitialized )
		return CS_PEEVADAPTOR_NOT_INIT;

	// Validation of new config
	if (u32NewConfig == 0 )
		return CS_PEEVADAPTOR_INVALID_CONFIG;

	if ( (u32NewConfig & (CTRL_CLKOUT_TRIGOUT_ON | CTRL_GND_ON))
		 == (CTRL_CLKOUT_TRIGOUT_ON | CTRL_GND_ON) )
		return CS_PEEVADAPTOR_INVALID_CONFIG;

	if ( (u32NewConfig & (CTRL_AUX_SRC | CTRL_GND_ON))
		 == (CTRL_AUX_SRC | CTRL_GND_ON) )
		return CS_PEEVADAPTOR_INVALID_CONFIG;


	uInt32	u32BitMask = 0;
	uInt8	u8Data;
	int16	i = 0;
	for ( i = PEEV_CTRLBITS-1; i >= 0  ; i -- )
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
