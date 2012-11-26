#ifndef CSV_WORLD_GLOBALS_H
#define CSV_WORLD_GLOBALS_H

#include "subview.hpp"

namespace CSVWorld
{
    class Globals : public SubView
    {

        public:

            Globals (const CSMWorld::UniversalId& id, CSMWorld::Data& data);
    };
}

#endif