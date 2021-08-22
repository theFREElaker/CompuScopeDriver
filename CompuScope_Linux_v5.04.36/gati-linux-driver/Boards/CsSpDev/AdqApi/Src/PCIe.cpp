// Project: SphinxAPIEXPRESS
// File: PCIe.cpp
#ifndef USB_ONLY

#include "PCIe.h"
#include "ADQAPI_definitions.h"
#include <time.h>

map<WDC_DEVICE_HANDLE, PCIe*>	PCIe::m_DeviceInstances; // Define static variable

PCIe::PCIe()
{
	m_Dma.hDma = NULL;
	m_Dma.pBuf = NULL;
	m_Dma.bufStatus = NULL;
	m_Dma.bufBytes = NULL;
    BZERO(m_Dma.hDma);
}

int PCIe::Open(WDC_DEVICE_HANDLE hDev, unsigned long  pcie_device_location)
{
	DWORD dwStatus;
	BOOL fIsRead = FALSE;

	m_Device = hDev;
	m_pcie_device_location = pcie_device_location;
	m_fPolling = FALSE;

	/* Get the max payload size from the device */
    m_max_wSize = VIRTEX5_DMAGetMaxPacketSize(m_Device, fIsRead);
	m_wSize = m_max_wSize;

	if (m_wSize == 0)
	{
		return 0;
	}

	if (!SetTransferBuffers(DMA_DEFAULT_NUMBER_OF_BUFFERS, DMA_DEFAULT_BUFFER_SIZE))
		return 0;

	//m_WaitForInterrupt = CreateSemaphore(NULL,1,1,NULL);
	m_WaitForInterrupt = CreateEvent(NULL,TRUE,FALSE,NULL);

	m_DeviceInstances.insert(pairType(m_Device,this));
	return 1;
}


PCIe::~PCIe()
{
    VIRTEX5_DmaIntDisable(m_Device, TRUE);
    VIRTEX5_DmaIntDisable(m_Device, FALSE);

    if (m_Device == NULL)
        return;

	mapType::iterator iter = m_DeviceInstances.find(m_Device);
	if( iter != m_DeviceInstances.end() ) 
		m_DeviceInstances.erase(iter);

    DMAClose(m_Device, &m_Dma);      
     
    VIRTEX5_DeviceClose(m_Device);

	if (m_Dma.pBuf != NULL)
		delete[] m_Dma.pBuf;

	if (m_Dma.bufStatus != NULL)
		delete[] m_Dma.bufStatus;

	if (m_Dma.bufBytes != NULL)
		delete[] m_Dma.bufBytes;

	m_Device = NULL;
}

int PCIe::SetTransferBuffers(unsigned int nOfBuffers, unsigned int bufferSize)
{
	DWORD dwStatus;
	DWORD dwTotalCount;

	m_Dma.nOfBuffers = nOfBuffers;
	m_Dma.maxBufferSize = bufferSize;

	if (!m_fPolling) /*  Enable DMA interrupts (if not polling) */
    {
		if (VIRTEX5_IntIsEnabled(m_Device))
        {
            dwStatus = VIRTEX5_IntDisable(m_Device);

            if (WD_STATUS_SUCCESS != dwStatus)
            {
				return 0;
            }

			VIRTEX5_DmaIntDisable(m_Device, FALSE);/*write*/
        }
    }

	if (m_Dma.hDma != NULL)
		DMAClose(m_Device, &m_Dma);

	// calculate wCount from wSize and batch_size
	m_wCount = (bufferSize+m_wSize-1) / m_wSize;

	dwTotalCount = (DWORD)(m_wCount * m_wSize * nOfBuffers);

	m_Dma.pBuf = (PVOID*)realloc((void*)m_Dma.pBuf, nOfBuffers * sizeof(PVOID*));
	if (m_Dma.pBuf == NULL) return 0;
	m_Dma.bufStatus = (PCIE_BUFFERSTATUS*)realloc((void*)m_Dma.bufStatus, nOfBuffers * sizeof(PCIE_BUFFERSTATUS));
	if (m_Dma.bufStatus == NULL) return 0;
	m_Dma.bufBytes = (unsigned int *)realloc((void*)m_Dma.bufBytes, nOfBuffers * sizeof(unsigned int));
	if (m_Dma.bufBytes == NULL) return 0;

	//The BMD reference design does not support s/g DMA, so we use contiguous memory
	DWORD dwOptions = DMA_FROM_DEVICE | DMA_KERNEL_BUFFER_ALLOC; // | DMA_ALLOW_CACHE; <- will maybe make quality of transfer uncertain, but faster.

    /* Open DMA handle */
    dwStatus = VIRTEX5_DMAOpen(m_Device, &m_Dma.pBuf[0], dwOptions,
        dwTotalCount, &m_Dma.hDma);

    if (WD_STATUS_SUCCESS != dwStatus)
    {
		return 0;
    }
	
	//debug memset((void*)m_Dma.pBuf[0], 0, dwTotalCount);

	for (unsigned int i=1; i < nOfBuffers; i++)
	{
		m_Dma.pBuf[i] = (CHAR*)m_Dma.pBuf[i-1] + (DWORD)(dwTotalCount/nOfBuffers);
	}
	DMASyncBuffers();

	/* Prepare the device registers for DMA transfer */
    VIRTEX5_DMADevicePrepare(m_Dma.hDma, FALSE/*write*/, (WORD)(m_wSize/sizeof(UINT32)),
        FALSE/*32bit*/, 0/*tc*/);

	if (!m_fPolling) /*  Enable DMA interrupts (if not polling) */
    {	
		VIRTEX5_DmaIntEnable(m_Device, FALSE);/*write*/

        if (!VIRTEX5_IntIsEnabled(m_Device))
        {
            dwStatus = VIRTEX5_IntEnable(m_Device, &PCIe::DmaIntHandler);

            if (WD_STATUS_SUCCESS != dwStatus)
            {
				return 0;
            }
        }
    }

	return 1;
}

