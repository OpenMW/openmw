#include "statesetupdater.hpp"

#include <osg/Node>
#include <osg/NodeVisitor>
#include <osgUtil/CullVisitor>

namespace SceneUtil
{

    void StateSetUpdater::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        bool isCullVisitor = nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR;
        if (!mStateSets[0])
        {
            for (int i=0; i<2; ++i)
            {
                if (!isCullVisitor)
                    mStateSets[i] = new osg::StateSet(*node->getOrCreateStateSet(), osg::CopyOp::SHALLOW_COPY); // Using SHALLOW_COPY for StateAttributes, if users want to modify it is their responsibility to set a non-shared one first in setDefaults
                else
                    mStateSets[i] = new osg::StateSet;
                setDefaults(mStateSets[i]);
            }
        }

        osg::ref_ptr<osg::StateSet> stateset = mStateSets[nv->getTraversalNumber()%2];
        apply(stateset, nv);

        if (!isCullVisitor)
            node->setStateSet(stateset);
        else
            static_cast<osgUtil::CullVisitor*>(nv)->pushStateSet(stateset);

        traverse(node, nv);

        if (isCullVisitor)
            static_cast<osgUtil::CullVisitor*>(nv)->popStateSet();
    }

    void StateSetUpdater::reset()
    {
        mStateSets[0] = nullptr;
        mStateSets[1] = nullptr;
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
            mCtrls.push_back(osg::clone(copy.mCtrls[i].get(), copyop));
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
        mCtrls.push_back(ctrl);
    }

}
