# HTTP Client Library

## What's Included?
See the [README.md](./README.md) for a complete description of the HTTP Client library.

## Known issues
| Problem | Workaround |
| ------- | ---------- |
| IAR 9.30 toolchain throws build errors on Debug mode, if application explicitly includes iar_dlmalloc.h file | Add '--advance-heap' to LDFLAGS in application Makefile. |

## Changelog

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
| ModusToolbox&trade; Software Environment                  | 3.0     |
| - ModusToolbox&trade; Device Configurator                 | 4.0     |
| - ModusToolbox&trade; CapSense Configurator / Tuner tools | 5.0     |
| PSoC 6 Peripheral Driver Library (PDL)                    | 3.0.0   |
| GCC Compiler                                              | 10.3.1  |
| IAR Compiler (Only for ModusToolbox&trade;)               | 9.30    |
| Arm Compiler 6                                            | 6.16    |
