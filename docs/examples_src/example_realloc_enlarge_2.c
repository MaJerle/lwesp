void* ptr1, *ptr2;

/* Allocate initial blocks */
ptr2 = lwmem_malloc(80);
ptr1 = lwmem_malloc(24);
lwmem_free_s(&ptr2);    /* Free first block and mark it free */

/* We assume allocation is successful */

printf("State at case 2a\r\n");
lwmem_debug_free();     /* This is debug function for sake of this example */

/* Now let's reallocate ptr1 */
ptr2 = lwmem_realloc(ptr1, 32);

printf("State at case 2b\r\n");
lwmem_debug_free();     /* This is debug function for sake of this example */
