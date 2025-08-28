#include "animationbindings.hpp"

#include <components/lua/luastate.hpp>
#include <components/misc/finitevalues.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/character.hpp"

#include "context.hpp"
#include "luamanagerimp.hpp"

namespace MWLua
{
    namespace
    {
        using BlendMask = MWRender::Animation::BlendMask;
        using BoneGroup = MWRender::Animation::BoneGroup;
        using Priority = MWMechanics::Priority;
        using AnimationPriorities = MWRender::Animation::AnimPriority;

        const MWWorld::Ptr& getMutablePtrOrThrow(const Object& object)
        {
            const MWWorld::Ptr& ptr = object.ptr();
            if (!ptr.getRefData().isEnabled())
                throw std::runtime_error("Can't use a disabled object");

            return ptr;
        }

        MWRender::Animation* getMutableAnimationOrThrow(const Object& object)
        {
            const MWWorld::Ptr& ptr = getMutablePtrOrThrow(object);
            auto world = MWBase::Environment::get().getWorld();
            MWRender::Animation* anim = world->getAnimation(ptr);
            if (!anim)
                throw std::runtime_error("Object has no animation");
            return anim;
        }

        const MWRender::Animation* getConstAnimationOrThrow(const Object& object)
        {
            auto world = MWBase::Environment::get().getWorld();
            const MWRender::Animation* anim = world->getAnimation(object.ptr());
            if (!anim)
                throw std::runtime_error("Object has no animation");
            return anim;
        }

        AnimationPriorities getPriorityArgument(const sol::table& args)
        {
            auto asPriorityEnum = args.get<sol::optional<Priority>>("priority");
            if (asPriorityEnum)
                return asPriorityEnum.value();

            auto asTable = args.get<sol::optional<sol::table>>("priority");
            if (asTable)
            {
                AnimationPriorities priorities = AnimationPriorities(Priority::Priority_Default);
                for (const auto& entry : asTable.value())
                {
                    if (!entry.first.is<BoneGroup>() || !entry.second.is<Priority>())
                        throw std::runtime_error("Priority table must consist of BoneGroup-Priority pairs only");
                    auto group = entry.first.as<BoneGroup>();
                    auto priority = entry.second.as<Priority>();
                    if (group < 0 || group >= BoneGroup::Num_BoneGroups)
                        throw std::runtime_error("Invalid bonegroup: " + std::to_string(group));
                    priorities[group] = priority;
                }

                return priorities;
            }

            return Priority::Priority_Default;
        }
    }

    sol::table initAnimationPackage(const Context& context)
    {
        using Misc::FiniteFloat;

        auto view = context.sol();
        auto mechanics = MWBase::Environment::get().getMechanicsManager();

        sol::table api(view, sol::create);

        api["PRIORITY"]
            = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, MWMechanics::Priority>(view,
                {
                    { "Default", MWMechanics::Priority::Priority_Default },
                    { "WeaponLowerBody", MWMechanics::Priority::Priority_WeaponLowerBody },
                    { "SneakIdleLowerBody", MWMechanics::Priority::Priority_SneakIdleLowerBody },
                    { "SwimIdle", MWMechanics::Priority::Priority_SwimIdle },
                    { "Jump", MWMechanics::Priority::Priority_Jump },
                    { "Movement", MWMechanics::Priority::Priority_Movement },
                    { "Hit", MWMechanics::Priority::Priority_Hit },
                    { "Weapon", MWMechanics::Priority::Priority_Weapon },
                    { "Block", MWMechanics::Priority::Priority_Block },
                    { "Knockdown", MWMechanics::Priority::Priority_Knockdown },
                    { "Torch", MWMechanics::Priority::Priority_Torch },
                    { "Storm", MWMechanics::Priority::Priority_Storm },
                    { "Death", MWMechanics::Priority::Priority_Death },
                    { "Scripted", MWMechanics::Priority::Priority_Scripted },
                }));

