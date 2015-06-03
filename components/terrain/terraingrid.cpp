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

#include "terraingrid.hpp"

#include <memory>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/texturemanager.hpp>

#include <components/sceneutil/lightmanager.hpp>

#include <osg/PositionAttitudeTransform>
#include <osg/Geometry>
#include <osg/Geode>

#include <osgFX/Effect>

#include <osgUtil/IncrementalCompileOperation>

#include "material.hpp"
#include "storage.hpp"

namespace
{
    class StaticBoundingBoxCallback : public osg::Drawable::ComputeBoundingBoxCallback
    {
    public:
        StaticBoundingBoxCallback(const osg::BoundingBox& bounds)
            : mBoundingBox(bounds)
        {
        }

        virtual osg::BoundingBox computeBound(const osg::Drawable&) const
        {
            return mBoundingBox;
        }

    private:
        osg::BoundingBox mBoundingBox;
    };
}

namespace Terrain
{

TerrainGrid::TerrainGrid(osg::Group* parent, Resource::ResourceSystem* resourceSystem, osgUtil::IncrementalCompileOperation* ico,
                         Storage* storage, int nodeMask)
    : Terrain::World(parent, resourceSystem, ico, storage, nodeMask)
{
}

TerrainGrid::~TerrainGrid()
{
    while (!mGrid.empty())
    {
        unloadCell(mGrid.begin()->first.first, mGrid.begin()->first.second);
    }
}

class GridElement
{
public:
    osg::ref_ptr<osg::PositionAttitudeTransform> mNode;
};

void TerrainGrid::loadCell(int x, int y)
{
    if (mGrid.find(std::make_pair(x, y)) != mGrid.end())
        return; // already loaded

    osg::Vec2f center(x+0.5f, y+0.5f);
    float minH, maxH;
    if (!mStorage->getMinMaxHeights(1, center, minH, maxH))
        return; // no terrain defined

    std::auto_ptr<GridElement> element (new GridElement);

    osg::Vec2f worldCenter = center*mStorage->getCellWorldSize();
    element->mNode = new osg::PositionAttitudeTransform;
    element->mNode->setPosition(osg::Vec3f(worldCenter.x(), worldCenter.y(), 0.f));
    mTerrainRoot->addChild(element->mNode);

    osg::ref_ptr<osg::Vec3Array> positions (new osg::Vec3Array);
    osg::ref_ptr<osg::Vec3Array> normals (new osg::Vec3Array);
    osg::ref_ptr<osg::Vec4Array> colors (new osg::Vec4Array);

    osg::ref_ptr<osg::VertexBufferObject> vbo (new osg::VertexBufferObject);
    positions->setVertexBufferObject(vbo);
    normals->setVertexBufferObject(vbo);
    colors->setVertexBufferObject(vbo);

    mStorage->fillVertexBuffers(0, 1, center, positions, normals, colors);

    osg::ref_ptr<osg::Geometry> geometry (new osg::Geometry);
    geometry->setVertexArray(positions);
    geometry->setNormalArray(normals, osg::Array::BIND_PER_VERTEX);
    geometry->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
    geometry->setUseDisplayList(false);
    geometry->setUseVertexBufferObjects(true);

    geometry->addPrimitiveSet(mCache.getIndexBuffer(0));

    // we already know the bounding box, so no need to let OSG compute it.
    osg::Vec3f min(-0.5f*mStorage->getCellWorldSize(),
                   -0.5f*mStorage->getCellWorldSize(),
                   minH);
    osg::Vec3f max (0.5f*mStorage->getCellWorldSize(),
                       0.5f*mStorage->getCellWorldSize(),
                       maxH);
    osg::BoundingBox bounds(min, max);
    geometry->setComputeBoundingBoxCallback(new StaticBoundingBoxCallback(bounds));

    osg::ref_ptr<osg::Geode> geode (new osg::Geode);
    geode->addDrawable(geometry);

    std::vector<LayerInfo> layerList;
    std::vector<osg::ref_ptr<osg::Image> > blendmaps;
    mStorage->getBlendmaps(1.f, center, false, blendmaps, layerList);

    // For compiling textures, I don't think the osgFX::Effect does it correctly
    osg::ref_ptr<osg::Node> textureCompileDummy (new osg::Node);

    std::vector<osg::ref_ptr<osg::Texture2D> > layerTextures;
    for (std::vector<LayerInfo>::const_iterator it = layerList.begin(); it != layerList.end(); ++it)
    {
        layerTextures.push_back(mResourceSystem->getTextureManager()->getTexture2D(it->mDiffuseMap, osg::Texture::REPEAT, osg::Texture::REPEAT));
        textureCompileDummy->getOrCreateStateSet()->setTextureAttributeAndModes(0, layerTextures.back());
    }

    std::vector<osg::ref_ptr<osg::Texture2D> > blendmapTextures;
    for (std::vector<osg::ref_ptr<osg::Image> >::const_iterator it = blendmaps.begin(); it != blendmaps.end(); ++it)
    {
        osg::ref_ptr<osg::Texture2D> texture (new osg::Texture2D);
        texture->setImage(*it);
        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        texture->setResizeNonPowerOfTwoHint(false);
        blendmapTextures.push_back(texture);

        textureCompileDummy->getOrCreateStateSet()->setTextureAttributeAndModes(0, layerTextures.back());
    }

    for (unsigned int i=0; i<blendmapTextures.size()+1; ++i)
        geometry->setTexCoordArray(i, mCache.getUVBuffer());

    osg::ref_ptr<osgFX::Effect> effect (new Terrain::Effect(layerTextures, blendmapTextures));

    effect->addCullCallback(new SceneUtil::LightListCallback);

    effect->addChild(geode);
    element->mNode->addChild(effect);

    if (mIncrementalCompileOperation)
    {
        mIncrementalCompileOperation->add(geode);
        mIncrementalCompileOperation->add(textureCompileDummy);
    }

    mGrid[std::make_pair(x,y)] = element.release();
}

void TerrainGrid::unloadCell(int x, int y)
{
    Grid::iterator it = mGrid.find(std::make_pair(x,y));
    if (it == mGrid.end())
        return;

    GridElement* element = it->second;
    mTerrainRoot->removeChild(element->mNode);
    delete element;

    mGrid.erase(it);
}

}
