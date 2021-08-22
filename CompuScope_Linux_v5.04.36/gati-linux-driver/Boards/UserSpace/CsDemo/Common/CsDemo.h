#ifndef __CSDEMO_H__
#define __CSDEMO_H__

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the CsDemo14_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// CsDemo_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#if defined (_WIN32) || defined (CS_WIN64)

	#ifdef CsDemo_EXPORTS
		#define CsDemo_API __declspec(dllexport)
	#else
		#define CsDemo_API __declspec(dllimport)
	#endif

#elif  defined __linux__

	void __attribute__((constructor)) init_Demo(void);
//	void __attribute__ ((destructor)) cleanup_Demo(void);

#else

	int32 DLL_Initialize();
	void DLL_CleanUp();

#endif

#endif // __CSDEMO_H__
