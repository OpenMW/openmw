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
            ReferenceableCheckStage(const CSMWorld::RefIdData& referenceable,
				    const CSMWorld::IdCollection<ESM::Race>& races,
				    const CSMWorld::IdCollection<ESM::Class>& classes,
				    const CSMWorld::IdCollection<ESM::Faction>& factions);

            virtual void perform(int stage, std::vector< std::string >& messages);
            virtual int setup();

        private:
            //CONCRETE CHECKS
            void bookCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Book >& records, std::vector< std::string >& messages);
            void activatorCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Activator >& records, std::vector< std::string >& messages);
            void potionCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Potion>& records, std::vector<std::string>& messages);
            void apparatusCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Apparatus>& records, std::vector<std::string>& messages);
            void armorCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Armor>& records, std::vector<std::string>& messages);
            void clothingCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Clothing>& records, std::vector<std::string>& messages);
            void containerCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Container>& records, std::vector<std::string>& messages);
            void creatureCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Creature>& records, std::vector<std::string>& messages);
            void doorCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Door>& records, std::vector<std::string>& messages);
            void ingredientCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Ingredient>& records, std::vector<std::string>& messages);
            void creaturesLevListCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::CreatureLevList>& records, std::vector<std::string>& messages);
            void itemLevelledListCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::ItemLevList>& records, std::vector<std::string>& messages);
            void lightCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Light>& records, std::vector<std::string>& messages);
            void lockpickCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Lockpick>& records, std::vector<std::string>& messages);
            void miscCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Miscellaneous>& records, std::vector<std::string>& messages);
            void npcCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::NPC>& records, std::vector<std::string>& messages);
            void weaponCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Weapon>& records, std::vector<std::string>& messages);
            void probeCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Probe>& records, std::vector<std::string>& messages);
            void repairCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Repair>& records, std::vector<std::string>& messages);
            void staticCheck(int stage, const CSMWorld::RefIdDataContainer<ESM::Static>& records, std::vector<std::string>& messages);
            
            //FINAL CHECK
            void finalCheck(std::vector<std::string>& messages);
            
	    //TEMPLATE CHECKS
	    template<typename ITEM> void inventoryItemCheck(const ITEM& someItem,
                                                            std::vector<std::string>& messages,
                                                            const std::string& someID,
                                                            bool enchantable); //for all enchantable items.
            
	    template<typename ITEM> void inventoryItemCheck(const ITEM& someItem,
                                                            std::vector<std::string>& messages,
                                                            const std::string& someID); //for non-enchantable items.
            
	    template<typename TOOL> void toolCheck(const TOOL& someTool,
                                                   std::vector<std::string>& messages,
                                                   const std::string& someID,
                                                   bool canbebroken); //for tools with uses.
            
	    template<typename TOOL> void toolCheck(const TOOL& someTool,
                                                   std::vector<std::string>& messages,
                                                   const std::string& someID); //for tools without uses.

	    template<typename LIST> void listCheck(const LIST& someList,
                                                   std::vector< std::string >& messages,
                                                   const std::string& someID);
	    
            const CSMWorld::RefIdData& mReferencables;
            const CSMWorld::IdCollection<ESM::Race>& mRaces;
            const CSMWorld::IdCollection<ESM::Class>& mClasses;
            const CSMWorld::IdCollection<ESM::Faction>& mFactions;
            bool mPlayerPresent;
    };
}
#endif // REFERENCEABLECHECKSTAGE_H
