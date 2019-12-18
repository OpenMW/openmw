#include "refidadapterimp.hpp"

#include <cassert>
#include <stdexcept>
#include <utility>

#include <components/esm/loadcont.hpp>
#include <components/esm/loadmgef.hpp>

#include "nestedtablewrapper.hpp"

CSMWorld::PotionColumns::PotionColumns (const InventoryColumns& columns)
: InventoryColumns (columns) {}

CSMWorld::PotionRefIdAdapter::PotionRefIdAdapter (const PotionColumns& columns,
    const RefIdColumn *autoCalc)
: InventoryRefIdAdapter<ESM::Potion> (UniversalId::Type_Potion, columns),
  mColumns(columns), mAutoCalc (autoCalc)
{}

QVariant CSMWorld::PotionRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data,
    int index) const
{
    const Record<ESM::Potion>& record = static_cast<const Record<ESM::Potion>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Potion)));

    if (column==mAutoCalc)
        return record.get().mData.mAutoCalc!=0;

    // to show nested tables in dialogue subview, see IdTree::hasChildren()
    if (column==mColumns.mEffects)
        return QVariant::fromValue(ColumnBase::TableEdit_Full);

    return InventoryRefIdAdapter<ESM::Potion>::getData (column, data, index);
}

void CSMWorld::PotionRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Potion>& record = static_cast<Record<ESM::Potion>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Potion)));

    ESM::Potion potion = record.get();

    if (column==mAutoCalc)
        potion.mData.mAutoCalc = value.toInt();
    else
    {
        InventoryRefIdAdapter<ESM::Potion>::setData (column, data, index, value);

        return;
    }

    record.setModified(potion);
}


CSMWorld::IngredientColumns::IngredientColumns (const InventoryColumns& columns)
: InventoryColumns (columns) {}

CSMWorld::IngredientRefIdAdapter::IngredientRefIdAdapter (const IngredientColumns& columns)
: InventoryRefIdAdapter<ESM::Ingredient> (UniversalId::Type_Ingredient, columns),
  mColumns(columns)
{}

QVariant CSMWorld::IngredientRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data,
    int index) const
{
    if (column==mColumns.mEffects)
        return QVariant::fromValue(ColumnBase::TableEdit_FixedRows);

    return InventoryRefIdAdapter<ESM::Ingredient>::getData (column, data, index);
}

void CSMWorld::IngredientRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    InventoryRefIdAdapter<ESM::Ingredient>::setData (column, data, index, value);

    return;
}


CSMWorld::IngredEffectRefIdAdapter::IngredEffectRefIdAdapter()
: mType(UniversalId::Type_Ingredient)
{}

CSMWorld::IngredEffectRefIdAdapter::~IngredEffectRefIdAdapter()
{}

void CSMWorld::IngredEffectRefIdAdapter::addNestedRow (const RefIdColumn *column,
        RefIdData& data, int index, int position) const
{
    // Do nothing, this table cannot be changed by the user
}

void CSMWorld::IngredEffectRefIdAdapter::removeNestedRow (const RefIdColumn *column,
        RefIdData& data, int index, int rowToRemove) const
{
    // Do nothing, this table cannot be changed by the user
}

void CSMWorld::IngredEffectRefIdAdapter::setNestedTable (const RefIdColumn* column,
        RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const
{
    Record<ESM::Ingredient>& record =
        static_cast<Record<ESM::Ingredient>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
    ESM::Ingredient ingredient = record.get();

    ingredient.mData =
        static_cast<const NestedTableWrapper<std::vector<ESM::Ingredient::IRDTstruct> >&>(nestedTable).mNestedTable.at(0);

    record.setModified (ingredient);
}

CSMWorld::NestedTableWrapperBase* CSMWorld::IngredEffectRefIdAdapter::nestedTable (const RefIdColumn* column,
        const RefIdData& data, int index) const
{
    const Record<ESM::Ingredient>& record =
        static_cast<const Record<ESM::Ingredient>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

    // return the whole struct
    std::vector<ESM::Ingredient::IRDTstruct> wrap;
    wrap.push_back(record.get().mData);

    // deleted by dtor of NestedTableStoring
    return new NestedTableWrapper<std::vector<ESM::Ingredient::IRDTstruct> >(wrap);
}

QVariant CSMWorld::IngredEffectRefIdAdapter::getNestedData (const RefIdColumn *column,
        const RefIdData& data, int index, int subRowIndex, int subColIndex) const
{
    const Record<ESM::Ingredient>& record =
        static_cast<const Record<ESM::Ingredient>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

    if (subRowIndex < 0 || subRowIndex >= 4)
        throw std::runtime_error ("index out of range");

    switch (subColIndex)
    {
        case 0: return record.get().mData.mEffectID[subRowIndex];
        case 1:
        {
            switch (record.get().mData.mEffectID[subRowIndex])
            {
                case ESM::MagicEffect::DrainSkill:
                case ESM::MagicEffect::DamageSkill:
                case ESM::MagicEffect::RestoreSkill:
                case ESM::MagicEffect::FortifySkill:
                case ESM::MagicEffect::AbsorbSkill:
                    return record.get().mData.mSkills[subRowIndex];
                default:
                    return QVariant();
            }
        }
        case 2:
        {
            switch (record.get().mData.mEffectID[subRowIndex])
            {
                case ESM::MagicEffect::DrainAttribute:
                case ESM::MagicEffect::DamageAttribute:
                case ESM::MagicEffect::RestoreAttribute:
                case ESM::MagicEffect::FortifyAttribute:
                case ESM::MagicEffect::AbsorbAttribute:
                    return record.get().mData.mAttributes[subRowIndex];
                default:
                    return QVariant();
            }
        }
        default:
            throw std::runtime_error("Trying to access non-existing column in the nested table!");
    }
}

void CSMWorld::IngredEffectRefIdAdapter::setNestedData (const RefIdColumn *column,
        RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const
{
    Record<ESM::Ingredient>& record =
        static_cast<Record<ESM::Ingredient>&> (data.getRecord (RefIdData::LocalIndex (row, mType)));
    ESM::Ingredient ingredient = record.get();

    if (subRowIndex < 0 || subRowIndex >= 4)
        throw std::runtime_error ("index out of range");

    switch(subColIndex)
    {
        case 0: ingredient.mData.mEffectID[subRowIndex] = value.toInt(); break;
        case 1: ingredient.mData.mSkills[subRowIndex] = value.toInt(); break;
        case 2: ingredient.mData.mAttributes[subRowIndex] = value.toInt(); break;
        default:
            throw std::runtime_error("Trying to access non-existing column in the nested table!");
    }

    record.setModified (ingredient);
}

int CSMWorld::IngredEffectRefIdAdapter::getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const
{
    return 3; // effect, skill, attribute
}

int CSMWorld::IngredEffectRefIdAdapter::getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const
{
    return 4; // up to 4 effects
}


CSMWorld::ApparatusRefIdAdapter::ApparatusRefIdAdapter (const InventoryColumns& columns,
    const RefIdColumn *type, const RefIdColumn *quality)
: InventoryRefIdAdapter<ESM::Apparatus> (UniversalId::Type_Apparatus, columns),
    mType (type), mQuality (quality)
{}

QVariant CSMWorld::ApparatusRefIdAdapter::getData (const RefIdColumn *column,
    const RefIdData& data, int index) const
{
    const Record<ESM::Apparatus>& record = static_cast<const Record<ESM::Apparatus>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Apparatus)));

    if (column==mType)
        return record.get().mData.mType;

    if (column==mQuality)
        return record.get().mData.mQuality;

    return InventoryRefIdAdapter<ESM::Apparatus>::getData (column, data, index);
}

