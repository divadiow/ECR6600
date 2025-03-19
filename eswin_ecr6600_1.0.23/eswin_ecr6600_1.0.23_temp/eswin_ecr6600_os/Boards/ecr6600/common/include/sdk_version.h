#ifndef __SDK_VERSION_H__
#define __SDK_VERSION_H__
extern const volatile char  sdk_version_lib_compile_date[]__attribute__ ((section(".version.data")));
extern const volatile char  sdk_version_lib_compile_time[]__attribute__ ((section(".version.data")));
extern const volatile char  wifi_version[]__attribute__ ((section(".version.data")));
extern const volatile char  sdk_version[]__attribute__ ((section(".version.data")));
extern const volatile char __release_version[];

#define RELEASE_VERSION ((char*)(&__release_version[4]))

#endif

