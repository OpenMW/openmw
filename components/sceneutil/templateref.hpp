#ifndef OPENMW_COMPONENTS_SCENEUTIL_TEMPLATEREF_H
#define OPENMW_COMPONENTS_SCENEUTIL_TEMPLATEREF_H

#include <osg/CopyOp>
#include <osg/Object>
#include <osg/UserDataContainer>
#include <osg/ref_ptr>

namespace SceneUtil
{
    /// Holds a reference to the object a shallow copy was made from, so the template outlives everything copied from
    /// it and a cache holding the template still sees it as in use. The wrapper is what makes that reference nameable:
    /// serialize.cpp ignores "SceneUtil::TemplateRef" so scene dumps skip the template, which a bare osg::Object
    /// parked in a user data container could not be singled out for. Nothing ever reads the reference back out.
    class TemplateRef : public osg::Object
    {
    public:
        TemplateRef() = default;

        explicit TemplateRef(const osg::Object* object)
            : mObject(object)
        {
        }

        TemplateRef(const TemplateRef& copy, const osg::CopyOp&)
            : mObject(copy.mObject)
        {
        }

        META_Object(SceneUtil, TemplateRef)

    private:
        osg::ref_ptr<const osg::Object> mObject;
    };

    /// Pin @a tmpl's lifetime to @a derived's. Call after a shallow copy; a null template is ignored.
    inline void addTemplateRef(osg::Object& derived, const osg::Object* tmpl)
    {
        if (tmpl != nullptr)
            derived.getOrCreateUserDataContainer()->addUserObject(new TemplateRef(tmpl));
    }
}

#endif
