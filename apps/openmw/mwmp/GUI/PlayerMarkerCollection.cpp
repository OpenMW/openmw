//
// Created by koncord on 30.09.16.
//

#include <stdexcept>
#include "PlayerMarkerCollection.hpp"


using namespace mwmp;

void PlayerMarkerCollection::addMarker(const ESM::CustomMarker &marker, bool triggerEvent)
{
    mMarkers.insert(std::make_pair(marker.mCell, marker));
    if (triggerEvent)
        eventMarkersChanged();
}

void PlayerMarkerCollection::deleteMarker(const ESM::CustomMarker &marker)
{
    std::pair<ContainerType::iterator, ContainerType::iterator> range = mMarkers.equal_range(marker.mCell);

    for (ContainerType::iterator it = range.first; it != range.second; ++it)
    {
        if (it->second == marker)
        {
            mMarkers.erase(it);
            eventMarkersChanged();
            return;
        }
    }
    throw std::runtime_error("can't find marker to delete");
}

void PlayerMarkerCollection::updateMarker(const ESM::CustomMarker &marker, const std::string &newNote)
{
    std::pair<ContainerType::iterator, ContainerType::iterator> range = mMarkers.equal_range(marker.mCell);

    for (ContainerType::iterator it = range.first; it != range.second; ++it)
    {
        if (it->second == marker)
        {
            it->second.mNote = newNote;
            eventMarkersChanged();
            return;
        }
    }
    throw std::runtime_error("can't find marker to update");
}

void PlayerMarkerCollection::clear()
{
    mMarkers.clear();
    eventMarkersChanged();
}

PlayerMarkerCollection::ContainerType::const_iterator PlayerMarkerCollection::begin() const
{
    return mMarkers.begin();
}

PlayerMarkerCollection::ContainerType::const_iterator PlayerMarkerCollection::end() const
{
    return mMarkers.end();
}

PlayerMarkerCollection::RangeType PlayerMarkerCollection::getMarkers(const ESM::CellId &cellId) const
{
    return mMarkers.equal_range(cellId);
}

size_t PlayerMarkerCollection::size() const
{
    return mMarkers.size();
}

bool PlayerMarkerCollection::contains(const ESM::CustomMarker &marker)
{
    std::pair<ContainerType::iterator, ContainerType::iterator> range = mMarkers.equal_range(marker.mCell);

    for (ContainerType::iterator it = range.first; it != range.second; ++it)
    {
        if (it->second == marker)
            return true;
    }

    return false;
}
