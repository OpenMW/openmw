#ifndef OPENMW_ESM_DOOR_H
#define OPENMW_ESM_DOOR_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

struct Door
{
    std::string mName, mModel, mScript, mOpenSound, mCloseSound;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
