/*E-----------------------------------------------------------------------------
  Project       : CS14105 acquisition card Linux driver.
  Author        :

  Module        : $RCSfile$
  Revision      : $Revision$
  Date          : $Date$
  Checked in by : $Author$
  Description   : Module load and remove functions.
*/
/*------------------------------------------------------------------------------

                              Copyright (C) 2005
                        Gage Applied Technologies Inc.

  This software is the sole property of Gage Applied Technologies Inc.
  and may not be copied or reproduced in any way without prior written
  permission from Gage Applied Technologies Inc. This software is intended for
  use in systems produced by Gage Applied Technologies Inc.

  This software or any other copies thereof may not be provided or otherwise
  made available to any other person without written permission from
  Gage Applied Technologies Inc. No title to and ownership of the software is
  hereby transferred.

  The information in this software is subject to change without notice and
  should not be construed as a commitment by Gage Applied Technologies Inc.
------------------------------------------------------------------------------*/

/*============================================================================*/
/*                         I N C L U D E   F I L E S                          */
/*============================================================================*/

#include <linux/module.h>
#include <linux/version.h>
#if defined(__linux__ ) && defined(__KERNEL__) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
#include <linux/config.h>
#endif
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>

#include "gen_kernel_interface.h"
#include "gen_kdebug.h"                /* debug output macro */

#if defined(_DECADE_DRV_ )
#  include "DecadeGageDrv.h"
#elif defined(_HEXAGON_DRV_ )  
   #  include "HexagonGageDrv.h"
#else
   #  include "GageDrv.h"
#endif

/*============================================================================*/
/*                                M A C R O S                                 */
/*============================================================================*/

/*============================================================================*/
/*          L O C A L    S T R U C T U R E   D E F I N I T I O N S            */
/*============================================================================*/

/*============================================================================*/
/*          E X T E R N A L   F U N C T I O N   P R O T O T Y P E S           */
/*============================================================================*/

/*============================================================================*/
/*           E X T E R N A L    D A T A   D E C L A R A T I O N S             */
/*============================================================================*/

/*============================================================================*/
/*                          G L O B A L    D A T A                            */
/*============================================================================*/


/*============================================================================*/
/*            L O C A L    F U N C T I O N   P R O T O T Y P E S              */
/*============================================================================*/

GEN_LOCAL void __exit GageDrv_exit    (void);

/*============================================================================*/
/*                           L O C A L    D A T A                             */
/*============================================================================*/

/*============================================================================*/
/*        G L O B A L   F U N C T I O N S   C O D E    S E C T I O N          */
/*============================================================================*/

/*============================================================================*/
/*     I N T E R F A C E    F U N C T I O N S   C O D E    S E C T I O N      */
/*============================================================================*/

/*============================================================================*/
/*         L O C A L    F U N C T I O N S   C O D E    S E C T I O N          */
/*============================================================================*/

/*E-----------------------------------------------------------------------------
Function    : GageDrv_init

Description : Module load function for
              the Linux Cs14105 acquisition card driver.

Parameters  : void

Return Value: 0 if successful
              negative Linux error code otherwise

Notes       : Called as a result of doing an insmod.
------------------------------------------------------------------------------*/

GEN_LOCAL int  __init GageDrv_init (void)
{
    int ret;

    ret = GageFindDevice();

    if ( ret > 0 )
    {
    	GEN_PRINT ("GageDrv module inserted\n");
    	return (GEN_OK);
    }
    else
    {
    	ret = -1;
    	GEN_PRINT ("GageDrv module initialization failed, error %d\n", ret);
    	return (ret);
    }
}


/*E-----------------------------------------------------------------------------
Function    : GageDrv_exit

Description : Module remove function for
              the Linux Cs14105 acquisition card driver.

Parameters  : void

Return Value: void

Notes       : Called as a result of doing an rmmod.
------------------------------------------------------------------------------*/

static void GageDrv_exit (void)
{
   //cs14105device_destructor ();
   GageReleaseDevice();
   GEN_PRINT ("GageDrv module removed\n");
}


/*----------------------------------------------------------*/
/* Linux convention is to put these at the end of the file. */
/*----------------------------------------------------------*/

module_init(GageDrv_init);
module_exit(GageDrv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Quang vu");
MODULE_DESCRIPTION("Gage CompuScope Driver");
MODULE_VERSION("1.0");
//MODULE_DEVICE_TABLE("GageDrv");
