/**	
 * \file            esp_pbuf.c
 * \brief           Packet buffer manager
 */
 
/*
 * Copyright (c) 2017 Tilen Majerle
 *  
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */
#define ESP_INTERNAL
#include "include/esp_private.h"
#include "include/esp_pbuf.h"
#include "include/esp_mem.h"

/**
 * \brief           Skip pbufs for desired offset
 * \param[in]       p: Source pbuf to skip
 * \param[in]       off: Offset in units of bytes to skip
 * \param[out]      new_off: New offset on new returned pbuf
 * \return          New pbuf where offset was found or NULL if offset too big for pbuf chain
 */
static esp_pbuf_p
pbuf_skip(esp_pbuf_p p, size_t off, size_t* new_off) {
    if (!p || p->tot_len < off) {               /* Check valid parameters */
        return NULL;
    }
    
    /**
     * Skip pbufs until we reach pbuf where offset is placed
     */
    for (; p && p->len <= off; p = p->next) {
        off -= p->len;                          /* Decrease offset by current pbuf length */
    }
    
    if (new_off) {                              /* Check new offset */
        *new_off = off;                         /* Set offset to output variable */
    }
    return p;
}

/**
 * \brief           Allocate packet buffer for network data of specific size
 * \param[in]       len: Length of payload memory to allocate
 * \return          Pointer to allocated memory or NULL in case of failure
 */
esp_pbuf_p
esp_pbuf_new(size_t len) {
    esp_pbuf_p p;
    
    p = esp_mem_calloc(1, ESP_MEM_ALIGN(sizeof(*p)) + len); /* Allocate memory for packet buffer */
    ESP_DEBUGW(ESP_DBG_PBUF, p == NULL, "PBUF: Failed to allocate %d bytes\r\n", (int)len);
    ESP_DEBUGW(ESP_DBG_PBUF, p != NULL, "PBUF: Allocated %d bytes on %p\r\n", (int)len, p);
    if (p) {
        p->next = NULL;                         /* No next element in chain */
        p->tot_len = len;                       /* Set total length of pbuf chain */
        p->len = len;                           /* Set payload length */
        p->payload = (uint8_t *)(((char *)p) + ESP_MEM_ALIGN(sizeof(*p)));  /* Set pointer to payload data */
        p->ref = 1;                             /* Single reference is used on this pbuf */
    }
    return p;
}

/**
 * \brief           Free previously allocated packet buffer
 * \param[in]       pbuf: Packet buffer to free
 * \return          Number of freed pbufs from head
 */
uint16_t
esp_pbuf_free(esp_pbuf_p pbuf) {
    esp_pbuf_p p, pn;
    uint16_t ref, cnt;
    
    ESP_ASSERT("pbuf != NULL", pbuf != NULL);   /* Assert input parameters */
    
    /**
     * Free all pbufs until first ->ref > 1 is reached
     * which means somebody has reference to part of pbuf and we have to keep it as is
     */
    cnt = 0;
    for (p = pbuf; p;) {
        ref = --p->ref;                         /* Decrease current value and save it */
        if (ref == 0) {                         /* Did we reach 0 and are ready to free it? */
            ESP_DEBUGF(ESP_DBG_PBUF, "PBUF: Deallocating %p with len/tot_len: %d/%d\r\n", p, (int)p->len, (int)p->tot_len);
            pn = p->next;                       /* Save next entry */
            esp_mem_free(p);                    /* Free memory for pbuf */
            p = pn;                             /* Restore with next entry */
            cnt++;                              /* Increase number of freed pbufs */
        } else {
            break;
        }
    }
    return cnt;
}

/**
 * \brief           Contencate 2 packet buffers together to one big packet
 * \note            After tail pbuf has been added to head pbuf chain,
 *                  it must not be referenced by user anymore as it is now completelly controlled by head pbuf.
 *                  In simple words, when user calls this function, it should not call esp_pbuf_free function anymore,
 *                  as it might make memory undefined for head pbuf.
 * \param[in]       head: Head packet buffer to append new pbuf to
 * \param[in]       tail: Tail packet buffer to append to head pbuf
 * \return          espOK on success, member of \ref espr_t otherwise
 * \sa              esp_pbuf_chain
 */
espr_t
esp_pbuf_cat(esp_pbuf_p head, esp_pbuf_p tail) {
    ESP_ASSERT("head != NULL", head != NULL);   /* Assert input parameters */
    ESP_ASSERT("tail != NULL", tail != NULL);   /* Assert input parameters */
    
    /**
     * For all pbuf packets in head,
     * increase total length parameter of all next entries
     */
    for (; head->next != NULL; head = head->next) {
        head->tot_len += tail->tot_len;         /* Increase total length of packet */
    }
    head->tot_len += tail->tot_len;             /* Increase total length of last packet in chain */
    head->next = tail;                          /* Set next packet buffer as next one */
    
    return espOK;
}

