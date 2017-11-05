/**	
 * \file            esp_buff.c
 * \brief           buff manager
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
#include "esp_buff.h"
#include "esp_mem.h"

uint8_t
esp_buff_init(esp_buff_t* buff, uint32_t size) {
    if (buff == NULL) {                         /* Check buffer structure */
        return 0;
    }
    memset(buff, 0, sizeof(*buff));             /* Set buffer values to all zeros */

    buff->size = size;                          /* Set default values */
    buff->buff = esp_mem_alloc(size);           /* Allocate memory for buffer */
    if (!buff->buff) {                          /* Check allocation */
        return 0;
    }

    return 1;                                   /* Initialized OK */
}

void
esp_buff_free(esp_buff_t* buff) {
    if (buff) {
        esp_mem_free(buff->buff);               /* Free memory */
    }
}

size_t
esp_buff_write(esp_buff_t* buff, const void* data, uint32_t count) {
    uint32_t i = 0;
    uint32_t free;
    const uint8_t* d = data;
    uint32_t tocopy;

    if (buff == NULL || count == 0) {           /* Check buffer structure */
        return 0;
    }
    if (buff->in >= buff->size) {               /* Check input pointer */
        buff->in = 0;
    }
    free = esp_buff_get_free(buff);             /* Get free memory */
    if (free < count) {                         /* Check available memory */	
        if (free == 0) {                        /* If no memory, stop execution */
            return 0;
        }
        count = free;                           /* Set values for write */
    }

    /* We have calculated memory for write */
    tocopy = buff->size - buff->in;             /* Calculate number of elements we can put at the end of buffer */
    if (tocopy > count) {                       /* Check for copy count */
        tocopy = count;
    }
    memcpy(&buff->buff[buff->in], d, tocopy);   /* Copy content to buffer */
    i += tocopy;                                /* Increase number of bytes we copied already */
    buff->in += tocopy;	
    count -= tocopy;
    if (count > 0) {                            /* Check if anything to write */	
        memcpy(buff->buff, (void *)&d[i], count);   /* Copy content */
        buff->in = count;									/* Set input pointer */
    }
    if (buff->in >= buff->size) {               /* Check input overflow */
        buff->in = 0;
    }
    return (i + count);                         /* Return number of elements stored in memory */
}

size_t
esp_buff_read(esp_buff_t* buff, void* data, size_t count) {
    uint8_t *d = data;
    size_t i = 0, full;
    size_t tocopy;

    if (buff == NULL || count == 0) {           /* Check buffer structure */
        return 0;
    }
    if (buff->out >= buff->size) {              /* Check output pointer */
        buff->out = 0;
    }
    full = esp_buff_get_full(buff);             /* Get free memory */
    if (full < count) {                         /* Check available memory */
        if (full == 0) {                        /* If no memory, stop execution */
            return 0;
        }
        count = full;                           /* Set values for write */
    }

    tocopy = buff->size - buff->out;            /* Calculate number of elements we can read from end of buffer */
    if (tocopy > count) {                       /* Check for copy count */
        tocopy = count;
    }
    memcpy(d, &buff->buff[buff->out], tocopy);  /* Copy content from buffer */
    i += tocopy;                                /* Increase number of bytes we copied already */
    buff->out += tocopy;
    count -= tocopy;
    if (count > 0) {                            /* Check if anything to read */
        memcpy(&d[i], buff->buff, count);       /* Copy content */
        buff->out = count;                      /* Set input pointer */
    }
    if (buff->out >= buff->size) {              /* Check output overflow */
        buff->out = 0;
    }
    return (i + count);                         /* Return number of elements stored in memory */
}

size_t
esp_buff_get_free(esp_buff_t* buff) {
    size_t size, in, out;

    if (buff == NULL) {                         /* Check buffer structure */
        return 0;
    }
    in = buff->in;                              /* Save values */
    out = buff->out;
    if (in == out) {                            /* Check if the same */
        size = buff->size;
    } else if (out > in) {                      /* Check normal mode */
        size = out - in;
    } else {                                    /* Check if overflow mode */
        size = buff->size - (in - out);
    }
    return size - 1;                            /* Return free memory */
}

size_t
esp_buff_get_full(esp_buff_t* buff) {
    size_t in, out, size;

    if (buff == NULL) {                         /* Check buffer structure */
        return 0;
    }
    in = buff->in;                              /* Save values */
    out = buff->out;
    if (in == out) {                            /* Pointer are same? */
        size = 0;
    } else if (in > out) {                      /* buff is not in overflow mode */
        size = in - out;
    } else {                                    /* buff is in overflow mode */
        size = buff->size - (out - in);
    }
    return size;                                /* Return number of elements in buffer */
}

void
esp_buff_reset(esp_buff_t* buff) {
	if (buff == NULL) {                         /* Check buffer structure */
		return;
	}
	buff->in = 0;                               /* Reset values */
	buff->out = 0;
}
