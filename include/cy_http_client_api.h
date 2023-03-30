/*
 * Copyright 2023, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */

/*******************************************************************************
* File Name: cy_http_client_api.h
*
* Description:
* API for the HTTP / HTTPS Client
*/

#ifndef CY_HTTP_CLIENT_API_H_
#define CY_HTTP_CLIENT_API_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cy_result_mw.h"
#include "cy_tcpip_port_secure_sockets.h"

/**
 * \defgroup group_c_api C APIs
 * \defgroup http_client_struct Structures and Enumerations
 * @ingroup group_c_api
 * \defgroup http_client_defines Macros
 * @ingroup group_c_api
 * \defgroup group_c_api_functions Functions
 * @ingroup group_c_api
 */

/**
 * @addtogroup http_client_defines
 *
 * HTTP Client library preprocessor directives such as results and error codes
 *
 * Cypress middleware APIs return results of type cy_rslt_t and consist of three parts:
 * - module base
 * - type
 * - error code
 *
 * \par Result Format
 *
   \verbatim
              Module base         Type    Library-specific error code
      +-------------------------+------+------------------------------+
      |          0x020E         | 0x2  |           Error Code         |
      +-------------------------+------+------------------------------+
                14 bits          2 bits            16 bits

   See the macro section of this document for library-specific error codes.
   \endverbatim
 *
 * The data structure cy_rslt_t is part of cy_result.h.
 * In ModusToolbox, the PSoC 6 MCU target platform is located in <core_lib/include>.
 *
 * Module base: This base is derived from CY_RSLT_MODULE_MIDDLEWARE_BASE (defined in cy_result.h) and is an offset of CY_RSLT_MODULE_MIDDLEWARE_BASE.
 *              Details of the offset and the middleware base are defined in cy_result_mw.h, which is part of [Github connectivity-utilities] (https://github.com/Infineon/connectivity-utilities).
 *              For example, the HTTP Client uses CY_RSLT_MODULE_HTTP_CLIENT as the module base, which is 0x020E.
 *
 * Type: This type is defined in cy_result.h and can be one of CY_RSLT_TYPE_FATAL, CY_RSLT_TYPE_ERROR, CY_RSLT_TYPE_WARNING, or CY_RSLT_TYPE_INFO. HTTP Client library error codes are of type CY_RSLT_TYPE_ERROR, which is 0x2.
 *
 * Library-specific error codes: These error codes are library-specific and defined in the macro section.
 *
 * Helper macros used for creating library-specific results are provided as part of cy_result.h.
 *
 *  @{
 */

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
/**
 */
/** HTTP Client library error code start. */
#define CY_RSLT_MODULE_HTTP_CLIENT_ERR_CODE_START           (0)

/** HTTP Client library error base. */
#define CY_RSLT_HTTP_CLIENT_ERR_BASE                        CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_HTTP_CLIENT, CY_RSLT_MODULE_HTTP_CLIENT_ERR_CODE_START)

/** Generic HTTP Client library error. */
#define CY_RSLT_HTTP_CLIENT_ERROR                           ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 1))
/** Invalid argument. */
#define CY_RSLT_HTTP_CLIENT_ERROR_BADARG                    ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 2))
/** Out of memory. */
#define CY_RSLT_HTTP_CLIENT_ERROR_NOMEM                     ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 3))
/** HTTP Client library Init failed. */
#define CY_RSLT_HTTP_CLIENT_ERROR_INIT_FAIL                 ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 4))
/** HTTP Client library Deinit failed. */
#define CY_RSLT_HTTP_CLIENT_ERROR_DEINIT_FAIL               ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 5))
/** HTTP Client object not initialized. */
#define CY_RSLT_HTTP_CLIENT_ERROR_OBJ_NOT_INITIALIZED       ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 6))
/** HTTP connect failed. */
#define CY_RSLT_HTTP_CLIENT_ERROR_CONNECT_FAIL              ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 7))
/** HTTP disconnect failed. */
#define CY_RSLT_HTTP_CLIENT_ERROR_DISCONNECT_FAIL           ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 8))
/** HTTP Client not connected. */
#define CY_RSLT_HTTP_CLIENT_ERROR_NOT_CONNECTED             ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 9))
/** HTTP Client already connected. */
#define CY_RSLT_HTTP_CLIENT_ERROR_ALREADY_CONNECTED         ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 10))
/** Invalid credentials. */
#define CY_RSLT_HTTP_CLIENT_ERROR_INVALID_CREDENTIALS       ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 11))
/** TLS handshake failed. */
#define CY_RSLT_HTTP_CLIENT_ERROR_HANDSHAKE_FAIL            ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 12))
/** HTTP response was partially received. */
#define CY_RSLT_HTTP_CLIENT_ERROR_PARTIAL_RESPONSE          ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 13))
/** No HTTP response was received. */
#define CY_RSLT_HTTP_CLIENT_ERROR_NO_RESPONSE               ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 14))
/** Server sent more headers than the configured number of headers. */
#define CY_RSLT_HTTP_CLIENT_ERROR_MAX_HEADER_SIZE_EXCEEDED  ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 15))
/** Server sent a chunk header containing one or more invalid characters. */
#define CY_RSLT_HTTP_CLIENT_ERROR_INVALID_CHUNK_HEADER      ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 16))
/** Response contains invalid characters in the Content-Length header. */
#define CY_RSLT_HTTP_CLIENT_ERROR_INVALID_CONTENT_LENGTH    ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 17))
/** Error occurred while parsing the response. */
#define CY_RSLT_HTTP_CLIENT_ERROR_PARSER                    ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 18))
/** The requested header field was not found in the response. */
#define CY_RSLT_HTTP_CLIENT_ERROR_NO_HEADER                 ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 19))
/** HTTP response is either corrupt or incomplete. */
#define CY_RSLT_HTTP_CLIENT_ERROR_INVALID_RESPONSE          ((cy_rslt_t)(CY_RSLT_HTTP_CLIENT_ERR_BASE + 20))

