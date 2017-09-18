#include "columnimp.hpp"

#include <stdexcept>

namespace CSMWorld
{
    /* LandTextureNicknameColumn */
    LandTextureNicknameColumn::LandTextureNicknameColumn()
        : Column<LandTexture>(Columns::ColumnId_TextureNickname, ColumnBase::Display_String)
    {
    }

    QVariant LandTextureNicknameColumn::get(const Record<LandTexture>& record) const
    {
        return QString::fromUtf8(record.get().mId.c_str());
    }

    void LandTextureNicknameColumn::set(Record<LandTexture>& record, const QVariant& data)
    {
        LandTexture copy = record.get();
        copy.mId = data.toString().toUtf8().constData();
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
        return record.get().mPlugin;
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

    /* LandMapLodColumn */
    LandMapLodColumn::LandMapLodColumn()
        : Column<Land>(Columns::ColumnId_LandMapLodIndex, ColumnBase::Display_String, 0)
    {
    }

    QVariant LandMapLodColumn::get(const Record<Land>& record) const
    {
        const int Size = Land::LAND_GLOBAL_MAP_LOD_SIZE;
        const Land& land = record.get();

        if (land.isDataLoaded(Land::DATA_WNAM))
        {
            // Note: original data is signed
            const char* rawData = reinterpret_cast<const char*>(&land.mWnam[0]);
            return QByteArray(rawData, Size);
        }
        else
        {
            // Return a blank array
            return QByteArray(Size, 0);
        }
    }

    void LandMapLodColumn::set(Record<Land>& record, const QVariant& data)
    {
        QByteArray array = data.toByteArray();
        const signed char* rawData = reinterpret_cast<const signed char*>(array.data());

        if (array.count() != Land::LAND_GLOBAL_MAP_LOD_SIZE)
            throw std::runtime_error("invalid land map LOD data");

        Land copy = record.get();
        copy.setDataLoaded(Land::DATA_WNAM);

        for (int i = 0; i < array.count(); ++i)
        {
            copy.mWnam[i] = rawData[i];
        }

        record.setModified(copy);
    }

    bool LandMapLodColumn::isEditable() const
    {
        return true;
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

        if (land.isDataLoaded(Land::DATA_VNML))
        {
            // Note: original data is signed
            const char* rawData = reinterpret_cast<const char*>(&land.getLandData()->mNormals[0]);
            return QByteArray(rawData, Size);
        }
        else
        {
            // Return a blank array
            return QByteArray(Size, 0);
        }
    }

    void LandNormalsColumn::set(Record<Land>& record, const QVariant& data)
    {
        QByteArray array = data.toByteArray();
        const signed char* rawData = reinterpret_cast<const signed char*>(array.data());

        if (array.count() != Land::LAND_NUM_VERTS * 3)
            throw std::runtime_error("invalid land normals data");

        Land copy = record.get();
        copy.setDataLoaded(Land::DATA_VNML);

        for (int i = 0; i < array.count(); ++i)
        {
            copy.getLandData()->mNormals[i] = rawData[i];
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
        const int Size = Land::LAND_NUM_VERTS * sizeof(float);
        const Land& land = record.get();

        if (land.isDataLoaded(Land::DATA_VHGT))
        {
            // Note: original data is float
            const char* rawData = reinterpret_cast<const char*>(&land.getLandData()->mHeights[0]);
            return QByteArray(rawData, Size);
        }
        else
        {
            return QByteArray(Size, 0);
        }
    }

    void LandHeightsColumn::set(Record<Land>& record, const QVariant& data)
    {
        QByteArray array = data.toByteArray();
        const float* rawData = reinterpret_cast<const float*>(array.data());

        if (array.count() != Land::LAND_NUM_VERTS * sizeof(float))
            throw std::runtime_error("invalid land heights data");

        Land copy = record.get();
        copy.setDataLoaded(Land::DATA_VHGT);

        int count = array.count() / sizeof(float);
        for (int i = 0; i < count; ++i)
        {
            copy.getLandData()->mHeights[i] = rawData[i];
        }

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

        if (land.isDataLoaded(Land::DATA_VCLR))
        {
            // Note: original data is unsigned char
            const char* rawData = reinterpret_cast<const char*>(&land.getLandData()->mColours[0]);
            return QByteArray(rawData, Size);
        }
        else
        {
            return QByteArray(Size, 0);
        }
    }

    void LandColoursColumn::set(Record<Land>& record, const QVariant& data)
    {
        QByteArray array = data.toByteArray();
        const unsigned char* rawData = reinterpret_cast<const unsigned char*>(array.data());

        if (array.count() != Land::LAND_NUM_VERTS * 3)
            throw std::runtime_error("invalid land colours data");

        Land copy = record.get();
        copy.setDataLoaded(Land::DATA_VCLR);

        for (int i = 0; i < array.count(); ++i)
        {
            copy.getLandData()->mColours[i] = rawData[i];
        }

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
        const int Size = Land::LAND_NUM_TEXTURES * sizeof(uint16_t);
        const Land& land = record.get();

        if (land.isDataLoaded(Land::DATA_VTEX))
        {
            // Note: original data is uint16_t
            const char* rawData = reinterpret_cast<const char*>(&land.getLandData()->mTextures[0]);
            return QByteArray(rawData, Size);
        }
        else
        {
            return QByteArray(Size, 0);
        }
    }

    void LandTexturesColumn::set(Record<Land>& record, const QVariant& data)
    {
        QByteArray array = data.toByteArray();
        const uint16_t* rawData = reinterpret_cast<const uint16_t*>(array.data());

        if (array.count() != Land::LAND_NUM_TEXTURES * sizeof(uint16_t))
            throw std::runtime_error("invalid land textures data");

        Land copy = record.get();
        copy.setDataLoaded(Land::DATA_VTEX);

        int count = array.count() / sizeof(uint16_t);
        for (int i = 0; i < count; ++i)
        {
            copy.getLandData()->mTextures[i] = rawData[i];
        }

        record.setModified(copy);
    }

    bool LandTexturesColumn::isEditable() const
    {
        return true;
    }

    /* BodyPartRaceColumn */
    BodyPartRaceColumn::BodyPartRaceColumn(const MeshTypeColumn<ESM::BodyPart> *meshType)
        : mMeshType(meshType)
    {}

    QVariant BodyPartRaceColumn::get(const Record<ESM::BodyPart> &record) const
    {
        if (mMeshType != NULL && mMeshType->get(record) == ESM::BodyPart::MT_Skin)
        {
            return QString::fromUtf8(record.get().mRace.c_str());
        }
        return QVariant(QVariant::UserType);
    }

    void BodyPartRaceColumn::set(Record<ESM::BodyPart> &record, const QVariant &data)
    {
        ESM::BodyPart record2 = record.get();

        record2.mRace = data.toString().toUtf8().constData();

        record.setModified(record2);
    }

    bool BodyPartRaceColumn::isEditable() const
    {
        return true;
    }
}
