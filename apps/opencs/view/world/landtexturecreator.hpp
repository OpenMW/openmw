#ifndef CSV_WORLD_LANDTEXTURECREATOR_H
#define CSV_WORLD_LANDTEXTURECREATOR_H

#include <string>

#include "genericcreator.hpp"

class QLineEdit;
class QSpinBox;

namespace CSVWorld
{
    class LandTextureCreator : public GenericCreator
    {
            Q_OBJECT

        public:

            LandTextureCreator(CSMWorld::Data& data, QUndoStack& undoStack, const CSMWorld::UniversalId& id);

            void cloneMode(const std::string& originId, const CSMWorld::UniversalId::Type type) override;

            void focus() override;

            void reset() override;

            std::string getErrors() const override;

        protected:

            void configureCreateCommand(CSMWorld::CreateCommand& command) const override;

            std::string getId() const override;

        private slots:

            void nameChanged(const QString& val);
            void indexChanged(int val);

        private:

            QLineEdit* mNameEdit;
            QSpinBox* mIndexBox;

            std::string mName;
    };
}

#endif
