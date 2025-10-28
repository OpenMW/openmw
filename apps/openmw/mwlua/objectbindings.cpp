#include "objectbindings.hpp"

#include <components/esm3/loadfact.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/shapes/box.hpp>
#include <components/lua/util.hpp>
#include <components/lua/utilpackage.hpp>
#include <components/misc/convert.hpp>
#include <components/misc/mathutil.hpp>

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/localscripts.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/scene.hpp"
#include "../mwworld/worldmodel.hpp"

#include "../mwrender/renderingmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "luaevents.hpp"
#include "luamanagerimp.hpp"
#include "types/types.hpp"

namespace sol
{
    template <>
    struct is_automagical<MWLua::LObject> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::GObject> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::LObjectList> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::GObjectList> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::Inventory<MWLua::LObject>> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::Inventory<MWLua::GObject>> : std::false_type
    {
    };
}

namespace MWLua
{

    namespace
    {
        MWWorld::CellStore* findCell(const sol::object& cellOrName, const osg::Vec3f& pos)
        {
            MWWorld::WorldModel* wm = MWBase::Environment::get().getWorldModel();
            MWWorld::CellStore* cell;
            if (cellOrName.is<GCell>())
                cell = cellOrName.as<const GCell&>().mStore;
            else
            {
                std::string_view name = LuaUtil::cast<std::string_view>(cellOrName);
                if (name.empty())
                    cell = nullptr; // default exterior worldspace
                else
                    cell = &wm->getCell(name);
            }
            if (cell != nullptr && !cell->isExterior())
                return cell;
            const ESM::RefId worldspace
                = cell == nullptr ? ESM::Cell::sDefaultWorldspaceId : cell->getCell()->getWorldSpace();
            return &wm->getExterior(ESM::positionToExteriorCellLocation(pos.x(), pos.y(), worldspace));
        }

        ESM::Position toPos(const osg::Vec3f& pos, const osg::Vec3f& rot)
        {
            ESM::Position esmPos;
            static_assert(sizeof(esmPos) == sizeof(osg::Vec3f) * 2);
            std::memcpy(esmPos.pos, &pos, sizeof(osg::Vec3f));
            std::memcpy(esmPos.rot, &rot, sizeof(osg::Vec3f));
            return esmPos;
        }

        void teleportPlayer(
            MWWorld::CellStore* destCell, const osg::Vec3f& pos, const osg::Vec3f& rot, bool placeOnGround)
        {
            MWBase::World* world = MWBase::Environment::get().getWorld();
            MWWorld::Ptr ptr = world->getPlayerPtr();
            auto& stats = ptr.getClass().getCreatureStats(ptr);
            stats.land(true);
            stats.setTeleported(true);
            world->getPlayer().setTeleported(true);
            bool differentCell = ptr.getCell() != destCell;
            world->changeToCell(destCell->getCell()->getId(), toPos(pos, rot), false, differentCell);
            MWWorld::Ptr newPtr = world->getPlayerPtr();
            world->moveObject(newPtr, pos);
            world->rotateObject(newPtr, rot);
            if (placeOnGround)
                world->adjustPosition(newPtr, true);
            MWBase::Environment::get().getLuaManager()->objectTeleported(newPtr);
        }

