#include "stdafx.h"
#include "CsAdqUpdate.h"
#include "CsiHeader.h"
#include "CsSpDevice.h"
#include "CsUsbmsglog.h"
#include <stdio.h>

#define ADQ_UPDATE_DLL _T("CsUsbUpdateApi.dll")


CsAdqUpdate::CsAdqUpdate(bool bForceUpdateAlgo, bool bForceUpdateComm) : m_bForceUpdateAlgo(bForceUpdateAlgo), 
																		 m_bForceUpdateComm(bForceUpdateComm),
																		 m_pControl(NULL),
																		 m_nDev(0),
																		 m_hDllMod(NULL)
{
	m_hDllMod = ::LoadLibrary(ADQ_UPDATE_DLL);

	if ( NULL == m_hDllMod )
	{
		throw( CsSpdevException(_T("Cannot load")ADQ_UPDATE_DLL) );
	}

	m_pfCreateControl = (_AdqCreateControlUnit*)::GetProcAddress(m_hDllMod, "CreateADQControlUnit" );
	if ( !m_pfCreateControl )
	{
		::FreeLibrary(m_hDllMod);
		throw ( CsSpdevException(_T("Cannot resolve CreateADQControlUnit")) );
	}

	m_pfDeleteControl = (_AdqDeleteControlUnit*)::GetProcAddress(m_hDllMod, "DeleteADQControlUnit" );
	if ( !m_pfDeleteControl )
	{
		::FreeLibrary(m_hDllMod);
		throw( CsSpdevException(_T("Cannot resolve DeleteADQControlUnit")) );
	}

	m_pfFindDevices = (_AdqFindDevices*)::GetProcAddress(m_hDllMod, "ADQControlUnit_FindDevices" );
	if ( !m_pfFindDevices )
	{
		::FreeLibrary(m_hDllMod);
		throw( CsSpdevException(_T("Cannot resolve ADQControlUnit_FindDevices")) );
	}

	m_pfEnableSpi = (_AdqEnableSpi*)::GetProcAddress(m_hDllMod, "ADQ_EnableSpi" );
	if ( !m_pfEnableSpi )
	{
		::FreeLibrary(m_hDllMod);
		throw( CsSpdevException(_T("Cannot resolve ADQ_EnableSpi")) );
	}
	m_pfDisableSpi = (_AdqDisableSpi*)::GetProcAddress(m_hDllMod, "ADQ_DisableSpi" );
	if ( !m_pfDisableSpi )
	{
		::FreeLibrary(m_hDllMod);
		throw( CsSpdevException(_T("Cannot resolve ADQ_DisableSpi")) );
	}
	m_pfFlashUploadStart = (_AdqFlashUploadStart*)::GetProcAddress(m_hDllMod, "ADQ_FlashUploadStart" );
	if ( !m_pfFlashUploadStart )
	{
		::FreeLibrary(m_hDllMod);
		throw( CsSpdevException(_T("Cannot resolve ADQ_FlashUploadStart")) );
	}
	m_pfFlashUploadPart = (_AdqFlashUploadPart*)::GetProcAddress(m_hDllMod, "ADQ_FlashUploadPart" );
	if ( !m_pfFlashUploadPart )
	{
		::FreeLibrary(m_hDllMod);
		throw( CsSpdevException(_T("Cannot resolve ADQ_FlashUploadPart")) );
	}
	m_pfFlashUploadEnd = (_AdqFlashUploadEnd*)::GetProcAddress(m_hDllMod, "ADQ_FlashUploadEnd" );
	if ( !m_pfFlashUploadEnd )
	{
		::FreeLibrary(m_hDllMod);
		throw( CsSpdevException(_T("Cannot resolve ADQ_FlashUploadEnd")) );
	}
	m_pfFlashDownloadStart = (_AdqFlashDownloadStart*)::GetProcAddress(m_hDllMod, "ADQ_FlashDownloadStart" );
	if ( !m_pfFlashDownloadStart )
	{
		::FreeLibrary(m_hDllMod);
		throw( CsSpdevException(_T("Cannot resolve ADQ_FlashDownloadStart")) );
	}
	m_pfFlashDownloadPart = (_AdqFlashDownloadPart*)::GetProcAddress(m_hDllMod, "ADQ_FlashDownloadPart" );
	if ( !m_pfFlashDownloadPart )
	{
		::FreeLibrary(m_hDllMod);
		throw( CsSpdevException(_T("Cannot resolve ADQ_FlashDownloadPart")) );
	}
	m_pfFlashDownloadEnd = (_AdqFlashDownloadEnd*)::GetProcAddress(m_hDllMod, "ADQ_FlashDownloadEnd" );
	if ( !m_pfFlashDownloadEnd )
	{
		::FreeLibrary(m_hDllMod);
		throw( CsSpdevException(_T("Cannot resolve ADQ_FlashDownloadEnd")) );
	}
	m_pfGetPid = (_AdqGetPid*)::GetProcAddress(m_hDllMod, "ADQ_GetPID" );
	if ( !m_pfGetPid )
	{
		::FreeLibrary(m_hDllMod);
		throw( CsSpdevException(_T("Cannot resolve ADQ_GetPID")) );
	}

	m_pfGetRevision = (_AdqGetRevision*)::GetProcAddress(m_hDllMod, "ADQ_GetRevision" );
	if ( !m_pfGetRevision )
	{
		::FreeLibrary(m_hDllMod);
		throw( CsSpdevException(_T("Cannot resolve ADQ_GetRevision")) );
	}
}

