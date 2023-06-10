/**
 * \file            lwesp_flash.c
 * \brief           System flash API
 */

/*
 * Copyright (c) 2023 Tilen MAJERLE
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
 * This file is part of LwESP - Lightweight ESP-AT parser library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         v1.1.2-dev
 */
#include "lwesp/lwesp_flash.h"
#include "lwesp/lwesp_private.h"

#if LWESP_CFG_FLASH || __DOXYGEN__

/**
 * \brief           Erase flash block
 * \param[in]       partition: Partition to do erase operation on
 * \param[in]       offset: Offset from start of partition. Must be `4kB` aligned when used.
 *                      Set to `0` to erase full partition
 * \param[in]       length: Size to erase. Must be `4kB` aligned when used.
 *                      Set to `0` to erase full partition
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
lwespr_t
lwesp_flash_erase(lwesp_flash_partition_t partition, uint32_t offset, uint32_t length,
                  const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    LWESP_MSG_VAR_DEFINE(msg);

    /* Check alignment if used */
    LWESP_ASSERT((offset == 0 || (offset & 0x0FFF) == 0) && (length == 0 || (length & 0x0FFF) == 0));

    LWESP_MSG_VAR_ALLOC(msg, blocking);
    LWESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    LWESP_MSG_VAR_REF(msg).cmd_def = LWESP_CMD_SYSFLASH_ERASE;
    LWESP_MSG_VAR_REF(msg).msg.flash_erase.partition = partition;
    LWESP_MSG_VAR_REF(msg).msg.flash_erase.offset = offset;
    LWESP_MSG_VAR_REF(msg).msg.flash_erase.length = length;

    return lwespi_send_msg_to_producer_mbox(&LWESP_MSG_VAR_REF(msg), lwespi_initiate_cmd, 5000);
}

lwespr_t
lwesp_flash_write(lwesp_flash_partition_t partition, uint32_t offset, uint32_t length, const void* data,
                  const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    LWESP_MSG_VAR_DEFINE(msg);
    
    LWESP_ASSERT(length > 0);

    /* 
     * Check alignment if used - according to the ESP-AT commands,
     * it is necessary to
     */
    if (partition == LWESP_FLASH_PARTITION_CLIENT_CA || partition == LWESP_FLASH_PARTITION_CLIENT_CERT
        || partition == LWESP_FLASH_PARTITION_CLIENT_KEY) {
        /* TODO: Do we need server too? */
        LWESP_ASSERT((length & 0x03) == 0);
    }

    LWESP_MSG_VAR_ALLOC(msg, blocking);
    LWESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    LWESP_MSG_VAR_REF(msg).cmd_def = LWESP_CMD_SYSFLASH_WRITE;
    LWESP_MSG_VAR_REF(msg).msg.flash_write.partition = partition;
    LWESP_MSG_VAR_REF(msg).msg.flash_write.offset = offset;
    LWESP_MSG_VAR_REF(msg).msg.flash_write.length = length;
    LWESP_MSG_VAR_REF(msg).msg.flash_write.data = data;

    return lwespi_send_msg_to_producer_mbox(&LWESP_MSG_VAR_REF(msg), lwespi_initiate_cmd, 5000);
}

#endif /* LWESP_CFG_FLASH || __DOXYGEN__ */
