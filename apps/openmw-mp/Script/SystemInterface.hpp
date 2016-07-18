//
// Created by koncord on 19.03.16.
//

#ifndef PLUGINSYSTEM3_SYSTEMINTERFACE_HPP
#define PLUGINSYSTEM3_SYSTEMINTERFACE_HPP

#ifdef __WIN32__
#include <winsock2.h>
#else
#include <dlfcn.h>
#endif

template<typename R = void*>
struct SystemInterface
{
#ifdef __WIN32__
    typedef HMODULE lib_t;
#else
    typedef void* lib_t;
#endif

    union
    {
        R result;
#ifdef __WIN32__
        decltype(GetProcAddress(lib_t(), nullptr)) data;
#else
        decltype(dlsym(lib_t(), nullptr)) data;
#endif
    };

    static_assert(sizeof(result) == sizeof(data), "R should have the same size");

    SystemInterface() : data(nullptr) {}
    explicit operator bool() { return data; }

#ifdef __WIN32__
    SystemInterface(lib_t handle, const char* name) : data(GetProcAddress(handle, name)) {}
#else
    SystemInterface(lib_t handle, const char* name) : data(dlsym(handle, name)) {}
#endif
};
#endif //PLUGINSYSTEM3_SYSTEMINTERFACE_HPP
