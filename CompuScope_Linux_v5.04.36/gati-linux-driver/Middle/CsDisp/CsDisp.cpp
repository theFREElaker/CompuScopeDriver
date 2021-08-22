// CsDisp.cpp : Defines the entry point for the DLL application.
//
#include "StdAfx.h"
#include "stdlib.h"

#include "CsDrvConst.h"
#include "CsDisp.h"
#include "CsSharedMemory.h"
#include "CsErrors.h"
#include "CsDefines.h"
#include "CsiHeader.h" 
#include "CsDeviceIDs.h" 

#if defined(_WINDOWS_)
#include "Debug.h" 

#include "misc.h"
#include "CsSupport.h"
#else
#include "xml.h"
#endif


#if defined(_WINDOWS_)
FWLnCSI_INFO	 g_GageProduct[] = 
{
	// Product Id			    Base Id					 Addon Id			   BoardType			Dll name		Classname		Product		     Fwl file name
	CS_DEVID_SPIDER,	        CSI_BBSPIDER_TARGET	,	 CSI_CS8xxx_TARGET,	     CS8_BT_FIRST_BOARD,"Cs8xxxWdf.dll",	"Spider",		"Cs8xxx",	     "Cs8xxx",
	CS_DEVID_SPIDER_v12,	    CSI_BBSPIDERV12_TARGET,	 CSI_CS8xxxV12_TARGET,   CS8_BT_FIRST_BOARD,"Cs8xxxWdf.dll",	"SpiderV12",	"Cs8yyy",	     "Cs8yyy",
	CS_DEVID_ZAP,       	    CSI_BBSPIDERV12_TARGET,	 CSI_CSZAP_TARGET,       CS8_BT_FIRST_BOARD,"Cs8xxxWdf.dll",	"SpiderV12",   	"CsZap",	     "CsZap",
	CS_DEVID_ST81AVG,		    CSI_BBSPIDERV12_TARGET,	 CSI_CS8xxxV12_TARGET,   CS8_BT_FIRST_BOARD,"Cs8xxxWdf.dll",	"SpiderV12",	"ST81AVG",	     "Cs8yyy",
	CS_DEVID_RABBIT,	        CSI_BBRABBIT_TARGET,	 CSI_CSXYG8_TARGET,      CS22G8_BOARDTYPE,	"CsxyG8Wdf.dll",	"Rabbit",		"CsxyG8",	     "CsxyG8",
	CS_DEVID_STLP81,		    CSI_BBSPIDER_LP_TARGET,	 CSI_CS8xxxLP_TARGET,    CS8_BT_FIRST_BOARD,"Cs8xxxWdf.dll",	"SpiderLP",		"STLP81",	     "Cs8zzz",
	CS_DEVID_SPLENDA,		    CSI_BBSPLENDA_TARGET,	 CSI_CSSPLENDA_TARGET,   CS16xyy_BASEBOARD, "Cs16xyyWdf.dll",	"Splenda",		"Cs16xyy",	     "Cs16xyy",	
	CS_DEVID_BRAINIAC,		    CSI_BRAIN_TARGET,		 CSI_BRAIN_TARGET,	     CSBRAIN_BOARDTYPE,	"CsabG12Wdf.dll",	"Brainiac",		"CsabG12",	     "CsabG12",
	CS_DEVID_PITCHER,		    CSI_PITCH_TARGET,		 CSI_PITCH_TARGET,	     CS_BASE8_BOARDTYPE,"CscdG8Wdf.dll",	"Base8",		"Base-8",	     "CscdG8p",
	CS_DEVID_PITCHER_MEM,	    CSI_PITCH_M_TARGET,		 CSI_PITCH_M_TARGET,     CS_BASE8_BOARDTYPE,"CscdG8Wdf.dll",	"Base8-Mem",	"Base-8M",	     "CscdG8pm",
	CS_DEVID_FASTBALL,          CSI_FASTBL_TARGET,	     CSI_FASTBL_TARGET,      CSX22G8_BOARDTYPE,	"CscdG8Wdf.dll",	"Bunny",	    "CobraMax",      "CscdG8pb",
	CS_DEVID_FASTBALL_NOMEM,    CSI_FASTBL_NM_TARGET,    CSI_FASTBL_NM_TARGET,   CSX22G8_BOARDTYPE,	"CscdG8Wdf.dll",	"Bunny-NoMem",  "CobraN",        "CscdG8pbnm",
	CS_DEVID_FASTBALL_4G,       CSI_FASTBL4G_TARGET,     CSI_FASTBL4G_TARGET,    CSX14G8_BOARDTYPE,	"CscdG8Wdf.dll",	"Bunny-4G",	    "CobraMax-4G",   "CscdG84g",
	CS_DEVID_FASTBALL_4G_NOMEM, CSI_FASTBL4G_NM_TARGET,  CSI_FASTBL4G_NM_TARGET, CSX14G8_BOARDTYPE,	"CscdG8Wdf.dll",	"Bunny-4Gnm",   "CobraN-4G",     "CscdG84gnm",
	CS_DEVID_FASTBALL_3G,       CSI_FASTBL3G_TARGET,	 CSI_FASTBL3G_TARGET,    CSX23G8_BOARDTYPE,	"CscdG8Wdf.dll",	"Bunny-3G",     "CobraMax-3G",   "CscdG83g",
	CS_DEVID_FASTBALL_3G_NOMEM, CSI_FASTBL3G_NM_TARGET,  CSI_FASTBL3G_NM_TARGET, CSX23G8_BOARDTYPE,	"CscdG8Wdf.dll",	"Bunny-3Gnm",   "CobraN-3G",     "CscdG83gnm",
	CS_DEVID_JOHNDEERE,		    CSI_DEERE_TARGET,		 CSI_DEERE_TARGET,	     CSJD_FIRST_BOARD,	"CscdG12Wdf.dll",	"JohnDeere",	"CscdG12",	     "CscdG12",
	CS_DEVID_CS0864G1U,			CSI_USB_BB_TARGET,		 CSI_USB_108_TARGET,	 CSUSB_BOARDTYPE,	"CsUsb.dll",		"Usb",			"CS0864G1U",	 "CS0864G1U",
	CS_DEVID_CS121G11U,			CSI_USB_BB_TARGET,		 CSI_USB_112_TARGET,	 CSUSB_BOARDTYPE,	"CsUsb.dll",		"Usb",			"CS121G11U",	 "CS121G11U",
	CS_DEVID_CS148001U,			CSI_USB_BB_TARGET,		 CSI_USB_114_TARGET,	 CSUSB_BOARDTYPE,	"CsUsb.dll",		"Usb",			"CS148001U",	 "CS148001U",
	CS_DEVID_CS144002U,			CSI_USB_BB_TARGET,		 CSI_USB_214_TARGET,	 CSUSB_BOARDTYPE,	"CsUsb.dll",		"Usb",			"CS144002U",	 "CS144002U",
	CS_DEVID_SPIDER_PCIE,		CSI_BBSPIDER_PCIEx_TARGET,	CSI_CS8xxxPciEx_TARGET,	 CS8_BT_FIRST_BOARD|CSNUCLEONBASE_BOARDTYPE,	"Cs8xxxWdf.dll",	"SpiderPciEx", "CsE8xxx", "CsE8xxx",
	CS_DEVID_SPLENDA_PCIE,		CSI_BBSPLENDA_PCIEx_TARGET,	CSI_SPLENDA_PCIEx_TARGET,	 CS16xyy_BASEBOARD|CSNUCLEONBASE_BOARDTYPE, "Cs16xyyWdf.dll",	"SplendaPciEx",	"CsE16xyy-Ra",	"CsE16xyy",
	CS_DEVID_OSCAR_PCIE,		CSI_BBOSCAR_PCIEx_TARGET,	CSI_SPLENDA_PCIEx_TARGET,	 CS16xyy_BASEBOARD|CSNUCLEONBASE_BOARDTYPE, "Cs16xyyWdf.dll",	"OscarPciEx",	"CsE16xyy-Os",	"CsE16xyo",
	CS_DEVID_ZAP_PCIE,			CSI_BBZAP_PCIEx_TARGET,	CSI_ZAP_PCIEx_TARGET,	 CS8_BT_FIRST_BOARD|CSNUCLEONBASE_BOARDTYPE, "Cs8xxxWdf.dll",	"ZapPciEx",	"CsEZap",	"CsEZap",
	CS_DEVID_COBRA_PCIE,		CSI_BBRABBIT_PCIEx_TARGET,	CSI_CSXYG8_PCIEx_TARGET,	 CS22G8_BOARDTYPE|CSNUCLEONBASE_BOARDTYPE, "CsxyG8Wdf.dll",	"RabbitPciEx",	"CsExyG8",	"CsExyG8",
	CS_DEVID_COBRA_MAX_PCIE,	CSI_BBCOBRAMAX_PCIEx_TARGET, CSI_CSCDG8_PCIEx_TARGET,	 CSX14G8_BOARDTYPE|CSNUCLEONBASE_BOARDTYPE, "CsEcdG8Wdf.dll",	"CobraMxPciEx",	"CsEcdG8",	"CsEcdG8",
	CS_DEVID_DECADE12,			CSI_BBDECADE12_TARGET,	CSI_CSDC12_TARGET,		CSDECADE123G_BOARDTYPE, "CsE12xGyWdf.dll",	"Decade12",	"CsE12xGy",		"CsE12xGy",
	CS_DEVID_HEXAGON,			CSI_BBHEXAGON_TARGET,	CSI_CSDC16_TARGET,		CSHEXAGON_FIRST_BOARD, "CsE16bcdWdf.dll",	"Hexagon PCIe",	"RazorMax-PCIe",		"CsE16bcd",
	CS_DEVID_HEXAGON_PXI,		CSI_BBHEXAGON_PXI_TARGET,	CSI_CSHEX_PXI_TARGET,	CSHEXAGON_FIRST_BOARD, "CsE16bcdWdf.dll",	"Hexagon PXI",	"RazorMax-PXI",		"CsE16bxd",
	CS_DEVID_HEXAGON_A3,		CSI_BBHEXAGON_A3_TARGET,	CSI_CSDC16_A3_TARGET,		CSHEXAGON_FIRST_BOARD, "CsE16bcdWdf.dll",	"HexagonA3 PCIe",	"RazorMaxA3-PCIe",		"CsE16bcd3",
	CS_DEVID_HEXAGON_A3_PXI,	CSI_BBHEXAGON_A3_PXI_TARGET,	CSI_CSHEX_PXI_A3_TARGET,	CSHEXAGON_FIRST_BOARD, "CsE16bcdWdf.dll",	"HexagonA3 PXI",	"RazorMaxA3-PXI",		"CsE16bxd3"
};
#else
// TODO: Update lib names to Linux's name, not Windows (must finish with ".so", not ".dll")
FWLnCSI_INFO	 g_GageProduct[] = 
{
	// Product Id			    Base Id					 Addon Id			   BoardType			Dll name		Classname		Product		     Fwl file name
	CS_DEVID_SPIDER,	        CSI_BBSPIDER_TARGET	,	 CSI_CS8xxx_TARGET,	     CS8_BT_FIRST_BOARD,"libCs8xxx.so",		"Spider",		"Cs8xxx",	     "Cs8xxx",
	CS_DEVID_SPIDER_v12,	    CSI_BBSPIDERV12_TARGET,	 CSI_CS8xxxV12_TARGET,   CS8_BT_FIRST_BOARD,"libCs8xxx.so",		"SpiderV12",	"Cs8yyy",	     "Cs8yyy",
	CS_DEVID_ZAP,       	    CSI_BBSPIDERV12_TARGET,	 CSI_CSZAP_TARGET,       CS8_BT_FIRST_BOARD,"libCs8xxx.so",		"SpiderV12",   	"CsZap",	     "CsZap",
	CS_DEVID_ST81AVG,		    CSI_BBSPIDERV12_TARGET,	 CSI_CS8xxxV12_TARGET,   CS8_BT_FIRST_BOARD,"libCs8xxx.so",		"SpiderV12",	"ST81AVG",	     "Cs8yyy",
	CS_DEVID_RABBIT,	        CSI_BBRABBIT_TARGET,	 CSI_CSXYG8_TARGET,      CS22G8_BOARDTYPE,	"libCsxyG8.so",		"Rabbit",		"CsxyG8",	     "CsxyG8",
	CS_DEVID_STLP81,		    CSI_BBSPIDER_LP_TARGET,	 CSI_CS8xxxLP_TARGET,    CS8_BT_FIRST_BOARD,"libCs8xxx.so",		"SpiderLP",		"STLP81",	     "Cs8zzz",
	CS_DEVID_SPLENDA,		    CSI_BBSPLENDA_TARGET,	 CSI_CSSPLENDA_TARGET,   CS16xyy_BASEBOARD, "libCs16xyy.so",	"Splenda",		"Cs16xyy",	     "Cs16xyy",	
	CS_DEVID_BRAINIAC,		    CSI_BRAIN_TARGET,		 CSI_BRAIN_TARGET,	     CSBRAIN_BOARDTYPE,	"CsabG12Wdf.dll",	"Brainiac",		"CsabG12",	     "CsabG12",
	CS_DEVID_PITCHER,		    CSI_PITCH_TARGET,		 CSI_PITCH_TARGET,	     CS_BASE8_BOARDTYPE,"libCscdG8.so",		"Base8",		"Base-8",	     "CscdG8p",
	CS_DEVID_PITCHER_MEM,	    CSI_PITCH_M_TARGET,		 CSI_PITCH_M_TARGET,     CS_BASE8_BOARDTYPE,"libCscdG8.so",		"Base8-Mem",	"Base-8M",	     "CscdG8pm",
	CS_DEVID_FASTBALL,          CSI_FASTBL_TARGET,	     CSI_FASTBL_TARGET,      CSX22G8_BOARDTYPE,	"libCscdG8.so",		"Bunny",	    "CobraMax",      "CscdG8pb",
	CS_DEVID_FASTBALL_NOMEM,    CSI_FASTBL_NM_TARGET,    CSI_FASTBL_NM_TARGET,   CSX22G8_BOARDTYPE,	"libCscdG8.so",		"Bunny-NoMem",  "CobraN",        "CscdG8pbnm",
	CS_DEVID_FASTBALL_4G,       CSI_FASTBL4G_TARGET,     CSI_FASTBL4G_TARGET,    CSX14G8_BOARDTYPE,	"libCscdG8.so",		"Bunny-4G",	    "CobraMax-4G",   "CscdG84g",
	CS_DEVID_FASTBALL_4G_NOMEM, CSI_FASTBL4G_NM_TARGET,  CSI_FASTBL4G_NM_TARGET, CSX14G8_BOARDTYPE,	"libCscdG8.so",		"Bunny-4Gnm",   "CobraN-4G",     "CscdG84gnm",
	CS_DEVID_FASTBALL_3G,       CSI_FASTBL3G_TARGET,	 CSI_FASTBL3G_TARGET,    CSX23G8_BOARDTYPE,	"libCscdG8.so",		"Bunny-3G",     "CobraMax-3G",   "CscdG83g",
	CS_DEVID_FASTBALL_3G_NOMEM, CSI_FASTBL3G_NM_TARGET,  CSI_FASTBL3G_NM_TARGET, CSX23G8_BOARDTYPE,	"libCscdG8.so",		"Bunny-3Gnm",   "CobraN-3G",     "CscdG83gnm",
	CS_DEVID_JOHNDEERE,		    CSI_DEERE_TARGET,		 CSI_DEERE_TARGET,	     CSJD_FIRST_BOARD,	"libCscdG12.so",	"JohnDeere",	"CscdG12",	     "CscdG12",
	CS_DEVID_CS0864G1U,			CSI_USB_BB_TARGET,		 CSI_USB_108_TARGET,	 CSUSB_BOARDTYPE,	"CsUsb.dll",		"Usb",			"CS0864G1U",	 "CS0864G1U",
	CS_DEVID_CS121G11U,			CSI_USB_BB_TARGET,		 CSI_USB_112_TARGET,	 CSUSB_BOARDTYPE,	"CsUsb.dll",		"Usb",			"CS121G11U",	 "CS121G11U",
	CS_DEVID_CS148001U,			CSI_USB_BB_TARGET,		 CSI_USB_114_TARGET,	 CSUSB_BOARDTYPE,	"CsUsb.dll",		"Usb",			"CS148001U",	 "CS148001U",
	CS_DEVID_CS144002U,			CSI_USB_BB_TARGET,		 CSI_USB_214_TARGET,	 CSUSB_BOARDTYPE,	"CsUsb.dll",		"Usb",			"CS144002U",	 "CS144002U",
	CS_DEVID_SPIDER_PCIE,		CSI_BBSPIDER_PCIEx_TARGET,	CSI_CS8xxxPciEx_TARGET,	 CS8_BT_FIRST_BOARD|CSNUCLEONBASE_BOARDTYPE,	"libCs8xxx.so",	"SpiderPciEx", "CsE8xxx", "CsE8xxx",
	CS_DEVID_SPLENDA_PCIE,		CSI_BBSPLENDA_PCIEx_TARGET,	CSI_SPLENDA_PCIEx_TARGET,	 CS16xyy_BASEBOARD|CSNUCLEONBASE_BOARDTYPE, "libCs16xyy.so",	"SplendaPciEx",	"CsE16xyy-Ra",	"CsE16xyy",
	CS_DEVID_OSCAR_PCIE,		CSI_BBOSCAR_PCIEx_TARGET,	CSI_SPLENDA_PCIEx_TARGET,	 CS16xyy_BASEBOARD|CSNUCLEONBASE_BOARDTYPE, "libCs16xyy.so",	"OscarPciEx",	"CsE16xyy-Os",	"CsE16xyo",
	CS_DEVID_ZAP_PCIE,			CSI_BBZAP_PCIEx_TARGET,	CSI_ZAP_PCIEx_TARGET,	 CS8_BT_FIRST_BOARD|CSNUCLEONBASE_BOARDTYPE, "libCs8xxx.so",	"ZapPciEx",	"CsEZap",	"CsEZap",
	CS_DEVID_COBRA_PCIE,		CSI_BBRABBIT_PCIEx_TARGET,	CSI_CSXYG8_PCIEx_TARGET,	 CS22G8_BOARDTYPE|CSNUCLEONBASE_BOARDTYPE, "libCsxyG8.so",	"RabbitPciEx",	"CsExyG8",	"CsExyG8",
	CS_DEVID_COBRA_MAX_PCIE,	CSI_BBCOBRAMAX_PCIEx_TARGET, CSI_CSCDG8_PCIEx_TARGET,	 CSX14G8_BOARDTYPE|CSNUCLEONBASE_BOARDTYPE, "libCsEcdG8.so",	"CobraMxPciEx",	"CsEcdG8",	"CsEcdG8",
	CS_DEVID_DECADE12,			CSI_BBDECADE12_TARGET,	CSI_CSDC12_TARGET,		CSDECADE123G_BOARDTYPE, "libCsE12xGy.so",	"Decade12",	"CsE12xGy",		"CsE12xGy",
	CS_DEVID_HEXAGON,			CSI_BBHEXAGON_TARGET,	CSI_CSDC16_TARGET,		CSHEXAGON_FIRST_BOARD, "libCsE16bcd.so",	"Hexagon PCIe",	"RazorMax-PCIe",		"CsE16bcd",
	CS_DEVID_HEXAGON_PXI,		CSI_BBHEXAGON_PXI_TARGET,	CSI_CSHEX_PXI_TARGET,	CSHEXAGON_FIRST_BOARD, "libCsE16bcd.so",	"Hexagon PXI",	"RazorMax-PXI",		"CsE16bxd",
	CS_DEVID_HEXAGON_A3,		CSI_BBHEXAGON_A3_TARGET,	CSI_CSDC16_A3_TARGET,		CSHEXAGON_FIRST_BOARD, "libCsE16bcd.so",	"HexagonA3 PCIe",	"RazorMaxA3-PCIe",		"CsE16bcd3",
	CS_DEVID_HEXAGON_A3_PXI,	CSI_BBHEXAGON_A3_PXI_TARGET,	CSI_CSHEX_PXI_A3_TARGET,	CSHEXAGON_FIRST_BOARD, "libCsE16bcd.so",	"HexagonA3 PXI",	"RazorMaxA3-PXI",		"CsE16bxd3"
};
#endif

