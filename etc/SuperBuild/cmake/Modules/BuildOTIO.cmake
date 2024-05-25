include(ExternalProject)

set(OTIO_GIT_REPOSITORY "https://github.com/AcademySoftwareFoundation/OpenTimelineIO.git")

# \@bug: Compiling OTIO on an M1 machine and running it on an M3 machine can
#        run into issues with the functions to/from_json_string() not decoing
#        and re-encoding valid .otio json files.
#set(OTIO_GIT_TAG "v0.16.0") # stable branch, has problems with M3 chips

set(OTIO_GIT_TAG 48f09e244c1b77190d1ecc6d4cdef942b8336960)

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
