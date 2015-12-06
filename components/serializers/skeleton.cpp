//#define SERIALIZER_DEBUG

#ifdef SERIALIZER_DEBUG
#include <iostream>
#endif

#include <osgDB/ObjectWrapper>
#include <osgDB/InputStream>
#include <osgDB/OutputStream>
#include <osgDB/Serializer>

#include <components/sceneutil/skeleton.hpp>

#include "fixes.hpp"

#define MyClass SceneUtil::Skeleton
REGISTER_OBJECT_WRAPPER(SceneUtil_Skeleton_Serializer,
                        new SceneUtil::Skeleton,
                        "OpenMW::Skeleton",
                        "osg::Object osg::Group OpenMW::Skeleton")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up OpenMW::Skeleton serializer..." << std::endl;
#endif
    // INCOMPLETE
}
