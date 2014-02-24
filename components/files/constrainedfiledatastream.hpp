#ifndef COMPONENTS_FILES_CONSTRAINEDFILEDATASTREAM_HPP
#define COMPONENTS_FILES_CONSTRAINEDFILEDATASTREAM_HPP

#include <OgreDataStream.h>

Ogre::DataStreamPtr openConstrainedFileDataStream (char const * filename, size_t offset = 0, size_t length = 0xFFFFFFFF);

#endif // COMPONENTS_FILES_CONSTRAINEDFILEDATASTREAM_HPP