void CSMWorld::ApparatusRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Apparatus>& record = static_cast<Record<ESM::Apparatus>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Apparatus)));

    ESM::Apparatus apparatus = record.get();

    if (column==mType)
        apparatus.mData.mType = value.toInt();
    else if (column==mQuality)
        apparatus.mData.mQuality = value.toFloat();
    else
    {
        InventoryRefIdAdapter<ESM::Apparatus>::setData (column, data, index, value);

        return;
    }
    record.setModified(apparatus);
}


CSMWorld::ArmorRefIdAdapter::ArmorRefIdAdapter (const EnchantableColumns& columns,
    const RefIdColumn *type, const RefIdColumn *health, const RefIdColumn *armor,
    const RefIdColumn *partRef)
: EnchantableRefIdAdapter<ESM::Armor> (UniversalId::Type_Armor, columns),
    mType (type), mHealth (health), mArmor (armor), mPartRef(partRef)
{}

QVariant CSMWorld::ArmorRefIdAdapter::getData (const RefIdColumn *column,
    const RefIdData& data, int index) const
{
    const Record<ESM::Armor>& record = static_cast<const Record<ESM::Armor>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Armor)));

    if (column==mType)
        return record.get().mData.mType;

    if (column==mHealth)
        return record.get().mData.mHealth;

    if (column==mArmor)
        return record.get().mData.mArmor;

    if (column==mPartRef)
        return QVariant::fromValue(ColumnBase::TableEdit_Full);

    return EnchantableRefIdAdapter<ESM::Armor>::getData (column, data, index);
}

void CSMWorld::ArmorRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Armor>& record = static_cast<Record<ESM::Armor>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Armor)));

    ESM::Armor armor = record.get();

    if (column==mType)
        armor.mData.mType = value.toInt();
    else if (column==mHealth)
        armor.mData.mHealth = value.toInt();
    else if (column==mArmor)
        armor.mData.mArmor = value.toInt();
    else
    {
        EnchantableRefIdAdapter<ESM::Armor>::setData (column, data, index, value);

        return;
    }

    record.setModified(armor);
}

CSMWorld::BookRefIdAdapter::BookRefIdAdapter (const EnchantableColumns& columns,
    const RefIdColumn *bookType, const RefIdColumn *skill, const RefIdColumn *text)
: EnchantableRefIdAdapter<ESM::Book> (UniversalId::Type_Book, columns),
    mBookType (bookType), mSkill (skill), mText (text)
{}

QVariant CSMWorld::BookRefIdAdapter::getData (const RefIdColumn *column,
    const RefIdData& data, int index) const
{
    const Record<ESM::Book>& record = static_cast<const Record<ESM::Book>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Book)));

    if (column==mBookType)
        return record.get().mData.mIsScroll;

    if (column==mSkill)
        return record.get().mData.mSkillId;

    if (column==mText)
        return QString::fromUtf8 (record.get().mText.c_str());

    return EnchantableRefIdAdapter<ESM::Book>::getData (column, data, index);
}

void CSMWorld::BookRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Book>& record = static_cast<Record<ESM::Book>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Book)));

    ESM::Book book = record.get();

    if (column==mBookType)
        book.mData.mIsScroll = value.toInt();
    else if (column==mSkill)
        book.mData.mSkillId = value.toInt();
    else if (column==mText)
        book.mText = value.toString().toUtf8().data();
    else
    {
        EnchantableRefIdAdapter<ESM::Book>::setData (column, data, index, value);

        return;
    }

    record.setModified(book);
}

CSMWorld::ClothingRefIdAdapter::ClothingRefIdAdapter (const EnchantableColumns& columns,
    const RefIdColumn *type, const RefIdColumn *partRef)
: EnchantableRefIdAdapter<ESM::Clothing> (UniversalId::Type_Clothing, columns), mType (type),
  mPartRef(partRef)
{}

QVariant CSMWorld::ClothingRefIdAdapter::getData (const RefIdColumn *column,
    const RefIdData& data, int index) const
{
    const Record<ESM::Clothing>& record = static_cast<const Record<ESM::Clothing>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Clothing)));

    if (column==mType)
        return record.get().mData.mType;

    if (column==mPartRef)
        return QVariant::fromValue(ColumnBase::TableEdit_Full);

    return EnchantableRefIdAdapter<ESM::Clothing>::getData (column, data, index);
}

void CSMWorld::ClothingRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Clothing>& record = static_cast<Record<ESM::Clothing>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Clothing)));

    ESM::Clothing clothing = record.get();

    if (column==mType)
        clothing.mData.mType = value.toInt();
    else
    {
        EnchantableRefIdAdapter<ESM::Clothing>::setData (column, data, index, value);

        return;
    }

    record.setModified(clothing);
}

CSMWorld::ContainerRefIdAdapter::ContainerRefIdAdapter (const NameColumns& columns,
    const RefIdColumn *weight, const RefIdColumn *organic, const RefIdColumn *respawn, const RefIdColumn *content)
: NameRefIdAdapter<ESM::Container> (UniversalId::Type_Container, columns), mWeight (weight),
  mOrganic (organic), mRespawn (respawn), mContent(content)
{}

QVariant CSMWorld::ContainerRefIdAdapter::getData (const RefIdColumn *column,
                                                   const RefIdData& data,
                                                   int index) const
{
    const Record<ESM::Container>& record = static_cast<const Record<ESM::Container>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Container)));

    if (column==mWeight)
        return record.get().mWeight;

    if (column==mOrganic)
        return (record.get().mFlags & ESM::Container::Organic)!=0;

    if (column==mRespawn)
        return (record.get().mFlags & ESM::Container::Respawn)!=0;

    if (column==mContent)
        return QVariant::fromValue(ColumnBase::TableEdit_Full);

    return NameRefIdAdapter<ESM::Container>::getData (column, data, index);
}

