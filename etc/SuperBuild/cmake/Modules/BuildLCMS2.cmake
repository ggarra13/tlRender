
if(WIN32)
else()

    if(APPLE)
        set(LCMS2_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        set(LCMS2_C_FLAGS "${CMAKE_C_FLAGS}")
	if (CMAKE_OSX_DEPLOYMENT_TARGET)
            list(APPEND LCMS2_CXX_FLAGS "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
            list(APPEND LCMS2_C_FLAGS "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
	endif()
    endif()

    set(LCMS2_BUILD_ARGS
	--enable-shared
	--disable-static
	--prefix=${CMAKE_INSTALL_PREFIX}
        "CFLAGS=${LCMS2_C_FLAGS}"
        "CXXFLAGS=${LCMS2_CXX_FLAGS}"
	)
    
    set(LCMS2_BUILD_COMMAND ./configure ${LCMS2_BUILD_ARGS})

    ExternalProject_Add(
        LCMS2
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/LCMS2
	GIT_REPOSITORY "https://github.com/mm2/Little-CMS.git"
	GIT_TAG lcms2.15
	GIT_SHALLOW 1
	GIT_PROGRESS 1
	CONFIGURE_COMMAND ${LCMS2_BUILD_COMMAND}
	BUILD_IN_SOURCE 1)
    
endif()