// Private typedef for dispach DLL
typedef int32	(*LPGetAcquisitionSystemCount)		(uInt16, uInt16*);
typedef uInt32	(*LPGetAcquisitionSystemHandles)	(PCSHANDLE, uInt32);
typedef int32	(*LPGetAcquisitionSystemInfo)		(CSHANDLE, PCSSYSTEMINFO);
typedef int32	(*LPGetAcquisitionSystemCaps)		(CSHANDLE, uInt32, PCSSYSTEMCONFIG, void*, uInt32*);
typedef int32	(*LPGetBoardsInfo)					(CSHANDLE, PARRAY_BOARDINFO);
typedef int32	(*LPAcquisitionSystemCleanup)		(CSHANDLE);
typedef int32	(*LPGetAcquisitionStatus)			(CSHANDLE, uInt32*);
typedef int32	(*LPUpdateLookupTable)				(CSHANDLE, RMHANDLE);
typedef int32	(*LPSetParams)						(CSHANDLE, uInt32, void*);
typedef int32	(*LPValidateParams)					(CSHANDLE, uInt32, uInt32, void*);
typedef int32	(*LPGetParams)						(CSHANDLE, uInt32, void*);
typedef int32	(*LPDrvDo)							(CSHANDLE, uInt32, void*);
typedef uInt32	(*LPTransferData)					(CSHANDLE, IN_PARAMS_TRANSFERDATA, POUT_PARAMS_TRANSFERDATA);
typedef uInt32	(*LPTransferDataEx)					(CSHANDLE, PIN_PARAMS_TRANSFERDATA_EX, POUT_PARAMS_TRANSFERDATA_EX);
typedef uInt32	(*LPGetAsyncTransferDataResult)		(CSHANDLE, uInt8, CSTRANSFERDATARESULT*, BOOL);
typedef int32	(*LPRegisterEventHandle)			(CSHANDLE, uInt32, HANDLE*);
typedef int32 (*LPGetAvailableSampleRates)			(CSHANDLE, uInt16, PCSSAMPLERATETABLE, uInt32 Size);
typedef int32 (*LPGetAvailableInputRanges)			(CSHANDLE, uInt16, PCSRANGETABLE, uInt32 Size);
typedef int32 (*LPGetAvailableImpedances)			(CSHANDLE, uInt32, uInt32);
typedef	int32 (*LPAcquisitionSystemInit)			( CSHANDLE, BOOL );
typedef	int32 (*LPOpenSystemForRm)					( DRVHANDLE	);
typedef	int32 (*LPCloseSystemForRm)					( DRVHANDLE	);
typedef	int32 (*LPCsDrvAutoFirmwareUpdate)			();
typedef	int32 (*LPCsDrvExpertCall)					(CSHANDLE,  void *);
typedef	int32 (*LPCsDrvCfgAuxIo)         			(CSHANDLE);
typedef	int32 (*LPCsDrvGetBoardType)				(uInt32 *,  uInt32 *);
typedef	int32 (*LPCsDrvGetHwOptionText)				(uInt32, PCS_HWOPTIONS_CONVERT2TEXT);
typedef	int32 (*LPCsDrvGetFwOptionText)				(PCS_FWOPTIONS_CONVERT2TEXT);
typedef int32 (*LPCsDrvGetBoardCaps)				(uInt32, uInt32, void* , uInt32*);
typedef	BOOL  (*LPCsDrvIsMyBoardType)				(uInt32);
typedef	int32 (*LPStmAllocateBuffer)				(CSHANDLE, int32, uInt32, PVOID *);
typedef	int32 (*LPStmFreeBuffer)					(CSHANDLE, int32, PVOID);
typedef	int32 (*LPStmTransferToBuffer)				(CSHANDLE, int32, PVOID, uInt32 );
typedef	int32 (*LPStmGetTransferStatus)				(CSHANDLE, int32, uInt32, uInt32 *, uInt32 *, uInt32 *);
typedef int32 (*LPDrvSetRemoteSystemInUse)			(CSHANDLE, BOOL);
typedef int32 (*LPDrvIsRemoteSystemInUse)			(CSHANDLE, BOOL *);
typedef int32 (*LPDrvGetRemoteUser)					(CSHANDLE, TCHAR *);

