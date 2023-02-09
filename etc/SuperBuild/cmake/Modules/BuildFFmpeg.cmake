include(ExternalProject)

set(FFmpeg_DEPS ZLIB NASM)

set(FFmpeg_SHARED_LIBS ON)
set(FFmpeg_DEBUG OFF)

if(WIN32)
    # See the directions for building FFmpeg on Windows in "docs/build_windows.html".
else()
    set(FFmpeg_CONFIGURE)
    set(FFmpeg_CFLAGS)
    set(FFmpeg_CXXFLAGS)
    set(FFmpeg_OBJCFLAGS)
    set(FFmpeg_LDFLAGS)
    
    
    find_package( ZLIB REQUIRED)
    list(APPEND FFmpeg_LDFLAGS
        --extra-ldflags="${ZLIB_LIBRARIES}")
    
    if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
        list(APPEND FFmpeg_CFLAGS "--extra-cflags=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
        list(APPEND FFmpeg_CXXFLAGS "--extra-cxxflags=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
        list(APPEND FFmpeg_OBJCFLAGS "--extra-objcflags=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
        list(APPEND FFmpeg_LDFLAGS "--extra-ldflags=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    endif()
    if(TLRENDER_VPX)
        list(APPEND FFmpeg_CONFIGURE_ARGS
            --enable-libvpx)
        #
        # Makre sure we pick the static libvpx we compiled, not the system one
        #
        list(APPEND FFmpeg_LDFLAGS
            --extra-ldflags="${CMAKE_PREFIX_PATH}/lib/libvpx.a")
        list(APPEND FFmpeg_DEPS VPX)
    endif()
    if(FFmpeg_DEBUG)
        list(APPEND FFmpeg_CFLAGS "--extra-cflags=-g")
        list(APPEND FFmpeg_CXXFLAGS "--extra-cxxflags=-g")
        list(APPEND FFmpeg_OBJCFLAGS "--extra-objcflags=-g")
        list(APPEND FFmpeg_LDFLAGS "--extra-ldflags=-g")
    endif()
    set(FFmpeg_CONFIGURE_ARGS
        --prefix=${CMAKE_INSTALL_PREFIX}
        --disable-programs
        --disable-bzlib
        --disable-iconv
        --disable-lzma
        --disable-appkit
        --disable-avfoundation
        --disable-coreimage
        --disable-audiotoolbox
        --disable-vaapi
        --disable-sdl2
        --enable-pic
        --enable-zlib
        ${FFmpeg_CFLAGS}
        ${FFmpeg_CXXFLAGS}
        ${FFmpeg_OBJCFLAGS}
        ${FFmpeg_LDFLAGS}
        --x86asmexe=${CMAKE_INSTALL_PREFIX}/bin/nasm)
<<<<<<< HEAD
    if (APPLE AND CMAKE_OSX_ARCHITECTURES )
        list(APPEND FFmpeg_CONFIGURE_ARGS --arch=${CMAKE_OSX_ARCHITECTURES} )
=======
    if (APPLE AND CMAKE_OSX_ARCHITECTURES)
        list(APPEND FFmpeg_CONFIGURE_ARGS
            --arch=${CMAKE_OSX_ARCHITECTURES})
>>>>>>> vpx
    endif()
    if(UNIX)
        list(APPEND FFmpeg_CONFIGURE_ARGS
            --disable-libxcb
            --disable-libxcb-shm
            --disable-libxcb-xfixes
            --disable-libxcb-shape
            --disable-xlib)
    endif()
    if(FFmpeg_SHARED_LIBS)
        list(APPEND FFmpeg_CONFIGURE_ARGS
            --disable-static
            --enable-shared)
    endif()
    if(FFmpeg_DEBUG)
        list(APPEND FFmpeg_CONFIGURE_ARGS
            --disable-optimizations
            --disable-stripping
            --enable-debug=3
            --assert-level=2)
    endif()
    ExternalProject_Add(
        FFmpeg
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/FFmpeg
        DEPENDS ${FFmpeg_DEPS}
        URL https://ffmpeg.org/releases/ffmpeg-5.1.2.tar.bz2
        CONFIGURE_COMMAND ./configure ${FFmpeg_CONFIGURE_ARGS}
        BUILD_IN_SOURCE 1)
endif()

