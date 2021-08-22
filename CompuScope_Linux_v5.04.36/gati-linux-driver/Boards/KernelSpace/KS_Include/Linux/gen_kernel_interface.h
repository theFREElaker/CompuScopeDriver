/*E-----------------------------------------------------------------------------
  Project       :
  Module        : $RCSfile$
  Revision      : $Revision$
  Date          : $Date$
  Checked in by : $Author$
  Description   : Kernel Interface header file
*/

/*------------------------------------------------------------------------------
  This header file defines the macros to provide a common interface to the
  Linux, Solaris, VxWorks and Windows kernel functions.
------------------------------------------------------------------------------*/

#ifndef gen_kernel_interface_H
#define gen_kernel_interface_H

/******************************************************************************/
/*============================================================================*/
/*                               C O M M O N                                  */
/*============================================================================*/
/******************************************************************************/

/*============================================================================*/
/*                         I N C L U D E   F I L E S                          */
/*============================================================================*/

#include "gen_lang.h"

#ifndef GEN_WINDOWS_2K_KERNEL_MODE
#include "gen_types.h"
#include "gen_port.h"
#endif


EXTERN_START

/*============================================================================*/
/*                                M A C R O S                                 */
/*============================================================================*/






/******************************************************************************/
/*============================================================================*/
/*                                L I N U X                                   */
/*============================================================================*/
/******************************************************************************/

#ifdef GEN_LINUX
#ifdef __KERNEL__

/*============================================================================*/
/*                         I N C L U D E   F I L E S                          */
/*============================================================================*/

#include <asm/io.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,2,18)
#include <asm/spinlock.h>
#include <asm/semaphore.h>
#else
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#endif

/*============================================================================*/
/*                                M A C R O S                                 */
/*============================================================================*/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,2,18)

typedef struct wait_queue  wait_queue_t;
typedef struct wait_queue *wait_queue_head_t;

#define init_waitqueue_head(x)    *(x)=NULL
#define init_waitqueue_entry(q,p) ((q)->task)=(p)

#endif

/*---------------------------------------------------*/
/* For function call compatibility only              */
/*---------------------------------------------------*/

#define GEN_DMA_SAFE  0

/*------------*/
/* Semaphores */
/*------------*/

typedef struct semaphore  GEN_SEMAPHORE;
typedef void             *GEN_SEM_ID;

/*-----------------*/
/* Ethernet Buffer */
/*-----------------*/

typedef int             GEN_NET_POOL_ID;
typedef struct sk_buff *GEN_ETH_BUF;

/*---------------------------------------------------------*/
/* Get an ethernet buffer specified by the size parameter, */
/* netPool and BoardIndex parameters are dummy on linux.   */
/*---------------------------------------------------------*/

#define GEN_GET_ETHERNET_BUFFER(_size_, _netPool, _BoardIndex) \
    dev_alloc_skb((_size_) + 2)

/*------------------------------------------------------------------*/
/* freeing the transmitted pSkb we can use this _irq version of     */
/* dev_kfree_skb cause we know for sure that we are in an interrupt */
/* context                                                          */
/*------------------------------------------------------------------*/

#define GEN_FREE_ETH_BUFFER(_buffer_) \
        dev_kfree_skb_irq(_buffer_)

#define GEN_ETHERNET_VIRT_TO_PHYS(_buffer_)    \
        GEN_ILF_EthernetVirtToPhys((_buffer_))


/*---------------------------------------------------------*/
/* Atomic code sections                                    */
/*                                                         */
/* These are section of code which must not be interrupted */
/* or subject to context switches                          */
/*---------------------------------------------------------*/

typedef spinlock_t     GEN_ATOMIC;
typedef unsigned long  GEN_ATOMIC_FLAGS;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
#define GEN_AtomicInit(_GEN_Atomic_) \
        spin_lock_init (&(_GEN_Atomic_))
#else
#define GEN_AtomicInit(_GEN_Atomic_) \
        (void) (&(_GEN_Atomic_))  /* prevent warnings compiling for 2.2 */
#endif

