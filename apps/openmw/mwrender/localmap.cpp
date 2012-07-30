#include "localmap.hpp"

#include <OgreOverlayManager.h>
#include <OgreMaterialManager.h>
#include <OgreHardwarePixelBuffer.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwgui/window_manager.hpp"

#include "renderconst.hpp"
#include "renderingmanager.hpp"

using namespace MWRender;
using namespace Ogre;

LocalMap::LocalMap(OEngine::Render::OgreRenderer* rend, MWRender::RenderingManager* rendering) :
    mInterior(false), mCellX(0), mCellY(0)
{
    mRendering = rend;
    mRenderingManager = rendering;

    mCameraPosNode = mRendering->getScene()->getRootSceneNode()->createChildSceneNode();
    mCameraRotNode = mCameraPosNode->createChildSceneNode();
    mCameraNode = mCameraRotNode->createChildSceneNode();

    mCellCamera = mRendering->getScene()->createCamera("CellCamera");
    mCellCamera->setProjectionType(PT_ORTHOGRAPHIC);
    // look down -y
    const float sqrt0pt5 = 0.707106781;
    mCellCamera->setOrientation(Quaternion(sqrt0pt5, -sqrt0pt5, 0, 0));

    mCameraNode->attachObject(mCellCamera);
}

LocalMap::~LocalMap()
{
    deleteBuffers();
}

const Ogre::Vector2 LocalMap::rotatePoint(const Ogre::Vector2& p, const Ogre::Vector2& c, const float angle)
{
    return Vector2( Math::Cos(angle) * (p.x - c.x) - Math::Sin(angle) * (p.y - c.y) + c.x,
                    Math::Sin(angle) * (p.x - c.x) + Math::Cos(angle) * (p.y - c.y) + c.y);
}

void LocalMap::deleteBuffers()
{
    mBuffers.clear();
}

void LocalMap::saveTexture(const std::string& texname, const std::string& filename)
{
    TexturePtr tex = TextureManager::getSingleton().getByName(texname);
    if (tex.isNull()) return;
    HardwarePixelBufferSharedPtr readbuffer = tex->getBuffer();
    readbuffer->lock(HardwareBuffer::HBL_NORMAL );
    const PixelBox &readrefpb = readbuffer->getCurrentLock();
    uchar *readrefdata = static_cast<uchar*>(readrefpb.data);

    Image img;
    img = img.loadDynamicImage (readrefdata, tex->getWidth(),
        tex->getHeight(), tex->getFormat());
    img.save("./" + filename);

    readbuffer->unlock();
}

std::string LocalMap::coordStr(const int x, const int y)
{
    return StringConverter::toString(x) + "_" + StringConverter::toString(y);
}

void LocalMap::saveFogOfWar(MWWorld::Ptr::CellStore* cell)
{
    if (!mInterior)
    {
        /*saveTexture("Cell_"+coordStr(mCellX, mCellY)+"_fog",
            "Cell_"+coordStr(mCellX, mCellY)+"_fog.png");*/
    }
    else
    {
        Vector2 min(mBounds.getMinimum().x, mBounds.getMinimum().z);
        Vector2 max(mBounds.getMaximum().x, mBounds.getMaximum().z);
        Vector2 length = max-min;

        // divide into segments
        const int segsX = std::ceil( length.x / sSize );
        const int segsY = std::ceil( length.y / sSize );

        for (int x=0; x<segsX; ++x)
        {
            for (int y=0; y<segsY; ++y)
            {
                /*saveTexture(
                    mInteriorName + "_" + coordStr(x,y) + "_fog",
                    mInteriorName + "_" + coordStr(x,y) + "_fog.png");*/
            }
        }
    }
}

void LocalMap::requestMap(MWWorld::Ptr::CellStore* cell)
{
    mInterior = false;

    mCameraRotNode->setOrientation(Quaternion::IDENTITY);

    std::string name = "Cell_"+coordStr(cell->cell->data.gridX, cell->cell->data.gridY);

    int x = cell->cell->data.gridX;
    int y = cell->cell->data.gridY;

    mCameraPosNode->setPosition(Vector3(0,0,0));

    render((x+0.5)*sSize, (-y-0.5)*sSize, -10000, 10000, sSize, sSize, name);
}

