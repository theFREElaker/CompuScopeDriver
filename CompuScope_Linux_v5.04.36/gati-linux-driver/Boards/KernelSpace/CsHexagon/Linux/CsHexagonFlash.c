#include "HexagonGageDrv.h"
#include "CsTypes.h"
#include "CsPlxDefines.h"

#define HEXAGONFLASH_SECTOR_SIZE   0x80000   // The Hexagon Flash sector in bytes
                  // Hw Sector Size = 128K x 2 (WORD=2bytes) x 2 Flashs in prrallele

#define FLASH_ADDR0_OFFSET   0
#define FLASH_ADDR1_OFFSET   1
#define FLASH_ADDR2_OFFSET   2
#define FLASH_ADDR3_OFFSET   3
#define FLASH_DATA0_OFFSET   4
#define FLASH_DATA1_OFFSET   5
#define FLASH_STATUS_OFFSET  6
#define FLASH_CMD_OFFSET     6

#define WRITE_FLASH_COMMAND(FdoData, u32FlashCmd) WriteGioCpld(FdoData, FLASH_CMD_OFFSET, (u32FlashCmd & 3))

//-----------------------------------------------------------------------------
// Hexagon
//-----------------------------------------------------------------------------

void   WriteGioCpld (PFDO_DATA FdoData, uInt32 u32Addr, uInt32 u32Value)
{
   uInt32   u32Result=0;
   int   count=20;

   //retry count min value = (host clock freq/config clock freq)* (latency + 3) = 250/100 * 6 = 15
   //all the state machines are non blocking (normal HW)
   do
   {
      //check if state machine is ready
      u32Result = ReadGageRegister (FdoData, FLASH_REG_OFFSET);
      if (--count < 0)
         break;   //should never happen but avoid lockdown if HW malfunction
   } while ((u32Result & io_ready_bit) == 0);


   WriteGageRegister (FdoData, FLASH_REG_OFFSET, (u32Value & 0xffff) | ((u32Addr & 0xf) << 16) | io_write_strobe_bit);

   count=20;
   do
   {
      //check if state machine is done
      u32Result = ReadGageRegister (FdoData, FLASH_REG_OFFSET);
      if (--count < 0)
         break;   //should never happen but avoid lockdown if HW malfunction
   } while ((u32Result & io_ready_bit) == 0);

   /*
      if (count < 0)
      GEN_DEBUG_PRINT (("WriteGioCpld - Error at addr: (0x%x), Command: (0x%x)\n",
      FLASH_REG_OFFSET, (u32Value & 0xffff) | ((u32Addr & 0xf) << 16) | io_write_strobe_bit));
      else
      GEN_DEBUG_PRINT (("WriteGioCpld - Completed addr: (0x%x), Command: (0x%x)\n",
      FLASH_REG_OFFSET, (u32Value & 0xffff) | ((u32Addr & 0xf) << 16) | io_write_strobe_bit));
      */
}

//-----------------------------------------------------------------------------
// Hexagon
//-----------------------------------------------------------------------------
uInt32   ReadGioCpld (PFDO_DATA FdoData, uInt32 u32Addr)
{
   uInt32   u32Result=0;
   int   count=20;

   //retry count min value = (host clock freq/config clock freq)* (latency + 3) = 250/100 * 6 = 15
   //all the state machines are non blocking (normal HW)
   do
   {
      //check if state machine is ready
      u32Result = ReadGageRegister (FdoData, FLASH_REG_OFFSET);
      if (--count < 0)
         break;   //should never happen but avoid lockdown if HW malfunction
   } while ((u32Result & io_ready_bit) == 0);


   WriteGageRegister (FdoData, FLASH_REG_OFFSET, ((u32Addr & 0xf) << 16) | io_read_enable_bit);

   // Patch for A3 FPGA
   // Dummy read to flush the previous write before checking the io_ready_bit
   u32Result = ReadGageRegister( FdoData, FLASH_REG_OFFSET );

   count=20;
   do
   {
      //check if state machine is done
      u32Result = ReadGageRegister (FdoData, FLASH_REG_OFFSET);
      if (--count < 0)
         break;   //should never happen but avoid lockdown if HW malfunction
   } while ((u32Result & io_ready_bit) == 0);

   /*
      if (count < 0)
      GEN_DEBUG_PRINT (("ReadGioCpld - Error at addr: (0x%x), Command: (0x%x)\n",
      FLASH_REG_OFFSET, ((u32Addr & 0xf) << 16) | io_read_enable_bit));
      else
      GEN_DEBUG_PRINT (("ReadGioCpld - Completed addr: (0x%x), Command: (0x%x)\n",
      FLASH_REG_OFFSET, ((u32Addr & 0xf) << 16) | io_read_enable_bit));
      */
   return u32Result;
}


