//
// Created by koncord on 23.01.16.
//

#ifndef TMPTYPES_HPP
#define TMPTYPES_HPP


#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <RakNetTypes.h>
#include <Utils.hpp>

#ifdef _WIN32
#include <winsock2.h>
#endif

#ifdef _WIN32
typedef HMODULE lib_t;
#else
typedef void* lib_t;
#endif

template<typename T> struct sizeof_void { enum { value = sizeof(T) }; };
template<> struct sizeof_void<void> { enum { value = 0 }; };


template<typename T, size_t t> struct TypeChar { static_assert(!t, "Unsupported type in variadic type list"); };
template<> struct TypeChar<bool, sizeof(bool)> { enum { value = 'b' }; };
template<typename T> struct TypeChar<T*, sizeof(void*)> { enum { value = 'p' }; };
template<> struct TypeChar<double*, sizeof(double*)> { enum { value = 'd' }; };
template<> struct TypeChar<RakNet::NetworkID**, sizeof(RakNet::NetworkID**)> { enum { value = 'n' }; };
template<typename T> struct TypeChar<T, sizeof(uint8_t)> { enum { value = std::is_signed<T>::value ? 'q' : 'i' }; };
template<typename T> struct TypeChar<T, sizeof(uint16_t)> { enum { value = std::is_signed<T>::value ? 'q' : 'i' }; };
template<typename T> struct TypeChar<T, sizeof(uint32_t)> { enum { value = std::is_signed<T>::value ? 'q' : 'i' }; };
template<typename T> struct TypeChar<T, sizeof(uint64_t)> { enum { value = std::is_signed<T>::value ? 'w' : 'l' }; };
template<> struct TypeChar<double, sizeof(double)> { enum { value = 'f' }; };
template<> struct TypeChar<char*, sizeof(char*)> { enum { value = 's' }; };
template<> struct TypeChar<const char*, sizeof(const char*)> { enum { value = 's' }; };
template<> struct TypeChar<void, sizeof_void<void>::value> { enum { value = 'v' }; };

template<const char t> struct CharType { static_assert(!t, "Unsupported type in variadic type list"); };
template<> struct CharType<'b'> { typedef bool type; };
template<> struct CharType<'p'> { typedef void* type; };
template<> struct CharType<'d'> { typedef double* type; };
template<> struct CharType<'n'> { typedef RakNet::NetworkID** type; };
template<> struct CharType<'q'> { typedef signed int type; };
template<> struct CharType<'i'> { typedef unsigned int type; };
template<> struct CharType<'w'> { typedef signed long long type; };
template<> struct CharType<'l'> { typedef unsigned long long type; };
template<> struct CharType<'f'> { typedef double type; };
template<> struct CharType<'s'> { typedef const char* type; };
template<> struct CharType<'v'> { typedef void type; };

template<typename... Types>
struct TypeString {
    static constexpr char value[sizeof...(Types) + 1] = {
            TypeChar<Types, sizeof(Types)>::value...
    };
};


template<typename R, typename... Types>
using Function = R(*)(Types...);

template<typename R>
using FunctionEllipsis = R(*)(...);

struct ScriptIdentity
{
    const char* types;
    const char ret;
    const unsigned int numargs;

    constexpr bool matches(const char* types, const unsigned int N = 0) const
    {
        return N < numargs ? this->types[N] == types[N] && matches(types, N + 1) : this->types[N] == types[N];
    }

    template<typename R, typename... Types>
    constexpr ScriptIdentity(Function<R, Types...>) : types(TypeString<Types...>::value), ret(TypeChar<R, sizeof_void<R>::value>::value), numargs(sizeof(TypeString<Types...>::value) - 1) {}
};



struct ScriptFunctionPointer : public ScriptIdentity
{
    Function<void> addr;

    template<typename R, typename... Types>
    constexpr ScriptFunctionPointer(Function<R, Types...> addr) : ScriptIdentity(addr), addr(reinterpret_cast<Function<void>>(addr)) {}
};

struct ScriptFunctionData
{
    const char* name;
    const ScriptFunctionPointer func;

    constexpr ScriptFunctionData(const char* name, ScriptFunctionPointer func) : name(name), func(func) {}
};

struct ScriptCallbackData
{
    const char* name;
    const unsigned int index;
    const ScriptIdentity callback;

    template<size_t N>
    constexpr ScriptCallbackData(const char(&name)[N], ScriptIdentity _callback) : name(name), index(Utils::hash(name)), callback(_callback) {}
};

#endif //TMPTYPES_HPP
