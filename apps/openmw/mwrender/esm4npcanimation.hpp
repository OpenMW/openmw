#ifndef GAME_RENDER_ESM4NPCANIMATION_H
#define GAME_RENDER_ESM4NPCANIMATION_H

#include "animation.hpp"

#include <functional>
#include <unordered_set>

#include <components/vfs/pathutil.hpp>

namespace ESM4
{
    struct Npc;
}

namespace MWWorld
{
    class ESMStore;
}

namespace MWRender
{
    std::unordered_set<VFS::Path::Normalized, VFS::Path::Hash, std::equal_to<>> getESM4ActorModels(
        const MWWorld::ESMStore& store);

    class ESM4NpcAnimation : public Animation
    {
    public:
        ESM4NpcAnimation(
            const MWWorld::Ptr& ptr, osg::ref_ptr<osg::Group> parentNode, Resource::ResourceSystem* resourceSystem);

    private:
        void insertPart(std::string_view model);

        // Works for FO3/FNV/TES5
        void insertHeadParts(const std::vector<ESM::FormId>& partIds, std::set<uint32_t>& usedHeadPartTypes);

        void updateParts();
        void updatePartsTES4(const ESM4::Npc& traits);
        void updatePartsTES5(const ESM4::Npc& traits);
    };
}

#endif // GAME_RENDER_ESM4NPCANIMATION_H
