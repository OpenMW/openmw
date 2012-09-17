#ifndef _ESM_ACTI_H
#define _ESM_ACTI_H

#include "record.hpp"

namespace ESM
{

struct Activator : public Record
{
    std::string mName, mScript, mModel;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_ACTI; }
};

}
#endif
