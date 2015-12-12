#include <osgDB/ObjectWrapper>
#include <osgDB/InputStream>
#include <osgDB/OutputStream>
#include <osgDB/Serializer>

// Enable debugging of all serializers
//#undef SERIALIZER_DEBUG
//#define SERIALIZER_DEBUG 3

#if SERIALIZER_DEBUG==0
#define SETUPMSG(CLASS) ;
#define CHECKMSG(MEMBER) ;
#define WRITEMSG(MEMBER) ;
#define READMSG(MEMBER) ;
#endif

#if SERIALIZER_DEBUG>=1
#include <iostream>
#define CLASSMSG node.libraryName() << "::" << node.className() << ".m"
#define SETUPMSG(CLASS) std::cout << "Setting up " << CLASS << " serializer..." << std::endl;
#endif

#if SERIALIZER_DEBUG>=2
#define CHECKMSG(MEMBER) std::cout << "Checking " << CLASSMSG << MEMBER << std::endl;
#define WRITEMSG(MEMBER) std::cout << "Writing " << CLASSMSG << MEMBER << std::endl;
#define READMSG(MEMBER) std::cout << "Reading " << CLASSMSG << MEMBER << std::endl;
#else
#define CHECKMSG(MEMBER) ;
#define WRITEMSG(MEMBER) ;
#define READMSG(MEMBER) ;
#endif

#if SERIALIZER_DEBUG>=3
#define WRITEVALUE std::cout << "Writing " << CLASSMSG
#define READVALUE std::cout << "Reading " << CLASSMSG
#endif
