#ifndef OPENCS_VIEW_PREVIEWWIDGET_H
#define OPENCS_VIEW_PREVIEWWIDGET_H

#include <components/nifogre/ogrenifloader.hpp>

#include "scenewidget.hpp"

#include "navigationorbit.hpp"

class QModelIndex;

namespace CSMWorld
{
    class Data;
}

namespace CSVRender
{
    class PreviewWidget : public SceneWidget
    {
            Q_OBJECT

            CSMWorld::Data& mData;
            CSVRender::NavigationOrbit mOrbit;
            NifOgre::ObjectScenePtr mObject;
            Ogre::SceneNode *mNode;
            std::string mReferenceId;
            std::string mReferenceableId;

            void setup();

            void setModel();

            void adjust();
            ///< Adjust referenceable preview according to the reference

        public:

            PreviewWidget (CSMWorld::Data& data, const std::string& referenceableId,
                QWidget *parent = 0);

            PreviewWidget (CSMWorld::Data& data, const std::string& referenceableId,
                const std::string& referenceId, QWidget *parent = 0);

        signals:

            void closeRequest();

            void referenceableIdChanged (const std::string& id);

        private slots:

            void ReferenceableDataChanged (const QModelIndex& topLeft,
                const QModelIndex& bottomRight);

            void ReferenceableAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            void ReferenceDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void ReferenceAboutToBeRemoved (const QModelIndex& parent, int start, int end);
    };
}

#endif
