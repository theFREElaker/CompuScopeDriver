/*
 * GageMisc.c
 *
 *  Created on: 4-Sep-2008
 *      Author: quang
 */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>

#if defined (_DECADE_DRV_ )
#  include "DecadeGageDrv.h"
#  include "DecadeNiosApi.h"
#elif defined (_HEXAGON_DRV_ )  
   #  include "HexagonGageDrv.h"
   #  include "HexagonNiosApi.h"
#else
   #  include "GageDrv.h"
   #  include "NiosApi.h"
#endif

#include "GageWrapper.h"
#include "CsDefines.h"

extern PFDO_DATA      g_FdoTable[];
extern CS_CARD_COUNT   g_CardsCount;
extern uInt32      g_u32FreeIndex;

#define MAX_DMA_PAGES 512

void Debug(PFDO_DATA FdoData)
{
   if ( IsNiosInit(FdoData))
      GEN_PRINT("Nios is init\n");
   else
      GEN_PRINT("Nios is not init\n");
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int GageAllocateSoftwareResource( PFDO_DATA FdoData )
{
   int      iRet = 0;

   if (!FdoData->pfnGageAllocateMemoryForDmaDescriptor (FdoData))
   {
      GEN_PRINT("Cannot allocate memory for Dma descriptor\n");
      iRet = ENOMEM;
      goto errorAllocate;
   }
   // Allocate Page and Scatter Gather list
   FdoData->PageList = kmalloc(MAX_DMA_PAGES * sizeof(struct page *), GFP_KERNEL);
   if (NULL == FdoData->PageList)
   {
      GEN_PRINT("KMALLOC for Pages List failed. (Attempt tp allocate %ld bytes)\n",
                MAX_DMA_PAGES * sizeof(struct page *));
      iRet = ENOMEM;
      goto errorAllocate;
   }

   FdoData->SgList = kmalloc(MAX_DMA_PAGES * sizeof(struct scatterlist), GFP_KERNEL);
   if (NULL == FdoData->SgList)
   {
      GEN_PRINT("KMALLOC for SC List failed. (Attempt tp allocate %ld bytes)\n",
                MAX_DMA_PAGES * sizeof(struct scatterlist));
      iRet = ENOMEM;
      goto errorAllocate;
   }

   init_waitqueue_head( &FdoData->DmaWaitQueue);

   /* stm_init (FdoData); */

   return 0;

errorAllocate:

   if (FdoData->PageList)
   {
      kfree(FdoData->PageList);
      FdoData->PageList = NULL;
   }

   if (FdoData->SgList)
   {
      kfree(FdoData->SgList);
      FdoData->SgList = NULL;
   }

   return iRet;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void   GageFreeSoftwareResource( PFDO_DATA FdoData )
{
   TRACEIN;

   FdoData->pfnGageFreeMemoryForDmaDescriptor(FdoData);

   if (FdoData->PageList)
   {
      kfree(FdoData->PageList);
      FdoData->PageList = NULL;
   }

   if (FdoData->SgList)
   {
      kfree(FdoData->SgList);
      FdoData->SgList = NULL;
   }

   stm_cleanup (FdoData);

   TRACEOUT;
}


//-----------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32   SetMasterSlaveLink( PFDO_DATA pMaster, uInt32 u32NumOfSlaves, MASTER_SLAVE_LINK_INFO *pSlaveInfo )
{
   uInt32      i;
   uInt32      j;

   PFDO_DATA   pLast = pMaster;
   BOOLEAN      bHandleFound = FALSE;

   GEN_PRINT ("u32NumOfSlaves = %d\n", u32NumOfSlaves );
   GEN_PRINT ("SlaveHandle = %d  ", pSlaveInfo->CardHandle );
   GEN_PRINT ("Ms cardId = %d\n", pSlaveInfo->u8MsCardId );

   if ( 0 == u32NumOfSlaves )
   {
      // Clear all Master/Slave links
      PFDO_DATA   pCurrent = pMaster;
      while( pLast )
      {
         pCurrent = pLast;
         pLast = pLast->pNextSlave;

         pCurrent->u8MsCardId = 0;
         pCurrent->pMasterIndependent = pCurrent;
         pCurrent->pTriggerMaster = pCurrent;
         pCurrent->pNextSlave = NULL;
      }

      return CS_SUCCESS;
   }
   else
   {
      for ( j = 0; j < u32NumOfSlaves; j ++ )
      {
         bHandleFound = FALSE;

         for ( i = 0; i < g_CardsCount.u32NumOfCards; i ++ )
         {
            if ( g_FdoTable[i]->PseudoHandle == pSlaveInfo[j].CardHandle )
            {
               bHandleFound = TRUE;

               g_FdoTable[i]->u8MsCardId = pSlaveInfo[j].u8MsCardId;

               pLast->pNextSlave = g_FdoTable[i];
               pLast = g_FdoTable[i];
               pLast->pMasterIndependent = pMaster;
               pLast->pTriggerMaster = pMaster;
               break;
            }
         }

         if ( !bHandleFound )
            break;
      }

      if ( bHandleFound )
         return CS_SUCCESS;
      else
      {
         pMaster->pNextSlave = NULL;

         // Roll back, clear all master slave linka
         for ( j = 0; j < u32NumOfSlaves; j ++ )
         {
            for ( i = 0; i < g_CardsCount.u32NumOfCards; i ++ )
            {
               if ( g_FdoTable[i]->PseudoHandle == pSlaveInfo[j].CardHandle )
               {
                  pLast = g_FdoTable[i];
                  pLast->u8MsCardId = 0;
                  pLast->pMasterIndependent = pLast;
                  pLast->pTriggerMaster = pLast;
                  pLast->pNextSlave = NULL;
               }
            }
         }
         return CS_INVALID_HANDLE;
      }
   }
}


//-----------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32   SetMasterSlaveCalibInfo( PFDO_DATA pMaster, uInt32 u32NumOfSlaves, int32 *MsAlignOffset )
{
   uInt32      i = 0;

   PFDO_DATA   pLast = pMaster->pNextSlave;      // Point to the first slave

   while( pLast && (i < u32NumOfSlaves)  )
   {
      pLast->i32MsAlignOffset = MsAlignOffset[i++];
      pLast = pLast->pNextSlave;
   }

   return CS_SUCCESS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int32   SetTriggerMasterLink( PFDO_DATA pMaster, DRVHANDLE TrigMasterHdl )
{
   uInt32      i;

   PFDO_DATA   pCard = pMaster;
   PFDO_DATA   pTrigMaster = NULL;

   for ( i = 0; i < g_CardsCount.u32NumOfCards; i ++ )
   {
      if ( g_FdoTable[i]->PseudoHandle == TrigMasterHdl )
      {
         pTrigMaster = g_FdoTable[i];
         break;
      }
   }

   if ( ! pTrigMaster )
      return CS_INVALID_HANDLE;

   pCard = pMaster;
   while( pCard )
   {
      pCard->pTriggerMaster = pTrigMaster;
      ConfigureInterrupts( pCard, 0 );

      pCard = pCard->pNextSlave;
   }

   return CS_SUCCESS;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32   SendStartAcquisition( PFDO_DATA pMaster, Pin_STARTACQUISITION_PARAMS pAcqParams   )
{
   PFDO_DATA   pCard = pMaster;
   int32      i32Status = CS_SUCCESS;

   // Fixed prefast warnings
   if ( NULL == pMaster || NULL == pAcqParams )
      return CS_MISC_ERROR;

   ConfigureInterrupts( pMaster->pTriggerMaster, pAcqParams->u32IntrMask );
   pMaster->pTriggerMaster->u32DpcIntrStatus = 0;
   pMaster->bEndOfBusy = FALSE;
   pMaster->bTriggered = FALSE;

   while ( pCard )
   {
      pCard->m_u32SegmentRead = 0;

      if( pCard != pCard->pTriggerMaster )
      {
         i32Status = StartAcquisition( pCard, pAcqParams->bRtMode );
         if ( CS_FAILED(i32Status) )
            break;
      }

      pCard = pCard->pNextSlave;
   }

   if ( CS_SUCCEEDED(i32Status) )
      i32Status = StartAcquisition( pMaster->pTriggerMaster, pAcqParams->bRtMode );

   return i32Status;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void Abort( PFDO_DATA pMaster )
{
   PFDO_DATA   pCard = pMaster;

   while ( pCard )
   {
      if( pCard != pCard->pTriggerMaster )
      {
         AbortAcquisition( pCard );
      }

      pCard = pCard->pNextSlave;
   }

   AbortAcquisition( pMaster->pTriggerMaster );

}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void   SetAcquisitionConfig( PFDO_DATA pMaster , PCSACQUISITIONCONFIG_EX pAcqCfgEx )
{
   PFDO_DATA   pCard = pMaster;

   while ( pCard )
   {
      GageCopyMemory( &pCard->AcqCfgEx, pAcqCfgEx, sizeof(CSACQUISITIONCONFIG_EX) );
      pCard = pCard->pNextSlave;
   }
}