CsAdqUpdate::~CsAdqUpdate(void)
{
	if ( NULL != m_hDllMod )
	{
		::FreeLibrary(m_hDllMod);
	}
}


int32 CsAdqUpdate::Initialize(void)
{
	m_pControl = m_pfCreateControl();
	m_nDev = m_pfFindDevices( m_pControl );
	return m_nDev;
}


int CsAdqUpdate::GetProductName(int nDevNum)
{
	int nPid = m_pfGetPid(m_pControl, nDevNum);
	switch ( nPid )
	{
		case 0x0001: return 214;
		case 0x0003: return 114;
		case 0x0005: return 112;
		case 0x000E: return 108;
		default:     return 0;
	}
}



int32 CsAdqUpdate::UpdateFirmware(int nDevNum)
{
	// determine if the firmware needs to be updated
	int32 i32Ret = CS_SUCCESS;

	CSI_FILE_HEADER header;
	CSI_ENTRY		CsiEntry;
	int* pnRev = NULL;

	pnRev = m_pfGetRevision(m_pControl, nDevNum);
	// don't have to check because if the result is bad we'll update the firmware anyways
	// we do need to check for a null pointer
	if( NULL == pnRev || IsBadReadPtr(pnRev, sizeof(int)*4 ) )
	{
#ifndef CS_WIN64
		TRACE ( DRV_TRACE_LEVEL, "Could not get firmware revision from USB device\n");
#endif
		// report to event log
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_TEXT, _T("Could not get firmware revision from USB device"));
		return CS_NULL_POINTER;
	}
	
	// Convert to Gage versions

	FPGA_FWVERSION fwBbVersion = {0};
	FPGA_FWVERSION fwAddonVersion = {0};
	fwBbVersion.Version.uMinor = (pnRev[3] & 0xff00)>>8;
	fwBbVersion.Version.uRev = pnRev[3] & 0x00ff;

	fwAddonVersion.Version.uMinor = (pnRev[0] & 0xff00)>>8;
	fwAddonVersion.Version.uRev = pnRev[0] & 0x00ff;


	size_t tTypeCount;
	const SpDevDef* pDevType = CsSpDeviceManager::GetDevTypeTable(&tTypeCount);
	int nDeviceType = GetProductName(nDevNum);

	for ( int i = 0; i < int(tTypeCount); i++ )
	{
		if ( nDeviceType == pDevType[i].nAdqType )
		{
			::GetSystemDirectory(m_szCsiName, MAX_PATH);
			::lstrcat(m_szCsiName, _T("\\drivers\\"));
			::lstrcat(m_szCsiName, pDevType[i].strCsName);
			::lstrcat(m_szCsiName, _T("U")); //only USB board are currently supported
			::lstrcat(m_szCsiName, _T(".csi"));
			break;
		}
	}	
	
	// the csi file has already been read and validated so these checks may be unnecessary
	if ( 0 == ::lstrcmp(m_szCsiName, _T("")) )
	{
		TCHAR szError[MAX_PATH];
		::_stprintf_s(szError, MAX_PATH, _T("Cannot find %s"), m_szCsiName);
#ifndef CS_WIN64
		TRACE ( DRV_TRACE_LEVEL, "%s\n", szError);
#endif
		// report to event log
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_CSI_FILE, szError);
		return CS_FALSE; // not success but we don't want to abort the program
	}

	HANDLE hCsiFile = ::CreateFile(m_szCsiName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if ( INVALID_HANDLE_VALUE == hCsiFile )
	{
		TCHAR szError[MAX_PATH];
		::_stprintf_s(szError, MAX_PATH, _T("Error opening %s"), m_szCsiName);
#ifndef CS_WIN64
		TRACE ( DRV_TRACE_LEVEL, "%s\n", szError);
#endif
		// report to event log
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_CSI_FILE, szError);
		return CS_FALSE; // not success but we don't want to abort the program
	}
	DWORD dwBytesRead;
	if ( !::ReadFile(hCsiFile, &header, sizeof(header), &dwBytesRead, NULL) )
	{
		TCHAR szError[MAX_PATH];
		::_stprintf_s(szError, MAX_PATH, _T("Error reading %s"), m_szCsiName);
#ifndef CS_WIN64
		TRACE ( DRV_TRACE_LEVEL, "%s\n", szError);
#endif
		// report to event log
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_CSI_FILE, szError);
		return CS_FALSE; // not success but we don't want to abort the program
	}
	
	bool bUpdateAlgo = false;
	bool bUpdateComm = false;
	bool bUpdateUsb = false;
	FPGA_FWVERSION u32NewBbVersion = {0};
	FPGA_FWVERSION u32NewAddonVersion = {0};

	for( uInt32 i = 0; i < header.u32NumberOfEntry; i++ )
	{
		if ( 0 == ::ReadFile( hCsiFile, &CsiEntry, sizeof(CsiEntry), &dwBytesRead, NULL) )
		{
			TCHAR szError[MAX_PATH];
			::_stprintf_s(szError, MAX_PATH, _T("Error reading %s"), m_szCsiName);
#ifndef CS_WIN64
			TRACE ( DRV_TRACE_LEVEL, "%s\n", szError);
#endif
			// report to event log
			CsEventLog EventLog;
			EventLog.Report(CS_ERROR_CSI_FILE, szError);
			return CS_FALSE; // not success but we don't want to abort the program
		}
		
		if( CsiEntry.u32Target == CSI_USB_BB_TARGET )
		{
			if ( m_bForceUpdateAlgo || (int(CsiEntry.u32FwVersion) != fwBbVersion.u32Reg) )
			{
				// Update
				bUpdateAlgo = true;
				u32NewBbVersion.u32Reg = CsiEntry.u32FwVersion;
			}
		}
		else if ( CsiEntry.u32Target == GetAddOnCsiTarget( nDeviceType ) ) // addon
		{
			// if it's the USB firmware entry, just continue (for now)
			if ( (CsiEntry.u32FwOptions & CS_OPTIONS_USB_FIRMWARE_MASK) == CS_OPTIONS_USB_FIRMWARE_MASK )
			{
				continue;
			}
			if ( m_bForceUpdateComm || int(CsiEntry.u32FwVersion) != fwAddonVersion.u32Reg ) // make it the usb version
			{
				// Update
				bUpdateComm = true;
				u32NewAddonVersion.u32Reg = CsiEntry.u32FwVersion;
				// need some logic to determine if the usb firmware should be updated
			}
		}
		else
		{
			continue;
		}
	}

	::CloseHandle( hCsiFile );

	if ( !bUpdateAlgo && !bUpdateComm )
	{
		// nothing to be done
		return CS_SUCCESS;
	}

	if ( bUpdateAlgo )
	{
		TCHAR szMessage[MAX_PATH];
		::_stprintf_s(szMessage, MAX_PATH, _T("Upgrading Alg Fpga from v. 0.%02d.%02d.0 (r%4d) to 0.%02d.%02d (r%4d)"), 
										fwBbVersion.Version.uMinor, fwBbVersion.Version.uRev, (fwBbVersion.Version.uMinor<<8) |fwBbVersion.Version.uRev, 
										u32NewBbVersion.Version.uMinor, u32NewBbVersion.Version.uRev, (u32NewBbVersion.Version.uMinor<<8) | u32NewBbVersion.Version.uRev);
		CsEventLog EventLog;
		EventLog.Report(CS_INFO_TEXT, szMessage);

		i32Ret = UpdateFpga( nDevNum, 1);
		// errors are reported in UpdateFpga

		if ( CS_SUCCEEDED(i32Ret) )
		{
			// report to event log
			EventLog.Report(CS_INFO_TEXT, _T("Alg upgrade successful"));
		}
	}
	if ( bUpdateComm )
	{
		TCHAR szMessage[MAX_PATH];
		::_stprintf_s(szMessage, MAX_PATH, _T("Upgrading Comm Fpga from v. 0.%02d.%02d.0 (r%4d) to 0.%02d.%02d (r%4d)"), 
											fwAddonVersion.Version.uMinor, fwAddonVersion.Version.uRev, (fwAddonVersion.Version.uMinor<<8) |fwAddonVersion.Version.uRev, 
											u32NewAddonVersion.Version.uMinor, u32NewAddonVersion.Version.uRev, (u32NewAddonVersion.Version.uMinor<<8) | u32NewAddonVersion.Version.uRev);
		CsEventLog EventLog;
		EventLog.Report(CS_INFO_TEXT, szMessage);

		i32Ret = UpdateFpga( nDevNum, 2);
		// errors are reported in UpdateFpga
		// report to event log
		if ( CS_SUCCEEDED(i32Ret) )
		{
			// report to event log
			EventLog.Report(CS_INFO_TEXT, _T("Comm upgrade successful"));
		}
	}
	if ( bUpdateUsb )
	{
		// to be added later
	}
	return CS_SUCCESS;
}


