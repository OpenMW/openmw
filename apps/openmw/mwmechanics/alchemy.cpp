
#include "alchemy.hpp"

#include <algorithm>
#include <stdexcept>

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"

void MWMechanics::Alchemy::setAlchemist (const MWWorld::Ptr& npc)
{
    mNpc = npc;
    
    mTools.resize (4);
    
    std::fill (mTools.begin(), mTools.end(), MWWorld::Ptr());
    
    MWWorld::ContainerStore& store = MWWorld::Class::get (npc).getContainerStore (npc);
    
    for (MWWorld::ContainerStoreIterator iter (store.begin (MWWorld::ContainerStore::Type_Apparatus));
        iter!=store.end(); ++iter)
    {    
        MWWorld::LiveCellRef<ESM::Apparatus>* ref = iter->get<ESM::Apparatus>();
    
        int type = ref->base->mData.mType;
    
        if (type<0 || type>=static_cast<int> (mTools.size()))
            throw std::runtime_error ("invalid apparatus type");
            
        if (!mTools[type].isEmpty())
            if (ref->base->mData.mQuality<=mTools[type].get<ESM::Apparatus>()->base->mData.mQuality)
                continue;
        
        mTools[type] = *iter;        
    }
}

MWMechanics::Alchemy::TToolsIterator MWMechanics::Alchemy::beginTools() const
{
    return mTools.begin();
}

MWMechanics::Alchemy::TToolsIterator MWMechanics::Alchemy::endTools() const
{
    return mTools.end();
}
