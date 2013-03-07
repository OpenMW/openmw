#include "selectionbuffer.hpp"

#include <OgreHardwarePixelBuffer.h>
#include <OgreRenderTexture.h>
#include <OgreSubEntity.h>
#include <OgreEntity.h>
#include <stdexcept>

#include <extern/shiny/Main/Factory.hpp>

namespace OEngine
{
namespace Render
{

    SelectionBuffer::SelectionBuffer(Ogre::Camera *camera, int sizeX, int sizeY, int visibilityFlags)
    {
        mTexture = Ogre::TextureManager::getSingleton().createManual("SelectionBuffer",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, sizeX, sizeY, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);

        mRenderTarget = mTexture->getBuffer()->getRenderTarget();
        Ogre::Viewport* vp = mRenderTarget->addViewport(camera);
        vp->setOverlaysEnabled(false);
        vp->setBackgroundColour(Ogre::ColourValue(0, 0, 0, 0));
        vp->setShadowsEnabled(false);
        vp->setMaterialScheme("selectionbuffer");
        if (visibilityFlags != 0)
            vp->setVisibilityMask (visibilityFlags);
        mRenderTarget->setActive(true);
        mRenderTarget->setAutoUpdated (false);

        mCurrentColour = Ogre::ColourValue(0.3, 0.3, 0.3);
    }

    SelectionBuffer::~SelectionBuffer()
    {
        Ogre::TextureManager::getSingleton ().remove("SelectionBuffer");
    }

    void SelectionBuffer::update ()
    {
        Ogre::MaterialManager::getSingleton ().addListener (this);

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
                throw std::runtime_error("selectionbuffer only works with entities");
        }
        return NULL;
    }

    void SelectionBuffer::getNextColour ()
    {
        Ogre::ARGB color = (float(rand()) / float(RAND_MAX)) * std::numeric_limits<Ogre::uint32>::max();

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
