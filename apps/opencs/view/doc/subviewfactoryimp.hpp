#ifndef CSV_DOC_SUBVIEWFACTORYIMP_H
#define CSV_DOC_SUBVIEWFACTORYIMP_H

#include "../../model/doc/document.hpp"

#include "subviewfactory.hpp"

namespace CSVDoc
{
    template<class SubViewT>
    class SubViewFactory : public SubViewFactoryBase
    {
        public:

            virtual CSVDoc::SubView *makeSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);
    };

    template<class SubViewT>
    CSVDoc::SubView *SubViewFactory<SubViewT>::makeSubView (const CSMWorld::UniversalId& id,
        CSMDoc::Document& document)
    {
        return new SubViewT (id, document);
    }

    template<class SubViewT>
    class SubViewFactoryWithCreateFlag : public SubViewFactoryBase
    {
        bool mCreateAndDelete;

        public:

            SubViewFactoryWithCreateFlag (bool createAndDelete);

            virtual CSVDoc::SubView *makeSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);
    };

    template<class SubViewT>
    SubViewFactoryWithCreateFlag<SubViewT>::SubViewFactoryWithCreateFlag (bool createAndDelete)
    : mCreateAndDelete (createAndDelete)
    {}

    template<class SubViewT>
    CSVDoc::SubView *SubViewFactoryWithCreateFlag<SubViewT>::makeSubView (const CSMWorld::UniversalId& id,
        CSMDoc::Document& document)
    {
        return new SubViewT (id, document, mCreateAndDelete);
    }
}

#endif