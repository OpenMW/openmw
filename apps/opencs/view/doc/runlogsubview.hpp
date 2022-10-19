#ifndef CSV_DOC_RUNLOGSUBVIEW_H
#define CSV_DOC_RUNLOGSUBVIEW_H

#include "subview.hpp"

#include <apps/opencs/model/world/universalid.hpp>

namespace CSMDoc
{
    class Document;
}

namespace CSVDoc
{
    class RunLogSubView : public SubView
    {
        Q_OBJECT

    public:
        RunLogSubView(const CSMWorld::UniversalId& id, CSMDoc::Document& document);

        void setEditLock(bool locked) override;
    };
}

#endif
