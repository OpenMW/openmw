#ifndef REFERENCEABLECHECKSTAGE_H
#define REFERENCEABLECHECKSTAGE_H

#include "../world/universalid.hpp"
#include "../doc/stage.hpp"
#include "../world/data.hpp"
#include "../world/refiddata.hpp"
#include "../world/resources.hpp"

namespace CSMTools
{
    class ReferenceableCheckStage : public CSMDoc::Stage
    {
        public:

            ReferenceableCheckStage (const CSMWorld::RefIdData& referenceable,
                const CSMWorld::IdCollection<ESM::Race>& races,
                const CSMWorld::IdCollection<ESM::Class>& classes,
                const CSMWorld::IdCollection<ESM::Faction>& factions,
                const CSMWorld::IdCollection<ESM::Script>& scripts,
                const CSMWorld::Resources& models,
                const CSMWorld::Resources& icons,
                const CSMWorld::IdCollection<ESM::BodyPart>& bodyparts);

            void perform(int stage, CSMDoc::Messages& messages) override;
            int setup() override;

        private:
            //CONCRETE CHECKS
            void bookCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Book >& records, CSMDoc::Messages& messages);
            void activatorCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Activator >& records, CSMDoc::Messages& messages);
            void potionCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Potion>& records, CSMDoc::Messages& messages);
            void apparatusCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Apparatus>& records, CSMDoc::Messages& messages);
            void armorCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Armor>& records, CSMDoc::Messages& messages);
            void clothingCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Clothing>& records, CSMDoc::Messages& messages);
            void containerCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Container>& records, CSMDoc::Messages& messages);
            void creatureCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Creature>& records, CSMDoc::Messages& messages);
            void doorCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Door>& records, CSMDoc::Messages& messages);
            void ingredientCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Ingredient>& records, CSMDoc::Messages& messages);
            void creaturesLevListCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::CreatureLevList>& records, CSMDoc::Messages& messages);
            void itemLevelledListCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::ItemLevList>& records, CSMDoc::Messages& messages);
            void lightCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Light>& records, CSMDoc::Messages& messages);
            void lockpickCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Lockpick>& records, CSMDoc::Messages& messages);
            void miscCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Miscellaneous>& records, CSMDoc::Messages& messages);
            void npcCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::NPC>& records, CSMDoc::Messages& messages);
            void weaponCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Weapon>& records, CSMDoc::Messages& messages);
            void probeCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Probe>& records, CSMDoc::Messages& messages);
            void repairCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Repair>& records, CSMDoc::Messages& messages);
            void staticCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Static>& records, CSMDoc::Messages& messages);

            //FINAL CHECK
            void finalCheck (CSMDoc::Messages& messages);

            //Convenience functions
            void inventoryListCheck(const std::vector<ESM::ContItem>& itemList, CSMDoc::Messages& messages, const std::string& id);

            template<typename ITEM> void inventoryItemCheck(const ITEM& someItem,
                                                            CSMDoc::Messages& messages,
                                                            const std::string& someID,
                                                            bool enchantable); //for all enchantable items.

            template<typename ITEM> void inventoryItemCheck(const ITEM& someItem,
                                                            CSMDoc::Messages& messages,
                                                            const std::string& someID); //for non-enchantable items.

            template<typename TOOL> void toolCheck(const TOOL& someTool,
                                                   CSMDoc::Messages& messages,
                                                   const std::string& someID,
                                                   bool canbebroken); //for tools with uses.

            template<typename TOOL> void toolCheck(const TOOL& someTool,
                                                   CSMDoc::Messages& messages,
                                                   const std::string& someID); //for tools without uses.

            template<typename LIST> void listCheck(const LIST& someList,
                                                   CSMDoc::Messages& messages,
                                                   const std::string& someID);

            template<typename TOOL> void scriptCheck(const TOOL& someTool,
                                                   CSMDoc::Messages& messages,
                                                   const std::string& someID);

            const CSMWorld::RefIdData& mReferencables;
            const CSMWorld::IdCollection<ESM::Race>& mRaces;
            const CSMWorld::IdCollection<ESM::Class>& mClasses;
            const CSMWorld::IdCollection<ESM::Faction>& mFactions;
            const CSMWorld::IdCollection<ESM::Script>& mScripts;
            const CSMWorld::Resources& mModels;
            const CSMWorld::Resources& mIcons;
            const CSMWorld::IdCollection<ESM::BodyPart>& mBodyParts;
            bool mPlayerPresent;
            bool mIgnoreBaseRecords;
    };
}
#endif // REFERENCEABLECHECKSTAGE_H
