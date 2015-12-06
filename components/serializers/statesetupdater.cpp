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
                        "OpenMW::StateSetUpdater",
                        "osg::Object osg::NodeCallback OpenMW::StateSetUpdater")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up OpenMW::StateSetUpdater serializer..." << std::endl;
#endif
    // No serialization for: osg::ref_ptr<osg::StateSet> mStateSets[2];  Transient?
}

#undef MyClass
#define MyClass SceneUtil::CompositeStateSetUpdater
REGISTER_OBJECT_WRAPPER(NifOsg_CompositeStateSetUpdater_Serializer,
                        new SceneUtil::CompositeStateSetUpdater,
                        "OpenMW::CompositeStateSetUpdater",
                        "osg::Object osg::NodeCallback OpenMW::StateSetUpdater OpenMW::CompositeStateSetUpdater")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up OpenMW::CompositeStateSetUpdater serializer..." << std::endl;
#endif
    // No serialization for: std::vector<osg::ref_ptr<StateSetUpdater> > mCtrls;  Transient?
}
