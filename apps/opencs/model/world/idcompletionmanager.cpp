#include "idcompletionmanager.hpp"

#include <boost/make_shared.hpp>

#include <QCompleter>

#include "data.hpp"
#include "idtablebase.hpp"

namespace
{
    std::map<CSMWorld::Columns::ColumnId, CSMWorld::UniversalId::Type> generateModelTypes()
    {
        std::map<CSMWorld::Columns::ColumnId, CSMWorld::UniversalId::Type> types;

        types[CSMWorld::Columns::ColumnId_Actor] = CSMWorld::UniversalId::Type_Referenceable;
        types[CSMWorld::Columns::ColumnId_AreaObject] = CSMWorld::UniversalId::Type_Referenceable;
        types[CSMWorld::Columns::ColumnId_AreaSound] = CSMWorld::UniversalId::Type_Sound;
        types[CSMWorld::Columns::ColumnId_BoltObject] = CSMWorld::UniversalId::Type_Referenceable;
        types[CSMWorld::Columns::ColumnId_BoltSound] = CSMWorld::UniversalId::Type_Sound;
        types[CSMWorld::Columns::ColumnId_CastingObject] = CSMWorld::UniversalId::Type_Referenceable;
        types[CSMWorld::Columns::ColumnId_CastingSound] = CSMWorld::UniversalId::Type_Sound;
        types[CSMWorld::Columns::ColumnId_Cell] = CSMWorld::UniversalId::Type_Cell;
        types[CSMWorld::Columns::ColumnId_Class] = CSMWorld::UniversalId::Type_Class;
        types[CSMWorld::Columns::ColumnId_Creature] = CSMWorld::UniversalId::Type_Referenceable;
        types[CSMWorld::Columns::ColumnId_DestinationCell] = CSMWorld::UniversalId::Type_Cell;
        types[CSMWorld::Columns::ColumnId_Enchantment] = CSMWorld::UniversalId::Type_Enchantment;
        types[CSMWorld::Columns::ColumnId_Faction] = CSMWorld::UniversalId::Type_Faction;
        types[CSMWorld::Columns::Columnid_Hair] = CSMWorld::UniversalId::Type_Mesh;
        types[CSMWorld::Columns::ColumnId_Head] = CSMWorld::UniversalId::Type_Mesh;
        types[CSMWorld::Columns::ColumnId_HitObject] = CSMWorld::UniversalId::Type_Referenceable;
        types[CSMWorld::Columns::ColumnId_HitSound] = CSMWorld::UniversalId::Type_Sound;
        types[CSMWorld::Columns::ColumnId_Icon] = CSMWorld::UniversalId::Type_Icon;
        types[CSMWorld::Columns::ColumnId_InventoryItemId] = CSMWorld::UniversalId::Type_Referenceable;
        types[CSMWorld::Columns::ColumnId_MajorSkill1] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::Columns::ColumnId_MajorSkill2] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::Columns::ColumnId_MajorSkill3] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::Columns::ColumnId_MajorSkill4] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::Columns::ColumnId_MajorSkill5] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::Columns::ColumnId_MinorSkill1] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::Columns::ColumnId_MinorSkill2] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::Columns::ColumnId_MinorSkill3] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::Columns::ColumnId_MinorSkill4] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::Columns::ColumnId_MinorSkill5] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::Columns::ColumnId_Model] = CSMWorld::UniversalId::Type_Mesh;
        types[CSMWorld::Columns::ColumnId_Owner] = CSMWorld::UniversalId::Type_Referenceable;
        types[CSMWorld::Columns::ColumnId_OwnerGlobal] = CSMWorld::UniversalId::Type_Global;
        types[CSMWorld::Columns::ColumnId_Particle] = CSMWorld::UniversalId::Type_Texture;
        types[CSMWorld::Columns::ColumnId_PcFaction] = CSMWorld::UniversalId::Type_Faction;
        types[CSMWorld::Columns::ColumnId_Race] = CSMWorld::UniversalId::Type_Race;
        types[CSMWorld::Columns::ColumnId_ReferenceableId] = CSMWorld::UniversalId::Type_Referenceable;
        types[CSMWorld::Columns::ColumnId_Region] = CSMWorld::UniversalId::Type_Region;
        types[CSMWorld::Columns::ColumnId_Skill1] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::Columns::ColumnId_Skill2] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::Columns::ColumnId_Skill3] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::Columns::ColumnId_Skill4] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::Columns::ColumnId_Skill5] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::Columns::ColumnId_Skill6] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::Columns::ColumnId_Skill7] = CSMWorld::UniversalId::Type_Skill;
        types[CSMWorld::Columns::ColumnId_SleepEncounter] = CSMWorld::UniversalId::Type_Referenceable;
        types[CSMWorld::Columns::ColumnId_Soul] = CSMWorld::UniversalId::Type_Referenceable;
        types[CSMWorld::Columns::ColumnId_Sound] = CSMWorld::UniversalId::Type_Sound;
        types[CSMWorld::Columns::ColumnId_SoundFile] = CSMWorld::UniversalId::Type_SoundRes;
        types[CSMWorld::Columns::ColumnId_SoundName] = CSMWorld::UniversalId::Type_Sound;
        types[CSMWorld::Columns::ColumnId_SpellId] = CSMWorld::UniversalId::Type_Spell;
        types[CSMWorld::Columns::ColumnId_Script] = CSMWorld::UniversalId::Type_Script;
        types[CSMWorld::Columns::ColumnId_TeleportCell] = CSMWorld::UniversalId::Type_Cell;
        types[CSMWorld::Columns::ColumnId_Texture] = CSMWorld::UniversalId::Type_Texture;
        types[CSMWorld::Columns::ColumnId_Trap] = CSMWorld::UniversalId::Type_Spell;

        return types;
    }
}

