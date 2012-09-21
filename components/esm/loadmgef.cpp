#include "loadmgef.hpp"

#include "esm_reader.hpp"
#include "esm_writer.hpp"

#include <stdio.h>

namespace
{
    const int NumberOfHardcodedFlags = 143;
    const int HardcodedFlags[NumberOfHardcodedFlags] = {
        0x11c8, 0x11c0, 0x11c8, 0x11e0, 0x11e0, 0x11e0, 0x11e0, 0x11d0,
        0x11c0, 0x11c0, 0x11e0, 0x11c0, 0x11184, 0x11184, 0x1f0, 0x1f0,
        0x1f0, 0x11d2, 0x11f0, 0x11d0, 0x11d0, 0x11d1, 0x1d2, 0x1f0,
        0x1d0, 0x1d0, 0x1d1, 0x1f0, 0x11d0, 0x11d0, 0x11d0, 0x11d0,
        0x11d0, 0x11d0, 0x11d0, 0x11d0, 0x11d0, 0x1d0, 0x1d0, 0x11c8,
        0x31c0, 0x11c0, 0x11c0, 0x11c0, 0x1180, 0x11d8, 0x11d8, 0x11d0,
        0x11d0, 0x11180, 0x11180, 0x11180, 0x11180, 0x11180, 0x11180, 0x11180,
        0x11180, 0x11c4, 0x111b8, 0x1040, 0x104c, 0x104c, 0x104c, 0x104c,
        0x1040, 0x1040, 0x1040, 0x11c0, 0x11c0, 0x1cc, 0x1cc, 0x1cc,
        0x1cc, 0x1cc, 0x1c2, 0x1c0, 0x1c0, 0x1c0, 0x1c1, 0x11c2,
        0x11c0, 0x11c0, 0x11c0, 0x11c1, 0x11c0, 0x21192, 0x20190, 0x20190,
        0x20190, 0x21191, 0x11c0, 0x11c0, 0x11c0, 0x11c0, 0x11c0, 0x11c0,
        0x11c0, 0x11c0, 0x11c0, 0x11c0, 0x1c0, 0x11190, 0x9048, 0x9048,
        0x9048, 0x9048, 0x9048, 0x9048, 0x9048, 0x9048, 0x9048, 0x9048,
        0x9048, 0x9048, 0x9048, 0x9048, 0x9048, 0x11c0, 0x1180, 0x1180,
        0x5048, 0x5048, 0x5048, 0x5048, 0x5048, 0x5048, 0x1188, 0x5048,
        0x5048, 0x5048, 0x5048, 0x5048, 0x1048, 0x104c, 0x1048, 0x40,
        0x11c8, 0x1048, 0x1048, 0x1048, 0x1048, 0x1048, 0x1048
    };
}

namespace ESM
{

void MagicEffect::load(ESMReader &esm)
{
  esm.getHNT(mIndex, "INDX");

  esm.getHNT(mData, "MEDT", 36);
  if (mIndex>=0 && mIndex<NumberOfHardcodedFlags)
    mData.mFlags |= HardcodedFlags[mIndex];

  mIcon = esm.getHNOString("ITEX");
  mParticle = esm.getHNOString("PTEX");

  mBoltSound = esm.getHNOString("BSND");
  mCastSound = esm.getHNOString("CSND");
  mHitSound = esm.getHNOString("HSND");
  mAreaSound = esm.getHNOString("ASND");

  mCasting = esm.getHNOString("CVFX");
  mBolt = esm.getHNOString("BVFX");
  mHit = esm.getHNOString("HVFX");
  mArea = esm.getHNOString("AVFX");

  mDescription = esm.getHNOString("DESC");
}
void MagicEffect::save(ESMWriter &esm)
{
    esm.writeHNT("INDX", mIndex);

    mData.mFlags &= 0xe00;
    esm.writeHNT("MEDT", mData, 36);
    if (mIndex>=0 && mIndex<NumberOfHardcodedFlags) {
        mData.mFlags |= HardcodedFlags[mIndex];
    }

    esm.writeHNOCString("ITEX", mIcon);
    esm.writeHNOCString("PTEX", mParticle);
    esm.writeHNOCString("BSND", mBoltSound);
    esm.writeHNOCString("CSND", mCastSound);
    esm.writeHNOCString("HSND", mHitSound);
    esm.writeHNOCString("ASND", mAreaSound);
    
    esm.writeHNOCString("CVFX", mCasting);
    esm.writeHNOCString("BVFX", mBolt);
    esm.writeHNOCString("HVFX", mHit);
    esm.writeHNOCString("AVFX", mArea);
    
    esm.writeHNOString("DESC", mDescription);
}

}
