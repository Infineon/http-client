# HTTP Client Library

## What's Included?
See the [README.md](./README.md) for a complete description of the HTTP Client library.

## Known issues
| Problem | Workaround |
| ------- | ---------- |
| IAR 9.30 toolchain throws build errors on Debug mode, if application explicitly includes iar_dlmalloc.h file | Add '--advance-heap' to LDFLAGS in application Makefile. |
| HTTPS conneciton over TLS version 1.3 fails with servers enabling session ticket suppport | Currently there is no workaround since the issue is in 3rd party library |

## Changelog

### v1.5.0

* Supports Mbed TLS version 3.4.0
* Supports connection to servers with TLS version 1.3
* Added support for CY8CEVAL-062S2-CYW43022CUB kit

### v1.4.0

* Fixed http disconnect notification issue.
* Added support for KIT_XMC72_EVK_MUR_43439M2 kit

### v1.3.0

* Added support for KIT_XMC72_EVK kit
* Removed deps folder

### v1.2.2

* Updated FreeRTOS specific code to make it generic.
* Documentation updates.

### v1.2.1

* Removed unwanted dependencies from the deps folder
* Added support for CM0P core
* Minor Documentation updates

### v1.2.0
* Added support for CY8CEVAL-062S2-MUR-43439M2 kit

### v1.1.1
* Documentation updates
* General bug fixes

### v1.1.0
* Added support for RESTful HTTP methods DELETE, PATCH, CONNECT, OPTIONS and TRACE
* Introduced ARMC6 compiler support for AnyCloud build.

### v1.0.0
* Initial release for AnyCloud.

### Supported Software and Tools
This version of the library was validated for compatibility with the following software and tools:

| Software and Tools                                        | Version |
| :---                                                      | :----:  |
| ModusToolbox&trade; Software Environment                  | 3.1     |
| ModusToolbox&trade; Device Configurator                   | 4.10    |
| GCC Compiler                                              | 11.3.1  |
| IAR Compiler (Only for ModusToolbox&trade;)               | 9.30    |
| Arm Compiler 6                                            | 6.16    |
