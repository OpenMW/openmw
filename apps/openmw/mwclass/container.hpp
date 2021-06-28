#ifndef GAME_MWCLASS_CONTAINER_H
#define GAME_MWCLASS_CONTAINER_H

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/customdata.hpp"

namespace ESM
{
    struct Container;
    struct InventoryState;
}

namespace MWClass
{
    class ContainerCustomData : public MWWorld::TypedCustomData<ContainerCustomData>
    {
        MWWorld::ContainerStore mStore;
    public:
        ContainerCustomData(const ESM::Container& container, MWWorld::CellStore* cell);
        ContainerCustomData(const ESM::InventoryState& inventory);

        ContainerCustomData& asContainerCustomData() override;
        const ContainerCustomData& asContainerCustomData() const override;

        friend class Container;
    };

    class Container : public MWWorld::Class
    {
            bool mHarvestEnabled;

            void ensureCustomData (const MWWorld::Ptr& ptr) const;

            MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const override;

            bool canBeHarvested(const MWWorld::ConstPtr& ptr) const;

        public:
            Container();

            void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const override;
            ///< Add reference into a cell for rendering

            void insertObject(const MWWorld::Ptr& ptr, const std::string& model, osg::Quat rotation, MWPhysics::PhysicsSystem& physics, bool skipAnimated = false) const override;

            std::string getName (const MWWorld::ConstPtr& ptr) const override;
            ///< \return name or ID; can return an empty string.

            std::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor) const override;
            ///< Generate action for activation

            bool hasToolTip (const MWWorld::ConstPtr& ptr) const override;
            ///< @return true if this object has a tooltip when focused (default implementation: true)

            MWGui::ToolTipInfo getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const override;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            MWWorld::ContainerStore& getContainerStore (const MWWorld::Ptr& ptr) const override;
            ///< Return container store

            std::string getScript (const MWWorld::ConstPtr& ptr) const override;
            ///< Return name of the script attached to ptr

            float getCapacity (const MWWorld::Ptr& ptr) const override;
            ///< Return total weight that fits into the object. Throws an exception, if the object can't
            /// hold other objects.

            float getEncumbrance (const MWWorld::Ptr& ptr) const override;
            ///< Returns total weight of objects inside this object (including modifications from magic
            /// effects). Throws an exception, if the object can't hold other objects.

            bool canLock(const MWWorld::ConstPtr &ptr) const override;

            void readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state)
                const override;
            ///< Read additional state from \a state into \a ptr.

            void writeAdditionalState (const MWWorld::ConstPtr& ptr, ESM::ObjectState& state) const override;
            ///< Write additional state from \a ptr into \a state.

            static void registerSelf();

            void respawn (const MWWorld::Ptr& ptr) const override;

            std::string getModel(const MWWorld::ConstPtr &ptr) const override;

            bool useAnim() const override;

            void modifyBaseInventory(const std::string& containerId, const std::string& itemId, int amount) const override;
    };
}

#endif
