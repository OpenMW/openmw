#ifndef OPENMW_COMPONENTS_NIFOSG_NODEINDEXHOLDER_H
#define OPENMW_COMPONENTS_NIFOSG_NODEINDEXHOLDER_H

#include <osg/Object>

namespace NifOsg
{

    class NodeIndexHolder : public osg::Object
    {
    public:
        NodeIndexHolder() = default;
        NodeIndexHolder(int index)
            : mIndex(index)
        {
        }
        NodeIndexHolder(const NodeIndexHolder& copy, const osg::CopyOp& copyop)
            : Object(copy, copyop)
            , mIndex(copy.mIndex)
        {
        }

        META_Object(NifOsg, NodeIndexHolder)

        int getIndex() const { return mIndex; }

    private:

        // NIF record index
        int mIndex{0};
    };

}

#endif