/**
* @}
*/


/**
 * @addtogroup http_client_struct
 *
 * HTTP Client library data structures and type definitions
 *
 * @{
 */
/******************************************************
 *                   Enumerations
 ******************************************************/
/**
 * HTTP Client supported methods
 */
typedef enum cy_http_client_method
{
    CY_HTTP_CLIENT_METHOD_GET,                       /**< HTTP GET Method     */
    CY_HTTP_CLIENT_METHOD_PUT,                       /**< HTTP PUT Method     */
    CY_HTTP_CLIENT_METHOD_POST,                      /**< HTTP POST Method    */
    CY_HTTP_CLIENT_METHOD_HEAD,                      /**< HTTP HEAD Method    */
    CY_HTTP_CLIENT_METHOD_DELETE,                    /**< HTTP DELETE Method  */
    CY_HTTP_CLIENT_METHOD_PATCH,                     /**< HTTP PATCH Method   */
    CY_HTTP_CLIENT_METHOD_CONNECT,                   /**< HTTP CONNECT Method */
    CY_HTTP_CLIENT_METHOD_OPTIONS,                   /**< HTTP OPTIONS Method */
    CY_HTTP_CLIENT_METHOD_TRACE                      /**< HTTP TRACE Method   */
} cy_http_client_method_t;

/**
 * HTTP Client disconnect type
 */
typedef enum cy_http_client_disconn_type
{
    CY_HTTP_CLIENT_DISCONN_TYPE_SERVER_INITIATED = 0, /**< Server initiated disconnect */
    CY_HTTP_CLIENT_DISCONN_TYPE_NETWORK_DOWN     = 1  /**< Network is disconnected */
} cy_http_client_disconn_type_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/
/**
 * HTTP Client handle
 */
typedef void* cy_http_client_t;

/**
 * Disconnect notification callback function which was registered while invoking /ref cy_http_client_create.
 * On disconnect event, the application needs to call cy_http_client_disconnect() to disconnect.
 *
 * @param handle [in]       : Handle for which disconnection has occurred.
 * @param type [in]         : Disconnect type.
 * @param user_data [in]    : User data provided by the caller while invoking /ref cy_http_client_create.
 *
 * @return                  : void
 */
typedef void (*cy_http_disconnect_callback_t)(cy_http_client_t handle, cy_http_client_disconn_type_t type, void *user_data);

/******************************************************
 *                    Structures
 ******************************************************/
/**
 * HTTP structure containing the HTTP header fields
 */
typedef struct
{
    char    *field;                                  /**< HTTP header field such as host, date, content_length, etc. */
    size_t   field_len;                              /**< HTTP field length. */
    char    *value;                                  /**< HTTP header value corresponding to the field. */
    size_t   value_len;                              /**< HTTP header value length. */
} cy_http_client_header_t;

/**
 * HTTP structure containing the fields required for the request header
 */
typedef struct
{
    cy_http_client_method_t   method;                /**< Method for which the HTTP Client request has to be sent. */
    const char               *resource_path;         /**< Path to which HTTP Client request has to be sent; NULL terminated. */
    uint8_t                  *buffer;                /**< Pointer to the buffer to store HTTP request and the HTTP response received from the server.
                                                          This buffer needs to be allocated by the caller and should not be freed before \ref cy_http_client_send returns. */
    size_t                    buffer_len;            /**< Length of the buffer in bytes. */
    size_t                    headers_len;           /**< Length of the request header updated in \ref cy_http_client_write_header, or
                                                          the user has to update this field if the header is generated by the application and passed to cy_http_client_send. */
    int32_t                   range_start;           /**< Indicates the Start Range from where the server should return. If the range header is not required, set this value to -1. */
    int32_t                   range_end;             /**< Indicates the End Range until where the data is expected.
                                                          Set this to -1 if requested range is all bytes from the starting range byte to the end of file or
                                                          the requested range is for the last N bytes of the file.*/
} cy_http_client_request_header_t;

