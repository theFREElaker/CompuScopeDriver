#include "StdAfx.h"
#include "CsTypes.h"
#include "CsDefines.h"
#include "GageWrapper.h"
#ifdef _WINDOWS_
#include "CsSupport.h"
#endif
//--------------------------------------------------------------------
//
//--------------------------------------------------------------------

void	timing_func (uInt32 point1msecs)
{
	Sleep( point1msecs / 10  );
}


//--------------------------------------------------------------------
//
//--------------------------------------------------------------------

void	*GageAllocateMemoryX (uInt32 u32Size)
{
#ifdef _WINDOWS_
	return VirtualAlloc( NULL, u32Size, MEM_COMMIT, PAGE_READWRITE );
#else
	return malloc( u32Size );
#endif
}


//--------------------------------------------------------------------
//
//--------------------------------------------------------------------

void	GageFreeMemoryX (void *ptr)
{
#ifdef _WINDOWS_
	VirtualFree( ptr, 0, MEM_RELEASE );
#else
	free( ptr );
#endif

}


//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void	GageZeroMemoryX (void *ptr, uInt32 u32Size)
{
	memset( ptr, 0, u32Size );
}


//--------------------------------------------------------------------
//
//--------------------------------------------------------------------

void	GageCopyMemoryX (void *Dest, const void *Src, uInt32 Size)
{
	memcpy( Dest, Src, Size );
}


//--------------------------------------------------------------------
//
//--------------------------------------------------------------------



//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void	GageSetMemoryX (void *Dest, uInt8 Pattern, uInt32 u32Size)
{
	memset( Dest, Pattern, u32Size );
}


//--------------------------------------------------------------------
//
//--------------------------------------------------------------------

int	GageCompareMemoryX( void * Mem1, void *Mem2, uInt32 Size )
{
	return memcmp(Mem1, Mem2, Size);
}



//--------------------------------------------------------------------
//
//--------------------------------------------------------------------

FILE_HANDLE	GageOpenFile ( const char * filename )
{
	FILE_HANDLE	fHandle = 0;
#ifdef _WINDOWS_

	PVOID	OldValue = NULL;
	BOOL bRet = TRUE;
	if ( Is32on64() )
	{
		// Disable, if available, the WOW64 file system redirection value.
		LPFN_WOW64DISABLEWOW64FSREDIRECTION pfnWow64DisableWow64FsRedirection = (LPFN_WOW64DISABLEWOW64FSREDIRECTION)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), 
																																	_T("Wow64DisableWow64FsRedirection"));
		if (NULL != pfnWow64DisableWow64FsRedirection)
		{
			bRet = pfnWow64DisableWow64FsRedirection (&OldValue);
		}
	}

	if ( bRet )
	{
		fHandle = CreateFile( filename,
							  GENERIC_READ,
							  FILE_SHARE_READ, 
							  NULL,
							  OPEN_EXISTING, 
							  0, 
							  NULL);

		if (Is32on64())
		{
			// Restore the previous WOW64 file system redirection value.
			LPFN_WOW64REVERTWOW64FSREDIRECTION pfnWow64RevertWow64FsRedirection = (LPFN_WOW64REVERTWOW64FSREDIRECTION)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), 
																																	_T("Wow64RevertWow64FsRedirection"));
			if (NULL != pfnWow64RevertWow64FsRedirection)
				pfnWow64RevertWow64FsRedirection(OldValue);
		}
	}

	if ( INVALID_HANDLE_VALUE == fHandle )
		return NULL;
	else
		return fHandle;

#else	// Linux

	fHandle = open (filename, O_RDONLY);

	if ( fHandle == (FILE_HANDLE) -1  )
	{
		TRACE(DISP_TRACE_LEVEL, "GageOpenFile: Error on file: %s\n", filename);
		return 0;
	}
	else
		return fHandle;

#endif

}


//--------------------------------------------------------------------
//
//--------------------------------------------------------------------

void	GageCloseFile ( FILE_HANDLE handle )
{
#ifdef _WINDOWS_
	CloseHandle( handle );
#else
	close( handle );
#endif


}


//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
uInt32	GageReadFile ( FILE_HANDLE handle, void * buffer, uInt32 Length, uInt32 *ReadPosition )
{
	uInt32	u32BytesRead = 0;
#ifdef _WINDOWS_

	if ( ReadPosition )
		SetFilePointer( handle, *ReadPosition, 0, FILE_BEGIN );

	ReadFile( handle, buffer, Length, &u32BytesRead, NULL );

#else	// Linux

	if ( ReadPosition )
		u32BytesRead = pread( handle, buffer, Length, *ReadPosition );
	else
		u32BytesRead = read( handle, buffer, Length );

#endif
	return u32BytesRead;

}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