void CSMWorld::ContainerRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Container>& record = static_cast<Record<ESM::Container>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Container)));

    ESM::Container container = record.get();

    if (column==mWeight)
        container.mWeight = value.toFloat();
    else if (column==mOrganic)
    {
        if (value.toInt())
            container.mFlags |= ESM::Container::Organic;
        else
            container.mFlags &= ~ESM::Container::Organic;
    }
    else if (column==mRespawn)
    {
        if (value.toInt())
            container.mFlags |= ESM::Container::Respawn;
        else
            container.mFlags &= ~ESM::Container::Respawn;
    }
    else
    {
        NameRefIdAdapter<ESM::Container>::setData (column, data, index, value);

        return;
    }

    record.setModified(container);
}

CSMWorld::CreatureColumns::CreatureColumns (const ActorColumns& actorColumns)
: ActorColumns (actorColumns),
  mType(nullptr),
  mScale(nullptr),
  mOriginal(nullptr),
  mAttributes(nullptr),
  mAttacks(nullptr),
  mMisc(nullptr),
  mBloodType(nullptr)
{}

CSMWorld::CreatureRefIdAdapter::CreatureRefIdAdapter (const CreatureColumns& columns)
: ActorRefIdAdapter<ESM::Creature> (UniversalId::Type_Creature, columns), mColumns (columns)
{}

QVariant CSMWorld::CreatureRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data,
    int index) const
{
    const Record<ESM::Creature>& record = static_cast<const Record<ESM::Creature>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Creature)));

    if (column==mColumns.mType)
        return record.get().mData.mType;

    if (column==mColumns.mScale)
        return record.get().mScale;

    if (column==mColumns.mOriginal)
        return QString::fromUtf8 (record.get().mOriginal.c_str());

    if (column==mColumns.mAttributes)
        return QVariant::fromValue(ColumnBase::TableEdit_FixedRows);

    if (column==mColumns.mAttacks)
        return QVariant::fromValue(ColumnBase::TableEdit_FixedRows);

    if (column==mColumns.mMisc)
        return QVariant::fromValue(ColumnBase::TableEdit_Full);

    if (column == mColumns.mBloodType)
        return record.get().mBloodType;

    std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
        mColumns.mFlags.find (column);

    if (iter!=mColumns.mFlags.end())
        return (record.get().mFlags & iter->second)!=0;

    return ActorRefIdAdapter<ESM::Creature>::getData (column, data, index);
}

void CSMWorld::CreatureRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Creature>& record = static_cast<Record<ESM::Creature>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Creature)));

    ESM::Creature creature = record.get();

    if (column==mColumns.mType)
        creature.mData.mType = value.toInt();
    else if (column==mColumns.mScale)
        creature.mScale = value.toFloat();
    else if (column==mColumns.mOriginal)
        creature.mOriginal = value.toString().toUtf8().constData();
    else if (column == mColumns.mBloodType)
        creature.mBloodType = value.toInt();
    else
    {
        std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
            mColumns.mFlags.find (column);

        if (iter!=mColumns.mFlags.end())
        {
            if (value.toInt()!=0)
                creature.mFlags |= iter->second;
            else
                creature.mFlags &= ~iter->second;
        }
        else
        {
            ActorRefIdAdapter<ESM::Creature>::setData (column, data, index, value);

            return;
        }
    }

    record.setModified(creature);
}

CSMWorld::DoorRefIdAdapter::DoorRefIdAdapter (const NameColumns& columns,
    const RefIdColumn *openSound, const RefIdColumn *closeSound)
: NameRefIdAdapter<ESM::Door> (UniversalId::Type_Door, columns), mOpenSound (openSound),
  mCloseSound (closeSound)
{}

QVariant CSMWorld::DoorRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data,
    int index) const
{
    const Record<ESM::Door>& record = static_cast<const Record<ESM::Door>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Door)));

    if (column==mOpenSound)
        return QString::fromUtf8 (record.get().mOpenSound.c_str());

    if (column==mCloseSound)
        return QString::fromUtf8 (record.get().mCloseSound.c_str());

    return NameRefIdAdapter<ESM::Door>::getData (column, data, index);
}

void CSMWorld::DoorRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Door>& record = static_cast<Record<ESM::Door>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Door)));

    ESM::Door door = record.get();

    if (column==mOpenSound)
        door.mOpenSound = value.toString().toUtf8().constData();
    else if (column==mCloseSound)
        door.mCloseSound = value.toString().toUtf8().constData();
    else
    {
        NameRefIdAdapter<ESM::Door>::setData (column, data, index, value);

        return;
    }

    record.setModified(door);
}

CSMWorld::LightColumns::LightColumns (const InventoryColumns& columns)
: InventoryColumns (columns) {}

CSMWorld::LightRefIdAdapter::LightRefIdAdapter (const LightColumns& columns)
: InventoryRefIdAdapter<ESM::Light> (UniversalId::Type_Light, columns), mColumns (columns)
{}

QVariant CSMWorld::LightRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data,
    int index) const
{
    const Record<ESM::Light>& record = static_cast<const Record<ESM::Light>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Light)));

    if (column==mColumns.mTime)
        return record.get().mData.mTime;

    if (column==mColumns.mRadius)
        return record.get().mData.mRadius;

    if (column==mColumns.mColor)
        return record.get().mData.mColor;

    if (column==mColumns.mSound)
        return QString::fromUtf8 (record.get().mSound.c_str());

    if (column == mColumns.mEmitterType)
    {
        int mask = ESM::Light::Flicker | ESM::Light::FlickerSlow | ESM::Light::Pulse | ESM::Light::PulseSlow;

        if ((record.get().mData.mFlags & mask) == ESM::Light::Flicker)
            return 1;

        if ((record.get().mData.mFlags & mask) == ESM::Light::FlickerSlow)
            return 2;

        if ((record.get().mData.mFlags & mask) == ESM::Light::Pulse)
            return 3;

        if ((record.get().mData.mFlags & mask) == ESM::Light::PulseSlow)
            return 4;

        return 0;
    }

    std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
        mColumns.mFlags.find (column);

    if (iter!=mColumns.mFlags.end())
        return (record.get().mData.mFlags & iter->second)!=0;

    return InventoryRefIdAdapter<ESM::Light>::getData (column, data, index);
}

