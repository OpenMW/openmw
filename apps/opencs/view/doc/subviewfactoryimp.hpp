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

            CSVDoc::SubView *makeSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document) override;
    };

    template<class SubViewT>
    CSVDoc::SubView *SubViewFactory<SubViewT>::makeSubView (const CSMWorld::UniversalId& id,
        CSMDoc::Document& document)
    {
        return new SubViewT (id, document);
    }


    template<class SubViewT, class CreatorFactoryT>
    class SubViewFactoryWithCreator : public SubViewFactoryBase
    {
            bool mSorting;

        public:

            SubViewFactoryWithCreator (bool sorting = true);

            CSVDoc::SubView *makeSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document) override;
    };

    template<class SubViewT, class CreatorFactoryT>
    SubViewFactoryWithCreator<SubViewT, CreatorFactoryT>::SubViewFactoryWithCreator (bool sorting)
    : mSorting (sorting)
    {}

    template<class SubViewT, class CreatorFactoryT>
    CSVDoc::SubView *SubViewFactoryWithCreator<SubViewT, CreatorFactoryT>::makeSubView (
        const CSMWorld::UniversalId& id, CSMDoc::Document& document)
    {
        return new SubViewT (id, document, CreatorFactoryT(), mSorting);
    }
}

#endif
