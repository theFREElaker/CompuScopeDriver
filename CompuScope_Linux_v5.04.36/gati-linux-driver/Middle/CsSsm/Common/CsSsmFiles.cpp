#include "StdAfx.h"
#include "CsSSM.h"


//! \if DOCUMENTED
//! \fn int32 SSM_API CsOpenFile(cschar* szFilename, CsFileType Filetype, CsFileMode cfMode);
//!	\brief Open the specified signal file. 
//! 
//! \param szFilename	: Name of the file to open.
//! \param Filetype		: Filetype: cftSigFile or cftAscii. 
//! \param cfMode		: mode of file operation - cfmRead or cfmWrite
//!
//!	\return
//!		- Token to be passed to CsSaveFile
//!
//! \sa CsSaveFile, CsAttachFile
////////////////////////////////////////////////////////////////////
//! \endif
int32 SSM_API CsOpenFile(cschar* szFilename, CsFileType FileType, CsFileMode cfMode)
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	int32 RetCode = CS_SUCCESS;

	RetCode = CsFsOpen(szFilename, FileType, cfMode);

	return (RetCode);
}

//! \if DOCUMENTED
//! \fn int32 SSM_API CsSaveFile(CSHANDLE   hSystem, int32 i32Token, PIN_PARAMS_TRANSFERDATA pInData, POUT_PARAMS_TRANSFERDATA outData);
//!	\brief Save buffer content to file based on the requested filetype and close the file handle
//! 
//! \param hSystem		: Handle of system obtained from CsGetSystem
//! \param i32Token		: Token returned by 
//! \param pInData		: Structure describing trasfer parameters 
//! \param outData		: Structure describing trasfer results 
//!
//!	\return - status of operation
//!
//! \sa CsOpenFile
////////////////////////////////////////////////////////////////////
//! \endif
int32 SSM_API CsSaveFile(CSHANDLE hSystem, int32 , PIN_PARAMS_TRANSFERDATA , POUT_PARAMS_TRANSFERDATA )
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	// Cannot save a File System
	if (FS_BASE_HANDLE & hSystem)
		return (CS_INVALID_HANDLE);

	int32 i32RetCode = CSystemData::instance().ValidateSystemHandle(hSystem);
	if (CS_INVALID_HANDLE == i32RetCode)
		return (i32RetCode);

	i32RetCode = CS_INTERFACE_NOT_FOUND;

	return i32RetCode;
}

//! \if DOCUMENTED
//! \fn int32 SSM_API CsAttachFile(int32 i32Token, CsFileAttach fmMode,  CSHANDLE* phSystem);
//!	\brief Attaches file to the system handle of create new system handle.
//! 
//! \param i32Token		: Token returned by CsOpenFile
//! \param fmMode		: Mode of operation
//! \param phSystem		: Handle of system 
//!
//!	\return - staus of operation
//!
//! \sa CsOpenFile
////////////////////////////////////////////////////////////////////
//! \endif
int32 SSM_API CsAttachFile(int32 , CsFileAttach ,  CSHANDLE* )
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	//int32 i32RetCode = CSystemData::instance().ValidateSystemHandle(hSystem);
	//if (CS_INVALID_HANDLE == i32RetCode)
	//	return (i32RetCode);

	int32 i32RetCode = CS_INTERFACE_NOT_FOUND;

	return i32RetCode;
}