int32 CsAdqUpdate::UpdateFpga(int nDevNum, int nFpgaNum)
{
	CSI_FILE_HEADER header;
	CSI_ENTRY		CsiEntry;

	uInt32			u32ImageOffset = 0;
	uInt32			u32ImageSize = 0;
	int32			i32Ret = CS_SUCCESS;
	FILE*			fp;

	int32 i32ByteCount = 0;
	uInt32 u32NumBytes = 0;
	uInt32 u32LowAddress = 0;
	uInt32 u32HighAddress = 0;
	uInt32 u32RowType = 0;
	uInt32 u32FileChecksum = 0;
	uInt32 u32Checksum = 0;
	uInt32 u32AddressCount = 0;
	uInt32 u32LineCount = 0;
	unsigned char ucData[PACKET_LENGTH];
	unsigned int nTempData = 0;

	errno_t err = ::fopen_s( &fp, m_szCsiName, "r" );
	if ( 0 != err )
	{
		TCHAR szError[MAX_PATH];
		::_stprintf_s(szError, MAX_PATH, _T("Cannot open %s"), m_szCsiName);
#ifndef CS_WIN64
		TRACE ( DRV_TRACE_LEVEL, "%s\n", szError);
#endif
		// report to event log
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_CSI_FILE, szError);
		return CS_FALSE; // not success but we don't want to abort the program
	}
	
	if ( 1 != ::fread( &header, sizeof(CSI_FILE_HEADER), 1, fp ) )
	{
		TCHAR szError[MAX_PATH];
		::_stprintf_s(szError, MAX_PATH, _T("Cannot read file header from %s"), m_szCsiName);
#ifndef CS_WIN64
		TRACE ( DRV_TRACE_LEVEL, "%s\n", szError);
#endif
		// report to event log
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_CSI_FILE, szError);
		return CS_FALSE; // not success but we don't want to abort the program
	}

	// nFpgaNum == 1 is the baseboard, 2 is the Addon
	uInt32 u32Target = ( 1 == nFpgaNum ) ? CSI_USB_BB_TARGET : GetAddOnCsiTarget(GetProductName(nDevNum));

	for( uInt32 i = 0; i < header.u32NumberOfEntry; i++ )
	{
		if ( 1 != ::fread( &CsiEntry, sizeof(CSI_ENTRY), 1, fp ) )
		{
			TCHAR szError[MAX_PATH];
			::_stprintf_s(szError, MAX_PATH, _T("Cannot read CSI entry from %s"), m_szCsiName);
#ifndef CS_WIN64
			TRACE ( DRV_TRACE_LEVEL, "%s\n", szError);
#endif
			// report to event log
			CsEventLog EventLog;
			EventLog.Report(CS_ERROR_CSI_FILE, szError);
			return CS_FALSE; // not success but we don't want to abort the program
		}
		if ( (u32Target == CsiEntry.u32Target) && !(CsiEntry.u32FwOptions & CS_OPTIONS_USB_FIRMWARE_MASK) )
		{
			u32ImageSize = CsiEntry.u32ImageSize;
			u32ImageOffset = CsiEntry.u32ImageOffset;
			break;
		}
		else
		{
			continue;
		}
	}
	
	if ( 0 == u32ImageSize )
	{
		TCHAR szError[MAX_PATH];
		::_stprintf_s(szError, MAX_PATH, _T("Image size is 0 in %s"), m_szCsiName);
#ifndef CS_WIN64
		TRACE ( DRV_TRACE_LEVEL, "%s\n", szError);
#endif
		// report to event log
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_CSI_FILE, szError);
		return CS_FALSE; // not success but we don't want to abort the program
	}

	// move back to the right place
	if ( 0 != fseek( fp, u32ImageOffset, SEEK_SET ) )
	{
		TCHAR szError[MAX_PATH];
		::_stprintf_s(szError, MAX_PATH, _T("Error seeking in %s"), m_szCsiName);
#ifndef CS_WIN64
		TRACE ( DRV_TRACE_LEVEL, "%s\n", szError);
#endif
		// report to event log
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_CSI_FILE, szError);
		return CS_FALSE; // not success but we don't want to abort the program
	}

	::Sleep(200); // ? was in ADQUploader
	m_pfEnableSpi(m_pControl, nDevNum);

	m_pfFlashUploadStart(m_pControl, nDevNum, nFpgaNum, 0);

	int nSizeToGo = int(u32ImageSize);
	while ( nSizeToGo > 0 ) 
	{
		u32LineCount ++;
		u32Checksum = 0;

		// read file and decrement nSizeToGo by the number of bytes we've read
		if ( 0 == ::fscanf_s( fp, ":%2X%4X%2X", &u32NumBytes, &u32LowAddress, &u32RowType, 9 ) ) // Unexpected structure in file
		{
			TCHAR szError[MAX_PATH];
			::_stprintf_s(szError, MAX_PATH, _T("Unexpected structure in %s"), m_szCsiName);
#ifndef CS_WIN64
			TRACE ( DRV_TRACE_LEVEL, "%s\n", szError);
#endif
			// report to event log
			CsEventLog EventLog;
			EventLog.Report(CS_ERROR_TEXT, szError);

			i32Ret = CS_MISC_ERROR; 
			break;
		}
		nSizeToGo -= 9;

		if (4 == u32RowType )		//address extension row (u32HighAddress in data field)
		{
			if ( 0 == ::fscanf_s( fp, "%4X%2X\n", &u32HighAddress, &u32FileChecksum ) )//Unexpected structure in file
			{
				// report to event log
				TCHAR szError[MAX_PATH];
				::_stprintf_s(szError, MAX_PATH, _T("Unexpected structure in %s"), m_szCsiName);
#ifndef CS_WIN64
				TRACE ( DRV_TRACE_LEVEL, "%s\n", szError);
#endif
				CsEventLog EventLog;
				EventLog.Report(CS_ERROR_TEXT, szError);
				i32Ret = CS_MISC_ERROR; 
				break;
			}
			nSizeToGo -= 8;
			u32Checksum += (u32HighAddress & 0xff) + (u32HighAddress >> 8);			
		}
		else if ( 1 == u32RowType )//End of file marker. The file must end with one of those
		{
			//send packet
			m_pfFlashUploadPart( m_pControl, nDevNum, i32ByteCount, (char*)ucData);
			nSizeToGo -= 4;
			break;
		}
		else if ( 0 == u32RowType ) //data
		{
			if ( ((u32AddressCount & 0xffff) != u32LowAddress) )// || ((u32AddressCount >> 16) != u32HighAddress))//Unexpected structure in file
			{
				// ?? error or the end
				break;
			}
			for ( int j = 0; j < int(u32NumBytes); j++ ) //read data from row and put in buffer
			{
				if ( i32ByteCount >= PACKET_LENGTH )
				{
					m_pfFlashUploadPart( m_pControl, nDevNum, i32ByteCount, (char*)ucData ); // send packet
					i32ByteCount = 0;
				}
				if ( 0 == ::fscanf_s( fp, "%02hX", &nTempData ) )//Unexpected structure in file
				{
					// report to event log
					TCHAR szError[MAX_PATH];
					::_stprintf_s(szError, MAX_PATH, _T("Unexpected structure in %s"), m_szCsiName);

#ifndef CS_WIN64
					TRACE ( DRV_TRACE_LEVEL, "%s\n", szError);
#endif
					CsEventLog EventLog;
					EventLog.Report(CS_ERROR_TEXT, szError);
					i32Ret = CS_MISC_ERROR; 
					break;
				}
				nSizeToGo -= 2;

				ucData[i32ByteCount] = nTempData & 0xff; //Scanf gives short int. We need unsigned char
				u32Checksum += ucData[i32ByteCount];
				i32ByteCount++;	
			}
			if ( 0 == ::fscanf_s( fp, "%2X\n", &u32FileChecksum ) )//Unexpected structure in file
			{
				// report to event log
				TCHAR szError[MAX_PATH];
				::_stprintf_s(szError, MAX_PATH, _T("Unexpected structure in %s"), m_szCsiName);
#ifndef CS_WIN64
				TRACE ( DRV_TRACE_LEVEL, "%s\n", szError);
#endif
				CsEventLog EventLog;
				EventLog.Report(CS_ERROR_TEXT, szError);				
				i32Ret = CS_MISC_ERROR;
				break;
			}
			nSizeToGo -= 4;
			u32AddressCount += u32NumBytes;
		}
		else
		{
			i32Ret = CS_MISC_ERROR;
			break;// Not a valid row-type
		}
		u32Checksum += (u32LowAddress & 0xff) + (u32LowAddress >> 8) + u32NumBytes + u32RowType;
		u32Checksum = (1 + ~u32Checksum) & 0xff;
		if (u32Checksum != u32FileChecksum)
		{
			// report to event log
			TCHAR szError[MAX_PATH];
			::_stprintf_s(szError, MAX_PATH, _T("Checksum error writing to firmware"));
#ifndef CS_WIN64
			TRACE ( DRV_TRACE_LEVEL, "%s\n", szError);
#endif
			CsEventLog EventLog;
			EventLog.Report(CS_ERROR_TEXT, szError);
			i32Ret = CS_MISC_ERROR; 
			break;
		}
	}
	::fclose( fp );

	m_pfFlashUploadEnd( m_pControl, nDevNum );	//VR_FLASH_UPLOAD_END