#define GEN_StartAtomicCodeSection(_GEN_Atomic_, _Gen_AtomicFlags_) \
        spin_lock_irqsave (&(_GEN_Atomic_), (_Gen_AtomicFlags_))

#define GEN_EndAtomicCodeSection(_GEN_Atomic_, _Gen_AtomicFlags_) \
        spin_unlock_irqrestore (&(_GEN_Atomic_), (_Gen_AtomicFlags_))

/*============================================================================*/
/*                 S T R U C T U R E   D E F I N I T I O N S                  */
/*============================================================================*/

/*============================================================================*/
/*                     F U N C T I O N   P R O T Y P E S                      */
/*============================================================================*/

/*============================================================================*/
/*           E X T E R N A L    D A T A   D E C L A R A T I O N S             */
/*============================================================================*/

/*============================================================================*/
/*                      I N L I N E   F U N C T I O N S                       */
/*============================================================================*/

/*--------------------------------------*/
/*  Virtual to Physical Memory Mapping  */
/*--------------------------------------*/

/*---------------------------------------------*/
/* Convert virtual address to physical address */
/*---------------------------------------------*/
/* Inline function Instead ????????????????/   */

#define GEN_VIRT_TO_PHYS(_address_) \
    ((_address_ != 0) ? ((GEN_UINT32) virt_to_bus ((void*) (_address_))) : 0)

/*---------------------------------------------*/
/* Convert physical address to virtual address */
/*---------------------------------------------*/

#define GEN_PHYS_TO_VIRT(_address_) \
    ((_address_ != 0) ? ((void *) bus_to_virt ((int) (_address_))) : 0)


GEN_INLINE GEN_UINT32 GEN_ILF_EthernetVirtToPhys
(
    GEN_ETH_BUF buffer
)
{
    return (GEN_VIRT_TO_PHYS ( (void *) buffer->data));
}


/*----------------------------------------*/
/* Cache management (flush - invalidate)  */
/*----------------------------------------*/

#define GEN_CACHE_FLUSH(_address_, _size_)
#define GEN_CACHE_INVALIDATE(_address_, _size_)

/*-------------------*/
/* Memory Management */
/*-------------------*/

/*---------------------------------*/
/* Allocate kernel memory (malloc) */
/*---------------------------------*/

GEN_INLINE void *GEN_ALLOCATE (size_t size, GEN_INT32 flags)
{
    return (kmalloc (size, flags));
}

/*----------------------------------------------*/
/* Allocate kernel memory aligned (malloc)      */
/*                                              */
/* On Linux this is the same as GEN_ALLOCATE    */
/* since Linux does not have aligned allocation */
/*----------------------------------------------*/

GEN_INLINE void *GEN_ALLOCATE_ALIGNED
                 (size_t size, GEN_INT32 flags, size_t align, void **palloc)
{
    if (palloc == NULL) return (NULL);

    if (align < 1) align = 1;
    *palloc = kmalloc (size + align - 1, flags);
    if ((size_t) *palloc % align == 0)
        return (*palloc);
    else
        return (*palloc + align - ((size_t) *palloc % align));
}

/*-------------------------------------------*/
/* Allocate and clear kernel memory (calloc) */
/*-------------------------------------------*/

GEN_INLINE void *GEN_CALLOCATE
                 (size_t elem_num, size_t elem_size, GEN_INT32 flags)
{
    void *memptr;

    if ((memptr = kmalloc (elem_num * elem_size, flags)) == NULL) return (NULL);

    memset (memptr, 0, elem_num * elem_size);
    return (memptr);
}

/*---------------------------------------------------*/
/* Allocate and clear kernel memory aligned (calloc) */
/*                                                   */
/* On Linux this is the same as GEN_ALLOCATE         */
/* since Linux does not have aligned allocation      */
/*---------------------------------------------------*/

GEN_INLINE void *GEN_CALLOCATE_ALIGNED
     (size_t elem_num, size_t elem_size, GEN_INT32 flags, size_t align, void **palloc)
{
    if (palloc == NULL) return (NULL);

    if (align < 1) align = 1;
    if ((*palloc = kmalloc ((elem_num * elem_size) + align - 1, flags)) == NULL)
        return (NULL);

    memset (*palloc, 0, (elem_num * elem_size) + align - 1);
    if ((size_t) *palloc % align == 0)
        return (*palloc);
    else
        return (*palloc + align - ((size_t) *palloc % align));
}

