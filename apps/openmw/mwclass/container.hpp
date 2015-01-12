#ifndef GAME_MWCLASS_CONTAINER_H
#define GAME_MWCLASS_CONTAINER_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Container : public MWWorld::Class
    {
            void ensureCustomData (const MWWorld::Ptr& ptr) const;


            virtual MWWorld::Ptr
            copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const;

        public:

            /// Return ID of \a ptr
            virtual std::string getId (const MWWorld::Ptr& ptr) const;

            virtual void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual void insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const;

            virtual std::string getName (const MWWorld::Ptr& ptr) const;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            virtual boost::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor) const;
            ///< Generate action for activation

            virtual bool hasToolTip (const MWWorld::Ptr& ptr) const;
            ///< @return true if this object has a tooltip when focused (default implementation: false)

            virtual MWGui::ToolTipInfo getToolTipInfo (const MWWorld::Ptr& ptr) const;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            virtual MWWorld::ContainerStore& getContainerStore (const MWWorld::Ptr& ptr) const;
            ///< Return container store

            virtual std::string getScript (const MWWorld::Ptr& ptr) const;
            ///< Return name of the script attached to ptr

            virtual float getCapacity (const MWWorld::Ptr& ptr) const;
            ///< Return total weight that fits into the object. Throws an exception, if the object can't
            /// hold other objects.

            virtual float getEncumbrance (const MWWorld::Ptr& ptr) const;
            ///< Returns total weight of objects inside this object (including modifications from magic
            /// effects). Throws an exception, if the object can't hold other objects.

            virtual void lock (const MWWorld::Ptr& ptr, int lockLevel = 0) const;
            ///< Lock object

            virtual void unlock (const MWWorld::Ptr& ptr) const;
            ///< Unlock object

            virtual void readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state)
                const;
            ///< Read additional state from \a state into \a ptr.

            virtual void writeAdditionalState (const MWWorld::Ptr& ptr, ESM::ObjectState& state)
                const;
            ///< Write additional state from \a ptr into \a state.

            static void registerSelf();

            virtual void respawn (const MWWorld::Ptr& ptr) const;

            virtual void restock (const MWWorld::Ptr &ptr) const;

            virtual std::string getModel(const MWWorld::Ptr &ptr) const;
    };
}

#endif
