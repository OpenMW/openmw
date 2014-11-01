#ifndef CSV_WORLD_PHYSICSMANAGER_H
#define CSV_WORLD_PHYSICSMANAGER_H

#include <map>
#include <list>

namespace Ogre
{
    class SceneManager;
}

namespace CSMDoc
{
    class Document;
}

namespace CSVRender
{
    class WorldspaceWidget;
    class SceneWidget;
}

namespace CSVWorld
{
    class PhysicsSystem;
}

namespace CSVWorld
{
    class PhysicsManager
    {
            static PhysicsManager *mPhysicsManagerInstance;

            //std::map<CSVRender::SceneWidget *, Ogre::SceneManager*>  mSceneManagers;
            std::map<CSMDoc::Document *, std::list<CSVRender::SceneWidget *> > mSceneWidgets;
            std::map<CSMDoc::Document *, CSVWorld::PhysicsSystem *> mPhysics;

        public:

            PhysicsManager();
            ~PhysicsManager();

            static PhysicsManager *instance();

            void setupPhysics(CSMDoc::Document *);
#if 0
            void addSceneManager(CSVRender::SceneWidget *sceneWidget, Ogre::SceneManager *sceneMgr);

            void removeSceneManager(CSVRender::SceneWidget *sceneWidget);
#endif

            PhysicsSystem *addSceneWidget(CSMDoc::Document &doc, CSVRender::WorldspaceWidget *widget);

            void removeSceneWidget(CSVRender::WorldspaceWidget *widget);
    };
}

#endif // CSV_WORLD_PHYSICSMANAGER_H
