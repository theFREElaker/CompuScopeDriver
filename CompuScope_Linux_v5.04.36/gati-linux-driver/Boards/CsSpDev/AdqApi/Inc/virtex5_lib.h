/* Jungo Confidential. Copyright (c) 2009 Jungo Ltd.  http://www.jungo.com */

#define DEBUG 1

#ifndef _VIRTEX5_LIB_H_
#define _VIRTEX5_LIB_H_

#define _CRT_SECURE_NO_DEPRECATE

/************************************************************************
*  File: virtex5_lib.h
*
*  Library for accessing VIRTEX5 devices.
*  The code accesses hardware using WinDriver's WDC library.
*************************************************************************/

#include "wdc_lib.h"
#include "pci_regs.h"
#include "bits.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
  General definitions
 *************************************************************/
/* Kernel PlugIn driver name (should be no more than 8 characters) */
#define KP_ADQ_DRIVER_NAME "KP_ADQ"
/* Kernel PlugIn messages - used in WDC_CallKerPlug() calls (user mode) /
 * KP_ADQ_Call() (kernel mode) */
enum {
    KP_ADQ_MSG_VERSION = 1, /* Query the version of the Kernel PlugIn */
};

/* Kernel Plugin messages status */
enum {
    KP_ADQ_STATUS_OK = 0x1,
    KP_ADQ_STATUS_MSG_NO_IMPL = 0x1000,
};

/* Default vendor and device IDs */
#define VIRTEX5_DEFAULT_VENDOR_ID 0x1B37 //0x10EE /* Vendor ID */
#define VIRTEX5_DEFAULT_DEVICE_ID 0x0044 //0x7 /* Device ID */

/* Direct Memory Access (DMA) */
typedef struct {
    WD_DMA *pDma;
    WDC_DEVICE_HANDLE hDev;
} VIRTEX5_DMA_STRUCT, *VIRTEX5_DMA_HANDLE;
 
/* Kernel PlugIn version information struct */
typedef struct {
    DWORD dwVer;
    CHAR cVer[100];
} KP_ADQ_VERSION;

/* Address space information struct */
#define MAX_TYPE 8
typedef struct {
    DWORD dwAddrSpace;
    CHAR sType[MAX_TYPE];
    CHAR sName[MAX_NAME];
    CHAR sDesc[MAX_DESC];
} VIRTEX5_ADDR_SPACE_INFO;

/* Interrupt result information struct */
typedef struct
{
    DWORD dwCounter; /* Number of interrupts received */
    DWORD dwLost;    /* Number of interrupts not yet handled */
    WD_INTERRUPT_WAIT_RESULT waitResult; /* See WD_INTERRUPT_WAIT_RESULT values
                                            in windrvr.h */
    BOOL fIsMessageBased;
    DWORD dwLastMessage;

    PVOID pBuf;
    DWORD dwTotalCount;
    BOOL fIsRead;
} VIRTEX5_INT_RESULT;
/* TODO: You can add fields to VIRTEX5_INT_RESULT to store any additional
         information that you wish to pass to your diagnostics interrupt
         handler routine (DiagIntHandler() in virtex5_diag.c) */

/* VIRTEX5 diagnostics interrupt handler function type */
typedef void (*VIRTEX5_INT_HANDLER)(WDC_DEVICE_HANDLE hDev,
    VIRTEX5_INT_RESULT *pIntResult);

/* VIRTEX5 diagnostics plug-and-play and power management events handler
 * function type */
typedef void (*VIRTEX5_EVENT_HANDLER)(WDC_DEVICE_HANDLE hDev,
    DWORD dwAction);

// VIRTEX5 register offsets 
#define VIRTEX5_SPACE AD_PCI_BAR0
enum {
    VIRTEX5_DCSR_OFFSET = 0x0,
    VIRTEX5_DLWSR_OFFSET = 0x4,
    VIRTEX5_DLTSR_OFFSET = 0x8,
    VIRTEX5_DLTCR_OFFSET = 0xc,

    VIRTEX5_WDPCR_OFFSET = 0x40,
    VIRTEX5_WDICSR_OFFSET = 0x44,
    VIRTEX5_WDLACR_OFFSET = 0x48,
    VIRTEX5_WDUACR_OFFSET = 0x4c,
    VIRTEX5_WDCCR_OFFSET = 0x50,
    VIRTEX5_WDSCSR_OFFSET = 0x54,

    VIRTEX5_CICCR_OFFSET = 0x80,
    VIRTEX5_CIACR_OFFSET = 0x84,
    VIRTEX5_CIMCR_OFFSET = 0x88,
    VIRTEX5_CIDCR_OFFSET = 0x8C,
    VIRTEX5_CISCSR_OFFSET = 0x90,
    VIRTEX5_CICSR_OFFSET = 0x94
};

