#ifndef CSV_WORLD_GENERICCREATOR_H
#define CSV_WORLD_GENERICCREATOR_H

#include "creator.hpp"

namespace CSVWorld
{
    class GenericCreator : public Creator
    {
            Q_OBJECT

        public:

            GenericCreator();

            virtual void reset();
    };
}

#endif