//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
void   HardResetFlash (PFDO_DATA FdoData)
{
   WriteGageRegister (FdoData, FLASH_OFFSET, 0x0400000);
   WriteGageRegister (FdoData, FLASH_OFFSET, 0);
}

//-----------------------------------------------------------------
// Hexagon
//-----------------------------------------------------------------
void WriteFlashData (PFDO_DATA FdoData, uInt32 u32Data)
{
   WriteGioCpld (FdoData, FLASH_DATA0_OFFSET, u32Data & 0xFFFF);
   WriteGioCpld (FdoData, FLASH_DATA1_OFFSET, (u32Data >> 16) & 0xFFFF);
}

//-----------------------------------------------------------------
// Hexagon
//-----------------------------------------------------------------
uInt32   ReadFlashData (PFDO_DATA FdoData)
{
   uInt32 u32Data = 0xFFFF & ReadGioCpld (FdoData, FLASH_DATA0_OFFSET);

   u32Data |= (ReadGioCpld (FdoData, FLASH_DATA1_OFFSET) << 16);

   return u32Data;
}

//-----------------------------------------------------------------
// Hexagon
//-----------------------------------------------------------------
void WriteFlashAddress (PFDO_DATA FdoData, uInt32 u32Addr)
{
   WriteGioCpld (FdoData, FLASH_ADDR0_OFFSET, u32Addr & 0xFFFF);
   WriteGioCpld (FdoData, FLASH_ADDR1_OFFSET, (u32Addr >> 16) & 0xFFFF);
}

//-----------------------------------------------------------------
// Hexagon
//-----------------------------------------------------------------
void WriteFlashCommand (PFDO_DATA FdoData, uInt32 u32FlashCmd)
{
   WriteGioCpld (FdoData, FLASH_CMD_OFFSET, u32FlashCmd & 3);
}

//-----------------------------------------------------------------
// Hexagon
//-----------------------------------------------------------------
uInt32   ReadFlashSMStatus (PFDO_DATA FdoData)
{
   uInt32   u32Result = 0;

   u32Result = ReadGioCpld (FdoData, FLASH_CMD_OFFSET);

   return (0xF & u32Result);
}

#define   FLASH_CMD_IDLE   0x0
#define   FLASH_CMD_READ   0x1
#define   FLASH_CMD_WRITE   0x2

#define   FLASH_STATE_READY   0x1
#define   FLASH_STATE_IDLE   0x2
#define   FLASH_STATE_DONE   0x4