/*************************************************************
  Function prototypes
 *************************************************************/
DWORD VIRTEX5_LibInit(void);
DWORD VIRTEX5_LibUninit(void);

#if !defined(__KERNEL__)
WDC_DEVICE_HANDLE VIRTEX5_DeviceOpen(const WD_PCI_CARD_INFO *pDeviceInfo,
    char *kp_name);
BOOL VIRTEX5_DeviceClose(WDC_DEVICE_HANDLE hDev);

DWORD VIRTEX5_IntEnable(WDC_DEVICE_HANDLE hDev,
    VIRTEX5_INT_HANDLER funcIntHandler);
DWORD VIRTEX5_IntDisable(WDC_DEVICE_HANDLE hDev);
BOOL VIRTEX5_IntIsEnabled(WDC_DEVICE_HANDLE hDev);

DWORD VIRTEX5_EventRegister(WDC_DEVICE_HANDLE hDev,
    VIRTEX5_EVENT_HANDLER funcEventHandler);
DWORD VIRTEX5_EventUnregister(WDC_DEVICE_HANDLE hDev);
BOOL VIRTEX5_EventIsRegistered(WDC_DEVICE_HANDLE hDev);

/* Direct Memory Access (DMA) */
DWORD VIRTEX5_DMAOpen(WDC_DEVICE_HANDLE hDev, PVOID *ppBuf, DWORD dwOptions,
    DWORD dwBytes, VIRTEX5_DMA_HANDLE *ppDmaHandle);
void VIRTEX5_DMAClose(VIRTEX5_DMA_HANDLE hDma);
void VIRTEX5_DMADevicePrepare(VIRTEX5_DMA_HANDLE hDma, BOOL fIsRead, WORD wSize,
    BOOL fEnable64bit, BYTE bTrafficClass);

void VIRTEX5_DMASyncCpu(VIRTEX5_DMA_HANDLE hDma);
void VIRTEX5_DMASyncIo(VIRTEX5_DMA_HANDLE hDma);
void VIRTEX5_DMAStart(VIRTEX5_DMA_HANDLE hDma, BOOL fIsRead, WORD wCount, DWORD bufferOffset);
BOOL VIRTEX5_DMAIsReady(WDC_DEVICE_HANDLE hDev);
BOOL VIRTEX5_DMAIsDone(WDC_DEVICE_HANDLE hDev, BOOL fIsRead);
BOOL VIRTEX5_DMAPollCompletion(VIRTEX5_DMA_HANDLE hDma, BOOL fIsRead);
WORD VIRTEX5_DMAGetMaxPacketSize(WDC_DEVICE_HANDLE hDev, BOOL fIsRead);

/* Enable DMA interrupts */
void VIRTEX5_DmaIntEnable(WDC_DEVICE_HANDLE hDev, BOOL fIsRead);
/* Disable DMA interrupts */
void VIRTEX5_DmaIntDisable(WDC_DEVICE_HANDLE hDev, BOOL fIsRead);
BOOL VIRTEX5_DmaIsReadSucceed(WDC_DEVICE_HANDLE hDev);
BOOL VIRTEX5_DmaFlush(WDC_DEVICE_HANDLE hDev);

/* Read from a 8/16/32bit register. */
BYTE VIRTEX5_ReadReg8(WDC_DEVICE_HANDLE hDev, DWORD offset);
WORD VIRTEX5_ReadReg16(WDC_DEVICE_HANDLE hDev, DWORD offset);
DWORD VIRTEX5_ReadReg32(WDC_DEVICE_HANDLE hDev, DWORD offset, UINT32* data);

/* Write to a 8/16/32bit register. */
void VIRTEX5_WriteReg8(WDC_DEVICE_HANDLE hDev, DWORD offset, BYTE data);
void VIRTEX5_WriteReg16(WDC_DEVICE_HANDLE hDev, DWORD offset, WORD data);
DWORD VIRTEX5_WriteReg32(WDC_DEVICE_HANDLE hDev, DWORD offset, UINT32 data);
#endif

DWORD VIRTEX5_GetNumAddrSpaces(WDC_DEVICE_HANDLE hDev);
BOOL VIRTEX5_GetAddrSpaceInfo(WDC_DEVICE_HANDLE hDev,
    VIRTEX5_ADDR_SPACE_INFO *pAddrSpaceInfo);
const char *VIRTEX5_GetLastErr(void);

#ifdef __cplusplus
}
#endif

#endif

