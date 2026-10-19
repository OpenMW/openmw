#ifndef OPENMW_MWRENDER_TEXTUREREPORT_H
#define OPENMW_MWRENDER_TEXTUREREPORT_H

#include <iosfwd>

#include <components/vfs/pathutil.hpp>

namespace MWWorld
{
    class Ptr;
}

namespace MWRender
{
    // Lists the textures bound to the object's scene graph, falling back to its model's template, then each NPC body
    // part with its model and textures. Output for the BetaComment console command.
    void reportTextures(std::ostream& msg, const MWWorld::Ptr& ptr, const VFS::Path::Normalized& model);
}

#endif
