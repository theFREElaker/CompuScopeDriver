#pragma once

#include <QtGlobal>

#ifdef Q_OS_WIN
    #include <windows.h>
    #include "CsTypes.h"
    #include "CsStruct.h"
#endif

#include "CsTypes.h"
#include "CsStruct.h"
#include <vector>
#include <map>
#include <QString>

using namespace std;


//typedef int	(__stdcall* LPCSGETSYSTEMCAPS) (CSHANDLE, unsigned int, void*, unsigned int*); // RG move somewhere else

typedef struct _SampleRateTable
{
	int nItemCount;
	PCSSAMPLERATETABLE pSRTable;
} SampleRateTable, *PSampleRateTable;

// map for sample rates, the key is the acquisition mode
typedef map<uInt32, SampleRateTable> SRTableMap;

typedef struct _InputRangeTable
{
	int nItemCount;
	PCSRANGETABLE pRangeTable;
} InputRangeTable, *PInputRangeTable;

// map for input ranges, the key is impedance
typedef map<uInt32, InputRangeTable> IRTableMap;

typedef struct _ModeNames
{
    QString strName;
	uInt32 u32Mode;
} ModeNames, *PModeNames;


class CSystemCaps
{
public:
	CSystemCaps(void);
	~CSystemCaps(void);

	uInt32 GetModeCount();
    int32 GetAcquisitionModes();
    int32 GetSampleRates();
    int32 GetImpedances();
    int32 GetInputRanges();
    int32 GetCouplings();
    int32 GetFilters();
    int32 GetExtClock();
    int32 GetMultipleRecord();

private:
    int32 QueryAcquisitionModes();
    int32 QuerySampleRates();
    int32 QueryImpedances();
    int32 QueryInputRanges();
    int32 QueryCouplings();
    int32 QueryFilters();
    int32 QueryExtClock();
    int32 QueryMultipleRecord();


	CSHANDLE			m_csHandle;
    //LPCSGETSYSTEMCAPS	m_pCsGetSystemCaps;

};


