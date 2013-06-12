#include "imagerotate.hpp"

#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <OgreImage.h>
#include <OgreTexture.h>
#include <OgreRenderTarget.h>
#include <OgreCamera.h>
#include <OgreTextureUnitState.h>
#include <OgreHardwarePixelBuffer.h>

using namespace Ogre;
using namespace OEngine::Render;

void ImageRotate::rotate(const std::string& sourceImage, const std::string& destImage, const float angle)
{
    Root* root = Ogre::Root::getSingletonPtr();

    std::string destImageRot = std::string(destImage) + std::string("_rot");

    SceneManager* sceneMgr = root->createSceneManager(ST_GENERIC);
    Camera* camera = sceneMgr->createCamera("ImageRotateCamera");

    MaterialPtr material = MaterialManager::getSingleton().create("ImageRotateMaterial", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    material->getTechnique(0)->getPass(0)->setLightingEnabled(false);
    material->getTechnique(0)->getPass(0)->setDepthCheckEnabled(false);
    TextureUnitState* tus = material->getTechnique(0)->getPass(0)->createTextureUnitState(sourceImage);
    Degree deg(angle);
    tus->setTextureRotate(Radian(deg.valueRadians()));
    tus->setTextureAddressingMode(TextureUnitState::TAM_BORDER);
    tus->setTextureBorderColour(ColourValue(0, 0, 0, 0));

    Rectangle2D* rect = new Rectangle2D(true);
    rect->setCorners(-1.0, 1.0, 1.0, -1.0);
    rect->setMaterial("ImageRotateMaterial");
    // Render the background before everything else
    rect->setRenderQueueGroup(RENDER_QUEUE_BACKGROUND);

    // Use infinite AAB to always stay visible
    AxisAlignedBox aabInf;
    aabInf.setInfinite();
    rect->setBoundingBox(aabInf);

    // Attach background to the scene
    SceneNode* node = sceneMgr->getRootSceneNode()->createChildSceneNode();
    node->attachObject(rect);

    // retrieve image width and height
    TexturePtr sourceTexture = TextureManager::getSingleton().getByName(sourceImage);
    unsigned int width = sourceTexture->getWidth();
    unsigned int height = sourceTexture->getHeight();

    TexturePtr destTextureRot = TextureManager::getSingleton().createManual(
                    destImageRot,
                    ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    TEX_TYPE_2D,
                    width, height,
                    0,
                    PF_FLOAT16_RGBA,
                    TU_RENDERTARGET);

    RenderTarget* rtt = destTextureRot->getBuffer()->getRenderTarget();
    rtt->setAutoUpdated(false);
    Viewport* vp = rtt->addViewport(camera);
    vp->setOverlaysEnabled(false);
    vp->setShadowsEnabled(false);
    vp->setBackgroundColour(ColourValue(0,0,0,0));

    rtt->update();

    //copy the rotated image to a static texture
    TexturePtr destTexture = TextureManager::getSingleton().createManual(
            destImage,
            ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            TEX_TYPE_2D,
            width, height,
            0,
            PF_FLOAT16_RGBA,
            Ogre::TU_STATIC);

    destTexture->getBuffer()->blit(destTextureRot->getBuffer());

    // remove all the junk we've created
    TextureManager::getSingleton().remove(destImageRot);
    MaterialManager::getSingleton().remove("ImageRotateMaterial");
    root->destroySceneManager(sceneMgr);
    delete rect;
}
