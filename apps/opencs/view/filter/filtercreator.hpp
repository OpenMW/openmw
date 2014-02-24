#ifndef CSV_FILTER_FILTERCREATOR_H
#define CSV_FILTER_FILTERCREATOR_H

class QComboBox;
class QLabel;

#include "../world/genericcreator.hpp"

namespace CSVFilter
{
    class FilterCreator : public CSVWorld::GenericCreator
    {
            Q_OBJECT

            QComboBox *mScope;
            QLabel *mNamespace;

        private:

            std::string getNamespace() const;

        protected:

            void update();

            virtual std::string getId() const;

            virtual void configureCreateCommand (CSMWorld::CreateCommand& command) const;

        public:

            FilterCreator (CSMWorld::Data& data, QUndoStack& undoStack,
                const CSMWorld::UniversalId& id);

            virtual void reset();

        private slots:

            void setScope (int index);
    };
}

#endif
