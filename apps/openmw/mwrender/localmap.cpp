#include "localmap.hpp"

#include <stdint.h>

#include <osg/Fog>
#include <osg/LightModel>
#include <osg/Texture2D>
#include <osg/ComputeBoundsVisitor>
#include <osg/LightSource>
#include <osg/PolygonMode>

#include <osgDB/ReadFile>

#include <components/debug/debuglog.hpp>
#include <components/esm/fogstate.hpp>
#include <components/esm/loadcell.hpp>
#include <components/misc/constants.hpp>
#include <components/settings/settings.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/sceneutil/shadow.hpp>
#include <components/sceneutil/vismask.hpp>
#include <components/files/memorystream.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/cellstore.hpp"

namespace
{

    class CameraLocalUpdateCallback : public osg::NodeCallback
    {
    public:
        CameraLocalUpdateCallback(MWRender::LocalMap* parent)
            : mRendered(false)
            , mParent(parent)
        {
        }

        virtual void operator()(osg::Node* node, osg::NodeVisitor*)
        {
            if (mRendered)
                node->setNodeMask(SceneUtil::Mask_Disabled);

            if (!mRendered)
            {
                mRendered = true;
                mParent->markForRemoval(static_cast<osg::Camera*>(node));
            }

            // Note, we intentionally do not traverse children here. The map camera's scene data is the same as the master camera's,
            // so it has been updated already.
            //traverse(node, nv);
        }

    private:
        bool mRendered;
        MWRender::LocalMap* mParent;
    };

    float square(float val)
    {
        return val*val;
    }

}

namespace MWRender
{

LocalMap::LocalMap(osg::Group* root)
    : mRoot(root)
    , mMapResolution(Settings::Manager::getInt("local map resolution", "Map"))
    , mMapWorldSize(Constants::CellSizeInUnits)
    , mCellDistance(Settings::Manager::getInt("local map cell distance", "Map"))
    , mAngle(0.f)
    , mInterior(false)
{
    // Increase map resolution, if use UI scaling
    float uiScale = Settings::Manager::getFloat("scaling factor", "GUI");
    if (uiScale > 1.0)
        mMapResolution *= uiScale;

    SceneUtil::FindByNameVisitor find("Scene Root");
    mRoot->accept(find);
    mSceneRoot = find.mFoundNode;
    if (!mSceneRoot)
        throw std::runtime_error("no scene root found");
}

LocalMap::~LocalMap()
{
    for (auto& camera : mActiveCameras)
        removeCamera(camera);
    for (auto& camera : mCamerasPendingRemoval)
        removeCamera(camera);
}

const osg::Vec2f LocalMap::rotatePoint(const osg::Vec2f& point, const osg::Vec2f& center, const float angle)
{
    return osg::Vec2f( std::cos(angle) * (point.x() - center.x()) - std::sin(angle) * (point.y() - center.y()) + center.x(),
                    std::sin(angle) * (point.x() - center.x()) + std::cos(angle) * (point.y() - center.y()) + center.y());
}

void LocalMap::clear()
{
    mSegments.clear();
}

void LocalMap::saveFogOfWar(MWWorld::CellStore* cell)
{
    if (!mInterior)
    {
        const MapSegment& segment = mSegments[std::make_pair(cell->getCell()->getGridX(), cell->getCell()->getGridY())];

        if (segment.mFogOfWarImage && segment.mHasFogState)
        {
            std::unique_ptr<ESM::FogState> fog (new ESM::FogState());
            fog->mFogTextures.push_back(ESM::FogTexture());

            segment.saveFogOfWar(fog->mFogTextures.back());

            cell->setFog(fog.release());
        }
    }
    else
    {
        // FIXME: segmenting code duplicated from requestMap
        osg::Vec2f min(mBounds.xMin(), mBounds.yMin());
        osg::Vec2f max(mBounds.xMax(), mBounds.yMax());
        osg::Vec2f length = max-min;
        const int segsX = static_cast<int>(std::ceil(length.x() / mMapWorldSize));
        const int segsY = static_cast<int>(std::ceil(length.y() / mMapWorldSize));

        std::unique_ptr<ESM::FogState> fog (new ESM::FogState());

        fog->mBounds.mMinX = mBounds.xMin();
        fog->mBounds.mMaxX = mBounds.xMax();
        fog->mBounds.mMinY = mBounds.yMin();
        fog->mBounds.mMaxY = mBounds.yMax();
        fog->mNorthMarkerAngle = mAngle;

        fog->mFogTextures.reserve(segsX*segsY);

        for (int x=0; x<segsX; ++x)
        {
            for (int y=0; y<segsY; ++y)
            {
                const MapSegment& segment = mSegments[std::make_pair(x,y)];

                fog->mFogTextures.push_back(ESM::FogTexture());

                // saving even if !segment.mHasFogState so we don't mess up the segmenting
                // plus, older openmw versions can't deal with empty images
                segment.saveFogOfWar(fog->mFogTextures.back());

                fog->mFogTextures.back().mX = x;
                fog->mFogTextures.back().mY = y;
            }
        }

        cell->setFog(fog.release());
    }
}

osg::ref_ptr<osg::Camera> LocalMap::createOrthographicCamera(float x, float y, float width, float height, const osg::Vec3d& upVector, float zmin, float zmax)
{
    osg::ref_ptr<osg::Camera> camera (new osg::Camera);

    camera->setProjectionMatrixAsOrtho(-width/2, width/2, -height/2, height/2, 5, (zmax-zmin) + 10);
    camera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
    camera->setViewMatrixAsLookAt(osg::Vec3d(x, y, zmax + 5), osg::Vec3d(x, y, zmin), upVector);
    camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT, osg::Camera::PIXEL_BUFFER_RTT);
    camera->setClearColor(osg::Vec4(0.f, 0.f, 0.f, 1.f));
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera->setRenderOrder(osg::Camera::PRE_RENDER);

