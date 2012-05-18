#ifndef GAME_MWMECHANICS_MAGICEFFECTS_H
#define GAME_MWMECHANICS_MAGICEFFECTS_H

#include <map>

namespace ESM
{
    struct ENAMstruct;
    struct EffectList;
}

namespace MWMechanics
{
    struct EffectKey
    {
        int mId;
        int mArg; // skill or ability

        EffectKey();

        EffectKey (int id, int arg = -1) : mId (id), mArg (arg) {}

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

            void add (const ESM::EffectList& list);

            MagicEffects& operator+= (const MagicEffects& effects);

            EffectParam get (const EffectKey& key) const;
            ///< This function can safely be used for keys that are not present.

            static MagicEffects diff (const MagicEffects& prev, const MagicEffects& now);
            ///< Return changes from \a prev to \a now.
    };
}

#endif