void LocalMap::requestMap(MWWorld::Ptr::CellStore* cell,
                            AxisAlignedBox bounds)
{
    mInterior = true;
    mBounds = bounds;

    Vector2 z(mBounds.getMaximum().y, mBounds.getMinimum().y);

    const Vector2& north = MWBase::Environment::get().getWorld()->getNorthVector(cell);
    Radian angle(std::atan2(-north.x, -north.y));
    mAngle = angle.valueRadians();
    mCameraRotNode->setOrientation(Quaternion(Math::Cos(angle/2.f), 0, Math::Sin(angle/2.f), 0));

    // rotate the cell and merge the rotated corners to the bounding box
    Vector2 _center(bounds.getCenter().x, bounds.getCenter().z);
    Vector3 _c1 = bounds.getCorner(AxisAlignedBox::NEAR_LEFT_BOTTOM);
    Vector3 _c2 = bounds.getCorner(AxisAlignedBox::FAR_LEFT_BOTTOM);
    Vector3 _c3 = bounds.getCorner(AxisAlignedBox::NEAR_RIGHT_BOTTOM);
    Vector3 _c4 = bounds.getCorner(AxisAlignedBox::FAR_RIGHT_BOTTOM);
    Vector2 c1(_c1.x, _c1.z);
    Vector2 c2(_c2.x, _c2.z);
    Vector2 c3(_c3.x, _c3.z);
    Vector2 c4(_c4.x, _c4.z);
    c1 = rotatePoint(c1, _center, mAngle);
    c2 = rotatePoint(c2, _center, mAngle);
    c3 = rotatePoint(c3, _center, mAngle);
    c4 = rotatePoint(c4, _center, mAngle);
    mBounds.merge(Vector3(c1.x, 0, c1.y));
    mBounds.merge(Vector3(c2.x, 0, c2.y));
    mBounds.merge(Vector3(c3.x, 0, c3.y));
    mBounds.merge(Vector3(c4.x, 0, c4.y));

    Vector2 center(mBounds.getCenter().x, mBounds.getCenter().z);

    Vector2 min(mBounds.getMinimum().x, mBounds.getMinimum().z);
    Vector2 max(mBounds.getMaximum().x, mBounds.getMaximum().z);

    Vector2 length = max-min;

    mCameraPosNode->setPosition(Vector3(center.x, 0, center.y));

    // divide into segments
    const int segsX = std::ceil( length.x / sSize );
    const int segsY = std::ceil( length.y / sSize );

    mInteriorName = cell->cell->name;

    for (int x=0; x<segsX; ++x)
    {
        for (int y=0; y<segsY; ++y)
        {
            Vector2 start = min + Vector2(sSize*x,sSize*y);
            Vector2 newcenter = start + 4096;

            render(newcenter.x - center.x, newcenter.y - center.y, z.y, z.x, sSize, sSize,
                cell->cell->name + "_" + coordStr(x,y));
        }
    }
}

