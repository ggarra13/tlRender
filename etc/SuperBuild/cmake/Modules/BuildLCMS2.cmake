
if(WIN32)
else()

    set(LCMS2_BUILD_COMMAND ./configure --enable-static --prefix=${CMAKE_INSTALL_PREFIX})

    ExternalProject_Add(
        LCMS2
	GIT_REPOSITORY "https://github.com/mm2/Little-CMS.git"
	GIT_PROGRESS 1
	CONFIGURE_COMMAND ${LCMS2_BUILD_COMMAND}
	BUILD_IN_SOURCE 1)
    
endif()
