
#include "StdAfx.h"
#include "CsSsm.h"
#include "CsSsmDebug.h"

int32 SSM_API CsDebugRetrieveStateName(CSHANDLE hSystem, LPTSTR szStateName, int16 i16Length)
{
	_ASSERT(g_RMInitialized);

	CStateMachine*	pStateMachine = NULL;
	int32			i32BytesCopied = 0;

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	pStateMachine = CSystemData::instance().GetStateMachine(hSystem);
	i32BytesCopied = pStateMachine->RetrieveStateName(szStateName, i16Length);

	return (i32BytesCopied);
}

int32 SSM_API CsDebugSetTrace(CSHANDLE hSystem, int8 bEnable, ostream& debug_stream)
{
	_ASSERT(g_RMInitialized);

	CStateMachine*	pStateMachine = NULL;

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	pStateMachine = CSystemData::instance().GetStateMachine(hSystem);
	pStateMachine->Set_Debug_Trace(bEnable, debug_stream);

	return (1);
}

bool SSM_API CsDebugPerformTransition(CSHANDLE hSystem, int16 i16Transition)
{
	_ASSERT(g_RMInitialized);

	CStateMachine*	pStateMachine = NULL;
	bool			bActionPerformed = false;
	
	if (!g_RMInitialized)
		return false;

	pStateMachine = CSystemData::instance().GetStateMachine(hSystem);
	bActionPerformed = pStateMachine->PerformTransition(i16Transition);

	return (bActionPerformed);
}

int32 SSM_API CsTraceConfiguration_Acquisition(CSACQUISITIONCONFIG& configAcquisition)
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: Size %i\n", configAcquisition.u32Size);
	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: SampleRate %I64d\n", configAcquisition.i64SampleRate);
	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: ExtClk %i\n", configAcquisition.u32ExtClk);
	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: ExtClkSkip %i\n", configAcquisition.u32ExtClkSampleSkip);
	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: Mode %i\n", configAcquisition.u32Mode);
	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: SampleBits %i\n", configAcquisition.u32SampleBits);
	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: SampleRes %i\n", configAcquisition.i32SampleRes);
	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: SampleSize %i\n", configAcquisition.u32SampleSize);
	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: SegmentCount %i\n", configAcquisition.u32SegmentCount);
	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: Depth %I64d\n", configAcquisition.i64Depth);
	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: SegmentSize %I64d\n", configAcquisition.i64SegmentSize);
	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: TriggerTimeout %I64d\n", configAcquisition.i64TriggerTimeout);
	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: SegmentCount %i\n", configAcquisition.u32SegmentCount);
	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: TriggerEngines %i\n", configAcquisition.u32TrigEnginesEn);
	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: TriggerDelay %I64d\n", configAcquisition.i64TriggerDelay);
	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: TriggerHoldoff %I64d\n", configAcquisition.i64TriggerHoldoff);
	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: SampleOffset %i\n", configAcquisition.i32SampleOffset);
	TRACE(SSM_TRACE_LEVEL, "AcquisitionConfig: TimeStamp %i\n", configAcquisition.u32TimeStampConfig);

	return (CS_SUCCESS);
}

int32 SSM_API CsTraceConfiguration_Channels(PARRAY_CHANNELCONFIG& arrayChannels)
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	for (uInt32 i = 0; i < arrayChannels->u32ChannelCount; i++)
	{
		TRACE(SSM_TRACE_LEVEL, "ChannelConfig: Size %i\n", arrayChannels->aChannel[i].u32Size);
		TRACE(SSM_TRACE_LEVEL, "ChannelConfig: Index %i\n", arrayChannels->aChannel[i].u32ChannelIndex);
		TRACE(SSM_TRACE_LEVEL, "ChannelConfig: Termination %i\n", arrayChannels->aChannel[i].u32Term);
		TRACE(SSM_TRACE_LEVEL, "ChannelConfig: InputRange %i\n", arrayChannels->aChannel[i].u32InputRange);
		TRACE(SSM_TRACE_LEVEL, "ChannelConfig: Impedance %i\n", arrayChannels->aChannel[i].u32Impedance);
		TRACE(SSM_TRACE_LEVEL, "ChannelConfig: Filter %i\n", arrayChannels->aChannel[i].u32Filter);
		TRACE(SSM_TRACE_LEVEL, "ChannelConfig: DC Offset %i\n", arrayChannels->aChannel[i].i32DcOffset);
		TRACE(SSM_TRACE_LEVEL, "ChannelConfig: Calib %i\n", arrayChannels->aChannel[i].i32Calib);
	}

	return (CS_SUCCESS);
}

int32 SSM_API CsTraceConfiguration_Triggers(PARRAY_TRIGGERCONFIG& arrayTriggers)
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	for (uInt32 i = 0; i < arrayTriggers->u32TriggerCount; i++)
	{
		TRACE(SSM_TRACE_LEVEL, "TriggerConfig: Size %i\n", arrayTriggers->aTrigger[i].u32Size);
		TRACE(SSM_TRACE_LEVEL, "TriggerConfig: Index %i\n", arrayTriggers->aTrigger[i].u32TriggerIndex);
		TRACE(SSM_TRACE_LEVEL, "TriggerConfig: Condition %i\n", arrayTriggers->aTrigger[i].u32Condition);
		TRACE(SSM_TRACE_LEVEL, "TriggerConfig: Level %i\n", arrayTriggers->aTrigger[i].i32Level);
		TRACE(SSM_TRACE_LEVEL, "TriggerConfig: Source %i\n", arrayTriggers->aTrigger[i].i32Source);
		TRACE(SSM_TRACE_LEVEL, "TriggerConfig: ExtCoupling %i\n", arrayTriggers->aTrigger[i].u32ExtCoupling);
		TRACE(SSM_TRACE_LEVEL, "TriggerConfig: ExtInputRange %i\n", arrayTriggers->aTrigger[i].u32ExtTriggerRange);
		TRACE(SSM_TRACE_LEVEL, "TriggerConfig: ExtImpedance %i\n", arrayTriggers->aTrigger[i].u32ExtImpedance);
		TRACE(SSM_TRACE_LEVEL, "TriggerConfig: Value1 %i\n", arrayTriggers->aTrigger[i].i32Value1);
		TRACE(SSM_TRACE_LEVEL, "TriggerConfig: Value2 %i\n", arrayTriggers->aTrigger[i].i32Value2);
		TRACE(SSM_TRACE_LEVEL, "TriggerConfig: Filter %i\n", arrayTriggers->aTrigger[i].u32Filter);
		TRACE(SSM_TRACE_LEVEL, "TriggerConfig: Relation %i\n", arrayTriggers->aTrigger[i].u32Relation);
	}

	return (CS_SUCCESS);
}