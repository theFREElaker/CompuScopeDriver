/*
 * GageWrapper.c
 *
 *  Created on: 29-Aug-2008
 *      Author: quang
 */
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/hardirq.h>

#include "gen_kernel_interface.h"
#include "gen_kdebug.h"                /* debug output macro */


#include "GageWrapper.h"

//-----------------------------------------------------------------------------

void *GageAllocateMemory(uInt32 size)
{
    PVOID p;

    if (in_atomic ())
        p = GEN_CALLOCATE (1, size, GFP_ATOMIC);
    else
        p = GEN_CALLOCATE (1, size, GFP_KERNEL);

    return (p);
}


//-----------------------------------------------------------------------------

void GageFreeMemory (void *ptr)
{
    GEN_FREE (ptr);
}

//-----------------------------------------------------------------------------

char *basename (char *path)
{
    char *pathend;

    if (path == NULL) return (NULL);

    pathend = path + strlen (path) - 1;
    while (pathend >= path) {
        if (*pathend == '/') return (pathend + 1);
        pathend --;
    }

    return (path);
}


//-----------------------------------------------------------------------------

void	GageCopyMemory(void *dst, void *src, uInt32 size)
{
	memcpy(dst, src, size);
}

//-----------------------------------------------------------------------------

void	GageZeroMemory(void *dst, uInt32 size)
{
	memset(dst, 0, size);
}


