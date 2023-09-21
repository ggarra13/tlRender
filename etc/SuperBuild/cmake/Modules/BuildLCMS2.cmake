
if(WIN32)
else()

    set(LCMS2_BUILD_ARGS
	--enable-shared
	--disable-static
	--prefix=${CMAKE_INSTALL_PREFIX}
	--without-pthreads
	)
    
    set(LCMS2_BUILD_COMMAND ./configure ${LCMS2_BUILD_ARGS})

    ExternalProject_Add(
        LCMS2
	GIT_REPOSITORY "https://github.com/mm2/Little-CMS.git"
	GIT_TAG lcms2.15
	GIT_PROGRESS 1
	CONFIGURE_COMMAND ${LCMS2_BUILD_COMMAND}
	BUILD_IN_SOURCE 1)
    
endif()
