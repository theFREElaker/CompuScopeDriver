/*
 * GageDrv.c
 *
 *  Created on: 29-Aug-2008
 *      Author: quang
 */

/*============================================================================*/
/*                         I N C L U D E   F I L E S                          */
/*============================================================================*/

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include "gen_kernel_interface.h"
#include "gen_kdebug.h"
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,0))
#  include <linux/sched/signal.h>
#endif

#include "CsDeviceIDs.h"
#include "GageWrapper.h"
#include "CsCardState.h"
#include "CsIoctl.h"
#include "CsIoctlTypes.h"
#include "CsSymLinks.h"
#include "CsErrors.h"
#include "CsLinuxSignals.h"

#include "GageDrv.h"
#include "NiosApi.h"

#ifdef   _PLXBASE_
#  include "PlxSupport.h"
#  define  MEMMAP_BARINDEX    0xD            // 001101 (6 Pci Bar, 1=memory map)
#else
#  include "PldaSupport.h"
#  define  MEMMAP_BARINDEX    0x3            // 000011 (6 Pci Bar, 1=memory map)
#endif
#define ALTMAP_BARINDEX 0x3

// RG needed for newer kernels - have to find out when SA_SHIRQ was replace with IRQF_SHARED
#if !defined(IRQF_SHARED)
#  define IRQF_SHARED SA_SHIRQ
#endif

#define MAX_DMA_PAGES   512

GEN_LOCAL struct   proc_dir_entry *g_pGageProcDir = 0;
GEN_LOCAL FDO_DATA g_FdoArray[MAX_CARDS] = {{0}};

PFDO_DATA      g_FdoTable[MAX_CARDS] = {0};
CS_CARD_COUNT  g_CardsCount = {0};
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
static DEFINE_MUTEX(g_devctl_mutex);
#endif
/*---------------------------*/
/* File operation structure. */
/*---------------------------*/
struct file_operations CsFastBlFops = {
   .owner   = THIS_MODULE,
   .open    = GageCreate,
   .release = GageClose,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
   .unlocked_ioctl = GageDeviceControl,
#else
   .ioctl   = GageDeviceControl,
#endif
   .read    = GageRead,
   .mmap    = stm_dma_mmap
};

/**************************************************************************
 *
 **************************************************************************/

int GageFindDevice (void)
{
   struct pci_dev *pciDev = NULL;
   int            ret = 0;
   int            i = 0;
   int            j = 0;
   PFDO_DATA      FdoData = NULL;
   uInt16         u16DevId[MAX_CARDS];

   TRACEIN;

   memset( g_FdoArray, 0, MAX_CARDS*sizeof(FDO_DATA));
   GetDeviceIdList( u16DevId, sizeof(u16DevId) );

   // Find all Gage compuScope cards
   while( u16DevId[j] != 0 )
   {
      do
      {
         pciDev = pci_get_device (GAGE_VENDOR_ID, u16DevId[j], pciDev);
         if ( pciDev )
         {
            g_FdoArray[i].u32CardIndex = i;
            g_FdoArray[i].PseudoHandle = PSEUDO_HANDLE_BASE + i;
            g_FdoArray[i].PciDev       = pciDev;
            g_FdoArray[i].Device       = &pciDev->dev;
            g_FdoArray[i].DeviceId     = u16DevId[j];
            g_FdoTable[i]              = &g_FdoArray[i];
            stm_init (&g_FdoArray[i]);
            atomic_set (&g_FdoArray[i].tsk_count, 0);
            i++;
         }
      } while (pciDev != NULL);

      j++;
   }

   g_CardsCount.u32NumOfCards = i;
   g_CardsCount.bChanged = TRUE;

   if ( g_CardsCount.u32NumOfCards > 0 )
   {

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
      g_pGageProcDir = proc_mkdir (GAGE_PROCDIR, proc_root_driver);
#else
      char str[256];
      sprintf(str, "driver/%s", GAGE_PROCDIR);
      g_pGageProcDir = proc_mkdir (str, NULL);
#endif
      GEN_DEBUG_PRINT("g_pGageProcDir = %p\n", g_pGageProcDir);

      for ( i=0; i < g_CardsCount.u32NumOfCards; i++ )
      {
         FdoData = g_FdoTable[i];

         ret = GageMapHWResource(FdoData);
         if ( ret < 0 )
            continue;      // skip this device

         ret = GageAllocateSoftwareResource(FdoData);
         if ( ret < 0 )
         {
            GageUnMapHWResource(FdoData);
            continue;      // skip this device
         }

         FdoData->pMasterIndependent = FdoData->pTriggerMaster = FdoData;


         CsAssignRegisterBaseAddress( FdoData );
         if ( IsNiosInit(FdoData))
            GEN_PRINT("Nios is init\n");
         else
            GEN_PRINT("Nios is not init\n");

         ret = GageCreateDeviceSymbolicLink(FdoData);
         if ( ret < 0 )
         {
            GageFreeSoftwareResource(FdoData);
            GageUnMapHWResource(FdoData);
            continue;      // skip this device
         }

         /* enable interrupt handler last */
         GEN_DEBUG_PRINT ("Interrupt Line %2d\n", FdoData->PciDev->irq);
         ret = request_irq (FdoData->PciDev->irq, /*test_isr*/ Isr_CsIrq, IRQF_SHARED, /*shared*/
                            CS_SYMLINK_NAME, (void *) FdoData);
         if (ret != 0)
         {
            GageFreeSoftwareResource(FdoData);
            GageUnMapHWResource(FdoData);
            continue;      // skip this device
         }
         FdoData->IrqLine = FdoData->PciDev->irq;
      }
   }

   TRACEOUT;

   return g_CardsCount.u32NumOfCards;
}

void   TestPciCapabilities(PFDO_DATA  FdoData)
{
   struct pci_dev *dev = FdoData->PciDev;
   int pos;

   // Assume first it is a PCI card.
   FdoData->CardState.bPciExpress = FALSE;

   if ((pos = pci_find_capability(dev, PCI_CAP_ID_EXP)))
   {
      u16 val;

      GEN_PRINT ("Found PCIe Card\n");
      FdoData->CardState.bPciExpress = TRUE;

      pci_read_config_word(dev, pos + PCI_EXP_LNKCAP, &val);
      FdoData->PciExLinkInfo.MaxLinkSpeed = (val & PCI_EXP_LNKCAP_SLS);
      FdoData->PciExLinkInfo.MaxLinkWidth = (val & PCI_EXP_LNKCAP_MLW) >> 4;

      GEN_PRINT("Capability.LinkCapabilities = 0x%x\n", val);
      GEN_PRINT("Capability.LinkCapabilities.MaxLinkSpeed = 0x%x\n", FdoData->PciExLinkInfo.MaxLinkSpeed);
      GEN_PRINT("Capability.LinkCapabilities.MaxLinkWidth = 0x%x\n", FdoData->PciExLinkInfo.MaxLinkWidth);

      pci_read_config_word(dev, pos + PCI_EXP_LNKSTA, &val);
      FdoData->PciExLinkInfo.LinkSpeed     = (val & PCI_EXP_LNKSTA_CLS);
      FdoData->PciExLinkInfo.LinkWidth     = (val & PCI_EXP_LNKSTA_NLW) >> 4;

      GEN_PRINT("Capability.LinkStatus = 0x%x\n", val);
      GEN_PRINT("Capability.LinkStatus.LinkSpeed = 0x%x\n", FdoData->PciExLinkInfo.LinkSpeed);
      GEN_PRINT("Capability.LinkStatus.LinkWidth = 0x%x\n", FdoData->PciExLinkInfo.LinkWidth);

      pci_read_config_word(dev, pos + PCI_EXP_DEVCTL, &val);
      FdoData->PciExLinkInfo.MaxPayloadSize   = (val & PCI_EXP_DEVCTL_PAYLOAD) >> 4;

      pci_read_config_word(dev, pos + PCI_EXP_DEVCAP, &val);
      FdoData->PciExLinkInfo.CapturedSlotPowerLimit = (val & PCI_EXP_DEVCAP_PWR_VAL) >> 18;
      FdoData->PciExLinkInfo.CapturedSlotPowerLimitScale = (val & PCI_EXP_DEVCAP_PWR_SCL) >> 26;
   }
}

