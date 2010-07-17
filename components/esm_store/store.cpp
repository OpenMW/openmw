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

  // Loop through all records
  while(esm.hasMoreRecs())
    {
      NAME n = esm.getRecName();
      esm.getRecHeader();

      // Look up the record type.
      RecListList::iterator it = recLists.find(n.val);

      if(it == recLists.end())
        {
          // Not found (this would be an error later)
          esm.skipRecord();
          missing.insert(n.toString());
          continue;
        }

      // Load it
      std::string id = esm.getHNOString("NAME");
      it->second->load(esm, id);

      // Insert the reference into the global lookup
      if(!id.empty())
        all[id] = n.val;
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
