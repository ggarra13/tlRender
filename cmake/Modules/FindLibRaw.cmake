# Find the libraw library.
#
# This module defines the following variables:
#
# * LibRaw_INCLUDE_DIRS
# * LibRaw_LIBRARIES
#
# This module defines the following imported targets:
#
# * LibRaw::libraw
#
# This module defines the following interfaces:
#
# * LibRaw

find_package(ZLIB REQUIRED)

find_path(LibRaw_INCLUDE_DIR NAMES libraw/libraw.h)
set(LibRaw_INCLUDE_DIRS
    ${LibRaw_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS})

if(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    find_library(LibRaw_LIBRARY
        NAMES rawd raw)
else()
    find_library(LibRaw_LIBRARY
        NAMES raw)
endif()
set(LibRaw_LIBRARIES
    ${LibRaw_LIBRARY}
    ${ZLIB_LIBRARIES})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    LibRaw
    REQUIRED_VARS LibRaw_INCLUDE_DIR LibRaw_LIBRARY)
mark_as_advanced(LibRaw_INCLUDE_DIR LibRaw_LIBRARY)

if(LibRaw_FOUND AND NOT TARGET LibRaw::libraw)
    add_library(LibRaw::libraw UNKNOWN IMPORTED)
    set_target_properties(LibRaw::libraw PROPERTIES
        IMPORTED_LOCATION "${LibRaw_LIBRARY}"
        INTERFACE_COMPILE_DEFINITIONS LibRaw_FOUND
        INTERFACE_INCLUDE_DIRECTORIES "${LibRaw_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "ZLIB")
endif()
if(LibRaw_FOUND AND NOT TARGET LibRaw)
    add_library(LibRaw INTERFACE)
    target_link_libraries(LibRaw INTERFACE LibRaw::libraw)
endif()
