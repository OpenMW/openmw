#ifndef OPENMW_ESM_GMST_H
#define OPENMW_ESM_GMST_H

#include <string>

#include "defs.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 *  Game setting, with automatic cleaning of "dirty" entries.
 *
 */

struct GameSetting
{
    std::string mId;
    // One of these is used depending on the variable type
    std::string mStr;
    int mI;
    float mF;
    VarType mType;

    void load(ESMReader &esm);
    
    int getInt() const;
    ///< Throws an exception if GMST is not of type int or float.
    
    float getFloat() const;
    ///< Throws an exception if GMST is not of type int or float.
    
    std::string getString() const;
    ///< Throwns an exception if GMST is not of type string.

    void save(ESMWriter &esm);
};
}
#endif
