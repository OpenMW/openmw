#ifndef _ESM_APPA_H
#define _ESM_APPA_H

#include "record.hpp"
#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

/*
 * Alchemist apparatus
 */

struct Apparatus : public Record
{
    enum AppaType
    {
        MortarPestle = 0,
        Albemic = 1,
        Calcinator = 2,
        Retort = 3
    };

    struct AADTstruct
    {
        int type;
        float quality;
        float weight;
        int value;
    };

    AADTstruct data;
    std::string model, icon, script, name;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_APPA; }
};
}
#endif
