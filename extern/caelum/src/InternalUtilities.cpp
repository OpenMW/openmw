/*
This file is part of Caelum.
See http://www.ogre3d.org/wiki/index.php/Caelum 

Copyright (c) 2006-2007 Caelum team. See Contributors.txt for details.

Caelum is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Caelum is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Caelum. If not, see <http://www.gnu.org/licenses/>.
*/

#include "CaelumPrecompiled.h"
#include "CaelumExceptions.h"
#include "InternalUtilities.h"
#include "PrivatePtr.h"

namespace Caelum
{
    Ogre::ColourValue InternalUtilities::getInterpolatedColour (
            float fx, float fy, Ogre::Image *img, bool wrapX)
    {
	    // Don't -> all the time, and avoid unsigned warnings
        int imgWidth = static_cast<int>(img->getWidth ());
        int imgHeight = static_cast<int>(img->getHeight ());

	    // Calculate pixel y coord.
        int py = Ogre::Math::IFloor(Ogre::Math::Abs (fy) * (imgHeight - 1));
        // Snap to py image bounds.
        py = std::max(0, std::min(py, imgHeight - 1));

	    // Get the two closest pixels on x.
        // px1 and px2 are the closest integer pixels to px.
        float px = fx * (img->getWidth () - 1);
	    int px1, px2;
        px1 = Ogre::Math::IFloor(px);
        px2 = Ogre::Math::ICeil(px);

        if (wrapX) {
            // Wrap x coords. The funny addition ensures that it does
            // "the right thing" for negative values.
            px1 = (px1 % imgWidth + imgWidth) % imgWidth;
            px2 = (px2 % imgWidth + imgWidth) % imgWidth;
        } else {
            px1 = std::max(0, std::min(px1, imgWidth - 1));
            px2 = std::max(0, std::min(px2, imgWidth - 1));
        }

	    // Calculate the interpolated pixel
	    Ogre::ColourValue c1, c2, cf;
        c1 = img->getColourAt (px1, py, 0);
        c2 = img->getColourAt (px2, py, 0);

        // Blend the two pixels together.
        // diff is the weight between pixel 1 and pixel 2.
        float diff = px - px1;
	    cf = c1 * (1 - diff) + c2 * diff;

	    return cf;
    }

    const Ogre::String InternalUtilities::pointerToString (void* pointer)
    {
        std::stringstream stream;
		stream.width(2 * sizeof(void *));
        stream.fill('0');
        stream.unsetf(std::ios::dec);
        stream.setf(std::ios::hex);
        stream.setf(std::ios::uppercase);
        stream << reinterpret_cast<ptrdiff_t>(pointer);
        return stream.str();
    }

    Ogre::MaterialPtr InternalUtilities::checkLoadMaterialClone (
            const Ogre::String& originalName,
            const Ogre::String& cloneName)
    {
        Ogre::MaterialPtr scriptMaterial = Ogre::MaterialManager::getSingletonPtr()->getByName(originalName);
        if (scriptMaterial.isNull()) {
            CAELUM_THROW_UNSUPPORTED_EXCEPTION (
                    "Can't find material \"" + originalName + "\"",
                    "Caelum");
        }

        // Create clone
        Caelum::PrivateMaterialPtr clonedMaterial (scriptMaterial->clone (cloneName));

        // Test clone loads and there is at least on supported technique
        clonedMaterial->load ();
        if (clonedMaterial->getBestTechnique () == 0) {
            CAELUM_THROW_UNSUPPORTED_EXCEPTION (
                    "Can't load material \"" + originalName + "\": " + clonedMaterial->getUnsupportedTechniquesExplanation(), 
                    "Caelum");
        }

        return clonedMaterial.release();
    }

    Ogre::CompositorPtr InternalUtilities::checkCompositorSupported (const Ogre::String& name)
    {
        Ogre::CompositorPtr comp = Ogre::CompositorManager::getSingletonPtr()->getByName(name);
        if (comp.isNull()) {
            CAELUM_THROW_UNSUPPORTED_EXCEPTION (
                    "Can't find compositor \"" + name + "\"",
                    "Caelum");
        }

        // Check the compositor is supported after loading.
        comp->load ();
        if (comp->getNumSupportedTechniques () == 0) {
            CAELUM_THROW_UNSUPPORTED_EXCEPTION (
                    "Can't load compositor \"" + name + "\"", 
                    "Caelum");
        }

        return comp;
    }

