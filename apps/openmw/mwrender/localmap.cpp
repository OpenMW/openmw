#include "localmap.hpp"
#include "renderingmanager.hpp"

#include <OgreOverlayManager.h>
#include <OgreMaterialManager.h>

#include <boost/filesystem.hpp>

using namespace MWRender;
using namespace Ogre;

#define CACHE_EXTENSION ".jpg"

#define MAP_RESOLUTION 1024 // 1024*1024 pixels for a 8192*8192 area in world units

LocalMap::LocalMap(OEngine::Render::OgreRenderer* rend)
{
    mRendering = rend;

    mCellCamera = mRendering->getScene()->createCamera("CellCamera");
    mCellCamera->setProjectionType(PT_ORTHOGRAPHIC);
    // look down -y
    const float sqrt0pt5 = 0.707106781;
    mCellCamera->setOrientation(Quaternion(sqrt0pt5, -sqrt0pt5, 0, 0));

    // Debug overlay to view the maps
    /*
    render(0, 0, 10000, 10000, 8192, 8192, "Cell_0_0");

    MaterialPtr mat = MaterialManager::getSingleton().create("testMaterial", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    mat->getTechnique(0)->getPass(0)->createTextureUnitState("Cell_0_0");
    
    OverlayManager& ovm = OverlayManager::getSingleton();

    Overlay* mOverlay = ovm.create( "testOverlay" );
    
    OverlayContainer* overlay_panel;
    overlay_panel = (OverlayContainer*)ovm.createOverlayElement("Panel", "testPanel");
    
    overlay_panel->_setPosition(0, 0);
    overlay_panel->_setDimensions(0.5, 0.5);
    
    overlay_panel->setMaterialName( "testMaterial" );
    overlay_panel->show();
    mOverlay->add2D(overlay_panel);
    mOverlay->show();
    */
}

void LocalMap::requestMap(MWWorld::Ptr::CellStore* cell)
{
    std::string name = "Cell_" + StringConverter::toString(cell->cell->data.gridX)
                        + "_" + StringConverter::toString(cell->cell->data.gridY);

    const int x = cell->cell->data.gridX;
    const int y = cell->cell->data.gridY;
    
    render((x+0.5)*8192, (-y-0.5)*8192, -10000, 10000, 8192, 8192, name);
}

void LocalMap::requestMap(MWWorld::Ptr::CellStore* cell,
                            AxisAlignedBox bounds)
{
    Vector2 z(bounds.getMaximum().y, bounds.getMinimum().y);
    Vector2 min(bounds.getMinimum().x, bounds.getMinimum().z);
    Vector2 max(bounds.getMaximum().x, bounds.getMaximum().z);

    /// \todo why is this workaround needed?
    min *= 1.3;
    max *= 1.3;
    
    Vector2 length = max-min;
    Vector2 center(bounds.getCenter().x, bounds.getCenter().z);

    // divide into 8192*8192 segments
    const int segsX = std::ceil( length.x / 8192 );
    const int segsY = std::ceil( length.y / 8192 );

    for (int x=0; x<segsX; ++x)
    {
        for (int y=0; y<segsY; ++y)
        {
            Vector2 start = min + Vector2(8192*x,8192*y);
            Vector2 newcenter = start + 4096;

            render(newcenter.x, newcenter.y, z.y, z.x, 8192, 8192,
                cell->cell->name + "_" + StringConverter::toString(x) + "_" + StringConverter::toString(y));
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
    
    mCellCamera->setPosition(Vector3(x, zhigh, y));
    mCellCamera->setFarClipDistance( (zhigh-zlow) * 1.1 );

    mCellCamera->setOrthoWindow(xw, yw);

    TexturePtr tex;
    // try loading from memory
    tex = TextureManager::getSingleton().getByName(texture);
    if (tex.isNull())
    {
        // try loading from disk
        //if (boost::filesystem::exists(texture+CACHE_EXTENSION))
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
                            xw*MAP_RESOLUTION/8192, yw*MAP_RESOLUTION/8192, 
                            0,
                            PF_R8G8B8,
                            TU_RENDERTARGET);

            RenderTarget* rtt = tex->getBuffer()->getRenderTarget();
            rtt->setAutoUpdated(false);
            Viewport* vp = rtt->addViewport(mCellCamera);
            vp->setOverlaysEnabled(false);
            vp->setShadowsEnabled(false);
            vp->setBackgroundColour(ColourValue(0, 0, 0));
            //vp->setVisibilityMask( ... );

            rtt->update();

            /// \todo
            // save to cache for next time
            //rtt->writeContentsToFile("./" + texture + CACHE_EXTENSION);
        }
    }


    /*
    if (!MaterialManager::getSingleton().getByName("testMaterial").isNull())
    {
        MaterialPtr mat = MaterialManager::getSingleton().getByName("testMaterial");
        mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(texture);
    }
    */

    // re-enable fog
    mRendering->getScene()->setFog(FOG_LINEAR, clr, 0, fStart, fEnd);
}