void CSMWorld::LightRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Light>& record = static_cast<Record<ESM::Light>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Light)));

    ESM::Light light = record.get();

    if (column==mColumns.mTime)
        light.mData.mTime = value.toInt();
    else if (column==mColumns.mRadius)
        light.mData.mRadius = value.toInt();
    else if (column==mColumns.mColor)
        light.mData.mColor = value.toInt();
    else if (column==mColumns.mSound)
        light.mSound = value.toString().toUtf8().constData();
    else if (column == mColumns.mEmitterType)
    {
        int mask = ~(ESM::Light::Flicker | ESM::Light::FlickerSlow | ESM::Light::Pulse | ESM::Light::PulseSlow);

        if (value.toInt() == 0)
            light.mData.mFlags = light.mData.mFlags & mask;
        else if (value.toInt() == 1)
            light.mData.mFlags = (light.mData.mFlags & mask) | ESM::Light::Flicker;
        else if (value.toInt() == 2)
            light.mData.mFlags = (light.mData.mFlags & mask) | ESM::Light::FlickerSlow;
        else if (value.toInt() == 3)
            light.mData.mFlags = (light.mData.mFlags & mask) | ESM::Light::Pulse;
        else
            light.mData.mFlags = (light.mData.mFlags & mask) | ESM::Light::PulseSlow;
    }
    else
    {
        std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
            mColumns.mFlags.find (column);

        if (iter!=mColumns.mFlags.end())
        {
            if (value.toInt()!=0)
                light.mData.mFlags |= iter->second;
            else
                light.mData.mFlags &= ~iter->second;
        }
        else
        {
            InventoryRefIdAdapter<ESM::Light>::setData (column, data, index, value);

            return;
        }
    }

    record.setModified (light);
}

CSMWorld::MiscRefIdAdapter::MiscRefIdAdapter (const InventoryColumns& columns, const RefIdColumn *key)
: InventoryRefIdAdapter<ESM::Miscellaneous> (UniversalId::Type_Miscellaneous, columns), mKey (key)
{}

QVariant CSMWorld::MiscRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data,
    int index) const
{
    const Record<ESM::Miscellaneous>& record = static_cast<const Record<ESM::Miscellaneous>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Miscellaneous)));

    if (column==mKey)
        return record.get().mData.mIsKey!=0;

    return InventoryRefIdAdapter<ESM::Miscellaneous>::getData (column, data, index);
}

void CSMWorld::MiscRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Miscellaneous>& record = static_cast<Record<ESM::Miscellaneous>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Miscellaneous)));

    ESM::Miscellaneous misc = record.get();

    if (column==mKey)
        misc.mData.mIsKey = value.toInt();
    else
    {
        InventoryRefIdAdapter<ESM::Miscellaneous>::setData (column, data, index, value);

        return;
    }

    record.setModified(misc);
}

CSMWorld::NpcColumns::NpcColumns (const ActorColumns& actorColumns)
: ActorColumns (actorColumns),
  mRace(nullptr),
  mClass(nullptr),
  mFaction(nullptr),
  mHair(nullptr),
  mHead(nullptr),
  mAttributes(nullptr),
  mSkills(nullptr),
  mMisc(nullptr),
  mBloodType(nullptr),
  mGender(nullptr)
{}

CSMWorld::NpcRefIdAdapter::NpcRefIdAdapter (const NpcColumns& columns)
: ActorRefIdAdapter<ESM::NPC> (UniversalId::Type_Npc, columns), mColumns (columns)
{}

QVariant CSMWorld::NpcRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data, int index)
    const
{
    const Record<ESM::NPC>& record = static_cast<const Record<ESM::NPC>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Npc)));

    if (column==mColumns.mRace)
        return QString::fromUtf8 (record.get().mRace.c_str());

    if (column==mColumns.mClass)
        return QString::fromUtf8 (record.get().mClass.c_str());

    if (column==mColumns.mFaction)
        return QString::fromUtf8 (record.get().mFaction.c_str());

    if (column==mColumns.mHair)
        return QString::fromUtf8 (record.get().mHair.c_str());

    if (column==mColumns.mHead)
        return QString::fromUtf8 (record.get().mHead.c_str());

    if (column==mColumns.mAttributes || column==mColumns.mSkills)
    {
        if ((record.get().mFlags & ESM::NPC::Autocalc) != 0)
            return QVariant::fromValue(ColumnBase::TableEdit_None);
        else
            return QVariant::fromValue(ColumnBase::TableEdit_FixedRows);
    }

    if (column==mColumns.mMisc)
        return QVariant::fromValue(ColumnBase::TableEdit_Full);

    if (column == mColumns.mBloodType)
        return record.get().mBloodType;

    if (column == mColumns.mGender)
    {
        // Implemented this way to allow additional gender types in the future.
        if ((record.get().mFlags & ESM::NPC::Female) == ESM::NPC::Female)
            return 1;

        return 0;
    }

    std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
        mColumns.mFlags.find (column);

    if (iter!=mColumns.mFlags.end())
        return (record.get().mFlags & iter->second)!=0;

    return ActorRefIdAdapter<ESM::NPC>::getData (column, data, index);
}

void CSMWorld::NpcRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::NPC>& record = static_cast<Record<ESM::NPC>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Npc)));

    ESM::NPC npc = record.get();

    if (column==mColumns.mRace)
        npc.mRace = value.toString().toUtf8().constData();
    else if (column==mColumns.mClass)
        npc.mClass = value.toString().toUtf8().constData();
    else if (column==mColumns.mFaction)
        npc.mFaction = value.toString().toUtf8().constData();
    else if (column==mColumns.mHair)
        npc.mHair = value.toString().toUtf8().constData();
    else if (column==mColumns.mHead)
        npc.mHead = value.toString().toUtf8().constData();
    else if (column == mColumns.mBloodType)
        npc.mBloodType = value.toInt();
    else if (column == mColumns.mGender)
    {
        // Implemented this way to allow additional gender types in the future.
        if (value.toInt() == 1)
            npc.mFlags = (npc.mFlags & ~ESM::NPC::Female) | ESM::NPC::Female;
        else
            npc.mFlags = npc.mFlags & ~ESM::NPC::Female;
    }
    else
    {
        std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
            mColumns.mFlags.find (column);

        if (iter!=mColumns.mFlags.end())
        {
            if (value.toInt()!=0)
                npc.mFlags |= iter->second;
            else
                npc.mFlags &= ~iter->second;

            if (iter->second == ESM::NPC::Autocalc)
                npc.mNpdtType = (value.toInt() != 0) ? ESM::NPC::NPC_WITH_AUTOCALCULATED_STATS
                                                     : ESM::NPC::NPC_DEFAULT;
        }
        else
        {
            ActorRefIdAdapter<ESM::NPC>::setData (column, data, index, value);

            return;
        }
    }

    record.setModified (npc);
}

