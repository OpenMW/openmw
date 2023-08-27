#ifndef VERSION_HPP
#define VERSION_HPP

#include <string>
#include <string_view>

namespace Version
{
    std::string_view getVersion();
    std::string_view getCommitHash();
    std::string_view getTagHash();
    int getLuaApiRevision();

    // Prepares string that contains version and commit hash.
    std::string getOpenmwVersionDescription();
}

#endif // VERSION_HPP
