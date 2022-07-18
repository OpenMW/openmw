add_subdirectory(shaders)
add_subdirectory(data)
add_subdirectory(lua_api)

copy_resource_file("launcher/images/openmw.png" "${OPENMW_RESOURCES_ROOT}" "resources/openmw.png")
