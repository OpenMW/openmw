#include "localmap.hpp"

#include <OgreMaterialManager.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreCamera.h>
#include <OgreTextureManager.h>
#include <OgreRenderTexture.h>
#include <OgreViewport.h>

#include <components/esm/fogstate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"

#include "renderconst.hpp"
#include "renderingmanager.hpp"

using namespace MWRender;
using namespace Ogre;

LocalMap::LocalMap(OEngine::Render::OgreRenderer* rend, MWRender::RenderingManager* rendering)
    : mMapResolution(Settings::Manager::getInt("local map resolution", "Map"))
    , mAngle(0.f)
    , mInterior(false)
{
    mRendering = rend;
    mRenderingManager = rendering;

    mCameraPosNode = mRendering->getScene()->getRootSceneNode()->createChildSceneNode();
    mCameraRotNode = mCameraPosNode->createChildSceneNode();
    mCameraNode = mCameraRotNode->createChildSceneNode();

    mCellCamera = mRendering->getScene()->createCamera("CellCamera");
    mCellCamera->setProjectionType(PT_ORTHOGRAPHIC);

    mCameraNode->attachObject(mCellCamera);

    mLight = mRendering->getScene()->createLight();
    mLight->setType (Ogre::Light::LT_DIRECTIONAL);
    mLight->setDirection (Ogre::Vector3(0.3f, 0.3f, -0.7f));
    mLight->setVisible (false);
    mLight->setDiffuseColour (ColourValue(0.7f,0.7f,0.7f));

    mRenderTexture = TextureManager::getSingleton().createManual(
                    "localmap/rtt",
                    ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    TEX_TYPE_2D,
                    mMapResolution, mMapResolution,
                    0,
                    PF_R8G8B8,
                    TU_RENDERTARGET);

    mRenderTarget = mRenderTexture->getBuffer()->getRenderTarget();
    mRenderTarget->setAutoUpdated(false);
    Viewport* vp = mRenderTarget->addViewport(mCellCamera);
    vp->setOverlaysEnabled(false);
    vp->setShadowsEnabled(false);
    vp->setBackgroundColour(ColourValue(0, 0, 0));
    vp->setVisibilityMask(RV_Map);
    vp->setMaterialScheme("local_map");
}

LocalMap::~LocalMap()
{
}

const Ogre::Vector2 LocalMap::rotatePoint(const Ogre::Vector2& p, const Ogre::Vector2& c, const float angle)
{
    return Vector2( Math::Cos(angle) * (p.x - c.x) - Math::Sin(angle) * (p.y - c.y) + c.x,
                    Math::Sin(angle) * (p.x - c.x) + Math::Cos(angle) * (p.y - c.y) + c.y);
}

std::string LocalMap::coordStr(const int x, const int y)
{
    return StringConverter::toString(x) + "_" + StringConverter::toString(y);
}

void LocalMap::clear()
{
    // Not actually removing the Textures here. That doesnt appear to work properly. It seems MyGUI still keeps some pointers.
    mBuffers.clear();
}

