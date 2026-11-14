#ifndef OPENMW_COMPONENTS_SPTOSG_LOADER
#define OPENMW_COMPONENTS_SPTOSG_LOADER

#include <components/spt/file.hpp>

#include <osg/ref_ptr>

namespace osg
{
    class Node;
}

namespace SptOsg
{
    class Loader
    {
    public:
        static osg::ref_ptr<osg::Node> load(const Spt::SPTFile& file);
    };
}

#endif
