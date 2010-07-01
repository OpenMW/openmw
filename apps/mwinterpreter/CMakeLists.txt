project(MWInterpreter)

set(TOOLS_MWINTERPRETER 
    ${INTERPRETER} 
    main.cpp
    context.cpp 
    context.hpp)

add_executable(mwinterpreter ${TOOLS_MWINTERPRETER})