void LocalMap::saveFogOfWar(MWWorld::CellStore* cell)
{
    if (!mInterior)
    {
        std::string textureName = "Cell_"+coordStr(cell->getCell()->getGridX(), cell->getCell()->getGridY())+"_fog";
        std::auto_ptr<ESM::FogState> fog (new ESM::FogState());
        fog->mFogTextures.push_back(ESM::FogTexture());

        TexturePtr tex = TextureManager::getSingleton().getByName(textureName);
        if (tex.isNull())
            return;

        Ogre::Image image;
        tex->load();
        tex->convertToImage(image);

        Ogre::DataStreamPtr encoded = image.encode("tga");
        fog->mFogTextures.back().mImageData.resize(encoded->size());
        encoded->read(&fog->mFogTextures.back().mImageData[0], encoded->size());

        cell->setFog(fog.release());
    }
    else
    {
        Vector2 min(mBounds.getMinimum().x, mBounds.getMinimum().y);
        Vector2 max(mBounds.getMaximum().x, mBounds.getMaximum().y);
        Vector2 length = max-min;
        const int segsX = static_cast<int>(std::ceil(length.x / sSize));
        const int segsY = static_cast<int>(std::ceil(length.y / sSize));

        mInteriorName = cell->getCell()->mName;

        std::auto_ptr<ESM::FogState> fog (new ESM::FogState());

        fog->mBounds.mMinX = mBounds.getMinimum().x;
        fog->mBounds.mMaxX = mBounds.getMaximum().x;
        fog->mBounds.mMinY = mBounds.getMinimum().y;
        fog->mBounds.mMaxY = mBounds.getMaximum().y;
        fog->mNorthMarkerAngle = mAngle;

        fog->mFogTextures.reserve(segsX*segsY);

        for (int x=0; x<segsX; ++x)
        {
            for (int y=0; y<segsY; ++y)
            {
                std::string textureName = cell->getCell()->mName + "_" + coordStr(x,y) + "_fog";

                TexturePtr tex = TextureManager::getSingleton().getByName(textureName);
                if (tex.isNull())
                    return;

                Ogre::Image image;
                tex->load();
                tex->convertToImage(image);

                fog->mFogTextures.push_back(ESM::FogTexture());

                Ogre::DataStreamPtr encoded = image.encode("tga");
                fog->mFogTextures.back().mImageData.resize(encoded->size());
                encoded->read(&fog->mFogTextures.back().mImageData[0], encoded->size());

                fog->mFogTextures.back().mX = x;
                fog->mFogTextures.back().mY = y;
            }
        }

        cell->setFog(fog.release());
    }
}

void LocalMap::requestMap(MWWorld::CellStore* cell, float zMin, float zMax)
{
    mInterior = false;

    mCameraRotNode->setOrientation(Quaternion::IDENTITY);
    mCellCamera->setOrientation(Quaternion(Ogre::Math::Cos(Ogre::Degree(0)/2.f), 0, 0, -Ogre::Math::Sin(Ogre::Degree(0)/2.f)));

    int x = cell->getCell()->getGridX();
    int y = cell->getCell()->getGridY();

    std::string name = "Cell_"+coordStr(x, y);

    mCameraPosNode->setPosition(Vector3(0,0,0));

    // Note: using force=true for exterior cell maps.
    // They must be updated even if they were visited before, because the set of surrounding active cells might be different
    // (and objects in a different cell can "bleed" into another cell's map if they cross the border)
    render((x+0.5f)*sSize, (y+0.5f)*sSize, zMin, zMax, static_cast<float>(sSize), static_cast<float>(sSize), name, true);

    if (mBuffers.find(name) == mBuffers.end())
    {
        if (cell->getFog())
            loadFogOfWar(name, cell->getFog()->mFogTextures.back());
        else
            createFogOfWar(name);
    }
}