CSMWorld::NpcAttributesRefIdAdapter::NpcAttributesRefIdAdapter ()
{}

void CSMWorld::NpcAttributesRefIdAdapter::addNestedRow (const RefIdColumn *column,
        RefIdData& data, int index, int position) const
{
    // Do nothing, this table cannot be changed by the user
}

void CSMWorld::NpcAttributesRefIdAdapter::removeNestedRow (const RefIdColumn *column,
        RefIdData& data, int index, int rowToRemove) const
{
    // Do nothing, this table cannot be changed by the user
}

void CSMWorld::NpcAttributesRefIdAdapter::setNestedTable (const RefIdColumn* column,
        RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const
{
    Record<ESM::NPC>& record =
        static_cast<Record<ESM::NPC>&> (data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Npc)));
    ESM::NPC npc = record.get();

    // store the whole struct
    npc.mNpdt =
        static_cast<const NestedTableWrapper<std::vector<ESM::NPC::NPDTstruct52> > &>(nestedTable).mNestedTable.at(0);

    record.setModified (npc);
}

CSMWorld::NestedTableWrapperBase* CSMWorld::NpcAttributesRefIdAdapter::nestedTable (const RefIdColumn* column,
        const RefIdData& data, int index) const
{
    const Record<ESM::NPC>& record =
        static_cast<const Record<ESM::NPC>&> (data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Npc)));

    // return the whole struct
    std::vector<ESM::NPC::NPDTstruct52> wrap;
    wrap.push_back(record.get().mNpdt);
    // deleted by dtor of NestedTableStoring
    return new NestedTableWrapper<std::vector<ESM::NPC::NPDTstruct52> >(wrap);
}

QVariant CSMWorld::NpcAttributesRefIdAdapter::getNestedData (const RefIdColumn *column,
        const RefIdData& data, int index, int subRowIndex, int subColIndex) const
{
    const Record<ESM::NPC>& record =
        static_cast<const Record<ESM::NPC>&> (data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Npc)));

    const ESM::NPC::NPDTstruct52& npcStruct = record.get().mNpdt;

    if (subColIndex == 0)
        return subRowIndex;
    else if (subColIndex == 1)
        switch (subRowIndex)
        {
            case 0: return static_cast<int>(npcStruct.mStrength);
            case 1: return static_cast<int>(npcStruct.mIntelligence);
            case 2: return static_cast<int>(npcStruct.mWillpower);
            case 3: return static_cast<int>(npcStruct.mAgility);
            case 4: return static_cast<int>(npcStruct.mSpeed);
            case 5: return static_cast<int>(npcStruct.mEndurance);
            case 6: return static_cast<int>(npcStruct.mPersonality);
            case 7: return static_cast<int>(npcStruct.mLuck);
            default: return QVariant(); // throw an exception here?
        }
    else
        return QVariant(); // throw an exception here?
}

void CSMWorld::NpcAttributesRefIdAdapter::setNestedData (const RefIdColumn *column,
        RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const
{
    Record<ESM::NPC>& record =
        static_cast<Record<ESM::NPC>&> (data.getRecord (RefIdData::LocalIndex (row, UniversalId::Type_Npc)));
    ESM::NPC npc = record.get();
    ESM::NPC::NPDTstruct52& npcStruct = npc.mNpdt;

    if (subColIndex == 1)
        switch(subRowIndex)
        {
            case 0: npcStruct.mStrength = static_cast<unsigned char>(value.toInt()); break;
            case 1: npcStruct.mIntelligence = static_cast<unsigned char>(value.toInt()); break;
            case 2: npcStruct.mWillpower = static_cast<unsigned char>(value.toInt()); break;
            case 3: npcStruct.mAgility = static_cast<unsigned char>(value.toInt()); break;
            case 4: npcStruct.mSpeed = static_cast<unsigned char>(value.toInt()); break;
            case 5: npcStruct.mEndurance = static_cast<unsigned char>(value.toInt()); break;
            case 6: npcStruct.mPersonality = static_cast<unsigned char>(value.toInt()); break;
            case 7: npcStruct.mLuck = static_cast<unsigned char>(value.toInt()); break;
            default: return; // throw an exception here?
        }
    else
        return; // throw an exception here?

    record.setModified (npc);
}

int CSMWorld::NpcAttributesRefIdAdapter::getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const
{
    return 2;
}

int CSMWorld::NpcAttributesRefIdAdapter::getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const
{
    // There are 8 attributes
    return 8;
}

CSMWorld::NpcSkillsRefIdAdapter::NpcSkillsRefIdAdapter ()
{}

void CSMWorld::NpcSkillsRefIdAdapter::addNestedRow (const RefIdColumn *column,
        RefIdData& data, int index, int position) const
{
    // Do nothing, this table cannot be changed by the user
}

void CSMWorld::NpcSkillsRefIdAdapter::removeNestedRow (const RefIdColumn *column,
        RefIdData& data, int index, int rowToRemove) const
{
    // Do nothing, this table cannot be changed by the user
}

void CSMWorld::NpcSkillsRefIdAdapter::setNestedTable (const RefIdColumn* column,
        RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const
{
    Record<ESM::NPC>& record =
        static_cast<Record<ESM::NPC>&> (data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Npc)));
    ESM::NPC npc = record.get();

    // store the whole struct
    npc.mNpdt =
        static_cast<const NestedTableWrapper<std::vector<ESM::NPC::NPDTstruct52> > &>(nestedTable).mNestedTable.at(0);

    record.setModified (npc);
}

CSMWorld::NestedTableWrapperBase* CSMWorld::NpcSkillsRefIdAdapter::nestedTable (const RefIdColumn* column,
        const RefIdData& data, int index) const
{
    const Record<ESM::NPC>& record =
        static_cast<const Record<ESM::NPC>&> (data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Npc)));

    // return the whole struct
    std::vector<ESM::NPC::NPDTstruct52> wrap;
    wrap.push_back(record.get().mNpdt);
    // deleted by dtor of NestedTableStoring
    return new NestedTableWrapper<std::vector<ESM::NPC::NPDTstruct52> >(wrap);
}

QVariant CSMWorld::NpcSkillsRefIdAdapter::getNestedData (const RefIdColumn *column,
        const RefIdData& data, int index, int subRowIndex, int subColIndex) const
{
    const Record<ESM::NPC>& record =
        static_cast<const Record<ESM::NPC>&> (data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Npc)));

    const ESM::NPC::NPDTstruct52& npcStruct = record.get().mNpdt;

    if (subRowIndex < 0 || subRowIndex >= ESM::Skill::Length)
        throw std::runtime_error ("index out of range");

    if (subColIndex == 0)
        return subRowIndex;
    else if (subColIndex == 1)
        return static_cast<int>(npcStruct.mSkills[subRowIndex]);
    else
        return QVariant(); // throw an exception here?
}

