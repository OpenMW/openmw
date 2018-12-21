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

        virtual void addRow(Record<Pathgrid>& record, int position) const;

        virtual void removeRow(Record<Pathgrid>& record, int rowToRemove) const;

        virtual void setTable(Record<Pathgrid>& record,
                const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* table(const Record<Pathgrid>& record) const;

        virtual QVariant getData(const Record<Pathgrid>& record,
                int subRowIndex, int subColIndex) const;

        virtual void setData(Record<Pathgrid>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getColumnsCount(const Record<Pathgrid>& record) const;

        virtual int getRowsCount(const Record<Pathgrid>& record) const;
    };

    class PathgridEdgeListAdapter : public NestedColumnAdapter<Pathgrid>
    {
    public:
        PathgridEdgeListAdapter ();

        virtual void addRow(Record<Pathgrid>& record, int position) const;

        virtual void removeRow(Record<Pathgrid>& record, int rowToRemove) const;

        virtual void setTable(Record<Pathgrid>& record,
                const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* table(const Record<Pathgrid>& record) const;

        virtual QVariant getData(const Record<Pathgrid>& record,
                int subRowIndex, int subColIndex) const;

        virtual void setData(Record<Pathgrid>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getColumnsCount(const Record<Pathgrid>& record) const;

        virtual int getRowsCount(const Record<Pathgrid>& record) const;
    };

    class FactionReactionsAdapter : public NestedColumnAdapter<ESM::Faction>
    {
    public:
        FactionReactionsAdapter ();

        virtual void addRow(Record<ESM::Faction>& record, int position) const;

        virtual void removeRow(Record<ESM::Faction>& record, int rowToRemove) const;

        virtual void setTable(Record<ESM::Faction>& record,
                const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* table(const Record<ESM::Faction>& record) const;

        virtual QVariant getData(const Record<ESM::Faction>& record,
                int subRowIndex, int subColIndex) const;

        virtual void setData(Record<ESM::Faction>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getColumnsCount(const Record<ESM::Faction>& record) const;

        virtual int getRowsCount(const Record<ESM::Faction>& record) const;
    };

    class FactionRanksAdapter : public NestedColumnAdapter<ESM::Faction>
    {
    public:
        FactionRanksAdapter ();

        virtual void addRow(Record<ESM::Faction>& record, int position) const;

        virtual void removeRow(Record<ESM::Faction>& record, int rowToRemove) const;

        virtual void setTable(Record<ESM::Faction>& record,
                const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* table(const Record<ESM::Faction>& record) const;

        virtual QVariant getData(const Record<ESM::Faction>& record,
                int subRowIndex, int subColIndex) const;

        virtual void setData(Record<ESM::Faction>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getColumnsCount(const Record<ESM::Faction>& record) const;

        virtual int getRowsCount(const Record<ESM::Faction>& record) const;
    };

    class RegionSoundListAdapter : public NestedColumnAdapter<ESM::Region>
    {
    public:
        RegionSoundListAdapter ();

        virtual void addRow(Record<ESM::Region>& record, int position) const;

        virtual void removeRow(Record<ESM::Region>& record, int rowToRemove) const;

        virtual void setTable(Record<ESM::Region>& record,
                const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* table(const Record<ESM::Region>& record) const;

        virtual QVariant getData(const Record<ESM::Region>& record,
                int subRowIndex, int subColIndex) const;

        virtual void setData(Record<ESM::Region>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getColumnsCount(const Record<ESM::Region>& record) const;

        virtual int getRowsCount(const Record<ESM::Region>& record) const;
    };

    template<typename ESXRecordT>
    class SpellListAdapter : public NestedColumnAdapter<ESXRecordT>
    {
    public:
        SpellListAdapter () {}

        virtual void addRow(Record<ESXRecordT>& record, int position) const
        {
            ESXRecordT raceOrBthSgn = record.get();

            std::vector<std::string>& spells = raceOrBthSgn.mPowers.mList;

            // blank row
            std::string spell = "";

            spells.insert(spells.begin()+position, spell);

            record.setModified (raceOrBthSgn);
        }

        virtual void removeRow(Record<ESXRecordT>& record, int rowToRemove) const
        {
            ESXRecordT raceOrBthSgn = record.get();

            std::vector<std::string>& spells = raceOrBthSgn.mPowers.mList;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (spells.size()))
                throw std::runtime_error ("index out of range");

            spells.erase(spells.begin()+rowToRemove);

            record.setModified (raceOrBthSgn);
        }

        virtual void setTable(Record<ESXRecordT>& record, const NestedTableWrapperBase& nestedTable) const
        {
            ESXRecordT raceOrBthSgn = record.get();

            raceOrBthSgn.mPowers.mList =
                static_cast<const NestedTableWrapper<std::vector<std::string> >&>(nestedTable).mNestedTable;

            record.setModified (raceOrBthSgn);
        }

        virtual NestedTableWrapperBase* table(const Record<ESXRecordT>& record) const
        {
            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::vector<std::string> >(record.get().mPowers.mList);
        }

        virtual QVariant getData(const Record<ESXRecordT>& record, int subRowIndex, int subColIndex) const
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

        virtual void setData(Record<ESXRecordT>& record, const QVariant& value,
                                    int subRowIndex, int subColIndex) const
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

        virtual int getColumnsCount(const Record<ESXRecordT>& record) const
        {
            return 1;
        }

        virtual int getRowsCount(const Record<ESXRecordT>& record) const
        {
            return static_cast<int>(record.get().mPowers.mList.size());
        }
    };

    template<typename ESXRecordT>
    class EffectsListAdapter : public NestedColumnAdapter<ESXRecordT>
    {
    public:
        EffectsListAdapter () {}

        virtual void addRow(Record<ESXRecordT>& record, int position) const
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

        virtual void removeRow(Record<ESXRecordT>& record, int rowToRemove) const
        {
            ESXRecordT magic = record.get();

            std::vector<ESM::ENAMstruct>& effectsList = magic.mEffects.mList;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (effectsList.size()))
                throw std::runtime_error ("index out of range");

            effectsList.erase(effectsList.begin()+rowToRemove);

            record.setModified (magic);
        }

        virtual void setTable(Record<ESXRecordT>& record, const NestedTableWrapperBase& nestedTable) const
        {
            ESXRecordT magic = record.get();

            magic.mEffects.mList =
                static_cast<const NestedTableWrapper<std::vector<ESM::ENAMstruct> >&>(nestedTable).mNestedTable;

            record.setModified (magic);
        }

        virtual NestedTableWrapperBase* table(const Record<ESXRecordT>& record) const
        {
            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::vector<ESM::ENAMstruct> >(record.get().mEffects.mList);
        }

        virtual QVariant getData(const Record<ESXRecordT>& record, int subRowIndex, int subColIndex) const
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

        virtual void setData(Record<ESXRecordT>& record, const QVariant& value,
                                    int subRowIndex, int subColIndex) const
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

        virtual int getColumnsCount(const Record<ESXRecordT>& record) const
        {
            return 8;
        }

        virtual int getRowsCount(const Record<ESXRecordT>& record) const
        {
            return static_cast<int>(record.get().mEffects.mList.size());
        }
    };

    class InfoListAdapter : public NestedColumnAdapter<Info>
    {
    public:
        InfoListAdapter ();

        virtual void addRow(Record<Info>& record, int position) const;

        virtual void removeRow(Record<Info>& record, int rowToRemove) const;

        virtual void setTable(Record<Info>& record,
                const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* table(const Record<Info>& record) const;

        virtual QVariant getData(const Record<Info>& record,
                int subRowIndex, int subColIndex) const;

        virtual void setData(Record<Info>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getColumnsCount(const Record<Info>& record) const;

        virtual int getRowsCount(const Record<Info>& record) const;
    };

    class InfoConditionAdapter : public NestedColumnAdapter<Info>
    {
    public:
        InfoConditionAdapter ();

        virtual void addRow(Record<Info>& record, int position) const;

        virtual void removeRow(Record<Info>& record, int rowToRemove) const;

        virtual void setTable(Record<Info>& record,
                const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* table(const Record<Info>& record) const;

        virtual QVariant getData(const Record<Info>& record,
                int subRowIndex, int subColIndex) const;

        virtual void setData(Record<Info>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getColumnsCount(const Record<Info>& record) const;

        virtual int getRowsCount(const Record<Info>& record) const;
    };

    class RaceAttributeAdapter : public NestedColumnAdapter<ESM::Race>
    {
    public:
        RaceAttributeAdapter ();

        virtual void addRow(Record<ESM::Race>& record, int position) const;

        virtual void removeRow(Record<ESM::Race>& record, int rowToRemove) const;

        virtual void setTable(Record<ESM::Race>& record,
                const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* table(const Record<ESM::Race>& record) const;

        virtual QVariant getData(const Record<ESM::Race>& record,
                int subRowIndex, int subColIndex) const;

        virtual void setData(Record<ESM::Race>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getColumnsCount(const Record<ESM::Race>& record) const;

        virtual int getRowsCount(const Record<ESM::Race>& record) const;
    };

    class RaceSkillsBonusAdapter : public NestedColumnAdapter<ESM::Race>
    {
    public:
        RaceSkillsBonusAdapter ();

        virtual void addRow(Record<ESM::Race>& record, int position) const;

        virtual void removeRow(Record<ESM::Race>& record, int rowToRemove) const;

        virtual void setTable(Record<ESM::Race>& record,
                const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* table(const Record<ESM::Race>& record) const;

        virtual QVariant getData(const Record<ESM::Race>& record,
                int subRowIndex, int subColIndex) const;

        virtual void setData(Record<ESM::Race>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getColumnsCount(const Record<ESM::Race>& record) const;

        virtual int getRowsCount(const Record<ESM::Race>& record) const;
    };

    class CellListAdapter : public NestedColumnAdapter<CSMWorld::Cell>
    {
    public:
        CellListAdapter ();

        virtual void addRow(Record<CSMWorld::Cell>& record, int position) const;

        virtual void removeRow(Record<CSMWorld::Cell>& record, int rowToRemove) const;

        virtual void setTable(Record<CSMWorld::Cell>& record,
                const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* table(const Record<CSMWorld::Cell>& record) const;

        virtual QVariant getData(const Record<CSMWorld::Cell>& record,
                int subRowIndex, int subColIndex) const;

        virtual void setData(Record<CSMWorld::Cell>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getColumnsCount(const Record<CSMWorld::Cell>& record) const;

        virtual int getRowsCount(const Record<CSMWorld::Cell>& record) const;
    };

    class RegionWeatherAdapter : public NestedColumnAdapter<ESM::Region>
    {
    public:
        RegionWeatherAdapter ();

        virtual void addRow(Record<ESM::Region>& record, int position) const;

        virtual void removeRow(Record<ESM::Region>& record, int rowToRemove) const;

        virtual void setTable(Record<ESM::Region>& record,
                const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* table(const Record<ESM::Region>& record) const;

        virtual QVariant getData(const Record<ESM::Region>& record,
                int subRowIndex, int subColIndex) const;

        virtual void setData(Record<ESM::Region>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getColumnsCount(const Record<ESM::Region>& record) const;

        virtual int getRowsCount(const Record<ESM::Region>& record) const;
    };
}

#endif // CSM_WOLRD_NESTEDCOLADAPTERIMP_H
