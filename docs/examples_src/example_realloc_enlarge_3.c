void* ptr1, *ptr2, *ptr3, *ptr4;

/* Allocate 4 blocks */
ptr1 = lwmem_malloc(8);
ptr2 = lwmem_malloc(4);
ptr3 = lwmem_malloc(4);
ptr4 = lwmem_malloc(16);
/* Free first and third block */
lwmem_free_s(&ptr1);
lwmem_free_s(&ptr3);

/* We assume allocation is successful */

printf("Initial state at case 3\r\n");
lwmem_debug_free();     /* This is debug function for sake of this example */