/**	
 * \file            esp_mem.c
 * \brief           Memory manager
 */
 
/*
 * Copyright (c) 2017 Tilen Majerle
 *  
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */
#define ESP_INTERNAL
#include "esp_mem.h"
#include "esp_debug.h"

/******************************************************************************/
/******************************************************************************/
/***                           Private structures                            **/
/******************************************************************************/
/******************************************************************************/
typedef struct MemBlock {
    struct MemBlock* NextFreeBlock;                 /*!< Pointer to next free block */
    size_t Size;                                    /*!< Size of block */
} MemBlock_t;

/**
 * \brief           Memory alignment bits and absolute number
 */
#define MEM_ALIGN_BITS              ((size_t)(ESP_MEM_ALIGNMENT - 1))
#define MEM_ALIGN_NUM               ((size_t)ESP_MEM_ALIGNMENT)
#define MEM_ALIGN(x)                ESP_MEM_ALIGN(x)

#define MEMBLOCK_METASIZE           MEM_ALIGN(sizeof(MemBlock_t))

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/

/******************************************************************************/
/******************************************************************************/
/***                            Private variables                            **/
/******************************************************************************/
/******************************************************************************/
static MemBlock_t StartBlock;
static MemBlock_t* EndBlock = 0;
static size_t MemAvailableBytes = 0;
static size_t MemMinAvailableBytes = 0;
static size_t MemAllocBit = 0;

static size_t MemTotalSize = 0;                     /* Size of memory in units of bytes */

/******************************************************************************/
/******************************************************************************/
/***                            Private functions                            **/
/******************************************************************************/
/******************************************************************************/
/* Insert block to list of free blocks */
static void
mem_insertfreeblock(MemBlock_t* newBlock) {
    MemBlock_t* ptr;
    uint8_t* addr;

    /**
     * Find block position to insert new block between
     */
    for (ptr = &StartBlock; ptr && ptr->NextFreeBlock < newBlock; ptr = ptr->NextFreeBlock);

    /**
     * If the new inserted block and block before create a one big block (contiguous)
     * then try to merge them together
     */
    addr = (uint8_t *)ptr;
    if ((uint8_t *)(addr + ptr->Size) == (uint8_t *)newBlock) {
        ptr->Size += newBlock->Size;                /* Expand size of block before new inserted */
        newBlock = ptr;                             /* Set new block pointer to block before (expanded block) */
    }

    /**
     * Check if new block and its size is the same address as next free block newBlock points to
     */
    addr = (uint8_t *)newBlock;
    if ((uint8_t *)(addr + newBlock->Size) == (uint8_t *)ptr->NextFreeBlock) {
        if (ptr->NextFreeBlock == EndBlock) {       /* Does it points to the end? */
            newBlock->NextFreeBlock = EndBlock;     /* Set end block pointer */
        } else {
            newBlock->Size += ptr->NextFreeBlock->Size; /* Expand of current block for size of next free block which is right behind new block */
            newBlock->NextFreeBlock = ptr->NextFreeBlock->NextFreeBlock; /* Next free is pointed to the next one of previous next */
        }
    } else {
        newBlock->NextFreeBlock = ptr->NextFreeBlock;   /* Our next element is now from pointer next element */
    }

    /**
    * If merge with new block and block before was not made then there
    * is a gap between free memory before and new free memory.
    *
    * We have to set block before to point to next free which is new block
    */
    if (ptr != newBlock) {
        ptr->NextFreeBlock = newBlock;
    }
}

