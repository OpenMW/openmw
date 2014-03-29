#ifndef OPENCS_VIEW_NAVIGATIONORBIT_H
#define OPENCS_VIEW_NAVIGATIONORBIT_H

#include "navigation.hpp"

#include <OgreVector3.h>

namespace CSVRender
{
    /// \brief Orbiting camera controls
    class NavigationOrbit : public Navigation
    {
            Ogre::Camera *mCamera;
            Ogre::Vector3 mCentre;
            int mDistance;

            void rotateCamera (const Ogre::Vector3& diff);
            ///< Rotate camera around centre.

        public:

            NavigationOrbit();

            virtual bool activate (Ogre::Camera *camera);
            ///< \return Update required?

            virtual bool wheelMoved (int delta);
            ///< \return Update required?

            virtual bool mouseMoved (const QPoint& delta, int mode);
            ///< \param mode: 0: default mouse key, 1: default mouse key and modifier key 1
            /// \return Update required?

            virtual bool handleMovementKeys (int vertical, int horizontal);
            ///< \return Update required?

            virtual bool handleRollKeys (int delta);
            ///< \return Update required?
    };
}

#endif
