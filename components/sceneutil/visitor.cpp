#include "visitor.hpp"

#include <osg/Version>
#include <osg/Drawable>
#include <osg/MatrixTransform>

#include <osgParticle/ParticleSystem>

#include <osgUtil/CullVisitor>

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

    class PreciseLeafDepthCullCallback : public osg::Drawable::CullCallback
    {

            inline osgUtil::CullVisitor::value_type distance(const osg::Vec3& coord,const osg::Matrix& matrix) const
            {
                return -((osgUtil::CullVisitor::value_type)coord[0]*(osgUtil::CullVisitor::value_type)matrix(0,2)+(osgUtil::CullVisitor::value_type)coord[1]*(osgUtil::CullVisitor::value_type)matrix(1,2)+(osgUtil::CullVisitor::value_type)coord[2]*(osgUtil::CullVisitor::value_type)matrix(2,2)+matrix(3,2));
            }

            virtual bool cull(osg::NodeVisitor* nv, osg::Drawable* drawable, osg::RenderInfo* renderInfo) const
            {
                osgUtil::CullVisitor * cv = static_cast<osgUtil::CullVisitor *>(nv);
                osg::RefMatrix& matrix = *cv->getModelViewMatrix();
                const osg::BoundingBox& bb = drawable->getBoundingBox();
                const osg::Drawable::CullCallback* nestedcb = dynamic_cast<const osg::Drawable::CullCallback*>(getNestedCallback());
                if(nestedcb && nestedcb->cull(nv, drawable, renderInfo)) return true;
                if (drawable->isCullingActive() && cv->isCulled(bb)) return true;


                if (cv->getComputeNearFarMode() && bb.valid())
                {
                    if (!cv->updateCalculatedNearFar(matrix,*drawable,false)) return true;
                }

                // need to track how push/pops there are, so we can unravel the stack correctly.
                unsigned int numPopStateSetRequired = 0;

                // push the geoset's state on the geostate stack.
                osg::StateSet* stateset = drawable->getStateSet();
                if (stateset)
                {
                    ++numPopStateSetRequired;
                    cv->pushStateSet(stateset);
                }

                osg::CullingSet& cs = cv->getCurrentCullingSet();
                if (!cs.getStateFrustumList().empty())
                {
                    osg::CullingSet::StateFrustumList& sfl = cs.getStateFrustumList();
                    for(osg::CullingSet::StateFrustumList::iterator itr = sfl.begin();
                        itr != sfl.end();
                        ++itr)
                    {
                        if (itr->second.contains(bb))
                        {
                            ++numPopStateSetRequired;
                            cv->pushStateSet(itr->first.get());
                        }
                    }
                }
                osg::Vec3 lookVector(-matrix(0,2),-matrix(1,2),-matrix(2,2));

                unsigned int bbCornerNear = (lookVector.x()>=0?0:1) +
                               (lookVector.y()>=0?0:2) +
                               (lookVector.z()>=0?0:4);

                float depth = FLT_MAX;
                if(bb.valid())
                {
                   depth = std::min((float)distance(bb.corner(bbCornerNear), matrix), depth);
                }
                else depth = 0.0f;

                if (osg::isNaN(depth))
                {
                    OSG_NOTICE<<"CullVisitor::apply(Geode&) detected NaN,"<<std::endl
                                            <<"    depth="<<depth<<", center=("<<bb.center()<<"),"<<std::endl
                                            <<"    matrix="<<matrix<<std::endl;
                    OSG_DEBUG << "    NodePath:" << std::endl;
                }
                else
                {
                    cv->addDrawableAndDepth(drawable,&matrix,depth);
                }

                for(unsigned int i=0;i< numPopStateSetRequired; ++i)
                {
                    cv->popStateSet();
                }
                return true;
            }
     };

    void AddRemoveTransparentCullCallback::apply(osg::Drawable &drw)
    {
#if OSG_MIN_VERSION_REQUIRED(3,6,0) //to precise
        if(_add) drw.addCullCallback(new PreciseLeafDepthCullCallback());
#else
        if(_add)
        {
            osg::Callback * cb = drw.getCullCallback(), *parentcb = 0;
            while(cb)
            {
                parentcb = cb; cb = cb->getNestedCallback();
            }
            if(parentcb)
                parentcb->setNestedCallback(new PreciseLeafDepthCullCallback());
            else drw.setCullCallback(new PreciseLeafDepthCullCallback());
        }
#endif
        else
        {
            osg::Callback * cb = drw.getCullCallback(), *parentcb = 0;
            while(cb)
            {
                if(dynamic_cast<PreciseLeafDepthCullCallback*>(cb)){
                    osg::ref_ptr<osg::Callback> nested_callback = cb->getNestedCallback();
                    if(parentcb)
                    {
                        parentcb->setNestedCallback(nested_callback);
                    }
                    else drw.setCullCallback(nested_callback);
                    return;
                }
                parentcb = cb;
                cb = cb->getNestedCallback();
            }
        }
    }
}
