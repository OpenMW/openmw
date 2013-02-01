#ifndef CSV_WORLD_DIALOGUESUBVIEW_H
#define CSV_WORLD_DIALOGUESUBVIEW_H

#include "../doc/subview.hpp"

class QDataWidgetMapper;

namespace CSMDoc
{
    class Document;
}

namespace CSVWorld
{
    class DialogueSubView : public CSVDoc::SubView
    {
            QDataWidgetMapper *mWidgetMapper;

        public:

            DialogueSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document, bool createAndDelete);

            virtual void setEditLock (bool locked);
    };
}

#endif