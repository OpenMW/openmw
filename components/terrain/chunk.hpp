#ifndef COMPONENTS_TERRAIN_TERRAINBATCH_H
#define COMPONENTS_TERRAIN_TERRAINBATCH_H

#include <OgreRenderable.h>
#include <OgreMovableObject.h>

namespace Terrain
{

    class QuadTreeNode;

    /**
     * @brief Renders a chunk of terrain, either using alpha splatting or a composite map.
     */
    class Chunk : public Ogre::Renderable, public Ogre::MovableObject
    {
    public:
        /// @param lodLevel LOD level for the vertex buffer.
        Chunk (QuadTreeNode* node, short lodLevel);
        virtual ~Chunk();

        void setMaterial (const Ogre::MaterialPtr& material);

        /// Set additional LOD applied on top of vertex LOD. \n
        /// This is achieved by changing the index buffer to omit vertices.
        void setAdditionalLod (size_t lod) { mAdditionalLod = lod; }
        size_t getAdditionalLod() { return mAdditionalLod; }

        void updateIndexBuffer();

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
        QuadTreeNode* mNode;
        Ogre::MaterialPtr mMaterial;

        size_t mVertexLod;
        size_t mAdditionalLod;

        Ogre::VertexData* mVertexData;
        Ogre::IndexData* mIndexData;
        Ogre::HardwareVertexBufferSharedPtr mVertexBuffer;
        Ogre::HardwareVertexBufferSharedPtr mNormalBuffer;
        Ogre::HardwareVertexBufferSharedPtr mColourBuffer;
        Ogre::HardwareIndexBufferSharedPtr mIndexBuffer;
    };

}

#endif
