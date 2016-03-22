#include "terraingrid.hpp"

#include <memory>

#include <osg/Material>

#include <OpenThreads/ScopedLock>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/imagemanager.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/unrefqueue.hpp>

#include <components/esm/loadland.hpp>

#include <osg/Geometry>
#include <osg/KdTree>

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

TerrainGrid::TerrainGrid(osg::Group* parent, Resource::ResourceSystem* resourceSystem, osgUtil::IncrementalCompileOperation* ico, Storage* storage, int nodeMask, Shader::ShaderManager* shaderManager, SceneUtil::UnrefQueue* unrefQueue)
    : Terrain::World(parent, resourceSystem, ico, storage, nodeMask)
    , mNumSplits(4)
    , mCache((storage->getCellVertices()-1)/static_cast<float>(mNumSplits) + 1)
    , mUnrefQueue(unrefQueue)
    , mShaderManager(shaderManager)
{
    osg::ref_ptr<osg::Material> material (new osg::Material);
    material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
    mTerrainRoot->getOrCreateStateSet()->setAttributeAndModes(material, osg::StateAttribute::ON);
}

TerrainGrid::~TerrainGrid()
{
    while (!mGrid.empty())
    {
        unloadCell(mGrid.begin()->first.first, mGrid.begin()->first.second);
    }
}

osg::ref_ptr<osg::Node> TerrainGrid::cacheCell(int x, int y)
{
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mGridCacheMutex);
        Grid::iterator found = mGridCache.find(std::make_pair(x,y));
        if (found != mGridCache.end())
            return found->second;
    }
    osg::ref_ptr<osg::Node> node = buildTerrain(NULL, 1.f, osg::Vec2f(x+0.5, y+0.5));

    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mGridCacheMutex);
    mGridCache.insert(std::make_pair(std::make_pair(x,y), node));
    return node;
}

osg::ref_ptr<osg::Node> TerrainGrid::buildTerrain (osg::Group* parent, float chunkSize, const osg::Vec2f& chunkCenter)
{
    if (chunkSize * mNumSplits > 1.f)
    {
        // keep splitting
        osg::ref_ptr<osg::Group> group (new osg::Group);
        if (parent)
            parent->addChild(group);

        float newChunkSize = chunkSize/2.f;
        buildTerrain(group, newChunkSize, chunkCenter + osg::Vec2f(newChunkSize/2.f, newChunkSize/2.f));
        buildTerrain(group, newChunkSize, chunkCenter + osg::Vec2f(newChunkSize/2.f, -newChunkSize/2.f));
        buildTerrain(group, newChunkSize, chunkCenter + osg::Vec2f(-newChunkSize/2.f, newChunkSize/2.f));
        buildTerrain(group, newChunkSize, chunkCenter + osg::Vec2f(-newChunkSize/2.f, -newChunkSize/2.f));
        return group;
    }
    else
    {
        float minH, maxH;
        if (!mStorage->getMinMaxHeights(chunkSize, chunkCenter, minH, maxH))
            return NULL; // no terrain defined

        osg::Vec2f worldCenter = chunkCenter*mStorage->getCellWorldSize();
        osg::ref_ptr<SceneUtil::PositionAttitudeTransform> transform (new SceneUtil::PositionAttitudeTransform);
        transform->setPosition(osg::Vec3f(worldCenter.x(), worldCenter.y(), 0.f));

        if (parent)
            parent->addChild(transform);

        osg::ref_ptr<osg::Vec3Array> positions (new osg::Vec3Array);
        osg::ref_ptr<osg::Vec3Array> normals (new osg::Vec3Array);
        osg::ref_ptr<osg::Vec4Array> colors (new osg::Vec4Array);

        osg::ref_ptr<osg::VertexBufferObject> vbo (new osg::VertexBufferObject);
        positions->setVertexBufferObject(vbo);
        normals->setVertexBufferObject(vbo);
        colors->setVertexBufferObject(vbo);

        mStorage->fillVertexBuffers(0, chunkSize, chunkCenter, positions, normals, colors);

        osg::ref_ptr<osg::Geometry> geometry (new osg::Geometry);
        geometry->setVertexArray(positions);
        geometry->setNormalArray(normals, osg::Array::BIND_PER_VERTEX);
        geometry->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
        geometry->setUseDisplayList(false);
        geometry->setUseVertexBufferObjects(true);

        geometry->addPrimitiveSet(mCache.getIndexBuffer(0));

        // we already know the bounding box, so no need to let OSG compute it.
        osg::Vec3f min(-0.5f*mStorage->getCellWorldSize()*chunkSize,
                       -0.5f*mStorage->getCellWorldSize()*chunkSize,
                       minH);
        osg::Vec3f max (0.5f*mStorage->getCellWorldSize()*chunkSize,
                           0.5f*mStorage->getCellWorldSize()*chunkSize,
                           maxH);
        osg::BoundingBox bounds(min, max);
        geometry->setComputeBoundingBoxCallback(new StaticBoundingBoxCallback(bounds));

        std::vector<LayerInfo> layerList;
        std::vector<osg::ref_ptr<osg::Image> > blendmaps;
        mStorage->getBlendmaps(chunkSize, chunkCenter, false, blendmaps, layerList);

        // For compiling textures, I don't think the osgFX::Effect does it correctly
        osg::ref_ptr<osg::Node> textureCompileDummy (new osg::Node);
        unsigned int dummyTextureCounter = 0;

        bool useShaders = mResourceSystem->getSceneManager()->getForceShaders();
        if (!mResourceSystem->getSceneManager()->getClampLighting())
            useShaders = true; // always use shaders when lighting is unclamped, this is to avoid lighting seams between a terrain chunk with normal maps and one without normal maps
        std::vector<TextureLayer> layers;
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mTextureCacheMutex);
            for (std::vector<LayerInfo>::const_iterator it = layerList.begin(); it != layerList.end(); ++it)
            {
                TextureLayer textureLayer;
                textureLayer.mParallax = it->mParallax;
                textureLayer.mSpecular = it->mSpecular;
                osg::ref_ptr<osg::Texture2D> texture = mTextureCache[it->mDiffuseMap];
                if (!texture)
                {
                    texture = new osg::Texture2D(mResourceSystem->getImageManager()->getImage(it->mDiffuseMap));
                    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
                    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
                    mResourceSystem->getSceneManager()->applyFilterSettings(texture);
                    mTextureCache[it->mDiffuseMap] = texture;
                }
                textureLayer.mDiffuseMap = texture;
                textureCompileDummy->getOrCreateStateSet()->setTextureAttributeAndModes(dummyTextureCounter++, texture);

                if (!it->mNormalMap.empty())
                {
                    texture = mTextureCache[it->mNormalMap];
                    if (!texture)
                    {
                        texture = new osg::Texture2D(mResourceSystem->getImageManager()->getImage(it->mNormalMap));
                        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
                        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
                        mResourceSystem->getSceneManager()->applyFilterSettings(texture);
                        mTextureCache[it->mNormalMap] = texture;
                    }
                    textureCompileDummy->getOrCreateStateSet()->setTextureAttributeAndModes(dummyTextureCounter++, texture);
                    textureLayer.mNormalMap = texture;
                }

                if (it->requiresShaders())
                    useShaders = true;

                layers.push_back(textureLayer);
            }
        }

        std::vector<osg::ref_ptr<osg::Texture2D> > blendmapTextures;
        for (std::vector<osg::ref_ptr<osg::Image> >::const_iterator it = blendmaps.begin(); it != blendmaps.end(); ++it)
        {
            osg::ref_ptr<osg::Texture2D> texture (new osg::Texture2D);
            texture->setImage(*it);
            texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            texture->setResizeNonPowerOfTwoHint(false);
            blendmapTextures.push_back(texture);

            textureCompileDummy->getOrCreateStateSet()->setTextureAttributeAndModes(dummyTextureCounter++, blendmapTextures.back());
        }

        // use texture coordinates for both texture units, the layer texture and blend texture
        for (unsigned int i=0; i<2; ++i)
            geometry->setTexCoordArray(i, mCache.getUVBuffer());

        float blendmapScale = ESM::Land::LAND_TEXTURE_SIZE*chunkSize;
        osg::ref_ptr<osgFX::Effect> effect (new Terrain::Effect(mShaderManager ? useShaders : false, mResourceSystem->getSceneManager()->getForcePerPixelLighting(), mResourceSystem->getSceneManager()->getClampLighting(),
                                                                mShaderManager, layers, blendmapTextures, blendmapScale, blendmapScale));

        effect->addCullCallback(new SceneUtil::LightListCallback);

        transform->addChild(effect);

        osg::Node* toAttach = geometry.get();

        effect->addChild(toAttach);

        if (mIncrementalCompileOperation)
        {
            mIncrementalCompileOperation->add(toAttach);
            mIncrementalCompileOperation->add(textureCompileDummy);
        }

        return transform;
    }
}

