include(ExternalProject)

include(GNUInstallDirs)

set(SvtAV1_TAG v2.1.2)
set(SvtAV1_ARGS ${TLRENDER_EXTERNAL_ARGS})

list(APPEND SvtAV1_ARGS
    -DENABLE_NASM=ON
    -DCMAKE_INSTALL_LIBDIR=lib
)

set(SvtAV1_DEPS )
if(UNIX)
    set(SvtAV1_DEPS NASM)
endif()


ExternalProject_Add(
    SvtAV1
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/SvtAV1
    DEPENDS ${SvtAV1_DEPS}
    GIT_REPOSITORY "https://gitlab.com/AOMediaCodec/SVT-AV1.git"
    GIT_TAG ${SvtAV1_TAG}
    GIT_SHALLOW 1
    LIST_SEPARATOR |
	CMAKE_ARGS ${SvtAV1_ARGS}
    )