void LocalMap::requestMap(MWWorld::CellStore* cell,
                            AxisAlignedBox bounds)
{
    // If we're in an empty cell, bail out
    // The operations in this function are only valid for finite bounds
    if (bounds.isNull ())
        return;

    mInterior = true;

    mBounds = bounds;

    // Get the cell's NorthMarker rotation. This is used to rotate the entire map.
    const Vector2& north = MWBase::Environment::get().getWorld()->getNorthVector(cell);
    Radian angle = Ogre::Math::ATan2 (north.x, north.y);
    mAngle = angle.valueRadians();

    // Rotate the cell and merge the rotated corners to the bounding box
    Vector2 _center(bounds.getCenter().x, bounds.getCenter().y);
    Vector3 _c1 = bounds.getCorner(AxisAlignedBox::FAR_LEFT_BOTTOM);
    Vector3 _c2 = bounds.getCorner(AxisAlignedBox::FAR_RIGHT_BOTTOM);
    Vector3 _c3 = bounds.getCorner(AxisAlignedBox::FAR_LEFT_TOP);
    Vector3 _c4 = bounds.getCorner(AxisAlignedBox::FAR_RIGHT_TOP);

    Vector2 c1(_c1.x, _c1.y);
    Vector2 c2(_c2.x, _c2.y);
    Vector2 c3(_c3.x, _c3.y);
    Vector2 c4(_c4.x, _c4.y);
    c1 = rotatePoint(c1, _center, mAngle);
    c2 = rotatePoint(c2, _center, mAngle);
    c3 = rotatePoint(c3, _center, mAngle);
    c4 = rotatePoint(c4, _center, mAngle);
    mBounds.merge(Vector3(c1.x, c1.y, 0));
    mBounds.merge(Vector3(c2.x, c2.y, 0));
    mBounds.merge(Vector3(c3.x, c3.y, 0));
    mBounds.merge(Vector3(c4.x, c4.y, 0));

    // Do NOT change padding! This will break older savegames.
    // If the padding really needs to be changed, then it must be saved in the ESM::FogState and
    // assume the old (500) value as default for older savegames.
    const Ogre::Real padding = 500.0f;

    // Apply a little padding
    mBounds.setMinimum (mBounds.getMinimum() - Vector3(padding,padding,0));
    mBounds.setMaximum (mBounds.getMaximum() + Vector3(padding,padding,0));

    float zMin = mBounds.getMinimum().z;
    float zMax = mBounds.getMaximum().z;

    // If there is fog state in the CellStore (e.g. when it came from a savegame) we need to do some checks
    // to see if this state is still valid.
    // Both the cell bounds and the NorthMarker rotation could be changed by the content files or exchanged models.
    // If they changed by too much (for bounds, < padding is considered acceptable) then parts of the interior might not
    // be covered by the map anymore.
    // The following code detects this, and discards the CellStore's fog state if it needs to.
    if (cell->getFog())
    {
        ESM::FogState* fog = cell->getFog();

        Ogre::Vector3 newMin (fog->mBounds.mMinX, fog->mBounds.mMinY, zMin);
        Ogre::Vector3 newMax (fog->mBounds.mMaxX, fog->mBounds.mMaxY, zMax);

        Ogre::Vector3 minDiff = newMin - mBounds.getMinimum();
        Ogre::Vector3 maxDiff = newMax - mBounds.getMaximum();

        if (std::abs(minDiff.x) > 500 || std::abs(minDiff.y) > 500
            || std::abs(maxDiff.x) > 500 || std::abs(maxDiff.y) > 500
                || std::abs(mAngle - fog->mNorthMarkerAngle) > Ogre::Degree(5).valueRadians())
        {
            // Nuke it
            cell->setFog(NULL);
        }
        else
        {
            // Looks sane, use it
            mBounds = Ogre::AxisAlignedBox(newMin, newMax);
            mAngle = fog->mNorthMarkerAngle;
        }
    }

    Vector2 center(mBounds.getCenter().x, mBounds.getCenter().y);

    Vector2 min(mBounds.getMinimum().x, mBounds.getMinimum().y);
    Vector2 max(mBounds.getMaximum().x, mBounds.getMaximum().y);

    Vector2 length = max-min;

    mCellCamera->setOrientation(Quaternion::IDENTITY);
    mCameraRotNode->setOrientation(Quaternion(Math::Cos(mAngle/2.f), 0, 0, -Math::Sin(mAngle/2.f)));

    mCameraPosNode->setPosition(Vector3(center.x, center.y, 0));

    // divide into segments
    const int segsX = static_cast<int>(std::ceil(length.x / sSize));
    const int segsY = static_cast<int>(std::ceil(length.y / sSize));

    mInteriorName = cell->getCell()->mName;

    int i=0;
    for (int x=0; x<segsX; ++x)
    {
        for (int y=0; y<segsY; ++y)
        {
            Vector2 start = min + Vector2(static_cast<Ogre::Real>(sSize*x), static_cast<Ogre::Real>(sSize*y));
            Vector2 newcenter = start + sSize/2;

            std::string texturePrefix = cell->getCell()->mName + "_" + coordStr(x,y);

            render(newcenter.x - center.x, newcenter.y - center.y, zMin, zMax, static_cast<float>(sSize), static_cast<float>(sSize), texturePrefix);

            if (!cell->getFog())
                createFogOfWar(texturePrefix);
            else
            {
                ESM::FogState* fog = cell->getFog();

                // We are using the same bounds and angle as we were using when the textures were originally made. Segments should come out the same.
                if (i >= int(fog->mFogTextures.size()))
                    throw std::runtime_error("fog texture count mismatch");

                ESM::FogTexture& esm = fog->mFogTextures[i];
                loadFogOfWar(texturePrefix, esm);
            }
            ++i;
        }
    }
}

