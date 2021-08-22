#include "StdAfx.h"
#include "CsSsm.h"
#include "CsDrvConst.h"

//!
//! \ingroup SSM_API
//!
//! \fn int32 SSM_API CsGetSystemCaps(CSHANDLE hSystem, uInt32 CapsId, void* pBuffer, uInt32* pBufferSize);
//!	Retrieves the capabilities of the CompuScope system in its current configuration
//!
//! This method is designed to dynamically query capabilities of the CompuScope hardware. The returned parameters are valid for
//! current CompuScope configuration. Usually, this method is called twice.
//! On the first call the pBuffer parameter is passed as NULL and method assigns *pBufferSize the size of the buffer
//! required to hold the requested data. The calling process can then allocate a sufficient buffer and pass its address to the method
//! on the second call. The user may query different capabilities by using different \link SYSTEM_CAPS ID CapsId constants \endlink.
//!
//! \param hSystem		: Handle of the CompuScope system to be addressed. Obtained from CsGetSystem
//! \param CapsId		: \link SYSTEM_CAPS ID of capabilities to retrieve \endlink
//! \param pBuffer		: Pointer to the output buffer.
//! \param pBufferSize	: Size of the output buffer. Cannot be NULL.
//!
//!	\return
//!		- Positive value (>0) for success
//!		- Negative value (<0) for error
//!			Call CsGetErrorString with the error code to obtain a descriptive error string.
//!
////////////////////////////////////////////////////////////////////
int32 SSM_API CsGetSystemCaps(CSHANDLE hSystem, uInt32 CapsId, void* pBuffer, uInt32* pBufferSize)
{
	_ASSERT(g_RMInitialized);

	if (g_RMInitialized)
	{
		if (NULL != pBufferSize && ::IsBadWritePtr(pBufferSize, sizeof(uInt32)))
			return (CS_INVALID_POINTER_BUFFER);

		if (NULL != pBuffer)
		{
			if (::IsBadWritePtr(pBuffer, *pBufferSize))
				return (CS_INVALID_POINTER_BUFFER);
		}

		if (FS_BASE_HANDLE & (ULONG_PTR) hSystem)
			return (CS_INVALID_HANDLE);

		int32 i32RetCode = CSystemData::instance().ValidateSystemHandle(hSystem);
		if (CS_INVALID_HANDLE == i32RetCode)
			return (i32RetCode);

		CSSYSTEMCONFIG pConfig;

		pConfig.u32Size = sizeof(CSSYSTEMCONFIG);
		CSystemData::instance().GetSystemConfig(hSystem, &pConfig);

		return (::CsDrvGetAcquisitionSystemCaps(hSystem, CapsId, &pConfig, pBuffer, pBufferSize));
	}
	else
		return CS_NOT_INITIALIZED;
}

