#include <components/nifosg/controller.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(NifOsg_NifControllerFunction_Serializer,
                         new NifOsg::NifControllerFunction,
                         NifOsg::NifControllerFunction,
                         "OpenMW::NifControllerFunction",
                         "osg::Object OpenMW::ControllerFunction OpenMW::NifControllerFunction")
{
    SETUPMSG("OpenMW::NifControllerFunction");
    ADD_FLOAT_SERIALIZER(Frequency, 0.0f);
    ADD_FLOAT_SERIALIZER(Phase, 0.0f);
    ADD_FLOAT_SERIALIZER(StartTime, 0.0f);
    ADD_FLOAT_SERIALIZER(StopTime, 0.0f);
    {
        typedef osgDB::EnumSerializer<NifOsg::NifControllerFunction,
                                      NifOsg::NifControllerFunction::ExtrapolationMode, void> MySerializer;
        osg::ref_ptr<MySerializer> serializer = new MySerializer(
           "ExtrapolationMode", NifOsg::NifControllerFunction::Cycle,
           &NifOsg::NifControllerFunction::getExtrapolationMode,
           &NifOsg::NifControllerFunction::setExtrapolationMode);
        serializer->add("Cycle", NifOsg::NifControllerFunction::Cycle);
        serializer->add("Reverse", NifOsg::NifControllerFunction::Reverse);
        serializer->add("Constant", NifOsg::NifControllerFunction::Constant);
        wrapper->addSerializer(serializer.get(), osgDB::BaseSerializer::RW_ENUM);
    }
}
