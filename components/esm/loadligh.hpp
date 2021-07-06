#ifndef OPENMW_ESM_LIGH_H
#define OPENMW_ESM_LIGH_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Lights. Includes static light sources and also carryable candles
 * and torches.
 */

struct Light
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "Light"; }

    enum Flags
    {
        Dynamic     = 0x001,
        Carry       = 0x002, // Can be carried
        Negative    = 0x004, // Negative light - i.e. darkness
        Flicker     = 0x008,
        Fire        = 0x010,
        OffDefault  = 0x020, // Off by default - does not burn while placed in a cell, but can burn when equipped by an NPC
        FlickerSlow = 0x040,
        Pulse       = 0x080,
        PulseSlow   = 0x100
    };

    struct LHDTstruct
    {
        float mWeight;
        int mValue;
        int mTime; // Duration
        int mRadius;
        unsigned int mColor; // 4-byte rgba value
        int mFlags;
    }; // Size = 24 bytes

    LHDTstruct mData;

    unsigned int mRecordFlags;
    std::string mSound, mScript, mModel, mIcon, mName, mId;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};
}
#endif
