#pragma once

typedef void*  (__cdecl _AdqCreateControlUnit) (void);
typedef void   (__cdecl _AdqDeleteControlUnit) (void* adq_cu_ptr);
typedef int	   (__cdecl _AdqFindDevices)       (void* adq_cu_ptr);

typedef int (__cdecl _AdqEnableSpi) (void* adq_cu_ptr, int adq_num);
typedef int (__cdecl _AdqDisableSpi) (void* adq_cu_ptr, int adq_num);
typedef int (__cdecl _AdqFlashUploadStart) (void* adq_cu_ptr, int adq_num, int fpga_num, int start_page);
typedef int (__cdecl _AdqFlashUploadPart) (void* adq_cu_ptr, int adq_num, long len, char * data);
typedef int (__cdecl _AdqFlashUploadEnd) (void* adq_cu_ptr, int adq_num);
typedef int (__cdecl _AdqFlashDownloadStart) (void* adq_cu_ptr, int adq_num, int fpga_num, int start_page);
typedef int (__cdecl _AdqFlashDownloadPart) (void* adq_cu_ptr, int adq_num, long len, char * data);
typedef int (__cdecl _AdqFlashDownloadEnd) (void* adq_cu_ptr, int adq_num);
typedef int (__cdecl _AdqGetPid) (void* adq_cu_ptr, int adq_num); 
typedef int* (__cdecl _AdqGetRevision) (void* adq_cu_ptr, int adq_num);	


#define PACKET_LENGTH	64


class CsAdqUpdate
{
public:
	CsAdqUpdate(bool bForceUpdateAlgo, bool bForceUpdateComm);
	~CsAdqUpdate(void);

	int32 Initialize(void);
	int32 UpdateFirmware(int nDev);
	static uInt32 GetAddOnCsiTarget(const int nAdqType);

private:

	HMODULE	m_hDllMod;
	void* m_pControl;
	int	 m_nDev;
	TCHAR m_szCsiName[MAX_PATH];

	_AdqCreateControlUnit* m_pfCreateControl;
	_AdqDeleteControlUnit* m_pfDeleteControl;
	_AdqFindDevices* m_pfFindDevices;
	_AdqEnableSpi* m_pfEnableSpi;
	_AdqDisableSpi* m_pfDisableSpi;
	_AdqFlashUploadStart* m_pfFlashUploadStart;
	_AdqFlashUploadPart* m_pfFlashUploadPart;
	_AdqFlashUploadEnd* m_pfFlashUploadEnd;
	_AdqFlashDownloadStart* m_pfFlashDownloadStart;
	_AdqFlashDownloadPart* m_pfFlashDownloadPart;
	_AdqFlashDownloadEnd* m_pfFlashDownloadEnd;
	_AdqGetPid* m_pfGetPid;
	_AdqGetRevision* m_pfGetRevision;


	bool m_bForceUpdateAlgo;
	bool m_bForceUpdateComm;

	int GetProductName(int nDevNum);
	int32 UpdateFpga(int nDevNum, int nFpgaNum);
	int32 VerifyBitfile(int nDevNum, int nFpgaNum, uInt32 u32ImageOffset, uInt32 u32ImageSize);
	int32 CompareData(unsigned char* file_data, unsigned char* flash_data, int32 i32ByteCount, uInt32* u32MismatchOffset);
};
