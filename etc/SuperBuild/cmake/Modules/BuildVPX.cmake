include(ExternalProject)


# set(LIBVPX_TAG main) # live on the cutting-edge!

set(VPX_TAG v1.12.0) # proven to work


if(WIN32 OR NOT TLRENDER_FFMPEG)
    # Use media_autobuild-suite to build FFmpeg with VPX support on Windows
else()

    set(VPX_CFLAGS)
    set(VPX_CXXFLAGS)
    set(VPX_OBJCFLAGS)
    set(VPX_LDFLAGS)

    if(APPLE)
        set(VPX_CXX_FLAGS "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} ${CMAKE_CXX_FLAGS}")
        set(VPX_C_FLAGS "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} ${CMAKE_C_FLAGS}")
    endif()

    set(VPX_CONFIGURE_ARGS
        --prefix=${CMAKE_INSTALL_PREFIX}
        --enable-pic
	--as=yasm
        --disable-examples
        --disable-tools
        --disable-docs
        --disable-unit-tests
        "CFLAGS=${VPX_C_FLAGS}"
        "CXXFLAGS=${VPX_CXX_FLAGS}"
    )

    set( YASM_BIN_PATH $ENV{PATH} )
    
    ExternalProject_Add(
        VPX
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/VPX
        DEPENDS ${TLRENDER_YASM_DEP} NASM
        GIT_REPOSITORY "https://github.com/webmproject/libvpx.git"
        GIT_TAG ${VPX_TAG}
        CONFIGURE_COMMAND PATH=${YASM_BIN_PATH} ./configure ${VPX_CONFIGURE_ARGS}
        BUILD_COMMAND PATH=${YASM_BIN_PATH} make
        BUILD_IN_SOURCE 1
    )

endif()
