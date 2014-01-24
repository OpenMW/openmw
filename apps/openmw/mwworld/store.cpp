#include "store.hpp"
#include "esmstore.hpp"

namespace MWWorld {


void Store<ESM::Cell>::load(ESM::ESMReader &esm, const std::string &id)
{
    // Don't automatically assume that a new cell must be spawned. Multiple plugins write to the same cell,
    //  and we merge all this data into one Cell object. However, we can't simply search for the cell id,
    //  as many exterior cells do not have a name. Instead, we need to search by (x,y) coordinates - and they
    //  are not available until both cells have been loaded! So first, proceed as usual.

    // All cells have a name record, even nameless exterior cells.
    std::string idLower = Misc::StringUtils::lowerCase(id);
    ESM::Cell *cell = new ESM::Cell;
    cell->mName = id;

    //First part of cell loading
    cell->preLoad(esm);

    //Handling MovedCellRefs, there is no way to do it inside loadcell
    while (esm.isNextSub("MVRF")) {
        ESM::CellRef ref;
        ESM::MovedCellRef cMRef;
        cell->getNextMVRF(esm, cMRef);

        MWWorld::Store<ESM::Cell> &cStore = const_cast<MWWorld::Store<ESM::Cell>&>(mEsmStore->get<ESM::Cell>());
        ESM::Cell *cellAlt = const_cast<ESM::Cell*>(cStore.searchOrCreate(cMRef.mTarget[0], cMRef.mTarget[1]));

        // Get regular moved reference data. Adapted from CellStore::loadRefs. Maybe we can optimize the following
        //  implementation when the oher implementation works as well.
        bool deleted = false;
        cell->getNextRef(esm, ref, deleted);

        // Add data required to make reference appear in the correct cell.
        // We should not need to test for duplicates, as this part of the code is pre-cell merge.
        cell->mMovedRefs.push_back(cMRef);
        // But there may be duplicates here!
        ESM::CellRefTracker::iterator iter = std::find(cellAlt->mLeasedRefs.begin(), cellAlt->mLeasedRefs.end(), ref.mRefNum);
        if (iter == cellAlt->mLeasedRefs.end())
          cellAlt->mLeasedRefs.push_back(ref);
        else
          *iter = ref;
    }

    //Second part of cell loading
    cell->postLoad(esm);

    if(cell->mData.mFlags & ESM::Cell::Interior)
    {
        // Store interior cell by name, try to merge with existing parent data.
        ESM::Cell *oldcell = const_cast<ESM::Cell*>(search(idLower));
        if (oldcell) {
            // push the new references on the list of references to manage
            oldcell->mContextList.push_back(cell->mContextList.at(0));
            // copy list into new cell
            cell->mContextList = oldcell->mContextList;
            // have new cell replace old cell
            *oldcell = *cell;
        } else
            mInt[idLower] = *cell;
    }
    else
    {
        // Store exterior cells by grid position, try to merge with existing parent data.
        ESM::Cell *oldcell = const_cast<ESM::Cell*>(search(cell->getGridX(), cell->getGridY()));
        if (oldcell) {
            // push the new references on the list of references to manage
            oldcell->mContextList.push_back(cell->mContextList.at(0));
            // copy list into new cell
            cell->mContextList = oldcell->mContextList;
            // merge lists of leased references, use newer data in case of conflict
            for (ESM::MovedCellRefTracker::const_iterator it = cell->mMovedRefs.begin(); it != cell->mMovedRefs.end(); ++it) {
                // remove reference from current leased ref tracker and add it to new cell
                ESM::MovedCellRefTracker::iterator itold = std::find(oldcell->mMovedRefs.begin(), oldcell->mMovedRefs.end(), it->mRefNum);
                if (itold != oldcell->mMovedRefs.end()) {
                    ESM::MovedCellRef target0 = *itold;
                    ESM::Cell *wipecell = const_cast<ESM::Cell*>(search(target0.mTarget[0], target0.mTarget[1]));
                    ESM::CellRefTracker::iterator it_lease = std::find(wipecell->mLeasedRefs.begin(), wipecell->mLeasedRefs.end(), it->mRefNum);
                    wipecell->mLeasedRefs.erase(it_lease);
                    *itold = *it;
                }
            }
            cell->mMovedRefs = oldcell->mMovedRefs;
            // have new cell replace old cell
            *oldcell = *cell;
        } else
            mExt[std::make_pair(cell->mData.mX, cell->mData.mY)] = *cell;
    }
    delete cell;
}

}
