#include "chunkmanager.hpp"

#include <sstream>

#include <osg/Texture2D>
#include <osg/TexEnv>
#include <osg/TexMat>

#include <osgUtil/IncrementalCompileOperation>

#include <components/resource/objectcache.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/lightmanager.hpp>

#include "terraindrawable.hpp"
#include "material.hpp"
#include "storage.hpp"
#include "texturemanager.hpp"
#include "compositemaprenderer.hpp"

namespace Terrain
{

ChunkManager::ChunkManager(Storage *storage, Resource::SceneManager *sceneMgr, TextureManager* textureManager, CompositeMapRenderer* renderer)
    : ResourceManager(nullptr)
    , mStorage(storage)
    , mSceneManager(sceneMgr)
    , mTextureManager(textureManager)
    , mCompositeMapRenderer(renderer)
    , mCompositeMapSize(512)
    , mCompositeMapLevel(1.f)
    , mCullingActive(true)
{

}

osg::ref_ptr<osg::Node> ChunkManager::getChunk(float size, const osg::Vec2f &center, int lod, unsigned int lodFlags)
{
    std::ostringstream stream;
    stream << size << " " << center.x() << " " << center.y() << " " << lod << " " << lodFlags;
    std::string id = stream.str();

    osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(id);
    if (obj)
        return obj->asNode();
    else
    {
        osg::ref_ptr<osg::Node> node = createChunk(size, center, lod, lodFlags);
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
    ResourceManager::clearCache();

    mBufferCache.clearCache();
}

void ChunkManager::releaseGLObjects(osg::State *state)
{
    ResourceManager::releaseGLObjects(state);
    mBufferCache.releaseGLObjects(state);
}

void ChunkManager::setCullingActive(bool active)
{
    mCullingActive = active;
}

osg::ref_ptr<osg::Texture2D> ChunkManager::createCompositeMapRTT()
{
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
    texture->setTextureWidth(mCompositeMapSize);
    texture->setTextureHeight(mCompositeMapSize);
    texture->setInternalFormat(GL_RGB);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

    return texture;
}

void ChunkManager::createCompositeMapGeometry(float chunkSize, const osg::Vec2f& chunkCenter, const osg::Vec4f& texCoords, CompositeMap& compositeMap)
{
    // Smaller value uses more draw calls and textures
    // Higher value creates more overdraw (not every texture layer is used everywhere)
    // This seems to be the sweet spot
    float maxCompositePiece = 4.f;

    if (chunkSize > maxCompositePiece)
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

        osg::ref_ptr<osg::Image> colorMap = new osg::Image;
        unsigned int colorMapSize = (mStorage->getCellVertices()-1)*chunkSize+1;
        colorMap->allocateImage(colorMapSize, colorMapSize, 1, GL_RGBA, GL_UNSIGNED_BYTE);

        osg::ref_ptr<osg::Vec4ubArray> colours = new osg::Vec4ubArray;
        osg::ref_ptr<osg::Vec3Array> dummy = new osg::Vec3Array; // fixme: maybe add another variant so we dont have to populate verts needlessly...
        mStorage->fillVertexBuffers(0, chunkSize, chunkCenter, dummy, dummy, colours);
        unsigned char* data = colorMap->data();
        memcpy(data, colours->getDataPointer(), colours->getTotalDataSize());

        osg::ref_ptr<osg::Texture2D> colorMapTexture = new osg::Texture2D(colorMap);
        mSceneManager->applyFilterSettings(colorMapTexture);
        colorMapTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        colorMapTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        colorMapTexture->setResizeNonPowerOfTwoHint(false);

        osg::ref_ptr<osg::TexEnv> texEnv = new osg::TexEnv;
        texEnv->setMode(osg::TexEnv::MODULATE);

        osg::Matrixf matrix;
        float scale = (colorMapSize-1)/(static_cast<float>(colorMapSize));
        matrix.preMultTranslate(osg::Vec3f(0.5f, 0.5f, 0.f));
        matrix.preMultScale(osg::Vec3f(scale, scale, 1.f));
        matrix.preMultRotate(osg::Quat(osg::PI/2, osg::Vec3f(0,0,1)));
        matrix.preMultTranslate(osg::Vec3f(-0.5f, -0.5f, 0.f));
        osg::ref_ptr<osg::TexMat> texMat = new osg::TexMat(matrix);

        for (std::vector<osg::ref_ptr<osg::StateSet> >::iterator it = passes.begin(); it != passes.end(); ++it)
        {
            osg::ref_ptr<osg::Geometry> geom = osg::createTexturedQuadGeometry(osg::Vec3(left,top,0), osg::Vec3(width,0,0), osg::Vec3(0,height,0));
            geom->setUseDisplayList(false); // don't bother making a display list for an object that is just rendered once.
            geom->setUseVertexBufferObjects(false);
            geom->setTexCoordArray(1, geom->getTexCoordArray(0), osg::Array::BIND_PER_VERTEX);
            geom->setTexCoordArray(2, geom->getTexCoordArray(0), osg::Array::BIND_PER_VERTEX);

            geom->setStateSet(*it);

            osg::StateSet* ss = (*it);
            int unit = ss->getTextureModeList().size();
            ss->setTextureAttributeAndModes(unit, colorMapTexture, osg::StateAttribute::ON);
            ss->setTextureAttributeAndModes(unit, texEnv, osg::StateAttribute::ON);
            ss->setTextureAttributeAndModes(unit, texMat, osg::StateAttribute::ON);

            compositeMap.mDrawables.push_back(geom);
        }
    }
}

std::vector<osg::ref_ptr<osg::StateSet> > ChunkManager::createPasses(float chunkSize, const osg::Vec2f &chunkCenter, bool forCompositeMap)
{
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

    return ::Terrain::createPasses(useShaders, mSceneManager->getForcePerPixelLighting(),
                                     mSceneManager->getClampLighting(), &mSceneManager->getShaderManager(), layers, blendmapTextures, blendmapScale, blendmapScale);
}

osg::ref_ptr<osg::Node> ChunkManager::createChunk(float chunkSize, const osg::Vec2f &chunkCenter, int lod, unsigned int lodFlags)
{
    osg::Vec2f worldCenter = chunkCenter*mStorage->getCellWorldSize();
    osg::ref_ptr<SceneUtil::PositionAttitudeTransform> transform (new SceneUtil::PositionAttitudeTransform);
    transform->setPosition(osg::Vec3f(worldCenter.x(), worldCenter.y(), 0.f));

    osg::ref_ptr<osg::Vec3Array> positions (new osg::Vec3Array);
    osg::ref_ptr<osg::Vec3Array> normals (new osg::Vec3Array);
    osg::ref_ptr<osg::Vec4ubArray> colors (new osg::Vec4ubArray);
    colors->setNormalize(true);

    osg::ref_ptr<osg::VertexBufferObject> vbo (new osg::VertexBufferObject);
    positions->setVertexBufferObject(vbo);
    normals->setVertexBufferObject(vbo);
    colors->setVertexBufferObject(vbo);

    mStorage->fillVertexBuffers(lod, chunkSize, chunkCenter, positions, normals, colors);

    bool useCompositeMap = chunkSize >= mCompositeMapLevel;

    osg::ref_ptr<TerrainDrawable> geometry (new TerrainDrawable);
    geometry->setVertexArray(positions);
    geometry->setNormalArray(normals, osg::Array::BIND_PER_VERTEX);
    if (!useCompositeMap)
        geometry->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
    else
    {
        osg::ref_ptr<osg::Vec4ubArray> colors = new osg::Vec4ubArray;
        colors->push_back(osg::Vec4ub(255,255,255,255));
        colors->setNormalize(true);
        geometry->setColorArray(colors, osg::Array::BIND_OVERALL);
    }

    geometry->setUseDisplayList(false);
    geometry->setUseVertexBufferObjects(true);

    if (chunkSize <= 2.f)
        geometry->setLightListCallback(new SceneUtil::LightListCallback);

    unsigned int numVerts = (mStorage->getCellVertices()-1) * chunkSize / (1 << lod) + 1;

    geometry->addPrimitiveSet(mBufferCache.getIndexBuffer(numVerts, lodFlags));

    unsigned int numUvSets = useCompositeMap ? 1 : 2;

    for (unsigned int i=0; i<numUvSets; ++i)
        geometry->setTexCoordArray(i, mBufferCache.getUVBuffer(numVerts));

    if (useCompositeMap)
    {
        osg::ref_ptr<CompositeMap> compositeMap = new CompositeMap;
        compositeMap->mTexture = createCompositeMapRTT();
        // use provided settings but no mipmapping (there are no mip maps..)
        mSceneManager->applyFilterSettings(compositeMap->mTexture);
        osg::Texture::FilterMode filter = compositeMap->mTexture->getFilter(osg::Texture::MIN_FILTER);
        if (filter == osg::Texture::LINEAR || filter == osg::Texture::LINEAR_MIPMAP_LINEAR || filter == osg::Texture::LINEAR_MIPMAP_NEAREST)
            filter = osg::Texture::LINEAR;
        else
            filter = osg::Texture::NEAREST;
        compositeMap->mTexture->setFilter(osg::Texture::MIN_FILTER, filter);

        createCompositeMapGeometry(chunkSize, chunkCenter, osg::Vec4f(0,0,1,1), *compositeMap);

        mCompositeMapRenderer->addCompositeMap(compositeMap.get(), false);

        transform->getOrCreateUserDataContainer()->setUserData(compositeMap);

        TextureLayer layer;
        layer.mDiffuseMap = compositeMap->mTexture;
        layer.mParallax = false;
        layer.mSpecular = false;
        geometry->setPasses(::Terrain::createPasses(mSceneManager->getForceShaders() || !mSceneManager->getClampLighting(), mSceneManager->getForcePerPixelLighting(),
                                                    mSceneManager->getClampLighting(), &mSceneManager->getShaderManager(), std::vector<TextureLayer>(1, layer), std::vector<osg::ref_ptr<osg::Texture2D> >(), 1.f, 1.f));
    }
    else
    {
        geometry->setPasses(createPasses(chunkSize, chunkCenter, false));
    }

    transform->addChild(geometry);

    if (!mCullingActive)
    {
        transform->setCullingActive(false);
        geometry->setCullingActive(false);
    }
    else
        transform->getBound();

    if (mSceneManager->getIncrementalCompileOperation())
    {
        mSceneManager->getIncrementalCompileOperation()->add(geometry);
    }
    return transform;
}

}
