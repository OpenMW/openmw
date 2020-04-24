#include "util.hpp"

#include <osg/Node>
#include <osg/ValueObject>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/imagemanager.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/sceneutil/visitor.hpp>

namespace MWRender
{

class TextureOverrideVisitor : public osg::NodeVisitor
    {
    public:
        TextureOverrideVisitor(const std::string& texture, Resource::ResourceSystem* resourcesystem)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mTexture(texture)
            , mResourcesystem(resourcesystem)
        {
        }

        virtual void apply(osg::Node& node)
        {
            int index = 0;
            osg::ref_ptr<osg::Node> nodePtr(&node);
            if (node.getUserValue("overrideFx", index))
            {
                if (index == 1) 
                    overrideTexture(mTexture, mResourcesystem, nodePtr);
            }
            traverse(node);
        }
        std::string mTexture;
        Resource::ResourceSystem* mResourcesystem;
};

void overrideFirstRootTexture(const std::string &texture, Resource::ResourceSystem *resourceSystem, osg::ref_ptr<osg::Node> node)
{
    TextureOverrideVisitor overrideVisitor(texture, resourceSystem);
    node->accept(overrideVisitor);
}

void overrideTexture(const std::string &texture, Resource::ResourceSystem *resourceSystem, osg::ref_ptr<osg::Node> node)
{
    if (texture.empty())
        return;
    std::string correctedTexture = Misc::ResourceHelpers::correctTexturePath(texture, resourceSystem->getVFS());
    // Not sure if wrap settings should be pulled from the overridden texture?
    osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D(resourceSystem->getImageManager()->getImage(correctedTexture));
    tex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    tex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    tex->setName("diffuseMap");

    osg::ref_ptr<osg::StateSet> stateset;
    if (node->getStateSet())
        stateset = new osg::StateSet(*node->getStateSet(), osg::CopyOp::SHALLOW_COPY);
    else
        stateset = new osg::StateSet;

    stateset->setTextureAttribute(0, tex, osg::StateAttribute::OVERRIDE);

    node->setStateSet(stateset);
}

}
