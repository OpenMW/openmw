project(MWCompiler)

set(TOOLS_MWCOMPILER ${COMPILER} 
    main.cpp 
    context.cpp
    context.hpp)

add_executable(mwcompiler ${TOOLS_MWCOMPILER})

