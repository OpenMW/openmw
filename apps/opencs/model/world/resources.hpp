#ifndef CSM_WOLRD_RESOURCES_H
#define CSM_WOLRD_RESOURCES_H

#include <string>
#include <map>
#include <vector>

#include "universalid.hpp"

namespace VFS
{
    class Manager;
}

namespace CSMWorld
{
    class Resources
    {
            std::map<std::string, int> mIndex;
            std::vector<std::string> mFiles;
            std::string mBaseDirectory;
            UniversalId::Type mType;

        public:

            /// \param type Type of resources in this table.
            Resources (const VFS::Manager* vfs, const std::string& baseDirectory, UniversalId::Type type,
                const char * const *extensions = nullptr);

            void recreate(const VFS::Manager* vfs, const char * const *extensions = nullptr);

            int getSize() const;

            std::string getId (int index) const;

            int getIndex (const std::string& id) const;

            int searchId (const std::string& id) const;

            UniversalId::Type getType() const;
    };
}

#endif
