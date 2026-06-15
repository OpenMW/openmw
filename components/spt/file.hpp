#ifndef OPENMW_COMPONENTS_SPT_FILE_HPP
#define OPENMW_COMPONENTS_SPT_FILE_HPP

#include <memory>

#include <components/files/istreamptr.hpp>

namespace Spt
{
    class SPTStream;

    struct SPTFile
    {
        SPTFile() = default;
        virtual void read(SPTStream& stream);
        virtual ~SPTFile() = default;
    };

    using SPTFilePtr = std::shared_ptr<const Spt::SPTFile>;

    SPTFilePtr parse(Files::IStreamPtr&& stream);
}

#endif