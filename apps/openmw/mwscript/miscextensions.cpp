#include "miscextensions.hpp"

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <sstream>

#include <components/compiler/extensions.hpp>
#include <components/compiler/locals.hpp>
#include <components/compiler/opcodes.hpp>

#include <components/debug/debuglog.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/opcodes.hpp>
#include <components/interpreter/runtime.hpp>

#include <components/misc/resourcehelpers.hpp>
#include <components/misc/rng.hpp>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/positionattitudetransform.hpp>

#include <components/esm3/loadacti.hpp>
#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadappa.hpp>
#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadbody.hpp>
#include <components/esm3/loadbook.hpp>
#include <components/esm3/loadclot.hpp>
#include <components/esm3/loadcont.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loaddoor.hpp>
#include <components/esm3/loadingr.hpp>
#include <components/esm3/loadlevlist.hpp>
#include <components/esm3/loadligh.hpp>
#include <components/esm3/loadlock.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadmisc.hpp>
#include <components/esm3/loadprob.hpp>
#include <components/esm3/loadrepa.hpp>
#include <components/esm3/loadscpt.hpp>
#include <components/esm3/loadstat.hpp>
#include <components/esm3/loadweap.hpp>

#include <components/files/conversion.hpp>
#include <components/misc/strings/conversion.hpp>
#include <components/sceneutil/util.hpp>
#include <components/vfs/manager.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/player.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/aicast.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/spellcasting.hpp"

#include "../mwrender/animation.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace
{

    struct TextureFetchVisitor : osg::NodeVisitor
    {
        std::vector<std::pair<std::string, std::string>> mTextures;

        TextureFetchVisitor(osg::NodeVisitor::TraversalMode mode = TRAVERSE_ALL_CHILDREN)
            : osg::NodeVisitor(mode)
        {
        }

        void apply(osg::Node& node) override
        {
            const osg::StateSet* stateset = node.getStateSet();
            if (stateset)
            {
                const osg::StateSet::TextureAttributeList& texAttributes = stateset->getTextureAttributeList();
                for (size_t i = 0; i < texAttributes.size(); i++)
                {
                    const osg::StateAttribute* attr = stateset->getTextureAttribute(i, osg::StateAttribute::TEXTURE);
                    if (!attr)
                        continue;
                    const osg::Texture* texture = attr->asTexture();
                    if (!texture)
                        continue;
                    const osg::Image* image = texture->getImage(0);
                    std::string fileName;
                    if (image)
                        fileName = image->getFileName();
                    mTextures.emplace_back(SceneUtil::getTextureType(*stateset, *texture, i), fileName);
                }
            }

            traverse(node);
        }
    };

    void addToLevList(ESM::LevelledListBase* list, const ESM::RefId& itemId, int level)
    {
        for (auto& levelItem : list->mList)
        {
            if (levelItem.mLevel == level && itemId == levelItem.mId)
                return;
        }

        ESM::LevelledListBase::LevelItem item;
        item.mId = itemId;
        item.mLevel = level;
        list->mList.push_back(item);
    }

    void removeFromLevList(ESM::LevelledListBase* list, const ESM::RefId& itemId, int level)
    {
        // level of -1 removes all items with that itemId
        for (std::vector<ESM::LevelledListBase::LevelItem>::iterator it = list->mList.begin(); it != list->mList.end();)
        {
            if (level != -1 && it->mLevel != level)
            {
                ++it;
                continue;
            }
            if (itemId == it->mId)
                it = list->mList.erase(it);
            else
                ++it;
        }
    }

}

