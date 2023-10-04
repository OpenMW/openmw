#include "esm4npc.hpp"

#include <components/esm4/loadarmo.hpp>
#include <components/esm4/loadclot.hpp>
#include <components/esm4/loadlvli.hpp>
#include <components/esm4/loadlvln.hpp>
#include <components/esm4/loadnpc.hpp>
#include <components/esm4/loadotft.hpp>
#include <components/esm4/loadrace.hpp>

#include "../mwworld/customdata.hpp"
#include "../mwworld/esmstore.hpp"

#include "esm4base.hpp"

namespace MWClass
{
    template <class LevelledRecord, class TargetRecord>
    static std::vector<const TargetRecord*> withBaseTemplates(
        const TargetRecord* rec, int level = MWClass::ESM4Impl::sDefaultLevel)
    {
        std::vector<const TargetRecord*> res{ rec };
        while (true)
        {
            const TargetRecord* newRec
                = MWClass::ESM4Impl::resolveLevelled<ESM4::LevelledNpc, ESM4::Npc>(rec->mBaseTemplate, level);
            if (!newRec || newRec == rec)
                return res;
            res.push_back(rec = newRec);
        }
    }

    static const ESM4::Npc* chooseTemplate(const std::vector<const ESM4::Npc*>& recs, uint16_t flag)
    {
        // If the record is neither TES4 nor TES5 (though maybe FO4 is compatible with tes5.templateFlags), then
        // the function can return nullptr that will lead to "ESM4 NPC traits not found" exception and the NPC
        // will not be added to the scene. But in any way it shouldn't cause a crash.
        for (const auto* rec : recs)
            if (rec->mIsTES4 || !(rec->mBaseConfig.tes5.templateFlags & flag))
                return rec;
        return nullptr;
    }

    class ESM4NpcCustomData : public MWWorld::TypedCustomData<ESM4NpcCustomData>
    {
    public:
        const ESM4::Npc* mTraits;
        const ESM4::Npc* mBaseData;
        const ESM4::Race* mRace;
        bool mIsFemale;

        // TODO: Use InventoryStore instead (currently doesn't support ESM4 objects)
        std::vector<const ESM4::Armor*> mEquippedArmor;
        std::vector<const ESM4::Clothing*> mEquippedClothing;

        ESM4NpcCustomData& asESM4NpcCustomData() override { return *this; }
        const ESM4NpcCustomData& asESM4NpcCustomData() const override { return *this; }
    };

    ESM4NpcCustomData& ESM4Npc::getCustomData(const MWWorld::Ptr& ptr)
    {
        if (auto* data = ptr.getRefData().getCustomData())
            return data->asESM4NpcCustomData();

        auto data = std::make_unique<ESM4NpcCustomData>();

        const MWWorld::ESMStore* store = MWBase::Environment::get().getESMStore();
        auto npcRecs = withBaseTemplates<ESM4::LevelledNpc, ESM4::Npc>(ptr.get<ESM4::Npc>()->mBase);

        data->mTraits = chooseTemplate(npcRecs, ESM4::Npc::TES5_UseTraits);
        data->mBaseData = chooseTemplate(npcRecs, ESM4::Npc::TES5_UseBaseData);

        if (!data->mTraits)
            throw std::runtime_error("ESM4 NPC traits not found");
        if (!data->mBaseData)
            throw std::runtime_error("ESM4 NPC base data not found");

        data->mRace = store->get<ESM4::Race>().find(data->mTraits->mRace);
        if (data->mTraits->mIsTES4)
            data->mIsFemale = data->mTraits->mBaseConfig.tes4.flags & ESM4::Npc::TES4_Female;
        else if (data->mTraits->mIsFONV)
            data->mIsFemale = data->mTraits->mBaseConfig.fo3.flags & ESM4::Npc::FO3_Female;
        else
            data->mIsFemale = data->mTraits->mBaseConfig.tes5.flags & ESM4::Npc::TES5_Female;

        if (auto inv = chooseTemplate(npcRecs, ESM4::Npc::TES5_UseInventory))
        {
            for (const ESM4::InventoryItem& item : inv->mInventory)
            {
                if (auto* armor
                    = ESM4Impl::resolveLevelled<ESM4::LevelledItem, ESM4::Armor>(ESM::FormId::fromUint32(item.item)))
                    data->mEquippedArmor.push_back(armor);
                else if (data->mTraits->mIsTES4)
                {
                    const auto* clothing = ESM4Impl::resolveLevelled<ESM4::LevelledItem, ESM4::Clothing>(
                        ESM::FormId::fromUint32(item.item));
                    if (clothing)
                        data->mEquippedClothing.push_back(clothing);
                }
            }
            if (!inv->mDefaultOutfit.isZeroOrUnset())
            {
                if (const ESM4::Outfit* outfit = store->get<ESM4::Outfit>().search(inv->mDefaultOutfit))
                {
                    for (ESM::FormId itemId : outfit->mInventory)
                        if (auto* armor = ESM4Impl::resolveLevelled<ESM4::LevelledItem, ESM4::Armor>(itemId))
                            data->mEquippedArmor.push_back(armor);
                }
                else
                    Log(Debug::Error) << "Outfit not found: " << ESM::RefId(inv->mDefaultOutfit);
            }
        }

        ESM4NpcCustomData& res = *data;
        ptr.getRefData().setCustomData(std::move(data));
        return res;
    }

    const std::vector<const ESM4::Armor*>& ESM4Npc::getEquippedArmor(const MWWorld::Ptr& ptr)
    {
        return getCustomData(ptr).mEquippedArmor;
    }

    const std::vector<const ESM4::Clothing*>& ESM4Npc::getEquippedClothing(const MWWorld::Ptr& ptr)
    {
        return getCustomData(ptr).mEquippedClothing;
    }

    const ESM4::Npc* ESM4Npc::getTraitsRecord(const MWWorld::Ptr& ptr)
    {
        return getCustomData(ptr).mTraits;
    }

    const ESM4::Race* ESM4Npc::getRace(const MWWorld::Ptr& ptr)
    {
        return getCustomData(ptr).mRace;
    }

    bool ESM4Npc::isFemale(const MWWorld::Ptr& ptr)
    {
        return getCustomData(ptr).mIsFemale;
    }

    std::string ESM4Npc::getModel(const MWWorld::ConstPtr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
            return "";
        const ESM4NpcCustomData& data = ptr.getRefData().getCustomData()->asESM4NpcCustomData();
        if (data.mTraits->mIsTES4)
            return "meshes\\" + data.mTraits->mModel;
        if (data.mIsFemale)
            return "meshes\\" + data.mRace->mModelFemale;
        else
            return "meshes\\" + data.mRace->mModelMale;
    }

    std::string_view ESM4Npc::getName(const MWWorld::ConstPtr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
            return "";
        const ESM4NpcCustomData& data = ptr.getRefData().getCustomData()->asESM4NpcCustomData();
        return data.mBaseData->mFullName;
    }
}
