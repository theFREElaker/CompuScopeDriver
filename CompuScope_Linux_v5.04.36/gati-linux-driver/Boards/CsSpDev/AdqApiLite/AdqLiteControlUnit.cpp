// Project: ADQ-API
// File: ADQControlUnit.cpp
//


#include "AdqLiteControlUnit.h"
#include "AdqApiLite_definitions.h"

using namespace std;

ADQControlUnit::ADQControlUnit()
{
	m_CyUSBDevice = new CCyUSBDevice();
	m_nof_adq = 0;
	m_ADQ_list.clear();
}


ADQControlUnit::~ADQControlUnit()
{
	m_ADQ_list.clear();
	//m_ADQ114_list.clear();
	//m_ADQ214_list.clear();
	delete m_CyUSBDevice;
}


int ADQControlUnit::FindDevices()
{
	int devices = m_CyUSBDevice->DeviceCount();
	int iter;
	int temp_vID;
	int temp_pID;
	//int temp_bcdUSB;
	//int temp_bcdDevice;
	
	//search for devices connected to driver
	for (iter=0; iter<devices; iter++)
	{
		m_CyUSBDevice->Open(UCHAR(iter));
		temp_vID = m_CyUSBDevice->VendorID;
		temp_pID = m_CyUSBDevice->ProductID;
		//temp_bcdUSB = m_CyUSBDevice->BcdUSB;
		//temp_bcdDevice = m_CyUSBDevice->BcdDevice;
		
		//if ((temp_vID == VID) && (temp_pID == PID_ADQ108)) //is the current device an ADQ108?
		//{
		//	//check if current device is already opened (checks if any devices has been recognised at the same usb address)
		//	int i;
		//	bool already_opened = false;
		//	unsigned int current_device_usbaddr = m_CyUSBDevice->USBAddress;
		//	for (i=0; i<m_nof_adq108; i++) 
		//	{
		//		unsigned int temp_device_usbaddr = (m_ADQ108_list[i])->GetUSBAddress();
		//		if (temp_device_usbaddr == current_device_usbaddr)
		//			already_opened = true;
		//	}
		//	//add new ADQ108 object if not already opened
		//	if (already_opened == false)
		//	{
		//		CCyUSBDevice* new_usb_dev = new CCyUSBDevice();
		//		ADQ108* new_ADQ108 = new ADQ108(new_usb_dev,iter); //DEBUG
		//		m_ADQ108_list.push_back(new_ADQ108);
		//	}
		//}
		//	
		//if ((temp_vID == VID) && (temp_pID == PID_ADQ114)) //is the current device an ADQ114? 
		//{
		//	//check if current device is already opened (checks if any devices has been recognised at the same usb address)
		//	int i;
		//	bool already_opened = false;
		//	unsigned int current_device_usbaddr = m_CyUSBDevice->USBAddress;
		//	for (i=0; i<m_nof_adq114; i++) 
		//	{
		//		unsigned int temp_device_usbaddr = (m_ADQ114_list[i])->GetUSBAddress();
		//		if (temp_device_usbaddr == current_device_usbaddr)
		//			already_opened = true;
		//	}
		//	//add new ADQ114 object if not already opened
		//	if (already_opened == false)
		//	{
		//		CCyUSBDevice* new_usb_dev = new CCyUSBDevice();
		//		ADQ114* new_ADQ114 = new ADQ114(new_usb_dev,iter); //DEBUG
		//		m_ADQ114_list.push_back(new_ADQ114);
		//	}
		//}
		//	
		//if ((temp_vID == VID) && (temp_pID == PID_ADQ214)) //is the current device an ADQ214? 
		//{
		//	//check if current device is already opened (checks if any devices has been recognised at the same usb address)
		//	int i;
		//	bool already_opened = false;
		//	unsigned int current_device_usbaddr = m_CyUSBDevice->USBAddress;
		//	for (i=0; i<m_nof_adq214; i++) 
		//	{
		//		unsigned int temp_device_usbaddr = (m_ADQ214_list[i])->GetUSBAddress();
		//		if (temp_device_usbaddr == current_device_usbaddr)
		//			already_opened = true;
		//	}
		//	//add new ADQ214 object if not already opened
		//	if (already_opened == false)
		//	{
		//		CCyUSBDevice* new_usb_dev = new CCyUSBDevice();
		//		ADQ214* new_ADQ214 = new ADQ214(new_usb_dev,iter);
		//		m_ADQ214_list.push_back(new_ADQ214);
		//	}
		//}

		//Added by Jonas Persson
		//from here...
		if (temp_vID == 0x1d73) //is the current device an SPDevices device? 
		{
			//check if current device is already opened (checks if any devices has been recognised at the same usb address)
			int i;
			bool already_opened = false;
			unsigned int current_device_usbaddr = m_CyUSBDevice->USBAddress;
			for (i=0; i<m_nof_adq; i++) 
			{
				unsigned int temp_device_usbaddr = (m_ADQ_list[i])->GetUSBAddress();
				if (temp_device_usbaddr == current_device_usbaddr)
					already_opened = true;
			}
			//add new ADQ108 object if not already opened
			if (already_opened == false)
			{
				CCyUSBDevice* new_usb_dev = new CCyUSBDevice();
				ADQ* new_ADQ = new ADQ(new_usb_dev,iter); //DEBUG
				m_ADQ_list.push_back(new_ADQ);
			}
		}
	}

	m_nof_adq = (int)m_ADQ_list.size();
	return m_nof_adq;
}


int ADQControlUnit::NofADQ()
{
	if (this != NULL) {
		return m_nof_adq;
	}
	else
	{
		return 0;
	}
}


ADQ* ADQControlUnit::GetADQ(int n)
{
	if ((n>=1) && (n<=m_nof_adq))
		return m_ADQ_list[n-1];
	else
		return NULL;
}


void ADQControlUnit::DeleteADQ(int n)
{
	if ((n>=1) && (n<=m_nof_adq))
	{
		m_ADQ_list.erase(m_ADQ_list.begin()+n);
		m_nof_adq = (int)m_ADQ_list.size();
	}
}
