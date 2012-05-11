#include "atlas.hpp"

#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <OgreImage.h>
#include <OgreTexture.h>
#include <OgreRenderTarget.h>
#include <OgreCamera.h>
#include <OgreTextureUnitState.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreConfigFile.h>
#include <OgreStringConverter.h>

using namespace Ogre;
using namespace OEngine::Render;

void Atlas::createFromFile (const std::string& filename, const std::string& textureName, const std::string& texturePrefix)
{
    ConfigFile file;
    file.load(filename, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, "\t:=", true);

    Root* root = Ogre::Root::getSingletonPtr();

    SceneManager* sceneMgr = root->createSceneManager(ST_GENERIC);
    Camera* camera = sceneMgr->createCamera("AtlasCamera");

    int width = StringConverter::parseInt(file.getSetting("size_x", "settings"));
    int height = StringConverter::parseInt(file.getSetting("size_y", "settings"));

    std::vector<Rectangle2D*> rectangles;
    int i = 0;

    ConfigFile::SectionIterator seci = file.getSectionIterator();
    while (seci.hasMoreElements())
    {
        Ogre::String sectionName = seci.peekNextKey();
        seci.getNext();

        if (sectionName == "settings" || sectionName == "")
            continue;

        MaterialPtr material = MaterialManager::getSingleton().create("AtlasMaterial" + StringConverter::toString(i), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        material->getTechnique(0)->getPass(0)->setLightingEnabled(false);
        material->getTechnique(0)->getPass(0)->setDepthCheckEnabled(false);
        TextureUnitState* tus = material->getTechnique(0)->getPass(0)->createTextureUnitState(texturePrefix + sectionName);
        tus->setTextureBorderColour(ColourValue(0, 0, 0, 0));

        Rectangle2D* rect = new Rectangle2D(true);
        rect->setMaterial("AtlasMaterial" + StringConverter::toString(i));
        rect->setRenderQueueGroup(RENDER_QUEUE_BACKGROUND);

        int x = StringConverter::parseInt(file.getSetting("x", sectionName));
        int y = StringConverter::parseInt(file.getSetting("y", sectionName));

        TexturePtr texture = TextureManager::getSingleton().getByName(texturePrefix + sectionName);
        if (texture.isNull())
        {
            std::cerr << "OEngine::Render::Atlas: Can't find texture " << texturePrefix + sectionName << ", skipping..." << std::endl;
            continue;
        }
        int textureWidth = texture->getWidth();
        int textureHeight = texture->getHeight();

        float left = x/float(width) * 2 - 1;
        float top = (1-(y/float(height))) * 2 - 1;
        float right = ((x+textureWidth))/float(width) * 2 - 1;
        float bottom = (1-((y+textureHeight)/float(height))) * 2 - 1;
        rect->setCorners(left, top, right, bottom);

        // Use infinite AAB to always stay visible
        AxisAlignedBox aabInf;
        aabInf.setInfinite();
        rect->setBoundingBox(aabInf);

        // Attach background to the scene
        SceneNode* node = sceneMgr->getRootSceneNode()->createChildSceneNode();
        node->attachObject(rect);

        rectangles.push_back(rect);
        ++i;
    }

    TexturePtr destTexture = TextureManager::getSingleton().createManual(
                    textureName,
                    ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    TEX_TYPE_2D,
                    width, height,
                    0,
                    PF_FLOAT16_RGBA,
                    TU_RENDERTARGET);

    RenderTarget* rtt = destTexture->getBuffer()->getRenderTarget();
    rtt->setAutoUpdated(false);
    Viewport* vp = rtt->addViewport(camera);
    vp->setOverlaysEnabled(false);
    vp->setShadowsEnabled(false);
    vp->setBackgroundColour(ColourValue(0,0,0,0));

    rtt->update();

    // remove all the junk we've created
    for (std::vector<Rectangle2D*>::iterator it=rectangles.begin();
        it!=rectangles.end(); ++it)
    {
        delete (*it);
    }
    while (i > 0)
    {
        MaterialManager::getSingleton().remove("AtlasMaterial" + StringConverter::toString(i-1));
        --i;
    }
    root->destroySceneManager(sceneMgr);
}
