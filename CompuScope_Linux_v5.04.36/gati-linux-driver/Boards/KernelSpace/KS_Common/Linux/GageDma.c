#if defined (_DECADE_DRV_ )
#  include "DecadeGageDrv.h"
#  include "DecadeNiosApi.h"
#  include "DecadeAlteraSupport.h"
#elif defined (_HEXAGON_DRV_ )  
   #  include "HexagonGageDrv.h"
   #  include "HexagonNiosApi.h"
   #  include "HexagonAlteraSupport.h"
#else
   #  include "GageDrv.h"
   #  include "NiosApi.h"
   #  include "AlteraSupport.h"
#endif
#include "CsPlxDefines.h"
#include "PlxSupport.h"

typedef int (*dmabuf_find_fp)(PFDO_DATA fdodatap, stmfdb_t *fdbctxp, stmdmabuf_t **dmabufpp);

static int dmabuf_find           (PFDO_DATA fdodatap, ulong uaddr, struct mm_struct *proc_mmp, int range_search, ulong *kaddrp, uint *avail_bytesp, stmdmabuf_t **dmabufpp);
static int dmabuf_get_baddr      (PFDO_DATA fdodatap, ulong uaddr, uint xfer_bytes, dma_addr_t *baddrp);
static int dmabuf_find_uaddr     (PFDO_DATA fdodatap, stmfdb_t *fdbctxp, stmdmabuf_t **dmabufpp);
static int dmabuf_find_kaddr     (PFDO_DATA fdodatap, stmfdb_t *fdbctxp, stmdmabuf_t **dmabufpp);
static int dmabuf_find_file      (PFDO_DATA fdodatap, stmfdb_t *fdbctxp, stmdmabuf_t **dmabufpp);
static int dmabuf_find_all       (PFDO_DATA fdodatap, stmfdb_t *fdbctxp, stmdmabuf_t **dmabufpp);
static int dmabuf_find_all_uaddr (PFDO_DATA fdodatap, stmfdb_t *fdbctxp, stmdmabuf_t **dmabufpp);

/*
 * --------------------------------------------------------------------------------
 * find dma buffer matching user-space address
 * --------------------------------------------------------------------------------
 */
int dmabuf_find (PFDO_DATA fdodatap,
                 ulong uaddr,                 /* in */
                 struct mm_struct *proc_mmp,  /* in; may be null */
                 int range_search,            /* in */
                 ulong *kaddrp,               /* out; may be null */
                 uint *avail_bytesp,          /* out; may be null */
                 stmdmabuf_t **dmabufpp)      /* out; may be null */
{
   stmdmabuf_t *dmabufp = 0;
   struct list_head *nodep = 0;
   ulong uaddr_end;
   uint offset;

   /* trace_in_(); */
   list_for_each (nodep, &fdodatap->dmabuf_list) {
      dmabufp = list_entry (nodep, stmdmabuf_t, list);
      if (proc_mmp && (proc_mmp != dmabufp->owner_mmp))
         continue;
      if (uaddr == dmabufp->uaddr) {
         if (kaddrp)       *kaddrp = (ulong)(dmabufp->kaddrp);
         if (dmabufpp)     *dmabufpp = dmabufp;
         if (avail_bytesp) *avail_bytesp = dmabufp->bytes;
         return 0;
      }
      if (range_search) {
         uaddr_end = dmabufp->uaddr + dmabufp->bytes;
         if ((uaddr >= dmabufp->uaddr) && (uaddr < uaddr_end)) {
            offset = uaddr - dmabufp->uaddr;
            if (kaddrp)       *kaddrp = ((ulong)dmabufp->kaddrp + offset);
            if (dmabufpp)     *dmabufpp = dmabufp;
            if (avail_bytesp) *avail_bytesp = dmabufp->bytes - offset;
            return 0;
         }
      }
   }
   return -EINVAL;
}

/*
 * --------------------------------------------------------------------------------
 * retrieve the bus address of the dma buffer matching user-space address and
 * transfer size constraint.
 * --------------------------------------------------------------------------------
 */
int dmabuf_get_baddr (PFDO_DATA fdodatap,
                      ulong uaddr,
                      uint xfer_bytes,
                      dma_addr_t *baddrp)
{
   stmdmabuf_t *dmabufp = 0;
   uint bytes_left;
   int ret;

   /* trace_in_(); */
   /* get bus address for user-space address; it must be a pre-allocated dma buffer */
   ret = dmabuf_find (fdodatap, uaddr, current->mm, 1, NULL, &bytes_left, &dmabufp);
   if (ret != 0)
      return ret;
   if (bytes_left < xfer_bytes)
      return -ENOMEM;
   *baddrp = dmabufp->baddr + (uaddr - dmabufp->uaddr);
   return 0;
}

