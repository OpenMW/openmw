#include "esmstore.hpp"

#include <set>
#include <iostream>

namespace MWWorld
{

static bool isCacheableRecord(int id)
{
    if (id == ESM::REC_ACTI || id == ESM::REC_ALCH || id == ESM::REC_APPA || id == ESM::REC_ARMO ||
        id == ESM::REC_BOOK || id == ESM::REC_CLOT || id == ESM::REC_CONT || id == ESM::REC_CREA ||
        id == ESM::REC_DOOR || id == ESM::REC_INGR || id == ESM::REC_LEVC || id == ESM::REC_LEVI ||
        id == ESM::REC_LIGH || id == ESM::REC_LOCK || id == ESM::REC_MISC || id == ESM::REC_NPC_ ||
        id == ESM::REC_PROB || id == ESM::REC_REPA || id == ESM::REC_STAT || id == ESM::REC_WEAP)
    {
        return true;
    }
    return false;
}

void ESMStore::load(ESM::ESMReader &esm)
{
    std::set<std::string> missing;

    ESM::Dialogue *dialogue = 0;

    // Loop through all records
    while(esm.hasMoreRecs())
    {
        ESM::NAME n = esm.getRecName();
        esm.getRecHeader();

        // Look up the record type.
        std::map<int, StoreBase *>::iterator it = mStores.find(n.val);

        if (it == mStores.end()) {
            if (n.val == ESM::REC_INFO) {
                if (dialogue) {
                    dialogue->mInfo.push_back(ESM::DialInfo());
                    dialogue->mInfo.back().load(esm);
                } else {
                    std::cerr << "error: info record without dialog" << std::endl;
                    esm.skipRecord();
                }
            } else if (n.val == ESM::REC_MGEF) {
                mMagicEffects.load (esm);
            } else if (n.val == ESM::REC_SKIL) {
                mSkills.load (esm);
            } else {
                // Not found (this would be an error later)
                esm.skipRecord();
                missing.insert(n.toString());
            }
        } else {
            // Load it
            std::string id = esm.getHNOString("NAME");
            it->second->load(esm, id);

            if (n.val==ESM::REC_DIAL) {
                // dirty hack, but it is better than non-const search()
                // or friends
                dialogue = const_cast<ESM::Dialogue *>(mDialogs.search(id));
                assert (dialogue != NULL);
            } else {
                dialogue = 0;
            }
            // Insert the reference into the global lookup
            if (!id.empty() && isCacheableRecord(n.val)) {
                mIds[id] = n.val;
            }
        }
    }

  /* This information isn't needed on screen. But keep the code around
     for debugging purposes later.

  cout << "\n" << mStores.size() << " record types:\n";
  for(RecListList::iterator it = mStores.begin(); it != mStores.end(); it++)
    cout << "  " << toStr(it->first) << ": " << it->second->getSize() << endl;
  cout << "\nNot implemented yet: ";
  for(set<string>::iterator it = missing.begin();
      it != missing.end(); it++ )
    cout << *it << " ";
  cout << endl;
  */
}

void ESMStore::setUp()
{
    std::map<int, StoreBase *>::iterator it = mStores.begin();
    for (; it != mStores.end(); ++it) {
        it->second->setUp();
    }
    mSkills.setUp();
    mMagicEffects.setUp();
    mAttributes.setUp();
}

} // end namespace
