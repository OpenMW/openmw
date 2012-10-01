#include <set>
#include <iostream>
#include "store.hpp"

using namespace std;
using namespace ESM;
using namespace ESMS;

/*
static string toStr(int i)
{
  char name[5];
  *((int*)name) = i;
  name[4] = 0;
  return std::string(name);
}
*/

void ESMStore::load(ESMReader &esm)
{
    set<string> missing;

    ESM::Dialogue *dialogue = 0;

    // Loop through all records
    while(esm.hasMoreRecs())
    {
        NAME n = esm.getRecName();
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
                RecListCaseT<Dialogue>& recList = static_cast<RecListCaseT<Dialogue>& > (*it->second);

                ESM::Dialogue* d = recList.search (id);

                assert (d != NULL);

                dialogue = d;
            }
            else
                dialogue = 0;

            // Insert the reference into the global lookup
            if(!id.empty() &&
                (n.val==REC_ACTI || n.val==REC_ALCH || n.val==REC_APPA || n.val==REC_ARMO ||
                n.val==REC_BOOK || n.val==REC_CLOT || n.val==REC_CONT || n.val==REC_CREA ||
                n.val==REC_DOOR || n.val==REC_INGR || n.val==REC_LEVC || n.val==REC_LEVI ||
                n.val==REC_LIGH || n.val==REC_LOCK || n.val==REC_MISC || n.val==REC_NPC_ ||
                n.val==REC_PROB || n.val==REC_REPA || n.val==REC_STAT || n.val==REC_WEAP)
                )
                all[id] = n.val;
        }
    }

    for (int i = 0; i < Attribute::Length; ++i)
    {
        Attribute::AttributeID id = Attribute::sAttributeIds[i];
        attributes.list.insert(std::make_pair(id, Attribute(id, Attribute::sGmstAttributeIds[i], Attribute::sGmstAttributeDescIds[i])));
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
