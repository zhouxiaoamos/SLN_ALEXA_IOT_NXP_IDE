/*
 * Copyright 2019-2020 NXP.
 * This software is owned or controlled by NXP and may only be used strictly in accordance with the
 * license terms that accompany it. By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read, and that you
 * agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include "FreeRTOS.h"

#include "flash_map.h"

#include "sln_flash_ops.h"
#include "fsl_flexspi.h"
#include "iot_flash.h"
#include "sln_flash.h"

#if ERASE_PARTIAL_SECTORS
static int32_t erase_region(IotFlashHandle_t const pxFlash, uint32_t ulAddress, size_t xBytes);
#endif /* ERASE_PARTIAL_SECTORS */
static int32_t rmw_pages(bool forceOnes, IotFlashHandle_t const pxFlash, uint32_t ulAddress, uint8_t *const pvBuffer, size_t xBytes);

static IotFlashInfo_t FL_Info;
static bool is_flash_drv_open;

IotFlashHandle_t iot_flash_open(int32_t lInstance)
{
    if (lInstance == -1) {
        return NULL;
    }

    is_flash_drv_open = true;
    return (IotFlashHandle_t) 1;
}

int32_t iot_flash_close(IotFlashHandle_t const pxFlash)
{
    if (is_flash_drv_open != true) {
        return (uint32_t)IOT_FLASH_INVALID_VALUE;
    }

    is_flash_drv_open = false;
    return (uint32_t)IOT_FLASH_SUCCESS;
}

IotFlashInfo_t *iot_flash_getinfo(IotFlashHandle_t const pxFlash)
{
    if (pxFlash == NULL) {
        return NULL;
    }

    FL_Info.ulFlashSize =       FLASH_SIZE;
    // TODO: What is the appropriate Flash Block Size
    FL_Info.ulBlockSize =       SECTOR_SIZE;
    FL_Info.ulSectorSize =      SECTOR_SIZE;
    FL_Info.ulPageSize =        FLASH_PAGE_SIZE;
    FL_Info.ulLockSupportSize = SECTOR_SIZE;
    FL_Info.ucAsyncSupported =  false;

    return &FL_Info;
}

void iot_flash_set_callback(IotFlashHandle_t const pxFlash, IotFlashCallback_t xCallback, void *pvUserContext)
{
    // Unsupported by RT106A
    (void) pxFlash;
    (void) xCallback;
    (void) pvUserContext;
}

int32_t iot_flash_erase_sectors(IotFlashHandle_t const pxFlash, uint32_t ulAddress, size_t lSize)
{
    int32_t status = IOT_FLASH_SUCCESS;
    status_t slnStatus = kStatus_Success;

    if ((NULL == pxFlash) || (NULL == ulAddress) || (FLASH_SIZE <= ulAddress) || (0 == lSize))
    {
        status = IOT_FLASH_INVALID_VALUE;
    }

    if (IOT_FLASH_SUCCESS == status)
    {
      if ((ulAddress % SECTOR_SIZE) != 0 || (lSize % SECTOR_SIZE) != 0)
      {
#if ERASE_PARTIAL_SECTORS
          /* ACS flash manager erasing unaligned region of hyper flash sector(s)
             Occurs as need to assign multiple flash manager partitions
             to a single hyper flash sector */
          status = erase_region(pxFlash, ulAddress, lSize);
#else
          // Region to erase must exactly map to a flash sector
          status = IOT_FLASH_INVALID_VALUE;
#endif
      }
      else
      {
          // lSize verified as nonzero and evenly divisible by SECTOR_SIZE above
          int numSectors = lSize / SECTOR_SIZE;
          for (int sector = 0; sector < numSectors; sector++)
          {
              slnStatus = SLN_Erase_Sector(ulAddress);
              if (kStatus_Success != slnStatus)
              {
                  status = IOT_FLASH_ERASE_FAILED;
                  break;
              }
              ulAddress += SECTOR_SIZE;
          }
      }
    }

    return status;
}