//-----------------------------------------------------------------
// Hexagon
//-----------------------------------------------------------------
int32 FlashReadValue (PFDO_DATA FdoData, uInt32 u32Addr, uInt32 *pu32Data)
{
   /* int32   i32Status; */
   uInt32   u32State = 0;
   int32   count = 0x10000;

#ifdef CHECK_FLASH_ADDRESS
   // Make sure register offset is valid
   if (u32Addr >= 0x10000000)
   {
      GEN_DEBUG_PRINT (("FlashReadValue ERROR - invalid Flash Address (0x%x)\n", u32Addr));
      *pu32Data = -1;
      return CS_FLASH_INVALID_SECTOR;
   }
#endif

   u32State = ReadGioCpld (FdoData, FLASH_STATUS_OFFSET);
   if (! (u32State & FLASH_STATE_IDLE))
      WRITE_FLASH_COMMAND (FdoData, FLASH_CMD_IDLE);

   WriteFlashAddress (FdoData, u32Addr);

   u32State = ReadGioCpld (FdoData, FLASH_STATUS_OFFSET);
   while (! (u32State & FLASH_STATE_IDLE))
   {
      u32State = ReadGioCpld (FdoData, FLASH_STATUS_OFFSET);
      if (count-- < 0)
      {
         GEN_DEBUG_PRINT (("ERROR - Read fails at Flash Address (0x%x)\n", u32Addr));
         return CS_FLASH_DATAREAD_ERROR;
      }
   }

   WRITE_FLASH_COMMAND (FdoData, FLASH_CMD_READ);   //write command READ

   count = 0x10000;

   u32State = ReadGioCpld (FdoData, FLASH_STATUS_OFFSET);
   while (! (u32State & FLASH_STATE_DONE))
   {
      u32State = ReadGioCpld (FdoData, FLASH_STATUS_OFFSET);
      if (u32State & FLASH_STATE_IDLE)
      {   //repeat if idle only
         WriteGioCpld (FdoData, FLASH_CMD_OFFSET, FLASH_CMD_READ);   //write command READ
         GEN_DEBUG_PRINT (("ERROR - Should be DONE by now!\n"));
      }
      if (count-- < 0)
      {
         GEN_DEBUG_PRINT (("ERROR - Read fails at Flash Address (0x%x)\n", u32Addr));
         return CS_FLASH_DATAREAD_ERROR;
      }
   }

   //If in DONE state, read data and send to IDLE
   *pu32Data = ReadFlashData (FdoData);

   WriteGioCpld (FdoData, FLASH_CMD_OFFSET, FLASH_CMD_IDLE);

   //   GEN_DEBUG_PRINT (("FLASH_READ - END - Flash Address (0x%x) - Data: (0x%x)\n", u32Addr, *pu32Data));

   return CS_SUCCESS;
}

//-----------------------------------------------------------------
// Hexagon
//-----------------------------------------------------------------
int32 FlashWriteValue (PFDO_DATA FdoData, uInt32 u32Addr, uInt32 u32Val)
{
   /* int32   i32Status; */
   uInt32   u32State;
   int32   count = 0x10000;

#ifdef CHECK_FLASH_ADDRESS
   // Make sure register offset is valid
   if (u32Addr >= 0x10000000)
   {
      GEN_DEBUG_PRINT (("FlashWriteValue ERROR - invalid Flash Address (0x%x)\n", u32Addr));
      return CS_FLASH_INVALID_SECTOR;
   }
#endif

   WriteFlashAddress (FdoData, u32Addr);

   u32State = ReadGioCpld (FdoData, FLASH_STATUS_OFFSET);
   while (! (u32State & FLASH_STATE_IDLE))
   {
      u32State = ReadGioCpld (FdoData, FLASH_STATUS_OFFSET);
      if (u32State & FLASH_STATE_DONE)
      {
         WriteGioCpld (FdoData, FLASH_CMD_OFFSET, FLASH_CMD_IDLE);
         GEN_DEBUG_PRINT (("ERROR - Should be IDLE by now!\n"));
      }

      if (count-- < 0)
      {
         GEN_DEBUG_PRINT (("ERROR - Write fails at Flash Address (0x%x)\n", u32Addr));
         return CS_FLASH_DATAWRITE_ERROR;
      }
   }

   WriteFlashData (FdoData, u32Val);           //write data
   u32State = ReadGioCpld (FdoData, FLASH_STATUS_OFFSET);
   while (! (u32State & FLASH_STATE_IDLE))
   {
      u32State = ReadGioCpld (FdoData, FLASH_STATUS_OFFSET);
      if (u32State & FLASH_STATE_DONE)
      {
         WriteGioCpld (FdoData, FLASH_CMD_OFFSET, FLASH_CMD_IDLE);
         GEN_DEBUG_PRINT (("ERROR - Should be IDLE by now!\n"));
      }

      if (count-- < 0)
      {
         GEN_DEBUG_PRINT (("ERROR - Write fails at Flash Address (0x%x)\n", u32Addr));
         return CS_FLASH_DATAWRITE_ERROR;
      }
   }

   WriteGioCpld (FdoData, FLASH_CMD_OFFSET, FLASH_CMD_WRITE);

   count = 0x10000;

   u32State = ReadGioCpld (FdoData, FLASH_STATUS_OFFSET);
   while (! (u32State & FLASH_STATE_DONE))
   {
      u32State = ReadGioCpld (FdoData, FLASH_STATUS_OFFSET);
      if (u32State & FLASH_STATE_IDLE)
      {   //repeat if idle only
         WriteGioCpld (FdoData, FLASH_CMD_OFFSET, FLASH_CMD_WRITE);
         GEN_DEBUG_PRINT (("ERROR - Should be DONE by now!\n"));
      }
      if (count-- < 0)
      {
         GEN_DEBUG_PRINT (("ERROR - Read fails at Flash Address (0x%x)\n", u32Addr));
         return CS_FLASH_DATAWRITE_ERROR;
      }
   }

   WriteGioCpld (FdoData, FLASH_CMD_OFFSET, FLASH_CMD_IDLE);
   //   GEN_DEBUG_PRINT (("FLASH_WRITE - END - Flash Address (0x%x) - Data: (0x%x)\n", u32Addr, u32Val));

   return CS_SUCCESS;
}