void LocalMap::createFogOfWar(const std::string& texturePrefix)
{
    const std::string texName = texturePrefix + "_fog";
    TexturePtr tex = createFogOfWarTexture(texName);

    // create a buffer to use for dynamic operations
    std::vector<uint32> buffer;

    // initialize to (0, 0, 0, 1)
    buffer.resize(sFogOfWarResolution*sFogOfWarResolution, 0xFF000000);

    // upload to the texture
    tex->load();
    memcpy(tex->getBuffer()->lock(HardwareBuffer::HBL_DISCARD), &buffer[0], sFogOfWarResolution*sFogOfWarResolution*4);
    tex->getBuffer()->unlock();

    mBuffers[texturePrefix] = buffer;
}

Ogre::TexturePtr LocalMap::createFogOfWarTexture(const std::string &texName)
{
    TexturePtr tex = TextureManager::getSingleton().getByName(texName);
    if (tex.isNull())
    {
        tex = TextureManager::getSingleton().createManual(
                        texName,
                        ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                        TEX_TYPE_2D,
                        sFogOfWarResolution, sFogOfWarResolution,
                        0,
                        PF_A8R8G8B8,
                        TU_DYNAMIC_WRITE_ONLY,
                    this // ManualResourceLoader required if the texture contents are lost (due to lost devices nonsense that can occur with D3D)
                    );
    }

    return tex;
}

void LocalMap::loadFogOfWar (const std::string& texturePrefix, ESM::FogTexture& esm)
{
    std::vector<char>& data = esm.mImageData;
    Ogre::DataStreamPtr stream(new Ogre::MemoryDataStream(&data[0], data.size()));
    Ogre::Image image;
    image.load(stream, "tga");

    if (int(image.getWidth()) != sFogOfWarResolution || int(image.getHeight()) != sFogOfWarResolution)
        throw std::runtime_error("fog texture size mismatch");

    std::string texName = texturePrefix + "_fog";

    Ogre::TexturePtr tex = createFogOfWarTexture(texName);

    tex->unload();
    tex->loadImage(image);

    // create a buffer to use for dynamic operations
    std::vector<uint32> buffer;
    buffer.resize(sFogOfWarResolution*sFogOfWarResolution);
    memcpy(&buffer[0], image.getData(), image.getSize());

    mBuffers[texturePrefix] = buffer;
}

void LocalMap::render(const float x, const float y,
                    const float zlow, const float zhigh,
                    const float xw, const float yw, const std::string& texture, bool force)
{
    mCellCamera->setFarClipDistance( (zhigh-zlow) + 2000 );
    mCellCamera->setNearClipDistance(50);

    mCellCamera->setOrthoWindow(xw, yw);
    mCameraNode->setPosition(Vector3(x, y, zhigh+1000));

    // disable fog (only necessary for fixed function, the shader based
    // materials already do this through local_map material configuration)
    float oldFogStart = mRendering->getScene()->getFogStart();
    float oldFogEnd = mRendering->getScene()->getFogEnd();
    Ogre::ColourValue oldFogColour = mRendering->getScene()->getFogColour();
    mRendering->getScene()->setFog(FOG_NONE);

    // set up lighting
    Ogre::ColourValue oldAmbient = mRendering->getScene()->getAmbientLight();
    mRendering->getScene()->setAmbientLight(Ogre::ColourValue(0.3f, 0.3f, 0.3f));
    mRenderingManager->disableLights(true);
    mLight->setVisible(true);

    TexturePtr tex;
    // try loading from memory
    tex = TextureManager::getSingleton().getByName(texture);
    if (tex.isNull())
    {
        // render
        mRenderTarget->update();

        // create a new texture and blit to it
        Ogre::TexturePtr tex = TextureManager::getSingleton().createManual(
                        texture,
                        ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                        TEX_TYPE_2D,
                        mMapResolution, mMapResolution,
                        0,
                        PF_R8G8B8);
        tex->getBuffer()->blit(mRenderTexture->getBuffer());
    }
    else if (force)
    {
        mRenderTarget->update();
        tex->getBuffer()->blit(mRenderTexture->getBuffer());
    }

    mRenderingManager->enableLights(true);
    mLight->setVisible(false);

    // re-enable fog
    mRendering->getScene()->setFog(FOG_LINEAR, oldFogColour, 0, oldFogStart, oldFogEnd);
    mRendering->getScene()->setAmbientLight(oldAmbient);
}

