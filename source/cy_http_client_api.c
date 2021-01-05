/*
 * Copyright 2020, Cypress Semiconductor Corporation or a subsidiary of
 * Cypress Semiconductor Corporation. All Rights Reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software"), is owned by Cypress Semiconductor Corporation
 * or one of its subsidiaries ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products. Any reproduction, modification, translation,
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

/** @file
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cyabs_rtos.h"
#include "cy_result_mw.h"
#include "cy_log.h"
#include "core_http_client.h"
#include "cy_http_client_api.h"

/******************************************************
 *                      Macros
 ******************************************************/
#ifdef ENABLE_HTTP_CLIENT_LOGS
#define cy_hc_log_msg cy_log_msg
#else
#define cy_hc_log_msg(a,b,c,...)
#endif
/******************************************************
 *                    Constants
 ******************************************************/
#define TMP_BUFFER_SIZE                                                 ( 256 )     /* Temporary buffer size used to read any pending data in the socket after the client send API returns */
#define RECV_TIMEOUT_UNCONSUMED_DATA                                    ( 100 )     /* Timeout configure in milliseconds to read the un-consumed data by http_client_send() */

#define CY_HTTP_CLIENT_DISCONNECT_EVENT_QUEUE_SIZE                      ( 8 )       /* Maximum number of simultaneous disconnect events that can be posted to the queue */

#ifdef ENABLE_HTTP_CLIENT_LOGS
    #define CY_HTTP_CLIENT_DISCONNECT_EVENT_THREAD_STACK_SIZE           ( (1024 * 1) + (1024 * 3) ) /* Additional 3kb of stack is added for enabling the prints */
#else
    #define CY_HTTP_CLIENT_DISCONNECT_EVENT_THREAD_STACK_SIZE           ( 1024 * 1 )
#endif

#define CY_HTTP_CLIENT_DISCONNECT_EVENT_THREAD_PRIORITY                 ( CY_RTOS_PRIORITY_NORMAL )
#define CY_HTTP_CLIENT_DISCONNECT_EVENT_QUEUE_TIMEOUT_IN_MSEC           ( 500 )
/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/
/*
 * http client handle
 */
typedef struct http_client_object
{
    NetworkContext_t              *network_context;          /* Network context for the transport layer interface.    */
    cy_awsport_ssl_credentials_t  *security;                 /* Security credentials.                                 */
    cy_awsport_server_info_t       server_info;              /* Connection server details.                            */
    cy_mutex_t                     obj_mutex;                /* Mutex to serialize object access in multi-threading.  */
    cy_http_disconnect_callback_t  disconn_cbf;              /* Callback for disconnect notification.                 */
    void                          *user_data;                /* User data to be given during a callback notification. */
    bool                           isobjinitialized;         /* Indicates that the client object is initialized.      */
    bool                           server_disconnect;        /* Indicates that the HTTP client is connected.          */
    bool                           user_disconnect;          /* Indicates that the user has initiated a disconnect.   */
} cy_http_client_object_t ;

/******************************************************
 *               Static Function Declarations
 ******************************************************/
static cy_rslt_t http_client_error( HTTPStatus_t status );

/******************************************************
 *                 Static Variables
 ******************************************************/
static uint8_t           http_client_instance_count = 0;
static cy_mutex_t        http_mutex;
static cy_thread_t       http_client_disconnect_event_thread = NULL;
static cy_queue_t        http_client_disconnect_event_queue = NULL;

/******************************************************
 *               Function Definitions
 ******************************************************/
static void cy_http_disconnect_callback( void *user_data )
{
    cy_rslt_t                     result;

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if (user_data == NULL)
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Discarding the disconnect event, since the HTTP client handle is NULL\n");
        return;
    }

    result = cy_rtos_put_queue( &http_client_disconnect_event_queue, (void *)&user_data, CY_HTTP_CLIENT_DISCONNECT_EVENT_QUEUE_TIMEOUT_IN_MSEC, false );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Pushing to disconnect event queue failed with Error : [0x%X]\n", (unsigned int)result );
    }

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );
    return;
}