/*--------------------*/
/* Free kernel memory */
/*--------------------*/

GEN_INLINE void  GEN_FREE (void *memptr)
{
    if (memptr) kfree (memptr);
}

GEN_INLINE void GEN_FREE_DMA_SAFE (void *memptr)
{
    if (memptr) kfree (memptr);
}

/*------------*/
/* Semaphores */
/*------------*/

/*-------------------------------------*/
/* Create a mutual exclusion semaphore */
/*-------------------------------------*/

/* init_MUTEX was not defined prior to kernel 2.2.18 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,2,18)
#define init_MUTEX(x) *(x)=MUTEX
#elif LINUX_VERSION_CODE > KERNEL_VERSION(2,6,37)
#define init_MUTEX(x) sema_init(x, 1)
#endif

GEN_INLINE GEN_SEM_ID GEN_CREATE_MX_SEMAPHORE
(
    GEN_SEMAPHORE *semName          /* Address of the semaphore struture */
)
{
    if (semName == NULL) return ((GEN_SEM_ID) !NULL);

    init_MUTEX (semName);           /* can't fail, so ... */
    return ((GEN_SEM_ID) !NULL);    /* return any value other than NULL */
}

/*----------------------------------------------------------------*/
/* Delete a semaphore                                             */
/*                                                                */
/* This is a dummy function since there is no such thing on Linux */
/*----------------------------------------------------------------*/

GEN_INLINE GEN_STATUS GEN_DELETE_SEMAPHORE
(
    GEN_SEMAPHORE *semName          /* Address of the semaphore struture */
)
{
    return (GEN_OK);                /* There is no semaphore delete on Linux */
}

/*---------------------*/
/* Request a semaphore */
/*---------------------*/

GEN_INLINE GEN_STATUS GEN_REQUEST_SEMAPHORE
(
    GEN_SEMAPHORE *semName          /* Address of the semaphore struture */
)
{
    if (semName == NULL) return (GEN_ERR);

    return ((down_interruptible (semName) == 0) ? GEN_OK : GEN_ERR);
}

/*------------------------------------*/
/* Request a semaphore with a timeout */
/*------------------------------------*/

#define GEN_REQUEST_SEMAPHORE_WITH_TIMEOUT(semName, timeout) \
ERROR - Semaphore requests on Linux do not have timeouts

/*---------------------*/
/* Release a semaphore */
/*---------------------*/

GEN_INLINE GEN_STATUS GEN_RELEASE_SEMAPHORE
(
    GEN_SEMAPHORE *semName          /* Address of the semaphore struture */
)
{
    if (semName == NULL) return (GEN_ERR);

    up (semName);
    return (GEN_OK);
}

//init_waitqueue_head((wait struct *)
//interruptible_sleep_on_timeout(wait struct *, long timeout)
//wake_up_interruptible(wait struct *)
//wait_event_interruptible(wait struct *, int condition)

#endif /* __KERNEL__ */
#endif /* GEN_LINUX */




/******************************************************************************/
/*============================================================================*/
/*                               V X W O R K S                                */
/*============================================================================*/
/******************************************************************************/

#ifdef GEN_VXWORKS

/*============================================================================*/
/*                         I N C L U D E   F I L E S                          */
/*============================================================================*/

#include <string.h>
#include <taskLib.h>
#include <intLib.h>
#include <cacheLib.h>
#include <net/mbuf.h>

/*============================================================================*/
/*                                M A C R O S                                 */
/*============================================================================*/

/*---------------------------------------------------------*/
/*  Virtual to Physical Memory Mapping (empty for vxWorks) */
/*---------------------------------------------------------*/

#define GEN_VIRT_TO_PHYS(_address_) (_address_)
#define GEN_PHYS_TO_VIRT(_address_) (_address_)


/*---------------------------------------*/
/* Cache management (flush - invalidate) */
/*---------------------------------------*/

#define GEN_CACHE_FLUSH(_address_, _size_) \
    cacheFlush (DATA_CACHE, (void *)(_address_), (_size_));

