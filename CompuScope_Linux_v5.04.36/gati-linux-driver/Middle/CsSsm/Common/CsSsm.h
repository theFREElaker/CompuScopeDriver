#ifndef __CS_SSM_H__
#define __CS_SSM_H__

#if defined(_WIN32)
	#include "Debug.h"
#endif

#include "CsTypes.h"
#include "CsStruct.h"
#include "CsPrivateStruct.h"
#include "CsSsmStateMachine.h"
#include "CsPrototypes.h"
#include "CsPrivatePrototypes.h"
#include "RmFunctions.h"
#include "CsDisp.h"
#include "CsFs.h"
#include "Queue/DeferCall.h"
#include "Queue/Worker.h"
#include "SingletonData.h"

#pragma once

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the SSM_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// SSM_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

//! No data available for transfer
#define EMPTY_DATA		0

//! Partial data has been acquired
#define PARTIAL_DATA	1

//! Full data has been acquired
#define FULL_DATA		2

//! Abort was called while arming
#define ARMING_ABORTED	3


#define	ACTIVATE_TOKEN	1
#define	COMPLETE_TOKEN	2

#define MASK_TOKEN_VALUE	0x0FFFFFFF
#define	MASK_TX_IN_PROGRESS	0x20000000
#define MASK_TX_COMPLETED	0x40000000

#define TX_PROGRESS		0x1000
#define	TX_COMPLETED	0x0100
#define	NO_TX_STARTED	0x0010


extern bool				g_RMInitialized;
//RG extern CTraceManager*	g_pTraceMngr;

// Prototype for functions
#ifdef __linux__
	void __attribute__((constructor)) init_Demo(void);
	void __attribute__ ((destructor)) cleanup_Demo(void);
	void* ThreadFunc( LPVOID lpParam );
	void* ThreadFuncTxfer( LPVOID lpParam );
#else
	unsigned int WINAPI ThreadFunc( LPVOID lpParam );
	unsigned int WINAPI ThreadFuncTxfer( LPVOID lpParam );
#endif

void PerformQueued(CSHANDLE   hSystem,
				   PIN_PARAMS_TRANSFERDATA pInData,
				   POUT_PARAMS_TRANSFERDATA	pOutParams,
				   int32 i32Token);

#endif // __CS_SSM_H__
