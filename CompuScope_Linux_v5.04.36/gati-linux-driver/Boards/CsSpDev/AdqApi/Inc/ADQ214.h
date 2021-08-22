// Project: ADQ-API
// File: ADQ214.h


#ifndef ADQ214_H
#define ADQ214_H

#include "ADQ.h"
#include "ADQAPI_definitions.h"
#include "HWIF.h"

class ADQ214 : public ADQ
    {
    public:
		ADQ214();
		~ADQ214();
        int GetADQType(void);
		int Open(HWIF* device);
		void* GetPtrData(unsigned int channel);
        int* GetPtrDataChA(void);            
        int* GetPtrDataChB(void);            
        int SetDataFormat(unsigned int format);
        int GetTriggedCh();
        int SetLvlTrigChannel(int channel);
        int GetLvlTrigChannel();
        unsigned int SetSampleSkip(unsigned int SkipFactor);
        unsigned int GetSampleSkip(void);
        unsigned int SetSampleDecimation(unsigned int);
		unsigned int GetSampleDecimation();
		int SetAfeSwitch(unsigned int afe);
    
    private:
		int ParseSampleData(unsigned int wordOffset, unsigned int sampleOffset, unsigned int destOffset, unsigned int maxsamples);

        int* m_data_buffer_chA;
        int* m_data_buffer_chB;
        int m_trig_channel;
};
#endif //ADQ214_H