int32_t iot_flash_erase_chip(IotFlashHandle_t const pxFlash)
{
    if (pxFlash == NULL) {
        return (uint32_t)IOT_FLASH_INVALID_VALUE;
    }

    // Unsupported by RT106A
    return (uint32_t)IOT_FLASH_FUNCTION_NOT_SUPPORTED;
}

// Target sector in flash must be erased prior to calling this function
int32_t iot_flash_write_sync(IotFlashHandle_t const pxFlash, uint32_t ulAddress, uint8_t *const pvBuffer, size_t xBytes)
{
    uint32_t status;

    // Delegate to rmw_pages() with forceOnes flag false
    status = rmw_pages(false, pxFlash, ulAddress, pvBuffer, xBytes);

    return status;
}

/*!
 * @Brief Read-Modify-Write flash pages(s) containing specified region using single
 *        heap-allocated SRAM scratch buffer one page in size
 *        - For sub-sector erase emulation, ignores pvBuffer parameter and forces bytes
 *          within specified region to 0xFF when called with 'forceOnes' parameter with
 *          value 'true'
 *
 * @note Region to be modified must be in erased state prior to calling this function
 *
 * @Param[in]  forceOnes  flag to write bytes of 0xFF rather than 'pvBuffer' contents
 * @Param[in]  pxFlash    handle to flash driver returned from iot_flash_open()
 * @Param[in]  ulAddress  potentially sector-unaligned address to beginning of region to modify
 * @Param[in]  pvBuffer   buffer of write data
 * @Param[in]  xBytes     number of bytes in region to modify
 *
 * @return
 *   - IOT_FLASH_SUCCESS upon success
 *   - IOT_FLASH_INVALID_VALUE for invalid parameter value(s)
 *   - IOT_FLASH_ERASE_FAILED upon error with 'forceOnes' value of 'false'
 *   - IOT_FLASH_WRITE_FAILED upon error with 'forceOnes' value of 'true'
 */
static int32_t rmw_pages(bool forceOnes, IotFlashHandle_t const pxFlash, uint32_t ulAddress, uint8_t *const pvBuffer, size_t xBytes)
{
    int32_t status               = IOT_FLASH_SUCCESS;
    status_t slnStatus            = kStatus_Success;
    uint8_t *sramFlashBuf        = NULL;

    if ((pxFlash == NULL)
            || (0 == xBytes)
            || (NULL == ulAddress)
            || (FLASH_SIZE <= ulAddress)
            || (!forceOnes && NULL == pvBuffer)
        )
    {
        status = IOT_FLASH_INVALID_VALUE;
    }

    if (IOT_FLASH_SUCCESS == status)
    {
        // Allocate SRAM buffer for flash page data
        sramFlashBuf = (uint8_t *)pvPortMalloc(FLASH_PAGE_SIZE);
        if (NULL == sramFlashBuf)
        {
            status = forceOnes ? IOT_FLASH_ERASE_FAILED : IOT_FLASH_WRITE_FAILED;
        }
    }

    if (IOT_FLASH_SUCCESS == status)
    {
        uint32_t curChunkBytes    = 0;
        uint32_t toWrite          = xBytes;
        uint32_t dstAddr          = ulAddress;
        uint32_t srcAddr          = (uint32_t)pvBuffer;
        uint32_t pageBackupBytes  = 0;

        // Perform writes to flash
        while (toWrite)
        {
            // Write only the amount staring from the current address up to the end of the current page
            pageBackupBytes  = dstAddr % FLASH_PAGE_SIZE;
            curChunkBytes = FLASH_PAGE_SIZE - pageBackupBytes;
            if (curChunkBytes > toWrite)
            {
                curChunkBytes = toWrite;
            }

             /* Backup the data from the current page (need to allow updates anywhere in the page as long as 0xFF in there)
                KVS is writing first the value, than the CRC, which is placed at offsets lower than the value */
            SLN_ram_memcpy(sramFlashBuf, (uint8_t *)SLN_Flash_Get_Read_Address(dstAddr - pageBackupBytes), FLASH_PAGE_SIZE);

            if(forceOnes)
            {
                // Overwrite region to be erased with flash-erased value, 0xFF
                SLN_ram_memset(sramFlashBuf + pageBackupBytes, 0xFF, curChunkBytes);
            }
            else
            {
                // Copy the bytes which will be updated in current page
                SLN_ram_memcpy(sramFlashBuf + pageBackupBytes, (void *)srcAddr, curChunkBytes);
            }

            // Write updated page of data to flash
            slnStatus = SLN_Write_Flash_Page(dstAddr - pageBackupBytes, sramFlashBuf, FLASH_PAGE_SIZE);
            if (kStatus_Success == slnStatus)
            {
                status = IOT_FLASH_SUCCESS;
            }
            else
            {
                status = forceOnes ? IOT_FLASH_ERASE_FAILED : IOT_FLASH_WRITE_FAILED;
                break;
            }

            // Update with number of source bytes copied
            dstAddr += curChunkBytes;
            srcAddr += curChunkBytes;
            toWrite -= curChunkBytes;
        }
    }

    vPortFree(sramFlashBuf);
    sramFlashBuf = NULL;

    return status;
}

