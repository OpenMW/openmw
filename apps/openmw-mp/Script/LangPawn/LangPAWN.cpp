//
// Created by koncord on 08.05.16.
//

#include "LangPAWN.hpp"


#include <amxmodules.h>
#include <amxaux.h>
#include "Script.hpp"

using namespace std;

typedef long NetworkID;

static vector<vector<char>> strings;
static vector<pair<cell*, double>> floats;
static pair<cell*, NetworkID*> data = {nullptr, nullptr};

void free_strings() noexcept {
    strings.clear();
}

void free_floats() noexcept {
    for (const auto& value : floats)
        *value.first = amx_ftoc(value.second);

    floats.clear();
}

void free_data(unsigned int size) noexcept {
    if (data.first && data.second)
        for (unsigned int i = 0; i < size; ++i)
            data.first[i] = data.second[i];

    data.first = nullptr;
    data.second = nullptr;
}

void after_call() noexcept {
    free_strings();
    free_floats();
}

template<typename R>
void after_call(const R&) noexcept {
    free_strings();
    free_floats();
}

template<>
void after_call(const unsigned int& result) noexcept {
    free_strings();
    free_floats();
    free_data(result);
}

template<typename R, unsigned int I, unsigned int F>
struct PAWN_extract_ {
    inline static R PAWN_extract(AMX*&&, const cell*&& params) noexcept {
        return static_cast<R>(forward<const cell*>(params)[I]);
    }
};

template<unsigned int I, unsigned int F>
struct PAWN_extract_<void*, I, F>
{
    inline static void* PAWN_extract(AMX *&&amx, const cell *&&params) noexcept
    {
        return amx_Address(amx, forward<const cell *>(params)[I]); // fixme: I'm not sure in this fix
    }
};

template<unsigned int I, unsigned int F>
struct PAWN_extract_<double, I, F> {
    inline static double PAWN_extract(AMX*&&, const cell*&& params) noexcept {
        return amx_ctof(forward<const cell*>(params)[I]);
    }
};

template<unsigned int I, unsigned int F>
struct PAWN_extract_<const char*, I, F> {
    inline static const char* PAWN_extract(AMX*&& amx, const cell*&& params) noexcept {
        int len;
        cell* source;

        source = amx_Address(amx, params[I]);
        amx_StrLen(source, &len);

        strings.emplace_back(len + 1);
        char* value = &strings.back()[0];
        amx_GetString(value, source, 0, UNLIMITED);

        return value;
    }
};

template<unsigned int I, unsigned int F>
struct PAWN_extract_<double*, I, F> {
    inline static double* PAWN_extract(AMX*&& amx, const cell*&& params) noexcept {
        floats.emplace_back(amx_Address(amx, params[I]), 0.00);
        return &floats.back().second;
    }
};

template<unsigned int I, unsigned int F>
struct PAWN_extract_<NetworkID**, I, F> {
    inline static NetworkID** PAWN_extract(AMX*&& amx, const cell*&& params) noexcept {
        constexpr ScriptFunctionData const& F_ = ScriptFunctions::functions[F];
        static_assert(F_.func.numargs == I, "NetworkID** must be the last parameter");
        data.first = amx_Address(amx, params[I]);
        return &data.second;
    }
};

template<unsigned int I, unsigned int F>
struct PAWN_dispatch_ {
    template<typename R, typename... Args>
    inline static R PAWN_dispatch(AMX*&& amx, const cell*&& params, Args&&... args) noexcept {
        constexpr ScriptFunctionData const& F_ = ScriptFunctions::functions[F];
        auto arg = PAWN_extract_<typename CharType<F_.func.types[I - 1]>::type, I, F>::PAWN_extract(forward<AMX*>(amx), forward<const cell*>(params));
        return PAWN_dispatch_<I - 1, F>::template PAWN_dispatch<R>(
                forward<AMX*>(amx),
                forward<const cell*>(params),
                arg,
                forward<Args>(args)...);
    }
};

template<unsigned int F>
struct PAWN_dispatch_<0, F> {
    template<typename R, typename... Args>
    inline static R PAWN_dispatch(AMX*&&, const cell*&&, Args&&... args) noexcept {
        constexpr ScriptFunctionData const& F_ = ScriptFunctions::functions[F];
        return reinterpret_cast<FunctionEllipsis<R>>(F_.func.addr)(forward<Args>(args)...);
    }
};

template<unsigned int I>
static typename enable_if<ScriptFunctions::functions[I].func.ret == 'v', cell>::type wrapper(AMX* amx, const cell* params) noexcept {
    PAWN_dispatch_<ScriptFunctions::functions[I].func.numargs, I>::template PAWN_dispatch<void>(forward<AMX*>(amx), forward<const cell*>(params));
    after_call();
    return 1;
}

