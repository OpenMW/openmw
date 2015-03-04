#include "textoverlay.hpp"

#include <OgreCamera.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>

#include <OgreOverlayManager.h>
#include <OgreOverlayContainer.h>
#include <OgreFontManager.h>
#include <OgreTextAreaOverlayElement.h>
#include <OgreEntity.h>
#include <OgreViewport.h>
#include <OgreRoot.h>
#include <OgreHardwarePixelBuffer.h>

namespace CSVRender
{

// Things to do:
// - configurable font size in pixels (automatically calulate everything else from it)
// - configurable texture to use
// - try material script
// - decide whether to use QPaint (http://www.ogre3d.org/tikiwiki/Ogre+overlays+using+Qt)

// http://www.ogre3d.org/tikiwiki/ObjectTextDisplay
// http://www.ogre3d.org/tikiwiki/MovableTextOverlay
// http://www.ogre3d.org/tikiwiki/Creating+dynamic+textures
// http://www.ogre3d.org/tikiwiki/ManualObject
TextOverlay::TextOverlay(const Ogre::MovableObject* obj, const Ogre::Camera* camera, const  Ogre::String& id)
    : mOverlay(0), mCaption(""), mDesc(""), mEnabled(true), mCamera(camera), mObj(obj), mId(id)
    , mOnScreen(false) , mInstance(0), mFontHeight(16) // FIXME: make font height configurable
{
    if(id == "" || !camera || !obj)
        throw std::runtime_error("TextOverlay could not be created.");

    // setup font
    Ogre::FontManager &fontMgr = Ogre::FontManager::getSingleton();
    if (fontMgr.resourceExists("DejaVuLGC"))
        mFont = fontMgr.getByName("DejaVuLGC","General");
    else
    {
        mFont = fontMgr.create("DejaVuLGC","General");
        mFont->setType(Ogre::FT_TRUETYPE);
        mFont->setSource("DejaVuLGCSansMono.ttf");
        mFont->setTrueTypeSize(mFontHeight);
        mFont->setTrueTypeResolution(96);
    }
    if(!mFont.isNull())
        mFont->load();
    else
        throw std::runtime_error("TextOverlay font not loaded.");

    // setup overlay
    Ogre::OverlayManager &overlayMgr = Ogre::OverlayManager::getSingleton();
    mOverlay = overlayMgr.getByName("CellIDPanel"+mId+Ogre::StringConverter::toString(mInstance));
    // FIXME: this logic is badly broken as it is possible to delete an earlier instance
    while(mOverlay != NULL)
    {
        mInstance++;
        mOverlay = overlayMgr.getByName("CellIDPanel"+mId+Ogre::StringConverter::toString(mInstance));
    }
    mOverlay = overlayMgr.create("CellIDPanel"+mId+Ogre::StringConverter::toString(mInstance));

    // create texture
    Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().getByName("DynamicTransBlue");
    if(texture.isNull())
    {
        texture = Ogre::TextureManager::getSingleton().createManual(
            "DynamicTransBlue", // name
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D,  // type
            8, 8,               // width & height
            0,                  // number of mipmaps
            Ogre::PF_BYTE_BGRA, // pixel format
            Ogre::TU_DEFAULT);  // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
                                // textures updated very often (e.g. each frame)

        Ogre::HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
        pixelBuffer->lock(Ogre::HardwareBuffer::HBL_NORMAL);
        const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();

        Ogre::uint8* pDest = static_cast<Ogre::uint8*>(pixelBox.data);

        // Fill in some pixel data. This will give a semi-transparent blue,
        // but this is of course dependent on the chosen pixel format.
        for (size_t j = 0; j < 8; j++)
        {
            for(size_t i = 0; i < 8; i++)
            {
                *pDest++ = 255; // B
                *pDest++ =   0; // G
                *pDest++ =   0; // R
                *pDest++ =  63; // A
            }

            pDest += pixelBox.getRowSkip() * Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
        }
        pixelBuffer->unlock();
    }

    // setup material for containers
    Ogre::MaterialPtr mQuadMaterial = Ogre::MaterialManager::getSingleton().getByName(
                "TransOverlayMaterial");
    if(mQuadMaterial.isNull())
    {
        Ogre::MaterialPtr mQuadMaterial = Ogre::MaterialManager::getSingleton().create(
                    "TransOverlayMaterial",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true );
        Ogre::Pass *pass = mQuadMaterial->getTechnique( 0 )->getPass( 0 );
        pass->setLightingEnabled( false );
        pass->setDepthWriteEnabled( false );
        pass->setSceneBlending( Ogre::SBT_TRANSPARENT_ALPHA );

        Ogre::TextureUnitState *tex = pass->createTextureUnitState("MyCustomState", 0);
        tex->setTextureName("DynamicTransBlue");
        tex->setTextureFiltering( Ogre::TFO_ANISOTROPIC );
        mQuadMaterial->load();
    }

    mContainer = static_cast<Ogre::OverlayContainer*>(overlayMgr.createOverlayElement(
                "Panel", "container"+mId +"#"+Ogre::StringConverter::toString(mInstance)));
    mContainer->setMaterialName("TransOverlayMaterial");
    mOverlay->add2D(mContainer);

    // setup text area overlay element
    mElement = static_cast<Ogre::TextAreaOverlayElement*>(overlayMgr.createOverlayElement(
                "TextArea", "text"+mId +"#"+Ogre::StringConverter::toString(mInstance)));
    mElement->setMetricsMode(Ogre::GMM_RELATIVE);
    mElement->setDimensions(1.0, 1.0);
    mElement->setMetricsMode(Ogre::GMM_PIXELS);
    mElement->setPosition(2*fontHeight()/3, 1.3*fontHeight()/3); // 1.3 & 2 = fudge factor

    mElement->setFontName("DejaVuLGC");
    mElement->setCharHeight(fontHeight()); // NOTE: seems that this is required as well as font->setTrueTypeSize()
    mElement->setHorizontalAlignment(Ogre::GHA_LEFT);
    //mElement->setColour(Ogre::ColourValue(1.0, 1.0, 1.0)); // R, G, B
    mElement->setColour(Ogre::ColourValue(1.0, 1.0, 0)); // yellow

    mContainer->addChild(mElement);
    mOverlay->show();
}

void TextOverlay::getScreenCoordinates(const Ogre::Vector3& position, Ogre::Real& x, Ogre::Real& y)
{
    Ogre::Vector3 hcsPosition = mCamera->getProjectionMatrix() * (mCamera->getViewMatrix() * position);

    x = 1.0f - ((hcsPosition.x * 0.5f) + 0.5f); // 0 <= x <= 1 // left := 0,right := 1
    y = ((hcsPosition.y * 0.5f) + 0.5f); // 0 <= y <= 1 // bottom := 0,top := 1
}

void TextOverlay::getMinMaxEdgesOfAABBIn2D(float& MinX, float& MinY, float& MaxX, float& MaxY,
                                           bool top)
{
    MinX = 0, MinY = 0, MaxX = 0, MaxY = 0;
    float X[4]; // the 2D dots of the AABB in screencoordinates
    float Y[4];

    if(!mObj->isInScene())
        return;

    const Ogre::AxisAlignedBox &AABB = mObj->getWorldBoundingBox(true); // the AABB of the target
    Ogre::Vector3 cornersOfAABB[4];
    if(top)
    {
        cornersOfAABB[0] = AABB.getCorner(Ogre::AxisAlignedBox::FAR_LEFT_TOP);
        cornersOfAABB[1] = AABB.getCorner(Ogre::AxisAlignedBox::FAR_RIGHT_TOP);
        cornersOfAABB[2] = AABB.getCorner(Ogre::AxisAlignedBox::NEAR_LEFT_TOP);
        cornersOfAABB[3] = AABB.getCorner(Ogre::AxisAlignedBox::NEAR_RIGHT_TOP);
    }
    else
    {
        cornersOfAABB[0] = AABB.getCorner(Ogre::AxisAlignedBox::FAR_LEFT_BOTTOM);
        cornersOfAABB[1] = AABB.getCorner(Ogre::AxisAlignedBox::FAR_RIGHT_BOTTOM);
        cornersOfAABB[2] = AABB.getCorner(Ogre::AxisAlignedBox::NEAR_LEFT_BOTTOM);
        cornersOfAABB[3] = AABB.getCorner(Ogre::AxisAlignedBox::NEAR_RIGHT_BOTTOM);
    }

    //The normal vector of the plane. This points directly infront of the camera.
    Ogre::Vector3 cameraPlainNormal = mCamera->getDerivedOrientation().zAxis();

    //the plane that devides the space before and behind the camera.
    Ogre::Plane CameraPlane = Ogre::Plane(cameraPlainNormal, mCamera->getDerivedPosition());

    for (int i = 0; i < 4; i++)
    {
        X[i] = 0;
        Y[i] = 0;

        getScreenCoordinates(cornersOfAABB[i],X[i],Y[i]); // transfor into 2d dots

        if (CameraPlane.getSide(cornersOfAABB[i]) == Ogre::Plane::NEGATIVE_SIDE)
        {
            if (i == 0) // accept the first set of values, no matter how bad it might be.
            {
                MinX = X[i];
                MinY = Y[i];
                MaxX = X[i];
                MaxY = Y[i];
            }
            else // now compare if you get "better" values
            {
                if (MinX > X[i]) MinX = X[i];
                if (MinY > Y[i]) MinY = Y[i];
                if (MaxX < X[i]) MaxX = X[i];
                if (MaxY < Y[i]) MaxY = Y[i];
            }
        }
        else
        {
            MinX = 0;
            MinY = 0;
            MaxX = 0;
            MaxY = 0;
            break;
        }
    }
}

TextOverlay::~TextOverlay()
{
    Ogre::OverlayManager::OverlayMapIterator iter = Ogre::OverlayManager::getSingleton().getOverlayIterator();
    if(!iter.hasMoreElements())
        mOverlay->hide();

    Ogre::OverlayManager *overlayMgr = Ogre::OverlayManager::getSingletonPtr();
    mContainer->removeChild("text"+mId+"#"+Ogre::StringConverter::toString(mInstance));
    mOverlay->remove2D(mContainer);

    if(!iter.hasMoreElements())
        overlayMgr->destroy(mOverlay);
}

void TextOverlay::show(bool show)
{
    if(show && mOnScreen)
        mContainer->show();
    else
        mContainer->hide();
}

void TextOverlay::enable(bool enable)
{
    if(enable == mOverlay->isVisible())
        return;

    mEnabled = enable;
    if(enable)
        mOverlay->show();
    else
        mOverlay->hide();
}

bool TextOverlay::isEnabled()
{
    return mEnabled;
}

void TextOverlay::setCaption(const Ogre::String& text)
{
    if(mCaption == text)
        return;

    mCaption = text;
    mElement->setCaption(text);
}

void TextOverlay::setDesc(const Ogre::String& text)
{
    if(mDesc == text)
        return;

    mDesc = text;
    mElement->setCaption(mCaption + ((text == "") ? "" : ("\n" + text)));
}

Ogre::FontPtr TextOverlay::getFont()
{
    return mFont;
}

int TextOverlay::textWidth()
{
    float captionWidth = 0;
    float descWidth = 0;

    for(Ogre::String::const_iterator i = mCaption.begin(); i < mCaption.end(); ++i)
    {
        if(*i == 0x0020)
            captionWidth += getFont()->getGlyphAspectRatio(0x0030);
        else
            captionWidth += getFont()->getGlyphAspectRatio(*i);
    }

    for(Ogre::String::const_iterator i = mDesc.begin(); i < mDesc.end(); ++i)
    {
        if(*i == 0x0020)
            descWidth += getFont()->getGlyphAspectRatio(0x0030);
        else
            descWidth += getFont()->getGlyphAspectRatio(*i);
    }

    captionWidth *= fontHeight();
    descWidth *= fontHeight();

    return (int) std::max(captionWidth, descWidth);
}

int TextOverlay::fontHeight()
{
    return mFontHeight;
}

void TextOverlay::update()
{
    float min_x, max_x, min_y, max_y;
    getMinMaxEdgesOfAABBIn2D(min_x, min_y, max_x, max_y, false);

    if ((min_x>0.0) && (max_x<1.0) && (min_y>0.0) && (max_y<1.0))
    {
        mOnScreen = true;
        mContainer->show();
    }
    else
    {
        mOnScreen = false;
        mContainer->hide();
        return;
    }

    getMinMaxEdgesOfAABBIn2D(min_x, min_y, max_x, max_y);

    Ogre::OverlayManager &overlayMgr = Ogre::OverlayManager::getSingleton();
    float viewportWidth = std::max(overlayMgr.getViewportWidth(), 1); // zero at the start
    float viewportHeight = std::max(overlayMgr.getViewportHeight(), 1); // zero at the start

    int width =  fontHeight()*2/3 + textWidth() + fontHeight()*2/3; // add margins
    int height = fontHeight()/3 + fontHeight() + fontHeight()/3;
    if(mDesc != "")
        height = fontHeight()/3 + 2*fontHeight() + fontHeight()/3;

    float relTextWidth = width / viewportWidth;
    float relTextHeight = height / viewportHeight;

    float posX = 1 - (min_x + max_x + relTextWidth)/2;
    float posY = 1 - max_y - (relTextHeight-fontHeight()/3/viewportHeight);

    mContainer->setMetricsMode(Ogre::GMM_RELATIVE);
    mContainer->setPosition(posX, posY);
    mContainer->setDimensions(relTextWidth, relTextHeight);

    mPos = QRect(posX*viewportWidth, posY*viewportHeight, width, height);
}

QRect TextOverlay::container()
{
    return mPos;
}

}
