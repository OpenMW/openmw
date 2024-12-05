#include "actorutil.hpp"

#include <components/settings/values.hpp>
#include <components/vfs/pathutil.hpp>

namespace MWRender
{
    const std::string& getActorSkeleton(bool firstPerson, bool isFemale, bool isBeast, bool isWerewolf)
    {
        if (!firstPerson)
        {
            if (isWerewolf)
                return Settings::models().mWolfskin.get().value();
            else if (isBeast)
                return Settings::models().mBaseanimkna.get().value();
            else if (isFemale)
                return Settings::models().mBaseanimfemale.get().value();
            else
                return Settings::models().mBaseanim.get().value();
        }
        else
        {
            if (isWerewolf)
                return Settings::models().mWolfskin1st.get().value();
            else if (isBeast)
                return Settings::models().mBaseanimkna1st.get().value();
            else if (isFemale)
                return Settings::models().mBaseanimfemale1st.get().value();
            else
                return Settings::models().mXbaseanim1st.get().value();
        }
    }

    bool isDefaultActorSkeleton(std::string_view model)
    {
        return VFS::Path::pathEqual(Settings::models().mBaseanimkna.get(), model)
            || VFS::Path::pathEqual(Settings::models().mBaseanimfemale.get(), model)
            || VFS::Path::pathEqual(Settings::models().mBaseanim.get(), model);
    }

    std::string addSuffixBeforeExtension(const std::string& filename, const std::string& suffix)
    {
        size_t dotPos = filename.rfind('.');

        // No extension found; return the original filename with suffix appended
        if (dotPos == std::string::npos)
            return filename + suffix;

        // Insert the suffix before the dot (extension) and return the new filename
        return filename.substr(0, dotPos) + suffix + filename.substr(dotPos);
    }
}
