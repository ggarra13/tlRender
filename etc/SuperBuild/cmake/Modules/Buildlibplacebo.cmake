include(ExternalProject)


set(libplacebo_GIT_REPO "https://code.videolan.org/videolan/libplacebo.git")
set(libplacebo_GIT_TAG v7.349.0)

if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    set(libplacebo_CFLAGS -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
    set(libplacebo_CXXFLAGS -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
    set(libplacebo_LDFLAGS -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
endif()

set(libplacebo_CONFIGURE ${CMAKE_COMMAND} -E env PYTHONPATH="" "CXXFLAGS=${libplacebo_CXXFLAGS}" "CFLAGS=${libplacebo_CFLAGS}" "LDFLAGS=${libplacebo_LDFLAGS}" -- meson setup --default-library=static -Dlibdir=${CMAKE_INSTALL_PREFIX}/lib --prefix=${CMAKE_INSTALL_PREFIX} build)
set(libplacebo_BUILD export PYTHONPATH="" && cd build && ninja)
set(libplacebo_INSTALL export PYTHONPATH="" && cd build && ninja install)

ExternalProject_Add(
    libplacebo
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libplacebo
    GIT_REPOSITORY ${libplacebo_GIT_REPO}
    GIT_TAG ${libplacebo_GIT_TAG}
    GIT_SHALLOW 1
    CONFIGURE_COMMAND ${libplacebo_CONFIGURE}
    BUILD_COMMAND ${libplacebo_BUILD}
    INSTALL_COMMAND ${libplacebo_INSTALL}
    BUILD_IN_SOURCE 1
)

