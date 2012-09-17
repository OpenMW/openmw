#ifndef _ESM_DOOR_H
#define _ESM_DOOR_H

#include <string>

#include "record.hpp"

namespace ESM
{

struct Door : public Record
{
    std::string mName, mModel, mScript, mOpenSound, mCloseSound;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_DOOR; }
};
}
#endif