typedef struct  _DRV_DLL_NODE
{
	char	DrvName[DRIVER_MAX_NAME_LENGTH];// Dll name
	HMODULE	DrvDllHandle;					// DLL handle
	uInt32	AcquisitionSystemCount;			// Number of Acquisition system found
	// Pointers to function exported by HW specific DLLs
	int32	(*GetAcquisitionSystemCount)	(uInt16, uInt16*);
	uInt32	(*GetAcquisitionSystemHandles)	(PCSHANDLE, uInt32);
    int32	(*GetAcquisitionSystemInfo)		(CSHANDLE, PCSSYSTEMINFO);
	int32	(*GetAcquisitionSystemCaps)		(CSHANDLE, uInt32, PCSSYSTEMCONFIG, void*, uInt32*);
	int32	(*AcquisitionSystemCleanup)		(CSHANDLE);
	int32	(*GetAcquisitionStatus)			(CSHANDLE, uInt32*);
	int32	(*GetBoardsInfo)				(CSHANDLE, PARRAY_BOARDINFO);
	int32	(*GetParams)					(CSHANDLE, uInt32, void*);
	int32	(*SetParams)					(CSHANDLE, uInt32, void*);
	int32	(*ValidateParams)				(CSHANDLE, uInt32, uInt32, void*);
	int32	(*DrvDo)						(CSHANDLE, uInt32, void*);
	uInt32	(*TransferData)					(CSHANDLE, IN_PARAMS_TRANSFERDATA, POUT_PARAMS_TRANSFERDATA);
	uInt32	(*TransferDataEx)				(CSHANDLE, PIN_PARAMS_TRANSFERDATA_EX, POUT_PARAMS_TRANSFERDATA_EX);
	uInt32	(*GetAsyncTransferDataResult)	(CSHANDLE, uInt8 , CSTRANSFERDATARESULT* , BOOL );
	int32	(*RegisterEventHandle)			(CSHANDLE, uInt32, HANDLE*);
	int32	(*AcquisitionSystemInit)		(CSHANDLE, BOOL);
	int32	(*RegisterStreamingBuffer)		(CSHANDLE, uInt8, CSFOLIOARRAY*, HANDLE*, PSTMSTATUS*);
	int32	(*ReleaseStreamingBuffer)		(CSHANDLE, uInt8 );
	int32	(*OpenSystemForRm)				(CSHANDLE);
	int32	(*CloseSystemForRm)				(CSHANDLE);
	int32	(*CsDrvAutoFirmwareUpdate)		();
	int32   (*CsDrvExpertCall)				(CSHANDLE,  void*);
	int32	(*CsDrvCfgAuxIo)		        (CSHANDLE);

	int32	(*_CsDrvGetBoardType)			(uInt32*,  uInt32*);
	int32	(*_CsDrvGetHwOptionText)		(uInt32, PCS_HWOPTIONS_CONVERT2TEXT);
	int32	(*_CsDrvGetFwOptionText)		(PCS_FWOPTIONS_CONVERT2TEXT);
	int32	(*_CsDrvGetBoardCaps)			(uInt32, uInt32, void* , uInt32*);
	BOOL	(*_CsDrvIsMyBoardType)			(uInt32);

	int32	(*CsDrvStmAllocateBuffer)		(CSHANDLE, int32, uInt32, PVOID *);
	int32	(*CsDrvStmFreeBuffer)			(CSHANDLE, int32, PVOID);
	int32	(*CsDrvStmTransferToBuffer)		(CSHANDLE, int32, PVOID, uInt32 );
	int32	(*CsDrvStmGetTransferStatus)	(CSHANDLE, int32, uInt32, uInt32 *, uInt32 *, uInt32 *);
	int32   (*CsDrvSetRemoteSystemInUse)	(CSHANDLE, BOOL);
	int32   (*CsDrvIsRemoteSystemInUse)		(CSHANDLE, BOOL*);
	int32   (*CsDrvGetRemoteUser)			(CSHANDLE, TCHAR*);

} DRV_DLL_NODE, *PDRV_DLL_NODE;



// Global variables

//
// Shared section of memory accross multiple instances of DLL
//

//
// Vaiables private for each instance of DLL
//
PDRV_DLL_NODE	g_pDrvDllNode = NULL;

#if defined (_WINDOWS_) && !defined(CS_WIN64)
	CTraceManager*	g_pTraceMngr= NULL;
#endif


#if defined(_WINDOWS_)
	
static SharedMem< SHARED_DISP_GLOBALS > DispGlobal(DISP_SHARED_MEM_FILE);

