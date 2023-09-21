# Find the libraw library.
#
# This module defines the following variables:
#
# * LIBRAW_INCLUDE_DIRS
# * LIBRAW_LIBRARIES
#
# This module defines the following imported targets:
#
# * LibRaw::libraw
#
# This module defines the following interfaces:
#
# * LibRaw

find_package(ZLIB REQUIRED)

find_path(LIBRAW_INCLUDE_DIR NAMES libraw.h)
set(LIBRAW_INCLUDE_DIRS
    ${LIBRAW_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS})

if(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    find_library(LIBRAW_LIBRARY
        NAMES libraw16d liblibraw16d liblibraw16_staticd librawd libraw16 libraw16_static liblibraw16_static libraw)
else()
    find_library(LIBRAW_LIBRARY
        NAMES libraw16 liblibraw16 libraw16_static liblibraw16_static libraw)
endif()
set(LIBRAW_LIBRARIES
    ${LIBRAW_LIBRARY}
    ${ZLIB_LIBRARIES})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    LIBRAW
    REQUIRED_VARS LIBRAW_INCLUDE_DIR LIBRAW_LIBRARY)
mark_as_advanced(LIBRAW_INCLUDE_DIR LIBRAW_LIBRARY)

if(LIBRAW_FOUND AND NOT TARGET LibRaw::libraw)
    add_library(LibRaw::libraw UNKNOWN IMPORTED)
    set_target_properties(LibRaw::libraw PROPERTIES
        IMPORTED_LOCATION "${LIBRAW_LIBRARY}"
        INTERFACE_COMPILE_DEFINITIONS LIBRAW_FOUND
        INTERFACE_INCLUDE_DIRECTORIES "${LIBRAW_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "ZLIB")
endif()
if(LIBRAW_FOUND AND NOT TARGET LIBRAW)
    add_library(LIBRAW INTERFACE)
    target_link_libraries(LIBRAW INTERFACE LibRaw::libraw)
endif()
