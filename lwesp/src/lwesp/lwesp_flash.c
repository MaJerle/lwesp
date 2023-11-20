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

/**
 * \brief           Write data to flash partition
 * \param[in]       partition: Partition to write to
 * \param[in]       offset: Offset from start of partition to start writing at
 * \param[in]       data: Actual data to write. Must not be `NULL`
 * \param[in]       length: Number of bytes to write
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
lwespr_t
lwesp_flash_write(lwesp_flash_partition_t partition, uint32_t offset, const void* data, uint32_t length,
                  const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    LWESP_MSG_VAR_DEFINE(msg);

    LWESP_ASSERT(length > 0);
    LWESP_ASSERT(data != NULL);
    LWESP_ASSERT(partition < LWESP_FLASH_PARTITION_END);

    LWESP_MSG_VAR_ALLOC(msg, blocking);
    LWESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    LWESP_MSG_VAR_REF(msg).cmd_def = LWESP_CMD_SYSFLASH_WRITE;
    LWESP_MSG_VAR_REF(msg).msg.flash_write.partition = partition;
    LWESP_MSG_VAR_REF(msg).msg.flash_write.offset = offset;
    LWESP_MSG_VAR_REF(msg).msg.flash_write.length = length;
    LWESP_MSG_VAR_REF(msg).msg.flash_write.data = data;

    return lwespi_send_msg_to_producer_mbox(&LWESP_MSG_VAR_REF(msg), lwespi_initiate_cmd, 5000);
}

/**
 * \brief           Write key-value pair into user MFG area.
 * 
 * \note            When writing into this section, no need to previously erase the data
 *                  System is smart enough to do this for us, if absolutely necessary
 * 
 * \param           namespace: User namespace option
 * \param           key: Key to write
 * \param           valtype: Value type to follow
 * \param           data: Pointer to data to write. If value type is primitive type,
 *                      then pointer is copied to the local structure. This means
 *                      even for non-blocking calls, user can safely use local variables for
 *                      data pointers.
 * \param           length: Length of data to write. It only makes sense for string and binary data types,
 *                      otherwise it is derived from value type parameter and can be set to `0` by user
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
lwespr_t
lwesp_mfg_write(lwesp_mfg_namespace_t namespace, const char* key, lwesp_mfg_valtype_t valtype, const void* data,
                uint32_t length, const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    LWESP_MSG_VAR_DEFINE(msg);

    LWESP_ASSERT(namespace < LWESP_MFG_NAMESPACE_END);
    LWESP_ASSERT(data != NULL);
    LWESP_ASSERT(valtype != LWESP_MFG_VALTYPE_INVAL && valtype < LWESP_MFG_VALTYPE_END);
    switch (valtype) {
        case LWESP_MFG_VALTYPE_U8:
        case LWESP_MFG_VALTYPE_I8: length = 1; break;
        case LWESP_MFG_VALTYPE_U16:
        case LWESP_MFG_VALTYPE_I16: length = 2; break;
        case LWESP_MFG_VALTYPE_U32:
        case LWESP_MFG_VALTYPE_I32: length = 4; break;
        default: break; /* Length as-is */
    }
    LWESP_ASSERT(length > 0);

    LWESP_MSG_VAR_ALLOC(msg, blocking);
    LWESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    LWESP_MSG_VAR_REF(msg).cmd_def = LWESP_CMD_SYSMFG_WRITE;
    LWESP_MSG_VAR_REF(msg).msg.mfg_write_read.namespace = namespace;
    LWESP_MSG_VAR_REF(msg).msg.mfg_write_read.key = key;
    LWESP_MSG_VAR_REF(msg).msg.mfg_write_read.valtype = valtype;
    LWESP_MSG_VAR_REF(msg).msg.mfg_write_read.length = length;
    if (LWESP_MFG_VALTYPE_IS_PRIM(valtype)) {
        switch (valtype) {
            case LWESP_MFG_VALTYPE_U8: LWESP_MSG_VAR_REF(msg).msg.mfg_write_read.data_prim.u8 = *(uint8_t*)data; break;
            case LWESP_MFG_VALTYPE_I8: LWESP_MSG_VAR_REF(msg).msg.mfg_write_read.data_prim.i8 = *(int8_t*)data; break;
            case LWESP_MFG_VALTYPE_U16: LWESP_MSG_VAR_REF(msg).msg.mfg_write_read.data_prim.u16 = *(uint16_t*)data; break;
            case LWESP_MFG_VALTYPE_I16: LWESP_MSG_VAR_REF(msg).msg.mfg_write_read.data_prim.i16 = *(int16_t*)data; break;
            case LWESP_MFG_VALTYPE_U32: LWESP_MSG_VAR_REF(msg).msg.mfg_write_read.data_prim.u32 = *(uint32_t*)data; break;
            case LWESP_MFG_VALTYPE_I32: LWESP_MSG_VAR_REF(msg).msg.mfg_write_read.data_prim.i32 = *(int32_t*)data; break;
            default: break; /* Length as-is */
        }
    } else {
        LWESP_MSG_VAR_REF(msg).msg.mfg_write_read.data_ptr = data;
    }

    return lwespi_send_msg_to_producer_mbox(&LWESP_MSG_VAR_REF(msg), lwespi_initiate_cmd, 5000);
}

/**
 * \brief           Read key-value pair from user MFG area.
 *
 *
 * \param           namespace: User namespace option
 * \param           key: Key to read
 * \param           data: Pointer to buffer to write data to.
 * \param           len: Size of data buffer
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
lwespr_t
lwesp_mfg_read(lwesp_mfg_namespace_t namespace, const char* key, const void* data, uint32_t len,
                const lwesp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    LWESP_MSG_VAR_DEFINE(msg);

    LWESP_ASSERT(namespace < LWESP_MFG_NAMESPACE_END);
    LWESP_ASSERT(data != NULL);

    LWESP_MSG_VAR_ALLOC(msg, blocking);
    LWESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    LWESP_MSG_VAR_REF(msg).cmd_def = LWESP_CMD_SYSMFG_READ;
    LWESP_MSG_VAR_REF(msg).msg.mfg_write_read.namespace = namespace;
    LWESP_MSG_VAR_REF(msg).msg.mfg_write_read.key = key;
    LWESP_MSG_VAR_REF(msg).msg.mfg_write_read.data_ptr = data;
    LWESP_MSG_VAR_REF(msg).msg.mfg_write_read.length = len;
    LWESP_MSG_VAR_REF(msg).msg.mfg_write_read.wait_second_ok = 0;

    return lwespi_send_msg_to_producer_mbox(&LWESP_MSG_VAR_REF(msg), lwespi_initiate_cmd, 5000);
}

#endif /* LWESP_CFG_FLASH || __DOXYGEN__ */