/*
 * --------------------------------------------------------------------------------
 * stm_dmabuf_find helper; find dma buffer matching user-space address
 * --------------------------------------------------------------------------------
 */
int dmabuf_find_uaddr (PFDO_DATA fdodatap,
                       stmfdb_t *fdbctxp,
                       stmdmabuf_t **dmabufpp)
{
   /* trace_in_(); */
   int ret = dmabuf_find (fdodatap, fdbctxp->uaddr, current->mm, 0, NULL, NULL, dmabufpp);
   return (!ret ? 0 : 1/* not found */);
}

/*
 * --------------------------------------------------------------------------------
 * stm_dmabuf_find helper; find dma buffer matching kernel-space address
 * --------------------------------------------------------------------------------
 */
int dmabuf_find_kaddr (PFDO_DATA fdodatap,
                       stmfdb_t *fdbctxp,
                       stmdmabuf_t **dmabufpp)
{
   stmdmabuf_t *dmabufp = 0;
   struct list_head *nodep = 0;

   /* trace_in_(); */
   list_for_each (nodep, &fdodatap->dmabuf_list) {
      dmabufp = list_entry (nodep, stmdmabuf_t, list);
      if (fdbctxp->kaddrp == dmabufp->kaddrp) {
         *dmabufpp = dmabufp;
         return 0;
      }
   }
   return 1; /* not found */
}

/*
 * --------------------------------------------------------------------------------
 * stm_dmabuf_find helper; find dma buffer matching file-handler
 * --------------------------------------------------------------------------------
 */
int dmabuf_find_file (PFDO_DATA fdodatap,
                      stmfdb_t *fdbctxp,
                      stmdmabuf_t **dmabufpp)
{
   stmdmabuf_t *dmabufp = 0;
   struct list_head *nodep = 0;

   /* trace_in_(); */
   list_for_each (nodep, &fdodatap->dmabuf_list) {
      dmabufp = list_entry (nodep, stmdmabuf_t, list);
      if (dmabufp->owner_filp == fdbctxp->filp) {
         *dmabufpp = dmabufp;
         return 0;
      }
   }
   return 1; /* not found */
}

/*
 * --------------------------------------------------------------------------------
 * stm_dmabuf_find helper; find all dma buffers
 * --------------------------------------------------------------------------------
 */
int dmabuf_find_all (PFDO_DATA fdodatap,
                     stmfdb_t *fdbctxp,
                     stmdmabuf_t **dmabufpp)
{
   struct list_head *nodep;

   /* trace_in_(); */
   list_for_each (nodep, &fdodatap->dmabuf_list) {
      *dmabufpp = list_entry (nodep, stmdmabuf_t, list);
      return 0;
   }
   return 1; /* not found */
}

/*
 * --------------------------------------------------------------------------------
 * stm_dmabuf_find helper; find all dma buffers having user-space address.
 * --------------------------------------------------------------------------------
 */
int dmabuf_find_all_uaddr (PFDO_DATA fdodatap,
                           stmfdb_t *fdbctxp,
                           stmdmabuf_t **dmabufpp)
{
   struct list_head *nodep;

   /* trace_in_(); */
   list_for_each (nodep, &fdodatap->dmabuf_list) {
      *dmabufpp = list_entry (nodep, stmdmabuf_t, list);
#if 0
      if ((*dmabufpp)->flags & STMDBUF_DRIVER)
         continue;
#endif
      return 0;
   }
   return 1; /* not found */
}

/*
 * --------------------------------------------------------------------------------
 * --------------------------------------------------------------------------------
 */