    void InternalUtilities::generateSphericDome (const Ogre::String &name, int segments, DomeType type)
    {
        // Return now if already exists
        if (Ogre::MeshManager::getSingleton ().resourceExists (name)) {
            return;
        }

        Ogre::LogManager::getSingleton ().logMessage (
                "Caelum: Creating " + name + " sphere mesh resource...");

        // Use the mesh manager to create the mesh
        Ogre::MeshPtr msh = Ogre::MeshManager::getSingleton ().createManual (name, RESOURCE_GROUP_NAME);
        // Create a submesh
        Ogre::SubMesh *sub = msh->createSubMesh ();

        // Create the shared vertex data
        Ogre::VertexData *vertexData = new Ogre::VertexData ();
        msh->sharedVertexData = vertexData;

        // Define the vertices' format
        Ogre::VertexDeclaration *vertexDecl = vertexData->vertexDeclaration;
        size_t currOffset = 0;
        // Position
        vertexDecl->addElement (0, currOffset, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
        currOffset += Ogre::VertexElement::getTypeSize (Ogre::VET_FLOAT3);
        // Normal
        vertexDecl->addElement (0, currOffset, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
        currOffset += Ogre::VertexElement::getTypeSize (Ogre::VET_FLOAT3);
        // Texture coordinates
        vertexDecl->addElement (0, currOffset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, 0);
        currOffset += Ogre::VertexElement::getTypeSize (Ogre::VET_FLOAT2);

        // Allocate the vertex buffer
        switch (type) {
            case DT_SKY_DOME:
                vertexData->vertexCount = segments * (segments - 1) + 2;
                break;
            case DT_IMAGE_STARFIELD:
                vertexData->vertexCount = (segments + 1) * (segments + 1);
                break;
        };
        Ogre::HardwareVertexBufferSharedPtr vBuf = Ogre::HardwareBufferManager::getSingleton ().createVertexBuffer (vertexDecl->getVertexSize (0), vertexData->vertexCount, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY, false);
        Ogre::VertexBufferBinding *binding = vertexData->vertexBufferBinding;
        binding->setBinding (0, vBuf);

        float *pVertex = static_cast<float *>(vBuf->lock (Ogre::HardwareBuffer::HBL_DISCARD));

        // Allocate the index buffer
        switch (type) {
            case DT_SKY_DOME:
                sub->indexData->indexCount = 2 * segments * (segments - 1) * 3;
                break;
            case DT_IMAGE_STARFIELD:
                sub->indexData->indexCount = 2 * (segments - 1) * segments * 3;
                break;
        };
        sub->indexData->indexBuffer = Ogre::HardwareBufferManager::getSingleton ().createIndexBuffer (Ogre::HardwareIndexBuffer::IT_16BIT, sub->indexData->indexCount, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY, false);
        Ogre::HardwareIndexBufferSharedPtr iBuf = sub->indexData->indexBuffer;
        unsigned short *pIndices = static_cast<unsigned short *>(iBuf->lock (Ogre::HardwareBuffer::HBL_DISCARD));

        // Fill the buffers
        switch (type) {
            case DT_SKY_DOME:
                fillGradientsDomeBuffers (pVertex, pIndices, segments);
                break;
            case DT_IMAGE_STARFIELD:
                fillStarfieldDomeBuffers (pVertex, pIndices, segments);
                break;
        };

        // Close the vertex buffer
        vBuf->unlock ();

        // Close the index buffer
        iBuf->unlock ();

        // Finishing it...
        sub->useSharedVertices = true;
        msh->_setBounds (Ogre::AxisAlignedBox (-1, -1, -1, 1, 1, 1), false);
        msh->_setBoundingSphereRadius (1);
        msh->load ();

        Ogre::LogManager::getSingleton ().logMessage (
                "Caelum: generateSphericDome DONE");
    }

    void InternalUtilities::fillGradientsDomeBuffers (float *pVertex, unsigned short *pIndices, int segments)
    {
        const float deltaLatitude = Ogre::Math::PI / (float )segments;
        const float deltaLongitude = Ogre::Math::PI * 2.0 / (float )segments;

        // Generate the rings
        for (int i = 1; i < segments; i++) {
            float r0 = Ogre::Math::Sin (Ogre::Radian (i * deltaLatitude));
            float y0 = Ogre::Math::Cos (Ogre::Radian (i * deltaLatitude));

            for (int j = 0; j < segments; j++) {
                float x0 = r0 * Ogre::Math::Sin (Ogre::Radian (j * deltaLongitude));
                float z0 = r0 * Ogre::Math::Cos (Ogre::Radian (j * deltaLongitude));

                *pVertex++ = x0;
                *pVertex++ = y0;
                *pVertex++ = z0;

                *pVertex++ = -x0;
                *pVertex++ = -y0;
                *pVertex++ = -z0;

                *pVertex++ = 0;
                *pVertex++ = 1 - y0;
            }
        }

        // Generate the "north pole"
        *pVertex++ = 0;	// Position
        *pVertex++ = 1;
        *pVertex++ = 0;
        *pVertex++ = 0;	// Normal
        *pVertex++ = -1;
        *pVertex++ = 0;
        *pVertex++ = 0;	// UV
        *pVertex++ = 0;

        // Generate the "south pole"
        *pVertex++ = 0;	// Position
        *pVertex++ = -1;
        *pVertex++ = 0;
        *pVertex++ = 0;	// Normal
        *pVertex++ = 1;
        *pVertex++ = 0;
        *pVertex++ = 0;	// UV
        *pVertex++ = 2;

        // Generate the mid segments
        for (int i = 0; i < segments - 2; i++) {
            for (int j = 0; j < segments; j++) {
                *pIndices++ = segments * i + j;
                *pIndices++ = segments * i + (j + 1) % segments;
                *pIndices++ = segments * (i + 1) + (j + 1) % segments;
                *pIndices++ = segments * i + j;
                *pIndices++ = segments * (i + 1) + (j + 1) % segments;
                *pIndices++ = segments * (i + 1) + j;
            }
        }

        // Generate the upper cap
        for (int i = 0; i < segments; i++) {
            *pIndices++ = segments * (segments - 1);
            *pIndices++ = (i + 1) % segments;
            *pIndices++ = i;
        }

        // Generate the lower cap
        for (int i = 0; i < segments; i++) {
            *pIndices++ = segments * (segments - 1) + 1;
            *pIndices++ = segments * (segments - 2) + i;
            *pIndices++ = segments * (segments - 2) + (i + 1) % segments;
        }
    }

    void InternalUtilities::fillStarfieldDomeBuffers (float *pVertex, unsigned short *pIndices, int segments)
    {
        const float deltaLatitude = Ogre::Math::PI / (float )segments;
        const float deltaLongitude = Ogre::Math::PI * 2.0 / (float )segments;

        // Generate the rings
        for (int i = 0; i <= segments; i++) {
            float r0 = Ogre::Math::Sin (Ogre::Radian (i * deltaLatitude));
            float y0 = Ogre::Math::Cos (Ogre::Radian (i * deltaLatitude));

            for (int j = 0; j <= segments; j++) {
                float x0 = r0 * Ogre::Math::Sin (Ogre::Radian (j * deltaLongitude));
                float z0 = r0 * Ogre::Math::Cos (Ogre::Radian (j * deltaLongitude));

                *pVertex++ = x0;
                *pVertex++ = y0;
                *pVertex++ = z0;

                *pVertex++ = -x0;
                *pVertex++ = -y0;
                *pVertex++ = -z0;

                *pVertex++ = (float )j / (float )segments;
                *pVertex++ = 1 - (y0 * 0.5 + 0.5);
            }
        }

        // Generate the mid segments
        int vRowSize = segments + 1;
        for (int i = 1; i < segments; i++) {
            for (int j = 0; j < segments; j++) {
                int baseIdx = vRowSize * i + j;
                *pIndices++ = baseIdx;
                *pIndices++ = baseIdx + 1;
                *pIndices++ = baseIdx + vRowSize + 1;
                *pIndices++ = baseIdx + 1;
                *pIndices++ = baseIdx;
                *pIndices++ = baseIdx - vRowSize;
            }
        }
    }
}
