#include "texturereport.hpp"

#include <array>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <osg/Image>
#include <osg/Node>
#include <osg/NodeVisitor>
#include <osg/StateSet>
#include <osg/Texture>

#include <components/esm3/loadarmo.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/util.hpp>
#include <components/vfs/manager.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/ptr.hpp"

#include "npcanimation.hpp"

namespace MWRender
{
    namespace
    {
        // Matches ESM::PartReferenceType order
        constexpr std::array<std::string_view, ESM::PRT_Count> sPartNames = { "Head", "Hair", "Neck", "Cuirass",
            "Groin", "Skirt", "Right Hand", "Left Hand", "Right Wrist", "Left Wrist", "Shield", "Right Forearm",
            "Left Forearm", "Right Upperarm", "Left Upperarm", "Right Foot", "Left Foot", "Right Ankle", "Left Ankle",
            "Right Knee", "Left Knee", "Right Leg", "Left Leg", "Right Pauldron", "Left Pauldron", "Weapon", "Tail" };

        struct TextureFetchVisitor : osg::NodeVisitor
        {
            std::vector<std::pair<std::string, std::string>> mTextures;
            std::set<const osg::Node*> mSkippedNodes;

            TextureFetchVisitor(osg::NodeVisitor::TraversalMode mode = TRAVERSE_ALL_CHILDREN)
                : osg::NodeVisitor(mode)
            {
            }

            void apply(osg::Node& node) override
            {
                if (mSkippedNodes.contains(&node))
                    return;

                const osg::StateSet* stateset = node.getStateSet();
                if (stateset)
                {
                    const osg::StateSet::TextureAttributeList& texAttributes = stateset->getTextureAttributeList();
                    for (unsigned i = 0; i < static_cast<unsigned>(texAttributes.size()); i++)
                    {
                        const osg::StateAttribute* attr
                            = stateset->getTextureAttribute(i, osg::StateAttribute::TEXTURE);
                        if (!attr)
                            continue;
                        const osg::Texture* texture = attr->asTexture();
                        if (!texture)
                            continue;
                        const osg::Image* image = texture->getImage(0);
                        std::string fileName;
                        if (image)
                            fileName = image->getFileName();
                        mTextures.emplace_back(SceneUtil::getTextureType(*stateset, *texture, i), fileName);
                    }
                }

                traverse(node);
            }
        };

        void printTextures(std::ostream& msg, const TextureFetchVisitor& visitor, const VFS::Manager& vfs)
        {
            std::string lastTextureSrc;
            for (const auto& [textureName, fileName] : visitor.mTextures)
            {
                std::string textureSrc;
                if (!fileName.empty())
                    textureSrc = vfs.getArchive(fileName);

                if (lastTextureSrc.empty() || textureSrc != lastTextureSrc)
                {
                    lastTextureSrc = std::move(textureSrc);
                    if (lastTextureSrc.empty())
                        lastTextureSrc = "[No Source]";

                    msg << "  " << lastTextureSrc << std::endl;
                }
                msg << "    ";
                msg << (textureName.empty() ? "[Anonymous]: " : textureName) << ": ";
                msg << (fileName.empty() ? "[No File]" : fileName) << std::endl;
            }
        }
    }

    void reportTextures(std::ostream& msg, const MWWorld::Ptr& ptr, const VFS::Path::Normalized& model)
    {
        const VFS::Manager* vfs = MWBase::Environment::get().getResourceSystem()->getVFS();
        TextureFetchVisitor visitor;
        const auto* npcAnimation
            = dynamic_cast<const MWRender::NpcAnimation*>(MWBase::Environment::get().getWorld()->getAnimation(ptr));
        if (npcAnimation)
        {
            for (int type = 0; type < ESM::PRT_Count; ++type)
                if (const auto* part = npcAnimation->getPart(static_cast<ESM::PartReferenceType>(type)))
                    visitor.mSkippedNodes.insert(part->getNode().get());
        }
        SceneUtil::PositionAttitudeTransform* baseNode = ptr.getRefData().getBaseNode();
        if (baseNode)
            baseNode->accept(visitor);
        // The instance might not have a physical model due to paging or scripting.
        // If this is the case, fall back to the template
        if (visitor.mTextures.empty() && !npcAnimation)
        {
            Resource::SceneManager* sceneManager = MWBase::Environment::get().getResourceSystem()->getSceneManager();
            const_cast<osg::Node*>(sceneManager->getTemplate(model).get())->accept(visitor);
            msg << "Bound textures: [None]" << std::endl;
            if (!visitor.mTextures.empty())
                msg << "Model textures: ";
        }
        else
        {
            msg << "Bound textures: ";
        }
        if (!visitor.mTextures.empty())
        {
            msg << std::endl;
            printTextures(msg, visitor, *vfs);
        }
        else
        {
            msg << "[None]" << std::endl;
        }
        for (int type = 0; npcAnimation && type < ESM::PRT_Count; ++type)
        {
            const auto* part = npcAnimation->getPart(static_cast<ESM::PartReferenceType>(type));
            if (!part)
                continue;
            msg << sPartNames[type] << " part: " << part->getModel().value() << std::endl;
            const std::string partArchive = vfs->getArchive(part->getModel());
            if (!partArchive.empty())
                msg << "(" << partArchive << ")" << std::endl;
            TextureFetchVisitor partVisitor;
            part->getNode()->accept(partVisitor);
            printTextures(msg, partVisitor, *vfs);
        }
    }
}
