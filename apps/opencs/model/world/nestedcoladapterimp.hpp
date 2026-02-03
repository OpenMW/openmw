#ifndef CSM_WOLRD_NESTEDCOLADAPTERIMP_H
#define CSM_WOLRD_NESTEDCOLADAPTERIMP_H

#include <QString>
#include <QVariant>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include <components/esm/attr.hpp>
#include <components/esm3/effectlist.hpp>
#include <components/esm3/loadmgef.hpp> // for converting magic effect id to string & back
#include <components/esm3/loadskil.hpp>

#include "idcollection.hpp"
#include "nestedcolumnadapter.hpp"
#include "nestedtablewrapper.hpp"

namespace ESM
{
    struct Faction;
    struct Region;
    struct Race;
}

namespace CSMWorld
{
    struct Pathgrid;
    struct Info;
    struct Cell;

    template <typename ESXRecordT>
    struct Record;

    class PathgridPointListAdapter : public NestedColumnAdapter<Pathgrid>
    {
    public:
        PathgridPointListAdapter() = default;

        void addRow(Record<Pathgrid>& record, int position) const override;

        void removeRow(Record<Pathgrid>& record, int rowToRemove) const override;

        void setTable(Record<Pathgrid>& record, const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<Pathgrid>& record) const override;

        QVariant getData(const Record<Pathgrid>& record, int subRowIndex, int subColIndex) const override;

        void setData(Record<Pathgrid>& record, const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<Pathgrid>& record) const override;

        int getRowsCount(const Record<Pathgrid>& record) const override;
    };

    class PathgridEdgeListAdapter : public NestedColumnAdapter<Pathgrid>
    {
    public:
        PathgridEdgeListAdapter() = default;

        void addRow(Record<Pathgrid>& record, int position) const override;

        void removeRow(Record<Pathgrid>& record, int rowToRemove) const override;

        void setTable(Record<Pathgrid>& record, const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<Pathgrid>& record) const override;

        QVariant getData(const Record<Pathgrid>& record, int subRowIndex, int subColIndex) const override;

        void setData(Record<Pathgrid>& record, const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<Pathgrid>& record) const override;

        int getRowsCount(const Record<Pathgrid>& record) const override;
    };

    class FactionReactionsAdapter : public NestedColumnAdapter<ESM::Faction>
    {
    public:
        FactionReactionsAdapter() = default;

        void addRow(Record<ESM::Faction>& record, int position) const override;

        void removeRow(Record<ESM::Faction>& record, int rowToRemove) const override;

        void setTable(Record<ESM::Faction>& record, const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<ESM::Faction>& record) const override;

        QVariant getData(const Record<ESM::Faction>& record, int subRowIndex, int subColIndex) const override;

        void setData(
            Record<ESM::Faction>& record, const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<ESM::Faction>& record) const override;

        int getRowsCount(const Record<ESM::Faction>& record) const override;
    };

    class FactionRanksAdapter : public NestedColumnAdapter<ESM::Faction>
    {
    public:
        FactionRanksAdapter() = default;

        void addRow(Record<ESM::Faction>& record, int position) const override;

        void removeRow(Record<ESM::Faction>& record, int rowToRemove) const override;

        void setTable(Record<ESM::Faction>& record, const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<ESM::Faction>& record) const override;

        QVariant getData(const Record<ESM::Faction>& record, int subRowIndex, int subColIndex) const override;

        void setData(
            Record<ESM::Faction>& record, const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<ESM::Faction>& record) const override;

        int getRowsCount(const Record<ESM::Faction>& record) const override;
    };

    class RegionSoundListAdapter : public NestedColumnAdapter<ESM::Region>
    {
    public:
        RegionSoundListAdapter() = default;

        void addRow(Record<ESM::Region>& record, int position) const override;

        void removeRow(Record<ESM::Region>& record, int rowToRemove) const override;

        void setTable(Record<ESM::Region>& record, const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<ESM::Region>& record) const override;

        QVariant getData(const Record<ESM::Region>& record, int subRowIndex, int subColIndex) const override;

        void setData(
            Record<ESM::Region>& record, const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<ESM::Region>& record) const override;

        int getRowsCount(const Record<ESM::Region>& record) const override;
    };

    template <typename ESXRecordT>
    class SpellListAdapter : public NestedColumnAdapter<ESXRecordT>
    {
    public:
        SpellListAdapter() = default;

