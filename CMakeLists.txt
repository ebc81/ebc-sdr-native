# Shared native SDR base for the EBC Android apps.
#
# Usage from an app:
#
#   add_subdirectory(ebc-sdr-native)
#   target_link_libraries(<app-target> PRIVATE ebc_sdr ${log-lib})
#
# That is all. Include paths, compile options and the 16 KB page-alignment link flags come
# along with the target -- do not repeat them in the app, and do not list any file of this
# tree in the app's own add_library().
#
# Provenance of every source here: PROVENANCE.md. Rules for working in it: AGENTS.md.

cmake_minimum_required(VERSION 3.22)

project(ebc_sdr C)

# =============================================================================
# Options
# =============================================================================

# rtl-sdr's convenience/ helpers (verbose_device_search, nearest_gain, set_gain_by_perc, ...).
# Only RTL_SDR_AIS_Driver compiles them today, so they are opt-in rather than dead weight in
# the other two apps. They are GPL like the rest; see LICENSE.md.
option(EBC_SDR_CONVENIENCE "Build rtl-sdr's convenience helpers into the library" OFF)

# logcat tag for everything this library prints -- both the bridge's aprintf_stderr() and the
# fprintf(stderr, ...) diagnostics that ebc_log.h redirects. One tag per app makes the log
# readable when several native components talk at once.
set(EBC_SDR_LOG_TAG "librtlsdr" CACHE STRING "logcat tag used by the shared SDR base")

set(RTLSDR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rtl-sdr)
set(LIBUSB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libusb-andro)
set(ANDROID_GLUE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android)

# =============================================================================
# Sources
# =============================================================================
# Note that librtlsdr.c IS listed. Until Phase 1 it could not be: android/librtlsdr_andro.c
# pulled it in with #include, so listing it here produced duplicate symbols. It reaches the
# private struct rtlsdr_dev through rtl-sdr/src/librtlsdr_internal.h now.

add_library(ebc_sdr STATIC
    # rtl-sdr
    ${RTLSDR_DIR}/src/librtlsdr.c
    ${RTLSDR_DIR}/src/tuner_e4k.c
    ${RTLSDR_DIR}/src/tuner_fc0012.c
    ${RTLSDR_DIR}/src/tuner_fc0013.c
    ${RTLSDR_DIR}/src/tuner_fc2580.c
    ${RTLSDR_DIR}/src/tuner_r82xx.c

    # libusb 1.0.23, Android port. Device enumeration and netlink hotplug are compiled out
    # inside the sources; the only way in is libusb_wrap_sys_device() with a file descriptor.
    ${LIBUSB_DIR}/libusb/core.c
    ${LIBUSB_DIR}/libusb/descriptor.c
    ${LIBUSB_DIR}/libusb/hotplug.c
    ${LIBUSB_DIR}/libusb/io.c
    ${LIBUSB_DIR}/libusb/strerror.c
    ${LIBUSB_DIR}/libusb/sync.c
    ${LIBUSB_DIR}/libusb/os/linux_netlink.c
    ${LIBUSB_DIR}/libusb/os/linux_usbfs.c
    ${LIBUSB_DIR}/libusb/os/poll_posix.c
    ${LIBUSB_DIR}/libusb/os/threads_posix.c

    # Android glue: fd -> rtlsdr_dev_t
    ${ANDROID_GLUE_DIR}/librtlsdr_andro.c
)

if(EBC_SDR_CONVENIENCE)
    target_sources(ebc_sdr PRIVATE ${RTLSDR_DIR}/src/convenience/convenience.c)
    target_include_directories(ebc_sdr PUBLIC ${RTLSDR_DIR}/src/convenience)
endif()

# rtl-sdr/src/getopt/ is deliberately absent -- no app compiles it, and Android has getopt in
# libc. It is not vendored in this tree at all; see PROVENANCE.md.

# =============================================================================
# Include paths
# =============================================================================
# PUBLIC is the app-facing contract:
#   rtl-sdr/include   rtl-sdr.h and the tuner headers
#   libusb-andro/libusb   libusb.h -- rtlsdr433's rtl433/src/sdr.c includes it directly
#   android           librtlsdr_andro.h (rtlsdr_open2 and friends) and ebc_log.h
#
# PRIVATE keeps the internals internal, and that is load-bearing rather than tidiness:
#   rtl-sdr/src       librtlsdr_internal.h exposes the private struct rtlsdr_dev. Only
#                     librtlsdr.c and the bridge may see it, and PRIVATE is what enforces it.
#   libusb-andro      config.h, reached as <config.h> from libusbi.h:26

