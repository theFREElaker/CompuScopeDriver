#ifndef __GAGECFGPARSER_H__

#define __GAGECFGPARSER_H__

#include "CsTypes.h"
//#include "CsDrvStructs.h"
#include "CsDrvConst.h"
#include "CsiHeader.h"
#include "CsPrivateStruct.h"
#include "CsLinuxPort.h"
#include <stdlib.h>

#define DRV_CFG_NAME          "GageDriverCfg.ini"
#define DRV_LOCAL_ENV_PATH    "GAGE_CFG"
#define DRV_DEFAULT_ENV_PATH  "/usr/local/share"

#define DRV_CFG_DEFAULT       "Defaults"            /* if this section exist, set default key value for all boards */
#define DISPLAY_CFG_KEYINFO   "DisplayKeyInfo"      /* if this key set, display cfg key list */

int IsCfgFileExist(char* szIniFile);
int GetCfgFile(char* szIniFile, int size);
int GetCfgSection(char* lpSectionName,
							   char* lpReturnedString,
							   int nSize,
							   char* lpFileName);

int GetCfgKeyString(char* lpSectionName,
							  char* lpKeyName,
							  char* lpDefault,
							  char* lpReturnedString,
							  int nSize,
							  char* lpFileName);
                        
int GetCfgKeyInt(char* lpSectionName,
							  const char* lpKeyName,
							  int nDefault,
							  char* lpFileName);	

#endif