bool	IsPowerOf2(uInt32 u32Number)
{
	uInt32	nCount1 = 0;

	for (uInt32 i = 0; i < 32; i++)
	{
		if ( u32Number & (1<<i) )
			nCount1++;
	}

	return( 1 == nCount1 );
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void FriendlyFwVersion( uInt32 u32FwVersion, char *zsText, uInt32 u32TextSize, bool bOldFormat )
{
	char	*szFwFormat = (char *)"00.00.00.000";
	char	szFwFriendlyVer[50];
	uInt32	u32CopySize = (uInt32) Min( u32TextSize-1, strlen(szFwFormat) );

	CSFWVERSION FwVersion;

	if ( bOldFormat )
	{
		FPGA_FWVERSION	   OldFwVer;
		OldFwVer.u32Reg = u32FwVersion;

		FwVersion.Version.uMajor	= OldFwVer.Version.uMajor;
		FwVersion.Version.uMinor	= OldFwVer.Version.uMinor;
		FwVersion.Version.uRev		= OldFwVer.Version.uRev;
		FwVersion.Version.uIncRev	= OldFwVer.Version.uIncRev;
	}
	else
		FwVersion.u32Reg = u32FwVersion;

	sprintf_s( szFwFriendlyVer, sizeof(szFwFriendlyVer),
				"%02d.%02d.%02d.%03d", FwVersion.Version.uMajor, FwVersion.Version.uMinor,
													 FwVersion.Version.uRev, FwVersion.Version.uIncRev );

	GageCopyMemoryX( zsText, szFwFriendlyVer, u32CopySize);
	zsText[u32CopySize] ='\0';
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void FwVersionToText( CSFWVERSION FwVersion, char *zsText, uInt32 u32TextSize )
{
	char *szFwFormat = (char *)"00.00.00.00";
	char szFwFriendlyVer[50];
	uInt32	u32CopySize = (uInt32) Min( u32TextSize-1, strlen(szFwFormat) );

	sprintf_s( szFwFriendlyVer, sizeof(szFwFriendlyVer),
				"%02d.%02d.%02d.%02d", FwVersion.Version.uMajor, FwVersion.Version.uMinor,
													 FwVersion.Version.uRev, FwVersion.Version.uIncRev );

	GageCopyMemoryX( zsText, szFwFriendlyVer, u32CopySize);
	zsText[u32CopySize] ='\0';
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32 FwRawPciVersion2UserVersion( uInt32 u32OldVersion )
{
	FPGA_FWVERSION		OldFwVer;
	CSFWVERSION			FwVersion;

	
	OldFwVer.u32Reg = u32OldVersion;
	FwVersion.Version.uMajor	= OldFwVer.Version.uMajor;
	FwVersion.Version.uMinor	= OldFwVer.Version.uMinor;
	FwVersion.Version.uRev		= OldFwVer.Version.uRev;
	FwVersion.Version.uIncRev	= OldFwVer.Version.uIncRev;

	return FwVersion.u32Reg;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

uInt32 FwUserVersion2RawPciVersion( uInt32 u32UserVersion )
{
	FPGA_FWVERSION		OldFwVer;
	CSFWVERSION			FwVersion;

	
	FwVersion.u32Reg = u32UserVersion;
	OldFwVer.Version.uMajor		= FwVersion.Version.uMajor; 
	OldFwVer.Version.uMinor		= FwVersion.Version.uMinor; 
	OldFwVer.Version.uRev		= FwVersion.Version.uRev;
	OldFwVer.Version.uIncRev	= FwVersion.Version.uIncRev; 

	return OldFwVer.u32Reg;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------

// Open a file from System32\Drivers
FILE_HANDLE	GageOpenFileSystem32Drivers ( const char * filename )
{
	char		szFilePath[MAX_PATH] = {0};

#ifdef _WINDOWS_

#define	WIN32DRIVERPATH	"\\SYSTEM32\\DRIVERS\\"

	GetWindowsDirectory(szFilePath, sizeof(szFilePath));
	strcat_s( szFilePath, sizeof(szFilePath),  WIN32DRIVERPATH );
	strcat_s( szFilePath, sizeof(szFilePath), filename );

	return GageOpenFile( szFilePath );
#else
   int szLen = 0;
	strncpy(szFilePath, CSI_FILE_PATH, MAX_PATH);
	strncat(szFilePath, "/", MAX_PATH);
   szLen = strlen(szFilePath);
	strncat(szFilePath, filename, MAX_PATH-szLen );

	return GageOpenFile( szFilePath );
#endif
}



#ifdef __linux__

LONGLONG GetTickCountEx()
{
	struct timeval _tstart;
	struct timezone tz;
	double dMicroseconds;

	gettimeofday(&_tstart, &tz);
	dMicroseconds = (double)(_tstart.tv_sec * 1000000.0) + (double)(_tstart.tv_usec);
	return (LONGLONG)dMicroseconds;
}


void	QueryPerformanceCounter(LARGE_INTEGER *TickCount)
{
	TickCount->QuadPart = GetTickCountEx();
}

void	QueryPerformanceFrequency(LARGE_INTEGER *Frq)
{
	Frq->QuadPart = 1000000;
}

void	strcat_s(char *Dest, size_t Size, char *Str)
{
	strcat(Dest, Str);
}

#endif