/**************************************************************************
 *
 **************************************************************************/
int GageMapHWResource(PFDO_DATA FdoData)
{
   int     i = 0;
   int     j = 0;
   int     ret = 0;
   struct pci_dev *pciDev = FdoData->PciDev;


   //   return 0;
   TRACEIN;
   ret = pci_enable_device (FdoData->PciDev);
   if (ret != 0)
   {
      FdoData->PciDev = 0;
      GEN_PRINT ("pci_enable_device failed, error = %d\n", ret);
      ret = -1;
      goto OnError;
   }

   TestPciCapabilities( FdoData );
   CsAssignHardwareParams( FdoData );
   CsAssignFunctionPointers( FdoData );

   // Find memory mapped bar index
   for (i = 0; i < 6; i ++)
   {
      if (FdoData->CardState.bPciExpress)
      {
         if (!(ALTMAP_BARINDEX & (1 << i)))
            continue;
      } else {
         if (!(MEMMAP_BARINDEX & (1 << i)))
            continue;
      }

      FdoData->MemPhysAddress[j].LowPart       = pci_resource_start (pciDev, i);
      FdoData->MemPhysAddress[j].HighPart    = 0;
      FdoData->MemLength[j]               = pci_resource_len   (pciDev, i);

      GEN_DEBUG_PRINT ("Physical base address %d start = 0x%08X\n", j, FdoData->MemPhysAddress[j].LowPart);
      GEN_DEBUG_PRINT ("Physical base address %d size  = 0x%08lx\n", j, FdoData->MemLength[j]);

      if (request_mem_region (FdoData->MemPhysAddress[j].LowPart, FdoData->MemLength[j], CS_SYMLINK_NAME) == NULL)
      {
         FdoData->MemPhysAddress[j].QuadPart = 0;
         FdoData->MemLength[j] = 0;
         GageUnMapHWResource(FdoData);
         GEN_PRINT ("Gage Failed request memory region %d for card %d\n", j, FdoData->u32CardIndex);
         ret = -ENXIO;
         goto OnError;
      }

      FdoData->MapAddress[j] = ioremap_nocache (FdoData->MemPhysAddress[j].LowPart, FdoData->MemLength[j]);
      if (NULL == FdoData->MapAddress[j])
      {
         GEN_PRINT ("Gage Failed on memory mapped %d for card %d\n", j, FdoData->u32CardIndex);
         GageUnMapHWResource(FdoData);
         ret = -1;
         goto OnError;
      }
      GEN_DEBUG_PRINT ("Virtual base address %d start  = 0x%p\n", j, FdoData->MapAddress[j]);
      j++;
   }
OnError:
   TRACEOUT;
   return ret;
}


/**************************************************************************
 *
 **************************************************************************/
int GageUnMapHWResource(PFDO_DATA FdoData)
{
   int    i = 0;
   TRACEIN;
   /* disable interrupt hanlder first */
   if ( FdoData->IrqLine )
   {
      GEN_PRINT ("Gage%d Freeing IRQ %2d\n", FdoData->u32CardIndex, FdoData->PciDev->irq);
      free_irq (FdoData->PciDev->irq, (void *) FdoData);
      FdoData->IrqLine = 0;
   }

   for ( i = 0; i <NUM_MEMORYMAP; i ++ )
   {
      if (FdoData->MapAddress[i])
      {
         iounmap( FdoData->MapAddress[i] );
         FdoData->MapAddress[i] = 0;
      }
      if (FdoData->MemPhysAddress[i].LowPart)
      {
         release_mem_region (FdoData->MemPhysAddress[i].LowPart, FdoData->MemLength[i]);
         FdoData->MemPhysAddress[i].QuadPart = 0;
      }
   }

   // bus master is disabled if we called pci_disable_device !!!
#ifdef   CALL_PCI_DISABLED_DEVICE
   if ( FdoData->PciDev )
   {
      pci_disable_device (FdoData->PciDev);
      FdoData->PciDev = 0;
   }
#endif
   TRACEOUT;

   return 0;
}

/**************************************************************************
 *
 **************************************************************************/
int GageReleaseDevice()
{
   uInt32 i;

   //TRACEIN;
   for (i=0; i < g_CardsCount.u32NumOfCards; i ++)
   {
      GageFreeSoftwareResource (g_FdoTable[i]);
      GageUnMapHWResource (g_FdoTable[i]);
      GageDestroyDeviceSymbolicLink (g_FdoTable[i]);
   }
   if (g_pGageProcDir) {
      char str[256];
      sprintf(str, "driver/%s", GAGE_PROCDIR);
      remove_proc_entry (str, NULL);
   }
   //TRACEOUT;

   return 0;
}


/**************************************************************************
 *
 **************************************************************************/
int   GageCreateDeviceSymbolicLink( PFDO_DATA FdoData )
{
   struct proc_dir_entry *ProcEntry = 0;

   TRACEIN;
   snprintf( FdoData->szSymbolLink, sizeof(FdoData->szSymbolLink)-1, CS_SYMLINK_NAME"%02d", FdoData->u32CardIndex );
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
   ProcEntry = create_proc_entry (FdoData->szSymbolLink, S_IFREG | S_IRUGO | S_IWUGO, g_pGageProcDir);
#else
   ProcEntry = proc_create_data (FdoData->szSymbolLink, S_IFREG | S_IRUGO | S_IWUGO, g_pGageProcDir, &CsFastBlFops, FdoData);
#endif
   if (NULL == ProcEntry)
   {
      GEN_PRINT ("%s proc entry creation failed\n", FdoData->szSymbolLink);
      TRACEOUT;
      return -1;
   }
   /* GEN_DEBUG_PRINT ("%s proc entry creation succeeded\n", FdoData->szSymbolLink); */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
#  if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
   ProcEntry->owner     = THIS_MODULE;
#  endif
   ProcEntry->data      = (void*) FdoData;
   ProcEntry->proc_fops = &CsFastBlFops;
#endif

   TRACEOUT;
   return 0;
}

/**************************************************************************
 *
 **************************************************************************/
void   GageDestroyDeviceSymbolicLink( PFDO_DATA FdoData )
{
   TRACEIN;
   remove_proc_entry (FdoData->szSymbolLink, g_pGageProcDir);  // master /proc entry
   TRACEOUT;
}


