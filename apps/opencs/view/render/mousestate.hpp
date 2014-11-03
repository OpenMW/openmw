#ifndef OPENCS_VIEW_MOUSESTATE_H
#define OPENCS_VIEW_MOUSESTATE_H

#include <map>
#include <QPoint>
#include <OgreVector3.h>

class QElapsedTimer;
class QMouseEvent;
class QWheelEvent;

namespace Ogre
{
    class Plane;
    class SceneManager;
    class Camera;
    class Viewport;
}

namespace CSVWorld
{
    class PhysicsSystem;
}

namespace CSMWorld
{
    class IdTable;
}

namespace CSVRender
{
    class WorldspaceWidget;

    class MouseState
    {
            enum MouseStates
            {
                Mouse_Grab,
                Mouse_Drag,
                Mouse_Edit,
                Mouse_Default
            };
            MouseStates mMouseState;

            WorldspaceWidget *mParent;
            CSVWorld::PhysicsSystem *mPhysics; // local copy
            Ogre::SceneManager *mSceneManager; // local copy

            QPoint mOldPos;
            std::string mCurrentObj;
            std::string mGrabbedSceneNode;
            QElapsedTimer *mMouseEventTimer;
            Ogre::Plane *mPlane;
            Ogre::Vector3 mOrigObjPos;
            Ogre::Vector3 mOrigMousePos;
            Ogre::Vector3 mCurrentMousePos;
            float mOffset;

            CSMWorld::IdTable *mIdTableModel;
            int mColIndexPosX;
            int mColIndexPosY;
            int mColIndexPosZ;

        public:

            MouseState(WorldspaceWidget *parent);
            ~MouseState();

            void mouseMoveEvent (QMouseEvent *event);
            void mousePressEvent (QMouseEvent *event);
            void mouseReleaseEvent (QMouseEvent *event);
            void mouseDoubleClickEvent (QMouseEvent *event);
            bool wheelEvent (QWheelEvent *event);

        private:

            std::pair<bool, Ogre::Vector3> mousePositionOnPlane(const QPoint &pos, const Ogre::Plane &plane);
            std::pair<std::string, Ogre::Vector3> terrainUnderCursor(const int mouseX, const int mouseY);
            std::pair<std::string, Ogre::Vector3> objectUnderCursor(const int mouseX, const int mouseY);
            std::pair<Ogre::Vector3, Ogre::Vector3> planeAxis();
            void updateSceneWidgets();
            bool isDebug();

            Ogre::Camera *getCamera();     // friend access
            Ogre::Viewport *getViewport(); // friend access
    };
}

#endif // OPENCS_VIEW_MOUSESTATE_H
