#ifndef GAME_MWCLASS_CREATURELEVLIST_H
#define GAME_MWCLASS_CREATURELEVLIST_H

#include "../mwworld/registeredclass.hpp"

namespace MWClass
{
    class CreatureLevList : public MWWorld::RegisteredClass<CreatureLevList>
    {
        friend MWWorld::RegisteredClass<CreatureLevList>;

        CreatureLevList();

        void ensureCustomData(const MWWorld::Ptr& ptr) const;

    public:
        std::string_view getName(const MWWorld::ConstPtr& ptr) const override;
        ///< \return name or ID; can return an empty string.

        bool hasToolTip(const MWWorld::ConstPtr& ptr) const override;
        ///< @return true if this object has a tooltip when focused (default implementation: true)

        void insertObjectRendering(const MWWorld::Ptr& ptr, const std::string& model,
            MWRender::RenderingInterface& renderingInterface) const override;
        ///< Add reference into a cell for rendering

        void readAdditionalState(const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const override;
        ///< Read additional state from \a state into \a ptr.

        void writeAdditionalState(const MWWorld::ConstPtr& ptr, ESM::ObjectState& state) const override;
        ///< Write additional state from \a ptr into \a state.

        void respawn(const MWWorld::Ptr& ptr) const override;

        MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const override;

        void adjustPosition(const MWWorld::Ptr& ptr, bool force) const override;
    };
}

#endif