void CSMWorld::NpcSkillsRefIdAdapter::setNestedData (const RefIdColumn *column,
        RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const
{
    Record<ESM::NPC>& record =
        static_cast<Record<ESM::NPC>&> (data.getRecord (RefIdData::LocalIndex (row, UniversalId::Type_Npc)));
    ESM::NPC npc = record.get();
    ESM::NPC::NPDTstruct52& npcStruct = npc.mNpdt;

    if (subRowIndex < 0 || subRowIndex >= ESM::Skill::Length)
        throw std::runtime_error ("index out of range");

    if (subColIndex == 1)
        npcStruct.mSkills[subRowIndex] = static_cast<unsigned char>(value.toInt());
    else
        return; // throw an exception here?

    record.setModified (npc);
}

int CSMWorld::NpcSkillsRefIdAdapter::getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const
{
    return 2;
}

int CSMWorld::NpcSkillsRefIdAdapter::getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const
{
    // There are 27 skills
    return ESM::Skill::Length;
}

CSMWorld::NpcMiscRefIdAdapter::NpcMiscRefIdAdapter ()
{}

CSMWorld::NpcMiscRefIdAdapter::~NpcMiscRefIdAdapter()
{}

void CSMWorld::NpcMiscRefIdAdapter::addNestedRow (const RefIdColumn *column,
        RefIdData& data, int index, int position) const
{
    throw std::logic_error ("cannot add a row to a fixed table");
}

void CSMWorld::NpcMiscRefIdAdapter::removeNestedRow (const RefIdColumn *column,
        RefIdData& data, int index, int rowToRemove) const
{
    throw std::logic_error ("cannot remove a row to a fixed table");
}

void CSMWorld::NpcMiscRefIdAdapter::setNestedTable (const RefIdColumn* column,
        RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const
{
    throw std::logic_error ("table operation not supported");
}

CSMWorld::NestedTableWrapperBase* CSMWorld::NpcMiscRefIdAdapter::nestedTable (const RefIdColumn* column,
        const RefIdData& data, int index) const
{
    throw std::logic_error ("table operation not supported");
}

QVariant CSMWorld::NpcMiscRefIdAdapter::getNestedData (const RefIdColumn *column,
        const RefIdData& data, int index, int subRowIndex, int subColIndex) const
{
    const Record<ESM::NPC>& record =
        static_cast<const Record<ESM::NPC>&> (data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Npc)));

    bool autoCalc = (record.get().mFlags & ESM::NPC::Autocalc) != 0;

    if (autoCalc)
        switch (subColIndex)
        {
            case 0: return static_cast<int>(record.get().mNpdt.mLevel);
            case 1: return QVariant(QVariant::UserType);
            case 2: return QVariant(QVariant::UserType);
            case 3: return QVariant(QVariant::UserType);
            case 4: return static_cast<int>(record.get().mNpdt.mDisposition);
            case 5: return static_cast<int>(record.get().mNpdt.mReputation);
            case 6: return static_cast<int>(record.get().mNpdt.mRank);
            case 7: return record.get().mNpdt.mGold;
            case 8: return record.get().mPersistent == true;
            default: return QVariant(); // throw an exception here?
        }
    else
        switch (subColIndex)
        {
            case 0: return static_cast<int>(record.get().mNpdt.mLevel);
            case 1: return static_cast<int>(record.get().mNpdt.mHealth);
            case 2: return static_cast<int>(record.get().mNpdt.mMana);
            case 3: return static_cast<int>(record.get().mNpdt.mFatigue);
            case 4: return static_cast<int>(record.get().mNpdt.mDisposition);
            case 5: return static_cast<int>(record.get().mNpdt.mReputation);
            case 6: return static_cast<int>(record.get().mNpdt.mRank);
            case 7: return record.get().mNpdt.mGold;
            case 8: return record.get().mPersistent == true;
            default: return QVariant(); // throw an exception here?
        }
}

void CSMWorld::NpcMiscRefIdAdapter::setNestedData (const RefIdColumn *column,
        RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const
{
    Record<ESM::NPC>& record =
        static_cast<Record<ESM::NPC>&> (data.getRecord (RefIdData::LocalIndex (row, UniversalId::Type_Npc)));
    ESM::NPC npc = record.get();

    bool autoCalc = (record.get().mFlags & ESM::NPC::Autocalc) != 0;

    if (autoCalc)
        switch(subColIndex)
        {
            case 0: npc.mNpdt.mLevel = static_cast<short>(value.toInt()); break;
            case 1: return;
            case 2: return;
            case 3: return;
            case 4: npc.mNpdt.mDisposition = static_cast<signed char>(value.toInt()); break;
            case 5: npc.mNpdt.mReputation = static_cast<signed char>(value.toInt()); break;
            case 6: npc.mNpdt.mRank = static_cast<signed char>(value.toInt()); break;
            case 7: npc.mNpdt.mGold = value.toInt(); break;
            case 8: npc.mPersistent = value.toBool(); break;
            default: return; // throw an exception here?
        }
    else
        switch(subColIndex)
        {
            case 0: npc.mNpdt.mLevel = static_cast<short>(value.toInt()); break;
            case 1: npc.mNpdt.mHealth = static_cast<unsigned short>(value.toInt()); break;
            case 2: npc.mNpdt.mMana = static_cast<unsigned short>(value.toInt()); break;
            case 3: npc.mNpdt.mFatigue = static_cast<unsigned short>(value.toInt()); break;
            case 4: npc.mNpdt.mDisposition = static_cast<signed char>(value.toInt()); break;
            case 5: npc.mNpdt.mReputation = static_cast<signed char>(value.toInt()); break;
            case 6: npc.mNpdt.mRank = static_cast<signed char>(value.toInt()); break;
            case 7: npc.mNpdt.mGold = value.toInt(); break;
            case 8: npc.mPersistent = value.toBool(); break;
            default: return; // throw an exception here?
        }

    record.setModified (npc);
}

int CSMWorld::NpcMiscRefIdAdapter::getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const
{
    return 9; // Level, Health, Mana, Fatigue, Disposition, Reputation, Rank, Gold, Persist
}

int CSMWorld::NpcMiscRefIdAdapter::getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const
{
    return 1; // fixed at size 1
}

CSMWorld::CreatureAttributesRefIdAdapter::CreatureAttributesRefIdAdapter()
{}

void CSMWorld::CreatureAttributesRefIdAdapter::addNestedRow (const RefIdColumn *column,
        RefIdData& data, int index, int position) const
{
    // Do nothing, this table cannot be changed by the user
}

void CSMWorld::CreatureAttributesRefIdAdapter::removeNestedRow (const RefIdColumn *column,
        RefIdData& data, int index, int rowToRemove) const
{
    // Do nothing, this table cannot be changed by the user
}

void CSMWorld::CreatureAttributesRefIdAdapter::setNestedTable (const RefIdColumn* column,
        RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const
{
    Record<ESM::Creature>& record =
        static_cast<Record<ESM::Creature>&> (data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Creature)));
    ESM::Creature creature = record.get();

    // store the whole struct
    creature.mData =
        static_cast<const NestedTableWrapper<std::vector<ESM::Creature::NPDTstruct> > &>(nestedTable).mNestedTable.at(0);

    record.setModified (creature);
}

