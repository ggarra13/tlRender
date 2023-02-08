include(ExternalProject)


# set(LIBVPX_TAG main) # live on the cutting-edge!

set(VPX_TAG v1.12.0) # ptoven to work


if(WIN32 OR NOT TLRENDER_FFMPEG)
    # Use media_autobuild-suite to build FFmpeg with VPX support on Windows
else()

    set(VPX_CFLAGS)
    set(VPX_CXXFLAGS)
    set(VPX_OBJCFLAGS)
    set(VPX_LDFLAGS)
    if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
        list(APPEND VPX_CFLAGS "--extra-cflags=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
        list(APPEND VPX_CXXFLAGS "--extra-cxxflags=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
        list(APPEND VPX_OBJCFLAGS "--extra-objcflags=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
        list(APPEND VPX_LDFLAGS "--extra-ldflags=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    endif()
    if(VPX_DEBUG)
        list(APPEND VPX_CFLAGS "--extra-cflags=-g")
        list(APPEND VPX_CXXFLAGS "--extra-cxxflags=-g")
        list(APPEND VPX_OBJCFLAGS "--extra-objcflags=-g")
        list(APPEND VPX_LDFLAGS "--extra-ldflags=-g")
    endif()
    
    set(VPX_CONFIGURE_ARGS
        --prefix=${CMAKE_INSTALL_PREFIX}
        ${VPX_CFLAGS}
        ${VPX_CXXFLAGS}
        ${VPX_OBJCFLAGS}
        ${VPX_LDFLAGS}
        )
ExternalProject_Add(
  VPX
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/VPX
  DEPENDS YASM
  GIT_REPOSITORY "https://github.com/webmproject/libvpx.git"
  GIT_TAG ${VPX_TAG}
  CONFIGURE_COMMAND sh ./configure ${VPX_CONFIGURE_ARGS}
  BUILD_IN_SOURCE 1
)

endif()

