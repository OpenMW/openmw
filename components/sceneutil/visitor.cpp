#include "visitor.hpp"

#include <osg/Drawable>
#include <osg/MatrixTransform>

#include <osgParticle/ParticleSystem>

#include <osgAnimation/Bone>

#include <components/debug/debuglog.hpp>
#include <components/misc/strings/algorithm.hpp>

#include <cstring>
#include <string_view>

namespace SceneUtil
{
    bool FindByNameVisitor::checkGroup(osg::Group& group)
    {
        if (Misc::StringUtils::ciEqual(group.getName(), mNameToFind))
        {
            mFoundNode = &group;
            return true;
        }

        return false;
    }

    void FindByClassVisitor::apply(osg::Node& node)
    {
        if (Misc::StringUtils::ciEqual(node.className(), mNameToFind))
            mFoundNodes.push_back(&node);

        traverse(node);
    }

    void FindByNameVisitor::apply(osg::Group& group)
    {
        if (!mFoundNode && !checkGroup(group))
            traverse(group);
    }

    void FindByNameVisitor::apply(osg::MatrixTransform& node)
    {
        if (!mFoundNode && !checkGroup(node))
            traverse(node);
    }

    void FindByNameVisitor::apply(osg::Geometry&) {}

    void NodeMapVisitorBoneOnly::apply(osg::MatrixTransform& trans)
    {
        // Choose first found bone in file
        if (dynamic_cast<osgAnimation::Bone*>(&trans) != nullptr)
            mMap.emplace(trans.getName(), &trans);

        traverse(trans);
    }

    void NodeMapVisitor::apply(osg::MatrixTransform& trans)
    {
        // Choose first found node in file
        mMap.emplace(trans.getName(), &trans);
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
            mToRemove.emplace_back(&node, node.getParent(0));
        else
            traverse(node);
    }

    void CleanObjectRootVisitor::applyDrawable(osg::Node& node)
    {
        osg::NodePath::iterator parent = getNodePath().end() - 2;
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
                mToRemove.emplace_back(parentGroup, parentParent);
                return;
            }
        }

        mToRemove.emplace_back(&node, parentGroup);
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
        if (Misc::StringUtils::ciStartsWith(node.getName(), "tri bip"))
        {
            osg::Group* parent = static_cast<osg::Group*>(*(getNodePath().end() - 2));
            // Not safe to remove in apply(), since the visitor is still iterating the child list
            mToRemove.emplace_back(&node, parent);
        }
    }
}
