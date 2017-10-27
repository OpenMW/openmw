#include "landtexturecreator.hpp"

#include <cstdint>
#include <limits>

#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

#include "../../model/world/commands.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/landtexture.hpp"

namespace CSVWorld
{
    LandTextureCreator::LandTextureCreator(CSMWorld::Data& data, QUndoStack& undoStack, const CSMWorld::UniversalId& id)
        : GenericCreator(data, undoStack, id)
    {
        // One index is reserved for a default texture
        const size_t MaxIndex = std::numeric_limits<uint16_t>::max() - 1;

        setManualEditing(false);

        QLabel* nameLabel = new QLabel("Name");
        insertBeforeButtons(nameLabel, false);

        mNameEdit = new QLineEdit(this);
        insertBeforeButtons(mNameEdit, true);

        QLabel* indexLabel = new QLabel("Index");
        insertBeforeButtons(indexLabel, false);

        mIndexBox = new QSpinBox(this);
        mIndexBox->setMinimum(0);
        mIndexBox->setMaximum(MaxIndex);
        insertBeforeButtons(mIndexBox, true);

        connect(mNameEdit, SIGNAL(textChanged(const QString&)), this, SLOT(nameChanged(const QString&)));
        connect(mIndexBox, SIGNAL(valueChanged(int)), this, SLOT(indexChanged(int)));
    }

    void LandTextureCreator::cloneMode(const std::string& originId, const CSMWorld::UniversalId::Type type)
    {
        GenericCreator::cloneMode(originId, type);

        CSMWorld::IdTable& table = dynamic_cast<CSMWorld::IdTable&>(*getData().getTableModel(getCollectionId()));

        int column = table.findColumnIndex(CSMWorld::Columns::ColumnId_TextureNickname);
        mNameEdit->setText((table.data(table.getModelIndex(originId, column)).toString()));

        column = table.findColumnIndex(CSMWorld::Columns::ColumnId_TextureIndex);
        mIndexBox->setValue((table.data(table.getModelIndex(originId, column)).toInt()));
    }

    void LandTextureCreator::focus()
    {
        mIndexBox->setFocus();
    }

    void LandTextureCreator::reset()
    {
        GenericCreator::reset();
        mNameEdit->setText("");
        mIndexBox->setValue(0);
    }

    std::string LandTextureCreator::getErrors() const
    {
        if (getData().getLandTextures().searchId(getId()) >= 0)
        {
            return "Index is already in use";
        }

        return "";
    }

    void LandTextureCreator::configureCreateCommand(CSMWorld::CreateCommand& command) const
    {
        GenericCreator::configureCreateCommand(command);

        CSMWorld::IdTable& table = dynamic_cast<CSMWorld::IdTable&>(*getData().getTableModel(getCollectionId()));
        int column = table.findColumnIndex(CSMWorld::Columns::ColumnId_TextureNickname);
        command.addValue(column, mName.c_str());
    }

    std::string LandTextureCreator::getId() const
    {
        return CSMWorld::LandTexture::createUniqueRecordId(0, mIndexBox->value());
    }

    void LandTextureCreator::nameChanged(const QString& value)
    {
        mName = value.toUtf8().constData();
        update();
    }

    void LandTextureCreator::indexChanged(int value)
    {
        update();
    }
}
