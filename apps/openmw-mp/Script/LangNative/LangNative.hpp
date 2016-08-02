//
// Created by koncord on 09.05.16.
//

#ifndef PLUGINSYSTEM3_LANGNATIVE_HPP
#define PLUGINSYSTEM3_LANGNATIVE_HPP


#include <Script/Language.hpp>
#include <Script/SystemInterface.hpp>

class LangNative : public Language
{
    lib_t lib;
public:
    virtual lib_t GetInterface() override;
    LangNative();
    ~LangNative();
    virtual void LoadProgram(const char *filename) override;
    virtual int FreeProgram() override;
    virtual bool IsCallbackPresent(const char *name) override;
    virtual boost::any Call(const char *name, const char *argl, int buf, ...) override;
    virtual boost::any Call(const char *name, const char *argl, const std::vector<boost::any> &args) override;

};


#endif //PLUGINSYSTEM3_LANGNATIVE_HPP
