#ifdef  _WIN32

#include <ntddk.h>
#include <wdf.h>

#include <ntstrsafe.h>

#include "wmilib.h"
#include <initguid.h>

#else	//__linux__

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/pci.h>

#include "gen_kernel_interface.h"
#include "gen_kdebug.h"

#endif

#define TRACE_LEVEL_DISABLE				0
#define TRACE_LEVEL_INFORMATION			1
#define TRACE_LEVEL_LIGHT				2
#define TRACE_LEVEL_MEDIUM				3
#define TRACE_LEVEL_HEAVY				4



#define TRACE_FLAG_INTERUPT				1
#define TRACE_FLAG_DMA					2

#if ! defined( TRACE_EVENTS_SRC )
extern ULONG	g_DebugLevel;
extern ULONG	g_DebugFlag;
#endif

VOID TraceEvents    ( IN ULONG   TraceEventsLevel, IN ULONG   TraceEventsFlag, IN PCCHAR  DebugMessage,   ...   );
