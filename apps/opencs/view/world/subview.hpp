#ifndef CSV_WORLD_SUBVIEW_H
#define CSV_WORLD_SUBVIEW_H

#include "../../model/doc/document.hpp"

#include "../../model/world/universalid.hpp"

#include <QDockWidget>

class QUndoStack;

namespace CSMWorld
{
    class Data;
}

namespace CSVWorld
{
    class SubView : public QDockWidget
    {
            Q_OBJECT

            CSMWorld::UniversalId mUniversalId;

            // not implemented
            SubView (const SubView&);
            SubView& operator= (SubView&);

        public:

            SubView (const CSMWorld::UniversalId& id);

            CSMWorld::UniversalId getUniversalId() const;

            virtual void setEditLock (bool locked) = 0;
    };

    struct SubViewFactoryBase
    {
        virtual SubView *makeSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
            = 0;
    };

    template<class SubViewT>
    struct SubViewFactory : public SubViewFactoryBase
    {
        virtual SubView *makeSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);
    };

    template<class SubViewT>
    SubView *SubViewFactory<SubViewT>::makeSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
    {
        return new SubViewT (id, document.getData(), document.getUndoStack());
    }

    template<class SubViewT>
    struct SubViewFactoryWithCreateFlag : public SubViewFactoryBase
    {
        bool mCreateAndDelete;

        SubViewFactoryWithCreateFlag (bool createAndDelete);

        virtual SubView *makeSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);
    };

    template<class SubViewT>
    SubViewFactoryWithCreateFlag<SubViewT>::SubViewFactoryWithCreateFlag (bool createAndDelete)
    : mCreateAndDelete (createAndDelete)
    {}

    template<class SubViewT>
    SubView *SubViewFactoryWithCreateFlag<SubViewT>::makeSubView (const CSMWorld::UniversalId& id,
        CSMDoc::Document& document)
    {
        return new SubViewT (id, document.getData(), document.getUndoStack(), mCreateAndDelete);
    }
}

#endif