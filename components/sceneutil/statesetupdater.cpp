#include "statesetupdater.hpp"

#include <osg/Node>
#include <osg/NodeVisitor>
#include <osgUtil/CullVisitor>

namespace SceneUtil
{

    void StateSetUpdater::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        bool isCullVisitor = nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR;

        if (isCullVisitor)
            return applyCull(node, static_cast<osgUtil::CullVisitor*>(nv));
        else
            return applyUpdate(node, nv);
    }

    void StateSetUpdater::applyUpdate(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (!mStateSetsUpdate[0])
        {
            for (int i = 0; i < 2; ++i)
            {
                mStateSetsUpdate[i] = new osg::StateSet(*node->getOrCreateStateSet(), osg::CopyOp::SHALLOW_COPY); // Using SHALLOW_COPY for StateAttributes, if users want to modify it is their responsibility to set a non-shared one first in setDefaults
                setDefaults(mStateSetsUpdate[i]);
            }
        }

        osg::ref_ptr<osg::StateSet> stateset = mStateSetsUpdate[nv->getTraversalNumber() % 2];
        apply(stateset, nv);
        node->setStateSet(stateset);
        traverse(node, nv);
    }
    
    void StateSetUpdater::applyCull(osg::Node* node, osgUtil::CullVisitor* cv)
    {
        auto stateset = getCvDependentStateset(cv);
        apply(stateset, cv);
        cv->pushStateSet(stateset);
        traverse(node, cv);
        cv->popStateSet();
    }

    osg::StateSet* StateSetUpdater::getCvDependentStateset(osgUtil::CullVisitor* cv)
    {
        auto it = mStateSetsCull.find(cv);
        if (it == mStateSetsCull.end())
        {
            osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;
            mStateSetsCull.emplace(cv, stateset);
            setDefaults(stateset);
            return stateset;
        }
        return it->second;
    }

    void StateSetUpdater::reset()
    {
        mStateSetsUpdate[0] = nullptr;
        mStateSetsUpdate[1] = nullptr;
        mStateSetsCull.clear();
    }

    StateSetUpdater::StateSetUpdater()
    {
    }

    StateSetUpdater::StateSetUpdater(const StateSetUpdater &copy, const osg::CopyOp &copyop)
        : osg::NodeCallback(copy, copyop)
    {
    }

    // ----------------------------------------------------------------------------------

    void CompositeStateSetUpdater::apply(osg::StateSet *stateset, osg::NodeVisitor *nv)
    {
        for (unsigned int i=0; i<mCtrls.size(); ++i)
            mCtrls[i]->apply(stateset, nv);
    }

    void CompositeStateSetUpdater::setDefaults(osg::StateSet *stateset)
    {
        for (unsigned int i=0; i<mCtrls.size(); ++i)
            mCtrls[i]->setDefaults(stateset);
    }

    CompositeStateSetUpdater::CompositeStateSetUpdater()
    {
    }

    CompositeStateSetUpdater::CompositeStateSetUpdater(const CompositeStateSetUpdater &copy, const osg::CopyOp &copyop)
        : StateSetUpdater(copy, copyop)
    {
        for (unsigned int i=0; i<copy.mCtrls.size(); ++i)
            mCtrls.emplace_back(osg::clone(copy.mCtrls[i].get(), copyop));
    }

    unsigned int CompositeStateSetUpdater::getNumControllers()
    {
        return mCtrls.size();
    }

    StateSetUpdater* CompositeStateSetUpdater::getController(int i)
    {
        return mCtrls[i];
    }

    void CompositeStateSetUpdater::addController(StateSetUpdater *ctrl)
    {
        mCtrls.emplace_back(ctrl);
    }

}