static void http_client_disconn_event_thread( cy_thread_arg_t arg )
{
    cy_rslt_t                      result = CY_RSLT_SUCCESS;
    cy_http_client_object_t       *http_obj;
    cy_http_client_disconn_type_t  type;
    (void)arg;

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    while( true )
    {
        result = cy_rtos_get_queue( &http_client_disconnect_event_queue, (void *)&http_obj, CY_RTOS_NEVER_TIMEOUT, false );
        if( CY_RSLT_SUCCESS != result )
        {
            continue;
        }

        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Trying to Acquire lock\n" );
        result = cy_rtos_get_mutex( &http_obj->obj_mutex, CY_RTOS_NEVER_TIMEOUT );
        if( result != CY_RSLT_SUCCESS )
        {
            cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
            continue;
        }
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Lock Acquired\n" );

        if ( http_obj->server_disconnect == false && http_obj->user_disconnect == true )
        {
            /* Indicates that the user has initiated a disconnect. Therefore, sending the notification is not required. Clear the user disconnect flag. */
            http_obj->user_disconnect = false;
        }
        else
        {
            if( http_obj->server_disconnect == true )
            {
                type = CY_HTTP_CLIENT_DISCONN_TYPE_SERVER_INITIATED;
            }
            else
            {
                type = CY_HTTP_CLIENT_DISCONN_TYPE_NETWORK_DOWN;
            }

            if( http_obj->disconn_cbf )
            {
                http_obj->disconn_cbf( http_obj, type, http_obj->user_data );
            }

            http_obj->server_disconnect = false;
        }

        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Trying to Release lock\n" );
        result = cy_rtos_set_mutex( &http_obj->obj_mutex );
        if( result != CY_RSLT_SUCCESS )
        {
            cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release lock failed with result = 0x%X\n", (unsigned long)result );
        }
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Lock Released\n" );
    }
}

static cy_rslt_t http_client_error( HTTPStatus_t status )
{
    switch( status )
    {
        case HTTPSuccess:
            return CY_RSLT_SUCCESS;
        case HTTPInvalidParameter:
            return CY_RSLT_HTTP_CLIENT_ERROR_BADARG;
        case HTTPNetworkError:
            return CY_RSLT_HTTP_CLIENT_ERROR;
        case HTTPPartialResponse:
            return CY_RSLT_HTTP_CLIENT_ERROR_PARTIAL_RESPONSE;
        case HTTPNoResponse:
            return CY_RSLT_HTTP_CLIENT_ERROR_NO_RESPONSE;
        case HTTPInsufficientMemory:
            return CY_RSLT_HTTP_CLIENT_ERROR_NOMEM;
        case HTTPSecurityAlertInvalidChunkHeader:
            return CY_RSLT_HTTP_CLIENT_ERROR_INVALID_CHUNK_HEADER;
        case HTTPSecurityAlertInvalidContentLength:
            return CY_RSLT_HTTP_CLIENT_ERROR_INVALID_CONTENT_LENGTH;
        case HTTPParserInternalError:
            return CY_RSLT_HTTP_CLIENT_ERROR_PARSER;
        case HTTPHeaderNotFound:
            return CY_RSLT_HTTP_CLIENT_ERROR_NO_HEADER;
        case HTTPInvalidResponse:
            return CY_RSLT_HTTP_CLIENT_ERROR_INVALID_RESPONSE;
        default:
            return CY_RSLT_HTTP_CLIENT_ERROR;
    }
}

static const char * convert_httpclient_method_to_string( cy_http_client_method_t method )
{
    const char *str = NULL;

    switch( method )
    {
        case CY_HTTP_CLIENT_METHOD_GET:
            str = HTTP_METHOD_GET;
            break;

        case CY_HTTP_CLIENT_METHOD_PUT:
            str = HTTP_METHOD_PUT;
            break;

        case CY_HTTP_CLIENT_METHOD_POST:
            str = HTTP_METHOD_POST;
            break;

        case CY_HTTP_CLIENT_METHOD_HEAD:
            str = HTTP_METHOD_HEAD;
            break;

        default:
            cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "\nInvalid method passed : [%d] \n",method );
            break;
    }

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\nRequested client method:%.*s\n",strlen(str), str );
    return str;
}

