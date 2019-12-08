void* ptr1, *ptr2, *ptr3, *ptr4, *ptrt;

/* We are now at case A */
printf("State at case A\r\n");
lwmem_debug_free();     /* This is debug function for sake of this example */

/* Each ptr points to its own block of allocated data */
/* Each block size is 24 bytes; 16 for user data and 8 for metadata */
ptr1 = lwmem_malloc(16);
ptr2 = lwmem_malloc(16);
ptr3 = lwmem_malloc(16);
ptr4 = lwmem_malloc(16);

/* We are now at case B */
printf("State at case B\r\n");
lwmem_debug_free();     /* This is debug function for sake of this example */

/* Reallocate ptr1, decrease its size to 12 user bytes */
/* Now we expect block size to be 20; 12 for user data and 8 for metadata */
ptrt = lwmem_realloc(ptr1, 12);
if (ptrt == NULL) {
    ptr1 = ptrt;
}

printf("State after first realloc\r\n");
lwmem_debug_free();     /* This is debug function for sake of this example */

/* At this point we are still at case B */
/* There was no modification of internal structure */
/* Difference between existing and new size (16 - 12 = 4) is too small
    to create new empty block, therefore block it is left unchanged */

/* Reallocate again, now to new size of 4 bytes */
/* Now we expect block size to be 16; 8 for user data and 8 for metadata */
ptrt = lwmem_realloc(ptr1, 8);
printf("State at case C\r\n");
lwmem_debug_free();     /* This is debug function for sake of this example */

/* We are now at case C */

/* Now free all memories */