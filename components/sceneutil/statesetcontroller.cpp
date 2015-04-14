#include "statesetcontroller.hpp"

#include <osg/Node>

namespace SceneUtil
{

    void StateSetController::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (!mStateSets[0])
        {
            // first time setup
            osg::StateSet* src = node->getOrCreateStateSet();
            for (int i=0; i<2; ++i) // Using SHALLOW_COPY for StateAttributes, if users want to modify it is their responsibility to set a non-shared one first
                                    // This can be done conveniently in user implementations of the setDefaults() method
            {
                mStateSets[i] = static_cast<osg::StateSet*>(osg::clone(src, osg::CopyOp::SHALLOW_COPY));
                setDefaults(mStateSets[i]);
            }
        }

        // Swap to make the StateSet in [0] writable, [1] is now the StateSet that was queued by the last frame
        std::swap(mStateSets[0], mStateSets[1]);
        node->setStateSet(mStateSets[0]);

        apply(mStateSets[0], nv);

        traverse(node, nv);
    }

    StateSetController::StateSetController()
    {
    }

    StateSetController::StateSetController(const StateSetController &copy, const osg::CopyOp &copyop)
        : osg::NodeCallback(copy, copyop)
    {
    }

    // ----------------------------------------------------------------------------------

    void CompositeStateSetController::apply(osg::StateSet *stateset, osg::NodeVisitor *nv)
    {
        for (unsigned int i=0; i<mCtrls.size(); ++i)
            mCtrls[i]->apply(stateset, nv);
    }

    void CompositeStateSetController::setDefaults(osg::StateSet *stateset)
    {
        for (unsigned int i=0; i<mCtrls.size(); ++i)
            mCtrls[i]->setDefaults(stateset);
    }

    CompositeStateSetController::CompositeStateSetController()
    {
    }

    CompositeStateSetController::CompositeStateSetController(const CompositeStateSetController &copy, const osg::CopyOp &copyop)
        : StateSetController(copy, copyop)
        , mCtrls(copy.mCtrls)
    {
    }

    unsigned int CompositeStateSetController::getNumControllers()
    {
        return mCtrls.size();
    }

    void CompositeStateSetController::addController(StateSetController *ctrl)
    {
        mCtrls.push_back(ctrl);
    }

}