unsigned int PCIe::GetMaxTransferSize()
{
	return m_Dma.maxBufferSize;
}

int PCIe::IsPCIeDevice()
{
	return 1;
}

int PCIe::SendCommand(unsigned char* message)
{
	int i;
	unsigned char msg_size = message[0];
	DWORD write_addr_base = PCI_CMD_MSG;
	DWORD write_addr;
	UINT32 data = 0;
	
	if (msg_size > 16) // Not supported
	{
		return 0;
	}
	
	unsigned int loop = 0;
	time_t Tstart, Tloop;
	double diff = 0;
	UINT32 busy = 0;
	DWORD status;
	
	time(&Tstart);
	status = ReadRegister(PCI_DRY_BUSY, &busy);
	while ((busy & 0x1) && (diff < 2.0f))//(loop<timeout)) // 1 - 2 s timeout
	{
		time(&Tloop);	
		diff = difftime(Tloop,Tstart); //[s]
		loop++;
		if (loop == 50)
		{
			loop = 0;
			status |= ReadRegister(PCI_DRY_BUSY, &busy);
		}
	}
	if (diff >= 2.0f)//(loop>=timeout)
	{
		
		return 0;
	}


	for (i=0; i<msg_size; i+=4)
	{
		data  = message[3+i] << 24;
		data |= message[2+i] << 16;
		data |= message[1+i] << 8;
		data |= message[0+i];
		write_addr = write_addr_base + i;
		status |= WriteRegister(write_addr, data);
	}

	status |= WriteRegister(PCI_DRY_BUSY, 0x1);

	return (status == 0);  //0 is success here
}

BOOL PCIe::DMASyncBuffers()
{
	int success = (int)VIRTEX5_DmaFlush(m_Device);

	for (unsigned int i=0; i < m_Dma.nOfBuffers; i++)
	{
		m_Dma.bufStatus[i] = PCIE_EMPTY;
		m_Dma.bufBytes[i] = 0;
	}
	m_Dma.pWrite = 0;
	m_Dma.pSync = 0;
	m_Dma.pRead = 0;

	return success;
}

int PCIe::DMADev2HostStart(unsigned int &transfer_size_bytes, unsigned int &maxPrefetch)
{
	if (maxPrefetch == 0)
		return 1;

	if (m_Dma.bufStatus[m_Dma.pRead] == PCIE_USER)
	{
		m_Dma.bufStatus[m_Dma.pRead] = PCIE_EMPTY;
		m_Dma.bufBytes[m_Dma.pRead] = 0;
		m_Dma.pRead = (m_Dma.pRead + 1) % m_Dma.nOfBuffers;
	}

	if (m_Dma.bufStatus[m_Dma.pWrite] != PCIE_EMPTY)
		return 1;

	if (m_wSize <= 0)
		return 0;

	m_wCount = (transfer_size_bytes+m_wSize-1)/m_wSize; // round up
	if (m_wCount > 65536)
	{
		transfer_size_bytes = 0;
		return 0;
	}
	transfer_size_bytes = m_wCount*m_wSize;

#if 0 //not using semaphore anymore
	DWORD dwStatus;
   
    if (!m_fPolling) /*  Enable DMA interrupts (if not polling) */
    {
		/* Check & init sync */
		dwStatus = WaitForSingleObject(m_WaitForInterrupt, 0);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
			return 0;
        }
    }
