#ifndef __CSDRV_GLBLVAR__
#define __CSDRV_GLBLVAR__

#include	"DllStruct.h"


#if defined BASEHWDLL

// Global variables accross driver

PDLL_ACQSYSTEM	g_pSystem = NULL;			// Pointer to the first system node
PVOID			g_pFirstUnCfg= NULL;		// Before organized master/slave operation, all card detected are
											// considered as unconfig cards.
											// This pointer points to the first unconfig card
PVOID			g_pLastUnCfg = NULL;		// Pointer to the last unconfig card
PVOID			g_pInvalidCfg= NULL;		// Pointer to list of card that have invalidate configuration
											// (Ex: Slave cards but missing the Master card) 
uInt16			g_NbSystem = 0;				// Number of System detected.
WCHAR			g_szWindowsPath[20] = {0};	// Windows Root (Ex:  F:\WINNT)

BOOLEAN			g_bForceIndependantMode = 0; // FALSE (0)  Default
											 // Driver will do Organize Master/Slave operaton
											 // TRUE (1)
											 // Each card detected in system will have its own DRVHANDLE
											 // This mode is used when we want to change the confiruration
											 // of M/S or Independant


PVOID			g_hWdkAcqSystem;

#else

extern	PDLL_ACQSYSTEM	g_pSystem;
extern	PVOID			g_pFirstUnCfg;
extern	PVOID			g_pLastUnCfg;
extern	PVOID			g_pInvalidCfg;
extern	uInt16			g_NbSystem;
extern	WCHAR			g_szWindowsPath[20];
extern	BOOLEAN			g_bForceIndependantMode;
extern	PVOID			g_hWdkAcqSystem;

#endif


#endif	//__CSDRV_GLBLVAR__