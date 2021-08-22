#ifndef __PeevAdaptor_h__
#define __PeevAdaptor_h__

#include "CsTypes.h"
#include "CsErrors.h"
#include "CsPeevStruct.h"


#define		 PEEVADAPTOR_DEFAULT_IO 0x378


class PeevAdaptor
{
private:
	uInt16	m_ParalellePort;
	BOOLEAN	m_bInitialized;
	uInt16	m_u16AdaptorPresent;
	uInt32	m_u32CurrentConfig;
	FILE_HANDLE	m_hIoctlHandle;


public:
	PeevAdaptor();
	~PeevAdaptor();

	void	Invalidate();
	int32	Initialize( FILE_HANDLE hDevIoctl );
	int32	SetConfig( uInt32 u32NewConfig, BOOLEAN bInit = 0 );
	uInt32	GetSwConfig();
	BOOLEAN	IsInitialized() { return m_bInitialized; };
	void	PeevOutp( uInt8 u8Val );
};


#endif