BOOL APIENTRY DllMain( HANDLE ,	
                       DWORD  ul_reason_for_call,
                       LPVOID   )
{
	if ( NULL == DispGlobal.it() )
	{
		return TRUE;
	}
	while ( ::InterlockedCompareExchange( &DispGlobal.it()->g_Critical, 1, 0 ) );

	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
#ifndef CS_WIN64
	#if defined (_DEBUG) || defined  (RELEASE_TRACE)
			g_pTraceMngr = new CTraceManager();
	#endif
#endif
			if (0 == DispGlobal.it()->g_ProcessAttachedCount )
				InitializationGlobal();

			InitializationPerProcess();
			DispGlobal.it()->g_ProcessAttachedCount++;
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;

		case DLL_PROCESS_DETACH:

			CleanupPerProcess();
			
			if (NULL != DispGlobal.it())
			{
				DispGlobal.it()->g_ProcessAttachedCount--;
				if (0 == DispGlobal.it()->g_ProcessAttachedCount)
					CleanupGlobal();
			}

#ifndef CS_WIN64
			delete g_pTraceMngr;
#endif
			break;
    }

	::InterlockedExchange( &DispGlobal.it()->g_Critical, 0 );

    return TRUE;
}

#else

static SharedMem< SHARED_DISP_GLOBALS > DispGlobal __attribute__ ((init_priority (101))) (DISP_SHARED_MEM_FILE);

void disp_cleanup(void)
{
//	printf("cleanup_Disp called\n");

	//EG Missing lock increment

	CleanupPerProcess();

	if (NULL != DispGlobal.it())
	{
		DispGlobal.it()->g_ProcessAttachedCount--;
		if (0 == DispGlobal.it()->g_ProcessAttachedCount)
			CleanupGlobal();
	}

//	printf("cleanup_Disp done\n");
	//EG Missing lock release
}


void __attribute__((constructor)) init_Disp(void)
{
//	printf("init_Disp called\n");

	//EG Missing lock increment


	if (0 == DispGlobal.it()->g_ProcessAttachedCount )
		InitializationGlobal();

	InitializationPerProcess();
	DispGlobal.it()->g_ProcessAttachedCount++;
//RG
	int i = atexit(disp_cleanup);
	if (i != 0)
		TRACE(DISP_TRACE_LEVEL, "error creating disp_cleanup");
//RG
	//EG Missing lock release
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
/*
void __attribute__ ((destructor)) cleanup_Disp(void)
{
	printf("cleanup_Disp called\n");

	//EG Missing lock increment

	CleanupPerProcess();

	if (NULL != DispGlobal.it())
	{
		DispGlobal.it()->g_ProcessAttachedCount--;
		if (0 == DispGlobal.it()->g_ProcessAttachedCount)
			CleanupGlobal();
	}

	printf("cleanup_Disp done\n");
	//EG Missing lock release
}
*/

#endif

//-----------------------------------------------------------------------------
// Initialization global once
//-----------------------------------------------------------------------------
uInt32	InitializationGlobal()
{
	DispGlobal.it()->g_NumOfDrivers = GetNumberOfDrivers();
	if (DispGlobal.it()->g_NumOfDrivers > 0 )
	{
		// Get the DLL file names from registry
		GetDrvDllFileNames();
	}
	return 0;
}
//-----------------------------------------------------------------------------
//
//	 Initialization  per process
//
//-----------------------------------------------------------------------------
int32	InitializationPerProcess()
{

	if (DispGlobal.it()->g_NumOfDrivers == 0 )
	{
		return -1;
	}

    // Allocate memory for DrvDLL node
	g_pDrvDllNode = ( PDRV_DLL_NODE) VirtualAlloc(NULL, sizeof(DRV_DLL_NODE) * MAX_DRIVERS, MEM_COMMIT, PAGE_READWRITE);

	_ASSERT(g_pDrvDllNode != NULL);


	// MEM_COMMIT flag make sure the memory region is set to 0
	// There is no need to call memset for g_pDrvDllNode

	memset( g_pDrvDllNode, 0 , sizeof(DRV_DLL_NODE) * MAX_DRIVERS );
	
	GetHwDllFunctionPointers();

	for ( uInt32 i = 0; i < DispGlobal.it()->g_NumOfDrivers; i++ )
	{
		if ( 0 == g_pDrvDllNode[i].DrvDllHandle )
			continue;

//		if ( g_pDrvDllNode[i]._CsDrvGetBoardType != NULL )
//			g_pDrvDllNode[i]._CsDrvGetBoardType( &g_pDrvDllNode[i].u32MinBoadType, 
//												 &g_pDrvDllNode[i].u32MaxBoadType );
	}

	return 0;
}

//-----------------------------------------------------------------------------
//
//		Clean up per process
//
//-----------------------------------------------------------------------------
void CleanupPerProcess()
{
	if (NULL != DispGlobal.it())
	{
		for (uInt32 i = 0; i < DispGlobal.it()->g_NumOfDrivers; i++)
		{
			if ( g_pDrvDllNode[i].DrvDllHandle )
				::FreeLibrary( g_pDrvDllNode[i].DrvDllHandle );
		}
	}
	::VirtualFree( g_pDrvDllNode, 0, MEM_RELEASE );

}
//-----------------------------------------------------------------------------
void CleanupGlobal()
{
	//
	// Any thing that we need to clean up before Dll is unloaded goes here ...
	//
	DispGlobal.Clear();
}


//-----------------------------------------------------------------------------
void	GetHwDllFunctionPointers()
{
	HMODULE		hModule;
	for ( uInt32 i = 0; i < DispGlobal.it()->g_NumOfDrivers; i++ )
	{
		strcpy_s(g_pDrvDllNode[i].DrvName, sizeof(g_pDrvDllNode[i].DrvName), DispGlobal.it()->g_DrvDLLNames[i]);

		// These next few lines just for linux for now
#ifdef __linux__
		string libraryName;
		libraryName = "lib";
		libraryName.append(g_pDrvDllNode[i].DrvName);
		libraryName.append(".so");
		strcpy(g_pDrvDllNode[i].DrvName, libraryName.c_str());
#endif

		// Attempt to open the specific HW Dll

		hModule = LoadLibrary( g_pDrvDllNode[i].DrvName );

		if ( hModule )
		{
		  	// Get all function pointers from specific HW DLL
			g_pDrvDllNode[i].DrvDllHandle = hModule;
			g_pDrvDllNode[i].GetAcquisitionSystemCount = (LPGetAcquisitionSystemCount)
									GetProcAddress(hModule, "CsDrvGetAcquisitionSystemCount");
			g_pDrvDllNode[i].GetAcquisitionSystemHandles = (LPGetAcquisitionSystemHandles)
									GetProcAddress(hModule, "CsDrvGetAcquisitionSystemHandles");
			g_pDrvDllNode[i].GetAcquisitionSystemInfo = (LPGetAcquisitionSystemInfo)
									GetProcAddress(hModule, "CsDrvGetAcquisitionSystemInfo");
			g_pDrvDllNode[i].GetAcquisitionSystemCaps = (LPGetAcquisitionSystemCaps)
									GetProcAddress(hModule, "CsDrvGetAcquisitionSystemCaps");
			g_pDrvDllNode[i].AcquisitionSystemCleanup = (LPAcquisitionSystemCleanup)
									GetProcAddress(hModule, "CsDrvAcquisitionSystemCleanup");
			g_pDrvDllNode[i].GetAcquisitionStatus =  (LPGetAcquisitionStatus)
									GetProcAddress(hModule, "CsDrvGetAcquisitionStatus");
			g_pDrvDllNode[i].GetBoardsInfo = (LPGetBoardsInfo)
									GetProcAddress(hModule, "CsDrvGetBoardsInfo");
			g_pDrvDllNode[i].SetParams = (LPSetParams)
									GetProcAddress(hModule, "CsDrvSetParams");
			g_pDrvDllNode[i].ValidateParams = (LPValidateParams)
									GetProcAddress(hModule, "CsDrvValidateParams");
			g_pDrvDllNode[i].GetParams = (LPGetParams)
									GetProcAddress(hModule, "CsDrvGetParams");
			g_pDrvDllNode[i].DrvDo = (LPDrvDo)
									GetProcAddress(hModule, "CsDrvDo");
			g_pDrvDllNode[i].TransferData = (LPTransferData)
									GetProcAddress(hModule, "CsDrvTransferData");
			g_pDrvDllNode[i].TransferDataEx = (LPTransferDataEx)
									GetProcAddress(hModule, "CsDrvTransferDataEx");
			g_pDrvDllNode[i].RegisterEventHandle = (LPRegisterEventHandle)
									GetProcAddress(hModule, "CsDrvRegisterEventHandle");
			g_pDrvDllNode[i].AcquisitionSystemInit = (LPAcquisitionSystemInit)
									GetProcAddress(hModule, "CsDrvAcquisitionSystemInit");
			g_pDrvDllNode[i].GetAsyncTransferDataResult = (LPGetAsyncTransferDataResult)
									GetProcAddress(hModule, "CsDrvGetAsyncTransferDataResult");
			g_pDrvDllNode[i].OpenSystemForRm = (LPOpenSystemForRm)
									GetProcAddress(hModule, "CsDrvOpenSystemForRm");
			g_pDrvDllNode[i].CloseSystemForRm = (LPCloseSystemForRm)
									GetProcAddress(hModule, "CsDrvCloseSystemForRm");
			g_pDrvDllNode[i].CsDrvAutoFirmwareUpdate = (LPCsDrvAutoFirmwareUpdate)
									GetProcAddress(hModule, "CsDrvAutoFirmwareUpdate");
			g_pDrvDllNode[i].CsDrvExpertCall = (LPCsDrvExpertCall)
									GetProcAddress(hModule, "CsDrvExpertCall");
			g_pDrvDllNode[i].CsDrvCfgAuxIo = (LPCsDrvCfgAuxIo)
									GetProcAddress(hModule, "CsDrvCfgAuxIo");

			g_pDrvDllNode[i]._CsDrvGetBoardType = (LPCsDrvGetBoardType)
									GetProcAddress(hModule, "_CsDrvGetBoardType");
			g_pDrvDllNode[i]._CsDrvGetHwOptionText = (LPCsDrvGetHwOptionText)
									GetProcAddress(hModule, "_CsDrvGetHwOptionText");
			g_pDrvDllNode[i]._CsDrvGetFwOptionText = (LPCsDrvGetFwOptionText)
									GetProcAddress(hModule, "_CsDrvGetFwOptionText");

			g_pDrvDllNode[i]._CsDrvGetFwOptionText = (LPCsDrvGetFwOptionText)
									GetProcAddress(hModule, "_CsDrvGetFwOptionText");

			g_pDrvDllNode[i]._CsDrvGetBoardCaps = (LPCsDrvGetBoardCaps)
									GetProcAddress(hModule, "_CsDrvGetBoardCaps");
//#if defined (_WINDOWS_)			
			g_pDrvDllNode[i]._CsDrvIsMyBoardType = (LPCsDrvIsMyBoardType)
									GetProcAddress(hModule, "_CsDrvIsMyBoardType");
//#endif

			g_pDrvDllNode[i].CsDrvStmAllocateBuffer = (LPStmAllocateBuffer)
									GetProcAddress(hModule, "CsDrvStmAllocateBuffer");
			g_pDrvDllNode[i].CsDrvStmFreeBuffer = (LPStmFreeBuffer)
									GetProcAddress(hModule, "CsDrvStmFreeBuffer");
			g_pDrvDllNode[i].CsDrvStmTransferToBuffer = (LPStmTransferToBuffer)
									GetProcAddress(hModule, "CsDrvStmTransferToBuffer");
			g_pDrvDllNode[i].CsDrvStmGetTransferStatus = (LPStmGetTransferStatus)
									GetProcAddress(hModule, "CsDrvStmGetTransferStatus");

			g_pDrvDllNode[i].CsDrvSetRemoteSystemInUse = (LPDrvSetRemoteSystemInUse)
									GetProcAddress(hModule, "CsDrvSetRemoteSystemInUse");

			g_pDrvDllNode[i].CsDrvIsRemoteSystemInUse = (LPDrvIsRemoteSystemInUse)
									GetProcAddress(hModule, "CsDrvIsRemoteSystemInUse");
			
			g_pDrvDllNode[i].CsDrvGetRemoteUser = (LPDrvGetRemoteUser)
									GetProcAddress(hModule, "CsDrvGetRemoteUser");

			TRACE(DISP_TRACE_LEVEL, "DISP::Loading %s...\n", g_pDrvDllNode[i].DrvName );
		}
		else
		{
			TRACE(DISP_TRACE_LEVEL, "DISP::Cannot load %s. (WinError = %x)\n", g_pDrvDllNode[i].DrvName, GetLastError() );
		}
	}
}

//-----------------------------------------------------------------------------
uInt16 GetNumberOfDrivers()
{
#if defined (_WINDOWS_)
	char	Subkey[128] = GAGE_SW_DRIVERS_REG;
	HKEY	hKey;
    CHAR     achClass[MAX_PATH] = "";  // buffer for class name
    DWORD    cchClassName = MAX_PATH;  // length of class string
    DWORD    cSubKeys;                 // number of subkeys
    DWORD    cbMaxSubKey;              // longest subkey size
    DWORD    cchMaxClass;              // longest class string
    DWORD    cValues;              // number of values for key
    DWORD    cchMaxValue;          // longest value name
    DWORD    cbMaxValueData;       // longest value data
    DWORD    cbSecurityDescriptor; // size of security descriptor
    FILETIME ftLastWriteTime;      // last write time
	uInt32	i32Status;

	DWORD dwFlags = KEY_READ;

	if (Is32on64())
		dwFlags |= KEY_WOW64_64KEY;

	i32Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, Subkey, 0, dwFlags, &hKey);

	if ( ERROR_SUCCESS != i32Status )
	{
		TRACE(DISP_TRACE_LEVEL, "DISP::Error opening Gage Drivers in registry (WinError = %x)\n", GetLastError() );
		return 0;
	}

	// Get the class name and the value count.
    RegQueryInfoKey(hKey,        // key handle
        achClass,                // buffer for class name
        &cchClassName,           // length of class string
        NULL,                    // reserved
        &cSubKeys,               // number of subkeys
        &cbMaxSubKey,            // longest subkey size
        &cchMaxClass,            // longest class string
        &cValues,                // number of values for this key
        &cchMaxValue,            // longest value name
        &cbMaxValueData,         // longest value data
        &cbSecurityDescriptor,   // security descriptor
        &ftLastWriteTime);       // last write time

	RegCloseKey(hKey);

#elif defined __linux__

	char 	filePath[250];
	uInt16 	cValues = 0;
/*
	char* homePath = getenv("HOME");

	strncpy(filePath, homePath, 250);
*/
	strncpy(filePath, XML_FILE_PATH, 250);
	strncat(filePath, "/GageDriver.xml", 250);

	XML *pDrivers = new XML(filePath);

	if (!pDrivers)
		return 0;

	XMLElement* pRoot = pDrivers->GetRootElement();
	if (!pRoot)
	{
		delete pDrivers;
		return 0;
	}

	cValues = pRoot->GetChildrenNum();
	if (0 == cValues)
	{
		delete pDrivers;
		return 0;
	}
	delete pDrivers;
#endif

	if ( cValues > MAX_DRIVERS )
	{
		cValues = MAX_DRIVERS;

		TRACE(DISP_TRACE_LEVEL, "DISP::WARNING : Number of drivers exceeds the maximum number supported.\n");
	}

	return ((uInt16) cValues);
}

