#ifndef OPENMW_ESM_SNDG_H
#define OPENMW_ESM_SNDG_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Sound generator. This describes the sounds a creature make.
 */

struct SoundGenerator
{
    static unsigned int sRecordId;

    enum Type
    {
        LeftFoot = 0,
        RightFoot = 1,
        SwimLeft = 2,
        SwimRight = 3,
        Moan = 4,
        Roar = 5,
        Scream = 6,
        Land = 7
    };

    // Type
    int mType;

    std::string mId, mCreature, mSound;

    void load(ESMReader &esm);
    void save(ESMWriter &esm) const;

    void blank();
};
}
#endif
