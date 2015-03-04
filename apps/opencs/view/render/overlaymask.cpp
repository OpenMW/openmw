#include "overlaymask.hpp"

#include <OgreOverlayManager.h>
#include <OgreOverlayContainer.h>

#include "textoverlay.hpp"
#include "../../model/world/cellcoordinates.hpp"

namespace CSVRender
{

// ideas from http://www.ogre3d.org/forums/viewtopic.php?f=5&t=44828#p486334
OverlayMask::OverlayMask(std::map<CSMWorld::CellCoordinates, TextOverlay *> &overlays, Ogre::Viewport* viewport)
    : mTextOverlays(overlays), mViewport(viewport)
{
}

OverlayMask::~OverlayMask()
{
}

void OverlayMask::setViewport(Ogre::Viewport *viewport)
{
    mViewport = viewport;
}

void OverlayMask::preViewportUpdate(const Ogre::RenderTargetViewportEvent &event)
{
    if(event.source == mViewport)
    {
        Ogre::OverlayManager &overlayMgr = Ogre::OverlayManager::getSingleton();
        for(Ogre::OverlayManager::OverlayMapIterator iter = overlayMgr.getOverlayIterator();
                iter.hasMoreElements();)
        {
            Ogre::Overlay* item = iter.getNext();
            for(Ogre::Overlay::Overlay2DElementsIterator it = item->get2DElementsIterator();
                    it.hasMoreElements();)
            {
                Ogre::OverlayContainer* container = it.getNext();
                if(container) container->hide();
            }
        }

        std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator it = mTextOverlays.begin();
        for(; it != mTextOverlays.end(); ++it)
        {
            it->second->show(true);
        }
    }
}

}