//-----------------------------------------------------------------------------
uInt32	GetDrvDllFileNames()
{
#if defined (_WINDOWS_)
	char	Subkey[128] = GAGE_SW_DRIVERS_REG;
	HKEY	hKey;
	char	ValueBuffer[DRIVER_MAX_NAME_LENGTH];
	uInt32 	ValueBuffSize;
	uInt32	DataBuffer;					// LoadOrder
	uInt32 	DataBuffSize;
	DWORD	DataType;
	DWORD	RetVal = 0;
    DWORD	i = 0;
	DWORD	j = 0;
	char	tmpNames[DRIVER_MAX_NAME_LENGTH][DRIVER_MAX_NAME_LENGTH] = {0};
	uInt32	LoadOrder[DRIVER_MAX_NAME_LENGTH] = {0};
	uInt32	Status;

	DWORD dwFlags = KEY_READ;

	if (Is32on64())
		dwFlags |= KEY_WOW64_64KEY;

	Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, Subkey, 0, dwFlags, &hKey);

	if ( ERROR_SUCCESS != Status )
	{
		TRACE(DISP_TRACE_LEVEL, "DISP::Error opening Gage Drivers in registry (WinError = %x)\n", Status );
		return 0;
	}

	// Get the DLL file names from registry
	for(i = 0; i< DispGlobal.it()->g_NumOfDrivers; i++)
	{
		ValueBuffSize = sizeof(ValueBuffer);
		DataBuffSize = sizeof(DataBuffer);
		RetVal = RegEnumValue( hKey, i, ValueBuffer, &ValueBuffSize, NULL,
		            &DataType, (LPBYTE) &DataBuffer, &DataBuffSize );

		if (RetVal == ERROR_NO_MORE_ITEMS)
			break;

		if ( DataType != REG_DWORD )
			continue;

		strcpy_s(tmpNames[j], sizeof(tmpNames[j]), ValueBuffer);
		LoadOrder[j++] =  DataBuffer;
	}

	RegCloseKey(hKey);

	//
	// Sort DLL names according to the Load order
	//
	uInt32 MinLoadOrder = 0xFFFFFFFF;
	uInt32 MinDifference = 0xFFFFFFFF;
    DWORD	 CurrentIndex = 0;
    DWORD	 NextIndex = 0;

	
	// Find the DLL with the minimum Load order. This one 
	// will be the first dll to be loaded
    j = 0;
	for(i = 0; i< DispGlobal.it()->g_NumOfDrivers; i++)
	{
		if ( LoadOrder[i] <= MinLoadOrder )
		{
			MinLoadOrder = LoadOrder[i];
			CurrentIndex = i;
		}
	}

	// Copy the Dll name
	strcpy_s(DispGlobal.it()->g_DrvDLLNames[j], sizeof(DispGlobal.it()->g_DrvDLLNames[j]), tmpNames[CurrentIndex]);
	j++;

	// Mark this one aleady done
	LoadOrder[CurrentIndex] = (uInt32) -1;

	// Find the next Dll with the load order closest to the first one
	// and so on ...
	while ( j <  DispGlobal.it()->g_NumOfDrivers )
	{
		for(i = 0; i< DispGlobal.it()->g_NumOfDrivers; i++)
		{
			// Skip DLLs already done
			if ( LoadOrder[i] == -1 )
				continue;

            if ( LoadOrder[i] - MinLoadOrder <= MinDifference )
			{
				MinDifference = LoadOrder[i] - MinLoadOrder;
				NextIndex = i;
			}
		}

		CurrentIndex = NextIndex;
		MinLoadOrder = LoadOrder[NextIndex];
		LoadOrder[NextIndex] = (uInt32)-1;
		MinDifference = 0xFFFFFFFF;
		strcpy_s(DispGlobal.it()->g_DrvDLLNames[j], sizeof(DispGlobal.it()->g_DrvDLLNames[j]),  tmpNames[NextIndex]);
		j++;
	}
