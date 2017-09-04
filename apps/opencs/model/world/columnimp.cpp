#include "columnimp.hpp"

namespace CSMWorld
{
    /* LandMapLodColumn */
    LandMapLodColumn::LandMapLodColumn()
        : Column<Land>(Columns::ColumnId_LandMapLodIndex, ColumnBase::Display_String, 0)
    {
    }

    QVariant LandMapLodColumn::get(const Record<Land>& record) const
    {
        // Note: original data is signed
        const char* rawData = reinterpret_cast<const char*>(&record.get().mWnam[0]);
        return QByteArray(rawData, Land::LAND_GLOBAL_MAP_LOD_SIZE);
    }

    void LandMapLodColumn::set(Record<Land>& record, const QVariant& data)
    {
        Land copy = record.get();
        QByteArray array = data.toByteArray();
        const signed char* rawData = reinterpret_cast<const signed char*>(array.data());

        assert (array.count() == Land::LAND_GLOBAL_MAP_LOD_SIZE);

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
        const Land::LandData* landData = record.get().getLandData();
        assert(landData);

        // Note: original data is signed
        const char* rawData = reinterpret_cast<const char*>(&landData->mNormals[0]);
        return QByteArray(rawData, Land::LAND_NUM_VERTS * 3);
    }

    void LandNormalsColumn::set(Record<Land>& record, const QVariant& data)
    {
        Land copy = record.get();
        Land::LandData* landData = copy.getLandData();
        assert (landData);

        QByteArray array = data.toByteArray();
        const signed char* rawData = reinterpret_cast<const signed char*>(array.data());

        assert (array.count() == Land::LAND_NUM_VERTS * 3);

        for (int i = 0; i < array.count(); ++i)
        {
            landData->mNormals[i] = rawData[i];
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
        const Land::LandData* landData = record.get().getLandData();
        assert(landData);

        // Note: original data is float
        const char* rawData = reinterpret_cast<const char*>(&landData->mHeights[0]);
        return QByteArray(rawData, Land::LAND_NUM_VERTS * sizeof(float));
    }

    void LandHeightsColumn::set(Record<Land>& record, const QVariant& data)
    {
        Land copy = record.get();
        Land::LandData* landData = copy.getLandData();
        assert (landData);

        QByteArray array = data.toByteArray();
        const float* rawData = reinterpret_cast<const float*>(array.data());

        assert (array.count() == Land::LAND_NUM_VERTS * sizeof(float));

        for (int i = 0; i < array.count(); ++i)
        {
            landData->mHeights[i] = rawData[i];
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
        const Land::LandData* landData = record.get().getLandData();
        assert(landData);

        // Note: original data is unsigned char
        const char* rawData = reinterpret_cast<const char*>(&landData->mColours[0]);
        return QByteArray(rawData, Land::LAND_NUM_VERTS * 3);
    }

    void LandColoursColumn::set(Record<Land>& record, const QVariant& data)
    {
        Land copy = record.get();
        Land::LandData* landData = copy.getLandData();
        assert (landData);

        QByteArray array = data.toByteArray();
        const unsigned char* rawData = reinterpret_cast<const unsigned char*>(array.data());

        assert (array.count() == Land::LAND_NUM_VERTS * 3);

        for (int i = 0; i < array.count(); ++i)
        {
            landData->mColours[i] = rawData[i];
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
        const Land::LandData* landData = record.get().getLandData();
        assert(landData);

        // Note: original data is uint16_t
        const char* rawData = reinterpret_cast<const char*>(&landData->mTextures[0]);
        return QByteArray(rawData, Land::LAND_NUM_TEXTURES * sizeof(uint16_t));
    }

    void LandTexturesColumn::set(Record<Land>& record, const QVariant& data)
    {
        Land copy = record.get();
        Land::LandData* landData = copy.getLandData();
        assert (landData);

        QByteArray array = data.toByteArray();
        const uint16_t* rawData = reinterpret_cast<const uint16_t*>(array.data());

        assert (array.count() == Land::LAND_NUM_TEXTURES * sizeof(uint16_t));

        for (int i = 0; i < array.count(); ++i)
        {
            landData->mTextures[i] = rawData[i];
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
