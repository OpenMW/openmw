#ifndef OPENMW_COMPONENTS_ESM3_TYPETRAITS
#define OPENMW_COMPONENTS_ESM3_TYPETRAITS

namespace ESM
{
    template <class T>
    concept HasIndex = requires
    {
        T::mIndex;
    };

    template <class T>
    concept HasStringId = requires
    {
        T::mStringId;
    };
}

#endif // OPENMW_COMPONENTS_ESM3_TYPETRAITS
