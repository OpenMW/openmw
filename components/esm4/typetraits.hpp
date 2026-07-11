#ifndef OPENMW_COMPONENTS_ESM4_TYPETRAITS
#define OPENMW_COMPONENTS_ESM4_TYPETRAITS

namespace ESM4
{
    template <class T>
    concept HasParent = requires
    {
        T::mParent;
    };

    template <class T>
    concept HasFlags = requires
    {
        T::mFlags;
    };

    template <class T>
    concept HasEditorId = requires
    {
        T::mEditorId;
    };

    template <class T>
    concept HasFullName = requires
    {
        T::mFullName;
    };

    template <class T>
    concept HasCellFlags = requires
    {
        T::mCellFlags;
    };

    template <class T>
    concept HasX = requires
    {
        T::mX;
    };

    template <class T>
    concept HasY = requires
    {
        T::mY;
    };

    template <class T>
    concept HasModelMale = requires
    {
        T::mModelMale;
    };

    template <class T>
    concept HasModelMaleWorld = requires
    {
        T::mModelMaleWorld;
    };

    template <class T>
    concept HasModelFemale = requires
    {
        T::mModelFemale;
    };

    template <class T>
    concept HasModelFemaleWorld = requires
    {
        T::mModelFemaleWorld;
    };

    template <class T>
    concept HasNif = requires
    {
        T::mNif;
    };

    template <class T>
    concept HasKf = requires
    {
        T::mKf;
    };

    template <class T>
    concept HasType = requires
    {
        T::mType;
    };

    template <class T>
    concept HasValue = requires
    {
        T::mValue;
    };

    template <class T>
    concept HasData = requires
    {
        T::mData;
    };
}

#endif // OPENMW_COMPONENTS_ESM4_TYPETRAITS