stmdmabuf_t* stm_dmabuf_allocate (PFDO_DATA fdodatap,
                                  ALLOCATE_DMA_BUFFER *ctxp)
{
   stmdmabuf_t *dmabufp = 0;
   dma_addr_t baddr;
   void *kaddrp = 0;

   trace_in_();
   kaddrp = dma_alloc_coherent (&fdodatap->PciDev->dev, ctxp->in.u32BufferSize, &baddr, GFP_KERNEL);
   if (NULL == kaddrp)
      return NULL;
   dmabufp = kmalloc (sizeof(stmdmabuf_t), GFP_KERNEL);
   if (NULL == dmabufp) {
      dma_free_coherent (&fdodatap->PciDev->dev, ctxp->in.u32BufferSize, kaddrp, baddr);
      return NULL;
   }
   memset (dmabufp, 0, sizeof(stmdmabuf_t));
   INIT_LIST_HEAD (&dmabufp->list);
   dmabufp->bytes = ctxp->in.u32BufferSize;
   dmabufp->baddr = baddr;
   dmabufp->kaddrp = kaddrp;
   dmabufp->flags = STMDBUF_INIT;
   /* spin_lock_ (fdodatap); run inside g_devctl_mutex */
   {
      list_add (&dmabufp->list, &fdodatap->dmabuf_list);
   }
   /* spin_unlock_ (fdodatap); */
   return dmabufp;
}

/*
 * --------------------------------------------------------------------------------
 * this function is used by various parts of the driver to find and remove dma
 * buffer descriptors from the main device dma buffer list. it is assumed that this
 * function will be running in spinlocked code. because of this, no dma buffers are
 * actually freed in this call. the stm_dmabuf_free function will usually be called
 * at some point after this function (and outside of spinlock) to do the actual
 * unmapping and freeing of dma buffer/descriptor memory.
 * --------------------------------------------------------------------------------
 */
int stm_dmabuf_find (PFDO_DATA fdodatap,
                      stmfdb_t *fdbctxp,
                      struct list_head *dmabuf_listp)
{
   stmdmabuf_t *dmabufp = 0;
   dmabuf_find_fp findfuncp;
   int ret, oneoff;

   trace_in_();
   switch (fdbctxp->how) {
      case STMFDB_UADDR:     findfuncp = dmabuf_find_uaddr;     oneoff = 1; break;
      case STMFDB_FILE:      findfuncp = dmabuf_find_file;      oneoff = 0; break;
      case STMFDB_ALL:       findfuncp = dmabuf_find_all;       oneoff = 0; break;
      case STMFDB_ALL_UADDR: findfuncp = dmabuf_find_all_uaddr; oneoff = 0; break;
      case STMFDB_KADDR:     findfuncp = dmabuf_find_kaddr;     oneoff = 1; break;
      default: return -EINVAL;
   }
   do {
      ret = (*findfuncp) (fdodatap, fdbctxp, &dmabufp);
      if (!ret) {
         list_del_init (&dmabufp->list);
         list_add_tail (&dmabufp->list, dmabuf_listp);
      }
   }  while (!ret && !oneoff);
   return 0;
}

/*
 * --------------------------------------------------------------------------------
 * --------------------------------------------------------------------------------
 */
int stm_dmabuf_size (PFDO_DATA fdodatap,
                      stmfdb_t *fdbctxp,
                      uint *sizep)
{
   stmdmabuf_t *dmabufp = 0;
   dmabuf_find_fp findfuncp;
   int ret;

   trace_in_();
   switch (fdbctxp->how) {
      case STMFDB_UADDR:
         findfuncp = dmabuf_find_uaddr;
         break;

      default:
         return -EINVAL;
   }
   ret = (*findfuncp) (fdodatap, fdbctxp, &dmabufp);
   if (!ret)
      *sizep = dmabufp->bytes;
   else
      *sizep = 0;
   return 0;
}

/*
 * --------------------------------------------------------------------------------
 * free/unmap all given dma buffers
 * --------------------------------------------------------------------------------
 */
void stm_dmabuf_free (PFDO_DATA fdodatap,
                       struct list_head *dmabuf_listp)
{
   stmdmabuf_t *dmabufp = 0;
   struct list_head *nodep = 0, *nextp = 0;

   trace_in_();
   list_for_each_safe (nodep, nextp, dmabuf_listp) {
      dmabufp = list_entry (nodep, stmdmabuf_t, list);
      if (dmabufp->flags & STMDBUF_PAGE_RESERVED) {
         ulong i, kaddr, page_count;
         kaddr = (ulong)dmabufp->kaddrp;
         page_count = dmabufp->bytes >> PAGE_SHIFT;
         if (dmabufp->bytes & (PAGE_SIZE - 1))
            page_count++;
         for (i=0; i<page_count; i++, kaddr+=PAGE_SIZE)
            ClearPageReserved (virt_to_page (kaddr));
      }
      dma_free_coherent (&fdodatap->PciDev->dev, dmabufp->bytes, dmabufp->kaddrp, dmabufp->baddr);
      list_del (nodep);
      kfree (dmabufp);
   }
}