//	m_pfDisableSpi( m_pControl, nDevNum );	// moved to bottom of file - May 12, 2010
	if ( CS_SUCCESS == i32Ret )
	{
		i32Ret = VerifyBitfile(nDevNum, nFpgaNum, u32ImageOffset, u32ImageSize);
	}
	m_pfDisableSpi( m_pControl, nDevNum );

	return i32Ret;
}

int32 CsAdqUpdate::VerifyBitfile(int nDevNum, int nFpgaNum, uInt32 u32ImageOffset, uInt32 u32ImageSize)
{
	int32			i32Ret = CS_SUCCESS;
	FILE*			fp;

	int32 i32ByteCount = 0;
	uInt32 u32NumBytes = 0;
	uInt32 u32LowAddress = 0;
	uInt32 u32HighAddress = 0;
	uInt32 u32RowType = 0;
	uInt32 u32FileChecksum = 0;
	uInt32 u32Checksum = 0;
	uInt32 u32AddressCount = 0;
	uInt32 u32LineCount = 0;
	unsigned char ucFileData[PACKET_LENGTH];
	unsigned char ucFlashData[PACKET_LENGTH];

	uInt32 u32TempData = 0;

	uInt32 u32MismatchOffset = 0;


	::ZeroMemory(ucFileData, PACKET_LENGTH);
	::ZeroMemory(ucFlashData, PACKET_LENGTH);

	errno_t err = ::fopen_s( &fp, m_szCsiName, "r" );
	if ( 0 != err )
	{
		TCHAR szError[MAX_PATH];
		::_stprintf_s(szError, MAX_PATH, _T("Error opening %s while verifying"), m_szCsiName);
#ifndef CS_WIN64
		TRACE ( DRV_TRACE_LEVEL, "%s\n", szError );
#endif
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_CSI_FILE, szError);
		return CS_FALSE; // not success but we don't want to abort the program
	}
	// move back to the right place
	if ( 0 != fseek( fp, u32ImageOffset, SEEK_SET ) )
	{
		TCHAR szError[MAX_PATH];
		::_stprintf_s(szError, MAX_PATH, _T("Error reading %s while verifying"), m_szCsiName);
#ifndef CS_WIN64
		TRACE ( DRV_TRACE_LEVEL, "%s\n", szError );
#endif
		CsEventLog EventLog;
		EventLog.Report(CS_ERROR_CSI_FILE, szError);
		return CS_FALSE; // not success but we don't want to abort the program
	}

