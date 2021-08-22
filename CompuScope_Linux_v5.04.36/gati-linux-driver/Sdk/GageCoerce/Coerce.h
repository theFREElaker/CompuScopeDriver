
#ifndef __GAGE_COERCE_H__
#define __GAGE_COERCE_H__

BOOL ReportAcquisitionChanges(CSHANDLE hSystem, CSACQUISITIONCONFIG *pAcqConfig);
BOOL ReportChannelChanges(CSHANDLE hSystem, uInt32 u32ChannelCount, CSCHANNELCONFIG *pChanConfig);
BOOL ReportTriggerChanges(CSHANDLE hSystem, CSTRIGGERCONFIG *pTrigConfig);

#endif // __GAGE_COERCE_H__
