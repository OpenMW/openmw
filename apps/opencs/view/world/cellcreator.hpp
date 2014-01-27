#ifndef CSV_WORLD_CELLCREATOR_H
#define CSV_WORLD_CELLCREATOR_H

class QLabel;
class QSpinBox;
class QComboBox;

#include "genericcreator.hpp"

namespace CSVWorld
{
    class CellCreator : public GenericCreator
    {
            Q_OBJECT

            QComboBox *mType;
            QLabel *mXLabel;
            QSpinBox *mX;
            QLabel *mYLabel;
            QSpinBox *mY;

        protected:

            virtual std::string getId() const;

        public:

            CellCreator (CSMWorld::Data& data, QUndoStack& undoStack, const CSMWorld::UniversalId& id);

            virtual void reset();

            virtual void toggleWidgets(bool active = true);

            virtual void cloneMode(const std::string& originId, 
                                   const CSMWorld::UniversalId::Type type);

        private slots:

            void setType (int index);

            void valueChanged (int index);
    };
}

#endif
