esp_pbuf_p a, b;

/* Create 2 pbufs of different sizes */
a = esp_pbuf_new(10);
b = esp_pbuf_new(20);

/* Chain both pbufs together */
/* This will increase reference on b as 2 variables now point to it */
esp_pbuf_chain(a, b);

/*
 * When application does not need a anymore, it may free it

 * This will free only pbuf a, as pbuf b has now 2 references:
 *  - one from pbuf a
 *  - one from variable b
 */

/* If application calls this, it will free only first pbuf */
/* As there is link to b pbuf somewhere */
esp_pbuf_free(a);                               

/* Reset a variable, not used anymore */
a = NULL;

/*
 * At this point, b is still valid memory block,
 * but when application doesn't need it anymore,
 * it should free it, otherwise memory leak appears
 */
esp_pbuf_free(b);

/* Reset b variable */
b = NULL;