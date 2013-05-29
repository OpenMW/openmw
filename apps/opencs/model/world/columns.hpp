#ifndef CSM_WOLRD_COLUMNS_H
#define CSM_WOLRD_COLUMNS_H

#include <sstream>

#include <boost/lexical_cast.hpp>

#include <QColor>

#include "columnbase.hpp"

namespace CSMWorld
{
    template<typename ESXRecordT>
    struct FloatValueColumn : public Column<ESXRecordT>
    {
        FloatValueColumn() : Column<ESXRecordT> ("Value", ColumnBase::Display_Float) {}

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
        StringIdColumn() : Column<ESXRecordT> ("ID", ColumnBase::Display_String) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return QString::fromUtf8 (record.get().mId.c_str());
        }

        virtual bool isEditable() const
        {
            return false;
        }
    };

    template<typename ESXRecordT>
    struct RecordStateColumn : public Column<ESXRecordT>
    {
        RecordStateColumn() : Column<ESXRecordT> ("*", ColumnBase::Display_RecordState) {}

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
        : Column<ESXRecordT> ("Record Type", ColumnBase::Display_Integer, 0), mType (type) {}

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
        VarTypeColumn (ColumnBase::Display display) : Column<ESXRecordT> ("Type", display) {}

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

    template<typename ESXRecordT>
    struct VarValueColumn : public Column<ESXRecordT>
    {
        VarValueColumn() : Column<ESXRecordT> ("Value", ColumnBase::Display_Var) {}

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
        DescriptionColumn() : Column<ESXRecordT> ("Description", ColumnBase::Display_String) {}

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
        SpecialisationColumn() : Column<ESXRecordT> ("Specialisation", ColumnBase::Display_Specialisation) {}

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
        : Column<ESXRecordT> ("Use value #" + boost::lexical_cast<std::string> (index),
            ColumnBase::Display_Float), mIndex (index)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mData.mUseValue[mIndex];
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            record2.mData.mUseValue[mIndex] = data.toInt();

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
        AttributeColumn() : Column<ESXRecordT> ("Attribute", ColumnBase::Display_Attribute) {}

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
        NameColumn() : Column<ESXRecordT> ("Name", ColumnBase::Display_String) {}

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
        : Column<ESXRecordT> ("Attribute #" + boost::lexical_cast<std::string> (index),
            ColumnBase::Display_Attribute), mIndex (index) {}

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
        : Column<ESXRecordT> ((typePrefix ? (major ? "Major Skill #" : "Minor Skill #") : "Skill #")+
            boost::lexical_cast<std::string> (index), ColumnBase::Display_String),
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
        PlayableColumn() : Column<ESXRecordT> ("Playable", ColumnBase::Display_Boolean) {}

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
        HiddenColumn() : Column<ESXRecordT> ("Hidden", ColumnBase::Display_Boolean) {}

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

        FlagColumn (const std::string& name, int mask)
        : Column<ESXRecordT> (name, ColumnBase::Display_Boolean), mMask (mask)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return (record.get().mData.mFlags & mMask)!=0;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            int flags = record2.mData.mFlags & ~mMask;

            if (data.toInt())
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
    struct WeightHeightColumn : public Column<ESXRecordT>
    {
        bool mMale;
        bool mWeight;

        WeightHeightColumn (bool male, bool weight)
        : Column<ESXRecordT> (male ? (weight ? "Male Weight" : "Male Height") :
          (weight ? "Female Weight" : "Female Height"), ColumnBase::Display_Float),
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
        : Column<ESXRecordT> (
            type==Type_Volume ? "Volume" : (type==Type_MinRange ? "Min Range" : "Max Range"),
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
        SoundFileColumn() : Column<ESXRecordT> ("Sound File", ColumnBase::Display_String) {}

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

    /// \todo QColor is a GUI class and should not be in model. Need to think of an alternative
    /// solution.
    template<typename ESXRecordT>
    struct MapColourColumn : public Column<ESXRecordT>
    {
        /// \todo Replace Display_Integer with something that displays the colour value more directly.
        MapColourColumn() : Column<ESXRecordT> ("Map Colour", ColumnBase::Display_Integer) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            int colour = record.get().mMapColor;

            return QColor (colour & 0xff, (colour>>8) & 0xff, (colour>>16) & 0xff);
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT record2 = record.get();

            QColor colour = data.value<QColor>();

            record2.mMapColor = colour.rgb() & 0xffffff;

            record.setModified (record2);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct SleepListColumn : public Column<ESXRecordT>
    {
        SleepListColumn() : Column<ESXRecordT> ("Sleep Encounter", ColumnBase::Display_String) {}

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
        TextureColumn() : Column<ESXRecordT> ("Texture", ColumnBase::Display_String) {}

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
        SpellTypeColumn() : Column<ESXRecordT> ("Type", ColumnBase::Display_SpellType) {}

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
        CostColumn() : Column<ESXRecordT> ("Cost", ColumnBase::Display_Integer) {}

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
        ScriptColumn() : Column<ESXRecordT> ("Script", ColumnBase::Display_Script, 0) {}

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
        RegionColumn() : Column<ESXRecordT> ("Region", ColumnBase::Display_String) {}

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
}

#endif