//-----------------------------------------------------------------
// Hexagon
//-----------------------------------------------------------------
uInt32 ReadFlashProgramStatus (PFDO_DATA FdoData, uInt32 u32Addr)
{
   int32   i32Status;
   uInt32   u32Value;

   i32Status = FlashReadValue (FdoData, u32Addr, &u32Value);

   if (u32Value != 0x800080)
      return 1;
   else
      return 0;
}

//-----------------------------------------------------------------
// Hexagon (to be reviewed)
//-----------------------------------------------------------------
int32   FlashProgramWord (PFDO_DATA FdoData, uInt32 u32Addr, uInt32 u32Val)
{
   int32   i32Status = CS_SUCCESS;
   uInt32   u32State;
   int32   count = 0x10000;


#ifdef CHECK_FLASH_ADDRESS
   // Make sure register offset is valid
   if (u32Addr >= 0x10000000)
   {
      GEN_DEBUG_PRINT (("ERROR - invalid Flash Address (0x%x)\n", u32Addr));
      return CS_FLASH_INVALID_SECTOR;
   }
#endif

   count = 0x10000;

   // Write program command
   i32Status = FlashWriteValue (FdoData, u32Addr, 0x00410041);
   if (CS_FAILED (i32Status))
      return i32Status;

   // Write program data to address
   i32Status = FlashWriteValue (FdoData, u32Addr, u32Val);
   if (CS_FAILED (i32Status))
      return i32Status;

   u32State = ReadFlashProgramStatus (FdoData, u32Addr);
   while (u32State)
   {
      //could loop for ever!
      u32State = ReadFlashProgramStatus (FdoData, u32Addr);
      if (count-- < 0)
      {
         GEN_DEBUG_PRINT (("ERROR - Program fails at Flash Address (0x%x): State=0x%x, count = %d\n", u32Addr, u32State, count));
         return CS_FLASH_DATAWRITE_ERROR;
      }
   }

   FlashWriteValue (FdoData, u32Addr, 0xFFFFFFFF); // Switch to read mode

   return CS_SUCCESS;
}

