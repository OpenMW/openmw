#ifndef SCENEUTIL_NODECALLBACK_H
#define SCENEUTIL_NODECALLBACK_H

#include <osg/Callback>

namespace osg
{
    class Node;
    class NodeVisitor;
}

namespace SceneUtil
{

template <class Derived, typename NodeType=osg::Node*, typename VisitorType=osg::NodeVisitor*>
class NodeCallback : public virtual osg::Callback
{
public:
    NodeCallback(){}
    NodeCallback(const NodeCallback& nc,const osg::CopyOp& copyop):
            osg::Callback(nc, copyop) {}
    META_Object(SceneUtil, NodeCallback)

    bool run(osg::Object* object, osg::Object* data) override
    {
        static_cast<Derived*>(this)->operator()((NodeType)object, (VisitorType)data->asNodeVisitor());
        return true;
    }

    template <typename VT>
    void traverse(NodeType object, VT data)
    {
        if (_nestedCallback.valid())
            _nestedCallback->run(object, data);
        else
            data->traverse(*object);
    }
};

}
#endif