    camera->setCullMask(SceneUtil::Mask_Scene | SceneUtil::Mask_SimpleWater | SceneUtil::Mask_Terrain | SceneUtil::Mask_Object | SceneUtil::Mask_Static);
    camera->setNodeMask(SceneUtil::Mask_RenderToTexture);

    osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;
    stateset->setAttribute(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL), osg::StateAttribute::OVERRIDE);

    // assign large value to effectively turn off fog
    // shaders don't respect glDisable(GL_FOG)
    osg::ref_ptr<osg::Fog> fog (new osg::Fog);
    fog->setStart(10000000);
    fog->setEnd(10000000);
    stateset->setAttributeAndModes(fog, osg::StateAttribute::OFF|osg::StateAttribute::OVERRIDE);

    osg::ref_ptr<osg::LightModel> lightmodel = new osg::LightModel;
    lightmodel->setAmbientIntensity(osg::Vec4(0.3f, 0.3f, 0.3f, 1.f));
    stateset->setAttributeAndModes(lightmodel, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

    osg::ref_ptr<osg::Light> light = new osg::Light;
    light->setPosition(osg::Vec4(-0.3f, -0.3f, 0.7f, 0.f));
    light->setDiffuse(osg::Vec4(0.7f, 0.7f, 0.7f, 1.f));
    light->setAmbient(osg::Vec4(0,0,0,1));
    light->setSpecular(osg::Vec4(0,0,0,0));
    light->setLightNum(0);
    light->setConstantAttenuation(1.f);
    light->setLinearAttenuation(0.f);
    light->setQuadraticAttenuation(0.f);

    osg::ref_ptr<osg::LightSource> lightSource = new osg::LightSource;
    lightSource->setLight(light);

    lightSource->setStateSetModes(*stateset, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

    SceneUtil::ShadowManager::disableShadowsForStateSet(stateset);

    camera->addChild(lightSource);
    camera->setStateSet(stateset);
    camera->setViewport(0, 0, mMapResolution, mMapResolution);
    camera->setUpdateCallback(new CameraLocalUpdateCallback(this));

    return camera;
}

void LocalMap::setupRenderToTexture(osg::ref_ptr<osg::Camera> camera, int x, int y)
{
    osg::ref_ptr<osg::Texture2D> texture (new osg::Texture2D);
    texture->setTextureSize(mMapResolution, mMapResolution);
    texture->setInternalFormat(GL_RGB);
    texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

    camera->attach(osg::Camera::COLOR_BUFFER, texture);

    camera->addChild(mSceneRoot);
    mRoot->addChild(camera);
    mActiveCameras.push_back(camera);

    MapSegment& segment = mSegments[std::make_pair(x, y)];
    segment.mMapTexture = texture;
}

bool needUpdate(std::set<std::pair<int, int> >& renderedGrid, std::set<std::pair<int, int> >& currentGrid, int cellX, int cellY)
{
    // if all the cells of the current grid are contained in the rendered grid then we can keep the old render
    for (int dx=-1;dx<2;dx+=1)
    {
        for (int dy=-1;dy<2;dy+=1)
        {
            bool haveInRenderedGrid = renderedGrid.find(std::make_pair(cellX+dx,cellY+dy)) != renderedGrid.end();
            bool haveInCurrentGrid = currentGrid.find(std::make_pair(cellX+dx,cellY+dy)) != currentGrid.end();
            if (haveInCurrentGrid && !haveInRenderedGrid)
                return true;
        }
    }
    return false;
}

void LocalMap::requestMap(const MWWorld::CellStore* cell)
{
    if (cell->isExterior())
    {
        int cellX = cell->getCell()->getGridX();
        int cellY = cell->getCell()->getGridY();

        MapSegment& segment = mSegments[std::make_pair(cellX, cellY)];
        if (!needUpdate(segment.mGrid, mCurrentGrid, cellX, cellY))
            return;
        else
        {
            segment.mGrid = mCurrentGrid;
            requestExteriorMap(cell);
        }
    }
    else
        requestInteriorMap(cell);
}

void LocalMap::addCell(MWWorld::CellStore *cell)
{
    if (cell->isExterior())
        mCurrentGrid.emplace(cell->getCell()->getGridX(), cell->getCell()->getGridY());
}

void LocalMap::removeCell(MWWorld::CellStore *cell)
{
    saveFogOfWar(cell);

    if (cell->isExterior())
    {
        std::pair<int, int> coords = std::make_pair(cell->getCell()->getGridX(), cell->getCell()->getGridY());
        mSegments.erase(coords);
        mCurrentGrid.erase(coords);
    }
    else
        mSegments.clear();
}

osg::ref_ptr<osg::Texture2D> LocalMap::getMapTexture(int x, int y)
{
    SegmentMap::iterator found = mSegments.find(std::make_pair(x, y));
    if (found == mSegments.end())
        return osg::ref_ptr<osg::Texture2D>();
    else
        return found->second.mMapTexture;
}

osg::ref_ptr<osg::Texture2D> LocalMap::getFogOfWarTexture(int x, int y)
{
    SegmentMap::iterator found = mSegments.find(std::make_pair(x, y));
    if (found == mSegments.end())
        return osg::ref_ptr<osg::Texture2D>();
    else
        return found->second.mFogOfWarTexture;
}

void LocalMap::removeCamera(osg::Camera *cam)
{
    cam->removeChildren(0, cam->getNumChildren());
    mRoot->removeChild(cam);
}

void LocalMap::markForRemoval(osg::Camera *cam)
{
    CameraVector::iterator found = std::find(mActiveCameras.begin(), mActiveCameras.end(), cam);
    if (found == mActiveCameras.end())
    {
        Log(Debug::Error) << "Error: trying to remove an inactive camera";
        return;
    }
    mActiveCameras.erase(found);
    mCamerasPendingRemoval.push_back(cam);
}

void LocalMap::cleanupCameras()
{
    if (mCamerasPendingRemoval.empty())
        return;

    for (auto& camera : mCamerasPendingRemoval)
        removeCamera(camera);

    mCamerasPendingRemoval.clear();
}

void LocalMap::requestExteriorMap(const MWWorld::CellStore* cell)
{
    mInterior = false;

    int x = cell->getCell()->getGridX();
    int y = cell->getCell()->getGridY();

    osg::BoundingSphere bound = mSceneRoot->getBound();
    float zmin = bound.center().z() - bound.radius();
    float zmax = bound.center().z() + bound.radius();

    osg::ref_ptr<osg::Camera> camera = createOrthographicCamera(x*mMapWorldSize + mMapWorldSize/2.f, y*mMapWorldSize + mMapWorldSize/2.f, mMapWorldSize, mMapWorldSize,
                                                                osg::Vec3d(0,1,0), zmin, zmax);
    camera->getOrCreateUserDataContainer()->addDescription("NoTerrainLod");
    std::ostringstream stream;
    stream << x << " " << y;
    camera->getOrCreateUserDataContainer()->addDescription(stream.str());

    setupRenderToTexture(camera, cell->getCell()->getGridX(), cell->getCell()->getGridY());

    MapSegment& segment = mSegments[std::make_pair(cell->getCell()->getGridX(), cell->getCell()->getGridY())];
    if (!segment.mFogOfWarImage)
    {
        if (cell->getFog())
            segment.loadFogOfWar(cell->getFog()->mFogTextures.back());
        else
            segment.initFogOfWar();
    }
}

void LocalMap::requestInteriorMap(const MWWorld::CellStore* cell)
{
    osg::ComputeBoundsVisitor computeBoundsVisitor;
    computeBoundsVisitor.setTraversalMask(SceneUtil::Mask_Scene | SceneUtil::Mask_Terrain | SceneUtil::Mask_Object | SceneUtil::Mask_Static);
    mSceneRoot->accept(computeBoundsVisitor);

    osg::BoundingBox bounds = computeBoundsVisitor.getBoundingBox();

    // If we're in an empty cell, bail out
    // The operations in this function are only valid for finite bounds
    if (!bounds.valid() || bounds.radius2() == 0.0)
        return;

    mInterior = true;

    mBounds = bounds;

    // Get the cell's NorthMarker rotation. This is used to rotate the entire map.
    osg::Vec2f north = MWBase::Environment::get().getWorld()->getNorthVector(cell);

    mAngle = std::atan2(north.x(), north.y());

    // Rotate the cell and merge the rotated corners to the bounding box
    osg::Vec2f origCenter(bounds.center().x(), bounds.center().y());
    osg::Vec3f origCorners[8];
    for (int i=0; i<8; ++i)
        origCorners[i] = mBounds.corner(i);

    for (int i=0; i<8; ++i)
    {
        osg::Vec3f corner = origCorners[i];
        osg::Vec2f corner2d (corner.x(), corner.y());
        corner2d = rotatePoint(corner2d, origCenter, mAngle);
        mBounds.expandBy(osg::Vec3f(corner2d.x(), corner2d.y(), 0));
    }

    // Do NOT change padding! This will break older savegames.
    // If the padding really needs to be changed, then it must be saved in the ESM::FogState and
    // assume the old (500) value as default for older savegames.
    const float padding = 500.0f;

    // Apply a little padding
    mBounds.set(mBounds._min - osg::Vec3f(padding,padding,0.f),
                mBounds._max + osg::Vec3f(padding,padding,0.f));

    float zMin = mBounds.zMin();
    float zMax = mBounds.zMax();

    // If there is fog state in the CellStore (e.g. when it came from a savegame) we need to do some checks
    // to see if this state is still valid.
    // Both the cell bounds and the NorthMarker rotation could be changed by the content files or exchanged models.
    // If they changed by too much (for bounds, < padding is considered acceptable) then parts of the interior might not
    // be covered by the map anymore.
    // The following code detects this, and discards the CellStore's fog state if it needs to.
    bool cellHasValidFog = false;
    if (cell->getFog())
    {
        ESM::FogState* fog = cell->getFog();

        osg::Vec3f newMin (fog->mBounds.mMinX, fog->mBounds.mMinY, zMin);
        osg::Vec3f newMax (fog->mBounds.mMaxX, fog->mBounds.mMaxY, zMax);

        osg::Vec3f minDiff = newMin - mBounds._min;
        osg::Vec3f maxDiff = newMax - mBounds._max;

        if (std::abs(minDiff.x()) > padding || std::abs(minDiff.y()) > padding
            || std::abs(maxDiff.x()) > padding || std::abs(maxDiff.y()) > padding
                || std::abs(mAngle - fog->mNorthMarkerAngle) > osg::DegreesToRadians(5.f))
        {
            // Nuke it
            cellHasValidFog = false;
        }
        else
        {
            // Looks sane, use it
            mBounds = osg::BoundingBox(newMin, newMax);
            mAngle = fog->mNorthMarkerAngle;
            cellHasValidFog = true;
        }
    }

    osg::Vec2f min(mBounds.xMin(), mBounds.yMin());
    osg::Vec2f max(mBounds.xMax(), mBounds.yMax());

    osg::Vec2f length = max-min;

    osg::Vec2f center(bounds.center().x(), bounds.center().y());

    // divide into segments
    const int segsX = static_cast<int>(std::ceil(length.x() / mMapWorldSize));
    const int segsY = static_cast<int>(std::ceil(length.y() / mMapWorldSize));

    int i = 0;
    for (int x=0; x<segsX; ++x)
    {
        for (int y=0; y<segsY; ++y)
        {
            osg::Vec2f start = min + osg::Vec2f(mMapWorldSize*x, mMapWorldSize*y);
            osg::Vec2f newcenter = start + osg::Vec2f(mMapWorldSize/2.f, mMapWorldSize/2.f);

            osg::Quat cameraOrient (mAngle, osg::Vec3d(0,0,-1));
            osg::Vec2f a = newcenter - center;
            osg::Vec3f rotatedCenter = cameraOrient * (osg::Vec3f(a.x(), a.y(), 0));

            osg::Vec2f pos = osg::Vec2f(rotatedCenter.x(), rotatedCenter.y()) + center;

            osg::ref_ptr<osg::Camera> camera = createOrthographicCamera(pos.x(), pos.y(),
                                                                        mMapWorldSize, mMapWorldSize,
                                                                        osg::Vec3f(north.x(), north.y(), 0.f), zMin, zMax);

            setupRenderToTexture(camera, x, y);

            MapSegment& segment = mSegments[std::make_pair(x,y)];
            if (!segment.mFogOfWarImage)
            {
                if (!cellHasValidFog)
                    segment.initFogOfWar();
                else
                {
                    ESM::FogState* fog = cell->getFog();

                    // We are using the same bounds and angle as we were using when the textures were originally made. Segments should come out the same.
                    if (i >= int(fog->mFogTextures.size()))
                    {
                        Log(Debug::Warning) << "Warning: fog texture count mismatch";
                        break;
                    }

                    segment.loadFogOfWar(fog->mFogTextures[i]);
                }
            }
            ++i;
        }
    }
}

void LocalMap::worldToInteriorMapPosition (osg::Vec2f pos, float& nX, float& nY, int& x, int& y)
{
    pos = rotatePoint(pos, osg::Vec2f(mBounds.center().x(), mBounds.center().y()), mAngle);

    osg::Vec2f min(mBounds.xMin(), mBounds.yMin());

    x = static_cast<int>(std::ceil((pos.x() - min.x()) / mMapWorldSize) - 1);
    y = static_cast<int>(std::ceil((pos.y() - min.y()) / mMapWorldSize) - 1);

    nX = (pos.x() - min.x() - mMapWorldSize*x)/mMapWorldSize;
    nY = 1.0f-(pos.y() - min.y() - mMapWorldSize*y)/mMapWorldSize;
}

osg::Vec2f LocalMap::interiorMapToWorldPosition (float nX, float nY, int x, int y)
{
    osg::Vec2f min(mBounds.xMin(), mBounds.yMin());
    osg::Vec2f pos (mMapWorldSize * (nX + x) + min.x(),
                    mMapWorldSize * (1.0f-nY + y) + min.y());

    pos = rotatePoint(pos, osg::Vec2f(mBounds.center().x(), mBounds.center().y()), -mAngle);
    return pos;
}

bool LocalMap::isPositionExplored (float nX, float nY, int x, int y)
{
    const MapSegment& segment = mSegments[std::make_pair(x, y)];
    if (!segment.mFogOfWarImage)
        return false;

    nX = std::max(0.f, std::min(1.f, nX));
    nY = std::max(0.f, std::min(1.f, nY));

    int texU = static_cast<int>((sFogOfWarResolution - 1) * nX);
    int texV = static_cast<int>((sFogOfWarResolution - 1) * nY);

    uint32_t clr = ((const uint32_t*)segment.mFogOfWarImage->data())[texV * sFogOfWarResolution + texU];
    uint8_t alpha = (clr >> 24);
    return alpha < 200;
}

osg::Group* LocalMap::getRoot()
{
    return mRoot;
}

void LocalMap::updatePlayer (const osg::Vec3f& position, const osg::Quat& orientation,
                             float& u, float& v, int& x, int& y, osg::Vec3f& direction)
{
    // retrieve the x,y grid coordinates the player is in
    osg::Vec2f pos(position.x(), position.y());

    if (mInterior)
    {
        worldToInteriorMapPosition(pos, u,v, x,y);

        osg::Quat cameraOrient (mAngle, osg::Vec3(0,0,-1));
        direction = orientation * cameraOrient.inverse() * osg::Vec3f(0,1,0);
    }
    else
    {
        direction = orientation * osg::Vec3f(0,1,0);

        x = static_cast<int>(std::ceil(pos.x() / mMapWorldSize) - 1);
        y = static_cast<int>(std::ceil(pos.y() / mMapWorldSize) - 1);

        // convert from world coordinates to texture UV coordinates
        u = std::abs((pos.x() - (mMapWorldSize*x))/mMapWorldSize);
        v = 1.0f-std::abs((pos.y() - (mMapWorldSize*y))/mMapWorldSize);
    }

    // explore radius (squared)
    const float exploreRadius = 0.17f * (sFogOfWarResolution-1); // explore radius from 0 to sFogOfWarResolution-1
    const float sqrExploreRadius = square(exploreRadius);
    const float exploreRadiusUV = exploreRadius / sFogOfWarResolution; // explore radius from 0 to 1 (UV space)

    // change the affected fog of war textures (in a 3x3 grid around the player)
    for (int mx = -mCellDistance; mx<=mCellDistance; ++mx)
    {
        for (int my = -mCellDistance; my<=mCellDistance; ++my)
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

            int texX = x + mx;
            int texY = y + my*-1;

            MapSegment& segment = mSegments[std::make_pair(texX, texY)];

            if (!segment.mFogOfWarImage || !segment.mMapTexture)
                continue;

            uint32_t* data = (uint32_t*)segment.mFogOfWarImage->data();
            bool changed = false;
            for (int texV = 0; texV<sFogOfWarResolution; ++texV)
            {
                for (int texU = 0; texU<sFogOfWarResolution; ++texU)
                {
                    float sqrDist = square((texU + mx*(sFogOfWarResolution-1)) - u*(sFogOfWarResolution-1))
                            + square((texV + my*(sFogOfWarResolution-1)) - v*(sFogOfWarResolution-1));

                    uint32_t clr = *(uint32_t*)data;
                    uint8_t alpha = (clr >> 24);

                    alpha = std::min( alpha, (uint8_t) (std::max(0.f, std::min(1.f, (sqrDist/sqrExploreRadius)))*255) );
                    uint32_t val = (uint32_t) (alpha << 24);
                    if ( *data != val)
                    {
                        *data = val;
                        changed = true;
                    }

                    ++data;
                }
            }

            if (changed)
            {
                segment.mHasFogState = true;
                segment.mFogOfWarImage->dirty();
            }
        }
    }
}

