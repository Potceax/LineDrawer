add_executable(lineDrawer)

set(LDRAW_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include/lineDrawer)
set(LDRAW_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_property(TARGET lineDrawer PROPERTY CXX_STANDARD 17)

target_include_directories(lineDrawer PRIVATE ${LDRAW_INCLUDE_PATH})

target_sources(lineDrawer
    PRIVATE
        ${LDRAW_SOURCE_PATH}/lineDrawer.cpp
    PUBLIC
        ${LDRAW_INCLUDE_PATH}/lineDrawer.h
)

target_link_libraries(lineDrawer
    PRIVATE
        frameworkLib
)

# copy needed .dlls to exec folder (Yes this works from libraries when target exec is specified in command)
add_custom_command(TARGET lineDrawer 
    PRE_LINK
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/framework/ext/freeglut/bin/freeglutd.dll" $<TARGET_FILE_DIR:lineDrawer>
)