static uint8_t
mem_assignmem(const mem_region_t* regions, size_t len) {
    uint8_t* MemStartAddr;
    size_t MemSize;
    MemBlock_t* FirstBlock;
    MemBlock_t* PreviousEndBlock = 0;
    size_t i;
    
    if (EndBlock) {                                 /* Regions already defined */
        return 0;
    }
    
    /**
     * Check if region address are linear and rising
     */
    MemStartAddr = (uint8_t *)0;
    for (i = 0; i < len; i++) {
        if (MemStartAddr >= (uint8_t *)regions[i].StartAddress) {   /* Check if previous greater than current */
            return 0;                               /* Return as invalid and failed */
        }
        MemStartAddr = (uint8_t *)regions[i].StartAddress;  /* Save as previous address */
    }

    while (len--) {
        /**
         * Check minimum region size
         */
        MemSize = regions->Size;
        if (MemSize < (MEM_ALIGN_NUM + MEMBLOCK_METASIZE)) {
            regions++;
            continue;
        }
        /**
         * Get start address and check memory alignment
         * if necessary, decrease memory region size
         */
        MemStartAddr = (uint8_t *)regions->StartAddress;    /* Actual heap memory address */
        if ((size_t)MemStartAddr & MEM_ALIGN_BITS) {    /* Check alignment boundary */
            MemStartAddr += MEM_ALIGN_NUM - ((size_t)MemStartAddr & MEM_ALIGN_BITS);
            MemSize -= MemStartAddr - (uint8_t *)regions->StartAddress;
        }
        
        /**
         * Check memory size alignment if match
         */
        if (MemSize & MEM_ALIGN_BITS) {
            MemSize &= ~MEM_ALIGN_BITS;             /* Clear lower bits of memory size only */
        }

        /**
         * StartBlock is fixed variable for start list of free blocks
         *
         * Set free blocks linked list on initialized
         *
         * Set Start block only if end block is not yet defined = first run
         */
        if (!EndBlock) {
            StartBlock.NextFreeBlock = (MemBlock_t *)MemStartAddr;
            StartBlock.Size = 0;
        }
        
        PreviousEndBlock = EndBlock;                /* Save previous end block to set next block later */
        
        /**
         * Set pointer to end of free memory - block region memory
         * Calculate new end block in region
         */
        EndBlock = (MemBlock_t *)((uint8_t *)MemStartAddr + MemSize - MEMBLOCK_METASIZE);
        EndBlock->NextFreeBlock = 0;                /* No more free blocks after end is reached */
        EndBlock->Size = 0;                         /* Empty block */

        /**
         * Initialize start of region memory
         * Create first block in region
         */
        FirstBlock = (MemBlock_t *)MemStartAddr;
        FirstBlock->Size = MemSize - MEMBLOCK_METASIZE; /* Exclude end block in chain */
        FirstBlock->NextFreeBlock = EndBlock;       /* Last block is next free in chain */

        /**
         * If we have previous end block
         * End block of previous region
         *
         * Set previous end block to start of next region
         */
        if (PreviousEndBlock) {
            PreviousEndBlock->NextFreeBlock = FirstBlock;
        }
        
        /**
         * Set number of free bytes available to allocate in region
         */
        MemAvailableBytes += FirstBlock->Size;
        
        regions++;                                  /* Go to next region */
    }
    
    MemMinAvailableBytes = MemAvailableBytes;       /* Save minimum ever available bytes in region */
    
    /**
     * Set upper bit in memory allocation bit
     */
    MemAllocBit = (size_t)((size_t)1 << ((sizeof(size_t) * 8 - 1)));
    
    return 1;                                       /* Regions set as expected */
}

uint32_t mem_allocations;

static void*
mem_alloc(size_t size) {
    MemBlock_t *Prev, *Curr, *Next;
    void* retval = 0;

    if (!EndBlock) {                                /* If end block is not yet defined */
        return 0;                                   /* Invalid, not initialized */
    }
    
    /**
     * TODO: Check alignment maybe?
     */    
    if (!size || size >= MemAllocBit) {             /* Check input parameters */
        return 0;
    }

    size = MEM_ALIGN(size) + MEMBLOCK_METASIZE;
    if (size > MemAvailableBytes) {                 /* Check if we have enough memory available */
        return 0;
    }

    /**
     * Try to find sufficient block for data
     * Go through free blocks until enough memory is found
     * or end block is reached (no next free block)
     */
    Prev = &StartBlock;                             /* Set first first block as previous */
    Curr = Prev->NextFreeBlock;                     /* Set next block as current */
    while ((Curr->Size < size) && (Curr->NextFreeBlock)) {
        Prev = Curr;
        Curr = Curr->NextFreeBlock;
    }
    
    /**
     * Possible improvements
     * Try to find smallest available block for desired amount of memory
     * 
     * Feature may be very risky later because of fragmentation
     */
    
    if (Curr != EndBlock) {                         /* We found empty block of enough memory available */
        retval = (void *)((uint8_t *)Prev->NextFreeBlock + MEMBLOCK_METASIZE);    /* Set return value */
        Prev->NextFreeBlock = Curr->NextFreeBlock;  /* Since block is now allocated, remove it from free chain */

        /**
         * If found free block is much bigger than required, 
         * then split big block by 2 blocks (one used, second available)
         * There should be available memory for at least 2 metadata block size = 8 bytes of useful memory
         */
        if ((Curr->Size - size) > (2 * MEMBLOCK_METASIZE)) {    /* There is more available memory then required = split memory to one free block */
            Next = (MemBlock_t *)(((uint8_t *)Curr) + size);    /* Create next memory block which is still free */
            Next->Size = Curr->Size - size;         /* Set new block size for remaining of before and used */
            Curr->Size = size;                      /* Set block size for used block */

            /**
             * Add virtual block to list of free blocks.
             * It is placed directly after currently allocated memory
             */
            mem_insertfreeblock(Next);              /* Insert free memory block to list of free memory blocks (linked list chain) */
        }
        Curr->Size |= MemAllocBit;                  /* Set allocated bit = memory is allocated */
        Curr->NextFreeBlock = 0;                    /* Clear next free block pointer as there is no one */

        MemAvailableBytes -= size;                  /* Decrease available memory */
        if (MemAvailableBytes < MemMinAvailableBytes) { /* Check if current available memory is less than ever before */
            MemMinAvailableBytes = MemAvailableBytes;   /* Update minimal available memory */
        }
    } else {
        /* Allocation failed, no free blocks of required size */
    }

    mem_allocations += !!retval;
    return retval;
}

