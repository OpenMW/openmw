#ifndef GAME_MWCLASS_DOOR_H
#define GAME_MWCLASS_DOOR_H

#include <components/esm/loaddoor.hpp>

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Door : public MWWorld::Class
    {
            void ensureCustomData (const MWWorld::Ptr& ptr) const;

            virtual MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const;

        public:

            virtual void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual void insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const;

            virtual bool useAnim() const;

            virtual std::string getName (const MWWorld::ConstPtr& ptr) const;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            virtual std::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor) const;
            ///< Generate action for activation

            virtual bool hasToolTip (const MWWorld::ConstPtr& ptr) const;
            ///< @return true if this object has a tooltip when focused (default implementation: false)

            virtual MWGui::ToolTipInfo getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            static std::string getDestination (const MWWorld::LiveCellRef<ESM::Door>& door);
            ///< @return destination cell name or token

            virtual void lock (const MWWorld::Ptr& ptr, int lockLevel = 0) const;
            ///< Lock object

            virtual void unlock (const MWWorld::Ptr& ptr) const;
            ///< Unlock object

            virtual bool canLock(const MWWorld::ConstPtr &ptr) const;

            virtual bool allowTelekinesis(const MWWorld::ConstPtr &ptr) const;
            ///< Return whether this class of object can be activated with telekinesis

            virtual std::string getScript (const MWWorld::ConstPtr& ptr) const;
            ///< Return name of the script attached to ptr

            static void registerSelf();

            virtual std::string getModel(const MWWorld::ConstPtr &ptr) const;

            /// 0 = nothing, 1 = opening, 2 = closing
            virtual int getDoorState (const MWWorld::ConstPtr &ptr) const;
            /// This does not actually cause the door to move. Use World::activateDoor instead.
            virtual void setDoorState (const MWWorld::Ptr &ptr, int state) const;


            virtual void readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state)
                const;
            ///< Read additional state from \a state into \a ptr.

            virtual void writeAdditionalState (const MWWorld::ConstPtr& ptr, ESM::ObjectState& state)
                const;
            ///< Write additional state from \a ptr into \a state.
    };
}

#endif