/**************************************************************************
 *
 **************************************************************************/
int GageCreate   (struct inode *inode, struct file *filp)
{
   void *private_data=0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
   private_data = PDE(inode)->data;
#else
   private_data = PDE_DATA(inode);
#endif
   /* if (((PFDO_DATA)private_data)->tskEvent)
    *    return -EBUSY; */
   filp->private_data = private_data;
   ((PFDO_DATA)private_data)->refcount++;
   /* ((PFDO_DATA)private_data)->tskEvent = get_current ();
    * ((PFDO_DATA)private_data)->pid = (((PFDO_DATA)private_data)->tskEvent)->pid; */
   return 0;
}


/**************************************************************************
 *
 **************************************************************************/
int GageClose (struct inode *inode, struct file *filp)
{
   void *private_data=0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
   private_data = PDE(inode)->data;
#else
   private_data = PDE_DATA(inode);
#endif
   ((PFDO_DATA)private_data)->refcount--;
   if (!((PFDO_DATA)private_data)->refcount) {
      ((PFDO_DATA)private_data)->tskEvent = 0;
      ((PFDO_DATA)private_data)->pid = 0;
   }
   return 0;
}


/**************************************************************************
 *
 **************************************************************************/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
long   GageDeviceControl(struct file *filp, UINT IoctlCmd, ULONG ioctl_arg)
#else
int   GageDeviceControl(struct inode *inode, struct file *filp, UINT IoctlCmd, ULONG ioctl_arg)
#endif
{
   int         iRet = 0;
   PFDO_DATA    FdoData = (PFDO_DATA) filp->private_data;
   int32      i32Status = CS_SUCCESS;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
   mutex_lock(&g_devctl_mutex);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
   iRet = CsBoardDevIoCtl(filp, IoctlCmd, ioctl_arg);
#else
   iRet = CsBoardDevIoCtl(inode, filp, IoctlCmd, ioctl_arg);
#endif
   if ( STAT_MORE_PROCESSING_REQUIRED != iRet ) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
      mutex_unlock(&g_devctl_mutex);
#endif
      return iRet;
   }

   switch (IoctlCmd)
   {
      case CS_IOCTL_GET_NUM_OF_DEVICES:
         {
            //VCTEST
            if (!ioctl_arg)
               g_CardsCount.bChanged = FALSE;
            else
               iRet = copy_to_user ((void __user *) ioctl_arg, &g_CardsCount, sizeof (CS_CARD_COUNT));

         }
         break;
      case CS_IOCTL_GET_DEVICE_INFO:
         {
            ULONG   i = 0;
            CSDEVICE_INFO    *HandleTable = (CSDEVICE_INFO *) ioctl_arg;
            CSDEVICE_INFO   DrvBuffer = {0};

            // Copy each DRVHANDLE back to application buffer,
            // one at a time since on Linux we don't know the user's buffer size
            for (i = 0; i < g_CardsCount.u32NumOfCards; i++)
            {
               DrvBuffer.DrvHdl = (DRVHANDLE) g_FdoTable[i]->PseudoHandle;
               DrvBuffer.DeviceId = g_FdoTable[i]->DeviceId;
               DrvBuffer.BusType   = g_FdoTable[i]->BusType;
               strcpy( DrvBuffer.SymbolicLink, GAGE_PROCDIR"/" );
               strcat( DrvBuffer.SymbolicLink, g_FdoTable[i]->szSymbolLink );
               if (copy_to_user ((void __user *) &HandleTable[i], &DrvBuffer, sizeof (CSDEVICE_INFO)) != 0)
                  break;   // no more room in the user's buffer404
            }
         }
         break;
      case CS_IOCTL_WRITE_PCI_REGISTER:
         {
            io_READWRITE_PCI_REG ioStruct = {0};
            iRet = copy_from_user(&ioStruct, (void __user*)ioctl_arg, sizeof(io_READWRITE_PCI_REG));

            _WriteRegister(FdoData, ioStruct.u32Offset, ioStruct.u32RegVal );
         }
         break;


      case CS_IOCTL_READ_PCI_REGISTER:
         {
            io_READWRITE_PCI_REG   ioStruct;

            iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_PCI_REG));
            ioStruct.u32RegVal = _ReadRegister(FdoData, ioStruct.u32Offset);
            iRet = copy_to_user ((void __user *) ioctl_arg, &ioStruct, sizeof (io_READWRITE_PCI_REG));
         }
         break;

      case CS_IOCTL_WRITE_GAGE_REGISTER:
         {
            io_READWRITE_PCI_REG   ioStruct;

            iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_PCI_REG));
            WriteGageRegister(FdoData, ioStruct.u32Offset, ioStruct.u32RegVal );
         }
         break;


      case CS_IOCTL_READ_GAGE_REGISTER:
         {
            io_READWRITE_PCI_REG   ioStruct;

            iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_PCI_REG));
            ioStruct.u32RegVal = ReadGageRegister(FdoData, ioStruct.u32Offset);
            iRet = copy_to_user ((void __user *) ioctl_arg, &ioStruct, sizeof (io_READWRITE_PCI_REG));
         }
         break;

      case CS_IOCTL_WRITE_NIOS_REGISTER:
         {
            io_READWRITE_NIOS_REGISTER   ioStruct;

            iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_NIOS_REGISTER));

            ioStruct.i32Status = WriteRegisterThruNios(FdoData, ioStruct.u32Data, ioStruct.u32Command);

            iRet = copy_to_user ((void __user *) ioctl_arg, &ioStruct, sizeof (io_READWRITE_NIOS_REGISTER));
         }
         break;

      case CS_IOCTL_WRITE_NIOS_REGISTER_EX:
         {
            io_READWRITE_NIOS_REGISTER   ioStruct;

            iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_NIOS_REGISTER));

            ioStruct.i32Status = WriteRegisterThruNiosEx(FdoData, ioStruct.u32Data, ioStruct.u32Command, &ioStruct.u32RetVal);

            iRet = copy_to_user ((void __user *) ioctl_arg, &ioStruct, sizeof (io_READWRITE_NIOS_REGISTER));
         }
         break;

      case CS_IOCTL_READ_NIOS_REGISTER:
         {
            io_READWRITE_NIOS_REGISTER ioStruct = {0};
            uInt32                     u32RetVal;

            iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_NIOS_REGISTER));

            u32RetVal = ReadRegisterFromNios(FdoData, ioStruct.u32Data, ioStruct.u32Command);

            ioStruct.u32RetVal = u32RetVal;
            ioStruct.i32Status = i32Status;

            iRet = copy_to_user((void __user *) ioctl_arg, &ioStruct, sizeof(io_READWRITE_NIOS_REGISTER));
         }
         break;

      case CS_IOCTL_READ_PCI_CONFIG_SPACE:
         {
            io_READWRITE_PCI_CONFIG ioStruct = {0};
            uInt32                  i = 0;
            uInt8                   u8Temp[256];
            uInt32                  u32Offset = 0;
            uInt32                  u32ByteCount = 0;

            iRet = copy_from_user ( &ioStruct, (void __user *)ioctl_arg, sizeof (io_READWRITE_PCI_CONFIG));

            u32Offset = ioStruct.u32Offset;
            u32ByteCount = ioStruct.u32ByteCount;

            for (i=0; i < u32ByteCount; i ++)
            {
               pci_read_config_byte( FdoData->PciDev, u32Offset++, &u8Temp[i]);
            }

            iRet = copy_to_user ((void __user *) ioStruct.ConfigBuffer, &u8Temp, u32ByteCount );
         }
         break;

      case CS_IOCTL_WRITE_PCI_CONFIG_SPACE:
         {
            uInt32                   i = 0;
            Pio_READWRITE_PCI_CONFIG    PioStruct   = (Pio_READWRITE_PCI_CONFIG) ioctl_arg;
            uInt32                  u32Offset = PioStruct->u32Offset;
            uInt8                  *u8Temp = (uInt8 *) PioStruct->ConfigBuffer;

            for (i=0; i < PioStruct->u32ByteCount; i ++)
            {
               pci_write_config_byte( FdoData->PciDev, u32Offset++, u8Temp[i]);
            }
         }
         break;

      case CS_IOCTL_REGISTER_EVENT_HANDLE:
         {
            FdoData->tskEvent = get_current();
            FdoData->pid = FdoData->tskEvent->pid;
         }
         break;

      case CS_IOCTL_START_ACQUISITION:
         {
            in_STARTACQUISITION_PARAMS StartAcqParams;

            iRet = copy_from_user ( &StartAcqParams, (void __user *)ioctl_arg, sizeof (in_STARTACQUISITION_PARAMS));
            if ( 0 == iRet )
            {
               FdoData->m_u32SegmentRead = 0;

               Abort( FdoData );
               SendStartAcquisition( FdoData->pMasterIndependent, &StartAcqParams );
            }
         }
         break;


      case CS_IOCTL_SET_ACQCONFIG:
         {
            CSACQUISITIONCONFIG_EX AcqCfgEx;

            iRet = copy_from_user ( &AcqCfgEx, (void __user *)ioctl_arg, sizeof (CSACQUISITIONCONFIG_EX));
            SetAcquisitionConfig( FdoData, &AcqCfgEx );
         }
         break;

      case CS_IOCTL_MASTERSLAVE_LINK:
         {
            MS_LINK_INFO   LocalMsLinkInfo;
            int32         i32Status;

            iRet = !access_ok( VERIFY_WRITE, (void __user *)ioctl_arg, sizeof (MS_LINK_INFO));
            if (iRet) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
               mutex_unlock(&g_devctl_mutex);
#endif
               return -EFAULT;
            }

            iRet = copy_from_user ( &LocalMsLinkInfo, (void __user *)ioctl_arg, sizeof (MS_LINK_INFO));
            if (iRet) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
               mutex_unlock(&g_devctl_mutex);
