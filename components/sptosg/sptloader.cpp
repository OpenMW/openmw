#include "sptloader.hpp"

#include <osg/Node>

namespace SptOsg
{
    osg::ref_ptr<osg::Node> Loader::load(const Spt::SPTFile& file)
    {
        // return empty node, not error
        return new osg::Node();
    }
}