/*
 * --------------------------------------------------------------------------------
 * --------------------------------------------------------------------------------
 */
int stm_init (PFDO_DATA fdodatap)
{
   CS_STREAM *streamp = &fdodatap->Stream;

   trace_in_();
   INIT_LIST_HEAD (&fdodatap->dmabuf_list);
   init_completion (&streamp->dma_done);
   streamp->flags = 0;
   return 0;
}

/*
 * --------------------------------------------------------------------------------
 * --------------------------------------------------------------------------------
 */
void stm_cleanup (PFDO_DATA fdodatap)
{
   struct list_head dmabuf_list;
   stmfdb_t fdb_ctx;

   trace_in_();
   INIT_LIST_HEAD (&dmabuf_list);
   fdb_ctx.how = STMFDB_ALL;
   stm_dmabuf_find (fdodatap, &fdb_ctx, &dmabuf_list);
   stm_dmabuf_free (fdodatap, &dmabuf_list);
}

/*
 * --------------------------------------------------------------------------------
 * Nov 13, 2015
 * only aysnc call is supported for now
 * --------------------------------------------------------------------------------
 */
int stm_dma_start (PFDO_DATA fdodatap,
                   TRANSFER_DMA_BUFFER *trfbufp)
{
   int ret, dma_xfer_size;
   dma_addr_t baddr;
   CS_STREAM *streamp = &fdodatap->Stream;

   /* trace_in_(); */
   /* caller's user-space virtual address must be mapped dma buffer;
    * dmabuf_get_baddr checks for the size requirement too */
   if ((ret = dmabuf_get_baddr (fdodatap, (ulong)(trfbufp->in.pBuffer), trfbufp->in.u32TransferSize, &baddr)))
      return ret;

   init_completion (&streamp->dma_done);
   set_flag_ (streamp, flags, STMF_DMA_XFER);
   clr_flag_ (streamp, flags, STMF_DMA_ABORT);
   /* clr_flag_ (streamp, flags, STMF_FIFOFULL); */
   fdodatap->pfnAbortDmaTransfer (fdodatap);    /* write register(s) */
   fdodatap->pfnEnableInterruptDMA (fdodatap);  /* write register(s) */

   dma_xfer_size = AltBuildDmaDescContiguous (fdodatap, baddr, trfbufp->in.u32TransferSize);
   if (dma_xfer_size != trfbufp->in.u32TransferSize) {
      /* TODO */
   }
   fdodatap->pfnStartDmaTransferDescriptor (fdodatap);   /* write register(s) */

   return 0;
}

/*
 * --------------------------------------------------------------------------------
 * --------------------------------------------------------------------------------
 */
void stm_dma_abort (PFDO_DATA fdodatap)
{
   CS_STREAM *streamp = &fdodatap->Stream;

   trace_in_();

   if (!is_flag_set_ (streamp, flags, STMF_ENABLED))
      return;
   if (test_and_clear_flag_ (streamp, flags, STMF_DMA_XFER)) {
      AltAbortDmaTransfer (fdodatap);
      AbortTransfer (fdodatap);
      fdodatap->pfnDisableInterruptDMA (fdodatap);
   }
   set_flag_ (streamp, flags, STMF_DMA_ABORT);
   complete_all (&streamp->dma_done);
}

/*
 * --------------------------------------------------------------------------------
 * --------------------------------------------------------------------------------
 */
int stm_dma_wait (PFDO_DATA fdodatap,
                  uint msec)
{
   int ret;
   CS_STREAM *streamp = &fdodatap->Stream;

   trace_in_();
   if (!msec)
      ret = wait_for_completion_interruptible (&streamp->dma_done);
   else
      ret = wait_for_completion_interruptible_timeout (&streamp->dma_done, msecs_to_jiffies (msec));
   /* spin_lock_ (fdodatap);  */
   if (msec && ret==0)
      ret = -ETIME;
   else if ((msec && ret<0) || (!msec && ret!=0))  {
      /* received a signal */
      /* stm_dma_abort (fdodatap); */
      ret = -ERESTARTSYS;
   }
   /* either (msec && ret>0) or (!msec && ret==0) is true,
    * which both implied event-received, from this pt onward */
   else if (is_flag_set_ (streamp, flags, STMF_DMA_ABORT))
      ret = -EACCES;
   else if (is_flag_set_ (streamp, flags, STMF_FIFOFULL))
      ret = -ENOMEM;
   else
      ret = 0;
   /* spin_unlock_ (fdodatap); */
   /* if (ret != 0)
    *    trace_err_ ("wait failed"); */
   return ret;
}

