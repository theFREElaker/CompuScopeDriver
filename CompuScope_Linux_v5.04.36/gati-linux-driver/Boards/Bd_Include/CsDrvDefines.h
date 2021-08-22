#ifndef	__CSDRVDEFINE_H__

#define __CSDRVDEFINE_H__

//#include <vdw.h>
#include "CsTypes.h"
#include "CsErrors.h"

#define		MAX_FILENAME_LENGTH			128

#ifdef _WINDOWS_

#define		CS8XXX_FPGA_IMAGE_FILE_NAME		"Cs8xxx"
#define		CS8YYY_FPGA_IMAGE_FILE_NAME		"Cs8yyy"
#define		CS8ZZZ_FPGA_IMAGE_FILE_NAME		"Cs8zzz"
#define		CSZAP_FPGA_IMAGE_FILE_NAME		"CsZap"
#define		CSXYG8_FPGA_IMAGE_FILE_NAME		"CsxyG8"
#define		CSSPLENDA_FPGA_IMAGE_FILE_NAME	"Cs16xyy"
#define		CSPITCH_FPGA_IMAGE_FILE_NAME	"CscdG8p"
#define		CSPITCHM_FPGA_IMAGE_FILE_NAME	"CscdG8pm"
#define		CSFASTBL4G_FPGA_IMAGE_FILE	    "CscdG84g"
#define		CSFASTBL3G_FPGA_IMAGE_FILE      "CscdG83g"
#define		CSFASTBL_NM_CSI_FILE_NAME       "CscdG8nm"
#define		DEERE_FPGA_IMAGE_FILE_NAME	    "CscdG12"
#define		CS8YYYPCIEX_FPGAIMAGE_FILENAME	"CsE8xxx"
#define		CSSPLENDA_PCIEX_FPGAIMAGE_FILENAME	"CsE16xyy"
#define		CSOSCAR_PCIEX_FPGAIMAGE_FILENAME	"CsE16xyo"
#define		CSZAPPCIEX_FPGAIMAGE_FILENAME	"CsEZap"
#define		CSXYG8_PCIEX_FPGAIMAGE_FILENAME		"CsExyG8"
#define		CSCDG8_PCIEX_FPGAIMAGE_FILENAME		"CsEcdG8"
#define		CSDECADE12_FPGAIMAGE_FILENAME		"CsE12xGy"
#define		CSHEXAGON_FPGAIMAGE_FILENAME		"CsE16bcd"

#define		CS14200_FWL_FILENAME	"Cs14200.fwl"
#define		CS14105_FWL_FILENAME	"Cs14105.fwl"
#define		CS12400_FWL_FILENAME	"Cs12400.fwl"
#define		CS14200x_FWL_FILENAME	"Cs14200x.fwl"
#define		CS8XXX_FWL_FILENAME		"Cs8xxx.fwl"
#define		CS8YYY_FWL_FILENAME		"Cs8yyy.fwl"
#define		CS8ZZZ_FWL_FILENAME		"Cs8zzz.fwl"
#define		CSZAP_FWL_FILENAME		"CsZap.fwl"
#define		CSXYG8_FWL_FILENAME		"CsxyG8.fwl"
#define		CS12400FAT_FWL_FILENAME	"Cs12400fat.fwl"
#define		CSBUNNY_FWL_FILENAME	"CsabG8.fwl"
#define		CSSPLENDA_FWL_FILENAME	"Cs16xyy.fwl"
#define     CSBRANIAC_FWL_FILENAME  "CsabG12.fwl"
#define		CSPITCH_FWL_FILENAME	"CscdG8p.fwl"
#define		CSPITCHM_FWL_FILENAME	"CscdG8pm.fwl"
#define		CSFASTBL_FWL_FILENAME	 ".fwl"
#define		CSFASTBL_NM_FWL_FILENAME "nm.fwl"
#define		SNA142_FWL_FILENAME		"CsSNA142.fwl"

#else

