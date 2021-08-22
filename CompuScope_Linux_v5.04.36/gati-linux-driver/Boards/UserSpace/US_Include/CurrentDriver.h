// CurrentDriver.h
//

#include "CsSymLinks.h"



#if defined _SPIDER_DRV_

#include "CsSpider.h"

typedef		CsSpiderDev			CSDEVICE_DRVDLL;

#elif defined _FASTBALL_DRV_

#include "CsFastBall.h"

typedef		CsFastBallDev		CSDEVICE_DRVDLL;

#elif defined _RABBIT_DRV_

#include "CsRabbit.h"

typedef		CsRabbitDev		CSDEVICE_DRVDLL;

#elif defined _SPLENDA_DRV_

#include "CsSplenda.h"

typedef		CsSplendaDev		CSDEVICE_DRVDLL;

#elif defined _JDEERE_DRV_

#include "CsDeere.h"

typedef		CsDeereDevice		CSDEVICE_DRVDLL;


#elif defined _COBRAMAXPCIE_DRV_

#include "CsRabbit.h"

typedef		CsRabbitDev		CSDEVICE_DRVDLL;

#elif defined _DECADE_DRV_

#include "CsDecade.h"

typedef		CsDecade		CSDEVICE_DRVDLL;

#elif defined _HEXAGON_DRV_

#include "CsHexagon.h"

typedef		CsHexagon		CSDEVICE_DRVDLL;
#else


#endif
