# Once found, defines:
#  LuaJit_FOUND
#  LuaJit_INCLUDE_DIR
#  LuaJit_LIBRARIES

include(LibFindMacros)

libfind_pkg_detect(LuaJit luajit
        FIND_PATH luajit.h PATH_SUFFIXES luajit luajit-2.1
        # note: vcpkg builds luatjit-5.1 as lua51
        FIND_LIBRARY lua51 luajit-5.1 luajit
        )

libfind_process(LuaJit)
