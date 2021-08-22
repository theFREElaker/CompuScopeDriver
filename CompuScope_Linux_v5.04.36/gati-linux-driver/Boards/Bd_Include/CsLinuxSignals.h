#ifdef __linux__

#ifndef __KERNEL__
	#include <signal.h>
#endif


#define SIGNAL_TRIGGER			0x01
#define SIGNAL_ENDOFBUSY		0x02
#define SIGNAL_ENDOFTRANSFER	0x03


#if defined _FASTBALL_DRV_

#define CS_EVENT_SIGNAL		(SIGRTMAX)

#elif defined _SPIDER_DRV_

#define CS_EVENT_SIGNAL		(SIGRTMAX-1)

#elif defined _RABBIT_DRV_

#define CS_EVENT_SIGNAL		(SIGRTMAX-2)

#elif defined _SPLENDA_DRV_

#define CS_EVENT_SIGNAL		(SIGRTMAX-3)

#elif defined _JDEERE_DRV_

#define CS_EVENT_SIGNAL		(SIGRTMAX-4)

#elif defined _DECADE_DRV_

#define CS_EVENT_SIGNAL		(SIGRTMAX-5)

#elif defined _HEXAGON_DRV_

#define CS_EVENT_SIGNAL		(SIGRTMAX-6)

#elif defined _COBRAMAXPCIE_DRV_

#define CS_EVENT_SIGNAL		(SIGRTMAX-7)

#endif //_FASTBALL_DRV_
#endif //__linux__
