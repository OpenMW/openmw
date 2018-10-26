#include "statesetupdater.hpp"

#include <osg/Node>
#include <osg/NodeVisitor>

namespace SceneUtil
{

    void StateSetUpdater::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (!mStateSets[0])
        {
            // first time setup
            osg::StateSet* src = node->getOrCreateStateSet();
            for (int i=0; i<2; ++i) // Using SHALLOW_COPY for StateAttributes, if users want to modify it is their responsibility to set a non-shared one first
                                    // This can be done conveniently in user implementations of the setDefaults() method
            {
                mStateSets[i] = new osg::StateSet(*src, osg::CopyOp::SHALLOW_COPY);
                setDefaults(mStateSets[i]);
            }
        }

        osg::StateSet* stateset = mStateSets[nv->getTraversalNumber()%2];
        node->setStateSet(stateset);

        apply(stateset, nv);

        traverse(node, nv);
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
