#ifndef GAME_RENDER_ESM4NPCANIMATION_H
#define GAME_RENDER_ESM4NPCANIMATION_H

#include "animation.hpp"

namespace MWRender
{
    class ESM4NpcAnimation : public Animation
    {
    public:
        ESM4NpcAnimation(
            const MWWorld::Ptr& ptr, osg::ref_ptr<osg::Group> parentNode, Resource::ResourceSystem* resourceSystem);

    private:
        void insertMesh(std::string_view model);

        void insertTes4NpcBodyPartsAndEquipment();
        void insertTes5NpcBodyPartsAndEquipment();
    };
}

#endif // GAME_RENDER_ESM4NPCANIMATION_H
