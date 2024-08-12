#include "chunkmanager.hpp"

#include <osg/Material>
#include <osg/Texture2D>

#include <osgUtil/IncrementalCompileOperation>

#include <components/resource/objectcache.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/lightmanager.hpp>

#include <components/esmterrain/storage.hpp>

#include "compositemaprenderer.hpp"
#include "material.hpp"
#include "storage.hpp"
#include "terraindrawable.hpp"
#include "texturemanager.hpp"

namespace Terrain
{

    ChunkManager::ChunkManager(Storage* storage, Resource::SceneManager* sceneMgr, TextureManager* textureManager,
        CompositeMapRenderer* renderer, ESM::RefId worldspace, double expiryDelay)
        : GenericResourceManager<ChunkKey>(nullptr, expiryDelay)
        , QuadTreeWorld::ChunkManager(worldspace)
        , mStorage(storage)
        , mSceneManager(sceneMgr)
        , mTextureManager(textureManager)
        , mCompositeMapRenderer(renderer)
        , mNodeMask(0)
        , mCompositeMapSize(512)
        , mCompositeMapLevel(1.f)
        , mMaxCompGeometrySize(1.f)
    {
        mMultiPassRoot = new osg::StateSet;
        mMultiPassRoot->setRenderingHint(osg::StateSet::OPAQUE_BIN);
        osg::ref_ptr<osg::Material> material(new osg::Material);
        material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
        mMultiPassRoot->setAttributeAndModes(material, osg::StateAttribute::ON);
    }

    // FIXME: don't know which is worse, adding duplicated code here or adding a parameter to
    //        Terrain::QuadTreeWorld::getChunk().
    osg::ref_ptr<osg::Node> ChunkManager::getChunk(float size, const osg::Vec2f& center, unsigned char lod,
        unsigned int lodFlags, bool activeGrid, const osg::Vec3f& viewPoint, bool compile, int quad)
    {
        // Override lod with the vertexLodMod adjusted value.
        // TODO: maybe we can refactor this code by moving all vertexLodMod code into this class.
        lod = static_cast<unsigned char>(lodFlags >> (4 * 4));

        const ChunkKey key{ .mCenter = center, .mLod = lod, .mLodFlags = lodFlags };
        if (osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(key))
            return static_cast<osg::Node*>(obj.get());

        const TerrainDrawable* templateGeometry = nullptr;
        const TemplateKey templateKey{ .mCenter = center, .mLod = lod };
        const auto pair = mCache->lowerBound(templateKey);
        if (pair.has_value() && templateKey == TemplateKey{ .mCenter = pair->first.mCenter, .mLod = pair->first.mLod })
            templateGeometry = static_cast<const TerrainDrawable*>(pair->second.get());

        osg::ref_ptr<osg::Node> node = createChunk(size, center, lod, lodFlags, compile, templateGeometry, quad);
        mCache->addEntryToObjectCache(key, node.get());
        return node;
    }

    // called from either TerrainGrid::buildTerrain() or QuadTreeWorld::loadRenderingNode()
    // calls createChunk()
    osg::ref_ptr<osg::Node> ChunkManager::getChunk(float size, const osg::Vec2f& center, unsigned char lod,
        unsigned int lodFlags, bool activeGrid, const osg::Vec3f& viewPoint, bool compile)
    {
        // Override lod with the vertexLodMod adjusted value.
        // TODO: maybe we can refactor this code by moving all vertexLodMod code into this class.
        lod = static_cast<unsigned char>(lodFlags >> (4 * 4));

        const ChunkKey key{ .mCenter = center, .mLod = lod, .mLodFlags = lodFlags };
        if (osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(key))
            return static_cast<osg::Node*>(obj.get());

        const TerrainDrawable* templateGeometry = nullptr;
        const TemplateKey templateKey{ .mCenter = center, .mLod = lod };
        const auto pair = mCache->lowerBound(templateKey);
        if (pair.has_value() && templateKey == TemplateKey{ .mCenter = pair->first.mCenter, .mLod = pair->first.mLod })
            templateGeometry = static_cast<const TerrainDrawable*>(pair->second.get());

        osg::ref_ptr<osg::Node> node = createChunk(size, center, lod, lodFlags, compile, templateGeometry);
        mCache->addEntryToObjectCache(key, node.get());
        return node;
    }