template<unsigned int I>
static typename enable_if<ScriptFunctions::functions[I].func.ret == 'f', cell>::type wrapper(AMX* amx, const cell* params) noexcept {
    double value = PAWN_dispatch_<ScriptFunctions::functions[I].func.numargs, I>::template PAWN_dispatch<double>(forward<AMX*>(amx), forward<const cell*>(params));
    after_call();
    return amx_ftoc(value);
}

template<unsigned int I>
static typename enable_if<ScriptFunctions::functions[I].func.ret == 's', cell>::type wrapper(AMX* amx, const cell* params) noexcept {
    const char* value = PAWN_dispatch_<ScriptFunctions::functions[I].func.numargs, I>::template PAWN_dispatch<const char*>(forward<AMX*>(amx), forward<const cell*>(params));
    after_call();

    if (value) {
        cell* dest = amx_Address(amx, params[ScriptFunctions::functions[I].func.numargs + 1]);
        amx_SetString(dest, value, 1, 0, strlen(value) + 1);
        return 1;
    }

    return 0;
}

template<unsigned int I>
static typename enable_if<ScriptFunctions::functions[I].func.ret != 'v' && ScriptFunctions::functions[I].func.ret != 'f' && ScriptFunctions::functions[I].func.ret != 's', cell>::type wrapper(AMX* amx, const cell* params) noexcept {
    auto result = PAWN_dispatch_<ScriptFunctions::functions[I].func.numargs, I>::template PAWN_dispatch<typename CharType<ScriptFunctions::functions[I].func.ret>::type>(forward<AMX*>(amx), forward<const cell*>(params));
    after_call(result);
    return result;
}

template<unsigned int I> struct F_ { static constexpr AMX_NATIVE_INFO F{ScriptFunctions::functions[I].name, wrapper<I>}; };
template<> struct F_<0> { static constexpr AMX_NATIVE_INFO F{"CreateTimer", LangPAWN::CreateTimer}; };
template<> struct F_<1> { static constexpr AMX_NATIVE_INFO F{"CreateTimerEx", LangPAWN::CreateTimerEx}; };
template<> struct F_<2> { static constexpr AMX_NATIVE_INFO F{"MakePublic", LangPAWN::MakePublic}; };
template<> struct F_<3> { static constexpr AMX_NATIVE_INFO F{"CallPublic", LangPAWN::CallPublic}; };

void LangPAWN::LoadProgram(const char *filename)
{
    int err = aux_LoadProgram(amx, filename, 0);
    if (err != AMX_ERR_NONE)
        throw runtime_error("PAWN script " + string(filename) + " error (" + to_string(err) + "): \"" + string(aux_StrError(err)) + "\"");

    amx_CoreInit(amx);
    amx_ConsoleInit(amx);
    amx_FloatInit(amx);
    amx_TimeInit(amx);
    amx_StringInit(amx);
    amx_FileInit(amx);

    constexpr auto functions_n = sizeof(ScriptFunctions::functions) / sizeof(ScriptFunctions::functions[0]);

    amx_Register(amx, functions(IndicesFor<functions_n>{}), functions_n); // TODO: throw if error

}

int LangPAWN::FreeProgram()
{
    int err = aux_FreeProgram(amx);
    delete amx;
    return err;
}

bool LangPAWN::IsCallbackPresent(const char *name)
{
    int idx;
    return (amx_FindPublic(amx, name, &idx) == AMX_ERR_NONE);
}

