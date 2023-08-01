#include "esm4npcanimation.hpp"

#include <components/esm4/loadarma.hpp>
#include <components/esm4/loadarmo.hpp>
#include <components/esm4/loadclot.hpp>
#include <components/esm4/loadhair.hpp>
#include <components/esm4/loadhdpt.hpp>
#include <components/esm4/loadnpc.hpp>
#include <components/esm4/loadrace.hpp>

#include "../mwworld/customdata.hpp"
#include "../mwworld/esmstore.hpp"

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>

#include "../mwclass/esm4npc.hpp"

namespace MWRender
{
    ESM4NpcAnimation::ESM4NpcAnimation(
        const MWWorld::Ptr& ptr, osg::ref_ptr<osg::Group> parentNode, Resource::ResourceSystem* resourceSystem)
        : Animation(ptr, std::move(parentNode), resourceSystem)
    {
        getOrCreateObjectRoot();
        const ESM4::Npc* traits = MWClass::ESM4Npc::getTraitsRecord(mPtr);
        if (traits->mIsTES4)
            insertTes4NpcBodyPartsAndEquipment();
        else
            insertTes5NpcBodyPartsAndEquipment();
    }

    void ESM4NpcAnimation::insertMesh(std::string_view model)
    {
        std::string path = "meshes\\";
        path.append(model);
        mResourceSystem->getSceneManager()->getInstance(path, mObjectRoot.get());
    }

    template <class Record>
    static std::string_view chooseTes4EquipmentModel(const Record* rec, bool isFemale)
    {
        if (isFemale && !rec->mModelFemale.empty())
            return rec->mModelFemale;
        else if (!isFemale && !rec->mModelMale.empty())
            return rec->mModelMale;
        else
            return rec->mModel;
    }

    void ESM4NpcAnimation::insertTes4NpcBodyPartsAndEquipment()
    {
        const ESM4::Npc* traits = MWClass::ESM4Npc::getTraitsRecord(mPtr);
        const ESM4::Race* race = MWClass::ESM4Npc::getRace(mPtr);
        bool isFemale = MWClass::ESM4Npc::isFemale(mPtr);

        // TODO: Body and head parts are placed incorrectly, need to attach to bones

        for (const ESM4::Race::BodyPart& bodyPart : (isFemale ? race->mBodyPartsFemale : race->mBodyPartsMale))
            if (!bodyPart.mesh.empty())
                insertMesh(bodyPart.mesh);
        for (const ESM4::Race::BodyPart& bodyPart : (isFemale ? race->mHeadPartsFemale : race->mHeadParts))
            if (!bodyPart.mesh.empty())
                insertMesh(bodyPart.mesh);
        if (!traits->mHair.isZeroOrUnset())
        {
            const MWWorld::ESMStore* store = MWBase::Environment::get().getESMStore();
            if (const ESM4::Hair* hair = store->get<ESM4::Hair>().search(traits->mHair))
                insertMesh(hair->mModel);
        }

        for (const ESM4::Armor* armor : MWClass::ESM4Npc::getEquippedArmor(mPtr))
            insertMesh(chooseTes4EquipmentModel(armor, isFemale));
        for (const ESM4::Clothing* clothing : MWClass::ESM4Npc::getEquippedClothing(mPtr))
            insertMesh(chooseTes4EquipmentModel(clothing, isFemale));
    }

    void ESM4NpcAnimation::insertTes5NpcBodyPartsAndEquipment()
    {
        const MWWorld::ESMStore* store = MWBase::Environment::get().getESMStore();

        const ESM4::Npc* traits = MWClass::ESM4Npc::getTraitsRecord(mPtr);
        const ESM4::Race* race = MWClass::ESM4Npc::getRace(mPtr);
        bool isFemale = MWClass::ESM4Npc::isFemale(mPtr);

        std::set<uint32_t> usedHeadPartTypes;
        auto addHeadParts = [&](const std::vector<ESM::FormId>& partIds) {
            for (ESM::FormId partId : partIds)
            {
                if (partId.isZeroOrUnset())
                    continue;
                const ESM4::HeadPart* part = store->get<ESM4::HeadPart>().search(partId);
                if (!part)
                {
                    Log(Debug::Error) << "Head part not found: " << ESM::RefId(partId);
                    continue;
                }
                if (usedHeadPartTypes.contains(part->mType))
                    continue;
                usedHeadPartTypes.insert(part->mType);
                insertMesh(part->mModel);
            }
        };

        std::vector<const ESM4::ArmorAddon*> armorAddons;

        auto findArmorAddons = [&](const ESM4::Armor* armor) {
            for (ESM::FormId armaId : armor->mAddOns)
            {
                const ESM4::ArmorAddon* arma = store->get<ESM4::ArmorAddon>().search(armaId);
                if (!arma)
                {
                    Log(Debug::Error) << "ArmorAddon not found: " << ESM::RefId(armaId);
                    continue;
                }
                bool compatibleRace = arma->mRacePrimary == traits->mRace;
                for (auto r : arma->mRaces)
                    if (r == traits->mRace)
                        compatibleRace = true;
                if (compatibleRace)
                    armorAddons.push_back(arma);
            }
        };

        for (const ESM4::Armor* armor : MWClass::ESM4Npc::getEquippedArmor(mPtr))
            findArmorAddons(armor);
        if (!traits->mWornArmor.isZeroOrUnset())
            findArmorAddons(store->get<ESM4::Armor>().find(traits->mWornArmor));
        findArmorAddons(store->get<ESM4::Armor>().find(race->mSkin));

        if (isFemale)
            std::sort(armorAddons.begin(), armorAddons.end(),
                [](auto x, auto y) { return x->mFemalePriority > y->mFemalePriority; });
        else
            std::sort(armorAddons.begin(), armorAddons.end(),
                [](auto x, auto y) { return x->mMalePriority > y->mMalePriority; });

        uint32_t usedParts = 0;
        for (const ESM4::ArmorAddon* arma : armorAddons)
        {
            const uint32_t covers = arma->mBodyTemplate.bodyPart;
            if (covers & usedParts & ESM4::Armor::TES5_Body)
                continue; // if body is already covered, skip to avoid clipping
            if (covers & ~usedParts)
            { // if covers at least something that wasn't covered before - add model
                usedParts |= covers;
                insertMesh(isFemale ? arma->mModelFemale : arma->mModelMale);
            }
        }

        if (usedParts & ESM4::Armor::TES5_Hair)
            usedHeadPartTypes.insert(ESM4::HeadPart::Type_Hair);
        addHeadParts(traits->mHeadParts);
        addHeadParts(isFemale ? race->mHeadPartIdsFemale : race->mHeadPartIdsMale);
    }
}
