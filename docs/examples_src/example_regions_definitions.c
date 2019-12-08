#include "lwmem/lwmem.h"

/*
 * \brief           Define regions for memory manager
 */
static
lwmem_region_t regions[] = {
    /* Set start address and size of each region */
    { (void *)0x10000000, 0x00001000 },
    { (void *)0xA0000000, 0x00008000 },
    { (void *)0xC0000000, 0x00008000 },
};

/* Later in the initialization process */
/* Assign regions for manager */
lwmem_assignmem(regions, sizeof(regions) / sizeof(regions[0]));