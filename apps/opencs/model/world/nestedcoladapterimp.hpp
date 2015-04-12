#ifndef CSM_WOLRD_NESTEDCOLADAPTERIMP_H
#define CSM_WOLRD_NESTEDCOLADAPTERIMP_H

#include <QVariant>

#include <components/esm/loadpgrd.hpp>
#include <components/esm/effectlist.hpp>
#include <components/esm/loadmgef.hpp> // for converting magic effect id to string & back
#include <components/esm/loadskil.hpp> // for converting skill names
#include <components/esm/attr.hpp>     // for converting attributes

#include "nestedcolumnadapter.hpp"
#include "nestedtablewrapper.hpp"

namespace ESM
{
    struct Faction;
    struct Region;
}

namespace CSMWorld
{
    struct Pathgrid;

    struct PathgridPointsWrap : public NestedTableWrapperBase
    {
        ESM::Pathgrid mRecord;

        PathgridPointsWrap(ESM::Pathgrid pathgrid)
            : mRecord(pathgrid) {}

        virtual ~PathgridPointsWrap() {}

        virtual int size() const
        {
            return mRecord.mPoints.size(); // used in IdTree::setNestedTable()
        }
    };

    class PathgridPointListAdapter : public NestedColumnAdapter<Pathgrid>
    {
    public:
        PathgridPointListAdapter ();

        virtual void addNestedRow(Record<Pathgrid>& record, int position) const;

        virtual void removeNestedRow(Record<Pathgrid>& record, int rowToRemove) const;

