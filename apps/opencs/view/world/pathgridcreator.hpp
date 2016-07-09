#ifndef PATHGRIDCREATOR_HPP
#define PATHGRIDCREATOR_HPP

#include "genericcreator.hpp"

namespace CSVWorld
{
    /// \brief Record creator for pathgrids.
    class PathgridCreator : public GenericCreator
    {
        Q_OBJECT

        public:

            PathgridCreator(
                CSMWorld::Data& data,
                QUndoStack& undoStack,
                const CSMWorld::UniversalId& id,
                bool relaxedIdRules = false);

            /// \return Error description for current user input.
            virtual std::string getErrors() const;
    };
}

#endif // PATHGRIDCREATOR_HPP
