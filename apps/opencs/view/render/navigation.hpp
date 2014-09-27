#ifndef OPENCS_VIEW_NAVIGATION_H
#define OPENCS_VIEW_NAVIGATION_H

class QPoint;

namespace Ogre
{
    class Camera;
}

namespace CSVRender
{
    class Navigation
    {
            float mFastModeFactor;

        protected:

            float getFactor (bool mouse) const;

        public:

            Navigation();
            virtual ~Navigation();

            void setFastModeFactor (float factor);
            ///< Set currently applying fast mode factor.

            virtual bool activate (Ogre::Camera *camera) = 0;
            ///< \return Update required?

            virtual bool wheelMoved (int delta) = 0;
            ///< \return Update required?

            virtual bool mouseMoved (const QPoint& delta, int mode) = 0;
            ///< \param mode: 0: default mouse key, 1: default mouse key and modifier key 1
            /// \return Update required?

            virtual bool handleMovementKeys (int vertical, int horizontal) = 0;
            ///< \return Update required?

            virtual bool handleRollKeys (int delta) = 0;
            ///< \return Update required?
    };
}

#endif
