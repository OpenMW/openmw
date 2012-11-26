#ifndef OPENMW_ESM_GLOB_H
#define OPENMW_ESM_GLOB_H

#include <string>

#include "defs.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Global script variables
 */

struct Global
{
    std::string mId;
    float mValue;
    VarType mType;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
