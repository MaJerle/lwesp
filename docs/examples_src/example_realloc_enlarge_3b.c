/* Now reallocate ptr2 */
ptr2 = lwmem_realloc(ptr2, 20);

printf("New state at case 3b\r\n");
lwmem_debug_free();     /* This is debug function for sake of this example */