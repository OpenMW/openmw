#ifndef OPENCS_VIEW_TEXTOVERLAY_H
#define OPENCS_VIEW_TEXTOVERLAY_H

#include <QRect>

#include <OgreString.h>
#include <OgreFont.h>

namespace Ogre
{
    class MovableObject;
    class Camera;
    class Font;
    class Overlay;
    class OverlayContainer;
    class TextAreaOverlayElement;
}

namespace CSVRender
{

    class TextOverlay
    {
            Ogre::Overlay* mOverlay;
            Ogre::OverlayContainer* mContainer;
            Ogre::TextAreaOverlayElement* mElement;
            Ogre::String mCaption;
            Ogre::String mDesc;

            const Ogre::MovableObject* mObj;
            const Ogre::Camera* mCamera;
            Ogre::FontPtr mFont;
            int mFontHeight; // in pixels
            Ogre::String mId;
            QRect mPos;

            bool mEnabled;
            bool mOnScreen;
            int mInstance;

            Ogre::FontPtr getFont();
            int textWidth();
            int fontHeight();
            void getScreenCoordinates(const Ogre::Vector3& position, Ogre::Real& x, Ogre::Real& y);
            void getMinMaxEdgesOfAABBIn2D(float& MinX, float& MinY, float& MaxX, float& MaxY,
                                          bool top = true);

        public:

            TextOverlay(const Ogre::MovableObject* obj, const Ogre::Camera* camera, const Ogre::String &id);
            virtual ~TextOverlay();

            void enable(bool enable); // controlled from scene widget toolbar visibility mask
            void show(bool show);  // for updating from render target listener
            bool isEnabled();
            void setCaption(const Ogre::String& text);
            void setDesc(const Ogre::String& text);
            void update();
            QRect container(); // for detection of mouse click on the overlay
            Ogre::String getCaption() { return mCaption; } // FIXME: debug
            Ogre::String getDesc() { return mDesc; }
    };

}

#endif // OPENCS_VIEW_TEXTOVERLAY_H
