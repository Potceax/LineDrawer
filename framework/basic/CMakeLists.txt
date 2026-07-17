add_library(basic)

# Use cpp sandard selected for project
set_property(TARGET basic PROPERTY CXX_STANDARD 17)

# Standard Include/Source path variables
set(BASIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/basic)
set(BASIC_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Remember to add the include path 
target_include_directories(basic PUBLIC ${BASIC_INCLUDE_DIR})

# set define paths for data files (this has to be absolute)
set(BASIC_DATA_PATH ${CMAKE_SOURCE_DIR}/data)
set(BASIC_TEXTURE_PATH ${BASIC_DATA_PATH}/texture)
set(BASIC_MESH_PATH ${BASIC_DATA_PATH}/mesh)
set(BASIC_SHADER_PATH ${BASIC_DATA_PATH}/shader)

# and for install
set(BASIC_INSTALL_DATA_PATH /data)
set(BASIC_INSTALL_TEXTURE_PATH ${BASIC_INSTALL_DATA_PATH}/texture)
set(BASIC_INSTALL_MESH_PATH ${BASIC_INSTALL_DATA_PATH}/mesh)
set(BASIC_INSTALL_SHADER_PATH ${BASIC_INSTALL_DATA_PATH}/shader)

target_compile_definitions(basic 
    PUBLIC
        DATA_PATH="${BASIC_DATA_PATH}/"
        TEXTURE_PATH="${BASIC_TEXTURE_PATH}/"
        MESH_PATH="${BASIC_MESH_PATH}/"
        SHADER_PATH="${BASIC_SHADER_PATH}/"

        INSTALL_DATA_PATH="${BASIC_INSTALL_DATA_PATH}/"
        INSTALL_TEXTURE_PATH="${BASIC_INSTALL_TEXTURE_PATH}/"
        INSTALL_MESH_PATH="${BASIC_INSTALL_MESH_PATH}/"
        INSTALL_SHADER_PATH="${BASIC_INSTALL_SHADER_PATH}/"
)

# find package for current lib ussage
find_package(glm REQUIRED)

# use something like this to link between libraries  
# for "made yourself libs" with outside target include and lib (look at framework CML) link will work out of the box
# with external libs you have to find its package and then link it
target_link_libraries(basic PRIVATE glloadLib glm)

target_sources(basic
    PRIVATE
        ${BASIC_SOURCE_DIR}/Basic.cpp
    PUBLIC
        FILE_SET HEADERS
        FILES 
            ${BASIC_INCLUDE_DIR}/Basic.h
)