CSMWorld::NestedTableWrapperBase* CSMWorld::CreatureAttributesRefIdAdapter::nestedTable (const RefIdColumn* column,
        const RefIdData& data, int index) const
{
    const Record<ESM::Creature>& record =
        static_cast<const Record<ESM::Creature>&> (data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Creature)));

    // return the whole struct
    std::vector<ESM::Creature::NPDTstruct> wrap;
    wrap.push_back(record.get().mData);
    // deleted by dtor of NestedTableStoring
    return new NestedTableWrapper<std::vector<ESM::Creature::NPDTstruct> >(wrap);
}

QVariant CSMWorld::CreatureAttributesRefIdAdapter::getNestedData (const RefIdColumn *column,
        const RefIdData& data, int index, int subRowIndex, int subColIndex) const
{
    const Record<ESM::Creature>& record =
        static_cast<const Record<ESM::Creature>&> (data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Creature)));

    const ESM::Creature& creature = record.get();

    if (subColIndex == 0)
        return subRowIndex;
    else if (subColIndex == 1)
        switch (subRowIndex)
        {
            case 0: return creature.mData.mStrength;
            case 1: return creature.mData.mIntelligence;
            case 2: return creature.mData.mWillpower;
            case 3: return creature.mData.mAgility;
            case 4: return creature.mData.mSpeed;
            case 5: return creature.mData.mEndurance;
            case 6: return creature.mData.mPersonality;
            case 7: return creature.mData.mLuck;
            default: return QVariant(); // throw an exception here?
        }
    else
        return QVariant(); // throw an exception here?
}

void CSMWorld::CreatureAttributesRefIdAdapter::setNestedData (const RefIdColumn *column,
        RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const
{
    Record<ESM::Creature>& record =
        static_cast<Record<ESM::Creature>&> (data.getRecord (RefIdData::LocalIndex (row, UniversalId::Type_Creature)));
    ESM::Creature creature = record.get();

    if (subColIndex == 1)
        switch(subRowIndex)
        {
            case 0: creature.mData.mStrength = value.toInt(); break;
            case 1: creature.mData.mIntelligence = value.toInt(); break;
            case 2: creature.mData.mWillpower = value.toInt(); break;
            case 3: creature.mData.mAgility = value.toInt(); break;
            case 4: creature.mData.mSpeed = value.toInt(); break;
            case 5: creature.mData.mEndurance = value.toInt(); break;
            case 6: creature.mData.mPersonality = value.toInt(); break;
            case 7: creature.mData.mLuck = value.toInt(); break;
            default: return; // throw an exception here?
        }
    else
        return; // throw an exception here?

    record.setModified (creature);
}

int CSMWorld::CreatureAttributesRefIdAdapter::getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const
{
    return 2;
}

int CSMWorld::CreatureAttributesRefIdAdapter::getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const
{
    // There are 8 attributes
    return 8;
}

CSMWorld::CreatureAttackRefIdAdapter::CreatureAttackRefIdAdapter()
{}

void CSMWorld::CreatureAttackRefIdAdapter::addNestedRow (const RefIdColumn *column,
        RefIdData& data, int index, int position) const
{
    // Do nothing, this table cannot be changed by the user
}

void CSMWorld::CreatureAttackRefIdAdapter::removeNestedRow (const RefIdColumn *column,
        RefIdData& data, int index, int rowToRemove) const
{
    // Do nothing, this table cannot be changed by the user
}

void CSMWorld::CreatureAttackRefIdAdapter::setNestedTable (const RefIdColumn* column,
        RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const
{
    Record<ESM::Creature>& record =
        static_cast<Record<ESM::Creature>&> (data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Creature)));
    ESM::Creature creature = record.get();

    // store the whole struct
    creature.mData =
        static_cast<const NestedTableWrapper<std::vector<ESM::Creature::NPDTstruct> > &>(nestedTable).mNestedTable.at(0);

    record.setModified (creature);
}

CSMWorld::NestedTableWrapperBase* CSMWorld::CreatureAttackRefIdAdapter::nestedTable (const RefIdColumn* column,
        const RefIdData& data, int index) const
{
    const Record<ESM::Creature>& record =
        static_cast<const Record<ESM::Creature>&> (data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Creature)));

    // return the whole struct
    std::vector<ESM::Creature::NPDTstruct> wrap;
    wrap.push_back(record.get().mData);
    // deleted by dtor of NestedTableStoring
    return new NestedTableWrapper<std::vector<ESM::Creature::NPDTstruct> >(wrap);
}

QVariant CSMWorld::CreatureAttackRefIdAdapter::getNestedData (const RefIdColumn *column,
        const RefIdData& data, int index, int subRowIndex, int subColIndex) const
{
    const Record<ESM::Creature>& record =
        static_cast<const Record<ESM::Creature>&> (data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Creature)));

    const ESM::Creature& creature = record.get();

    if (subRowIndex < 0 || subRowIndex > 2)
        throw std::runtime_error ("index out of range");

    if (subColIndex == 0)
        return subRowIndex + 1;
    else if (subColIndex == 1 || subColIndex == 2)
        return creature.mData.mAttack[(subRowIndex * 2) + (subColIndex - 1)];
    else
        throw std::runtime_error ("index out of range");
}