/**
 * HTTP structure containing the fields required for response header and body
 */
typedef struct
{
    uint16_t         status_code;                    /**< Standard HTTP response status code. */
    uint8_t         *buffer;                         /**< Pointer to the buffer containing the HTTP response. This buffer is the same as the buffer provided in cy_http_client_request_header_t. */
    size_t           buffer_len;                     /**< Length of the buffer in bytes. */
    const uint8_t   *header;                         /**< The starting location of the response headers in the buffer. */
    size_t           headers_len;                    /**< Byte length of the response headers in the buffer. */
    size_t           header_count;                   /**< Count of the headers sent by the server. */
    const uint8_t   *body;                           /**< The starting location of the response body in the buffer. */
    size_t           body_len;                       /**< Byte length of the body in the buffer. */
    size_t           content_len;                    /**< The value in the "Content-Length" header is updated here. */
} cy_http_client_response_t;

/**
 * @}
 */

/******************************************************
 *               Function Declarations
 ******************************************************/
/*******************************************************
 *                   API State machine
 *                  ===================
 *******************************************************
 *
 *              HTTP Client send request
 *             ==========================
 *
 *              +-----------------------+
 *              | cy_http_client_init() |
 *              +-----------------------+
 *                         |
 *                         v
 *            +----------------------------+
 *            |    cy_http_client_create() |
 *            +----------------------------+
 *                         |
 *                         v
 *             +--------------------------+
 *             | cy_http_client_connect() |
 *             +--------------------------+
 *                         |
 *                         v
 *           +------------------------------+
 *           | cy_http_client_write_header()|
 *           +------------------------------+
 *                         |
 *                         v
 *              +-----------------------+
 *              | cy_http_client_send() |
 *              +-----------------------+
 *                         |
 *                         v
 *            +-----------------------------+
 *            | cy_http_client_disconnect() |
 *            +-----------------------------+
 *                         |
 *                         v
 *           +--------------------------------+
 *           | cy_http_client_delete_delete() |
 *           +---------------- ---------------+
 *                         |
 *                         v
 *             +-------------------------+
 *             | cy_http_client_deinit() |
 *             +-------------------------+
 *
 * **************************************************************************************************/
 /**
 *
 * @addtogroup group_c_api_functions
 *
 * C APIs provided by the HTTP Client library.
 *
 * @{
 */

/**
 * Initializes the Http Client library and its components.
 * This function must be called before using any other HTTP Client library functions.
 *
 * @note \ref cy_http_client_init and \ref cy_http_client_deinit API functions are not thread-safe. The caller
 *       must ensure that these two API functions are not invoked simultaneously from different threads.
 *
 * @return cy_rslt_t        : CY_RSLT_SUCCESS on success; error codes in @ref http_client_defines otherwise.
 */
cy_rslt_t cy_http_client_init(void);

/**
 * Creates a HTTP Client instance and initializes its members based on the input arguments.
 * The handle to the HTTP Client instance is returned via the handle pointer supplied by the user on success.
 * This handle is used for connect, disconnect, and sending HTTP Client requests.
 * This function must be called after calling \ref cy_http_client_init.
 *
 * @param security [in]              : Credentials for TLS secure connection. For non-secure connection, set it to NULL.
 *                                     The application must allocate memory for keys and should not be freed until the HTTP Client object is deleted.
 * @param server_info [in]           : Pointer for the HTTP Client Server information required during connect and send.
 * @param disconn_cb [in]            : Pointer to the callback function to be invoked on disconnect.
 * @param user_data [in]             : User data to be sent while invoking the disconnect callback.
 * @param handle [out]               : Pointer to store the HTTP Client handle allocated by this function on a successful return. 
 *                                     Caller should not free the handle directly. User needs to invoke \ref cy_http_client_delete to free the handle.
 *
 * @return cy_rslt_t                 : CY_RSLT_SUCCESS on success; error codes in @ref http_client_defines otherwise.
 */
cy_rslt_t cy_http_client_create(cy_awsport_ssl_credentials_t *security, cy_awsport_server_info_t *server_info, cy_http_disconnect_callback_t disconn_cb, void *user_data, cy_http_client_t *handle);

