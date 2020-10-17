#ifndef CSV_WORLD_PREVIEWSUBVIEW_H
#define CSV_WORLD_PREVIEWSUBVIEW_H

#include "../doc/subview.hpp"

namespace CSMDoc
{
    class Document;
}

namespace CSVRender
{
    class PreviewWidget;
}

namespace CSVWorld
{
    class PreviewSubView : public CSVDoc::SubView
    {
            Q_OBJECT

            CSVRender::PreviewWidget *mScene;
            std::string mTitle;

        public:

            PreviewSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);

            void setEditLock (bool locked) override;

            std::string getTitle() const override;

        private slots:

            void referenceableIdChanged (const std::string& id);
    };
}

#endif
