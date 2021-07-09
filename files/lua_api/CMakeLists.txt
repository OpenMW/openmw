set(LUA_API_FILES
    README.md
    coroutine.doclua
    global.doclua
    math.doclua
    string.doclua
    table.doclua
    openmw/async.lua
    openmw/core.lua
    openmw/nearby.lua
    openmw/query.lua
    openmw/self.lua
    openmw/ui.lua
    openmw/util.lua
    openmw/world.lua
)

foreach (f ${LUA_API_FILES})
    copy_resource_file("${CMAKE_CURRENT_SOURCE_DIR}/${f}" "${OpenMW_BINARY_DIR}" "resources/lua_api/${f}")
endforeach (f)

