#ifndef GAME_MWCLASS_CONTAINER_H
#define GAME_MWCLASS_CONTAINER_H

#include "../mwworld/class.hpp"
#include "../mwworld/customdata.hpp"

namespace ESM
{
    struct Container;
    struct InventoryState;
}

namespace MWClass
{
    class ResolutionListener;

    class ContainerCustomData : public MWWorld::CustomData
    {
        /// A store changed by the player and therefore saved.
        std::unique_ptr<MWWorld::ContainerStore> mResolvedStore;
        /// A reflection of the base record that does not get saved.
        /// Does not contain leveled items.
        std::unique_ptr<MWWorld::ContainerStore> mUnresolvedStore;
        /// The seed used to resolve leveled items so a container contains the same items until the game is restarted.
        const unsigned int mSeed;
        /// Swaps the unresolved store for a resolved store if a change has been made.
        std::weak_ptr<ResolutionListener> mListener;

        void assignListener(std::shared_ptr<ResolutionListener>& listener, MWWorld::CellStore* cell);
    public:
        ContainerCustomData(const ESM::Container& container, MWWorld::CellStore* cell);
        ContainerCustomData(const ESM::InventoryState& inventory);
        ContainerCustomData(const ContainerCustomData& other);

        virtual MWWorld::CustomData *clone() const;

        virtual ContainerCustomData& asContainerCustomData();
        virtual const ContainerCustomData& asContainerCustomData() const;

        const MWWorld::ContainerStore& getImmutable(std::shared_ptr<ResolutionListener>& listener, MWWorld::CellStore* cell);

        MWWorld::ContainerStore& getMutable(std::shared_ptr<ResolutionListener>& listener, const ESM::Container& container, MWWorld::CellStore* cell);

        bool isModified() const;

        friend class Container;
        friend class ResolutionListener;
    };

    class Container : public MWWorld::Class
    {
            void ensureCustomData (const MWWorld::Ptr& ptr) const;


            virtual MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const;

        public:

            virtual void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual void insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const;

            virtual std::string getName (const MWWorld::ConstPtr& ptr) const;
            ///< \return name or ID; can return an empty string.

            virtual std::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor) const;
            ///< Generate action for activation

            virtual bool hasToolTip (const MWWorld::ConstPtr& ptr) const;
            ///< @return true if this object has a tooltip when focused (default implementation: true)

            virtual MWGui::ToolTipInfo getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            virtual MWWorld::StoreManager getStoreManager (const MWWorld::Ptr& ptr) const;
            ///< Return container store

            virtual std::string getScript (const MWWorld::ConstPtr& ptr) const;
            ///< Return name of the script attached to ptr

            virtual float getCapacity (const MWWorld::Ptr& ptr) const;
            ///< Return total weight that fits into the object. Throws an exception, if the object can't
            /// hold other objects.

            virtual float getEncumbrance (const MWWorld::Ptr& ptr) const;
            ///< Returns total weight of objects inside this object (including modifications from magic
            /// effects). Throws an exception, if the object can't hold other objects.

            virtual bool canLock(const MWWorld::ConstPtr &ptr) const;

            virtual void readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state)
                const;
            ///< Read additional state from \a state into \a ptr.

            virtual void writeAdditionalState (const MWWorld::ConstPtr& ptr, ESM::ObjectState& state)
                const;
            ///< Write additional state from \a ptr into \a state.

            static void registerSelf();

            virtual void respawn (const MWWorld::Ptr& ptr) const;

            virtual std::string getModel(const MWWorld::ConstPtr &ptr) const;

            virtual bool useAnim() const;
    };
}

#endif