cy_rslt_t cy_http_client_init( void )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( http_client_instance_count )
    {
        http_client_instance_count++;
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\nHttp library Library is already initialized. Number of http client instance : [%d] \n",http_client_instance_count );
        return result;
    }

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Initialize http client library\n" );
    result = cy_awsport_network_init();
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Initialize http client library failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_HTTP_CLIENT_ERROR_INIT_FAIL;
    }

    result = cy_rtos_init_mutex2( &http_mutex, false );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Creating new mutex failed with result = 0x%X\n", (unsigned long)result );
        (void)cy_awsport_network_deinit();
        return CY_RSLT_HTTP_CLIENT_ERROR_INIT_FAIL;
    }
    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Global Mutex created : %p..!\n", http_mutex );

    /* Create the queue for disconnect events */
    result = cy_rtos_init_queue( &http_client_disconnect_event_queue, CY_HTTP_CLIENT_DISCONNECT_EVENT_QUEUE_SIZE, sizeof(cy_http_client_object_t *) );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\ncy_rtos_init_queue failed with Error : [0x%X] ", (unsigned int)result );
        (void)cy_awsport_network_deinit();
        (void)cy_rtos_deinit_mutex( &http_mutex );
        return result;
    }

    /* Create the thread to handle disconnect events */
    result = cy_rtos_create_thread( &http_client_disconnect_event_thread, http_client_disconn_event_thread, "HTTPCLIENTdisconnectEventThread", NULL,
                                    CY_HTTP_CLIENT_DISCONNECT_EVENT_THREAD_STACK_SIZE, CY_HTTP_CLIENT_DISCONNECT_EVENT_THREAD_PRIORITY, NULL );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\ncy_rtos_create_thread failed with Error : [0x%X] ", (unsigned int)result );
        (void)cy_awsport_network_deinit();
        (void)cy_rtos_deinit_mutex( &http_mutex );
        (void)cy_rtos_deinit_queue( &http_client_disconnect_event_queue );
        return result;
    }

    http_client_instance_count++;

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_http_client_create( cy_awsport_ssl_credentials_t *security,
                                 cy_awsport_server_info_t *server_info,
                                 cy_http_disconnect_callback_t disconn_cb,
                                 void *user_data,
                                 cy_http_client_t *handle )
{
    cy_rslt_t                 result          = CY_RSLT_SUCCESS;
    cy_http_client_object_t  *http_obj        = NULL;
    NetworkContext_t         *network_context = NULL;
    char                     *server_name     = NULL;
    bool                      obj_mutex_init_status = false;

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( server_info == NULL || server_info->host_name == NULL || handle == NULL )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid arguments passed \n" );
        return CY_RSLT_HTTP_CLIENT_ERROR_BADARG;
    }

    if( http_client_instance_count == 0 )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n HTTP client library not initialized \n" );
        return CY_RSLT_HTTP_CLIENT_ERROR_OBJ_NOT_INITIALIZED;
    }

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", http_mutex );
    result = cy_rtos_get_mutex( &http_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_HTTP_CLIENT_ERROR;
    }

    http_obj = ( cy_http_client_object_t * )malloc( sizeof( cy_http_client_object_t ) );
    if( http_obj == NULL )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\nMalloc for HTTP client handle failed..!\n" );
        result = CY_RSLT_HTTP_CLIENT_ERROR_NOMEM;
        goto exit;
    }
    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\nhttp_obj : %p..!\n", http_obj );
    memset( http_obj, 0x00, sizeof( cy_http_client_object_t ) );

    network_context = ( NetworkContext_t * )malloc( sizeof( NetworkContext_t ) );
    if( network_context == NULL )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\nMemory not available to create Network context..!\n" );
        result = CY_RSLT_HTTP_CLIENT_ERROR_NOMEM;
        goto exit;
    }
    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\network_context : %p..!\n", network_context );
    memset( network_context, 0x00, sizeof( NetworkContext_t ) );

    server_name = ( char * )malloc( strlen( server_info->host_name ) + 1 );
    if( server_name == NULL )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\nMemory not available to store server name..!\n" );
        result = CY_RSLT_HTTP_CLIENT_ERROR_NOMEM;
        goto exit;
    }
    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "server_name : %p..!\n", server_name );
    memset( server_name, 0x00, strlen( server_info->host_name ) + 1 );

    result = cy_rtos_init_mutex2( &(http_obj->obj_mutex), true );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Creating new mutex failed with result = 0x%X\n", (unsigned long)result );
        goto exit;
    }
    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Mutex created : %p..!\n", http_obj->obj_mutex );

    obj_mutex_init_status = true;

    memcpy( server_name, server_info->host_name, strlen( server_info->host_name ) + 1 );

    http_obj->security = security;
    http_obj->server_info.host_name = server_name;
    http_obj->server_info.port = server_info->port;
    http_obj->network_context = network_context;

    http_obj->disconn_cbf = disconn_cb;
    http_obj->user_data = user_data;

    *handle = (cy_http_client_t *)http_obj;

    http_obj->server_disconnect = false;
    http_obj->user_disconnect = false;
    http_obj->isobjinitialized = true;

    result = cy_rtos_set_mutex( &http_mutex );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release global lock failed with result = 0x%X\n", (unsigned long)result );
        goto exit;
    }
    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", http_mutex );

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;

