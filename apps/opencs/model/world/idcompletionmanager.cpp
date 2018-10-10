#include "idcompletionmanager.hpp"

#include <QCompleter>

#include "../../view/widget/completerpopup.hpp"

#include "data.hpp"
#include "idtablebase.hpp"

namespace
{
    std::map<CSMWorld::ColumnBase::Display, CSMWorld::UniversalId::Type> generateModelTypes()
    {
        std::map<CSMWorld::ColumnBase::Display, CSMWorld::UniversalId::Type> types;

        types[CSMWorld::ColumnBase::Display_BodyPart            ] = CSMWorld::UniversalId::Type_BodyPart;
        types[CSMWorld::ColumnBase::Display_Cell                ] = CSMWorld::UniversalId::Type_Cell;
        types[CSMWorld::ColumnBase::Display_Class               ] = CSMWorld::UniversalId::Type_Class;
        types[CSMWorld::ColumnBase::Display_CreatureLevelledList] = CSMWorld::UniversalId::Type_Referenceable;
        types[CSMWorld::ColumnBase::Display_Creature            ] = CSMWorld::UniversalId::Type_Referenceable;
        types[CSMWorld::ColumnBase::Display_Enchantment         ] = CSMWorld::UniversalId::Type_Enchantment;
        types[CSMWorld::ColumnBase::Display_Faction             ] = CSMWorld::UniversalId::Type_Faction;
        types[CSMWorld::ColumnBase::Display_GlobalVariable      ] = CSMWorld::UniversalId::Type_Global;
        types[CSMWorld::ColumnBase::Display_Icon                ] = CSMWorld::UniversalId::Type_Icon;
        types[CSMWorld::ColumnBase::Display_Journal             ] = CSMWorld::UniversalId::Type_Journal;
        types[CSMWorld::ColumnBase::Display_Mesh                ] = CSMWorld::UniversalId::Type_Mesh;
        types[CSMWorld::ColumnBase::Display_Miscellaneous       ] = CSMWorld::UniversalId::Type_Referenceable;
        types[CSMWorld::ColumnBase::Display_Npc                 ] = CSMWorld::UniversalId::Type_Referenceable;
        types[CSMWorld::ColumnBase::Display_Race                ] = CSMWorld::UniversalId::Type_Race;
        types[CSMWorld::ColumnBase::Display_Region              ] = CSMWorld::UniversalId::Type_Region;
        types[CSMWorld::ColumnBase::Display_Referenceable       ] = CSMWorld::UniversalId::Type_Referenceable;
        types[CSMWorld::ColumnBase::Display_Script              ] = CSMWorld::UniversalId::Type_Script;
        types[CSMWorld::ColumnBase::Display_Skill               ] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::ColumnBase::Display_Sound               ] = CSMWorld::UniversalId::Type_Sound;
        types[CSMWorld::ColumnBase::Display_SoundRes            ] = CSMWorld::UniversalId::Type_SoundRes;
        types[CSMWorld::ColumnBase::Display_Spell               ] = CSMWorld::UniversalId::Type_Spell;
        types[CSMWorld::ColumnBase::Display_Static              ] = CSMWorld::UniversalId::Type_Referenceable;
        types[CSMWorld::ColumnBase::Display_Texture             ] = CSMWorld::UniversalId::Type_Texture;
        types[CSMWorld::ColumnBase::Display_Topic               ] = CSMWorld::UniversalId::Type_Topic;
        types[CSMWorld::ColumnBase::Display_Weapon              ] = CSMWorld::UniversalId::Type_Referenceable;

        return types;
    }

    typedef std::map<CSMWorld::ColumnBase::Display, 
                     CSMWorld::UniversalId::Type>::const_iterator ModelTypeConstIterator;
}

const std::map<CSMWorld::ColumnBase::Display, CSMWorld::UniversalId::Type>
    CSMWorld::IdCompletionManager::sCompleterModelTypes = generateModelTypes();

std::vector<CSMWorld::ColumnBase::Display> CSMWorld::IdCompletionManager::getDisplayTypes()
{
    std::vector<CSMWorld::ColumnBase::Display> types;
    ModelTypeConstIterator current = sCompleterModelTypes.begin();
    ModelTypeConstIterator end = sCompleterModelTypes.end();
    for (; current != end; ++current)
    {
        types.push_back(current->first);
    }

    // Hack for Display_InfoCondVar
    types.push_back(CSMWorld::ColumnBase::Display_InfoCondVar);

    return types;
}

CSMWorld::IdCompletionManager::IdCompletionManager(CSMWorld::Data &data)
{
    generateCompleters(data);
}

bool CSMWorld::IdCompletionManager::hasCompleterFor(CSMWorld::ColumnBase::Display display) const
{
    return mCompleters.find(display) != mCompleters.end();
}

std::shared_ptr<QCompleter> CSMWorld::IdCompletionManager::getCompleter(CSMWorld::ColumnBase::Display display)
{
    if (!hasCompleterFor(display))
    {
        throw std::logic_error("This column doesn't have an ID completer");
    }
    return mCompleters[display];
}

void CSMWorld::IdCompletionManager::generateCompleters(CSMWorld::Data &data)
{
    ModelTypeConstIterator current = sCompleterModelTypes.begin();
    ModelTypeConstIterator end = sCompleterModelTypes.end();
    for (; current != end; ++current)
    {
        QAbstractItemModel *model = data.getTableModel(current->second);
        CSMWorld::IdTableBase *table = dynamic_cast<CSMWorld::IdTableBase *>(model);
        if (table != nullptr)
        {
            int idColumn = table->searchColumnIndex(CSMWorld::Columns::ColumnId_Id);
            if (idColumn != -1)
            {
                std::shared_ptr<QCompleter> completer = std::make_shared<QCompleter>(table);
                completer->setCompletionColumn(idColumn);
                // The completion role must be Qt::DisplayRole to get the ID values from the model
                completer->setCompletionRole(Qt::DisplayRole);
                completer->setCaseSensitivity(Qt::CaseInsensitive);

                QAbstractItemView *popup = new CSVWidget::CompleterPopup();
                completer->setPopup(popup); // The completer takes ownership of the popup
                completer->setMaxVisibleItems(10);

                mCompleters[current->first] = completer;
            }
        }
    }
}
