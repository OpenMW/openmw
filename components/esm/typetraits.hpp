#ifndef OPENMW_COMPONENTS_ESM_TYPETRAITS
#define OPENMW_COMPONENTS_ESM_TYPETRAITS

namespace ESM
{
    template <class T>
    concept HasId = requires
    {
        T::mId;
    };

    template <class T>
    concept HasModel = requires
    {
        T::mModel;
    };
}

#endif // OPENMW_COMPONENTS_ESM_TYPETRAITS
