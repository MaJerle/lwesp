#define ASSERT(x)           do {        \
    if (!(x)) {                         \
        printf("Assert failed with condition (" # x ")\r\n");  \
    } else {\
        printf("Assert passed with condition (" # x ")\r\n");  \
    }\
} while (0) 

/* For debug purposes */
lwmem_region_t* regions_used;
size_t regions_count = 1;       /* Use only 1 region for debug purposes of non-free areas */

int
main(void) {
    uint8_t* ptr1, *ptr2, *ptr3, *ptr4;
    uint8_t* rptr1, *rptr2, *rptr3, *rptr4;

    /* Create regions for debug purpose */
    if (!lwmem_debug_create_regions(&regions_used, regions_count, 128)) {
        printf("Cannot allocate memory for regions for debug purpose!\r\n");
        return -1;
    }
    lwmem_assignmem(regions_used, regions_count);
    printf("Manager is ready!\r\n");
    lwmem_debug_print(1, 1);

    /* Test case 1, allocate 3 blocks, each of different size */
    /* We know that sizeof internal metadata block is 8 bytes on win32 */
    printf("\r\n\r\nAllocating 4 pointers and freeing first and third..\r\n");
    ptr1 = lwmem_malloc(8);
    ptr2 = lwmem_malloc(4);
    ptr3 = lwmem_malloc(4);
    ptr4 = lwmem_malloc(16);
    lwmem_free(ptr1);               /* Free but keep value for future comparison */
    lwmem_free(ptr3);               /* Free but keep value for future comparison */
    lwmem_debug_print(1, 1);
    printf("Debug above is effectively state 3\r\n");
    lwmem_debug_save_state();       /* Every restore operations rewinds here */

    /* We always try to reallocate pointer ptr2 */

    /* Create 3a case */
    printf("\r\n------------------------------------------------------------------------\r\n");
    lwmem_debug_restore_to_saved();
    printf("State 3a\r\n");
    rptr1 = lwmem_realloc(ptr2, 8);
    lwmem_debug_print(1, 1);
    ASSERT(rptr1 == ptr2);

    /* Create 3b case */
    printf("\r\n------------------------------------------------------------------------\r\n");
    lwmem_debug_restore_to_saved();
    printf("State 3b\r\n");
    rptr2 = lwmem_realloc(ptr2, 20);
    lwmem_debug_print(1, 1);
    ASSERT(rptr2 == ptr2);

    /* Create 3c case */
    printf("\r\n------------------------------------------------------------------------\r\n");
    lwmem_debug_restore_to_saved();
    printf("State 3c\r\n");
    rptr3 = lwmem_realloc(ptr2, 24);
    lwmem_debug_print(1, 1);
    ASSERT(rptr3 == ptr1);

    /* Create 3d case */
    printf("\r\n------------------------------------------------------------------------\r\n");
    lwmem_debug_restore_to_saved();
    printf("State 3d\r\n");
    rptr4 = lwmem_realloc(ptr2, 36);
    lwmem_debug_print(1, 1);
    ASSERT(rptr4 != ptr1 && rptr4 != ptr2 && rptr4 != ptr3 && rptr4 != ptr4);

    return 0;
}
