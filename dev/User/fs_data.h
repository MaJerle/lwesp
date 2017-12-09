#ifndef __FS_DATA_H
#define __FS_DATA_H

#include "esp.h"

typedef struct {
    const char* path;
    const void* data;
    uint32_t len;
                                                
    uint8_t is_404;                             /*!< Flag in case file is 404 response */
    uint8_t is_asset;                           /*!< Flag in case file is asset file */
    uint8_t on_get;                             /*!< Flag in case file is allowed on GET method */
    uint8_t on_post;                            /*!< Flag in case file is allowed on POST method */
} fs_file_t;

const fs_file_t*    fs_data_open_file(const char* path, uint8_t is_404);
void                fs_data_close_file(const fs_file_t* file);

#endif
