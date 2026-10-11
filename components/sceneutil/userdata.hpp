#ifndef OPENMW_COMPONENTS_SCENEUTIL_USERDATA_HPP
#define OPENMW_COMPONENTS_SCENEUTIL_USERDATA_HPP

#include <utility>

#include <osg/CopyOp>
#include <osg/Object>
#include <osg/UserDataContainer>

namespace SceneUtil
{
    /// Carries a plain value in an osg::UserDataContainer, which only accepts osg::Object.
    /// Lets the payload type stay unaware that OSG exists.
    template <class T>
    class UserData final : public osg::Object
    {
    public:
        UserData() = default;

        explicit UserData(T value)
            : mValue(std::move(value))
        {
        }

        UserData(const UserData& copy, const osg::CopyOp& copyop)
            : osg::Object(copy, copyop)
            , mValue(copy.mValue)
        {
        }

        osg::Object* cloneType() const override { return new UserData; }

        osg::Object* clone(const osg::CopyOp& copyop) const override { return new UserData(*this, copyop); }

        bool isSameKindAs(const osg::Object* obj) const override
        {
            return dynamic_cast<const UserData*>(obj) != nullptr;
        }

        const char* libraryName() const override { return "SceneUtil"; }

        // ponytail: every instantiation reports one name. The serializer only uses it to skip these objects,
        // never to tell them apart, so a per-T name would buy nothing but a trait to specialise.
        const char* className() const override { return "UserData"; }

        T mValue{};
    };

    /// Returns the first T attached to the object, or nullptr. Never recurses into children.
    template <class T>
    T* findUserData(osg::Object& object)
    {
        osg::UserDataContainer* container = object.getUserDataContainer();
        if (container == nullptr)
            return nullptr;
        for (unsigned int i = 0; i < container->getNumUserObjects(); ++i)
            if (UserData<T>* data = dynamic_cast<UserData<T>*>(container->getUserObject(i)))
                return &data->mValue;
        return nullptr;
    }

    /// Calls f for every T attached to the object, in the order they were added. Never recurses into children.
    /// Needed where one object legitimately carries several of the same payload, e.g. the RefnumMarkers that
    /// the optimizer's merged geometry accumulates, one per source reference.
    template <class T, class F>
    void forEachUserData(osg::Object& object, F&& f)
    {
        osg::UserDataContainer* container = object.getUserDataContainer();
        if (container == nullptr)
            return;
        for (unsigned int i = 0; i < container->getNumUserObjects(); ++i)
            if (UserData<T>* data = dynamic_cast<UserData<T>*>(container->getUserObject(i)))
                f(data->mValue);
    }

    template <class T>
    void addUserData(osg::Object& object, T value)
    {
        object.getOrCreateUserDataContainer()->addUserObject(new UserData<T>(std::move(value)));
    }

}

#endif
