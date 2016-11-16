//
// Created by koncord on 14.05.16.
//

#ifndef PLUGINSYSTEM3_PUBLICFNAPI_HPP
#define PLUGINSYSTEM3_PUBLICFNAPI_HPP

#include <unordered_map>
#include <Script/ScriptFunction.hpp>


class Public : public ScriptFunction
{
private:
    ~Public();

    static std::unordered_map<std::string, Public *> publics;

    Public(ScriptFunc _public, const std::string &name, char ret_type, const std::string &def);
#if defined(ENABLE_PAWN)
    Public(ScriptFuncPAWN _public, AMX* amx, const std::string& name, char ret_type, const std::string& def);
#endif
#if defined(ENABLE_LUA)
    Public(ScriptFuncLua _public, lua_State *lua, const std::string &name, char ret_type, const std::string &def);
#endif

public:
    template<typename... Args>
    static void MakePublic(Args &&... args)
    { new Public(std::forward<Args>(args)...); }

    static boost::any Call(const std::string &name, const std::vector<boost::any> &args);

    static const std::string& GetDefinition(const std::string& name);

    static bool IsPAWN(const std::string &name);
    static bool IsLua(const std::string &name);

    static void DeleteAll();
};

#endif //PLUGINSYSTEM3_PUBLICFNAPI_HPP
