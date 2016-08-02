//
// Created by koncord on 08.05.16.
//

#ifndef PLUGINSYSTEM3_LANGPAWN_HPP
#define PLUGINSYSTEM3_LANGPAWN_HPP

#include "Language.hpp"
#include <amx.h>

class LangPAWN: public Language
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
    static AMX_NATIVE_INFO* functions(indices<Indices...>);

    AMX *amx;
public:
    LangPAWN();
    LangPAWN(AMX *amx);
    ~LangPAWN();
    static cell MakePublic(AMX *amx, const cell *params) noexcept;
    static cell CallPublic(AMX *amx, const cell *params) noexcept;
    static cell CreateTimer(AMX *amx, const cell *params) noexcept;
    static cell CreateTimerEx(AMX *amx, const cell *params) noexcept;

    virtual void LoadProgram(const char *filename) override;
    virtual int FreeProgram() override;
    virtual bool IsCallbackPresent(const char *name) override;
    virtual boost::any Call(const char *name, const char *argl, int buf, ...) override;
    virtual boost::any Call(const char *name, const char *argl, const std::vector<boost::any> &args) override;
};


#endif //PLUGINSYSTEM3_LANGPAWN_HPP
