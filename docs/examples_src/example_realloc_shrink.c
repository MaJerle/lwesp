int* ints, *ints2;

ints = lwmem_malloc(15 * sizeof(*ints));  /* Allocate memory for 15 integers */
if (ints == NULL) {
    printf("Allocation failed ints!\r\n");
    return -1;
}
printf("ints allocated for 15 integers\r\n");
lwmem_debug_free();     /* This is debug function for sake of this example */

/* Now reallocte ints and write result to new variable */
ints2 = lwmem_realloc(ints, 12 * sizeof(*ints));
if (ints == NULL) {
    printf("Allocation failed ints2!\r\n");
    return -1;
}
printf("ints re-allocated for 12 integers\r\n");
lwmem_debug_free();     /* This is debug function for sake of this example */

/* ints is successfully reallocated and it is no longer valid pointer to read/write from/to */

/* For the sake of example, let's test pointers */
if (ints2 == ints) {
	printf("New block reallocated to the same address as previous one\r\n");
} else {
	printf("New block reallocated to new address\r\n");
}

/* Free ints2 */
lwmem_free_s(&ints2);
/* ints is already freed by successful realloc function */
ints = NULL;            /* It is enough to set it to NULL */

lwmem_debug_free();     /* This is debug function for sake of this example */