#pragma once
#include "CsTypes.h"
#include "CsStruct.h"
#include "CsErrors.h"
#include "CsThreadLock.h"
#include "CsSpDevFuncTypes.h"
#include "CsiHeader.h"

#include <map>

using namespace std;

#define SPD_INIT_DELAY       4000//2000 //2 sec
#define USB_PARAMS_KEY _T("SYSTEM\\CurrentControlSet\\Services\\CsUsb\\Configuration")

#define CS_OPTIONS_USB_FIRMWARE_MASK 0x1
#define CS_OPTIONS_USB_SX50T_MASK	 0x2
#define CS_OPTIONS_USB_FX70T_MASK	 0x4

#define CS_USB_AC_DC_INPUT           0x1

#define CS_USB_ALG_IMAGE_SIZE        7038721	// FPGA 1
#define CS_USB_ALG_FX70T_IMAGE_SIZE	 9290829	// FPGA 1 !! THIS IS A DUMMY SIZE, FOR NOW
#define CS_USB_COMM_LX30T_IMAGE_SIZE 3294859	// FPGA 2
#define CS_USB_COMM_SX50T_IMAGE_SIZE 7038721	// FPGA 2
#define CS_USB_FIRMWARE_IMAGE_SIZE   5722
#define CS_USB_DEFAULT_TRIG_SENSE    48

#define CSISEPARATOR_LENGTH			16
const uInt8 CsiSeparator[] = "-- SEPARATOR --|";		// 16 character long + NULL character


#pragma pack (1)

typedef struct _SpDevDef
{
	int			   nAdqType;	
	char           strName[8];
	char           strCsName[16];
	LONGLONG       llBaseFrequecy;
	long           lMaxDiv;
	long           lResolution;
	long           lMemSize; //in MS
	unsigned short usBits;
	unsigned short usChanNumber;
	long           lExtTrigAddrAdj;
}SpDevDef;

typedef struct _SpDevSr
{
	int64		   i64Sr;	
	int            nDiv;
	unsigned int   uSkip;
}SpDevSr;

#pragma pack ()


class CsEventLog
{
public:
	CsEventLog(void);
	~CsEventLog(void);

	BOOL Report(DWORD dwErrorId, LPTSTR lpMsgIn);

private:
	HANDLE m_hEventLog;
	DWORD  m_dwError;
};


class CsSpdevException
{
public:
	CsSpdevException(void) {}
	CsSpdevException(TCHAR* cause) {_tcsncpy_s(_cause, sizeof(_cause), cause, _countof(_cause) - 1);}
	CsSpdevException(TCHAR* pre, TCHAR* post ) {::_stprintf_s(_cause, _countof(_cause),"%s%s", pre, post ); }
	~CsSpdevException(void) {}

	TCHAR _cause[250];
};

class CsSpDeviceManager;

#define SDP_DEV_SIG_SIZE              16
#define SDP_DEV_SIG_ADDR              332
#define SDP_DEV_DC_SIG                "004"
#define SDP_DEV_DC_AC_SIG             "005"
#define SDP_DEV_DC_SIG_SIZE			  (_countof(SDP_DEV_DC_SIG)-1)

#define SPD_INPUT_RANGE               2200           //2.2 Vp-p
#define SPD_DC_INPUT_RANGE            250            //2.2 Vp-p
#define SPD_EXT_TRIG_LEVEL            40
#define SPD_EXTTRIG_RANGE             CS_GAIN_5_V    //5  Vp-p
#define SPD_MIN_EXT_CLK               34000000       //34 MHz
#define SPD_LOW_EXT_CLK               240000000      //240 MHz
#define SPD_MIN_DEPTH                 8              //samples
#define SPD_ITRA_RECORD               8              //Uint32
#define SPD_MAX_TRIG_DELAY            (1<<31)        //samples
#define SPD_ALINGMENT                 128            //bytes
#define SPD_SKIP_SR                   100000000      // 100 MHz

