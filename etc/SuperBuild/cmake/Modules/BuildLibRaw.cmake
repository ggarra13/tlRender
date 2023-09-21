include(ExternalProject)


find_package(LCMS2 REQUIRED)

set(LibRaw_URL "https://www.libraw.org/data/LibRaw-0.21.1.tar.gz")


if(WIN32)
    set(LibRaw_CFLAGS)
    set(LibRaw_CXXFLAGS)
    set(LibRaw_OBJCFLAGS)
    set(LibRaw_LDFLAGS)
    
    set(LibRaw_PATCH "")
    set(LibRaw_BUILD nmake -f Makefile.msvc)
else()
    set(LibRaw_CFLAGS)
    set(LibRaw_CXXFLAGS)
    set(LibRaw_OBJCFLAGS)
    set(LibRaw_LDFLAGS)

    set(LibRaw_CONFIGURE_ARGS
        --prefix=${CMAKE_INSTALL_PREFIX}
	--enable-lcms2
	--enable-zlib
	--enable-jpeg
	--enable-static
	--disable-shared
	--disable-examples
    )
    set(LibRaw_PATCH "")
    set(LibRaw_CONFIGURE ./configure ${LibRaw_CONFIGURE_ARGS})
    set(LibRaw_BUILD make -j 4)
    set(LibRaw_INSTALL make -j 4 install)
endif()
    
ExternalProject_Add(
    LibRaw
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/LibRaw
    URL ${LibRaw_URL}
    DEPENDS ZLIB libjpeg-turbo
    PATCH_COMMAND ${LibRaw_PATCH}
    CONFIGURE_COMMAND ${LibRaw_CONFIGURE}
    BUILD_COMMAND ${LibRaw_BUILD}
    INSTALL_COMMAND ${LibRaw_INSTALL}
    BUILD_IN_SOURCE 1
)
