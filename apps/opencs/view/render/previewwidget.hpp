#ifndef OPENCS_VIEW_PREVIEWWIDGET_H
#define OPENCS_VIEW_PREVIEWWIDGET_H

#include "scenewidget.hpp"

#include "navigationorbit.hpp"
#include "object.hpp"

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
            Object mObject;

        public:

            PreviewWidget (CSMWorld::Data& data, const std::string& id, bool referenceable,
                QWidget *parent = 0);

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