LocalMap::MapSegment::MapSegment()
    : mHasFogState(false)
{
}

LocalMap::MapSegment::~MapSegment()
{

}

void LocalMap::MapSegment::createFogOfWarTexture()
{
    if (mFogOfWarTexture)
        return;
    mFogOfWarTexture = new osg::Texture2D;
    // TODO: synchronize access? for now, the worst that could happen is the draw thread jumping a frame ahead.
    //mFogOfWarTexture->setDataVariance(osg::Object::DYNAMIC);
    mFogOfWarTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    mFogOfWarTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    mFogOfWarTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    mFogOfWarTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    mFogOfWarTexture->setUnRefImageDataAfterApply(false);
}

void LocalMap::MapSegment::initFogOfWar()
{
    mFogOfWarImage = new osg::Image;
    // Assign a PixelBufferObject for asynchronous transfer of data to the GPU
    mFogOfWarImage->setPixelBufferObject(new osg::PixelBufferObject);
    mFogOfWarImage->allocateImage(sFogOfWarResolution, sFogOfWarResolution, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    assert(mFogOfWarImage->isDataContiguous());
    std::vector<uint32_t> data;
    data.resize(sFogOfWarResolution*sFogOfWarResolution, 0xff000000);

    memcpy(mFogOfWarImage->data(), &data[0], data.size()*4);

    createFogOfWarTexture();
    mFogOfWarTexture->setImage(mFogOfWarImage);
}

void LocalMap::MapSegment::loadFogOfWar(const ESM::FogTexture &esm)
{
    const std::vector<char>& data = esm.mImageData;
    if (data.empty())
    {
        initFogOfWar();
        return;
    }

    // TODO: deprecate tga and use raw data instead

    osgDB::ReaderWriter* readerwriter = osgDB::Registry::instance()->getReaderWriterForExtension("tga");
    if (!readerwriter)
    {
        Log(Debug::Error) << "Error: Unable to load fog, can't find a tga ReaderWriter" ;
        return;
    }

    Files::IMemStream in(&data[0], data.size());

    osgDB::ReaderWriter::ReadResult result = readerwriter->readImage(in);
    if (!result.success())
    {
        Log(Debug::Error) << "Error: Failed to read fog: " << result.message() << " code " << result.status();
        return;
    }

    mFogOfWarImage = result.getImage();
    mFogOfWarImage->flipVertical();
    mFogOfWarImage->dirty();

    createFogOfWarTexture();
    mFogOfWarTexture->setImage(mFogOfWarImage);
    mHasFogState = true;
}

void LocalMap::MapSegment::saveFogOfWar(ESM::FogTexture &fog) const
{
    if (!mFogOfWarImage)
        return;

    std::ostringstream ostream;

    osgDB::ReaderWriter* readerwriter = osgDB::Registry::instance()->getReaderWriterForExtension("tga");
    if (!readerwriter)
    {
        Log(Debug::Error) << "Error: Unable to write fog, can't find a tga ReaderWriter";
        return;
    }

    // extra flips are unfortunate, but required for compatibility with older versions
    mFogOfWarImage->flipVertical();
    osgDB::ReaderWriter::WriteResult result = readerwriter->writeImage(*mFogOfWarImage, ostream);
    if (!result.success())
    {
        Log(Debug::Error) << "Error: Unable to write fog: " << result.message() << " code " << result.status();
        return;
    }
    mFogOfWarImage->flipVertical();

    std::string data = ostream.str();
    fog.mImageData = std::vector<char>(data.begin(), data.end());
}

}