namespace MWScript
{
    namespace Misc
    {
        class OpMenuMode : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                runtime.push(MWBase::Environment::get().getWindowManager()->isGuiMode());
            }
        };

        class OpRandom : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                Interpreter::Type_Integer limit = runtime[0].mInteger;
                runtime.pop();

                if (limit < 0)
                    throw std::runtime_error("random: argument out of range (Don't be so negative!)");

                auto& prng = MWBase::Environment::get().getWorld()->getPrng();
                runtime.push(static_cast<Interpreter::Type_Float>(::Misc::Rng::rollDice(limit, prng))); // [o, limit)
            }
        };

        template <class R>
        class OpStartScript : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr target = R()(runtime, false);
                ESM::RefId name = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                if (!MWBase::Environment::get().getESMStore()->get<ESM::Script>().search(name))
                {
                    runtime.getContext().report(
                        "Failed to start global script '" + name.getRefIdString() + "': script record not found");
                    return;
                }

                MWBase::Environment::get().getScriptManager()->getGlobalScripts().addScript(name, target);
            }
        };

        class OpScriptRunning : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                const ESM::RefId& name = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();
                runtime.push(MWBase::Environment::get().getScriptManager()->getGlobalScripts().isRunning(name));
            }
        };

        class OpStopScript : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                const ESM::RefId& name = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                if (!MWBase::Environment::get().getESMStore()->get<ESM::Script>().search(name))
                {
                    runtime.getContext().report(
                        "Failed to stop global script '" + name.getRefIdString() + "': script record not found");
                    return;
                }

                MWBase::Environment::get().getScriptManager()->getGlobalScripts().removeScript(name);
            }
        };

        class OpGetSecondsPassed : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                runtime.push(MWBase::Environment::get().getFrameDuration());
            }
        };

        template <class R>
        class OpEnable : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                MWBase::Environment::get().getWorld()->enable(ptr);
            }
        };

        template <class R>
        class OpDisable : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                MWBase::Environment::get().getWorld()->disable(ptr);
            }
        };

        template <class R>
        class OpGetDisabled : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                runtime.push(!ptr.getRefData().isEnabled());
            }
        };

        class OpPlayBink : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                std::string_view name = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();

                bool allowSkipping = runtime[0].mInteger != 0;
                runtime.pop();

                MWBase::Environment::get().getWindowManager()->playVideo(name, allowSkipping);
            }
        };

        class OpGetPcSleep : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                runtime.push(MWBase::Environment::get().getWindowManager()->getPlayerSleeping());
            }
        };

        class OpGetPcJumping : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWBase::World* world = MWBase::Environment::get().getWorld();
                runtime.push(world->getPlayer().getJumping());
            }
        };

        class OpWakeUpPc : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWBase::Environment::get().getWindowManager()->wakeUpPlayer();
            }
        };

        class OpXBox : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override { runtime.push(0); }
        };

        template <class R>
        class OpOnActivate : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                runtime.push(ptr.getRefData().onActivate());
            }
        };

        template <class R>
        class OpActivate : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                InterpreterContext& context = static_cast<InterpreterContext&>(runtime.getContext());

                MWWorld::Ptr ptr = R()(runtime);

                if (ptr.getRefData().activateByScript() || ptr.getContainerStore())
                    context.executeActivation(ptr, MWMechanics::getPlayer());
            }
        };

        template <class R>
        class OpLock : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Integer lockLevel = ptr.getCellRef().getLockLevel();
                if (lockLevel == 0)
                { // no lock level was ever set, set to 100 as default
                    lockLevel = 100;
                }

                if (arg0 == 1)
                {
                    lockLevel = runtime[0].mInteger;
                    runtime.pop();
                }

                ptr.getCellRef().lock(lockLevel);

                // Instantly reset door to closed state
                // This is done when using Lock in scripts, but not when using Lock spells.
                if (ptr.getType() == ESM::Door::sRecordId && !ptr.getCellRef().getTeleport())
                {
                    MWBase::Environment::get().getWorld()->activateDoor(ptr, MWWorld::DoorState::Idle);
                }
            }
        };

        template <class R>
        class OpUnlock : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                if (ptr.getCellRef().isLocked())
                    ptr.getCellRef().unlock();
            }
        };

        class OpToggleCollisionDebug : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                bool enabled = MWBase::Environment::get().getWorld()->toggleRenderMode(MWRender::Render_CollisionDebug);

                runtime.getContext().report(
                    enabled ? "Collision Mesh Rendering -> On" : "Collision Mesh Rendering -> Off");
            }
        };

        class OpToggleCollisionBoxes : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                bool enabled = MWBase::Environment::get().getWorld()->toggleRenderMode(MWRender::Render_CollisionDebug);

                runtime.getContext().report(
                    enabled ? "Collision Mesh Rendering -> On" : "Collision Mesh Rendering -> Off");
            }
        };

        class OpToggleWireframe : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                bool enabled = MWBase::Environment::get().getWorld()->toggleRenderMode(MWRender::Render_Wireframe);

                runtime.getContext().report(enabled ? "Wireframe Rendering -> On" : "Wireframe Rendering -> Off");
            }
        };

        class OpToggleBorders : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                bool enabled = MWBase::Environment::get().getWorld()->toggleBorders();

                runtime.getContext().report(enabled ? "Border Rendering -> On" : "Border Rendering -> Off");
            }
        };

        class OpTogglePathgrid : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                bool enabled = MWBase::Environment::get().getWorld()->toggleRenderMode(MWRender::Render_Pathgrid);

                runtime.getContext().report(enabled ? "Path Grid rendering -> On" : "Path Grid Rendering -> Off");
            }
        };

        class OpFadeIn : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                Interpreter::Type_Float time = runtime[0].mFloat;
                runtime.pop();

                MWBase::Environment::get().getWindowManager()->fadeScreenIn(time, false);
            }
        };

        class OpFadeOut : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                Interpreter::Type_Float time = runtime[0].mFloat;
                runtime.pop();

                MWBase::Environment::get().getWindowManager()->fadeScreenOut(time, false);
            }
        };

        class OpFadeTo : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                Interpreter::Type_Float alpha = runtime[0].mFloat;
                runtime.pop();

                Interpreter::Type_Float time = runtime[0].mFloat;
                runtime.pop();

                MWBase::Environment::get().getWindowManager()->fadeScreenTo(static_cast<int>(alpha), time, false);
            }
        };

        class OpToggleWater : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                runtime.getContext().report(
                    MWBase::Environment::get().getWorld()->toggleWater() ? "Water -> On" : "Water -> Off");
            }
        };

        class OpToggleWorld : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                runtime.getContext().report(
                    MWBase::Environment::get().getWorld()->toggleWorld() ? "World -> On" : "World -> Off");
            }
        };

        class OpDontSaveObject : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                // We are ignoring the DontSaveObject statement for now. Probably not worth
                // bothering with. The incompatibility we are creating should be marginal at most.
            }
        };

        class OpPcForce1stPerson : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                if (!MWBase::Environment::get().getWorld()->isFirstPerson())
                    MWBase::Environment::get().getWorld()->togglePOV(true);
            }
        };

        class OpPcForce3rdPerson : public Interpreter::Opcode0
        {
            void execute(Interpreter::Runtime& runtime) override
            {
                if (MWBase::Environment::get().getWorld()->isFirstPerson())
                    MWBase::Environment::get().getWorld()->togglePOV(true);
            }
        };

        class OpPcGet3rdPerson : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                runtime.push(!MWBase::Environment::get().getWorld()->isFirstPerson());
            }
        };

        class OpToggleVanityMode : public Interpreter::Opcode0
        {
            static bool sActivate;

        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWBase::World* world = MWBase::Environment::get().getWorld();

                if (world->toggleVanityMode(sActivate))
                {
                    runtime.getContext().report(sActivate ? "Vanity Mode -> On" : "Vanity Mode -> Off");
                    sActivate = !sActivate;
                }
                else
                {
                    runtime.getContext().report("Vanity Mode -> No");
                }
            }
        };
        bool OpToggleVanityMode::sActivate = true;

        template <class R>
        class OpGetLocked : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                runtime.push(ptr.getCellRef().isLocked());
            }
        };

        template <class R>
        class OpGetEffect : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                std::string_view effect = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();

                if (!ptr.getClass().isActor())
                {
                    runtime.push(0);
                    return;
                }

                long key;

                if (const auto k = ::Misc::StringUtils::toNumeric<long>(effect.data());
                    k.has_value() && *k >= 0 && *k <= 32767)
                    key = *k;
                else
                    key = ESM::MagicEffect::effectGmstIdToIndex(effect);

                const MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
                for (const auto& spell : stats.getActiveSpells())
                {
                    for (const auto& effect : spell.getEffects())
                    {
                        if (effect.mFlags & ESM::ActiveEffect::Flag_Applied && effect.mEffectId == key)
                        {
                            runtime.push(1);
                            return;
                        }
                    }
                }
                runtime.push(0);
            }
        };

        template <class R>
        class OpAddSoulGem : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId creature = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                ESM::RefId gem = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                if (!ptr.getClass().isActor())
                    return;

                const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
                store.get<ESM::Creature>().find(
                    creature); // This line throws an exception if it can't find the creature

                MWWorld::Ptr item = *ptr.getClass().getContainerStore(ptr).add(gem, 1);

                // Set the soul on just one of the gems, not the whole stack
                item.getContainerStore()->unstack(item);
                item.getCellRef().setSoul(creature);

                // Restack the gem with other gems with the same soul
                item.getContainerStore()->restack(item);
            }
        };

        template <class R>
        class OpRemoveSoulGem : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId soul = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                // throw away additional arguments
                for (unsigned int i = 0; i < arg0; ++i)
                    runtime.pop();

                if (!ptr.getClass().isActor())
                    return;

                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
                for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
                {
                    if (it->getCellRef().getSoul() == soul)
                    {
                        store.remove(*it, 1);
                        return;
                    }
                }
            }
        };

        template <class R>
        class OpDrop : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {

                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId item = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                Interpreter::Type_Integer amount = runtime[0].mInteger;
                runtime.pop();

                if (amount < 0)
                    throw std::runtime_error("amount must be non-negative");

                // no-op
                if (amount == 0)
                    return;

                if (!ptr.getClass().isActor())
                    return;

                MWWorld::InventoryStore* invStorePtr = nullptr;
                if (ptr.getClass().hasInventoryStore(ptr))
                {
                    invStorePtr = &ptr.getClass().getInventoryStore(ptr);
                    // Prefer dropping unequipped items first; re-stack if possible by unequipping items before dropping
                    // them.
                    int numNotEquipped = invStorePtr->count(item);
                    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
                    {
                        MWWorld::ConstContainerStoreIterator it = invStorePtr->getSlot(slot);
                        if (it != invStorePtr->end() && it->getCellRef().getRefId() == item)
                        {
                            numNotEquipped -= it->getCellRef().getCount();
                        }
                    }

                    for (int slot = 0; slot < MWWorld::InventoryStore::Slots && amount > numNotEquipped; ++slot)
                    {
                        MWWorld::ContainerStoreIterator it = invStorePtr->getSlot(slot);
                        if (it != invStorePtr->end() && it->getCellRef().getRefId() == item)
                        {
                            int numToRemove = std::min(amount - numNotEquipped, it->getCellRef().getCount());
                            invStorePtr->unequipItemQuantity(*it, numToRemove);
                            numNotEquipped += numToRemove;
                        }
                    }
                }

                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
                for (MWWorld::ContainerStoreIterator iter(store.begin()); iter != store.end(); ++iter)
                {
                    if (iter->getCellRef().getRefId() == item && (!invStorePtr || !invStorePtr->isEquipped(*iter)))
                    {
                        int removed = store.remove(*iter, amount);
                        MWWorld::Ptr dropped
                            = MWBase::Environment::get().getWorld()->dropObjectOnGround(ptr, *iter, removed);
                        dropped.getCellRef().setOwner(ESM::RefId());

                        amount -= removed;

                        if (amount <= 0)
                            break;
                    }
                }

                MWWorld::ManualRef ref(*MWBase::Environment::get().getESMStore(), item, 1);
                MWWorld::Ptr itemPtr(ref.getPtr());
                if (amount > 0)
                {
                    if (itemPtr.getClass().getScript(itemPtr).empty())
                    {
                        MWBase::Environment::get().getWorld()->dropObjectOnGround(ptr, itemPtr, amount);
                    }
                    else
                    {
                        // Dropping one item per time to prevent making stacks of scripted items
                        for (int i = 0; i < amount; i++)
                            MWBase::Environment::get().getWorld()->dropObjectOnGround(ptr, itemPtr, 1);
                    }
                }

                MWBase::Environment::get().getSoundManager()->playSound3D(
                    ptr, itemPtr.getClass().getDownSoundId(itemPtr), 1.f, 1.f);
            }
        };

        template <class R>
        class OpDropSoulGem : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {

                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId soul = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                if (!ptr.getClass().isActor())
                    return;

                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);

                for (MWWorld::ContainerStoreIterator iter(store.begin()); iter != store.end(); ++iter)
                {
                    if (iter->getCellRef().getSoul() == soul)
                    {
                        MWBase::Environment::get().getWorld()->dropObjectOnGround(ptr, *iter, 1);
                        store.remove(*iter, 1);
                        break;
                    }
                }
            }
        };

        template <class R>
        class OpGetAttacked : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                runtime.push(ptr.getClass().getCreatureStats(ptr).getAttacked());
            }
        };

        template <class R>
        class OpGetWeaponDrawn : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                auto& cls = ptr.getClass();
                if (!cls.hasInventoryStore(ptr) && !cls.isBipedal(ptr))
                {
                    runtime.push(0);
                    return;
                }

                if (cls.getCreatureStats(ptr).getDrawState() != MWMechanics::DrawState::Weapon)
                {
                    runtime.push(0);
                    return;
                }

                MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);
                runtime.push(anim && anim->getWeaponsShown());
            }
        };

        template <class R>
        class OpGetSpellReadied : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                runtime.push(ptr.getClass().getCreatureStats(ptr).getDrawState() == MWMechanics::DrawState::Spell);
            }
        };

        template <class R>
        class OpGetSpellEffects : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                ESM::RefId id = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                if (!ptr.getClass().isActor())
                {
                    runtime.push(0);
                    return;
                }

                const MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
                runtime.push(stats.getActiveSpells().isSpellActive(id));
            }
        };

        class OpGetCurrentTime : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                runtime.push(MWBase::Environment::get().getWorld()->getTimeStamp().getHour());
            }
        };

        template <class R>
        class OpSetDelete : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                int parameter = runtime[0].mInteger;
                runtime.pop();

                if (parameter == 1)
                    MWBase::Environment::get().getWorld()->deleteObject(ptr);
                else if (parameter == 0)
                    MWBase::Environment::get().getWorld()->undeleteObject(ptr);
                else
                    throw std::runtime_error("SetDelete: unexpected parameter");
            }
        };

        class OpGetSquareRoot : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                float param = runtime[0].mFloat;
                runtime.pop();

                if (param < 0)
                    throw std::runtime_error("square root of negative number (we aren't that imaginary)");

                runtime.push(std::sqrt(param));
            }
        };

        template <class R>
        class OpFall : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override {}
        };

        template <class R>
        class OpGetStandingPc : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                runtime.push(MWBase::Environment::get().getWorld()->getPlayerStandingOn(ptr));
            }
        };

        template <class R>
        class OpGetStandingActor : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                runtime.push(MWBase::Environment::get().getWorld()->getActorStandingOn(ptr));
            }
        };

        template <class R>
        class OpGetCollidingPc : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                runtime.push(MWBase::Environment::get().getWorld()->getPlayerCollidingWith(ptr));
            }
        };

        template <class R>
        class OpGetCollidingActor : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                runtime.push(MWBase::Environment::get().getWorld()->getActorCollidingWith(ptr));
            }
        };

        template <class R>
        class OpHurtStandingActor : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                float healthDiffPerSecond = runtime[0].mFloat;
                runtime.pop();

                MWBase::Environment::get().getWorld()->hurtStandingActors(ptr, healthDiffPerSecond);
            }
        };

        template <class R>
        class OpHurtCollidingActor : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                float healthDiffPerSecond = runtime[0].mFloat;
                runtime.pop();

                MWBase::Environment::get().getWorld()->hurtCollidingActors(ptr, healthDiffPerSecond);
            }
        };

        class OpGetWindSpeed : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                runtime.push(MWBase::Environment::get().getWorld()->getWindSpeed());
            }
        };

        template <class R>
        class OpHitOnMe : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId objectID = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
                bool hit = objectID == stats.getLastHitObject();
                runtime.push(hit);
                if (hit)
                    stats.clearLastHitObject();
            }
        };

        template <class R>
        class OpHitAttemptOnMe : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId objectID = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
                bool hit = objectID == stats.getLastHitAttemptObject();
                runtime.push(hit);
                if (hit)
                    stats.clearLastHitAttemptObject();
            }
        };

        template <bool Enable>
        class OpEnableTeleporting : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWBase::World* world = MWBase::Environment::get().getWorld();
                world->enableTeleporting(Enable);
            }
        };

        template <bool Enable>
        class OpEnableLevitation : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWBase::World* world = MWBase::Environment::get().getWorld();
                world->enableLevitation(Enable);
            }
        };

        template <class R>
        class OpShow : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime, false);
                std::string_view var = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();

                std::stringstream output;

                if (!ptr.isEmpty())
                {
                    ESM::RefId script = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                    if (!script.empty())
                    {
                        const Compiler::Locals& locals
                            = MWBase::Environment::get().getScriptManager()->getLocals(script);
                        char type = locals.getType(var);
                        std::string refId = ptr.getCellRef().getRefId().getRefIdString();
                        if (refId.find(' ') != std::string::npos)
                            refId = '"' + refId + '"';
                        switch (type)
                        {
                            case 'l':
                            case 's':
                                output << refId << "." << var << " = "
                                       << ptr.getRefData().getLocals().getIntVar(script, var);
                                break;
                            case 'f':
                                output << refId << "." << var << " = "
                                       << ptr.getRefData().getLocals().getFloatVar(script, var);
                                break;
                        }
                    }
                }
                if (output.rdbuf()->in_avail() == 0)
                {
                    MWBase::World* world = MWBase::Environment::get().getWorld();
                    char type = world->getGlobalVariableType(var);

                    switch (type)
                    {
                        case 's':
                            output << var << " = " << runtime.getContext().getGlobalShort(var);
                            break;
                        case 'l':
                            output << var << " = " << runtime.getContext().getGlobalLong(var);
                            break;
                        case 'f':
                            output << var << " = " << runtime.getContext().getGlobalFloat(var);
                            break;
                        default:
                            output << "unknown variable";
                    }
                }
                runtime.getContext().report(output.str());
            }
        };

        template <class R>
        class OpShowVars : public Interpreter::Opcode0
        {
            void printLocalVars(Interpreter::Runtime& runtime, const MWWorld::Ptr& ptr)
            {
                std::stringstream str;

                const ESM::RefId& script = ptr.getClass().getScript(ptr);
                if (script.empty())
                    str << ptr.getCellRef().getRefId() << " does not have a script.";
                else
                {
                    str << "Local variables for " << ptr.getCellRef().getRefId();

                    const Locals& locals = ptr.getRefData().getLocals();
                    const Compiler::Locals& complocals
                        = MWBase::Environment::get().getScriptManager()->getLocals(script);

                    const std::vector<std::string>* names = &complocals.get('s');
                    for (size_t i = 0; i < names->size(); ++i)
                    {
                        if (i >= locals.mShorts.size())
                            break;
                        str << std::endl << "  " << (*names)[i] << " = " << locals.mShorts[i] << " (short)";
                    }
                    names = &complocals.get('l');
                    for (size_t i = 0; i < names->size(); ++i)
                    {
                        if (i >= locals.mLongs.size())
                            break;
                        str << std::endl << "  " << (*names)[i] << " = " << locals.mLongs[i] << " (long)";
                    }
                    names = &complocals.get('f');
                    for (size_t i = 0; i < names->size(); ++i)
                    {
                        if (i >= locals.mFloats.size())
                            break;
                        str << std::endl << "  " << (*names)[i] << " = " << locals.mFloats[i] << " (float)";
                    }
                }

                runtime.getContext().report(str.str());
            }

            void printGlobalVars(Interpreter::Runtime& runtime)
            {
                std::stringstream str;
                str << "Global variables:";

                MWBase::World* world = MWBase::Environment::get().getWorld();
                std::vector<std::string> names = runtime.getContext().getGlobals();
                for (size_t i = 0; i < names.size(); ++i)
                {
                    char type = world->getGlobalVariableType(names[i]);
                    str << std::endl << " " << names[i] << " = ";

                    switch (type)
                    {
                        case 's':

                            str << runtime.getContext().getGlobalShort(names[i]) << " (short)";
                            break;

                        case 'l':

                            str << runtime.getContext().getGlobalLong(names[i]) << " (long)";
                            break;

                        case 'f':

                            str << runtime.getContext().getGlobalFloat(names[i]) << " (float)";
                            break;

                        default:

                            str << "<unknown type>";
                    }
                }

                runtime.getContext().report(str.str());
            }

        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime, false);
                if (!ptr.isEmpty())
                    printLocalVars(runtime, ptr);
                else
                {
                    // No reference, no problem.
                    printGlobalVars(runtime);
                }
            }
        };

        class OpToggleScripts : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                bool enabled = MWBase::Environment::get().getWorld()->toggleScripts();

                runtime.getContext().report(enabled ? "Scripts -> On" : "Scripts -> Off");
            }
        };

        class OpToggleGodMode : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                bool enabled = MWBase::Environment::get().getWorld()->toggleGodMode();

                runtime.getContext().report(enabled ? "God Mode -> On" : "God Mode -> Off");
            }
        };

        template <class R>
        class OpCast : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId spellId = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                ESM::RefId targetId = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                const ESM::Spell* spell = MWBase::Environment::get().getESMStore()->get<ESM::Spell>().search(spellId);
                if (!spell)
                {
                    runtime.getContext().report(
                        "spellcasting failed: cannot find spell \"" + spellId.getRefIdString() + "\"");
                    return;
                }

                if (ptr == MWMechanics::getPlayer())
                {
                    MWBase::Environment::get().getWorld()->getPlayer().setSelectedSpell(spell->mId);
                    return;
                }

                if (ptr.getClass().isActor())
                {
                    if (!MWBase::Environment::get().getMechanicsManager()->isCastingSpell(ptr))
                    {
                        MWMechanics::AiCast castPackage(targetId, spell->mId, true);
                        ptr.getClass().getCreatureStats(ptr).getAiSequence().stack(castPackage, ptr);
                    }
                    return;
                }

                MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtr(targetId, false, false);
                if (target.isEmpty())
                    return;

                MWMechanics::CastSpell cast(ptr, target, false, true);
                cast.playSpellCastingEffects(spell);
                cast.mHitPosition = target.getRefData().getPosition().asVec3();
                cast.mAlwaysSucceed = true;
                cast.cast(spell);
            }
        };

        template <class R>
        class OpExplodeSpell : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId spellId = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                const ESM::Spell* spell = MWBase::Environment::get().getESMStore()->get<ESM::Spell>().search(spellId);
                if (!spell)
                {
                    runtime.getContext().report(
                        "spellcasting failed: cannot find spell \"" + spellId.getRefIdString() + "\"");
                    return;
                }

                if (ptr == MWMechanics::getPlayer())
                {
                    MWBase::Environment::get().getWorld()->getPlayer().setSelectedSpell(spell->mId);
                    return;
                }

                if (ptr.getClass().isActor())
                {
                    if (!MWBase::Environment::get().getMechanicsManager()->isCastingSpell(ptr))
                    {
                        MWMechanics::AiCast castPackage(ptr.getCellRef().getRefId(), spell->mId, true);
                        ptr.getClass().getCreatureStats(ptr).getAiSequence().stack(castPackage, ptr);
                    }
                    return;
                }

                MWMechanics::CastSpell cast(ptr, ptr, false, true);
                cast.mHitPosition = ptr.getRefData().getPosition().asVec3();
                cast.mAlwaysSucceed = true;
                cast.cast(spell);
            }
        };

        class OpGoToJail : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWBase::World* world = MWBase::Environment::get().getWorld();
                world->goToJail();
            }
        };

        class OpPayFine : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr player = MWMechanics::getPlayer();
                player.getClass().getNpcStats(player).setBounty(0);
                MWBase::World* world = MWBase::Environment::get().getWorld();
                world->confiscateStolenItems(player);
                world->getPlayer().recordCrimeId();
                world->getPlayer().setDrawState(MWMechanics::DrawState::Nothing);
            }
        };

        class OpPayFineThief : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr player = MWMechanics::getPlayer();
                player.getClass().getNpcStats(player).setBounty(0);
                MWBase::Environment::get().getWorld()->getPlayer().recordCrimeId();
            }
        };

        class OpGetPcInJail : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                runtime.push(MWBase::Environment::get().getWorld()->isPlayerInJail());
            }
        };

        class OpGetPcTraveling : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                runtime.push(MWBase::Environment::get().getWorld()->isPlayerTraveling());
            }
        };

        template <class R>
        class OpBetaComment : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                std::stringstream msg;

                msg << "Report time: ";

                std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                tm timeinfo{};
