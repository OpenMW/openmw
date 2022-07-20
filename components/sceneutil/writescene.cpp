#include "writescene.hpp"

#include <stdexcept>
#include <fstream>

#include <osgDB/Registry>


#include "serialize.hpp"

void SceneUtil::writeScene(osg::Node *node, const std::string& filename, const std::string& format)
{
    registerSerializers();

    osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension("osgt");
    if (!rw)
        throw std::runtime_error("can not find readerwriter for " + format);

    std::ofstream stream;
    stream.open(filename);

    osg::ref_ptr<osgDB::Options> options = new osgDB::Options;
    options->setPluginStringData("fileType", format);
    options->setPluginStringData("WriteImageHint", "UseExternal");

    rw->writeNode(*node, stream, options);
}
