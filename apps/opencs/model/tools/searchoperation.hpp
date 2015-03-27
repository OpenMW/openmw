#ifndef CSM_TOOLS_SEARCHOPERATION_H
#define CSM_TOOLS_SEARCHOPERATION_H

#include "../doc/operation.hpp"

#include "search.hpp"

namespace CSMDoc
{
    class Document;
}

namespace CSMTools
{
    class SearchStage;
    
    class SearchOperation : public CSMDoc::Operation
    {
            Search mSearch;
            
        public:

            SearchOperation (CSMDoc::Document& document);

            /// \attention Do not call this function while a search is running.
            void configure (const Search& search);

            void appendStage (SearchStage *stage);
            ///< The ownership of \a stage is transferred to *this.
            ///
            /// \attention Do no call this function while this Operation is running.

            const Search& getSearch() const;
            
    };
}

#endif
