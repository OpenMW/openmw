# Designed for GNU Make

# Compiler settings
CXX?= g++
CXXFLAGS?= -Wall -g
DMD=gdmd -version=Posix

# Some extra flags for niftool and bsatool
NIFFLAGS=

# Compiler settings for Ogre + OIS. Change as needed.
OGCC=$(CXX) $(CXXFLAGS) `pkg-config --cflags OGRE OIS openal`

# Ogre C++ files, on the form ogre/cpp_X.cpp. Only the first file is
# passed to the compiler, the rest are dependencies.
ogre_cpp=ogre framelistener interface overlay bsaarchive

## The rest of this file is automatic ##

ogre_cpp_files=$(ogre_cpp:%=ogre/cpp_%.cpp)

d_files=$(wildcard */*.d) $(wildcard monster/util/*.d)
d_files_nif=$(wildcard nif/*.d) $(wildcard util/*.d) $(wildcard core/memory.d) $(wildcard monster/util/*.d)

# The NIF object files for niftool and bsatool are put in a separate
# directory, since they are built with different flags.
d_objs=$(d_files:%.d=objs/%.o)
d_objs_nif=$(d_files_nif:%.d=nifobjs/%.o)

.PHONY: cpp all clean makedirs

# By default, make will only build the Ogre C++ sources.
cpp: cpp_ogre.o

all: makedirs openmw esmtool niftool bsatool bored

cpp_ogre.o: $(ogre_cpp_files)
	$(OGCC) -c $<

objs/%.o: %.d
	$(DMD) -c $< -of$@

nifobjs/%.o: %.d
	$(DMD) -debug=warnstd -debug=check -debug=statecheck -debug=strict -debug=verbose -c $< -of$@

# This is a hack for gdmd (dmd-like frontend to gdc), since it does
# not automatically create directories as it should.
makedirs:
	mkdir -p objs/bsa
	mkdir -p objs/core
	mkdir -p objs/esm
	mkdir -p objs/input
	mkdir -p objs/monster/util
	mkdir -p objs/nif
	mkdir -p objs/ogre
	mkdir -p objs/scene
	mkdir -p objs/sound
	mkdir -p objs/util
	mkdir -p nifobjs/nif
	mkdir -p nifobjs/util
	mkdir -p nifobjs/core
	mkdir -p nifobjs/monster/util
	mkdir -p nifobjs/bsa

openmw: openmw.d cpp_ogre.o $(d_objs)
	$(DMD) $^ -of$@ -L-lalut -L-lopenal -L-lOgreMain -L-lOIS

esmtool: esmtool.d cpp_ogre.o $(d_objs)
	$(DMD) $^ -of$@ -L-lalut -L-lopenal -L-lOgreMain -L-lOIS

niftool: niftool.d $(d_objs_nif)
	$(DMD) $^ -of$@

bsatool: bsatool.d $(d_objs_nif) bsa/bsafile.d
	$(DMD) $^ -of$@

bored: bored.d
	$(DMD) $^

clean:
	-rm cpp_ogre.o
	-rm openmw esmtool niftool bsatool bored
	-rm -r objs/ nifobjs/ dsss_objs/
