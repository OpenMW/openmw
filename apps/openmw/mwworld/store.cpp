#include "store.hpp"
#include "esmstore.hpp"

#include <components/esm/esmreader.hpp>

namespace MWWorld {

void Store<ESM::Cell>::handleMovedCellRefs(ESM::ESMReader& esm, ESM::Cell* cell)
{
    //Handling MovedCellRefs, there is no way to do it inside loadcell
    while (esm.isNextSub("MVRF")) {
        ESM::CellRef ref;
        ESM::MovedCellRef cMRef;
        cell->getNextMVRF(esm, cMRef);

        ESM::Cell *cellAlt = const_cast<ESM::Cell*>(searchOrCreate(cMRef.mTarget[0], cMRef.mTarget[1]));

        // Get regular moved reference data. Adapted from CellStore::loadRefs. Maybe we can optimize the following
        //  implementation when the oher implementation works as well.
        bool deleted = false;
        cell->getNextRef(esm, ref, deleted);

        // Add data required to make reference appear in the correct cell.
        // We should not need to test for duplicates, as this part of the code is pre-cell merge.
        cell->mMovedRefs.push_back(cMRef);
        // But there may be duplicates here!
        if (!deleted)
        {
            ESM::CellRefTracker::iterator iter = std::find(cellAlt->mLeasedRefs.begin(), cellAlt->mLeasedRefs.end(), ref.mRefNum);
            if (iter == cellAlt->mLeasedRefs.end())
              cellAlt->mLeasedRefs.push_back(ref);
            else
              *iter = ref;
        }
    }
}

void Store<ESM::Cell>::load(ESM::ESMReader &esm, const std::string &id)
{
    // Don't automatically assume that a new cell must be spawned. Multiple plugins write to the same cell,
    //  and we merge all this data into one Cell object. However, we can't simply search for the cell id,
    //  as many exterior cells do not have a name. Instead, we need to search by (x,y) coordinates - and they
    //  are not available until both cells have been loaded at least partially!

    // All cells have a name record, even nameless exterior cells.
    std::string idLower = Misc::StringUtils::lowerCase(id);
    ESM::Cell cell;
    cell.mName = id;

    // Load the (x,y) coordinates of the cell, if it is an exterior cell,
    // so we can find the cell we need to merge with
    cell.loadData(esm);

    if(cell.mData.mFlags & ESM::Cell::Interior)
    {
        // Store interior cell by name, try to merge with existing parent data.
        ESM::Cell *oldcell = const_cast<ESM::Cell*>(search(idLower));
        if (oldcell) {
            // merge new cell into old cell
            // push the new references on the list of references to manage (saveContext = true)
            oldcell->mData = cell.mData;
            oldcell->mName = cell.mName; // merge name just to be sure (ID will be the same, but case could have been changed)
            oldcell->loadCell(esm, true);
        } else
        {
            // spawn a new cell
            cell.loadCell(esm, true);

            mInt[idLower] = cell;
        }
    }
    else
    {
        // Store exterior cells by grid position, try to merge with existing parent data.
        ESM::Cell *oldcell = const_cast<ESM::Cell*>(search(cell.getGridX(), cell.getGridY()));
        if (oldcell) {
            // merge new cell into old cell
            oldcell->mData = cell.mData;
            oldcell->mName = cell.mName;
            oldcell->loadCell(esm, false);

            // handle moved ref (MVRF) subrecords
            handleMovedCellRefs (esm, &cell);

            // push the new references on the list of references to manage
            oldcell->postLoad(esm);

            // merge lists of leased references, use newer data in case of conflict
            for (ESM::MovedCellRefTracker::const_iterator it = cell.mMovedRefs.begin(); it != cell.mMovedRefs.end(); ++it) {
                // remove reference from current leased ref tracker and add it to new cell
                ESM::MovedCellRefTracker::iterator itold = std::find(oldcell->mMovedRefs.begin(), oldcell->mMovedRefs.end(), it->mRefNum);
                if (itold != oldcell->mMovedRefs.end()) {
                    ESM::MovedCellRef target0 = *itold;
                    ESM::Cell *wipecell = const_cast<ESM::Cell*>(search(target0.mTarget[0], target0.mTarget[1]));
                    ESM::CellRefTracker::iterator it_lease = std::find(wipecell->mLeasedRefs.begin(), wipecell->mLeasedRefs.end(), it->mRefNum);
                    wipecell->mLeasedRefs.erase(it_lease);
                    *itold = *it;
                }
                else
                    oldcell->mMovedRefs.push_back(*it);
            }

            // We don't need to merge mLeasedRefs of cell / oldcell. This list is filled when another cell moves a
            // reference to this cell, so the list for the new cell should be empty. The list for oldcell,
            // however, could have leased refs in it and so should be kept.
        } else
        {
            // spawn a new cell
            cell.loadCell(esm, false);

            // handle moved ref (MVRF) subrecords
            handleMovedCellRefs (esm, &cell);

            // push the new references on the list of references to manage
            cell.postLoad(esm);

            mExt[std::make_pair(cell.mData.mX, cell.mData.mY)] = cell;
        }
    }
}

void Store<ESM::LandTexture>::load(ESM::ESMReader &esm, const std::string &id)
{
    load(esm, id, esm.getIndex());
}

}
