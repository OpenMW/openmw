#ifndef CSM_DOC_STAGE_H
#define CSM_DOC_STAGE_H

#include <vector>
#include <string>

#include "../world/universalid.hpp"

namespace CSMDoc
{
    class Stage
    {
        public:

            typedef std::vector<std::pair<CSMWorld::UniversalId, std::string> > Messages;

            virtual ~Stage();

            virtual int setup() = 0;
            ///< \return number of steps

            virtual void perform (int stage, Messages& messages) = 0;
            ///< Messages resulting from this stage will be appended to \a messages.
    };
}

#endif

