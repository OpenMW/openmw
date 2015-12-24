#ifndef CSM_DOC_STAGE_H
#define CSM_DOC_STAGE_H

#include <vector>
#include <string>

#include "../world/universalid.hpp"

#include "messages.hpp"

class QString;

namespace CSMDoc
{
    class Stage
    {
        public:

            virtual ~Stage();

            virtual int setup() = 0;
            ///< \return number of steps

            virtual void perform (int stage, Messages& messages) = 0;
            ///< Messages resulting from this stage will be appended to \a messages.
    };
}

#endif