#elif  defined __linux__

	char filePath[250];
/*
	char* homePath = getenv("HOME");

	strncpy(filePath, homePath, 250);
*/
	strncpy(filePath, XML_FILE_PATH, 250);
	strncat(filePath, "/GageDriver.xml", 250);

	XML *pDrivers = new XML(filePath);

	if (!pDrivers)
		return 0;

	XMLElement* pRoot = pDrivers->GetRootElement();
	if (!pRoot)
	{
		delete pDrivers;
		return 0;
	}

	//assert(DispGlobal.it()->g_NumOfDrivers != MIN(pRoot->GetChildrenNum(), MAX_DRIVERS));

	XMLElement** pDriverList = pRoot->GetChildren();
	if (!pDriverList)
	{
		delete pDrivers;
		return 0;
	}

	cschar strDriverName[MAX_PATH];

	for(int i = 0; i< DispGlobal.it()->g_NumOfDrivers; i++)
	{
		XMLVariable* pDriverName = pDriverList[i]->FindVariableZ((char *)"name");

		if ((NULL == pDriverName) || (-1 == pDriverName->GetValue(strDriverName)))
		{
			pDriverList[i]->GetElementName(strDriverName);
		}

		strcpy_s(DispGlobal.it()->g_DrvDLLNames[i], sizeof(DispGlobal.it()->g_DrvDLLNames[i]), strDriverName);
	}

	delete pDrivers;

#endif

	return 0;
}

//-----------------------------------------------------------------------------
// Check the loopup table for specified Handle.
// If found, return the index in lookuptable and the Specific Driver DLL handle.
int32 ValidateHandle(CSHANDLE CsHandle)
{
	uInt32	i;

	for(i = 0; i< DispGlobal.it()->g_NumOfSystems; i++)
	{
		if (DispGlobal.it()->g_LookupTable[i].CsHdl == CsHandle)
		{
			return i;
		}
	}

	TRACE(DISP_TRACE_LEVEL, "DISP::Invalid handle %x\n", CsHandle );
	return -1;
}


//-----------------------------------------------------------------------------

PDRV_DLL_NODE LookUpDrvDllNode( char *DrvDLLName, uInt32  )
{
	uInt32	i;

	for(i = 0; i< DispGlobal.it()->g_NumOfDrivers; i++)
	{
		if ( ! strcmp( DrvDLLName, g_pDrvDllNode[i].DrvName ) )
			return &g_pDrvDllNode[i];				
	}
	
	// Cannot be here

	TRACE(DISP_TRACE_LEVEL, "DISP::Dll name not found in lookup table.\n");
	_ASSERT(FALSE);
	return NULL;

}

//-----------------------------------------------------------------------------
int32 CsDrvGetAcquisitionSystemCount( uInt16 bForceIndependantMode, uInt16 *u16SystemFound )
{
	uInt32 i;
	uInt32	j;
	uInt32	k = 0;						// Index for DLL Lookup table
	uInt32	RetCode;
	bool	bAutoUpdateFirmwareRequired = false;
	int32	i32Status = CS_SUCCESS;

	// Assume that the only process connected and calls this function is
	// the Resource Manager.
	// Otherwise this function should be proteced for race condition.

	// Race condition start

	if ( DispGlobal.it()->g_NumOfDrivers == 0 )
		return 0;

	// Reset lookup table
	for ( i = 0; i< DispGlobal.it()->g_NumOfSystems; i++ )
	{
		// Save the Drv Handle and the DllNode
		DispGlobal.it()->g_LookupTable[i].DrvHdl = 0;
		strcpy_s(DispGlobal.it()->g_LookupTable[i].DrvName, sizeof(DispGlobal.it()->g_LookupTable[i].DrvName), "");
	}
	DispGlobal.it()->g_NumOfSystems = 0;

	if ( bForceIndependantMode != DispGlobal.it()->g_bForceIndependantMode )
	{
		DispGlobal.it()->g_bForceIndependantMode = bForceIndependantMode;
/*		//
		// The number of drivers may have changed because
		// Instrument Manager may modify registry for adding demo systems
		// Reload the number of drivers and Dll file names
		//
		for ( i = 0; i < DispGlobal.it()->g_NumOfDrivers; i ++ )
		{
			if( g_pDrvDllNode[i].DrvDllHandle )
				FreeLibrary( g_pDrvDllNode[i].DrvDllHandle );
		}

		memset( g_pDrvDllNode, 0, sizeof(DRV_DLL_NODE) * MAX_DRIVERS );
		DispGlobal.it()->g_NumOfDrivers = GetNumberOfDrivers();
		if (DispGlobal.it()->g_NumOfDrivers > 0 )
		{
			// Get the DLL file names from registry
			GetDrvDllFileNames();
			GetHwDllFunctionPointers();
		}
*/
	}
	// Find out the total of acquisition system found
	//
	DRVHANDLE TempHdl[MAX_ACQ_SYSTEMS];	// Temporary place to hold Drv handles
	uInt16	SystemCountPerDriver;

	// First detect if an auto update firmware is required
	for (i = 0; i< DispGlobal.it()->g_NumOfDrivers; i++)
	{
		if ( g_pDrvDllNode[i].DrvDllHandle == NULL )
			continue;

		// Send down the request to every HW specific DLL
		SystemCountPerDriver = 0;
		if ( NULL != g_pDrvDllNode[i].GetAcquisitionSystemCount )
		{
			i32Status = g_pDrvDllNode[i].GetAcquisitionSystemCount( bForceIndependantMode, &SystemCountPerDriver );
			if ( CS_INVALID_FW_VERSION == i32Status )
				bAutoUpdateFirmwareRequired = true;
		}
	}

	if ( bAutoUpdateFirmwareRequired )
	{
		DispGlobal.it()->g_NumOfSystems = *u16SystemFound = 0;
		return CS_INVALID_FW_VERSION;
	}

	// Get the acquisition system count per driver
	for (i = 0; i< DispGlobal.it()->g_NumOfDrivers; i++)
	{
		if ( g_pDrvDllNode[i].DrvDllHandle == NULL )
			continue;

		// Send down the request to every HW specific DLL
		SystemCountPerDriver = 0;
		if ( NULL != g_pDrvDllNode[i].GetAcquisitionSystemCount )
		{
			i32Status = g_pDrvDllNode[i].GetAcquisitionSystemCount( bForceIndependantMode, &SystemCountPerDriver );
			if ( CS_FAILED( i32Status ) )
			{
				DispGlobal.it()->g_NumOfSystems = *u16SystemFound = 0;
				return i32Status;
			}			
		}

		if ( 0 == SystemCountPerDriver )
			continue;

		// Sum of all acquisition systems found
		DispGlobal.it()->g_NumOfSystems = DispGlobal.it()->g_NumOfSystems + SystemCountPerDriver;

		if ( g_pDrvDllNode[i].DrvDllHandle )
		{
			RetCode = g_pDrvDllNode[i].GetAcquisitionSystemHandles( TempHdl, sizeof(TempHdl) );
			_ASSERT( RetCode == SystemCountPerDriver);

			// Build the DLL look up table
			for ( j = 0; j< SystemCountPerDriver; j++ )
			{
				// Save the Drv Handle and the DllNode
				DispGlobal.it()->g_LookupTable[k].DrvHdl = TempHdl[j];
				strcpy_s(DispGlobal.it()->g_LookupTable[k].DrvName, sizeof(DispGlobal.it()->g_LookupTable[k].DrvName), g_pDrvDllNode[i].DrvName);
				k++;
			}
		}
	}

	_ASSERT(k == DispGlobal.it()->g_NumOfSystems);

	// Race condition end
	*u16SystemFound = DispGlobal.it()->g_NumOfSystems;

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
uInt32 CsDrvGetAcquisitionSystemHandles(PDRVHANDLE pDrvHandle, uInt32 SizeOfCSHandle )
{
	uInt32	i;			// loop counter
	uInt32	Count;		// Size o


	// Validation of the size of buffer received
	Count = SizeOfCSHandle / sizeof(CSHANDLE);
	if ( Count >= DispGlobal.it()->g_NumOfSystems )
		Count = DispGlobal.it()->g_NumOfSystems;

	// Copy back all Handles to the application's buffer
	for (i = 0; i < Count; i++)
	{
		if ( !DispGlobal.it()->g_LookupTable[i].DrvHdl )
			break;

		pDrvHandle[i] = DispGlobal.it()->g_LookupTable[i].DrvHdl;
	}

	_ASSERT(i == Count);

	return Count;
}

//-----------------------------------------------------------------------------

int32 CsDrvAcquisitionSystemInit( CSHANDLE	CsHandle, BOOL bSystemDefaultSetting )
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->AcquisitionSystemInit )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->AcquisitionSystemInit(DispGlobal.it()->g_LookupTable[i].DrvHdl,
																		 bSystemDefaultSetting );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}

	return DispGlobal.it()->g_DispatchDllLastError;
}


//-----------------------------------------------------------------------------
int32 CsDrvAcquisitionSystemCleanup(CSHANDLE CsHandle)
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError =  CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if (pDrvDllNode->AcquisitionSystemCleanup)
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->AcquisitionSystemCleanup(
																DispGlobal.it()->g_LookupTable[i].DrvHdl );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}


//-----------------------------------------------------------------------------
int32 CsDrvUpdateLookupTable(DRVHANDLE DrvHandle, CSHANDLE CsHandle)
{
	uInt32	i;

	for(i = 0; i< DispGlobal.it()->g_NumOfSystems; i++)
	{
		if (DispGlobal.it()->g_LookupTable[i].DrvHdl == DrvHandle)
		{
			DispGlobal.it()->g_LookupTable[i].CsHdl = CsHandle;
			return CS_SUCCESS;
		}
	}
	
	return (DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE);
}


