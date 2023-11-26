#include "columnimp.hpp"

#include <apps/opencs/model/world/columnbase.hpp>
#include <apps/opencs/model/world/columns.hpp>
#include <apps/opencs/model/world/land.hpp>
#include <apps/opencs/model/world/landtexture.hpp>
#include <apps/opencs/model/world/record.hpp>

#include <components/esm3/loadland.hpp>
#include <components/esm3/loadmgef.hpp>

#include <algorithm>
#include <stdexcept>

namespace CSMWorld
{
    namespace
    {
        struct GetStringId
        {
            std::string operator()(ESM::EmptyRefId /*value*/) const { return std::string(); }

            std::string operator()(ESM::StringRefId value) const { return value.getValue(); }

            std::string operator()(ESM::FormId value) const { return value.toString("FormId:"); }

            std::string operator()(ESM::IndexRefId value) const
            {
                switch (value.getRecordType())
                {
                    case ESM::REC_MGEF:
                        return std::string(ESM::MagicEffect::sIndexNames[value.getValue()]);
                    default:
                        break;
                }

                return value.toDebugString();
            }

            template <class T>
            std::string operator()(const T& value) const
            {
                return value.toDebugString();
            }
        };
    }

    /* LandTextureNicknameColumn */
    LandTextureNicknameColumn::LandTextureNicknameColumn()
        : Column<LandTexture>(Columns::ColumnId_TextureNickname, ColumnBase::Display_String)
    {
    }

    QVariant LandTextureNicknameColumn::get(const Record<LandTexture>& record) const
    {
        return QString::fromStdString(record.get().mId.toString());
    }

    void LandTextureNicknameColumn::set(Record<LandTexture>& record, const QVariant& data)
    {
        LandTexture copy = record.get();
        copy.mId = ESM::RefId::stringRefId(data.toString().toUtf8().constData());
        record.setModified(copy);
    }

    bool LandTextureNicknameColumn::isEditable() const
    {
        return true;
    }

    /* LandTextureIndexColumn */
    LandTextureIndexColumn::LandTextureIndexColumn()
        : Column<LandTexture>(Columns::ColumnId_TextureIndex, ColumnBase::Display_Integer)
    {
    }

    QVariant LandTextureIndexColumn::get(const Record<LandTexture>& record) const
    {
        return record.get().mIndex;
    }

    bool LandTextureIndexColumn::isEditable() const
    {
        return false;
    }

    /* LandPluginIndexColumn */
    LandPluginIndexColumn::LandPluginIndexColumn()
        : Column<Land>(Columns::ColumnId_PluginIndex, ColumnBase::Display_Integer, 0)
    {
    }

    QVariant LandPluginIndexColumn::get(const Record<Land>& record) const
    {
        return record.get().getPlugin();
    }

    bool LandPluginIndexColumn::isEditable() const
    {
        return false;
    }

    /* LandTexturePluginIndexColumn */
    LandTexturePluginIndexColumn::LandTexturePluginIndexColumn()
        : Column<LandTexture>(Columns::ColumnId_PluginIndex, ColumnBase::Display_Integer, 0)
    {
    }

    QVariant LandTexturePluginIndexColumn::get(const Record<LandTexture>& record) const
    {
        return record.get().mPluginIndex;
    }

    bool LandTexturePluginIndexColumn::isEditable() const
    {
        return false;
    }

    /* LandNormalsColumn */
    LandNormalsColumn::LandNormalsColumn()
        : Column<Land>(Columns::ColumnId_LandNormalsIndex, ColumnBase::Display_String, 0)
    {
    }

    QVariant LandNormalsColumn::get(const Record<Land>& record) const
    {
        const int Size = Land::LAND_NUM_VERTS * 3;
        const Land& land = record.get();

        DataType values(Size, 0);

        if (land.isDataLoaded(Land::DATA_VNML))
        {
            for (int i = 0; i < Size; ++i)
                values[i] = land.getLandData()->mNormals[i];
        }

        QVariant variant;
        variant.setValue(values);
        return variant;
    }

    void LandNormalsColumn::set(Record<Land>& record, const QVariant& data)
    {
        DataType values = data.value<DataType>();

        if (values.size() != Land::LAND_NUM_VERTS * 3)
            throw std::runtime_error("invalid land normals data");

        Land copy = record.get();
        copy.add(Land::DATA_VNML);

        for (int i = 0; i < values.size(); ++i)
        {
            copy.getLandData()->mNormals[i] = values[i];
        }

        record.setModified(copy);
    }

    bool LandNormalsColumn::isEditable() const
    {
        return true;
    }

    /* LandHeightsColumn */
    LandHeightsColumn::LandHeightsColumn()
        : Column<Land>(Columns::ColumnId_LandHeightsIndex, ColumnBase::Display_String, 0)
    {
    }

    QVariant LandHeightsColumn::get(const Record<Land>& record) const
    {
        const int Size = Land::LAND_NUM_VERTS;
        const Land& land = record.get();

        DataType values(Size, 0);

        if (land.isDataLoaded(Land::DATA_VHGT))
        {
            for (int i = 0; i < Size; ++i)
                values[i] = land.getLandData()->mHeights[i];
        }

        QVariant variant;
        variant.setValue(values);
        return variant;
    }

