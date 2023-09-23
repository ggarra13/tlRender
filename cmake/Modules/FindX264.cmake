# Find the zlib library.
#
# This module defines the following variables:
#
# * X264_VERSION
# * X264_INCLUDE_DIRS
# * X264_LIBRARIES
#
# This module defines the following imported targets:
#
# * X264::X264
#
# This module defines the following interfaces:
#
# * X264


find_path(X264_INCLUDE_DIR NAMES x264.h)
set(X264_INCLUDE_DIRS ${X264_INCLUDE_DIR})

if(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    find_library(X264_LIBRARY NAMES x264d x264 libx264d libx264)
else()
    find_library(X264_LIBRARY NAMES x264 libx264)
endif()
set(X264_LIBRARIES ${X264_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    X264
    REQUIRED_VARS X264_INCLUDE_DIR X264_LIBRARY
    VERSION_VAR X264_VERSION)
mark_as_advanced(X264_INCLUDE_DIR X264_LIBRARY)

if(X264_FOUND AND NOT TARGET X264::X264)
    add_library(X264::X264 UNKNOWN IMPORTED)
    set_target_properties(X264::X264 PROPERTIES
        IMPORTED_LOCATION "${X264_LIBRARY}"
        INTERFACE_COMPILE_DEFINITIONS X264_FOUND
        INTERFACE_INCLUDE_DIRECTORIES "${X264_INCLUDE_DIR}")
endif()
if(X264_FOUND AND NOT TARGET X264)
    add_library(X264 INTERFACE)
    target_link_libraries(X264 INTERFACE X264::X264)
endif()
