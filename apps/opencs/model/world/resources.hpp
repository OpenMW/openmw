#ifndef CSM_WOLRD_RESOURCES_H
#define CSM_WOLRD_RESOURCES_H

#include <string>
#include <map>
#include <vector>

namespace CSMWorld
{
    class Resources
    {
            std::map<std::string, int> mIndex;
            std::vector<std::string> mFiles;
            std::string mBaseDirectory;

        public:

            Resources (const std::string& baseDirectory);

            int getSize() const;

            std::string getId (int index) const;

            int getIndex (const std::string& id) const;

            int searchId (const std::string& id) const;
    };
}

#endif