    void LandHeightsColumn::set(Record<Land>& record, const QVariant& data)
    {
        DataType values = data.value<DataType>();

        if (values.size() != Land::LAND_NUM_VERTS)
            throw std::runtime_error("invalid land heights data");

        Land copy = record.get();
        copy.add(Land::DATA_VHGT);

        for (int i = 0; i < values.size(); ++i)
        {
            copy.getLandData()->mHeights[i] = values[i];
        }

        copy.mFlags |= Land::FLAG_HEIGHT;

        record.setModified(copy);
    }

    bool LandHeightsColumn::isEditable() const
    {
        return true;
    }

    /* LandColoursColumn */
    LandColoursColumn::LandColoursColumn()
        : Column<Land>(Columns::ColumnId_LandColoursIndex, ColumnBase::Display_String, 0)
    {
    }

    QVariant LandColoursColumn::get(const Record<Land>& record) const
    {
        const int Size = Land::LAND_NUM_VERTS * 3;
        const Land& land = record.get();

        DataType values(Size, 0);

        if (land.isDataLoaded(Land::DATA_VCLR))
        {
            for (int i = 0; i < Size; ++i)
                values[i] = land.getLandData()->mColours[i];
        }

        QVariant variant;
        variant.setValue(values);
        return variant;
    }

    void LandColoursColumn::set(Record<Land>& record, const QVariant& data)
    {
        DataType values = data.value<DataType>();

        if (values.size() != Land::LAND_NUM_VERTS * 3)
            throw std::runtime_error("invalid land colours data");

        Land copy = record.get();
        copy.add(Land::DATA_VCLR);

        for (int i = 0; i < values.size(); ++i)
        {
            copy.getLandData()->mColours[i] = values[i];
        }

        copy.mFlags |= Land::FLAG_COLOR;

        record.setModified(copy);
    }

    bool LandColoursColumn::isEditable() const
    {
        return true;
    }

    /* LandTexturesColumn */
    LandTexturesColumn::LandTexturesColumn()
        : Column<Land>(Columns::ColumnId_LandTexturesIndex, ColumnBase::Display_String, 0)
    {
    }

    QVariant LandTexturesColumn::get(const Record<Land>& record) const
    {
        const int Size = Land::LAND_NUM_TEXTURES;
        const Land& land = record.get();

        DataType values(Size, 0);

        if (land.isDataLoaded(Land::DATA_VTEX))
        {
            for (int i = 0; i < Size; ++i)
                values[i] = land.getLandData()->mTextures[i];
        }

        QVariant variant;
        variant.setValue(values);
        return variant;
    }

    void LandTexturesColumn::set(Record<Land>& record, const QVariant& data)
    {
        DataType values = data.value<DataType>();

        if (values.size() != Land::LAND_NUM_TEXTURES)
            throw std::runtime_error("invalid land textures data");

        Land copy = record.get();
        copy.add(Land::DATA_VTEX);

        for (int i = 0; i < values.size(); ++i)
        {
            copy.getLandData()->mTextures[i] = values[i];
        }

        copy.mFlags |= Land::FLAG_TEXTURE;

        record.setModified(copy);
    }

    bool LandTexturesColumn::isEditable() const
    {
        return true;
    }

    /* BodyPartRaceColumn */
    BodyPartRaceColumn::BodyPartRaceColumn(const MeshTypeColumn<ESM::BodyPart>* meshType)
        : mMeshType(meshType)
    {
    }

    QVariant BodyPartRaceColumn::get(const Record<ESM::BodyPart>& record) const
    {
        if (mMeshType != nullptr && mMeshType->get(record) == ESM::BodyPart::MT_Skin)
        {
            return QString::fromUtf8(record.get().mRace.getRefIdString().c_str());
        }
        return QVariant(QVariant::UserType);
    }

    void BodyPartRaceColumn::set(Record<ESM::BodyPart>& record, const QVariant& data)
    {
        ESM::BodyPart record2 = record.get();

        record2.mRace = ESM::RefId::stringRefId(data.toString().toUtf8().constData());

        record.setModified(record2);
    }

    bool BodyPartRaceColumn::isEditable() const
    {
        return true;
    }

    SelectionGroupColumn::SelectionGroupColumn()
        : Column<ESM::SelectionGroup>(Columns::ColumnId_SelectionGroupObjects, ColumnBase::Display_None)
    {
    }

    QVariant SelectionGroupColumn::get(const Record<ESM::SelectionGroup>& record) const
    {
        QVariant data;
        QStringList selectionInfo;
        const std::vector<std::string>& instances = record.get().selectedInstances;

        for (const std::string& instance : instances)
            selectionInfo << QString::fromStdString(instance);
        data.setValue(selectionInfo);

        return data;
    }

    void SelectionGroupColumn::set(Record<ESM::SelectionGroup>& record, const QVariant& data)
    {
        ESM::SelectionGroup record2 = record.get();
        for (const auto& item : data.toStringList())
            record2.selectedInstances.push_back(item.toStdString());
        record.setModified(record2);
    }

    bool SelectionGroupColumn::isEditable() const
    {
        return false;
    }

    std::optional<std::uint32_t> getSkillIndex(std::string_view value)
    {
        int index = ESM::Skill::refIdToIndex(ESM::RefId::stringRefId(value));
        if (index < 0)
            return std::nullopt;
        return static_cast<std::uint32_t>(index);
    }

    std::string getStringId(ESM::RefId value)
    {
        return visit(GetStringId{}, value);
    }
}
