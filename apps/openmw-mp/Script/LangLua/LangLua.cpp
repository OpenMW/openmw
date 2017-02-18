//
// Created by koncord on 08.05.16.
//

#include <iostream>
#include "LangLua.hpp"
#include <Script/Script.hpp>
#include <Script/Types.hpp>

using namespace std;

lib_t LangLua::GetInterface()
{
    return reinterpret_cast<lib_t>(lua);
}

LangLua::LangLua(lua_State *lua)
{
    this->lua = lua;
}

LangLua::LangLua()
{
    lua = luaL_newstate();
    luaL_openlibs(lua); // load all lua std libs
#if defined(ENABLE_TERRA)
    terra_init(lua);
#endif
}

LangLua::~LangLua()
{

}

template<unsigned int I, unsigned int F>
struct Lua_dispatch_ {
    template<typename R, typename... Args>
    inline static R Lua_dispatch(lua_State*&& lua, Args&&... args) noexcept {
        constexpr ScriptFunctionData const& F_ = ScriptFunctions::functions[F];
        auto arg = luabridge::Stack<typename CharType<F_.func.types[I - 1]>::type>::get(lua, I);
        return Lua_dispatch_<I - 1, F>::template Lua_dispatch<R>(
                forward<lua_State*>(lua),
                arg,
                forward<Args>(args)...);
    }
};

template<unsigned int F>
struct Lua_dispatch_<0, F> {
    template<typename R, typename... Args>
    inline static R Lua_dispatch(lua_State*&&, Args&&... args) noexcept {
        constexpr ScriptFunctionData const& F_ = ScriptFunctions::functions[F];
        return reinterpret_cast<FunctionEllipsis<R>>(F_.func.addr)(forward<Args>(args)...);
    }
};

template<unsigned int I>
static typename enable_if<ScriptFunctions::functions[I].func.ret == 'v', int>::type wrapper(lua_State* lua) noexcept {
    Lua_dispatch_<ScriptFunctions::functions[I].func.numargs, I>::template Lua_dispatch<void>(forward<lua_State*>(lua));
    return 0;
}

template<unsigned int I>
static typename enable_if<ScriptFunctions::functions[I].func.ret != 'v', int>::type wrapper(lua_State* lua) noexcept {
    auto ret = Lua_dispatch_<ScriptFunctions::functions[I].func.numargs, I>::template Lua_dispatch<typename CharType<ScriptFunctions::functions[I].func.ret>::type>(forward<lua_State*>(lua));
    luabridge::Stack <typename CharType<ScriptFunctions::functions[I].func.ret>::type>::push (lua, ret);
    return 1;
}

template<unsigned int I>
struct F_
{
    static constexpr LuaFuctionData F{ScriptFunctions::functions[I].name, wrapper<I>};
};


template<> struct F_<0> { static constexpr LuaFuctionData F{"CreateTimer", LangLua::CreateTimer}; };
template<> struct F_<1> { static constexpr LuaFuctionData F{"CreateTimerEx", LangLua::CreateTimerEx}; };
template<> struct F_<2> { static constexpr LuaFuctionData F{"MakePublic", LangLua::MakePublic}; };
template<> struct F_<3> { static constexpr LuaFuctionData F{"CallPublic", LangLua::CallPublic}; };

template<size_t... Indices>
inline LuaFuctionData *LangLua::functions(indices<Indices...>)
{

    static LuaFuctionData functions_[sizeof...(Indices)]{
            F_<Indices>::F...
    };

    static_assert(
            sizeof(functions_) / sizeof(functions_[0]) ==
            sizeof(ScriptFunctions::functions) / sizeof(ScriptFunctions::functions[0]),
            "Not all functions have been mapped to Lua");

    return functions_;
}

void LangLua::LoadProgram(const char *filename)
{
    int err = 0;

#if defined(ENABLE_TERRA)
    if ((err = terra_loadfile(lua, filename)) != 0)
#else
    if ((err =luaL_loadfile(lua, filename)) != 0)
#endif
        throw runtime_error("Lua script " + string(filename) + " error (" + to_string(err) + "): \"" +
                            string(lua_tostring(lua, -1)) + "\"");

    constexpr auto functions_n = sizeof(ScriptFunctions::functions) / sizeof(ScriptFunctions::functions[0]);

    LuaFuctionData *functions_ = functions(IndicesFor<functions_n>{});

    luabridge::Namespace tes3mp = luabridge::getGlobalNamespace(lua).beginNamespace("tes3mp");

    for (unsigned i = 0; i < functions_n; i++)
        tes3mp.addCFunction(functions_[i].name, functions_[i].func);

    tes3mp.endNamespace();

    if ((err = lua_pcall(lua, 0, 0, 0)) != 0) // Run once script for load in memory.
        throw runtime_error("Lua script " + string(filename) + " error (" + to_string(err) + "): \"" +
                            string(lua_tostring(lua, -1)) + "\"");
}

int LangLua::FreeProgram()
{
    lua_close(lua);
    return 0;
}

bool LangLua::IsCallbackPresent(const char *name)
{
    return luabridge::getGlobal(lua, name).isFunction();
}

boost::any LangLua::Call(const char *name, const char *argl, int buf, ...)
{
    va_list vargs;
    va_start(vargs, buf);
    std::vector<boost::any> args;

    ScriptFunctions::GetArguments(args, vargs, argl);

    return Call(name, argl, args);
}

boost::any LangLua::Call(const char *name, const char *argl, const std::vector<boost::any> &args)
{
    int n_args = (int)(strlen(argl)) ;

    lua_getglobal (lua, name);

    for (intptr_t i = 0; i < n_args; i++)
    {
        switch (argl[i])
        {
            case 'i':
                luabridge::Stack<unsigned int>::push(lua, boost::any_cast<unsigned int>(args.at(i)));
                break;

            case 'q':
                luabridge::Stack<signed int>::push(lua, boost::any_cast<signed int>(args.at(i)));
                break;

            case 'l':
                luabridge::Stack<unsigned long long>::push(lua, boost::any_cast<unsigned long long>(args.at(i)));
                break;

            case 'w':
                luabridge::Stack<signed long long>::push(lua, boost::any_cast<signed long long>(args.at(i)));
                break;

            case 'f':
                luabridge::Stack<double>::push(lua, boost::any_cast<double>(args.at(i)));
                break;

            case 'p':
                luabridge::Stack<void *>::push(lua, boost::any_cast<void *>(args.at(i)));
                break;

            case 's':
                luabridge::Stack<const char *>::push(lua, boost::any_cast<const char *>(args.at(i)));
                break;

            case 'b':
                luabridge::Stack<bool>::push(lua, boost::any_cast<int>(args.at(i)));
                break;
            default:
                throw runtime_error("Lua call: Unknown argument identifier " + argl[i]);
        }
    }

    luabridge::LuaException::pcall (lua, n_args, 1);
    return boost::any(luabridge::LuaRef::fromStack(lua, -1));
}
