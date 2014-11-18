#ifndef OPENCS_VIEW_OVERLAYMASK_H
#define OPENCS_VIEW_OVERLAYMASK_H

#include <OgreRenderTargetListener.h>

namespace Ogre
{
    class Viewport;
    class RendertargetViewportEvent;
}

namespace CSMWorld
{
    class CellCoordinates;
}

namespace CSVRender
{
    class TextOverlay;

    class OverlayMask : public Ogre::RenderTargetListener
    {

        std::map<CSMWorld::CellCoordinates, TextOverlay *> &mTextOverlays;
        Ogre::Viewport* mViewport;

    public:

        OverlayMask(std::map<CSMWorld::CellCoordinates, TextOverlay *> &overlays,
                    Ogre::Viewport* viewport);

        virtual ~OverlayMask();

        void setViewport(Ogre::Viewport *viewport);

    protected:

        virtual void preViewportUpdate(const Ogre::RenderTargetViewportEvent &event);
    };
}

#endif // OPENCS_VIEW_OVERLAYMASK_H
