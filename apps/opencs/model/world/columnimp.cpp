#include "columnimp.hpp"

CSMWorld::BodyPartRaceColumn::BodyPartRaceColumn(const MeshTypeColumn<ESM::BodyPart> *meshType)
    : mMeshType(meshType)
{}

QVariant CSMWorld::BodyPartRaceColumn::get(const Record<ESM::BodyPart> &record) const
{
    if (mMeshType != NULL && mMeshType->get(record) == ESM::BodyPart::MT_Skin)
    {
        return QString::fromUtf8(record.get().mRace.c_str());
    }
    return QVariant(QVariant::UserType);
}

void CSMWorld::BodyPartRaceColumn::set(Record<ESM::BodyPart> &record, const QVariant &data)
{
    ESM::BodyPart record2 = record.get();

    record2.mRace = data.toString().toUtf8().constData();

    record.setModified(record2);
}

bool CSMWorld::BodyPartRaceColumn::isEditable() const
{
    return true;
}
