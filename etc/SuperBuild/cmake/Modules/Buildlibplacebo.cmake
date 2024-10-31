include(ExternalProject)


set(libplacebo_GIT_REPO "https://code.videolan.org/videolan/libplacebo.git")
set(libplacebo_GIT_TAG v7.349.0)

set(libplacebo_DEPS ${PYTHON_DEP})

if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    set(libplacebo_CFLAGS -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
    set(libplacebo_CXXFLAGS -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
    set(libplacebo_LDFLAGS -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
endif()

set(libplacebo_CONFIGURE
    COMMAND git submodule update --init
    COMMAND ${Python_COMMAND} -m pip install meson
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH="" "CXXFLAGS=${libplacebo_CXXFLAGS}" "CFLAGS=${libplacebo_CFLAGS}" "LDFLAGS=${libplacebo_LDFLAGS}" -- meson setup --default-library=static -Ddemos=false -Dlibdir=${CMAKE_INSTALL_PREFIX}/lib --prefix=${CMAKE_INSTALL_PREFIX} build)
set(libplacebo_BUILD cd build && ninja)
set(libplacebo_INSTALL cd build && ninja install)

set(libplacebo_PATCH)
if (WIN32)
    list(APPEND libplacebo_PATCH
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/libplacebo-patch/meson.build
        ${CMAKE_CURRENT_BINARY_DIR}/libplacebo/src/libplacebo/)
    list(APPEND libplacebo_PATCH
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/libplacebo-patch/src/meson.build
        ${CMAKE_CURRENT_BINARY_DIR}/libplacebo/src/libplacebo/src/)
endif()


ExternalProject_Add(
    libplacebo
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libplacebo
    GIT_REPOSITORY ${libplacebo_GIT_REPO}
    GIT_TAG ${libplacebo_GIT_TAG}
    GIT_SHALLOW 1
    DEPENDS ${libplacebo_DEPS}
    CONFIGURE_COMMAND ${libplacebo_CONFIGURE}
    PATCH_COMMAND ${libplacebo_PATCH}
    BUILD_COMMAND ${libplacebo_BUILD}
    INSTALL_COMMAND ${libplacebo_INSTALL}
    BUILD_IN_SOURCE 1
)

