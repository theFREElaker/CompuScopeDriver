// Project: ADQ-API
// File: ADQ.cpp
//
// Initial revision: 2008-10-27 by Peter Johansson
// Changes for ADQ108: 2009-01-12 by Robert Kihlberg
// Changes for usb configuration of ADQ: 2009-03-17 by Jonas Persson

#include "AdqLite.h"
#include "AdqApiLite_definitions.h"
#include <stdio.h>



ADQ::ADQ(CCyUSBDevice* cy_usb_device, int device_no)
{
    unsigned int success;

	m_CyUSBDevice = cy_usb_device;
	m_CyUSBDevice->Open(UCHAR(device_no));

	m_EP_COMMAND = (CCyBulkEndPoint*) m_CyUSBDevice->EndPointOf(0x02);  //SEND MESSAGES TO DEVICE ON THIS EP
	m_EP_DATA = (CCyBulkEndPoint*) m_CyUSBDevice->EndPointOf(0x86);  //RECEIVE DATA ON THIS EP
	m_EP_STATUS = (CCyBulkEndPoint*) m_CyUSBDevice->EndPointOf(0x88);  //RECEIVE STATUS ON THIS EP

	m_EP_CONTROL = (CCyControlEndPoint*) m_CyUSBDevice->ControlEndPt;


	m_EP_DATA->SetXferSize(MAX_DMA_TRANSFER);
	m_EP_DATA->TimeOut = TIMEOUT;
	m_EP_STATUS->SetXferSize(STATUS_BUFFER_SIZE);
	m_EP_STATUS->TimeOut = TIMEOUT;
	m_EP_COMMAND->SetXferSize(COMMAND_BUFFER_SIZE);
	m_EP_COMMAND->TimeOut = TIMEOUT;
	
	m_status = 0;
	m_page_count = 0;
	m_trig_point = 0;
	m_overflow = 0;
	m_revision = new int[6];
	m_status_buffer = new unsigned char[STATUS_BUFFER_SIZE];
    m_temp_data_buffer = new unsigned char[MAX_DMA_TRANSFER];
	m_data_buffer = new int[MAX_DMA_TRANSFER];

	long bytes = MAX_DMA_TRANSFER * DMA_NUMBER_OF_BUFFERS;
	m_Queue.pBuf[0] = (unsigned char*)malloc(bytes * sizeof(unsigned char));
	success = (m_Queue.pBuf[0] != NULL);
	for (int i=1; i < DMA_NUMBER_OF_BUFFERS; i++)
	{
		m_Queue.pBuf[i] = m_Queue.pBuf[i-1] + bytes/DMA_NUMBER_OF_BUFFERS;
	}
	success = success && SyncBuffers();

    m_bcdUSB = m_CyUSBDevice->BcdUSB;
	m_bcdDevice = m_CyUSBDevice->BcdDevice;
	m_PID = m_CyUSBDevice->ProductID;
	m_VID = m_CyUSBDevice->VendorID;
}


ADQ::~ADQ()
{
	delete m_CyUSBDevice;
	delete[] m_revision;
	delete[] m_status_buffer;
	delete[] m_data_buffer;
	delete[] m_temp_data_buffer;
}

unsigned int ADQ::GetUSBAddress()
{
	unsigned int addr = m_CyUSBDevice->USBAddress;
	return addr;
}

int* ADQ::GetRevision()
{
	int success;
	unsigned int revision_lowbyte16 = 0;
	unsigned int revision_highbyte16 = 0;
	unsigned char message[4];
	message[0] = 4;
	message[1] = PROCESSOR_CMD;
	message[2] = 0;
	message[3] = SEND_REVISION_LSB;
	ReceiveStatusInit();
	success = SendCommand(message);
	success = ReceiveStatusFinish(&m_status) && success; //Always do function call
	if (success)
		revision_lowbyte16 = m_status;
	message[0] = 4;
	message[1] = PROCESSOR_CMD;
	message[2] = 0;
	message[3] = SEND_REVISION_MSB;
	if (success)
	{
		ReceiveStatusInit();
		success = SendCommand(message);
		success = ReceiveStatusFinish(&m_status) && success; //Always do function call
		if (success)
			revision_highbyte16 = m_status;
	}
	m_revision[0] = (int)((revision_highbyte16 & 0x3FFF)*65536 + revision_lowbyte16);
	m_revision[1] = (int)((revision_highbyte16 & 0x8000));
	m_revision[2] = (int)((revision_highbyte16 & 0x4000));

	revision_lowbyte16 = ReadAlgoRegister(4);
	revision_highbyte16 = ReadAlgoRegister(5);

	m_revision[3] = (int)(revision_lowbyte16);
	m_revision[4] = (int)((revision_highbyte16 & 0x8000));
	m_revision[5] = (int)((revision_highbyte16 & 0x4000));

	return m_revision;
}

