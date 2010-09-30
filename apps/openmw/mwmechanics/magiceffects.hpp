#ifndef GAME_MWMECHANICS_MAGICEFFECTS_H
#define GAME_MWMECHANICS_MAGICEFFECTS_H

#include <map>

namespace ESM
{
    struct ENAMstruct;
}

namespace MWMechanics
{
    struct EffectKey
    {
        int mId;
        int mArg; // skill or ability

        EffectKey();

        EffectKey (const ESM::ENAMstruct& effect);
    };

    bool operator< (const EffectKey& left, const EffectKey& right);

    struct EffectParam
    {
        int mMagnitude;

        EffectParam();

        EffectParam& operator+= (const EffectParam& param);

        EffectParam& operator-= (const EffectParam& param);
    };

    inline EffectParam operator+ (const EffectParam& left, const EffectParam& right)
    {
        EffectParam param (left);
        return param += right;
    }

    inline EffectParam operator- (const EffectParam& left, const EffectParam& right)
    {
        EffectParam param (left);
        return param -= right;
    }

    /// \brief Effects currently affecting a NPC or creature
    class MagicEffects
    {
        public:

            typedef std::map<EffectKey, EffectParam> Collection;

        private:

            Collection mCollection;

        public:

            Collection::const_iterator Begin() const { return mCollection.begin(); }

            Collection::const_iterator End() const { return mCollection.end(); }

            void add (const EffectKey& key, const EffectParam& param);

            static MagicEffects diff (const MagicEffects& prev, const MagicEffects& now);
            ///< Return changes from \a prev to \a now.
    };
}

#endif