#define GEN_CACHE_INVALIDATE(_address_, _size_) \
    cacheInvalidate (DATA_CACHE, (void *)(_address_), (_size_));

/*---------------------------------------------------*/
/* Linux Memory Management symbols for kmalloc flags */
/*                                                   */
/* For function call compatibility only              */
/*---------------------------------------------------*/

#define GFP_BUFFER    0
#define GFP_ATOMIC    0
#define GFP_USER      0
#define GFP_HIGHUSER  0
#define GFP_KERNEL    0
#define GFP_NFS       0
#define GFP_KSWAPD    0
#define GEN_DMA_SAFE  0

/*------------*/
/* Semaphores */
/*------------*/

typedef GEN_SEM_ID GEN_SEMAPHORE;

/*-----------------*/
/* Ethernet Buffer */
/*-----------------*/

typedef NET_POOL_ID     GEN_NET_POOL_ID;
typedef M_BLK_ID        GEN_ETH_BUF;

#define GEN_GET_ETHERNET_BUFFER(_size_, _netPool, _BoardIndex) \
    GEN_ILF_GetEthernetBuffer((_size_),(_netPool))

#define GEN_FREE_ETH_BUFFER(_buffer_) \
        netMblkClChainFree((_buffer_))

#define GEN_ETHERNET_VIRT_TO_PHYS(_buffer_) \
            GEN_ILF_EthernetVirtToPhys ((_buffer_))

GEN_INLINE void * GEN_ILF_EthernetVirtToPhys
(
    GEN_ETH_BUF buffer
)
{
    return (buffer->mBlkHdr.mData);
}

/*-------------------------------------------------------------------------*/
/* Atomic code sections                                                    */
/*                                                                         */
/* These are section of code which must not be interrupted or subject to   */
/* context switches. They are implemented as macros for compatability with */
/* the Linux calling sequence.                                             */
/* They call inline functions to avoid side effects.                       */
/* Note that GEN_AtomInit is empty on VxWorks.                             */
/*-------------------------------------------------------------------------*/

typedef int GEN_ATOMIC;             /* Dummy on VxWorks */
typedef int GEN_ATOMIC_FLAGS;

#define GEN_AtomicInit(_GEN_Atomic_) \
        _GEN_Atomic_ = 0;           /* to avoid unused variable warning */

#define GEN_StartAtomicCodeSection(_GEN_Atomic_, _Gen_AtomicFlags_) \
        _Gen_AtomicFlags_ = GEN_ILF_StartAtomicCodeSection ()

#define GEN_EndAtomicCodeSection(_GEN_Atomic_, _Gen_AtomicFlags_) \
        GEN_ILF_EndAtomicCodeSection (_Gen_AtomicFlags_)

/*============================================================================*/
/*                 S T R U C T U R E   D E F I N I T I O N S                  */
/*============================================================================*/

/*============================================================================*/
/*                     F U N C T I O N   P R O T Y P E S                      */
/*============================================================================*/

/*============================================================================*/
/*           E X T E R N A L    D A T A   D E C L A R A T I O N S             */
/*============================================================================*/

/*============================================================================*/
/*                      I N L I N E   F U N C T I O N S                       */
/*============================================================================*/

/*--------------------------------------------*/
/* Ethernet Buffer management                 */
/*--------------------------------------------*/

GEN_ETH_BUF GEN_INLINE  GEN_ILF_GetEthernetBuffer (GEN_INT32 size,
                                                   NET_POOL_ID netPool)
{
     return (netTupleGet (netPool, size, M_DONTWAIT, MT_DATA, TRUE));
}



/*--------------------------------------------*/
/* Memory Management                          */
/*                                            */
/* The flags parameter is not used on VxWorks */
/*--------------------------------------------*/



/*---------------------------------*/
/* Allocate kernel memory (malloc) */
/*---------------------------------*/

GEN_INLINE void *GEN_ALLOCATE (size_t size, GEN_INT32 flags)
{
    return (malloc (size));
}

 /*----------------------------------------------*/
 /* Allocate kernel memory aligned (malloc)      */
 /*                                              */
 /* On Linux this is the same as GEN_ALLOCATE    */
 /* since Linux does not have aligned allocation */
 /*----------------------------------------------*/

