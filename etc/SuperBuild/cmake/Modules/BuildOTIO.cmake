include(ExternalProject)

set(OTIO_GIT_REPOSITORY "https://github.com/ggarra13/OpenTimelineIO.git")
set(OTIO_GIT_TAG "test_edit_commands")

#set(OTIO_GIT_REPOSITORY "https://github.com/PixarAnimationStudios/OpenTimelineIO.git")
#set(OTIO_GIT_TAG "v0.15")

set(OTIO_SHARED_LIBS ON)
if(NOT BUILD_SHARED_LIBS)
    set(OTIO_SHARED_LIBS OFF)
endif()

set(OTIO_ARGS
    ${TLRENDER_EXTERNAL_ARGS}
    -DOTIO_FIND_IMATH=ON
    -DOTIO_SHARED_LIBS=${OTIO_SHARED_LIBS}
    -DOTIO_PYTHON_INSTALL=${TLRENDER_ENABLE_PYTHON})

ExternalProject_Add(
    OTIO
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/OTIO
    DEPENDS Imath
    GIT_REPOSITORY ${OTIO_GIT_REPOSITORY}
    GIT_TAG ${OTIO_GIT_TAG}
    LIST_SEPARATOR |
    CMAKE_ARGS ${OTIO_ARGS})