exit :
    if( http_obj != NULL )
    {
        if( obj_mutex_init_status == true )
        {
            cy_rtos_deinit_mutex( &(http_obj->obj_mutex) );
        }

        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\n Free http_obj : %p..!\n", http_obj );
        free( http_obj );
    }

    if( server_name != NULL )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\n Free server_name : %p..!\n", server_name );
        free( server_name );
    }

    if( network_context != NULL )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\n Free network_context : %p..!\n", network_context );
        free( network_context );
    }

    cy_rtos_set_mutex( &http_mutex );
    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release global mutex : %p..!\n", http_mutex );

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_http_client_connect( cy_http_client_t handle,
                                  uint32_t send_timeout_ms,
                                  uint32_t receive_timeout_ms )
{
    cy_rslt_t                result = CY_RSLT_SUCCESS;
    cy_http_client_object_t *http_obj;
    cy_awsport_callback_t    nw_disconn_cb;

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( handle == NULL )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_HTTP_CLIENT_ERROR_OBJ_NOT_INITIALIZED;
    }

    http_obj = (cy_http_client_object_t *)handle;

    if( http_obj->isobjinitialized != true )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n HTTP client handle not initialized \n" );
        return CY_RSLT_HTTP_CLIENT_ERROR_OBJ_NOT_INITIALIZED;
    }

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire object mutex : %p..!\n", http_obj->obj_mutex );
    result = cy_rtos_get_mutex( &(http_obj->obj_mutex), CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_HTTP_CLIENT_ERROR;
    }

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Socket creation with %.*s:%d\n",
                                                 strlen(http_obj->server_info.host_name), http_obj->server_info.host_name,
                                                 http_obj->server_info.port );

    nw_disconn_cb.cbf = cy_http_disconnect_callback;
    nw_disconn_cb.user_data = ( void * )http_obj;
    http_obj->user_disconnect = false;

    result = cy_awsport_network_create( http_obj->network_context, &(http_obj->server_info), http_obj->security, &nw_disconn_cb );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "http client library network create failed with result = 0x%X\n", (unsigned long)result );
        result = CY_RSLT_HTTP_CLIENT_ERROR;
        goto exit;
    }

    result = cy_awsport_network_connect( http_obj->network_context, send_timeout_ms, receive_timeout_ms );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "http client library network connect failed with result = 0x%X\n", (unsigned long)result );
        result = CY_RSLT_HTTP_CLIENT_ERROR_CONNECT_FAIL;
        goto exit;
    }

