include(ExternalProject)

include(GNUInstallDirs)

set(SvtAV1_TAG v1.8.0)
set(SvtAV1_ARGS ${TLRENDER_EXTERNAL_ARGS})

list(APPEND SvtAV1_ARGS
    -DENABLE_NASM=ON
    -DCMAKE_INSTALL_LIBDIR=lib
    )


ExternalProject_Add(
    SvtAV1
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/SvtAV1
    DEPENDS NASM
    GIT_REPOSITORY "https://gitlab.com/AOMediaCodec/SVT-AV1.git"
    GIT_TAG ${SvtAV1_TAG}
    LIST_SEPARATOR |
	CMAKE_ARGS ${SvtAV1_ARGS}
    )

