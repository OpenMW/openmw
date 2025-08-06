#ifndef CSV_WORLD_CELLCREATOR_H
#define CSV_WORLD_CELLCREATOR_H

#include <string>

#include <apps/opencs/model/world/universalid.hpp>

#include "genericcreator.hpp"

class QComboBox;
class QLabel;
class QObject;
class QSpinBox;
class QUndoStack;

namespace CSMWorld
{
    class CreateCommand;
    class Data;
}

namespace CSVWorld
{
    class CellCreator : public GenericCreator
    {
        Q_OBJECT

        QComboBox* mType;
        QLabel* mXLabel;
        QSpinBox* mX;
        QLabel* mYLabel;
        QSpinBox* mY;

    protected:
        std::string getId() const override;

        /// Allow subclasses to add additional data to \a command.
        void configureCreateCommand(CSMWorld::CreateCommand& command) const override;

    public:
        explicit CellCreator(CSMWorld::Data& worldData, QUndoStack& undoStack, const CSMWorld::UniversalId& id);

        void reset() override;

        void cloneMode(const std::string& originId, const CSMWorld::UniversalId::Type type) override;

        std::string getErrors() const override;
        ///< Return formatted error descriptions for the current state of the creator. if an empty
        /// string is returned, there is no error.

    private slots:

        void setType(int index);

        void valueChanged(int index);
    };
}

#endif