        virtual void setNestedTable(Record<Pathgrid>& record,
                const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* nestedTable(const Record<Pathgrid>& record) const;

        virtual QVariant getNestedData(const Record<Pathgrid>& record,
                int subRowIndex, int subColIndex) const;

        virtual void setNestedData(Record<Pathgrid>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getNestedColumnsCount(const Record<Pathgrid>& record) const;

        virtual int getNestedRowsCount(const Record<Pathgrid>& record) const;
    };

    class PathgridEdgeListAdapter : public NestedColumnAdapter<Pathgrid>
    {
    public:
        PathgridEdgeListAdapter ();

        virtual void addNestedRow(Record<Pathgrid>& record, int position) const;

        virtual void removeNestedRow(Record<Pathgrid>& record, int rowToRemove) const;

        virtual void setNestedTable(Record<Pathgrid>& record,
                const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* nestedTable(const Record<Pathgrid>& record) const;

        virtual QVariant getNestedData(const Record<Pathgrid>& record,
                int subRowIndex, int subColIndex) const;

        virtual void setNestedData(Record<Pathgrid>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getNestedColumnsCount(const Record<Pathgrid>& record) const;

        virtual int getNestedRowsCount(const Record<Pathgrid>& record) const;
    };

    class FactionReactionsAdapter : public NestedColumnAdapter<ESM::Faction>
    {
    public:
        FactionReactionsAdapter ();

        virtual void addNestedRow(Record<ESM::Faction>& record, int position) const;

        virtual void removeNestedRow(Record<ESM::Faction>& record, int rowToRemove) const;

        virtual void setNestedTable(Record<ESM::Faction>& record,
                const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* nestedTable(const Record<ESM::Faction>& record) const;

        virtual QVariant getNestedData(const Record<ESM::Faction>& record,
                int subRowIndex, int subColIndex) const;

        virtual void setNestedData(Record<ESM::Faction>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getNestedColumnsCount(const Record<ESM::Faction>& record) const;

        virtual int getNestedRowsCount(const Record<ESM::Faction>& record) const;
    };

    class RegionSoundListAdapter : public NestedColumnAdapter<ESM::Region>
    {
    public:
        RegionSoundListAdapter ();

        virtual void addNestedRow(Record<ESM::Region>& record, int position) const;

        virtual void removeNestedRow(Record<ESM::Region>& record, int rowToRemove) const;

        virtual void setNestedTable(Record<ESM::Region>& record,
                const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* nestedTable(const Record<ESM::Region>& record) const;

        virtual QVariant getNestedData(const Record<ESM::Region>& record,
                int subRowIndex, int subColIndex) const;

        virtual void setNestedData(Record<ESM::Region>& record,
                const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getNestedColumnsCount(const Record<ESM::Region>& record) const;

        virtual int getNestedRowsCount(const Record<ESM::Region>& record) const;
    };

    template<typename ESXRecordT>
    class SpellListAdapter : public NestedColumnAdapter<ESXRecordT>
    {
    public:
        SpellListAdapter () {}

        virtual void addNestedRow(Record<ESXRecordT>& record, int position) const
        {
            ESXRecordT raceOrBthSgn = record.get();

            std::vector<std::string>& spells = raceOrBthSgn.mPowers.mList;

            // blank row
            std::string spell = "";

            spells.insert(spells.begin()+position, spell);

            record.setModified (raceOrBthSgn);
        }

        virtual void removeNestedRow(Record<ESXRecordT>& record, int rowToRemove) const
        {
            ESXRecordT raceOrBthSgn = record.get();

            std::vector<std::string>& spells = raceOrBthSgn.mPowers.mList;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (spells.size()))
                throw std::runtime_error ("index out of range");

            spells.erase(spells.begin()+rowToRemove);

            record.setModified (raceOrBthSgn);
        }

        virtual void setNestedTable(Record<ESXRecordT>& record, const NestedTableWrapperBase& nestedTable) const
        {
            ESXRecordT raceOrBthSgn = record.get();

           raceOrBthSgn.mPowers.mList =
                static_cast<const NestedTableWrapper<std::vector<std::string> >&>(nestedTable).mNestedTable;

            record.setModified (raceOrBthSgn);
        }

        virtual NestedTableWrapperBase* nestedTable(const Record<ESXRecordT>& record) const
        {
            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::vector<std::string> >(record.get().mPowers.mList);
        }

        virtual QVariant getNestedData(const Record<ESXRecordT>& record, int subRowIndex, int subColIndex) const
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

        virtual void setNestedData(Record<ESXRecordT>& record, const QVariant& value,
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

        virtual int getNestedColumnsCount(const Record<ESXRecordT>& record) const
        {
            return 1;
        }

        virtual int getNestedRowsCount(const Record<ESXRecordT>& record) const
        {
            return static_cast<int>(record.get().mPowers.mList.size());
        }
    };

    template<typename ESXRecordT>
    class EffectsListAdapter : public NestedColumnAdapter<ESXRecordT>
    {
    public:
        EffectsListAdapter () {}

        virtual void addNestedRow(Record<ESXRecordT>& record, int position) const
        {
            ESXRecordT magic = record.get();

            std::vector<ESM::ENAMstruct>& effectsList = magic.mEffects.mList;

            // blank row
            ESM::ENAMstruct effect;
            effect.mEffectID = 0;
            effect.mSkill = 0;
            effect.mAttribute = 0;
            effect.mRange = 0;
            effect.mArea = 0;
            effect.mDuration = 0;
            effect.mMagnMin = 0;
            effect.mMagnMax = 0;

            effectsList.insert(effectsList.begin()+position, effect);

            record.setModified (magic);
        }

        virtual void removeNestedRow(Record<ESXRecordT>& record, int rowToRemove) const
        {
            ESXRecordT magic = record.get();

            std::vector<ESM::ENAMstruct>& effectsList = magic.mEffects.mList;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (effectsList.size()))
                throw std::runtime_error ("index out of range");

            effectsList.erase(effectsList.begin()+rowToRemove);

            record.setModified (magic);
        }

        virtual void setNestedTable(Record<ESXRecordT>& record, const NestedTableWrapperBase& nestedTable) const
        {
            ESXRecordT magic = record.get();

            magic.mEffects.mList =
                static_cast<const NestedTableWrapper<std::vector<ESM::ENAMstruct> >&>(nestedTable).mNestedTable;

            record.setModified (magic);
        }

        virtual NestedTableWrapperBase* nestedTable(const Record<ESXRecordT>& record) const
        {
            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::vector<ESM::ENAMstruct> >(record.get().mEffects.mList);
        }

        virtual QVariant getNestedData(const Record<ESXRecordT>& record, int subRowIndex, int subColIndex) const
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
                    // indexToId() prepends "#d+" hence not so user friendly
                    QString effectId(ESM::MagicEffect::effectIdToString(effect.mEffectID).c_str());
                    return effectId.remove(0, 7); // 7 == sizeof("sEffect") - 1
                }
                case 1:
                {
                    switch (effect.mSkill)
                    {
                        // see ESM::Skill::SkillEnum in <component/esm/loadskil.hpp>
                        case ESM::Skill::Block:
                        case ESM::Skill::Armorer:
                        case ESM::Skill::MediumArmor:
                        case ESM::Skill::HeavyArmor:
                        case ESM::Skill::BluntWeapon:
                        case ESM::Skill::LongBlade:
                        case ESM::Skill::Axe:
                        case ESM::Skill::Spear:
                        case ESM::Skill::Athletics:
                        case ESM::Skill::Enchant:
                        case ESM::Skill::Destruction:
                        case ESM::Skill::Alteration:
                        case ESM::Skill::Illusion:
                        case ESM::Skill::Conjuration:
                        case ESM::Skill::Mysticism:
                        case ESM::Skill::Restoration:
                        case ESM::Skill::Alchemy:
                        case ESM::Skill::Unarmored:
                        case ESM::Skill::Security:
                        case ESM::Skill::Sneak:
                        case ESM::Skill::Acrobatics:
                        case ESM::Skill::LightArmor:
                        case ESM::Skill::ShortBlade:
                        case ESM::Skill::Marksman:
                        case ESM::Skill::Mercantile:
                        case ESM::Skill::Speechcraft:
                        case ESM::Skill::HandToHand:
                        {
                            return QString(ESM::Skill::sSkillNames[effect.mSkill].c_str());
                        }
                        case -1: return QString("N/A");
                        default: return QVariant();
                    }
                }
                case 2:
                {
                    switch (effect.mAttribute)
                    {
                        // see ESM::Attribute::AttributeID in <component/esm/attr.hpp>
                        case ESM::Attribute::Strength:
                        case ESM::Attribute::Intelligence:
                        case ESM::Attribute::Willpower:
                        case ESM::Attribute::Agility:
                        case ESM::Attribute::Speed:
                        case ESM::Attribute::Endurance:
                        case ESM::Attribute::Personality:
                        case ESM::Attribute::Luck:
                        {
                            return QString(ESM::Attribute::sAttributeNames[effect.mAttribute].c_str());
                        }
                        case -1: return QString("N/A");
                        default: return QVariant();
                    }
                }
                case 3:
                {
                    switch (effect.mRange)
                    {
                        // see ESM::RangeType in <component/esm/defs.hpp>
                        case ESM::RT_Self: return QString("Self");
                        case ESM::RT_Touch: return QString("Touch");
                        case ESM::RT_Target: return QString("Target");
                        default: return QVariant();
                    }
                }
                case 4: return effect.mArea;
                case 5: return effect.mDuration;
                case 6: return effect.mMagnMin;
                case 7: return effect.mMagnMax;
                default: throw std::runtime_error("Magic Effects subcolumn index out of range");
            }
        }

        virtual void setNestedData(Record<ESXRecordT>& record, const QVariant& value,
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
                    effect.mEffectID =
                        ESM::MagicEffect::effectStringToId("sEffect"+value.toString().toStdString());
                    break;
                }
                case 1:
                {
                    std::string skillName = value.toString().toStdString();
                    if ("N/A" == skillName)
                    {
                        effect.mSkill = -1;
                        break;
                    }

                    for (unsigned int i = 0; i < ESM::Skill::Length; ++i)
                    {
                        if (ESM::Skill::sSkillNames[i] == skillName)
                        {
                            effect.mSkill = static_cast<signed char>(i);
                            break;
                        }
                    }
                    break;
                }
                case 2:
                {
                    std::string attr = value.toString().toStdString();
                    if ("N/A" == attr)
                    {
                        effect.mAttribute = -1;
                        break;
                    }

                    for (unsigned int i = 0; i < ESM::Attribute::Length; ++i)
                    {
                        if (ESM::Attribute::sAttributeNames[i] == attr)
                        {
                            effect.mAttribute = static_cast<signed char>(i);
                            break;
                        }
                    }
                    break;
                }
                case 3:
                {
                    std::string effectId = value.toString().toStdString();
                    if (effectId == "Self")
                        effect.mRange = ESM::RT_Self;
                    else if (effectId == "Touch")
                        effect.mRange = ESM::RT_Touch;
                    else if (effectId == "Target")
                        effect.mRange = ESM::RT_Target;
                    // else leave unchanged
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

        virtual int getNestedColumnsCount(const Record<ESXRecordT>& record) const
        {
            return 8;
        }

        virtual int getNestedRowsCount(const Record<ESXRecordT>& record) const
        {
            return static_cast<int>(record.get().mEffects.mList.size());
        }
    };
}

#endif // CSM_WOLRD_NESTEDCOLADAPTERIMP_H
