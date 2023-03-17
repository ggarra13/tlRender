include(ExternalProject)

set(RtAudio_GIT_REPOSITORY "https://github.com/thestk/rtaudio.git")

#set(RtAudio_GIT_TAG "d7f12763c55795ef8a71a9b589b39e7be01db7b2") # 2020/06/07
set(RtAudio_GIT_TAG  "268e64a7b5e4b72de7668be6945c497693839d8f") # 2023/02/07

set(RtAudio_ARGS
    ${TLRENDER_EXTERNAL_ARGS}
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_DEBUG_POSTFIX=
    -DRTAUDIO_BUILD_TESTING=FALSE
    -DRTAUDIO_STATIC_MSVCRT=FALSE)
if(BUILD_SHARED_LIBS)
    list(APPEND RtAudio_ARGS -DRTAUDIO_BUILD_SHARED_LIBS=TRUE)
else()
    list(APPEND RtAudio_ARGS -DRTAUDIO_BUILD_STATIC_LIBS=TRUE)
endif()
if(APPLE)
    list(APPEND RtAudio_ARGS RTAUDIO_API_JACK=OFF)
endif()

ExternalProject_Add(
    RtAudio
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/RtAudio
    GIT_REPOSITORY ${RtAudio_GIT_REPOSITORY}
    GIT_TAG ${RtAudio_GIT_TAG}
    PATCH_COMMAND ${RtAudio_PATCH}
    LIST_SEPARATOR |
    CMAKE_ARGS ${RtAudio_ARGS})