exit :
    if( cy_rtos_set_mutex( &http_obj->obj_mutex ) != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release lock failed with result = 0x%X\n", (unsigned long)result );
        result = CY_RSLT_HTTP_CLIENT_ERROR;
    }
    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release object mutex : %p..!\n", http_obj->obj_mutex );

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_http_client_write_header( cy_http_client_t handle,
                                       cy_http_client_request_header_t *request,
                                       cy_http_client_header_t *header,
                                       uint32_t num_header )
{
    cy_rslt_t                result = CY_RSLT_SUCCESS;
    HTTPStatus_t             httpstatus;
    cy_http_client_object_t *http_obj;
    HTTPRequestInfo_t        request_info;
    HTTPRequestHeaders_t     request_headers;
    int                      i;

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( handle == NULL )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Object not initialized\n" );
        return CY_RSLT_HTTP_CLIENT_ERROR_OBJ_NOT_INITIALIZED;
    }

    if( request == NULL || header == NULL || num_header == 0 )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Invalid Arguments \n" );
        return CY_RSLT_HTTP_CLIENT_ERROR_BADARG;
    }

    http_obj = ( cy_http_client_object_t * )handle;

    if( http_obj->isobjinitialized != true )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "HTTP client library not initialized \n" );
        return CY_RSLT_HTTP_CLIENT_ERROR;
    }

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire object mutex : %p..!\n", http_obj->obj_mutex );
    result = cy_rtos_get_mutex( &(http_obj->obj_mutex), CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_HTTP_CLIENT_ERROR;
    }

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "HTTP client header preparation \n" );

    ( void ) memset( &request_info, 0, sizeof( HTTPRequestInfo_t ) );
    ( void ) memset( &request_headers, 0, sizeof( HTTPRequestHeaders_t ) );

    /* Initialize the request object. */
    request_info.pHost = http_obj->server_info.host_name;
    request_info.hostLen = strlen( http_obj->server_info.host_name );
    request_info.pMethod = convert_httpclient_method_to_string( request->method );
    request_info.methodLen = strlen( request_info.pMethod );
    request_info.pPath = request->resource_path;
    request_info.pathLen = strlen( request->resource_path );

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "\nrequestInfo.pHost= %.*s\n"
                                                "requestInfo.method= %.*s\n"
                                                "requestInfo.pPath= %.*s\n",
                                                strlen(request_info.pHost), request_info.pHost,
                                                strlen(request_info.pMethod), request_info.pMethod,
                                                strlen(request_info.pPath), request_info.pPath );

    /* Application will send the connection header */
    request_info.reqFlags = 0;

    /* Set the buffer used for storing request headers. */
    request_headers.pBuffer = request->buffer;
    request_headers.bufferLen = request->buffer_len;

    httpstatus = HTTPClient_InitializeRequestHeaders( &request_headers, &request_info );
    if( httpstatus != HTTPSuccess )
    {
        result = http_client_error( httpstatus );
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "http client request header initialization failed with result = %lu\n", (unsigned long)result );
        goto exit;
    }

    /* Add all user given headers into the header request. */
    for ( i= 0; i<num_header; i++ )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "\nheader.field= %.*s\n"
                                                    "header.value= %.*s\n",
                                                    header[i].field_len, header[i].field ,
                                                    header[i].value_len, header[i].value );

        httpstatus = HTTPClient_AddHeader( &request_headers, header[i].field , header[i].field_len, header[i].value, header[i].value_len );
        if( httpstatus != HTTPSuccess )
        {
            result = http_client_error( httpstatus );
            cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "http client header addition failed with result = %lu\n", (unsigned long)result );
            goto exit;
        }
    }

    if( i != num_header )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Adding all the user given headers failed\n" );
        result = CY_RSLT_HTTP_CLIENT_ERROR_NOMEM;
        goto exit;
    }

    /* Add the range header into the header request. */
    if( request->range_start != -1 )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Adding header range \n" );
        httpstatus = HTTPClient_AddRangeHeader( &request_headers, request->range_start, request->range_end );
        if( httpstatus != HTTPSuccess )
        {
            result = http_client_error( httpstatus );
            cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "http client range header failed with result = %lu\n", (unsigned long)result );
            goto exit;
        }
    }

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\nRequest Headers:\n%.*s\n", ( int ) request_headers.headersLen, ( char * ) request_headers.pBuffer );

    request->headers_len = request_headers.headersLen;