GEN_INLINE void *GEN_ALLOCATE_ALIGNED
                 (size_t size, GEN_INT32 flags, size_t align, void **palloc)
{
    register void *memptr;

    memptr = memalign (align, size);
    if (palloc != NULL) *palloc = memptr;
    return (memptr);
}

/*-------------------------------------------*/
/* Allocate and clear kernel memory (calloc) */
/*-------------------------------------------*/

GEN_INLINE void *GEN_CALLOCATE
                 (size_t elem_num, size_t elem_size, GEN_INT32 flags)
{
    return (calloc (elem_num, elem_size));
}

 /*---------------------------------------------------*/
 /* Allocate and clear kernel memory aligned (calloc) */
 /*                                                   */
 /* On Linux this is the same as GEN_ALLOCATE         */
 /* since Linux does not have aligned allocation      */
 /*---------------------------------------------------*/

GEN_INLINE void *GEN_CALLOCATE_ALIGNED
     (size_t elem_num, size_t elem_size, GEN_INT32 flags, size_t align, void **palloc)
{
    void *memptr;

    if ((memptr = memalign (align, elem_num * elem_size)) == NULL)return (NULL);

    if (palloc != NULL) *palloc = memptr;
    memset (memptr, 0, elem_num * elem_size);
    return (memptr);
}

/*--------------------*/
/* Free kernel memory */
/*--------------------*/

GEN_INLINE void  GEN_FREE (void *memptr)
{
    if (memptr) free (memptr);
}

GEN_INLINE void  GEN_FREE_DMA_SAFE (void *memptr)
{
    if (memptr) free (memptr);
}

/*------------*/
/* Semaphores */
/*------------*/

/*-------------------------------------*/
/* Create a mutual exclusion semaphore */
/*-------------------------------------*/

GEN_INLINE GEN_SEM_ID GEN_CREATE_MX_SEMAPHORE
(
    GEN_SEMAPHORE    *semId         /* Add of variable to hold semaphore ID */
)
{
    return (*semId = semMCreate (SEM_Q_PRIORITY | SEM_INVERSION_SAFE));
}

/*--------------------*/
/* Delete a semaphore */
/*--------------------*/

GEN_INLINE GEN_STATUS GEN_DELETE_SEMAPHORE
(
    GEN_SEMAPHORE *semId            /* ID of the semaphore to request */
)
{
    return ((semDelete (*semId) == OK) ? GEN_OK : GEN_ERR);
}

/*---------------------*/
/* Request a semaphore */
/*---------------------*/

GEN_INLINE GEN_STATUS GEN_REQUEST_SEMAPHORE
(
    GEN_SEMAPHORE *semId            /* ID of the semaphore to request */
)
{
    return ((semTake (*semId, WAIT_FOREVER) == OK) ? GEN_OK : GEN_ERR);
}

/*------------------------------------*/
/* Request a semaphore with a timeout */
/*------------------------------------*/

GEN_INLINE GEN_STATUS GEN_REQUEST_SEMAPHORE_WITH_TIMEOUT
(
    GEN_SEMAPHORE *semId,           /* ID of the semaphore to request */
    GEN_INT32      timeout          /* timeout value */
)
{
    return ((semTake (*semId, timeout) == OK) ? GEN_OK : GEN_ERR);
}

/*---------------------*/
/* Release a semaphore */
/*---------------------*/

GEN_INLINE GEN_STATUS GEN_RELEASE_SEMAPHORE
(
    GEN_SEMAPHORE *semId            /* ID of the semaphore to release */
)
{
    return ((semGive (*semId) == OK) ? GEN_OK : GEN_ERR);
}

/*-----------------------------------------------------------------------*/
/* Atomic code sections                                                  */
/*                                                                       */
/* These are section of code which must not be interrupted or subject to */
/* context switches. These are the inline functions called by the macros */
/* defined above.                                                        */
/*-----------------------------------------------------------------------*/

