#ifndef MWLUA_ACTIONS_H
#define MWLUA_ACTIONS_H

#include <variant>

#include "object.hpp"
#include "worldview.hpp"

namespace LuaUtil
{
    class LuaState;
}

namespace MWLua
{

    // Some changes to the game world can not be done from the scripting thread (because it runs in parallel with OSG Cull),
    // so we need to queue it and apply from the main thread. All such changes should be implemented as classes inherited
    // from MWLua::Action.

    class Action
    {
    public:
        Action(LuaUtil::LuaState* state);
        virtual ~Action() {}

        void safeApply(WorldView&) const;
        virtual void apply(WorldView&) const = 0;
        virtual std::string toString() const = 0;

    private:
#ifndef NDEBUG
        std::string mCallerTraceback;
#endif
    };

    class TeleportAction final : public Action
    {
    public:
        TeleportAction(LuaUtil::LuaState* state, ObjectId object, std::string cell, const osg::Vec3f& pos, const osg::Vec3f& rot)
            : Action(state), mObject(object), mCell(std::move(cell)), mPos(pos), mRot(rot) {}

        void apply(WorldView&) const override;
        std::string toString() const override { return "TeleportAction"; }

    private:
        ObjectId mObject;
        std::string mCell;
        osg::Vec3f mPos;
        osg::Vec3f mRot;
    };

    class SetEquipmentAction final : public Action
    {
    public:
        using Item = std::variant<std::string, ObjectId>;  // recordId or ObjectId
        using Equipment = std::map<int, Item>;  // slot to item

        SetEquipmentAction(LuaUtil::LuaState* state, ObjectId actor, Equipment equipment)
            : Action(state), mActor(actor), mEquipment(std::move(equipment)) {}

        void apply(WorldView&) const override;
        std::string toString() const override { return "SetEquipmentAction"; }

    private:
        ObjectId mActor;
        Equipment mEquipment;
    };

}

#endif // MWLUA_ACTIONS_H