#if ERASE_PARTIAL_SECTORS
/*!
 * @Brief Erases region in flash spanning one or more flash sectors
 *        - Native sector erase operation for region aligned with flash sector boundaries
 *        - Emulates erase via read-modify-write with a single additional flash sector
 *          as scratch buffer for region or portion thereof not aligned with flash
 *          sector boundaries
 *
 * @Param[in]  pxFlash    handle to flash driver returned from iot_flash_open()
 * @Param[in]  ulAddress  potentially sector-unaligned address to beginning of region to erase
 * @Param[in]  xBytes     number of bytes in region to erase
 *
 * @return
 *   - IOT_FLASH_SUCCESS upon success
 *   - IOT_FLASH_INVALID_VALUE for invalid parameter value(s)
 *   - IOT_FLASH_ERASE_FAILED upon error
 */
static int32_t erase_region(IotFlashHandle_t const pxFlash, uint32_t ulAddress, size_t xBytes)
{
    int32_t status               = IOT_FLASH_SUCCESS;
    status_t slnStatus           = kStatus_Success;

    if ((NULL == pxFlash) || (0 == xBytes) || (NULL == ulAddress) || (FLASH_SIZE <= ulAddress) ||
        (ulAddress % FLASH_PAGE_SIZE) || (xBytes % FLASH_PAGE_SIZE))
    {
        status = IOT_FLASH_INVALID_VALUE;
    }

    if (IOT_FLASH_SUCCESS == status)
    {
        uint32_t scratchAddr           = FLASH_SCRATCH_BUF_ADDR;
        uint32_t dstAddr               = ulAddress;
        uint32_t toErase               = xBytes;
        uint32_t curChunkBytes         = 0;

        while (toErase)
        {
           /*
            * Process one sector per iteration as have only one sector of flash scratch buffer
            *
            * 1. Copy bytes in multiples of page size before the beginning of current region to erase
            *    up to the page boundary immediately before the beginning of the current region to erase
            * 2. Read-Modify-Write bytes in pages overlapping region to erase
            * 3. Copy bytes in multiples of page size from the page boundary immediately after the
            *    end of the region to erase to the end of the current sector
            */

            /* Number of bytes between current beginning of region to erase
               and the immediately preceding sector boundary */
            uint32_t sctrBegCopyBytes      = dstAddr % SECTOR_SIZE;

            /* Address of sector boundary immediately preceding the beginning of
               the current region to erase */
            uint32_t algnSctrBegAddr       = dstAddr - sctrBegCopyBytes;

            // Number of bytes to copy or read-modify-write in this iteration
            curChunkBytes = SECTOR_SIZE - sctrBegCopyBytes;
            curChunkBytes = (curChunkBytes > toErase) ? toErase : curChunkBytes;

            // Address of page boundary immediately following the end of the current region to erase
            uint32_t algnPgEndAddr         = dstAddr + curChunkBytes;

            /* Number of bytes to copy after end of current region to erase up to
               the immediately following page boundary */
            uint32_t algnSctrResidualBytes = algnPgEndAddr % SECTOR_SIZE;
            // Number of bytes to copy in aligned pages after the end of the current region to erase
            uint32_t algnPgEndBytes        = algnSctrResidualBytes ? SECTOR_SIZE - algnSctrResidualBytes : 0;

            // Erase scratch flash sector
            slnStatus = SLN_Erase_Sector(scratchAddr);
            if (kStatus_Success != slnStatus)
            {
                status = IOT_FLASH_ERASE_FAILED;
                break;
            }

            // Copy any pages before and non-overlapping the region to be erased to the flash scratch buffer
            if (0 < sctrBegCopyBytes)
            {
                status = rmw_pages(false, pxFlash, scratchAddr, (uint8_t *)SLN_Flash_Get_Read_Address(algnSctrBegAddr), sctrBegCopyBytes);
                scratchAddr += sctrBegCopyBytes;
                if (IOT_FLASH_SUCCESS != status)
                {
                    break;
                }
            }

            /* Copy pages overlapping region to be erased to the flash scratch buffer
               Bytes within region to be erased forced to 0xFF */
            status = rmw_pages(true, pxFlash, scratchAddr, (uint8_t *)SLN_Flash_Get_Read_Address(dstAddr), curChunkBytes);
            scratchAddr += toErase;
            if (IOT_FLASH_SUCCESS != status)
            {
                break;
            }

            // Copy any pages after and non-overlapping the region to be erased to the flash scratch buffer
            if (0 < algnPgEndBytes)
            {
                status = rmw_pages(false, pxFlash, scratchAddr, (uint8_t *)SLN_Flash_Get_Read_Address(algnPgEndAddr), algnPgEndBytes);
                scratchAddr += algnPgEndBytes;
                if (IOT_FLASH_SUCCESS != status)
                {
                    break;
                }
            }

            // Erase destination flash sector
            slnStatus = SLN_Erase_Sector(algnSctrBegAddr);
            if (kStatus_Success != slnStatus)
            {
                status = IOT_FLASH_ERASE_FAILED;
                break;
            }

            // Write updated data in the flash scratch buffer to the destination flash sector
            slnStatus = SLN_Write_Sector(algnSctrBegAddr, (uint8_t *)SLN_Flash_Get_Read_Address(FLASH_SCRATCH_BUF_ADDR), SECTOR_SIZE);
            if (kStatus_Success != slnStatus)
            {
                status = IOT_FLASH_ERASE_FAILED;
                break;
            }

            // Update with number bytes *erased* in this iteration
            dstAddr += curChunkBytes;
            toErase -= curChunkBytes;
        }
    }

    return status;
}
#endif /* ERASE_PARTIAL_SECTORS */

int32_t iot_flash_read_sync(IotFlashHandle_t const pxFlash, uint32_t ulAddress, uint8_t *const pvBuffer, size_t xBytes)
{
    if ((NULL == pxFlash)
            || (NULL == ulAddress)
            || (FLASH_SIZE <= ulAddress)
            || (NULL == pvBuffer)
            || (0 == xBytes)
        )
    {
        return (uint32_t)IOT_FLASH_INVALID_VALUE;
    }

    status_t slnStatus = SLN_Read_Flash_At_Address(ulAddress, pvBuffer, xBytes);

    return (kStatus_Success == slnStatus) ? (uint32_t)IOT_FLASH_SUCCESS : (uint32_t)IOT_FLASH_READ_FAILED;
}

int32_t iot_flash_ioctl(IotFlashHandle_t const pxFlash, IotFlashIoctlRequest_t xRequest, void *const pvBuffer)
{

    if ((NULL == pxFlash) || (eSetFlashBlockProtection > xRequest) || (eGetFlashRxNoOfbytes < xRequest))
    {
        return (uint32_t)IOT_FLASH_INVALID_VALUE;
    }

    // Unsupported by RT106A
    return IOT_FLASH_FUNCTION_NOT_SUPPORTED;
}