        api["BLEND_MASK"] = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, BlendMask>(view,
            {
                { "LowerBody", BlendMask::BlendMask_LowerBody },
                { "Torso", BlendMask::BlendMask_Torso },
                { "LeftArm", BlendMask::BlendMask_LeftArm },
                { "RightArm", BlendMask::BlendMask_RightArm },
                { "UpperBody", BlendMask::BlendMask_UpperBody },
                { "All", BlendMask::BlendMask_All },
            }));

        api["BONE_GROUP"] = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, BoneGroup>(view,
            {
                { "LowerBody", BoneGroup::BoneGroup_LowerBody },
                { "Torso", BoneGroup::BoneGroup_Torso },
                { "LeftArm", BoneGroup::BoneGroup_LeftArm },
                { "RightArm", BoneGroup::BoneGroup_RightArm },
            }));

        api["hasAnimation"] = [](const LObject& object) -> bool {
            return MWBase::Environment::get().getWorld()->getAnimation(object.ptr()) != nullptr;
        };

        // equivalent to MWScript's SkipAnim
        api["skipAnimationThisFrame"] = [mechanics](const SelfObject& object) {
            const MWWorld::Ptr& ptr = getMutablePtrOrThrow(object);
            // This sets a flag that is only used during the update pass, so
            // there's no need to queue
            mechanics->skipAnimation(ptr);
        };

        api["getTextKeyTime"] = [](const LObject& object, std::string_view key) -> sol::optional<float> {
            float time = getConstAnimationOrThrow(object)->getTextKeyTime(key);
            if (time >= 0.f)
                return time;
            return sol::nullopt;
        };
        api["isPlaying"] = [](const LObject& object, std::string_view groupname) {
            return getConstAnimationOrThrow(object)->isPlaying(groupname);
        };
        api["getCurrentTime"] = [](const LObject& object, std::string_view groupname) -> sol::optional<float> {
            float time = getConstAnimationOrThrow(object)->getCurrentTime(groupname);
            if (time >= 0.f)
                return time;
            return sol::nullopt;
        };
        api["isLoopingAnimation"] = [](const LObject& object, std::string_view groupname) {
            return getConstAnimationOrThrow(object)->isLoopingAnimation(groupname);
        };
        api["cancel"] = [](const SelfObject& object, std::string_view groupname) {
            return getMutableAnimationOrThrow(object)->disable(groupname);
        };
        api["setLoopingEnabled"] = [](const SelfObject& object, std::string_view groupname, bool enabled) {
            return getMutableAnimationOrThrow(object)->setLoopingEnabled(groupname, enabled);
        };
        // MWRender::Animation::getInfo can also return the current speed multiplier, but this is never used.
        api["getCompletion"] = [](const LObject& object, std::string_view groupname) -> sol::optional<float> {
            float completion = 0.f;
            if (getConstAnimationOrThrow(object)->getInfo(groupname, &completion))
                return completion;
            return sol::nullopt;
        };
        api["getLoopCount"] = [](const LObject& object, std::string groupname) -> sol::optional<size_t> {
            size_t loops = 0;
            if (getConstAnimationOrThrow(object)->getInfo(groupname, nullptr, nullptr, &loops))
                return loops;
            return sol::nullopt;
        };
        api["getSpeed"] = [](const LObject& object, std::string groupname) -> sol::optional<float> {
            float speed = 0.f;
            if (getConstAnimationOrThrow(object)->getInfo(groupname, nullptr, &speed, nullptr))
                return speed;
            return sol::nullopt;
        };
        api["setSpeed"] = [](const SelfObject& object, std::string groupname, const FiniteFloat speed) {
            getMutableAnimationOrThrow(object)->adjustSpeedMult(groupname, speed);
        };
        api["getActiveGroup"] = [](const LObject& object, MWRender::BoneGroup boneGroup) -> std::string_view {
            if (boneGroup < 0 || boneGroup >= BoneGroup::Num_BoneGroups)
                throw std::runtime_error("Invalid bonegroup: " + std::to_string(boneGroup));
            return getConstAnimationOrThrow(object)->getActiveGroup(boneGroup);
        };

        // Clears out the animation queue, and cancel any animation currently playing from the queue
        api["clearAnimationQueue"] = [mechanics](const SelfObject& object, bool clearScripted) {
            const MWWorld::Ptr& ptr = getMutablePtrOrThrow(object);
            mechanics->clearAnimationQueue(ptr, clearScripted);
        };

        // Extended variant of MWScript's PlayGroup and LoopGroup
        api["playQueued"] = sol::overload(
            [mechanics](const SelfObject& object, const std::string& groupname, const sol::table& options) {
                uint32_t numberOfLoops = options.get_or("loops", std::numeric_limits<uint32_t>::max());
                float speed = options.get_or("speed", 1.f);
                std::string startKey = options.get_or<std::string>("startKey", "start");
                std::string stopKey = options.get_or<std::string>("stopKey", "stop");
                bool forceLoop = options.get_or("forceLoop", false);

                const MWWorld::Ptr& ptr = getMutablePtrOrThrow(object);
                mechanics->playAnimationGroupLua(ptr, groupname, numberOfLoops, speed, startKey, stopKey, forceLoop);
            },
            [mechanics](const SelfObject& object, const std::string& groupname) {
                const MWWorld::Ptr& ptr = getMutablePtrOrThrow(object);
                mechanics->playAnimationGroupLua(
                    ptr, groupname, std::numeric_limits<int>::max(), 1, "start", "stop", false);
            });

        api["playBlended"] = [](const SelfObject& object, std::string_view groupName, const sol::table& options) {
            uint32_t loops = options.get_or("loops", 0u);
            MWRender::Animation::AnimPriority priority = getPriorityArgument(options);
            BlendMask blendMask = options.get_or("blendMask", BlendMask::BlendMask_All);
            bool autoDisable = options.get_or("autoDisable", true);
            float speed = options.get_or("speed", 1.0f);
            std::string start = options.get_or<std::string>("startKey", "start");
            std::string stop = options.get_or<std::string>("stopKey", "stop");
            float startPoint = options.get_or("startPoint", 0.0f);
            bool forceLoop = options.get_or("forceLoop", false);

            const std::string lowerGroup = Misc::StringUtils::lowerCase(groupName);

            auto animation = getMutableAnimationOrThrow(object);
            animation->play(lowerGroup, priority, blendMask, autoDisable, speed, start, stop, startPoint, loops,
                forceLoop || animation->isLoopingAnimation(lowerGroup));
        };

        api["hasGroup"] = [](const LObject& object, std::string_view groupname) -> bool {
            const MWRender::Animation* anim = getConstAnimationOrThrow(object);
            return anim->hasAnimation(groupname);
        };

        // Note: This checks the nodemap, and does not read the scene graph itself, and so should be thread safe.
        api["hasBone"] = [](const LObject& object, std::string_view bonename) -> bool {
            const MWRender::Animation* anim = getConstAnimationOrThrow(object);
            return anim->getNode(bonename) != nullptr;
        };

        api["addVfx"] = [context](const SelfObject& object, std::string_view model, sol::optional<sol::table> options) {
            if (options)
            {
                context.mLuaManager->addAction(
                    [object = Object(object), model = std::string(model),
                        effectId = options->get_or<std::string>("vfxId", ""), loop = options->get_or("loop", false),
                        boneName = options->get_or<std::string>("boneName", ""),
                        particleTexture = options->get_or<std::string>("particleTextureOverride", ""),
                        useAmbientLight = options->get_or("useAmbientLight", true)] {
                        MWRender::Animation* anim = getMutableAnimationOrThrow(object);

                        anim->addEffect(model, effectId, loop, boneName, particleTexture, useAmbientLight);
                    },
                    "addVfxAction");
            }
            else
            {
                context.mLuaManager->addAction(
                    [object = Object(object), model = std::string(model)] {
                        MWRender::Animation* anim = getMutableAnimationOrThrow(object);
                        anim->addEffect(model, "");
                    },
                    "addVfxAction");
            }
        };

        api["removeVfx"] = [context](const SelfObject& object, std::string_view effectId) {
            context.mLuaManager->addAction(
                [object = Object(object), effectId = std::string(effectId)] {
                    MWRender::Animation* anim = getMutableAnimationOrThrow(object);
                    anim->removeEffect(effectId);
                },
                "removeVfxAction");
        };

        api["removeAllVfx"] = [context](const SelfObject& object) {
            context.mLuaManager->addAction(
                [object = Object(object)] {
                    MWRender::Animation* anim = getMutableAnimationOrThrow(object);
                    anim->removeEffects();
                },
                "removeVfxAction");
        };

        return LuaUtil::makeReadOnly(api);
    }

    sol::table initWorldVfxBindings(const Context& context)
    {
        sol::table api(context.mLua->unsafeState(), sol::create);

        api["spawn"]
            = [context](std::string_view model, const osg::Vec3f& worldPos, sol::optional<sol::table> options) {
                  if (options)
                  {
                      bool magicVfx = options->get_or("mwMagicVfx", true);
                      std::string texture = options->get_or<std::string>("particleTextureOverride", "");
                      float scale = options->get_or("scale", 1.f);
                      bool useAmbientLight = options->get_or("useAmbientLight", true);
                      context.mLuaManager->addAction(
                          [model = VFS::Path::Normalized(model), texture = std::move(texture), worldPos, scale,
                              magicVfx, useAmbientLight]() {
                              MWBase::Environment::get().getWorld()->spawnEffect(
                                  model, texture, worldPos, scale, magicVfx, useAmbientLight);
                          },
                          "openmw.vfx.spawn");
                  }
                  else
                  {
                      context.mLuaManager->addAction(
                          [model = VFS::Path::Normalized(model), worldPos]() {
                              MWBase::Environment::get().getWorld()->spawnEffect(model, "", worldPos, 1.f);
                          },
                          "openmw.vfx.spawn");
                  }
              };

        return api;
    }
}
