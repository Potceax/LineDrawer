cmake_minimum_required(VERSION 3.25)
project(lineDrawer)

add_subdirectory(framework)
add_subdirectory(lineDrawer)

include(GNUInstallDirs)

install(
    TARGETS lineDrawer frameworkLib
    FILE_SET HEADERS
)

# set the shaders as the resource (for install)
set(SHADERS_PATH ${PROJECT_SOURCE_DIR}/data/shader)
install(
    FILES 
        ${SHADERS_PATH}/line.geom
        ${SHADERS_PATH}/line.frag
        ${SHADERS_PATH}/line.vert
    DESTINATION bin/data/shader
)

# installs all needed dlls for target executable
install(FILES $<TARGET_RUNTIME_DLLS:lineDrawer> TYPE BIN)