GEN_INLINE GEN_ATOMIC_FLAGS GEN_ILF_StartAtomicCodeSection (void)
{
    taskLock ();
    return (intLock ());
}

GEN_INLINE void GEN_ILF_EndAtomicCodeSection (GEN_ATOMIC_FLAGS flags)
{
    intUnlock (flags);
    taskUnlock ();
}

#endif /* GEN_VXWORKS */




/******************************************************************************/
/*============================================================================*/
/*                         W I N D O W S  2 0 0 0                             */
/*============================================================================*/
/******************************************************************************/

#ifdef GEN_WINDOWS_2K_KERNEL_MODE

/*============================================================================*/
/*                         I N C L U D E   F I L E S                          */
/*============================================================================*/

#include "miniportNdis.h"     /* Generic NDIS driver header file */
#include "DevQueue.h"

/*============================================================================*/
/*                                M A C R O S                                 */
/*============================================================================*/

/*---------------------------------------------------*/
/* Linux Memory Management symbols                   */
/*                                                   */
/* For function call compatibility only              */
/*---------------------------------------------------*/

#define GFP_BUFFER    0
#define GFP_ATOMIC    0
#define GFP_USER      0
#define GFP_HIGHUSER  0
#define GFP_KERNEL    0
#define GFP_NFS       0
#define GFP_KSWAPD    0
#define GEN_DMA_SAFE 20

/*------------*/
/* Semaphores */
/*------------*/

typedef FAST_MUTEX  GEN_SEMAPHORE;
typedef PFAST_MUTEX GEN_PSEMAPHORE;
typedef GEN_PVOID   GEN_SEM_ID;

/*-----------------*/
/* Ethernet Buffer */
/*-----------------*/

typedef GEN_INT32 GEN_NET_POOL_ID;
typedef PRX_FRAME_DESCRIPTOR GEN_ETH_BUF;

/*---------------------------------------------------------*/
/* Get an ethernet buffer specified by the BoardIndex      */
/* parameter, netPool parameter is dummy on windows.       */
/*---------------------------------------------------------*/

#define GEN_GET_ETHERNET_BUFFER(_size_, _netPool, _BoardIndex) \
    GEN_ILF_GetEthernetBuffer((_size_),(_BoardIndex))

#define GEN_ETHERNET_VIRT_TO_PHYS(_buffer_)    \
        GEN_ILF_EthernetVirtToPhys((_buffer_))

/*-----------------*/
/* empty for now...*/
/*-----------------*/

#define GEN_FREE_ETH_BUFFER(_buffer_) (_buffer_)
#if 0
// freeing the GEN_ETH_BUF.
#define GEN_FREE_ETH_BUFFER(_buffer_) \
        InitAndChainPacket(_buffer_)

#endif

/*---------------------------------------------------------*/
/* Atomic code sections                                    */
/*                                                         */
/* These are section of code which must not be interrupted */
/* or subject to context switches                          */
/*---------------------------------------------------------*/

typedef KSPIN_LOCK     GEN_ATOMIC;
typedef KIRQL          GEN_ATOMIC_FLAGS;
typedef PKSPIN_LOCK    GEN_PATOMIC;
typedef PKIRQL         GEN_PATOMIC_FLAGS;

#define GEN_AtomicInit(_GEN_Atomic_)      \
        GEN_ILF_AtomicInit(&_GEN_Atomic_)

#define GEN_StartAtomicCodeSection(_GEN_Atomic_, _GEN_AtomicFlags_)       \
        GEN_ILF_StartAtomicCodeSection(&_GEN_Atomic_, &_GEN_AtomicFlags_)

#define GEN_EndAtomicCodeSection(_GEN_Atomic_, _GEN_AtomicFlags_)      \
        GEN_ILF_EndAtomicCodeSection(&_GEN_Atomic_, _GEN_AtomicFlags_)

/*============================================================================*/
/*                 S T R U C T U R E   D E F I N I T I O N S                  */
/*============================================================================*/

/*============================================================================*/
/*                     F U N C T I O N   P R O T Y P E S                      */
/*============================================================================*/

// prototypes in ntddk.h and functions definition are in ntoskrnl.lib
//
NTKERNELAPI
PHYSICAL_ADDRESS
MmGetPhysicalAddress (
    IN PVOID BaseAddress
);

