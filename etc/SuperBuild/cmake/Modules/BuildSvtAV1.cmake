include(ExternalProject)

include(GNUInstallDirs)

#set(SvtAV1_TAG v3.0.0)  # not ocmpatible with FFmpeg 7.0.1.
set(SvtAV1_TAG v3.0.2)   # was v2.3.0

set(SvtAV1_ARGS ${TLRENDER_EXTERNAL_ARGS})
list(APPEND SvtAV1_ARGS
    -DENABLE_NASM=ON
    -DCMAKE_INSTALL_LIBDIR=lib
)

set(SvtAV1_DEPS )
if(NOT WIN32)
    set(SvtAV1_DEPS NASM)
else()
    # Build SvtAV1 with MSYS2 on Windows.
    find_package(Msys REQUIRED)
    set(SvtAV1_MSYS2
        ${MSYS_CMD}
        -use-full-path
        -defterm
        -no-start
        -here)
endif()

ExternalProject_Add(
    SvtAV1
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/SvtAV1
    DEPENDS ${SvtAV1_DEPS}
    GIT_REPOSITORY "https://gitlab.com/AOMediaCodec/SVT-AV1.git"
    GIT_TAG ${SvtAV1_TAG}
    
    LIST_SEPARATOR |
    CMAKE_ARGS ${SvtAV1_ARGS}
)