void LocalMap::render(const float x, const float y,
                    const float zlow, const float zhigh,
                    const float xw, const float yw, const std::string& texture)
{
    // disable fog
    // changing FOG_MODE is not a solution when using shaders, thus we have to push linear start/end
    const float fStart = mRendering->getScene()->getFogStart();
    const float fEnd = mRendering->getScene()->getFogEnd();
    const ColourValue& clr = mRendering->getScene()->getFogColour();
    mRendering->getScene()->setFog(FOG_LINEAR, clr, 0, 1000000, 10000000);

    // make everything visible
    mRendering->getScene()->setAmbientLight(ColourValue(1,1,1));
    mRenderingManager->disableLights();

    mCameraNode->setPosition(Vector3(x, zhigh+100000, y));
    //mCellCamera->setFarClipDistance( (zhigh-zlow) * 1.1 );
    mCellCamera->setFarClipDistance(0); // infinite

    mCellCamera->setOrthoWindow(xw, yw);

    TexturePtr tex;
    // try loading from memory
    tex = TextureManager::getSingleton().getByName(texture);
    if (tex.isNull())
    {
        // try loading from disk
        //if (boost::filesystem::exists(texture+".jpg"))
        //{
            /// \todo
        //}
        //else
        {
            // render
            tex = TextureManager::getSingleton().createManual(
                            texture,
                            ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                            TEX_TYPE_2D,
                            xw*sMapResolution/sSize, yw*sMapResolution/sSize,
                            0,
                            PF_R8G8B8,
                            TU_RENDERTARGET);

            RenderTarget* rtt = tex->getBuffer()->getRenderTarget();
            rtt->setAutoUpdated(false);
            Viewport* vp = rtt->addViewport(mCellCamera);
            vp->setOverlaysEnabled(false);
            vp->setShadowsEnabled(false);
            vp->setBackgroundColour(ColourValue(0, 0, 0));
            vp->setVisibilityMask(RV_Map);

            // use fallback techniques without shadows and without mrt
            vp->setMaterialScheme("local_map");

            rtt->update();

            // create "fog of war" texture
            TexturePtr tex2 = TextureManager::getSingleton().createManual(
                            texture + "_fog",
                            ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                            TEX_TYPE_2D,
                            xw*sFogOfWarResolution/sSize, yw*sFogOfWarResolution/sSize,
                            0,
                            PF_A8R8G8B8,
                            TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

            // create a buffer to use for dynamic operations
            std::vector<uint32> buffer;
            buffer.resize(sFogOfWarResolution*sFogOfWarResolution);

            // initialize to (0, 0, 0, 1)
            for (int p=0; p<sFogOfWarResolution*sFogOfWarResolution; ++p)
            {
                buffer[p] = (255 << 24);
            }

            memcpy(tex2->getBuffer()->lock(HardwareBuffer::HBL_DISCARD), &buffer[0], sFogOfWarResolution*sFogOfWarResolution*4);
            tex2->getBuffer()->unlock();

            mBuffers[texture] = buffer;

            // save to cache for next time
            //rtt->writeContentsToFile("./" + texture + ".jpg");
        }
    }

    mRenderingManager->enableLights();

    // re-enable fog
    mRendering->getScene()->setFog(FOG_LINEAR, clr, 0, fStart, fEnd);
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
    Vector3 _pos(position.x, 0, position.z);
    Vector2 pos(_pos.x, _pos.z);

    if (mInterior)
    {
        pos = rotatePoint(pos, Vector2(mBounds.getCenter().x, mBounds.getCenter().z), mAngle);
    }


    Vector3 playerdirection = -mCameraRotNode->convertWorldToLocalOrientation(orientation).zAxis();

    Vector2 min(mBounds.getMinimum().x, mBounds.getMinimum().z);

    if (!mInterior)
    {
        x = std::ceil(pos.x / sSize)-1;
        y = std::ceil(-pos.y / sSize)-1;
        mCellX = x;
        mCellY = y;
    }
    else
    {
        x = std::ceil((pos.x - min.x)/sSize)-1;
        y = std::ceil((pos.y - min.y)/sSize)-1;

        MWBase::Environment::get().getWindowManager()->setInteriorMapTexture(x,y);
    }

    // convert from world coordinates to texture UV coordinates
    float u,v;
    std::string texBaseName;
    if (!mInterior)
    {
        u = std::abs((pos.x - (sSize*x))/sSize);
        v = 1-std::abs((pos.y + (sSize*y))/sSize);
        texBaseName = "Cell_";
    }
    else
    {
        u = (pos.x - min.x - sSize*x)/sSize;
        v = (pos.y - min.y - sSize*y)/sSize;

        texBaseName = mInteriorName + "_";
    }

    MWBase::Environment::get().getWindowManager()->setPlayerPos(u, v);
    MWBase::Environment::get().getWindowManager()->setPlayerDir(playerdirection.x, -playerdirection.z);

    // explore radius (squared)
    const float sqrExploreRadius = (mInterior ? 0.01 : 0.09) * sFogOfWarResolution*sFogOfWarResolution;
    const float exploreRadius = (mInterior ? 0.1 : 0.3) * sFogOfWarResolution; // explore radius from 0 to sFogOfWarResolution
    const float exploreRadiusUV = exploreRadius / sFogOfWarResolution; // explore radius from 0 to 1 (UV space)

    int intExtMult = mInterior ? 1 : -1; // interior and exterior have reversed Y coordinates (interior: top to bottom)

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

            std::string texName = texBaseName + coordStr(x+mx,y+my*intExtMult);

            TexturePtr tex = TextureManager::getSingleton().getByName(texName+"_fog");
            if (!tex.isNull())
            {
                // get its buffer
                if (mBuffers.find(texName) == mBuffers.end()) return;
                int i=0;
                for (int texV = 0; texV<sFogOfWarResolution; ++texV)
                {
                    for (int texU = 0; texU<sFogOfWarResolution; ++texU)
                    {
                        // fix into range of 0 ... sFogOfWarResolution
                        int _texU = texU * (float(sFogOfWarResolution+1) / float(sFogOfWarResolution));
                        int _texV = texV * (float(sFogOfWarResolution+1) / float(sFogOfWarResolution));

                        float sqrDist = Math::Sqr((_texU + mx*sFogOfWarResolution) - u*sFogOfWarResolution) + Math::Sqr((_texV + my*sFogOfWarResolution) - v*sFogOfWarResolution);
                        uint32 clr = mBuffers[texName][i];
                        uint8 alpha = (clr >> 24);
                        alpha = std::min( alpha, (uint8) (std::max(0.f, std::min(1.f, (sqrDist/sqrExploreRadius)))*255) );
                        mBuffers[texName][i] = (uint32) (alpha << 24);

                        ++i;
                    }
                }

                // copy to the texture
                memcpy(tex->getBuffer()->lock(HardwareBuffer::HBL_DISCARD), &mBuffers[texName][0], sFogOfWarResolution*sFogOfWarResolution*4);
                tex->getBuffer()->unlock();
            }
        }
    }
}
