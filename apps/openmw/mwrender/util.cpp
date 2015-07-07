#include "util.hpp"

#include <osg/Node>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/texturemanager.hpp>
#include <components/misc/resourcehelpers.hpp>

namespace MWRender
{

void overrideTexture(const std::string &texture, Resource::ResourceSystem *resourceSystem, osg::ref_ptr<osg::Node> node)
{
    if (texture.empty())
        return;
    std::string correctedTexture = Misc::ResourceHelpers::correctTexturePath(texture, resourceSystem->getVFS());
    // Not sure if wrap settings should be pulled from the overridden texture?
    osg::ref_ptr<osg::Texture2D> tex = resourceSystem->getTextureManager()->getTexture2D(correctedTexture, osg::Texture2D::CLAMP,
                                                                                          osg::Texture2D::CLAMP);
    osg::ref_ptr<osg::StateSet> stateset;
    if (node->getStateSet())
        stateset = static_cast<osg::StateSet*>(node->getStateSet()->clone(osg::CopyOp::SHALLOW_COPY));
    else
        stateset = new osg::StateSet;

    stateset->setTextureAttribute(0, tex, osg::StateAttribute::OVERRIDE);

    node->setStateSet(stateset);
}

}
