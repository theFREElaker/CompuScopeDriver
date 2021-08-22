// Project: ADQ-API
// File: ADQControlUnit.cpp

#include "ADQControlUnit.h"
#include "ADQAPI_definitions.h"
#include "swrev.h"
#include "USB.h"
#include "PCIe.h"
#include "virtex5_lib.h"

using namespace std;

ADQListEntry::~ADQListEntry()
{
    if (ADQPtr != NULL)
    {
        delete ADQPtr;
    }
}

ADQControlUnit::ADQControlUnit(HANDLE windowhandle)
{
	m_nof_adq = 0;
	m_ADQ_list.clear();
	n_of_failed_devices = 0;

//	wchar_t *USBLib = L"cyapi.dll";
//	m_HUSBDll = LoadLibrary(USBLib);
	m_HUSBDll = (HANDLE)1; //Link statically for now
	if (m_HUSBDll)
	{
		m_CyUSBDevice = new CCyUSBDevice(windowhandle); //USB::m_ADQ_USB_GUID
		m_CyUSBDevice->Close(); //'new' always opens device 0, close this
	}
#ifndef USB_ONLY
	wchar_t *PCIeLib = L"wdapi1002.dll";

	m_HPCIExpressDll = LoadLibrary(PCIeLib);
	if (m_HPCIExpressDll)
	{
		DWORD dwStatus = VIRTEX5_LibInit();
	}
#endif
}

ADQControlUnit::~ADQControlUnit()
{
	int i;
	for (i=0; i<(int)m_ADQ_list.size(); i++)
	{
        delete m_ADQ_list.at(i);
	}
	
	m_nof_adq = 0;
	m_ADQ_list.clear();
	n_of_failed_devices = 0;

#ifndef USB_ONLY
	if (m_HPCIExpressDll)
	{
		VIRTEX5_LibUninit();
	}
#endif
	if ((m_HUSBDll != NULL) && (m_CyUSBDevice != NULL))
	{
		delete m_CyUSBDevice;
		m_CyUSBDevice = NULL;
	}
}

int ADQControlUnit::GetNumberOfFailedDevices(void)
{
	return n_of_failed_devices;
}

