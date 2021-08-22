#pragma once

#ifdef _WIN32
#include <tchar.h>
#else
#include "CsLinuxPort.h"
#endif


#include "DiskHead.h"
#include "gage_drv.h"
#include "Structs.h"

#ifndef CS_WIN64
#define __unaligned
#endif

const int SIG_VERSION_COUNT = 16;

const LPCTSTR SIG_VERSION_120 = _T("GS V.1.20");
const LPCTSTR SIG_VERSION_215 = _T("GS V.2.15");
const LPCTSTR SIG_VERSION_270 = _T("GS V.2.70");
const LPCTSTR SIG_VERSION_280 = _T("GS V.2.80");
const LPCTSTR SIG_VERSION_285 = _T("GS V.2.85");
const LPCTSTR SIG_VERSION_300 = _T("GS V.3.00");

// Utility functions for saving SIG files

short GetSRIndex(float fTBS);
unsigned short MakeBoardType(uInt32 u32SampleBits);
short GetProbe(short shAdjust);
short ConstructGainConstant(uInt32 u32InputRange, int16 __unaligned *CapturedGain);
short ConstructDcOffset(int32 i32DcOffset, uInt32 u32InputRange);

// Utility functions for reading SIG files

uInt32 ConstructDataRange(short shGain);
int32 ConstructDcOffsetPercent(short sDcOffset, uInt32 u32Range);
BOOL GetSIGVersion(LPCTSTR Version);
short CheckDiskBoardType (disk_file_header *header);
void CheckDiskSampleRateIndex (short sCheckV270, 
							   short sCheckV280, 
							   short sCheckV285, 
							   short sCheckV300, 
							   short __unaligned *sSampleRateIndex);
void AdjustHeaderToVersion (disk_file_header *header);
int32 GetSampleResFromHeader(disk_file_header *header);
int32 GetSampleOffsetFromHeader(disk_file_header *header);
uInt32 GetMulRecCountFromHeader(disk_file_header *header);
int64 GetSampleRateFromHeader(disk_file_header *header);
