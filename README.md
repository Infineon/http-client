# HTTP Client Library


## Introduction

This library provides the HTTP Client implementation that can work on the PSoC&trade; 6 MCU platforms with Wi-Fi connectivity.
This library supports RESTful methods such as GET, PUT, POST, and HEAD to communicate with the HTTP Server. It uses **coreHTTP** module of **AWS IoT Device SDK for Embedded C** library. Please refer to **AWS IoT Device SDK for Embedded C** library's [readme](https://github.com/aws/aws-iot-device-sdk-embedded-C/tree/202011.00#corehttp) for the HTTP protocol versions supported.

## Features

- Supports Wi-Fi and Ethernet connections.

- Secure [with TLS security] and non-secure modes of connection.

- Supports RESTful HTTP methods: HEAD, GET, PUT, POST, DELETE, PATCH, CONNECT, OPTIONS and TRACE.

- Provides synchronous API to send the request and receive the response.

- Provides utility APIs to create HTTP headers for forming the HTTP request, and to parse the HTTP headers received in the response.

- Supports large data downloads through range requests.

- Multiple HTTP Client instance is supported.

## Limitations

- This library does not support pipelining the HTTP requests. It supports one request-response at a time.

- Transfer-encoding with "chunked" type is partially supported with a transfer size limit of size equal to the user-given buffer; streaming data transfer is not supported.

## Supported Platforms

- [PSoC&trade; 6 WiFi-BT Prototyping Kit (CY8CPROTO-062-4343W)](https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-062-4343w/)

- [PSoC&trade; 62S2 Wi-Fi BT Pioneer Kit (CY8CKIT-062S2-43012)](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-062s2-43012/)

- [PSoC&trade; 6 WiFi-BT Pioneer Kit (CY8CKIT-062-WiFi-BT)](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-062-wifi-bt/)

- [PSoC&trade; 62S2 evaluation kit (CY8CEVAL-062S2-LAI-4373M2)](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ceval-062s2/)

- [PSoC&trade; 62S2 evaluation kit (CY8CEVAL-062S2-MUR-43439M2)](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ceval-062s2/)

