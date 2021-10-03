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
class NodeCallback : public osg::Callback
{
public:
    virtual bool run(osg::Object* object, osg::Object* data)
    {
        static_cast<Derived*>(this)->operator()(static_cast<NodeType>(object), static_cast<VisitorType>(data));
        return true;
    }

    void traverse(NodeType object, VisitorType data)
    {
        if (_nestedCallback.valid())
            _nestedCallback->run(object, data);
        else
            data->traverse(*object);
    }
};

}
#endif
