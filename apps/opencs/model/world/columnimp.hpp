#ifndef CSM_WOLRD_COLUMNIMP_H
#define CSM_WOLRD_COLUMNIMP_H

#include <cassert>
#include <cstdint>
#include <sstream>
#include <stdexcept>

#include <QColor>
#include <QVector>

#include <components/esm/loadbody.hpp>
#include <components/esm/loadskil.hpp>
#include <components/esm/loadrace.hpp>

#include "columnbase.hpp"
#include "columns.hpp"
#include "info.hpp"

#include "land.hpp"
#include "landtexture.hpp"

namespace CSMWorld
{
    /// \note Shares ID with VarValueColumn. A table can not have both.
    template<typename ESXRecordT>
    struct FloatValueColumn : public Column<ESXRecordT>
    {
        FloatValueColumn() : Column<ESXRecordT> (Columns::ColumnId_Value, ColumnBase::Display_Float) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mValue.getFloat();
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();
            record2.mValue.setFloat (data.toFloat());
            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct StringIdColumn : public Column<ESXRecordT>
    {
        StringIdColumn (bool hidden = false)
        : Column<ESXRecordT> (Columns::ColumnId_Id, ColumnBase::Display_Id,
            hidden ? 0 : ColumnBase::Flag_Table | ColumnBase::Flag_Dialogue)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mId.c_str());
        }

