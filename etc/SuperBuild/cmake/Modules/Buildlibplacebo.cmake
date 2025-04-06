include(ExternalProject)


set(libplacebo_GIT_REPO "https://code.videolan.org/videolan/libplacebo.git")
set(libplacebo_GIT_TAG v7.349.0)

set(libplacebo_DEPS ${PYTHON_DEP})


if(NOT BUILD_PYTHON)
    find_program(MESON_EXECUTABLE NAMES meson meson.exe)
    if(NOT MESON_EXECUTABLE)
	message(FATAL_ERROR "Meson build system not found!")
    endif()
else()
    set(MESON_EXECUTABLE ${CMAKE_INSTALL_PREFIX}/bin/meson)
endif()

if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    set(libplacebo_CFLAGS -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
    set(libplacebo_CXXFLAGS -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
    set(libplacebo_LDFLAGS -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
endif()

set(CLANG_ENV )
if(WIN32)
    set(CLANG_ENV CC=clang CXX=clang)
endif()

set (libplacebo_USE_WRAPPER )
if (APPLE)
    # Define the wrapper path
    set(PYTHON_WRAPPER "${CMAKE_BINARY_DIR}/python3.11-wrapper.sh")

    # Create the wrapper script file
    file(WRITE "${PYTHON_WRAPPER}" "#!/bin/bash\n")
    file(APPEND "${PYTHON_WRAPPER}" "export DYLD_LIBRARY_PATH=\"${CMAKE_INSTALL_PREFIX}/lib:\$DYLD_LIBRARY_PATH\"\n")
    file(APPEND "${PYTHON_WRAPPER}" "exec ${Python_EXECUTABLE} \"\$@\"\n")

    # Make it executable
    file(COPY "${PYTHON_WRAPPER}" DESTINATION "${CMAKE_BINARY_DIR}")
    file(CALL "chmod +x ${PYTHON_WRAPPER}")
    
    set(libplacebo_USE_WRAPPER -Dpython=${PYTHON_WRAPPER})
elseif(UNIX)
    set(libplacebo_LDFLAGS -lstdc++)  # \@bug: in Rocky Linux 8.10+
endif()

set(libplacebo_CONFIGURE
    COMMAND git submodule update --init
    COMMAND ${CMAKE_COMMAND} -E env
    ${CLANG_ENV}
    "DYLD_LIBRARY_PATH=${CMAKE_INSTALL_PREFIX}/lib"
    "CXXFLAGS=${libplacebo_CXXFLAGS}"
    "CFLAGS=${libplacebo_CFLAGS}"
    "LDFLAGS=${libplacebo_LDFLAGS}" --
    ${MESON_EXECUTABLE} setup
    --wipe
    -Dvulkan=disabled
    -Ddemos=false
    -Dshaderc=disabled
    -Dlcms=disabled
    -Dglslang=disabled
    -Dlibdir=${CMAKE_INSTALL_PREFIX}/lib
    --prefix=${CMAKE_INSTALL_PREFIX}
    ${libplacebo_USE_WRAPPER}
    build)

set(libplacebo_BUILD
    cd build && ninja)

set(libplacebo_INSTALL
    cd build && inja install)

set(libplacebo_PATCH)


ExternalProject_Add(
    libplacebo
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libplacebo
    GIT_REPOSITORY ${libplacebo_GIT_REPO}
    GIT_TAG ${libplacebo_GIT_TAG}
    DEPENDS ${libplacebo_DEPS}
    CONFIGURE_COMMAND ${libplacebo_CONFIGURE}
    PATCH_COMMAND ${libplacebo_PATCH}
    BUILD_COMMAND ${libplacebo_BUILD}
    INSTALL_COMMAND ${libplacebo_INSTALL}
    BUILD_IN_SOURCE 1
)

