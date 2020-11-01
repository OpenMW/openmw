#ifndef CSM_TOOLS_SEARCHSTAGE_H
#define CSM_TOOLS_SEARCHSTAGE_H

#include "../doc/stage.hpp"

#include "search.hpp"

namespace CSMWorld
{
    class IdTableBase;
}

namespace CSMTools
{
    class SearchOperation;
    
    class SearchStage : public CSMDoc::Stage
    {
            const CSMWorld::IdTableBase *mModel;
            Search mSearch;
            const SearchOperation *mOperation;

        public:

            SearchStage (const CSMWorld::IdTableBase *model);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, CSMDoc::Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.

            void setOperation (const SearchOperation *operation);
    };
}

#endif
