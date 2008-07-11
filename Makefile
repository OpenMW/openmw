# Designed for GNU Make

# Compiler settings
CXX?= g++
CXXFLAGS?= -Wall -g

# Compiler settings for Ogre + OIS. Change as needed.
OGCC=$(CXX) $(CXXFLAGS) `pkg-config --cflags OGRE OIS`

# Compiler settings for Audiere
AGCC=$(CXX) $(CXXFLAGS) `audiere-config --cxxflags`

# Ogre C++ files, on the form ogre/cpp_X.cpp. Only the first file is
# passed to the compiler, the rest are dependencies.
ogre_cpp=ogre framelistener interface overlay bsaarchive

# Audiere C++ files, on the form sound/cpp_X.cpp.
audiere_cpp=audiere

## The rest of this file is automatic ##
ogre_cpp_files=$(ogre_cpp:%=ogre/cpp_%.cpp)
audiere_cpp_files=$(audiere_cpp:%=sound/cpp_%.cpp)

all: cpp_ogre.o cpp_audiere.o

cpp_ogre.o: $(ogre_cpp_files)
	$(OGCC) -c $<

cpp_audiere.o: $(audiere_cpp_files)
	$(AGCC) -c $<
