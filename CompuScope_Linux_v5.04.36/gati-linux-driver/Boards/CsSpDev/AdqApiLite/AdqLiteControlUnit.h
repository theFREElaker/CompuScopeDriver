// Project: ADQ-API_Lite
// File: ADQControlUnit.h
//



#ifndef ADQCONTROLUNIT
#define ADQCONTROLUNIT

#include "windows.h"
#include "CyAPI.h"
#include <vector>
#include "AdqLite.h"

using namespace std; 

class ADQControlUnit
    {
    public:
		ADQControlUnit();  
		~ADQControlUnit();
		int FindDevices();
		int NofADQ();
		ADQ* GetADQ(int n);
		void DeleteADQ(int n);
    private:
		CCyUSBDevice* m_CyUSBDevice;
		vector<ADQ*> m_ADQ_list;
		int m_nof_adq;
    };
#endif //ADQCONTROLUNIT
