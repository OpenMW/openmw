#include "LocalEvent.hpp"
#include "Networking.hpp"
#include "Main.hpp"

using namespace mwmp;
using namespace std;

LocalEvent::LocalEvent(RakNet::RakNetGUID guid)
{
    this->guid = guid;
}

LocalEvent::~LocalEvent()
{

}

Networking *LocalEvent::getNetworking()
{
    return mwmp::Main::get().getNetworking();
}

void LocalEvent::addCellRef(MWWorld::CellRef worldCellRef)
{
    cellRef.mRefID = worldCellRef.getRefId();
    cellRef.mRefNum = worldCellRef.getRefNum();
    cellRef.mGoldValue = worldCellRef.getGoldValue();
}

void LocalEvent::addRefId(std::string refId)
{
    cellRef.mRefID = refId;
}
