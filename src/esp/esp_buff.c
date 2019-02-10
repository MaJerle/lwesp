/**
 * \file            esp_buff.c
 * \brief           Buff manager
 */

/*
 * Copyright (c) 2018 Tilen Majerle
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
 * This file is part of ESP-AT library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */
#include "esp/esp_private.h"
#include "esp/esp_buff.h"
#include "esp/esp_mem.h"

#define BUF_MEMSET                      ESP_MEMSET
#define BUF_MEMCPY                      ESP_MEMCPY

/**
 * \brief           Initialize buffer
 * \param[in]       buff: Pointer to buffer structure
 * \param[in]       size: Size of buffer in units of bytes
 * \return          `1` on success, `0` otherwise
 */
uint8_t
esp_buff_init(esp_buff_t* buff, size_t size) {
    if (buff == NULL || size == 0) {
        return 0;
    }
    BUF_MEMSET(buff, 0, sizeof(*buff));         /* Set buffer values to all zeros */

    buff->size = size;                          /* Set default values */
    buff->buff = esp_mem_alloc(sizeof(*buff->buff) * size); /* Allocate memory for buffer */
    if (buff->buff == NULL) {
        return 0;
    }
    return 1;                                   /* Initialized OK */
}

/**
 * \brief           Free dynamic allocation if used on memory
 * \param[in]       buff: Pointer to buffer structure
 */
void
esp_buff_free(esp_buff_t* buff) {
    if (buff != NULL && buff->buff != NULL) {
        esp_mem_free(buff->buff);
        buff->buff = NULL;
    }
}

/**
 * \brief           Write data to buffer
 * \param[in]       buff: Pointer to buffer structure
 * \param[in]       data: Pointer to data to copy memory from
 * \param[in]       count: Number of bytes we want to write
 * \return          Number of bytes actually written to buffer
 */
size_t
esp_buff_write(esp_buff_t* buff, const void* data, size_t count) {
    size_t tocopy, free;
    const uint8_t* d = data;

    if (buff == NULL || buff->buff == NULL || count == 0) {
        return 0;
    }

    if (buff->w >= buff->size) {                /* Check input pointer */
        buff->w = 0;                            /* On normal use, this should never happen */
    }

    /* Calculate maximum number of bytes we can write */
    free = esp_buff_get_free(buff);
    if (free < count) {
        if (free == 0) {
            return 0;
        }
        count = free;
    }

    /* Write data to linear part of buffer */
    tocopy = buff->size - buff->w;              /* Calculate number of elements we can put at the end of buffer */
    if (tocopy > count) {
        tocopy = count;
    }
    BUF_MEMCPY(&buff->buff[buff->w], d, tocopy);
    buff->w += tocopy;
    count -= tocopy;

    /* Write data to overflow part of buffer */
    if (count > 0) {
        BUF_MEMCPY(buff->buff, (void *)&d[tocopy], count);
        buff->w = count;
    }
    if (buff->w >= buff->size) {                /* Check input overflow */
        buff->w = 0;
    }
    return tocopy + count;                      /* Return number of elements stored in memory */
}

/**
 * \brief           Read data from buffer
 * \param[in]       buff: Pointer to buffer structure
 * \param[out]      data: Pointer to data to copy memory to
 * \param[in]       count: Number of bytes we want to read
 * \return          Number of bytes actually read and saved to data variable
 */
size_t
esp_buff_read(esp_buff_t* buff, void* data, size_t count) {
    size_t tocopy, full;
    uint8_t *d = data;

    if (buff == NULL || buff->buff == NULL || count == 0) {
        return 0;
    }

    if (buff->r >= buff->size) {                /* Check output pointer */
        buff->r = 0;                            /* On normal use, this should never happen */
    }

    /* Calculate maximum number of bytes we can read */
    full = esp_buff_get_full(buff);
    if (full < count) {
        if (full == 0) {
            return 0;
        }
        count = full;
    }

    /* Read data from linear part of buffer */
    tocopy = buff->size - buff->r;
    if (tocopy > count) {
        tocopy = count;
    }
    BUF_MEMCPY(d, &buff->buff[buff->r], tocopy);
    buff->r += tocopy;
    count -= tocopy;

    /* Read data from overflow part of buffer */
    if (count > 0) {
        BUF_MEMCPY(&d[tocopy], buff->buff, count);
        buff->r = count;
    }
    if (buff->r >= buff->size) {                /* Check output overflow */
        buff->r = 0;
    }
    return tocopy + count;                      /* Return number of elements stored in memory */
}

