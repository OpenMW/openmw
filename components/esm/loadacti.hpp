#ifndef _ESM_ACTI_H
#define _ESM_ACTI_H

#include "record.hpp"
#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

struct Activator : public Record
{
    std::string name, script, model;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_ACTI; }
};
}
#endif
