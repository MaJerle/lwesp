/**
 * \addtogroup      ESP_PBUF
 * \{
 *
 * Packet buffer is special memory for incoming (received) network data on active connections.
 *
 * Purpose of it is to have a support of making one big buffer from chunks of fragmented received data,
 * without need of having one big linear array.
 *
 * \image html pbuf_block_diagram.svg Block diagram of pbuf chain
 *
 * From the image above, we can see that we can chain buffers together to create quasi-linear block of data.
 * Each packet buffer consists of:
 *
 *  - Pointer to next pbuf in a chain or NULL of last one
 *  - Length of current pbuf
 *  - Length of current and all next in chain
 *      - When pbuf is last, this value is the same as length of it
 *  - Reference counter, which holds number of pointers pointing to this block.
 *
 * If we describe image more into details, we can see we have <b>3</b> pbufs in chain and we can also see that
 * there is some variable involved with pointing to second pbuf, therefore it has reference count set to <b>2</b>.
 *
 * In table below you can see what is written in memory on the image above.
 *
 * <table>
 *  <tr><th>Block number    <th>Next packet buffer  <th>Size of block   <th>Total size of chain <th>Reference counter   </tr>
 *  <tr><td>Block 1         <td>Block 2             <td>150             <td>550                 <td>1                   </tr>
 *  <tr><td>Block 2         <td>Block 3             <td>130             <td>400                 <td>2                   </tr>
 *  <tr><td>Block 3         <td>NULL                <td>270             <td>270                 <td>1                   </tr>
 * </table>
 *
 * \par             Reference counter
 *
 * Reference counter is important variable as it holds number of references pointing to block.
 * It is used when user wants to free the block. Since block may be referenced from different locations,
 * doing free from one reference would make undefined behavior for all other references pointing to this pbuf
 *
 * If we go back to image above, we can see that variable points to first pbuf and reference count is set to <b>1</b>,
 * which means we are the only one pointing to this pbuf at the moment.
 * If we try to free the memory, this operation is perfectly valid as nobody else is pointing to memory.
 *
 * Steps to remove packet buffers are:
 *
 *  - Decrease reference counter by <b>1</b>
 *  - If reference counter is now <b>0</b>, free the packet buffer
 *      - Set next pbuf in chain as current and start over
 *  - If reference counter is still not <b>0</b>, return from function
 *
 * A new memory structure is visible on image below.
 *
 * \image html pbuf_block_diagram_after_free.svg Block diagram of pbuf chain after free from user variable 1.
 *
 * <table>
 *  <tr><th>Block number    <th>Next packet buffer  <th>Size of block   <th>Total size of chain <th>Reference counter   </tr>
 *  <tr><td>Block 2         <td>Block 3             <td>130             <td>400                 <td>1                   </tr>
 *  <tr><td>Block 3         <td>NULL                <td>270             <td>270                 <td>1                   </tr>
 * </table>
 *
 * \par             Concatenating vs chaining
 *
 * When we are dealing with application part, it is important to know what is the difference between \ref esp_pbuf_cat and \ref esp_pbuf_chain.
 *
 * Imagine we have <b>2</b> pbufs and each of them is pointed to by <b>2</b> different variables, like on image below.
 *
 * \image html pbuf_cat_vs_chain_1.svg <b>2</b> independent pbufs with <b>2</b> variables
 *
 * After we call \ref esp_pbuf_cat, we get a new structure which is on image below.
 *
 * \image html pbuf_cat_vs_chain_2.svg Pbufs structure after calling esp_pbuf_cat
 * 
 * We can see that reference of second is still set to <b>1</b>, but <b>2</b> variables are pointing to this block.
 * After we call \ref esp_pbuf_cat, we have to forget using user variable 2, because if we somehow try to free pbuf from variable <b>1</b>, 
 * then we point with variable <b>2</b> to memory which is not defined anymore.
 *
 * \code{c}
esp_pbuf_p a, b;
                                                
a = esp_pbuf_new(10);                           // Create pbuf with 10 bytes of memory
b = esp_pbuf_new(20);                           // Create pbuf with 20 bytes of memory

esp_pbuf_cat(a, b);                             // Link them together
esp_pbuf_free(a);                               // If we call this, it will free pbuf for a and b together

// From now on, operating with b variable has undefined behavior, we should not use it anymore.
// The best way would be to do
b = NULL;                                       // We are not pointing anymore here
\endcode
 *
 * If we need to link pbuf to another pbuf and we still need to use variable, then use \ref esp_pbuf_chain instead.
 *
 * \image html pbuf_cat_vs_chain_3.svg Pbufs structure after calling esp_pbuf_chain
 *
 * When we use this method, second pbuf has reference set to <b>2</b> 
 * and now it is perfectly valid to use our user variable 2 to access the memory.
 *
 * Once we are done using pbuf, we have to free it using \ref esp_pbuf_free function.
 *
 * \code{c}
esp_pbuf_p a, b;
                                                
a = esp_pbuf_new(10);                           // Create pbuf with 10 bytes of memory
b = esp_pbuf_new(20);                           // Create pbuf with 20 bytes of memory

esp_pbuf_chain(a, b);                           // Link them together and increase reference count on b
esp_pbuf_free(a);                               // If we call this, it will free only first pbuf
                                                // As there is link to b pbuf somewhere

// From now on, we can still use b as reference to some pbuf,
// but when we are finish using it, we have to free the memory
esp_pbuf_free(b);                               // Try to free our memory. 
                                                // If reference is more than 1, 
                                                // It will not be removed from memory but only reference will be decreased
b = NULL                                        // After free call, do not use b anymore
\endcode
 *
 * \}
 */