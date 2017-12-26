#ifndef __FS_DATA_H
#define __FS_DATA_H

#include "esp/esp.h"

typedef struct {
    const char* path;
    const void* data;
    uint32_t len;
    
    uint8_t is_404;                             /*!< Flag in case file is 404 response */
    uint8_t is_asset;                           /*!< Flag in case file is asset file */
    uint8_t on_get;                             /*!< Flag in case file is allowed on GET method */
    uint8_t on_post;                            /*!< Flag in case file is allowed on POST method */
} fs_file_table_t;

typedef struct {
    const uint8_t* data;                        /*!< Pointer to data array in case file is static */
    uint8_t is_static;                          /*!< Flag indicating file is static and no dynamic read is required */
    
    uint32_t len;                               /*!< Total length of file */
    uint32_t fptr;                              /*!< File pointer to indicate next read position */
    
    uint32_t sent;                              /*!< Number of bytes sent in current send command */
    uint32_t sent_total;                        /*!< Total number of bytes sent on a file already */
} fs_file_t;

uint8_t     fs_data_open_file(fs_file_t* file, const char* path);
uint8_t     fs_data_read_file(fs_file_t* file, void* buff, size_t btr, size_t* br);
void        fs_data_close_file(fs_file_t* file);

#endif