exit :
    if( cy_rtos_set_mutex( &http_obj->obj_mutex ) )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release lock failed with result = 0x%X\n", (unsigned long)result );
        result = CY_RSLT_HTTP_CLIENT_ERROR;
    }
    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release object mutex : %p..!\n", http_obj->obj_mutex );

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_http_client_send( cy_http_client_t handle,
                               cy_http_client_request_header_t *request,
                               uint8_t *payload,
                               uint32_t payload_len,
                               cy_http_client_response_t *response )
{
    cy_rslt_t                  result = CY_RSLT_SUCCESS;
    HTTPStatus_t               httpstatus;
    cy_http_client_object_t   *http_obj;
    TransportInterface_t       transport_interface;
    HTTPRequestHeaders_t       request_headers;
    HTTPResponse_t             httpresponse;
    int32_t                    byte_received = 0;
    uint8_t                    is_content;
    uint8_t                    temp_buffer[TMP_BUFFER_SIZE];

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( handle == NULL )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_HTTP_CLIENT_ERROR_OBJ_NOT_INITIALIZED;
    }

    if( ( payload == NULL && (payload_len > 0) ) || ( payload != NULL && (payload_len == 0) ) )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_HTTP_CLIENT_ERROR_BADARG;
    }

    http_obj = (cy_http_client_object_t *)handle;

    if( http_obj->isobjinitialized != true )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n HTTP client library not initialized \n" );
        return CY_RSLT_HTTP_CLIENT_ERROR;
    }

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire object mutex : %p..!\n", http_obj->obj_mutex );
    result = cy_rtos_get_mutex( &(http_obj->obj_mutex), CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_HTTP_CLIENT_ERROR;
    }

    ( void ) memset( &httpresponse, 0, sizeof( httpresponse ) );

    ( void ) memset( &transport_interface, 0, sizeof( transport_interface ) );
    transport_interface.recv = ( TransportRecv_t )cy_awsport_network_receive;
    transport_interface.send = ( TransportSend_t )cy_awsport_network_send;
    transport_interface.pNetworkContext = http_obj->network_context;

    request_headers.pBuffer = request->buffer;
    request_headers.bufferLen = request->buffer_len;
    request_headers.headersLen = request->headers_len;

    /* Initialize the response object. The same buffer used for storing
     * request headers is reused here. */
    httpresponse.pBuffer = request->buffer;
    httpresponse.bufferLen = request->buffer_len;

    is_content = ( payload_len > 0 ) ? !HTTP_SEND_DISABLE_CONTENT_LENGTH_FLAG : HTTP_SEND_DISABLE_CONTENT_LENGTH_FLAG;

    httpstatus = HTTPClient_Send( &transport_interface, &request_headers, payload, payload_len, &httpresponse, is_content );
    if( httpstatus != HTTPSuccess )
    {
        result = http_client_error( httpstatus );
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "HTTPClient_Send() failed with result = %lu httpstatus = %lu\n", (unsigned long)result, httpstatus );
    }

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "HTTPClient_Send() API returned\n" );

    response->status_code = httpresponse.statusCode;
    response->buffer = httpresponse.pBuffer;
    response->header = httpresponse.pHeaders;
    response->headers_len = httpresponse.headersLen;
    response->header_count = httpresponse.headerCount;
    response->body = httpresponse.pBody;
    response->body_len = httpresponse.bodyLen;
    response->content_len = httpresponse.contentLength;
    response->buffer_len = httpresponse.bufferLen;
    if( httpresponse.respFlags & HTTP_RESPONSE_CONNECTION_CLOSE_FLAG )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Connection Closed found in response header. httpresponse.respFlags:[%d]\n", httpresponse.respFlags );
        http_obj->server_disconnect = true;
        cy_http_client_disconnect( handle );
    }

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "http client library : Received HTTP response from %.*s%.*s...\n"
               "Response Headers:\n%.*s\n"
               "Response Status:\n%u\n",
               ( int ) strlen(http_obj->server_info.host_name), http_obj->server_info.host_name,
               ( int ) strlen(request->resource_path), request->resource_path,
               ( int ) response->headers_len, response->header,
               response->status_code );
    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "buffer_len:[%d] headers_len:[%d] header_count:[%d] body_len:[%d] content_len:[%d]\n",
             response->buffer_len, response->headers_len, response->header_count, response->body_len, response->content_len );
    /* Read and ignore the data in the socket that is not consumed by HTTPClient_Send. */
    if( httpstatus != HTTPSuccess && httpstatus != HTTPInvalidParameter &&
        httpstatus != HTTPNetworkError && httpstatus != HTTPNoResponse )
    {
        uint32_t read_recv_timeout;
        uint32_t revcv_timeout_new = RECV_TIMEOUT_UNCONSUMED_DATA;
        uint32_t timeout_length = sizeof(uint32_t);

        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Read and ignore the data in the socket that is not consumed by HTTPClient_Send\n" );

        if( cy_socket_getsockopt(http_obj->network_context->handle, CY_SOCKET_SOL_SOCKET, CY_SOCKET_SO_SNDTIMEO, (void*)(&read_recv_timeout), &timeout_length ) != CY_RSLT_SUCCESS )
        {
            cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "read receive timeout using cy_socket_getsockopt failed...\n" );
        }
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "read receive timeout value : [%d]\n", read_recv_timeout );

        if( cy_socket_setsockopt( http_obj->network_context->handle, CY_SOCKET_SOL_SOCKET, CY_SOCKET_SO_SNDTIMEO, (void*)(&revcv_timeout_new), timeout_length ) != CY_RSLT_SUCCESS )
        {
            cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "write new receive timeout using cy_socket_setsockopt failed...\n" );
        }

        do
        {
            byte_received = transport_interface.recv( http_obj->network_context , temp_buffer, TMP_BUFFER_SIZE );
            cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "byte_received:[%d]...\n", byte_received );
        } while ( byte_received > 0 && byte_received == TMP_BUFFER_SIZE );

        if( cy_socket_setsockopt( http_obj->network_context->handle, CY_SOCKET_SOL_SOCKET, CY_SOCKET_SO_SNDTIMEO, (void*)(&read_recv_timeout), timeout_length ) != CY_RSLT_SUCCESS )
        {
            cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "write back original receive timeout using cy_socket_setsockopt failed...\n" );
        }
    }

    if( cy_rtos_set_mutex( &http_obj->obj_mutex ) != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release lock failed with result = 0x%X\n", (unsigned long)result );
        result = CY_RSLT_HTTP_CLIENT_ERROR;
    }
    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release object mutex : %p..!\n", http_obj->obj_mutex );

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_http_client_read_header( cy_http_client_t handle,
                                      cy_http_client_response_t *response,
                                      cy_http_client_header_t *header,
                                      uint32_t num_header )
{
    cy_rslt_t                result = CY_RSLT_SUCCESS;
    HTTPStatus_t             httpstatus;
    HTTPResponse_t           httpresponse;
    cy_http_client_object_t *http_obj;
    uint32_t                 read_header_count;

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( handle == NULL || response == NULL || header == NULL )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_HTTP_CLIENT_ERROR_OBJ_NOT_INITIALIZED;
    }
    http_obj = (cy_http_client_object_t *)handle;

    if( http_obj->isobjinitialized != true )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n HTTP client library not initialized \n" );
        return CY_RSLT_HTTP_CLIENT_ERROR;
    }

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire object mutex : %p..!\n", http_obj->obj_mutex );
    result = cy_rtos_get_mutex( &(http_obj->obj_mutex), CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_HTTP_CLIENT_ERROR;
    }

    read_header_count = ( num_header > response->header_count ) ? response->header_count : num_header;
    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Number of headers to read : [%d] \n", read_header_count );

    httpresponse.pBuffer = response->buffer;
    httpresponse.bufferLen = response->buffer_len;

    for ( int i = 0; i < read_header_count; i++ )
    {
        httpstatus = HTTPClient_ReadHeader( &httpresponse, header[i].field, header[i].field_len, (const char **)&(header[i].value), &(header[i].value_len) );
        if( httpstatus != HTTPSuccess )
        {
            cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, " http client read header failed header index : [%d]\n", i+1 );
            header[i].field     = NULL;
            header[i].field_len = 0;
            header[i].value     = NULL;
            header[i].value_len = 0;
        }
    }

    result = cy_rtos_set_mutex( &http_obj->obj_mutex );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release lock failed with result = 0x%X\n", (unsigned long)result );
        result = CY_RSLT_HTTP_CLIENT_ERROR;
    }
    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release object mutex : %p..!\n", http_obj->obj_mutex );

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_http_client_disconnect( cy_http_client_t handle )
{
    cy_rslt_t                result;
    cy_http_client_object_t *http_obj;

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( handle == NULL )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return CY_RSLT_HTTP_CLIENT_ERROR_OBJ_NOT_INITIALIZED;
    }

    http_obj = (cy_http_client_object_t *)handle;

    if( http_obj->isobjinitialized != true )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n HTTP client library not initialized \n" );
        return CY_RSLT_HTTP_CLIENT_ERROR;
    }

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire object mutex : %p..!\n", http_obj->obj_mutex );
    result = cy_rtos_get_mutex( &(http_obj->obj_mutex), CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_HTTP_CLIENT_ERROR;
    }

    if( http_obj->network_context->handle != NULL)
    {
        http_obj->user_disconnect = true;

        result = cy_awsport_network_disconnect( http_obj->network_context );
        if( result != CY_RSLT_SUCCESS )
        {
            cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "De-initialize http client library failed with result = %lu\n", (unsigned long)result );
            /* Fall-through. It's intentional. */
        }

        result = cy_awsport_network_delete( http_obj->network_context );
        if( result != CY_RSLT_SUCCESS )
        {
            cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "De-initialize http client library failed with result = %lu\n", (unsigned long)result );
            /* Fall-through. It's intentional. */
        }
    }

    http_obj->network_context->handle = NULL;

    result = cy_rtos_set_mutex( &http_obj->obj_mutex );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Release lock failed with result = 0x%X\n", (unsigned long)result );
        /* Fall-through. It's intentional. */
    }
    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release object mutex : %p..!\n", http_obj->obj_mutex );

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return CY_RSLT_SUCCESS;
}

