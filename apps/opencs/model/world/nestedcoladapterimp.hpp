#ifndef CSM_WOLRD_NESTEDCOLADAPTERIMP_H
#define CSM_WOLRD_NESTEDCOLADAPTERIMP_H

#include <QVariant>

#include <components/esm/loadpgrd.hpp>
#include <components/esm/effectlist.hpp>
#include <components/esm/loadmgef.hpp> // for converting magic effect id to string & back
#include <components/esm/loadskil.hpp> // for converting skill names
#include <components/esm/attr.hpp>     // for converting attributes
#include <components/esm/loadrace.hpp>

#include "nestedcolumnadapter.hpp"
#include "nestedtablewrapper.hpp"
#include "cell.hpp"

namespace ESM
{
    struct Faction;
    struct Region;
}

namespace CSMWorld
{
    struct Pathgrid;
    struct Info;

    class PathgridPointListAdapter : public NestedColumnAdapter<Pathgrid>
    {
    public:
        PathgridPointListAdapter ();

        void addRow(Record<Pathgrid>& record, int position) const override;

        void removeRow(Record<Pathgrid>& record, int rowToRemove) const override;

        void setTable(Record<Pathgrid>& record,
                const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<Pathgrid>& record) const override;

        QVariant getData(const Record<Pathgrid>& record,
                int subRowIndex, int subColIndex) const override;

        void setData(Record<Pathgrid>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<Pathgrid>& record) const override;

        int getRowsCount(const Record<Pathgrid>& record) const override;
    };

    class PathgridEdgeListAdapter : public NestedColumnAdapter<Pathgrid>
    {
    public:
        PathgridEdgeListAdapter ();

        void addRow(Record<Pathgrid>& record, int position) const override;

        void removeRow(Record<Pathgrid>& record, int rowToRemove) const override;

        void setTable(Record<Pathgrid>& record,
                const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<Pathgrid>& record) const override;

        QVariant getData(const Record<Pathgrid>& record,
                int subRowIndex, int subColIndex) const override;

        void setData(Record<Pathgrid>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<Pathgrid>& record) const override;

        int getRowsCount(const Record<Pathgrid>& record) const override;
    };

    class FactionReactionsAdapter : public NestedColumnAdapter<ESM::Faction>
    {
    public:
        FactionReactionsAdapter ();

        void addRow(Record<ESM::Faction>& record, int position) const override;

        void removeRow(Record<ESM::Faction>& record, int rowToRemove) const override;

        void setTable(Record<ESM::Faction>& record,
                const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<ESM::Faction>& record) const override;

        QVariant getData(const Record<ESM::Faction>& record,
                int subRowIndex, int subColIndex) const override;

        void setData(Record<ESM::Faction>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<ESM::Faction>& record) const override;

        int getRowsCount(const Record<ESM::Faction>& record) const override;
    };

    class FactionRanksAdapter : public NestedColumnAdapter<ESM::Faction>
    {
    public:
        FactionRanksAdapter ();

        void addRow(Record<ESM::Faction>& record, int position) const override;

        void removeRow(Record<ESM::Faction>& record, int rowToRemove) const override;

        void setTable(Record<ESM::Faction>& record,
                const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<ESM::Faction>& record) const override;

        QVariant getData(const Record<ESM::Faction>& record,
                int subRowIndex, int subColIndex) const override;

        void setData(Record<ESM::Faction>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<ESM::Faction>& record) const override;

        int getRowsCount(const Record<ESM::Faction>& record) const override;
    };

    class RegionSoundListAdapter : public NestedColumnAdapter<ESM::Region>
    {
    public:
        RegionSoundListAdapter ();

        void addRow(Record<ESM::Region>& record, int position) const override;

        void removeRow(Record<ESM::Region>& record, int rowToRemove) const override;

        void setTable(Record<ESM::Region>& record,
                const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<ESM::Region>& record) const override;

        QVariant getData(const Record<ESM::Region>& record,
                int subRowIndex, int subColIndex) const override;

        void setData(Record<ESM::Region>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<ESM::Region>& record) const override;