int ADQ::SyncBuffers()
{
	int success;
	success = m_EP_DATA->Abort();

	for (int i=0; i < DMA_NUMBER_OF_BUFFERS; i++)
	{
		m_Queue.bufStatus[i] = USB_EMPTY;
		m_Queue.bufBytes[i] = 0;
	}
	m_Queue.pWrite = 0;
	m_Queue.pSync = 0;
	m_Queue.pRead = 0;

	return success;
}

int ADQ::SendCommand(unsigned char* message)
{
    unsigned int curr_msg_length;
    long usb_msg_length;
    usb_msg_length = USB_MSG_LENGTH;
    curr_msg_length = message[0];

    memset(m_message_holder, 0, USB_MSG_LENGTH);
	memcpy(m_message_holder, message, curr_msg_length);
	
    return m_EP_COMMAND->XferData(m_message_holder, usb_msg_length);
}

int ADQ::ReceiveStatusInit()
{
	long buffersize = STATUS_BUFFER_SIZE;
	m_OvLap[1].hEvent = CreateEvent(NULL, false, false, NULL);
	m_OvContext[1] = m_EP_STATUS->BeginDataXfer(m_status_buffer, buffersize, &m_OvLap[1]);
	return 1;
}

int ADQ::ReceiveStatusFinish(unsigned int* data)
{
	int success;
	long buffersize = STATUS_BUFFER_SIZE;
	success = m_EP_STATUS->WaitForXfer(&m_OvLap[1], TIMEOUT);
	if (!success) 
	{ 
		m_EP_DATA->Abort(); 
		//WaitForSingleObject(m_OvLap[1].hEvent, INFINITE); 
	}
	success = m_EP_STATUS->FinishDataXfer(m_status_buffer, buffersize, &m_OvLap[1], m_OvContext[1]) && success;
	*data = ((unsigned int)m_status_buffer[3] << 24) +
			((unsigned int)m_status_buffer[2] << 16) + 
			((unsigned int)m_status_buffer[1] <<  8) + 
			((unsigned int)m_status_buffer[0]);
	if (buffersize != STATUS_BUFFER_SIZE) success = 0;
	CloseHandle(m_OvLap[1].hEvent);
	return success; 
}


int ADQ::EnableSpi()
{
	m_EP_CONTROL->Target = TGT_DEVICE;
	m_EP_CONTROL->ReqType = REQ_VENDOR;
	m_EP_CONTROL->ReqCode = VR_SPI_ENABLE;
	m_EP_CONTROL->Value = 0;
	m_EP_CONTROL->Index = 0;

	long len = 0;
	m_EP_CONTROL->Write(0,len);
	return 1;
}

int ADQ::DisableSpi()
{
	m_EP_CONTROL->Target = TGT_DEVICE;
	m_EP_CONTROL->ReqType = REQ_VENDOR;
	m_EP_CONTROL->ReqCode = VR_SPI_DISABLE;
	m_EP_CONTROL->Value = 0;
	m_EP_CONTROL->Index = 0;

	long len = 0;

	m_EP_CONTROL->Write(0,len);
	return 1;
}

//Requires some uncommenting and recompilation of firmware
int ADQ::FlashRead(unsigned int pagenum, unsigned int buf_address, long len, char *buf)
{
	m_EP_CONTROL->Target = TGT_DEVICE;
	m_EP_CONTROL->ReqType = REQ_VENDOR;
	m_EP_CONTROL->ReqCode = 0x82;
	m_EP_CONTROL->Value = WORD(pagenum);
	m_EP_CONTROL->Index = WORD(buf_address);
	m_EP_CONTROL->Read((PUCHAR)buf,len);
	return 1;
}

