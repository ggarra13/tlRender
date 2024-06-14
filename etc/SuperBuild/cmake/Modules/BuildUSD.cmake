
include(ExternalProject)


if(NOT DEFINED PYTHON_EXECUTABLE)
  if(WIN32)
      set(PYTHON_EXECUTABLE python)
  else()
      set(PYTHON_EXECUTABLE python3)
  endif()
endif()


message(STATUS "USD will be built with ${PYTHON_EXECUTABLE}")


set(USD_DEPS ${PYTHON_DEP})

set(USD_GIT_REPOSITORY https://github.com/PixarAnimationStudios/OpenUSD.git)
set(USD_GIT_TAG v24.05)

string(TOLOWER ${CMAKE_BUILD_TYPE} cmake_build_type)

set(USD_ARGS --build-variant ${cmake_build_type})
if(CMAKE_OSX_ARCHITECTURES)
    list(APPEND USD_ARGS --build-target ${CMAKE_OSX_ARCHITECTURES})
endif()
if(CMAKE_OSX_DEPLOYMENT_TARGET)
    list(APPEND USD_ARGS --build-args)
    list(APPEND USD_ARGS USD,"-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    list(APPEND USD_ARGS OpenSubdiv,"-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    list(APPEND USD_ARGS MaterialX,"-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    #list(APPEND USD_ARGS TBB,"CFLAGS=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} CXXFLAGS=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
endif()

list(APPEND USD_ARGS --no-python --no-examples --no-tutorials --no-tools)
list(APPEND USD_ARGS --generator Ninja --verbose)

set(USD_INSTALL_COMMAND )
if(WIN32)
    # \todo On Windows the USD cmake build system installs the "*.dll" files
    # and "usd" directory into "lib", however it seems like they need to be
    # in "bin" instead.
    cmake_path(CONVERT ${CMAKE_INSTALL_PREFIX} TO_NATIVE_PATH_LIST cmake_install)
    set(USD_INSTALL_COMMAND
        ${CMAKE_COMMAND} -E copy_directory ${CMAKE_INSTALL_PREFIX}/lib/usd  ${CMAKE_INSTALL_PREFIX}/bin/usd
        COMMAND copy "${cmake_install}\\lib\\*.dll" "${cmake_install}\\bin")

endif()

ExternalProject_Add(
    USD
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/USD
    DEPENDS ${USD_DEPS}
    GIT_REPOSITORY ${USD_GIT_REPOSITORY}
    GIT_TAG ${USD_GIT_TAG}
    GIT_SHALLOW 1
    CONFIGURE_COMMAND ""
    PATCH_COMMAND ${USD_PATCH_COMMAND}
    BUILD_COMMAND ${PYTHON_EXECUTABLE} build_scripts/build_usd.py ${CMAKE_INSTALL_PREFIX} ${USD_ARGS} 
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND "${USD_INSTALL_COMMAND}")