        int getRowsCount(const Record<ESM::Region>& record) const override;
    };

    template<typename ESXRecordT>
    class SpellListAdapter : public NestedColumnAdapter<ESXRecordT>
    {
    public:
        SpellListAdapter () {}

        void addRow(Record<ESXRecordT>& record, int position) const override
        {
            ESXRecordT raceOrBthSgn = record.get();

            std::vector<std::string>& spells = raceOrBthSgn.mPowers.mList;

            // blank row
            std::string spell = "";

            spells.insert(spells.begin()+position, spell);

            record.setModified (raceOrBthSgn);
        }

        void removeRow(Record<ESXRecordT>& record, int rowToRemove) const override
        {
            ESXRecordT raceOrBthSgn = record.get();

            std::vector<std::string>& spells = raceOrBthSgn.mPowers.mList;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (spells.size()))
                throw std::runtime_error ("index out of range");

            spells.erase(spells.begin()+rowToRemove);

            record.setModified (raceOrBthSgn);
        }

        void setTable(Record<ESXRecordT>& record, const NestedTableWrapperBase& nestedTable) const override
        {
            ESXRecordT raceOrBthSgn = record.get();

            raceOrBthSgn.mPowers.mList =
                static_cast<const NestedTableWrapper<std::vector<std::string> >&>(nestedTable).mNestedTable;

            record.setModified (raceOrBthSgn);
        }

        NestedTableWrapperBase* table(const Record<ESXRecordT>& record) const override
        {
            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::vector<std::string> >(record.get().mPowers.mList);
        }

        QVariant getData(const Record<ESXRecordT>& record, int subRowIndex, int subColIndex) const override
        {
            ESXRecordT raceOrBthSgn = record.get();

            std::vector<std::string>& spells = raceOrBthSgn.mPowers.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (spells.size()))
                throw std::runtime_error ("index out of range");