void LocalMap::worldToInteriorMapPosition (Ogre::Vector2 pos, float& nX, float& nY, int& x, int& y)
{
    pos = rotatePoint(pos, Vector2(mBounds.getCenter().x, mBounds.getCenter().y), mAngle);

    Vector2 min(mBounds.getMinimum().x, mBounds.getMinimum().y);

    x = static_cast<int>(std::ceil((pos.x - min.x) / sSize) - 1);
    y = static_cast<int>(std::ceil((pos.y - min.y) / sSize) - 1);

    nX = (pos.x - min.x - sSize*x)/sSize;
    nY = 1.0f-(pos.y - min.y - sSize*y)/sSize;
}

Ogre::Vector2 LocalMap::interiorMapToWorldPosition (float nX, float nY, int x, int y)
{
    Vector2 min(mBounds.getMinimum().x, mBounds.getMinimum().y);
    Ogre::Vector2 pos;

    pos.x = sSize * (nX + x) + min.x;
    pos.y = sSize * (1.0f-nY + y) + min.y;

    pos = rotatePoint(pos, Vector2(mBounds.getCenter().x, mBounds.getCenter().y), -mAngle);
    return pos;
}

bool LocalMap::isPositionExplored (float nX, float nY, int x, int y, bool interior)
{
    std::string texName = (interior ? mInteriorName + "_" : "Cell_") + coordStr(x, y);

    if (mBuffers.find(texName) == mBuffers.end())
        return false;

    nX = std::max(0.f, std::min(1.f, nX));
    nY = std::max(0.f, std::min(1.f, nY));

    int texU = static_cast<int>((sFogOfWarResolution - 1) * nX);
    int texV = static_cast<int>((sFogOfWarResolution - 1) * nY);

    Ogre::uint32 clr = mBuffers[texName][texV * sFogOfWarResolution + texU];
    uint8 alpha = (clr >> 24);
    return alpha < 200;
}

void LocalMap::loadResource(Ogre::Resource* resource)
{
    std::string resourceName = resource->getName();
    size_t pos = resourceName.find("_fog");
    if (pos != std::string::npos)
        resourceName = resourceName.substr(0, pos);
    if (mBuffers.find(resourceName) == mBuffers.end())
    {
        // create a buffer to use for dynamic operations
        std::vector<uint32> buffer;

        // initialize to (0, 0, 0, 1)
        buffer.resize(sFogOfWarResolution*sFogOfWarResolution, 0xFF000000);
        mBuffers[resourceName] = buffer;
    }

    std::vector<uint32>& buffer = mBuffers[resourceName];

    Ogre::Texture* tex = static_cast<Ogre::Texture*>(resource);
    tex->createInternalResources();
    memcpy(tex->getBuffer()->lock(HardwareBuffer::HBL_DISCARD), &buffer[0], sFogOfWarResolution*sFogOfWarResolution*4);
    tex->getBuffer()->unlock();
}