//-----------------------------------------------------------------
// Hexagon
// Write to the base board Flash withtin the same sector
// The Sector (block) should have been previously erased with FlashBlockErase ()
//-----------------------------------------------------------------
int32   FlashProgramBuffer (PFDO_DATA FdoData, uInt32 u32WordAddr, void *pBuffer, uInt32 u32BufferSize)
{
   int32   i32Status = CS_SUCCESS;
   uInt32   u32State;
   int32   count = 0x10000;
   uInt32   u32WordCount =  u32BufferSize >> 2;
   uInt32   u32Addr =  u32WordAddr;
   uInt32   u32WriteCount =  0;
   uInt32   *pu32Buffer = (uInt32 *) pBuffer;
   uInt32   u32Val = 0;
   uInt32   u32StartSectorAddr = (4*u32WordAddr/HEXAGONFLASH_SECTOR_SIZE)*HEXAGONFLASH_SECTOR_SIZE;
   uInt32   u32EndSectorAddr = ((4*u32WordAddr + u32BufferSize - 1)/HEXAGONFLASH_SECTOR_SIZE)*HEXAGONFLASH_SECTOR_SIZE;

   /* GEN_DEBUG_PRINT (("FlashProgramBuffer - WordCount = %d\n", u32WordCount)); */

   if (u32StartSectorAddr != u32EndSectorAddr)
      return CS_FLASH_SECTORCROSS_ERROR;

   if (u32WordCount > 512)
      return CS_FLASH_BUFFER_SIZE_ERROR;

   // Write buffer load command
   i32Status = FlashWriteValue (FdoData, u32WordAddr, 0x00E900E9);
   if (CS_FAILED (i32Status))
      return i32Status;

   // Write word count
   i32Status = FlashWriteValue (FdoData, u32WordAddr, (u32WordCount-1) | (( (u32WordCount-1))<<16));
   if (CS_FAILED (i32Status))
      return i32Status;

   // Load buffer
   while (u32WriteCount < u32WordCount )
   {
      u32Val = *pu32Buffer;
      i32Status = FlashWriteValue (FdoData, u32Addr, u32Val);
      if (CS_FAILED (i32Status))
         return i32Status;

      u32Addr++;
      pu32Buffer++;
      u32WriteCount ++;
   }

   i32Status = FlashWriteValue (FdoData, u32WordAddr, 0x0D000D0);
   if (CS_FAILED (i32Status))
      return i32Status;

   //   u32State = ReadFlashProgramStatus (FdoData, u32WordAddr);
   FlashReadValue (FdoData, u32WordAddr, &u32State);
   while (u32State != 0x800080)
   {
      //could loop for ever!
      //   u32State = ReadFlashProgramStatus (FdoData, u32WordAddr);
      FlashReadValue (FdoData, u32WordAddr, &u32State);
      if (count-- < 0)
      {
         GEN_DEBUG_PRINT (("ERROR - FlashProgramBuffer fails at Flash Address (0x%x) count: 0x%x\n", u32WordAddr, count));
         return CS_FLASH_DATAWRITE_ERROR;
      }
   }

   /* GEN_DEBUG_PRINT (("FlashBufferProgram end \n")); */

   return CS_SUCCESS;
}



//-----------------------------------------------------------------
// Hexagon
//-----------------------------------------------------------------
int FlashBlockErase (PFDO_DATA FdoData, uInt32 u32Addr)
{
   uInt32   u32Temp = 0;
   int /* first_read = 0,second_read = 0xffffffff, */locked_stat = 0;
   int count = 0;
#define LOOP_COUNT 1000000

   FlashWriteValue (FdoData, u32Addr, 0x00700070); //autoselect

   // Check if the status of the block. If it is locked then unlock it
   FlashReadValue (FdoData, u32Addr, &locked_stat);
   /* GEN_DEBUG_PRINT (("locked_stat = 0x%x\n", locked_stat)); */
   //if (0 != (locked_stat & 0x020002))
   {
      //   GEN_DEBUG_PRINT (("Sector Blocked !!!\n"));
      // Unlock
      FlashWriteValue (FdoData, u32Addr, 0x00600060);
      FlashWriteValue (FdoData, u32Addr, 0x00D000D0);

      FlashReadValue (FdoData, u32Addr + 2, &locked_stat);
      /* GEN_DEBUG_PRINT (("locked_stat (after unlock) = 0x%x\n", locked_stat)); */
   }

   // Send erase command
   FlashWriteValue (FdoData, u32Addr, 0x00200020);
   FlashWriteValue (FdoData, u32Addr, 0x00D000D0);

   FlashReadValue (FdoData, u32Addr, &u32Temp);
   /* GEN_DEBUG_PRINT (("Erase status = 0x%x\n", u32Temp)); */
   while (( (0x800080 & u32Temp) != 0x800080)&& (count < LOOP_COUNT))
   {
      FlashReadValue (FdoData, u32Addr, &u32Temp);
      count++;//DQ7 should be 1
   }

   FlashWriteValue (FdoData, u32Addr, 0x00FF00FF);   //clear status


   if (count > LOOP_COUNT - 1)
   {
      GEN_DEBUG_PRINT (("Sector Erased FAILED !!!\n"));
      return CS_FLASH_ERASESECTOR_ERROR;
   }
   else
   {
      /* GEN_DEBUG_PRINT (("Sector Erased SUCCEED !!!\n")); */
      return CS_SUCCESS;
   }
}