        void teleportNotPlayer(const MWWorld::Ptr& ptr, MWWorld::CellStore* destCell, const osg::Vec3f& pos,
            const osg::Vec3f& rot, bool placeOnGround)
        {
            MWBase::World* world = MWBase::Environment::get().getWorld();
            MWWorld::WorldModel* wm = MWBase::Environment::get().getWorldModel();
            const MWWorld::Class& cls = ptr.getClass();
            if (cls.isActor())
            {
                auto& stats = cls.getCreatureStats(ptr);
                stats.land(false);
                stats.setTeleported(true);
            }
            const MWWorld::CellStore* srcCell = ptr.getCell();
            MWWorld::Ptr newPtr;
            if (srcCell == &wm->getDraftCell())
            {
                newPtr = cls.moveToCell(ptr, *destCell, toPos(pos, rot));
                ptr.getCellRef().unsetRefNum();
                ptr.getRefData().setLuaScripts(nullptr);
                ptr.getCellRef().setCount(0);
                ESM::RefId script = cls.getScript(newPtr);
                if (!script.empty())
                    world->getLocalScripts().add(script, newPtr);
                world->addContainerScripts(newPtr, newPtr.getCell());
            }
            else
            {
                newPtr = world->moveObject(ptr, destCell, pos);
                if (srcCell == destCell)
                {
                    ESM::RefId script = cls.getScript(newPtr);
                    if (!script.empty())
                        world->getLocalScripts().add(script, newPtr);
                }
                world->rotateObject(newPtr, rot, MWBase::RotationFlag_none);
            }
            if (placeOnGround)
                world->adjustPosition(newPtr, true);
            if (cls.isDoor())
            { // Change "original position and rotation" because without it teleported animated doors don't work
              // properly.
                newPtr.getCellRef().setPosition(newPtr.getRefData().getPosition());
            }
            if (!newPtr.getRefData().isEnabled())
                world->enable(newPtr);
            MWBase::Environment::get().getLuaManager()->objectTeleported(newPtr);
        }

        template <typename ObjT>
        using Cell = std::conditional_t<std::is_same_v<ObjT, LObject>, LCell, GCell>;

        template <class ObjectT>
        void registerObjectList(const std::string& prefix, const Context& context)
        {
            using ListT = ObjectList<ObjectT>;
            sol::state_view lua = context.sol();
            sol::usertype<ListT> listT = lua.new_usertype<ListT>(prefix + "ObjectList");
            listT[sol::meta_function::to_string]
                = [](const ListT& list) { return "{" + std::to_string(list.mIds->size()) + " objects}"; };
            listT[sol::meta_function::length] = [](const ListT& list) { return list.mIds->size(); };
            listT[sol::meta_function::index] = [](const ListT& list, size_t index) -> sol::optional<ObjectT> {
                if (index > 0 && index <= list.mIds->size())
                    return ObjectT((*list.mIds)[LuaUtil::fromLuaIndex(index)]);
                else
                    return sol::nullopt;
            };
            listT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();
            listT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
        }

        osg::Vec3f toEulerRotation(const sol::object& transform, bool isActor)
        {
            if (transform.is<LuaUtil::TransformQ>())
            {
                const osg::Quat& q = transform.as<LuaUtil::TransformQ>().mQ;
                return isActor ? Misc::toEulerAnglesXZ(q) : Misc::toEulerAnglesZYX(q);
            }
            else
            {
                const osg::Matrixf& m = LuaUtil::cast<LuaUtil::TransformM>(transform).mM;
                return isActor ? Misc::toEulerAnglesXZ(m) : Misc::toEulerAnglesZYX(m);
            }
        }

        osg::Quat toQuat(const ESM::Position& pos, bool isActor)
        {
            if (isActor)
                return osg::Quat(pos.rot[0], osg::Vec3(-1, 0, 0)) * osg::Quat(pos.rot[2], osg::Vec3(0, 0, -1));
            else
                return Misc::Convert::makeOsgQuat(pos.rot);
        }