#define SPD_CHAN_AFE_MASK             0x5

class CsSpDevice
{
private:
	CsSpDeviceManager* m_pManager;
	SpDevDef           m_Info;
	int                m_nNum;
	int                m_nType;

	ADQInterface*	   m_pAdq;

	int                m_nRevFpga1;
	int                m_nRevFpga2;
	unsigned int       m_uBusAddr;
	char               m_strSerNum[17];
	uInt32             m_u32MemSize;
	uInt32             m_u32PcbRev;
	bool               m_bStarted;
	int64              m_i64TickFrequency;
	int                m_nTimeStampMode;
	bool               m_bDisplayTrace;
	bool               m_bDisplayXferTrace;
	int                m_nTrigSense;
	int64              m_i64TrigAddrAdj;

	CSI_ENTRY		   m_BaseBoardCsiEntry;		// the Alg fpga
	CSI_ENTRY		   m_AddonCsiEntry;			// the Comm fpga
	CSI_ENTRY		   m_UsbFpgaEntry;

	bool               m_bSkipFwUpdate;
	bool               m_bForceFwUpdateComm;
	bool               m_bForceFwUpdateAlgo;

	bool               m_bFavourClockSkip;

	bool               m_bDcInput;
	bool               m_bDcAcInput;
	bool               m_bUsb;
	bool               m_bPxi;

	uInt32             m_u32AFE;  // AC / DC configuration for both channels

	SpDevSr*           m_pSrTable;
	size_t             m_szSrTableSize;

	CSACQUISITIONCONFIG m_Acq;
	CSCHANNELCONFIG     m_aChan[2];
	CSTRIGGERCONFIG     m_Trig;

	CSACQUISITIONCONFIG m_ReqAcq;
	CSCHANNELCONFIG     m_aReqChan[2];
	CSTRIGGERCONFIG     m_ReqTrig;

	int32 ConfigureAcq( PCSACQUISITIONCONFIG pAcqCfg, bool bCoerce, bool bValidateOnly);
	int32 ConfigureChan( PCSCHANNELCONFIG aChanCfg, bool bCoerce, bool bValidateOnly);
	int32 ConfigureTrig( PCSTRIGGERCONFIG pTrigCfg, bool bCoerce, bool bValidateOnly);

	int32  ValidateBufferForReadWrite(uInt32 u32ParamID, void* pvParamBuffer, bool bRead);
	uInt16 ConvertToHw( uInt16 u16SwChannel );

	int64 GetMaxSr(void){return m_pSrTable[0].i64Sr;}
	
	void  BuildSrTable(void);
	int32 StartAcquisition(void);
	int32 ForceTrigger(void);
	int32 Abort(void);
	int32 Reset(void);
	int32 ResetTimeStamp(void);
	int32 ResetDevice(void);
	void ReadRegistryInfo(void);
	
public:
	CsSpDevice(CsSpDeviceManager* pMngr, int nNum);
	~CsSpDevice(void);


	//// For CsDrvApi
	int32 GetAcquisitionSystemInfo(PCSSYSTEMINFO pSysInfo);

	int32 AcquisitionSystemInit(BOOL bResetDefaultSetting);
	int32 AcquisitionSystemCleanup(void);
	int32 GetAcquisitionSystemCaps(uInt32 u32CapsId, PCSSYSTEMCONFIG pSysCfg, void* pBuffer, uInt32* pu32BufferSize);
	int32 GetBoardsInfo(PARRAY_BOARDINFO pABoardInfo);
	int32 GetAcquisitionStatus(uInt32* pu32Status);
	int32 GetParams(uInt32 u32ParamID, void* pParambuffer);
	int32 SetParams(uInt32 u32ParamID, void* pParambuffer);
	int32 ValidateParams(uInt32 u32ParamID, uInt32 u32Coerce, void* pParamBuffer);
	int32 Do(uInt32 u32ActionID, void* pActionBuffer);
	int32 TransferData(IN_PARAMS_TRANSFERDATA inParams, POUT_PARAMS_TRANSFERDATA pOutParams);
	int32 TransferDataEx(PIN_PARAMS_TRANSFERDATA_EX pInParamsEx, POUT_PARAMS_TRANSFERDATA_EX pOutParamsEx);
	int32 RegisterEventHandle(uInt32 , HANDLE* ){return CS_INVALID_EVENT_TYPE;}

