#include "chunkmanager.hpp"

#include <sstream>

#include <osg/Texture2D>

#include <osgUtil/IncrementalCompileOperation>

#include <components/resource/objectcache.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/positionattitudetransform.hpp>

#include "terraindrawable.hpp"
#include "material.hpp"
#include "storage.hpp"
#include "texturemanager.hpp"

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

ChunkManager::ChunkManager(Storage *storage, Resource::SceneManager *sceneMgr, TextureManager* textureManager)
    : ResourceManager(NULL)
    , mStorage(storage)
    , mSceneManager(sceneMgr)
    , mTextureManager(textureManager)
    , mShaderManager(NULL)
{

}

osg::ref_ptr<osg::Node> ChunkManager::getChunk(float size, const osg::Vec2f &center)
{
    std::ostringstream stream;
    stream << size << " " << center.x() << " " << center.y();
    std::string id = stream.str();

    osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(id);
    if (obj)
        return obj->asNode();
    else
    {
        osg::ref_ptr<osg::Node> node = createChunk(size, center);
        mCache->addEntryToObjectCache(id, node.get());
        return node;
    }
}

void ChunkManager::setShaderManager(Shader::ShaderManager *shaderManager)
{
    mShaderManager = shaderManager;
}

void ChunkManager::reportStats(unsigned int frameNumber, osg::Stats *stats)
{
    stats->setAttribute(frameNumber, "Terrain Chunk", mCache->getCacheSize());
}

osg::ref_ptr<osg::Node> ChunkManager::createChunk(float chunkSize, const osg::Vec2f &chunkCenter)
{
    float minH, maxH;
    if (!mStorage->getMinMaxHeights(chunkSize, chunkCenter, minH, maxH))
        return NULL; // no terrain defined

    osg::Vec2f worldCenter = chunkCenter*mStorage->getCellWorldSize();
    osg::ref_ptr<SceneUtil::PositionAttitudeTransform> transform (new SceneUtil::PositionAttitudeTransform);
    transform->setPosition(osg::Vec3f(worldCenter.x(), worldCenter.y(), 0.f));

    osg::ref_ptr<osg::Vec3Array> positions (new osg::Vec3Array);
    osg::ref_ptr<osg::Vec3Array> normals (new osg::Vec3Array);
    osg::ref_ptr<osg::Vec4Array> colors (new osg::Vec4Array);

    osg::ref_ptr<osg::VertexBufferObject> vbo (new osg::VertexBufferObject);
    positions->setVertexBufferObject(vbo);
    normals->setVertexBufferObject(vbo);
    colors->setVertexBufferObject(vbo);

    unsigned int lod = 0;

    mStorage->fillVertexBuffers(lod, chunkSize, chunkCenter, positions, normals, colors);

    osg::ref_ptr<TerrainDrawable> geometry (new TerrainDrawable);
    geometry->setVertexArray(positions);
    geometry->setNormalArray(normals, osg::Array::BIND_PER_VERTEX);
    geometry->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
    geometry->setUseDisplayList(false);
    geometry->setUseVertexBufferObjects(true);

    unsigned int numVerts = (mStorage->getCellVertices()-1) * chunkSize / (1 << lod) + 1;

    geometry->addPrimitiveSet(mBufferCache.getIndexBuffer(numVerts, 0));


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

            if (!it->mNormalMap.empty())
                textureLayer.mNormalMap = mTextureManager->getTexture(it->mNormalMap);

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
    }

    // use texture coordinates for both texture units, the layer texture and blend texture
    for (unsigned int i=0; i<2; ++i)
        geometry->setTexCoordArray(i, mBufferCache.getUVBuffer(numVerts));

    float blendmapScale = mStorage->getBlendmapScale(chunkSize);

    geometry->setPasses(createPasses(mShaderManager ? useShaders : false, mSceneManager->getForcePerPixelLighting(),
                                     mSceneManager->getClampLighting(), mShaderManager, layers, blendmapTextures, blendmapScale, blendmapScale));

    transform->addChild(geometry);

    if (mSceneManager->getIncrementalCompileOperation())
    {
        mSceneManager->getIncrementalCompileOperation()->add(geometry);
    }
    return transform;
}

}