void TerrainGrid::loadCell(int x, int y)
{
    if (mGrid.find(std::make_pair(x, y)) != mGrid.end())
        return; // already loaded

    // try to get it from the cache
    osg::ref_ptr<osg::Node> terrainNode;
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mGridCacheMutex);
        Grid::const_iterator found = mGridCache.find(std::make_pair(x,y));
        if (found != mGridCache.end())
        {
            terrainNode = found->second;
            if (!terrainNode)
                return; // no terrain defined
        }
    }

    // didn't find in cache, build it
    if (!terrainNode)
    {
        osg::Vec2f center(x+0.5f, y+0.5f);
        terrainNode = buildTerrain(NULL, 1.f, center);
        if (!terrainNode)
            return; // no terrain defined
    }

    mTerrainRoot->addChild(terrainNode);

    mGrid[std::make_pair(x,y)] = terrainNode;
}

void TerrainGrid::unloadCell(int x, int y)
{
    Grid::iterator it = mGrid.find(std::make_pair(x,y));
    if (it == mGrid.end())
        return;

    osg::ref_ptr<osg::Node> terrainNode = it->second;
    mTerrainRoot->removeChild(terrainNode);

    if (mUnrefQueue.get())
        mUnrefQueue->push(terrainNode);

    mGrid.erase(it);
}

void TerrainGrid::updateCache()
{
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mGridCacheMutex);
        for (Grid::iterator it = mGridCache.begin(); it != mGridCache.end();)
        {
            if (it->second->referenceCount() <= 1)
                mGridCache.erase(it++);
            else
                ++it;
        }
    }

    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mTextureCacheMutex);
        for (TextureCache::iterator it = mTextureCache.begin(); it != mTextureCache.end();)
        {
            if (it->second->referenceCount() <= 1)
                mTextureCache.erase(it++);
            else
                ++it;
        }
    }
}

void TerrainGrid::updateTextureFiltering()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mTextureCacheMutex);
    for (TextureCache::iterator it = mTextureCache.begin(); it != mTextureCache.end(); ++it)
        mResourceSystem->getSceneManager()->applyFilterSettings(it->second);
}

}