static void
mem_free(void* ptr) {
    MemBlock_t* block;

    if (!ptr) {                                     /* To be in compliance with C free function */
        return;
    }

    block = (MemBlock_t *)(((uint8_t *)ptr) - MEMBLOCK_METASIZE);   /* Get block data pointer from input pointer */

    /**
     * Check if block is even allocated by upper bit on size
     * and next free block must be set to NULL in order to work properly
     */
    if ((block->Size & MemAllocBit) && !block->NextFreeBlock) {
        /**
         * Clear allocated bit before entering back to free list
         * List will automatically take care for fragmentation and mix segments back
         */
        block->Size &= ~MemAllocBit;                /* Clear allocated bit */
        MemAvailableBytes += block->Size;           /* Increase available bytes back */
        mem_insertfreeblock(block);                 /* Insert block to list of free blocks */
    }
    mem_allocations--;
}

/* Get size of user memory from input pointer */
static size_t
mem_getusersize(void* ptr) {
    MemBlock_t* block;
    
    if (!ptr) {
        return 0;
    }
    block = (MemBlock_t *)(((uint8_t *)ptr) - MEMBLOCK_METASIZE);   /* Get block meta data pointer */
    if (block->Size & MemAllocBit) {                /* Memory is actually allocated */
        return (block->Size & ~MemAllocBit) - MEMBLOCK_METASIZE;    /* Return size of block */
    }
    return 0;
}

/* Allocate memory and set it to 0 */
static void*
mem_calloc(size_t num, size_t size) {
    void* ptr;
    size_t tot_len = num * size;
    
    if ((ptr = mem_alloc(tot_len)) != NULL) {       /* Try to allocate memory */
        memset(ptr, 0x00, tot_len);                 /* Reset entire memory */
    }
    return ptr;
}

/* Reallocate previously allocated memory */
static void*
mem_realloc(void* ptr, size_t size) {
    void* newPtr;
    size_t oldSize;
    
    if (!ptr) {                                     /* If pointer is not valid */
        return mem_alloc(size);                     /* Only allocate memory */
    }
    
    oldSize = mem_getusersize(ptr);                 /* Get size of old pointer */
    newPtr = mem_alloc(size);                       /* Try to allocate new memory block */
    if (newPtr) {                                   /* Check success */
        memcpy(newPtr, ptr, size > oldSize ? oldSize : size);   /* Copy old data to new array */
        mem_free(ptr);                              /* Free old pointer */
        return newPtr;                              /* Return new pointer */
    }
    return 0;
}

static size_t
mem_getfree(void) {
    return MemAvailableBytes;                       /* Return free bytes available for allocation */
}

static size_t
mem_getfull(void) {
    return MemTotalSize - MemAvailableBytes;        /* Return remaining bytes */
}

static size_t
mem_getminfree(void) {
    return MemMinAvailableBytes;                    /* Return minimal bytes ever available */
}

/******************************************************************************/
/******************************************************************************/
/***                                Public API                               **/
/******************************************************************************/
/******************************************************************************/

