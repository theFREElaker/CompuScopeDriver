#ifdef _WIN32
#include "StdAfx.h"
#endif

#include "CsSsm.h"


// These calls are wrappers for the functions in FS.cpp

//!
//! \ingroup SSM_API
//!



//! \fn int32 SSM_API  CsConvertToSigHeader( PCSDISKFILEHEADER pHeader, PCSSIGSTRUCT pSigStruct, TCHAR* szComment, TCHAR* szName );
//! Creates a SIG file header from the supplied parameters.
//! 
//! This method can be used to create a header for a GageScope SIG file.
//! The method creates the header from the parameters the user supplies in CSSIGSTRUCT structure. It is up to the user 
//! to ensure that the parameters are correct. The data should then be saved immediately after the header in the file.
//! The method does not make contact with CompuScope hardware and so does not require CompuScope hardware to be installed 
//! in order to operate. The CompuScope drivers, however, must be installed.
//!
//! \param   pHeader          : Pointer to an empty CSDISKFILEHEADER buffer to return to the caller
//! \param   pSigStruct       : Pointer to a CSSIGSTRUCT structure which contains information about the file to be saved
//! \param   szComment        : String to put into the comment field of the SIG file header. If null, a default string is used
//! \param   szName		      : String to put into the name field of the SIG file header. If null, a default string is used
//!
//!	\return
//!		- Positive value (>0) for success.
//!		- Negative value (<0) for error.
//!			Call CsGetErrorString with the error code to obtain a descriptive error string.
//!
//! \sa CsConvertFromSigHeader, CSDISKFILEHEADER, CSSIGSTRUCT
////////////////////////////////////////////////////////////////////
int32 SSM_API CsConvertToSigHeader(PCSDISKFILEHEADER pHeader, PCSSIGSTRUCT pSigStruct, TCHAR* szComment, TCHAR* szName)
{
	return ConvertToSigHeader(pHeader, pSigStruct, szComment, szName);
}


//!
//! \ingroup SSM_API
//!


//! \fn int32 SSM_API  CsConvertFromSigHeader( PCSDISKFILEHEADER pHeader, PCSSIGSTRUCT pSigStruct, TCHAR* szComment, TCHAR* szName );
//! Extracts signal parameters from the supplied SIG file header.
//! 
//! This method can be used to read a header from a GageScope SIG file and convert it into a CSSIGSTRUCT structure.
//! The method reads the user-supplied SIG file header and returns the relevant information in the supplied CSSIGSTRUCT structure.
//! The method does not make contact with CompuScope hardware and so does not require CompuScope hardware to be installed in order to operate.
//! The CompuScope drivers, however, must be installed.
//!
//! \param   pHeader          : Pointer to a filled in CSDISKFILEHEADER buffer read from a SIG file
//! \param   pSigStruct       : Pointer to an empty CSSIGSTRUCT structure which will be filled in by the driver
//! \param   szComment        : Empty string in which to put the comment field from the SIG file header. If null, the parameter is ignored
//! \param   szName           : Empty string in which to put the name field from the SIG file header. If null, the parameter is ignored
//!
//!	\return
//!		- Positive value (>0) for success.
//!		- Negative value (<0) for error.
//!			Call CsGetErrorString with the error code to obtain a descriptive error string.
//!
//! \sa CsConvertToSigHeader, CSDISKFILEHEADER, CSSIGSTRUCT
////////////////////////////////////////////////////////////////////
int32 SSM_API CsConvertFromSigHeader(PCSDISKFILEHEADER pHeader, PCSSIGSTRUCT pSigStruct, TCHAR* szComment, TCHAR* szName)
{
	return ConvertFromSigHeader(pHeader, pSigStruct, szComment, szName);
}

//!
//! \ingroup SSM_API
//!

//! \fn int32 SSM_API  CsRetrieveChannelFromRawBuffer( PVOID pMulrecRawDataBuffer, int64 i64RawDataBufferSize, uInt32 u32SegmentId, uInt16 u16ChannelIndex, int64 i64Start,int64 i64Length, PVOID pvNormalizedDataBuffer, int64* pi64TrigTimeStamp, int64* pi64ActualStart, int64* pi64ActualLength );
//! Retrieves normalized data from Raw data buffer.
//! 
//! This method is used by the Advanced Multiple Record sample program within the CompuScope C/C# SDK.
//! The method extracts waveform data for a single specified Multiple Record segment from a data buffer, to which pRawDataBuffer points,
//! that contains raw data for all records acquired in a Multiple Record acquisition.  The output data for a single record are placed
//! in an output buffer, to which pDataBuffer points.
//! The method does not make contact with CompuScope hardware and so does not require CompuScope hardware to be installed in order to operate.
//! The CompuScope drivers, however, must be installed.
//!
//! \param   pMulrecRawDataBuffer   : Pointer to buffer that contains all raw data from a complete Multiple Record acquisition
//! \param   i64RawDataBufferSize   : Size in Bytes of buffer to which pRawDataBuffer points
//! \param   u32SegmentId           : Multiple Record segment number for which data are to be extracted
//! \param   u16ChannelIndex        : Channel number for which data are to be extracted
//! \param   i64Start               : Starting address of the record for which data are to be extracted. Address 0 is the Trigger position
//! \param   i64Length              : Length in Samples of the record for which data are to be extracted
//! \param   pvNormalizedDataBuffer : Pointer to buffer that will contain data for the extracted Multiple Record segment
//! \param   pi64TrigTimeStamp      : Pointer to variable that will be filled with Time-Stamp counter value for the extracted Multiple Record
//! \param   pi64ActualStart        : Pointer to variable that will be filled with the actual start of the extracted Multiple Record
//! \param   pi64ActualLength       : Pointer to variable that will be filled with the actual length in Samples of the extracted Multiple Record
//!
//!	\return
//!		- Positive value (>0) for success.
//!		- Negative value (<0) for error.
//!			Call CsGetErrorString with the error code to obtain a descriptive error string.
//!
//! \sa CsTransfer, IN_PARAMS_TRANSFERDATA, OUT_PARAMS_TRANSFERDATA
////////////////////////////////////////////////////////////////////

int32 SSM_API CsRetrieveChannelFromRawBuffer  ( PVOID	pMulrecRawDataBuffer,
												int64	i64RawDataBufferSize,
												uInt32	u32SegmentId,
												uInt16	ChannelIndex,
												int64	i64Start,
												int64	i64Length,
												PVOID	pNormalizedDataBuffer,
												int64*	pi64TrigTimeStamp,
												int64*	pi64ActualStart,
												int64*	pi64ActualLength )
{
	return RetrieveChannelFromRawBuffer( pMulrecRawDataBuffer, 
										 i64RawDataBufferSize, 
										 u32SegmentId, 
										 ChannelIndex,
										 i64Start,
										 i64Length,
										 pNormalizedDataBuffer,
										 pi64TrigTimeStamp,
										 pi64ActualStart,
										 pi64ActualLength );
}
