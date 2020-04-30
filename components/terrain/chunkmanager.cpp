#include "chunkmanager.hpp"

#include <sstream>

#include <osg/Texture2D>
#include <osg/ClusterCullingCallback>

#include <osgUtil/IncrementalCompileOperation>

#include <components/resource/objectcache.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/lightmanager.hpp>

#include "terraindrawable.hpp"
#include "material.hpp"
#include "storage.hpp"
#include "texturemanager.hpp"
#include "compositemaprenderer.hpp"

namespace Terrain
{

ChunkManager::ChunkManager(Storage *storage, Resource::SceneManager *sceneMgr, TextureManager* textureManager, CompositeMapRenderer* renderer)
    : GenericResourceManager<ChunkId>(nullptr)
    , mStorage(storage)
    , mSceneManager(sceneMgr)
    , mTextureManager(textureManager)
    , mCompositeMapRenderer(renderer)
    , mCompositeMapSize(512)
    , mCompositeMapLevel(1.f)
    , mMaxCompGeometrySize(1.f)
{

}

osg::ref_ptr<osg::Node> ChunkManager::getChunk(float size, const osg::Vec2f &center, unsigned char lod, unsigned int lodFlags, bool far, const osg::Vec3f& viewPoint, bool compile)
{
    ChunkId id = std::make_tuple(center, lod, lodFlags);
    osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(id);
    if (obj)
        return obj->asNode();
    else
    {
        osg::ref_ptr<osg::Node> node = createChunk(size, center, lod, lodFlags, compile);
        mCache->addEntryToObjectCache(id, node.get());
        return node;
    }
}

void ChunkManager::reportStats(unsigned int frameNumber, osg::Stats *stats) const
{
    stats->setAttribute(frameNumber, "Terrain Chunk", mCache->getCacheSize());
}

void ChunkManager::clearCache()
{
    GenericResourceManager<ChunkId>::clearCache();

    mBufferCache.clearCache();
}

void ChunkManager::releaseGLObjects(osg::State *state)
{
    GenericResourceManager<ChunkId>::releaseGLObjects(state);
    mBufferCache.releaseGLObjects(state);
}

osg::ref_ptr<osg::Texture2D> ChunkManager::createCompositeMapRTT()
{
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
    texture->setTextureWidth(mCompositeMapSize);
    texture->setTextureHeight(mCompositeMapSize);
    texture->setInternalFormat(GL_RGB);
    texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

    return texture;
}

void ChunkManager::createCompositeMapGeometry(float chunkSize, const osg::Vec2f& chunkCenter, const osg::Vec4f& texCoords, CompositeMap& compositeMap)
{
    if (chunkSize > mMaxCompGeometrySize)
    {
        createCompositeMapGeometry(chunkSize/2.f, chunkCenter + osg::Vec2f(chunkSize/4.f, chunkSize/4.f), osg::Vec4f(texCoords.x() + texCoords.z()/2.f, texCoords.y(), texCoords.z()/2.f, texCoords.w()/2.f), compositeMap);
        createCompositeMapGeometry(chunkSize/2.f, chunkCenter + osg::Vec2f(-chunkSize/4.f, chunkSize/4.f), osg::Vec4f(texCoords.x(), texCoords.y(), texCoords.z()/2.f, texCoords.w()/2.f), compositeMap);
        createCompositeMapGeometry(chunkSize/2.f, chunkCenter + osg::Vec2f(chunkSize/4.f, -chunkSize/4.f), osg::Vec4f(texCoords.x() + texCoords.z()/2.f, texCoords.y()+texCoords.w()/2.f, texCoords.z()/2.f, texCoords.w()/2.f), compositeMap);
        createCompositeMapGeometry(chunkSize/2.f, chunkCenter + osg::Vec2f(-chunkSize/4.f, -chunkSize/4.f), osg::Vec4f(texCoords.x(), texCoords.y()+texCoords.w()/2.f, texCoords.z()/2.f, texCoords.w()/2.f), compositeMap);
    }
    else
    {
        float left = texCoords.x()*2.f-1;
        float top = texCoords.y()*2.f-1;
        float width = texCoords.z()*2.f;
        float height = texCoords.w()*2.f;

        std::vector<osg::ref_ptr<osg::StateSet> > passes = createPasses(chunkSize, chunkCenter, true);
        for (std::vector<osg::ref_ptr<osg::StateSet> >::iterator it = passes.begin(); it != passes.end(); ++it)
        {
            osg::ref_ptr<osg::Geometry> geom = osg::createTexturedQuadGeometry(osg::Vec3(left,top,0), osg::Vec3(width,0,0), osg::Vec3(0,height,0));
            geom->setUseDisplayList(false); // don't bother making a display list for an object that is just rendered once.
            geom->setUseVertexBufferObjects(false);
            geom->setTexCoordArray(1, geom->getTexCoordArray(0), osg::Array::BIND_PER_VERTEX);

            geom->setStateSet(*it);

            compositeMap.mDrawables.push_back(geom);
        }
    }
}

std::vector<osg::ref_ptr<osg::StateSet> > ChunkManager::createPasses(float chunkSize, const osg::Vec2f &chunkCenter, bool forCompositeMap)
{
    std::vector<LayerInfo> layerList;
    std::vector<osg::ref_ptr<osg::Image> > blendmaps;
    mStorage->getBlendmaps(chunkSize, chunkCenter, blendmaps, layerList);

    bool useShaders = mSceneManager->getForceShaders();
    if (!mSceneManager->getClampLighting())
        useShaders = true; // always use shaders when lighting is unclamped, this is to avoid lighting seams between a terrain chunk with normal maps and one without normal maps

    std::vector<TextureLayer> layers;
    {
        for (std::vector<LayerInfo>::const_iterator it = layerList.begin(); it != layerList.end(); ++it)
        {
            TextureLayer textureLayer;
            textureLayer.mParallax = it->mParallax;
            textureLayer.mSpecular = it->mSpecular;

            textureLayer.mDiffuseMap = mTextureManager->getTexture(it->mDiffuseMap);

            if (!forCompositeMap && !it->mNormalMap.empty())
                textureLayer.mNormalMap = mTextureManager->getTexture(it->mNormalMap);

            if (it->requiresShaders())
                useShaders = true;

            layers.push_back(textureLayer);
        }
    }

    if (forCompositeMap)
        useShaders = false;

    std::vector<osg::ref_ptr<osg::Texture2D> > blendmapTextures;
    for (std::vector<osg::ref_ptr<osg::Image> >::const_iterator it = blendmaps.begin(); it != blendmaps.end(); ++it)
    {
        osg::ref_ptr<osg::Texture2D> texture (new osg::Texture2D);
        texture->setImage(*it);
        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        texture->setResizeNonPowerOfTwoHint(false);
        blendmapTextures.push_back(texture);
    }

    float blendmapScale = mStorage->getBlendmapScale(chunkSize);

    return ::Terrain::createPasses(useShaders, &mSceneManager->getShaderManager(), layers, blendmapTextures, blendmapScale, blendmapScale);
}

osg::ref_ptr<osg::Node> ChunkManager::createChunk(float chunkSize, const osg::Vec2f &chunkCenter, unsigned char lod, unsigned int lodFlags, bool compile)
{
    osg::ref_ptr<osg::Vec3Array> positions (new osg::Vec3Array);
    osg::ref_ptr<osg::Vec3Array> normals (new osg::Vec3Array);
    osg::ref_ptr<osg::Vec4ubArray> colors (new osg::Vec4ubArray);
    colors->setNormalize(true);

    osg::ref_ptr<osg::VertexBufferObject> vbo (new osg::VertexBufferObject);
    positions->setVertexBufferObject(vbo);
    normals->setVertexBufferObject(vbo);
    colors->setVertexBufferObject(vbo);

    mStorage->fillVertexBuffers(lod, chunkSize, chunkCenter, positions, normals, colors);

    osg::ref_ptr<TerrainDrawable> geometry (new TerrainDrawable);
    geometry->setVertexArray(positions);
    geometry->setNormalArray(normals, osg::Array::BIND_PER_VERTEX);
    geometry->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
    geometry->setUseDisplayList(false);
    geometry->setUseVertexBufferObjects(true);

    if (chunkSize <= 1.f)
        geometry->setLightListCallback(new SceneUtil::LightListCallback);

    unsigned int numVerts = (mStorage->getCellVertices()-1) * chunkSize / (1 << lod) + 1;

    geometry->addPrimitiveSet(mBufferCache.getIndexBuffer(numVerts, lodFlags));

    bool useCompositeMap = chunkSize >= mCompositeMapLevel;
    unsigned int numUvSets = useCompositeMap ? 1 : 2;

    for (unsigned int i=0; i<numUvSets; ++i)
        geometry->setTexCoordArray(i, mBufferCache.getUVBuffer(numVerts));

    geometry->createClusterCullingCallback();

    if (useCompositeMap)
    {
        osg::ref_ptr<CompositeMap> compositeMap = new CompositeMap;
        compositeMap->mTexture = createCompositeMapRTT();

        createCompositeMapGeometry(chunkSize, chunkCenter, osg::Vec4f(0,0,1,1), *compositeMap);

        mCompositeMapRenderer->addCompositeMap(compositeMap.get(), false);

        geometry->setCompositeMap(compositeMap);
        geometry->setCompositeMapRenderer(mCompositeMapRenderer);

        TextureLayer layer;
        layer.mDiffuseMap = compositeMap->mTexture;
        layer.mParallax = false;
        layer.mSpecular = false;
        geometry->setPasses(::Terrain::createPasses(mSceneManager->getForceShaders() || !mSceneManager->getClampLighting(), &mSceneManager->getShaderManager(), std::vector<TextureLayer>(1, layer), std::vector<osg::ref_ptr<osg::Texture2D> >(), 1.f, 1.f));
    }
    else
    {
        geometry->setPasses(createPasses(chunkSize, chunkCenter, false));
    }

    geometry->setupWaterBoundingBox(-1, chunkSize * mStorage->getCellWorldSize() / numVerts);

    if (compile && mSceneManager->getIncrementalCompileOperation())
    {
        mSceneManager->getIncrementalCompileOperation()->add(geometry);
    }
    geometry->setNodeMask(mNodeMask);

    return geometry;
}

}