#endif

    /* Start DMA */
	while ((m_Dma.bufStatus[m_Dma.pWrite] == PCIE_EMPTY) && maxPrefetch && VIRTEX5_DMAIsReady(m_Dma.hDma))
	{
		maxPrefetch--;
		m_Dma.bufStatus[m_Dma.pWrite] = PCIE_WRITE;
		VIRTEX5_DMAStart(m_Dma.hDma, FALSE/*write*/, (WORD)m_wCount, (DWORD)((unsigned long long)m_Dma.pBuf[m_Dma.pWrite]-(unsigned long long)m_Dma.pBuf[0]));
		m_Dma.bufBytes[m_Dma.pWrite] = transfer_size_bytes;
		m_Dma.pWrite = (m_Dma.pWrite + 1) % m_Dma.nOfBuffers;
	}
	return 1;
}

int PCIe::DMADev2HostFinish(unsigned char** data_buffer, unsigned int &returnedBytes)
{
	BOOL success = TRUE;
	BOOL fIsRead = FALSE;
	DWORD dwStatus;
	returnedBytes = 0;

	ResetEvent(m_WaitForInterrupt);

	if (m_Dma.bufStatus[m_Dma.pRead] == PCIE_USER)
	{
		m_Dma.bufStatus[m_Dma.pRead] = PCIE_EMPTY;
		m_Dma.bufBytes[m_Dma.pRead] = 0;
		m_Dma.pRead = (m_Dma.pRead + 1) % m_Dma.nOfBuffers;
	}

	if (m_Dma.bufStatus[m_Dma.pRead] == PCIE_FULL)
	{
		m_Dma.bufStatus[m_Dma.pRead] = PCIE_USER;
		*data_buffer = (unsigned char*)(m_Dma.pBuf[m_Dma.pRead]);
		returnedBytes = m_Dma.bufBytes[m_Dma.pRead];
		return TRUE;
	}

    /* Poll for completion (if polling selected) */
	if (m_fPolling)
    {
        if (VIRTEX5_DMAPollCompletion(m_Dma.hDma, fIsRead))
		{
			success = TRUE;
		}
		else //timeout (10 sec)
		{
			success = FALSE;
		}
    }
	else
	{
		dwStatus = WaitForSingleObject(m_WaitForInterrupt, 1000);

//		ReleaseSemaphore(m_WaitForInterrupt,1,NULL);

		if (WD_STATUS_SUCCESS != dwStatus)//timeout (1 sec)
		{
			if (VIRTEX5_DMAIsDone(m_Dma.hDma->hDev, fIsRead))
				success = TRUE;
			else
				success = FALSE;
		}
		else
		{
			success = TRUE;
		}
	}

	if (success && (m_Dma.bufStatus[m_Dma.pRead] == PCIE_FULL))
	{
		m_Dma.bufStatus[m_Dma.pRead] = PCIE_USER;
		*data_buffer = (unsigned char*)(m_Dma.pBuf[m_Dma.pRead]);
		returnedBytes = m_Dma.bufBytes[m_Dma.pRead];
		return TRUE;
	}
    return FALSE;
}

int PCIe::ReceiveDataAbort()
{
	BOOL success = TRUE;
	BOOL fIsRead = FALSE;
	DWORD dwStatus;

	ResetEvent(m_WaitForInterrupt);

	if (m_Dma.bufStatus[m_Dma.pRead] == PCIE_USER)
	{
		m_Dma.bufStatus[m_Dma.pRead] = PCIE_EMPTY;
		m_Dma.bufBytes[m_Dma.pRead] = 0;
		m_Dma.pRead = (m_Dma.pRead + 1) % m_Dma.nOfBuffers;
	}

	while (success && (m_Dma.bufStatus[m_Dma.pRead] != PCIE_EMPTY))
	{
		if (m_Dma.bufStatus[m_Dma.pRead] == PCIE_WRITE)
		{
			/* Poll for completion (if polling selected) */
			if (m_fPolling)
			{
				if (VIRTEX5_DMAPollCompletion(m_Dma.hDma, fIsRead))
					success = TRUE;
				else //timeout (10 sec)
					success = FALSE;
			}
			else
			{
				dwStatus = WaitForSingleObject(m_WaitForInterrupt, 1000);
				ResetEvent(m_WaitForInterrupt);

				if (WD_STATUS_SUCCESS != dwStatus)//timeout (1 sec)
				{
					if (VIRTEX5_DMAIsDone(m_Dma.hDma->hDev, fIsRead))
						success = TRUE;
					else
						success = FALSE;
				}
			}
		}

		if (success)
		{
			m_Dma.bufStatus[m_Dma.pRead] = PCIE_EMPTY;
			m_Dma.bufBytes[m_Dma.pRead] = 0;
			m_Dma.pRead = (m_Dma.pRead + 1) % m_Dma.nOfBuffers;
		}
	}

	return success;
}