//-----------------------------------------------------------------------------
 int32 CsDrvGetAcquisitionSystemInfo(CSHANDLE CsHandle, PCSSYSTEMINFO pSysInfo)
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError =  CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->GetAcquisitionSystemInfo )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->GetAcquisitionSystemInfo(
																DispGlobal.it()->g_LookupTable[i].DrvHdl,
																pSysInfo );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}



//-----------------------------------------------------------------------------
int32 CsDrvGetBoardsInfo(CSHANDLE CsHandle, PARRAY_BOARDINFO pABoardInfo)
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError =  CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->GetBoardsInfo )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->GetBoardsInfo(
																DispGlobal.it()->g_LookupTable[i].DrvHdl,
																(PARRAY_BOARDINFO)pABoardInfo );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}



//-----------------------------------------------------------------------------
int32 CsDrvGetAcquisitionSystemCaps(CSHANDLE CsHandle, uInt32 CapsId, PCSSYSTEMCONFIG pSysCfg,
									 void *pBuffer, uInt32 *BufferSize)
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;


	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError =  CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->GetAcquisitionSystemCaps  )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->GetAcquisitionSystemCaps( 
														DispGlobal.it()->g_LookupTable[i].DrvHdl,
														CapsId,
														pSysCfg,
														pBuffer,
														BufferSize );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}

//-----------------------------------------------------------------------------
int32 CsDrvGetParams(CSHANDLE CsHandle, uInt32 ParamID, void *ParamBuffer)
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->GetParams )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->GetParams(DispGlobal.it()->g_LookupTable[i].DrvHdl,
												ParamID,
												ParamBuffer );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}

//-----------------------------------------------------------------------------
int32 CsDrvSetParams(CSHANDLE CsHandle, uInt32 ParamID, void *ParamBuffer)
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->SetParams )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->SetParams(DispGlobal.it()->g_LookupTable[i].DrvHdl,
														ParamID,
														ParamBuffer );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}

//-----------------------------------------------------------------------------
int32 CsDrvValidateParams(CSHANDLE CsHandle, uInt32 ParamID, uInt32 Coerce, void *ParamBuffer)
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
		DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->ValidateParams )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->ValidateParams(DispGlobal.it()->g_LookupTable[i].DrvHdl,
														ParamID,
														Coerce,
														ParamBuffer );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}

//-----------------------------------------------------------------------------
int32 CsDrvDo(CSHANDLE CsHandle, uInt32 ActionID, void *ActionBuffer)
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->DrvDo && !DispGlobal.it()->g_bForceIndependantMode )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->DrvDo( 
														DispGlobal.it()->g_LookupTable[i].DrvHdl,
														ActionID, ActionBuffer );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}

//-----------------------------------------------------------------------------
int32 CsDrvTransferData(CSHANDLE CsHandle, IN_PARAMS_TRANSFERDATA InParams,
							POUT_PARAMS_TRANSFERDATA	OutParams )
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->TransferData && !DispGlobal.it()->g_bForceIndependantMode )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->TransferData(DispGlobal.it()->g_LookupTable[i].DrvHdl,
																InParams,
                                                                OutParams );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}

//-----------------------------------------------------------------------------
int32 CsDrvTransferDataEx(CSHANDLE CsHandle, PIN_PARAMS_TRANSFERDATA_EX InParamsEx, POUT_PARAMS_TRANSFERDATA_EX	OutParamsEx )
{
#if defined(_WINDOWS_)
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}
	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->TransferDataEx && !DispGlobal.it()->g_bForceIndependantMode )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->TransferDataEx(DispGlobal.it()->g_LookupTable[i].DrvHdl,
																InParamsEx,
																OutParamsEx );
		}
		else
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
	}
#else
	DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
#endif
	return DispGlobal.it()->g_DispatchDllLastError;
}

//-----------------------------------------------------------------------------
#if defined(_WINDOWS_)
int32 CsDrvRegisterEventHandle(CSHANDLE CsHandle, uInt32 EventType, EVENT_HANDLE *EventHandle)
#else
int32 CsDrvRegisterEventHandle(CSHANDLE CsHandle, uInt32 EventType, /*EVENT_HANDLE*/ HANDLE *EventHandle)
#endif
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
		DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->RegisterEventHandle && !DispGlobal.it()->g_bForceIndependantMode )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->RegisterEventHandle(
															DispGlobal.it()->g_LookupTable[i].DrvHdl,
															EventType, EventHandle );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}


//-----------------------------------------------------------------------------

int32 CsDrvGetAcquisitionStatus(CSHANDLE CsHandle, uInt32 *AcqStatus)
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->GetAcquisitionStatus && !DispGlobal.it()->g_bForceIndependantMode )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->GetAcquisitionStatus( 
														DispGlobal.it()->g_LookupTable[i].DrvHdl,
														AcqStatus );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}

//-----------------------------------------------------------------------------
int32 CsDrvGetAsyncTransferDataResult( CSHANDLE CsHandle, uInt8 Channel, 
									   CSTRANSFERDATARESULT *pTxDataResult, BOOL bWait )
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->GetAsyncTransferDataResult && !DispGlobal.it()->g_bForceIndependantMode)
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->GetAsyncTransferDataResult( 
															DispGlobal.it()->g_LookupTable[i].DrvHdl,
															Channel, 
															pTxDataResult, bWait );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}


//-----------------------------------------------------------------------------
int32 CsDrvRegisterStreamingBuffers( CSHANDLE CsHandle, uInt8 u8CardIndex, CSFOLIOARRAY *pStmBufferList, HANDLE *hSemaphore, PSTMSTATUS *pStmStatus )
{
	UNREFERENCED_PARAMETER(CsHandle);
	UNREFERENCED_PARAMETER(u8CardIndex);
	UNREFERENCED_PARAMETER(pStmBufferList);
	UNREFERENCED_PARAMETER(hSemaphore);
	UNREFERENCED_PARAMETER(pStmStatus);
	DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;

	return DispGlobal.it()->g_DispatchDllLastError;
}

//-----------------------------------------------------------------------------
int32 CsDrvReleaseStreamingBuffers( CSHANDLE CsHandle, uInt8 u8CardIndex )
{
	UNREFERENCED_PARAMETER(CsHandle);
	UNREFERENCED_PARAMETER(u8CardIndex);
	DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
	
	return DispGlobal.it()->g_DispatchDllLastError;
}


//-----------------------------------------------------------------------------

int32 CsDrvOpenSystemForRm( CSHANDLE CsHandle )
{
	TCHAR tstrProcessName[_MAX_PATH];

#ifdef __linux__
	::GetProcessName(getpid(), tstrProcessName);
	char *strName = strrchr(tstrProcessName, '/');
	if (NULL != strName)
		strName = strName + 1; // skip over the /
	else
		strName = tstrProcessName; // there's no path, only the process name

	// strip off everything but the process name

#else
	::GetModuleBaseName( GetCurrentProcess(), NULL, tstrProcessName, _countof(tstrProcessName) );
#endif

#ifdef __linux__
	if ( _tcsicmp( strName, _T("csrmd") ) )
#else // not linux

#ifdef CS_WIN64
	if ( _tcsicmp( tstrProcessName, _T("CsRm64.exe") ) )
#else
	if ( _tcsicmp( tstrProcessName, _T("CsRm.exe") ) && _tcsicmp( tstrProcessName, _T("GageServer.exe") ) )
#endif
#endif
		return CS_MISC_ERROR;

	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->OpenSystemForRm )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->OpenSystemForRm(DispGlobal.it()->g_LookupTable[i].DrvHdl );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}

//-----------------------------------------------------------------------------

int32 CsDrvCloseSystemForRm( CSHANDLE CsHandle )
{
	TCHAR tstrProcessName[_MAX_PATH];

#ifdef __linux__
	::GetProcessName(getpid(), tstrProcessName);
	char *strName = strrchr(tstrProcessName, '/');
	if (NULL != strName)
		strName = strName + 1; // skip over the /
	else
		strName = tstrProcessName; // there's no path, only the process name

	// strip off everything but the process name
#else
	::GetModuleBaseName( GetCurrentProcess(), NULL, tstrProcessName, _countof(tstrProcessName) );
#endif

#ifdef __linux__
	if ( _tcsicmp(  strName, _T("csrmd") ) )
		return CS_MISC_ERROR;
#else

#ifdef CS_WIN64
	if ( _tcsicmp(  tstrProcessName, _T("CsRm64.exe") ) )
		return CS_MISC_ERROR;
#else
		if ( _tcsicmp( tstrProcessName, _T("CsRm.exe") ) && _tcsicmp( tstrProcessName, _T("GageServer.exe") ) )
		return CS_MISC_ERROR;
#endif
#endif


	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->CloseSystemForRm )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->CloseSystemForRm(DispGlobal.it()->g_LookupTable[i].DrvHdl );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}



//-----------------------------------------------------------------------------

