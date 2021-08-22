/*--------------------------------------------------------------------------*
 *
 * File:        gen_types.h
 *
 * Description: Generic type definitions
 *
 *--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*
 * $Id$
 *--------------------------------------------------------------------------*/

#ifndef GENTYPES_H
#define GENTYPES_H

/* Mandatory for vxWorks asm file compilation */

#ifndef GEN_ASM

/*--------------------------------------------------------------------------*
 * Dependencies
 *--------------------------------------------------------------------------*/

#include    <stddef.h>
#include    "gen_version.h"

/*--------------------------------------------------------------------------*
 * C++ Support
 *--------------------------------------------------------------------------*/

#if     defined(__cplusplus)
extern  "C"
{
#endif  // __cplusplus

/*--------------------------------------------------------------------------*
 * Global Defines
 *--------------------------------------------------------------------------*/

#define GEN_EOS     '\0'    // End Of String
#define GEN_GLOBAL          // Global Variable or Function
#define GEN_LOCAL   static  // Local Variable or Function
#define GEN_EXPORT          // Export a Variable or Function
#define GEN_IMPORT  extern  // Import a Variable or Function

/*--------------------------------------------------------------------------*
 * Global Macros
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*
 * Swap 16 bits value
 *--------------------------------------------------------------------------*/

#define GEN_UINT16SWAP(x) ((((x) & 0x00ff) << 8) | \
                          (((x) & 0xff00) >> 8))

#define GEN_UINT16SWAP_BUFFER(pBuff,length)                                    \
    {                                                                          \
        GEN_UINT32  _i;                /* loop control */                      \
        GEN_PUINT16 _pB;               /* Temporary pointer */                 \
                                                                               \
        /* Get the address of the buffer to swap */                            \
                                                                               \
        for(_pB=(GEN_PUINT16) pBuff, _i=0; _i<length; _i+=sizeof(*_pB), _pB++) \
            *_pB = GEN_UINT16SWAP(*_pB);                                       \
    }

/*--------------------------------------------------------------------------*
 * Swap 32 bits value
 *--------------------------------------------------------------------------*/

#define GEN_UINT32SWAP(x) ((((x) & 0x000000ff) << 24) | \
                          (((x) & 0x0000ff00) <<  8)  | \
                          (((x) & 0x00ff0000) >>  8)  | \
                          (((x) & 0xff000000) >> 24))

/* 32 bits swap on a buffer  */

#define GEN_UINT32SWAP_BUFFER(pBuff,length)                                    \
    {                                                                          \
        GEN_UINT32  _i;                /* loop control */                      \
        GEN_PUINT32 _pB;               /* Temporary pointer */                 \
                                                                               \
        /* Get the address of the buffer to swap */                            \
                                                                               \
        for(_pB=(GEN_PUINT32) pBuff, _i=0; _i<length; _i+=sizeof(*_pB), _pB++) \
            *_pB = GEN_UINT32SWAP(*_pB);                        \
    }

/*--------------------------------------------------------------------------*
 * Swap 64 bits value
 *--------------------------------------------------------------------------*/

#define GEN_UINT64SWAP(x)                          \
            (                                      \
            (((x) & 0x00000000000000ffLL) << 56) | \
            (((x) & 0x000000000000ff00LL) << 40) | \
            (((x) & 0x0000000000ff0000LL) << 24) | \
            (((x) & 0x00000000ff000000LL) <<  8) | \
            (((x) & 0x000000ff00000000LL) >>  8) | \
            (((x) & 0x0000ff0000000000LL) >> 24) | \
            (((x) & 0x00ff000000000000LL) >> 40) | \
            (((x) & 0xff00000000000000LL) >> 56)   \
            )

/* 64 bits swap on a buffer  */

#define GEN_UINT64SWAP_BUFFER(pBuff,length)                                    \
    {                                                                          \
        GEN_UINT32  _i;                /* loop control */                      \
        GEN_PUINT64 _pB;               /* Temporary pointer */                 \
                                                                               \
        /* Get the address of the buffer to swap */                            \
                                                                               \
        for(_pB=(GEN_PUINT64) pBuff, _i=0; _i<length; _i+=sizeof(*_pB), _pB++) \
            *_pB = GEN_UINT64SWAP(*_pB);                                       \
    }