/*!
 \ingroup SSM_API

 \fn int32 SSM_API CsGet(CSHANDLE hSystem, int32 nIndex, int32 nConfig, void* pData)
	Retrieves the current configuration settings in the driver or on the CompuScope hardware.

 This method is used to retrieve the current values of various configuration settings in the driver or on the CompuScope hardware.
 If the nConfig parameter is set to CS_CURRENT_CONFIGURATION, then the method retrieves the configuration setting variable
 values in the driver.  These values may not be the actual current configuration settings on the CompuScope hardware
 if CsDo() with the ACTION_COMMIT parameter has not yet been called.
 If the nConfig parameter is set to CS_ACQUISITION_CONFIGURATION, then the method retrieves the configuration settings from the CompuScope hardware.
 If the nConfig parameter is set to CS_ACQUIRED_CONFIGURATION, then the method retrieves the configuration settings that
 were current during the last acquisition by the CompuScope system.  Consequently, data from the last acquisition should be
 interpreted using configuration settings retrieved with the CS_ACQUIRED_CONFIGURATION parameter.

 CS_CURRENT_CONFIGURATION:	Configuration modified by CsSet but not yet committed
 CS_ACQUISITION_CONFIGURATION: Configuration committed to the hardware. This configuration might have been coerced.
 CS_ACQUIRED_CONFIGURATION: Configuration that corresponds to the last Start Acquisition command.

 On a Commit, the Current configuration and the Acquisition configuration are synchronized.
 On a Start Acquisition, the Acquisition configuration and the Acquired configuration are synchronized.

 \param hSystem	: Handle of the CompuScope system to be addressed. Obtained from CsGetSystem
 \param nIndex	: Specifies the \link CONF_QUERY type \endlink of configuration settings to retrieve.
 \param nConfig	: Type of settings to retrieve.
       - CS_CURRENT_CONFIGURATION retrieves current driver variable settings.
       - CS_ACQUISITION_CONFIGURATION retrieves hardware settings from the most recent acquisition.
 \param pData	: Pointer to the structure that will be filled with the configuration settings. Structures types should be:
         - CSACQUISITIONCONFIG  for nIndex = CS_ACQUISITION
         - CSCHANNELCONFIG      for nIndex = CS_CHANNEL
         - CSTRIGGERCONFIG      for nIndex = CS_TRIGGER

 \return
		- Positive value (>0) for success
		- Negative value (<0) for error
			Call CsGetErrorString with the error code to obtain a descriptive error string.

  Example:
 \code
	CSACQUISITIONCONFIG acqConf;
	acqConf.u32Size = sizeof(CSACQUISITIONCONFIG);
	CsGet(hSystem, CS_ACQUISITION, CS_CURRENT_CONFIGURATION, &acqConf);
 \endcode

  \sa CSACQUISITIONCONFIG, CSCHANNELCONFIG, CSTRIGGERCONFIG
*/
////////////////////////////////////////////////////////////////////
int32 SSM_API CsGet(CSHANDLE hSystem, int32 nIndex, int32 nConfig, void* pData)
{
	size_t	sizeStruct = 0;

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	if (NULL == pData)
		return (CS_NULL_POINTER);

	// Only System card handle can be configured
	if (FS_BASE_HANDLE & (ULONG_PTR)hSystem)
		return (CS_INVALID_HANDLE);

	int32 i32RetCode = CSystemData::instance().ValidateSystemHandle(hSystem);
	if (CS_INVALID_HANDLE == i32RetCode)
		return (i32RetCode);

	switch(nIndex)
	{
		case CS_ACQUISITION:
		{
			sizeStruct = ((PCSACQUISITIONCONFIG)pData)->u32Size;
			if (sizeStruct > sizeof(CSACQUISITIONCONFIG) || 0 == sizeStruct)
				return (CS_INVALID_STRUCT_SIZE);

			if (::IsBadWritePtr(pData, sizeStruct))
				return (CS_INVALID_POINTER_BUFFER);

			return (CSystemData::instance().GetAcquisitionConfig(hSystem, nConfig, (PCSACQUISITIONCONFIG)pData));
		}
		break;

		case CS_CHANNEL:
		{
			PCSCHANNELCONFIG pChannel = NULL;

			pChannel = (PCSCHANNELCONFIG)pData;
			sizeStruct = pChannel->u32Size;
			if (sizeStruct > sizeof(CSCHANNELCONFIG) || 0 == sizeStruct)
				return (CS_INVALID_STRUCT_SIZE);

			if (::IsBadWritePtr(pData, sizeStruct))
				return (CS_INVALID_POINTER_BUFFER);

			if ( (0 == pChannel->u32ChannelIndex) || (CSystemData::instance().GetChannelCount(hSystem) < pChannel->u32ChannelIndex) )
				return (CS_INVALID_CHANNEL);

			return (CSystemData::instance().GetChannelConfig(hSystem, nConfig, pChannel));
		}
		break;

		case CS_CHANNEL_ARRAY:
		{
			PARRAY_CHANNELCONFIG	pArrayChannels = NULL;

			if (::IsBadWritePtr(pData, sizeof(ARRAY_CHANNELCONFIG)))
				return (CS_INVALID_POINTER_BUFFER);

			pArrayChannels = (PARRAY_CHANNELCONFIG)pData;
			if (pArrayChannels->u32ChannelCount < 1 || pArrayChannels->u32ChannelCount > CSystemData::instance().GetChannelCount(hSystem))
				return (CS_INVALID_CHANNEL);

			return (CSystemData::instance().GetArrayChannelConfig(hSystem, nConfig, pArrayChannels));
		}
		break;

		case CS_TRIGGER:
		{
			PCSTRIGGERCONFIG pTrigger = NULL;

			pTrigger = (PCSTRIGGERCONFIG)pData;
			sizeStruct = pTrigger->u32Size;
			if (sizeStruct > sizeof(CSTRIGGERCONFIG) || 0 == sizeStruct)
				return (CS_INVALID_STRUCT_SIZE);

			if (::IsBadWritePtr(pData, sizeStruct))
				return (CS_INVALID_POINTER_BUFFER);

			if (!pTrigger->u32TriggerIndex || CSystemData::instance().GetTriggerCount(hSystem) < pTrigger->u32TriggerIndex)
				return (CS_INVALID_TRIGGER);

			return (CSystemData::instance().GetTriggerConfig(hSystem, nConfig, pTrigger));
		}
		break;

		case CS_TRIGGER_ARRAY:
		{
			PARRAY_TRIGGERCONFIG	pArrayTriggers = NULL;

			if (::IsBadWritePtr(pData, sizeof(ARRAY_TRIGGERCONFIG)))
				return (CS_INVALID_POINTER_BUFFER);

			pArrayTriggers = (PARRAY_TRIGGERCONFIG)pData;
			if (pArrayTriggers->u32TriggerCount < 1 || pArrayTriggers->u32TriggerCount > CSystemData::instance().GetTriggerCount(hSystem))
				return (CS_INVALID_TRIGGER);

			return (CSystemData::instance().GetArrayTriggerConfig(hSystem, nConfig, pArrayTriggers));
		}
		break;

		case CS_BOARD_INFO:
		{
			return (::CsDrvGetBoardsInfo(hSystem, static_cast<PARRAY_BOARDINFO>(pData)));

		}
		break;

		default:
		{
			return (::CsDrvGetParams(hSystem, nConfig, pData));
		}
		break;

	} // end switch(nIndex)
}