#ifdef _WIN32
                gmtime_s(&timeinfo, &currentTime);
#else
                gmtime_r(&currentTime, &timeinfo);
#endif
                msg << std::put_time(&timeinfo, "%Y.%m.%d %T UTC") << std::endl;

                msg << "Content file: " << ptr.getCellRef().getRefNum().mContentFile;

                if (!ptr.getCellRef().hasContentFile())
                    msg << " [None]" << std::endl;
                else
                {
                    std::vector<std::string> contentFiles = MWBase::Environment::get().getWorld()->getContentFiles();

                    msg << " [" << contentFiles.at(ptr.getCellRef().getRefNum().mContentFile) << "]" << std::endl;
                }

                msg << "RefNum: " << ptr.getCellRef().getRefNum().mIndex << std::endl;

                if (ptr.getRefData().isDeletedByContentFile())
                    msg << "[Deleted by content file]" << std::endl;
                if (!ptr.getCellRef().getCount())
                    msg << "[Deleted]" << std::endl;

                msg << "RefID: " << ptr.getCellRef().getRefId() << std::endl;
                msg << "Memory address: " << ptr.getBase() << std::endl;

                if (ptr.isInCell())
                {
                    MWWorld::CellStore* cell = ptr.getCell();
                    msg << "Cell: " << MWBase::Environment::get().getWorld()->getCellName(cell) << std::endl;
                    if (cell->getCell()->isExterior())
                        msg << "Grid: " << cell->getCell()->getGridX() << " " << cell->getCell()->getGridY()
                            << std::endl;
                    osg::Vec3f pos(ptr.getRefData().getPosition().asVec3());
                    msg << "Coordinates: " << pos.x() << " " << pos.y() << " " << pos.z() << std::endl;
                    auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();
                    const VFS::Path::Normalized model(
                        ::Misc::ResourceHelpers::correctActorModelPath(ptr.getClass().getCorrectedModel(ptr), vfs));
                    msg << "Model: " << model.value() << std::endl;
                    if (!model.empty())
                    {
                        const std::string archive = vfs->getArchive(model);
                        if (!archive.empty())
                            msg << "(" << archive << ")" << std::endl;
                        TextureFetchVisitor visitor;
                        SceneUtil::PositionAttitudeTransform* baseNode = ptr.getRefData().getBaseNode();
                        if (baseNode)
                            baseNode->accept(visitor);
                        // The instance might not have a physical model due to paging or scripting.
                        // If this is the case, fall back to the template
                        if (visitor.mTextures.empty())
                        {
                            Resource::SceneManager* sceneManager
                                = MWBase::Environment::get().getResourceSystem()->getSceneManager();
                            const_cast<osg::Node*>(sceneManager->getTemplate(model).get())->accept(visitor);
                            msg << "Bound textures: [None]" << std::endl;
                            if (!visitor.mTextures.empty())
                                msg << "Model textures: ";
                        }
                        else
                        {
                            msg << "Bound textures: ";
                        }
                        if (!visitor.mTextures.empty())
                        {
                            msg << std::endl;
                            std::string lastTextureSrc;
                            for (auto& [textureName, fileName] : visitor.mTextures)
                            {
                                std::string textureSrc;
                                if (!fileName.empty())
                                    textureSrc = vfs->getArchive(fileName);

                                if (lastTextureSrc.empty() || textureSrc != lastTextureSrc)
                                {
                                    lastTextureSrc = std::move(textureSrc);
                                    if (lastTextureSrc.empty())
                                        lastTextureSrc = "[No Source]";

                                    msg << "  " << lastTextureSrc << std::endl;
                                }
                                msg << "    ";
                                msg << (textureName.empty() ? "[Anonymous]: " : textureName) << ": ";
                                msg << (fileName.empty() ? "[No File]" : fileName) << std::endl;
                            }
                        }
                        else
                        {
                            msg << "[None]" << std::endl;
                        }
                    }
                    if (!ptr.getClass().getScript(ptr).empty())
                        msg << "Script: " << ptr.getClass().getScript(ptr) << std::endl;
                }

                while (arg0 > 0)
                {
                    std::string_view notes = runtime.getStringLiteral(runtime[0].mInteger);
                    runtime.pop();
                    if (!notes.empty())
                        msg << "Notes: " << notes << std::endl;
                    --arg0;
                }

                Log(Debug::Warning) << "\n" << msg.str();

                runtime.getContext().report(msg.str());
            }
        };

        class OpAddToLevCreature : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                const ESM::RefId& levId = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();
                const ESM::RefId& creatureId = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();
                int level = runtime[0].mInteger;
                runtime.pop();

                ESM::CreatureLevList listCopy
                    = *MWBase::Environment::get().getESMStore()->get<ESM::CreatureLevList>().find(levId);
                addToLevList(&listCopy, creatureId, level);
                MWBase::Environment::get().getESMStore()->overrideRecord(listCopy);
            }
        };

        class OpRemoveFromLevCreature : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                const ESM::RefId& levId = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();
                const ESM::RefId& creatureId = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();
                int level = runtime[0].mInteger;
                runtime.pop();

                ESM::CreatureLevList listCopy
                    = *MWBase::Environment::get().getESMStore()->get<ESM::CreatureLevList>().find(levId);
                removeFromLevList(&listCopy, creatureId, level);
                MWBase::Environment::get().getESMStore()->overrideRecord(listCopy);
            }
        };

        class OpAddToLevItem : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                const ESM::RefId& levId = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();
                const ESM::RefId& itemId = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();
                int level = runtime[0].mInteger;
                runtime.pop();

                ESM::ItemLevList listCopy
                    = *MWBase::Environment::get().getESMStore()->get<ESM::ItemLevList>().find(levId);
                addToLevList(&listCopy, itemId, level);
                MWBase::Environment::get().getESMStore()->overrideRecord(listCopy);
            }
        };

        class OpRemoveFromLevItem : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                const ESM::RefId& levId = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();
                const ESM::RefId& itemId = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();
                int level = runtime[0].mInteger;
                runtime.pop();

                ESM::ItemLevList listCopy
                    = *MWBase::Environment::get().getESMStore()->get<ESM::ItemLevList>().find(levId);
                removeFromLevList(&listCopy, itemId, level);
                MWBase::Environment::get().getESMStore()->overrideRecord(listCopy);
            }
        };

        template <class R>
        class OpShowSceneGraph : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::Ptr ptr = R()(runtime, false);

                int confirmed = 0;
                if (arg0 == 1)
                {
                    confirmed = runtime[0].mInteger;
                    runtime.pop();
                }

                if (ptr.isEmpty() && !confirmed)
                    runtime.getContext().report(
                        "Exporting the entire scene graph will result in a large file. Confirm this action using "
                        "'showscenegraph 1' or select an object instead.");
                else
                {
                    const auto filename = MWBase::Environment::get().getWorld()->exportSceneGraph(ptr);
                    runtime.getContext().report("Wrote '" + Files::pathToUnicodeString(filename) + "'");
                }
            }
        };

        class OpToggleNavMesh : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                bool enabled = MWBase::Environment::get().getWorld()->toggleRenderMode(MWRender::Render_NavMesh);

                runtime.getContext().report(
                    enabled ? "Navigation Mesh Rendering -> On" : "Navigation Mesh Rendering -> Off");
            }
        };

        class OpToggleActorsPaths : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                bool enabled = MWBase::Environment::get().getWorld()->toggleRenderMode(MWRender::Render_ActorsPaths);

                runtime.getContext().report(enabled ? "Agents Paths Rendering -> On" : "Agents Paths Rendering -> Off");
            }
        };

        class OpSetNavMeshNumberToRender : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                const auto navMeshNumber = runtime[0].mInteger;
                runtime.pop();

                if (navMeshNumber < 0)
                {
                    runtime.getContext().report("Invalid navmesh number: use not less than zero values");
                    return;
                }

                MWBase::Environment::get().getWorld()->setNavMeshNumberToRender(
                    static_cast<std::size_t>(navMeshNumber));
            }
        };

        template <class R>
        class OpRepairedOnMe : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                // Broken in vanilla and deliberately no-op.
                runtime.push(0);
            }
        };

        class OpToggleRecastMesh : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                bool enabled = MWBase::Environment::get().getWorld()->toggleRenderMode(MWRender::Render_RecastMesh);

                runtime.getContext().report(enabled ? "Recast Mesh Rendering -> On" : "Recast Mesh Rendering -> Off");
            }
        };

        class OpHelp : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                std::stringstream message;
                message << MWBase::Environment::get().getWindowManager()->getVersionDescription() << "\n\n";
                std::vector<std::string> commands;
                MWBase::Environment::get().getScriptManager()->getExtensions().listKeywords(commands);
                for (const auto& command : commands)
                    message << command << "\n";
                runtime.getContext().report(message.str());
            }
        };

        class OpReloadLua : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWBase::Environment::get().getLuaManager()->reloadAllScripts();
                runtime.getContext().report("All Lua scripts are reloaded");
            }
        };

        class OpTestModels : public Interpreter::Opcode0
        {
            template <class T>
            void test(int& count) const
            {
                Resource::SceneManager* sceneManager
                    = MWBase::Environment::get().getResourceSystem()->getSceneManager();
                const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
                for (const T& record : store.get<T>())
                {
                    MWWorld::ManualRef ref(store, record.mId);
                    VFS::Path::Normalized model(ref.getPtr().getClass().getCorrectedModel(ref.getPtr()));
                    if (!model.empty())
                    {
                        sceneManager->getTemplate(model);
                        ++count;
                    }
                }
            }

        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                Resource::SceneManager* sceneManager
                    = MWBase::Environment::get().getResourceSystem()->getSceneManager();
                double delay = sceneManager->getExpiryDelay();
                sceneManager->setExpiryDelay(0.0);
                int count = 0;

                test<ESM::Activator>(count);
                test<ESM::Apparatus>(count);
                test<ESM::Armor>(count);
                test<ESM::Potion>(count);
                test<ESM::BodyPart>(count);
                test<ESM::Book>(count);
                test<ESM::Clothing>(count);
                test<ESM::Container>(count);
                test<ESM::Creature>(count);
                test<ESM::Door>(count);
                test<ESM::Ingredient>(count);
                test<ESM::Light>(count);
                test<ESM::Lockpick>(count);
                test<ESM::Miscellaneous>(count);
                test<ESM::Probe>(count);
                test<ESM::Repair>(count);
                test<ESM::Static>(count);
                test<ESM::Weapon>(count);

                sceneManager->setExpiryDelay(delay);
                std::stringstream message;
                message << "Attempted to load models for " << count << " objects. Check the log for details.";
                runtime.getContext().report(message.str());
            }
        };

        void installOpcodes(Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5<OpMenuMode>(Compiler::Misc::opcodeMenuMode);
            interpreter.installSegment5<OpRandom>(Compiler::Misc::opcodeRandom);
            interpreter.installSegment5<OpScriptRunning>(Compiler::Misc::opcodeScriptRunning);
            interpreter.installSegment5<OpStartScript<ImplicitRef>>(Compiler::Misc::opcodeStartScript);
            interpreter.installSegment5<OpStartScript<ExplicitRef>>(Compiler::Misc::opcodeStartScriptExplicit);
            interpreter.installSegment5<OpStopScript>(Compiler::Misc::opcodeStopScript);
            interpreter.installSegment5<OpGetSecondsPassed>(Compiler::Misc::opcodeGetSecondsPassed);
            interpreter.installSegment5<OpEnable<ImplicitRef>>(Compiler::Misc::opcodeEnable);
            interpreter.installSegment5<OpEnable<ExplicitRef>>(Compiler::Misc::opcodeEnableExplicit);
            interpreter.installSegment5<OpDisable<ImplicitRef>>(Compiler::Misc::opcodeDisable);
            interpreter.installSegment5<OpDisable<ExplicitRef>>(Compiler::Misc::opcodeDisableExplicit);
            interpreter.installSegment5<OpGetDisabled<ImplicitRef>>(Compiler::Misc::opcodeGetDisabled);
            interpreter.installSegment5<OpGetDisabled<ExplicitRef>>(Compiler::Misc::opcodeGetDisabledExplicit);
            interpreter.installSegment5<OpXBox>(Compiler::Misc::opcodeXBox);
            interpreter.installSegment5<OpOnActivate<ImplicitRef>>(Compiler::Misc::opcodeOnActivate);
            interpreter.installSegment5<OpOnActivate<ExplicitRef>>(Compiler::Misc::opcodeOnActivateExplicit);
            interpreter.installSegment5<OpActivate<ImplicitRef>>(Compiler::Misc::opcodeActivate);
            interpreter.installSegment5<OpActivate<ExplicitRef>>(Compiler::Misc::opcodeActivateExplicit);
            interpreter.installSegment3<OpLock<ImplicitRef>>(Compiler::Misc::opcodeLock);
            interpreter.installSegment3<OpLock<ExplicitRef>>(Compiler::Misc::opcodeLockExplicit);
            interpreter.installSegment5<OpUnlock<ImplicitRef>>(Compiler::Misc::opcodeUnlock);
            interpreter.installSegment5<OpUnlock<ExplicitRef>>(Compiler::Misc::opcodeUnlockExplicit);
            interpreter.installSegment5<OpToggleCollisionDebug>(Compiler::Misc::opcodeToggleCollisionDebug);
            interpreter.installSegment5<OpToggleCollisionBoxes>(Compiler::Misc::opcodeToggleCollisionBoxes);
            interpreter.installSegment5<OpToggleWireframe>(Compiler::Misc::opcodeToggleWireframe);
            interpreter.installSegment5<OpFadeIn>(Compiler::Misc::opcodeFadeIn);
            interpreter.installSegment5<OpFadeOut>(Compiler::Misc::opcodeFadeOut);
            interpreter.installSegment5<OpFadeTo>(Compiler::Misc::opcodeFadeTo);
            interpreter.installSegment5<OpTogglePathgrid>(Compiler::Misc::opcodeTogglePathgrid);
            interpreter.installSegment5<OpToggleWater>(Compiler::Misc::opcodeToggleWater);
            interpreter.installSegment5<OpToggleWorld>(Compiler::Misc::opcodeToggleWorld);
            interpreter.installSegment5<OpDontSaveObject>(Compiler::Misc::opcodeDontSaveObject);
            interpreter.installSegment5<OpPcForce1stPerson>(Compiler::Misc::opcodePcForce1stPerson);
            interpreter.installSegment5<OpPcForce3rdPerson>(Compiler::Misc::opcodePcForce3rdPerson);
            interpreter.installSegment5<OpPcGet3rdPerson>(Compiler::Misc::opcodePcGet3rdPerson);
            interpreter.installSegment5<OpToggleVanityMode>(Compiler::Misc::opcodeToggleVanityMode);
            interpreter.installSegment5<OpGetPcSleep>(Compiler::Misc::opcodeGetPcSleep);
            interpreter.installSegment5<OpGetPcJumping>(Compiler::Misc::opcodeGetPcJumping);
            interpreter.installSegment5<OpWakeUpPc>(Compiler::Misc::opcodeWakeUpPc);
            interpreter.installSegment5<OpPlayBink>(Compiler::Misc::opcodePlayBink);
            interpreter.installSegment5<OpPayFine>(Compiler::Misc::opcodePayFine);
            interpreter.installSegment5<OpPayFineThief>(Compiler::Misc::opcodePayFineThief);
            interpreter.installSegment5<OpGoToJail>(Compiler::Misc::opcodeGoToJail);
            interpreter.installSegment5<OpGetLocked<ImplicitRef>>(Compiler::Misc::opcodeGetLocked);
            interpreter.installSegment5<OpGetLocked<ExplicitRef>>(Compiler::Misc::opcodeGetLockedExplicit);
            interpreter.installSegment5<OpGetEffect<ImplicitRef>>(Compiler::Misc::opcodeGetEffect);
            interpreter.installSegment5<OpGetEffect<ExplicitRef>>(Compiler::Misc::opcodeGetEffectExplicit);
            interpreter.installSegment5<OpAddSoulGem<ImplicitRef>>(Compiler::Misc::opcodeAddSoulGem);
            interpreter.installSegment5<OpAddSoulGem<ExplicitRef>>(Compiler::Misc::opcodeAddSoulGemExplicit);
            interpreter.installSegment3<OpRemoveSoulGem<ImplicitRef>>(Compiler::Misc::opcodeRemoveSoulGem);
            interpreter.installSegment3<OpRemoveSoulGem<ExplicitRef>>(Compiler::Misc::opcodeRemoveSoulGemExplicit);
            interpreter.installSegment5<OpDrop<ImplicitRef>>(Compiler::Misc::opcodeDrop);
            interpreter.installSegment5<OpDrop<ExplicitRef>>(Compiler::Misc::opcodeDropExplicit);
            interpreter.installSegment5<OpDropSoulGem<ImplicitRef>>(Compiler::Misc::opcodeDropSoulGem);
            interpreter.installSegment5<OpDropSoulGem<ExplicitRef>>(Compiler::Misc::opcodeDropSoulGemExplicit);
            interpreter.installSegment5<OpGetAttacked<ImplicitRef>>(Compiler::Misc::opcodeGetAttacked);
            interpreter.installSegment5<OpGetAttacked<ExplicitRef>>(Compiler::Misc::opcodeGetAttackedExplicit);
            interpreter.installSegment5<OpGetWeaponDrawn<ImplicitRef>>(Compiler::Misc::opcodeGetWeaponDrawn);
            interpreter.installSegment5<OpGetWeaponDrawn<ExplicitRef>>(Compiler::Misc::opcodeGetWeaponDrawnExplicit);
            interpreter.installSegment5<OpGetSpellReadied<ImplicitRef>>(Compiler::Misc::opcodeGetSpellReadied);
            interpreter.installSegment5<OpGetSpellReadied<ExplicitRef>>(Compiler::Misc::opcodeGetSpellReadiedExplicit);
            interpreter.installSegment5<OpGetSpellEffects<ImplicitRef>>(Compiler::Misc::opcodeGetSpellEffects);
            interpreter.installSegment5<OpGetSpellEffects<ExplicitRef>>(Compiler::Misc::opcodeGetSpellEffectsExplicit);
            interpreter.installSegment5<OpGetCurrentTime>(Compiler::Misc::opcodeGetCurrentTime);
            interpreter.installSegment5<OpSetDelete<ImplicitRef>>(Compiler::Misc::opcodeSetDelete);
            interpreter.installSegment5<OpSetDelete<ExplicitRef>>(Compiler::Misc::opcodeSetDeleteExplicit);
            interpreter.installSegment5<OpGetSquareRoot>(Compiler::Misc::opcodeGetSquareRoot);
            interpreter.installSegment5<OpFall<ImplicitRef>>(Compiler::Misc::opcodeFall);
            interpreter.installSegment5<OpFall<ExplicitRef>>(Compiler::Misc::opcodeFallExplicit);
            interpreter.installSegment5<OpGetStandingPc<ImplicitRef>>(Compiler::Misc::opcodeGetStandingPc);
            interpreter.installSegment5<OpGetStandingPc<ExplicitRef>>(Compiler::Misc::opcodeGetStandingPcExplicit);
            interpreter.installSegment5<OpGetStandingActor<ImplicitRef>>(Compiler::Misc::opcodeGetStandingActor);
            interpreter.installSegment5<OpGetStandingActor<ExplicitRef>>(
                Compiler::Misc::opcodeGetStandingActorExplicit);
            interpreter.installSegment5<OpGetCollidingPc<ImplicitRef>>(Compiler::Misc::opcodeGetCollidingPc);
            interpreter.installSegment5<OpGetCollidingPc<ExplicitRef>>(Compiler::Misc::opcodeGetCollidingPcExplicit);
            interpreter.installSegment5<OpGetCollidingActor<ImplicitRef>>(Compiler::Misc::opcodeGetCollidingActor);
            interpreter.installSegment5<OpGetCollidingActor<ExplicitRef>>(
                Compiler::Misc::opcodeGetCollidingActorExplicit);
            interpreter.installSegment5<OpHurtStandingActor<ImplicitRef>>(Compiler::Misc::opcodeHurtStandingActor);
            interpreter.installSegment5<OpHurtStandingActor<ExplicitRef>>(
                Compiler::Misc::opcodeHurtStandingActorExplicit);
            interpreter.installSegment5<OpHurtCollidingActor<ImplicitRef>>(Compiler::Misc::opcodeHurtCollidingActor);
            interpreter.installSegment5<OpHurtCollidingActor<ExplicitRef>>(
                Compiler::Misc::opcodeHurtCollidingActorExplicit);
            interpreter.installSegment5<OpGetWindSpeed>(Compiler::Misc::opcodeGetWindSpeed);
            interpreter.installSegment5<OpHitOnMe<ImplicitRef>>(Compiler::Misc::opcodeHitOnMe);
            interpreter.installSegment5<OpHitOnMe<ExplicitRef>>(Compiler::Misc::opcodeHitOnMeExplicit);
            interpreter.installSegment5<OpHitAttemptOnMe<ImplicitRef>>(Compiler::Misc::opcodeHitAttemptOnMe);
            interpreter.installSegment5<OpHitAttemptOnMe<ExplicitRef>>(Compiler::Misc::opcodeHitAttemptOnMeExplicit);
            interpreter.installSegment5<OpEnableTeleporting<false>>(Compiler::Misc::opcodeDisableTeleporting);
            interpreter.installSegment5<OpEnableTeleporting<true>>(Compiler::Misc::opcodeEnableTeleporting);
            interpreter.installSegment5<OpShowVars<ImplicitRef>>(Compiler::Misc::opcodeShowVars);
            interpreter.installSegment5<OpShowVars<ExplicitRef>>(Compiler::Misc::opcodeShowVarsExplicit);
            interpreter.installSegment5<OpShow<ImplicitRef>>(Compiler::Misc::opcodeShow);
            interpreter.installSegment5<OpShow<ExplicitRef>>(Compiler::Misc::opcodeShowExplicit);
            interpreter.installSegment5<OpToggleGodMode>(Compiler::Misc::opcodeToggleGodMode);
            interpreter.installSegment5<OpToggleScripts>(Compiler::Misc::opcodeToggleScripts);
            interpreter.installSegment5<OpEnableLevitation<false>>(Compiler::Misc::opcodeDisableLevitation);
            interpreter.installSegment5<OpEnableLevitation<true>>(Compiler::Misc::opcodeEnableLevitation);
            interpreter.installSegment5<OpCast<ImplicitRef>>(Compiler::Misc::opcodeCast);
            interpreter.installSegment5<OpCast<ExplicitRef>>(Compiler::Misc::opcodeCastExplicit);
            interpreter.installSegment5<OpExplodeSpell<ImplicitRef>>(Compiler::Misc::opcodeExplodeSpell);
            interpreter.installSegment5<OpExplodeSpell<ExplicitRef>>(Compiler::Misc::opcodeExplodeSpellExplicit);
            interpreter.installSegment5<OpGetPcInJail>(Compiler::Misc::opcodeGetPcInJail);
            interpreter.installSegment5<OpGetPcTraveling>(Compiler::Misc::opcodeGetPcTraveling);
            interpreter.installSegment3<OpBetaComment<ImplicitRef>>(Compiler::Misc::opcodeBetaComment);
            interpreter.installSegment3<OpBetaComment<ExplicitRef>>(Compiler::Misc::opcodeBetaCommentExplicit);
            interpreter.installSegment5<OpAddToLevCreature>(Compiler::Misc::opcodeAddToLevCreature);
            interpreter.installSegment5<OpRemoveFromLevCreature>(Compiler::Misc::opcodeRemoveFromLevCreature);
            interpreter.installSegment5<OpAddToLevItem>(Compiler::Misc::opcodeAddToLevItem);
            interpreter.installSegment5<OpRemoveFromLevItem>(Compiler::Misc::opcodeRemoveFromLevItem);
            interpreter.installSegment3<OpShowSceneGraph<ImplicitRef>>(Compiler::Misc::opcodeShowSceneGraph);
            interpreter.installSegment3<OpShowSceneGraph<ExplicitRef>>(Compiler::Misc::opcodeShowSceneGraphExplicit);
            interpreter.installSegment5<OpToggleBorders>(Compiler::Misc::opcodeToggleBorders);
            interpreter.installSegment5<OpToggleNavMesh>(Compiler::Misc::opcodeToggleNavMesh);
            interpreter.installSegment5<OpToggleActorsPaths>(Compiler::Misc::opcodeToggleActorsPaths);
            interpreter.installSegment5<OpSetNavMeshNumberToRender>(Compiler::Misc::opcodeSetNavMeshNumberToRender);
            interpreter.installSegment5<OpRepairedOnMe<ImplicitRef>>(Compiler::Misc::opcodeRepairedOnMe);
            interpreter.installSegment5<OpRepairedOnMe<ExplicitRef>>(Compiler::Misc::opcodeRepairedOnMeExplicit);
            interpreter.installSegment5<OpToggleRecastMesh>(Compiler::Misc::opcodeToggleRecastMesh);
            interpreter.installSegment5<OpHelp>(Compiler::Misc::opcodeHelp);
            interpreter.installSegment5<OpReloadLua>(Compiler::Misc::opcodeReloadLua);
            interpreter.installSegment5<OpTestModels>(Compiler::Misc::opcodeTestModels);
        }
    }
}
