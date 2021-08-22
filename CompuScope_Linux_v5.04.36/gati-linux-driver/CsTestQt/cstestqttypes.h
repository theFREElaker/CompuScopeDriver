#ifndef CSTESTQTTYPES_H
#define CSTESTQTTYPES_H

#if defined(_WIN32)
    #include "CsCvi.h"
#else
    #ifndef __KERNEL__
    #include <stdint.h>
    #endif
        #include <inttypes.h>
        //#include <ieee754.h>
        //typedef ieee754_double real64;

        typedef void* HANDLE;

        #define FALSE   0
        #define TRUE    1

        #ifdef UNICODE

            #define _tcslen     wcslen
            #define _tcscpy     wcscpy
            #define _tcscpy_s   wcscpy_s
            #define _tcsncpy    wcsncpy
            #define _tcsncpy_s  wcsncpy_s
            #define _tcscat     wcscat
            #define _tcscat_s   wcscat_s
            #define _tcsupr     wcsupr
            #define _tcsupr_s   wcsupr_s
            #define _tcslwr     wcslwr
            #define _tcslwr_s   wcslwr_s

            #define _stprintf_s swprintf_s
            #define _stprintf   swprintf
            #define _tprintf    wprintf

            #define _vstprintf_s    vswprintf_s
            #define _vstprintf      vswprintf

            #define _tscanf     wscanf


            #define TCHAR wchar_t

        #else

            #define _tcslen     strlen
            #define _tcscpy     strcpy
            #define _tcscpy_s   strcpy_s
            #define _tcsncpy    strncpy
            #define _tcsncpy_s  strncpy_s
            #define _tcscat     strcat
            #define _tcscat_s   strcat_s
            #define _tcsupr     strupr
            #define _tcsupr_s   strupr_s
            #define _tcslwr     strlwr
            #define _tcslwr_s   strlwr_s

            #define _stprintf_s sprintf_s
            #define _stprintf   sprintf
            #define _tprintf    printf

            #define _vstprintf_s    vsprintf_s
            #define _vstprintf      vsprintf

            #define _tscanf     scanf

            #define TCHAR char
        #endif

        typedef union _LARGE_INTEGER {
          struct {
            unsigned long LowPart;
            long  HighPart;
          };
          struct {
            unsigned long LowPart;
            long  HighPart;
          } u;
          long long QuadPart;
        } LARGE_INTEGER, *PLARGE_INTEGER;
#endif

typedef struct _DATA_DETAILS
{
    bool bOverdraw;
    unsigned long long u64CurrSeg;
} DATA_DETAILS, *PDATA_DETAILS;

typedef enum    _BOARD_STATUS
{
    BoardReady, BoardStart, BoardAcquire, BoardWaitForTrigger, BoardCapture, BoardTransfer, GraphUpdate
} BOARD_STATUS, *PBOARD_STATUS;

typedef enum    _DRAW_CHAN_TYPE
{
    drawchanreg, drawchanfilesig, drawchanfiledata
} DRAW_CHAN_TYPE, *PDRAW_CHAN_TYPE;

typedef enum    _GRAPH_UPDATE
{
    GraphInit, GraphUpdatePlot, GraphUpdateOnly
} GRAPH_UPDATE, *PGRAPH_UPDATE;

typedef struct  _STATUS_BAR_INFO
{
    long long               m_i64StatBarStartPos;
    unsigned long long      m_u64StatBarAcqCount;
    unsigned long long      m_u64StatBarSegCount;
    unsigned long long      m_u64StatBarCurrSeg;
    unsigned long long      m_u64StatBarSampRate;
    unsigned long           m_u32StatBarHorizZoom;
    BOARD_STATUS            m_BoardStatus;
    double                  m_dblStatBarCommitTimeMsec;
    double                  m_dblStatBarXferTimeMsec;
} STATUS_BAR_INFO, *PSTATUS_BAR_INFO;

#endif // CSTESTQTTYPES_H