cy_rslt_t cy_http_client_delete( cy_http_client_t handle )
{
    cy_rslt_t                result = CY_RSLT_SUCCESS;
    cy_http_client_object_t *http_obj;

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( http_client_instance_count == 0 )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n HTTP client library not initialized \n" );
        return CY_RSLT_HTTP_CLIENT_ERROR_OBJ_NOT_INITIALIZED;
    }

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Acquire global mutex : %p..!\n", http_mutex );
    result = cy_rtos_get_mutex( &http_mutex, CY_RTOS_NEVER_TIMEOUT );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Acquire lock failed with result = 0x%X\n", (unsigned long)result );
        return CY_RSLT_HTTP_CLIENT_ERROR;
    }

    if( handle == NULL )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        result = CY_RSLT_HTTP_CLIENT_ERROR_OBJ_NOT_INITIALIZED;
        goto exit;
    }

    http_obj = (cy_http_client_object_t *)handle;

    if( http_obj->isobjinitialized != true )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n HTTP client library not initialized \n" );
        result = CY_RSLT_HTTP_CLIENT_ERROR;
        goto exit;
    }

    if( cy_rtos_deinit_mutex( &(http_obj->obj_mutex) ) != CY_RSLT_SUCCESS )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "Mutex deinit failed with result = 0x%X\n", (unsigned long)result );
        /* Fall-through. It's intentional */
    }
    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Deinit object mutex : %p..!\n", http_obj->obj_mutex );

    /* Clear this flag to avoid calling this API multiple times. */
    http_obj->isobjinitialized = false;

    free( (char *)http_obj->server_info.host_name );
    free( http_obj->network_context );
    free( http_obj );

    handle = NULL;

