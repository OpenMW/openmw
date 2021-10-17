#ifndef OPENMW_COMPONENTS_CONTENT_LESSBYID_H
#define OPENMW_COMPONENTS_CONTENT_LESSBYID_H

#include <string_view>

namespace EsmLoader
{
    struct LessById
    {
        template <class T>
        bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs.mId < rhs.mId;
        }

        template <class T>
        bool operator()(const T& lhs, std::string_view rhs) const
        {
            return lhs.mId < rhs;
        }
    };
}

#endif
