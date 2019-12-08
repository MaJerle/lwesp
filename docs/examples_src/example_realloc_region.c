#include "lwmem/lwmem.h"

/* Define one region used by lwmem */
static unsigned char region_mem[128];

/*
 * \brief           Define regions for memory manager
 */
static
lwmem_region_t regions[] = {
    /* Set start address and size of each region */
    { region_mem, sizeof(region_mem) }
};

/* Later in the initialization process */
/* Assign regions for manager */
lwmem_assignmem(regions, sizeof(regions) / sizeof(regions[0]));
lwmem_debug_free();     /* This is debug function for sake of this example */