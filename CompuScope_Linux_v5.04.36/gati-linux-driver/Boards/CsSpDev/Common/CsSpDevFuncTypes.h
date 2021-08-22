#pragma once

#include "AdqApi.h"

typedef void*         (__cdecl _AdqCreateControlUnit)    (void);
typedef void          (__cdecl _AdqDeleteControlUnit)    (void* adq_cu_ptr);
typedef int	          (__cdecl _AdqGetApiRevision)       (void);
typedef int	          (__cdecl _AdqFindDevices)          (void* adq_cu_ptr);
typedef int	          (__cdecl _AdqDeleteDev)            (void* adq_cu_ptr, int adq_num);
typedef int	          (__cdecl _AdqGetFailedDeviceCount) (void* adq_cu_ptr);
typedef ADQInterface* (__cdecl _AdqGetADQInterface)      (void* adq_cu_ptr, int adq_num);


												        
													        
       