	int32 ReadAndValidateCsiFile(void);
	int32 CompareFirmwareVersions(void);

	TCHAR* GetSerialNumber() { return m_strSerNum; }
	TCHAR* GetDeviceName() { return m_Info.strCsName; }
};

typedef map<DRVHANDLE, CsSpDevice*> USBDEVMAP;

//#define CSUSB_BASE_HANDLE 0x23450000
#define CSUSB_BASE_HANDLE 0x00234500	// changed so as not to interfere with remote system handles

class CsSpDeviceManager
{
private:
	CsSpDeviceManager(void);
	~CsSpDeviceManager(void);
	static CsSpDeviceManager* m_pInstance;
	static Critical_Section __key;
public:
	static CsSpDeviceManager& GetInstance()
	{
		if(NULL == m_pInstance )
		{
			Lock_Guard<Critical_Section> __lock(__key);
			if(NULL == m_pInstance )
			{
				m_pInstance = new CsSpDeviceManager();
			}
		}
		return *m_pInstance;
	}
	static void RemoveInstance()
	{
		Lock_Guard<Critical_Section> __lock(__key);
		delete m_pInstance;
		m_pInstance = NULL;
	}

	int    GetFailedNum(void){return m_nFailedNum;}
	size_t GetDevNum(void){return m_mapDev.size();}
	int    GetApiRev(void){return m_nApiRev;}

	int32 GetAcquisitionSystemCount(uInt16* pu16SystemFound);
	int32 GetAcquisitionSystemHandles(PDRVHANDLE pDrvHdl, uInt32 Size);
	int32 AutoFirmwareUpdate(void);

	CsSpDevice* GetDevice(DRVHANDLE DrvHdl)
	{
		USBDEVMAP::iterator theMapIterator;
		if ((theMapIterator = m_mapDev.find(DrvHdl)) != m_mapDev.end())
			return ((*theMapIterator).second);
		else
			return NULL;
	}

	
	HANDLE GetCommEvent(){return m_hCommEventHandle;}
	static const SpDevDef* GetDevTypeTable(size_t* pSize);

protected:
	const static  SpDevSr  SrTable108_fcd[];
	const static  SpDevSr  SrTable112_fcd[];
	const static  SpDevSr  SrTable114_fcd[];
	const static  SpDevSr  SrTable214_fcd[];
	const static  SpDevSr  SrTable108_fcs[];
	const static  SpDevSr  SrTable112_fcs[];
	const static  SpDevSr  SrTable114_fcs[];
	const static  SpDevSr  SrTable214_fcs[];
private:
	const static  SpDevDef DevType[];
	bool		m_bInitOK;

	HANDLE      m_hCommEventHandle;

	USBDEVMAP   m_mapDev;	
	void*       m_pControl;
	int         m_nTypeCount;
	int         m_nFailedNum;
	int         m_nApiRev;

	int	        ResolveProcAddresses(HMODULE _hDllMod);	
	int			MakeDevicesAndControl(void);
	void		DeleteDevicesAndControl(void);

protected:
	_AdqCreateControlUnit*     m_pfCreateControl;
	_AdqDeleteControlUnit*     m_pfDeleteControl;
	_AdqGetApiRevision*        m_pfGetApiRevision;
	_AdqFindDevices*           m_pfFindDevices;
	_AdqGetFailedDeviceCount*  m_pfGetFailedDevCount;
	_AdqGetADQInterface*	   m_pfGetAdqInterface;

	void* GetControl(void){return m_pControl;}
	
	friend class CsSpDevice;
};