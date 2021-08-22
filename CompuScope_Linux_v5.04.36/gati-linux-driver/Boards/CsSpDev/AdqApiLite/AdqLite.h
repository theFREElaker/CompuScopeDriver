// Project: ADQ-API_Lite
// File: ADQ.h
//
// Initial revision: 2008-10-27 by Peter Johansson



#ifndef ADQ_H
#define ADQ_H

#include "windows.h"
#include "CyAPI.h"

//Global error codes and other definitions
#define PACKET_LENGTH 		64
#define MCS_PARSE_FAIL 		1
#define MCS_MISMATCH 		2
#define MCS_SUCCESS 		0

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

enum USB_BUFFERSTATUS {USB_EMPTY, USB_WRITE, USB_USER};
#define USB_MSG_LENGTH  1024
#define DMA_NUMBER_OF_BUFFERS	(8)
#define MAX_DMA_TRANSFER        (128*1024)  //64 kSampel (128 kbyte)
#define STATUS_BUFFER_SIZE              4
#define COMMAND_BUFFER_SIZE          1024

typedef struct {
	OVERLAPPED OvLap[DMA_NUMBER_OF_BUFFERS];
	UCHAR *OvContext[DMA_NUMBER_OF_BUFFERS];
    unsigned char* pBuf[DMA_NUMBER_OF_BUFFERS];
	volatile USB_BUFFERSTATUS bufStatus[DMA_NUMBER_OF_BUFFERS];
	volatile unsigned int bufBytes[DMA_NUMBER_OF_BUFFERS];
	volatile DWORD pWrite;
	volatile DWORD pSync;
	volatile DWORD pRead;
} t_queue;

class ADQ
    {
    public:
		ADQ(CCyUSBDevice* cy_usb_device, int device_no);  
		~ADQ();
		unsigned int GetUSBAddress();
		int* GetRevision();
        unsigned int ADQ::SendProcessorCommand(unsigned int command, unsigned int addr, unsigned int mask, unsigned int data);
        unsigned int ReadAlgoRegister(unsigned int);
		int ADQ::EnableSpi();
		int ADQ::DisableSpi();
		int ADQ::FlashRead(unsigned int pagenum, unsigned int buf_address, long len, char *buf);
		int ADQ::FlashBufRead(char buf_num, unsigned int buf_address, long len, char *buf);
		int ADQ::FlashWrite();
		int ADQ::FlashBufWrite();
		int ADQ::FlashUploadStart(int fpga_num, int start_page);
		int ADQ::FlashUploadPart(long len, char * data);
		int ADQ::FlashUploadEnd();
		int ADQ::FlashDownloadStart(int fpga_num, int start_page);
		int ADQ::FlashDownloadPart(long len, char * data);
		int ADQ::FlashDownloadEnd();
		int ADQ::FlashGetStatus(char *buf);
		int ADQ::FlashBlockErase(int fpga_number, int block);
		int ADQ::FlashGetModelInfo(int fpga_number, char* buf);
		int ADQ::FlashGetUniqueId(int fpga_number, char* buf);
		int ADQ::GetVID();
		int ADQ::GetPID();
		int ADQ::GetBcdUSB();
		int ADQ::GetBcdDevice();
 
    private:
        int SyncBuffers();
        int SendCommand(unsigned char* message);
		int SendMessage(unsigned char* message);
		int ReceiveFromDataEP();
        int ReceiveStatusFinish(unsigned int* data);
        int ReceiveStatusInit();
		
		CCyUSBDevice* m_CyUSBDevice;
		unsigned char* m_status_buffer;
		unsigned char* m_temp_data_buffer;
		int* m_data_buffer;
		int* m_revision;
		unsigned int m_status;
		unsigned long m_no_bytes_last_received;
		CCyBulkEndPoint* m_EP_COMMAND;
		CCyBulkEndPoint* m_EP_DATA;
		CCyBulkEndPoint* m_EP_STATUS;

		CCyControlEndPoint* m_EP_CONTROL;
        OVERLAPPED m_OvLap[2];
		UCHAR *m_OvContext[2];
		t_queue m_Queue;
        unsigned char m_message_holder[1024];

		int m_trig_lvl;
		int m_trig_flank;
		int m_clock_source;
		int m_freq_divider;
		int m_trig_mode;
		int m_trig_point;
		int m_overflow;
		unsigned int m_buffersize_pages;
		unsigned int m_page_count;
		
		//FpgaTableRow fpga_table[3];
		
		int m_bcdUSB;
		int m_bcdDevice;
		int m_PID;
		int m_VID;
	};
#endif //ADQ_H
