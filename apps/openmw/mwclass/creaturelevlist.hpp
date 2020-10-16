#ifndef GAME_MWCLASS_CREATURELEVLIST_H
#define GAME_MWCLASS_CREATURELEVLIST_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class CreatureLevList : public MWWorld::Class
    {
        void ensureCustomData (const MWWorld::Ptr& ptr) const;

        public:

            std::string getName (const MWWorld::ConstPtr& ptr) const override;
            ///< \return name or ID; can return an empty string.

            bool hasToolTip (const MWWorld::ConstPtr& ptr) const override;
            ///< @return true if this object has a tooltip when focused (default implementation: true)

            static void registerSelf();

            void getModelsToPreload(const MWWorld::Ptr& ptr, std::vector<std::string>& models) const override;
            ///< Get a list of models to preload that this object may use (directly or indirectly). default implementation: list getModel().

            void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const override;
            ///< Add reference into a cell for rendering

            void readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const override;
            ///< Read additional state from \a state into \a ptr.

            void writeAdditionalState (const MWWorld::ConstPtr& ptr, ESM::ObjectState& state) const override;
            ///< Write additional state from \a ptr into \a state.

            void respawn (const MWWorld::Ptr& ptr) const override;
    };
}

#endif
