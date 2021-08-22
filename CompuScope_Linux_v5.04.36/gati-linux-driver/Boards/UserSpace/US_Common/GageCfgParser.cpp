//	GageCfgParser
//	Implementation Config file (.ini) parser
//
//	This file contains APIs for reading, parsing .ini config file 
//	
//
//
#include "GageCfgParser.h"

#ifdef __linux__

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "limits.h"

#ifndef MAX_LINE_LENGTH
#define MAX_LINE_LENGTH 2048
#endif

// convert ascii string to 64 bit integer.
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
long long  _atoll(const char *szValue)
{
   long long i64Value = 0;
   int iVal = 0;
   int is_neg = 0;
   unsigned char  *pos = (unsigned char *)szValue;
   if (!isdigit(iVal = *pos)) {
      while (isspace(iVal))
         iVal = *++pos;
      switch (iVal) {
         case '-':
            is_neg=1;
         case '+':
            iVal = *++pos;
      }
      if (!isdigit(iVal))
         return (0);
   }
   for (i64Value = '0' - iVal; isdigit(iVal = *++pos); ) {
      i64Value *= 10;          /* compute the value */
      i64Value += '0' - iVal;  
   }
   return (is_neg ? i64Value : -i64Value);
}

int GetCfgFileAttributes(char* szFilePath)
{
	// for some reason, Linux returns 0 on success
	if(0 == access( szFilePath, F_OK ) )
	{
	    return 1;
	}
	else
	    return -1;
}

//-----------------------------------------------------------------------------
// Validate if Cfg file exist
//-----------------------------------------------------------------------------
int IsCfgFileExist(char* szIniFile)
{
	FILE *fp = fopen(szIniFile, "r");
	if ( NULL == fp )
		return 0;   
   else
      fclose(fp);
   return 1;
}

//-----------------------------------------------------------------------------
// Get the config file with full path
// Search for cfg file under local path defined under env variable "GAGE_CFG"
// if not found then try the default path. make sure buffer size is large enough
//-----------------------------------------------------------------------------
int GetCfgFile(char* szIniFile, int size)
{
	FILE *fp = NULL;
   int ret = 1;
   char szTmp[256]={0};
   const char* s = getenv(DRV_LOCAL_ENV_PATH);  
   //printf("GAGE_CFG :%s\n",(s!=NULL)? s : "getenv returned NULL");   
   if (s)
   {
      sprintf(szTmp,"%s%s%s",s,"/",DRV_CFG_NAME);
      fp = fopen(szTmp, "r");
      /* File exist with read permission ? */
      //printf("FULL path :%s fp = %d \n",szTmp, fp);   
      if (fp)
      {
         strcpy(szIniFile, szTmp);
         fclose(fp);      
         return ret;
      }
   }

   /* Config file not found in local path, search in common path */
   sprintf(szTmp,"%s%s%s",DRV_DEFAULT_ENV_PATH,"/",DRV_CFG_NAME);
   fp = fopen(szTmp, "r");
   /* File exist with read permission ? */
   if (fp)
   {
      strcpy(szIniFile, szTmp);
      fclose(fp);
      return ret;
   }
   
   return 0;
}

void RemoveCharFromLine(char *line, char ch)
{
   // remove all occurrences for ch from line
   const char *src = line;
   char *dst = line;
   do
   {
      while (*src == ch)
         ++src;
   }
   while ((*dst++ = *src++) != '\0');
}

void RemoveWhiteSpaceFromLine(char *line)
{
   // remove all occurrences for ch from line
   const char *src = line;
   char *dst = line;
   do
   {
      while (*src == ' ' || *src == '\t')
         ++src;
   }
   while ((*dst++ = *src++) != '\0');
}

void RemoveEndOfLineChars(char *line)
{
   // remove all occurrences for ch from line
   const char *src = line;
   char *dst = line;

   do
   {
      while (*src == '\r' || *src == '\n')
         ++src;
   }
   while ((*dst++ = *src++) != '\0');
}