int ADQControlUnit::FindDevices(void)
{
	int iter;
	int temp_vID;
	int temp_pID;
	int failures = 0;

	// Check lists for dead cards and clean

	for (iter=0; iter<(int)m_ADQ_list.size();)
	{
        ADQ* device = m_ADQ_list.at(iter)->ADQPtr;
		if (!device->IsAlive())
		{
			// Remove from list
			this->DeleteADQ(iter+1);
		}
		else
		{
			iter++;
		}
	}

	// Find USB devices
	if (m_HUSBDll != NULL)
	{
		int usb_devices = m_CyUSBDevice->DeviceCount();

		//search for devices connected to driver
		for (iter=0; iter<usb_devices; iter++)
		{
			unsigned int current_device_usbaddr;
			if(!m_CyUSBDevice->Open(UCHAR(iter)))
			{
				failures++;
				continue;
			}
			temp_vID = m_CyUSBDevice->VendorID;
			temp_pID = m_CyUSBDevice->ProductID;
			current_device_usbaddr = m_CyUSBDevice->USBAddress;
			m_CyUSBDevice->Close();
			
			if (temp_vID == USB_VID)
			{
				//check if current device is already opened (checks if any devices has been recognised at the same usb address)
				int i;
				bool already_opened = false;
                for (i=0; i<(int)m_ADQ_list.size(); i++) 
				{
                    unsigned int temp_device_usbaddr = (m_ADQ_list[i]->ADQPtr)->GetUSBAddress();
					if (temp_device_usbaddr == current_device_usbaddr)
						already_opened = true;
				}
				//add new ADQ object if not already opened
				if (already_opened == false)
				{
					USB* new_USB = new USB();
                    ADQListEntry* new_ADQListEntry = new ADQListEntry();
                    new_ADQListEntry->pID = temp_pID;
                    switch(temp_pID)
                    {   
                        case PID_ADQ112:
                            new_ADQListEntry->ADQPtr = new ADQ112();
                            break;
                        case PID_ADQ114:
                            new_ADQListEntry->ADQPtr = new ADQ114();
                            break;
                        case PID_ADQ214:
                            new_ADQListEntry->ADQPtr = new ADQ214();
                            break;
                        case PID_ADQ108:
                            new_ADQListEntry->ADQPtr = new ADQ108();
                            break;
                        default:
                            new_ADQListEntry->ADQPtr = NULL;
                            break;
                    }
					if (   new_ADQListEntry->ADQPtr 
					    && new_USB->Open(iter) 
                        && new_ADQListEntry->ADQPtr->Open(new_USB))
					{
						m_ADQ_list.push_back(new_ADQListEntry);
					}
					else
					{
						failures++;
                        if (new_ADQListEntry->ADQPtr)
                        {
						    delete new_ADQListEntry->ADQPtr; //Also deletes it's device
                            new_ADQListEntry->ADQPtr = NULL;
                        }
					}
				}
			}			
        }
    }

#ifndef USB_ONLY
	// Find P*Ie devices (if and only if DLL is possible to load)

	if (m_HPCIExpressDll)
	{
		DWORD dwStatus;
		WDC_PCI_SCAN_RESULT scanResult;
		int pcie_devices;
        bool ADQ_found = true;

		//----search for ADQ214----
		BZERO(scanResult);
		dwStatus = WDC_PciScanDevices(PCIE_VID, NULL, &scanResult);
		if (WD_STATUS_SUCCESS != dwStatus)
		{
			ADQ_found = false;
		}

		pcie_devices = scanResult.dwNumDevices;
		if (pcie_devices == 0)
		{
			ADQ_found = false;
		}

		if (ADQ_found)
		{
			//search for devices connected to driver
			for (iter=0; iter<pcie_devices; iter++)
			{
    			unsigned int current_device_pcieaddr;
				WDC_DEVICE_HANDLE hDev;
				WD_PCI_CARD_INFO deviceInfo;
                WD_PCI_ID devID;

                devID = scanResult.deviceId[iter];
                temp_pID = devID.dwDeviceId;
				deviceInfo.pciSlot = scanResult.deviceSlot[iter];
				dwStatus = WDC_PciGetDeviceInfo(&deviceInfo);
				if (WD_STATUS_SUCCESS != dwStatus)
				{
					continue;
				}
				hDev = VIRTEX5_DeviceOpen(&deviceInfo, "KP_ADQ");
				if (!hDev)
				{
					continue;
				}
				
				unsigned long device_location;
				device_location = (((unsigned int)deviceInfo.pciSlot.dwBus << 16) | deviceInfo.pciSlot.dwSlot);
                current_device_pcieaddr = device_location;

				//check if current device is already opened (checks if any devices has been recognised at the same usb address)
				int i;
				bool already_opened = false;
                for (i=0; i<(int)m_ADQ_list.size(); i++) 
				{
                    unsigned int temp_device_pcieaddr = (m_ADQ_list[i]->ADQPtr)->GetPCIeAddress();
					if (temp_device_pcieaddr == current_device_pcieaddr)
						already_opened = true;
				}
				//add new ADQ object if not already opened
				if (already_opened == false)
				{
					PCIe* new_PCIe = new PCIe();
                    ADQListEntry* new_ADQListEntry = new ADQListEntry();
                    new_ADQListEntry->pID = temp_pID;
                    switch(temp_pID)
                    {   
                        case PID_ADQ112:
                            new_ADQListEntry->ADQPtr = new ADQ112();
                            break;
                        case PID_ADQ114:
                            new_ADQListEntry->ADQPtr = new ADQ114();
                            break;
                        case PID_ADQ214:
                            new_ADQListEntry->ADQPtr = new ADQ214();
                            break;
                        case PID_ADQ108:
                            new_ADQListEntry->ADQPtr = new ADQ108();
                            break;
                        default:
                            new_ADQListEntry->ADQPtr = NULL;
                            break;
                    }
					if (   new_ADQListEntry->ADQPtr 
                        && new_PCIe->Open(hDev, device_location) 
                        && new_ADQListEntry->ADQPtr->Open(new_PCIe))
					{
						m_ADQ_list.push_back(new_ADQListEntry);
					}
					else
					{
						failures++;
                        if (new_ADQListEntry->ADQPtr)
                        {
						    delete new_ADQListEntry->ADQPtr; //Also deletes it's device
                            new_ADQListEntry->ADQPtr = NULL;
                        }
					}
				}
			}
		}
	}
#endif

	m_nof_adq = (int)m_ADQ_list.size();

	n_of_started_devices = m_nof_adq;
	n_of_failed_devices = failures;

	return n_of_started_devices;
}


int ADQControlUnit::NofADQ()
{
    return (int)m_ADQ_list.size();
}


int ADQControlUnit::NofADQ112()
{
    unsigned int LoopVar;
    unsigned int Index = 0;

    for (LoopVar = 0; LoopVar < m_ADQ_list.size();LoopVar++)
    {
        if (m_ADQ_list.at(LoopVar)->pID == PID_ADQ112)
        {
            Index++;
        }
    }

    return Index;
}


int ADQControlUnit::NofADQ114()
{
    unsigned int LoopVar;
    unsigned int Index = 0;

    for (LoopVar = 0; LoopVar < m_ADQ_list.size();LoopVar++)
    {
        if (m_ADQ_list.at(LoopVar)->pID == PID_ADQ114)
        {
            Index++;
        }
    }

    return Index;
}