//Requires some uncommenting and recompilation of firmware
int ADQ::FlashBufRead(char buf_num, unsigned int buf_address, long len, char *buf)
{
	if (len >64)
		return -1;
	m_EP_CONTROL->Target = TGT_DEVICE;
	m_EP_CONTROL->ReqType = REQ_VENDOR;
	m_EP_CONTROL->ReqCode = 0x7F; //spi buffer read
	m_EP_CONTROL->Value = WORD(buf_num); //Buf 1 addr. 0
	m_EP_CONTROL->Index = WORD(buf_address); //Buffer read

	m_EP_CONTROL->Read((PUCHAR)buf,len);
	return 1;
}

int ADQ::FlashWrite()
{
	/*m_EP_CONTROL->Target = TGT_DEVICE;
	m_EP_CONTROL->ReqType = REQ_VENDOR;
	m_EP_CONTROL->ReqCode = ;
	m_EP_CONTROL->Value = 0x0b00;
	m_EP_CONTROL->Index = 0x0040;

	m_EP_CONTROL->Read((PUCHAR)buf,len);*/
	return 1;
}

int ADQ::FlashBufWrite(/*Statical write to buffer 1 address 0 */)
{
	/*m_EP_CONTROL->Target = TGT_DEVICE;
	m_EP_CONTROL->ReqType = REQ_VENDOR;
	m_EP_CONTROL->ReqCode = ;
	m_EP_CONTROL->Value = 0x0b00;
	m_EP_CONTROL->Index = 0x0040;

	m_EP_CONTROL->Read((PUCHAR)buf,len);*/
	return 1;
}



int ADQ::FlashUploadStart(int fpga_num, int start_page)
{
//Command to initiate upload
	m_EP_CONTROL->Target = TGT_DEVICE;
	m_EP_CONTROL->ReqType = REQ_VENDOR;
	m_EP_CONTROL->ReqCode = VR_FLASH_UPLOAD_START;
	m_EP_CONTROL->Value = WORD(start_page);
	m_EP_CONTROL->Index = WORD(fpga_num);
	long no_bytes =0;
	m_EP_CONTROL->Write(0,no_bytes);
	return 1;
}

int ADQ::FlashUploadPart(long len, char * data)
{
	m_EP_CONTROL->Target = TGT_DEVICE;
	m_EP_CONTROL->ReqType = REQ_VENDOR;
	m_EP_CONTROL->ReqCode = VR_FLASH_UPLOAD_PART;
	m_EP_CONTROL->Value = 0x0000;
	m_EP_CONTROL->Index = 0x0000;
	m_EP_CONTROL->Write((PUCHAR)data,len);
	return 1;
}

int ADQ::FlashUploadEnd()
{
//Command to initiate upload
	m_EP_CONTROL->Target = TGT_DEVICE;
	m_EP_CONTROL->ReqType = REQ_VENDOR;
	m_EP_CONTROL->ReqCode = VR_FLASH_UPLOAD_END;
	m_EP_CONTROL->Value = 0x0000;
	m_EP_CONTROL->Index = 0x0000;
	long no_bytes =0;
	m_EP_CONTROL->Write(0,no_bytes);
	return 1;
}

int ADQ::FlashDownloadStart(int fpga_num, int start_page)
{
//Command to initiate upload
	m_EP_CONTROL->Target = TGT_DEVICE;
	m_EP_CONTROL->ReqType = REQ_VENDOR;
	m_EP_CONTROL->ReqCode = VR_FLASH_DOWNLOAD_START;
	m_EP_CONTROL->Value = WORD(start_page);
	m_EP_CONTROL->Index = WORD(fpga_num);
	long no_bytes =0;
	m_EP_CONTROL->Write(0,no_bytes);
	return 1;
}

int ADQ::FlashDownloadPart(long len, char * data)
{
	m_EP_CONTROL->Target = TGT_DEVICE;
	m_EP_CONTROL->ReqType = REQ_VENDOR;
	m_EP_CONTROL->ReqCode = VR_FLASH_DOWNLOAD_PART;
	m_EP_CONTROL->Value = 0x0000;
	m_EP_CONTROL->Index = 0x0000;
	m_EP_CONTROL->Read((PUCHAR)data,len);
	return 1;
}

int ADQ::FlashDownloadEnd()
{
//Command to initiate upload
	m_EP_CONTROL->Target = TGT_DEVICE;
	m_EP_CONTROL->ReqType = REQ_VENDOR;
	m_EP_CONTROL->ReqCode = VR_FLASH_DOWNLOAD_END;
	m_EP_CONTROL->Value = 0x0000;
	m_EP_CONTROL->Index = 0x0000;
	long no_bytes =0;
	m_EP_CONTROL->Write(0,no_bytes);
	return 1;
}