#endif
               return -EFAULT;
            }
            i32Status  = SetMasterSlaveLink( FdoData, LocalMsLinkInfo.u32NumOfSlave, LocalMsLinkInfo.MsLinkInfo );

            put_user(i32Status, (int32 __user *)ioctl_arg);
         }
         break;

      case CS_IOCTL_MASTERSLAVE_CALIBINFO:
         {
            MS_CALIB_INFO   LocalMsCalibInfo;
            int32         i32Status;

            iRet = !access_ok( VERIFY_WRITE, (void __user *)ioctl_arg, sizeof (MS_CALIB_INFO));
            if (iRet) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
               mutex_unlock(&g_devctl_mutex);
#endif
               return -EFAULT;
            }

            iRet = copy_from_user ( &LocalMsCalibInfo, (void __user *)ioctl_arg, sizeof (MS_CALIB_INFO));
            if (iRet) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
               mutex_unlock(&g_devctl_mutex);
#endif
               return -EFAULT;
            }

            i32Status  = SetMasterSlaveCalibInfo( FdoData, LocalMsCalibInfo.u32NumOfSlave, LocalMsCalibInfo.MsAlignOffset );

            put_user(i32Status, (int32 __user *)ioctl_arg);
         }
         break;

         /*      case CS_IOCTL_SET_TRIGGERMASTER:
                 {
                 PCS_IOCTL_STATUS   pOutStruct = (PCS_IOCTL_STATUS) OutBuffer;
                 DRVHANDLE         *pTrigMAsterHdl = (DRVHANDLE *) InBuffer;

                 pOutStruct->i32Status  = SetTriggerMasterLink( FdoData, *pTrigMAsterHdl );

                 Information = OutputBufferLength;
                 }
                 break;
                 */
      case CS_IOCTL_PRE_DATATRANSFER:
         {
            Pio_PREPARE_DATATRANSFER    temp = (Pio_PREPARE_DATATRANSFER) ioctl_arg;
            io_PREPARE_DATATRANSFER      DataXferParams;

            Pin_PREPARE_DATATRANSFER   pInTransferEx = (Pin_PREPARE_DATATRANSFER) &DataXferParams.InParams;
            Pout_PREPARE_DATATRANSFER   pOutTransferEx = (Pout_PREPARE_DATATRANSFER) &DataXferParams.OutParams;
            int64                  i64PreDepthLimit;

            uInt32   u32LocalReadingOffset;
            uInt32   u32LocalAlignOffset;
            int64   i64StartAddress;

            iRet = copy_from_user ( &DataXferParams.InParams, (void __user *)&temp->InParams, sizeof (in_PREPARE_DATATRANSFER));

            GEN_PRINT ("FDO Data = %p\n", FdoData );
            GEN_PRINT ("InParams Channel = %d\n", pInTransferEx->u16Channel );
            GEN_PRINT ("InParams DataMask = %d\n", pInTransferEx->u32DataMask );

            FdoData->bDataTransferIntrEn    = pInTransferEx->bIntrEnable;
            FdoData->bDataTransferBusMaster = pInTransferEx->bBusMasterDma;
#ifdef _USE_EXTRAS_
            FdoData->u32Extras             = pInTransferEx->u32Extras;
#endif

            i64PreDepthLimit = GetPreTrigDepthLimit ( FdoData,  pInTransferEx->u32Segment );
            i64PreDepthLimit *= -1;

            GEN_PRINT ("Pretrigger limit = %lld\n", i64PreDepthLimit );

            pOutTransferEx->i64ActualStartAddr = pInTransferEx->i64StartAddr;
            if ( pOutTransferEx->i64ActualStartAddr  < 0 )
            {
               //----- Application requested for pretrigger data
               //----- Adjust ActualStart address acordingly to pre trigger data
               if ( i64PreDepthLimit > pOutTransferEx->i64ActualStartAddr )
               {
                  //----- There is less pre trigger data than requested. Re-adjust the start position
                  pOutTransferEx->i64ActualStartAddr = i64PreDepthLimit;
               }
            }

            i64StartAddress  = pOutTransferEx->i64ActualStartAddr + pInTransferEx->i32OffsetAdj;


            u32LocalReadingOffset = CalculateReadingOffset( FdoData, (int32) i64StartAddress, pInTransferEx->u32Segment, &u32LocalAlignOffset );

            SetupPreDataTransfer( FdoData, pInTransferEx->u32Segment,  pInTransferEx->u16Channel, u32LocalReadingOffset, pInTransferEx );

            pOutTransferEx->i32ActualStartOffset = u32LocalAlignOffset;
            pOutTransferEx->i32ActualStartOffset *= -1;


            pOutTransferEx->u32LocalAlignOffset = u32LocalAlignOffset;
            pOutTransferEx->u32LocalReadingOffset = FdoData->u32ReadingOffset = u32LocalReadingOffset;

            pOutTransferEx->m_u32HwTrigAddr = FdoData->m_u32HwTrigAddr;
            pOutTransferEx->m_Etb         = FdoData->m_Etb;
            pOutTransferEx->m_MemEtb      = FdoData->m_MemEtb;

            pOutTransferEx->m_Stb         = FdoData->m_Stb;
            pOutTransferEx->m_SkipEtb      = FdoData->m_SkipEtb;
            pOutTransferEx->m_AddonEtb      = FdoData->m_AddonEtb;
            pOutTransferEx->m_ChannelSkipEtb   = FdoData->m_ChannelSkipEtb;
            pOutTransferEx->m_DecEtb      = FdoData->m_DecEtb;
            pOutTransferEx->i64PreTrigLimit = i64PreDepthLimit;

            pOutTransferEx->i32OffsetAdjust = pOutTransferEx->i32ActualStartOffset;

            pOutTransferEx->m_bMemRollOver = 0;

            iRet = copy_to_user ( (void __user *)&temp->OutParams, &DataXferParams.OutParams, sizeof (out_PREPARE_DATATRANSFER));

         }
         break;
      case CS_IOCTL_GET_CARDSTATE:
         {
            iRet = copy_to_user ((void __user *) ioctl_arg, &FdoData->CardState, sizeof (CSCARD_STATE));
         }
         break;
      case CS_IOCTL_SET_CARDSTATE:
         {
            iRet = copy_from_user (&FdoData->CardState, (void __user *)ioctl_arg, sizeof (CSCARD_STATE));
         }
         break;
      case CS_IOCTL_ABORT_TRANSFER:
         {
#ifdef CS_STREAM_SUPPORTED
            if (is_flag_set_ (&FdoData->Stream, flags, STMF_DMA_XFER))
               stm_dma_abort (FdoData);
            else
#endif
            {
               if (FdoData->bDmaIntrWait)
               {
                  FdoData->pfnAbortDmaTransfer( FdoData );
                  AbortTransfer( FdoData );
                  FdoData->pfnDisableInterruptDMA( FdoData );
                  FdoData->XferRead.i32TxStatus = CS_ASYNCTRANSFER_ABORTED;
                  FdoData->bDmaIntrWait = FALSE;
                  wake_up_interruptible( &FdoData->DmaWaitQueue );
               }
            }
         }
         break;

      case CS_IOCTL_ALLOCATE_STM_DMA_BUFFER:
         {
            int ok = 1, errcode = -EFAULT;
            stmdmabuf_t *dmabufp = 0;
            ALLOCATE_DMA_BUFFER ctx;

            if (copy_from_user (&ctx, (void*)ioctl_arg, sizeof (ALLOCATE_DMA_BUFFER)) ||
                ctx.in.u32BufferSize <= 0) {
               ok = !ok;
               errcode = -EINVAL;
            }
            if (ok) {
               if (ctx.in.u32BufferSize & (PAGE_SIZE - 1))
                  ctx.in.u32BufferSize = (ctx.in.u32BufferSize + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
               dmabufp = stm_dmabuf_allocate (FdoData, &ctx);
               if (!dmabufp) {
                  ok = !ok;
                  errcode = -ENOMEM;
               }
            }
            if (ok) {
               /* spin_lock_ (devp); */
               {
                  dmabufp->owner_mmp = current->mm;
                  dmabufp->owner_filp = filp;
                  ctx.out.pVa = dmabufp->kaddrp;
                  ctx.out.i32Status = CS_SUCCESS;
               }
               /* spin_unlock_ (devp); */
               if (copy_to_user ((void*)ioctl_arg, &ctx, sizeof(ALLOCATE_DMA_BUFFER)))
                  ok = !ok;
            }
            if (!ok) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
               mutex_unlock (&g_devctl_mutex);
#endif
               return errcode;
            }
         }
         break;

      case CS_IOCTL_FREE_STM_DMA_BUFFER:
         {
            int ok = 1, errcode = -EFAULT;
            struct list_head dmabuf_list;
            stmfdb_t fdb_ctx;
            FREE_DMA_BUFFER ctx;

            INIT_LIST_HEAD (&dmabuf_list);

            /* trace_in_(); */
            if (copy_from_user (&ctx, (void*)ioctl_arg, sizeof(FREE_DMA_BUFFER)) ||
                ctx.in.pVa == 0) {
               ok = !ok;
               errcode = -EINVAL;
            }
            /* ctx.pVa is a user-space addr during normal operation; if mmap
             * failed, kernel-space addr could be back passed down and we need
             * in.kaddr to indicate this */
            if (ok) {
               if (ctx.in.kaddr) {
                  fdb_ctx.how = STMFDB_KADDR;
                  fdb_ctx.kaddrp = (void*)(ulong)ctx.in.pVa;
               }
               else {
                  fdb_ctx.how = STMFDB_UADDR;
                  fdb_ctx.uaddr = (ulong)ctx.in.pVa;
               }
               /* spin_lock_ (devp); */
               {
                  stm_dmabuf_find (FdoData, &fdb_ctx,  &dmabuf_list);
               }
               /* spin_unlock_ (devp); */
               if (list_empty (&dmabuf_list)) {
                  ok = !ok;
                  errcode = -EINVAL;
               }
            }
            if (ok) {
               stm_dmabuf_free (FdoData, &dmabuf_list);
               /* ctx.out.i32Status = CS_SUCCESS;
                * if (copy_to_user ((void*)ioctl_arg, &ctx, sizeof(FREE_DMA_BUFFER)))
                *    ok = !ok; */
            }
            else {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
               mutex_unlock (&g_devctl_mutex);
#endif
               return errcode;
            }
         }
         break;

      case CS_IOCTL_STM_DMA_BUFFER_SIZE:
         {
            int ok = 1, errcode = -EFAULT, ret;
            uint bytes;
            stmfdb_t fdb_ctx;
            FREE_DMA_BUFFER ctx;

            if (copy_from_user (&ctx, (void*)ioctl_arg, sizeof(FREE_DMA_BUFFER)) ||
                ctx.in.pVa == 0) {
               ok = !ok;
               errcode = -EINVAL;
            }
            if (ok) {
               fdb_ctx.how = STMFDB_UADDR;
               fdb_ctx.uaddr = (ulong)ctx.in.pVa;
               /* spin_lock_ (devp); */
               {
                  ret = stm_dmabuf_size (FdoData, &fdb_ctx, &bytes);
               }
               /* spin_unlock_ (devp); */
               if (ret != 0) {
                  ok = !ok;
                  errcode = -EINVAL;
               }
            }
            if (ok) {
               ctx.out.i32Status = (int32)bytes;
               if (copy_to_user ((void*)ioctl_arg, &ctx, sizeof(FREE_DMA_BUFFER)))
                  ok = !ok;
            }
            if (!ok) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
               mutex_unlock (&g_devctl_mutex);
#endif
               return errcode;
            }
         }
         break;

      case CS_IOCTL_START_STM_DMA_BUFFER:
         {
            int ok = 1, errcode = -EFAULT;
            TRANSFER_DMA_BUFFER ctx;

            if (copy_from_user (&ctx, (void*)ioctl_arg, sizeof(TRANSFER_DMA_BUFFER)) ||
                ctx.in.pBuffer == 0 ||
                ctx.in.u32TransferSize <= 0) {
               ok = !ok;
               errcode = -EINVAL;
               /* ctx.out.i32Status = CS_INVALID_PARAMETER; */
            }
            if (ok && is_flag_set_ (&FdoData->Stream, flags, STMF_DMA_XFER)) {
               ok = !ok;
               errcode = -EPERM;
               /* ctx.out.i32Status = CS_INVALID_STATE; */
            }
            if (ok && stm_dma_start (FdoData, &ctx) != 0) {
               ok = !ok;
               errcode = -EINVAL;
               /* ctx.out.i32Status = CS_INVALID_PARAMETER; */
            }
            if (!ok) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
               mutex_unlock (&g_devctl_mutex);
#endif
               return errcode;
            }
         }
         break;

      case CS_IOCTL_SET_STREAM_MODE:
         {
            SET_STREAM_MODE ctx;
            CS_STREAM *streamp = &FdoData->Stream;

            if (copy_from_user (&ctx, (void*)ioctl_arg, sizeof(SET_STREAM_MODE))) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
               mutex_unlock (&g_devctl_mutex);
#endif
               return -EFAULT;
            }
            Abort (FdoData);
            stm_dma_abort (FdoData);
            if (ctx.in.i32Mode != 0) {
               /* in.i32Mode = 0  ====>  Disable Streaming
                * in.i32Mode > 0  ====>  Enable Streaming
                * in.i32Mode < 0  ====>  Reset Stream params */
               clr_flag_ (streamp, flags, STMF_DMA_XFER);
               clr_flag_ (streamp, flags, STMF_FIFOFULL);
               clr_flag_ (streamp, flags, STMF_STARTED);
               set_flag_ (streamp, flags, STMF_ENABLED);
            }
            else {
               clr_flag_ (streamp, flags, STMF_ENABLED);
            }
         }
         break;

      case CS_IOCTL_WAIT_STREAM_DMA_DONE:
         {
            int ok = 1, errcode = -EFAULT;
            WAIT_STREAM_DMA_DONE ctx;

            if (copy_from_user (&ctx, (void*)ioctl_arg, sizeof(WAIT_STREAM_DMA_DONE)) ||
                ctx.in.u32TimeoutMs < 0) {
               ok = !ok;
               errcode = -EINVAL;
            }
            if (ok &&
                (!is_flag_set_ (&FdoData->Stream, flags, STMF_ENABLED) ||
                  is_flag_set_ (&FdoData->Stream, flags, STMF_DMA_ABORT))) {
               ok = !ok;
               errcode = -EPERM;
            }
#if 0
            /* fifo-overflow is only an indicator to be returned, never to block
             * or fail an operation. we want to wait for dma completion still */
            if (ok && is_flag_set_ (&FdoData->Stream, flags, STMF_FIFOFULL)) {
               ok = !ok;
               errcode = -ENOMEM;
            }
#endif
            if (ok) {
               if (!is_flag_set_ (&FdoData->Stream, flags, STMF_DMA_XFER)) {
                  /* nothing to do; no dma in-progress, or had just finished */
                  if (is_flag_set_ (&FdoData->Stream, flags, STMF_FIFOFULL)) {
                     ok = !ok;
                     errcode = -ENOMEM;
                  }
               }
               else {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
                  mutex_unlock (&g_devctl_mutex);
#endif
                  return stm_dma_wait (FdoData, ctx.in.u32TimeoutMs);
               }
            }
            if (!ok) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
               mutex_unlock (&g_devctl_mutex);
#endif
               return errcode;
            }
         }
         break;

      default:
         GEN_PRINT("UNKOWN IOCTL (0x%08x) ????????????? \n", IoctlCmd);
         break;
   }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
   mutex_unlock(&g_devctl_mutex);
