// Project: SphinxAPIEXPRESS
// File: PCIe.cpp


#include "USB.h"
#include "ADQAPI_definitions.h"

// {7042A079-7F42-433c-883B-5A0412DBC1F6}
const GUID USB::m_ADQ_USB_GUID = { 0x7042a079, 0x7f42, 0x433c, { 0x88, 0x3b, 0x5a, 0x4, 0x12, 0xdb, 0xc1, 0xf6 } };

USB::USB()
{
	m_CyUSBDevice = NULL;
	m_status_buffer = new unsigned char[STATUS_BUFFER_SIZE];
	m_Queue.OvLap = NULL;
	m_Queue.OvContext = NULL;
	m_Queue.pBuf = NULL;
	m_Queue.bufStatus = NULL;
	m_Queue.bufBytes = NULL;
}

int USB::Open(int device_no)
{
	int success;
	m_CyUSBDevice = new CCyUSBDevice(NULL);//m_ADQ_USB_GUID
    success = m_CyUSBDevice->Open(char(device_no));
	success = success && m_CyUSBDevice->Reset();

	if (success)
	{
    	m_EP_CONTROL = (CCyControlEndPoint*) m_CyUSBDevice->ControlEndPt; // This is the control EndPt for USB firmware
		m_EP_COMMAND = (CCyBulkEndPoint*) m_CyUSBDevice->EndPointOf(0x02);  //SEND MESSAGES TO DEVICE ON THIS EP
		m_EP_DATA = (CCyBulkEndPoint*) m_CyUSBDevice->EndPointOf(0x86);  //RECEIVE DATA ON THIS EP
		m_EP_STATUS = (CCyBulkEndPoint*) m_CyUSBDevice->EndPointOf(0x88);  //RECEIVE STATUS ON THIS EP
		m_EP_DATA->TimeOut = TIMEOUT;
		m_EP_STATUS->SetXferSize(STATUS_BUFFER_SIZE);
		m_EP_STATUS->TimeOut = TIMEOUT;
		m_EP_COMMAND->SetXferSize(COMMAND_BUFFER_SIZE);
		m_EP_COMMAND->TimeOut = TIMEOUT;

		success = success && SetTransferBuffers(DMA_DEFAULT_NUMBER_OF_BUFFERS, DMA_DEFAULT_BUFFER_SIZE);
	}
	return success;
}

USB::~USB()
{
	ReceiveDataAbort();

	if (m_CyUSBDevice != NULL)
		delete m_CyUSBDevice;

	if (m_status_buffer != NULL)
		delete[] m_status_buffer;

	if (m_Queue.pBuf != NULL)
	{
		if (m_Queue.pBuf[0] != NULL)
			delete[] m_Queue.pBuf[0];
		delete[] m_Queue.pBuf;
	}

	if (m_Queue.OvLap != NULL)
		delete[] m_Queue.OvLap;

	if (m_Queue.OvContext != NULL)
		delete[] m_Queue.OvContext;

	if (m_Queue.bufStatus != NULL)
		delete[] m_Queue.bufStatus;

	if (m_Queue.bufBytes != NULL)
		delete[] m_Queue.bufBytes;
}

int USB::SetTransferBuffers(unsigned int nOfBuffers, unsigned int bufferSize)
{
	long bytes;
	bool pBufEmpty = (m_Queue.pBuf == NULL);

	m_Queue.nOfBuffers = nOfBuffers;
	m_Queue.maxBufferSize = bufferSize;

	bytes = bufferSize * nOfBuffers;

	m_EP_DATA->SetXferSize(bufferSize);

	m_Queue.OvLap = (OVERLAPPED*)realloc((void*)m_Queue.OvLap, nOfBuffers * sizeof(OVERLAPPED));
	if (m_Queue.OvLap == NULL) return 0;
	m_Queue.OvContext = (UCHAR**)realloc((void*)m_Queue.OvContext, nOfBuffers * sizeof(UCHAR*));
	if (m_Queue.OvContext == NULL) return 0;
	m_Queue.pBuf = (unsigned char**)realloc((void*)m_Queue.pBuf, nOfBuffers * sizeof(unsigned char*));
	if (m_Queue.pBuf == NULL) return 0;
	m_Queue.bufStatus = (USB_BUFFERSTATUS*)realloc((void*)m_Queue.bufStatus, nOfBuffers * sizeof(USB_BUFFERSTATUS));
	if (m_Queue.bufStatus == NULL) return 0;
	m_Queue.bufBytes = (unsigned int *)realloc((void*)m_Queue.bufBytes, nOfBuffers * sizeof(unsigned int));
	if (m_Queue.bufBytes == NULL) return 0;

	if (pBufEmpty)
		m_Queue.pBuf[0] = (unsigned char*)malloc(bytes * sizeof(unsigned char));
	else
		m_Queue.pBuf[0] = (unsigned char*)realloc((void*)m_Queue.pBuf[0], bytes * sizeof(unsigned char));

	if (m_Queue.pBuf[0] == NULL) return 0;

	for (unsigned int i=1; i < nOfBuffers; i++)
	{
		m_Queue.pBuf[i] = m_Queue.pBuf[i-1] + bytes/nOfBuffers;
	}

	return SyncBuffers();
}

