#ifndef SOL_SINGLE_CONFIG_HPP
#define SOL_SINGLE_CONFIG_HPP

#define SOL_SAFE_USERTYPE 1
#define SOL_SAFE_REFERENCES 1
#define SOL_SAFE_FUNCTION_CALLS 1
#define SOL_SAFE_FUNCTION 1
#define SOL_NO_NIL 0
#define SOL_IN_DEBUG_DETECTED 0

#ifndef NO_LUAJIT
#define SOL_LUAJIT 1
#define SOL_EXCEPTIONS_SAFE_PROPAGATION 0
#endif

#include <limits> // missing in sol/sol.hpp

#endif // SOL_SINGLE_CONFIG_HPP