/**
 * \brief           Chain 2 pbufs together. Similar to \ref esp_pbuf_cat
 *                  but now new reference is done from head pbuf to tail pbuf.
 * \note            After this function call, user must call \ref esp_pbuf_free(tail) to remove
 *                  its reference to tail pbuf and allow control to head pbuf
 * \param[in]       head: Head packet buffer to append new pbuf to
 * \param[in]       tail: Tail packet buffer to append to head pbuf
 * \return          espOK on success, member of \ref espr_t otherwise
 * \sa              esp_pbuf_cat
 */
espr_t
esp_pbuf_chain(esp_pbuf_p head, esp_pbuf_p tail) {
    espr_t res;
    
    /**
     * First contencate them together
     * Second create a new reference from head buffer to tail buffer
     * so user can normally use tail pbuf and free it when it wants    
     */
    if ((res = esp_pbuf_cat(head, tail)) == espOK) {    /* Did we contencate them together successfully? */
        esp_pbuf_ref(tail);                     /* Reference tail pbuf by head pbuf now */
    }
    return res;
}

/**
 * \brief           Increment reference count on pbuf
 * \param[in]       pbuf: pbuf to increase reference
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_pbuf_ref(esp_pbuf_p pbuf) {
    ESP_ASSERT("pbuf != NULL", pbuf != NULL);   /* Assert input parameters */
    
    pbuf->ref++;                                /* Increase reference count for pbuf */
    return espOK;
}

/**
 * \brief           Copy user data to chain of pbufs
 * \param[in]       pbuf: First pbuf in chain to start copying to
 */
espr_t
esp_pbuf_take(esp_pbuf_p pbuf, const void* data, size_t len, size_t offset) {
    const uint8_t* d = data;
    size_t copy_len;
    
    ESP_ASSERT("pbuf != NULL", pbuf != NULL);   /* Assert input parameters */
    ESP_ASSERT("data != NULL", data != NULL);   /* Assert input parameters */
    ESP_ASSERT("len > 0", len > 0);             /* Assert input parameters */
    ESP_ASSERT("pbuf->tot_len >= len", pbuf->tot_len >= len);   /* Assert input parameters */
    
    /**
     * Skip if necessary and check if we are in valid range
     */
    if (offset) {
        pbuf = pbuf_skip(pbuf, offset, &offset);    /* Offset and check for new length */
    }
    if (pbuf->tot_len < (len + offset)) {
        return espPARERR;
    }
    
    /**
     * First only copy in case we have some offset from first pbuf
     */
    if (offset) {
        size_t l = ESP_MIN(pbuf->len - offset, len);    /* Get length to copy to current pbuf */
        memcpy(pbuf->payload + offset, d, l);   /* Copy to memory with offset */
        len -= l;                               /* Decrease remaining bytes to copy */
        d += l;                                 /* Increase data pointer */
        pbuf = pbuf->next;                      /* Go to next pbuf */
    }
    
    /**
     * Copy user memory to sequence of pbufs
     */
    for (; len; pbuf = pbuf->next) {
        copy_len = len > pbuf->len ? pbuf->len : len;   /* Get copy length */
        memcpy(pbuf->payload, d, copy_len);     /* Copy memory to pbuf payload */
        len -= copy_len;                        /* Decrease number of remaining bytes to send */
        d += copy_len;                          /* Increase data pointer */
    }
    return espOK;
}

/**
 * \brief           Copy memory from pbuf to user linear memory
 * \param[in]       pbuf: Pbuf to copy from
 * \param[out]      data: User linear memory to copy to
 * \param[in]       len: Length of data in units of bytes
 * \param[in]       offset: Possible start offset in pbuf
 * \param[out]      copied: Pointer to output variable to save number of bytes copied to user memory
 * \return          Number of bytes copied
 */
size_t
esp_pbuf_copy(esp_pbuf_p pbuf, void* data, size_t len, size_t offset) {
    size_t tot, tc;
    uint8_t* d = data;
    
    if (!pbuf || !data || !len || pbuf->tot_len < offset) { /* Assert input parameters */
        return 0;
    }
    
    /**
     * In case user wants offset, 
     * skip to necessary pbuf
     */
    if (offset) {
        pbuf = pbuf_skip(pbuf, offset, &offset);    /* Skip offset if necessary */
    }
    
    /**
     * Copy data from pbufs to memory
     * with checking for initial offset (only one can have offset)
     */
    tot = 0;
    for (; pbuf && len; pbuf = pbuf->next) {
        tc = ESP_MIN(pbuf->len - offset, len);      /* Get length of data to copy */
        memcpy(d, pbuf->payload + offset, tc);      /* Copy data from pbuf */
        d += tc;
        len -= tc;
        tot += tc;
        offset = 0;                                 /* No more offset in this case */
    }
    return tot;
}