int ADQControlUnit::NofADQ214()
{
    unsigned int LoopVar;
    unsigned int Index = 0;

    for (LoopVar = 0; LoopVar < m_ADQ_list.size();LoopVar++)
    {
        if (m_ADQ_list.at(LoopVar)->pID == PID_ADQ214)
        {
            Index++;
        }
    }

    return Index;
}


ADQ112* ADQControlUnit::GetADQ112(unsigned int n)
{
    unsigned int LoopVar;
    unsigned int Index = 0;
    ADQ112* result = NULL;

    if (n >= 1)
    {
        for (LoopVar = 0; LoopVar < m_ADQ_list.size();LoopVar++)
        {
            if (m_ADQ_list.at(LoopVar)->pID == PID_ADQ112)
            {
                Index++;
            }

            if (n == Index)
            {
                result = (ADQ112*)m_ADQ_list.at(LoopVar)->ADQPtr;
            }
        }
    }

    return result;
}

ADQInterface* ADQControlUnit::GetADQ(unsigned int n)
{
    ADQInterface* result = NULL;

    if ((n >= 1) && (n <= (int)m_ADQ_list.size()))
    {
        result = (ADQInterface*)m_ADQ_list.at(n-1)->ADQPtr;
    }

    return result;
}


ADQ114* ADQControlUnit::GetADQ114(unsigned int n)
{
    unsigned int LoopVar;
    unsigned int Index = 0;
    ADQ114* result = NULL;

    if (n >= 1)
    {
        for (LoopVar = 0; LoopVar < m_ADQ_list.size();LoopVar++)
        {
            if (m_ADQ_list.at(LoopVar)->pID == PID_ADQ114)
            {
                Index++;
            }

            if (n == Index)
            {
                result = (ADQ114*)m_ADQ_list.at(LoopVar)->ADQPtr;
            }
        }
    }

    return result;
}


ADQ214* ADQControlUnit::GetADQ214(unsigned int n)
{
    unsigned int LoopVar;
    unsigned int Index = 0;
    ADQ214* result = NULL;

    if (n >= 1)
    {
        for (LoopVar = 0; LoopVar < m_ADQ_list.size();LoopVar++)
        {
            if (m_ADQ_list.at(LoopVar)->pID == PID_ADQ214)
            {
                Index++;
            }

            if (n == Index)
            {
                result = (ADQ214*)m_ADQ_list.at(LoopVar)->ADQPtr;
            }
        }
    }

    return result;

}
	

void ADQControlUnit::DeleteADQ(unsigned int n)
{
	if ((n>=1) && (n<=(int)m_ADQ_list.size()))
	{
		delete m_ADQ_list.at(n-1);
		m_ADQ_list.erase(m_ADQ_list.begin()+n-1);
		m_nof_adq = (int)m_ADQ_list.size();
	}
}

void ADQControlUnit::DeleteADQ112(unsigned int n)
{
    unsigned int LoopVar;
    unsigned int ADQ112Index = 0;

    if (n >= 1)
    {
        for (LoopVar = 0; LoopVar < m_ADQ_list.size();LoopVar++)
        {
            if (m_ADQ_list.at(LoopVar)->pID == PID_ADQ112)
            {
                ADQ112Index++;
            }

            if (n == ADQ112Index)
            {
                delete m_ADQ_list.at(LoopVar);
                m_ADQ_list.erase(m_ADQ_list.begin()+LoopVar);
                break;
            }
        }
    }

    m_nof_adq = (int)m_ADQ_list.size();
}


void ADQControlUnit::DeleteADQ114(unsigned int n)
{
    unsigned int LoopVar;
    unsigned int ADQ114Index = 0;

    if (n >= 1)
    {
        for (LoopVar = 0; LoopVar < m_ADQ_list.size();LoopVar++)
        {
            if (m_ADQ_list.at(LoopVar)->pID == PID_ADQ114)
            {
                ADQ114Index++;
            }

            if (n == ADQ114Index)
            {
                delete m_ADQ_list.at(LoopVar);
                m_ADQ_list.erase(m_ADQ_list.begin()+LoopVar);
                break;
            }
        }
    }

    m_nof_adq = (int)m_ADQ_list.size();
}


void ADQControlUnit::DeleteADQ214(unsigned int n)
{
    unsigned int LoopVar;
    unsigned int ADQ214Index = 0;

    if (n >= 1)
    {
        for (LoopVar = 0; LoopVar < m_ADQ_list.size();LoopVar++)
        {
            if (m_ADQ_list.at(LoopVar)->pID == PID_ADQ214)
            {
                ADQ214Index++;
            }

            if (n == ADQ214Index)
            {
                delete m_ADQ_list.at(LoopVar);
                m_ADQ_list.erase(m_ADQ_list.begin()+LoopVar);
                break;
            }
        }
    }

    m_nof_adq = (int)m_ADQ_list.size();
}