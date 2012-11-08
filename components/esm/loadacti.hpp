#ifndef OPENMW_ESM_ACTI_H
#define OPENMW_ESM_ACTI_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

struct Activator
{
    std::string mId, mName, mScript, mModel;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};

}
#endif
