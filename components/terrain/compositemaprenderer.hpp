#ifndef OPENMW_COMPONENTS_TERRAIN_COMPOSITEMAPRENDERER_H
#define OPENMW_COMPONENTS_TERRAIN_COMPOSITEMAPRENDERER_H

#include <osg/Drawable>

#include <OpenThreads/Mutex>

#include <set>

namespace osg
{
    class FrameBufferObject;
    class RenderInfo;
    class Texture2D;
}

namespace SceneUtil
{
    class UnrefQueue;
    class WorkQueue;
}

namespace Terrain
{

    class CompositeMap : public osg::Referenced
    {
    public:
        CompositeMap();
        ~CompositeMap();
        std::vector<osg::ref_ptr<osg::Drawable> > mDrawables;
        osg::ref_ptr<osg::Texture2D> mTexture;
        unsigned int mCompiled;
    };

    /**
     * @brief The CompositeMapRenderer is responsible for updating composite map textures in a blocking or non-blocking way.
     */
    class CompositeMapRenderer : public osg::Drawable
    {
    public:
        CompositeMapRenderer();
        ~CompositeMapRenderer();

        virtual void drawImplementation(osg::RenderInfo& renderInfo) const;

        void compile(CompositeMap& compositeMap, osg::RenderInfo& renderInfo, double* timeLeft) const;

        /// Set a WorkQueue to delete compiled composite map layers in the background thread
        void setWorkQueue(SceneUtil::WorkQueue* workQueue);

        /// Set the available time in seconds for compiling (non-immediate) composite maps each frame
        void setMinimumTimeAvailableForCompile(double time);

        /// If current frame rate is higher than this, the extra time will be set aside to do more compiling
        void setTargetFrameRate(float framerate);

        /// Add a composite map to be rendered
        void addCompositeMap(CompositeMap* map, bool immediate=false);

        /// Mark this composite map to be required for the current frame
        void setImmediate(CompositeMap* map);

        unsigned int getCompileSetSize() const;

    private:
        float mTargetFrameRate;
        double mMinimumTimeAvailable;
        mutable osg::Timer mTimer;

        osg::ref_ptr<SceneUtil::UnrefQueue> mUnrefQueue;
        osg::ref_ptr<SceneUtil::WorkQueue> mWorkQueue;

        typedef std::set<osg::ref_ptr<CompositeMap> > CompileSet;

        mutable CompileSet mCompileSet;
        mutable CompileSet mImmediateCompileSet;

        mutable OpenThreads::Mutex mMutex;

        osg::ref_ptr<osg::FrameBufferObject> mFBO;
    };

}

#endif
