include(ExternalProject)


set(LibRaw_URL "https://www.libraw.org/data/LibRaw-0.21.1.tar.gz")

ExternalProject_Add(
    LibRaw_cmake
    GIT_REPOSITORY "https://github.com/LibRaw/LibRaw-cmake"
    GIT_TAG master
    BUILD_IN_SOURCE 0
    BUILD_ALWAYS 0
    UPDATE_COMMAND ""
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/LibRaw_cmake
    INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

if(APPLE)
    set(CMAKE_CXX_FLAGS "-Wno-register ${CMAKE_CXX_FLAGS}")
endif()

set(LibRaw_ARGS
    ${TLRENDER_EXTERNAL_ARGS}
    -DBUILD_SHARED_LIBS=ON
    -DBUILD_STATIC_LIBS=OFF
    -DENABLE_OPENMP=ON
    -DENABLE_JASPER=ON
    -DENABLE_LCMS=ON
    -DENABLE_EXAMPLES=OFF
    -DBUILD_TESTING=OFF
    -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
)

set(LibRaw_PATCH
    ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/LibRaw-patch/CMakeLists.txt <SOURCE_DIR> &&
    ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/LibRaw_cmake/src/LibRaw_cmake/cmake <SOURCE_DIR>/cmake
)

set(LibRaw_DEPS LibRaw_cmake ZLIB)
if(TLRENDER_JPEG)
    list(APPEND LibRaw_DEPS libjpeg-turbo)
endif()
if(UNIX)
    list(APPEND LibRaw_DEPS LCMS2)
endif()
    
ExternalProject_Add(
     LibRaw
     PREFIX ${CMAKE_CURRENT_BINARY_DIR}/LibRaw
     URL ${LibRaw_URL}
     DEPENDS ${LibRaw_DEPS}
     PATCH_COMMAND ${LibRaw_PATCH}
     LIST_SEPARATOR |
     CMAKE_ARGS ${LibRaw_ARGS}
)