void PCIe::DMAClose(WDC_DEVICE_HANDLE hDev, PDMA pDma)
{
    if (!pDma)
        return;
    
    if (VIRTEX5_IntIsEnabled(hDev))
    {
        VIRTEX5_IntDisable(hDev);
    }
    if (pDma->hDma)
    {
        VIRTEX5_DMAClose(pDma->hDma);
    }
    BZERO(pDma->hDma);
	return;
}

void PCIe::DmaIntHandler(WDC_DEVICE_HANDLE hDev, VIRTEX5_INT_RESULT *pIntResult)
{
// Members in pIntResult
//    DWORD pIntResult->dwCounter; /* Number of interrupts received */
//    DWORD pIntResult->dwLost;    /* Number of interrupts not yet handled */
//    WD_INTERRUPT_WAIT_RESULT pIntResult->waitResult; /* See WD_INTERRUPT_WAIT_RESULT values in windrvr.h */
//    BOOL pIntResult->fIsMessageBased;
//    DWORD pIntResult->dwLastMessage;
//    PVOID pIntResult->pBuf;
//    UINT32 pIntResult->u32Pattern;
//    DWORD pIntResult->dwTotalCount;
//    BOOL pIntResult->fIsRead;

// Put code here to handle interrupts
// ex)
	//   if (pIntResult->fIsMessageBased)
    //		printf("Message data 0x%lx\n", pIntResult->dwLastMessage);

	mapType::iterator iter = m_DeviceInstances.find(hDev);
	if( iter != m_DeviceInstances.end() ) 
	{
		for (unsigned int i = 0; i <= pIntResult->dwLost; i++)
		{
			if (iter->second->m_Dma.bufStatus[iter->second->m_Dma.pSync] == PCIE_WRITE)
			{
				iter->second->m_Dma.bufStatus[iter->second->m_Dma.pSync] = PCIE_FULL;
				iter->second->m_Dma.pSync = (iter->second->m_Dma.pSync + 1) % iter->second->m_Dma.nOfBuffers;
				SetEvent(iter->second->m_WaitForInterrupt);
			}
		}
	}
}

DWORD	PCIe::WriteRegister(DWORD write_reg, unsigned int data)
{
	DWORD status;
	status = VIRTEX5_WriteReg32(m_Device, write_reg, (UINT32)data);
	return status;
}


DWORD	PCIe::ReadRegister(DWORD read_reg, unsigned int* data)
{
	DWORD status;
	status = VIRTEX5_ReadReg32(m_Device, read_reg, (UINT32*)data);
	return status;
}

int PCIe::ReceiveStatusFinish(unsigned int* data)
{
	unsigned int loop = 0;
	time_t Tstart, Tloop;
	double diff = 0;
	UINT32 dry = 0;
	DWORD status = 1; //not success
	int retry_cnt = 0;
	int retry_max = 100; 

	while ((retry_cnt < retry_max ) && (status != 0))
	{
		retry_cnt++;
		time(&Tstart);
		status = ReadRegister(PCI_DRY_BUSY, &dry);
		while ((dry & 0x1) && (diff < 2.0f))//(loop<timeout)) // 1 - 2 s timeout
		{
			time(&Tloop);	
			diff = difftime(Tloop,Tstart); //[s]
			loop++;
			if (loop == 50)
			{
				loop = 0;
				status |= ReadRegister(PCI_DRY_BUSY, &dry);
			}
		}
		if (diff >= 2.0f)
		{
			return 0;
		}
		status |= ReadRegister(PCI_CPU_OUT, data);
	}
	return (status == 0); //0 is success here
}

int PCIe::ReceiveDataInit(int transfer_size_bytes, unsigned int maxPrefetch)
{
	int success;
	unsigned int tfsize_bytes = transfer_size_bytes;
	success = DMADev2HostStart(tfsize_bytes, maxPrefetch);
	return success && (tfsize_bytes == unsigned int(transfer_size_bytes));
}

int PCIe::ReceiveDataFinish(int transfer_size_bytes, unsigned char **data_buffer_ptr)
{
	int success;
	unsigned int returnedBytes;
	success = DMADev2HostFinish(data_buffer_ptr, returnedBytes);
	return success && (returnedBytes == unsigned int(transfer_size_bytes));
}

unsigned int PCIe::Address()
{
	return m_pcie_device_location;
}

unsigned int PCIe::HwVersion()
{
	return 5; //PCIe controller rev -> TODO: Read from HW
}

unsigned int PCIe::ResetDevice(int resetlevel)
{
    unsigned int success = 0;

    if (resetlevel == 1)
    {
        ReceiveDataAbort();
        DMASyncBuffers();
    }

    return success; // Software Reset is not supported over PCIe
}

#endif