        void addRow(Record<ESXRecordT>& record, int position) const override
        {
            ESXRecordT raceOrBthSgn = record.get();

            std::vector<ESM::RefId>& spells = raceOrBthSgn.mPowers.mList;

            // blank row
            ESM::RefId spell;

            spells.insert(spells.begin() + position, spell);

            record.setModified(raceOrBthSgn);
        }

        void removeRow(Record<ESXRecordT>& record, int rowToRemove) const override
        {
            ESXRecordT raceOrBthSgn = record.get();

            std::vector<ESM::RefId>& spells = raceOrBthSgn.mPowers.mList;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int>(spells.size()))
                throw std::runtime_error("index out of range");

            spells.erase(spells.begin() + rowToRemove);

            record.setModified(raceOrBthSgn);
        }

        void setTable(Record<ESXRecordT>& record, const NestedTableWrapperBase& nestedTable) const override
        {
            ESXRecordT raceOrBthSgn = record.get();

            raceOrBthSgn.mPowers.mList
                = static_cast<const NestedTableWrapper<std::vector<ESM::RefId>>&>(nestedTable).mNestedTable;

            record.setModified(raceOrBthSgn);
        }

        NestedTableWrapperBase* table(const Record<ESXRecordT>& record) const override
        {
            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::vector<ESM::RefId>>(record.get().mPowers.mList);
        }

        QVariant getData(const Record<ESXRecordT>& record, int subRowIndex, int subColIndex) const override
        {
            ESXRecordT raceOrBthSgn = record.get();

            std::vector<ESM::RefId>& spells = raceOrBthSgn.mPowers.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int>(spells.size()))
                throw std::runtime_error("index out of range");

