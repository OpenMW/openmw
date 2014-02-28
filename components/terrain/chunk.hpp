#ifndef COMPONENTS_TERRAIN_TERRAINBATCH_H
#define COMPONENTS_TERRAIN_TERRAINBATCH_H

#include <OgreRenderable.h>
#include <OgreMovableObject.h>

namespace Terrain
{

    class BufferCache;
    struct LoadResponseData;

    /**
     * @brief Renders a chunk of terrain, either using alpha splatting or a composite map.
     */
    class Chunk : public Ogre::Renderable, public Ogre::MovableObject
    {
    public:
        Chunk (Ogre::HardwareVertexBufferSharedPtr uvBuffer, const Ogre::AxisAlignedBox& bounds, const LoadResponseData& data);

        virtual ~Chunk();

        /// @note Takes ownership of \a material
        void setMaterial (const Ogre::MaterialPtr& material);

        void setIndexBuffer(Ogre::HardwareIndexBufferSharedPtr buffer);

        // Inherited from MovableObject
        virtual const Ogre::String& getMovableType(void) const { static Ogre::String t = "MW_TERRAIN"; return t; }
        virtual const Ogre::AxisAlignedBox& getBoundingBox(void) const;
        virtual Ogre::Real getBoundingRadius(void) const;
        virtual void _updateRenderQueue(Ogre::RenderQueue* queue);
        virtual void visitRenderables(Renderable::Visitor* visitor,
            bool debugRenderables = false);

        // Inherited from Renderable
        virtual const Ogre::MaterialPtr& getMaterial(void) const;
        virtual void getRenderOperation(Ogre::RenderOperation& op);
        virtual void getWorldTransforms(Ogre::Matrix4* xform) const;
        virtual Ogre::Real getSquaredViewDepth(const Ogre::Camera* cam) const;
        virtual const Ogre::LightList& getLights(void) const;

    private:
        Ogre::AxisAlignedBox mBounds;
        Ogre::MaterialPtr mMaterial;

        Ogre::VertexData* mVertexData;
        Ogre::IndexData* mIndexData;
    };

}

#endif
