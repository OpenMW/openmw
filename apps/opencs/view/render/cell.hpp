#ifndef OPENCS_VIEW_CELL_H
#define OPENCS_VIEW_CELL_H

#include <string>
#include <map>
#include <memory>

#include <boost/shared_ptr.hpp>

#include <OgreVector3.h>

#ifndef Q_MOC_RUN
#include <components/terrain/terraingrid.hpp>
#endif

#include "object.hpp"

class QModelIndex;

namespace Ogre
{
    class SceneManager;
    class SceneNode;
    class ManualObject;
}

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class Pathgrid;
    class NestedTableProxyModel;
    class IdTree;
    class SignalHandler;
}

namespace CSVWorld
{
    class PhysicsSystem;
}

namespace CSVRender
{
    class PathgridPoint;

    class Cell
    {
            CSMDoc::Document& mDocument;
            std::string mId;
            Ogre::SceneNode *mCellNode;
            std::map<std::string, Object *> mObjects;
            std::map<std::string, PathgridPoint *> mPgPoints;
            std::map<std::pair<int, int>, std::string> mPgEdges;

            CSMWorld::NestedTableProxyModel *mProxyModel;
            CSMWorld::IdTree *mModel;
            int mPgIndex;
            CSMWorld::SignalHandler *mHandler;

            std::auto_ptr<Terrain::TerrainGrid> mTerrain;
            boost::shared_ptr<CSVWorld::PhysicsSystem> mPhysics;
            Ogre::SceneManager *mSceneMgr;
            int mX;
            int mY;

            /// Ignored if cell does not have an object with the given ID.
            ///
            /// \return Was the object deleted?
            bool removeObject (const std::string& id);

            /// Add objects from reference table that are within this cell.
            ///
            /// \return Have any objects been added?
            bool addObjects (int start, int end);

        public:

            Cell (CSMDoc::Document& document, Ogre::SceneManager *sceneManager, const std::string& id,
                boost::shared_ptr<CSVWorld::PhysicsSystem> physics, const Ogre::Vector3& origin = Ogre::Vector3 (0, 0, 0));

            ~Cell();

            /// \return Did this call result in a modification of the visual representation of
            /// this cell?
            bool referenceableDataChanged (const QModelIndex& topLeft,
                const QModelIndex& bottomRight);

            /// \return Did this call result in a modification of the visual representation of
            /// this cell?
            bool referenceableAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            /// \return Did this call result in a modification of the visual representation of
            /// this cell?
            bool referenceDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            /// \return Did this call result in a modification of the visual representation of
            /// this cell?
            bool referenceAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            /// \return Did this call result in a modification of the visual representation of
            /// this cell?
            bool referenceAdded (const QModelIndex& parent, int start, int end);

            float getTerrainHeightAt(const Ogre::Vector3 &pos) const;

            void pathgridPointAdded(const Ogre::Vector3 &pos, bool interior = false);
            void pathgridPointMoved(const std::string &name,
                    const Ogre::Vector3 &newPos, bool interior = false);
            void pathgridPointRemoved(const std::string &name);

            void pathgridDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

        private:

            // for drawing pathgrid points & lines
            void createGridMaterials();
            void destroyGridMaterials();
            void setupPathgrid();
            Ogre::ManualObject *createPathgridEdge(const std::string &name,
                    const Ogre::Vector3 &start, const Ogre::Vector3 &end);

            void addPathgridEdge();
            void removePathgridEdge();

        public:

            void clearPathgrid();
            void buildPathgrid();
            CSMWorld::SignalHandler *getSignalHandler();
    };
}

#endif
