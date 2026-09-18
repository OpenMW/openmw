#ifndef OPENMW_COMPONENTS_SCENEUTIL_TEMPLATEREF_H
#define OPENMW_COMPONENTS_SCENEUTIL_TEMPLATEREF_H

#include <osg/Object>
#include <osg/UserDataContainer>

namespace SceneUtil
{
    // Pin `tmpl`'s lifetime to `derived`'s by parking it in derived's user-data
    // container. Used to keep a cached template alive as long as any object
    // shallow-cloned from it survives. The const_cast is safe: we only need
    // the ref-count bump, never mutate through the stored pointer.
    inline void addTemplateRef(osg::Object& derived, const osg::Object* tmpl)
    {
        derived.getOrCreateUserDataContainer()->addUserObject(const_cast<osg::Object*>(tmpl));
    }
}

#endif
