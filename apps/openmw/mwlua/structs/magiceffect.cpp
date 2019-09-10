#include "magiceffect.hpp"

#include "../luamanager.hpp"

#include "../../mwbase/environment.hpp"
#include "../../mwbase/world.hpp"

#include "../../mwworld/esmstore.hpp"

#include <components/esm/effectlist.hpp>
#include <components/esm/loadmgef.hpp>

namespace mwse
{
    namespace lua
    {
        void bindTES3MagicEffect()
        {
            // Get our lua state.
            auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            // Binding for ESM::MagicEffect
            {
                // Start our usertype. We must finish this with state.set_usertype.
                auto usertypeDefinition = state.create_simple_usertype<ESM::MagicEffect>();
                usertypeDefinition.set("new", sol::no_constructor);

                // Basic property binding.
                usertypeDefinition.set("areaVisualEffect", &ESM::MagicEffect::mArea);
                usertypeDefinition.set("baseMagickaCost", sol::property(
                    [](ESM::MagicEffect& self) { return self.mData.mBaseCost; },
                    [](ESM::MagicEffect& self, int value) { self.mData.mBaseCost = value; }
                ));
                usertypeDefinition.set("boltVisualEffect", &ESM::MagicEffect::mBolt);
                usertypeDefinition.set("castVisualEffect", &ESM::MagicEffect::mCasting);
                usertypeDefinition.set("description", &ESM::MagicEffect::mDescription);

                // FIXME: it is unclear, why there are two flags structs in Morrowind
                usertypeDefinition.set("flags", sol::property(
                    [](ESM::MagicEffect& self) { return self.mData.mFlags; },
                    [](ESM::MagicEffect& self, double value) { self.mData.mFlags = (int) value; }
                ));
                usertypeDefinition.set("hitVisualEffect", &ESM::MagicEffect::mHit);
                usertypeDefinition.set("id", sol::readonly_property(&ESM::MagicEffect::mId));
                usertypeDefinition.set("lightingBlue", sol::property(
                    [](ESM::MagicEffect& self) { return self.mData.mBlue; },
                    [](ESM::MagicEffect& self, int value) { self.mData.mBlue = value; }
                ));
                usertypeDefinition.set("lightingGreen", sol::property(
                    [](ESM::MagicEffect& self) { return self.mData.mGreen; },
                    [](ESM::MagicEffect& self, int value) { self.mData.mGreen = value; }
                ));
                usertypeDefinition.set("lightingRed", sol::property(
                    [](ESM::MagicEffect& self) { return self.mData.mRed; },
                    [](ESM::MagicEffect& self, int value) { self.mData.mRed = value; }
                ));
                usertypeDefinition.set("school", sol::property(
                    [](ESM::MagicEffect& self) { return self.mData.mSchool; },
                    [](ESM::MagicEffect& self, int value) { self.mData.mSchool = value; }
                ));
                usertypeDefinition.set("size", sol::property(
                    [](ESM::MagicEffect& self) { return self.mData.mUnknown1; },
                    [](ESM::MagicEffect& self, double value) { self.mData.mUnknown1 = value; }
                ));
                usertypeDefinition.set("sizeCap", sol::property(
                    [](ESM::MagicEffect& self) { return self.mData.mUnknown2; },
                    [](ESM::MagicEffect& self, double value) { self.mData.mUnknown2 = value; }
                ));
                usertypeDefinition.set("speed", sol::property(
                    [](ESM::MagicEffect& self) { return self.mData.mSpeed; },
                    [](ESM::MagicEffect& self, double value) { self.mData.mSpeed = value; }
                ));

                // Allow access to base effect flags.
                usertypeDefinition.set("baseFlags", sol::property(
                    [](ESM::MagicEffect& self) { return self.mData.mFlags; },
                    [](ESM::MagicEffect& self, double value) { self.mData.mFlags = (int) value; }
                ));

                // User-friendly access to those base effects.
                usertypeDefinition.set("targetsSkills", sol::property(
                    [](ESM::MagicEffect& self) { return (self.mData.mFlags & ESM::MagicEffect::TargetSkill) != 0; },
                    [](ESM::MagicEffect& self, bool set) { set ? self.mData.mFlags |= ESM::MagicEffect::TargetSkill : self.mData.mFlags &= ~ESM::MagicEffect::TargetSkill; }
                ));
                usertypeDefinition.set("targetsAttributes", sol::property(
                    [](ESM::MagicEffect& self) { return (self.mData.mFlags & ESM::MagicEffect::TargetAttribute) != 0; },
                    [](ESM::MagicEffect& self, bool set) { set ? self.mData.mFlags |= ESM::MagicEffect::TargetSkill : self.mData.mFlags &= ~ESM::MagicEffect::TargetAttribute; }
                ));
                usertypeDefinition.set("hasNoDuration", sol::property(
                    [](ESM::MagicEffect& self) { return (self.mData.mFlags & ESM::MagicEffect::NoDuration) != 0; },
                    [](ESM::MagicEffect& self, bool set) { set ? self.mData.mFlags |= ESM::MagicEffect::TargetSkill : self.mData.mFlags &= ~ESM::MagicEffect::NoDuration; }
                ));
                usertypeDefinition.set("hasNoMagnitude", sol::property(
                    [](ESM::MagicEffect& self) { return (self.mData.mFlags & ESM::MagicEffect::NoMagnitude) != 0; },
                    [](ESM::MagicEffect& self, bool set) { set ? self.mData.mFlags |= ESM::MagicEffect::TargetSkill : self.mData.mFlags &= ~ESM::MagicEffect::NoMagnitude; }
                ));
                usertypeDefinition.set("isHarmful", sol::property(
                    [](ESM::MagicEffect& self) { return (self.mData.mFlags & ESM::MagicEffect::Harmful) != 0; },
                    [](ESM::MagicEffect& self, bool set) { set ? self.mData.mFlags |= ESM::MagicEffect::TargetSkill : self.mData.mFlags &= ~ESM::MagicEffect::Harmful; }
                ));
                usertypeDefinition.set("hasContinuousVFX", sol::property(
                    [](ESM::MagicEffect& self) { return (self.mData.mFlags & ESM::MagicEffect::ContinuousVfx) != 0; },
                    [](ESM::MagicEffect& self, bool set) { set ? self.mData.mFlags |= ESM::MagicEffect::TargetSkill : self.mData.mFlags &= ~ESM::MagicEffect::ContinuousVfx; }
                ));
                usertypeDefinition.set("canCastSelf", sol::property(
                    [](ESM::MagicEffect& self) { return (self.mData.mFlags & ESM::MagicEffect::CastSelf) != 0; },
                    [](ESM::MagicEffect& self, bool set) { set ? self.mData.mFlags |= ESM::MagicEffect::TargetSkill : self.mData.mFlags &= ~ESM::MagicEffect::CastSelf; }
                ));
                usertypeDefinition.set("canCastTouch", sol::property(
                    [](ESM::MagicEffect& self) { return (self.mData.mFlags & ESM::MagicEffect::CastTouch) != 0; },
                    [](ESM::MagicEffect& self, bool set) { set ? self.mData.mFlags |= ESM::MagicEffect::TargetSkill : self.mData.mFlags &= ~ESM::MagicEffect::CastTouch; }
                ));
                usertypeDefinition.set("canCastTarget", sol::property(
                    [](ESM::MagicEffect& self) { return (self.mData.mFlags & ESM::MagicEffect::CastTarget) != 0; },
                    [](ESM::MagicEffect& self, bool set) { set ? self.mData.mFlags |= ESM::MagicEffect::TargetSkill : self.mData.mFlags &= ~ESM::MagicEffect::CastTarget; }
                ));
                usertypeDefinition.set("allowSpellmaking", sol::property(
                    [](ESM::MagicEffect& self) { return (self.mData.mFlags & ESM::MagicEffect::AllowSpellmaking) != 0; },
                    [](ESM::MagicEffect& self, bool set) { set ? self.mData.mFlags |= ESM::MagicEffect::TargetSkill : self.mData.mFlags &= ~ESM::MagicEffect::AllowSpellmaking; }
                ));
                usertypeDefinition.set("allowEnchanting", sol::property(
                    [](ESM::MagicEffect& self) { return (self.mData.mFlags & ESM::MagicEffect::AllowEnchanting) != 0; },
                    [](ESM::MagicEffect& self, bool set) { set ? self.mData.mFlags |= ESM::MagicEffect::TargetSkill : self.mData.mFlags &= ~ESM::MagicEffect::AllowEnchanting; }
                ));
                usertypeDefinition.set("usesNegativeLighting", sol::property(
                    [](ESM::MagicEffect& self) { return (self.mData.mFlags & ESM::MagicEffect::NegativeLight) != 0; },
                    [](ESM::MagicEffect& self, bool set) { set ? self.mData.mFlags |= ESM::MagicEffect::TargetSkill : self.mData.mFlags &= ~ESM::MagicEffect::NegativeLight; }
                ));
                usertypeDefinition.set("appliesOnce", sol::property(
                    [](ESM::MagicEffect& self) { return (self.mData.mFlags & ESM::MagicEffect::UncappedDamage) != 0; },
                    [](ESM::MagicEffect& self, bool set) { set ? self.mData.mFlags |= ESM::MagicEffect::TargetSkill : self.mData.mFlags &= ~ESM::MagicEffect::UncappedDamage; }
                ));
                usertypeDefinition.set("nonRecastable", sol::property(
                    [](ESM::MagicEffect& self) { return (self.mData.mFlags & ESM::MagicEffect::NonRecastable) != 0; },
                    [](ESM::MagicEffect& self, bool set) { set ? self.mData.mFlags |= ESM::MagicEffect::TargetSkill : self.mData.mFlags &= ~ESM::MagicEffect::NonRecastable; }
                ));
                usertypeDefinition.set("illegalDaedra", sol::property(
                    [](ESM::MagicEffect& self) { return (self.mData.mFlags & ESM::MagicEffect::IllegalDaedra) != 0; },
                    [](ESM::MagicEffect& self, bool set) { set ? self.mData.mFlags |= ESM::MagicEffect::TargetSkill : self.mData.mFlags &= ~ESM::MagicEffect::IllegalDaedra; }
                ));
                usertypeDefinition.set("unreflectable", sol::property(
                    [](ESM::MagicEffect& self) { return (self.mData.mFlags & ESM::MagicEffect::Unreflectable) != 0; },
                    [](ESM::MagicEffect& self, bool set) { set ? self.mData.mFlags |= ESM::MagicEffect::TargetSkill : self.mData.mFlags &= ~ESM::MagicEffect::Unreflectable; }
                ));
                usertypeDefinition.set("casterLinked", sol::property(
                    [](ESM::MagicEffect& self) { return (self.mData.mFlags & ESM::MagicEffect::CasterLinked) != 0; },
                    [](ESM::MagicEffect& self, bool set) { set ? self.mData.mFlags |= ESM::MagicEffect::TargetSkill : self.mData.mFlags &= ~ESM::MagicEffect::CasterLinked; }
                ));

                // FIXME: effect name
                usertypeDefinition.set("name", sol::readonly_property([](ESM::MagicEffect& self) { return ""; }));

                usertypeDefinition.set("areaSoundEffect", &ESM::MagicEffect::mAreaSound);
                usertypeDefinition.set("boltSoundEffect", &ESM::MagicEffect::mBoltSound);
                usertypeDefinition.set("castSoundEffect", &ESM::MagicEffect::mCastSound);
                usertypeDefinition.set("hitSoundEffect", &ESM::MagicEffect::mHitSound);
                usertypeDefinition.set("icon", sol::property(
                    [](ESM::MagicEffect& self) { return self.mIcon; },
                    [](ESM::MagicEffect& self, const char* value) { if (strlen(value) < 32) self.mIcon = value; }
                ));
                usertypeDefinition.set("particleTexture", sol::readonly_property([](ESM::MagicEffect& self) { return self.mParticle; }));

                // Finish up our usertype.
                state.set_usertype("tes3magicEffect", usertypeDefinition);
            }

            // Binding for ESM::ENAMstruct
            {
                // Start our usertype. We must finish this with state.set_usertype.
                auto usertypeDefinition = state.create_simple_usertype<ESM::ENAMstruct>();
                usertypeDefinition.set("new", sol::no_constructor);

                // Convert to string.
                // TODO: implement to_String for mapped structures
                usertypeDefinition.set(sol::meta_function::to_string, [](ESM::ENAMstruct& self) -> sol::optional<std::string>
                {
                    if (self.mEffectID == -1) {
                        return sol::optional<std::string>();
                    }
                    //return self.toString();
                    return std::string("");
                });

                // Basic property binding.
                usertypeDefinition.set("attribute", &ESM::ENAMstruct::mAttribute);
                usertypeDefinition.set("duration", &ESM::ENAMstruct::mDuration);
                usertypeDefinition.set("id", &ESM::ENAMstruct::mEffectID);
                usertypeDefinition.set("max", &ESM::ENAMstruct::mMagnMax);
                usertypeDefinition.set("min", &ESM::ENAMstruct::mMagnMin);
                usertypeDefinition.set("radius", &ESM::ENAMstruct::mArea);
                usertypeDefinition.set("rangeType", &ESM::ENAMstruct::mRange);
                usertypeDefinition.set("skill", &ESM::ENAMstruct::mSkill);

                // Allow easy access to the base magic effect.
                usertypeDefinition.set("object", sol::readonly_property([](ESM::ENAMstruct& self)
                {
                    const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
                    const ESM::MagicEffect *effect = store.get<ESM::MagicEffect>().find(self.mEffectID);
                    return effect;
                }));

                // Finish up our usertype.
                state.set_usertype("tes3effect", usertypeDefinition);
            }
        }
    }
}
