#include "customdata.hpp"

#include <stdexcept>
#include <sstream>
#include <typeinfo>

namespace MWWorld
{

MWClass::CreatureCustomData &CustomData::asCreatureCustomData()
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to CreatureCustomData";
    throw std::logic_error(error.str());
}

const MWClass::CreatureCustomData &CustomData::asCreatureCustomData() const
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to CreatureCustomData";
    throw std::logic_error(error.str());
}

MWClass::NpcCustomData &CustomData::asNpcCustomData()
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to NpcCustomData";
    throw std::logic_error(error.str());
}

const MWClass::NpcCustomData &CustomData::asNpcCustomData() const
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to NpcCustomData";
    throw std::logic_error(error.str());
}

MWClass::ContainerCustomData &CustomData::asContainerCustomData()
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to ContainerCustomData";
    throw std::logic_error(error.str());
}

const MWClass::ContainerCustomData &CustomData::asContainerCustomData() const
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to ContainerCustomData";
    throw std::logic_error(error.str());
}

MWClass::DoorCustomData &CustomData::asDoorCustomData()
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to DoorCustomData";
    throw std::logic_error(error.str());
}

const MWClass::DoorCustomData &CustomData::asDoorCustomData() const
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to DoorCustomData";
    throw std::logic_error(error.str());
}

MWClass::CreatureLevListCustomData &CustomData::asCreatureLevListCustomData()
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to CreatureLevListCustomData";
    throw std::logic_error(error.str());
}

const MWClass::CreatureLevListCustomData &CustomData::asCreatureLevListCustomData() const
{
    std::stringstream error;
    error << "bad cast " << typeid(this).name() << " to CreatureLevListCustomData";
    throw std::logic_error(error.str());
}


}
