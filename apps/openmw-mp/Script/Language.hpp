//
// Created by koncord on 19.03.16.
//

#ifndef PLUGINSYSTEM3_LANGUAGE_HPP
#define PLUGINSYSTEM3_LANGUAGE_HPP

#include "Types.hpp"

#include <boost/any.hpp>
#include <vector>

class Language
{
public:
    virtual ~Language(){}
    virtual void LoadProgram(const char* filename) = 0;
    virtual int FreeProgram() = 0;
    virtual bool IsCallbackPresent(const char* name) = 0;
    virtual boost::any Call(const char* name, const char* argl, int buf, ...) = 0;
    virtual boost::any Call(const char* name, const char* argl, const std::vector<boost::any>& args) = 0;

    virtual lib_t GetInterface() = 0;

};


#endif //PLUGINSYSTEM3_LANGUAGE_HPP