PVOID
MmAllocateContiguousMemory(
IN ULONG NumberOfBytes,
IN PHYSICAL_ADDRESS HighestAcceptableAddress
);

VOID
  MmFreeContiguousMemory(
  IN PVOID BaseAddress
);

/*============================================================================*/
/*           E X T E R N A L    D A T A   D E C L A R A T I O N S             */
/*============================================================================*/

/*============================================================================*/
/*                      I N L I N E   F U N C T I O N S                       */
/*============================================================================*/

/*--------------------------------------------*/
/* Ethernet Buffer management                 */
/*--------------------------------------------*/

GEN_INLINE GEN_ETH_BUF GEN_ILF_GetEthernetBuffer
(
GEN_INT32 size,
GEN_UINT8 nicIndex
)
{
    return(genTargetNic_GetRxDescriptor(nicIndex, size));
}

/*--------------------------------------*/
/*  Virtual to Physical Memory Mapping  */
/*--------------------------------------*/

/*---------------------------------------------*/
/* Convert virtual address to physical address */
/*---------------------------------------------*/

#define GEN_VIRT_TO_PHYS(_address_) \
    ((_address_ != 0) ?             \
    (GEN_UINT32) (MmGetPhysicalAddress ((GEN_PVOID) _address_)).LowPart : 0)

GEN_INLINE GEN_UINT32 GEN_ILF_EthernetVirtToPhys
(
    GEN_ETH_BUF buffer
)
{
    return (buffer->FrameBufferPhys);
}

/*---------------------------------------------*/
/* Convert physical address to virtual address */
/*---------------------------------------------*/

#define GEN_PHYS_TO_VIRT(_address_) (_address_)

/*----------------------------------------*/
/* Cache management (flush - invalidate)  */
/*----------------------------------------*/

#define GEN_CACHE_FLUSH(_address_, _size_)
#define GEN_CACHE_INVALIDATE(_address_, _size_)

/*-------------------*/
/* Memory Management */
/*-------------------*/

/*---------------------------------*/
/* Allocate kernel memory (malloc) */
/*---------------------------------*/

GEN_INLINE GEN_PVOID GEN_ALLOCATE (size_t size, GEN_INT32 flags)
{
    return(ExAllocatePool(NonPagedPool, size));
}

/*----------------------------------------------*/
/* Allocate kernel memory aligned (malloc)      */
/*----------------------------------------------*/

GEN_INLINE GEN_PVOID GEN_ALLOCATE_ALIGNED
           (size_t size, GEN_INT32 flags, size_t align, GEN_PVOID *palloc)
{
    PHYSICAL_ADDRESS  PhysicalAddress;

    NdisSetPhysicalAddressLow(PhysicalAddress, -1);

    if (palloc == NULL)
        return (NULL);

    if (align < 1)
        align = 1;

    if((flags & GEN_DMA_SAFE) == GEN_DMA_SAFE)
        *palloc = MmAllocateContiguousMemory(size + align - 1, PhysicalAddress);
    else
        *palloc = ExAllocatePool(NonPagedPoolCacheAligned, size + align - 1);

    return (GEN_PVOID)((GEN_PUINT8)*palloc + ((size_t) *palloc % align));
}

/*-------------------------------------------*/
/* Allocate and clear kernel memory (calloc) */
/*-------------------------------------------*/

GEN_INLINE GEN_PVOID GEN_CALLOCATE
                           (size_t elem_num, size_t elem_size, GEN_INT32 flags)
{
    GEN_PVOID memptr;

  if ((memptr = ExAllocatePool(NonPagedPool, elem_num * elem_size)) == NULL)
        return (NULL);
    memset (memptr, 0, elem_num * elem_size);
    return (memptr);
}

/*---------------------------------------------------*/
/* Allocate and clear kernel memory aligned (calloc) */
/*---------------------------------------------------*/

