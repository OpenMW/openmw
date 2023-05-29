#ifndef CSV_DOC_SUBVIEWFACTORY_H
#define CSV_DOC_SUBVIEWFACTORY_H

#include <map>

#include "../../model/world/universalid.hpp"

namespace CSMDoc
{
    class Document;
}

namespace CSVDoc
{
    class SubView;

    class SubViewFactoryBase
    {
    public:
        SubViewFactoryBase() = default;
        SubViewFactoryBase(const SubViewFactoryBase&) = delete;
        SubViewFactoryBase& operator=(const SubViewFactoryBase&) = delete;
        virtual ~SubViewFactoryBase() = default;

        virtual SubView* makeSubView(const CSMWorld::UniversalId& id, CSMDoc::Document& document) = 0;
        ///< The ownership of the returned sub view is not transferred.
    };

    class SubViewFactoryManager
    {
        std::map<CSMWorld::UniversalId::Type, SubViewFactoryBase*> mSubViewFactories;

    public:
        SubViewFactoryManager() = default;
        SubViewFactoryManager(const SubViewFactoryManager&) = delete;
        SubViewFactoryManager& operator=(const SubViewFactoryManager&) = delete;
        ~SubViewFactoryManager();

        void add(const CSMWorld::UniversalId::Type& id, SubViewFactoryBase* factory);
        ///< The ownership of \a factory is transferred to this.

        SubView* makeSubView(const CSMWorld::UniversalId& id, CSMDoc::Document& document);
        ///< The ownership of the returned sub view is not transferred.
    };
}

#endif