- [XMC7200D-E272K8384 kit (KIT-XMC72-EVK)](https://www.infineon.com/cms/en/product/evaluation-boards/kit_xmc72_evk/)

- [XMC7200D-E272K8384 kit (KIT_XMC72_EVK_MUR_43439M2)](https://www.infineon.com/cms/en/product/evaluation-boards/kit_xmc72_evk/)

- [PSoC&trade; 62S2 evaluation kit (CY8CEVAL-062S2-CYW43022CUB)](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ceval-062s2/)

- [CYW955913EVK-01 Wi-Fi Bluetooth&reg; Prototyping Kit (CYW955913EVK-01)](https://www.infineon.com/CYW955913EVK-01)

- [PSoC&trade; 62S2 evaluation kit (CY8CEVAL-062S2-CYW955513SDM2WLIPA)]( https://www.infineon.com/cms/en/product/evaluation-boards/cy8ceval-062s2/ )

- PSOC&trade; Edge E84 Evaluation Kit

## Supported Frameworks

This middleware library supports the ModusToolbox&trade; environment.

In this environment the HTTP Client Library uses the [abstraction-rtos](https://github.com/Infineon/abstraction-rtos) library for RTOS abstraction APIs and the [secure-sockets](https://github.com/Infineon/secure-sockets) and [wifi-connection-manager](https://github.com/Infineon/wifi-connection-manager) libraries for network and connectivity functions.

## Dependencies

- [AWS IoT SDK PORT](https://github.com/Infineon/aws-iot-device-sdk-port)

## Quick Start

This library is supported only on ModusToolbox&trade;.

1. To use http-client library with Wi-Fi kits on FreeRTOS, lwIP, and Mbed TLS combination, the application should pull [http-client](https://github.com/Infineon/http-client) library and [wifi-core-freertos-lwip-mbedtls](https://github.com/Infineon/wifi-core-freertos-lwip-mbedtls) library which will internally pull secure-sockets, wifi-connection-manager, FreeRTOS, lwIP, Mbed TLS and other dependent modules.
To pull wifi-core-freertos-lwip-mbedtls and http-client libraries create the following *.mtb* files in deps folder.
   - *wifi-core-freertos-lwip-mbedtls.mtb:*
      <code>
      https://github.com/Infineon/wifi-core-freertos-lwip-mbedtls#latest-v1.X#$$ASSET_REPO$$/wifi-core-freertos-lwip-mbedtls/latest-v1.X
      </code>

      **Note:** To use TLS version 1.3, please upgrade wifi-core-freertos-lwip-mbedtls to latest-v2.X (It is supported on all the platforms except [PSoC&trade; 64S0S2 Wi-Fi Bluetooth&reg; pioneer kit (CY8CKIT-064S0S2-4343W)](https://www.cypress.com/documentation/development-kitsboards/psoc-64-standard-secure-aws-wi-fi-bt-pioneer-kit-cy8ckit))
   - *http-client.mtb:*
      <code>
      https://github.com/Infineon/http-client#latest-v1.X#$$ASSET_REPO$$/http-client/latest-v1.X
      </code>

2. To use http-client library with CYW955913EVK-01 kit, the application should pull [http-client](https://github.com/Infineon/http-client) library and [wifi-core-threadx-cat5](https://github.com/Infineon/wifi-core-threadx-cat5) library which will internally pull secure-sockets, wifi-connection-manager and other dependent modules.
To pull wifi-core-threadx-cat5 and http-client libraries create the following *.mtb* files in deps folder.
   - *wifi-core-threadx-cat5.mtb:*
      <code>
      https://github.com/Infineon/wifi-core-threadx-cat5#latest-v1.X#$$ASSET_REPO$$/wifi-core-threadx-cat5/latest-v1.X
      </code>

   - *http-client.mtb:*
      <code>
      https://github.com/Infineon/http-client#latest-v1.X#$$ASSET_REPO$$/http-client/latest-v1.X
      </code>

3. To use http-client library with Ethernet kits on FreeRTOS, lwIP, and Mbed TLS combination, the application should pull [http-client](https://github.com/Infineon/http-client) library and [ethernet-core-freertos-lwip-mbedtls](https://github.com/Infineon/ethernet-core-freertos-lwip-mbedtls) library which will internally pull secure-sockets, ethernet-connection-manager, FreeRTOS, lwIP, Mbed TLS and other dependent modules.
To pull ethernet-core-freertos-lwip-mbedtls and http-client libraries create the following *.mtb* files in deps folder.
   - *ethernet-core-freertos-lwip-mbedtls.mtb:*
      <code>
      https://github.com/Infineon/ethernet-core-freertos-lwip-mbedtls#latest-v1.X#$$ASSET_REPO$$/ethernet-core-freertos-lwip-mbedtls/latest-v1.X
      </code>

      **Note:** To use TLS version 1.3, please upgrade ethernet-core-freertos-lwip-mbedtls to latest-v2.X.
   - *http-client.mtb:*
      <code>
      https://github.com/Infineon/http-client#latest-v1.X#$$ASSET_REPO$$/http-client/latest-v1.X
      </code>

4. Review and make the required changes to the pre-defined configuration files.
 - The configuration files are bundled with the wifi-mw-core library for FreeRTOS, lwIP, and Mbed TLS. See [README.md](https://github.com/Infineon/wifi-mw-core/blob/master/README.md) for details.
 - If the application is using bundle library then the configuration files are in the bundle library. For example if the application is using **Wi-Fi core freertos lwip mbedtls bundle library**, the configuration files are in `wifi-core-freertos-lwip-mbedtls/configs` folder. Similarly if the application is using **Ethernet Core FreeRTOS lwIP mbedtls library**, the configuration files are in `ethernet-core-freertos-lwip-mbedtls/configs` folder.
 **Note:** Configuration file changes are not required for CYW955913EVK-01.

5. Define the following COMPONENTS in the application's Makefile for the Azure port library.
    ```
    COMPONENTS=FREERTOS MBEDTLS LWIP SECURE_SOCKETS
    ```
    **Note:** For CYW955913EVK-01 only the following COMPONENTS are required.
              ```
              COMPONENTS=SECURE_SOCKETS
              ```
6. By default, the HTTP Client Library disables all the debug log messages. To enable log messages, the application must perform the following:

   1. Add the `ENABLE_HTTP_CLIENT_LOGS` macro to the *DEFINES* in the code example's Makefile. The Makefile entry would look like as follows:
       ```
       DEFINES+=ENABLE_HTTP_CLIENT_LOGS
       ```
   2. Call the `cy_log_init()` function provided by the *cy-log* module. cy-log is part of the *connectivity-utilities* library.

      See [connectivity-utilities library API documentation](https://infineon.github.io/connectivity-utilities/api_reference_manual/html/group__logging__utils.html) for cy-log details.

7. Define the following macro in the application's Makefile to configure the response header maximum length to 'N'. By default, this value will be set to 2048:
   ```
   DEFINES+=HTTP_MAX_RESPONSE_HEADERS_SIZE_BYTES=<N>
   ```
8. Define the following macro in the application's Makefile to configure the user agent name in all request headers. By default, this component will be added to the request header. Update this for user-defined agent values:

   ```
   DEFINES += HTTP_USER_AGENT_VALUE="\"anycloud-http-client\""
   ```
9. Define the following macro in the application's Makefile to mandatorily disable custom configuration header file:
   ```
   DEFINES += HTTP_DO_NOT_USE_CUSTOM_CONFIG
   DEFINES += MQTT_DO_NOT_USE_CUSTOM_CONFIG
   ```
10. Define the following macro in the application's makefile to configure the send timeout 'M' & receive timeout 'N' for HTTP client library. By default these timeout values will be set to 10ms.
   ```
   DEFINES += HTTP_SEND_RETRY_TIMEOUT_MS=<M>
   DEFINES += HTTP_RECV_RETRY_TIMEOUT_MS=<N>
   ```
11. The "aws-iot-device-sdk-port" layer includes the "coreHTTP" and "coreMQTT" modules of the "aws-iot-device-sdk-embedded-C" library by default. If the user application doesn't use MQTT client features, add the following path in the .cyignore file of the application to exclude the coreMQTT source files from the build.

       ```
       $(SEARCH_aws-iot-device-sdk-embedded-C)/libraries/standard/coreMQTT
       libs/aws-iot-device-sdk-embedded-C/libraries/standard/coreMQTT
       ```

12. Enable smart HTTP response buffering to prevent timeouts.

      - Define the `CY_HTTP_CLIENT_ENABLE_SMART_BUFFERING` macro in the application's Makefile to enable this feature. By default, this feature is disabled.
         ```
         DEFINES += CY_HTTP_CLIENT_ENABLE_SMART_BUFFERING
         ```
         When enabled, this feature optimizes HTTP response handling by:
         1. First reading a small chunk of the response to analyze the headers
         2. Determining the exact response size from the Content-Length header
         3. Requesting the precise response size needed for the complete HTTP response
         4. Reading the remaining response data without unnecessary waiting

         This prevents timeouts that occur when the requested response size is greater than the actual response size, eliminating unnecessary network wait times. Enable this feature to improve response time and reduce network timeouts for HTTP requests where response sizes vary significantly.
      

13. Define the following macro in the application's Makefile to configure the first chunk size 'N' for HTTP client library. By default this value will be set to 128 bytes and will be keep incrementing by 128 bytes until "Content-Length" and header ending byte sequence is found. Note: Ideal case is that the complete HTTP header and header field "Content-Length" be present within this chunk size. If your HTTP responses have large headers, you may need to increase this value accordingly. Keep this value such that it will accomodate the complete header but not complete response ( this should accomodate the complete Header and partial Body ) and should not be more than the total size of http response buffer.
      - Ideally this value should be set to size of complete header including terminating byte sequence + 0 or more byte of body but not complete body.
         ```
            ------------------------------------------------------------------------------------
            | HTTP Response Header  |  HTTP Response Body                                      |
            ------------------------------------------------------------------------------------
            -----------------------------↑ 
            CY_HTTP_RESPONSE_FIRST_CHUNK_SIZE
         ```
         ```
         DEFINES += CY_HTTP_RESPONSE_FIRST_CHUNK_SIZE=<N>
         ```
      - **Note:** The library includes a compile-time sanity check that gives a compile time warning to ensure `CY_HTTP_RESPONSE_FIRST_CHUNK_SIZE` is at least 128 bytes. If you need to use a smaller value (not recommended), make sure it will have atleast one header field in that.
         
         **Warning:** First chunk should contain ATLEAST one Header-Field : Value pair in it. Otherwise the app will fail.

## Notes

`cy_http_client_init` will start a thread which is responsible for sending http disconnect notification to application. This thread is created with priority `CY_RTOS_PRIORITY_ABOVENORMAL`. It is recommended to configure a less priority for the application than the http disconnect event thread. If the application has higher priority and running busy loop, http thread might not get scheduled by the OS which will result in missing of disconnect notification.

## Additional Information

- [HTTP Client RELEASE.md](./RELEASE.md)

- [HTTP Client API Reference Guide](https://infineon.github.io/http-client/api_reference_manual/html/index.html)

- [HTTP Client Library Version](./version.xml)