void LocalMap::updatePlayer (const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
{
    if (sFogOfWarSkip != 0)
    {
        static int count=0;
        if (++count % sFogOfWarSkip != 0)
            return;
    }

    // retrieve the x,y grid coordinates the player is in
    int x,y;
    float u,v;

    Vector2 pos(position.x, position.y);

    if (mInterior)
        worldToInteriorMapPosition(pos, u,v, x,y);

    Vector3 playerdirection = mCameraRotNode->convertWorldToLocalOrientation(orientation).yAxis();

    if (!mInterior)
    {
        x = static_cast<int>(std::ceil(pos.x / sSize) - 1);
        y = static_cast<int>(std::ceil(pos.y / sSize) - 1);
    }
    else
        MWBase::Environment::get().getWindowManager()->setActiveMap(x,y,mInterior);

    // convert from world coordinates to texture UV coordinates
    std::string texBaseName;
    if (!mInterior)
    {
        u = std::abs((pos.x - (sSize*x))/sSize);
        v = 1.0f-std::abs((pos.y - (sSize*y))/sSize);
        texBaseName = "Cell_";
    }
    else
    {
        texBaseName = mInteriorName + "_";
    }

    MWBase::Environment::get().getWindowManager()->setPlayerPos(x, y, u, v);
    MWBase::Environment::get().getWindowManager()->setPlayerDir(playerdirection.x, playerdirection.y);

    // explore radius (squared)
    const float exploreRadius = (mInterior ? 0.1f : 0.3f) * (sFogOfWarResolution-1); // explore radius from 0 to sFogOfWarResolution-1
    const float sqrExploreRadius = Math::Sqr(exploreRadius);
    const float exploreRadiusUV = exploreRadius / sFogOfWarResolution; // explore radius from 0 to 1 (UV space)

    // change the affected fog of war textures (in a 3x3 grid around the player)
    for (int mx = -1; mx<2; ++mx)
    {
        for (int my = -1; my<2; ++my)
        {

            // is this texture affected at all?
            bool affected = false;
            if (mx == 0 && my == 0) // the player is always in the center of the 3x3 grid
                affected = true;
            else
            {
                bool affectsX = (mx > 0)? (u + exploreRadiusUV > 1) : (u - exploreRadiusUV < 0);
                bool affectsY = (my > 0)? (v + exploreRadiusUV > 1) : (v - exploreRadiusUV < 0);
                affected = (affectsX && (my == 0)) || (affectsY && mx == 0) || (affectsX && affectsY);
            }

            if (!affected)
                continue;

            std::string texName = texBaseName + coordStr(x+mx,y+my*-1);

            TexturePtr tex = TextureManager::getSingleton().getByName(texName+"_fog");
            if (!tex.isNull())
            {
                std::map <std::string, std::vector<Ogre::uint32> >::iterator anIter;

                // get its buffer
                anIter = mBuffers.find(texName);
                if (anIter == mBuffers.end()) return;

                std::vector<Ogre::uint32>& aBuffer = (*anIter).second;
                int i=0;
                for (int texV = 0; texV<sFogOfWarResolution; ++texV)
                {
                    for (int texU = 0; texU<sFogOfWarResolution; ++texU)
                    {
                        float sqrDist = Math::Sqr((texU + mx*(sFogOfWarResolution-1)) - u*(sFogOfWarResolution-1))
                                + Math::Sqr((texV + my*(sFogOfWarResolution-1)) - v*(sFogOfWarResolution-1));
                        uint32 clr = aBuffer[i];
                        uint8 alpha = (clr >> 24);
                        alpha = std::min( alpha, (uint8) (std::max(0.f, std::min(1.f, (sqrDist/sqrExploreRadius)))*255) );
                        aBuffer[i] = (uint32) (alpha << 24);

                        ++i;
                    }
                }

                tex->load();

                // copy to the texture
                // NOTE: Could be optimized later. We actually only need to update the region that changed.
                // Not a big deal at the moment, the FoW is only 32x32 anyway.
                memcpy(tex->getBuffer()->lock(HardwareBuffer::HBL_DISCARD), &aBuffer[0], sFogOfWarResolution*sFogOfWarResolution*4);
                tex->getBuffer()->unlock();
            }
        }
    }
}
