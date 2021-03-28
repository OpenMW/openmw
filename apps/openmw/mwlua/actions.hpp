#ifndef MWLUA_ACTIONS_H
#define MWLUA_ACTIONS_H

#include <variant>

#include "object.hpp"
#include "worldview.hpp"

namespace MWLua
{

    // Some changes to the game world can not be done from the scripting thread (because it runs in parallel with OSG Cull),
    // so we need to queue it and apply from the main thread. All such changes should be implemented as classes inherited
    // from MWLua::Action.

    class Action
    {
    public:
        virtual ~Action() {}
        virtual void apply(WorldView&) const = 0;
    };

    class TeleportAction final : public Action
    {
    public:
        TeleportAction(ObjectId object, std::string cell, const osg::Vec3f& pos, const osg::Vec3f& rot)
            : mObject(object), mCell(std::move(cell)), mPos(pos), mRot(rot) {}

        void apply(WorldView&) const override;

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

        SetEquipmentAction(ObjectId actor, Equipment equipment) : mActor(actor), mEquipment(std::move(equipment)) {}

        void apply(WorldView&) const override;

    private:
        ObjectId mActor;
        Equipment mEquipment;
    };

}

#endif // MWLUA_ACTIONS_H