int32 CsDrvAutoFirmwareUpdate()
{
	int32	i32Status = CS_SUCCESS;

	DispGlobal.it()->g_DispatchDllLastError = CS_SUCCESS;
	for (uInt16 i = 0; i< DispGlobal.it()->g_NumOfDrivers; i++)
	{
		if ( g_pDrvDllNode[i].DrvDllHandle == NULL )
			continue;

		// Send down the request to every HW specific DLL
		if ( g_pDrvDllNode[i].CsDrvAutoFirmwareUpdate )
		{
			i32Status = g_pDrvDllNode[i].CsDrvAutoFirmwareUpdate();
			if ( CS_FAILED( i32Status ) )
			{
				DispGlobal.it()->g_DispatchDllLastError = i32Status;
			}			
		}
	}

	if ( CS_FAILED( i32Status ) )
		return i32Status;
	
	if ( CS_FAILED(DispGlobal.it()->g_DispatchDllLastError ) )
		return DispGlobal.it()->g_DispatchDllLastError;

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int32 CsDrvExpertCall(CSHANDLE CsHandle, void *FuncParams)
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->CsDrvExpertCall && !DispGlobal.it()->g_bForceIndependantMode )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->CsDrvExpertCall(DispGlobal.it()->g_LookupTable[i].DrvHdl,
																 FuncParams );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}

	return DispGlobal.it()->g_DispatchDllLastError;
}
//-----------------------------------------------------------------
//-----------------------------------------------------------------
int32 CsDrvCfgAuxIo(CSHANDLE CsHandle)
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
		DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
	else if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->CsDrvCfgAuxIo )
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->CsDrvCfgAuxIo(DispGlobal.it()->g_LookupTable[i].DrvHdl);
		else
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
	}

	return DispGlobal.it()->g_DispatchDllLastError;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 _CsDrvGetHwOptionText(uInt32 u32BoardType, PCS_HWOPTIONS_CONVERT2TEXT pHwOptionsText)
{
	for ( uInt32 i = 0; i < DispGlobal.it()->g_NumOfDrivers; i++ )
	{
		if ( 0 == g_pDrvDllNode[i].DrvDllHandle )
			continue;

		if ( NULL != g_pDrvDllNode[i]._CsDrvIsMyBoardType &&
			g_pDrvDllNode[i]._CsDrvIsMyBoardType(u32BoardType) )
		{
			if ( g_pDrvDllNode[i]._CsDrvGetHwOptionText != NULL )
			{
				DispGlobal.it()->g_DispatchDllLastError = g_pDrvDllNode[i]._CsDrvGetHwOptionText( u32BoardType, pHwOptionsText );
				break;
			}
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}

	return DispGlobal.it()->g_DispatchDllLastError;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 _CsDrvGetFwOptionText(uInt32 u32BoardType, PCS_HWOPTIONS_CONVERT2TEXT pFwOptionsText)
{

	for ( uInt32 i = 0; i < DispGlobal.it()->g_NumOfDrivers; i++ )
	{
		if ( 0 == g_pDrvDllNode[i].DrvDllHandle )
			continue;

		if ( NULL != g_pDrvDllNode[i]._CsDrvIsMyBoardType &&
			g_pDrvDllNode[i]._CsDrvIsMyBoardType(u32BoardType) )
		{
			if ( g_pDrvDllNode[i]._CsDrvGetFwOptionText != NULL )
			{
				DispGlobal.it()->g_DispatchDllLastError = g_pDrvDllNode[i]._CsDrvGetFwOptionText( pFwOptionsText );
				break;
			}
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}

	return DispGlobal.it()->g_DispatchDllLastError;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 _CsDrvGetFwlCsiInfo( PFWLnCSI_INFO pBuffer, uInt32 *u32BufferSize )
{
	if ( NULL == pBuffer )
	{
		if ( NULL == u32BufferSize || IsBadWritePtr( u32BufferSize, sizeof(uInt32)) )
			return CS_INVALID_POINTER_BUFFER;
		else
		{
			*u32BufferSize = sizeof(g_GageProduct);
			return  CS_SUCCESS;
		}
	}

	if ( IsBadWritePtr( pBuffer, *u32BufferSize ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( *u32BufferSize < sizeof (g_GageProduct) )
		return CS_BUFFER_TOO_SMALL;

	memset( pBuffer, 0, *u32BufferSize );
	memcpy( pBuffer, g_GageProduct, sizeof(g_GageProduct) );
	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 _CsDrvGetBoardCaps(uInt32	u32ProductId, uInt32 CapsId, void *pBuffer, uInt32 *BufferSize)
{
	uInt32	i;
	uInt32	j;


	for ( i = 0; i < sizeof(g_GageProduct)/sizeof(FWLnCSI_INFO); i++ )
	{
		if (u32ProductId == g_GageProduct[i].u32ProductId)
			break;
	}

	if ( i > sizeof(g_GageProduct)/sizeof(FWLnCSI_INFO))
		return CS_INVALID_HANDLE;

	for ( j = 0; j < DispGlobal.it()->g_NumOfDrivers; ++j )
	{
		if ( 0 == g_pDrvDllNode[j].DrvDllHandle )
			continue;
		if ( 0 == strcmp( g_pDrvDllNode[j].DrvName, g_GageProduct[i].szHwDllName ) )
			break;
	}

	if ( j > DispGlobal.it()->g_NumOfDrivers )
		return CS_INVALID_HANDLE;

	DispGlobal.it()->g_DispatchDllLastError = g_pDrvDllNode[j]._CsDrvGetBoardCaps(g_GageProduct[i].u32ProductId, CapsId, pBuffer, BufferSize );
	return DispGlobal.it()->g_DispatchDllLastError;
}

//-----------------------------------------------------------------------------
uInt32 CsDrvGetAcquisitionSystemHandlesDbg(PCSHANDLE pDrvHandle, uInt32 SizeOfpCSHandle )
{
	uInt32	i;			// loop counter
	uInt32	Count;		// Size o


	// Validation of the size of buffer received
	Count = SizeOfpCSHandle / sizeof(CSHANDLE);
	if ( Count >= DispGlobal.it()->g_NumOfSystems )
		Count = DispGlobal.it()->g_NumOfSystems;

	// Copy back all Handles to the application's buffer
	for (i = 0; i < Count; i++)
	{
		if ( !DispGlobal.it()->g_LookupTable[i].CsHdl )
			break;

		pDrvHandle[i] =DispGlobal.it()->g_LookupTable[i].CsHdl;
	}

	_ASSERT(i == Count);

	return Count;
}

//-----------------------------------------------------------------------------
int32 CsDrvStmAllocateBuffer( CSHANDLE CsHandle, int32 nCardIndex, uInt32 u32BufferSize, PVOID *pVa )
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->CsDrvStmAllocateBuffer && !DispGlobal.it()->g_bForceIndependantMode )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->CsDrvStmAllocateBuffer( 
														DispGlobal.it()->g_LookupTable[i].DrvHdl, nCardIndex,
														u32BufferSize, pVa );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}


//-----------------------------------------------------------------------------
int32 CsDrvStmFreeBuffer( CSHANDLE CsHandle, int32 nCardIndex, PVOID pVa )
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->CsDrvStmFreeBuffer && !DispGlobal.it()->g_bForceIndependantMode )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->CsDrvStmFreeBuffer( 
														DispGlobal.it()->g_LookupTable[i].DrvHdl, nCardIndex,
														pVa );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}


//-----------------------------------------------------------------------------
int32 CsDrvStmTransferToBuffer( CSHANDLE CsHandle, int32 nCardIndex, PVOID pBuffer, uInt32 u32TransferSize )
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->CsDrvStmTransferToBuffer && !DispGlobal.it()->g_bForceIndependantMode )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->CsDrvStmTransferToBuffer( 
														DispGlobal.it()->g_LookupTable[i].DrvHdl, nCardIndex,
														pBuffer, u32TransferSize );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}



//-----------------------------------------------------------------------------
int32 CsDrvStmGetTransferStatus( CSHANDLE CsHandle, int32 nCardIndex, uInt32 u32TimeoutMs, uInt32 *u32ErrorFlag, uInt32 *u32ActualSize, PVOID Reserved )
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->CsDrvStmGetTransferStatus && !DispGlobal.it()->g_bForceIndependantMode )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->CsDrvStmGetTransferStatus( 
														DispGlobal.it()->g_LookupTable[i].DrvHdl, nCardIndex, 
														u32TimeoutMs, u32ErrorFlag, u32ActualSize, (uInt32*)Reserved );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}

//-----------------------------------------------------------------------------
int32 CsDrvSetRemoteSystemInUse( CSHANDLE CsHandle, BOOL bSet )
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->CsDrvSetRemoteSystemInUse && ((CsHandle & RM_REMOTE_HANDLE) != 0) )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->CsDrvSetRemoteSystemInUse( 
														DispGlobal.it()->g_LookupTable[i].DrvHdl, bSet );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}
//-----------------------------------------------------------------------------
int32 CsDrvIsRemoteSystemInUse( CSHANDLE CsHandle, BOOL* pbSet )
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->CsDrvIsRemoteSystemInUse && ((CsHandle & RM_REMOTE_HANDLE) != 0) )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->CsDrvIsRemoteSystemInUse( 
														DispGlobal.it()->g_LookupTable[i].DrvHdl, pbSet );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}
//-----------------------------------------------------------------------------
int32 CsDrvGetRemoteUser( CSHANDLE CsHandle, TCHAR* lpUser )
{
	int32		i;
	PDRV_DLL_NODE	pDrvDllNode;

	if ((i = ValidateHandle( CsHandle )) < 0)
	{
        DispGlobal.it()->g_DispatchDllLastError = CS_INVALID_HANDLE;
		return DispGlobal.it()->g_DispatchDllLastError;
	}

	if ( NULL != (pDrvDllNode = LookUpDrvDllNode (DispGlobal.it()->g_LookupTable[i].DrvName, GetCurrentProcessId() )) )
	{
		// Re-direct the call to the appropriate HW Specific DLL
		if ( pDrvDllNode->CsDrvGetRemoteUser && ((CsHandle & RM_REMOTE_HANDLE) != 0) )
		{
			DispGlobal.it()->g_DispatchDllLastError = pDrvDllNode->CsDrvGetRemoteUser( 
														DispGlobal.it()->g_LookupTable[i].DrvHdl, lpUser );
		}
		else
		{
			DispGlobal.it()->g_DispatchDllLastError = CS_FUNCTION_NOT_SUPPORTED;
		}
	}
	return DispGlobal.it()->g_DispatchDllLastError;
}