//Reads status (8 bytes) from FX2 and puts into buf
int ADQ::FlashGetStatus(char *buf)
{
	m_EP_CONTROL->Target = TGT_DEVICE;
	m_EP_CONTROL->ReqType = REQ_VENDOR;
	m_EP_CONTROL->ReqCode = VR_FLASH_STATUS;
	m_EP_CONTROL->Value = 0x0000;
	m_EP_CONTROL->Index = 0x0000;
	long no_bytes = 8;
	m_EP_CONTROL->Read((PUCHAR)buf,no_bytes);
	return 1;
}

int ADQ::FlashBlockErase(int fpga_number, int block)
{
	long no_bytes = 0;
	m_EP_CONTROL->Target = TGT_DEVICE;
	m_EP_CONTROL->ReqType = REQ_VENDOR;
	m_EP_CONTROL->ReqCode = VR_FLASH_ERASE_BLOCK;
	m_EP_CONTROL->Index = WORD(fpga_number);
	m_EP_CONTROL->Value = WORD(block);
	m_EP_CONTROL->Write(0,no_bytes);
	return 1;
}

//Puts 4 bytes of model info into buf
int ADQ::FlashGetModelInfo(int fpga_number, char* buf)
{
	long no_bytes = 4;
	m_EP_CONTROL->Target = TGT_DEVICE;
	m_EP_CONTROL->ReqType = REQ_VENDOR;
	m_EP_CONTROL->ReqCode = VR_FLASH_ID;
	m_EP_CONTROL->Index = WORD(fpga_number);
	m_EP_CONTROL->Value = 0;
	m_EP_CONTROL->Read((PUCHAR)buf,no_bytes);
	return 1;
}

//Puts a 64 byte unique flash id into buf
int ADQ::FlashGetUniqueId(int fpga_number, char* buf)
{
	long no_bytes = 64;
	m_EP_CONTROL->Target = TGT_DEVICE;
	m_EP_CONTROL->ReqType = REQ_VENDOR;
	m_EP_CONTROL->ReqCode = VR_FLASH_UNIQUE_ID;
	m_EP_CONTROL->Index = WORD(fpga_number);
	m_EP_CONTROL->Value = 0;
	m_EP_CONTROL->Read((PUCHAR)buf,no_bytes);
	return 1;
}

unsigned int ADQ::SendProcessorCommand(unsigned int command, unsigned int addr, unsigned int mask, unsigned int data)
{
	int success;
	int temp_word = 0;
	unsigned char message[16];

	message[ 0] = 16;
	message[ 1] = PROCESSOR_CMD;
	message[ 2] = unsigned char((command & 0xFF00) >> 8);
	message[ 3] = unsigned char((command & 0x00FF));
	message[ 4] = unsigned char((addr & 0x000000FF));
	message[ 5] = unsigned char((addr & 0x0000FF00) >>  8);
	message[ 6] = unsigned char((addr & 0x00FF0000) >> 16);
	message[ 7] = unsigned char((addr & 0xFF000000) >> 24);
	message[ 8] = unsigned char((mask & 0x000000FF));
	message[ 9] = unsigned char((mask & 0x0000FF00) >>  8);
	message[10] = unsigned char((mask & 0x00FF0000) >> 16);
	message[11] = unsigned char((mask & 0xFF000000) >> 24);
	message[12] = unsigned char((data & 0x000000FF));
	message[13] = unsigned char((data & 0x0000FF00) >>  8);
	message[14] = unsigned char((data & 0x00FF0000) >> 16);
	message[15] = unsigned char((data & 0xFF000000) >> 24);
	ReceiveStatusInit();
	success = SendCommand(message);
	success = ReceiveStatusFinish(&m_status) && success; //Always do function call
	if (success)
		temp_word = m_status;

	return temp_word;
}

unsigned int ADQ::ReadAlgoRegister(unsigned int addr)
{
	int temp_word = 0;
	if ((addr < (1<<15)))
		temp_word = SendProcessorCommand(READ_ALGO_REG, addr, 0, 0);
	return temp_word;
}

int ADQ::GetVID()
{
	return this->m_VID;
}

int ADQ::GetPID()
{
	return this->m_PID;
}

int ADQ::GetBcdUSB()
{
	return this->m_bcdUSB;
}

int ADQ::GetBcdDevice()
{
	return this->m_bcdDevice;
}
