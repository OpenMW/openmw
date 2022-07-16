#ifndef OPENMW_COMPONENTS_FILES_ISTREAMPTR_H
#define OPENMW_COMPONENTS_FILES_ISTREAMPTR_H

#include <iosfwd>
#include <memory>

namespace Files
{
    using IStreamPtr = std::unique_ptr<std::istream>;
}

#endif