const std::map<CSMWorld::Columns::ColumnId, CSMWorld::UniversalId::Type>
    CSMWorld::IdCompletionManager::sCompleterModelTypes = generateModelTypes();

CSMWorld::IdCompletionManager::IdCompletionManager(CSMWorld::Data &data)
{
    generateCompleters(data);
}

bool CSMWorld::IdCompletionManager::isCompleterExistFor(CSMWorld::Columns::ColumnId id) const
{
    return mCompleters.find(id) != mCompleters.end();
}

boost::shared_ptr<QCompleter> CSMWorld::IdCompletionManager::getCompleter(CSMWorld::Columns::ColumnId id)
{
    if (!isCompleterExistFor(id))
    {
        throw std::logic_error("This column doesn't have an ID completer");
    }
    return mCompleters[id];
}

void CSMWorld::IdCompletionManager::generateCompleters(CSMWorld::Data &data)
{
    typedef std::map<CSMWorld::Columns::ColumnId, CSMWorld::UniversalId::Type>::const_iterator ModelTypeConstIterator;

    ModelTypeConstIterator current = sCompleterModelTypes.begin();
    ModelTypeConstIterator end = sCompleterModelTypes.end();
    for (; current != end; ++current)
    {
        QAbstractItemModel *model = data.getTableModel(current->second);
        CSMWorld::IdTableBase *table = dynamic_cast<CSMWorld::IdTableBase *>(model);
        if (table != NULL)
        {
            int idColumn = table->searchColumnIndex(CSMWorld::Columns::ColumnId_Id);
            if (idColumn != -1)
            {
                boost::shared_ptr<QCompleter> completer = boost::make_shared<QCompleter>(table);
                completer->setCompletionColumn(idColumn);
                // The completion role must be Qt::DisplayRole to get the ID values from the model
                completer->setCompletionRole(Qt::DisplayRole);
                completer->setCaseSensitivity(Qt::CaseInsensitive);
                mCompleters[current->first] = completer;
            }
        }
    }
}