/*!
 \ingroup SSM_API

 \fn int32 SSM_API CsSet(CSHANDLE hSystem, int32 nIndex, const void* const pData);
	Assigns parameter values to the configuration setting variables in the driver.

 This method assigns parameter values to the configuration setting variables in the driver.
 It may be called several times to configure different settings. All the changes are accumulated by the driver and will be
 passed to the CompuScope hardware upon calling CsDo() with the ACTION_COMMIT parameter.  No parameter compatibility validation
 is performed by this method.  Parameter compatibility is only performed upon calling CsDo() with the ACTION_COMMIT parameter.

 \param hSystem	: Handle of the CompuScope system to be addressed. Obtained from CsGetSystem.
 \param nIndex	: Specifies the \link CONF_QUERY type \endlink of configuration settings to be modified.
 \param pData	: Pointer to the structure containing the configuration settings. Structures types should be:
         - CSACQUISITIONCONFIG  for nIndex = CS_ACQUISITION
         - CSCHANNELCONFIG      for nIndex = CS_CHANNEL
         - CSTRIGGERCONFIG      for nIndex = CS_TRIGGER

	\return
		- Positive value (>0) for success
		- Negative value (<0) for error
			Call CsGetErrorString with the error code to obtain a descriptive error string.

  Example:
 \code
	CSCHANNELCONFIG chConf;
	chConf.u32Size = sizeof(CSCHANNELCONFIG);
	chConf.u32ChannelIndex = 1;     //Channel A
	chConf.u32Term = CS_COUPLING_DC;
	chConf.u32InputRange = 2000;    //1 Vp-p
	chConf.u32Impedance = 50;
	CsSet(hSystem, CS_CHANNEL, &chConf);
 \endcode

 \sa CSACQUISITIONCONFIG, CSCHANNELCONFIG, CSTRIGGERCONFIG

*/
////////////////////////////////////////////////////////////////////
int32 SSM_API CsSet(CSHANDLE hSystem, int32 nIndex, const void* const pData)
{
	size_t sizeStruct = 0;

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	if (NULL == pData)
		return (CS_NULL_POINTER);

	// Only System card handle can be configured
	if (FS_BASE_HANDLE & (ULONG_PTR)hSystem)
		return (CS_INVALID_HANDLE);

	int32 i32RetCode = CSystemData::instance().ValidateSystemHandle(hSystem);
	if (CS_INVALID_HANDLE == i32RetCode)
		return (i32RetCode);

	// Validate Operation

	int32 i32PreviousStateId = CSystemData::instance().GetStateMachine(hSystem)->RetrieveStateID();

	if (!CSystemData::instance().PerformTransition(hSystem, MODIFY_TRANSITION))
		return (CS_INVALID_STATE);

	// Update Requested Structure
	switch(nIndex)
	{
		case CS_ACQUISITION:
		{
			sizeStruct = ((PCSACQUISITIONCONFIG)pData)->u32Size;
			if (sizeStruct > sizeof(CSACQUISITIONCONFIG))
				return (CS_INVALID_STRUCT_SIZE);

			if (::IsBadReadPtr(pData, sizeStruct))
				return (CS_INVALID_POINTER_BUFFER);

			CSystemData::instance().SetAcquisitionConfig(hSystem, (PCSACQUISITIONCONFIG)pData);
			return (CS_SUCCESS);
		}
		break;

		case CS_CHANNEL:
		{
			PCSCHANNELCONFIG pChannel = NULL;

			pChannel = (PCSCHANNELCONFIG)pData;
			sizeStruct = pChannel->u32Size;
			if (sizeStruct > sizeof(CSCHANNELCONFIG))
				return (CS_INVALID_STRUCT_SIZE);

			if (::IsBadReadPtr(pData, sizeStruct))
				return (CS_INVALID_POINTER_BUFFER);

			if (pChannel->u32ChannelIndex && CSystemData::instance().GetChannelCount(hSystem) < pChannel->u32ChannelIndex)
				return (CS_INVALID_CHANNEL);

			CSystemData::instance().SetChannelConfig(hSystem, pChannel);
			return (CS_SUCCESS);
		}
		break;

		case CS_CHANNEL_ARRAY:
		{
			PARRAY_CHANNELCONFIG	pArrayChannels = NULL;

			if (::IsBadReadPtr(pData, sizeof(ARRAY_CHANNELCONFIG)))
				return (CS_INVALID_POINTER_BUFFER);

			pArrayChannels = (PARRAY_CHANNELCONFIG)pData;
			if (pArrayChannels->u32ChannelCount < 1 || pArrayChannels->u32ChannelCount > CSystemData::instance().GetChannelCount(hSystem))
				return (CS_INVALID_CHANNEL);

			return (CSystemData::instance().SetArrayChannelConfig(hSystem, pArrayChannels));
		}
		break;

		case CS_TRIGGER:
		{
			PCSTRIGGERCONFIG pTrigger = NULL;

			pTrigger = (PCSTRIGGERCONFIG)pData;
			sizeStruct = pTrigger->u32Size;
			if (sizeStruct > sizeof(CSTRIGGERCONFIG))
				return (CS_INVALID_STRUCT_SIZE);

			if (::IsBadReadPtr(pData, sizeStruct))
				return (CS_INVALID_POINTER_BUFFER);

			if (!pTrigger->u32TriggerIndex || CSystemData::instance().GetTriggerCount(hSystem) < pTrigger->u32TriggerIndex)
				return (CS_INVALID_TRIGGER);

			CSystemData::instance().SetTriggerConfig(hSystem, pTrigger);
			return (CS_SUCCESS);
		}
		break;

		case CS_TRIGGER_ARRAY:
		{
			PARRAY_TRIGGERCONFIG	pArrayTriggers = NULL;

			if (::IsBadReadPtr(pData, sizeof(ARRAY_TRIGGERCONFIG)))
				return (CS_INVALID_POINTER_BUFFER);

			pArrayTriggers = (PARRAY_TRIGGERCONFIG)pData;
			if (pArrayTriggers->u32TriggerCount < 1 || pArrayTriggers->u32TriggerCount > CSystemData::instance().GetChannelCount(hSystem))
				return (CS_INVALID_TRIGGER);

			return (CSystemData::instance().SetArrayTriggerConfig(hSystem, pArrayTriggers));
		}
		break;

		default:
		{
			// Set the state machine back to Ready state
			if (!CSystemData::instance().PerformTransition(hSystem, COMMIT_TRANSITION))
				return (CS_INVALID_STATE);

			if (!CSystemData::instance().PerformTransition(hSystem, DONE_TRANSITION))
				return (CS_INVALID_STATE);

			if( SSM_DATAREADY_ID == i32PreviousStateId )
			{
				if (!CSystemData::instance().PerformTransition(hSystem, START_TRANSITION))
					return (CS_INVALID_STATE);

				if (!CSystemData::instance().PerformTransition(hSystem, HWEVENT_TRANSITION))
					return (CS_INVALID_STATE);

				if (!CSystemData::instance().PerformTransition(hSystem, HWEVENT_TRANSITION))
					return (CS_INVALID_STATE);
			}

			return (::CsDrvSetParams(hSystem, nIndex, (void*)pData));
		}
		break;
	}
}

//!
//! \}
//!

int32 SSM_API CsGetOptionText(uInt32 u32BoardType, int32 i32OptionType, PCS_HWOPTIONS_CONVERT2TEXT pOptionText)
{
	if (CS_FIRMWARE_OPTION_TYPE == i32OptionType)
		return ::_CsDrvGetFwOptionText(u32BoardType, pOptionText);
	else if (CS_HARDWARE_OPTION_TYPE == i32OptionType)
		return ::_CsDrvGetHwOptionText(u32BoardType, pOptionText);
	else
		return CS_INVALID_PARAMETER;
}

int32 SSM_API CsGetFwlCsiInfo(PFWLnCSI_INFO pBuffer, uInt32 *u32BufferSize)
{
	return (_CsDrvGetFwlCsiInfo(pBuffer, u32BufferSize));
}