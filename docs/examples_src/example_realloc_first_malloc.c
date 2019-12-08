int* ints = lwmem_malloc(12 * sizeof(*ints));  /* Allocate memory for 12 integers */

/* Check for successful allocation */
if (ints == NULL) {
    printf("Allocation failed!\r\n");
    return -1;
}
lwmem_debug_free();     /* This is debug function for sake of this example */

/* ints is a pointer to memory size for our integers */
/* Do not forget to free it when not used anymore */
lwmem_free_s(&ints);