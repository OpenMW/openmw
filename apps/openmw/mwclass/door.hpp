#ifndef GAME_MWCLASS_DOOR_H
#define GAME_MWCLASS_DOOR_H

#include <components/esm3/loaddoor.hpp>

#include "../mwworld/registeredclass.hpp"

namespace MWClass
{
    class Door : public MWWorld::RegisteredClass<Door>
    {
        friend MWWorld::RegisteredClass<Door>;

        Door();

        void ensureCustomData(const MWWorld::Ptr& ptr) const;

        MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const override;

    public:
        void insertObjectRendering(const MWWorld::Ptr& ptr, const std::string& model,
            MWRender::RenderingInterface& renderingInterface) const override;
        ///< Add reference into a cell for rendering

        void insertObject(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics) const override;
        void insertObjectPhysics(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics) const override;

        bool isDoor() const override;

        bool useAnim() const override;

        std::string_view getName(const MWWorld::ConstPtr& ptr) const override;
        ///< \return name or ID; can return an empty string.

        std::unique_ptr<MWWorld::Action> activate(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor) const override;
        ///< Generate action for activation

        MWGui::ToolTipInfo getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const override;
        ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

        static std::string getDestination(const MWWorld::LiveCellRef<ESM::Door>& door);
        ///< @return destination cell name or token

        bool canLock(const MWWorld::ConstPtr& ptr) const override;

        bool allowTelekinesis(const MWWorld::ConstPtr& ptr) const override;
        ///< Return whether this class of object can be activated with telekinesis

        ESM::RefId getScript(const MWWorld::ConstPtr& ptr) const override;
        ///< Return name of the script attached to ptr

        std::string_view getModel(const MWWorld::ConstPtr& ptr) const override;

        MWWorld::DoorState getDoorState(const MWWorld::ConstPtr& ptr) const override;
        /// This does not actually cause the door to move. Use World::activateDoor instead.
        void setDoorState(const MWWorld::Ptr& ptr, MWWorld::DoorState state) const override;

        void readAdditionalState(const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const override;
        ///< Read additional state from \a state into \a ptr.

        void writeAdditionalState(const MWWorld::ConstPtr& ptr, ESM::ObjectState& state) const override;
        ///< Write additional state from \a ptr into \a state.
    };
}

#endif
