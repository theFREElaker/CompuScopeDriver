#ifndef	__GAGEWRAPPER_H__

#define __GAGEWRAPPER_H__

#include "CsTypes.h"
#include "CsDrvStructs.h"
#include "CsDrvConst.h"
#include "CsiHeader.h"
#include "CsPrivateStruct.h"
#include <stdlib.h>


#ifdef _WINDOWS_

#define	FILE_HANDLE		HANDLE
#define	EVENT_HANDLE	HANDLE
#define	SEM_HANDLE		HANDLE

#endif

#ifdef __linux__

LONGLONG GetTickCountEx();
void	QueryPerformanceCounter(LARGE_INTEGER *TickCount);
void	QueryPerformanceFrequency(LARGE_INTEGER *Frq);
void	strcat_s(char *Dest, size_t Size, char *Str);

#endif



class CSTIMER
{
private:
	LARGE_INTEGER   m_StartTickCount;		// start tick count
	LARGE_INTEGER   m_TickCount;			// current tick count;
	LARGE_INTEGER	m_TimeIncr;				// Time increment unit
	LARGE_INTEGER   m_DueTime;				// Due time in 100 ns unit
	LONGLONG		m_Temp;

public:
	void	Set( LONGLONG	DueTime )
	{
		::QueryPerformanceFrequency(&m_TimeIncr);
		m_DueTime.QuadPart = DueTime;
		::QueryPerformanceCounter(&m_StartTickCount);
	};

	BOOL	State(void)
	{
		::QueryPerformanceCounter(&m_TickCount);

		m_Temp = (m_TickCount.QuadPart - m_StartTickCount.QuadPart) * 10000000;  //in 100 ns unit
		m_Temp = (m_Temp / m_TimeIncr.QuadPart);

		if ( m_Temp  > m_DueTime.QuadPart )
			return TRUE;
		else
			return FALSE;

	};
};


void	timing_func (uInt32 point1msecs);
void	*GageAllocateMemoryX ( uInt32 u32Size );
void	GageFreeMemoryX (void *ptr);


void	GageCloseFile ( FILE_HANDLE handle );
uInt32	GageReadFile ( FILE_HANDLE handle, void * buffer, uInt32 Length, uInt32 *ReadPosition = NULL );


FILE_HANDLE	GageOpenFile ( const char * filename );
FILE_HANDLE	GageOpenFileSystem32Drivers ( const char * filename );

void	GageCopyMemoryX (void *Dest, const void *Src, uInt32 Size);
void	GageSetMemoryX (void *Dest, uInt8 Pattern, uInt32 Size);
void	GageZeroMemoryX (void *Dest, uInt32 Size);
int		GageCompareMemoryX( void * Mem1, void *Mem2, uInt32 Size );

void	FriendlyFwVersion( uInt32 u32FwVersion, char *zsText, uInt32 u32TextSize, bool bOldformat = true );
uInt32	FwRawPciVersion2UserVersion( uInt32 u32OldVersion );
uInt32	FwUserVersion2RawPciVersion( uInt32 u32UserVersion );

bool	IsPowerOf2(uInt32 u32Number);

#endif
