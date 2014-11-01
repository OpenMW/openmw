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

            std::map<CSMDoc::Document *, std::list<CSVRender::SceneWidget *> > mSceneWidgets;
            std::map<CSMDoc::Document *, CSVWorld::PhysicsSystem *> mPhysics;

        public:

            PhysicsManager();
            ~PhysicsManager();

            static PhysicsManager *instance();

            void setupPhysics(CSMDoc::Document *);

            PhysicsSystem *addSceneWidget(CSMDoc::Document &doc, CSVRender::WorldspaceWidget *widget);

            void removeSceneWidget(CSVRender::WorldspaceWidget *widget);

            void removeDocument(CSMDoc::Document *doc);
    };
}

#endif // CSV_WORLD_PHYSICSMANAGER_H
