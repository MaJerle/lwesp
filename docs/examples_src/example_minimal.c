#include "lwmem/lwmem.h"

void* ptr;

/* Create regions, address and length of regions */
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

ptr = lwmem_malloc(8);                          /* Allocate 8 bytes of memory */
if (ptr != NULL) {
    /* Allocation successful */
}

/* Later... */                                  /* Free allocated memory */
lwmem_free(ptr);
ptr = NULL;
/* .. or */
lwmem_free_s(&ptr);