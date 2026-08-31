#include "loadweap.hpp"

#include <components/esm/defs.hpp>
#include <components/misc/concepts.hpp>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include <array>
#include <format>
#include <stdexcept>

namespace ESM
{
    const WeaponType::TypeId WeaponType::PickProbe("PickProbe");
    const WeaponType::TypeId WeaponType::HandToHand("HandToHand");
    const WeaponType::TypeId WeaponType::Spell("Spell");
    const WeaponType::TypeId WeaponType::ShortBladeOneHand("ShortBladeOneHand");
    const WeaponType::TypeId WeaponType::LongBladeOneHand("LongBladeOneHand");
    const WeaponType::TypeId WeaponType::LongBladeTwoHand("LongBladeTwoHand");
    const WeaponType::TypeId WeaponType::BluntOneHand("BluntOneHand");
    const WeaponType::TypeId WeaponType::BluntTwoClose("BluntTwoClose");
    const WeaponType::TypeId WeaponType::BluntTwoWide("BluntTwoWide");
    const WeaponType::TypeId WeaponType::SpearTwoWide("SpearTwoWide");
    const WeaponType::TypeId WeaponType::AxeOneHand("AxeOneHand");
    const WeaponType::TypeId WeaponType::AxeTwoHand("AxeTwoHand");
    const WeaponType::TypeId WeaponType::MarksmanBow("MarksmanBow");
    const WeaponType::TypeId WeaponType::MarksmanCrossbow("MarksmanCrossbow");
    const WeaponType::TypeId WeaponType::MarksmanThrown("MarksmanThrown");
    const WeaponType::TypeId WeaponType::Arrow("Arrow");
    const WeaponType::TypeId WeaponType::Bolt("Bolt");

    void WeaponType::load(ESMReader& esm, bool& isDeleted)
    {
        throw std::runtime_error("Weapon type loading not yet implemented");
    }

    void WeaponType::save(ESMWriter& esm, bool isDeleted) const
    {
        throw std::runtime_error("Weapon type saving not yet implemented");
    }

    namespace
    {
        struct EsmWPDTstruct
        {
            float mWeight;
            int32_t mValue;
            int16_t mType;
            uint16_t mHealth;
            float mSpeed, mReach;
            uint16_t mEnchant;
            std::array<unsigned char, 2> mChop, mSlash, mThrust;
            int32_t mFlags;
        };

        const std::array sWeaponTypeIds = {
            WeaponType::ShortBladeOneHand,
            WeaponType::LongBladeOneHand,
            WeaponType::LongBladeTwoHand,
            WeaponType::BluntOneHand,
            WeaponType::BluntTwoClose,
            WeaponType::BluntTwoWide,
            WeaponType::SpearTwoWide,
            WeaponType::AxeOneHand,
            WeaponType::AxeTwoHand,
            WeaponType::MarksmanBow,
            WeaponType::MarksmanCrossbow,
            WeaponType::MarksmanThrown,
            WeaponType::Arrow,
            WeaponType::Bolt,
        };

        void fromBinary(const EsmWPDTstruct& src, Weapon::WPDTstruct& dst)
        {
            dst.mWeight = src.mWeight;
            dst.mValue = src.mValue;
            dst.mType = Weapon::indexToRefId(src.mType);
            dst.mHealth = src.mHealth;
            dst.mSpeed = src.mSpeed;
            dst.mReach = src.mReach;
            dst.mEnchant = src.mEnchant;
            dst.mChop = src.mChop;
            dst.mSlash = src.mSlash;
            dst.mThrust = src.mThrust;
            dst.mFlags = src.mFlags;
        }

        void toBinary(const Weapon::WPDTstruct& src, EsmWPDTstruct& dst)
        {
            const int index = Weapon::refIdToIndex(src.mType);
            if (index < 0)
                throw std::runtime_error(std::format("Cannot serialize weapon type {}", src.mType.toDebugString()));
            dst.mWeight = src.mWeight;
            dst.mValue = src.mValue;
            dst.mType = static_cast<int16_t>(index);
            dst.mHealth = src.mHealth;
            dst.mSpeed = src.mSpeed;
            dst.mReach = src.mReach;
            dst.mEnchant = src.mEnchant;
            dst.mChop = src.mChop;
            dst.mSlash = src.mSlash;
            dst.mThrust = src.mThrust;
            dst.mFlags = src.mFlags;
        }
    }

    template <Misc::SameAsWithoutCvref<EsmWPDTstruct> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mWeight, v.mValue, v.mType, v.mHealth, v.mSpeed, v.mReach, v.mEnchant, v.mChop, v.mSlash, v.mThrust,
            v.mFlags);
    }

    RefId Weapon::indexToRefId(int index)
    {
        if (index < 0 || index >= static_cast<int>(sWeaponTypeIds.size()))
            return {};
        return sWeaponTypeIds[index];
    }

    int Weapon::refIdToIndex(RefId id)
    {
        for (std::size_t i = 0; i < sWeaponTypeIds.size(); ++i)
        {
            if (sWeaponTypeIds[i] == id)
                return static_cast<int>(i);
        }
        return -1;
    }

    void Weapon::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        bool hasName = false;
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case SREC_NAME:
                    mId = esm.getRefId();
                    hasName = true;
                    break;
                case fourCC("MODL"):
                    mModel = esm.getHString();
                    break;
                case fourCC("FNAM"):
                    mName = esm.getHString();
                    break;
                case fourCC("WPDT"):
                {
                    EsmWPDTstruct data;
                    esm.getSubComposite(data);
                    fromBinary(data, mData);
                    hasData = true;
                    break;
                }
                case fourCC("SCRI"):
                    mScript = esm.getRefId();
                    break;
                case fourCC("ITEX"):
                    mIcon = esm.getHString();
                    break;
                case fourCC("ENAM"):
                    mEnchant = esm.getRefId();
                    break;
                case SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
        if (!hasData && !isDeleted)
            esm.fail("Missing WPDT subrecord");
    }
    void Weapon::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNCString("MODL", mModel.getOriginal());
        esm.writeHNOCString("FNAM", mName);
        EsmWPDTstruct data;
        toBinary(mData, data);
        esm.writeNamedComposite("WPDT", data);
        esm.writeHNOCRefId("SCRI", mScript);
        esm.writeHNOCString("ITEX", mIcon.getOriginal());
        esm.writeHNOCRefId("ENAM", mEnchant);
    }

    void Weapon::blank()
    {
        mRecordFlags = 0;
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mType = WeaponType::ShortBladeOneHand;
        mData.mHealth = 0;
        mData.mSpeed = 0;
        mData.mReach = 0;
        mData.mEnchant = 0;
        mData.mChop.fill(0);
        mData.mSlash.fill(0);
        mData.mThrust.fill(0);
        mData.mFlags = 0;

        mName.clear();
        mModel.clear();
        mIcon.clear();
        mEnchant = ESM::RefId();
        mScript = ESM::RefId();
    }
}
