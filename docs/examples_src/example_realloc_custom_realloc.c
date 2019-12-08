int* ints = lwmem_malloc(12 * sizeof(*ints));  /* Allocate memory for 12 integers */

/* Check for successful allocation */
if (ints == NULL) {
    printf("Allocation failed ints!\r\n");
    return -1;
}
printf("ints allocated for 12 integers\r\n");
lwmem_debug_free();     /* This is debug function for sake of this example */

/* Now allocate new one for new size */
int* ints2 = lwmem_malloc(13 * sizeof(*ints)); /* Allocate memory for 13 integers */
if (ints2 == NULL) {
    printf("Allocation failed ints2!\r\n");
    return -1;
}

printf("ints2 allocated for 13 integers\r\n");
lwmem_debug_free();     /* This is debug function for sake of this example */

/* Copy content of 12-integers to 13-integers long array */
memcpy(ints2, ints, 12 * sizeof(12));

/* Free first block */
lwmem_free(ints);       /* Free memory */
ints = ints2;           /* Use ints2 as new array now */
ints2 = NULL;           /* Set it to NULL to prevent accessing same memory from different pointers */

printf("old ints freed\r\n");
lwmem_debug_free();     /* This is debug function for sake of this example */

/* Do not forget to free it when not used anymore */
lwmem_free_s(&ints);

printf("ints and ints2 freed\r\n");
lwmem_debug_free();     /* This is debug function for sake of this example */