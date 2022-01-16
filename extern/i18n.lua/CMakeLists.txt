if (NOT DEFINED OPENMW_RESOURCES_ROOT)
    return()
endif()

# Copy resource files into the build directory
set(SDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDIRRELATIVE resources/lua_libs/i18n)

set(I18N_LUA_FILES
    i18n/init.lua
    i18n/interpolate.lua
    i18n/plural.lua
    i18n/variants.lua
    i18n/version.lua
)

copy_all_resource_files(${CMAKE_CURRENT_SOURCE_DIR} ${OPENMW_RESOURCES_ROOT} ${DDIRRELATIVE} "${I18N_LUA_FILES}")
