//
// Created by koncord on 19.03.16.
//

#ifndef PLUGINSYSTEM3_SYSTEMINTERFACE_HPP
#define PLUGINSYSTEM3_SYSTEMINTERFACE_HPP

#ifdef _WIN32
#include <winsock2.h>
#else
#include <dlfcn.h>
#endif

template<typename R = void*>
struct SystemInterface
{
#ifdef _WIN32
    typedef HMODULE lib_t;
#else
    typedef void* lib_t;
#endif

    union
    {
        R result;
#ifdef _WIN32
        decltype(GetProcAddress(lib_t(), nullptr)) data;
#else
        decltype(dlsym(lib_t(), nullptr)) data;
#endif
    };

#ifndef _WIN32
    static_assert(sizeof(result) == sizeof(data), "R should have the same size");
#endif

    SystemInterface() : data(nullptr) {}
    explicit operator bool() { return data; }

#ifdef _WIN32
    SystemInterface(lib_t handle, const char* name) : data(GetProcAddress(handle, name)) {}
#else
    SystemInterface(lib_t handle, const char* name) : data(dlsym(handle, name)) {}
#endif
};
#endif //PLUGINSYSTEM3_SYSTEMINTERFACE_HPP
