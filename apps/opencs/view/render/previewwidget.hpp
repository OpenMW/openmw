#ifndef OPENCS_VIEW_PREVIEWWIDGET_H
#define OPENCS_VIEW_PREVIEWWIDGET_H

#include "scenewidget.hpp"

#include "object.hpp"

class QModelIndex;

namespace VFS
{
    class Manager;
}

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
            CSVRender::Object mObject;

        public:

            PreviewWidget (CSMWorld::Data& data, const std::string& id, bool referenceable,
                QWidget *parent = nullptr);

        signals:

            void closeRequest();

            void referenceableIdChanged (const std::string& id);

        private slots:

            void referenceableDataChanged (const QModelIndex& topLeft,
                const QModelIndex& bottomRight);

            void referenceableAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            void referenceDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void referenceAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            void assetTablesChanged ();
    };
}

#endif
