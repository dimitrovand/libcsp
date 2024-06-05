/*****************************************************************************
 * **File:** csp/csp_sfp.h
 *
 * **Description:** Simple Fragmentation Protocol (SFP).
 *
 * The SFP API can transfer a blob of data across an established CSP connection,
 * by chopping the data into smaller chuncks of data, that can fit into a single CSP message.
 *
 * SFP will add a small header to each packet, containing information about the transfer.
 * SFP is usually sent over a RDP connection (which also adds a header),
 ****************************************************************************/
#pragma once

#include <string.h> // memcpy()

#include <csp/csp_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Structure to be passed as an input parameter to csp_sfp_send(...).
 * This structure encapsulates user-defined data and a function for reading data from storage.
 */
typedef struct {
    /**
     * User-defined data. The SFP does not interpret or manipulate this data.
     * It can be any user-defined data such as a file handle, a raw buffer pointer, or any other 
     * relevant information required for the implementation.
     */
    void * data;

    /** 
     * User-defined function for reading data from any type of storage.
     * 
     * @param[in] user_data Pointer to user-specific data required for the read operation.
     * @param[out] data Pointer to the buffer where the read data should be copied.
     * @param[in] offset The offset in the storage from where the data should be read.
     * @param[in] size The number of bytes to read from the storage.
     * 
     * @return Read bytes, or a value different from \a size to terminate.
     */
    uint16_t (* read)(void * user_data, uint8_t * data, uint32_t offset, uint16_t size);
} csp_sfp_read_t;

/**
 * Structure to be passed as an input parameter to csp_sfp_recv(...).
 * This structure encapsulates user-defined data and a function for writing data to storage.
 */
typedef struct {
    /**
     * User-defined data. The SFP does not interpret or manipulate this data.
     * It can be any user-defined data such as a file handle, a raw buffer pointer, or any other 
     * relevant information required for the implementation.
     */
    void * data;

    /** 
     * User-defined function for writing data to any type of storage.
     * 
     * @param[in] user_data Pointer to user-specific data required for the write operation.
     * @param[in] data Pointer to the buffer containing the data to be written.
     * @param[in] offset The offset in the storage where the data should be written.
     * @param[in] size The number of bytes to write to the storage.
     * @param[in] totalsize total size to write
     * 
     * @return Written bytes, or a value different from \a size to terminate.
     */
    uint16_t (* write)(void * user_data, const uint8_t * data, uint32_t offset, uint16_t size, uint32_t totalsize);
} csp_sfp_write_t;

/**
 * Send data over a CSP connection.
 *
 * Data will be send in chunks of \a mtu bytes. The MTU must be small enough to fit
 * into a CSP packat + SFP header + other transport headers.
 *
 * csp_sfp_recv() or csp_sfp_recv_fp() can be used at the other end to receive data.
 *
 * This is usefull if you wish to send data stored in flash memory or another location, where standard memcpy() doesn't work.
 *
 * @param[in] conn established connection for sending SFP packets.
 * @param[in] user User-defined data.
 * @param[in] totalsize Total size to send.
 * @param[in] mtu  maximum transfer unit (bytes), max data chunk to send.
 * @return #CSP_ERR_NONE on success, otherwise an error.
 */
int csp_sfp_send(csp_conn_t * conn, const csp_sfp_read_t * user, uint32_t totalsize, uint32_t mtu);
	
/**
 * Receive data over a CSP connection.
 *
 * This is the counterpart to the csp_sfp_send().
 *
 * @param[in] conn established connection for receiving SFP packets.
 * @param[in] user User-defined data
 * @param[out] datasize size of received data.
 * @param[in] timeout timeout in ms to wait for csp_read()
 * @param[in] first_packet First packet of a SFP transfer.
 * 			  Use NULL to receive first packet on the connection.
 * @return #CSP_ERR_NONE on success, otherwise an error.
 */
int csp_sfp_recv_fp(csp_conn_t * conn, const csp_sfp_write_t * user, uint32_t * datasize, uint32_t timeout, csp_packet_t * first_packet);

/**
 * Receive data over a CSP connection.
 *
 * This is the counterpart to the csp_sfp_send().
 *
 * @param[in] conn established connection for receiving SFP packets.
 * @param[in] user User-defined data
 * @param[out] datasize size of received data.
 * @param[in] timeout timeout in ms to wait for csp_read()
 * @return #CSP_ERR_NONE on success, otherwise an error.
*/
static inline int csp_sfp_recv(csp_conn_t * conn, const csp_sfp_write_t * user, uint32_t * datasize, uint32_t timeout) {
	return csp_sfp_recv_fp(conn, user, datasize, timeout, NULL);
}

#ifdef __cplusplus
}
#endif