#endif
   return 0;
}


/**************************************************************************
 *
 **************************************************************************/

int32   GageLockUserBuffer(PFDO_DATA FdoData, void *pBuffer, uInt32 u32BuffSize)
{
   struct task_struct *tsk;
   int      iRet;

   unsigned long first = 0;
   unsigned long last = 0;
   unsigned long offset = 0;
   unsigned long nr_pages = 0;
   struct page *Page = NULL;
   ULONG   u32BuffAddr = (ULONG) pBuffer;

   int err = 0;
   int x = 0;


   GEN_DEBUG_PRINT("BufferAddr =  0x%lx\n", u32BuffAddr);
   GEN_DEBUG_PRINT("BufferSize =  0x%x\n", u32BuffSize);

   first = (u32BuffAddr & PAGE_MASK) >> PAGE_SHIFT;
   last = ((u32BuffAddr + u32BuffSize - 1) & PAGE_MASK) >> PAGE_SHIFT;
   offset = u32BuffAddr & ~PAGE_MASK;
   nr_pages = last - first + 1;
   if (nr_pages > MAX_DMA_PAGES)
   {
      GEN_PRINT("Pages too big %ld, using 256\n", nr_pages);
      nr_pages = MAX_DMA_PAGES;
   }

   tsk   = get_current();
   down_read(&current->mm->mmap_sem);

   iRet = get_user_pages(
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0))
                          tsk,
                          tsk->mm,