GEN_INLINE GEN_PVOID GEN_CALLOCATE_ALIGNED
   (size_t elem_num, size_t elem_size, GEN_INT32 flags,
    size_t align, GEN_PVOID *palloc)
{
    if (palloc == NULL)
        return (NULL);
    if (align < 1)
        align = 1;
    if ((*palloc = ExAllocatePool(NonPagedPoolCacheAligned,
        (elem_num * elem_size) + align - 1)) == NULL
       )
        return (NULL);

    memset (*palloc, 0, (elem_num * elem_size) + align - 1);
    return (GEN_PVOID)((GEN_PUINT8)*palloc + ((size_t) *palloc % align));
}

/*--------------------*/
/* Free kernel memory */
/*--------------------*/

GEN_INLINE void GEN_FREE (GEN_PVOID memptr)
{
    if (memptr) ExFreePool(memptr);
}

GEN_INLINE void GEN_FREE_DMA_SAFE (GEN_PVOID memptr)
{
    if (memptr) MmFreeContiguousMemory(memptr);
}


/*------------*/
/* Semaphores */
/*------------*/

/*-------------------------------------*/
/* Create a mutual exclusion semaphore */
/*-------------------------------------*/

GEN_INLINE GEN_SEM_ID GEN_CREATE_MX_SEMAPHORE
(
    GEN_PSEMAPHORE FastMutex            /* Address of the semaphore struture */
)
{

    if (FastMutex == NULL)
        return ((GEN_SEM_ID) !NULL);
    ExInitializeFastMutex(FastMutex);   /* can't fail, so ... */

    return ((GEN_SEM_ID) FastMutex);    /* return any value other than NULL */
}

/*----------------------------------------------------------------*/
/* Delete a semaphore                                             */
/*                                                                */
/* This is a dummy function since there is no such thing on Win2k */
/*----------------------------------------------------------------*/

GEN_INLINE GEN_STATUS GEN_DELETE_SEMAPHORE
(
    GEN_PSEMAPHORE semName          /* Address of the semaphore struture */
)
{
    return (GEN_OK);                /* There is no semaphore delete on Linux */
}

/*---------------------*/
/* Request a semaphore */
/*---------------------*/

GEN_INLINE GEN_STATUS GEN_REQUEST_SEMAPHORE
(
    GEN_PSEMAPHORE FastMutex        /* Address of the semaphore struture */
)
{
    if (FastMutex == NULL)
        return (GEN_ERR);
    ExAcquireFastMutex(FastMutex);
    return (GEN_OK);
}

/*------------------------------------*/
/* Request a semaphore with a timeout */
/*------------------------------------*/

#define GEN_REQUEST_SEMAPHORE_WITH_TIMEOUT(semName, timeout) \
ERROR - Semaphore requests on Win2k do not have timeouts

/*---------------------*/
/* Release a semaphore */
/*---------------------*/

GEN_INLINE GEN_STATUS GEN_RELEASE_SEMAPHORE
(
    GEN_PSEMAPHORE FastMutex        /* Address of the semaphore struture */
)
{
    if (FastMutex == NULL)
        return (GEN_ERR);
    ExReleaseFastMutex(FastMutex);
    return (GEN_OK);
}

/*-----------------------------------------------------------------------*/
/* Atomic code sections                                                  */
/*                                                                       */
/* These are section of code which must not be interrupted or subject to */
/* context switches. These are the inline functions called by the macros */
/* defined above.                                                        */
/*-----------------------------------------------------------------------*/

GEN_INLINE void GEN_ILF_AtomicInit(GEN_PATOMIC SpinLock)
{
    KeInitializeSpinLock(SpinLock);
}

GEN_INLINE void GEN_ILF_StartAtomicCodeSection
(
GEN_PATOMIC       SpinLock,
GEN_PATOMIC_FLAGS OldIrql
)
{
    KeAcquireSpinLock(SpinLock, OldIrql);
}

GEN_INLINE void GEN_ILF_EndAtomicCodeSection
(
GEN_PATOMIC      SpinLock,
GEN_ATOMIC_FLAGS NewIrql
)
{
    KeReleaseSpinLock(SpinLock, NewIrql);
}

#endif /* GEN_WINDOWS_2K_KERNEL_MODE */

/*============================================================================*/
/******************************************************************************/

EXTERN_END

#endif
