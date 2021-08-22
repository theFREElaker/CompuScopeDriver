// Project: SphinxAPIEXPRESS
// File: PCIe.h


#ifndef PCIe_H
#define PCIe_H

#include "HWIF.h"
#include "wdc_lib.h"
#include "virtex5_lib.h"
#include "ADQAPI_definitions.h"
#include <map>
#include <utility>
using namespace std;

// PCIe register offsets 
enum {
	/*
    PCI_DCSR		= 0x000,	// Device Control Status Register			R/W
    PCI_DDMACR		= 0x004,	// Device DMA Control Status Register		R/W
    PCI_WDMATLPA	= 0x008,	// Write DMA TLP Address					R/W
    PCI_WDMATLPS	= 0x00c,	// Write DMA TLP Size						R/W
    PCI_WDMATLPC	= 0x010,	// Write DMA TLP Count						R/W
    PCI_WDMAPERF	= 0x028,	// Write DMA Performance					RO
    PCI_DLWSTAT		= 0x03c,	// Device Link Width Status					RO
    PCI_DLTRSSTAT	= 0x040,	// Device Link Transaction Size Status		RO
	*/

	PCI_CMD_MSG		= 0x80,	// Command Message							R/W
	PCI_CMD_ARG1	= 0x84,	// CPU Command Argument 1					R/W
	PCI_CMD_ARG2	= 0x88,	// CPU Command Argument 2					R/W
	PCI_CMD_ARG3	= 0x8c,	// CPU Command Argument 3					R/W
	PCI_DRY_BUSY	= 0x90,	// CPU Data ready / CPU busy				R/W
	PCI_CPU_OUT		= 0x94	// CPU Return register						RO
};



enum PCIE_BUFFERSTATUS { PCIE_EMPTY, PCIE_WRITE, PCIE_FULL, PCIE_USER};


typedef struct {
    VIRTEX5_DMA_HANDLE hDma;
    PVOID *pBuf;
	volatile PCIE_BUFFERSTATUS *bufStatus;
	volatile unsigned int *bufBytes;
	volatile DWORD pWrite;
	volatile DWORD pSync;
	volatile DWORD pRead;
	unsigned int nOfBuffers;
	unsigned int maxBufferSize;
} DMA, *PDMA;

class PCIe : public HWIF
    {
    public:
		PCIe();  
		~PCIe();
		int		Open(WDC_DEVICE_HANDLE hDev, unsigned long  pcie_device_location);
		int     SetTransferBuffers(unsigned int nOfBuffers, unsigned int bufferSize);
		unsigned int GetMaxTransferSize();
		int		IsPCIeDevice();
		int		SendCommand(unsigned char* message);
		int		ReceiveStatusFinish(unsigned int* data);
		int		ReceiveDataInit(int transfer_size_bytes, unsigned int maxPrefetch);
		int		ReceiveDataFinish(int transfer_size_bytes, unsigned char **data_buffer_ptr);
		int		ReceiveDataAbort();
        unsigned int    ResetDevice(int);
		unsigned int	Address();
		unsigned int	HwVersion();
		static void		DmaIntHandler(WDC_DEVICE_HANDLE hDev, VIRTEX5_INT_RESULT *pIntResult);

    private:
		int		DMADev2HostStart(unsigned int &transfer_size_bytes, unsigned int &maxPrefetch);
		int		DMADev2HostFinish(unsigned char** data_buffer, unsigned int &returnedBytes);
		BOOL	DMASyncBuffers();
		DWORD	WriteRegister(DWORD write_reg, unsigned int data);
		DWORD	ReadRegister(DWORD read_reg, unsigned int* data);
		void	DMAClose(WDC_DEVICE_HANDLE hDev, PDMA pDma);

		WDC_DEVICE_HANDLE	m_Device;
		DMA					m_Dma;
		unsigned long		m_pcie_device_location;
		DWORD				m_max_wSize;
		DWORD				m_wSize;
		DWORD				m_wCount;
		BOOL				m_fPolling;
		HANDLE				m_WaitForInterrupt;

		static map<WDC_DEVICE_HANDLE, PCIe*>	m_DeviceInstances;
	
	};
	typedef map<WDC_DEVICE_HANDLE, PCIe*> mapType;
	typedef pair<WDC_DEVICE_HANDLE, PCIe*> pairType;

#endif //PCIe_H
