// Project: SphinxAPIEXPRESS
// File: PCIe.h


#ifndef USB_H
#define USB_H

#include "windows.h"

#include "HWIF.h"
#include "ADQAPI_definitions.h"
#include "CyAPI.h"

enum USB_BUFFERSTATUS {USB_EMPTY, USB_WRITE, USB_USER};

#define USB_MSG_LENGTH  1024

//USB vendor commands
#define VR_FLASH_UPLOAD_START	0x89
#define VR_FLASH_UPLOAD_PART	0x8A
#define VR_FLASH_UPLOAD_END		0x8B
#define VR_FLASH_STATUS			0x8C
#define VR_FLASH_DOWNLOAD_START	0x8D
#define VR_FLASH_DOWNLOAD_PART	0x8E
#define VR_FLASH_DOWNLOAD_END	0x8F
#define VR_SPI_ENABLE			0x87
#define VR_SPI_DISABLE			0x88
#define VR_FLASH_ID				0x97
#define VR_FLASH_UNIQUE_ID		0x98
#define VR_FLASH_ERASE_BLOCK	0x99


typedef struct {
	OVERLAPPED *OvLap;
	UCHAR **OvContext;
    unsigned char **pBuf;
	volatile USB_BUFFERSTATUS *bufStatus;
	volatile unsigned int *bufBytes;
	volatile DWORD pWrite;
	volatile DWORD pSync;
	volatile DWORD pRead;
	unsigned int nOfBuffers;
	unsigned int maxBufferSize;
} t_queue;



class USB : public HWIF
    {
    public:
		USB();  
		~USB();
		int		Open(int device_no);
		int     SetTransferBuffers(unsigned int nOfBuffers, unsigned int bufferSize);
		unsigned int GetMaxTransferSize();
		int		IsUSBDevice();
		int		SendCommand(unsigned char* message);
		int		ReceiveStatusInit();
		int		ReceiveStatusFinish(unsigned int* data);
		int		ReceiveDataInit(int transfer_size_bytes, unsigned int maxPrefetch);
		int		ReceiveDataFinish(int transfer_size_byte, unsigned char **data_buffer_ptr);
		int		ReceiveDataAbort();
		int		FlushBuffers();
        unsigned int ResetDevice(int);
		unsigned int Address();
		unsigned int HwVersion();

		// {7042A079-7F42-433c-883B-5A0412DBC1F6}
		static const GUID m_ADQ_USB_GUID;

    private:
		int		Dev2HostStart(unsigned int &transfer_size_bytes, unsigned int &maxPrefetch);
		int		Dev2HostFinish(unsigned char** data_buffer, unsigned int &returnedBytes);
		int		SyncBuffers();

		CCyUSBDevice* m_CyUSBDevice;
		OVERLAPPED m_OvLap[2];
		UCHAR *m_OvContext[2];
        CCyControlEndPoint* m_EP_CONTROL;
		CCyBulkEndPoint* m_EP_COMMAND;
		CCyBulkEndPoint* m_EP_DATA;
		CCyBulkEndPoint* m_EP_STATUS;
		t_queue m_Queue;
		unsigned int m_BcdDevice;
		unsigned char* m_status_buffer;
		unsigned char* m_temp_data_buffer;
        unsigned char m_message_holder[1024];
	
	};
#endif //USB_H