/**
 * \brief           Allocate memory of specific size
 * \note            Since this function is private, it can only be used by user inside ESP library
 * \param[in]       size: Number of bytes to allocate
 * \retval          0: Allocation failed
 * \retval          >0: Pointer to allocated memory
 */
void*
esp_mem_alloc(uint32_t size) {
    void* ptr;
    esp_sys_protect();
    ptr = mem_alloc(size);                          /* Allocate memory and return pointer */
    if (!ptr) {
        ESP_DEBUGF(ESP_DBG_MEM, "Allocation failed: %d bytes\r\n", (int)size);
    }
    esp_sys_unprotect();
    return ptr;
}

/**
 * \brief           Allocate memory of specific size
 * \note            After new memory is allocated, content of old one is copied to new allocated memory
 *
 * \note            Since this function is private, it can only be used by user inside ESP library
 * \param[in]       *ptr: Pointer to current allocated memory to resize, returned using \ref esp_mem_alloc, \ref esp_mem_calloc or \ref esp_mem_realloc functions
 * \param[in]       size: Number of bytes to allocate on new memory
 * \retval          0: Allocation failed
 * \retval          >0: Pointer to allocated memory
 */
void*
esp_mem_realloc(void* ptr, size_t size) {
    esp_sys_protect();
    ptr = mem_realloc(ptr, size);                   /* Reallocate and return pointer */
    if (!ptr) {
        ESP_DEBUGF(ESP_DBG_MEM, "Reallocation failed: %d bytes\r\n", (int)size);
    }
    esp_sys_unprotect();
    return ptr;
}

/**
 * \brief           Allocate memory of specific size and set memory to zero
 * \note            Since this function is private, it can only be used by user inside ESP library
 * \param[in]       num: Number of elements to allocate
 * \param[in]       size: Size of each element
 * \retval          0: Allocation failed
 * \retval          >0: Pointer to allocated memory
 */
void*
esp_mem_calloc(size_t num, size_t size) {
    void* ptr;
    esp_sys_protect();
    ptr = mem_calloc(num, size);                   /* Allocate memory and clear it to 0. Then return pointer */
    if (!ptr) {
        ESP_DEBUGF(ESP_DBG_MEM, "(C)Allocation failed: %d bytes\r\n", (int)(size * num));
    }
    esp_sys_unprotect();
    return ptr;
}

/**
 * \brief           Free memory
 * \param[in]       *ptr: Pointer to memory previously returned using \ref esp_mem_alloc, \ref esp_mem_calloc or \ref esp_mem_realloc functions
 */
void
esp_mem_free(void* ptr) {
    esp_sys_protect();
    mem_free(ptr);                                  /* Free already allocated memory */
    esp_sys_unprotect();
}

/**
 * \brief           Get total free size still available in memory to allocate
 * \note            Since this function is private, it can only be used by user inside ESP library
 * \retval          Number of bytes available to allocate
 */
size_t
esp_mem_getfree(void) {
    return mem_getfree();                           /* Get free bytes available to allocate */
}

/**
 * \brief           Get total currently allocated memory in regions
 * \note            Since this function is private, it can only be used by user inside ESP library
 * \retval          Number of bytes in use
 */
size_t
esp_mem_getfull(void) {
    return mem_getfull();                           /* Get number of bytes allocated already */
}

/**
 * \brief           Get minimal available number of bytes ever for allocation
 * \note            Since this function is private, it can only be used by user inside ESP library
 * \retval          Number of minimal available number of bytes ever
 */
size_t
esp_mem_getminfree(void) {
    return mem_getminfree();                        /* Get minimal number of bytes ever available for allocation */
}

/******************************************************************************/
/******************************************************************************/
/***                  Thread safe version of public API                      **/
/******************************************************************************/
/******************************************************************************/

/**
 * \brief           Assign memory region(s) for allocation functions
 * \note            You can allocate multiple regions by assigning start address and region size in units of bytes
 * \param[in]       *regions: Pointer to list of regions to use for allocations
 * \param[in]       len: Number of regions to use
 * \retval          1: Memory assigned ok
 * \retval          0: Memory was not assigned
 */
uint8_t
esp_mem_assignmemory(const esp_mem_region_t* regions, size_t len) {
    uint8_t ret;
    ret = mem_assignmem(regions, len);              /* Assign memory */
    return ret;                                     
}