target_include_directories(ebc_sdr
    PUBLIC
        ${RTLSDR_DIR}/include
        ${LIBUSB_DIR}/libusb
        ${ANDROID_GLUE_DIR}
    PRIVATE
        ${RTLSDR_DIR}/src
        ${LIBUSB_DIR}
)

# =============================================================================
# Compile options
# =============================================================================
# These sat in RTL_SDR_AIS_Driver's global CMAKE_C_FLAGS and therefore also hit that app's own
# sources and its AIS decoder; rtlsdr433 and rtlsdrPager set none of them. On the target they
# apply to exactly this library.

target_compile_options(ebc_sdr PRIVATE
    # Safe because rtl-sdr_export.h marks the public API
    # __attribute__((visibility("default"))) explicitly -- so RTLSDR_API stays exported from
    # the app's .so while everything else, including struct rtlsdr_dev's helpers, stops
    # leaking out of it.
    -fvisibility=hidden

    # Better native backtraces from the Play Console. This one earns its place.
    -funwind-tables

    # Inherited from RTL_SDR_AIS_Driver, kept for continuity rather than effect: since
    # ebc_log.h redirects fprintf to ebc_log_fprintf, there is no builtin fprintf call left
    # in this library for the compiler to fold, and no printf call at all. Harmless, and
    # cheaper to keep than to re-derive why it went away.
    -fno-builtin-printf
    -fno-builtin-fprintf
)

# Vendored code is third-party: -Wall on it would drown the build in warnings that must not be
# "fixed" (see AGENTS.md). The Android glue is ours, so it gets the strict set -- including
# -Werror=shorten-64-to-32, which is the contract the five backported narrowing casts exist
# for. Keep this in step with the app-side flags on their own bridge files.
set_source_files_properties(
    ${ANDROID_GLUE_DIR}/librtlsdr_andro.c
    PROPERTIES COMPILE_OPTIONS
        "-Wall;-Wextra;-Wno-unused-parameter;-Werror=shorten-64-to-32;-Werror=implicit-function-declaration;-Werror=return-type;-Werror=pointer-to-int-cast;-Werror=int-to-pointer-cast"
)

target_compile_definitions(ebc_sdr PUBLIC EBC_LOG_TAG="${EBC_SDR_LOG_TAG}")

# =============================================================================
# Link interface
# =============================================================================

find_library(log-lib log)
target_link_libraries(ebc_sdr PUBLIC ${log-lib})

# Android 15+ requires 16 KB-page-aligned shared libraries on 64-bit devices. NDK r28+ aligns
# by default, but an NDK downgrade below r28 would break installs on a green build -- which is
# exactly what rtlsdr433 and rtlsdrPager are exposed to today, since only
# RTL_SDR_AIS_Driver pins these flags.
#
# INTERFACE, not PRIVATE: a static library has no link step of its own. This way the flags
# reach whatever shared object links ebc_sdr, and all three apps get the guarantee by linking
# the library rather than by remembering to copy two lines.
if(ANDROID_ABI STREQUAL "arm64-v8a" OR ANDROID_ABI STREQUAL "x86_64")
    target_link_options(ebc_sdr INTERFACE
        "-Wl,-z,max-page-size=16384"
        "-Wl,-z,common-page-size=16384"
    )
endif()

# =============================================================================
# API level
# =============================================================================
# RTL_SDR_AIS_Driver has minSdk 23, rtlsdr433 and rtlsdrPager 29. The floor for this library
# is therefore 23, and it is verified there -- see PROVENANCE.md. Fail loudly rather than
# emit a library that cannot load on a device the app claims to support.
if(DEFINED ANDROID_PLATFORM_LEVEL AND ANDROID_PLATFORM_LEVEL LESS 21)
    message(FATAL_ERROR
        "ebc_sdr needs API 21 or later (libusb's linux_usbfs backend); got ${ANDROID_PLATFORM_LEVEL}.")
endif()
