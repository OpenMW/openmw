#ifndef GAME_MWMECHANICS_MAGICEFFECTS_H
#define GAME_MWMECHANICS_MAGICEFFECTS_H

#include <map>
#include <string>

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
        // Note usually this would be int, but applying partial resistance might introduce decimal point.
        float mMagnitude;

        EffectParam();

        EffectParam(float magnitude) : mMagnitude(magnitude) {}

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

    // Used by effect management classes (ActiveSpells, InventoryStore, Spells) to list active effect sources for GUI display
    struct EffectSourceVisitor
    {
        virtual void visit (MWMechanics::EffectKey key,
                                 const std::string& sourceName, int casterActorId,
                            float magnitude, float remainingTime = -1) = 0;
    };

    /// \brief Effects currently affecting a NPC or creature
    class MagicEffects
    {
        public:

            typedef std::map<EffectKey, EffectParam> Collection;

        private:

            Collection mCollection;

        public:

            Collection::const_iterator begin() const { return mCollection.begin(); }

            Collection::const_iterator end() const { return mCollection.end(); }

            void add (const EffectKey& key, const EffectParam& param);

            MagicEffects& operator+= (const MagicEffects& effects);

            EffectParam get (const EffectKey& key) const;
            ///< This function can safely be used for keys that are not present.

            static MagicEffects diff (const MagicEffects& prev, const MagicEffects& now);
            ///< Return changes from \a prev to \a now.
    };
}

#endif
