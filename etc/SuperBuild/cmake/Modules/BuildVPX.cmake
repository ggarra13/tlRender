include(ExternalProject)


# set(LIBVPX_TAG main) # live on the cutting-edge!

set(VPX_TAG v1.12.0) # proven to work


if(WIN32 OR NOT TLRENDER_FFMPEG)
     # Compiled with MSys scripts
else()

    set(VPX_CFLAGS)
    set(VPX_CXXFLAGS)
    set(VPX_OBJCFLAGS)
    set(VPX_LDFLAGS)

    if(APPLE)
        set(VPX_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        set(VPX_C_FLAGS "${CMAKE_C_FLAGS}")
	if (CMAKE_OSX_DEPLOYMENT_TARGET)
            list(APPEND VPX_CXX_FLAGS "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
            list(APPEND VPX_C_FLAGS "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
	endif()
    endif()

    set(VPX_CONFIGURE_ARGS
        --prefix=${CMAKE_INSTALL_PREFIX}
        --enable-pic
        --disable-examples
        --disable-tools
        --disable-docs
        --disable-unit-tests
        --enable-vp9-highbitdepth
        --extra-cflags=${VPX_C_FLAGS}
        --extra-cxxflags=${VPX_CXX_FLAGS}
    )


    if(TLRENDER_YASM)
	list(APPEND VPX_CONFIGURE_ARGS --as=yasm)
    else()
	list(APPEND VPX_CONFIGURE_ARGS --as=nasm)
    endif()

    
    set(YASM_BIN_PATH "$ENV{PATH}")
    
    ExternalProject_Add(
        VPX
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/VPX
        DEPENDS ${TLRENDER_YASM_DEP} NASM
        GIT_REPOSITORY "https://github.com/webmproject/libvpx.git"
        GIT_TAG ${VPX_TAG}
	GIT_SHALLOW 1
	CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env PATH=${YASM_BIN_PATH} -- ./configure ${VPX_CONFIGURE_ARGS}
	BUILD_COMMAND ${CMAKE_COMMAND} -E env PATH=${YASM_BIN_PATH} -- make
        BUILD_IN_SOURCE 1
    )

endif()
