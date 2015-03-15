#include "selectionbuffer.hpp"

#include <OgreHardwarePixelBuffer.h>
#include <OgreRenderTexture.h>
#include <OgreSubEntity.h>
#include <OgreEntity.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>
#include <OgreViewport.h>

#include <stdexcept>

#include <openengine/misc/rng.hpp>

#include <extern/shiny/Main/Factory.hpp>

namespace OEngine
{
namespace Render
{

    SelectionBuffer::SelectionBuffer(Ogre::Camera *camera, int sizeX, int sizeY, int visibilityFlags)
        : mCamera(camera)
        , mVisibilityFlags(visibilityFlags)
    {
        mTexture = Ogre::TextureManager::getSingleton().createManual("SelectionBuffer",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, sizeX, sizeY, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET, this);

        setupRenderTarget();

        mCurrentColour = Ogre::ColourValue(0.3f, 0.3f, 0.3f);
    }

    void SelectionBuffer::setupRenderTarget()
    {
        mRenderTarget = mTexture->getBuffer()->getRenderTarget();
        mRenderTarget->removeAllViewports();
        Ogre::Viewport* vp = mRenderTarget->addViewport(mCamera);
        vp->setOverlaysEnabled(false);
        vp->setBackgroundColour(Ogre::ColourValue(0, 0, 0, 0));
        vp->setShadowsEnabled(false);
        vp->setMaterialScheme("selectionbuffer");
        if (mVisibilityFlags != 0)
            vp->setVisibilityMask (mVisibilityFlags);
        mRenderTarget->setActive(true);
        mRenderTarget->setAutoUpdated (false);
    }

    void SelectionBuffer::loadResource(Ogre::Resource *resource)
    {
        Ogre::Texture* tex = dynamic_cast<Ogre::Texture*>(resource);
        if (!tex)
            return;

        tex->createInternalResources();

        mRenderTarget = NULL;

        // Don't need to re-render texture, because we have a copy in system memory (mBuffer)
    }

    SelectionBuffer::~SelectionBuffer()
    {
        Ogre::TextureManager::getSingleton ().remove("SelectionBuffer");
    }

    void SelectionBuffer::update ()
    {
        Ogre::MaterialManager::getSingleton ().addListener (this);

        mTexture->load();
        if (mRenderTarget == NULL)
            setupRenderTarget();

        mRenderTarget->update();

        Ogre::MaterialManager::getSingleton ().removeListener (this);

        mTexture->convertToImage(mBuffer);
    }

    int SelectionBuffer::getSelected(int xPos, int yPos)
    {
        Ogre::ColourValue clr = mBuffer.getColourAt (xPos, yPos, 0);
        clr.a = 1;
        if (mColourMap.find(clr) != mColourMap.end())
            return mColourMap[clr];
        else
            return -1; // nothing selected
    }

    Ogre::Technique* SelectionBuffer::handleSchemeNotFound (
        unsigned short schemeIndex, const Ogre::String &schemeName, Ogre::Material *originalMaterial,
        unsigned short lodIndex, const Ogre::Renderable *rend)
    {
        if (schemeName == "selectionbuffer")
        {
            sh::Factory::getInstance ()._ensureMaterial ("SelectionColour", "Default");

            Ogre::MaterialPtr m = Ogre::MaterialManager::getSingleton ().getByName("SelectionColour");


            if(typeid(*rend) == typeid(Ogre::SubEntity))
            {
                const Ogre::SubEntity *subEntity = static_cast<const Ogre::SubEntity *>(rend);
                int id = Ogre::any_cast<int>(subEntity->getParent ()->getUserObjectBindings().getUserAny());
                bool found = false;
                Ogre::ColourValue colour;
                for (std::map<Ogre::ColourValue, int, cmp_ColourValue>::iterator it = mColourMap.begin(); it != mColourMap.end(); ++it)
                {
                    if (it->second == id)
                    {
                        found = true;
                        colour = it->first;
                    }
                }


                if (!found)
                {
                    getNextColour();
                    const_cast<Ogre::SubEntity *>(subEntity)->setCustomParameter(1, Ogre::Vector4(mCurrentColour.r, mCurrentColour.g, mCurrentColour.b, 1.0));
                    mColourMap[mCurrentColour] = id;
                }
                else
                {
                    const_cast<Ogre::SubEntity *>(subEntity)->setCustomParameter(1, Ogre::Vector4(colour.r, colour.g, colour.b, 1.0));
                }

                assert(m->getTechnique(1));
                return m->getTechnique(1);
            }
            else
            {
                m = Ogre::MaterialManager::getSingleton().getByName("NullMaterial");
                if(m.isNull())
                {
                    m = Ogre::MaterialManager::getSingleton().create("NullMaterial", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                    m->getTechnique(0)->getPass(0)->setDepthCheckEnabled(true);
                    m->getTechnique(0)->getPass(0)->setDepthFunction(Ogre::CMPF_ALWAYS_FAIL);
                }
                return m->getTechnique(0);
            }
        }
        return NULL;
    }

    void SelectionBuffer::getNextColour ()
    {
        Ogre::ARGB color = static_cast<Ogre::ARGB>(OEngine::Misc::Rng::rollClosedProbability() * std::numeric_limits<Ogre::uint32>::max());

        if (mCurrentColour.getAsARGB () == color)
        {
            getNextColour();
            return;
        }

        mCurrentColour.setAsARGB(color);
        mCurrentColour.a = 1;
    }


}
}
