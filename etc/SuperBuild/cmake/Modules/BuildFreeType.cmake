include(ExternalProject)

set(FreeType_ARGS
    ${TLR_EXTERNAL_ARGS}
    -DCMAKE_INSTALL_LIBDIR=lib
    -DFT_WITH_ZLIB=ON
    -DCMAKE_DISABLE_FIND_PACKAGE_BZip2=TRUE
    -DCMAKE_DISABLE_FIND_PACKAGE_PNG=TRUE
    -DCMAKE_DISABLE_FIND_PACKAGE_HarfBuzz=TRUE
    -DCMAKE_DISABLE_FIND_PACKAGE_BrotliDec=TRUE)

ExternalProject_Add(
    FreeType
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/FreeType
    DEPENDS ZLIB
    GIT_REPOSITORY https://github.com/freetype/freetype
    GIT_TAG VER-2-10-4
    LIST_SEPARATOR |
    CMAKE_ARGS ${FreeType_ARGS})
