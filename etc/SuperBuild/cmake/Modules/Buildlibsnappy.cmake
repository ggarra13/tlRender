include(ExternalProject)


set(libsnappy_REPO "https://github.com/google/snappy.git")
set(libsnappy_TAG 1.2.1)


set(libsnappy_ARGS ${TLRENDER_EXTERNAL_ARGS})

list(APPEND libsnappy_ARGS
    -DBENCHMARK_ENABLE_INSTALL=OFF
    -DBENCHMARK_INSTALL_DOCS=OFF
    -DBENCHMARK_USE_BUNDLED_GTEST=OFF
)

set(libsnappy_UPDATE_CMD git submodule update --init)


ExternalProject_Add(
    libsnappy
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libsnappy
    GIT_REPOSITORY ${libsnappy_REPO}
    GIT_TAG ${libsnappy_TAG}
    GIT_SHALLOW 1

    UPDATE_COMMAND ${libsnappy_UPDATE_CMD}
    
    LIST_SEPARATOR |
    CMAKE_ARGS ${libsnappy_ARGS}
)