unsigned int USB::GetMaxTransferSize()
{
	return m_Queue.maxBufferSize;
}

int USB::SendCommand(unsigned char* message)
{
    unsigned int curr_msg_length;
    long usb_msg_length;
    usb_msg_length = USB_MSG_LENGTH;
    curr_msg_length = message[0];

    memset(m_message_holder, 0, USB_MSG_LENGTH);
	memcpy(m_message_holder, message, curr_msg_length);
	
    return m_EP_COMMAND->XferData(m_message_holder, usb_msg_length);
}

int USB::IsUSBDevice()
{
	return 1;
}

int USB::SyncBuffers()
{
	int success;
	success = m_EP_DATA->Abort();

	for (unsigned int i=0; i < m_Queue.nOfBuffers; i++)
	{
		m_Queue.bufStatus[i] = USB_EMPTY;
		m_Queue.bufBytes[i] = 0;
	}
	m_Queue.pWrite = 0;
	m_Queue.pSync = 0;
	m_Queue.pRead = 0;

	return success;
}

int USB::Dev2HostStart(unsigned int &transfer_size_bytes, unsigned int &maxPrefetch)
{
	if (maxPrefetch == 0)
		return 1;

	if (m_Queue.bufStatus[m_Queue.pRead] == USB_USER)
	{
		m_Queue.bufStatus[m_Queue.pRead] = USB_EMPTY;
		m_Queue.bufBytes[m_Queue.pRead] = 0;
		m_Queue.pRead = (m_Queue.pRead + 1) % m_Queue.nOfBuffers;
	}

	if (m_Queue.bufStatus[m_Queue.pWrite] != USB_EMPTY)
		return 1;

    /* Start DMA */
	while ((m_Queue.bufStatus[m_Queue.pWrite] == USB_EMPTY) && maxPrefetch)
	{
		maxPrefetch--;
		m_Queue.bufStatus[m_Queue.pWrite] = USB_WRITE;
		m_Queue.OvLap[m_Queue.pWrite].hEvent = CreateEvent(NULL, false, false, NULL);
		m_Queue.OvContext[m_Queue.pWrite] = m_EP_DATA->BeginDataXfer(m_Queue.pBuf[m_Queue.pWrite], transfer_size_bytes, &m_Queue.OvLap[m_Queue.pWrite]);
		m_Queue.bufBytes[m_Queue.pWrite] = transfer_size_bytes;
		m_Queue.pWrite = (m_Queue.pWrite + 1) % m_Queue.nOfBuffers;
	}
	return 1;
}

int USB::Dev2HostFinish(unsigned char** data_buffer, unsigned int &returnedBytes)
{
	returnedBytes = 0;

	if (m_Queue.bufStatus[m_Queue.pRead] == USB_USER)
	{
		m_Queue.bufStatus[m_Queue.pRead] = USB_EMPTY;
		m_Queue.bufBytes[m_Queue.pRead] = 0;
		m_Queue.pRead = (m_Queue.pRead + 1) % m_Queue.nOfBuffers;
	}

	if (m_Queue.bufStatus[m_Queue.pRead] == USB_WRITE)
	{
		int success = 0;
		long tfsize_bytes = m_Queue.bufBytes[m_Queue.pRead];
		success = m_EP_DATA->WaitForXfer(&m_Queue.OvLap[m_Queue.pRead], TIMEOUT);
		if (!success) 
		{ 
			m_EP_DATA->Abort(); 
			//success = WaitForSingleObject(m_Queue.OvLap[m_Queue.pRead].hEvent, INFINITE); 
		}
		success = m_EP_DATA->FinishDataXfer(m_Queue.pBuf[m_Queue.pRead], tfsize_bytes, &m_Queue.OvLap[m_Queue.pRead], m_Queue.OvContext[m_Queue.pRead]) && success;
		if (unsigned int(tfsize_bytes) != m_Queue.bufBytes[m_Queue.pRead]) success = 0;
		CloseHandle(m_Queue.OvLap[m_Queue.pRead].hEvent);
		m_Queue.bufBytes[m_Queue.pRead] = tfsize_bytes;

		m_Queue.bufStatus[m_Queue.pRead] = USB_USER;
		*data_buffer = m_Queue.pBuf[m_Queue.pRead];
		returnedBytes = m_Queue.bufBytes[m_Queue.pRead];
		return success;
	}

    return 0;
}

