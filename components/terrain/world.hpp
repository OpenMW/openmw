/*
 * Copyright (c) 2015 scrawl <scrawl@baseoftrash.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef COMPONENTS_TERRAIN_WORLD_H
#define COMPONENTS_TERRAIN_WORLD_H

#include <OgreVector3.h>

#include "defs.hpp"
#include "buffercache.hpp"

namespace Ogre
{
    class SceneManager;
}

namespace Terrain
{
    class Storage;

    /**
     * @brief The basic interface for a terrain world. How the terrain chunks are paged and displayed
     *  is up to the implementation.
     */
    class World
    {
    public:
        /// @note takes ownership of \a storage
        /// @param sceneMgr scene manager to use
        /// @param storage Storage instance to get terrain data from (heights, normals, colors, textures..)
        /// @param visbilityFlags visibility flags for the created meshes
        /// @param shaders Whether to use splatting shader, or multi-pass fixed function splatting. Shader is usually
        ///         faster so this is just here for compatibility.
        /// @param align The align of the terrain, see Alignment enum
        World(Ogre::SceneManager* sceneMgr,
                Storage* storage, int visiblityFlags, bool shaders, Alignment align);
        virtual ~World();

        bool getShadersEnabled() { return mShaders; }
        bool getShadowsEnabled() { return mShadows; }
        bool getSplitShadowsEnabled() { return mSplitShadows; }

        float getHeightAt (const Ogre::Vector3& worldPos);

        /// Update chunk LODs according to this camera position
        /// @note Calling this method might lead to composite textures being rendered, so it is best
        /// not to call it when render commands are still queued, since that would cause a flush.
        virtual void update (const Ogre::Vector3& cameraPos) = 0;

        // This is only a hint and may be ignored by the implementation.
        virtual void loadCell(int x, int y) {}
        virtual void unloadCell(int x, int y) {}

        /// Get the world bounding box of a chunk of terrain centered at \a center
        virtual Ogre::AxisAlignedBox getWorldBoundingBox (const Ogre::Vector2& center) = 0;

        Ogre::SceneManager* getSceneManager() { return mSceneMgr; }

        Storage* getStorage() { return mStorage; }

        /// Show or hide the whole terrain
        /// @note this setting may be invalidated once you call Terrain::update, so do not call it while the terrain should be hidden
        virtual void setVisible(bool visible) = 0;
        virtual bool getVisible() = 0;

        /// Recreate materials used by terrain chunks. This should be called whenever settings of
        /// the material factory are changed. (Relying on the factory to update those materials is not
        /// enough, since turning a feature on/off can change the number of texture units available for layer/blend
        /// textures, and to properly respond to this we may need to change the structure of the material, such as
        /// adding or removing passes. This can only be achieved by a full rebuild.)
        virtual void applyMaterials(bool shadows, bool splitShadows) = 0;

        int getVisibilityFlags() { return mVisibilityFlags; }

        Alignment getAlign() { return mAlign; }

        /// Wait until all background loading is complete.
        virtual void syncLoad() {}

    protected:
        bool mShaders;
        bool mShadows;
        bool mSplitShadows;
        Alignment mAlign;

        Storage* mStorage;

        int mVisibilityFlags;

        Ogre::SceneManager* mSceneMgr;

        BufferCache mCache;

    public:
        // ----INTERNAL----
        BufferCache& getBufferCache() { return mCache; }

        // Convert the given position from Z-up align, i.e. Align_XY to the wanted align set in mAlign
        void convertPosition (float& x, float& y, float& z);
        void convertPosition (Ogre::Vector3& pos);
        void convertBounds (Ogre::AxisAlignedBox& bounds);
    };

}

#endif
