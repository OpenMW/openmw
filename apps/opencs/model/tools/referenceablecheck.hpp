#ifndef REFERENCEABLECHECKSTAGE_H
#define REFERENCEABLECHECKSTAGE_H

#include "../world/universalid.hpp"
#include "../doc/stage.hpp"
#include "../world/data.hpp"
#include "../world/refiddata.hpp"

namespace CSMTools
{
    class ReferenceableCheckStage : public CSMDoc::Stage
    {
        public:

            ReferenceableCheckStage (const CSMWorld::RefIdData& referenceable,
                const CSMWorld::IdCollection<ESM::Race>& races,
                const CSMWorld::IdCollection<ESM::Class>& classes,
                const CSMWorld::IdCollection<ESM::Faction>& factions);

            virtual void perform(int stage, Messages& messages);
            virtual int setup();

        private:
            //CONCRETE CHECKS
            void bookCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Book >& records, Messages& messages);
            void activatorCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Activator >& records, Messages& messages);
            void potionCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Potion>& records, Messages& messages);
            void apparatusCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Apparatus>& records, Messages& messages);
            void armorCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Armor>& records, Messages& messages);
            void clothingCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Clothing>& records, Messages& messages);
            void containerCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Container>& records, Messages& messages);
            void creatureCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Creature>& records, Messages& messages);
            void doorCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Door>& records, Messages& messages);
            void ingredientCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Ingredient>& records, Messages& messages);
            void creaturesLevListCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::CreatureLevList>& records, Messages& messages);
            void itemLevelledListCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::ItemLevList>& records, Messages& messages);
            void lightCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Light>& records, Messages& messages);
            void lockpickCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Lockpick>& records, Messages& messages);
            void miscCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Miscellaneous>& records, Messages& messages);
            void npcCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::NPC>& records, Messages& messages);
            void weaponCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Weapon>& records, Messages& messages);
            void probeCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Probe>& records, Messages& messages);
            void repairCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Repair>& records, Messages& messages);
            void staticCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Static>& records, Messages& messages);

            //FINAL CHECK
            void finalCheck (Messages& messages);

	    //TEMPLATE CHECKS
	    template<typename ITEM> void inventoryItemCheck(const ITEM& someItem,
                                                            Messages& messages,
                                                            const std::string& someID,
                                                            bool enchantable); //for all enchantable items.

	    template<typename ITEM> void inventoryItemCheck(const ITEM& someItem,
                                                            Messages& messages,
                                                            const std::string& someID); //for non-enchantable items.

	    template<typename TOOL> void toolCheck(const TOOL& someTool,
                                                   Messages& messages,
                                                   const std::string& someID,
                                                   bool canbebroken); //for tools with uses.

	    template<typename TOOL> void toolCheck(const TOOL& someTool,
                                                   Messages& messages,
                                                   const std::string& someID); //for tools without uses.

	    template<typename LIST> void listCheck(const LIST& someList,
                                                   Messages& messages,
                                                   const std::string& someID);

            const CSMWorld::RefIdData& mReferencables;
            const CSMWorld::IdCollection<ESM::Race>& mRaces;
            const CSMWorld::IdCollection<ESM::Class>& mClasses;
            const CSMWorld::IdCollection<ESM::Faction>& mFactions;
            bool mPlayerPresent;
    };
}
#endif // REFERENCEABLECHECKSTAGE_H
