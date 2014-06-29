#ifndef OPENCS_VIEW_CELL_H
#define OPENCS_VIEW_CELL_H

#include <string>
#include <map>

#include <OgreVector3.h>

#include "object.hpp"

class QModelIndex;

namespace Ogre
{
    class SceneManager;
    class SceneNode;
}

namespace CSMWorld
{
    class Data;
}

namespace CSVRender
{
    class Cell
    {
            CSMWorld::Data& mData;
            std::string mId;
            Ogre::SceneNode *mCellNode;
            std::map<std::string, Object *> mObjects;

            /// Ignored if cell does not have an object with the given ID.
            ///
            /// \return Was the object deleted?
            bool removeObject (const std::string& id);

            /// Add objects from reference table that are within this cell.
            ///
            /// \return Have any objects been added?
            bool addObjects (int start, int end);

        public:

            Cell (CSMWorld::Data& data, Ogre::SceneManager *sceneManager,
                const std::string& id, const Ogre::Vector3& origin = Ogre::Vector3 (0, 0, 0));

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
    };
}

#endif