/**
 * Connects to the given HTTP server and establishes a connection.
 * This function must be called after calling \ref cy_http_client_create.
 *
 * @param handle [in]                : HTTP Client handle created using \ref cy_http_client_create.
 * @param send_timeout_ms [in]       : Socket send timeout in milliseconds.
 * @param receive_timeout_ms [in]    : Socket receive timeout in milliseconds.
 * @return cy_rslt_t                 : CY_RSLT_SUCCESS on success; error codes in @ref http_client_defines otherwise.
 */
cy_rslt_t cy_http_client_connect(cy_http_client_t handle, uint32_t send_timeout_ms, uint32_t receive_timeout_ms);

/**
 * Generates the request Header used as HTTP Client request header during \ref cy_http_client_send.
 * This function must be called after calling \ref cy_http_client_create.
 *
 * @param handle [in]                : HTTP Client handle created using \ref cy_http_client_create.
 * @param request [in/out]           : Pointer to the HTTP request structure. The list of HTTP request headers are stored in the HTTP protocol header format.
 * @param header [in]                : Pointer to the list of headers to be updated in the request buffer.
 * @param num_header [in]            : Indicates the number of headers in the header list.
 *
 * @return cy_rslt_t                 : CY_RSLT_SUCCESS on success; error codes in @ref http_client_defines otherwise.
 */
cy_rslt_t cy_http_client_write_header(cy_http_client_t handle, cy_http_client_request_header_t *request, cy_http_client_header_t *header, uint32_t num_header);

/**
 * Sends the HTTP request to the server and returns the received HTTP response from the server.
 * This function must be called after calling \ref cy_http_client_connect.
 * This API will return if the data is not sent or the response is not received within the timeout configured in \ref cy_http_client_connect.
 * This is a synchronous API. For a given HTTP Client instance, the caller has to wait till this API returns to initiate a new \ref cy_http_client_send.
 *
 * @param handle [in]                : HTTP Client handle created using \ref cy_http_client_create.
 * @param request [in]               : Pointer containing the HTTP request header updated at \ref cy_http_client_write_header.
 * @param payload [in]               : Pointer to the payload which must be sent with the HTTP request.
 * @param payload_len [in]           : Length of the payload.
 * @param response [out]             : Pointer updated with the response of the request with the header and body on success.
 *
 * @return cy_rslt_t                 : CY_RSLT_SUCCESS on success; error codes in @ref http_client_defines otherwise.
 */
cy_rslt_t cy_http_client_send(cy_http_client_t handle, cy_http_client_request_header_t *request, uint8_t *payload, uint32_t payload_len, cy_http_client_response_t *response);

/**
 * Parses the headers received in the HTTP response.
 * This function must be called after calling \ref cy_http_client_send.
 * While parsing the headers from the response, if any error occurs, the particular
 * header/value entries in the output array will have the value and length fields set to NULL and 0 respectively.
 *
 * @param handle [in]                : HTTP Client handle created using \ref cy_http_client_create.
 * @param response [in]              : Pointer to the HTTP response updated during \ref cy_http_client_send.
 * @param header [out]               : Pointer to the header list to store the header fields parsed from the response.
 * @param num_header [in]            : Indicates the number of headers to be parsed.
 *
 * @return cy_rslt_t                 : CY_RSLT_SUCCESS on success; error codes in @ref http_client_defines otherwise.
 */
cy_rslt_t cy_http_client_read_header(cy_http_client_t handle, cy_http_client_response_t *response, cy_http_client_header_t *header, uint32_t num_header);

/** Disconnects the HTTP Client network connection.
 * This function must be called after calling \ref cy_http_client_connect.
 *
 * @param handle [in]                : HTTP Client handle created using \ref cy_http_client_create.
 *
 * @return cy_rslt_t                 : CY_RSLT_SUCCESS on success; error codes in @ref http_client_defines otherwise.
 */
cy_rslt_t cy_http_client_disconnect(cy_http_client_t handle);

/** Deletes the HTTP Client library Object.
 *  Frees the resources assigned during object creation.
 * This function must be called after calling \ref cy_http_client_create.
 *
 * @param handle [in]                : HTTP Client handle created using \ref cy_http_client_create.
 *
 * @return cy_rslt_t                 : CY_RSLT_SUCCESS on success; error codes in @ref http_client_defines otherwise.
 */
cy_rslt_t cy_http_client_delete(cy_http_client_t handle);

/** De-initializes the global resources used by the HTTP Client library.
 *  Removes the resources assigned for the library during initialization.
 *
 * @note \ref cy_http_client_init and \ref cy_http_client_deinit API functions are not thread-safe. The caller
 *       must ensure that these two API functions are not invoked simultaneously from different threads.
 *
 * @return cy_rslt_t        : CY_RSLT_SUCCESS on success; error codes in @ref http_client_defines otherwise.
 */
cy_rslt_t cy_http_client_deinit(void);

/**
* @}
*/

#endif // CY_HTTP_CLIENT_API_H_
