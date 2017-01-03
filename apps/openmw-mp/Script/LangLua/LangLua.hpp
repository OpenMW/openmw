//
// Created by koncord on 08.05.16.
//

#ifndef PLUGINSYSTEM3_LANGLUA_HPP
#define PLUGINSYSTEM3_LANGLUA_HPP

#ifdef ENABLE_TERRA
#include <terra/terra.h>
#else
#include "lua.hpp"
#endif

#include <extern/LuaBridge/LuaBridge.h>
#include <LuaBridge.h>

#include <boost/any.hpp>
#include "../ScriptFunction.hpp"
#include "../Language.hpp"

struct LuaFuctionData
{
    const char* name;
    lua_CFunction func;
};

class LangLua: public Language
{
private:
    template<std::size_t... Is>
    struct indices {};
    template<std::size_t N, std::size_t... Is>
    struct build_indices : build_indices<N-1, N-1, Is...> {};
    template<std::size_t... Is>
    struct build_indices<0, Is...> : indices<Is...> {};
    template<std::size_t N>
    using IndicesFor = build_indices<N>;

public:
    virtual lib_t GetInterface() override;
    template<std::size_t... Indices>
    static LuaFuctionData* functions(indices<Indices...>);
    lua_State *lua;
public:
    LangLua();
    LangLua(lua_State *lua);
    ~LangLua();
    static int MakePublic(lua_State *lua) noexcept;
    static int CallPublic(lua_State *lua);

    static int CreateTimer(lua_State *lua) noexcept;
    static int CreateTimerEx(lua_State *lua);

    virtual void LoadProgram(const char *filename) override;
    virtual int FreeProgram() override;
    virtual bool IsCallbackPresent(const char *name) override;
    virtual boost::any Call(const char *name, const char *argl, int buf, ...) override;
    virtual boost::any Call(const char *name, const char *argl, const std::vector<boost::any> &args) override;
};


#endif //PLUGINSYSTEM3_LANGLUA_HPP