#define TEST_COUNT  20

//-----------------------------------------------------------------
// Hexagon
//-----------------------------------------------------------------
int32 HexagonFlashRead (PFDO_DATA FdoData,  uInt32 u32Addr, void *pBuffer,  uInt32 u32BufferSize)
{
   /* uInt32   u32ReadCount = 0; */
   /* uInt32   u32Data = 0; */
   uInt32   i = 0;
   uInt32   *pu32Buffer = (uInt32 *) pBuffer;

   //   if (u32BufferSize > HEXAGONFLASH_SECTOR_SIZE)
   //   return CS_FLASH_SECTORCROSS_ERROR;
   if (0!= (u32BufferSize % 4))
      return CS_FLASH_BUFFER_SIZE_ERROR;

   HardResetFlash (FdoData);

   u32Addr >>= 2;   // Word Address
   u32BufferSize >>= 2;
   for (i = 0; i < u32BufferSize; i ++)
   {
      FlashReadValue (FdoData, u32Addr + i, pu32Buffer);
      //   GEN_DEBUG_PRINT (("Data Check = 0x%x\n", *pu32Buffer));
      pu32Buffer++;
   }

   return CS_SUCCESS;
}

//-----------------------------------------------------------------
// Hexagon
//-----------------------------------------------------------------
int32 HexagonFlashWriteSector (PFDO_DATA FdoData,  uInt32 u32Addr, uInt32 u32Offset, void *pBuffer,  uInt32 u32BufferSize)
{
   int32   i32Status = CS_SUCCESS;
   /* uInt32   i = 0; */
   uInt32   u32SectorAddr = 0;
   uInt32   *pu32Buffer = (uInt32 *) pBuffer;
   uInt32   u32ChunkSize = 0;

   if ((u32BufferSize + u32Offset) > HEXAGONFLASH_SECTOR_SIZE)
   {
      return CS_FLASH_SECTORCROSS_ERROR;
   }
   if (0 != (u32BufferSize % 4))
   {
      return CS_FLASH_BUFFER_SIZE_ERROR;
   }
   if ((0 != (u32Addr % 4))||  (0 != (u32Offset % 4)))
   {
      return CS_FLASH_ADDRESS_ERROR;
   }

   // Erase the sector
   u32SectorAddr = (u32Addr/HEXAGONFLASH_SECTOR_SIZE) * HEXAGONFLASH_SECTOR_SIZE;
   u32SectorAddr >>= 2;
   i32Status = FlashBlockErase (FdoData, u32SectorAddr);
   if (CS_FAILED (i32Status))
      return i32Status;

   // Convert to Word address
   // >> 1 byte -> word, >> 1 for 2 flash paralell
   u32Addr >>= 2;

   // Write to the Flash block by block
   // The Maximum size of each block is 2048 bytes
   while (u32BufferSize > 0)
   {
      if (u32BufferSize > 2048)
         u32ChunkSize = 2048;
      else
         u32ChunkSize = u32BufferSize;

      i32Status = FlashProgramBuffer (FdoData, u32Addr, pu32Buffer, u32ChunkSize);
      if (CS_FAILED (i32Status))
      {
         FlashWriteValue (FdoData, u32Addr, 0x00500050);//clear status
         return i32Status;
      }
      pu32Buffer   += u32ChunkSize/sizeof (uInt32);
      u32Addr   += u32ChunkSize/sizeof (uInt32);
      u32BufferSize -=  u32ChunkSize;
   }

   i32Status = FlashWriteValue (FdoData, u32Addr, 0x00500050);//clear status
   return i32Status;
}