void CSMWorld::CreatureAttackRefIdAdapter::setNestedData (const RefIdColumn *column,
        RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const
{
    Record<ESM::Creature>& record =
        static_cast<Record<ESM::Creature>&> (data.getRecord (RefIdData::LocalIndex (row, UniversalId::Type_Creature)));
    ESM::Creature creature = record.get();

    if (subRowIndex < 0 || subRowIndex > 2)
        throw std::runtime_error ("index out of range");

    if (subColIndex == 1 || subColIndex == 2)
        creature.mData.mAttack[(subRowIndex * 2) + (subColIndex - 1)] = value.toInt();
    else
        return; // throw an exception here?

    record.setModified (creature);
}

int CSMWorld::CreatureAttackRefIdAdapter::getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const
{
    return 3;
}

int CSMWorld::CreatureAttackRefIdAdapter::getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const
{
    // There are 3 attacks
    return 3;
}

CSMWorld::CreatureMiscRefIdAdapter::CreatureMiscRefIdAdapter()
{}

CSMWorld::CreatureMiscRefIdAdapter::~CreatureMiscRefIdAdapter()
{}

void CSMWorld::CreatureMiscRefIdAdapter::addNestedRow (const RefIdColumn *column,
        RefIdData& data, int index, int position) const
{
    throw std::logic_error ("cannot add a row to a fixed table");
}

void CSMWorld::CreatureMiscRefIdAdapter::removeNestedRow (const RefIdColumn *column,
        RefIdData& data, int index, int rowToRemove) const
{
    throw std::logic_error ("cannot remove a row to a fixed table");
}

void CSMWorld::CreatureMiscRefIdAdapter::setNestedTable (const RefIdColumn* column,
        RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const
{
    throw std::logic_error ("table operation not supported");
}

CSMWorld::NestedTableWrapperBase* CSMWorld::CreatureMiscRefIdAdapter::nestedTable (const RefIdColumn* column,
        const RefIdData& data, int index) const
{
    throw std::logic_error ("table operation not supported");
}

QVariant CSMWorld::CreatureMiscRefIdAdapter::getNestedData (const RefIdColumn *column,
        const RefIdData& data, int index, int subRowIndex, int subColIndex) const
{
    const Record<ESM::Creature>& record =
        static_cast<const Record<ESM::Creature>&> (data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Creature)));

    const ESM::Creature& creature = record.get();

    switch (subColIndex)
    {
        case 0: return creature.mData.mLevel;
        case 1: return creature.mData.mHealth;
        case 2: return creature.mData.mMana;
        case 3: return creature.mData.mFatigue;
        case 4: return creature.mData.mSoul;
        case 5: return creature.mData.mCombat;
        case 6: return creature.mData.mMagic;
        case 7: return creature.mData.mStealth;
        case 8: return creature.mData.mGold;
        default: return QVariant(); // throw an exception here?
    }
}

void CSMWorld::CreatureMiscRefIdAdapter::setNestedData (const RefIdColumn *column,
        RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const
{
    Record<ESM::Creature>& record =
        static_cast<Record<ESM::Creature>&> (data.getRecord (RefIdData::LocalIndex (row, UniversalId::Type_Creature)));
    ESM::Creature creature = record.get();

    switch(subColIndex)
    {
        case 0: creature.mData.mLevel   = value.toInt(); break;
        case 1: creature.mData.mHealth  = value.toInt(); break;
        case 2: creature.mData.mMana    = value.toInt(); break;
        case 3: creature.mData.mFatigue = value.toInt(); break;
        case 4: creature.mData.mSoul    = value.toInt(); break;
        case 5: creature.mData.mCombat  = value.toInt(); break;
        case 6: creature.mData.mMagic   = value.toInt(); break;
        case 7: creature.mData.mStealth = value.toInt(); break;
        case 8: creature.mData.mGold    = value.toInt(); break;
        default: return; // throw an exception here?
    }

    record.setModified (creature);
}

int CSMWorld::CreatureMiscRefIdAdapter::getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const
{
    return 9; // Level, Health, Mana, Fatigue, Soul, Combat, Magic, Steath, Gold
}

int CSMWorld::CreatureMiscRefIdAdapter::getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const
{
    return 1; // fixed at size 1
}

CSMWorld::WeaponColumns::WeaponColumns (const EnchantableColumns& columns)
: EnchantableColumns (columns) {}

CSMWorld::WeaponRefIdAdapter::WeaponRefIdAdapter (const WeaponColumns& columns)
: EnchantableRefIdAdapter<ESM::Weapon> (UniversalId::Type_Weapon, columns), mColumns (columns)
{}

QVariant CSMWorld::WeaponRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data,
    int index) const
{
    const Record<ESM::Weapon>& record = static_cast<const Record<ESM::Weapon>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Weapon)));

    if (column==mColumns.mType)
        return record.get().mData.mType;

    if (column==mColumns.mHealth)
        return record.get().mData.mHealth;

    if (column==mColumns.mSpeed)
        return record.get().mData.mSpeed;

    if (column==mColumns.mReach)
        return record.get().mData.mReach;

    for (int i=0; i<2; ++i)
    {
        if (column==mColumns.mChop[i])
            return record.get().mData.mChop[i];

        if (column==mColumns.mSlash[i])
            return record.get().mData.mSlash[i];

        if (column==mColumns.mThrust[i])
            return record.get().mData.mThrust[i];
    }

    std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
        mColumns.mFlags.find (column);

    if (iter!=mColumns.mFlags.end())
        return (record.get().mData.mFlags & iter->second)!=0;

    return EnchantableRefIdAdapter<ESM::Weapon>::getData (column, data, index);
}

void CSMWorld::WeaponRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Weapon>& record = static_cast<Record<ESM::Weapon>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Weapon)));

    ESM::Weapon weapon = record.get();

    if (column==mColumns.mType)
        weapon.mData.mType = value.toInt();
    else if (column==mColumns.mHealth)
        weapon.mData.mHealth = value.toInt();
    else if (column==mColumns.mSpeed)
        weapon.mData.mSpeed = value.toFloat();
    else if (column==mColumns.mReach)
        weapon.mData.mReach = value.toFloat();
    else if (column==mColumns.mChop[0])
        weapon.mData.mChop[0] = value.toInt();
    else if (column==mColumns.mChop[1])
        weapon.mData.mChop[1] = value.toInt();
    else if (column==mColumns.mSlash[0])
        weapon.mData.mSlash[0] = value.toInt();
    else if (column==mColumns.mSlash[1])
        weapon.mData.mSlash[1] = value.toInt();
    else if (column==mColumns.mThrust[0])
        weapon.mData.mThrust[0] = value.toInt();
    else if (column==mColumns.mThrust[1])
        weapon.mData.mThrust[1] = value.toInt();
    else
    {
        std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
            mColumns.mFlags.find (column);

        if (iter!=mColumns.mFlags.end())
        {
            if (value.toInt()!=0)
                weapon.mData.mFlags |= iter->second;
            else
                weapon.mData.mFlags &= ~iter->second;
        }
        else
        {
            EnchantableRefIdAdapter<ESM::Weapon>::setData (column, data, index, value);
            return; // Don't overwrite changes made by base class
        }
    }

    record.setModified(weapon);
}
