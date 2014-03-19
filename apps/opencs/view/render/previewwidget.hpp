#ifndef OPENCS_VIEW_PREVIEWWIDGET_H
#define OPENCS_VIEW_PREVIEWWIDGET_H

#include <components/nifogre/ogrenifloader.hpp>

#include "scenewidget.hpp"

#include "navigationorbit.hpp"

namespace CSMWorld
{
    class Data;
}

namespace CSVRender
{
    class PreviewWidget : public SceneWidget
    {
            Q_OBJECT

            const CSMWorld::Data& mData;
            CSVRender::NavigationOrbit mOrbit;
            NifOgre::ObjectScenePtr mObject;
            Ogre::SceneNode *mNode;

            void setup (const std::string& id);
            ///< \param id ID of the referenceable to be viewed

            void adjust (const std::string& id);
            ///< \param id ID of the reference to be viewed

        public:

            PreviewWidget (const CSMWorld::Data& data, const std::string& referenceableId,
                QWidget *parent = 0);

            PreviewWidget (const CSMWorld::Data& data, const std::string& referenceableId,
                const std::string& referenceId, QWidget *parent = 0);

        signals:

            void closeRequest();
    };
}

#endif
