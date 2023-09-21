# Find the LibRaw library.
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

find_library(LibRaw_libraw_LIBRARY NAMES raw)
set(LibRaw_LIBRARIES
    ${LibRaw_libraw_LIBRARY}
    ${ZLIB_LIBRARIES})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    LibRaw
    REQUIRED_VARS
        LibRaw_INCLUDE_DIR
        LibRaw_libraw_LIBRARY)

mark_as_advanced(
    LibRaw_INCLUDE_DIR
    LibRaw_libraw_LIBRARY)

if(LibRaw_FOUND AND NOT TARGET LibRaw::libraw)
    add_library(LibRaw::libraw UNKNOWN IMPORTED)
    set(LibRaw_libraw_LINK_LIBRARIES LibRaw::libraw ZLIB)
    set_target_properties(LibRaw::libraw PROPERTIES
        IMPORTED_LOCATION "${LibRaw_libraw_LIBRARY}"
        INTERFACE_LINK_LIBRARIES "${LibRaw_libraw_LINK_LIBRARIES}")
endif()
if(LibRaw_FOUND AND NOT TARGET LibRaw)
    add_library(LibRaw INTERFACE)
    target_link_libraries(LibRaw INTERFACE LibRaw::libraw)
endif()