#define		CS8XXX_FPGA_IMAGE_FILE_NAME		"cs8xxx"
#define		CS8YYY_FPGA_IMAGE_FILE_NAME		"cs8yyy"
#define		CS8ZZZ_FPGA_IMAGE_FILE_NAME		"cs8zzz"
#define		CSZAP_FPGA_IMAGE_FILE_NAME		"cszap"
#define		CSXYG8_FPGA_IMAGE_FILE_NAME		"csxyg8"
#define		CSSPLENDA_FPGA_IMAGE_FILE_NAME	"cs16xyy"
#define		CSPITCH_FPGA_IMAGE_FILE_NAME	"cscdg8p"
#define		CSPITCHM_FPGA_IMAGE_FILE_NAME	"cscdg8pm"
#define		CSFASTBL4G_FPGA_IMAGE_FILE	    "cscdg84g"
#define		CSFASTBL3G_FPGA_IMAGE_FILE      "cscdg83g"
#define		CSFASTBL_NM_CSI_FILE_NAME       "cscdg8nm"
#define		DEERE_FPGA_IMAGE_FILE_NAME	    "cscdg12"
#define		CS8YYYPCIEX_FPGAIMAGE_FILENAME	"cse8xxx"
#define		CSSPLENDA_PCIEX_FPGAIMAGE_FILENAME	"cse16xyy"
#define		CSOSCAR_PCIEX_FPGAIMAGE_FILENAME	"cse16xyo"
#define		CSZAPPCIEX_FPGAIMAGE_FILENAME	"csezap"
#define		CSXYG8_PCIEX_FPGAIMAGE_FILENAME		"csexyg8"
#define		CSCDG8_PCIEX_FPGAIMAGE_FILENAME		"csecdg8"
#define		CSDECADE12_FPGAIMAGE_FILENAME		"cse12xgy"
#define		CSHEXAGON_FPGAIMAGE_FILENAME		"CsE16bcd"

#define		CS14200_FWL_FILENAME	"cs14200.fwl"
#define		CS14105_FWL_FILENAME	"cs14105.fwl"
#define		CS12400_FWL_FILENAME	"cs12400.fwl"
#define		CS14200x_FWL_FILENAME	"cs14200x.fwl"
#define		CS8XXX_FWL_FILENAME		"cs8xxx.fwl"
#define		CS8YYY_FWL_FILENAME		"cs8yyy.fwl"
#define		CS8ZZZ_FWL_FILENAME		"cs8zzz.fwl"
#define		CSZAP_FWL_FILENAME		"cszap.fwl"
#define		CSXYG8_FWL_FILENAME		"csxyg8.fwl"
#define		CS12400FAT_FWL_FILENAME	"cs12400fat.fwl"
#define		CSBUNNY_FWL_FILENAME	"csabg8.fwl"
#define		CSSPLENDA_FWL_FILENAME	"cs16xyy.fwl"
#define     CSBRANIAC_FWL_FILENAME  "csabg12.fwl"
#define		CSPITCH_FWL_FILENAME	"cscdg8p.fwl"
#define		CSPITCHM_FWL_FILENAME	"cscdg8pm.fwl"
#define		CSFASTBL_FWL_FILENAME	 ".fwl"
#define		CSFASTBL_NM_FWL_FILENAME "nm.fwl"
#define		SNA142_FWL_FILENAME		"cssna142.fwl"

#endif

#define		MAX_DRVPATH				260

// Maccros for conversion of time unit used by most of DDK functions
#define KTIME_NANOSEC(t)	((t)/100)
#define KTIME_MICROSEC(t)   KTIME_NANOSEC((LONGLONG)1000*(t))
#define KTIME_MILISEC(t)	KTIME_MICROSEC((LONGLONG)1000*(t))
#define KTIME_SECOND(t)		KTIME_MILISEC((LONGLONG)1000*(t))
#define KTIME_RELATIVE(t)	(-1*(t))

// Limitation of sofware averaging
#define MAX_SW_AVERAGING_SEMGENTSIZE	512*1024		// Max segment size in Software averaging mode (in samples)
#define	SW_AVERAGING_BUFFER_PADDING		16				// Padding for Sw Avearge buffer
#define MAX_SW_AVERAGE_SEGMENTCOUNT		4096			// Max segment count in SW averaging mode