//-----------------------------------------------------------------
// Hexagon
//-----------------------------------------------------------------
void   TestFlash (PFDO_DATA FdoData)
{
   uInt32   u32State = 0;
   uInt32   u32Data = 0;
   uInt32   u32Addr = 0;
   uInt32   i = 0;
   uInt32   u32FlashsAddr = 0;

#define TEST_COUNT  20
   uInt32   TestBuffer[TEST_COUNT];

   HardResetFlash (FdoData);
   u32State = ReadGioCpld (FdoData, FLASH_STATUS_OFFSET);
   if (! (u32State & FLASH_STATE_IDLE))
   {
      /* GEN_DEBUG_PRINT (("FLASH not IDLE\n")); */
      WRITE_FLASH_COMMAND (FdoData, FLASH_CMD_IDLE);
   }
   u32State = ReadGioCpld (FdoData, FLASH_STATUS_OFFSET);
   /* GEN_DEBUG_PRINT (("State = 0x%x\n", u32State)); */

   // Verify the manufacturer code is indeed 0x89 for INTEL
   FlashWriteValue (FdoData, 0x0, 0x00900090);
   FlashReadValue (FdoData, 0, &u32Data);
   /* GEN_DEBUG_PRINT (("Data = 0x%x\n", u32Data)); */

   HardResetFlash (FdoData);

   u32Data = 0x87654321;
   //u32Data = 0x12345678;
   for (i = 0; i < TEST_COUNT; i++)
   {
      TestBuffer[i] = u32Data + i;
   }

   // Test write Flash
   u32Addr = 0x4000000;      // Address
   u32FlashsAddr = u32Addr >> 2;   // DWORD address
   FlashBlockErase (FdoData, u32FlashsAddr); // Erase a block of 0x80000 bytes

   u32Data = 0x87654321;
   u32Data = 0x12345678;
   /*
   //   for (i = 0; i < TEST_COUNT; i++)
   for (i = 7; i < 15; i++)
   {
   FlashProgramWord (FdoData, u32FlashsAddr +i, u32Data+i);
   }

   for (i = 7; i < 15; i++)
   {
   FlashProgramWord (FdoData, u32FlashsAddr +i, u32Data+i);
   }
   */

   FlashProgramBuffer (FdoData, u32FlashsAddr, TestBuffer, sizeof (TestBuffer));
   FlashProgramBuffer (FdoData, u32FlashsAddr+0x1000, TestBuffer, sizeof (TestBuffer));
   FlashProgramBuffer (FdoData, u32FlashsAddr+0x2000, TestBuffer, sizeof (TestBuffer));
   FlashProgramBuffer (FdoData, u32FlashsAddr+0x10000, TestBuffer, sizeof (TestBuffer));
   FlashProgramBuffer (FdoData, u32FlashsAddr+0x20000, TestBuffer, sizeof (TestBuffer));
   //   FlashProgramBuffer (FdoData, u32FlashsAddr+0x80000, TestBuffer, sizeof (TestBuffer));

   FlashWriteValue (FdoData, u32FlashsAddr, 0x00500050);//clear status
   HardResetFlash (FdoData);

   for (i = 0; i < TEST_COUNT; i++)
   {
      FlashReadValue (FdoData, u32FlashsAddr +i, &u32Data);
      /* GEN_DEBUG_PRINT (("Data Check = 0x%x\n", u32Data)); */
   }

   HardResetFlash (FdoData);

   /* GEN_DEBUG_PRINT (("-----------------\n", u32Data)); */
   u32Addr = 0x4000000;   // Address*/
   u32FlashsAddr = u32Addr >> 2;
   for (i = 0; i < TEST_COUNT; i++)
   {
      FlashReadValue (FdoData, u32FlashsAddr +i, &u32Data);
      /* GEN_DEBUG_PRINT (("Data Check = 0x%x\n", u32Data)); */
   }


}

