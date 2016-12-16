//
// Created by koncord on 19.03.16.
//

#ifndef PLUGINSYSTEM3_SCRIPT_HPP
#define PLUGINSYSTEM3_SCRIPT_HPP
#include "Types.hpp"
#include "SystemInterface.hpp"
#include "ScriptFunction.hpp"
#include "ScriptFunctions.hpp"
#include "Language.hpp"

#include <boost/any.hpp>
#include <Utils.hpp>
#include <unordered_map>
#include <memory>

class Script : private ScriptFunctions
{
    // http://imgur.com/hU0N4EH
private:

    Language *lang;

    enum
    {
        SCRIPT_CPP,
        SCRIPT_PAWN,
        SCRIPT_LUA
    };

    template<typename R>
    R GetScript(const char *name)
    {
        if (script_type == SCRIPT_CPP)
        {
            return SystemInterface<R>(lang->GetInterface(), name).result;
        }
        else
        {
            return reinterpret_cast<R>(lang->IsCallbackPresent(name));
        }
    }

    int script_type;
    std::unordered_map<unsigned int, FunctionEllipsis<void>> callbacks_;

    typedef std::vector<std::unique_ptr<Script>> ScriptList;
    static ScriptList scripts;

    Script(const char *path);

    Script(const Script&) = delete;
    Script& operator=(const Script&) = delete;

public:
    ~Script();

    static void LoadScript(const char *script, const char* base);
    static void LoadScripts(char* scripts, const char* base);
    static void UnloadScripts();

    static constexpr ScriptCallbackData const& CallBackData(const unsigned int I, const unsigned int N = 0) {
        return callbacks[N].index == I ? callbacks[N] : CallBackData(I, N + 1);
    }

    template<unsigned int I>
    using CallBackReturn = typename CharType<CallBackData(I).callback.ret>::type;

    template<size_t N>
    static constexpr unsigned int CallbackIdentity(const char(&str)[N])
    {
        return Utils::hash(str);
    }


    template<unsigned int I, bool B = false, typename... Args>
    static unsigned int Call(CallBackReturn<I>& result, Args&&... args) {
        constexpr ScriptCallbackData const& data = CallBackData(I);
        static_assert(data.callback.matches(TypeString<typename std::remove_reference<Args>::type...>::value), "Wrong number or types of arguments");

        unsigned int count = 0;

        for (auto& script : scripts)
        {
            if (!script->callbacks_.count(I))
                script->callbacks_.emplace(I, script->GetScript<FunctionEllipsis<void>>(data.name));

            auto callback = script->callbacks_[I];

            if (!callback)
                continue;


            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Called function \"%s\"", data.name);

            if (script->script_type == SCRIPT_CPP)
                result = reinterpret_cast<FunctionEllipsis<CallBackReturn<I>>>(callback)(std::forward<Args>(args)...);
#if defined (ENABLE_PAWN)
            else if (script->script_type == SCRIPT_PAWN)
            {
                boost::any any = script->lang->Call(data.name, data.callback.types, B, std::forward<Args>(args)...);
                result = reinterpret_cast<CallBackReturn<I>> ((int)boost::any_cast<int64_t>(any)); // TODO: WTF?! int?!
            }
#endif
#if defined (ENABLE_LUA)
            else if (script->script_type == SCRIPT_LUA)
            {
                boost::any any = script->lang->Call(data.name, data.callback.types, B, std::forward<Args>(args)...);
                result = static_cast<CallBackReturn<I>>(boost::any_cast<luabridge::LuaRef>(any).cast<CallBackReturn<I>>());
            }
#endif
            ++count;
        }

        return count;
    }

    template<unsigned int I, bool B = false, typename... Args>
    static unsigned int Call(Args&&... args) {
        constexpr ScriptCallbackData const& data = CallBackData(I);
        static_assert(data.callback.matches(TypeString<typename std::remove_reference<Args>::type...>::value), "Wrong number or types of arguments");

        unsigned int count = 0;

        for (auto& script : scripts)
        {
            if (!script->callbacks_.count(I))
                script->callbacks_.emplace(I, script->GetScript<FunctionEllipsis<void>>(data.name));

            auto callback = script->callbacks_[I];

            if (!callback)
                continue;

            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Called function \"%s\"", data.name);

            if (script->script_type == SCRIPT_CPP)
                reinterpret_cast<FunctionEllipsis<CallBackReturn<I>>>(callback)(std::forward<Args>(args)...);
#if defined (ENABLE_PAWN)
            else if (script->script_type == SCRIPT_PAWN)
                script->lang->Call(data.name, data.callback.types, B, std::forward<Args>(args)...);
#endif
#if defined (ENABLE_LUA)
            else if (script->script_type == SCRIPT_LUA)
                script->lang->Call(data.name, data.callback.types, B, std::forward<Args>(args)...);
#endif
            ++count;
        }

        return count;
    }
};

#endif //PLUGINSYSTEM3_SCRIPT_HPP
