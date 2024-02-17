include(ExternalProject)


set(dav1d_GIT_TAG 1.3.0)

set(dav1d_CONFIGURE ${CMAKE_COMMAND} -E env PYTHONPATH="" "CXXFLAGS=${CMAKE_CXX_FLAGS}" "CFLAGS=${CMAKE_C_FLAGS}" "LDFLAGS=${CMAKE_SHARED_LINKER_FLAGS}" -- meson setup -Denable_tools=false -Denable_tests=false --default-library=static -Dlibdir=${CMAKE_INSTALL_PREFIX}/lib --prefix=${CMAKE_INSTALL_PREFIX} build)
set(dav1d_BUILD export PYTHONPATH="" && cd build && ninja)
set(dav1d_INSTALL export PYTHONPATH="" && cd build && ninja install)

ExternalProject_Add(
    dav1d
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/dav1d
    DEPENDS NASM
    GIT_REPOSITORY "https://code.videolan.org/videolan/dav1d.git"
    GIT_TAG ${dav1d_GIT_TAG}
    GIT_SHALLOW 1
    CONFIGURE_COMMAND ${dav1d_CONFIGURE}
    BUILD_COMMAND ${dav1d_BUILD}
    INSTALL_COMMAND ${dav1d_INSTALL}
    BUILD_IN_SOURCE 1
)

