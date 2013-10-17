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


    template<class SubViewT, class CreatorFactoryT>
    class SubViewFactoryWithCreator : public SubViewFactoryBase
    {
        public:

            virtual CSVDoc::SubView *makeSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document);
    };

    template<class SubViewT, class CreatorFactoryT>
    CSVDoc::SubView *SubViewFactoryWithCreator<SubViewT, CreatorFactoryT>::makeSubView (
        const CSMWorld::UniversalId& id, CSMDoc::Document& document)
    {
        return new SubViewT (id, document, CreatorFactoryT());
    }
}

#endif