boost::any LangPAWN::Call(const char *name, const char *argl, int buf, ...)
{
    va_list args;
    va_start(args, buf);
    cell ret = 0;
    vector<pair<cell *, char *>> strings;

    try
    {
        int idx = 0;
        int err = 0;

        err = amx_FindPublic(amx, name, &idx);

        if (err != AMX_ERR_NONE)
            throw runtime_error("PAWN runtime error (" + to_string(err) + "): \"" + string(aux_StrError(err)) + "\".");

        unsigned int len = strlen(argl);
        vector<cell> args_amx;

        for (unsigned int i = 0; i < len; ++i)
        {
            switch (argl[i])
            {
                case 'i':
                    args_amx.emplace_back(va_arg(args, unsigned
                            int));
                    break;

                case 'q':
                    args_amx.emplace_back(va_arg(args, signed
                            int));
                    break;

                case 'l':
                    args_amx.emplace_back(va_arg(args, unsigned
                    long
                            long));
                    break;

                case 'w':
                    args_amx.emplace_back(va_arg(args, signed
                    long
                            long));
                    break;

                case 'f':
                {
                    double value = va_arg(args, double);
                    args_amx.emplace_back(amx_ftoc(value));
                    break;
                }

                case 'p':
                    args_amx.emplace_back(reinterpret_cast<uintptr_t>(va_arg(args, void*)));
                    break;

                case 's':
                    args_amx.emplace_back(reinterpret_cast<uintptr_t>(va_arg(args, char*)));
                    break;

                default:
                    throw runtime_error("PAWN call: Unknown argument identifier " + argl[i]);
            }
        }

        for (unsigned int i = len; i; --i)
        {
            switch (argl[i - 1])
            {
                case 's':
                {
                    char *string = reinterpret_cast<char *>(static_cast<unsigned int>(args_amx[i - 1]));
                    cell *store;
                    amx_PushString(amx, &store, string, 1, 0);
                    strings.emplace_back(store, string);
                    break;
                }

                default:
                    amx_Push(amx, args_amx[i - 1]);
                    break;
            }
        }

        err = amx_Exec(amx, &ret, idx);

        if (err != AMX_ERR_NONE)
            throw runtime_error("PAWN runtime error (" + to_string(err) + "): \"" + string(aux_StrError(err)) + "\".");

        if (buf != 0)
            for (const auto &str : strings)
                amx_GetString(str.second, str.first, 0, strlen(str.second) + 1);

        if (!strings.empty())
            amx_Release(amx, strings[0].first);
    }

    catch (...)
    {
        va_end(args);

        if (!strings.empty())
            amx_Release(amx, strings[0].first);

        throw;
    }

    return boost::any(ret);
}

boost::any LangPAWN::Call(const char *name, const char *argl, const std::vector<boost::any> &args)
{
    cell ret = 0;
    cell *str = nullptr;

    try
    {
        int idx = 0;
        int err = 0;

        err = amx_FindPublic(amx, name, &idx);

        if (err != AMX_ERR_NONE)
            throw runtime_error("PAWN runtime error (" + to_string(err) + "): \"" + string(aux_StrError(err)) + "\".");

        for (intptr_t i = strlen(argl) - 1; i >= 0; i--)
        {
            switch (argl[i])
            {
                case 'i':
                {
                    cell value = (cell) boost::any_cast<unsigned int>(args.at(i));
                    amx_Push(amx, value);
                    break;
                }

                case 'q':
                {
                    cell value = (cell) boost::any_cast<signed int>(args.at(i));
                    amx_Push(amx, value);
                    break;
                }

                case 'l':
                {
                    cell value = (cell) boost::any_cast<unsigned long long>(args.at(i));
                    amx_Push(amx, value);
                    break;
                }

                case 'w':
                {
                    cell value = (cell) boost::any_cast<signed long long>(args.at(i));
                    amx_Push(amx, value);
                    break;
                }

                case 'f':
                {
                    double value = boost::any_cast<double>(args.at(i));
                    amx_Push(amx, amx_ftoc(value));
                    break;
                }

                case 'p':
                {
                    cell value = (cell) boost::any_cast<void *>(args.at(i));
                    amx_Push(amx, value);
                    break;
                }

                case 's':
                {
                    string string_ = boost::any_cast<string>(args.at(i));
                    cell *store;
                    amx_PushString(amx, &store, string_.c_str(), 1, 0);

                    if (!str)
                        str = store;

                    break;
                }

                default:
                    throw runtime_error("PAWN call: Unknown argument identifier " + argl[i]);
            }
        }

        err = amx_Exec(amx, &ret, idx);

        if (err != AMX_ERR_NONE)
            throw runtime_error("PAWN runtime error (" + to_string(err) + "): \"" + string(aux_StrError(err)) + "\".");

        if (str)
            amx_Release(amx, str);
    }

    catch (...)
    {
        if (str)
            amx_Release(amx, str);

        throw;
    }

    return ret;
}

template<size_t... Indices>
inline AMX_NATIVE_INFO *LangPAWN::functions(indices<Indices...>)
{

    static AMX_NATIVE_INFO functions_[sizeof...(Indices)]{
            F_<Indices>::F...
    };

    static_assert(
            sizeof(functions_) / sizeof(functions_[0]) == sizeof(ScriptFunctions::functions) / sizeof(ScriptFunctions::functions[0]),
            "Not all functions have been mapped to PAWN");

    return functions_;
}


lib_t LangPAWN::GetInterface()
{
    return reinterpret_cast<lib_t>(amx);
}

LangPAWN::LangPAWN()
{
    //throw std::runtime_error("Pawn is no longer supported, use Terra/Lua!");
    amx = new AMX();
}


LangPAWN::LangPAWN(AMX *amx)
{
    this->amx = amx;
}


LangPAWN::~LangPAWN()
{

}
