#ifndef OPENMW_COMPONENTS_SPT_FILE_HPP
#define OPENMW_COMPONENTS_SPT_FILE_HPP

#include <memory>
#include <string>

#include <components/files/istreamptr.hpp>

namespace Spt
{
    struct SPTFile
    {
        std::string mVersionId;

        SPTFile() = default;
    };

    using SPTFilePtr = std::shared_ptr<const Spt::SPTFile>;

    SPTFilePtr parse(Files::IStreamPtr&& stream);
}

#endif