/*
 * --------------------------------------------------------------------------------
 * --------------------------------------------------------------------------------
 */
int stm_dma_mmap (struct file *filp,
                  struct vm_area_struct *vmap)
{
   ulong kaddr, mm_bytes, page_cnt, i;
   stmdmabuf_t *dmabufp = 0;
   struct list_head *nodep = 0;
   PFDO_DATA devp = 0;
   void *mm_kaddrp = 0;

   trace_in_();
   devp = (PFDO_DATA)filp->private_data;
   mm_kaddrp = (void*)(vmap->vm_pgoff << PAGE_SHIFT);
   mm_bytes = vmap->vm_end - vmap->vm_start;

   /* find the dma buffer  */
   list_for_each (nodep, &devp->dmabuf_list) {
      dmabufp = list_entry (nodep, stmdmabuf_t, list);
      if (mm_kaddrp == dmabufp->kaddrp)
         break;
   }
   if (!dmabufp || (dmabufp->kaddrp != mm_kaddrp))
      return -EFAULT;
   if (mm_bytes > dmabufp->bytes)
      return -ENOMEM;
   if (dmabufp->uaddr != 0) /* already mapped */
      return -EINVAL;
   page_cnt = mm_bytes >> PAGE_SHIFT;
   if (mm_bytes & (PAGE_SIZE - 1))
      ++page_cnt;
   kaddr = (ulong)dmabufp->kaddrp;
   for (i=0; i<page_cnt; ++i, kaddr+=PAGE_SIZE)
      SetPageReserved (virt_to_page (kaddr));
   set_flag_ (dmabufp, flags, STMDBUF_PAGE_RESERVED);
   if (remap_pfn_range (vmap,
                        vmap->vm_start,
                        virt_to_phys (mm_kaddrp) >> PAGE_SHIFT,
                        mm_bytes,
                        vmap->vm_page_prot))
      return -EFAULT;
   /* vmap->vm_ops = &f_vmops; */
   /* spin_lock_ (devp); */
   {
      dmabufp->uaddr = (ulong)vmap->vm_start;
   }
   /* spin_unlock_ (devp); */
   return 0;
}

/*
 * --------------------------------------------------------------------------------
 * --------------------------------------------------------------------------------
 */
#if defined (_DECADE_DRV_) || defined (_HEXAGON_DRV_)
# define NIOS_EXT ,-1
#else
# define NIOS_EXT
#endif
int stm_start_capture (PFDO_DATA fdodatap,
                       AcquisitionCommand cmd)
{
   int32      ret     = CS_SUCCESS;
   uInt32     intr    = STREAM_OVERFLOW; // | N_MBUSY_REC_DONE
   CS_STREAM *streamp = &fdodatap->Stream;

   trace_in_();
   if (is_flag_set_ (streamp, flags, STMF_STARTED))
      return CS_INVALID_STATE;

   // Enable trigger int will cause FIFO int with small segmentsize and high sample rate.
   // anyways, trigger interrupt in streammming is not required. 
   //if (1 == fdodatap->CardState.u16CardNumber)
   //     intr |= MTRIG;   // Trigger interrupt enabled only on master or independent card

   clr_flag_ (streamp, flags, STMF_FIFOFULL);

   SetFifoMode (fdodatap, 0);
   ClearInterrupts (fdodatap);

   // Reset the FIFO_OVERLOW status
   // The reset also generate a FIFO_OVERLOW interrupt. That is why we have to do this with interrupt disabled.
   ReadGageRegister (fdodatap, STREAM_STATUS0);

   fdodatap->pfnEnableInterruptDMA (fdodatap);
   EnableInterrupts (fdodatap, intr);

   // Send command start stream
	if (AcqCmdStreaming_OCT == cmd)
      ret = WriteRegisterThruNios (fdodatap, 0, 0x4100000 NIOS_EXT);
   else
      ret = WriteRegisterThruNios (fdodatap, 0, CSPLXBASE_START_STREAM_CMD NIOS_EXT);
   set_flag_ (streamp, flags, STMF_STARTED);

   return ret;
}