#endif
                          u32BuffAddr & PAGE_MASK,
                          nr_pages,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,9,0))
                          1, 0,
#else
                          FOLL_WRITE,
#endif
                          FdoData->PageList,
                          NULL );
   up_read(&current->mm->mmap_sem);

   if (iRet != nr_pages)
   {
      nr_pages = (err >= 0) ? err : 0;
      GEN_PRINT("get_user_pages: err=%d [%ld]\n", iRet, nr_pages);
      return err < 0 ? err : -EINVAL;
   }

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))

   for (x = 0; x < nr_pages; x++)
   {
      Page = FdoData->PageList[x];
      if (!Page || IS_ERR(Page))
      {
         GEN_PRINT("Missing page in DumpUserPages %d\n", x);
      }
      else
      {
         FdoData->SgList[x].page = Page;
         FdoData->SgList[x].length = PAGE_SIZE;
         if (1 == nr_pages)
         {
            FdoData->SgList[x].offset = offset;
            FdoData->SgList[x].length = u32BuffSize;
         }
         else if (0 == x)
         {
            FdoData->SgList[x].offset = offset;
            FdoData->SgList[x].length = PAGE_SIZE - (u32BuffAddr & ~PAGE_MASK);
         }
         else
         {
            FdoData->SgList[x].offset = 0;
            if (x == nr_pages-1)
               FdoData->SgList[x].length = ((u32BuffAddr + u32BuffSize) & ~PAGE_MASK);
            else
               FdoData->SgList[x].length = PAGE_SIZE;
         }

         GEN_DEBUG_PRINT("Page%d Offset =  0x%lx\n", x, FdoData->SgList[x].offset);
         GEN_DEBUG_PRINT("Page%d Length =  0x%lx\n", x, FdoData->SgList[x].length);
      }
   }
