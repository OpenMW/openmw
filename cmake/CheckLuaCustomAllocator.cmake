set(TMP_ROOT ${CMAKE_BINARY_DIR}/try-compile)
file(MAKE_DIRECTORY ${TMP_ROOT})

file(WRITE ${TMP_ROOT}/checkluacustomallocator.c
"
#include <stdlib.h>
#include <lua.h>

void* custom_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
  if (nsize == 0) {
    free(ptr);
    return NULL;
  } else {
    return realloc(ptr, nsize);
  }
}

int main(void) {
  return lua_newstate(custom_alloc, NULL) ? 0 : 1;
}
")

message(STATUS "Checking if Lua allows to provide a custom allocator")

try_run(RUN_RESULT_VAR COMPILE_RESULT_VAR
    ${TMP_ROOT}/temp
    ${TMP_ROOT}/checkluacustomallocator.c
    CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${LUA_INCLUDE_DIR}"
    LINK_LIBRARIES "${LUA_LIBRARIES}"
    )

if (NOT ${COMPILE_RESULT_VAR})
    message(WARNING "Incorrect Lua library: can't compile checkluacustomallocator.c" )
elseif(NOT ${RUN_RESULT_VAR} EQUAL 0)
    message(WARNING "Incorrect Lua library: custom allocator not supported (likely LuaJit compiled with LJ_64 but without LJ_GC64)" )
else()
    message(STATUS "Lua supports custom allocator")
endif()

