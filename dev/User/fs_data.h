#ifndef __FS_DATA_H
#define __FS_DATA_H

#include "esp.h"

typedef struct {
    const char* path;
    const void* data;
    uint32_t len;
    uint8_t is_404;
} fs_file_t;

fs_file_t*      fs_data_open_file(esp_pbuf_p pbuf, uint8_t is_get);
void            fs_data_close_file(fs_file_t* file);

#endif
