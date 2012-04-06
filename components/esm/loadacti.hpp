#ifndef _ESM_ACTI_H
#define _ESM_ACTI_H

#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

struct Activator
{
    std::string name, script, model;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
