//#define SERIALIZER_DEBUG

#ifdef SERIALIZER_DEBUG
#include <iostream>
#endif

#include <osgDB/ObjectWrapper>
#include <osgDB/InputStream>
#include <osgDB/OutputStream>
#include <osgDB/Serializer>

#include <components/sceneutil/statesetupdater.hpp>

#include "fixes.hpp"

#define MyClass SceneUtil::StateSetUpdater
REGISTER_OBJECT_WRAPPER(NifOsg_StateSetUpdater_Serializer,
                        new SceneUtil::StateSetUpdater,
                        SceneUtil::StateSetUpdater,
                        "osg::Object osg::NodeCallback SceneUtil::StateSetUpdater")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up SceneUtil::StateSetUpdater serializer..." << std::endl;
#endif
    // No serialization for: osg::ref_ptr<osg::StateSet> mStateSets[2];  Transient?
}

#undef MyClass
#define MyClass SceneUtil::CompositeStateSetUpdater
REGISTER_OBJECT_WRAPPER(NifOsg_CompositeStateSetUpdater_Serializer,
                        new SceneUtil::CompositeStateSetUpdater,
                        SceneUtil::CompositeStateSetUpdater,
                        "osg::Object osg::NodeCallback SceneUtil::StateSetUpdater SceneUtil::CompositeStateSetUpdater")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up SceneUtil::CompositeStateSetUpdater serializer..." << std::endl;
#endif
    // No serialization for: std::vector<osg::ref_ptr<StateSetUpdater> > mCtrls;  Transient?
}
