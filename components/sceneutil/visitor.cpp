#include "visitor.hpp"

#include <osg/Drawable>
#include <osg/MatrixTransform>

#include <osgParticle/ParticleSystem>

#include <components/debug/debuglog.hpp>

#include <components/misc/stringops.hpp>

namespace SceneUtil
{

    bool FindByNameVisitor::checkGroup(osg::Group &group)
    {
        if (Misc::StringUtils::ciEqual(group.getName(), mNameToFind))
        {
            mFoundNode = &group;
            return true;
        }
        return false;
    }

    void FindByClassVisitor::apply(osg::Node &node)
    {
        if (Misc::StringUtils::ciEqual(node.className(), mNameToFind))
            mFoundNodes.push_back(&node);

        traverse(node);
    }

    void FindByNameVisitor::apply(osg::Group &group)
    {
        if (!checkGroup(group))
            traverse(group);
    }

    void FindByNameVisitor::apply(osg::MatrixTransform &node)
    {
        if (!checkGroup(node))
            traverse(node);
    }

    void FindByNameVisitor::apply(osg::Geometry&)
    {
    }

    void DisableFreezeOnCullVisitor::apply(osg::MatrixTransform &node)
    {
        traverse(node);
    }

    void DisableFreezeOnCullVisitor::apply(osg::Drawable& drw)
    {
        if (osgParticle::ParticleSystem* partsys = dynamic_cast<osgParticle::ParticleSystem*>(&drw))
            partsys->setFreezeOnCull(false);
    }

    void NodeMapVisitor::apply(osg::MatrixTransform& trans)
    {
        // Take transformation for first found node in file
        const std::string nodeName = Misc::StringUtils::lowerCase(trans.getName());
        mMap.emplace(nodeName, &trans);

        traverse(trans);
    }

    void RemoveVisitor::remove()
    {
        for (RemoveVec::iterator it = mToRemove.begin(); it != mToRemove.end(); ++it)
        {
            if (!it->second->removeChild(it->first))
                Log(Debug::Error) << "error removing " << it->first->getName();
        }
    }

    void CleanObjectRootVisitor::apply(osg::Drawable& drw)
    {
        applyDrawable(drw);
    }

    void CleanObjectRootVisitor::apply(osg::Group& node)
    {
        applyNode(node);
    }

    void CleanObjectRootVisitor::apply(osg::MatrixTransform& node)
    {
        applyNode(node);
    }

    void CleanObjectRootVisitor::apply(osg::Node& node)
    {
        applyNode(node);
    }

    void CleanObjectRootVisitor::applyNode(osg::Node& node)
    {
        if (node.getStateSet())
            node.setStateSet(nullptr);

        if (node.getNodeMask() == 0x1 && node.getNumParents() == 1)
            mToRemove.push_back(std::make_pair(&node, node.getParent(0)));
        else
            traverse(node);
    }

    void CleanObjectRootVisitor::applyDrawable(osg::Node& node)
    {
        osg::NodePath::iterator parent = getNodePath().end()-2;
        // We know that the parent is a Group because only Groups can have children.
        osg::Group* parentGroup = static_cast<osg::Group*>(*parent);

        // Try to prune nodes that would be empty after the removal
        if (parent != getNodePath().begin())
        {
            // This could be extended to remove the parent's parent, and so on if they are empty as well.
            // But for NIF files, there won't be a benefit since only TriShapes can be set to STATIC dataVariance.
            osg::Group* parentParent = static_cast<osg::Group*>(*(parent - 1));
            if (parentGroup->getNumChildren() == 1 && parentGroup->getDataVariance() == osg::Object::STATIC)
            {
                mToRemove.push_back(std::make_pair(parentGroup, parentParent));
                return;
            }
        }

        mToRemove.push_back(std::make_pair(&node, parentGroup));
    }

    void RemoveTriBipVisitor::apply(osg::Drawable& drw)
    {
        applyImpl(drw);
    }

    void RemoveTriBipVisitor::apply(osg::Group& node)
    {
        traverse(node);
    }

    void RemoveTriBipVisitor::apply(osg::MatrixTransform& node)
    {
        traverse(node);
    }

    void RemoveTriBipVisitor::applyImpl(osg::Node& node)
    {
        const std::string toFind = "tri bip";
        if (Misc::StringUtils::ciCompareLen(node.getName(), toFind, toFind.size()) == 0)
        {
            osg::Group* parent = static_cast<osg::Group*>(*(getNodePath().end()-2));
            // Not safe to remove in apply(), since the visitor is still iterating the child list
            mToRemove.push_back(std::make_pair(&node, parent));
        }
    }
}
