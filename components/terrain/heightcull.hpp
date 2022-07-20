#ifndef COMPONENTS_TERRAIN_HEIGHTCULL_H
#define COMPONENTS_TERRAIN_HEIGHTCULL_H

#include <osg/ref_ptr>
#include <osg/Referenced>

#include <limits>

#include <components/sceneutil/nodecallback.hpp>

namespace osg
{
    class Node;
    class NodeVisitor;
}

namespace Terrain
{
    class HeightCullCallback : public SceneUtil::NodeCallback<HeightCullCallback>
    {
    public:
        void setLowZ(float z)
        {
            mLowZ = z;
        }
        float getLowZ() const
        {
            return mLowZ;
        }

        void setHighZ(float highZ)
        {
            mHighZ = highZ;
        }
        float getHighZ() const
        {
            return mHighZ;
        }

        void setCullMask(unsigned int mask)
        {
            mMask = mask;
        }
        unsigned int getCullMask() const
        {
            return mMask;
        }

        void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            if (mLowZ <= mHighZ)
                traverse(node, nv);
        }
    private:
        float mLowZ{ -std::numeric_limits<float>::max() };
        float mHighZ{ std::numeric_limits<float>::max() };
        unsigned int mMask{ ~0u };
    };

}

#endif
