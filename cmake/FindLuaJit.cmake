# Once found, defines:
#  LuaJit_FOUND
#  LuaJit_INCLUDE_DIR
#  LuaJit_LIBRARIES

include(LibFindMacros)

libfind_pkg_detect(LuaJit luajit
        FIND_PATH luajit.h PATH_SUFFIXES luajit luajit-2.1
        FIND_LIBRARY luajit-5.1 luajit
        )

libfind_process(LuaJit)