/**
 * \brief           Get value from pbuf at specific position
 * \param[in]       pbuf: Pbuf used to get data from
 * \param[in]       pos: Position at which to get element
 * \param[out]      el: Output variable to save element value at desired position
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_pbuf_get_at(const esp_pbuf_p pbuf, size_t pos, uint8_t* el) {
    esp_pbuf_p p;
    size_t off;
    
    if (pbuf) {
        p = pbuf_skip(pbuf, pos, &off);         /* Skip pbufs to desired position and get new offset from new pbuf */
        if (p) {                                /* Do we have new pbuf? */
            *el = p->payload[off];              /* Return memory at desired new offset from latest pbuf */
            return 1;
        }
    }
    return 0;                                   /* Invalid character */
}

size_t
esp_pbuf_memfind(const esp_pbuf_p pbuf, const void* data, size_t len, size_t off) {
    size_t i;
    if (pbuf && data && pbuf->tot_len >= (len + off)) { /* Check if valid entries */
        /**
         * Try entire buffer element by element
         * and in case we have a match, report it
         */
        for (i = off; i <= pbuf->tot_len - len; i++) {
            if (!esp_pbuf_memcmp(pbuf, i, data, len)) { /* CHeck if identical */
                return i;                       /* We have a match! */
            }
        }
    }
    return ESP_SIZET_MAX;                       /* Return maximal value of size_t variable to indicate error */
}

/**
 * \brief           Compare pbuf memory with memory from data
 * \note            Compare is done on entire pbuf chain
 * \param[in]       pbuf: Pbuf used to compare with data memory
 * \param[in]       offset: Start offset to use when comparing data
 * \param[in]       data: Actual data to compare with
 * \param[in]       len: Length of input data in units of bytes
 * \return          0 if equal, ESP_SIZET_MAX if memory/offset too big or anything between if not equal
 */
size_t
esp_pbuf_memcmp(const esp_pbuf_p pbuf, size_t offset, const void* data, size_t len) {
    esp_pbuf_p p;
    size_t i;
    uint8_t el;
    const uint8_t* d = data;
    
    if (pbuf == NULL || data == NULL || !len || /* Input parameters check */
        pbuf->tot_len < (offset + len)) {       /* Check of valid ranges */
        return ESP_SIZET_MAX;                   /* Invalid check here */
    }
    
    /**
     * Find start pbuf to have more optimized search at the end
     * Since we had a check on beginning, we must pass this for loop without any problems
     */
    for (p = pbuf; p != NULL && p->len <= offset; p = p->next) {
        offset -= p->len;                       /* Decrease offset by length of pbuf */
    }
    
    /**
     * We have known starting pbuf.
     * Now it is time to check byte by byte from pbuf and memory
     *
     * Use byte by byte read function to inspect bytes separatelly
     */
    for (i = 0; i < len; i++) {
        if (!esp_pbuf_get_at(p, offset + i, &el) || el != d[i]) {   /* Get value from pbuf at specific offset */
            return offset + 1;                  /* Return value from offset where it failed */
        }
    }
    return 0;                                   /* Memory matches at this point */
}

/**
 * \brief           Get data pointer from packet buffer
 * \param[in]       pbuf: Packet buffer
 * \return          Pointer to data buffer or NULL if invalid buffer
 */
const void *
esp_pbuf_data(const esp_pbuf_p pbuf) {
    return pbuf ? pbuf->payload : NULL;
}

/**
 * \brief           Get length of packet buffer
 * \param[in]       pbuf: Packet buffer to get length for
 * \param[in]       tot: Set to 1 to return total packet chain length or 0 to get only first packet length
 * \return          Length of data in units of bytes
 */
size_t
esp_pbuf_length(const esp_pbuf_p pbuf, uint8_t tot) {
    return pbuf ? (tot ? pbuf->tot_len : pbuf->len) : 0;
}

/**
 * \brief           Set IP address and port number for received data
 * \param[in]       pbuf: Packet buffer
 * \param[in]       ip: IP to assing to packet buffer
 * \param[in]       port: Port number to assign to packet buffer
 */
void
esp_pbuf_set_ip(esp_pbuf_p pbuf, const void* ip, uint16_t port) {
    if (pbuf && ip) {
        memcpy(pbuf->ip, ip, 4);
        pbuf->port = port;
    }
}
