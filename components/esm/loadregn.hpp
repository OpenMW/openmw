#ifndef _ESM_REGN_H
#define _ESM_REGN_H

#include "record.hpp"
#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

/*
 * Region data
 */

struct Region : public Record
{
#pragma pack(push)
#pragma pack(1)
    struct WEATstruct
    {
        // I guess these are probabilities
        char clear, cloudy, foggy, overcast, rain, thunder, ash, blight,
        // Unknown weather, probably snow and something. Only
        // present in file version 1.3.
                a, b;
    }; // 10 bytes

    // Reference to a sound that is played randomly in this region
    struct SoundRef
    {
        NAME32 sound;
        char chance;
    }; // 33 bytes
#pragma pack(pop)

    WEATstruct data;
    int mapColor; // RGBA

    // sleepList refers to a eveled list of creatures you can meet if
    // you sleep outside in this region.
    std::string name, sleepList;

    std::vector<SoundRef> soundList;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_REGN; }
};
}
#endif
