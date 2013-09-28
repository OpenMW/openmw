#include "creaturestats.hpp"

namespace MWMechanics
{
    CreatureStats::CreatureStats()
    {}
    
    // Can't use all benefits of members initialization because of
    // lack of copy constructors
    CreatureStats::CreatureStats(const CreatureStats &orig)
      : mLevel(orig.mLevel), mHello(orig.mHello), mFight(orig.mFight),
      mFlee(orig.mFlee), mAlarm(orig.mAlarm)
    {
        for (int i = 0; i < 8; ++i) {
            mAttributes[i] = orig.mAttributes[i];
        }
        for (int i = 0; i < 3; ++i) {
            mDynamic[i] = orig.mDynamic[i];
        }
        mSpells = orig.mSpells;
        mActiveSpells = orig.mActiveSpells;
        mMagicEffects = orig.mMagicEffects;
    }

    CreatureStats::~CreatureStats()
    {}

    const CreatureStats &
    CreatureStats::operator=(const CreatureStats &orig)
    {
        for (int i = 0; i < 8; ++i) {
            mAttributes[i] = orig.mAttributes[i];
        }
        for (int i = 0; i < 3; ++i) {
            mDynamic[i] = orig.mDynamic[i];
        }
        mLevel = orig.mLevel;
        mSpells = orig.mSpells;
        mActiveSpells = orig.mActiveSpells;
        mMagicEffects = orig.mMagicEffects;
        mHello = orig.mHello;
        mFight = orig.mFight;
        mFlee = orig.mFlee;
        mAlarm = orig.mAlarm;

        return *this;
    }
}