#define MAX_HW_DEPTH_COUNTER			0x40000000		// Max depth counter registers (30 bits)


// Time out for while loop waiting for Hardware response at Dispatch Level
// This Time out will be used in Gage Driver CsTimer class functions.
// By default this time out is RELATIVE and the value should be short
#define	CS_DP_WAIT_TIMEOUT		KTIME_SECOND ( 2 )
#define	CS_XFER_WAIT_TIMEOUT	KTIME_SECOND ( 4 )

// Time out for while loop waiting for Hardware response
#define	 CS_WAIT_TIMEOUT	KTIME_SECOND ( 4 )
// Time out for while loop waiting for lengthy HW operation (erasing the flash)
#define	 CS_ERASE_WAIT_TIMEOUT	KTIME_RELATIVE( KTIME_SECOND ( 200 ) )

// MAX_CARD_INDEX limited by LONGLONG  (64 bits)
// refer to m_u64FreeIndex in CompuScope Driver Class
#define		MAX_CARD_INDEX		64


// Bits in PCI Config Header Command Register
#define	CS_RESOURCE_PCI_IO				0x1
#define CS_RESOURCE_PCI_MEMORY			0x2
#define CS_RESOURCE_PCI_BUSMASTER		0x4

#define	Abs(a)		((a) > 0 ? (a) : -(a))
#define Max(a,b)  (((a) > (b)) ? (a) : (b))
#define Min(a,b)  (((a) < (b)) ? (a) : (b))

#define CONVERT_EEPROM_OPTIONS(x) (0xFFFFFFFF == (x) ? 0 : x)

// Realtime HW state
#define  CARD_STATE_INIT		0
#define  CARD_STATE_ABORT		1
#define  CARD_STATE_CAPTURE		2
#define  CARD_STATE_TRIGGERED	3
#define	 CARD_STATE_EoTx		4
#define	 CARD_STATE_EoBUSY		5

// Realtime interrupt ID
#define INTR_EVENT_END_OF_TRANSFER	  0
#define INTR_EVENT_START_OF_BUSY	  1
#define INTR_EVENT_START_OF_TRIGGER   2
#define INTR_EVENT_END_OF_BUSY		  3
#define INTR_EVENT_END_OF_TRIGGER	  4
#define INTR_EVENT_TRIGGER_TIMEOUT	  5
#define INTR_EVENT_BUSY_TIMEOUT		  6
#define INTR_EVENT_TRANSFER_TIMEOUT   7

#define NUMBER_OF_INTR_EVENTS		  8


#define timing(x)				Sleep( x/10 );

#ifdef _WINDOWS_
#define		CS_FAIL				CS_FALSE
#else
#define		CS_FAIL				CS_MISC_ERROR
#endif

#define NUMBER_OF_PLDS				4	//	0 = CTRL FPGA, 1 = DATA FPGA, 2 = ADD-ON FPGA, 3 = DMB FPGA.
#define NUMBER_OF_REGISTERS			80	//	64 + 16 extra registers accessed via indexed within a supermap environment or when
										//	more than 16 registers are required within the indexed add-on area (CS12130).
#define NUMBER_OF_MODES				256

#define SIZE_OF_PLD_PROGRAMM		31639L
#define NAME_SIZE					16


// Debug calibration flags
#define		DBG_NORMAL_CALIB			0
#define		DBG_NEVER_CALIB				1
#define		DBG_ALWAYS_CALIB			2


typedef enum _AcquisitionCommand
{
	AcqCmdMemory			= 0,		// Acquisition normal Memory mode (default)
	AcqCmdMemory_Average	= 1,		// Acquisition with eXpert AVG in Memory mode
	AcqCmdMemory_OCT		= 2,		// Acquisition with eXpert OCT in Memory mode
	AcqCmdStreaming			= 100,		// Acquisition with eXpert Streaming
	AcqCmdStreaming_OCT		= 101		// Acquisition with eXpert OCT in Streaming mode

} AcquisitionCommand;


#endif	// CSDRVDEFINE_H__
