#define TRACE_EVENTS_SRC

#include "TraceEvents.h"


ULONG	g_DebugLevel = 0;
ULONG	g_DebugFlag = TRACE_FLAG_DMA;



VOID
TraceEvents    (
    IN ULONG   TraceEventsLevel,
    IN ULONG   TraceEventsFlag,
    IN PCCHAR  DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for the sample driver.

Arguments:

    TraceEventsLevel - print level between 0 and 3, with 3 the most verbose

Return Value:

    None.

 --*/
 {
#if DBG
#define     TEMP_BUFFER_SIZE        512
    va_list    list;
    CHAR       debugMessageBuffer[TEMP_BUFFER_SIZE];
    NTSTATUS   status;

    va_start(list, DebugMessage);

    if (DebugMessage)
	{

        //
        // Using new safe string functions instead of _vsnprintf.
        // This function takes care of NULL terminating if the message
        // is longer than the buffer.
        //
        status = RtlStringCbVPrintfA( debugMessageBuffer,
                                      sizeof(debugMessageBuffer),
                                      DebugMessage,
                                      list );
        if(!NT_SUCCESS(status))
		{

            DbgPrint (" RtlStringCbVPrintfA failed %x\n", status);
            return;
        }
        if (g_DebugLevel > TRACE_LEVEL_DISABLE && (TraceEventsLevel >= g_DebugLevel && ((TraceEventsFlag & g_DebugFlag) == TraceEventsFlag)))
            DbgPrint(debugMessageBuffer);
    }
    va_end(list);

    return;
#else
    UNREFERENCED_PARAMETER(TraceEventsLevel);
    UNREFERENCED_PARAMETER(TraceEventsFlag);
    UNREFERENCED_PARAMETER(DebugMessage);
#endif
}
