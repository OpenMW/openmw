#ifndef COMPONENTS_TERRAIN_WORLD_H
#define COMPONENTS_TERRAIN_WORLD_H

#include <osg/ref_ptr>
#include <osg/Referenced>
#include <osg/Vec3f>

#include <memory>

#include "defs.hpp"

namespace osg
{
    class Group;
    class Stats;
    class Node;
    class Object;
}

namespace Resource
{
    class ResourceSystem;
}

namespace Terrain
{
    class Storage;

    class TextureManager;
    class ChunkManager;
    class CompositeMapRenderer;

    /**
     * @brief A View is a collection of rendering objects that are visible from a given camera/intersection.
     * The base View class is part of the interface for usage in conjunction with preload feature.
     */
    class View : public osg::Referenced
    {
    public:
        virtual ~View() {}

        /// Reset internal structure so that the next addition to the view will override the previous frame's contents.
        virtual void reset(unsigned int frame) = 0;
    };

    /**
     * @brief The basic interface for a terrain world. How the terrain chunks are paged and displayed
     *  is up to the implementation.
     */
    class World
    {
    public:
        /// @note takes ownership of \a storage
        /// @param storage Storage instance to get terrain data from (heights, normals, colors, textures..)
        /// @param nodeMask mask for the terrain root
        /// @param preCompileMask mask for pre compiling textures
        World(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, Storage* storage, int nodeMask, int preCompileMask);
        virtual ~World();

        /// Apply the scene manager's texture filtering settings to all cached textures.
        /// @note Thread safe.
        void updateTextureFiltering();

        float getHeightAt (const osg::Vec3f& worldPos);

        /// Clears the cached land and landtexture data.
        /// @note Thread safe.
        virtual void clearAssociatedCaches();

        /// Load a terrain cell at maximum LOD and store it in the View for later use.
        /// @note Thread safe.
        virtual void cacheCell(View* view, int x, int y) {}

        /// Load the cell into the scene graph.
        /// @note Not thread safe.
        /// @note May be ignored by derived implementations that don't organize the terrain into cells.
        virtual void loadCell(int x, int y) {}

        /// Remove the cell from the scene graph.
        /// @note Not thread safe.
        /// @note May be ignored by derived implementations that don't organize the terrain into cells.
        virtual void unloadCell(int x, int y) {}

        virtual void enable(bool enabled) {}

        /// Create a View to use with preload feature. The caller is responsible for deleting the view.
        /// @note Thread safe.
        virtual View* createView() { return NULL; }

        /// @note Thread safe, as long as you do not attempt to load into the same view from multiple threads.
        virtual void preload(View* view, const osg::Vec3f& eyePoint) {}

        virtual void reportStats(unsigned int frameNumber, osg::Stats* stats) {}

        /// Set the default viewer (usually a Camera), used as viewpoint for any viewers that don't use their own viewpoint.
        virtual void setDefaultViewer(osg::Object* obj) {}

        Storage* getStorage() { return mStorage; }

    protected:
        Storage* mStorage;

        osg::ref_ptr<osg::Group> mParent;
        osg::ref_ptr<osg::Group> mTerrainRoot;

        osg::ref_ptr<osg::Group> mCompositeMapCamera;
        osg::ref_ptr<CompositeMapRenderer> mCompositeMapRenderer;

        Resource::ResourceSystem* mResourceSystem;

        std::unique_ptr<TextureManager> mTextureManager;
        std::unique_ptr<ChunkManager> mChunkManager;
    };

}

#endif
