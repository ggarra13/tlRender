# Find the NDI library.
#
# This module defines the following variables:
#
# * NDI_INCLUDE_DIRS
# * NDI_LIBRARIES
#
# This module defines the following imported targets:
#
# * NDI::ndi
#
# This module defines the following interfaces:
#
# * NDI

find_path(NDI_INCLUDE_DIR
    NAMES Processing.NDI.Recv.h
    PATHS ${TLRENDER_NDI_SDK}/include
)
set(NDI_INCLUDE_DIRS
    ${NDI_INCLUDE_DIR})

find_library(NDI_LIBRARY NAMES ndi
    PATHS ${TLRENDER_NDI_SDK}/lib/x86_64-linux-gnu)
set(NDI_LIBRARIES
    ${NDI_LIBRARY})
if(APPLE)
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "arm")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    NDI
    REQUIRED_VARS
        NDI_INCLUDE_DIR
        NDI_LIBRARY)
mark_as_advanced(
    NDI_INCLUDE_DIR
    NDI_LIBRARY)

set(NDI_LINK_LIBRARIES )
if(NDI_FOUND AND NOT TARGET NDI::ndi)
    set(NDI_INTERFACE_LINK_LIBRARIES)
    add_library(NDI::ndi UNKNOWN IMPORTED)
    set_target_properties(NDI::ndi PROPERTIES
        IMPORTED_LOCATION "${NDI_LIBRARY}"
        INTERFACE_COMPILE_DEFINITIONS NDI_FOUND
        INTERFACE_INCLUDE_DIRECTORIES "${NDI_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${NDI_LINK_LIBRARIES}")
endif()
if(NDI_FOUND AND NOT TARGET NDI)
    add_library(NDI INTERFACE)
    target_link_libraries(NDI INTERFACE NDI::ndi)
endif()
