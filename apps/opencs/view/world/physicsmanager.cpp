#include "physicsmanager.hpp"

#include <iostream>

#include "../render/worldspacewidget.hpp"
#include "physicssystem.hpp"

namespace CSVWorld
{
    PhysicsManager *PhysicsManager::mPhysicsManagerInstance = 0;

    PhysicsManager::PhysicsManager()
    {
        assert(!mPhysicsManagerInstance);
        mPhysicsManagerInstance = this;
    }

    PhysicsManager::~PhysicsManager()
    {
        std::map<CSMDoc::Document *, CSVWorld::PhysicsSystem *>::iterator iter = mPhysics.begin();
        for(; iter != mPhysics.end(); ++iter)
            delete iter->second; // shouldn't be any left but just in case
    }

    PhysicsManager *PhysicsManager::instance()
    {
        assert(mPhysicsManagerInstance);
        return mPhysicsManagerInstance;
    }

    // create a physics instance per document, called from CSVDoc::View() to get Document*
    void PhysicsManager::setupPhysics(CSMDoc::Document *doc)
    {
        std::map<CSMDoc::Document *, std::list<CSVRender::SceneWidget *> >::iterator iter = mSceneWidgets.find(doc);
        if(iter == mSceneWidgets.end())
        {
            mSceneWidgets[doc] = std::list<CSVRender::SceneWidget *> (); // zero elements
            mPhysics[doc] = new PhysicsSystem();
        }
    }

    // destroy physics, called from CSVDoc::ViewManager
    void PhysicsManager::removeDocument(CSMDoc::Document *doc)
    {
        std::map<CSMDoc::Document *, CSVWorld::PhysicsSystem *>::iterator iter = mPhysics.find(doc);
        if(iter != mPhysics.end())
        {
            delete iter->second;
            mPhysics.erase(iter);
        }

        std::map<CSMDoc::Document *, std::list<CSVRender::SceneWidget *> >::iterator it = mSceneWidgets.find(doc);
        if(it != mSceneWidgets.end())
        {
            mSceneWidgets.erase(it);
        }
    }

    // called from CSVRender::WorldspaceWidget() to get widgets' association with Document&
    PhysicsSystem *PhysicsManager::addSceneWidget(CSMDoc::Document &doc, CSVRender::WorldspaceWidget *widget)
    {
        CSVRender::SceneWidget *sceneWidget = static_cast<CSVRender::SceneWidget *>(widget);

        std::map<CSMDoc::Document *, std::list<CSVRender::SceneWidget *> >::iterator iter = mSceneWidgets.begin();
        for(; iter != mSceneWidgets.end(); ++iter)
        {
            if((*iter).first == &doc)
            {
                (*iter).second.push_back(sceneWidget);
                return mPhysics[(*iter).first];  // TODO: consider using shared_ptr instead
            }
        }

        throw std::runtime_error("No physics system found for the given document.");
    }

    // deprecated by removeDocument() and may be deleted in future code updates
    // however there may be some value in removing the deleted scene widgets from the
    // list so that the list does not grow forever
    void PhysicsManager::removeSceneWidget(CSVRender::WorldspaceWidget *widget)
    {
        CSVRender::SceneWidget *sceneWidget = static_cast<CSVRender::SceneWidget *>(widget);

        std::map<CSMDoc::Document *, std::list<CSVRender::SceneWidget *> >::iterator iter = mSceneWidgets.begin();
        for(; iter != mSceneWidgets.end(); ++iter)
        {
            std::list<CSVRender::SceneWidget *>::iterator itWidget = (*iter).second.begin();
            for(; itWidget != (*iter).second.end(); ++itWidget)
            {
                if((*itWidget) == sceneWidget)
                {
                    (*iter).second.erase(itWidget);

                    //if((*iter).second.empty()) // last one for the document
                        // NOTE: do not delete physics until the document itself is closed

                    break;
                }
            }
        }
    }
}