int USB::ReceiveDataAbort()
{
	BOOL success = TRUE;

	if (m_Queue.bufStatus[m_Queue.pRead] == USB_USER)
	{
		m_Queue.bufStatus[m_Queue.pRead] = USB_EMPTY;
		m_Queue.bufBytes[m_Queue.pRead] = 0;
		m_Queue.pRead = (m_Queue.pRead + 1) % m_Queue.nOfBuffers;
	}

	while (m_Queue.bufStatus[m_Queue.pRead] != USB_EMPTY)
	{
		if (m_Queue.bufStatus[m_Queue.pRead] == USB_WRITE)
		{
			long tfsize_bytes = m_Queue.bufBytes[m_Queue.pRead];
			success = m_EP_DATA->WaitForXfer(&m_Queue.OvLap[m_Queue.pRead], TIMEOUT);
			if (!success) 
			{ 
				m_EP_DATA->Abort(); 
				//success = WaitForSingleObject(m_Queue.OvLap[m_Queue.pRead].hEvent, INFINITE); 
			}
			success = m_EP_STATUS->FinishDataXfer(m_Queue.pBuf[m_Queue.pRead], tfsize_bytes, &m_Queue.OvLap[m_Queue.pRead], m_Queue.OvContext[m_Queue.pRead]) && success;
			CloseHandle(m_Queue.OvLap[m_Queue.pRead].hEvent);
		}

		m_Queue.bufStatus[m_Queue.pRead] = USB_EMPTY;
		m_Queue.bufBytes[m_Queue.pRead] = 0;
		m_Queue.pRead = (m_Queue.pRead + 1) % m_Queue.nOfBuffers;
	}

	return success;
}

int USB::ReceiveStatusInit()
{
	long buffersize = STATUS_BUFFER_SIZE;
	m_OvLap[1].hEvent = CreateEvent(NULL, false, false, NULL);
	m_OvContext[1] = m_EP_STATUS->BeginDataXfer(m_status_buffer, buffersize, &m_OvLap[1]);
	return 1;
}

int USB::ReceiveStatusFinish(unsigned int* data)
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

int USB::ReceiveDataInit(int transfer_size_bytes, unsigned int maxPrefetch)
{
	int success;
	unsigned int tfsize_bytes = transfer_size_bytes;
	success = Dev2HostStart(tfsize_bytes, maxPrefetch);
	return success && (tfsize_bytes == unsigned int(transfer_size_bytes));
}

int USB::ReceiveDataFinish(int transfer_size_bytes, unsigned char **data_buffer_ptr)
{
	int success;
	unsigned int returnedBytes;
	success = Dev2HostFinish(data_buffer_ptr, returnedBytes);
	return success && (returnedBytes == unsigned int(transfer_size_bytes));
}

/* Old single-buffer solution
int USB::ReceiveDataInit(int transfer_size_bytes, unsigned int maxPrefetch)
{
	m_OvLap[0].hEvent = CreateEvent(NULL, false, false, NULL);
	m_OvContext[0] = m_EP_DATA->BeginDataXfer(m_temp_data_buffer, transfer_size_bytes, &m_OvLap[0]);
	return 1;
}

int USB::ReceiveDataFinish(int transfer_size_bytes, unsigned char **data_buffer_ptr)
{
	int success;
	long tfsize_bytes = transfer_size_bytes;
	success = m_EP_DATA->WaitForXfer(&m_OvLap[0], TIMEOUT);
	if (!success) 
	{ 
		m_EP_DATA->Abort(); 
		//WaitForSingleObject(m_OvLap[0].hEvent, INFINITE); 
	}
	success = m_EP_DATA->FinishDataXfer(m_temp_data_buffer, tfsize_bytes, &m_OvLap[0], m_OvContext[0]) && success;
	if (tfsize_bytes != transfer_size_bytes) success = 0;
	CloseHandle(m_OvLap[0].hEvent);
	*data_buffer_ptr = m_temp_data_buffer;
	return success;
}
*/

int USB::FlushBuffers()
{
	long length;

	// Loop until returned amount data is zero
	do
	{
		length = m_Queue.maxBufferSize;
		m_EP_DATA->XferData(m_Queue.pBuf[0], length);
	} while ((unsigned int)length == m_Queue.maxBufferSize);

	return 1;
}

unsigned int USB::Address()
{
	return (unsigned int)m_CyUSBDevice->USBAddress;
}

unsigned int USB::HwVersion()
{
	return m_CyUSBDevice->BcdDevice;
}

unsigned int USB::ResetDevice(int resetlevel)
{
    unsigned int success = 0;

    if (resetlevel == 16)
    {
        m_EP_CONTROL->Target = TGT_DEVICE;
        m_EP_CONTROL->ReqType = REQ_VENDOR;
        m_EP_CONTROL->ReqCode = VR_SPI_ENABLE;
        m_EP_CONTROL->Value = 0;
        m_EP_CONTROL->Index = 0;

        long len = 0;
        m_EP_CONTROL->Write(0,len);
        Sleep(100);

        m_EP_CONTROL->Target = TGT_DEVICE;
        m_EP_CONTROL->ReqType = REQ_VENDOR;
        m_EP_CONTROL->ReqCode = VR_SPI_DISABLE;
        m_EP_CONTROL->Value = 0;
        m_EP_CONTROL->Index = 0;

        len = 0;
        m_EP_CONTROL->Write(0,len);
        success = 1;
    }
    else if (resetlevel == 8)
    {
        ReceiveDataAbort();
        SyncBuffers();
        success = 1;
    }
    return success;
}