/*--------------------------------------------------------------------------*
 * Global Typedefs
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/

typedef unsigned int       GEN_BITS;

/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/

typedef unsigned char      GEN_UCHAR;
typedef GEN_UCHAR         *GEN_PUCHAR;
typedef unsigned short     GEN_USHORT;
typedef GEN_USHORT        *GEN_PUSHORT;
typedef unsigned int       GEN_UINT;
typedef GEN_UINT          *GEN_PUINT;
typedef unsigned long      GEN_ULONG;
typedef GEN_ULONG          GEN_PULONG;

#ifdef GEN_WINDOWS_2K_KERNEL_MODE
typedef unsigned __int64   GEN_ULONGLONG;
#else
typedef unsigned long long GEN_ULONGLONG;
#endif

typedef GEN_ULONGLONG     *GEN_PULONGLONG;

/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/

typedef char               GEN_INT8;
typedef GEN_INT8          *GEN_PINT8;
typedef unsigned char      GEN_UINT8;
typedef GEN_UINT8         *GEN_PUINT8;

/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/

typedef short              GEN_INT16;
typedef GEN_INT16         *GEN_PINT16;
typedef unsigned short     GEN_UINT16;
typedef GEN_UINT16        *GEN_PUINT16;

/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/

#if defined(GEN_WINDOWS)
typedef long               GEN_INT32;
typedef GEN_INT32         *GEN_PINT32;
typedef unsigned long      GEN_UINT32;
typedef GEN_UINT32        *GEN_PUINT32;
#else
typedef int                GEN_INT32;
typedef GEN_INT32         *GEN_PINT32;
typedef unsigned int       GEN_UINT32;
typedef GEN_UINT32        *GEN_PUINT32;
#endif

/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/

#if defined(GEN_WINDOWS)
typedef __int64            GEN_INT64;
typedef GEN_INT64         *GEN_PINT64;
typedef unsigned __int64   GEN_UINT64;
typedef GEN_UINT6         *GEN_PUINT64;
#else
typedef long long          GEN_INT64;
typedef GEN_INT64         *GEN_PINT64;
typedef unsigned long long GEN_UINT64;
typedef GEN_UINT64        *GEN_PUINT64;
#endif

/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/

typedef void              *GEN_HANDLE;
typedef GEN_HANDLE        *GEN_PHANDLE;

/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/

typedef enum
{
    GEN_FALSE     = 0,
    GEN_TRUE      = 1,
    GEN_BOOL_ERR,
    GEN_BOOL_MIN  = GEN_FALSE,
    GEN_BOOL_MAX  = GEN_BOOL_ERR
}   GEN_BOOL;

typedef GEN_BOOL *GEN_PBOOL;

#define GEN_BOOL_TEXT(x)                \
        (                               \
        ((x)==GEN_FALSE) ? "False"    : \
        ((x)==GEN_TRUE)  ? "True"       \
                        : "Unknown"     \
        )

/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/

typedef enum
{
    GEN_ERR        = -1,
    GEN_OK         = 0,
    GEN_STATUS_MAX,
    GEN_STATUS_MIN = GEN_ERR
}   GEN_STATUS;

typedef GEN_STATUS *GEN_PSTATUS;

#define GEN_STATUS_TEXT(x)            \
        (                             \
        ((x)==GEN_OK)     ? "Ok"    : \
        ((x)==GEN_ERR)    ? "Err"     \
                        : "Unknown"   \
        )

/*--------------------------------------------------------------------------*
 * Global Variables
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*
 * Global Prototypes
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*
 * Global Functions
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*
 *
 * Function:    ~
 *
 * Desription:  ~
 *
 * Input:       ~
 *
 * Output:      ~
 *
 * Return Code: ~
 *
 *--------------------------------------------------------------------------*/
//inline    ~

#if     defined(__cplusplus)
}
#endif  // __cplusplus

#endif /* GEN_ASM */


#endif  // GENTYPES_H