        virtual bool isEditable() const
        {
            return false;
        }
    };

    template<>
    inline QVariant StringIdColumn<Land>::get(const Record<Land>& record) const
    {
        const Land& land = record.get();
        return QString::fromUtf8(Land::createUniqueRecordId(land.mX, land.mY).c_str());
    }

    template<>
    inline QVariant StringIdColumn<LandTexture>::get(const Record<LandTexture>& record) const
    {
        const LandTexture& ltex = record.get();
        return QString::fromUtf8(LandTexture::createUniqueRecordId(ltex.mPluginIndex, ltex.mIndex).c_str());
    }

    template<typename ESXRecordT>
    struct RecordStateColumn : public Column<ESXRecordT>
    {
        RecordStateColumn()
        : Column<ESXRecordT> (Columns::ColumnId_Modification, ColumnBase::Display_RecordState)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            if (record.mState==Record<ESXRecordT>::State_Erased)
                return static_cast<int> (Record<ESXRecordT>::State_Deleted);

            return static_cast<int> (record.mState);
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            record.mState = static_cast<RecordBase::State> (data.toInt());
        }

        virtual bool isEditable() const
        {
            return true;
        }

        virtual bool isUserEditable() const
        {
            return false;
        }
    };

    template<typename ESXRecordT>
    struct FixedRecordTypeColumn : public Column<ESXRecordT>
    {
        int mType;

        FixedRecordTypeColumn (int type)
        : Column<ESXRecordT> (Columns::ColumnId_RecordType, ColumnBase::Display_Integer, 0),
          mType (type)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return mType;
        }

        virtual bool isEditable() const
        {
            return false;
        }
    };

    /// \attention A var type column must be immediately followed by a suitable value column.
    template<typename ESXRecordT>
    struct VarTypeColumn : public Column<ESXRecordT>
    {
        VarTypeColumn (ColumnBase::Display display)
            : Column<ESXRecordT> (Columns::ColumnId_ValueType, display, ColumnBase::Flag_Table | ColumnBase::Flag_Dialogue | ColumnBase::Flag_Dialogue_Refresh)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return static_cast<int> (record.get().mValue.getType());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();
            record2.mValue.setType (static_cast<ESM::VarType> (data.toInt()));
            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    /// \note Shares ID with FloatValueColumn. A table can not have both.
    template<typename ESXRecordT>
    struct VarValueColumn : public Column<ESXRecordT>
    {
        VarValueColumn() : Column<ESXRecordT> (Columns::ColumnId_Value, ColumnBase::Display_Var, ColumnBase::Flag_Table | ColumnBase::Flag_Dialogue | ColumnBase::Flag_Dialogue_Refresh) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            switch (record.get().mValue.getType())
            {
                case ESM::VT_String:

                    return QString::fromUtf8 (record.get().mValue.getString().c_str());

                case ESM::VT_Int:
                case ESM::VT_Short:
                case ESM::VT_Long:

                    return record.get().mValue.getInteger();

                case ESM::VT_Float:

                    return record.get().mValue.getFloat();

                default: return QVariant();
            }
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            switch (record2.mValue.getType())
            {
                case ESM::VT_String:

                    record2.mValue.setString (data.toString().toUtf8().constData());
                    break;

                case ESM::VT_Int:
                case ESM::VT_Short:
                case ESM::VT_Long:

                    record2.mValue.setInteger (data.toInt());
                    break;

                case ESM::VT_Float:

                    record2.mValue.setFloat (data.toFloat());
                    break;

                default: break;
            }

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct DescriptionColumn : public Column<ESXRecordT>
    {
        DescriptionColumn()
        : Column<ESXRecordT> (Columns::ColumnId_Description, ColumnBase::Display_LongString)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mDescription.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mDescription = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct SpecialisationColumn : public Column<ESXRecordT>
    {
        SpecialisationColumn()
        : Column<ESXRecordT> (Columns::ColumnId_Specialisation, ColumnBase::Display_Specialisation)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mData.mSpecialization;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mData.mSpecialization = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct UseValueColumn : public Column<ESXRecordT>
    {
        int mIndex;

        UseValueColumn (int index)
        : Column<ESXRecordT> (Columns::ColumnId_UseValue1 + index,  ColumnBase::Display_Float),
          mIndex (index)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mData.mUseValue[mIndex];
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mData.mUseValue[mIndex] = data.toFloat();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct AttributeColumn : public Column<ESXRecordT>
    {
        AttributeColumn()
        : Column<ESXRecordT> (Columns::ColumnId_Attribute, ColumnBase::Display_Attribute)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mData.mAttribute;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mData.mAttribute = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct NameColumn : public Column<ESXRecordT>
    {
        NameColumn() : Column<ESXRecordT> (Columns::ColumnId_Name, ColumnBase::Display_String) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mName.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mName = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct AttributesColumn : public Column<ESXRecordT>
    {
        int mIndex;

        AttributesColumn (int index)
        : Column<ESXRecordT> (Columns::ColumnId_Attribute1 + index, ColumnBase::Display_Attribute),
          mIndex (index)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mData.mAttribute[mIndex];
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mData.mAttribute[mIndex] = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct SkillsColumn : public Column<ESXRecordT>
    {
        int mIndex;
        bool mMajor;

        SkillsColumn (int index, bool typePrefix = false, bool major = false)
        : Column<ESXRecordT> ((typePrefix ? (
            major ? Columns::ColumnId_MajorSkill1 : Columns::ColumnId_MinorSkill1) :
            Columns::ColumnId_Skill1) + index, ColumnBase::Display_Skill),
            mIndex (index), mMajor (major)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            int skill = record.get().mData.getSkill (mIndex, mMajor);

            return QString::fromUtf8 (ESM::Skill::indexToId (skill).c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            std::istringstream stream (data.toString().toUtf8().constData());

            int index = -1;
            char c;

            stream >> c >> index;

            if (index!=-1)
            {
                ESXRecordT record2 = record.get();

                record2.mData.getSkill (mIndex, mMajor) = index;

                record.setModified (record2);
            }
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct PlayableColumn : public Column<ESXRecordT>
    {
        PlayableColumn() : Column<ESXRecordT> (Columns::ColumnId_Playable, ColumnBase::Display_Boolean)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mData.mIsPlayable!=0;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mData.mIsPlayable = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct HiddenColumn : public Column<ESXRecordT>
    {
        HiddenColumn() : Column<ESXRecordT> (Columns::ColumnId_Hidden, ColumnBase::Display_Boolean) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mData.mIsHidden!=0;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mData.mIsHidden = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct FlagColumn : public Column<ESXRecordT>
    {
        int mMask;
        bool mInverted;

        FlagColumn (int columnId, int mask,
                int flags = ColumnBase::Flag_Table | ColumnBase::Flag_Dialogue, bool inverted = false)
        : Column<ESXRecordT> (columnId, ColumnBase::Display_Boolean, flags), mMask (mask),
          mInverted (inverted)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            bool flag = (record.get().mData.mFlags & mMask)!=0;

            if (mInverted)
                flag = !flag;

            return flag;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            int flags = record2.mData.mFlags & ~mMask;

            if ((data.toInt()!=0)!=mInverted)
                flags |= mMask;

            record2.mData.mFlags = flags;

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct FlagColumn2 : public Column<ESXRecordT>
    {
        int mMask;
        bool mInverted;

        FlagColumn2 (int columnId, int mask, bool inverted = false)
        : Column<ESXRecordT> (columnId, ColumnBase::Display_Boolean), mMask (mask),
          mInverted (inverted)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            bool flag = (record.get().mFlags & mMask)!=0;

            if (mInverted)
                flag = !flag;

            return flag;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            int flags = record2.mFlags & ~mMask;

            if ((data.toInt()!=0)!=mInverted)
                flags |= mMask;

            record2.mFlags = flags;

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct WeightHeightColumn : public Column<ESXRecordT>
    {
        bool mMale;
        bool mWeight;

        WeightHeightColumn (bool male, bool weight)
        : Column<ESXRecordT> (male ?
          (weight ? Columns::ColumnId_MaleWeight : Columns::ColumnId_MaleHeight) :
          (weight ? Columns::ColumnId_FemaleWeight : Columns::ColumnId_FemaleHeight),
          ColumnBase::Display_Float),
          mMale (male), mWeight (weight)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            const ESM::Race::MaleFemaleF& value =
                mWeight ? record.get().mData.mWeight : record.get().mData.mHeight;

            return mMale ? value.mMale : value.mFemale;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            ESM::Race::MaleFemaleF& value =
                mWeight ? record2.mData.mWeight : record2.mData.mHeight;

            (mMale ? value.mMale : value.mFemale) = data.toFloat();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct SoundParamColumn : public Column<ESXRecordT>
    {
        enum Type
        {
            Type_Volume,
            Type_MinRange,
            Type_MaxRange
        };

        Type mType;

        SoundParamColumn (Type type)
        : Column<ESXRecordT> (type==Type_Volume ? Columns::ColumnId_Volume :
          (type==Type_MinRange ? Columns::ColumnId_MinRange : Columns::ColumnId_MaxRange),
          ColumnBase::Display_Integer),
          mType (type)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            int value = 0;

            switch (mType)
            {
                case Type_Volume: value = record.get().mData.mVolume; break;
                case Type_MinRange: value = record.get().mData.mMinRange; break;
                case Type_MaxRange: value = record.get().mData.mMaxRange; break;
            }

            return value;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            int value = data.toInt();

            if (value<0)
                value = 0;
            else if (value>255)
                value = 255;

            ESXRecordT record2 = record.get();

            switch (mType)
            {
                case Type_Volume: record2.mData.mVolume = static_cast<unsigned char> (value); break;
                case Type_MinRange: record2.mData.mMinRange = static_cast<unsigned char> (value); break;
                case Type_MaxRange: record2.mData.mMaxRange = static_cast<unsigned char> (value); break;
            }

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct SoundFileColumn : public Column<ESXRecordT>
    {
        SoundFileColumn()
        : Column<ESXRecordT> (Columns::ColumnId_SoundFile, ColumnBase::Display_SoundRes)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mSound.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mSound = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct MapColourColumn : public Column<ESXRecordT>
    {
        MapColourColumn()
        : Column<ESXRecordT> (Columns::ColumnId_MapColour, ColumnBase::Display_Colour)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mMapColor;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT copy = record.get();
            copy.mMapColor = data.toInt();
            record.setModified (copy);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct SleepListColumn : public Column<ESXRecordT>
    {
        SleepListColumn()
        : Column<ESXRecordT> (Columns::ColumnId_SleepEncounter, ColumnBase::Display_CreatureLevelledList)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mSleepList.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mSleepList = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct TextureColumn : public Column<ESXRecordT>
    {
        TextureColumn() : Column<ESXRecordT> (Columns::ColumnId_Texture, ColumnBase::Display_Texture) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mTexture.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mTexture = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct SpellTypeColumn : public Column<ESXRecordT>
    {
        SpellTypeColumn()
        : Column<ESXRecordT> (Columns::ColumnId_SpellType, ColumnBase::Display_SpellType)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mData.mType;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mData.mType = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct CostColumn : public Column<ESXRecordT>
    {
        CostColumn() : Column<ESXRecordT> (Columns::ColumnId_Cost, ColumnBase::Display_Integer) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mData.mCost;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();
            record2.mData.mCost = data.toInt();
            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct ScriptColumn : public Column<ESXRecordT>
    {
        enum Type
        {
            Type_File, // regular script record
            Type_Lines, // console context
            Type_Info // dialogue context (not implemented yet)
        };

        ScriptColumn (Type type)
        : Column<ESXRecordT> (Columns::ColumnId_ScriptText,
            type==Type_File ? ColumnBase::Display_ScriptFile : ColumnBase::Display_ScriptLines,
            type==Type_File ? 0 : ColumnBase::Flag_Dialogue)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mScriptText.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mScriptText = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct RegionColumn : public Column<ESXRecordT>
    {
        RegionColumn() : Column<ESXRecordT> (Columns::ColumnId_Region, ColumnBase::Display_Region) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mRegion.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mRegion = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct CellColumn : public Column<ESXRecordT>
    {
        bool mBlocked;

        /// \param blocked Do not allow user-modification
        CellColumn (bool blocked = false)
        : Column<ESXRecordT> (Columns::ColumnId_Cell, ColumnBase::Display_Cell),
          mBlocked (blocked)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mCell.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mCell = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }

        virtual bool isUserEditable() const
        {
            return !mBlocked;
        }
    };

    template<typename ESXRecordT>
    struct OriginalCellColumn : public Column<ESXRecordT>
    {
        OriginalCellColumn()
        : Column<ESXRecordT> (Columns::ColumnId_OriginalCell, ColumnBase::Display_Cell)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mOriginalCell.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mOriginalCell = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }

        virtual bool isUserEditable() const
        {
            return false;
        }
    };

    template<typename ESXRecordT>
    struct IdColumn : public Column<ESXRecordT>
    {
        IdColumn() : Column<ESXRecordT> (Columns::ColumnId_ReferenceableId,
            ColumnBase::Display_Referenceable) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mRefID.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mRefID = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct ScaleColumn : public Column<ESXRecordT>
    {
        ScaleColumn() : Column<ESXRecordT> (Columns::ColumnId_Scale, ColumnBase::Display_Float) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mScale;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();
            record2.mScale = data.toFloat();
            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct OwnerColumn : public Column<ESXRecordT>
    {
        OwnerColumn() : Column<ESXRecordT> (Columns::ColumnId_Owner, ColumnBase::Display_Npc) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mOwner.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mOwner = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct SoulColumn : public Column<ESXRecordT>
    {
        SoulColumn() : Column<ESXRecordT> (Columns::ColumnId_Soul, ColumnBase::Display_Creature) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mSoul.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mSoul = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct FactionColumn : public Column<ESXRecordT>
    {
        FactionColumn() : Column<ESXRecordT> (Columns::ColumnId_Faction, ColumnBase::Display_Faction) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mFaction.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mFaction = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct FactionIndexColumn : public Column<ESXRecordT>
    {
        FactionIndexColumn()
        : Column<ESXRecordT> (Columns::ColumnId_FactionIndex, ColumnBase::Display_Integer)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mFactionRank;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();
            record2.mFactionRank = data.toInt();
            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct ChargesColumn : public Column<ESXRecordT>
    {
        ChargesColumn() : Column<ESXRecordT> (Columns::ColumnId_Charges, ColumnBase::Display_Integer) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mChargeInt;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();
            record2.mChargeInt = data.toInt();
            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct EnchantmentChargesColumn : public Column<ESXRecordT>
    {
        EnchantmentChargesColumn()
        : Column<ESXRecordT> (Columns::ColumnId_Enchantment, ColumnBase::Display_Float)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mEnchantmentCharge;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();
            record2.mEnchantmentCharge = data.toFloat();
            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct GoldValueColumn : public Column<ESXRecordT>
    {
        GoldValueColumn()
        : Column<ESXRecordT> (Columns::ColumnId_CoinValue, ColumnBase::Display_Integer) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mGoldValue;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();
            record2.mGoldValue = data.toInt();
            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct TeleportColumn : public Column<ESXRecordT>
    {
        TeleportColumn()
        : Column<ESXRecordT> (Columns::ColumnId_Teleport, ColumnBase::Display_Boolean)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mTeleport;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mTeleport = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct TeleportCellColumn : public Column<ESXRecordT>
    {
        TeleportCellColumn()
        : Column<ESXRecordT> (Columns::ColumnId_TeleportCell, ColumnBase::Display_Cell)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mDestCell.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mDestCell = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }

        virtual bool isUserEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct LockLevelColumn : public Column<ESXRecordT>
    {
        LockLevelColumn()
        : Column<ESXRecordT> (Columns::ColumnId_LockLevel, ColumnBase::Display_Integer)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mLockLevel;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();
            record2.mLockLevel = data.toInt();
            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct KeyColumn : public Column<ESXRecordT>
    {
        KeyColumn() : Column<ESXRecordT> (Columns::ColumnId_Key, ColumnBase::Display_Miscellaneous) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mKey.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mKey = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct TrapColumn : public Column<ESXRecordT>
    {
        TrapColumn() : Column<ESXRecordT> (Columns::ColumnId_Trap, ColumnBase::Display_Spell) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mTrap.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mTrap = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct FilterColumn : public Column<ESXRecordT>
    {
        FilterColumn() : Column<ESXRecordT> (Columns::ColumnId_Filter, ColumnBase::Display_Filter) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mFilter.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mFilter = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct PosColumn : public Column<ESXRecordT>
    {
        ESM::Position ESXRecordT::* mPosition;
        int mIndex;

        PosColumn (ESM::Position ESXRecordT::* position, int index, bool door)
        : Column<ESXRecordT> (
          (door ? Columns::ColumnId_DoorPositionXPos : Columns::ColumnId_PositionXPos)+index,
          ColumnBase::Display_Float), mPosition (position), mIndex (index) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            const ESM::Position& position = record.get().*mPosition;
            return position.pos[mIndex];
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            ESM::Position& position = record2.*mPosition;

            position.pos[mIndex] = data.toFloat();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct RotColumn : public Column<ESXRecordT>
    {
        ESM::Position ESXRecordT::* mPosition;
        int mIndex;

        RotColumn (ESM::Position ESXRecordT::* position, int index, bool door)
        : Column<ESXRecordT> (
          (door ? Columns::ColumnId_DoorPositionXRot : Columns::ColumnId_PositionXRot)+index,
          ColumnBase::Display_Double), mPosition (position), mIndex (index) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            const ESM::Position& position = record.get().*mPosition;
            return osg::RadiansToDegrees(position.rot[mIndex]);
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            ESM::Position& position = record2.*mPosition;

            position.rot[mIndex] = osg::DegreesToRadians(data.toFloat());

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct DialogueTypeColumn : public Column<ESXRecordT>
    {
        DialogueTypeColumn (bool hidden = false)
        : Column<ESXRecordT> (Columns::ColumnId_DialogueType, ColumnBase::Display_DialogueType,
            hidden ? 0 : ColumnBase::Flag_Table | ColumnBase::Flag_Dialogue)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return static_cast<int> (record.get().mType);
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mType = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }

        virtual bool isUserEditable() const
        {
            return false;
        }
    };

    template<typename ESXRecordT>
    struct QuestStatusTypeColumn : public Column<ESXRecordT>
    {
        QuestStatusTypeColumn()
        : Column<ESXRecordT> (Columns::ColumnId_QuestStatusType, ColumnBase::Display_QuestStatusType)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return static_cast<int> (record.get().mQuestStatus);
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mQuestStatus = static_cast<Info::QuestStatus> (data.toInt());

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct QuestDescriptionColumn : public Column<ESXRecordT>
    {
        QuestDescriptionColumn() : Column<ESXRecordT> (Columns::ColumnId_QuestDescription, ColumnBase::Display_LongString) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mResponse.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mResponse = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct QuestIndexColumn : public Column<ESXRecordT>
    {
        QuestIndexColumn()
        : Column<ESXRecordT> (Columns::ColumnId_QuestIndex, ColumnBase::Display_Integer)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mData.mDisposition;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();
            record2.mData.mDisposition = data.toInt();
            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct TopicColumn : public Column<ESXRecordT>
    {
        TopicColumn (bool journal)
        : Column<ESXRecordT> (journal ? Columns::ColumnId_Journal : Columns::ColumnId_Topic,
                              journal ? ColumnBase::Display_Journal : ColumnBase::Display_Topic)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mTopicId.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mTopicId = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }

        virtual bool isUserEditable() const
        {
            return false;
        }
    };

    template<typename ESXRecordT>
    struct ActorColumn : public Column<ESXRecordT>
    {
        ActorColumn() : Column<ESXRecordT> (Columns::ColumnId_Actor, ColumnBase::Display_Npc) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mActor.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mActor = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct RaceColumn : public Column<ESXRecordT>
    {
        RaceColumn() : Column<ESXRecordT> (Columns::ColumnId_Race, ColumnBase::Display_Race) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mRace.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mRace = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct ClassColumn : public Column<ESXRecordT>
    {
        ClassColumn() : Column<ESXRecordT> (Columns::ColumnId_Class, ColumnBase::Display_Class) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mClass.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mClass = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct PcFactionColumn : public Column<ESXRecordT>
    {
        PcFactionColumn() : Column<ESXRecordT> (Columns::ColumnId_PcFaction, ColumnBase::Display_Faction) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mPcFaction.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mPcFaction = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct ResponseColumn : public Column<ESXRecordT>
    {
        ResponseColumn() : Column<ESXRecordT> (Columns::ColumnId_Response, ColumnBase::Display_LongString) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mResponse.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mResponse = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct DispositionColumn : public Column<ESXRecordT>
    {
        DispositionColumn()
        : Column<ESXRecordT> (Columns::ColumnId_Disposition, ColumnBase::Display_Integer)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mData.mDisposition;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();
            record2.mData.mDisposition = data.toInt();
            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct RankColumn : public Column<ESXRecordT>
    {
        RankColumn()
        : Column<ESXRecordT> (Columns::ColumnId_Rank, ColumnBase::Display_Integer)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return static_cast<int> (record.get().mData.mRank);
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();
            record2.mData.mRank = static_cast<signed char> (data.toInt());
            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct PcRankColumn : public Column<ESXRecordT>
    {
        PcRankColumn()
        : Column<ESXRecordT> (Columns::ColumnId_PcRank, ColumnBase::Display_Integer)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return static_cast<int> (record.get().mData.mPCrank);
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();
            record2.mData.mPCrank = static_cast<signed char> (data.toInt());
            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct GenderColumn : public Column<ESXRecordT>
    {
        GenderColumn()
        : Column<ESXRecordT> (Columns::ColumnId_Gender, ColumnBase::Display_Gender)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return static_cast<int> (record.get().mData.mGender);
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mData.mGender = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct GenderNpcColumn : public Column<ESXRecordT>
    {
        GenderNpcColumn()
            : Column<ESXRecordT>(Columns::ColumnId_Gender, ColumnBase::Display_GenderNpc)
        {}

        virtual QVariant get(const Record<ESXRecordT>& record) const
        {
            // Implemented this way to allow additional gender types in the future.
            if ((record.get().mData.mFlags & ESM::BodyPart::BPF_Female) == ESM::BodyPart::BPF_Female)
                return 1;

            return 0;
        }

        virtual void set(Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            // Implemented this way to allow additional gender types in the future.
            if (data.toInt() == 1)
                record2.mData.mFlags = (record2.mData.mFlags & ~ESM::BodyPart::BPF_Female) | ESM::BodyPart::BPF_Female;
            else
                record2.mData.mFlags = record2.mData.mFlags & ~ESM::BodyPart::BPF_Female;

            record.setModified(record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct EnchantmentTypeColumn : public Column<ESXRecordT>
    {
        EnchantmentTypeColumn()
        : Column<ESXRecordT> (Columns::ColumnId_EnchantmentType, ColumnBase::Display_EnchantmentType)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return static_cast<int> (record.get().mData.mType);
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mData.mType = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct ChargesColumn2 : public Column<ESXRecordT>
    {
        ChargesColumn2() : Column<ESXRecordT> (Columns::ColumnId_Charges, ColumnBase::Display_Integer) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mData.mCharge;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();
            record2.mData.mCharge = data.toInt();
            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct AutoCalcColumn : public Column<ESXRecordT>
    {
        AutoCalcColumn() : Column<ESXRecordT> (Columns::ColumnId_AutoCalc, ColumnBase::Display_Boolean)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mData.mAutocalc!=0;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mData.mAutocalc = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct ModelColumn : public Column<ESXRecordT>
    {
        ModelColumn() : Column<ESXRecordT> (Columns::ColumnId_Model, ColumnBase::Display_Mesh) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mModel.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mModel = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct VampireColumn : public Column<ESXRecordT>
    {
        VampireColumn() : Column<ESXRecordT> (Columns::ColumnId_Vampire, ColumnBase::Display_Boolean)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mData.mVampire!=0;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mData.mVampire = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct BodyPartTypeColumn : public Column<ESXRecordT>
    {
        BodyPartTypeColumn()
        : Column<ESXRecordT> (Columns::ColumnId_BodyPartType, ColumnBase::Display_BodyPartType)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return static_cast<int> (record.get().mData.mPart);
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mData.mPart = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct MeshTypeColumn : public Column<ESXRecordT>
    {
        MeshTypeColumn(int flags = ColumnBase::Flag_Table | ColumnBase::Flag_Dialogue)
        : Column<ESXRecordT> (Columns::ColumnId_MeshType, ColumnBase::Display_MeshType, flags)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return static_cast<int> (record.get().mData.mType);
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mData.mType = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct OwnerGlobalColumn : public Column<ESXRecordT>
    {
        OwnerGlobalColumn()
        : Column<ESXRecordT> (Columns::ColumnId_OwnerGlobal, ColumnBase::Display_GlobalVariable)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mGlobalVariable.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mGlobalVariable = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct RefNumCounterColumn : public Column<ESXRecordT>
    {
        RefNumCounterColumn()
        : Column<ESXRecordT> (Columns::ColumnId_RefNumCounter, ColumnBase::Display_Integer, 0)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return static_cast<int> (record.get().mRefNumCounter);
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mRefNumCounter = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }

        virtual bool isUserEditable() const
        {
            return false;
        }
    };

    template<typename ESXRecordT>
    struct RefNumColumn : public Column<ESXRecordT>
    {
        RefNumColumn()
        : Column<ESXRecordT> (Columns::ColumnId_RefNum, ColumnBase::Display_Integer, 0)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return static_cast<int> (record.get().mRefNum.mIndex);
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mRefNum.mIndex = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }

        virtual bool isUserEditable() const
        {
            return false;
        }
    };

    template<typename ESXRecordT>
    struct SoundColumn : public Column<ESXRecordT>
    {
        SoundColumn()
        : Column<ESXRecordT> (Columns::ColumnId_Sound, ColumnBase::Display_Sound)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mSound.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mSound = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct CreatureColumn : public Column<ESXRecordT>
    {
        CreatureColumn()
        : Column<ESXRecordT> (Columns::ColumnId_Creature, ColumnBase::Display_Creature)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mCreature.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mCreature = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct SoundGeneratorTypeColumn : public Column<ESXRecordT>
    {
        SoundGeneratorTypeColumn()
        : Column<ESXRecordT> (Columns::ColumnId_SoundGeneratorType, ColumnBase::Display_SoundGeneratorType)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return static_cast<int> (record.get().mType);
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mType = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct BaseCostColumn : public Column<ESXRecordT>
    {
        BaseCostColumn() : Column<ESXRecordT> (Columns::ColumnId_BaseCost, ColumnBase::Display_Float) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mData.mBaseCost;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();
            record2.mData.mBaseCost = data.toFloat();
            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct SchoolColumn : public Column<ESXRecordT>
    {
        SchoolColumn()
        : Column<ESXRecordT> (Columns::ColumnId_School, ColumnBase::Display_School)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mData.mSchool;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mData.mSchool = data.toInt();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct EffectTextureColumn : public Column<ESXRecordT>
    {
        EffectTextureColumn (Columns::ColumnId columnId)
        : Column<ESXRecordT> (columnId,
                              columnId == Columns::ColumnId_Particle ? ColumnBase::Display_Texture
                                                                     : ColumnBase::Display_Icon)
        {
            assert (this->mColumnId==Columns::ColumnId_Icon ||
                this->mColumnId==Columns::ColumnId_Particle);
        }

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (
                (this->mColumnId==Columns::ColumnId_Icon ?
                record.get().mIcon : record.get().mParticle).c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            (this->mColumnId==Columns::ColumnId_Icon ?
                record2.mIcon : record2.mParticle)
                = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct EffectObjectColumn : public Column<ESXRecordT>
    {
        EffectObjectColumn (Columns::ColumnId columnId)
        : Column<ESXRecordT> (columnId, columnId==Columns::ColumnId_BoltObject ? ColumnBase::Display_Weapon : ColumnBase::Display_Static)
        {
            assert (this->mColumnId==Columns::ColumnId_CastingObject ||
                this->mColumnId==Columns::ColumnId_HitObject ||
                this->mColumnId==Columns::ColumnId_AreaObject ||
                this->mColumnId==Columns::ColumnId_BoltObject);
        }

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            const std::string *string = 0;

            switch (this->mColumnId)
            {
                case Columns::ColumnId_CastingObject: string = &record.get().mCasting; break;
                case Columns::ColumnId_HitObject: string = &record.get().mHit; break;
                case Columns::ColumnId_AreaObject: string = &record.get().mArea; break;
                case Columns::ColumnId_BoltObject: string = &record.get().mBolt; break;
            }

            if (!string)
                throw std::logic_error ("Unsupported column ID");

            return QString::fromUtf8 (string->c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            std::string *string = 0;

            ESXRecordT record2 = record.get();

            switch (this->mColumnId)
            {
                case Columns::ColumnId_CastingObject: string = &record2.mCasting; break;
                case Columns::ColumnId_HitObject: string = &record2.mHit; break;
                case Columns::ColumnId_AreaObject: string = &record2.mArea; break;
                case Columns::ColumnId_BoltObject: string = &record2.mBolt; break;
            }

            if (!string)
                throw std::logic_error ("Unsupported column ID");

            *string = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct EffectSoundColumn : public Column<ESXRecordT>
    {
        EffectSoundColumn (Columns::ColumnId columnId)
        : Column<ESXRecordT> (columnId, ColumnBase::Display_Sound)
        {
            assert (this->mColumnId==Columns::ColumnId_CastingSound ||
                this->mColumnId==Columns::ColumnId_HitSound ||
                this->mColumnId==Columns::ColumnId_AreaSound ||
                this->mColumnId==Columns::ColumnId_BoltSound);
        }

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            const std::string *string = 0;

            switch (this->mColumnId)
            {
                case Columns::ColumnId_CastingSound: string = &record.get().mCastSound; break;
                case Columns::ColumnId_HitSound: string = &record.get().mHitSound; break;
                case Columns::ColumnId_AreaSound: string = &record.get().mAreaSound; break;
                case Columns::ColumnId_BoltSound: string = &record.get().mBoltSound; break;
            }

            if (!string)
                throw std::logic_error ("Unsupported column ID");

            return QString::fromUtf8 (string->c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            std::string *string = 0;

            ESXRecordT record2 = record.get();

            switch (this->mColumnId)
            {
                case Columns::ColumnId_CastingSound: string = &record2.mCastSound; break;
                case Columns::ColumnId_HitSound: string = &record2.mHitSound; break;
                case Columns::ColumnId_AreaSound: string = &record2.mAreaSound; break;
                case Columns::ColumnId_BoltSound: string = &record2.mBoltSound; break;
            }

            if (!string)
                throw std::logic_error ("Unsupported column ID");

            *string = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct FormatColumn : public Column<ESXRecordT>
    {
        FormatColumn()
        : Column<ESXRecordT> (Columns::ColumnId_FileFormat, ColumnBase::Display_Integer)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mFormat;
        }

        virtual bool isEditable() const
        {
            return false;
        }
    };

    template<typename ESXRecordT>
    struct AuthorColumn : public Column<ESXRecordT>
    {
        AuthorColumn()
        : Column<ESXRecordT> (Columns::ColumnId_Author, ColumnBase::Display_String32)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mAuthor.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mAuthor = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct FileDescriptionColumn : public Column<ESXRecordT>
    {
        FileDescriptionColumn()
        : Column<ESXRecordT> (Columns::ColumnId_FileDescription, ColumnBase::Display_LongString256)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mDescription.c_str());
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mDescription = data.toString().toUtf8().constData();

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    struct LandTextureNicknameColumn : public Column<LandTexture>
    {
        LandTextureNicknameColumn();

        QVariant get(const Record<LandTexture>& record) const override;
        void set(Record<LandTexture>& record, const QVariant& data) override;
        bool isEditable() const override;
    };

    struct LandTextureIndexColumn : public Column<LandTexture>
    {
        LandTextureIndexColumn();

        QVariant get(const Record<LandTexture>& record) const override;
        bool isEditable() const override;
    };

    struct LandPluginIndexColumn : public Column<Land>
    {
        LandPluginIndexColumn();

        QVariant get(const Record<Land>& record) const override;
        bool isEditable() const override;
    };

    struct LandTexturePluginIndexColumn : public Column<LandTexture>
    {
        LandTexturePluginIndexColumn();

        QVariant get(const Record<LandTexture>& record) const override;
        bool isEditable() const override;
    };

    struct LandNormalsColumn : public Column<Land>
    {
        using DataType = QVector<signed char>;

        LandNormalsColumn();

        QVariant get(const Record<Land>& record) const override;
        void set(Record<Land>& record, const QVariant& data) override;
        bool isEditable() const override;
    };

    struct LandHeightsColumn : public Column<Land>
    {
        using DataType = QVector<float>;

        LandHeightsColumn();

        QVariant get(const Record<Land>& record) const override;
        void set(Record<Land>& record, const QVariant& data) override;
        bool isEditable() const override;
    };

    struct LandColoursColumn : public Column<Land>
    {
        using DataType = QVector<unsigned char>;

        LandColoursColumn();

        QVariant get(const Record<Land>& record) const override;
        void set(Record<Land>& record, const QVariant& data) override;
        bool isEditable() const override;
    };

    struct LandTexturesColumn : public Column<Land>
    {
        using DataType = QVector<uint16_t>;

        LandTexturesColumn();

        QVariant get(const Record<Land>& record) const override;
        void set(Record<Land>& record, const QVariant& data) override;
        bool isEditable() const override;
    };

    struct BodyPartRaceColumn : public RaceColumn<ESM::BodyPart>
    {
        const MeshTypeColumn<ESM::BodyPart> *mMeshType;

        BodyPartRaceColumn(const MeshTypeColumn<ESM::BodyPart> *meshType);

        virtual QVariant get(const Record<ESM::BodyPart> &record) const;
        virtual void set(Record<ESM::BodyPart> &record, const QVariant &data);
        virtual bool isEditable() const;
    };
}

// This is required to access the type as a QVariant.
Q_DECLARE_METATYPE(CSMWorld::LandNormalsColumn::DataType)
Q_DECLARE_METATYPE(CSMWorld::LandHeightsColumn::DataType)
Q_DECLARE_METATYPE(CSMWorld::LandColoursColumn::DataType)
Q_DECLARE_METATYPE(CSMWorld::LandTexturesColumn::DataType)

#endif