        template <class ObjectT>
        void addOwnerbindings(sol::usertype<ObjectT>& objectT, const std::string& prefix, const Context& context)
        {
            using OwnerT = Owner<ObjectT>;
            sol::usertype<OwnerT> ownerT = context.sol().new_usertype<OwnerT>(prefix + "Owner");

            ownerT[sol::meta_function::to_string] = [](const OwnerT& o) { return "Owner[" + o.mObj.toString() + "]"; };

            auto getOwnerRecordId = [](const OwnerT& o) -> sol::optional<std::string> {
                ESM::RefId owner = o.mObj.ptr().getCellRef().getOwner();
                if (owner.empty())
                    return sol::nullopt;
                else
                    return owner.serializeText();
            };
            auto setOwnerRecordId = [](const OwnerT& o, sol::optional<std::string_view> ownerId) {
                if (std::is_same_v<ObjectT, LObject> && !dynamic_cast<const SelfObject*>(&o.mObj))
                    throw std::runtime_error("Local scripts can set an owner only on self");
                const MWWorld::Ptr& ptr = o.mObj.ptr();

                if (!ownerId)
                {
                    ptr.getCellRef().setOwner(ESM::RefId());
                    return;
                }
                ESM::RefId owner = ESM::RefId::deserializeText(*ownerId);
                const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
                if (!store.get<ESM::NPC>().search(owner))
                    throw std::runtime_error("Invalid owner record id");
                ptr.getCellRef().setOwner(owner);
            };
            ownerT["recordId"] = sol::property(getOwnerRecordId, setOwnerRecordId);

            auto getOwnerFactionId = [](const OwnerT& o) -> sol::optional<std::string> {
                ESM::RefId owner = o.mObj.ptr().getCellRef().getFaction();
                if (owner.empty())
                    return sol::nullopt;
                else
                    return owner.serializeText();
            };
            auto setOwnerFactionId = [](const OwnerT& o, sol::optional<std::string> ownerId) {
                ESM::RefId ownerFac;
                if (std::is_same_v<ObjectT, LObject> && !dynamic_cast<const SelfObject*>(&o.mObj))
                    throw std::runtime_error("Local scripts can set an owner faction only on self");
                if (!ownerId)
                {
                    o.mObj.ptr().getCellRef().setFaction(ESM::RefId());
                    return;
                }
                ownerFac = ESM::RefId::deserializeText(*ownerId);
                const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
                if (!store.get<ESM::Faction>().search(ownerFac))
                    throw std::runtime_error("Invalid owner faction id");
                o.mObj.ptr().getCellRef().setFaction(ownerFac);
            };
            ownerT["factionId"] = sol::property(getOwnerFactionId, setOwnerFactionId);

            auto getOwnerFactionRank = [](const OwnerT& o) -> sol::optional<size_t> {
                int rank = o.mObj.ptr().getCellRef().getFactionRank();
                if (rank < 0)
                    return sol::nullopt;
                return LuaUtil::toLuaIndex(rank);
            };
            auto setOwnerFactionRank = [](const OwnerT& o, sol::optional<size_t> factionRank) {
                if (std::is_same_v<ObjectT, LObject> && !dynamic_cast<const SelfObject*>(&o.mObj))
                    throw std::runtime_error("Local scripts can set an owner faction rank only on self");
                o.mObj.ptr().getCellRef().setFactionRank(LuaUtil::fromLuaIndex(factionRank.value_or(0)));
            };
            ownerT["factionRank"] = sol::property(getOwnerFactionRank, setOwnerFactionRank);

            objectT["owner"] = sol::readonly_property([](const ObjectT& object) { return OwnerT{ object }; });
        }