exit :
    cy_rtos_set_mutex( &http_mutex );
    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Release Global mutex : %p..!\n", http_mutex );

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return result;
}

cy_rslt_t cy_http_client_deinit( void )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): START \n", __FUNCTION__ );

    if( http_client_instance_count == 0 )
    {
        cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "http client Library is not initialized (or) already de-initialized\n" );
        return CY_RSLT_HTTP_CLIENT_ERROR_DEINIT_FAIL;
    }
    else
    {
        http_client_instance_count--;
        if( http_client_instance_count == 0 )
        {
            cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "De-initialize http client library\n");
            result = cy_awsport_network_deinit();
            if( result != CY_RSLT_SUCCESS )
            {
                cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "De-initialize http client library failed with result = %lu\n", (unsigned long)result );
                /* Fall-through. It's intentional. */
            }

            /* Terminate the disconnect thread */
            if( http_client_disconnect_event_thread != NULL )
            {
                cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\nTerminating HTTP CLIENT disconnect event thread %p..!\n", http_client_disconnect_event_thread );
                result = cy_rtos_terminate_thread( &http_client_disconnect_event_thread );
                if( result != CY_RSLT_SUCCESS )
                {
                    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\nTerminate HTTP CLIENT disconnect event thread failed with Error : [0x%X] ", (unsigned int)result );
                    /* Fall-through. It's intentional. */
                }

                cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\nJoining HTTP CLIENT disconnect event thread %p..!\n", http_client_disconnect_event_thread );
                result = cy_rtos_join_thread( &http_client_disconnect_event_thread );
                if( result != CY_RSLT_SUCCESS )
                {
                    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\nJoin HTTP CLIENT disconnect event thread failed with Error : [0x%X] ", (unsigned int)result );
                    /* Fall-through. It's intentional. */
                }
                http_client_disconnect_event_thread = NULL;
            }

            /* Delete the disconnect event queue */
            result = cy_rtos_deinit_queue( &http_client_disconnect_event_queue );
            if( result != CY_RSLT_SUCCESS )
            {
                cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\ncy_rtos_deinit_queue failed with Error : [0x%X] ", (unsigned int)result );
                /* Fall-through. It's intentional. */
            }
            else
            {
                cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\ncy_rtos_deinit_queue successful." );
            }

            cy_rtos_deinit_mutex( &http_mutex );
            cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Global Mutex Deinit..!\n" );

            cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\nDe-initialized http client library. Number of http instance : [%d] \n",http_client_instance_count );
        }
    }

    cy_hc_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s(): END \n", __FUNCTION__ );

    return CY_RSLT_SUCCESS;
}
