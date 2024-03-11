#ifndef OPENMW_APPS_OPENMW_MWSOUND_CONSTANTS_H
#define OPENMW_APPS_OPENMW_MWSOUND_CONSTANTS_H

#include <components/vfs/pathutil.hpp>

namespace MWSound
{
    constexpr VFS::Path::NormalizedView battlePlaylist("battle");
    constexpr VFS::Path::NormalizedView explorePlaylist("explore");
}

#endif