/**
 * \brief           Read from buffer but do not change read pointer
 * \param[in]       buff: Pointer to buffer structure
 * \param[in]       skip_count: Number of bytes to skip before reading data
 * \param[out]      data: Pointer to data to save read memory
 * \param[in]       count: Number of bytes to peek
 * \return          Number of bytes written to data array
 */
size_t
esp_buff_peek(esp_buff_t* buff, size_t skip_count, void* data, size_t count) {
    size_t full, tocopy, r;
    uint8_t *d = data;

    if (buff == NULL || buff->buff == NULL || count == 0) {
        return 0;
    }

    if (buff->r >= buff->size) {                /* Check output pointer */
        buff->r = 0;                            /* On normal use, this should never happen */
    }
    r = buff->r;

    /* Calculate maximum number of bytes we can read */
    full = esp_buff_get_full(buff);

    /* Skip beginning of buffer */
    if (skip_count >= full) {
        return 0;
    }
    r += skip_count;
    full -= skip_count;
    if (r >= buff->size) {
        r -= buff->size;
    }

    /* Check if we can read something after skip */
    if (full < count) {
        if (full == 0) {
            return 0;
        }
        count = full;
    }

    /* Read data from linear part of buffer */
    tocopy = buff->size - r;
    if (tocopy > count) {
        tocopy = count;
    }
    BUF_MEMCPY(d, &buff->buff[r], tocopy);
    count -= tocopy;

    /* Read data from overflow part of buffer */
    if (count > 0) {
        BUF_MEMCPY(&d[tocopy], buff->buff, count);
    }
    return tocopy + count;                      /* Return number of elements stored in memory */
}

/**
 * \brief           Get number of bytes in buffer available to write
 * \param[in]       buff: Pointer to buffer structure
 * \return          Number of free bytes in memory
 */
size_t
esp_buff_get_free(esp_buff_t* buff) {
    size_t size, w, r;

    if (buff == NULL || buff->buff == NULL) {
        return 0;
    }

    /* Operate on temporary values in case they change in between */
    w = buff->w;
    r = buff->r;
    if (w == r) {
        size = buff->size;
    } else if (r > w) {
        size = r - w;
    } else {
        size = buff->size - (w - r);
    }

    /* Buffer free size is always 1 less than actual size */
    return size - 1;
}

/**
 * \brief           Get number of bytes in buffer available to read
 * \param[in]       buff: Pointer to buffer structure
 * \return          Number of bytes ready to be read
 */
size_t
esp_buff_get_full(esp_buff_t* buff) {
    size_t w, r, size;

    if (buff == NULL || buff->buff == NULL) {
        return 0;
    }

    /* Operate on temporary values in case they change in between */
    w = buff->w;
    r = buff->r;
    if (w == r) {
        size = 0;
    } else if (w > r) {
        size = w - r;
    } else {
        size = buff->size - (r - w);
    }
    return size;
}

/**
 * \brief           Resets and clears buffer
 * \param[in]       buff: Pointer to buffer structure
 */
void
esp_buff_reset(esp_buff_t* buff) {
    if (buff == NULL) {
        return;
    }
    buff->w = 0;
    buff->r = 0;
}

/**
 * \brief           Get linear address for buffer for fast read
 * \param[in]       buff: Pointer to buffer
 * \return          Linear buffer start address
 */
void *
esp_buff_get_linear_block_address(esp_buff_t* buff) {
    if (buff == NULL || buff->buff == NULL) {
        return NULL;
    }
    return &buff->buff[buff->r];
}

/**
 * \brief           Get length of linear block address before it overflows
 * \param[in]       buff: Pointer to buffer
 * \return          Linear buffer size in units of bytes
 */
size_t
esp_buff_get_linear_block_length(esp_buff_t* buff) {
    size_t w, r, len;

    if (buff == NULL) {
        return 0;
    }

    /* Operate on temporary values in case they change in between */
    w = buff->w;
    r = buff->r;
    if (w > r) {
        len = w - r;
    } else if (r > w) {
        len = buff->size - r;
    } else {
        len = 0;
    }
    return len;
}

/**
 * \brief           Skip (ignore) buffer data.
 * \note            Useful at the end of streaming transfer such as DMA
 * \param[in]       buff: Pointer to buffer structure
 * \param[in]       len: Number of bytes to skip
 * \return          Number of bytes skipped
 */
size_t
esp_buff_skip(esp_buff_t* buff, size_t len) {
    size_t full;

    if (buff == NULL || len == 0) {
        return 0;
    }

    full = esp_buff_get_full(buff);             /* Get buffer used length */
    if (len > full) {
        len = full;
    }
    buff->r += len;                             /* Advance buffer */
    if (buff->r >= buff->size) {                /* Subtract possible overflow */
        buff->r -= buff->size;                  /* Do subtract */
    }
    return len;
}
