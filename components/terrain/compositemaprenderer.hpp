#ifndef OPENMW_COMPONENTS_TERRAIN_COMPOSITEMAPRENDERER_H
#define OPENMW_COMPONENTS_TERRAIN_COMPOSITEMAPRENDERER_H

#include <osg/Node>

#include <OpenThreads/Mutex>

#include <deque>

namespace Terrain
{

    /**
     * @brief The CompositeMapRenderer is responsible for updating composite map textures in a blocking or non-blocking way.
     */
    class CompositeMapRenderer : public osg::Node
    {
    public:
        CompositeMapRenderer();

        virtual void traverse(osg::NodeVisitor& nv);

        /// Set the maximum number of (non-immediate) composite maps to compile per frame
        void setNumCompilePerFrame(int num);

        /// Add a composite map to be rendered
        void addCompositeMap(osg::Node* node, bool immediate=false);

        /// Mark this composite map to be required for the current frame
        void setImmediate(osg::Node* node);

    private:
        unsigned int mNumCompilePerFrame;
        unsigned int mLastFrame;

        typedef std::set<osg::ref_ptr<osg::Node> > CompileSet;

        CompileSet mCompileSet;
        CompileSet mImmediateCompileSet;

        CompileSet mCompiled;

        OpenThreads::Mutex mMutex;
    };

}

#endif