            ESM::RefId spell = spells[subRowIndex];
            switch (subColIndex)
            {
                case 0:
                    return QString(spell.getRefIdString().c_str());
                default:
                    throw std::runtime_error("Spells subcolumn index out of range");
            }
        }

        void setData(Record<ESXRecordT>& record, const QVariant& value, int subRowIndex, int subColIndex) const override
        {
            ESXRecordT raceOrBthSgn = record.get();

            std::vector<ESM::RefId>& spells = raceOrBthSgn.mPowers.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int>(spells.size()))
                throw std::runtime_error("index out of range");

            ESM::RefId spell = spells[subRowIndex];
            switch (subColIndex)
            {
                case 0:
                    spell = ESM::RefId::stringRefId(value.toString().toUtf8().constData());
                    break;
                default:
                    throw std::runtime_error("Spells subcolumn index out of range");
            }

            raceOrBthSgn.mPowers.mList[subRowIndex] = spell;

            record.setModified(raceOrBthSgn);
        }

        int getColumnsCount(const Record<ESXRecordT>& record) const override { return 1; }

        int getRowsCount(const Record<ESXRecordT>& record) const override
        {
            return static_cast<int>(record.get().mPowers.mList.size());
        }
    };

    template <typename ESXRecordT>
    class EffectsListAdapter : public NestedColumnAdapter<ESXRecordT>
    {
        const IdCollection<ESM::MagicEffect>& mMagicEffects;

    public:
        EffectsListAdapter(const IdCollection<ESM::MagicEffect>& magicEffects)
            : mMagicEffects(magicEffects)
        {
        }

        void addRow(Record<ESXRecordT>& record, int position) const override
        {
            ESXRecordT magic = record.get();

            std::vector<ESM::IndexedENAMstruct>& effectsList = magic.mEffects.mList;

            // blank row
            ESM::IndexedENAMstruct effect;
            effect.mIndex = position;
            effect.mData.mEffectID = ESM::MagicEffect::WaterBreathing;
            effect.mData.mSkill = ESM::RefId();
            effect.mData.mAttribute = ESM::RefId();
            effect.mData.mRange = 0;
            effect.mData.mArea = 0;
            effect.mData.mDuration = 0;
            effect.mData.mMagnMin = 0;
            effect.mData.mMagnMax = 0;

            effectsList.insert(effectsList.begin() + position, effect);
            magic.mEffects.updateIndexes();

            record.setModified(magic);
        }

        void removeRow(Record<ESXRecordT>& record, int rowToRemove) const override
        {
            ESXRecordT magic = record.get();

            std::vector<ESM::IndexedENAMstruct>& effectsList = magic.mEffects.mList;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int>(effectsList.size()))
                throw std::runtime_error("index out of range");

            effectsList.erase(effectsList.begin() + rowToRemove);
            magic.mEffects.updateIndexes();

            record.setModified(magic);
        }

        void setTable(Record<ESXRecordT>& record, const NestedTableWrapperBase& nestedTable) const override
        {
            ESXRecordT magic = record.get();

            magic.mEffects.mList
                = static_cast<const NestedTableWrapper<std::vector<ESM::IndexedENAMstruct>>&>(nestedTable).mNestedTable;

            record.setModified(magic);
        }

        NestedTableWrapperBase* table(const Record<ESXRecordT>& record) const override
        {
            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::vector<ESM::IndexedENAMstruct>>(record.get().mEffects.mList);
        }

        QVariant getData(const Record<ESXRecordT>& record, int subRowIndex, int subColIndex) const override
        {
            ESXRecordT magic = record.get();

            std::vector<ESM::IndexedENAMstruct>& effectsList = magic.mEffects.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int>(effectsList.size()))
                throw std::runtime_error("index out of range");

            ESM::ENAMstruct effect = effectsList[subRowIndex].mData;
            bool targetSkill = false, targetAttribute = false;

            int recordIndex = mMagicEffects.searchId(effect.mEffectID);
            if (recordIndex != -1)
            {
                const ESM::MagicEffect& mgef = mMagicEffects.getRecord(recordIndex).get();
                targetSkill = mgef.mData.mFlags & ESM::MagicEffect::TargetSkill;
                targetAttribute = mgef.mData.mFlags & ESM::MagicEffect::TargetAttribute;
            }

            switch (subColIndex)
            {
                case 0:
                    return ESM::MagicEffect::refIdToIndex(effect.mEffectID);
                case 1:
                {
                    if (targetSkill)
                        return ESM::Skill::refIdToIndex(effect.mSkill);
                    else
                        return QVariant();
                }
                case 2:
                {
                    if (targetAttribute)
                        return ESM::Attribute::refIdToIndex(effect.mAttribute);
                    else
                        return QVariant();
                }
                case 3:
                    return effect.mRange;
                case 4:
                    return effect.mArea;
                case 5:
                    return effect.mDuration;
                case 6:
                    return effect.mMagnMin;
                case 7:
                    return effect.mMagnMax;
                default:
                    throw std::runtime_error("Magic Effects subcolumn index out of range");
            }
        }

        void setData(Record<ESXRecordT>& record, const QVariant& value, int subRowIndex, int subColIndex) const override
        {
            ESXRecordT magic = record.get();

            std::vector<ESM::IndexedENAMstruct>& effectsList = magic.mEffects.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int>(effectsList.size()))
                throw std::runtime_error("index out of range");

            ESM::ENAMstruct effect = effectsList[subRowIndex].mData;
            switch (subColIndex)
            {
                case 0:
                {
                    bool targetSkill = false, targetAttribute = false;
                    effect.mEffectID = ESM::MagicEffect::indexToRefId(value.toInt());
                    int recordIndex = mMagicEffects.searchId(effect.mEffectID);
                    if (recordIndex != -1)
                    {
                        const ESM::MagicEffect& mgef = mMagicEffects.getRecord(recordIndex).get();
                        targetSkill = mgef.mData.mFlags & ESM::MagicEffect::TargetSkill;
                        targetAttribute = mgef.mData.mFlags & ESM::MagicEffect::TargetAttribute;
                    }
                    if (!targetSkill)
                        effect.mSkill = ESM::RefId();
                    if (!targetAttribute)
                        effect.mAttribute = ESM::RefId();
                    break;
                }
                case 1:
                {
                    effect.mSkill = ESM::Skill::indexToRefId(value.toInt());
                    break;
                }
                case 2:
                {
                    effect.mAttribute = ESM::Attribute::indexToRefId(value.toInt());
                    break;
                }
                case 3:
                {
                    effect.mRange = value.toInt();
                    break;
                }
                case 4:
                    effect.mArea = value.toInt();
                    break;
                case 5:
                    effect.mDuration = value.toInt();
                    break;
                case 6:
                    effect.mMagnMin = value.toInt();
                    break;
                case 7:
                    effect.mMagnMax = value.toInt();
                    break;
                default:
                    throw std::runtime_error("Magic Effects subcolumn index out of range");
            }

            magic.mEffects.mList[subRowIndex].mData = effect;

            record.setModified(magic);
        }

        int getColumnsCount(const Record<ESXRecordT>& record) const override { return 8; }

        int getRowsCount(const Record<ESXRecordT>& record) const override
        {
            return static_cast<int>(record.get().mEffects.mList.size());
        }
    };

    class InfoListAdapter : public NestedColumnAdapter<Info>
    {
    public:
        InfoListAdapter() = default;

        void addRow(Record<Info>& record, int position) const override;

        void removeRow(Record<Info>& record, int rowToRemove) const override;

        void setTable(Record<Info>& record, const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<Info>& record) const override;

        QVariant getData(const Record<Info>& record, int subRowIndex, int subColIndex) const override;

        void setData(Record<Info>& record, const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<Info>& record) const override;

        int getRowsCount(const Record<Info>& record) const override;
    };

    class InfoConditionAdapter : public NestedColumnAdapter<Info>
    {
    public:
        InfoConditionAdapter() = default;

        void addRow(Record<Info>& record, int position) const override;

        void removeRow(Record<Info>& record, int rowToRemove) const override;

        void setTable(Record<Info>& record, const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<Info>& record) const override;

        QVariant getData(const Record<Info>& record, int subRowIndex, int subColIndex) const override;

        void setData(Record<Info>& record, const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<Info>& record) const override;

        int getRowsCount(const Record<Info>& record) const override;
    };

    class RaceAttributeAdapter : public NestedColumnAdapter<ESM::Race>
    {
    public:
        RaceAttributeAdapter() = default;

        void addRow(Record<ESM::Race>& record, int position) const override;

        void removeRow(Record<ESM::Race>& record, int rowToRemove) const override;

        void setTable(Record<ESM::Race>& record, const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<ESM::Race>& record) const override;

        QVariant getData(const Record<ESM::Race>& record, int subRowIndex, int subColIndex) const override;

        void setData(Record<ESM::Race>& record, const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<ESM::Race>& record) const override;

        int getRowsCount(const Record<ESM::Race>& record) const override;
    };

    class RaceSkillsBonusAdapter : public NestedColumnAdapter<ESM::Race>
    {
    public:
        RaceSkillsBonusAdapter() = default;

        void addRow(Record<ESM::Race>& record, int position) const override;

        void removeRow(Record<ESM::Race>& record, int rowToRemove) const override;

        void setTable(Record<ESM::Race>& record, const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<ESM::Race>& record) const override;

        QVariant getData(const Record<ESM::Race>& record, int subRowIndex, int subColIndex) const override;

        void setData(Record<ESM::Race>& record, const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<ESM::Race>& record) const override;

        int getRowsCount(const Record<ESM::Race>& record) const override;
    };

    class CellListAdapter : public NestedColumnAdapter<CSMWorld::Cell>
    {
    public:
        CellListAdapter() = default;

        void addRow(Record<CSMWorld::Cell>& record, int position) const override;

        void removeRow(Record<CSMWorld::Cell>& record, int rowToRemove) const override;

        void setTable(Record<CSMWorld::Cell>& record, const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<CSMWorld::Cell>& record) const override;

        QVariant getData(const Record<CSMWorld::Cell>& record, int subRowIndex, int subColIndex) const override;

        void setData(
            Record<CSMWorld::Cell>& record, const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<CSMWorld::Cell>& record) const override;

        int getRowsCount(const Record<CSMWorld::Cell>& record) const override;
    };

    class RegionWeatherAdapter : public NestedColumnAdapter<ESM::Region>
    {
    public:
        RegionWeatherAdapter() = default;

        void addRow(Record<ESM::Region>& record, int position) const override;

        void removeRow(Record<ESM::Region>& record, int rowToRemove) const override;

        void setTable(Record<ESM::Region>& record, const NestedTableWrapperBase& nestedTable) const override;

        NestedTableWrapperBase* table(const Record<ESM::Region>& record) const override;

        QVariant getData(const Record<ESM::Region>& record, int subRowIndex, int subColIndex) const override;

        void setData(
            Record<ESM::Region>& record, const QVariant& value, int subRowIndex, int subColIndex) const override;

        int getColumnsCount(const Record<ESM::Region>& record) const override;

        int getRowsCount(const Record<ESM::Region>& record) const override;
    };
}

#endif // CSM_WOLRD_NESTEDCOLADAPTERIMP_H
