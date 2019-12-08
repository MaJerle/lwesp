void* ptr1, *ptr2;

/* Allocate initial block */
ptr1 = lwmem_malloc(24);

/* We assume allocation is successful */

printf("State at case 1a\r\n");
lwmem_debug_free();     /* This is debug function for sake of this example */

/* Now let's reallocate ptr1 */
ptr2 = lwmem_realloc(ptr1, 32);

printf("State at case 1b\r\n");
lwmem_debug_free();     /* This is debug function for sake of this example */
