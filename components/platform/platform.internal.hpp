#ifndef COMPONENT_PLATFORM_FILE_HPP
#define COMPONENT_PLATFORM_FILE_HPP

namespace Platform
{
#define PLATFORM_TYPE_STDIO 0
#define PLATFORM_TYPE_WIN32 1
#define PLATFORM_TYPE_POSIX 2
    
#if defined(__linux) || defined(__unix) || defined(__posix)
#define PLATFORM_TYPE PLATFORM_TYPE_POSIX
#elif defined(_WIN32)
#define PLATFORM_TYPE PLATFORM_TYPE_WIN32
#else
#define PLATFORM_TYPE PLATFORM_TYPE_STDIO
#endif
}

#endif 
