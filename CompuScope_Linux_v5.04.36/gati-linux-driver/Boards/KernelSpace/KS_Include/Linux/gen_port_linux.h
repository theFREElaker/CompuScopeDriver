/*--------------------------------------------------------------------------*
 *
 * File:        gen_port_linux.h
 *
 * Description: - Generic definitions, Linux version.
 *
 *--------------------------------------------------------------------------*
 *
 * Log:
 *    [date]        [username]    [comments]
 *
 *
 *--------------------------------------------------------------------------*
 * $Id$
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/

#ifndef GENPORT_LINUX_H
#define GENPORT_LINUX_H

/*--------------------------------------------------------------------------*
 * C++ Support
 *--------------------------------------------------------------------------*/

#if     defined(__cplusplus)
extern  "C"
{
#endif  // __cplusplus

/*--------------------------------------------------------------------------*
 * Global Includes
 *--------------------------------------------------------------------------*/

#ifdef GEN_KERNEL_MODE
#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netdb.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

/*--------------------------------------------------------------------------*
 * Global Defines
 *--------------------------------------------------------------------------*/

//#define GEN_NEED_GETTIMEOFDAY
//#define GEN_NEED_STRDUP
//#define GEN_NEED_STRCASECMP
//#define GEN_NEED_GETOPT
//#define GEN_NEED_SLEEP
//#define GEN_NEED_SNPRINTF

/*--------------------------------------------------------------------------*
 * Global Macros
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*
 * Global Typedefs
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------*/

#undef  GEN_INLINE
#define GEN_INLINE  static __inline__

/*--------------------------------------------------------------------------*
 * Global Prototypes
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*
 * Global Variables
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*
 * Global Functions
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*
 *
 * Function:    ~
 *
 * Description: ~
 *
 * Input:       ~
 *
 * Output:      ~
 *
 * Return:      ~
 *
 *--------------------------------------------------------------------------*/
//inline

/*--------------------------------------------------------------------------*
 * Tick Port
 *--------------------------------------------------------------------------*/

#ifdef GEN_KERNEL_MODE
#else
GEN_INLINE long genGetTick() {
    struct timeval tv;
    timerclear(&tv);
    if (gettimeofday(&tv,NULL)) {
        return (long) ((long) tv.tv_sec + (long) tv.tv_usec);
    } else {
        return 0;
    }
}
#endif

/*--------------------------------------------------------------------------*
 * Stdarg Port
 *--------------------------------------------------------------------------*/
// Just to be coherent
// In Windows, it is _vsnprintf instead of vsnprintf

/*--------------------------------------------------------------------------*
 * Socket Port
 *--------------------------------------------------------------------------*/

#ifdef GEN_KERNEL_MODE
#else
typedef int       GEN_SOCKET;
typedef socklen_t GEN_SOCKLEN;

#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1

GEN_INLINE int  genSock_Init() {return 0;}

GEN_INLINE void genSock_Cleanup() {}

GEN_INLINE void genSock_Destroy(GEN_SOCKET sock) { close(sock);}

GEN_INLINE GEN_STATUS genSock_GetName(GEN_SOCKET sock, struct sockaddr *psa) {
    socklen_t salen;
    salen = sizeof(struct sockaddr);
    memset((void *) psa, 0, salen);
    if (getsockname(sock, psa, &salen) == 0) return GEN_OK; else return GEN_ERR;
}

GEN_INLINE GEN_STATUS genSock_SetSocketNonBlockingMode(GEN_SOCKET s) {

    if (fcntl(s,F_SETFL, O_NONBLOCK) != -1) return GEN_OK; else return GEN_ERR;
}

GEN_INLINE int genSock_GetLastError() { return errno;}
#endif

/*--------------------------------------------------------------------------*
 * Mutex Port
 *--------------------------------------------------------------------------*/

#ifdef GEN_KERNEL_MODE
#else
typedef pthread_mutex_t  GEN_MUTEX;
typedef GEN_MUTEX       *GEN_PMUTEX;

GEN_INLINE void _genMutex_Init ( GEN_PMUTEX    pMutex)
{
    pthread_mutex_t _y = PTHREAD_MUTEX_INITIALIZER;

    memcpy(pMutex,&_y,sizeof(*pMutex));
}

#define genMutex_Init(x)   _genMutex_Init((x))
#define genMutex_Lock(x)   pthread_mutex_lock((x))
#define genMutex_UnLock(x) pthread_mutex_unlock((x))
#endif

/*--------------------------------------------------------------------------*
 * Threads Port
 *--------------------------------------------------------------------------*/

#ifdef GEN_KERNEL_MODE
#else
typedef pthread_t   GEN_THREAD;
typedef GEN_THREAD *GEN_PTHREAD;

GEN_INLINE void genThread_Create
(
    GEN_PINT8   pName,
    GEN_PTHREAD pThread,
    GEN_UINT32  Priority,
    GEN_PVOID   (*pFunction)(GEN_PVOID pParms),
    GEN_PVOID   pParms
)
{
    pthread_create(pThread,NULL,pFunction,pParms);
}

#define genThread_Exit() pthread_exit(NULL);

GEN_INLINE GEN_STATUS genThread_Destroy(GEN_PTHREAD pthread){ return GEN_OK;}
#endif

/*--------------------------------------------------------------------------*
 * C++ Support
 *--------------------------------------------------------------------------*/
#if        defined(__cplusplus)
}
#endif  // __cplusplus

#endif  // GENPORT_LINUX_H
