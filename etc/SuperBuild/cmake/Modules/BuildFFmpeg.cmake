include(ExternalProject)


set(FFmpeg_DEPS)
if(WIN32)
     # Compiled with MSYS script as a pre-flight step.
elseif(APPLE)
    list(APPEND FFmpeg_LDFLAGS "--extra-ldflags=-L${CMAKE_INSTALL_PREFIX}/lib")
else()
    set(FFmpeg_CFLAGS)
    set(FFmpeg_CXXFLAGS)
    set(FFmpeg_OBJCFLAGS)
    set(FFmpeg_LDFLAGS)
    
    if(TLRENDER_NET)
	list(APPEND FFmpeg_DEPS OpenSSL)
    endif()

    if(TLRENDER_VPX)
	list(APPEND FFmpeg_LDFLAGS
	    --extra-ldflags="-L${CMAKE_INSTALL_PREFIX}/lib/"
	    --extra-ldflags="${CMAKE_INSTALL_PREFIX}/lib/libvpx.a")
	list(APPEND FFmpeg_DEPS VPX)
    endif()
    if(TLRENDER_X264)
	#
	# Make sure we pick the static libx264 we compiled, not the system one
	#
	list(APPEND FFmpeg_LDFLAGS
	    --extra-ldflags="-L${CMAKE_INSTALL_PREFIX}/lib/"
	    --extra-ldflags="${CMAKE_INSTALL_PREFIX}/lib/libx264.a")
	list(APPEND FFmpeg_DEPS X264)
    endif()
    
    list(APPEND FFmpeg_DEPS NASM)

    set(FFmpeg_SHARED_LIBS ON)
    set(FFmpeg_DEBUG OFF)
    set(FFmpeg_CFLAGS "--extra-cflags=-I${CMAKE_INSTALL_PREFIX}/include")
    set(FFmpeg_CXXFLAGS "--extra-cxxflags=-I${CMAKE_INSTALL_PREFIX}/include")
    set(FFmpeg_OBJCFLAGS "--extra-objcflags=-I${CMAKE_INSTALL_PREFIX}/include")

    list(APPEND FFmpeg_LDFLAGS "--extra-ldflags=-L${CMAKE_INSTALL_PREFIX}/lib")
    list(APPEND FFmpeg_LDFLAGS "--extra-ldflags=-L${CMAKE_INSTALL_PREFIX}/lib64")

    if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
	list(APPEND FFmpeg_CFLAGS "--extra-cflags=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
	list(APPEND FFmpeg_CXXFLAGS "--extra-cxxflags=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
	list(APPEND FFmpeg_OBJCFLAGS "--extra-objcflags=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
	list(APPEND FFmpeg_LDFLAGS "--extra-ldflags=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
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
	--disable-doc
	--disable-postproc
	--disable-avfilter
	--disable-hwaccels
	--disable-devices
	--disable-filters
	--disable-alsa
	--disable-appkit
	--disable-avfoundation
	--disable-bzlib
	--disable-coreimage
	--disable-iconv
	--disable-libxcb
	--disable-libxcb-shm
	--disable-libxcb-xfixes
	--disable-libxcb-shape
	--disable-lzma
	--disable-metal
	--disable-sndio
	--disable-schannel
	--disable-sdl2
	--disable-securetransport
	--disable-vulkan
	--disable-xlib
	--disable-zlib
	--disable-amf
	--disable-audiotoolbox
	--disable-cuda-llvm
	--disable-cuvid
	--disable-d3d11va
	--disable-dxva2
	--disable-ffnvcodec
	--disable-nvdec
	--disable-nvenc
	--disable-v4l2-m2m
	--disable-vaapi
	--disable-vdpau
	--disable-videotoolbox
	--enable-pic
	${FFmpeg_CFLAGS}
	${FFmpeg_CXXFLAGS}
	${FFmpeg_OBJCFLAGS}
	${FFmpeg_LDFLAGS})
    if(TLRENDER_FFMPEG_MINIMAL)
	list(APPEND FFmpeg_CONFIGURE_ARGS
            --disable-decoders
            --enable-decoder=aac
            --enable-decoder=ac3
            --enable-decoder=av1
            --enable-decoder=ayuv
            --enable-decoder=dnxhd
            --enable-decoder=eac3
            --enable-decoder=flac
            --enable-decoder=h264
            --enable-decoder=hevc
            --enable-decoder=mjpeg
            --enable-decoder=mp3
            --enable-decoder=mpeg2video
            --enable-decoder=mpeg4
            --enable-decoder=pcm_alaw
            --enable-decoder=pcm_alaw_at
            --enable-decoder=pcm_bluray
            --enable-decoder=pcm_dvd
            --enable-decoder=pcm_f16le
            --enable-decoder=pcm_f24le
            --enable-decoder=pcm_f32be
            --enable-decoder=pcm_f32le
            --enable-decoder=pcm_f64be
            --enable-decoder=pcm_f64le
            --enable-decoder=pcm_lxf
            --enable-decoder=pcm_mulaw
            --enable-decoder=pcm_mulaw_at
            --enable-decoder=pcm_s16be
            --enable-decoder=pcm_s16be_planar
            --enable-decoder=pcm_s16le
            --enable-decoder=pcm_s16le_planar
            --enable-decoder=pcm_s24be
            --enable-decoder=pcm_s24daud
            --enable-decoder=pcm_s24le
            --enable-decoder=pcm_s24le_planar
            --enable-decoder=pcm_s32be
            --enable-decoder=pcm_s32le
            --enable-decoder=pcm_s32le_planar
            --enable-decoder=pcm_s64be
            --enable-decoder=pcm_s64le
            --enable-decoder=pcm_s8
            --enable-decoder=pcm_s8_planar
            --enable-decoder=pcm_sga
            --enable-decoder=pcm_u16be
            --enable-decoder=pcm_u16le
            --enable-decoder=pcm_u24be
            --enable-decoder=pcm_u24le
            --enable-decoder=pcm_u32be
            --enable-decoder=pcm_u32le
            --enable-decoder=pcm_u8
            --enable-decoder=pcm_vidc
            --enable-decoder=prores
            --enable-decoder=rawvideo
            --enable-decoder=v210
            --enable-decoder=v210x
            --enable-decoder=v308
            --enable-decoder=v408
            --enable-decoder=v410
            --enable-decoder=vp9
            --enable-decoder=yuv4
            --disable-encoders
            --enable-encoder=aac
            --enable-encoder=ac3
            --enable-encoder=ayuv
            --enable-encoder=dnxhd
            --enable-encoder=eac3
            --enable-encoder=mjpeg
            --enable-encoder=mpeg2video
            --enable-encoder=mpeg4
            --enable-encoder=pcm_alaw
            --enable-encoder=pcm_alaw_at
            --enable-encoder=pcm_bluray
            --enable-encoder=pcm_dvd
            --enable-encoder=pcm_f32be
            --enable-encoder=pcm_f32le
            --enable-encoder=pcm_f64be
            --enable-encoder=pcm_f64le
            --enable-encoder=pcm_mulaw
            --enable-encoder=pcm_mulaw_at
            --enable-encoder=pcm_s16be
            --enable-encoder=pcm_s16be_planar
            --enable-encoder=pcm_s16le
            --enable-encoder=pcm_s16le_planar
            --enable-encoder=pcm_s24be
            --enable-encoder=pcm_s24daud
            --enable-encoder=pcm_s24le
            --enable-encoder=pcm_s24le_planar
            --enable-encoder=pcm_s32be
            --enable-encoder=pcm_s32le
            --enable-encoder=pcm_s32le_planar
            --enable-encoder=pcm_s64be
            --enable-encoder=pcm_s64le
            --enable-encoder=pcm_s8
            --enable-encoder=pcm_s8_planar
            --enable-encoder=pcm_u16be
            --enable-encoder=pcm_u16le
            --enable-encoder=pcm_u24be
            --enable-encoder=pcm_u24le
            --enable-encoder=pcm_u32be
            --enable-encoder=pcm_u32le
            --enable-encoder=pcm_u8
            --enable-encoder=pcm_vidc
            --enable-encoder=prores
            --enable-encoder=rawvideo
            --enable-encoder=v210
            --enable-encoder=v308
            --enable-encoder=v408
            --enable-encoder=v410
            --enable-encoder=yuv4
            --disable-demuxers
            --enable-demuxer=aac
            --enable-demuxer=ac3
            --enable-demuxer=aiff
            --enable-demuxer=av1
            --enable-demuxer=dnxhd
            --enable-demuxer=dts
            --enable-demuxer=dtshd
            --enable-demuxer=eac3
            --enable-demuxer=flac
            --enable-demuxer=h264
            --enable-demuxer=hevc
            --enable-demuxer=imf
            --enable-demuxer=m4v
            --enable-demuxer=mjpeg
            --enable-demuxer=mov
            --enable-demuxer=mp3
            --enable-demuxer=mxf
            --enable-demuxer=pcm_alaw
            --enable-demuxer=pcm_f32be
            --enable-demuxer=pcm_f32le
            --enable-demuxer=pcm_f64be
            --enable-demuxer=pcm_f64le
            --enable-demuxer=pcm_mulaw
            --enable-demuxer=pcm_s16be
            --enable-demuxer=pcm_s16le
            --enable-demuxer=pcm_s24be
            --enable-demuxer=pcm_s24le
            --enable-demuxer=pcm_s32be
            --enable-demuxer=pcm_s32le
            --enable-demuxer=pcm_s8
            --enable-demuxer=pcm_u16be
            --enable-demuxer=pcm_u16le
            --enable-demuxer=pcm_u24be
            --enable-demuxer=pcm_u24le
            --enable-demuxer=pcm_u32be
            --enable-demuxer=pcm_u32le
            --enable-demuxer=pcm_u8
            --enable-demuxer=pcm_vidc
            --enable-demuxer=rawvideo
            --enable-demuxer=v210
            --enable-demuxer=v210x
            --enable-demuxer=wav
            --enable-demuxer=yuv4mpegpipe
            --disable-muxers
            --enable-muxer=ac3
            --enable-muxer=aiff
            --enable-muxer=dnxhd
            --enable-muxer=dts
            --enable-muxer=eac3
            --enable-muxer=flac
            --enable-muxer=h264
            --enable-muxer=hevc
            --enable-muxer=m4v
            --enable-muxer=mjpeg
            --enable-muxer=mov
            --enable-muxer=mp4
            --enable-muxer=mpeg2video
            --enable-muxer=mxf
            --enable-muxer=pcm_alaw
            --enable-muxer=pcm_f32be
            --enable-muxer=pcm_f32le
            --enable-muxer=pcm_f64be
            --enable-muxer=pcm_f64le
            --enable-muxer=pcm_mulaw
            --enable-muxer=pcm_s16be
            --enable-muxer=pcm_s16le
            --enable-muxer=pcm_s24be
            --enable-muxer=pcm_s24le
            --enable-muxer=pcm_s32be
            --enable-muxer=pcm_s32le
            --enable-muxer=pcm_s8
            --enable-muxer=pcm_u16be
            --enable-muxer=pcm_u16le
            --enable-muxer=pcm_u24be
            --enable-muxer=pcm_u24le
            --enable-muxer=pcm_u32be
            --enable-muxer=pcm_u32le
            --enable-muxer=pcm_u8
            --enable-muxer=pcm_vidc
            --enable-muxer=rawvideo
            --enable-muxer=wav
            --enable-muxer=yuv4mpegpipe
            --disable-parsers
            --enable-parser=aac
            --enable-parser=ac3
            --enable-parser=av1
            --enable-parser=dnxhd
            --enable-parser=dolby_e
            --enable-parser=flac
            --enable-parser=h264
            --enable-parser=hevc
            --enable-parser=mjpeg
            --enable-parser=mpeg4video
            --enable-parser=mpegaudio
            --enable-parser=mpegvideo
            --enable-parser=vp9
            --disable-protocols
            --enable-protocol=crypto
            --enable-protocol=file
            --enable-protocol=ftp
            --enable-protocol=http
            --enable-protocol=httpproxy
            --enable-protocol=https
            --enable-protocol=md5)
    endif()
    list(APPEND FFmpeg_CONFIGURE_ARGS
	--x86asmexe=${CMAKE_INSTALL_PREFIX}/bin/nasm)
    if(TLRENDER_NET)
	list(APPEND FFmpeg_CONFIGURE_ARGS
            --enable-openssl)
    endif()

    if(TLRENDER_VPX)
	list(APPEND FFmpeg_CONFIGURE_ARGS
	    --enable-libvpx)
    endif()
    if(TLRENDER_X264)
	list(APPEND FFmpeg_CONFIGURE_ARGS
	    --enable-libx264 --enable-gpl)
	if(TLRENDER_NET)
	    list(APPEND FFmpeg_CONFIGURE_ARGS
		--enable-version3)
	endif()
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

    set(FFmpeg_CONFIGURE ./configure ${FFmpeg_CONFIGURE_ARGS})
    set(FFmpeg_BUILD make -j 4)
    set(FFmpeg_INSTALL make install)

    ExternalProject_Add(
	FFmpeg
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/FFmpeg
	DEPENDS ${FFmpeg_DEPS}
	URL https://ffmpeg.org/releases/ffmpeg-6.0.tar.bz2
	CONFIGURE_COMMAND ${FFmpeg_CONFIGURE}
	BUILD_COMMAND ${FFmpeg_BUILD}
	INSTALL_COMMAND ${FFmpeg_INSTALL}
	BUILD_IN_SOURCE 1)
endif()
