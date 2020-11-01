#ifndef GAME_MWMECHANICS_MAGICEFFECTS_H
#define GAME_MWMECHANICS_MAGICEFFECTS_H

#include <map>
#include <string>

namespace ESM
{
    struct ENAMstruct;
    struct EffectList;

    struct MagicEffects;
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
    private:
        // Note usually this would be int, but applying partial resistance might introduce a decimal point.
        float mModifier;

        int mBase;

    public:
        /// Get the total magnitude including base and modifier.
        float getMagnitude() const;

        void setModifier(float mod);
        float getModifier() const;

        /// Change mBase by \a diff
        void modifyBase(int diff);
        void setBase(int base);
        int getBase() const;

        EffectParam();

        EffectParam(float magnitude) : mModifier(magnitude), mBase(0) {}

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
        virtual ~EffectSourceVisitor() { }

        virtual void visit (EffectKey key, int effectIndex,
                            const std::string& sourceName, const std::string& sourceId, int casterActorId,
                            float magnitude, float remainingTime = -1, float totalTime = -1) = 0;
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

            void readState (const ESM::MagicEffects& state);
            void writeState (ESM::MagicEffects& state) const;

            void add (const EffectKey& key, const EffectParam& param);
            void remove (const EffectKey& key);

            void modifyBase (const EffectKey& key, int diff);

            /// Copy Modifier values from \a effects, but keep original mBase values.
            void setModifiers(const MagicEffects& effects);

            MagicEffects& operator+= (const MagicEffects& effects);

            EffectParam get (const EffectKey& key) const;
            ///< This function can safely be used for keys that are not present.

            static MagicEffects diff (const MagicEffects& prev, const MagicEffects& now);
            ///< Return changes from \a prev to \a now.
    };
}

#endif