        template <class ObjectT>
        void addBasicBindings(sol::usertype<ObjectT>& objectT, const Context& context)
        {
            objectT["id"] = sol::readonly_property([](const ObjectT& o) -> std::string { return o.id().toString(); });
            objectT["contentFile"] = sol::readonly_property([](const ObjectT& o) -> sol::optional<std::string> {
                int contentFileIndex = o.id().mContentFile;
                const std::vector<std::string>& contentList = MWBase::Environment::get().getWorld()->getContentFiles();
                if (contentFileIndex < 0 || contentFileIndex >= static_cast<int>(contentList.size()))
                    return sol::nullopt;
                return Misc::StringUtils::lowerCase(contentList[contentFileIndex]);
            });
            objectT["isValid"] = [](const ObjectT& o) { return !o.ptrOrEmpty().isEmpty(); };
            objectT["recordId"] = sol::readonly_property(
                [](const ObjectT& o) -> std::string { return o.ptr().getCellRef().getRefId().serializeText(); });
            objectT["globalVariable"] = sol::readonly_property([](const ObjectT& o) -> sol::optional<std::string> {
                std::string_view globalVariable = o.ptr().getCellRef().getGlobalVariable();
                if (globalVariable.empty())
                    return sol::nullopt;
                else
                    return ESM::RefId::stringRefId(globalVariable).serializeText();
            });
            objectT["cell"] = sol::readonly_property([](const ObjectT& o) -> sol::optional<Cell<ObjectT>> {
                const MWWorld::Ptr& ptr = o.ptr();
                MWWorld::WorldModel* wm = MWBase::Environment::get().getWorldModel();
                if (ptr.isInCell() && ptr.getCell() != &wm->getDraftCell())
                    return Cell<ObjectT>{ ptr.getCell() };
                else
                    return sol::nullopt;
            });
            objectT["parentContainer"] = sol::readonly_property([](const ObjectT& o) -> sol::optional<ObjectT> {
                const MWWorld::Ptr& ptr = o.ptr();
                if (ptr.getContainerStore())
                    return ObjectT(ptr.getContainerStore()->getPtr());
                else
                    return sol::nullopt;
            });
            objectT["position"] = sol::readonly_property(
                [](const ObjectT& o) -> osg::Vec3f { return o.ptr().getRefData().getPosition().asVec3(); });
            objectT["scale"]
                = sol::readonly_property([](const ObjectT& o) -> float { return o.ptr().getCellRef().getScale(); });
            objectT["rotation"] = sol::readonly_property([](const ObjectT& o) -> LuaUtil::TransformQ {
                return { toQuat(o.ptr().getRefData().getPosition(), o.ptr().getClass().isActor()) };
            });
            objectT["startingPosition"] = sol::readonly_property(
                [](const ObjectT& o) -> osg::Vec3f { return o.ptr().getCellRef().getPosition().asVec3(); });
            objectT["startingRotation"] = sol::readonly_property([](const ObjectT& o) -> LuaUtil::TransformQ {
                return { toQuat(o.ptr().getCellRef().getPosition(), o.ptr().getClass().isActor()) };
            });
            objectT["getBoundingBox"] = [](const ObjectT& o) {
                MWRender::RenderingManager* renderingManager
                    = MWBase::Environment::get().getWorld()->getRenderingManager();
                osg::BoundingBox bb = renderingManager->getCullSafeBoundingBox(o.ptr());
                return LuaUtil::Box{ bb.center(), bb._max - bb.center() };
            };

            objectT["type"] = sol::readonly_property(
                [types = getTypeToPackageTable(context.sol())](
                    const ObjectT& o) -> sol::object { return types[getLiveCellRefType(o.ptr().mRef)]; });

            objectT["count"] = sol::readonly_property([](const ObjectT& o) { return o.ptr().getCellRef().getCount(); });
            objectT[sol::meta_function::equal_to] = [](const ObjectT& a, const ObjectT& b) { return a.id() == b.id(); };
            objectT[sol::meta_function::to_string] = &ObjectT::toString;
            objectT["sendEvent"] = [context](const ObjectT& dest, std::string eventName, const sol::object& eventData) {
                context.mLuaEvents->addLocalEvent(
                    { dest.id(), std::move(eventName), LuaUtil::serialize(eventData, context.mSerializer) });
            };

            objectT["activateBy"] = [](const ObjectT& object, const ObjectT& actor) {
                const MWWorld::Ptr& objPtr = object.ptr();
                const MWWorld::Ptr& actorPtr = actor.ptr();
                uint32_t esmRecordType = actorPtr.getType();
                if (esmRecordType != ESM::REC_CREA && esmRecordType != ESM::REC_NPC_)
                    throw std::runtime_error(
                        "The argument of `activateBy` must be an actor who activates the object. Got: "
                        + actor.toString());

                MWBase::Environment::get().getLuaManager()->objectActivated(objPtr, actorPtr);
            };

            auto isEnabled = [](const ObjectT& o) { return o.ptr().getRefData().isEnabled(); };
            auto setEnabled = [context](const GObject& object, bool enable) {
                if (enable && object.ptr().mRef->isDeleted())
                    throw std::runtime_error("Object is removed");
                context.mLuaManager->addAction([object, enable] {
                    if (object.ptr().mRef->isDeleted())
                        return;
                    if (object.ptr().isInCell())
                    {
                        if (enable)
                            MWBase::Environment::get().getWorld()->enable(object.ptr());
                        else
                            MWBase::Environment::get().getWorld()->disable(object.ptr());
                    }
                    else
                    {
                        if (enable)
                            object.ptr().getRefData().enable();
                        else
                            throw std::runtime_error("Objects in containers can't be disabled");
                    }
                });
            };
            if constexpr (std::is_same_v<ObjectT, GObject>)
                objectT["enabled"] = sol::property(isEnabled, setEnabled);
            else
                objectT["enabled"] = sol::readonly_property(isEnabled);

            if constexpr (std::is_same_v<ObjectT, GObject>)
            { // Only for global scripts
                objectT["setScale"] = [context](const GObject& object, float scale) {
                    context.mLuaManager->addAction(
                        [object, scale] { MWBase::Environment::get().getWorld()->scaleObject(object.ptr(), scale); });
                };
                objectT["addScript"] = [context](const GObject& object, std::string_view path, sol::object initData) {
                    const LuaUtil::ScriptsConfiguration& cfg = context.mLua->getConfiguration();
                    std::optional<int> scriptId = cfg.findId(VFS::Path::Normalized(path));
                    if (!scriptId)
                        throw std::runtime_error("Unknown script: " + std::string(path));
                    if (!(cfg[*scriptId].mFlags & ESM::LuaScriptCfg::sCustom))
                        throw std::runtime_error(
                            "Script without CUSTOM tag can not be added dynamically: " + std::string(path));
                    if (object.ptr().getType() == ESM::REC_STAT)
                        throw std::runtime_error("Attaching scripts to Static is not allowed: " + std::string(path));
                    if (initData != sol::nil)
                        context.mLuaManager->addCustomLocalScript(object.ptr(), *scriptId,
                            LuaUtil::serialize(LuaUtil::cast<sol::table>(initData), context.mSerializer));
                    else
                        context.mLuaManager->addCustomLocalScript(
                            object.ptr(), *scriptId, cfg[*scriptId].mInitializationData);
                };
                objectT["hasScript"] = [lua = context.mLua](const GObject& object, std::string_view path) {
                    const LuaUtil::ScriptsConfiguration& cfg = lua->getConfiguration();
                    std::optional<int> scriptId = cfg.findId(VFS::Path::Normalized(path));
                    if (!scriptId)
                        return false;
                    MWWorld::Ptr ptr = object.ptr();
                    LocalScripts* localScripts = ptr.getRefData().getLuaScripts();
                    if (localScripts)
                        return localScripts->hasScript(*scriptId);
                    else
                        return false;
                };
                objectT["removeScript"] = [lua = context.mLua](const GObject& object, std::string_view path) {
                    const LuaUtil::ScriptsConfiguration& cfg = lua->getConfiguration();
                    std::optional<int> scriptId = cfg.findId(VFS::Path::Normalized(path));
                    if (!scriptId)
                        throw std::runtime_error("Unknown script: " + std::string(path));
                    MWWorld::Ptr ptr = object.ptr();
                    LocalScripts* localScripts = ptr.getRefData().getLuaScripts();
                    if (!localScripts || !localScripts->hasScript(*scriptId))
                        throw std::runtime_error("There is no script " + std::string(path) + " on " + ptr.toString());
                    if (localScripts->getAutoStartConf().count(*scriptId) > 0)
                        throw std::runtime_error("Autostarted script can not be removed: " + std::string(path));
                    localScripts->removeScript(*scriptId);
                };

                using DelayedRemovalFn = std::function<void(MWWorld::Ptr)>;
                auto removeFn = [](const MWWorld::Ptr ptr, int countToRemove) -> std::optional<DelayedRemovalFn> {
                    int rawCount = ptr.getCellRef().getCount(false);
                    int currentCount = std::abs(rawCount);
                    int signedCountToRemove = (rawCount < 0 ? -1 : 1) * countToRemove;

                    if (countToRemove <= 0 || countToRemove > currentCount)
                        throw std::runtime_error("Can't remove " + std::to_string(countToRemove) + " of "
                            + std::to_string(currentCount) + " items");
                    ptr.getCellRef().setCount(rawCount - signedCountToRemove); // Immediately change count
                    if (!ptr.getContainerStore() && currentCount > countToRemove)
                        return std::nullopt;
                    // Delayed action to trigger side effects
                    return [signedCountToRemove](MWWorld::Ptr p) {
                        // Restore the original count
                        p.getCellRef().setCount(p.getCellRef().getCount(false) + signedCountToRemove);
                        // And now remove properly
                        if (p.getContainerStore())
                            p.getContainerStore()->remove(p, std::abs(signedCountToRemove), false);
                        else
                        {
                            MWBase::Environment::get().getWorld()->disable(p);
                            MWBase::Environment::get().getWorld()->deleteObject(p);
                        }
                    };
                };
                objectT["remove"] = [removeFn, context](const GObject& object, sol::optional<int> count) {
                    std::optional<DelayedRemovalFn> delayed
                        = removeFn(object.ptr(), count.value_or(object.ptr().getCellRef().getCount()));
                    if (delayed.has_value())
                        context.mLuaManager->addAction([fn = *delayed, object] { fn(object.ptr()); });
                };
                objectT["split"] = [removeFn, context](const GObject& object, int count) -> GObject {
                    // Doesn't matter which cell to use because the new instance will be in disabled state.
                    MWWorld::CellStore* cell = MWBase::Environment::get().getWorldScene()->getCurrentCell();

                    const MWWorld::Ptr& ptr = object.ptr();
                    MWWorld::Ptr splitted = ptr.getClass().copyToCell(ptr, *cell, count);
                    splitted.getRefData().disable();

                    std::optional<DelayedRemovalFn> delayedRemovalFn = removeFn(ptr, count);
                    if (delayedRemovalFn.has_value())
                        context.mLuaManager->addAction([fn = *delayedRemovalFn, object] { fn(object.ptr()); });

                    return GObject(splitted);
                };
                objectT["moveInto"] = [removeFn, context](const GObject& object, const sol::object& dest) {
                    const MWWorld::Ptr& ptr = object.ptr();
                    int count = ptr.getCellRef().getCount();
                    MWWorld::Ptr destPtr;
                    if (dest.is<GObject>())
                        destPtr = dest.as<GObject>().ptr();
                    else
                        destPtr = LuaUtil::cast<Inventory<GObject>>(dest).mObj.ptr();
                    destPtr.getClass().getContainerStore(destPtr); // raises an error if there is no container store

                    std::optional<DelayedRemovalFn> delayedRemovalFn = removeFn(ptr, count);
                    context.mLuaManager->addAction([item = object, count, cont = GObject(destPtr), delayedRemovalFn] {
                        const MWWorld::Ptr& oldPtr = item.ptr();
                        auto& refData = oldPtr.getCellRef();
                        refData.setCount(count); // temporarily undo removal to run ContainerStore::add
                        oldPtr.getRefData().enable();
                        cont.ptr().getClass().getContainerStore(cont.ptr()).add(oldPtr, count, false);
                        refData.setCount(0);
                        if (delayedRemovalFn.has_value())
                            (*delayedRemovalFn)(oldPtr);
                    });
                };
                objectT["teleport"] = [removeFn, context](const GObject& object, const sol::object& cellOrName,
                                          const osg::Vec3f& pos, const sol::object& options) {
                    MWWorld::CellStore* cell = findCell(cellOrName, pos);
                    MWWorld::Ptr ptr = object.ptr();
                    int count = ptr.getCellRef().getCount();
                    if (count == 0)
                        throw std::runtime_error("Object is either removed or already in the process of teleporting");
                    osg::Vec3f rot = ptr.getRefData().getPosition().asRotationVec3();
                    bool placeOnGround = false;
                    if (LuaUtil::isTransform(options))
                        rot = toEulerRotation(options, ptr.getClass().isActor());
                    else if (options != sol::nil)
                    {
                        sol::table t = LuaUtil::cast<sol::table>(options);
                        sol::object rotationArg = t["rotation"];
                        if (rotationArg != sol::nil)
                            rot = toEulerRotation(rotationArg, ptr.getClass().isActor());
                        placeOnGround = LuaUtil::getValueOrDefault(t["onGround"], placeOnGround);
                    }
                    if (ptr.getContainerStore())
                    {
                        DelayedRemovalFn delayedRemovalFn = *removeFn(ptr, count);
                        context.mLuaManager->addAction(
                            [object, cell, pos, rot, count, delayedRemovalFn, placeOnGround] {
                                MWWorld::Ptr oldPtr = object.ptr();
                                oldPtr.getCellRef().setCount(count);
                                MWWorld::Ptr newPtr = oldPtr.getClass().moveToCell(oldPtr, *cell);
                                oldPtr.getCellRef().setCount(0);
                                newPtr.getRefData().disable();
                                teleportNotPlayer(newPtr, cell, pos, rot, placeOnGround);
                                delayedRemovalFn(oldPtr);
                            },
                            "TeleportFromContainerAction");
                    }
                    else if (ptr == MWBase::Environment::get().getWorld()->getPlayerPtr())
                        context.mLuaManager->addTeleportPlayerAction(
                            [cell, pos, rot, placeOnGround] { teleportPlayer(cell, pos, rot, placeOnGround); });
                    else
                    {
                        ptr.getCellRef().setCount(0);
                        context.mLuaManager->addAction(
                            [object, cell, pos, rot, count, placeOnGround] {
                                object.ptr().getCellRef().setCount(count);
                                teleportNotPlayer(object.ptr(), cell, pos, rot, placeOnGround);
                            },
                            "TeleportAction");
                    }
                };
            }
        }

