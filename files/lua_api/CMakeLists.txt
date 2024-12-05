if (NOT DEFINED OPENMW_RESOURCES_ROOT)
    return()
endif()

set(LUA_API_FILES
    README.md
    coroutine.doclua
    global.doclua
    math.doclua
    string.doclua
    table.doclua
    openmw/ambient.lua
    openmw/animation.lua
    openmw/async.lua
    openmw/camera.lua
    openmw/core.lua
    openmw/debug.lua
    openmw/input.lua
    openmw/interfaces.lua
    openmw/markup.lua
    openmw/menu.lua
    openmw/nearby.lua
    openmw/postprocessing.lua
    openmw/self.lua
    openmw/storage.lua
    openmw/types.lua
    openmw/ui.lua
    openmw/util.lua
    openmw/vfs.lua
    openmw/world.lua
)

foreach (f ${LUA_API_FILES})
    copy_resource_file("${CMAKE_CURRENT_SOURCE_DIR}/${f}" "${OPENMW_RESOURCES_ROOT}" "resources/lua_api/${f}")
endforeach (f)