#else

   sg_init_table(FdoData->SgList, nr_pages);//RG MOVE TO WHERE WE CALL KCALLOC ??

   for (x = 0; x < nr_pages; x++)
   {
      Page = FdoData->PageList[x];
      if (!Page || IS_ERR(Page))
      {
         GEN_PRINT("Missing page in DumpUserPages %d\n", x);
      }
      else
      {
         //         FdoData->SgList[x].page = Page;
         //         FdoData->SgList[x].length = PAGE_SIZE;
         if (1 == nr_pages)
         {
            //            FdoData->SgList[x].offset = offset;
            //            FdoData->SgList[x].length = u32BuffSize;
            sg_set_page(&FdoData->SgList[x], Page, u32BuffSize, offset);
         }
         else if (0 == x)
         {
            //            FdoData->SgList[x].offset = offset;
            //            FdoData->SgList[x].length = PAGE_SIZE - (u32BuffAddr & ~PAGE_MASK);
            sg_set_page(&FdoData->SgList[x], Page, PAGE_SIZE - (u32BuffAddr & ~PAGE_MASK), offset);
         }
         else
         {
            //            FdoData->SgList[x].offset = 0;
            if (x == nr_pages-1)
               sg_set_page(&FdoData->SgList[x], Page, ((u32BuffAddr + u32BuffSize) & ~PAGE_MASK), 0);
            //               FdoData->SgList[x].length = ((u32BuffAddr + u32BuffSize) & ~PAGE_MASK);
            else
               sg_set_page(&FdoData->SgList[x], Page, PAGE_SIZE, 0);
            //               FdoData->SgList[x].length = PAGE_SIZE;
         }

         //         GEN_DEBUG_PRINT("Page%d Offset =  0x%lx\n", x, FdoData->SgList[x].offset);
         //         GEN_DEBUG_PRINT("Page%d Length =  0x%lx\n", x, FdoData->SgList[x].length);
      }
   }
#endif // < KERNEL_VERSION(2,6,24)


   dma_map_sg(FdoData->Device, FdoData->SgList, nr_pages, DMA_FROM_DEVICE);

   FdoData->u32PageLocked = nr_pages;

   return FdoData->u32PageLocked;
}


/**************************************************************************
 *
 **************************************************************************/
void   GageUnlockUserBuffer(PFDO_DATA FdoData)
{
   uInt32      x;
   struct page *Page = NULL;

   if ( FdoData->u32PageLocked != 0 )
      dma_unmap_sg(FdoData->Device, FdoData->SgList, FdoData->u32PageLocked, DMA_FROM_DEVICE);

   for (x = 0; x < FdoData->u32PageLocked; x++)
   {
      Page = FdoData->PageList[x];

      if ( !PageReserved(Page) )
         SetPageDirty(Page);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0))
      page_cache_release( Page );
#else
      put_page( Page );
#endif
   }

   FdoData->u32PageLocked = 0;
}


/**************************************************************************
 *
 **************************************************************************/

ssize_t   GageRead (struct file *filp, char __user *pBuffer, size_t Size, loff_t *Offset )
{
   struct task_struct *tsk;
   int      iRet;
   PFDO_DATA    FdoData = (PFDO_DATA) filp->private_data;
   ULONG   u32BuffAddr = (ULONG) pBuffer;
   UINT   u32BuffSize = (UINT) Size;

   unsigned long offset = 0;
   unsigned long nr_pages = 0;

   struct page *Page = NULL;

   int err = 0;
   int x = 0;

   // TRACEIN;

#ifdef GEN_DEBUG_DMA
   GEN_DEBUG_PRINT("BufferAddr =  0x%lx\n", u32BuffAddr);
   GEN_DEBUG_PRINT("BufferSize =  0x%x\n", u32BuffSize);
#endif

   offset = u32BuffAddr & ~PAGE_MASK;
   nr_pages = PAGE_ALIGN(u32BuffSize + offset) >> PAGE_SHIFT;
#ifdef GEN_DEBUG_DMA
   GEN_PRINT("offset 0x%lx, nr_pages %ld\n", offset, nr_pages);
#endif
   if (nr_pages > MAX_DMA_PAGES)
   {
      GEN_PRINT("Pages too big %ld, using 256\n", nr_pages);
      nr_pages = MAX_DMA_PAGES;
   }
   /*
      FdoData->PageList = kmalloc(nr_pages * sizeof(struct page *), GFP_KERNEL);
      if (NULL == FdoData->PageList)
      {
      GEN_PRINT("KMALLOC for Pages failed in DumpUserPages %ld %p %p %p\n",
      nr_pages, (void *) first, (void *) last, (void *) offset);
      return -ENOMEM;
      }
   //   memset(FdoData->PageList, 0, nr_pages * sizeof(struct page *));

   FdoData->SgList = kmalloc(nr_pages * sizeof(struct scatterlist), GFP_KERNEL);
   if (NULL == FdoData->SgList)
   {
   GEN_PRINT("KMALLOC for scatterlist failed in DumpUserPages %ld %p %p %p\n",
   nr_pages, (void *) first, (void *) last, (void *) offset);
   return -ENOMEM;
   }
   //   memset(FdoData->SgList, 0, nr_pages * sizeof(struct scatterlist));
   */

   tsk   = get_current();
   down_read(&current->mm->mmap_sem);

   iRet = get_user_pages(
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0))
                          tsk,
                          tsk->mm,
#endif
                          u32BuffAddr & PAGE_MASK,
                          nr_pages,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,9,0))
                          1, 0,
#else
                          FOLL_WRITE,
#endif
                          FdoData->PageList,
                          NULL );
   up_read(&current->mm->mmap_sem);

   if (iRet != nr_pages)
   {
      nr_pages = (err >= 0) ? err : 0;
      GEN_PRINT("get_user_pages: err=%d [%ld]\n", iRet, nr_pages);
      return err < 0 ? err : -EINVAL;
   }

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))

   for (x = 0; x < nr_pages; x++)
   {
      Page = FdoData->PageList[x];
      if (!Page || IS_ERR(Page))
      {
         GEN_PRINT("Missing page in DumpUserPages %d\n", x);
      }
      else
      {
         FdoData->SgList[x].page = Page;
         FdoData->SgList[x].length = PAGE_SIZE;
         if (1 == nr_pages)
         {
            FdoData->SgList[x].offset = offset;
            FdoData->SgList[x].length = u32BuffSize;
         }
         else if (0 == x)
         {
            FdoData->SgList[x].offset = offset;
            FdoData->SgList[x].length = PAGE_SIZE - (u32BuffAddr & ~PAGE_MASK);
         }
         else
         {
            FdoData->SgList[x].offset = 0;
            if (x == nr_pages-1 && ((u32BuffAddr + u32BuffSize) & ~PAGE_MASK))
               FdoData->SgList[x].length = ((u32BuffAddr + u32BuffSize) & ~PAGE_MASK);
            else
               FdoData->SgList[x].length = PAGE_SIZE;
         }

#ifdef GEN_DEBUG_DMA
         GEN_DEBUG_PRINT("Page%d Offset =  0x%lx\n", x, FdoData->SgList[x].offset);
         GEN_DEBUG_PRINT("Page%d Length =  0x%lx\n", x, FdoData->SgList[x].length);
#endif
      }
   }