        template <class ObjectT>
        void addInventoryBindings(sol::usertype<ObjectT>& objectT, const std::string& prefix, const Context& context)
        {
            using InventoryT = Inventory<ObjectT>;
            sol::usertype<InventoryT> inventoryT = context.sol().new_usertype<InventoryT>(prefix + "Inventory");

            inventoryT[sol::meta_function::to_string]
                = [](const InventoryT& inv) { return "Inventory[" + inv.mObj.toString() + "]"; };

            inventoryT["getAll"] = [ids = getPackageToTypeTable(context.mLua->unsafeState())](
                                       const InventoryT& inventory, sol::optional<sol::table> type) {
                int mask = -1;
                sol::optional<uint32_t> typeId = sol::nullopt;
                if (type.has_value())
                    typeId = ids[*type];
                else
                    mask = MWWorld::ContainerStore::Type_All;

                if (typeId.has_value())
                {
                    switch (*typeId)
                    {
                        case ESM::REC_ALCH:
                            mask = MWWorld::ContainerStore::Type_Potion;
                            break;
                        case ESM::REC_ARMO:
                            mask = MWWorld::ContainerStore::Type_Armor;
                            break;
                        case ESM::REC_BOOK:
                            mask = MWWorld::ContainerStore::Type_Book;
                            break;
                        case ESM::REC_CLOT:
                            mask = MWWorld::ContainerStore::Type_Clothing;
                            break;
                        case ESM::REC_INGR:
                            mask = MWWorld::ContainerStore::Type_Ingredient;
                            break;
                        case ESM::REC_LIGH:
                            mask = MWWorld::ContainerStore::Type_Light;
                            break;
                        case ESM::REC_MISC:
                            mask = MWWorld::ContainerStore::Type_Miscellaneous;
                            break;
                        case ESM::REC_WEAP:
                            mask = MWWorld::ContainerStore::Type_Weapon;
                            break;
                        case ESM::REC_APPA:
                            mask = MWWorld::ContainerStore::Type_Apparatus;
                            break;
                        case ESM::REC_LOCK:
                            mask = MWWorld::ContainerStore::Type_Lockpick;
                            break;
                        case ESM::REC_PROB:
                            mask = MWWorld::ContainerStore::Type_Probe;
                            break;
                        case ESM::REC_REPA:
                            mask = MWWorld::ContainerStore::Type_Repair;
                            break;
                        default:;
                    }
                }

                if (mask == -1)
                    throw std::runtime_error(
                        std::string("Incorrect type argument in inventory:getAll: " + LuaUtil::toString(*type)));

                const MWWorld::Ptr& ptr = inventory.mObj.ptr();
                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
                ObjectIdList list = std::make_shared<std::vector<ObjectId>>();
                auto it = store.begin(mask);
                while (it.getType() != -1)
                {
                    const MWWorld::Ptr& item = *(it++);
                    MWBase::Environment::get().getWorldModel()->registerPtr(item);
                    list->push_back(getId(item));
                }
                return ObjectList<ObjectT>{ std::move(list) };
            };

            inventoryT["countOf"] = [](const InventoryT& inventory, std::string_view recordId) {
                const MWWorld::Ptr& ptr = inventory.mObj.ptr();
                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
                return store.count(ESM::RefId::deserializeText(recordId));
            };
            if constexpr (std::is_same_v<ObjectT, GObject>)
            {
                inventoryT["resolve"] = [](const InventoryT& inventory) {
                    const MWWorld::Ptr& ptr = inventory.mObj.ptr();
                    MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
                    store.resolve();
                };
            }
            inventoryT["isResolved"] = [](const InventoryT& inventory) -> bool {
                const MWWorld::Ptr& ptr = inventory.mObj.ptr();
                // Avoid initializing custom data
                if (!ptr.getRefData().getCustomData())
                    return false;
                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
                return store.isResolved();
            };
            inventoryT["find"] = [](const InventoryT& inventory, std::string_view recordId) -> sol::optional<ObjectT> {
                const MWWorld::Ptr& ptr = inventory.mObj.ptr();
                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
                auto itemId = ESM::RefId::deserializeText(recordId);
                for (const MWWorld::Ptr& item : store)
                {
                    if (item.getCellRef().getRefId() == itemId)
                    {
                        MWBase::Environment::get().getWorldModel()->registerPtr(item);
                        return ObjectT(getId(item));
                    }
                }
                return sol::nullopt;
            };
            inventoryT["findAll"] = [](const InventoryT& inventory, std::string_view recordId) {
                const MWWorld::Ptr& ptr = inventory.mObj.ptr();
                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
                auto itemId = ESM::RefId::deserializeText(recordId);
                ObjectIdList list = std::make_shared<std::vector<ObjectId>>();
                for (const MWWorld::Ptr& item : store)
                {
                    if (item.getCellRef().getRefId() == itemId)
                    {
                        MWBase::Environment::get().getWorldModel()->registerPtr(item);
                        list->push_back(getId(item));
                    }
                }
                return ObjectList<ObjectT>{ std::move(list) };
            };
        }

        template <class ObjectT>
        void initObjectBindings(const std::string& prefix, const Context& context)
        {
            sol::usertype<ObjectT> objectT
                = context.sol().new_usertype<ObjectT>(prefix + "Object", sol::base_classes, sol::bases<Object>());
            addBasicBindings<ObjectT>(objectT, context);
            addInventoryBindings<ObjectT>(objectT, prefix, context);
            addOwnerbindings<ObjectT>(objectT, prefix, context);

            registerObjectList<ObjectT>(prefix, context);
        }
    } // namespace

    void initObjectBindingsForLocalScripts(const Context& context)
    {
        initObjectBindings<LObject>("L", context);
    }

    void initObjectBindingsForGlobalScripts(const Context& context)
    {
        initObjectBindings<GObject>("G", context);
    }

}
