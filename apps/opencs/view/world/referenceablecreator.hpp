#ifndef CSV_WORLD_REFERENCEABLECREATOR_H
#define CSV_WORLD_REFERENCEABLECREATOR_H

#include "genericcreator.hpp"

#include <string>

#include <apps/opencs/model/world/universalid.hpp>

class QComboBox;
class QObject;
class QUndoStack;

namespace CSMWorld
{
    class CreateCommand;
    class Data;
}

namespace CSVWorld
{
    class ReferenceableCreator : public GenericCreator
    {
        Q_OBJECT

        QComboBox* mType;

    private:
        void configureCreateCommand(CSMWorld::CreateCommand& command) const override;

    public:
        explicit ReferenceableCreator(
            CSMWorld::Data& worldData, QUndoStack& undoStack, const CSMWorld::UniversalId& id);

        void reset() override;

        void cloneMode(const std::string& originId, const CSMWorld::UniversalId::Type type) override;

        void toggleWidgets(bool active = true) override;

    private slots:

        void setType(int index);
    };
}

#endif
