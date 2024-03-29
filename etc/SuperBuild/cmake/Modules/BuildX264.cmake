include(ExternalProject)


set(X264_TAG master) # live on the cutting-edge!


if(WIN32 OR NOT TLRENDER_FFMPEG)
     # Compiled with MSys scripts
else()

    set(X264_CFLAGS)
    set(X264_CXXFLAGS)
    set(X264_OBJCFLAGS)
    set(X264_LDFLAGS)

    if(APPLE)
        set(X264_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        set(X264_C_FLAGS "${CMAKE_C_FLAGS}")
	if (CMAKE_OSX_DEPLOYMENT_TARGET)
            list(APPEND X264_CXX_FLAGS "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
            list(APPEND X264_C_FLAGS "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
	endif()
    endif()

    set(X264_CONFIGURE_ARGS
        --prefix=${CMAKE_INSTALL_PREFIX}
        --enable-pic
	--enable-static
	--disable-shared
        "CFLAGS=${X264_C_FLAGS}"
        "CXXFLAGS=${X264_CXX_FLAGS}"
    )

    ExternalProject_Add(
        X264
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/X264
        DEPENDS NASM
        GIT_REPOSITORY "https://code.videolan.org/videolan/x264.git"
        GIT_TAG ${X264_TAG}
	GIT_SHALLOW 1
        CONFIGURE_COMMAND ./configure ${X264_CONFIGURE_ARGS}
        BUILD_IN_SOURCE 1
    )

endif()