//-----------------------------------------------------------------------------
// Validate if SectionName exist
//-----------------------------------------------------------------------------
int GetCfgSection(char* lpSectionName,
							   char* lpReturnedString,
							   int nSize,
							   char* lpFileName)
{
	// used mainly to determine if the section exists in the ini file
	// the function reads the key-value pairs that come after the section
	// name and returns the number of characters read.  So, if the section name
	// exists and we're able to read some characters we can assume that
	// it exists. If the buffer is not big enough to hold all the key-value
	// pairs the return value will be equal to nSize minus 2

	FILE *fp = fopen(lpFileName, "r");

	if ( NULL == fp )
		return 0;

	char *in_str = (char *)calloc(nSize, sizeof(char));
	char *section = (char *)calloc(nSize, sizeof(char));

	strcpy(section, "[");
	strcat(section, lpSectionName);

	while (!feof(fp)
		&& !ferror(fp)
		&& fgets(in_str, nSize, fp) != NULL)
	{
		RemoveWhiteSpaceFromLine(in_str);
		if ( 0 == strncmp(in_str, section, strlen(section)) )
		{
			break;
		}
	}
	// now we're in the section we want so we need to read nSize characters
	// (while stripping out the white space)
	int count = 0;
	int remaining = nSize - 1;
	while (!feof(fp)
		&& !ferror(fp)
		&& fgets(in_str, nSize, fp) != NULL)
	{
		RemoveWhiteSpaceFromLine(in_str);
		// if the 1st character of the string is an '[', we've
		// reached a new section
		if ( in_str[0] == '[' )
		   break;
		// ignore blank strings
		if ( strlen(in_str) == 0 )
		   continue;
		// ignore comments
		if ( in_str[0] == ';' )
		   continue;
		strncat(lpReturnedString, in_str, remaining);
		remaining -= strlen(in_str);
		count += strlen(in_str);
		if ( remaining <= 0 )
		   break;
	}
	fclose(fp);
	free(in_str);
	free(section);
	if ( count <= nSize )
	   return count;
	else
	{
	   lpReturnedString[nSize-2] = '\0';
	   lpReturnedString[nSize-1] = '\0';
	   return nSize - 2;
	}
}

//-----------------------------------------------------------------------------
// Get the key string value
//-----------------------------------------------------------------------------
int GetCfgKeyString(char* lpSectionName,
							  char* lpKeyName,
							  char* lpDefault,
							  char* lpReturnedString,
							  int nSize,
							  char* lpFileName)
{
	// if lpSectionName is NULL, we copy all section names to the buffer
	// if lpSectionName is not NULL and lpKeyName is NULL, we copy all key
	// names in that section to the buffer.
	int dwCount = 0;

	char *str = (char *)calloc(nSize, sizeof(char));

	FILE *fp = fopen(lpFileName, "r");

	if ( NULL == fp )
   	   return 0;

	char AllSections = (NULL == lpSectionName) ? 1 : 0;
	char AllKeys = (NULL == lpKeyName) ? 1 : 0;

	char *in_str = (char *)calloc(nSize, sizeof(char));
	char *section = (char *)calloc(nSize, sizeof(char));

	if ( AllSections )
	{
		strcpy(section, "[");
		int count = 0;
		int remaining = nSize - 1;

		while (!feof(fp)
	   			&& !ferror(fp)
				&& fgets(in_str, nSize, fp) != NULL)
		{
			RemoveWhiteSpaceFromLine(in_str);
		   	if ( in_str[0] == '[' )
		   	{
				// remove characters after the trailing ], if any
				char *p = strchr(in_str, ']');
				if ( NULL != ++p )
				{
					if ( 0 != strcmp(p, "\n") )
						strcpy(p, "\n");
				}
				// remove trailing ], if it's there
				RemoveCharFromLine(in_str, '[');
				RemoveCharFromLine(in_str, ']');
				int length = strlen(in_str);
				strncat(str, in_str, remaining);
				count += length;
				remaining -= length;
	   		}
			if ( remaining <= 0 )
				break;
		}
		str[nSize-1] = '\0';
		if ( count < nSize )
		{
			strcpy(lpReturnedString, str);
			dwCount = (int)count;
		}
	   else
	   {
			strcpy(lpReturnedString, str);
	        lpReturnedString[nSize-2] = '\0';
	        lpReturnedString[nSize-1] = '\0';
	        dwCount = (int)nSize - 2;
	   }
	}
	else // we know what section to look for
	{
		strcpy(section, "[");
		strcat(section, lpSectionName);

        while (!feof(fp)
	   			&& !ferror(fp)
				&& fgets(in_str, nSize, fp) != NULL)
		{
	   		RemoveWhiteSpaceFromLine(in_str);
	   		if ( 0 == strncmp(in_str, section, strlen(section)) )
	   		{
				break;
	   		}
		}
		// at this point we should be in the right section
		if ( AllKeys )
		{
			// return all the keys in this section that fit into lpReturned string
			int count = 0;
			int remaining = nSize - 1; // for the '\0' at end
			while (!feof(fp)
					&& !ferror(fp)
					&& fgets(in_str, nSize, fp) != NULL)
			{
				RemoveWhiteSpaceFromLine(in_str);
				// if the 1st character of the string is an '[', we've
				// reached a new section
                if ( in_str[0] == '[' )
					break;
				// ignore blank strings
				if ( strlen(in_str) == 0 )
					continue;
				// ignore line if it's only a line return
				if ( 0 == strcmp(in_str, "") )
					continue;
				// ignore comments
				if ( in_str[0] == ';' )
					continue;

				char *p = strchr(in_str, '=');
				if ( !p )
					continue;
				*p = '\0';
				if ( strlen(in_str) == 0 )
					continue;

				strcat(in_str, "\r\n");

				strncat(str, in_str, remaining);
				remaining -= strlen(in_str);
				count += strlen(in_str);
				if ( remaining <= 0 )
					break;
			}
			str[nSize-1] = '\0';
			if ( count <= nSize )
			{
				strcpy(lpReturnedString, str);
				dwCount = (int)count;
			}
			else
			{
				strcpy(lpReturnedString, str);
				lpReturnedString[nSize-2] = '\0';
				lpReturnedString[nSize-1] = '\0';
				dwCount = (int)nSize - 2;
			}
	   }
	   else // find the key and return the value (or the default value)
	   {
			while (!feof(fp)
				&& !ferror(fp)
				&& fgets(in_str, nSize, fp) != NULL)
			{
				RemoveWhiteSpaceFromLine(in_str);
				// if the 1st character of the string is an '[', we've
				// reached a new section without finding our string, so
				// return the default
				if ( in_str[0] == '[' )
				{
					if ( NULL == lpDefault )
						strcpy(str, "");
					else
		      			strncpy(str, lpDefault, nSize);
				  break;
				}
				// ignore blank strings
				if ( strlen(in_str) == 0 )
					continue;
				// ignore comments
				if ( in_str[0] == ';' )
					continue;
				if ( 0 == strncasecmp(in_str, lpKeyName, strlen(lpKeyName)) )
				{
					// we've found a match
					char *p = strchr(in_str, '=');
					if ( p )
					{
						p++;

						if ( !p )
							strcpy(str, lpDefault);
						else
						{
							// remove line feed and carriage return
							RemoveEndOfLineChars(p);
							strcpy(str, p);
						}
						break;
					}
				}
			}
			strcpy(lpReturnedString, str);
			dwCount = (int)strlen(lpReturnedString);
		}
	}
	fclose(fp);
	free(str);
	free(in_str);
	free(section);
	return dwCount;
}

