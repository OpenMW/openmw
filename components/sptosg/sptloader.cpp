#include "sptloader.hpp"

#include <stdexcept>

namespace SptOsg
{
    osg::ref_ptr<osg::Node> Loader::load(const Spt::SPTFile& file)
    {
        throw std::runtime_error("SPT model construction is not yet implemented");
    }
}