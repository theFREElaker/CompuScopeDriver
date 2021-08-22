/* gen_kdebug.h - Generic Kernel debug header file  */

/* $Id$ */

/*
 * Modifications History:
 * ----------------------
 */

/*
 * DESCRIPTION:
 *
 * This file includes macros to help print debug information.
 *
 */

#ifndef gen_kdebug_h
#define gen_kdebug_h

#define		KDEBUG_LEVEL	KERN_INFO

/* Dependencies */

#include "gen_types.h"       /* Generic types definition */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Constants */

/* Macros */



#ifdef GEN_LINUX

char *basename (char *path);

#define GEN_PRINT printk

#define GEN_ERROR_PRINT(fmt, args...)                               \
    printk (KDEBUG_LEVEL "Cs14105Drv %s: " fmt, __PRETTY_FUNCTION__ , ## args)

#undef  GEN_DEBUG_PRINT      /* undef it, just in case */

#ifdef  GEN_DEBUG

#ifdef GEN_DEBUG_PRINT_LONG

#define GEN_DEBUG_PRINT(fmt, args...)                                                \
            printk (KDEBUG_LEVEL "[%s:%d] %s: " fmt, basename (__FILE__), __LINE__ , \
                    __PRETTY_FUNCTION__ , ## args)

#elif defined GEN_DEBUG_PRINT_MEDIUM  // not GEN_DEBUG_PRINT_LONG

#define GEN_DEBUG_PRINT(fmt, args...)                              \
    printk (KDEBUG_LEVEL "%s: " fmt, __PRETTY_FUNCTION__ , ## args)

#else   // not GEN_DEBUG_PRINT_MEDIUM

#define GEN_DEBUG_PRINT(fmt, args...) \
    printk (KDEBUG_LEVEL fmt, ## args)
#endif  // GEN_DEBUG_PRINT_MEDIUM GEN_DEBUG_PRINT_LONG



#define GEN_SHORT_DEBUG_PRINT(fmt, args...) \
    printk (KDEBUG_LEVEL fmt, ## args)

#define GEN_MEDIUM_DEBUG_PRINT(fmt, args...) \
    printk (KDEBUG_LEVEL "%s: " fmt, __PRETTY_FUNCTION__ , ## args)

#define GEN_LONG_DEBUG_PRINT(fmt, args...)                                           \
            printk (KDEBUG_LEVEL "[%s:%d] %s: " fmt, basename (__FILE__), __LINE__ , \
                    __PRETTY_FUNCTION__ , ## args)

#else   // not GEN_DEBUG

#define GEN_DEBUG_PRINT(fmt, args...)
#define GEN_SHORT_DEBUG_PRINT(fmt, args...)
#define GEN_MEDIUM_DEBUG_PRINT(fmt, args...)
#define GEN_LONG_DEBUG_PRINT(fmt, args...)

#endif  // GEN_DEBUG

#define GEN_GOT_HERE GEN_LONG_DEBUG_PRINT ("***** GOT HERE *****\n")

#ifdef GEN_TRACE
#define TRACEIN  printk (KDEBUG_LEVEL "[%s:%d] ENTERING %s: \n", basename (__FILE__), __LINE__ , __PRETTY_FUNCTION__)
#define TRACEOUT printk (KDEBUG_LEVEL "[%s:%d] LEAVING  %s: \n", basename (__FILE__), __LINE__ , __PRETTY_FUNCTION__)
#else  // GEN_TRACE
#define TRACEIN
#define TRACEOUT
#endif // GEN_TRACE


#undef  GEN_ASSERT      /* undef it, just in case */
#ifdef  GEN_DEBUG
#define GEN_ASSERT(condition) \
    if (!(condition)) GEN_LONG_DEBUG_PRINT ( "*** ASSERTION FAILED: ["#condition"] ***\n" )
#else   // GEN_DEBUG
#define GEN_ASSERT(condition)
#endif  // GEN_DEBUG

#endif /* GEN_LINUX */


#ifdef GEN_WINDOWS_2K_KERNEL_MODE

#undef  GEN_DEBUG_PRINT      /* undef it, just in case */

#undef  GEN_ASSERT      /* undef it, just in case */
#define GEN_ASSERT(condition) \
    ASSERT( !condition )

#endif /*GEN_WINDOWS_2K_KERNEL_MODE */

/* Type Definitions */

/* Item structure to by use by Add and delete */

/* Global Variables */

/* Prototypes */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* gen_kdebug_h */

/* End of gen_kdebug.h */
