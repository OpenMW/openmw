#ifndef STARTSCRIPTCREATOR_HPP
#define STARTSCRIPTCREATOR_HPP

#include "genericcreator.hpp"

namespace CSVWorld {

    class StartScriptCreator : public GenericCreator
    {
        Q_OBJECT

        public:
            StartScriptCreator(CSMWorld::Data& data, QUndoStack& undoStack,
                               const CSMWorld::UniversalId& id, bool relaxedIdRules = false);

            virtual std::string getErrors() const;
            ///< Return formatted error descriptions for the current state of the creator. if an empty
            /// string is returned, there is no error.
     };

}



#endif // STARTSCRIPTCREATOR_HPP
