#include "esmstore.hpp"

#include <set>
#include <iostream>

namespace MWWorld
{

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
        RecListList::iterator it = recLists.find(n.val);

        if(it == recLists.end())
        {
            if (n.val==ESM::REC_INFO)
            {
                if (dialogue)
                {
                    ESM::DialInfo info;
                    info.load (esm);

                    dialogue->mInfo.push_back (info);
                }
                else
                {
                    std::cerr << "error: info record without dialog" << std::endl;
                    esm.skipRecord();
                }
            }
            else if (n.val==ESM::REC_MGEF)
            {
                magicEffects.load (esm);
            }
            else if (n.val==ESM::REC_SKIL)
            {
                skills.load (esm);
            }
            else
            {
                // Not found (this would be an error later)
                esm.skipRecord();
                missing.insert(n.toString());
            }
        }
        else
        {
            // Load it
            std::string id = esm.getHNOString("NAME");
            it->second->load(esm, id);

            if (n.val==ESM::REC_DIAL)
            {
                RecListCaseT<ESM::Dialogue>& recList =
                    static_cast<RecListCaseT<ESM::Dialogue>& > (*it->second);

                ESM::Dialogue* d = recList.search (id);

                assert (d != NULL);

                dialogue = d;
            }
            else
                dialogue = 0;

            // Insert the reference into the global lookup
            if(!id.empty() &&
                (n.val==ESM::REC_ACTI || n.val==ESM::REC_ALCH || n.val==ESM::REC_APPA || n.val==ESM::REC_ARMO ||
                n.val==ESM::REC_BOOK || n.val==ESM::REC_CLOT || n.val==ESM::REC_CONT || n.val==ESM::REC_CREA ||
                n.val==ESM::REC_DOOR || n.val==ESM::REC_INGR || n.val==ESM::REC_LEVC || n.val==ESM::REC_LEVI ||
                n.val==ESM::REC_LIGH || n.val==ESM::REC_LOCK || n.val==ESM::REC_MISC || n.val==ESM::REC_NPC_ ||
                n.val==ESM::REC_PROB || n.val==ESM::REC_REPA || n.val==ESM::REC_STAT || n.val==ESM::REC_WEAP)
                )
                all[id] = n.val;
        }
    }

    for (int i = 0; i < ESM::Attribute::Length; ++i)
    {
        typedef ESM::Attribute EsmAttr;

        EsmAttr::AttributeID id = EsmAttr::sAttributeIds[i];
        attributes.list.insert(std::make_pair(id, ESM::Attribute(id, EsmAttr::sGmstAttributeIds[i], EsmAttr::sGmstAttributeDescIds[i])));
    }

  /* This information isn't needed on screen. But keep the code around
     for debugging purposes later.

  cout << "\n" << recLists.size() << " record types:\n";
  for(RecListList::iterator it = recLists.begin(); it != recLists.end(); it++)
    cout << "  " << toStr(it->first) << ": " << it->second->getSize() << endl;
  cout << "\nNot implemented yet: ";
  for(set<string>::iterator it = missing.begin();
      it != missing.end(); it++ )
    cout << *it << " ";
  cout << endl;
  */
}

} // end namespace
