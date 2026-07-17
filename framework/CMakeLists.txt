add_library(frameworkLib)

set(glload_PATH ${CMAKE_CURRENT_SOURCE_DIR}/ext/glload)
set(glm_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/GLM/lib/cmake/glm)
set(FreeGLUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/freeglut/lib/cmake/FreeGLUT)

set_property(TARGET frameworkLib PROPERTY CXX_STANDARD 17)

add_subdirectory(basic)

add_library(glloadLib INTERFACE)
target_include_directories(glloadLib INTERFACE ${glload_PATH}/include)
target_link_directories(glloadLib INTERFACE ${glload_PATH}/lib)
target_link_libraries(glloadLib INTERFACE glload)

find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)
find_package(FreeGLUT REQUIRED)

target_sources(frameworkLib
    PRIVATE
        framework.cpp
    PUBLIC
        FILE_SET HEADERS
        FILES framework.h
)

target_link_libraries(frameworkLib
    PUBLIC
        OpenGL::GL
        glloadLib
        glm
        FreeGLUT::freeglut
        basic
)