            std::string spell = spells[subRowIndex];
            switch (subColIndex)
            {
                case 0: return QString(spell.c_str());
                default: throw std::runtime_error("Spells subcolumn index out of range");
            }
        }

        void setData(Record<ESXRecordT>& record, const QVariant& value,
                                    int subRowIndex, int subColIndex) const override
        {
            ESXRecordT raceOrBthSgn = record.get();

            std::vector<std::string>& spells = raceOrBthSgn.mPowers.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (spells.size()))
                throw std::runtime_error ("index out of range");

            std::string spell = spells[subRowIndex];
            switch (subColIndex)
            {
                case 0: spell = value.toString().toUtf8().constData(); break;
                default: throw std::runtime_error("Spells subcolumn index out of range");
            }

            raceOrBthSgn.mPowers.mList[subRowIndex] = spell;

            record.setModified (raceOrBthSgn);
        }

        int getColumnsCount(const Record<ESXRecordT>& record) const override
        {
            return 1;
        }

        int getRowsCount(const Record<ESXRecordT>& record) const override
        {
            return static_cast<int>(record.get().mPowers.mList.size());
        }
    };

    template<typename ESXRecordT>
    class EffectsListAdapter : public NestedColumnAdapter<ESXRecordT>
    {
    public:
        EffectsListAdapter () {}

        void addRow(Record<ESXRecordT>& record, int position) const override
        {
            ESXRecordT magic = record.get();

            std::vector<ESM::ENAMstruct>& effectsList = magic.mEffects.mList;

            // blank row
            ESM::ENAMstruct effect;
            effect.mEffectID = 0;
            effect.mSkill = -1;
            effect.mAttribute = -1;
            effect.mRange = 0;
            effect.mArea = 0;
            effect.mDuration = 0;
            effect.mMagnMin = 0;
            effect.mMagnMax = 0;

            effectsList.insert(effectsList.begin()+position, effect);

            record.setModified (magic);
        }

        void removeRow(Record<ESXRecordT>& record, int rowToRemove) const override
        {
            ESXRecordT magic = record.get();

            std::vector<ESM::ENAMstruct>& effectsList = magic.mEffects.mList;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (effectsList.size()))
                throw std::runtime_error ("index out of range");

            effectsList.erase(effectsList.begin()+rowToRemove);

            record.setModified (magic);
        }

        void setTable(Record<ESXRecordT>& record, const NestedTableWrapperBase& nestedTable) const override
        {
            ESXRecordT magic = record.get();

            magic.mEffects.mList =
                static_cast<const NestedTableWrapper<std::vector<ESM::ENAMstruct> >&>(nestedTable).mNestedTable;

            record.setModified (magic);
        }

        NestedTableWrapperBase* table(const Record<ESXRecordT>& record) const override
        {
            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::vector<ESM::ENAMstruct> >(record.get().mEffects.mList);
        }

        QVariant getData(const Record<ESXRecordT>& record, int subRowIndex, int subColIndex) const override
        {
            ESXRecordT magic = record.get();

            std::vector<ESM::ENAMstruct>& effectsList = magic.mEffects.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (effectsList.size()))
                throw std::runtime_error ("index out of range");

            ESM::ENAMstruct effect = effectsList[subRowIndex];
            switch (subColIndex)
            {
                case 0:
                {
                    if (effect.mEffectID >=0 && effect.mEffectID < ESM::MagicEffect::Length)
                        return effect.mEffectID;
                    else
                        throw std::runtime_error("Magic effects ID unexpected value");
                }
                case 1:
                {
                    switch (effect.mEffectID)
                    {
                        case ESM::MagicEffect::DrainSkill:
                        case ESM::MagicEffect::DamageSkill:
                        case ESM::MagicEffect::RestoreSkill:
                        case ESM::MagicEffect::FortifySkill:
                        case ESM::MagicEffect::AbsorbSkill:
                             return effect.mSkill;
                        default:
                            return QVariant();
                    }
                }
                case 2:
                {
                    switch (effect.mEffectID)
                    {
                        case ESM::MagicEffect::DrainAttribute:
                        case ESM::MagicEffect::DamageAttribute:
                        case ESM::MagicEffect::RestoreAttribute:
                        case ESM::MagicEffect::FortifyAttribute:
                        case ESM::MagicEffect::AbsorbAttribute:
                             return effect.mAttribute;
                        default:
                            return QVariant();
                    }
                }
                case 3:
                {
                    if (effect.mRange >=0 && effect.mRange <=2)
                        return effect.mRange;
                    else
                        throw std::runtime_error("Magic effects range unexpected value");
                }
                case 4: return effect.mArea;
                case 5: return effect.mDuration;
                case 6: return effect.mMagnMin;
                case 7: return effect.mMagnMax;
                default: throw std::runtime_error("Magic Effects subcolumn index out of range");
            }
        }

        void setData(Record<ESXRecordT>& record, const QVariant& value,
                                    int subRowIndex, int subColIndex) const override
        {
            ESXRecordT magic = record.get();

            std::vector<ESM::ENAMstruct>& effectsList = magic.mEffects.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (effectsList.size()))
                throw std::runtime_error ("index out of range");

            ESM::ENAMstruct effect = effectsList[subRowIndex];
            switch (subColIndex)
            {
                case 0:
                {
                    effect.mEffectID = static_cast<short>(value.toInt());
                    break;
                }
                case 1:
                {
                    effect.mSkill = static_cast<signed char>(value.toInt());
                    break;
                }
                case 2:
                {
                    effect.mAttribute = static_cast<signed char>(value.toInt());
                    break;
                }
                case 3:
                {
                    effect.mRange = value.toInt();
                    break;
                }
                case 4: effect.mArea = value.toInt(); break;
                case 5: effect.mDuration = value.toInt(); break;
                case 6: effect.mMagnMin = value.toInt(); break;
                case 7: effect.mMagnMax = value.toInt(); break;
                default: throw std::runtime_error("Magic Effects subcolumn index out of range");
            }

            magic.mEffects.mList[subRowIndex] = effect;

            record.setModified (magic);
        }

        int getColumnsCount(const Record<ESXRecordT>& record) const override
        {
            return 8;
        }

        int getRowsCount(const Record<ESXRecordT>& record) const override
        {
            return static_cast<int>(record.get().mEffects.mList.size());
        }
    };

    class InfoListAdapter : public NestedColumnAdapter<Info>
    {
    public:
        InfoListAdapter ();

        void addRow(Record<Info>& record, int position) const override;

        void removeRow(Record<Info>& record, int rowToRemove) const override;

        void setTable(Record<Info>& record,
                const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<Info>& record) const override;

        QVariant getData(const Record<Info>& record,
                int subRowIndex, int subColIndex) const override;

        void setData(Record<Info>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<Info>& record) const override;

        int getRowsCount(const Record<Info>& record) const override;
    };

    class InfoConditionAdapter : public NestedColumnAdapter<Info>
    {
    public:
        InfoConditionAdapter ();

        void addRow(Record<Info>& record, int position) const override;

        void removeRow(Record<Info>& record, int rowToRemove) const override;

        void setTable(Record<Info>& record,
                const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<Info>& record) const override;

        QVariant getData(const Record<Info>& record,
                int subRowIndex, int subColIndex) const override;

        void setData(Record<Info>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<Info>& record) const override;

        int getRowsCount(const Record<Info>& record) const override;
    };

    class RaceAttributeAdapter : public NestedColumnAdapter<ESM::Race>
    {
    public:
        RaceAttributeAdapter ();

        void addRow(Record<ESM::Race>& record, int position) const override;

        void removeRow(Record<ESM::Race>& record, int rowToRemove) const override;

        void setTable(Record<ESM::Race>& record,
                const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<ESM::Race>& record) const override;

        QVariant getData(const Record<ESM::Race>& record,
                int subRowIndex, int subColIndex) const override;

        void setData(Record<ESM::Race>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<ESM::Race>& record) const override;

        int getRowsCount(const Record<ESM::Race>& record) const override;
    };

    class RaceSkillsBonusAdapter : public NestedColumnAdapter<ESM::Race>
    {
    public:
        RaceSkillsBonusAdapter ();

        void addRow(Record<ESM::Race>& record, int position) const override;

        void removeRow(Record<ESM::Race>& record, int rowToRemove) const override;

        void setTable(Record<ESM::Race>& record,
                const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<ESM::Race>& record) const override;

        QVariant getData(const Record<ESM::Race>& record,
                int subRowIndex, int subColIndex) const override;

        void setData(Record<ESM::Race>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<ESM::Race>& record) const override;

        int getRowsCount(const Record<ESM::Race>& record) const override;
    };

    class CellListAdapter : public NestedColumnAdapter<CSMWorld::Cell>
    {
    public:
        CellListAdapter ();

        void addRow(Record<CSMWorld::Cell>& record, int position) const override;

        void removeRow(Record<CSMWorld::Cell>& record, int rowToRemove) const override;

        void setTable(Record<CSMWorld::Cell>& record,
                const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<CSMWorld::Cell>& record) const override;

        QVariant getData(const Record<CSMWorld::Cell>& record,
                int subRowIndex, int subColIndex) const override;

        void setData(Record<CSMWorld::Cell>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<CSMWorld::Cell>& record) const override;

        int getRowsCount(const Record<CSMWorld::Cell>& record) const override;
    };

    class RegionWeatherAdapter : public NestedColumnAdapter<ESM::Region>
    {
    public:
        RegionWeatherAdapter ();

        void addRow(Record<ESM::Region>& record, int position) const override;

        void removeRow(Record<ESM::Region>& record, int rowToRemove) const override;

        void setTable(Record<ESM::Region>& record,
                const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<ESM::Region>& record) const override;

        QVariant getData(const Record<ESM::Region>& record,
                int subRowIndex, int subColIndex) const override;

        void setData(Record<ESM::Region>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<ESM::Region>& record) const override;

        int getRowsCount(const Record<ESM::Region>& record) const override;
    };
}

#endif // CSM_WOLRD_NESTEDCOLADAPTERIMP_H
