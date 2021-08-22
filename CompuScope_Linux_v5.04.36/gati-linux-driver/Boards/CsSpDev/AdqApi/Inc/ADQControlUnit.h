// Project: ADQ-API
// File: ADQControlUnit.h
//
// latest revision: 2008-10-26
// by Peter Johansson


#ifndef ADQCONTROLUNIT
#define ADQCONTROLUNIT

#include "windows.h"
#include "CyAPI.h"
#include <vector>
#include "ADQ.h"
#include "ADQ108.h"
#include "ADQ112.h"
#include "ADQ114.h"
#include "ADQ214.h"

using namespace std; 

class ADQListEntry
{
public:
    ~ADQListEntry();
    ADQ* ADQPtr;
    unsigned int pID;
};

class ADQControlUnit
    {
    public:
		ADQControlUnit(HANDLE);  
		~ADQControlUnit();
		int FindDevices();
		int RescanDevices();
		int GetNumberOfFailedDevices();
		int NofADQ112();
		int NofADQ114();
		int NofADQ214();
		int NofADQ();
		ADQ112* GetADQ112(unsigned int n);
		ADQ114* GetADQ114(unsigned int n);
		ADQ214* GetADQ214(unsigned int n);
        ADQInterface* GetADQ(unsigned int n);
		void DeleteADQ112(unsigned int n);
		void DeleteADQ114(unsigned int n);
		void DeleteADQ214(unsigned int n);
        void DeleteADQ(unsigned int n);
    private:
		CCyUSBDevice* m_CyUSBDevice;
        vector<ADQListEntry*> m_ADQ_list;
        int m_nof_adq;
		int n_of_failed_devices;
		int n_of_started_devices;
		HANDLE m_HPCIExpressDll;
		HANDLE m_HUSBDll;
    };
#endif //ADQCONTROLUNIT
