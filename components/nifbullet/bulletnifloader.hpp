#ifndef OPENMW_COMPONENTS_NIFBULLET_BULLETNIFLOADER_HPP
#define OPENMW_COMPONENTS_NIFBULLET_BULLETNIFLOADER_HPP

#include <osg/ref_ptr>
#include <osg/Referenced>

#include <components/nif/niffile.hpp>
#include <components/resource/bulletshape.hpp>

namespace NifBullet
{

/**
*Load bulletShape from NIF files.
*/
class BulletNifLoader
{
public:
    static osg::ref_ptr<Resource::BulletShape> load(const Nif::File& file);
};

}

#endif
