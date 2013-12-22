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
            ReferenceableCheckStage(const CSMWorld::RefIdData& referenceable);
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

            void setSizeVariables();

            const CSMWorld::RefIdData mReferencables;
	    
	    //SIZES OF CONCRETE TYPES
            int mBooksSize;
            int mActivatorsSize;
            int mPotionsSize;
	    int mApparatiSize;
	    int mArmorsSzie;
	    int mClothingSize;
	    int mContainersSize;
    };
}
#endif // REFERENCEABLECHECKSTAGE_H