//-----------------------------------------------------------------------------
// Get the key int value
//-----------------------------------------------------------------------------
int GetCfgKeyInt(char* lpSectionName,
							  const char* lpKeyName,
							  int nDefault,
							  char* lpFileName)	
{   
    char szSectionName[MAX_LINE_LENGTH];
    char buff[MAX_LINE_LENGTH];
    char *szTmp;
    int len = strlen(lpKeyName);
    int i = 0;
    int isOK =0;

    FILE *fp = fopen(lpFileName, "r");
    if( !fp ) return(0);

    sprintf(szSectionName,"[%s]",lpSectionName); 
    /*  Search for SectionName */
    do
    {   
      if (!fgets(buff, MAX_LINE_LENGTH, fp))
      {   
          fclose(fp);
          return(nDefault);
      }
   } while( strncmp(buff,szSectionName, strlen(szSectionName)) );
	
    /* SectionName found. Search for the KeyName */
    do
    {   
      if (!fgets(buff, MAX_LINE_LENGTH, fp) || buff[0] == '\0' )
      {
         fclose(fp);
         return(nDefault);
      }
    }  while( strncasecmp(buff,lpKeyName,len) );

	/* Parse value */
    szTmp = strrchr(buff,'=');    /* equal sign ?*/
    if (szTmp==NULL)
    {
		fclose(fp);	
      return(nDefault);
    }
   szTmp++;

	/* valid digit string */
   for(i = 0; i < strlen(szTmp); i++ )
   {
        if (isdigit(szTmp[i]))
        {
            isOK = 1;
            break;
        }
   }
   
   // minor validation only, atol can take care of space +1/-
   if (!isOK)
	{	
		fclose(fp);	
      return(nDefault);
	}
   
    fclose(fp);                
    return(atol(szTmp));		/* Return the value */   
}
#endif // __linux__







