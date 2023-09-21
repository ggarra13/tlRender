include(ExternalProject)


set(LibRaw_URL "https://www.libraw.org/data/LibRaw-0.21.1.tar.gz")

ExternalProject_Add(
    LibRaw_cmake
    GIT_REPOSITORY "https://github.com/LibRaw/LibRaw-cmake"
    GIT_TAG master
    BUILD_IN_SOURCE 0
    BUILD_ALWAYS 0
    UPDATE_COMMAND ""
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/LibRaw_cmake
    INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

set(LibRaw_ARGS
    ${TLRENDER_EXTERNAL_ARGS}
    -DENABLE_OPENMP=ON 
    -DENABLE_EXAMPLES=OFF
    -DENABLE_LCMS=OFF
    -DBUILD_TESTING=OFF
)

set(LibRaw_PATCH
    ${CMAKE_COMMAND} -E copy_if_different <SOURCE_DIR>_cmake/CMakeLists.txt <SOURCE_DIR> &&
    ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>_cmake/cmake
    )

ExternalProject_Add(
     LibRaw
     PREFIX ${CMAKE_CURRENT_BINARY_DIR}/LibRaw
     URL ${LibRaw_URL}
     DEPENDS LibRaw_cmake ZLIB libjpeg-turbo # LCMS2
     PATCH_COMMAND ${LibRaw_PATCH}
     LIST_SEPARATOR |
     CMAKE_ARGS ${LibRaw_ARGS}
)
