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
            ESP_DEBUGF(ESP_DBG_PBUF, "PBUF: Deallocating %p\r\n", p);
            pn = p->next;                       /* Save next entry */
            esp_mem_free(p);                    /* Free memory for pbuf */
            p = pn;                             /* Restore with next entry */
            cnt++;                              /* Increase number of freed pbufs */
        } else {
            p = NULL;                           /* Stop at this point */
        }
    }
    return cnt;
}

/**
 * \brief           Contencate 2 packet buffers together to one big packet
 * \note            After tail pbuf has been added to head pbuf chain,
 *                  it must not be referenced by user anymore as it is now completelly replaced by head pbuf.
 *                  In simple words, when user calls this function, it should not call any esp_pbuf_* function anymore,
 *                  not even \ref esp_pbuf_free as it will make memory undefined for head pbuf.
 * \param[in]       head: Head packet buffer to append new pbuf to
 * \param[in]       tail: Tail packet buffer to append to head pbuf
 * \return          espOK on success, member of \ref espr_t otherwise
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
 * \brief           Chain 2 pbufs together. Similar to \ref esp_pbuf_chain
 *                  but now new reference is done from head pbuf to tail pbuf.
 * \note            After this function call, user must call \ref esp_pbuf_free(tail) to remove
 *                  its reference to tail pbuf and allow control to head pbuf
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
 * \brief           Get value from pbuf at specific position
 * \param[in]       pbuf: Pbuf used to get data from
 * \param[in]       pos: Position at which to get element
 * \return          Element at position
 */
uint8_t
pbuf_get_at(esp_pbuf_p pbuf, size_t pos) {
    return 0;
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
        if (pbuf_get_at(p, offset + i) != d[i]) {   /* Get value from pbuf at specific offset */
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
esp_pbuf_set_ip(esp_pbuf_p pbuf, void* ip, uint16_t port) {
    if (pbuf && ip) {
        memcpy(pbuf->ip, ip, 4);
        pbuf->port = port;
    }
}
