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
            void bookCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Book >& records, std::vector< std::string >& messages);
            void activatorCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Activator >& records, std::vector< std::string >& messages);
	    void setSizeVariables();

            const CSMWorld::RefIdData mReferencables;
	    int mBooksSize;
	    int mActivatorsSize;
	    int mPotionsSize;
    };
}
#endif // REFERENCEABLECHECKSTAGE_H