#else
   sg_init_table(FdoData->SgList, nr_pages);//RG MOVE TO WHERE WE CALL KCALLOC ??

   for (x = 0; x < nr_pages; x++)
   {
      Page = FdoData->PageList[x];
      if (!Page || IS_ERR(Page))
      {
         GEN_PRINT("Missing page in DumpUserPages %d\n", x);
      }
      else
      {
         if (1 == nr_pages)
         {
            sg_set_page(&FdoData->SgList[x], Page, u32BuffSize, offset);
         }
         else if (0 == x)
         {
            sg_set_page(&FdoData->SgList[x], Page, PAGE_SIZE - offset, offset);
         }
         else
         {
            if (x == nr_pages-1 && ((u32BuffAddr + u32BuffSize) & ~PAGE_MASK))
               sg_set_page(&FdoData->SgList[x], Page, ((u32BuffAddr + u32BuffSize) & ~PAGE_MASK), 0);
            else
               sg_set_page(&FdoData->SgList[x], Page, PAGE_SIZE, 0);
         }

#ifdef GEN_DEBUG_DMA
         GEN_DEBUG_PRINT("Page%d Offset =  0x%lx\n", x,offset);
         GEN_DEBUG_PRINT("Page%d Length =  0x%x\n", x, FdoData->SgList[x].length);
#endif
      }
   }

#endif


   dma_map_sg(FdoData->Device, FdoData->SgList, nr_pages, DMA_FROM_DEVICE);

   if ( FdoData->CardState.bPciExpress )
      FdoData->pfnAbortDmaTransfer(FdoData);

   if ( FdoData->bDataTransferIntrEn )
      FdoData->pfnEnableInterruptDMA( FdoData );
   else
      FdoData->pfnDisableInterruptDMA( FdoData );

   FdoData->pfnBuildDmaTransferDescriptor(FdoData, FdoData->SgList, nr_pages);

   FdoData->bDmaIntrWait = TRUE;

   FdoData->pfnStartDmaTransferDescriptor(FdoData);

   if ( FdoData->bDataTransferIntrEn )
   {
      iRet = wait_event_interruptible_timeout(FdoData->DmaWaitQueue, ! FdoData->bDmaIntrWait,  4*HZ);

      GEN_DEBUG_PRINT("wait_event_interruptible_timeout  return = %d\n", iRet);
      if ( 0 == iRet )
         GEN_PRINT("Error DMA timeout (DMA Intr not signal)\n");
      else
      {
         if ( iRet > 0 )
            GEN_DEBUG_PRINT("Wait DMA interrupted\n");
      }
   }
   else
   {
      if ( ! FdoData->pfnCheckBlockTransferDone(FdoData, TRUE) )
         GEN_PRINT("Error DMA timeout (Status timeout)\n");
      else
         FdoData->XferRead.u32ByteTransfered = FdoData->XferRead.u32CurrentDmaSize;
   }

   if ( FdoData->CardState.bPciExpress )
      FdoData->pfnAbortDmaTransfer( FdoData );

   dma_unmap_sg(FdoData->Device, FdoData->SgList, nr_pages, DMA_FROM_DEVICE);

   for (x = 0; x < nr_pages; x++)
   {
      Page = FdoData->PageList[x];

      if ( !PageReserved(Page) )
         SetPageDirty(Page);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0))
      page_cache_release( Page );
#else
      put_page( Page );
#endif
   }

   //   kfree(FdoData->PageList);
   //   kfree(FdoData->SgList);
   //   FdoData->PageList = NULL;
   //   FdoData->SgList = NULL;
   // TRACEOUT;
   return FdoData->XferRead.u32ByteTransfered;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VOID SignalEvents (PFDO_DATA  FdoData)
{
   struct siginfo info;
   struct task_struct *my=0;

   memset (&info, 0, sizeof(struct siginfo));
   info.si_signo = CS_EVENT_SIGNAL;
   info.si_code  = SI_QUEUE;

   if (!FdoData->pid)
      return;
   rcu_read_lock();
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
   my = pid_task (find_vpid (FdoData->pid), PIDTYPE_PID);
#else
   my = find_task_by_pid (tpid);
#endif
   rcu_read_unlock();
   if (!my) {
      GEN_DEBUG_PRINT ("SignalEvents: cannot find pid %d on bTriggered\n", (int)tpid);
      return;
   }
   if (FdoData->bTriggered) {
      info.si_int = SIGNAL_TRIGGER;
      send_sig_info (CS_EVENT_SIGNAL, &info, my);
      FdoData->bTriggered = FALSE;
   }
   if (FdoData->bEndOfBusy) {
      info.si_int = SIGNAL_ENDOFBUSY;
      send_sig_info (CS_EVENT_SIGNAL, &info, my);
      FdoData->bEndOfBusy = FALSE;
   }
#if 0
   int RetVal = 0;
   int tpid = (FdoData->tskEvent && FdoData->tskEvent->pid)? FdoData->tskEvent->pid : 0;
   if ( FdoData->bTriggered )//---- busy, so received trigger interrupt
   {
      //----- Signal registered events for trigger
      if ( tpid )
      {
         memset(&info, 0, sizeof(struct siginfo));

         info.si_signo = CS_EVENT_SIGNAL;
         info.si_errno = 0;
         info.si_code  = SI_QUEUE;
         info.si_int   = SIGNAL_TRIGGER;

         rcu_read_lock();
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
         my = pid_task(find_vpid(tpid), PIDTYPE_PID);
#else
         my = find_task_by_pid(tpid);
#endif
         if (!my) {
            GEN_DEBUG_PRINT ("SignalEvents: cannot find pid %d on bTriggered\n",
                             (int)tpid);
            rcu_read_unlock();
            return;
         }
         RetVal = send_sig_info(CS_EVENT_SIGNAL, &info, my);
         rcu_read_unlock();
      }
      FdoData->bTriggered = FALSE;
   }

   if ( FdoData->bEndOfBusy )//---- received End of busy Interrupt
   {
      //----- Signal registered events for busy
      if ( FdoData->tskEvent )
      {
         memset(&info, 0, sizeof(struct siginfo));

         info.si_signo = CS_EVENT_SIGNAL;
         info.si_errno = 0;
         info.si_code  = SI_QUEUE;
         info.si_int   = SIGNAL_ENDOFBUSY;

         rcu_read_lock();
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
         my = pid_task(find_vpid(tpid), PIDTYPE_PID);
#else
         my = find_task_by_pid(tpid);
#endif
         if (!my) {
            GEN_DEBUG_PRINT ("SignalEvents: cannot find pid %d on bEndOfBusy\n",
                             (int)tpid);
            rcu_read_unlock();
            return;
         }
         RetVal = send_sig_info(CS_EVENT_SIGNAL, &info, my);
         rcu_read_unlock();
      }
      FdoData->bEndOfBusy = FALSE;
   }
#endif
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void GageGetDeviceList (PFDO_DATA **listpp, uInt32 *sizep)
{
   *listpp = g_FdoTable;
   *sizep = g_CardsCount.u32NumOfCards;
}

