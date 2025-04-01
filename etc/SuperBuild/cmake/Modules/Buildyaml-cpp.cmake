include(ExternalProject)

set(yaml-cpp_GIT_REPOSITORY "https://github.com/jbeder/yaml-cpp.git")
set(yaml-cpp_GIT_TAG 28f93bdec6387d42332220afa9558060c8016795)

set(yaml-cpp_ARGS
    -DYAML_CPP_BUILD_CONTRIB=OFF
    -DYAML_CPP_BUILD_TOOLS=OFF
    -DYAML_CPP_BUILD_TESTS=OFF
    ${TLRENDER_EXTERNAL_ARGS})

ExternalProject_Add(
    yaml-cpp
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/yaml-cpp
    GIT_REPOSITORY ${yaml-cpp_GIT_REPOSITORY}
    GIT_TAG ${yaml-cpp_GIT_TAG}
    
    LIST_SEPARATOR |
    CMAKE_ARGS ${yaml-cpp_ARGS})