    void ChunkManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        Resource::reportStats("Terrain Chunk", frameNumber, mCache->getStats(), *stats);
    }

    void ChunkManager::clearCache()
    {
        GenericResourceManager<ChunkKey>::clearCache();

        mBufferCache.clearCache();
    }

    void ChunkManager::releaseGLObjects(osg::State* state)
    {
        GenericResourceManager<ChunkKey>::releaseGLObjects(state);
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

    void ChunkManager::createCompositeMapGeometry(
        float chunkSize, const osg::Vec2f& chunkCenter, const osg::Vec4f& texCoords, CompositeMap& compositeMap)
    {
        if (chunkSize > mMaxCompGeometrySize)
        {
            createCompositeMapGeometry(chunkSize / 2.f, chunkCenter + osg::Vec2f(chunkSize / 4.f, chunkSize / 4.f),
                osg::Vec4f(
                    texCoords.x() + texCoords.z() / 2.f, texCoords.y(), texCoords.z() / 2.f, texCoords.w() / 2.f),
                compositeMap);
            createCompositeMapGeometry(chunkSize / 2.f, chunkCenter + osg::Vec2f(-chunkSize / 4.f, chunkSize / 4.f),
                osg::Vec4f(texCoords.x(), texCoords.y(), texCoords.z() / 2.f, texCoords.w() / 2.f), compositeMap);
            createCompositeMapGeometry(chunkSize / 2.f, chunkCenter + osg::Vec2f(chunkSize / 4.f, -chunkSize / 4.f),
                osg::Vec4f(texCoords.x() + texCoords.z() / 2.f, texCoords.y() + texCoords.w() / 2.f,
                    texCoords.z() / 2.f, texCoords.w() / 2.f),
                compositeMap);
            createCompositeMapGeometry(chunkSize / 2.f, chunkCenter + osg::Vec2f(-chunkSize / 4.f, -chunkSize / 4.f),
                osg::Vec4f(
                    texCoords.x(), texCoords.y() + texCoords.w() / 2.f, texCoords.z() / 2.f, texCoords.w() / 2.f),
                compositeMap);
        }
        else
        {
            float left = texCoords.x() * 2.f - 1;
            float top = texCoords.y() * 2.f - 1;
            float width = texCoords.z() * 2.f;
            float height = texCoords.w() * 2.f;

            std::vector<osg::ref_ptr<osg::StateSet>> passes = createPasses(chunkSize, chunkCenter, true);
            for (std::vector<osg::ref_ptr<osg::StateSet>>::iterator it = passes.begin(); it != passes.end(); ++it)
            {
                osg::ref_ptr<osg::Geometry> geom = osg::createTexturedQuadGeometry(
                    osg::Vec3(left, top, 0), osg::Vec3(width, 0, 0), osg::Vec3(0, height, 0));
                geom->setUseDisplayList(
                    false); // don't bother making a display list for an object that is just rendered once.
                geom->setUseVertexBufferObjects(false);
                geom->setTexCoordArray(1, geom->getTexCoordArray(0), osg::Array::BIND_PER_VERTEX);

                geom->setStateSet(*it);

                compositeMap.mDrawables.emplace_back(geom);
            }
        }
    }

// >   openmw.exe!Terrain::ChunkManager::createPasses() Line 188
//     openmw.exe!Terrain::ChunkManager::createCompositeMapGeometry() Line 123
//     openmw.exe!Terrain::ChunkManager::createChunk() Line 275
//     openmw.exe!Terrain::ChunkManager::getChunk() Line 59
//     openmw.exe!Terrain::QuadTreeWorld::loadRenderingNode() Line 397
//     openmw.exe!Terrain::QuadTreeWorld::preload() Line 558
//     openmw.exe!MWWorld::TerrainPreloadItem::doWork() Line 184
//     openmw.exe!SceneUtil::WorkThread::run() Line 135
    std::vector<osg::ref_ptr<osg::StateSet>> ChunkManager::createPasses(
        float chunkSize, const osg::Vec2f& chunkCenter, bool forCompositeMap, int quad)
    {
        std::vector<LayerInfo> layerList;
        std::vector<osg::ref_ptr<osg::Image>> blendmaps;

        if (quad >= 0) // NOTE: quad == -1 has a special meaning of "no quads"
        {
            static_cast<ESMTerrain::Storage*>(mStorage)
                ->getQuadBlendmaps(chunkSize, chunkCenter, blendmaps, layerList, mWorldspace, quad);
        }
        else
            mStorage->getBlendmaps(chunkSize, chunkCenter, blendmaps, layerList, mWorldspace);

        bool useShaders = mSceneManager->getForceShaders();
        if (!mSceneManager->getClampLighting())
            useShaders = true; // always use shaders when lighting is unclamped, this is to avoid lighting seams between
                               // a terrain chunk with normal maps and one without normal maps

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

        std::vector<osg::ref_ptr<osg::Texture2D>> blendmapTextures;
        for (std::vector<osg::ref_ptr<osg::Image>>::const_iterator it = blendmaps.begin(); it != blendmaps.end(); ++it)
        {
            osg::ref_ptr<osg::Texture2D> texture(new osg::Texture2D);
            texture->setImage(*it);
            texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            texture->setResizeNonPowerOfTwoHint(false);
            blendmapTextures.push_back(texture);
        }

        // NOTE: This needs to get different values for TES4.  That is, blendmapScale should
        //       be 16 for TES3 and 6 for TES4 after calling getBlendmapScale() if we were
        //       using TerrainGrid (i.e. chunksize of 1.f).  See the way Terrain::createPasses()
        //       uses BlendmapTexMat::value(blendmapScale).  This scaling won't work if using
        //       QuadTree with chunkSize other than 1.f (in which case need to do sampling).
        //
        //       We should remember that in TES4/ESM4 the land "chunk" size is not the same
        //       size as the texture.  So we may have to do some maths here but it is unclear
        //       whether it will work out properly until some testing is done.
        //
        // FIXME: TES5 and FO3/FONV may have different texture scaling - requires testing.
        float blendmapScale = mStorage->getBlendmapScale(chunkSize);

        // TODO: not so sure about (i.e. don't understand) using blendmapScale for layerTileSize
        return ::Terrain::createPasses(
            useShaders, mSceneManager, layers, blendmapTextures, blendmapScale, blendmapScale, quad);
    }

    osg::ref_ptr<osg::Node> ChunkManager::createChunk(float chunkSize, const osg::Vec2f& chunkCenter, unsigned char lod,
        unsigned int lodFlags, bool compile, const TerrainDrawable* templateGeometry, int quad)
    {
        osg::ref_ptr<TerrainDrawable> geometry(new TerrainDrawable);

        if (!templateGeometry)
        {
            osg::ref_ptr<osg::Vec3Array> positions(new osg::Vec3Array);
            osg::ref_ptr<osg::Vec3Array> normals(new osg::Vec3Array);
            osg::ref_ptr<osg::Vec4ubArray> colors(new osg::Vec4ubArray);
            colors->setNormalize(true);

// FIXME: I have a suspicion that existing fillVertexBuffers() probably already works even with
//        the unwanted Morrowind specific "fixes".
#if 1
            // NOTE: decided on a new method rather than pass quad to fillVertexBuffers() just
            //       in case the Morrowind specific "fixes" causes problems
            // NOTE: LOD is not supported
            if (quad >= 0) // NOTE: quad == -1 has a special meaning of "no quads"
                static_cast<ESMTerrain::Storage*>(mStorage)
                    ->fillQuadVertexBuffers(chunkSize, chunkCenter, mWorldspace, *positions, *normals, *colors, quad);
            else
#endif
                mStorage->fillVertexBuffers(lod, chunkSize, chunkCenter, mWorldspace, *positions, *normals, *colors);

            osg::ref_ptr<osg::VertexBufferObject> vbo(new osg::VertexBufferObject);
            positions->setVertexBufferObject(vbo);
            normals->setVertexBufferObject(vbo);
            colors->setVertexBufferObject(vbo);

            geometry->setVertexArray(positions);
            geometry->setNormalArray(normals, osg::Array::BIND_PER_VERTEX);
            geometry->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
        }
        else
        {
            // Unfortunately we need to copy vertex data because of poor coupling with VertexBufferObject.
            osg::ref_ptr<osg::Array> positions
                = static_cast<osg::Array*>(templateGeometry->getVertexArray()->clone(osg::CopyOp::DEEP_COPY_ALL));
            osg::ref_ptr<osg::Array> normals
                = static_cast<osg::Array*>(templateGeometry->getNormalArray()->clone(osg::CopyOp::DEEP_COPY_ALL));
            osg::ref_ptr<osg::Array> colors
                = static_cast<osg::Array*>(templateGeometry->getColorArray()->clone(osg::CopyOp::DEEP_COPY_ALL));

            osg::ref_ptr<osg::VertexBufferObject> vbo(new osg::VertexBufferObject);
            positions->setVertexBufferObject(vbo);
            normals->setVertexBufferObject(vbo);
            colors->setVertexBufferObject(vbo);

            geometry->setVertexArray(positions);
            geometry->setNormalArray(normals, osg::Array::BIND_PER_VERTEX);
            geometry->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
        }

        geometry->setUseDisplayList(false);
        geometry->setUseVertexBufferObjects(true);

        if (chunkSize <= 1.f)
            geometry->setLightListCallback(new SceneUtil::LightListCallback);

        unsigned int numVerts = (mStorage->getCellVertices(mWorldspace) - 1) * chunkSize / (1 << lod) + 1;

        geometry->addPrimitiveSet(mBufferCache.getIndexBuffer(numVerts, lodFlags));

        bool useCompositeMap = chunkSize >= mCompositeMapLevel;
        unsigned int numUvSets = useCompositeMap ? 1 : 2;

        geometry->setTexCoordArrayList(osg::Geometry::ArrayList(numUvSets, mBufferCache.getUVBuffer(numVerts)));

        geometry->createClusterCullingCallback();

        geometry->setStateSet(mMultiPassRoot);

        if (templateGeometry)
        {
            if (templateGeometry->getCompositeMap())
            {
                geometry->setCompositeMap(templateGeometry->getCompositeMap());
                geometry->setCompositeMapRenderer(mCompositeMapRenderer);
            }
            geometry->setPasses(templateGeometry->getPasses());
        }
        else
        {
            if (useCompositeMap)
            {
                osg::ref_ptr<CompositeMap> compositeMap = new CompositeMap;
                compositeMap->mTexture = createCompositeMapRTT();

                createCompositeMapGeometry(chunkSize, chunkCenter, osg::Vec4f(0, 0, 1, 1), *compositeMap);

                mCompositeMapRenderer->addCompositeMap(compositeMap.get(), false);

                geometry->setCompositeMap(compositeMap);
                geometry->setCompositeMapRenderer(mCompositeMapRenderer);

                TextureLayer layer;
                layer.mDiffuseMap = compositeMap->mTexture;
                layer.mParallax = false;
                layer.mSpecular = false;
                geometry->setPasses(::Terrain::createPasses(
                    mSceneManager->getForceShaders() || !mSceneManager->getClampLighting(), mSceneManager,
                    std::vector<TextureLayer>(1, layer), std::vector<osg::ref_ptr<osg::Texture2D>>(), 1.f, 1.f));
            }
            else
            {
                // FIXME: maybe we need to pass quad here or call a new method
                //        e.g. createQuadPasses()
                geometry->setPasses(createPasses(chunkSize, chunkCenter, false, quad));
            }
        }

        geometry->setupWaterBoundingBox(-1, chunkSize * mStorage->getCellWorldSize(mWorldspace) / numVerts);

        if (!templateGeometry && compile && mSceneManager->getIncrementalCompileOperation())
        {
            mSceneManager->getIncrementalCompileOperation()->add(geometry);
        }
        geometry->setNodeMask(mNodeMask);

        return geometry;
    }

}
