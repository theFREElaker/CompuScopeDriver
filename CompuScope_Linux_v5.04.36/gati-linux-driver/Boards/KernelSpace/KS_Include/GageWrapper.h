/*
 * GageWrapper.h
 *
 *  Created on: 29-Aug-2008
 *      Author: quang
 */

#ifndef GAGEWRAPPER_H_
#define GAGEWRAPPER_H_

#include "CsTypes.h"

void 	*GageAllocateMemory(uInt32 size);
void 	GageFreeMemory (void *ptr);
char	*basename (char *path);
void	GageCopyMemory(void *dst, void *src, uInt32 size);
void	GageZeroMemory(void *dst, uInt32 size);

#endif /* GAGEWRAPPER_H_ */