//	Commented out May 12, 2010. There seems to be a problem if you set a breakpoint before
//  the call to EnableSpi the call to FlashDownloadStart fails. Seems to be fixed by removing the
//  moving the DisableSpi call in WriteFpga and commenting out the EnableSpi and DisableSpi calls 
//  in this routine.  This way we call EnableSpi at the start of WriteFpga and DisableSpi once we're
//  done with writing and verifying

//	m_pfEnableSpi(m_pControl, nDevNum); //Enable SPI and put FPGA into reset 
	m_pfFlashDownloadStart(m_pControl, nDevNum, nFpgaNum, 0);

	int nSizeToGo = int(u32ImageSize);
	
	//Parse file and compare data
	while ( nSizeToGo > 0 ) 
	{
		u32LineCount ++;
		u32Checksum = 0;
		if ( 0 == ::fscanf_s( fp, ":%2X%4X%2X", &u32NumBytes, &u32LowAddress, &u32RowType, 9 ) ) // Unexpected structure in file
		{
			TCHAR szError[MAX_PATH];
			::_stprintf_s(szError, MAX_PATH, _T("Unexpected structure in %s while verifying at line %d"), m_szCsiName, u32LineCount);
#ifndef CS_WIN64
			TRACE ( DRV_TRACE_LEVEL, "%s\n", szError );
#endif
			CsEventLog EventLog;
			EventLog.Report(CS_ERROR_TEXT, szError);
			i32Ret = CS_MISC_ERROR; 
			break;
		}
		nSizeToGo -= 9;
	
		if ( 4 == u32RowType ) //address extension row (u32HighAddress in data field)
		{
			if ( ::fscanf_s(fp, "%4X%2X\n", &u32HighAddress, &u32FileChecksum) == 0 )//Unexpected structure in file
			{
				TCHAR szError[MAX_PATH];
				::_stprintf_s(szError, MAX_PATH, _T("Unexpected structure in %s while verifying at line %d"), m_szCsiName, u32LineCount);
#ifndef CS_WIN64
				TRACE ( DRV_TRACE_LEVEL, "%s\n", szError );
#endif
				CsEventLog EventLog;
				EventLog.Report(CS_ERROR_TEXT, szError);
				i32Ret = CS_MISC_ERROR; 
				break;
			}
			nSizeToGo -= 8;
			u32Checksum += (u32HighAddress & 0xff) + (u32HighAddress >> 8);
		}
		else if ( 1 == u32RowType )	//End of file marker. The file must end with one of those
		{
			//	get data from flash.
			m_pfFlashDownloadPart(m_pControl, nDevNum, i32ByteCount, (char*)ucFlashData);
			nSizeToGo -= 2;
			if ( !CompareData(ucFileData, ucFlashData, i32ByteCount, &u32MismatchOffset) )
			{	
				TCHAR szError[MAX_PATH];
				::_stprintf_s(szError, MAX_PATH, _T("Mismatch in %s firmware data - address: 0x%08X"), nFpgaNum == 1 ? "algo": "comm", u32AddressCount - PACKET_LENGTH + u32MismatchOffset);
#ifndef CS_WIN64
				TRACE ( DRV_TRACE_LEVEL, "%s\n", szError );
#endif
				CsEventLog EventLog;
				EventLog.Report(CS_ERROR_TEXT, szError);
				i32Ret = CS_MISC_ERROR; 
			}
			break;
		}
		else if ( 0 == u32RowType ) //data
		{
			if ( ((u32AddressCount & 0xffff) != u32LowAddress) )	//Unexpected structure in file
			{
				TCHAR szError[MAX_PATH];
				::_stprintf_s(szError, MAX_PATH, _T("Unexpected structure in %s while verifying at line %d"), m_szCsiName, u32LineCount);
#ifndef CS_WIN64
				TRACE ( DRV_TRACE_LEVEL, "%s\n", szError );
#endif
				CsEventLog EventLog;
				EventLog.Report(CS_ERROR_TEXT, szError);
				i32Ret = CS_MISC_ERROR;
				break;
			}
			if ( i32ByteCount >= PACKET_LENGTH )
			{
				m_pfFlashDownloadPart(m_pControl, nDevNum, i32ByteCount, (char*)ucFlashData);
				if ( !CompareData(ucFileData, ucFlashData, i32ByteCount, &u32MismatchOffset) )
				{

					TCHAR szError[MAX_PATH];
					::_stprintf_s(szError, MAX_PATH, _T("Mismatch in %s firmware data - address: 0x%08X"), nFpgaNum == 1 ? "algo": "comm", u32AddressCount - PACKET_LENGTH + u32MismatchOffset);
#ifndef CS_WIN64
					TRACE ( DRV_TRACE_LEVEL, "%s\n", szError);
#endif
					CsEventLog EventLog;
					EventLog.Report(CS_ERROR_TEXT, szError);
					i32Ret = CS_MISC_ERROR;
					break;
				}
				i32ByteCount = 0;
			}
			for ( uInt32 j = 0; j < u32NumBytes; j++ ) //read data from row and put in buffer
			{
				if ( ::fscanf_s(fp, "%02hX", &u32TempData ) == 0)//Unexpected structure in file
				{
					TCHAR szError[MAX_PATH];
					::_stprintf_s(szError, MAX_PATH, _T("Unexpected structure in %s while verifying at line %d"), m_szCsiName, u32LineCount);
#ifndef CS_WIN64
					TRACE ( DRV_TRACE_LEVEL, "%s\n", szError );
#endif
					CsEventLog EventLog;
					EventLog.Report(CS_ERROR_TEXT, szError);
					i32Ret = CS_MISC_ERROR;
					break;
				}
				nSizeToGo -= 2;
				ucFileData[i32ByteCount] = u32TempData & 0xff; //Scanf gives short int. We need unsigned char
				u32Checksum += ucFileData[i32ByteCount];
				i32ByteCount++;	
			}
			if ( fscanf_s(fp, "%2X\n", &u32FileChecksum) == 0 )//Unexpected structure in file
			{
				TCHAR szError[MAX_PATH];
				::_stprintf_s(szError, MAX_PATH, _T("Unexpected structure in %s while verifying at line %d"), m_szCsiName, u32LineCount);
#ifndef CS_WIN64
				TRACE ( DRV_TRACE_LEVEL, "%s\n", szError );
#endif
				CsEventLog EventLog;
				EventLog.Report(CS_ERROR_TEXT, szError);
				i32Ret = CS_MISC_ERROR;
				break;
			}
			nSizeToGo -= 4;
			u32AddressCount += u32NumBytes;
		}
		else
		{
				TCHAR szError[MAX_PATH];
				::_stprintf_s(szError, MAX_PATH, _T("Error in %s while verifying. Invalid row type at line %d"), m_szCsiName, u32LineCount);
#ifndef CS_WIN64
				TRACE ( DRV_TRACE_LEVEL, "%s\n", szError );
#endif
				CsEventLog EventLog;
				EventLog.Report(CS_ERROR_TEXT, szError);
			i32Ret = CS_MISC_ERROR;
			break;// Not a valid row-type
		}
		u32Checksum += (u32LowAddress & 0xff) + (u32LowAddress >> 8) + u32NumBytes + u32RowType;
		u32Checksum = (1 + ~u32Checksum) & 0xff;
		if ( u32Checksum != u32FileChecksum )
		{
			TCHAR szError[MAX_PATH];
			::_stprintf_s(szError, MAX_PATH, _T("Checksum mismatch in %s firmware while verifying at line %d"), nFpgaNum == 1 ? "algo": "comm", u32LineCount);
#ifndef CS_WIN64
			TRACE ( DRV_TRACE_LEVEL, "%s\n", szError );
#endif
			CsEventLog EventLog;
			EventLog.Report(CS_ERROR_TEXT, szError);
			i32Ret = CS_MISC_ERROR;
			break;
		}
	}


	fclose(fp);
	m_pfFlashDownloadEnd(m_pControl, nDevNum);
//	m_pfDisableSpi(m_pControl, nDevNum);	Commented out May 12, 2010

	return i32Ret;
}

//	Compare ucFileData and ucFlashData byte-wise.
//	If a difference is found, function set u32MismatchOffset to
//	the current position and returns 0.  Return value of 1 indicates success;

int32 CsAdqUpdate::CompareData(unsigned char* ucFileData, unsigned char* ucFlashData, int32 i32ByteCount, uInt32* u32MismatchOffset)
{
	for ( int32 i = 0; i < i32ByteCount; i++ )
	{
		if ( ucFileData[i] != ucFlashData[i] )
		{
			*u32MismatchOffset = i;
			return 0;
		}
	}
	return 1;
}


uInt32 CsAdqUpdate::GetAddOnCsiTarget(int nAdqType)
{
	switch( nAdqType )
	{
		case 108: return CSI_USB_108_TARGET;
		case 112: return CSI_USB_112_TARGET;
		case 114: return CSI_USB_114_TARGET;
		case 214: return CSI_USB_214_TARGET;
		default : return 0;
	}
}
