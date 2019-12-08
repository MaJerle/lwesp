/* Now reallocate ptr2 */
ptr2 = lwmem_realloc(ptr2, 8);

printf("New state at case 3a\r\n");
lwmem_debug_free();     /* This is debug function for sake of this example */