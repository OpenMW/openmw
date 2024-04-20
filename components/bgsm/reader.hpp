#ifndef OPENMW_COMPONENTS_BGSM_READER_HPP
#define OPENMW_COMPONENTS_BGSM_READER_HPP

#include <memory>

#include <components/files/istreamptr.hpp>

#include "file.hpp"

namespace Bgsm
{
    class Reader
    {
        std::unique_ptr<MaterialFile> mFile;

    public:
        void parse(Files::IStreamPtr&& stream);

        std::unique_ptr<MaterialFile> getFile() { return std::move(mFile); }
    };
}
#endif
